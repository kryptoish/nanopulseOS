#include <kernel/tetris.h>
#include <kernel/gamble.h>
#include <drivers/screen.h>
#include <drivers/keyboard.h>
#include <drivers/types.h>
#include <interrupts/timer.h>
#include <string.h>

void int_to_ascii(int n, char str[]);

#define BOARD_W 10
#define BOARD_H 20
#define BOARD_L 22
#define BOARD_R 43
#define BOARD_T 2
#define BOARD_B 23
#define SIDE_C  47

#define P_NONE  -1

/* 4x4 bitfield: bit (15 - r*4 - c) = filled. Top-left = MSB. */
static const u16 piece_rots[7][4] = {
    { 0x0F00, 0x2222, 0x00F0, 0x4444 },
    { 0x6600, 0x6600, 0x6600, 0x6600 },
    { 0x4E00, 0x4640, 0x0E40, 0x4C40 },
    { 0x6C00, 0x4620, 0x6C00, 0x4620 },
    { 0xC600, 0x2640, 0xC600, 0x2640 },
    { 0x8E00, 0x6440, 0x0E20, 0x44C0 },
    { 0x2E00, 0x4460, 0x0E80, 0xC440 },
};

static const u8 piece_color[7] = {
    0x0B, 0x0E, 0x0D, 0x0A, 0x0C, 0x09, 0x06,
};

typedef struct { int type, rot, x, y; } piece_t;

static u8 board[BOARD_H][BOARD_W];
static piece_t cur;
static int next_type;
static int hold_type;
static int hold_used;
static u32 score;
static int level;
static int lines;
static int game_over;

static u64 prng_state = 0;

static u64 rdtsc(void) {
    u32 lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((u64)hi << 32) | lo;
}

static u64 prng_next(void) {
    if (prng_state == 0) {
        prng_state = rdtsc() ^ ((u64)get_tick() * 0x9E3779B97F4A7C15ULL);
        if (prng_state == 0) prng_state = 0xCAFEF00DDEADBEEFULL;
    }
    u64 x = prng_state;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    prng_state = x ^ rdtsc();
    return x;
}

static u32 prng_range(u32 b) {
    if (b == 0) return 0;
    return (u32)(prng_next() % b);
}

static int bag[7];
static int bag_pos = 7;

static int next_piece_type(void) {
    if (bag_pos >= 7) {
        for (int i = 0; i < 7; i++) bag[i] = i;
        for (int i = 6; i > 0; i--) {
            int j = (int)prng_range((u32)(i + 1));
            int t = bag[i]; bag[i] = bag[j]; bag[j] = t;
        }
        bag_pos = 0;
    }
    return bag[bag_pos++];
}

static int piece_cell(int type, int rot, int r, int c) {
    return (piece_rots[type][rot] >> (15 - (r * 4 + c))) & 1;
}

static int collides(const piece_t *p) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (!piece_cell(p->type, p->rot, r, c)) continue;
            int bx = p->x + c;
            int by = p->y + r;
            if (bx < 0 || bx >= BOARD_W) return 1;
            if (by >= BOARD_H) return 1;
            if (by >= 0 && board[by][bx]) return 1;
        }
    }
    return 0;
}

static void put_at(int col, int row, char c, u8 attr) {
    if (col < 0 || col >= MAX_COLS || row < 0 || row >= MAX_ROWS) return;
    u8 *vid = (u8*)VIDEO_ADDRESS;
    int off = 2 * (row * MAX_COLS + col);
    vid[off]     = (u8)c;
    vid[off + 1] = attr;
}

static void write_str(int col, int row, const char *s, u8 attr) {
    for (int i = 0; s[i]; i++) put_at(col + i, row, s[i], attr);
}

static void fill_rect(int col, int row, int w, int h, char c, u8 attr) {
    for (int r = 0; r < h; r++)
        for (int cc = 0; cc < w; cc++)
            put_at(col + cc, row + r, c, attr);
}

