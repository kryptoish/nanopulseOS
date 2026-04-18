#include <kernel/ramfs.h>
#include <drivers/types.h>
#include <string.h>

/*
 * RAM filesystem internals.
 *
 * Layout (all in kernel .bss — sized at compile time, zero malloc):
 *
 *   data_blocks[RAMFS_BLOCKS][RAMFS_BLOCK_SIZE]  raw storage
 *   fat[RAMFS_BLOCKS]                            next-block table (FAT-style)
 *   block_bitmap[BLOCKS/8]                       1 = allocated
 *   inodes[MAX_INODES]                           inode table (0 = root)
 *   inode_bitmap[MAX_INODES/8]                   1 = allocated
 *   fds[MAX_FDS]                                 open-file table
 *
 * Invariants:
 *   - inode 0 is always the root directory and always allocated.
 *   - fat[i] is valid only when block_bitmap bit i is set. Otherwise fat[i]
 *     is garbage and must not be followed.
 *   - A file/dir's size is authoritative for reads; the block chain may be
 *     longer if we over-allocated, but only `size` bytes are readable.
 *   - Dirents with inode == RAMFS_SLOT_FREE are holes to be reused.
 */

#define RAMFS_MAGIC        0x4E414E4FU  /* 'NANO' */
#define RAMFS_BLOCK_SIZE   512
#define RAMFS_BLOCKS       512          /* 256 KB of data */
#define RAMFS_MAX_INODES   64
#define RAMFS_MAX_FDS      16
#define RAMFS_END          0xFFFFFFFFU
#define RAMFS_SLOT_FREE    0xFFFFFFFFU

/* Compile-time sanity: dirent must pack cleanly into a block. */
typedef struct {
    char name[RAMFS_MAX_NAME];   /* 28 bytes, always null-terminated */
    u32  inode;                   /* 4  — RAMFS_SLOT_FREE marks a hole */
} dirent_t;                      /* 32 bytes → 16 per block */

typedef struct {
    u8  type;           /* RAMFS_T_* */
    u8  _pad[3];
    u32 size;           /* bytes stored */
    u32 first_block;    /* RAMFS_END if empty */
} inode_t;

typedef struct {
    int  in_use;
    u32  inode;
    u32  offset;
    int  flags;
} fd_entry_t;

static u8         data_blocks[RAMFS_BLOCKS][RAMFS_BLOCK_SIZE];
static u32        fat[RAMFS_BLOCKS];
static u8         block_bitmap[(RAMFS_BLOCKS + 7) / 8];
static inode_t    inodes[RAMFS_MAX_INODES];
static u8         inode_bitmap[(RAMFS_MAX_INODES + 7) / 8];
static fd_entry_t fds[RAMFS_MAX_FDS];
static u32        magic = 0;

static const u32 ROOT_INODE = 0;

/* ------------------------------------------------------------------------ */
/* Bitmap helpers                                                           */
/* ------------------------------------------------------------------------ */

static int bit_get(const u8 *bm, u32 i) { return (bm[i >> 3] >> (i & 7)) & 1; }
static void bit_set(u8 *bm, u32 i)       { bm[i >> 3] |=  (u8)(1u << (i & 7)); }
static void bit_clear(u8 *bm, u32 i)     { bm[i >> 3] &= (u8)~(1u << (i & 7)); }

/* ------------------------------------------------------------------------ */
/* Block allocator                                                          */
/* ------------------------------------------------------------------------ */

static int block_alloc(u32 *out) {
    for (u32 i = 0; i < RAMFS_BLOCKS; i++) {
        if (!bit_get(block_bitmap, i)) {
            bit_set(block_bitmap, i);
            fat[i] = RAMFS_END;
            memset(data_blocks[i], 0, RAMFS_BLOCK_SIZE);
            *out = i;
            return RAMFS_OK;
        }
    }
    return RAMFS_ERR_NO_SPACE;
}

