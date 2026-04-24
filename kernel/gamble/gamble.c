#include <kernel/gamble.h>
#include <drivers/screen.h>
#include <drivers/keyboard.h>
#include <drivers/speaker.h>
#include <drivers/types.h>
#include <interrupts/timer.h>
#include <string.h>

/* From libc/stdio/itoa.c -> no public header yet. */
void int_to_ascii(int n, char str[]);

// Rarity
typedef enum {
    R_COMMON = 0,
    R_UNCOMMON,
    R_RARE,
    R_EPIC,
    R_LEGENDARY,
    R_COUNT
} rarity_t;

/* VGA text-mode attributes (foreground on black). */
static const u8 rarity_color[R_COUNT] = {
    0x07,  /* common    -> light gray  */
    0x0B,  /* uncommon  -> light cyan  */
    0x0D,  /* rare      -> light magenta */
    0x0C,  /* epic      -> light red/pink */
    0x0E,  /* legendary -> yellow/gold */
};

static const char *rarity_name[R_COUNT] = {
    "Common", "Uncommon", "Rare", "Epic", "Legendary"
};

/* Skins database */

typedef struct {
    const char *name;
    const char *full;
    u8          rarity;
    const char *art[3];
} skin_t;

static const skin_t skins[] = {
    /* -------- COMMON (everyday bugs) -------- */
    { "404",        "404 Not Found",         R_COMMON,    { "  ______  ", " | 404  | ", "  ------  " } },
    { "Syntax",     "Syntax Error",          R_COMMON,    { "  x = 5   ", "  if ( {  ", "   ???    " } },
    { "NoSemi",     "Missing Semicolon",     R_COMMON,    { "  code    ", "  code    ", "  line;   " } },
    { "NullPtr",    "Null Pointer",          R_COMMON,    { "   NULL   ", "    ->    ", "    X     " } },
    { "NoFile",     "File Not Found",        R_COMMON,    { "  [ ? ]   ", " no file  ", "  missing " } },
    { "Denied",     "Access Denied",         R_COMMON,    { "  [LOCK]  ", "   |X|    ", "  denied  " } },
    { "Timeout",    "Timeout",               R_COMMON,    { "   ___    ", "  |tO|    ", "  too slow" } },
    { "Refused",    "Connection Refused",    R_COMMON,    { "   >|<    ", "  X---X   ", "  closed  " } },
    { "BadInput",   "Invalid Input",         R_COMMON,    { "   !?!    ", "   bad    ", "   <<<    " } },
    { "Undef",      "Undefined Variable",    R_COMMON,    { "    x     ", "   = ?    ", "  where?  " } },
    { "OffByOne",   "Off-By-One",            R_COMMON,    { " 0 1 ... n", "    ^     ", "  n-1? n? " } },
    { "Unused",     "Unused Variable",       R_COMMON,    { "  int x;  ", "   ...    ", "  lonely  " } },

    /* -------- UNCOMMON -------- */
    { "Segfault",   "Segmentation Fault",    R_UNCOMMON,  { "  [MEM]   ", "  ** **   ", "  CRASH   " } },
    { "StackOver",  "Stack Overflow",        R_UNCOMMON,  { "  [][][]  ", "  [][][]  ", "   ^^^!   " } },
    { "BufOver",    "Buffer Overflow",       R_UNCOMMON,  { "  [####]  ", "  [####>> ", "    >>>>  " } },
    { "Race",       "Race Condition",        R_UNCOMMON,  { "  T1 ->   ", "    <- T2 ", "    ><!   " } },
    { "MemLeak",    "Memory Leak",           R_UNCOMMON,  { "  [MEM]   ", "   drip   ", "    ...   " } },
    { "Dangling",   "Dangling Pointer",      R_UNCOMMON,  { "   ptr    ", "    ->    ", "   void   " } },
    { "InfLoop",    "Infinite Loop",         R_UNCOMMON,  { "   /-\\    ", "  | O |   ", "   \\-/    " } },
    { "DblFree",    "Double Free",           R_UNCOMMON,  { "  free()  ", "  free()  ", "   BOOM   " } },
    { "HeapCorr",   "Heap Corruption",       R_UNCOMMON,  { "  HEAP~~  ", "  ~corr~  ", "  broken  " } },
    { "ForkBomb",   "Fork Bomb",             R_UNCOMMON,  { "  () ()   ", " () () () ", "  spawn.. " } },

    /* -------- RARE -------- */
    { "KPanic",     "Kernel Panic",          R_RARE,      { " [KERNEL] ", "  PANIC!! ", "   !!!    " } },
    { "BSOD",       "Blue Screen of Death",  R_RARE,      { "  :-(     ", "  0x0000  ", "  dump..  " } },
    { "DiskFail",   "Disk Failure",          R_RARE,      { "  [HDD]   ", "   XXX    ", "  dying   " } },
    { "OOM",        "Out Of Memory",         R_RARE,      { "  MEM=0   ", "  [////]  ", "   dead   " } },
    { "Deadlock",   "Deadlock",              R_RARE,      { "  A->B    ", "  B->A    ", "  frozen  " } },
    { "Rootkit",    "Rootkit Detected",      R_RARE,      { "  [root]  ", "   \\o/    ", "  owned!  " } },
    { "NoBoot",     "Bootloader Missing",    R_RARE,      { " [no boot]", "   ...    ", "   void   " } },
    { "BadCert",    "Invalid Certificate",   R_RARE,      { "  [cert]  ", "   /X/    ", "  invalid " } },

    /* -------- EPIC -------- */
    { "Fatal",      "Fatal Exception",       R_EPIC,      { "  FATAL   ", "  [0xDE]  ", "  EXIT!!  " } },
    { "Meltdown",   "CPU Meltdown",          R_EPIC,      { "  [CPU]   ", "  ~melt~  ", "   drip   " } },
    { "Quantum",    "Quantum Glitch",        R_EPIC,      { "   |0>    ", "   + |1>  ", "   ???    " } },
    { "Cosmic",     "Cosmic Ray Bitflip",    R_EPIC,      { "   ***    ", "   -->|   ", "   bit!   " } },
    { "HCF",        "Halt And Catch Fire",   R_EPIC,      { "   HCF    ", "  (*_*)   ", "  /FIRE\\  " } },
    { "DivZero",    "Divide By Zero",        R_EPIC,      { "   1 / 0  ", "   = ??   ", "  [NaN!]  " } },

    /* -------- LEGENDARY -------- */
    { "0xDEADBEEF", "0xDEADBEEF",            R_LEGENDARY, { "  DEAD    ", "  BEEF    ", " 0xDEAD!  " } },
    { "Y2K",        "The Y2K Bug",           R_LEGENDARY, { "  1999    ", "   +1     ", "  19100!  " } },
    { "Heartbleed", "Heartbleed",            R_LEGENDARY, { "   <3     ", "  bleed.. ", "   /!\\    " } },
    { "Spectre",    "Spectre",               R_LEGENDARY, { "  .-^-.   ", " (ghost)  ", "   CPU    " } },
    { "Halting",    "The Halting Problem",   R_LEGENDARY, { "  halt?   ", "   ...    ", "  unknown " } },
};

