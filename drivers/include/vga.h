#ifndef VGA_H
#define VGA_H

#include "types.h"

#define VGA_WIDTH  320
#define VGA_HEIGHT 200

/* Suspend the text renderer and prepare for graphics-mode drawing. */
void vga_enter_graphics(void);

/* Wipe the framebuffer and resume the text renderer. */
void vga_exit_graphics(void);

/* Draw primitives (graphics mode only). */
void vga_put_pixel(int x, int y, u8 color);
void vga_fill(u8 color);
void vga_fill_rect(int x, int y, int w, int h, u8 color);

/* Program a single DAC palette slot (each channel is 6 bits, 0–63). */
void vga_set_palette(u8 index, u8 r, u8 g, u8 b);

#endif
