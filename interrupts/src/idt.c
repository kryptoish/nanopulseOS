#include "../include/idt.h"
#include <string.h>

idt_gate_t idt[IDT_ENTRIES];
idt_register_t idt_reg;

gdt_entry_t gdt[7];
gdt_register_t gdt_reg;
tss_entry_t tss;

void set_gdt_entry(int num, u32 base, u32 limit, u8 access, u8 gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

void write_tss(int num, u16 ss0, u32 esp0) {
    // Firstly, let's compute the base and limit of our entry into the GDT.
    u32 base = (u32) &tss;
    u32 limit = base + sizeof(tss);

    // Now, add our TSS descriptor's address to the GDT.
    set_gdt_entry(num, base, limit, 0xE9, 0x00);

    // Ensure the descriptor is initially zero.
    memset(&tss, 0, sizeof(tss));

    tss.ss0  = ss0;  // Set the kernel stack segment.
    tss.esp0 = esp0; // Set the kernel stack pointer.
    tss.cs   = 0x0b;     
    tss.ss   = 0x13;
    tss.ds   = 0x13;
    tss.es   = 0x13;
    tss.fs   = 0x13;
    tss.gs   = 0x13;
}

void set_kernel_stack(u32 stack) {
    tss.esp0 = stack;
}

void set_gdt() {
    gdt_reg.base = (u32) &gdt;
    gdt_reg.limit = (sizeof(gdt_entry_t) * 7) - 1;
    __asm__ __volatile__("lgdt (%0)" : : "r" (&gdt_reg));
    
    // Load the TSS
    __asm__ __volatile__("ltr %%ax" : : "a" (TSS_SEG));
}

void set_idt_gate(int n, u32 handler) {
    idt[n].low_offset = low_16(handler);
    idt[n].sel = KERNEL_CS;
    idt[n].always0 = 0;
    idt[n].flags = 0x8E; // 32-bit interrupt gate
    idt[n].high_offset = high_16(handler);
}

void set_idt() {
    idt_reg.base = (u32) &idt;
    idt_reg.limit = IDT_ENTRIES * sizeof(idt_gate_t) - 1;
    /* Don't make the mistake of loading &idt -- always load &idt_reg */
    __asm__ __volatile__("lidtl (%0)" : : "r" (&idt_reg));
}
