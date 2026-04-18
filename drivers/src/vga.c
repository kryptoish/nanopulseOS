#include "../include/vga.h"
#include "../include/ports.h"
#include "../include/types.h"

#define VGA_AC_INDEX      0x3C0
#define VGA_AC_WRITE      0x3C0
#define VGA_MISC_WRITE    0x3C2
#define VGA_SEQ_INDEX     0x3C4
#define VGA_SEQ_DATA      0x3C5
#define VGA_DAC_READ_IDX  0x3C7
#define VGA_DAC_WRITE_IDX 0x3C8
#define VGA_DAC_DATA      0x3C9
#define VGA_GC_INDEX      0x3CE
#define VGA_GC_DATA       0x3CF
#define VGA_CRTC_INDEX    0x3D4
#define VGA_CRTC_DATA     0x3D5
#define VGA_INSTAT_READ   0x3DA

/* Register dump for 320x200x256 Mode 13h. */
static u8 mode13h_regs[61] = {
    /* MISC */
    0x63,
    /* SEQ: 5 bytes */
    0x03, 0x01, 0x0F, 0x00, 0x0E,
    /* CRTC: 25 bytes */
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
    0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
    0xFF,
    /* GC: 9 bytes */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
    0xFF,
    /* AC: 21 bytes */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x41, 0x00, 0x0F, 0x00, 0x00
};

/* Register dump for 80x25 16-color text mode. */
static u8 text_mode_regs[61] = {
    /* MISC */
    0x67,
    /* SEQ */
    0x03, 0x00, 0x03, 0x00, 0x02,
    /* CRTC */
    0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
    0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0x50,
    0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
    0xFF,
    /* GC */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
    0xFF,
    /* AC */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x0C, 0x00, 0x0F, 0x08, 0x00
};

/* Saved 8KB of plane 2 (text-mode font storage). */
static u8 saved_font[8192];
static int font_saved = 0;

/* Saved DAC palette so the original text-mode colors come back on exit. */
static u8 saved_palette[256 * 3];
static int palette_saved = 0;

static void save_palette(void) {
    port_byte_out(VGA_DAC_READ_IDX, 0);
    for (int i = 0; i < 256 * 3; i++) saved_palette[i] = port_byte_in(VGA_DAC_DATA);
    palette_saved = 1;
}

static void restore_palette(void) {
    if (!palette_saved) return;
    port_byte_out(VGA_DAC_WRITE_IDX, 0);
    for (int i = 0; i < 256 * 3; i++) port_byte_out(VGA_DAC_DATA, saved_palette[i]);
}

static void write_regs(const u8 *regs) {
    u8 i;

    /* MISC */
    port_byte_out(VGA_MISC_WRITE, *regs++);

    /* SEQ */
    for (i = 0; i < 5; i++) {
        port_byte_out(VGA_SEQ_INDEX, i);
        port_byte_out(VGA_SEQ_DATA, *regs++);
    }

    /* Unlock CRTC (clear bit 7 of CRTC index 0x11). */
    port_byte_out(VGA_CRTC_INDEX, 0x03);
    port_byte_out(VGA_CRTC_DATA, port_byte_in(VGA_CRTC_DATA) | 0x80);
    port_byte_out(VGA_CRTC_INDEX, 0x11);
    port_byte_out(VGA_CRTC_DATA, port_byte_in(VGA_CRTC_DATA) & ~0x80);

    /* CRTC */
    for (i = 0; i < 25; i++) {
        port_byte_out(VGA_CRTC_INDEX, i);
        /* Honor the unlock we just did. */
        u8 val = *regs++;
        if (i == 0x03) val |= 0x80;
        if (i == 0x11) val &= ~0x80;
        port_byte_out(VGA_CRTC_DATA, val);
    }

    /* GC */
    for (i = 0; i < 9; i++) {
        port_byte_out(VGA_GC_INDEX, i);
        port_byte_out(VGA_GC_DATA, *regs++);
    }

    /* AC: need to reset flip-flop via port 0x3DA read, then write index then data. */
    for (i = 0; i < 21; i++) {
        (void)port_byte_in(VGA_INSTAT_READ);
        port_byte_out(VGA_AC_INDEX, i);
        port_byte_out(VGA_AC_WRITE, *regs++);
    }

    /* Lock 16-color palette and re-enable screen output. */
    (void)port_byte_in(VGA_INSTAT_READ);
    port_byte_out(VGA_AC_INDEX, 0x20);
}

