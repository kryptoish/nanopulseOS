#include <string.h>
#include <stdio.h>
#include <drivers/screen.h>
#include <drivers/types.h>
#include <kernel/art.h>
#include <kernel/portal.h>
#include <kernel/gamble.h>
#include <kernel/tetris.h>
#include <kernel/ramfs.h>

/* From libc/stdio/itoa.c - no header declares it yet, so forward-declare. */
void int_to_ascii(int n, char str[]);

// Small formatting helpers
static void print_u32(u32 v) {
    char buf[16];
    int_to_ascii((int)v, buf);
    kprint(buf);
}

static const char *fs_err_msg(int err) {
    switch (err) {
        case RAMFS_ERR_NOT_FOUND: return "not found";
        case RAMFS_ERR_EXISTS:    return "already exists";
        case RAMFS_ERR_NO_SPACE:  return "no space left";
        case RAMFS_ERR_NO_INODES: return "inode table full";
        case RAMFS_ERR_NO_FDS:    return "too many open files";
        case RAMFS_ERR_BAD_FD:    return "bad file descriptor";
        case RAMFS_ERR_BAD_PATH:  return "bad path (must start with /)";
        case RAMFS_ERR_NOT_DIR:   return "not a directory";
        case RAMFS_ERR_IS_DIR:    return "is a directory";
        case RAMFS_ERR_NOT_EMPTY: return "directory not empty";
        case RAMFS_ERR_NAME_LONG: return "name too long";
        case RAMFS_ERR_IO:        return "i/o error";
        default:                  return "unknown error";
    }
}

static void print_error(const char *cmd, int err) {
    kprint("\n");
    kprint((char *)cmd);
    kprint(": ");
    kprint((char *)fs_err_msg(err));
}

/* Trim whitespace at both ends in place. Returns pointer to first non-space. */
static char *trim(char *s) {
    while (*s == ' ') s++;
    u32 len = strlen(s);
    while (len > 0 && s[len - 1] == ' ') { s[len - 1] = '\0'; len--; }
    return s;
}

/*
 * Split a two-arg command like "write /foo hello world" into (path, rest).
 * `input` points at the text after the command keyword. Returns 1 on success,
 * 0 if only one token is present, -1 if the input is empty.
 */
static int split_two(char *input, char **path, char **rest) {
    input = trim(input);
    if (*input == '\0') return -1;
    *path = input;
    while (*input && *input != ' ') input++;
    if (*input == '\0') { *rest = input; return 0; }
    *input = '\0';
    input++;
    while (*input == ' ') input++;
    *rest = input;
    return 1;
}

// Filesystem command handlers
static void cmd_ls(char *arg) {
    arg = trim(arg);
    const char *path = (*arg == '\0') ? "/" : arg;

    ramfs_stat_t entries[32];
    int n = ramfs_list(path, entries, 32);
    if (n < 0) { print_error("ls", n); return; }

    kprint("\n");
    if (n == 0) { kprint("(empty)"); return; }

    for (int i = 0; i < n; i++) {
        kprint(entries[i].type == RAMFS_T_DIR ? "d " : "- ");
        kprint(entries[i].name);
        if (entries[i].type == RAMFS_T_DIR) {
            kprint("/");
        } else {
            kprint("  (");
            print_u32(entries[i].size);
            kprint("b)");
        }
        if (i + 1 < n) kprint("\n");
    }
}

static void cmd_mkdir(char *arg) {
    arg = trim(arg);
    if (*arg == '\0') { kprint("\nmkdir: path required"); return; }
    int r = ramfs_mkdir(arg);
    if (r != RAMFS_OK) { print_error("mkdir", r); return; }
    kprint("\ncreated ");
    kprint(arg);
}

static void cmd_touch(char *arg) {
    arg = trim(arg);
    if (*arg == '\0') { kprint("\ntouch: path required"); return; }
    int r = ramfs_create(arg);
    if (r != RAMFS_OK) { print_error("touch", r); return; }
    kprint("\ncreated ");
    kprint(arg);
}

static void cmd_rm(char *arg) {
    arg = trim(arg);
    if (*arg == '\0') { kprint("\nrm: path required"); return; }
    int r = ramfs_unlink(arg);
    if (r != RAMFS_OK) { print_error("rm", r); return; }
    kprint("\nremoved ");
    kprint(arg);
}

static void cmd_rmdir(char *arg) {
    arg = trim(arg);
    if (*arg == '\0') { kprint("\nrmdir: path required"); return; }
    int r = ramfs_rmdir(arg);
    if (r != RAMFS_OK) { print_error("rmdir", r); return; }
    kprint("\nremoved ");
    kprint(arg);
}

static void cmd_cat(char *arg) {
    arg = trim(arg);
    if (*arg == '\0') { kprint("\ncat: path required"); return; }

    ramfs_fd fd = ramfs_open(arg, RAMFS_O_READ);
    if (fd < 0) { print_error("cat", fd); return; }

    kprint("\n");
    char buf[128];
    while (1) {
        int n = ramfs_read(fd, buf, sizeof(buf) - 1);
        if (n <= 0) break;
        buf[n] = '\0';
        kprint(buf);
    }
    ramfs_close(fd);
}

