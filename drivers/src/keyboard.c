#include "../include/keyboard.h"
#include "../include/ports.h"
#include "../../interrupts/include/isr.h"
#include "../include/screen.h"
#include "../../libc/include/string.h"
#include "../../libc/include/function.h"

#define BACKSPACE 0x0E
#define ENTER 0x1C

static char key_buffer[256];

const int SC_MAX = 57;
const char *sc_name[] = { "ERROR", "Esc", "1", "2", "3", "4", "5", "6", 
    "7", "8", "9", "0", "-", "=", "Backspace", "Tab", "Q", "W", "E", 
        "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Lctrl", 
        "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "`", 
        "LShift", "\\", "Z", "X", "C", "V", "B", "N", "M", ",", ".", 
        "/", "RShift", "Keypad *", "LAlt", "Spacebar"};
const char sc_ascii[] = { '?', '?', '1', '2', '3', '4', '5', '6',     
    '7', '8', '9', '0', '-', '=', '?', '?', 'Q', 'W', 'E', 'R', 'T', 'Y', 
        'U', 'I', 'O', 'P', '[', ']', '?', '?', 'A', 'S', 'D', 'F', 'G', 
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

static void keyboard_callback(registers_t regs)
{
    // Do absolutely nothing - just acknowledge the interrupt
    // Don't even read the scancode for now
    (void)regs;  // Suppress unused parameter warning
}

char keyboard_getchar(void)
{
    char c = key_queue;
    key_queue = 0;
    return c;
}

void keyboard_init(void)
{
    kprint("Initializing keyboard...\n");
    
    // Temporarily disable keyboard handler registration
    // register_interrupt_handler(IRQ1, keyboard_callback);
    kprint("Interrupt handler registration disabled for testing\n");
    
    // Minimal keyboard enable - just send the enable command
    kprint("Enabling keyboard...\n");
    port_byte_out(0x64, 0xAE);  // Enable keyboard
    
    kprint("Keyboard initialized!\n");
}
