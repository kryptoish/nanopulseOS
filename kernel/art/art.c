#include <kernel/art.h>
#include <drivers/vga.h>
#include <drivers/keyboard.h>
#include <drivers/types.h>
#include <interrupts/timer.h>

#define ESC_SC    0x01

/*
 * Read the CPU's timestamp counter. Combined with the PIT tick and the
 * memory already in 0xA0000 this is our entropy source for the fingerprint.
 */
static u64 rdtsc(void) {
    u32 lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((u64)hi << 32) | lo;
}

/*
 * xorshift64 — fast, tiny, good enough for visuals. Not cryptographic.
 */
static u64 prng_state;
static u64 xorshift(void) {
    u64 x = prng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    prng_state = x;
    return x;
}

static void seed_from_entropy(void) {
    u64 s = rdtsc();
    s ^= (u64)get_tick() * 0x9E3779B97F4A7C15ULL;
    /* Sample framebuffer noise — uninitialized video RAM has device-specific
     * patterns that contribute a small amount of per-machine entropy. */
    volatile u8 *fb = (volatile u8*)0xA0000;
    for (int i = 0; i < 64; i++) {
        s ^= (u64)fb[i * 257] << ((i * 7) & 63);
    }
    if (s == 0) s = 0xDEADBEEFCAFEBABEULL;
    prng_state = s;
}

/*
 * Build a palette: index 0 = black background, 1..255 are warm/cool gradients
 * picked from the PRNG so different boots get different color schemes.
 */
static void install_palette(void) {
    vga_set_palette(0, 0, 0, 0);
    for (int i = 1; i < 256; i++) {
        u64 r = xorshift();
        u8 base_r = (r >> 0)  & 0x3F;
        u8 base_g = (r >> 8)  & 0x3F;
        u8 base_b = (r >> 16) & 0x3F;
        /* Bias towards vivid mid-to-high values. */
        if (base_r < 16) base_r += 16;
        if (base_g < 16) base_g += 16;
        if (base_b < 16) base_b += 16;
        vga_set_palette((u8)i, base_r, base_g, base_b);
    }
}

/*
 * Draw a symmetric fingerprint. We generate an 80x100 tile from the PRNG,
 * then mirror it horizontally so the result looks like a unique emblem.
 * A few bright accent squares are added on top.
 */
static void render_fingerprint(void) {
    vga_fill(0);

    const int tile_w = 80;
    const int tile_h = 100;
    const int ox = (VGA_WIDTH / 2) - tile_w;   /* left edge of left half */
    const int oy = (VGA_HEIGHT - tile_h) / 2;

    for (int y = 0; y < tile_h; y++) {
        for (int x = 0; x < tile_w; x++) {
            u64 r = xorshift();
            /* Only paint a fraction of cells so the piece has negative space. */
            if ((r & 0x7) < 3) continue;
            u8 color = (u8)(((r >> 8) & 0xFF) | 1);

            /* 2x2 blocks for a chunkier, pixel-art feel. */
            int bx = ox + x * 1;
            int by = oy + y * 1;
            vga_put_pixel(bx, by, color);
            /* Mirror to the right half. */
            vga_put_pixel(VGA_WIDTH - 1 - bx, by, color);
        }
    }

    /* A few accent rectangles for visual variety. */
    for (int i = 0; i < 6; i++) {
        u64 r = xorshift();
        int w = 4 + (r & 7);
        int h = 4 + ((r >> 3) & 7);
        int x = (int)((r >> 8) % (tile_w - w));
        int y = (int)((r >> 16) % (tile_h - h));
        u8 color = (u8)(((r >> 24) & 0xFE) | 1);
        vga_fill_rect(ox + x, oy + y, w, h, color);
        vga_fill_rect(VGA_WIDTH - 1 - (ox + x + w), oy + y, w, h, color);
    }

    /* Thin frame so the art has a clear boundary. */
    u8 frame = (u8)((xorshift() & 0xFE) | 1);
    vga_fill_rect(ox - 2, oy - 2, tile_w * 2 + 4, 1, frame);
    vga_fill_rect(ox - 2, oy + tile_h + 1, tile_w * 2 + 4, 1, frame);
    vga_fill_rect(ox - 2, oy - 2, 1, tile_h + 4, frame);
    vga_fill_rect(ox + tile_w * 2 + 1, oy - 2, 1, tile_h + 4, frame);
}

void art_run(void) {
    seed_from_entropy();

    keyboard_clear_cancel();
    keyboard_set_raw_mode(1);
    vga_enter_graphics();
    install_palette();
    render_fingerprint();

    /*
     * We are reached from within the IRQ1 handler (Enter → shell_execute_command
     * → art_run), and the IDT uses 32-bit interrupt gates which clear IF on
     * entry. Without sti here, hlt would halt forever and no keyboard IRQ
     * would ever fire to set the cancel flag. The PIC was EOI'd in the outer
     * callback, so re-enabling is safe — nested keyboard IRQs just update
     * modifier flags and return.
     */
    __asm__ __volatile__("sti");

    /* Block until ESC or a system-wide Ctrl+C. hlt sleeps the CPU between
     * interrupts instead of spinning. */
    while (1) {
        if (keyboard_cancel_requested()) break;
        u8 sc = keyboard_poll_scancode();
        if (sc == ESC_SC) break;
        __asm__ __volatile__("hlt");
    }

    vga_exit_graphics();
    keyboard_set_raw_mode(0);
    keyboard_clear_cancel();
}
