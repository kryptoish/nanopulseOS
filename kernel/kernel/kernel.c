#include <stdio.h>

#include <kernel/tty.h>
#include <drivers/screen.h>

void kernel_main(void) {
	terminal_initialize();
	printf("Welcome to nanopulseOS!\n");
    kprint("Type something, it will go through the kernel\n Type END to halt the CPU\n> ");
}
