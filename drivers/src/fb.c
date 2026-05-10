#include "../include/fb.h"
#include "../include/font.h"

u8 vga_shadow[FB_TEXT_COLS * FB_TEXT_ROWS * 2];

static u8  *fb_base   = 0;
static u32  fb_pitch  = 0;   /* bytes per row */
static u32  fb_width  = 0;
static u32  fb_height = 0;
static u32  fb_bypp   = 0;   /* bytes per pixel - 3 (24bpp) or 4 (32bpp) */
static int  fb_ok     = 0;
static int  fb_suspended = 0;

/* Origin in the framebuffer where the 80x25 grid is centered. */
static u32  fb_origin_x = 0;
static u32  fb_origin_y = 0;

/* VGA 16-color palette → 0x00RRGGBB. */
static const u32 vga_palette[16] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
    0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
    0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
};

int fb_available(void) { return fb_ok; }

int fb_init(u32 fb_addr_lo, u32 pitch, u32 width, u32 height, u8 bpp) {
    if (!fb_addr_lo || (bpp != 32 && bpp != 24)) return 0;
    fb_base   = (u8 *)fb_addr_lo;
    fb_pitch  = pitch;
    fb_width  = width;
    fb_height = height;
    fb_bypp   = bpp / 8;

    u32 grid_w = FB_TEXT_COLS * FB_GLYPH_W;
    u32 grid_h = FB_TEXT_ROWS * FB_GLYPH_H;
    fb_origin_x = (width  > grid_w) ? (width  - grid_w) / 2 : 0;
    fb_origin_y = (height > grid_h) ? (height - grid_h) / 2 : 0;

    /* Clear screen to black. */
    for (u32 y = 0; y < height; y++) {
        u8 *row = fb_base + y * pitch;
        for (u32 x = 0; x < width * fb_bypp; x++) row[x] = 0;
    }

    fb_ok = 1;
    return 1;
}

static inline void put_pixel(u32 px, u32 py, u32 color) {
    u8 *p = fb_base + py * fb_pitch + px * fb_bypp;
    p[0] = (u8)(color);          /* B */
    p[1] = (u8)(color >> 8);     /* G */
    p[2] = (u8)(color >> 16);    /* R */
    if (fb_bypp == 4) p[3] = 0;
}

static void draw_glyph(int col, int row, u8 ch, u8 attr) {
    u32 fg = vga_palette[attr & 0x0F];
    u32 bg = vga_palette[(attr >> 4) & 0x0F];
    const u8 *glyph = &vga_font[(u32)ch * FB_GLYPH_H];

    u32 px0 = fb_origin_x + (u32)col * FB_GLYPH_W;
    u32 py0 = fb_origin_y + (u32)row * FB_GLYPH_H;

    for (int y = 0; y < FB_GLYPH_H; y++) {
        u8 bits = glyph[y];
        for (int x = 0; x < FB_GLYPH_W; x++) {
            put_pixel(px0 + x, py0 + y, (bits & (0x80 >> x)) ? fg : bg);
        }
    }
}

void fb_present(void) {
    if (!fb_ok || fb_suspended) return;
    for (int row = 0; row < FB_TEXT_ROWS; row++) {
        for (int col = 0; col < FB_TEXT_COLS; col++) {
            int off = (row * FB_TEXT_COLS + col) * 2;
            u8 ch   = vga_shadow[off];
            u8 attr = vga_shadow[off + 1];
            if (ch == 0) ch = ' ';
            draw_glyph(col, row, ch, attr);
        }
    }
}

u32  fb_get_width(void)  { return fb_width; }
u32  fb_get_height(void) { return fb_height; }

void fb_put_pixel(u32 x, u32 y, u32 color) {
    if (!fb_ok) return;
    if (x >= fb_width || y >= fb_height) return;
    put_pixel(x, y, color);
}

void fb_clear(u32 color) {
    if (!fb_ok) return;
    for (u32 y = 0; y < fb_height; y++) {
        for (u32 x = 0; x < fb_width; x++) {
            put_pixel(x, y, color);
        }
    }
}

void fb_set_suspended(int s) { fb_suspended = s; }