static void chain_free(u32 head) {
    u32 cur = head;
    while (cur != RAMFS_END && cur < RAMFS_BLOCKS) {
        u32 next = fat[cur];
        bit_clear(block_bitmap, cur);
        fat[cur] = RAMFS_END;
        cur = next;
    }
}

/* ------------------------------------------------------------------------ */
/* Inode allocator                                                          */
/* ------------------------------------------------------------------------ */

static int inode_alloc(u8 type, u32 *out) {
    for (u32 i = 0; i < RAMFS_MAX_INODES; i++) {
        if (!bit_get(inode_bitmap, i)) {
            bit_set(inode_bitmap, i);
            inodes[i].type = type;
            inodes[i].size = 0;
            inodes[i].first_block = RAMFS_END;
            *out = i;
            return RAMFS_OK;
        }
    }
    return RAMFS_ERR_NO_INODES;
}

static void inode_free(u32 idx) {
    if (idx >= RAMFS_MAX_INODES) return;
    chain_free(inodes[idx].first_block);
    inodes[idx].type = RAMFS_T_FREE;
    inodes[idx].size = 0;
    inodes[idx].first_block = RAMFS_END;
    bit_clear(inode_bitmap, idx);
}

static int inode_valid(u32 idx) {
    return idx < RAMFS_MAX_INODES &&
           bit_get(inode_bitmap, idx) &&
           inodes[idx].type != RAMFS_T_FREE;
}

/* ------------------------------------------------------------------------ */
/* Block-chain I/O                                                          */
/* ------------------------------------------------------------------------ */

/*
 * Read up to `count` bytes starting at `offset` from a block chain of length
 * `size` bytes. Returns bytes read (possibly 0) or a negative error.
 */
static int chain_read(u32 first_block, u32 offset, u32 size,
                      void *buf, u32 count)
{
    if (offset >= size) return 0;
    if (offset + count > size) count = size - offset;

    u32 block_idx = offset / RAMFS_BLOCK_SIZE;
    u32 block_off = offset % RAMFS_BLOCK_SIZE;

    u32 cur = first_block;
    for (u32 i = 0; i < block_idx; i++) {
        if (cur == RAMFS_END || cur >= RAMFS_BLOCKS) return RAMFS_ERR_IO;
        cur = fat[cur];
    }

    u32 total = 0;
    u8 *out = (u8 *)buf;
    while (count > 0) {
        if (cur == RAMFS_END || cur >= RAMFS_BLOCKS) return RAMFS_ERR_IO;
        u32 chunk = RAMFS_BLOCK_SIZE - block_off;
        if (chunk > count) chunk = count;
        memcpy(out + total, data_blocks[cur] + block_off, chunk);
        total += chunk;
        count -= chunk;
        block_off = 0;
        cur = fat[cur];
    }
    return (int)total;
}

/*
 * Write `count` bytes to the inode's chain at `offset`, extending as needed.
 * Updates inode->size. Returns bytes written (may be less than count if we
 * run out of blocks) or a negative error.
 */
static int chain_write(inode_t *inode, u32 offset, const void *buf, u32 count) {
    u32 block_idx = offset / RAMFS_BLOCK_SIZE;
    u32 block_off = offset % RAMFS_BLOCK_SIZE;

    /* Walk (allocating any missing intermediate blocks) to the starting
     * block. `link` always points at the slot we'd write a new block index
     * into if we must allocate — either inode->first_block or the FAT cell
     * of the block we just traversed. */
    u32 *link = &inode->first_block;
    u32 cur = inode->first_block;
    for (u32 i = 0; i < block_idx; i++) {
        if (cur == RAMFS_END) {
            u32 newb;
            if (block_alloc(&newb) != RAMFS_OK) return RAMFS_ERR_NO_SPACE;
            *link = newb;
            cur = newb;
        }
        if (cur >= RAMFS_BLOCKS) return RAMFS_ERR_IO;
        link = &fat[cur];
        cur = fat[cur];
    }

    u32 total = 0;
    const u8 *in = (const u8 *)buf;
    while (count > 0) {
        if (cur == RAMFS_END) {
            u32 newb;
            if (block_alloc(&newb) != RAMFS_OK) break;
            *link = newb;
            cur = newb;
        }
        if (cur >= RAMFS_BLOCKS) return RAMFS_ERR_IO;

        u32 chunk = RAMFS_BLOCK_SIZE - block_off;
        if (chunk > count) chunk = count;
        memcpy(data_blocks[cur] + block_off, in + total, chunk);
        total += chunk;
        count -= chunk;
        block_off = 0;
        link = &fat[cur];
        cur = fat[cur];
    }

    if (offset + total > inode->size) inode->size = offset + total;
    return (int)total;
}