#define SKIN_COUNT ((int)(sizeof(skins) / sizeof(skins[0])))

/* Per-rarity index pool, built once on first use. */
static int  pool[R_COUNT][SKIN_COUNT];
static int  pool_size[R_COUNT];
static int  pool_ready = 0;

static void build_pools(void) {
    if (pool_ready) return;
    for (int r = 0; r < R_COUNT; r++) pool_size[r] = 0;
    for (int i = 0; i < SKIN_COUNT; i++) {
        int r = skins[i].rarity;
        pool[r][pool_size[r]++] = i;
    }
    pool_ready = 1;
}

// Cases
typedef struct {
    const char *name;
    u32         price;
    u16         drop_bp[R_COUNT]; /* basis points; sums to 10000 */
    u8          color;
    const char *blurb;
} case_t;

static const case_t cases[4] = {
    { "Bronze",   100,
      /* Common  Uncommon  Rare  Epic   Legendary */
      {  7600,    2000,    380,  18,    2    },
      0x06, "mostly common errors, thin rare chance" },
    { "Silver",   300,
      {  5300,    3000,    1450, 230,   20   },
      0x07, "steady rares, faint whiff of epic"    },
    { "Gold",     750,
      {  3400,    3500,    2000, 950,   150  },
      0x0E, "solid rare/epic odds, real legendary shot" },
    { "Diamond",  2000,
      {  1400,    2500,    3000, 2300,  800  },
      0x0B, "elite tier - best legendary rate"      },
};

