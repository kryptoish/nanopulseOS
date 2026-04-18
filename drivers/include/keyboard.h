#include "types.h"

extern char key_buffer[256];

void init_keyboard();
void keyboard_init();
char keyboard_getchar();
int check_keyboard_interrupt();

/*
 * Suspend/resume the in-callback text rendering and shell dispatch so the
 * keyboard can be used from graphics-mode apps. While suspended, raw scancodes
 * are available through keyboard_poll_scancode().
 */
void keyboard_set_raw_mode(int on);

/* Non-blocking: returns 0 if nothing pending, else the last scancode. */
u8 keyboard_poll_scancode(void);

/* 1 while ctrl is held, 0 otherwise. Updated in the keyboard callback. */
int keyboard_ctrl_down(void);

/*
 * System-wide Ctrl+C cancel. The keyboard callback sets this flag the moment
 * Ctrl+C is detected, so long-running commands can poll for it without
 * needing a ring buffer — no race even if the user chords the keys quickly.
 */
int  keyboard_cancel_requested(void);
void keyboard_clear_cancel(void);
