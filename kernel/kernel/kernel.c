#include <stdio.h>

#include <kernel/tty.h>
#include <drivers/screen.h>
#include <drivers/keyboard.h>
#include <drivers/ports.h>
#include <interrupts/isr.h>

void kernel_main(void) {
	terminal_initialize();

	kprint("=== nanopulseOS Kernel Starting ===\n");
	kprint("Step 1: Terminal initialized\n");
	
	// Test basic functionality
	kprint("Step 2: Testing basic print\n");
	
	// Test port I/O
	kprint("Step 3: Testing port I/O\n");
	u8 test_port = port_byte_in(0x60);
	kprint("Port 0x60 read: ");
	kprint("\n");
	
	kprint("Step 4: About to test keyboard init\n");
	
	// Test keyboard initialization
	init_keyboard();
	
	kprint("Step 5: Keyboard init completed\n");
	
	kprint("Step 6: About to enable interrupts\n");
	__asm__ __volatile__("sti");
	kprint("Step 7: Interrupts enabled\n");
	
	// Simple infinite loop with hlt
	kprint("Step 8: About to enter idle loop\n");
	
	kprint("Step 8: Kernel initialization complete\n");
	kprint("Welcome to nanopulseOS!\n");
	kprint("OS initialization successful!\n");
	kprint("System will now halt (debug).\n");
	
	// Just halt the system
	__asm__ __volatile__("cli");  // Disable interrupts
	__asm__ __volatile__("hlt");  // Halt the CPU
}