/*
 * Expose plane 2 at 0xA0000 so we can save/restore the 8x16 character ROM
 * the VGA BIOS loaded into it. Without this, returning to text mode gives
 * garbage glyphs because Mode 13h overwrites plane 2.
 */
static void font_access_begin(void) {
    /* Sequencer: write to plane 2 only, sequential memory. */
    port_byte_out(VGA_SEQ_INDEX, 0x02); port_byte_out(VGA_SEQ_DATA, 0x04);
    port_byte_out(VGA_SEQ_INDEX, 0x04); port_byte_out(VGA_SEQ_DATA, 0x07);
    /* Graphics Controller: read plane 2, no odd/even, A0000-AFFFF map. */
    port_byte_out(VGA_GC_INDEX, 0x04); port_byte_out(VGA_GC_DATA, 0x02);
    port_byte_out(VGA_GC_INDEX, 0x05); port_byte_out(VGA_GC_DATA, 0x00);
    port_byte_out(VGA_GC_INDEX, 0x06); port_byte_out(VGA_GC_DATA, 0x04);
}

static void font_access_end_text(void) {
    /* Restore text-mode plane settings. */
    port_byte_out(VGA_SEQ_INDEX, 0x02); port_byte_out(VGA_SEQ_DATA, 0x03);
    port_byte_out(VGA_SEQ_INDEX, 0x04); port_byte_out(VGA_SEQ_DATA, 0x02);
    port_byte_out(VGA_GC_INDEX, 0x04); port_byte_out(VGA_GC_DATA, 0x00);
    port_byte_out(VGA_GC_INDEX, 0x05); port_byte_out(VGA_GC_DATA, 0x10);
    port_byte_out(VGA_GC_INDEX, 0x06); port_byte_out(VGA_GC_DATA, 0x0E);
}

static void save_font(void) {
    volatile u8 *plane = (volatile u8*)0xA0000;
    font_access_begin();
    for (int i = 0; i < 8192; i++) saved_font[i] = plane[i];
    font_saved = 1;
}

static void restore_font(void) {
    if (!font_saved) return;
    volatile u8 *plane = (volatile u8*)0xA0000;
    font_access_begin();
    for (int i = 0; i < 8192; i++) plane[i] = saved_font[i];
    font_access_end_text();
}

void vga_enter_graphics(void) {
    if (!font_saved) save_font();
    if (!palette_saved) save_palette();
    write_regs(mode13h_regs);
}

void vga_exit_graphics(void) {
    write_regs(text_mode_regs);
    restore_font();
    restore_palette();
}

void vga_put_pixel(int x, int y, u8 color) {
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) return;
    VGA_FRAMEBUFFER[y * VGA_WIDTH + x] = color;
}

void vga_fill(u8 color) {
    int n = VGA_WIDTH * VGA_HEIGHT;
    for (int i = 0; i < n; i++) VGA_FRAMEBUFFER[i] = color;
}

void vga_fill_rect(int x, int y, int w, int h, u8 color) {
    for (int j = 0; j < h; j++) {
        int py = y + j;
        if (py < 0 || py >= VGA_HEIGHT) continue;
        for (int i = 0; i < w; i++) {
            int px = x + i;
            if (px < 0 || px >= VGA_WIDTH) continue;
            VGA_FRAMEBUFFER[py * VGA_WIDTH + px] = color;
        }
    }
}

void vga_set_palette(u8 index, u8 r, u8 g, u8 b) {
    port_byte_out(VGA_DAC_WRITE_IDX, index);
    port_byte_out(VGA_DAC_DATA, r & 0x3F);
    port_byte_out(VGA_DAC_DATA, g & 0x3F);
    port_byte_out(VGA_DAC_DATA, b & 0x3F);
}
