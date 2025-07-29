#include "../include/timer.h"
#include "../../drivers/include/screen.h"
#include "../../drivers/include/types.h"
#include "../../drivers/include/ports.h"
#include "../include/isr.h"
#include <string.h>

u32 tick = 0;

static void timer_callback(registers_t regs) {
    tick++;
    // Temporarily disable timer output to avoid conflicts with keyboard
    // if (tick % 50 == 0) {  // Every 50 ticks (1 second at 50Hz)
    //     kprint("Timer tick: ");
    //     char tick_ascii[256];
    //     int_to_ascii(tick, tick_ascii);
    //     kprint(tick_ascii);
    //     kprint("\n");
    // }
    (void)regs;  // Suppress unused parameter warning
}

void init_timer(u32 freq) {
    /* Install the function we just wrote */
    register_interrupt_handler(IRQ0, timer_callback);

    /* Get the PIT value: hardware clock at 1193180 Hz */
    u32 divisor = 1193180 / freq;
    u8 low  = (u8)(divisor & 0xFF);
    u8 high = (u8)( (divisor >> 8) & 0xFF);
    /* Send the command */
    port_byte_out(0x43, 0x36); /* Command port */
    port_byte_out(0x40, low);
    port_byte_out(0x40, high);
}


