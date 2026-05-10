#ifndef SCREEN_H
#define SCREEN_H

#include "types.h"
#include "fb.h"

#define VIDEO_ADDRESS (vga_shadow)
#define MAX_ROWS 25
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0f
#define RED_ON_WHITE 0xf4

/* Public kernel API */
void clear_screen();
void kprint_at(char *message, int col, int row);
void kprint(char *message);
void kprint_backspace();

/* Helper functions for cursor management */
int get_cursor_offset();
void set_cursor_offset(int offset);
int get_offset(int col, int row);
int get_offset_row(int offset);
int get_offset_col(int offset);
int print_char(char c, int col, int row, char attr);

#endif