/* Shrink/empty an inode: release all blocks, size -> 0. */
static void chain_truncate(inode_t *inode) {
    chain_free(inode->first_block);
    inode->first_block = RAMFS_END;
    inode->size = 0;
}

/* ------------------------------------------------------------------------ */
/* Directory helpers                                                        */
/* ------------------------------------------------------------------------ */

static int dir_find(u32 dir_idx, const char *name, u32 *child_idx) {
    if (!inode_valid(dir_idx) || inodes[dir_idx].type != RAMFS_T_DIR)
        return RAMFS_ERR_NOT_DIR;

    inode_t *dir = &inodes[dir_idx];
    u32 n = dir->size / sizeof(dirent_t);
    for (u32 i = 0; i < n; i++) {
        dirent_t e;
        int r = chain_read(dir->first_block, i * sizeof(dirent_t),
                           dir->size, &e, sizeof(e));
        if (r != (int)sizeof(e)) return RAMFS_ERR_IO;
        if (e.inode == RAMFS_SLOT_FREE) continue;
        if (strcmp(e.name, (char *)name) == 0) {
            *child_idx = e.inode;
            return RAMFS_OK;
        }
    }
    return RAMFS_ERR_NOT_FOUND;
}

static void name_copy(char *dst, const char *src, u32 cap) {
    u32 i = 0;
    while (i + 1 < cap && src[i]) { dst[i] = src[i]; i++; }
    while (i < cap) { dst[i++] = '\0'; }
}

static int dir_add(u32 dir_idx, const char *name, u32 child_idx) {
    if (strlen(name) >= RAMFS_MAX_NAME) return RAMFS_ERR_NAME_LONG;
    if (!inode_valid(dir_idx) || inodes[dir_idx].type != RAMFS_T_DIR)
        return RAMFS_ERR_NOT_DIR;

    u32 existing;
    if (dir_find(dir_idx, name, &existing) == RAMFS_OK)
        return RAMFS_ERR_EXISTS;

    inode_t *dir = &inodes[dir_idx];

    dirent_t e;
    memset(&e, 0, sizeof(e));
    name_copy(e.name, name, RAMFS_MAX_NAME);
    e.inode = child_idx;

    /* Prefer a free slot (hole) over appending. */
    u32 n = dir->size / sizeof(dirent_t);
    for (u32 i = 0; i < n; i++) {
        dirent_t scan;
        if (chain_read(dir->first_block, i * sizeof(dirent_t),
                       dir->size, &scan, sizeof(scan)) != (int)sizeof(scan))
            return RAMFS_ERR_IO;
        if (scan.inode == RAMFS_SLOT_FREE) {
            int w = chain_write(dir, i * sizeof(dirent_t), &e, sizeof(e));
            return (w == (int)sizeof(e)) ? RAMFS_OK : RAMFS_ERR_NO_SPACE;
        }
    }

    int w = chain_write(dir, dir->size, &e, sizeof(e));
    return (w == (int)sizeof(e)) ? RAMFS_OK : RAMFS_ERR_NO_SPACE;
}

