#include "../include/keyboard.h"
#include "../include/ports.h"
#include "../../interrupts/include/isr.h"
#include <drivers/screen.h>
#include "../../kernel/include/kernel/shell.h"
#include <string.h>
#include <stddef.h>

#define BACKSPACE   0x0E
#define ENTER       0x1C
#define SPACEBAR    0x39
#define LSHIFT      0x2A
#define RSHIFT      0x36
#define LSHIFT_REL  0xAA
#define RSHIFT_REL  0xB6
#define CAPS_LOCK   0x3A
#define LCTRL       0x1D
#define LCTRL_REL   0x9D
#define C_KEY       0x2E

char key_buffer[256];

static int shift_pressed = 0;
static int caps_active   = 0;
static int ctrl_pressed  = 0;
static int raw_mode      = 0;
static volatile u8  raw_last_scancode = 0;
static volatile int cancel_requested  = 0;

const int SC_MAX = 58;

/* Unshifted ASCII for scancodes 0x00–0x39 */
const char sc_ascii[] = {
    0,    0,    '1',  '2',  '3',  '4',  '5',  '6',   /* 00-07 */
    '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',  /* 08-0F */
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',   /* 10-17 */
    'o',  'p',  '[',  ']',  '\n', 0,    'a',  's',   /* 18-1F */
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',   /* 20-27 */
    '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',   /* 28-2F */
    'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',   /* 30-37 */
    0,    ' '                                          /* 38-39 */
};

/* Shifted ASCII for the same scancodes */
const char sc_ascii_shift[] = {
    0,    0,    '!',  '@',  '#',  '$',  '%',  '^',   /* 00-07 */
    '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',  /* 08-0F */
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',   /* 10-17 */
    'O',  'P',  '{',  '}',  '\n', 0,    'A',  'S',   /* 18-1F */
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',   /* 20-27 */
    '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',   /* 28-2F */
    'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',   /* 30-37 */
    0,    ' '                                          /* 38-39 */
};

static char resolve_char(u8 scancode) {
    char base = sc_ascii[scancode];
    if (base == 0) return 0;
    if (base >= 'a' && base <= 'z') {
        /* XOR: either shift or caps (not both) produces uppercase */
        if (shift_pressed ^ caps_active) return base - 32;
        return base;
    }
    /* Non-letter: use shift table for symbols/numbers */
    return shift_pressed ? sc_ascii_shift[scancode] : base;
}

void append(char s[], char n) {
    int len = strlen(s);
    s[len] = n;
    s[len+1] = '\0';
}

void backspace(char s[]) {
    int len = strlen(s);
    if (len > 0) s[len-1] = '\0';
}

/* Global flag to indicate keyboard interrupt received */
volatile int keyboard_interrupt_received = 0;

int check_keyboard_interrupt() {
    if (keyboard_interrupt_received) {
        keyboard_interrupt_received = 0;
        return 1;
    }
    return 0;
}

static volatile char key_queue = 0;

static void keyboard_callback(registers_t regs __attribute__((unused)))
{
    u8 scancode = port_byte_in(0x60);
    port_byte_out(0x20, 0x20); /* EOI */

    /* Track modifier state regardless of mode. */
    if (scancode == LSHIFT || scancode == RSHIFT) { shift_pressed = 1; }
    else if (scancode == LSHIFT_REL || scancode == RSHIFT_REL) { shift_pressed = 0; }
    else if (scancode == LCTRL) { ctrl_pressed = 1; }
    else if (scancode == LCTRL_REL) { ctrl_pressed = 0; }

    /*
     * Detected here (before mode-specific branches) so it is never lost to 
     * a race with hlt/poll timing. Raising the flag inside the ISR means 
     * any chord ordering — press C then Ctrl, release Ctrl before the app 
     * polls, etc. — still registers.
     */
    if (scancode == C_KEY && ctrl_pressed) {
        cancel_requested = 1;
        if (!raw_mode) {
            /* Shell-mode behavior: abandon the current input line and give
             * the user a fresh prompt, like a terminal SIGINT. */
            for (int i = 0; i < 256; i++) key_buffer[i] = '\0';
            kprint("^C\n> ");
        }
        key_queue = scancode;
        keyboard_interrupt_received = 1;
        return;
    }

    /* In raw mode, skip text rendering and shell dispatch; just expose the
     * scancode so graphics-mode apps can poll it. */
    if (raw_mode) {
        raw_last_scancode = scancode;
        key_queue = scancode;
        keyboard_interrupt_received = 1;
        return;
    }

    if (scancode == LSHIFT || scancode == RSHIFT ||
        scancode == LSHIFT_REL || scancode == RSHIFT_REL ||
        scancode == LCTRL || scancode == LCTRL_REL) {
        key_queue = scancode;
        keyboard_interrupt_received = 1;
        return;
    }

    /* Only process key presses, not releases (release codes have bit 7 set) */
    if (scancode & 0x80) {
        key_queue = scancode;
        keyboard_interrupt_received = 1;
        return;
    }

    /* Caps Lock toggle */
    if (scancode == CAPS_LOCK) {
        caps_active = !caps_active;
        key_queue = scancode;
        keyboard_interrupt_received = 1;
        return;
    }

    if (scancode < SC_MAX) {
        char c = resolve_char(scancode);
        if (c == 0) {
            key_queue = scancode;
            keyboard_interrupt_received = 1;
            return;
        }

        int offset = get_cursor_offset() - 2;
        int col    = get_offset_col(offset);

        if (scancode == BACKSPACE) {
            kprint_backspace();
        } else if (scancode == ENTER) {
            key_buffer[col] = '\0';
            shell_execute_command(key_buffer);
            kprint("\n> ");
        } else if (scancode == SPACEBAR) {
            kprint(" ");
            if (col - 1 >= 0 && col - 1 < 255) key_buffer[col - 1] = ' ';
        } else {
            char str[2] = {c, '\0'};
            kprint(str);
            if (col - 1 >= 0 && col - 1 < 255) key_buffer[col - 1] = c;
        }
    }

    key_queue = scancode;
    keyboard_interrupt_received = 1;
}

char keyboard_getchar(void)
{
    char c = key_queue;
    key_queue = 0;
    return c;
}

void keyboard_init(void)
{
    register_interrupt_handler(IRQ1, keyboard_callback);
    port_byte_out(0x64, 0xAE); /* Enable keyboard */
}

void keyboard_set_raw_mode(int on) {
    raw_mode = on ? 1 : 0;
    raw_last_scancode = 0;
}

u8 keyboard_poll_scancode(void) {
    u8 sc = raw_last_scancode;
    raw_last_scancode = 0;
    return sc;
}

int keyboard_ctrl_down(void) { return ctrl_pressed; }

int  keyboard_cancel_requested(void) { return cancel_requested; }
void keyboard_clear_cancel(void)     { cancel_requested = 0; }