// Session State
/* Starting balance. Set to e.g. 1000 for a playable session without grinding. */
#define GAMBLE_STARTING_COINS  0

#define INV_CAP 256

static u32 coins = GAMBLE_STARTING_COINS;

typedef struct {
    u16 skin_idx;
    u16 seq;
} inv_entry_t;

static inv_entry_t inventory[INV_CAP];
static int         inv_count = 0;
static u32         inv_seq_counter = 0;

// PRNG
static u64 prng_state = 0;

static u64 rdtsc(void) {
    u32 lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((u64)hi << 32) | lo;
}

static void prng_seed(void) {
    if (prng_state != 0) return;
    u64 s = rdtsc() ^ ((u64)get_tick() * 0x9E3779B97F4A7C15ULL);
    if (s == 0) s = 0xCAFEF00DDEADBEEFULL;
    prng_state = s;
}

static u64 prng_next(void) {
    prng_seed();
    u64 x = prng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    prng_state = x;
    /* Re-inject timing entropy so back-to-back rolls during one tick still differ. */
    prng_state ^= rdtsc();
    return x;
}

static u32 prng_range(u32 bound) {
    if (bound == 0) return 0;
    return (u32)(prng_next() % bound);
}

// Rolling
static int roll_rarity(int case_id) {
    u32 r = prng_range(10000);
    u32 acc = 0;
    for (int i = 0; i < R_COUNT; i++) {
        acc += cases[case_id].drop_bp[i];
        if (r < acc) return i;
    }
    return R_COMMON;
}

static int roll_skin(int case_id) {
    build_pools();
    int rarity = roll_rarity(case_id);
    int n = pool_size[rarity];
    if (n == 0) {
        /* Fallback shouldn't fire - every rarity has entries. */
        for (int r = 0; r < R_COUNT; r++) if (pool_size[r] > 0) return pool[r][0];
        return 0;
    }
    return pool[rarity][prng_range((u32)n)];
}

// Low-level text rendering (direct VRAM - bypasses kprint's cursor/scroll)
static void put_at(int col, int row, char c, u8 attr) {
    if (col < 0 || col >= MAX_COLS || row < 0 || row >= MAX_ROWS) return;
    u8 *vid = (u8*)VIDEO_ADDRESS;
    int off = 2 * (row * MAX_COLS + col);
    vid[off]     = (u8)c;
    vid[off + 1] = attr;
}

static void write_str(int col, int row, const char *s, u8 attr) {
    for (int i = 0; s[i] != '\0'; i++) {
        put_at(col + i, row, s[i], attr);
    }
}

static void write_padded(int col, int row, const char *s, int w, u8 attr) {
    int i = 0;
    for (; s[i] != '\0' && i < w; i++) put_at(col + i, row, s[i], attr);
    for (; i < w; i++)                 put_at(col + i, row, ' ', attr);
}

static void fill_rect(int col, int row, int w, int h, char ch, u8 attr) {
    for (int r = 0; r < h; r++) {
        for (int c = 0; c < w; c++) {
            put_at(col + c, row + r, ch, attr);
        }
    }
}

static void draw_hr(int row, char ch, u8 attr) {
    for (int c = 0; c < MAX_COLS; c++) put_at(c, row, ch, attr);
}

static void park_cursor(void) {
    /* Hide the blinking cursor off the artwork by parking it bottom-right. */
    set_cursor_offset(get_offset(MAX_COLS - 1, MAX_ROWS - 1));
}

/* Timing */

static u32 ms_to_ticks(u32 ms) {
    u32 t = (ms + 9) / 10;
    return t == 0 ? 1 : t;
}