static void cmd_write(char *arg) {
    char *path, *content;
    int split = split_two(arg, &path, &content);
    if (split < 0) { kprint("\nwrite: path required"); return; }
    /* split == 0 means empty content - valid (truncates file). */

    ramfs_fd fd = ramfs_open(path, RAMFS_O_WRITE | RAMFS_O_CREATE | RAMFS_O_TRUNC);
    if (fd < 0) { print_error("write", fd); return; }

    u32 len = strlen(content);
    int w = 0;
    if (len > 0) w = ramfs_write(fd, content, len);
    ramfs_close(fd);

    if (w < 0) { print_error("write", w); return; }
    kprint("\nwrote ");
    print_u32((u32)w);
    kprint(" bytes to ");
    kprint(path);
}

static void cmd_stat(char *arg) {
    arg = trim(arg);
    if (*arg == '\0') { kprint("\nstat: path required"); return; }
    ramfs_stat_t s;
    int r = ramfs_stat(arg, &s);
    if (r != RAMFS_OK) { print_error("stat", r); return; }
    kprint("\nname: ");
    kprint(s.name);
    kprint("\ntype: ");
    kprint(s.type == RAMFS_T_DIR ? "directory" : "file");
    kprint("\nsize: ");
    print_u32(s.size);
    kprint(" bytes");
}

static void cmd_df(void) {
    u32 bu, bt, iu, it;
    ramfs_usage(&bu, &bt, &iu, &it);
    kprint("\nblocks: ");
    print_u32(bu); kprint("/"); print_u32(bt);
    kprint("  inodes: ");
    print_u32(iu); kprint("/"); print_u32(it);
}

static void cmd_help(void) {
    kprint("\nCommands:");
    kprint("\n  clear               clear the screen");
    kprint("\n  echo <text>         print text");

    // Misc:
    kprint("\n  art                 run the generative fingerprint (Ctrl+C / ESC to exit)");
    kprint("\n  portal              play \"Still Alive\" from Portal (Ctrl+C to exit)");
    kprint("\n  gamble              open the casino - spend coins on error-skin cases");
    kprint("\n  inventory [recent]  list collected error skins (default: sort by rarity)");
    kprint("\n  tetris              play tetris (get coins!, Ctrl+C to exit)");
    // IDE

    // File related:
    kprint("\n  ls [path]           list directory (default /)");
    kprint("\n  mkdir <path>        create a directory");
    kprint("\n  rmdir <path>        remove an empty directory");
    kprint("\n  touch <path>        create an empty file");
    kprint("\n  rm <path>           delete a file");
    kprint("\n  cat <path>          print a file's contents");
    kprint("\n  write <path> <txt>  overwrite file with text");
    kprint("\n  stat <path>         show file/directory info");
    kprint("\n  df                  show filesystem usage");

    kprint("\n  help                show this list");
}

// Dispatch
void shell_execute_command(char *input) {
    if (strcasecmp(input, "clear") == 0) {
        clear_screen();
        kprint("> ");
    }
    else if (strncasecmp(input, "echo ", 5) == 0) {
        kprint("\n");
        kprint(input + 5);
    }
    else if (strcasecmp(input, "art") == 0) {
        art_run();
        clear_screen();
        kprint("nanopulseOS> ");
    }
    else if (strcasecmp(input, "portal") == 0) {
        portal_run();
        clear_screen();
        kprint("nanopulseOS> ");
    }
    else if (strcasecmp(input, "gamble") == 0) {
        gamble_run();
        clear_screen();
        kprint("nanopulseOS> ");
    }
    else if (strcasecmp(input, "tetris") == 0) {
        tetris_run();
        clear_screen();
        kprint("nanopulseOS> ");
    }
    else if (strcasecmp(input, "inventory") == 0 ||
             strcasecmp(input, "inv") == 0) {
        gamble_inventory(0);
    }
    else if (strcasecmp(input, "inventory recent") == 0 ||
             strcasecmp(input, "inv recent") == 0) {
        gamble_inventory(1);
    }
    else if (strncasecmp(input, "ls",    2) == 0 && (input[2] == '\0' || input[2] == ' ')) {
        cmd_ls(input + 2);
    }
    else if (strncasecmp(input, "mkdir ", 6) == 0) cmd_mkdir(input + 6);
    else if (strncasecmp(input, "rmdir ", 6) == 0) cmd_rmdir(input + 6);
    else if (strncasecmp(input, "touch ", 6) == 0) cmd_touch(input + 6);
    else if (strncasecmp(input, "rm ",    3) == 0) cmd_rm(input + 3);
    else if (strncasecmp(input, "cat ",   4) == 0) cmd_cat(input + 4);
    else if (strncasecmp(input, "write ", 6) == 0) cmd_write(input + 6);
    else if (strncasecmp(input, "stat ",  5) == 0) cmd_stat(input + 5);
    else if (strcasecmp(input, "df")   == 0) cmd_df();
    else if (strcasecmp(input, "help") == 0) cmd_help();
    else if (input[0] != '\0') {
        kprint("\nUnknown command: ");
        kprint(input);
    }

    for (int i = 0; i < 256; i++) input[i] = '\0';
}