static int dir_remove(u32 dir_idx, const char *name) {
    if (!inode_valid(dir_idx) || inodes[dir_idx].type != RAMFS_T_DIR)
        return RAMFS_ERR_NOT_DIR;

    inode_t *dir = &inodes[dir_idx];
    u32 n = dir->size / sizeof(dirent_t);
    for (u32 i = 0; i < n; i++) {
        dirent_t e;
        if (chain_read(dir->first_block, i * sizeof(dirent_t),
                       dir->size, &e, sizeof(e)) != (int)sizeof(e))
            return RAMFS_ERR_IO;
        if (e.inode == RAMFS_SLOT_FREE) continue;
        if (strcmp(e.name, (char *)name) == 0) {
            memset(&e, 0, sizeof(e));
            e.inode = RAMFS_SLOT_FREE;
            if (chain_write(dir, i * sizeof(dirent_t), &e, sizeof(e))
                != (int)sizeof(e))
                return RAMFS_ERR_IO;
            return RAMFS_OK;
        }
    }
    return RAMFS_ERR_NOT_FOUND;
}

static int dir_is_empty(u32 dir_idx) {
    if (!inode_valid(dir_idx) || inodes[dir_idx].type != RAMFS_T_DIR)
        return RAMFS_ERR_NOT_DIR;
    inode_t *dir = &inodes[dir_idx];
    u32 n = dir->size / sizeof(dirent_t);
    for (u32 i = 0; i < n; i++) {
        dirent_t e;
        if (chain_read(dir->first_block, i * sizeof(dirent_t),
                       dir->size, &e, sizeof(e)) != (int)sizeof(e))
            return RAMFS_ERR_IO;
        if (e.inode != RAMFS_SLOT_FREE) return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------------ */
/* Path parsing                                                             */
/* ------------------------------------------------------------------------ */

/*
 * Tokenize an absolute path. On each call writes the next '/'-separated
 * component into `out` (null-terminated) and advances `*cursor`. Returns 1
 * on success, 0 at end of path, negative on error (empty segment, too long).
 */
static int path_next(const char **cursor, char *out, u32 cap) {
    const char *p = *cursor;
    while (*p == '/') p++;
    if (*p == '\0') { *cursor = p; return 0; }

    u32 i = 0;
    while (*p && *p != '/') {
        if (i + 1 >= cap) return RAMFS_ERR_NAME_LONG;
        out[i++] = *p++;
    }
    out[i] = '\0';
    *cursor = p;
    return 1;
}

/*
 * Resolve an absolute path to an inode index. Returns RAMFS_OK and sets
 * *out on success.
 */
static int path_resolve(const char *path, u32 *out) {
    if (!path || path[0] != '/') return RAMFS_ERR_BAD_PATH;

    u32 cur = ROOT_INODE;
    const char *cursor = path;
    char seg[RAMFS_MAX_NAME];
    int r;
    while ((r = path_next(&cursor, seg, sizeof(seg))) > 0) {
        if (!inode_valid(cur) || inodes[cur].type != RAMFS_T_DIR)
            return RAMFS_ERR_NOT_DIR;
        u32 child;
        int lr = dir_find(cur, seg, &child);
        if (lr != RAMFS_OK) return lr;
        cur = child;
    }
    if (r < 0) return r;
    *out = cur;
    return RAMFS_OK;
}

/*
 * Split a path into (parent inode, leaf name). The parent must already
 * exist and be a directory. The leaf need not exist.
 */
static int path_resolve_parent(const char *path,
                               u32 *parent_out, char *leaf_out)
{
    if (!path || path[0] != '/') return RAMFS_ERR_BAD_PATH;
    u32 len = strlen((char *)path);

    /* Strip trailing slashes (but never strip the leading one). */
    while (len > 1 && path[len - 1] == '/') len--;
    if (len == 1) return RAMFS_ERR_BAD_PATH; /* can't split root */

    /* Find last '/' in the trimmed region. */
    int last_slash = -1;
    for (int i = (int)len - 1; i >= 0; i--) {
        if (path[i] == '/') { last_slash = i; break; }
    }
    if (last_slash < 0) return RAMFS_ERR_BAD_PATH;

    u32 name_len = len - (u32)last_slash - 1;
    if (name_len == 0)          return RAMFS_ERR_BAD_PATH;
    if (name_len >= RAMFS_MAX_NAME) return RAMFS_ERR_NAME_LONG;

    memcpy(leaf_out, path + last_slash + 1, name_len);
    leaf_out[name_len] = '\0';

    if (last_slash == 0) { *parent_out = ROOT_INODE; return RAMFS_OK; }

    char parent_buf[RAMFS_MAX_PATH];
    if ((u32)last_slash >= sizeof(parent_buf))
        return RAMFS_ERR_BAD_PATH;
    memcpy(parent_buf, path, (u32)last_slash);
    parent_buf[last_slash] = '\0';

    return path_resolve(parent_buf, parent_out);
}

/* ------------------------------------------------------------------------ */
/* Public API                                                               */
/* ------------------------------------------------------------------------ */

void ramfs_init(void) {
    memset(block_bitmap, 0, sizeof(block_bitmap));
    memset(inode_bitmap, 0, sizeof(inode_bitmap));
    for (u32 i = 0; i < RAMFS_BLOCKS; i++) fat[i] = RAMFS_END;
    for (u32 i = 0; i < RAMFS_MAX_INODES; i++) {
        inodes[i].type = RAMFS_T_FREE;
        inodes[i].size = 0;
        inodes[i].first_block = RAMFS_END;
    }
    for (u32 i = 0; i < RAMFS_MAX_FDS; i++) fds[i].in_use = 0;

    /* Reserve inode 0 as the root directory. */
    bit_set(inode_bitmap, ROOT_INODE);
    inodes[ROOT_INODE].type = RAMFS_T_DIR;
    inodes[ROOT_INODE].size = 0;
    inodes[ROOT_INODE].first_block = RAMFS_END;

    magic = RAMFS_MAGIC;
}

static int check_init(void) {
    return magic == RAMFS_MAGIC ? RAMFS_OK : RAMFS_ERR_IO;
}

int ramfs_mkdir(const char *path) {
    if (check_init() != RAMFS_OK) return RAMFS_ERR_IO;

    u32 parent;
    char leaf[RAMFS_MAX_NAME];
    int r = path_resolve_parent(path, &parent, leaf);
    if (r != RAMFS_OK) return r;

    u32 dummy;
    if (dir_find(parent, leaf, &dummy) == RAMFS_OK) return RAMFS_ERR_EXISTS;

    u32 new_inode;
    r = inode_alloc(RAMFS_T_DIR, &new_inode);
    if (r != RAMFS_OK) return r;

    r = dir_add(parent, leaf, new_inode);
    if (r != RAMFS_OK) { inode_free(new_inode); return r; }

    return RAMFS_OK;
}

int ramfs_rmdir(const char *path) {
    if (check_init() != RAMFS_OK) return RAMFS_ERR_IO;

    u32 parent;
    char leaf[RAMFS_MAX_NAME];
    int r = path_resolve_parent(path, &parent, leaf);
    if (r != RAMFS_OK) return r;

    u32 target;
    r = dir_find(parent, leaf, &target);
    if (r != RAMFS_OK) return r;

    if (inodes[target].type != RAMFS_T_DIR) return RAMFS_ERR_NOT_DIR;
    int empty = dir_is_empty(target);
    if (empty < 0) return empty;
    if (!empty) return RAMFS_ERR_NOT_EMPTY;

    r = dir_remove(parent, leaf);
    if (r != RAMFS_OK) return r;
    inode_free(target);
    return RAMFS_OK;
}

int ramfs_create(const char *path) {
    if (check_init() != RAMFS_OK) return RAMFS_ERR_IO;

    u32 parent;
    char leaf[RAMFS_MAX_NAME];
    int r = path_resolve_parent(path, &parent, leaf);
    if (r != RAMFS_OK) return r;

    u32 dummy;
    if (dir_find(parent, leaf, &dummy) == RAMFS_OK) return RAMFS_ERR_EXISTS;

    u32 new_inode;
    r = inode_alloc(RAMFS_T_FILE, &new_inode);
    if (r != RAMFS_OK) return r;

    r = dir_add(parent, leaf, new_inode);
    if (r != RAMFS_OK) { inode_free(new_inode); return r; }
    return RAMFS_OK;
}

int ramfs_unlink(const char *path) {
    if (check_init() != RAMFS_OK) return RAMFS_ERR_IO;

    u32 parent;
    char leaf[RAMFS_MAX_NAME];
    int r = path_resolve_parent(path, &parent, leaf);
    if (r != RAMFS_OK) return r;

    u32 target;
    r = dir_find(parent, leaf, &target);
    if (r != RAMFS_OK) return r;

    if (inodes[target].type == RAMFS_T_DIR) return RAMFS_ERR_IS_DIR;

    /* Refuse to unlink a file with open descriptors to avoid use-after-free. */
    for (u32 i = 0; i < RAMFS_MAX_FDS; i++) {
        if (fds[i].in_use && fds[i].inode == target) return RAMFS_ERR_IO;
    }

    r = dir_remove(parent, leaf);
    if (r != RAMFS_OK) return r;
    inode_free(target);
    return RAMFS_OK;
}

int ramfs_stat(const char *path, ramfs_stat_t *out) {
    if (check_init() != RAMFS_OK) return RAMFS_ERR_IO;
    if (!out) return RAMFS_ERR_BAD_PATH;

    u32 idx;
    int r = path_resolve(path, &idx);
    if (r != RAMFS_OK) return r;

    /* Derive display name from the path itself (last component). */
    u32 len = strlen((char *)path);
    while (len > 1 && path[len - 1] == '/') len--;
    int last = -1;
    for (int i = (int)len - 1; i >= 0; i--) if (path[i] == '/') { last = i; break; }

    if (last < 0 || len == 1) {
        out->name[0] = '/'; out->name[1] = '\0';
    } else {
        u32 nl = len - (u32)last - 1;
        if (nl >= RAMFS_MAX_NAME) nl = RAMFS_MAX_NAME - 1;
        memcpy(out->name, path + last + 1, nl);
        out->name[nl] = '\0';
    }
    out->type = inodes[idx].type;
    out->size = inodes[idx].size;
    return RAMFS_OK;
}

int ramfs_exists(const char *path) {
    if (check_init() != RAMFS_OK) return 0;
    u32 idx;
    return path_resolve(path, &idx) == RAMFS_OK ? 1 : 0;
}

/* ------------------------------------------------------------------------ */
/* FD layer                                                                 */
/* ------------------------------------------------------------------------ */

static int fd_alloc(void) {
    for (int i = 0; i < RAMFS_MAX_FDS; i++) {
        if (!fds[i].in_use) return i;
    }
    return RAMFS_ERR_NO_FDS;
}

static int fd_check(ramfs_fd fd) {
    if (fd < 0 || fd >= RAMFS_MAX_FDS) return RAMFS_ERR_BAD_FD;
    if (!fds[fd].in_use) return RAMFS_ERR_BAD_FD;
    return RAMFS_OK;
}

ramfs_fd ramfs_open(const char *path, int flags) {
    if (check_init() != RAMFS_OK) return RAMFS_ERR_IO;

    u32 target;
    int r = path_resolve(path, &target);

    if (r == RAMFS_ERR_NOT_FOUND && (flags & RAMFS_O_CREATE)) {
        r = ramfs_create(path);
        if (r != RAMFS_OK) return r;
        r = path_resolve(path, &target);
    }
    if (r != RAMFS_OK) return r;
    if (inodes[target].type == RAMFS_T_DIR) return RAMFS_ERR_IS_DIR;

    int slot = fd_alloc();
    if (slot < 0) return slot;

    if (flags & RAMFS_O_TRUNC) chain_truncate(&inodes[target]);

    fds[slot].in_use = 1;
    fds[slot].inode  = target;
    fds[slot].offset = (flags & RAMFS_O_APPEND) ? inodes[target].size : 0;
    fds[slot].flags  = flags;
    return slot;
}

int ramfs_read(ramfs_fd fd, void *buf, u32 count) {
    int r = fd_check(fd);
    if (r != RAMFS_OK) return r;
    if (!(fds[fd].flags & RAMFS_O_READ)) return RAMFS_ERR_BAD_FD;

    inode_t *in = &inodes[fds[fd].inode];
    int n = chain_read(in->first_block, fds[fd].offset, in->size, buf, count);
    if (n < 0) return n;
    fds[fd].offset += (u32)n;
    return n;
}

int ramfs_write(ramfs_fd fd, const void *buf, u32 count) {
    int r = fd_check(fd);
    if (r != RAMFS_OK) return r;
    if (!(fds[fd].flags & RAMFS_O_WRITE)) return RAMFS_ERR_BAD_FD;

    inode_t *in = &inodes[fds[fd].inode];
    if (fds[fd].flags & RAMFS_O_APPEND) fds[fd].offset = in->size;

    int n = chain_write(in, fds[fd].offset, buf, count);
    if (n < 0) return n;
    fds[fd].offset += (u32)n;
    return n;
}

int ramfs_seek(ramfs_fd fd, u32 offset) {
    int r = fd_check(fd);
    if (r != RAMFS_OK) return r;
    fds[fd].offset = offset;
    return RAMFS_OK;
}

u32 ramfs_tell(ramfs_fd fd) {
    if (fd_check(fd) != RAMFS_OK) return 0;
    return fds[fd].offset;
}

int ramfs_close(ramfs_fd fd) {
    int r = fd_check(fd);
    if (r != RAMFS_OK) return r;
    fds[fd].in_use = 0;
    fds[fd].inode  = 0;
    fds[fd].offset = 0;
    fds[fd].flags  = 0;
    return RAMFS_OK;
}

/* ------------------------------------------------------------------------ */
/* Listing and accounting                                                   */
/* ------------------------------------------------------------------------ */

int ramfs_list(const char *path, ramfs_stat_t *out, u32 max) {
    if (check_init() != RAMFS_OK) return RAMFS_ERR_IO;

    u32 idx;
    int r = path_resolve(path, &idx);
    if (r != RAMFS_OK) return r;
    if (inodes[idx].type != RAMFS_T_DIR) return RAMFS_ERR_NOT_DIR;

    inode_t *dir = &inodes[idx];
    u32 n_entries = dir->size / sizeof(dirent_t);
    u32 written = 0;
    for (u32 i = 0; i < n_entries && written < max; i++) {
        dirent_t e;
        if (chain_read(dir->first_block, i * sizeof(dirent_t),
                       dir->size, &e, sizeof(e)) != (int)sizeof(e))
            return RAMFS_ERR_IO;
        if (e.inode == RAMFS_SLOT_FREE) continue;
        if (!inode_valid(e.inode)) continue;

        name_copy(out[written].name, e.name, RAMFS_MAX_NAME);
        out[written].type = inodes[e.inode].type;
        out[written].size = inodes[e.inode].size;
        written++;
    }
    return (int)written;
}

void ramfs_usage(u32 *bu, u32 *bt, u32 *iu, u32 *it) {
    u32 b = 0, n = 0;
    for (u32 i = 0; i < RAMFS_BLOCKS;     i++) if (bit_get(block_bitmap, i)) b++;
    for (u32 i = 0; i < RAMFS_MAX_INODES; i++) if (bit_get(inode_bitmap, i)) n++;
    if (bu) *bu = b;
    if (bt) *bt = RAMFS_BLOCKS;
    if (iu) *iu = n;
    if (it) *it = RAMFS_MAX_INODES;
}