static void delay_ms(u32 ms) {
    u32 target = get_tick() + ms_to_ticks(ms);
    while (get_tick() < target) {
        if (keyboard_cancel_requested()) return;
        __asm__ __volatile__("hlt");
    }
}

// Menu Screen
static int int_width(u32 v) {
    int w = 1;
    while (v >= 10) { v /= 10; w++; }
    return w;
}

static void draw_balance(void) {
    /* Top-right: "Balance: NNNN coins" */
    char num[16];
    int_to_ascii((int)coins, num);
    int len = 9 /* "Balance: " */ + (int)strlen(num) + 6 /* " coins" */;
    int col = MAX_COLS - len - 1;
    if (col < 0) col = 0;
    write_str(col,     0, "Balance: ", 0x0E);
    write_str(col + 9, 0, num,         0x0E);
    write_str(col + 9 + (int)strlen(num), 0, " coins", 0x0E);
}

static void draw_menu(const char *status_msg, u8 status_color) {
    fill_rect(0, 0, MAX_COLS, MAX_ROWS, ' ', WHITE_ON_BLACK);

    const char *title = "== NANOPULSE CASINO ==";
    int tlen = (int)strlen((char*)title);
    write_str((MAX_COLS - tlen) / 2, 0, title, 0x0E);
    draw_hr(1, '=', 0x0E);
    draw_balance();

    write_str(3, 3, "Buy cases. Collect errors. The worse the bug, the rarer.", 0x07);

    /* Rarity legend */
    write_str(3, 5, "Rarity:", 0x07);
    int c = 11;
    for (int r = 0; r < R_COUNT; r++) {
        write_str(c, 5, rarity_name[r], rarity_color[r]);
        c += (int)strlen(rarity_name[r]) + 2;
    }

    /* Cases */
    int row = 8;
    for (int i = 0; i < 4; i++) {
        const case_t *cs = &cases[i];
        char tag[4] = { '[', (char)('1' + i), ']', '\0' };
        write_str(4, row, tag, 0x0F);
        write_padded(8, row, cs->name, 8, cs->color);
        write_str(17, row, "Case",  cs->color);
        write_str(23, row, "-",     0x08);

        /* Right-justify price to col 30. */
        char buf[16];
        int_to_ascii((int)cs->price, buf);
        int pcol = 30 - (int)strlen(buf);
        write_str(pcol, row, buf,    0x0E);
        write_str(31,   row, "coins", 0x07);

        write_str(40, row, cs->blurb, 0x08);
        row += 2;
    }

    draw_hr(MAX_ROWS - 2, '-', 0x08);
    write_str(3, MAX_ROWS - 1,
              "[1-4] open case    [I] inventory    [Q or Ctrl+C] exit",
              0x07);

    if (status_msg != 0) {
        int mlen = (int)strlen((char*)status_msg);
        int mcol = (MAX_COLS - mlen) / 2;
        if (mcol < 0) mcol = 0;
        write_str(mcol, MAX_ROWS - 3, status_msg, status_color);
    }

    park_cursor();
}

// Animation
#define SLOT_W      12
#define SLOT_H       5   /* 3 art rows + blank + name row */
#define SLOTS_SHOWN  5
#define STRIP_COL   ((MAX_COLS - SLOTS_SHOWN * SLOT_W) / 2)
#define STRIP_ROW    9
#define STRIP_LEN   60
#define WINNER_POS  55   /* winner's index in the strip */