/* Each board cell renders as 2 chars wide × 1 char tall (≈square). */
static void draw_block(int board_col, int board_row, u8 attr) {
    if (board_row < 0) return;
    int sc = BOARD_L + 1 + board_col * 2;
    int sr = BOARD_T + 1 + board_row;
    if (attr == 0) {
        put_at(sc,     sr, ' ', WHITE_ON_BLACK);
        put_at(sc + 1, sr, ' ', WHITE_ON_BLACK);
    } else {
        put_at(sc,     sr, 0xDB, attr);
        put_at(sc + 1, sr, 0xDB, attr);
    }
}

static void draw_board(void) {
    for (int r = 0; r < BOARD_H; r++) {
        for (int c = 0; c < BOARD_W; c++) {
            u8 cell = board[r][c];
            u8 attr = cell ? piece_color[cell - 1] : 0;
            draw_block(c, r, attr);
        }
    }
}

static void draw_piece(const piece_t *p, int erase) {
    u8 attr = erase ? 0 : piece_color[p->type];
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (!piece_cell(p->type, p->rot, r, c)) continue;
            int bx = p->x + c;
            int by = p->y + r;
            if (by < 0 || by >= BOARD_H || bx < 0 || bx >= BOARD_W) continue;
            draw_block(bx, by, attr);
        }
    }
}

static void draw_chrome(void) {
    fill_rect(0, 0, MAX_COLS, MAX_ROWS, ' ', WHITE_ON_BLACK);
    write_str(36, 0, "TETRIS", 0x0E);

    for (int r = BOARD_T; r <= BOARD_B; r++) {
        put_at(BOARD_L, r, '|', 0x07);
        put_at(BOARD_R, r, '|', 0x07);
    }
    for (int c = BOARD_L; c <= BOARD_R; c++) {
        put_at(c, BOARD_T, '-', 0x07);
        put_at(c, BOARD_B, '-', 0x07);
    }
    put_at(BOARD_L, BOARD_T, '+', 0x07);
    put_at(BOARD_R, BOARD_T, '+', 0x07);
    put_at(BOARD_L, BOARD_B, '+', 0x07);
    put_at(BOARD_R, BOARD_B, '+', 0x07);

    write_str(SIDE_C, 3,  "NEXT:",  0x07);
    write_str(SIDE_C, 9,  "HOLD:",  0x07);
    write_str(SIDE_C, 14, "SCORE:", 0x07);
    write_str(SIDE_C, 16, "LEVEL:", 0x07);
    write_str(SIDE_C, 18, "LINES:", 0x07);
    write_str(SIDE_C, 20, "COINS:", 0x0E);

    write_str(2, 24,
              " <-/-> move | v soft | SPACE drop | ^/X rot | Z ccw | C hold | Ctrl+C exit",
              0x08);
}

static void draw_preview(int col, int row, int type) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 8; c++)
            put_at(col + c, row + r, ' ', WHITE_ON_BLACK);
    if (type < 0) return;
    u8 attr = piece_color[type];
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if ((piece_rots[type][0] >> (15 - r * 4 - c)) & 1) {
                int sc = col + c * 2;
                int sr = row + r;
                put_at(sc,     sr, 0xDB, attr);
                put_at(sc + 1, sr, 0xDB, attr);
            }
        }
    }
}

static void draw_number(int col, int row, u32 v, u8 attr) {
    char buf[16];
    int_to_ascii((int)v, buf);
    for (int i = 0; i < 12; i++) put_at(col + i, row, ' ', WHITE_ON_BLACK);
    write_str(col, row, buf, attr);
}

static void draw_status(void) {
    draw_preview(SIDE_C, 5,  next_type);
    draw_preview(SIDE_C, 11, hold_type);
    draw_number (SIDE_C, 15, score,            0x0F);
    draw_number (SIDE_C, 17, (u32)level,       0x0F);
    draw_number (SIDE_C, 19, (u32)lines,       0x0F);
    draw_number (SIDE_C, 21, score / 10,       0x0E);
}

#define SC_LEFT   0x4B
#define SC_RIGHT  0x4D
#define SC_DOWN   0x50
#define SC_UP     0x48
#define SC_SPACE  0x39
#define SC_Z      0x2C
#define SC_X      0x2D
#define SC_C      0x2E
#define SC_R      0x13
#define SC_Q      0x10
#define SC_ESC    0x01
#define SC_A      0x1E
#define SC_S      0x1F
#define SC_D      0x20
#define SC_W      0x11

