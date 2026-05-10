#ifndef FB_H
#define FB_H

#include "types.h"

/* 80x25 text grid that mirrors what kernel writes via screen.h */
#define FB_TEXT_COLS 80
#define FB_TEXT_ROWS 25
#define FB_GLYPH_W   8
#define FB_GLYPH_H   16

/* Shadow text buffer (char/attr pairs) used in place of 0xB8000. */
extern u8 vga_shadow[FB_TEXT_COLS * FB_TEXT_ROWS * 2];

/* Init from multiboot2 framebuffer tag. Returns 1 on success. */
int  fb_init(u32 fb_addr_lo, u32 fb_pitch, u32 fb_width, u32 fb_height, u8 bpp);

/* Render the shadow buffer to the framebuffer. */
void fb_present(void);

/* Whether fb_init succeeded - if not, kernel falls back to writing 0xB8000 only. */
int  fb_available(void);

/* Direct framebuffer access (for graphics modes like the art command). */
u32  fb_get_width(void);
u32  fb_get_height(void);
void fb_put_pixel(u32 x, u32 y, u32 color);
void fb_clear(u32 color);

/* Suspend/resume fb_present so the text renderer doesn't trample
 * graphics-mode rendering. */
void fb_set_suspended(int s);

#endif
