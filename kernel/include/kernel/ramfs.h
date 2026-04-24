#ifndef RAMFS_H
#define RAMFS_H

#include <drivers/types.h>

/*
 * RAM-resident filesystem. FAT-style block chain with a fixed inode pool.
 * Survives until power-off - no disk, no persistence. All operations return
 * 0 on success and a negative error code on failure.
 */

#define RAMFS_OK             0
#define RAMFS_ERR_NOT_FOUND  (-1)
#define RAMFS_ERR_EXISTS     (-2)
#define RAMFS_ERR_NO_SPACE   (-3)
#define RAMFS_ERR_NO_INODES  (-4)
#define RAMFS_ERR_NO_FDS     (-5)
#define RAMFS_ERR_BAD_FD     (-6)
#define RAMFS_ERR_BAD_PATH   (-7)
#define RAMFS_ERR_NOT_DIR    (-8)
#define RAMFS_ERR_IS_DIR     (-9)
#define RAMFS_ERR_NOT_EMPTY  (-10)
#define RAMFS_ERR_NAME_LONG  (-11)
#define RAMFS_ERR_IO         (-12)

#define RAMFS_MAX_NAME       28
#define RAMFS_MAX_PATH       256

/* Open flags (bitmask). */
#define RAMFS_O_READ         0x01
#define RAMFS_O_WRITE        0x02
#define RAMFS_O_CREATE       0x04
#define RAMFS_O_TRUNC        0x08
#define RAMFS_O_APPEND       0x10

/* File types. */
#define RAMFS_T_FREE         0
#define RAMFS_T_FILE         1
#define RAMFS_T_DIR          2

typedef int ramfs_fd;

typedef struct {
    char name[RAMFS_MAX_NAME];
    u8   type;   /* RAMFS_T_FILE or RAMFS_T_DIR */
    u32  size;   /* bytes for files, entry bytes for dirs */
} ramfs_stat_t;

/* Lifecycle. Call once at boot. */
void ramfs_init(void);

/* Namespace operations. All paths must be absolute (start with '/'). */
int ramfs_mkdir (const char *path);
int ramfs_rmdir (const char *path);
int ramfs_create(const char *path);
int ramfs_unlink(const char *path);
int ramfs_stat  (const char *path, ramfs_stat_t *out);
int ramfs_exists(const char *path);

/* File descriptor operations. */
ramfs_fd ramfs_open (const char *path, int flags);
int      ramfs_read (ramfs_fd fd, void *buf, u32 count);
int      ramfs_write(ramfs_fd fd, const void *buf, u32 count);
int      ramfs_seek (ramfs_fd fd, u32 offset);
u32      ramfs_tell (ramfs_fd fd);
int      ramfs_close(ramfs_fd fd);

/*
 * Directory listing. `out` receives up to `max` entries; the return value is
 * the number of entries actually written, or a negative error code.
 */
int ramfs_list(const char *path, ramfs_stat_t *out, u32 max);

/* Space accounting - useful for a `df` command. */
void ramfs_usage(u32 *blocks_used, u32 *blocks_total,
                 u32 *inodes_used, u32 *inodes_total);

#endif
