#include <interrupts/timer.h>
#include <drivers/screen.h>
#include <drivers/types.h>
#include <drivers/ports.h>
#include <drivers/fb.h>
#include <interrupts/isr.h>

static volatile u32 tick = 0;

u32 get_tick(void) { return tick; }

static void timer_callback(registers_t regs) {
    tick++;
    /* Refresh framebuffer at ~33 Hz (every 3 ticks at 100 Hz). */
    if ((tick % 3) == 0) fb_present();
    (void)regs;
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
