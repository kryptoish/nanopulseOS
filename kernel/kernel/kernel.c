#include <stdio.h>

#include <kernel/tty.h>
#include <drivers/screen.h>
#include <drivers/keyboard.h>
#include <drivers/ports.h>
#include <interrupts/isr.h>

// External reference to scancode arrays from keyboard driver
extern const char sc_ascii[];
extern const int SC_MAX;

void kernel_main(void) {
	terminal_initialize();

	// Initialize interrupt system FIRST
	isr_install();
	kprint("Step 1: Interrupt system initialized\n");
	
	// Use only kprint to avoid text overlap
	kprint("=== nanopulseOS Kernel Starting ===\n");
	kprint("Step 2: Terminal initialized\n");
	
	// Test basic functionality
	kprint("Step 3: Testing basic print\n");
	
	// Test port I/O
	kprint("Step 4: Testing port I/O\n");
	kprint("Port I/O test completed\n");
	
	kprint("Step 5: About to test keyboard init\n");
	
	// Test keyboard initialization
	keyboard_init();
	
	kprint("Step 6: Keyboard init completed\n");
	
	kprint("Step 7: About to enable interrupts\n");
	__asm__ __volatile__("sti");
	kprint("Step 8: Interrupts enabled\n");
	
	kprint("Step 9: Kernel initialization complete\n");
	kprint("Welcome to nanopulseOS!\n");
	kprint("OS initialization successful!\n");
	kprint("> ");
	
	// Main loop - handle keyboard input
	while(1) {
		char sc = keyboard_getchar();
		if (sc) {
			// Convert scancode to ASCII safely
			if (sc <= SC_MAX) {
				char c = sc_ascii[(int)sc];
				if (c != '?') {
					char s[2] = { c, 0 };
					kprint(s);
				}
			}
		}
		__asm__ __volatile__("hlt");
	}
}