static void draw_slot(int slot_i, int skin_idx, int highlight) {
    const skin_t *s = &skins[skin_idx];
    int col = STRIP_COL + slot_i * SLOT_W;
    u8 attr = rarity_color[s->rarity];

    fill_rect(col, STRIP_ROW, SLOT_W, SLOT_H, ' ', WHITE_ON_BLACK);

    /* 10-char art centered in 12-wide slot (1 col pad each side). */
    for (int r = 0; r < 3; r++) {
        write_padded(col + 1, STRIP_ROW + r, s->art[r], 10, attr);
    }
    /* Name row (row 4), with one blank row above for breathing room. */
    write_padded(col + 1, STRIP_ROW + 4, s->name, 10, attr);

    if (highlight) {
        /* Yellow frame around center slot. */
        for (int r = 0; r < SLOT_H; r++) {
            put_at(col,             STRIP_ROW + r, '|', 0x0E);
            put_at(col + SLOT_W - 1, STRIP_ROW + r, '|', 0x0E);
        }
        put_at(col,              STRIP_ROW - 1,       '+', 0x0E);
        put_at(col + SLOT_W - 1, STRIP_ROW - 1,       '+', 0x0E);
        put_at(col,              STRIP_ROW + SLOT_H,  '+', 0x0E);
        put_at(col + SLOT_W - 1, STRIP_ROW + SLOT_H,  '+', 0x0E);
        for (int dc = 1; dc < SLOT_W - 1; dc++) {
            put_at(col + dc, STRIP_ROW - 1,      '-', 0x0E);
            put_at(col + dc, STRIP_ROW + SLOT_H, '-', 0x0E);
        }
    }
}

static void draw_reticle(void) {
    int center_col = STRIP_COL + 2 * SLOT_W + SLOT_W / 2;
    put_at(center_col,     STRIP_ROW - 2,     'v', 0x0E);
    put_at(center_col - 1, STRIP_ROW - 2,     'v', 0x0E);
    put_at(center_col,     STRIP_ROW + SLOT_H + 1, '^', 0x0E);
    put_at(center_col - 1, STRIP_ROW + SLOT_H + 1, '^', 0x0E);
}

