#include "../include/keyboard.h"
#include "../include/ports.h"
#include "../../interrupts/include/isr.h"
#include "../include/screen.h"
#include <string.h>
#include <stddef.h>

#define BACKSPACE 0x0E
#define ENTER 0x1C
#define SPACEBAR 0x39

static char key_buffer[256] __attribute__((unused));

const int SC_MAX = 58;
const char *sc_name[] = { "ERROR", "Esc", "1", "2", "3", "4", "5", "6", 
    "7", "8", "9", "0", "-", "=", "Backspace", "Tab", "Q", "W", "E", 
        "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Lctrl", 
        "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "`", 
        "LShift", "\\", "Z", "X", "C", "V", "B", "N", "M", ",", ".", 
        "/", "RShift", "Keypad *", "LAlt", "Spacebar"};
const char sc_ascii[] = { '?', '?', '1', '2', '3', '4', '5', '6',     
    '7', '8', '9', '0', '-', '=', '\b', '?', 'Q', 'W', 'E', 'R', 'T', 'Y', 
        'U', 'I', 'O', 'P', '[', ']', '\n', '?', 'A', 'S', 'D', 'F', 'G', 
        'H', 'J', 'K', 'L', ';', '\'', '`', '?', '\\', 'Z', 'X', 'C', 'V', 
        'B', 'N', 'M', ',', '.', '/', '?', '?', '?', ' '};


int strcmp(char s1[], char s2[]) {
    int i;
    for (i = 0; s1[i] == s2[i]; i++) {
        if (s1[i] == '\0') return 0;
    }
    return s1[i] - s2[i];
}

void append(char s[], char n) {
    int len = strlen(s);
    s[len] = n;
    s[len+1] = '\0';
}

void backspace(char s[]) {
    int len = strlen(s);
    s[len-1] = '\0';
}


void user_input(char *input) {
    kprint("You said: ");
    kprint(input);
    kprint("\n> ");
}

// Global flag to indicate keyboard interrupt received
volatile int keyboard_interrupt_received = 0;

// Function to check and reset the keyboard interrupt flag
int check_keyboard_interrupt() {
    if (keyboard_interrupt_received) {
        keyboard_interrupt_received = 0;
        return 1;
    }
    return 0;
}

static volatile char key_queue = 0;   // 1-byte FIFO for demo

static void keyboard_callback(registers_t regs __attribute__((unused)))
{
    // Read the scancode from port 0x60
    u8 scancode = port_byte_in(0x60);
    
    // Send EOI to PIC
    port_byte_out(0x20, 0x20);

    // Only process key press (not key release - scancodes > 0x80 are releases)
    if (scancode <= 0x80) {
        // Convert scancode to ASCII character
        if (scancode < SC_MAX) {
            char c = sc_ascii[scancode];
            
            // Handle special keys
            if (scancode == BACKSPACE) {
                kprint_backspace();
            } else if (scancode == ENTER) {
                kprint("\n> ");
            } else if (scancode == SPACEBAR) {
                kprint(" ");
            } else {
                // Print the character
                char str[2] = {c, '\0'};
                kprint(str);
            }
        }
    }
    
    // Store the scancode for processing
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
    // Register the keyboard interrupt handler
    register_interrupt_handler(IRQ1, keyboard_callback);
    port_byte_out(0x64, 0xAE);  // Enable keyboard
}
