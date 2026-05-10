#include "../include/vga.h"
#include "../include/fb.h"
#include "../include/types.h"

/* Software palette: index → 24-bit packed RGB (matches fb's color format). */
static u32 software_palette[256];

/* Computed at vga_enter_graphics() time based on framebuffer dimensions. */
static u32 scale = 1;
static u32 grid_origin_x = 0;
static u32 grid_origin_y = 0;

static void compute_layout(void) {
    u32 fbw = fb_get_width();
    u32 fbh = fb_get_height();
    u32 sx = fbw / VGA_WIDTH;
    u32 sy = fbh / VGA_HEIGHT;
    u32 s = (sx < sy) ? sx : sy;
    if (s < 1) s = 1;
    if (s > 6) s = 6;   /* cap to keep the piece readable */
    scale = s;
    u32 w = VGA_WIDTH  * scale;
    u32 h = VGA_HEIGHT * scale;
    grid_origin_x = (fbw > w) ? (fbw - w) / 2 : 0;
    grid_origin_y = (fbh > h) ? (fbh - h) / 2 : 0;
}

void vga_enter_graphics(void) {
    /* Stop the text renderer so it doesn't overwrite our pixels. */
    fb_set_suspended(1);
    fb_clear(0);
    compute_layout();
}

void vga_exit_graphics(void) {
    /* Wipe the screen so leftover art doesn't bleed through, then let the
     * text renderer start drawing again on its next tick. */
    fb_clear(0);
    fb_set_suspended(0);
}

static void scaled_block(u32 px, u32 py, u32 color) {
    u32 fx = grid_origin_x + px * scale;
    u32 fy = grid_origin_y + py * scale;
    for (u32 dy = 0; dy < scale; dy++) {
        for (u32 dx = 0; dx < scale; dx++) {
            fb_put_pixel(fx + dx, fy + dy, color);
        }
    }
}

void vga_put_pixel(int x, int y, u8 color) {
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) return;
    scaled_block((u32)x, (u32)y, software_palette[color]);
}

void vga_fill(u8 color) {
    u32 c = software_palette[color];
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            scaled_block((u32)x, (u32)y, c);
        }
    }
}

void vga_fill_rect(int x, int y, int w, int h, u8 color) {
    u32 c = software_palette[color];
    for (int j = 0; j < h; j++) {
        int py = y + j;
        if (py < 0 || py >= VGA_HEIGHT) continue;
        for (int i = 0; i < w; i++) {
            int px = x + i;
            if (px < 0 || px >= VGA_WIDTH) continue;
            scaled_block((u32)px, (u32)py, c);
        }
    }
}

void vga_set_palette(u8 index, u8 r, u8 g, u8 b) {
    /* DAC values are 6-bit (0-63). Expand to full 8-bit. */
    u32 R = (u32)(r & 0x3F) << 2; R |= R >> 6;
    u32 G = (u32)(g & 0x3F) << 2; G |= G >> 6;
    u32 B = (u32)(b & 0x3F) << 2; B |= B >> 6;
    software_palette[index] = (R << 16) | (G << 8) | B;
}
