#ifndef SPEAKER_H
#define SPEAKER_H

#include "types.h"

/*
 * PC speaker driver. Routes PIT channel 2's square-wave output to the
 * speaker via keyboard-controller port B (0x61).
 *
 * Only one tone at a time.
 */

/* Start a continuous tone at freq_hz. Pass 0 to silence (equivalent to
 * speaker_stop). The tone keeps playing until changed or stopped. */
void speaker_play_freq(u32 freq_hz);

/* Disconnect the speaker from the PIT (silence). */
void speaker_stop(void);

#endif
