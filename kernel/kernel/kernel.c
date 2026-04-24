#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/ramfs.h>
#include <drivers/screen.h>
#include <drivers/keyboard.h>
#include <drivers/ports.h>
#include <interrupts/isr.h>
#include <interrupts/timer.h>

extern const char sc_ascii[];
extern const int SC_MAX;

void kernel_main(void) {
	terminal_initialize();
	isr_install();
	kprint("=== nanopulseOS Kernel Starting ===\n");

	keyboard_init();
	kprint("Keyboard init success\n");

	init_timer(100); /* 100 Hz PIT tick - game loop/animation base. */
	kprint("Timer init (100 Hz)\n");

	ramfs_init();
	kprint("RAM filesystem init\n");

	__asm__ __volatile__("sti");
	kprint("Interrupts enabled\n");
	kprint("Welcome to nanopulseOS!\n");
	kprint("OS initialization successful!\n");
	kprint("> ");
	
	while(1) {
        // Check for keyboard input
		if (check_keyboard_interrupt()) {
			// Keyboard input is handled in the interrupt callback
			// This just ensures we process any pending input
		}
		__asm__ __volatile__("hlt");
	}
}
