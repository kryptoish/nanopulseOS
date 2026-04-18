#ifndef VGA_H
#define VGA_H

#include "types.h"

#define VGA_WIDTH  320
#define VGA_HEIGHT 200
#define VGA_FRAMEBUFFER ((u8*)0xA0000)

/* Switch the adapter into 320x200x256 linear chained Mode 13h. */
void vga_enter_graphics(void);

/* Restore 80x25 color text mode and the BIOS font. */
void vga_exit_graphics(void);

/* Draw primitives (graphics mode only). */
void vga_put_pixel(int x, int y, u8 color);
void vga_fill(u8 color);
void vga_fill_rect(int x, int y, int w, int h, u8 color);

/* Program a single DAC palette slot (each channel is 6 bits, 0–63). */
void vga_set_palette(u8 index, u8 r, u8 g, u8 b);

#endif