static int try_move(int dx, int dy) {
    piece_t p = cur;
    p.x += dx; p.y += dy;
    if (collides(&p)) return 0;
    draw_piece(&cur, 1);
    cur = p;
    draw_piece(&cur, 0);
    return 1;
}

static int try_rotate(int dir) {
    piece_t p = cur;
    p.rot = (p.rot + (dir > 0 ? 1 : 3)) & 3;
    if (collides(&p)) return 0;
    draw_piece(&cur, 1);
    cur = p;
    draw_piece(&cur, 0);
    return 1;
}

static int hard_drop(void) {
    draw_piece(&cur, 1);
    int cells = 0;
    while (1) {
        piece_t p = cur;
        p.y++;
        if (collides(&p)) break;
        cur = p;
        cells++;
    }
    draw_piece(&cur, 0);
    return cells;
}

/* Linear ramp: ~800ms at level 1, capped at 50ms past level 16. */
static u32 drop_ms(void) {
    int ms = 850 - level * 50;
    if (ms < 50) ms = 50;
    return (u32)ms;
}

static u32 score_for_lines(int n) {
    static const u32 base[5] = { 0, 100, 300, 500, 800 };
    if (n < 0 || n > 4) return 0;
    return base[n] * (u32)level;
}

static void spawn(int type) {
    cur.type = type;
    cur.rot  = 0;
    cur.x    = (BOARD_W - 4) / 2;
    cur.y    = 0;
}

static void spawn_next(void) {
    spawn(next_type);
    next_type = next_piece_type();
    hold_used = 0;
    if (collides(&cur)) { game_over = 1; return; }
    draw_piece(&cur, 0);
}

static void try_hold(void) {
    if (hold_used) return;
    int t = cur.type;
    draw_piece(&cur, 1);
    if (hold_type < 0) {
        spawn(next_type);
        next_type = next_piece_type();
    } else {
        spawn(hold_type);
    }
    hold_type = t;
    hold_used = 1;
    if (collides(&cur)) { game_over = 1; return; }
    draw_piece(&cur, 0);
}

/* Returns 1 if any cell of the locked piece sat above the playfield. */
static int lock_piece(void) {
    int above = 0;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (!piece_cell(cur.type, cur.rot, r, c)) continue;
            int bx = cur.x + c;
            int by = cur.y + r;
            if (by < 0) { above = 1; continue; }
            if (by < BOARD_H && bx >= 0 && bx < BOARD_W) {
                board[by][bx] = (u8)(cur.type + 1);
            }
        }
    }
    return above;
}

static int clear_lines(void) {
    int cleared = 0;
    int r = BOARD_H - 1;
    while (r >= 0) {
        int full = 1;
        for (int c = 0; c < BOARD_W; c++) {
            if (!board[r][c]) { full = 0; break; }
        }
        if (full) {
            for (int rr = r; rr > 0; rr--)
                for (int c = 0; c < BOARD_W; c++)
                    board[rr][c] = board[rr - 1][c];
            for (int c = 0; c < BOARD_W; c++) board[0][c] = 0;
            cleared++;
        } else r--;
    }
    return cleared;
}

static void update_level(void) {
    int nl = 1 + lines / 10;
    if (nl > level) level = nl;
}

static void process_lock(void) {
    int top_out = lock_piece();
    int cleared = clear_lines();
    if (cleared > 0) {
        lines += cleared;
        score += score_for_lines(cleared);
        update_level();
        draw_board();
    }
    if (top_out) { game_over = 1; return; }
    spawn_next();
}

static u32 ms_to_ticks(u32 ms) {
    u32 t = (ms + 9) / 10;
    return t == 0 ? 1 : t;
}

static void init_game(void) {
    for (int r = 0; r < BOARD_H; r++)
        for (int c = 0; c < BOARD_W; c++) board[r][c] = 0;
    score = 0;
    level = 1;
    lines = 0;
    game_over = 0;
    hold_type = P_NONE;
    hold_used = 0;
    bag_pos = 7;
    next_type = next_piece_type();
}

