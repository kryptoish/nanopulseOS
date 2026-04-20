#include "../include/speaker.h"
#include "../include/ports.h"

/* PIT input clock: ~1.19318 MHz. Dividing yields the square-wave period. */
#define PIT_INPUT_HZ 1193180

void speaker_play_freq(u32 freq_hz) {
    if (freq_hz == 0) { speaker_stop(); return; }

    u32 divisor = PIT_INPUT_HZ / freq_hz;
    if (divisor > 0xFFFF) divisor = 0xFFFF; /* clamp for audible low end */
    if (divisor == 0)     divisor = 1;

    /* Command byte: channel 2, access lobyte/hibyte, mode 3 (square wave). */
    port_byte_out(0x43, 0xB6);
    port_byte_out(0x42, (u8)(divisor & 0xFF));
    port_byte_out(0x42, (u8)((divisor >> 8) & 0xFF));

    /* 
     * Gate + output-enable: bits 0 and 1 of port 0x61 route channel 2
     * to the speaker. Only rewrite if the bits aren't already set, to
     * avoid a perceptible click between same-state updates. 
     */
    u8 tmp = port_byte_in(0x61);
    if ((tmp & 0x03) != 0x03) {
        port_byte_out(0x61, tmp | 0x03);
    }
}

void speaker_stop(void) {
    u8 tmp = port_byte_in(0x61);
    port_byte_out(0x61, tmp & (u8)~0x03);
}