static void animate_case_open(int case_id, int winner) {
    int strip[STRIP_LEN];
    for (int i = 0; i < STRIP_LEN; i++) {
        strip[i] = (i == WINNER_POS) ? winner : roll_skin(case_id);
    }

    /* Chrome */
    fill_rect(0, 0, MAX_COLS, MAX_ROWS, ' ', WHITE_ON_BLACK);

    /* "OPENING <CASE> CASE" in the case's own theme color. */
    char title[48];
    int k = 0;
    const char *pre = "OPENING ";
    for (int i = 0; pre[i]; i++) title[k++] = pre[i];
    for (int i = 0; cases[case_id].name[i]; i++) {
        char c = cases[case_id].name[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        title[k++] = c;
    }
    const char *post = " CASE";
    for (int i = 0; post[i]; i++) title[k++] = post[i];
    title[k] = '\0';
    write_str((MAX_COLS - k) / 2, 0, title, cases[case_id].color);
    draw_hr(1, '=', cases[case_id].color);
    draw_balance();
    draw_reticle();

    /* cur advances so that strip[cur + 2] sits in the center slot.
     * Winner at index 55 → stop when cur = 53. */
    int last = WINNER_POS - 2;
    for (int cur = 0; cur <= last; cur++) {
        if (keyboard_cancel_requested()) { speaker_stop(); return; }

        for (int i = 0; i < SLOTS_SHOWN; i++) {
            draw_slot(i, strip[cur + i], i == 2);
        }
        draw_reticle();
        park_cursor();

        /* Quadratic ease-out: 40ms → 520ms across the run. */
        u32 progress = (last > 0) ? (u32)(100 * (u64)cur / (u64)last) : 100;
        u32 p2 = (progress * progress) / 100;
        u32 delay = 40 + (p2 * (520 - 40)) / 100;

        /* Short tick beep each slot - pitch rises with tension. */
        u32 beep_hz = 900 + (progress * 6);
        u32 beep_ms = delay / 3;
        if (beep_ms > 35) beep_ms = 35;
        speaker_play_freq(beep_hz);
        delay_ms(beep_ms);
        speaker_stop();
        if (delay > beep_ms) delay_ms(delay - beep_ms);
    }
    speaker_stop();
}

// Reveal
static void play_tune(const u32 *notes, u32 per_note_ms) {
    for (int i = 0; notes[i] != 0; i++) {
        if (keyboard_cancel_requested()) { speaker_stop(); return; }
        speaker_play_freq(notes[i]);
        delay_ms(per_note_ms);
    }
    speaker_stop();
}

static void reveal_winner(int winner) {
    const skin_t *s = &skins[winner];
    u8 attr = rarity_color[s->rarity];

    /* Three flashes on the center card to punctuate the landing. */
    int center = STRIP_COL + 2 * SLOT_W;
    for (int f = 0; f < 3; f++) {
        if (keyboard_cancel_requested()) return;
        u8 flash = (f & 1) ? WHITE_ON_BLACK : attr;
        for (int r = 0; r < SLOT_H; r++) {
            for (int c = 0; c < SLOT_W; c++) {
                u8 *vid = (u8*)VIDEO_ADDRESS;
                int off = 2 * ((STRIP_ROW + r) * MAX_COLS + (center + c));
                /* Keep glyphs; flip the attribute for the flash frame. */
                vid[off + 1] = (f & 1) ? attr : (u8)((attr & 0x0F) | 0x70);
                (void)flash;
            }
        }
        park_cursor();
        speaker_play_freq(700 + (u32)f * 160);
        delay_ms(110);
        speaker_stop();
        delay_ms(70);
    }
    /* Restore the center slot with its rarity frame. */
    draw_slot(2, winner, 1);
    draw_reticle();

    /* Reveal panel below the strip. */
    int row = STRIP_ROW + SLOT_H + 3;
    fill_rect(0, row, MAX_COLS, MAX_ROWS - row - 2, ' ', WHITE_ON_BLACK);

    const char *got = "You got...";
    write_str((MAX_COLS - (int)strlen(got)) / 2, row, got, 0x07);

    write_str((MAX_COLS - (int)strlen(rarity_name[s->rarity])) / 2,
              row + 2, rarity_name[s->rarity], attr);

    write_str((MAX_COLS - (int)strlen(s->full)) / 2,
              row + 3, s->full, attr);

    const char *hint = "added to inventory  -  press any key to continue";
    write_str((MAX_COLS - (int)strlen(hint)) / 2,
              MAX_ROWS - 1, hint, 0x07);
    park_cursor();

    /* Celebratory fanfare scales with rarity. */
    if (s->rarity == R_LEGENDARY) {
        static const u32 t[] = { 523, 659, 784, 1047, 1319, 1568, 0 };
        play_tune(t, 120);
    } else if (s->rarity == R_EPIC) {
        static const u32 t[] = { 523, 659, 784, 1047, 0 };
        play_tune(t, 110);
    } else if (s->rarity == R_RARE) {
        static const u32 t[] = { 523, 784, 0 };
        play_tune(t, 120);
    }
}

// Input Helpers
#define SC_ESC   0x01
#define SC_1     0x02
#define SC_2     0x03
#define SC_3     0x04
#define SC_4     0x05
#define SC_I     0x17
#define SC_Q     0x10
#define SC_R     0x13
#define SC_LSH   0x2A
#define SC_RSH   0x36
#define SC_CAPS  0x3A
#define SC_CTRL  0x1D

static int is_modifier(u8 sc) {
    u8 b = sc & 0x7F;
    return b == SC_LSH || b == SC_RSH || b == SC_CAPS || b == SC_CTRL;
}

static void wait_any_key(void) {
    /* Drain whatever was pressed to get here. */
    (void)keyboard_poll_scancode();
    while (!keyboard_cancel_requested()) {
        u8 sc = keyboard_poll_scancode();
        if (sc != 0 && !(sc & 0x80) && !is_modifier(sc)) return;
        __asm__ __volatile__("hlt");
    }
}

static u8 wait_press(void) {
    while (!keyboard_cancel_requested()) {
        u8 sc = keyboard_poll_scancode();
        if (sc != 0 && !(sc & 0x80) && !is_modifier(sc)) return sc;
        __asm__ __volatile__("hlt");
    }
    return 0;
}

// Inventory Management
static void inventory_add(int skin_idx) {
    if (inv_count >= INV_CAP) {
        /* Drop the oldest entry to make room. */
        for (int i = 1; i < INV_CAP; i++) inventory[i - 1] = inventory[i];
        inv_count = INV_CAP - 1;
    }
    inventory[inv_count].skin_idx = (u16)skin_idx;
    inventory[inv_count].seq      = (u16)(inv_seq_counter++);
    inv_count++;
}

/* Write a single character at the current cursor using the given attribute,
 * advancing the cursor (and scrolling if needed). */
static void kputc_attr(char c, u8 attr) {
    int off = get_cursor_offset();
    int r = get_offset_row(off);
    int cc = get_offset_col(off);
    print_char(c, cc, r, (char)attr);
}

static void kprint_attr(const char *s, u8 attr) {
    for (int i = 0; s[i] != '\0'; i++) kputc_attr(s[i], attr);
}

void gamble_inventory(int sort_recent) {
    if (inv_count == 0) {
        kprint("\n(inventory is empty - try `gamble` to open a case)");
        return;
    }

    int order[INV_CAP];
    for (int i = 0; i < inv_count; i++) order[i] = i;

    /* Insertion sort; N <= 256. */
    if (sort_recent) {
        for (int i = 1; i < inv_count; i++) {
            int v = order[i];
            u16 vs = inventory[v].seq;
            int j = i - 1;
            while (j >= 0 && inventory[order[j]].seq < vs) {
                order[j + 1] = order[j];
                j--;
            }
            order[j + 1] = v;
        }
    } else {
        for (int i = 1; i < inv_count; i++) {
            int v = order[i];
            u8  vr = skins[inventory[v].skin_idx].rarity;
            u16 vs = inventory[v].seq;
            int j = i - 1;
            while (j >= 0) {
                u8  jr = skins[inventory[order[j]].skin_idx].rarity;
                u16 js = inventory[order[j]].seq;
                if (jr < vr || (jr == vr && js < vs)) {
                    order[j + 1] = order[j];
                    j--;
                } else break;
            }
            order[j + 1] = v;
        }
    }

    char nb[16];
    int_to_ascii(inv_count, nb);
    kprint("\nINVENTORY (");
    kprint(nb);
    kprint(sort_recent ? " items, by recent)" : " items, by rarity)");
    kprint("\n(sort by most recent with: inventory recent)");

    for (int i = 0; i < inv_count; i++) {
        const skin_t *s = &skins[inventory[order[i]].skin_idx];
        u8 attr = rarity_color[s->rarity];

        kprint("\n  [");
        /* Rarity name padded to 9 chars, in rarity color. */
        char rbuf[10];
        const char *rn = rarity_name[s->rarity];
        int len = (int)strlen((char*)rn);
        for (int j = 0; j < 9; j++) rbuf[j] = (j < len) ? rn[j] : ' ';
        rbuf[9] = '\0';
        kprint_attr(rbuf, attr);
        kprint("] ");
        kprint_attr((char*)s->full, attr);
    }
}

// Main
static void try_open_case(int case_id, const char **status, u8 *status_color) {
    const case_t *cs = &cases[case_id];
    if (coins < cs->price) {
        *status = "** not enough coins **";
        *status_color = 0x0C;
        speaker_play_freq(220);
        delay_ms(120);
        speaker_stop();
        return;
    }

    coins -= cs->price;
    int winner = roll_skin(case_id);

    animate_case_open(case_id, winner);
    if (keyboard_cancel_requested()) return;
    reveal_winner(winner);
    if (keyboard_cancel_requested()) return;

    inventory_add(winner);
    wait_any_key();

    *status = 0;
    *status_color = 0;
}

static void show_inventory_screen(void) {
    fill_rect(0, 0, MAX_COLS, MAX_ROWS, ' ', WHITE_ON_BLACK);

    const char *title = "== INVENTORY ==";
    write_str((MAX_COLS - (int)strlen((char*)title)) / 2, 0, title, 0x0E);
    draw_hr(1, '=', 0x0E);
    draw_balance();

    int sort_recent = 0;
    while (!keyboard_cancel_requested()) {
        /* Repaint body on each toggle. */
        fill_rect(0, 2, MAX_COLS, MAX_ROWS - 4, ' ', WHITE_ON_BLACK);

        if (inv_count == 0) {
            const char *msg = "(empty - open a case to collect errors)";
            write_str((MAX_COLS - (int)strlen((char*)msg)) / 2,
                      12, msg, 0x07);
        } else {
            /* Build sort order */
            int order[INV_CAP];
            for (int i = 0; i < inv_count; i++) order[i] = i;
            if (sort_recent) {
                for (int i = 1; i < inv_count; i++) {
                    int v = order[i];
                    u16 vs = inventory[v].seq;
                    int j = i - 1;
                    while (j >= 0 && inventory[order[j]].seq < vs) {
                        order[j + 1] = order[j]; j--;
                    }
                    order[j + 1] = v;
                }
            } else {
                for (int i = 1; i < inv_count; i++) {
                    int v = order[i];
                    u8  vr = skins[inventory[v].skin_idx].rarity;
                    u16 vs = inventory[v].seq;
                    int j = i - 1;
                    while (j >= 0) {
                        u8  jr = skins[inventory[order[j]].skin_idx].rarity;
                        u16 js = inventory[order[j]].seq;
                        if (jr < vr || (jr == vr && js < vs)) {
                            order[j + 1] = order[j]; j--;
                        } else break;
                    }
                    order[j + 1] = v;
                }
            }

            char nb[16];
            int_to_ascii(inv_count, nb);
            int c = 3;
            write_str(c, 3, sort_recent ? "Sort: recent  " : "Sort: rarity  ", 0x0F);
            c += 14;
            write_str(c, 3, "(", 0x08); c++;
            write_str(c, 3, nb, 0x0E); c += (int)strlen(nb);
            write_str(c, 3, " items)", 0x08);

            /* Two-column list: up to 2 × 18 = 36 entries visible per page. */
            int max_rows = MAX_ROWS - 10;  /* 25 - 10 = 15 rows */
            int shown = inv_count;
            if (shown > max_rows * 2) shown = max_rows * 2;

            for (int i = 0; i < shown; i++) {
                const skin_t *s = &skins[inventory[order[i]].skin_idx];
                u8 attr = rarity_color[s->rarity];
                int col = (i < max_rows) ? 3 : 42;
                int rr  = 5 + (i % max_rows);
                put_at(col,     rr, '[', 0x07);
                write_padded(col + 1, rr, rarity_name[s->rarity], 9, attr);
                put_at(col + 10, rr, ']', 0x07);
                write_padded(col + 12, rr, s->full, 24, attr);
            }

            if (inv_count > shown) {
                char more[48] = "... and ";
                char extra[16];
                int_to_ascii(inv_count - shown, extra);
                int mk = (int)strlen(more);
                for (int i = 0; extra[i]; i++) more[mk++] = extra[i];
                const char *tail = " more";
                for (int i = 0; tail[i]; i++) more[mk++] = tail[i];
                more[mk] = '\0';
                write_str(3, MAX_ROWS - 4, more, 0x08);
            }
        }

        draw_hr(MAX_ROWS - 2, '-', 0x08);
        write_str(3, MAX_ROWS - 1,
                  "[R] toggle sort    [any other key] back",
                  0x07);
        park_cursor();

        u8 sc = wait_press();
        if (sc == 0) return;
        if (sc == SC_R) { sort_recent = !sort_recent; continue; }
        return;
    }
}

void gamble_run(void) {
    /* Raw mode so typing doesn't bleed into the shell; Ctrl+C still observed
     * via keyboard_cancel_requested(). */
    keyboard_clear_cancel();
    keyboard_set_raw_mode(1);
    __asm__ __volatile__("sti");

    const char *status = 0;
    u8 status_color = 0;

    while (!keyboard_cancel_requested()) {
        draw_menu(status, status_color);
        status = 0;
        status_color = 0;

        u8 sc = wait_press();
        if (sc == 0) break;

        int case_id = -1;
        switch (sc) {
            case SC_1: case_id = 0; break;
            case SC_2: case_id = 1; break;
            case SC_3: case_id = 2; break;
            case SC_4: case_id = 3; break;
            case SC_I:
                show_inventory_screen();
                continue;
            case SC_Q:
            case SC_ESC:
                goto exit;
            default:
                continue;
        }

        try_open_case(case_id, &status, &status_color);
    }

exit:
    speaker_stop();
    keyboard_set_raw_mode(0);
    keyboard_clear_cancel();
    (void)int_width;  /* kept for future right-alignment tweaks */
}
