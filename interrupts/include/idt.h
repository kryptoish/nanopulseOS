#ifndef IDT_H
#define IDT_H

#include "../../drivers/include/types.h"

/* Segment selectors */
#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define TSS_SEG   0x28

/* How every interrupt gate (handler) is defined */
typedef struct {
    u16 low_offset; /* Lower 16 bits of handler function address */
    u16 sel; /* Kernel segment selector */
    u8 always0;
    /* First byte
     * Bit 7: "Interrupt is present"
     * Bits 6-5: Privilege level of caller (0=kernel..3=user)
     * Bit 4: Set to 0 for interrupt gates
     * Bits 3-0: bits 1110 = decimal 14 = "32 bit interrupt gate" */
    u8 flags; 
    u16 high_offset; /* Higher 16 bits of handler function address */
} __attribute__((packed)) idt_gate_t ;

/* A pointer to the array of interrupt handlers.
 * Assembly instruction 'lidt' will read it */
typedef struct {
    u16 limit;
    u32 base;
} __attribute__((packed)) idt_register_t;

#define IDT_ENTRIES 256
extern idt_gate_t idt[IDT_ENTRIES];
extern idt_register_t idt_reg;

/* GDT structures */
typedef struct {
    u16 limit_low;
    u16 base_low;
    u8 base_middle;
    u8 access;
    u8 granularity;
    u8 base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    u16 limit;
    u32 base;
} __attribute__((packed)) gdt_register_t;

/* TSS structure for 32-bit protected mode */
typedef struct {
    u32 prev_tss;   // The previous TSS - if we used hardware task switching this would form a linked list.
    u32 esp0;       // The stack pointer to load when we change to kernel mode.
    u32 ss0;        // The stack segment to load when we change to kernel mode.
    u32 esp1;       // Unused...
    u32 ss1;
    u32 esp2;
    u32 ss2;
    u32 cr3;
    u32 eip;
    u32 eflags;
    u32 eax;
    u32 ecx;
    u32 edx;
    u32 ebx;
    u32 esp;
    u32 ebp;
    u32 esi;
    u32 edi;
    u32 es;         // The value to load into ES when we change to kernel mode.
    u32 cs;         // The value to load into CS when we change to kernel mode.
    u32 ss;         // The value to load into SS when we change to kernel mode.
    u32 ds;         // The value to load into DS when we change to kernel mode.
    u32 fs;         // The value to load into FS when we change to kernel mode.
    u32 gs;         // The value to load into GS when we change to kernel mode.
    u32 ldt;        // Unused...
    u16 trap;
    u16 iomap_base;
} __attribute__((packed)) tss_entry_t;

extern gdt_entry_t gdt[7];
extern gdt_register_t gdt_reg;
extern tss_entry_t tss;

/* Functions implemented in idt.c */
void set_idt_gate(int n, u32 handler);
void set_idt();
void set_gdt_entry(int num, u32 base, u32 limit, u8 access, u8 gran);
void set_gdt();
void write_tss(int num, u16 ss0, u32 esp0);
void set_kernel_stack(u32 stack);

#endif
