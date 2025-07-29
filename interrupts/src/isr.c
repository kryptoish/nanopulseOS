#include "../include/isr.h"
#include "../include/idt.h"
#include "../../drivers/include/screen.h"
#include "../../drivers/include/ports.h"
#include "../../drivers/include/types.h"
#include <string.h>

isr_t interrupt_handlers[256];

// Simple test interrupt handler that does absolutely nothing
static void test_interrupt_handler(registers_t regs) {
    // Do absolutely nothing - just return immediately
    // Don't even cast the parameter
}

/* Can't do this with a loop because we need the address
 * of the function names */
void isr_install() {
    // Initialize all interrupt handlers to NULL
    for (int i = 0; i < 256; i++) {
        interrupt_handlers[i] = 0;
    }
    
    // Register a test handler for IRQ1 instead of the keyboard handler
    interrupt_handlers[33] = test_interrupt_handler;  // IRQ1 = interrupt 33
    
    set_idt_gate(0, (u32)isr0);
    set_idt_gate(1, (u32)isr1);
    set_idt_gate(2, (u32)isr2);
    set_idt_gate(3, (u32)isr3);
    set_idt_gate(4, (u32)isr4);
    set_idt_gate(5, (u32)isr5);
    set_idt_gate(6, (u32)isr6);
    set_idt_gate(7, (u32)isr7);
    set_idt_gate(8, (u32)isr8);
    set_idt_gate(9, (u32)isr9);
    set_idt_gate(10, (u32)isr10);
    set_idt_gate(11, (u32)isr11);
    set_idt_gate(12, (u32)isr12);
    set_idt_gate(13, (u32)isr13);
    set_idt_gate(14, (u32)isr14);
    set_idt_gate(15, (u32)isr15);
    set_idt_gate(16, (u32)isr16);
    set_idt_gate(17, (u32)isr17);
    set_idt_gate(18, (u32)isr18);
    set_idt_gate(19, (u32)isr19);
    set_idt_gate(20, (u32)isr20);
    set_idt_gate(21, (u32)isr21);
    set_idt_gate(22, (u32)isr22);
    set_idt_gate(23, (u32)isr23);
    set_idt_gate(24, (u32)isr24);
    set_idt_gate(25, (u32)isr25);
    set_idt_gate(26, (u32)isr26);
    set_idt_gate(27, (u32)isr27);
    set_idt_gate(28, (u32)isr28);
    set_idt_gate(29, (u32)isr29);
    set_idt_gate(30, (u32)isr30);
    set_idt_gate(31, (u32)isr31);

    // Remap the PIC to avoid conflicts with CPU exceptions
    kprint("Remapping PIC...\n");
    
    // ICW1: start initialization sequence
    port_byte_out(0x20, 0x11);
    port_byte_out(0xA0, 0x11);
    
    // ICW2: remap IRQ table
    port_byte_out(0x21, 0x20);  // Master PIC: IRQ 0-7 -> interrupts 32-39
    port_byte_out(0xA1, 0x28);  // Slave PIC: IRQ 8-15 -> interrupts 40-47
    
    // ICW3: tell PICs how they're cascaded
    port_byte_out(0x21, 0x04);
    port_byte_out(0xA1, 0x02);
    
    // ICW4: set 8086 mode
    port_byte_out(0x21, 0x01);
    port_byte_out(0xA1, 0x01);
    
    // Set interrupt masks (only enable keyboard IRQ1)
    port_byte_out(0x21, 0xFD);  // Master PIC: enable IRQ1 (keyboard), disable others
    port_byte_out(0xA1, 0xFF);  // Slave PIC: disable all
    
    kprint("PIC remapping complete\n");

    // Install the IRQs
    set_idt_gate(32, (u32)irq0);
    set_idt_gate(33, (u32)irq1);
    set_idt_gate(34, (u32)irq2);
    set_idt_gate(35, (u32)irq3);
    set_idt_gate(36, (u32)irq4);
    set_idt_gate(37, (u32)irq5);
    set_idt_gate(38, (u32)irq6);
    set_idt_gate(39, (u32)irq7);
    set_idt_gate(40, (u32)irq8);
    set_idt_gate(41, (u32)irq9);
    set_idt_gate(42, (u32)irq10);
    set_idt_gate(43, (u32)irq11);
    set_idt_gate(44, (u32)irq12);
    set_idt_gate(45, (u32)irq13);
    set_idt_gate(46, (u32)irq14);
    set_idt_gate(47, (u32)irq15);

    set_idt(); // Load with ASM
    
    kprint("ISR installation complete\n");
}

/* To print the message which defines every exception */
char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void isr_handler(registers_t r) {
    kprint("received interrupt: ");
    kprint("(number)\n");
    kprint(exception_messages[r.int_no]);
    kprint("\n");
}

void register_interrupt_handler(u8 n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

void irq_handler(registers_t r) {
    /* After every interrupt we need to send an EOI to the PICs
     * or they will not send another interrupt again */
    if (r.int_no >= 40) port_byte_out(0xA0, 0x20); /* slave */
    port_byte_out(0x20, 0x20); /* master */

    /* Handle the interrupt in a more modular way */
    if (interrupt_handlers[r.int_no] != 0) {
        isr_t handler = interrupt_handlers[r.int_no];
        handler(r);
    }
}