static int game_over_screen(void) {
    int cx = MAX_COLS / 2;
    int cy = MAX_ROWS / 2;
    int boxw = 36, boxh = 9;
    int bx = cx - boxw / 2, by = cy - boxh / 2;

    fill_rect(bx, by, boxw, boxh, ' ', WHITE_ON_BLACK);
    for (int c = 0; c < boxw; c++) {
        put_at(bx + c, by,             '-', 0x0C);
        put_at(bx + c, by + boxh - 1,  '-', 0x0C);
    }
    for (int r = 0; r < boxh; r++) {
        put_at(bx,             by + r, '|', 0x0C);
        put_at(bx + boxw - 1,  by + r, '|', 0x0C);
    }
    put_at(bx,             by,             '+', 0x0C);
    put_at(bx + boxw - 1,  by,             '+', 0x0C);
    put_at(bx,             by + boxh - 1,  '+', 0x0C);
    put_at(bx + boxw - 1,  by + boxh - 1,  '+', 0x0C);

    const char *t = "YOU LOSE";
    write_str(cx - (int)strlen((char*)t) / 2, by + 1, t, 0x0C);

    char buf[16];
    int_to_ascii((int)score, buf);
    write_str(bx + 4,  by + 3, "Score:", 0x07);
    write_str(bx + 12, by + 3, buf,      0x0F);

    u32 coins = score / 10;
    int_to_ascii((int)coins, buf);
    write_str(bx + 4,  by + 4, "Coins earned:", 0x07);
    write_str(bx + 18, by + 4, buf,             0x0E);

    write_str(bx + 4, by + 6, "[R] new game   [Q/Ctrl+C] exit", 0x07);

    set_cursor_offset(get_offset(MAX_COLS - 1, MAX_ROWS - 1));

    while (!keyboard_cancel_requested()) {
        u8 sc = keyboard_poll_scancode();
        if (sc == 0 || sc == 0xE0 || (sc & 0x80)) {
            __asm__ __volatile__("hlt");
            continue;
        }
        if (sc == SC_R) return 1;
        if (sc == SC_Q || sc == SC_ESC) return 0;
    }
    return 0;
}

static void play_round(void) {
    init_game();
    draw_chrome();
    draw_board();
    spawn_next();
    draw_status();

    u32 next_drop = get_tick() + ms_to_ticks(drop_ms());

    while (!game_over) {
        if (keyboard_cancel_requested()) return;

        u8 sc = keyboard_poll_scancode();
        if (sc != 0 && sc != 0xE0 && !(sc & 0x80)) {
            switch (sc) {
                case SC_LEFT:  case SC_A: try_move(-1, 0); break;
                case SC_RIGHT: case SC_D: try_move( 1, 0); break;
                case SC_DOWN:  case SC_S:
                    if (try_move(0, 1)) score += 1;
                    break;
                case SC_UP: case SC_X: case SC_W:
                    try_rotate(1); break;
                case SC_Z:
                    try_rotate(-1); break;
                case SC_SPACE: {
                    int cells = hard_drop();
                    score += (u32)cells * 2;
                    process_lock();
                    next_drop = get_tick() + ms_to_ticks(drop_ms());
                    break;
                }
                case SC_C:
                    try_hold();
                    next_drop = get_tick() + ms_to_ticks(drop_ms());
                    break;
            }
            draw_status();
        }

        if (!game_over && get_tick() >= next_drop) {
            if (!try_move(0, 1)) {
                process_lock();
                draw_status();
            }
            next_drop = get_tick() + ms_to_ticks(drop_ms());
        }

        set_cursor_offset(get_offset(MAX_COLS - 1, MAX_ROWS - 1));
        __asm__ __volatile__("hlt");
    }
}

void tetris_run(void) {
    keyboard_clear_cancel();
    keyboard_set_raw_mode(1);
    __asm__ __volatile__("sti");

    while (1) {
        play_round();
        gamble_add_coins(score / 10);
        if (keyboard_cancel_requested()) break;
        if (!game_over_screen()) break;
        keyboard_clear_cancel();
    }

    keyboard_set_raw_mode(0);
    keyboard_clear_cancel();
}
