#include <kernel/portal.h>
#include <drivers/speaker.h>
#include <drivers/screen.h>
#include <drivers/keyboard.h>
#include <drivers/types.h>
#include <interrupts/timer.h>

/*
 * Typing cadence
 */
#define PORTAL_CHAR_MS 60

/*
 * Pseudo-polyphony: Adust for real hardware vs QEMU test
 */
#define PORTAL_SWITCH_SPINS 600000

/* VGA brown (#AA5500) — stock-palette orange. */
#define PORTAL_ORANGE 0x06

/* 1 = wipe lyric box and restart at top when full; 0 = scroll up. */
#define PORTAL_LYRIC_CLEAR_ON_FULL 1

typedef struct {
    u32 freq_hz;      /* 0 = rest (silence for duration_ms) */
    u32 duration_ms;  /* note/rest length */
} note_t;

typedef struct {
    u32 beat_index;       /* index into notes[] that triggers this line */
    const char *text;     /* typed char-by-char (\n and spaces allowed) */
    u32 char_ms;          /* per-char delay; 0 = use PORTAL_CHAR_MS default */
} lyric_t;

// PORTAL — "Still Alive" data tables
// Generated fom a Midi file
static const note_t still_alive_notes[] = {
    { 0,   1000 },  // rest
    { 784,  250 },  // This was a triumph.
    { 740,  250 },  // MIDI note 78
    { 659,  250 },  // MIDI note 76
    { 659,  250 },  // MIDI note 76
    { 740,  250 },  // MIDI note 78
    { 0,   2500 },  // rest
    { 440,  250 },  // I'm making a note
    { 784,  250 },  // MIDI note 79
    { 740,  250 },  // MIDI note 78
    { 659,  250 },  // MIDI note 76
    { 659,  500 },  // MIDI note 76
    { 740,  250 },  // here:
    { 0,    500 },  // rest
    { 587,  500 },  // HUGE
    { 659,  250 },  // SUCCESS.
    { 440,  250 },  // MIDI note 69
    { 440,  250 },  // MIDI note 69
    { 0,   1500 },  // rest
    { 440,  250 },  // It's hard
    { 659,  500 },  // MIDI note 76
    { 740,  250 },  // to
    { 784,  750 },  // overstate
    { 659,  250 },  // MIDI note 76
    { 554,  250 },  // MIDI note 73
    { 554,  250 },  // MIDI note 73
    { 587,  750 },  // my
    { 659,  500 },  // satisfaction
    { 440,  250 },  // MIDI note 69
    { 440,  500 },  // MIDI note 69
    { 740,  500 },  // MIDI note 78
    { 0,   2250 },  // rest
    { 784,  250 },  // Aperture Science
    { 740,  250 },  // MIDI note 78
    { 659,  250 },  // MIDI note 76
    { 659,  250 },  // MIDI note 76
    { 740,  250 },  // MIDI note 78
    { 0,   2500 },  // rest
    { 440,  250 },  // 38
    { 784,  250 },  // MIDI note 79
    { 740,  250 },  // MIDI note 78
    { 659,  250 },  // MIDI note 76
    { 659,  500 },  // MIDI note 76
    { 0,    250 },  // rest
    { 740,  250 },  // 44
    { 587,  500 },  // MIDI note 74
    { 0,    250 },  // rest
    { 659,  250 },  // 47
    { 440,  250 },  // MIDI note 69
    { 440,  250 },  // MIDI note 69
    { 0,   1750 },  // rest
    { 659,  500 },  // 51
    { 740,  250 },  // MIDI note 78
    { 784,  750 },  // MIDI note 79
    { 659,  250 },  // MIDI note 76
    { 554,  250 },  // MIDI note 73
    { 554,  500 },  // MIDI note 73
    { 587,  250 },  // MIDI note 74
    { 659,  250 },  // MIDI note 76
    { 0,    250 },  // rest
    { 440,  250 },  // 60
    { 587,  250 },  // MIDI note 74
    { 659,  250 },  // MIDI note 76
    { 698,  250 },  // MIDI note 77
    { 659,  250 },  // MIDI note 76
    { 587,  250 },  // MIDI note 74
    { 523,  250 },  // MIDI note 72
    { 0,    500 },  // rest
    { 440,  250 },  // 68
    { 466,  250 },  // MIDI note 70
    { 523,  500 },  // MIDI note 72
    { 698,  500 },  // MIDI note 77
    { 659,  250 },  // MIDI note 76
    { 587,  250 },  // MIDI note 74
    { 587,  250 },  // MIDI note 74
    { 523,  250 },  // MIDI note 72
    { 587,  250 },  // MIDI note 74
    { 523,  250 },  // MIDI note 72
    { 523,  500 },  // MIDI note 72
    { 523,  250 },  // MIDI note 72
    { 0,    250 },  // rest
    { 440,  250 },  // 81
    { 466,  250 },  // MIDI note 70
    { 523,  500 },  // MIDI note 72
    { 698,  500 },  // MIDI note 77
    { 784,  250 },  // MIDI note 79
    { 698,  250 },  // MIDI note 77
    { 659,  250 },  // 87
    { 587,  250 },  // MIDI note 74
    { 587,  250 },  // MIDI note 74
    { 659,  250 },  // MIDI note 76
    { 698,  500 },  // MIDI note 77
    { 698,  250 },  // MIDI note 77
    { 0,    250 },  // rest
    { 784,  250 },  // 94
    { 880,  250 },  // MIDI note 81
    { 932,  250 },  // MIDI note 82
    { 932,  250 },  // MIDI note 82
    { 880,  500 },  // 98
    { 784,  500 },  // MIDI note 79
    { 698,  250 },  // 100
    { 784,  250 },  // MIDI note 79
    { 880,  250 },  // MIDI note 81
    { 880,  250 },  // MIDI note 81
    { 784,  500 },  // 104
    { 698,  500 },  // MIDI note 77
    { 587,  250 },  // 106
    { 523,  250 },  // MIDI note 72
    { 587,  250 },  // MIDI note 74
    { 698,  250 },  // MIDI note 77
    { 698,  250 },  // MIDI note 77
    { 659,  500 },  // 111
    { 659,  250 },  // MIDI note 76
    { 740,  250 },  // MIDI note 78
    { 740, 1000 },  // MIDI note 78
    { 0,   1000 },  // rest
    { 0,   5000 },  // rest
    { 440,  250 },  // 117
    { 784,  250 },  // MIDI note 79
    { 740,  250 },  // MIDI note 78
    { 659,  250 },  // MIDI note 76
    { 659,  250 },  // MIDI note 76
    { 740,  250 },  // MIDI note 78
    { 0,   2750 },  // rest
    { 784,  250 },  // 124
    { 740,  250 },  // MIDI note 78
    { 659,  250 },  // MIDI note 76
    { 659,  750 },  // MIDI note 76
    { 740,  250 },  // MIDI note 78
    { 587,  500 },  // MIDI note 74
    { 659,  500 },  // MIDI note 76
    { 440,  250 },  // MIDI note 69
    { 440,  250 },  // MIDI note 69
    { 0,   1750 },  // rest
    { 659,  500 },  // 134
    { 740,  250 },  // MIDI note 78
    { 784,  750 },  // MIDI note 79
    { 659,  500 },  // MIDI note 76
    { 554,  500 },  // MIDI note 73
    { 587,  250 },  // MIDI note 74
    { 659,  750 },  // MIDI note 76
    { 440,  250 },  // 141
    { 440,  500 },  // MIDI note 69
    { 740,  500 },  // MIDI note 78
    { 0,   2000 },  // rest
    { 587,  250 },  // 145
    { 988,  250 },  // MIDI note 83
    { 880,  250 },  // MIDI note 81
    { 784,  250 },  // MIDI note 79
    { 784,  250 },  // MIDI note 79
    { 880,  250 },  // MIDI note 81
    { 0,   2500 },  // rest
    { 587,  250 },  // 152
    { 988,  250 },  // MIDI note 83
    { 880,  250 },  // MIDI note 81
    { 784,  250 },  // MIDI note 79
    { 784,  500 },  // MIDI note 79
    { 0,    250 },  // rest
    { 880,  250 },  // 158
    { 740,  500 },  // MIDI note 78
    { 0,    250 },  // rest
    { 784,  250 },  // 161
    { 587,  250 },  // MIDI note 74
    { 587,  250 },  // MIDI note 74
    { 0,   1750 },  // rest
    { 659,  500 },  // 164
    { 740,  250 },  // MIDI note 78
    { 784,  750 },  // MIDI note 79
    { 659,  500 },  // MIDI note 76
    { 554,  500 },  // MIDI note 73
    { 587,  250 },  // MIDI note 74
    { 659,  250 },  // MIDI note 76
    { 0,    250 },  // rest
    { 440,  250 },  // 172
    { 587,  250 },  // MIDI note 74
    { 659,  250 },  // MIDI note 76
    { 698,  250 },  // MIDI note 77
    { 659,  250 },  // MIDI note 76
    { 587,  250 },  // MIDI note 74
    { 523,  250 },  // MIDI note 72
    { 0,    500 },  // rest
    { 440,  250 },  // 180
    { 466,  250 },  // MIDI note 70
    { 523,  500 },  // MIDI note 72
    { 698,  500 },  // MIDI note 77
    { 659,  250 },  // MIDI note 76
    { 587,  250 },  // 185
    { 587,  250 },  // 186
    { 523,  250 },  // MIDI note 72
    { 587,  250 },  // MIDI note 74
    { 523,  250 },  // MIDI note 72
    { 523,  500 },  // MIDI note 72
    { 523,  500 },  // 191
    { 440,  250 },  // 192
    { 466,  250 },  // MIDI note 70
    { 523,  500 },  // MIDI note 72
    { 698,  500 },  // MIDI note 77
    { 784,  250 },  // MIDI note 79
    { 698,  250 },  // 197
    { 659,  250 },  // 198
    { 587,  250 },  // MIDI note 74
    { 587,  250 },  // MIDI note 74
    { 659,  250 },  // MIDI note 76
    { 698,  500 },  // MIDI note 77
    { 698,  500 },  // MIDI note 77
    { 784,  250 },  // 204
    { 880,  250 },  // MIDI note 81
    { 932,  250 },  // MIDI note 82
    { 932,  250 },  // MIDI note 82
    { 880,  500 },  // MIDI note 81
    { 784,  500 },  // MIDI note 79
    { 698,  250 },  // 210
    { 784,  250 },  // MIDI note 79
    { 880,  250 },  // MIDI note 81
    { 880,  250 },  // MIDI note 81
    { 784,  250 },  // MIDI note 79
    { 698,  250 },  // MIDI note 77
    { 698,  500 },  // MIDI note 77
    { 587,  250 },  // 217
    { 523,  250 },  // MIDI note 72
    { 587,  250 },  // MIDI note 74
    { 698,  250 },  // MIDI note 77
    { 698,  250 },  // MIDI note 77
    { 659,  500 },  // MIDI note 76
    { 659,  250 },  // 224
    { 740,  250 },  // MIDI note 78
    { 740, 1000 },  // MIDI note 78
    { 0,   1000 },  // rest
    { 0,   5250 },  // rest
    { 784,  125 },  // 229
    { 784,  125 },  // MIDI note 79
    { 740,  250 },  // MIDI note 78
    { 659,  250 },  // MIDI note 76
    { 659,  375 },  // MIDI note 76
    { 740,  375 },  // MIDI note 78
    { 0,   2250 },  // rest
    { 440,  250 },  // 235
    { 784,  250 },  // MIDI note 79
    { 740,  250 },  // MIDI note 78
    { 659,  250 },  // MIDI note 76
    { 659,  500 },  // MIDI note 76
    { 0,    250 },  // rest
    { 740,  250 },  // MIDI note 78
    { 587,  500 },  // MIDI note 74
    { 0,    250 },  // rest
    { 659,  250 },  // 244
    { 440,  250 },  // MIDI note 69
    { 440,  250 },  // MIDI note 69
    { 0,   1750 },  // rest
    { 659,  500 },  // 248
    { 740,  250 },  // MIDI note 78
    { 784,  750 },  // MIDI note 79
    { 659,  500 },  // MIDI note 76
    { 554,  500 },  // MIDI note 73
    { 587,  250 },  // MIDI note 74
    { 659,  750 },  // MIDI note 76
    { 440,  250 },  // 255
    { 440,  500 },  // MIDI note 69
    { 740,  500 },  // MIDI note 78
    { 0,   2250 },  // rest
    { 988,  250 },  // 259
    { 880,  250 },  // MIDI note 81
    { 784,  250 },  // MIDI note 79
    { 784,  312 },  // MIDI note 79
    { 880,  250 },  // MIDI note 81
    { 0,   2688 },  // rest
    { 988,  250 },  // 265
    { 880,  250 },  // MIDI note 81
    { 784,  250 },  // MIDI note 79
    { 784,  500 },  // MIDI note 79
    { 0,    250 },  // rest
    { 880,  250 },  // MIDI note 81
    { 740,  500 },  // MIDI note 78
    { 0,    250 },  // rest
    { 784,  250 },  // 273
    { 587,  250 },  // MIDI note 74
    { 587,  250 },  // MIDI note 74
    { 0,   1750 },  // rest
    { 659,  500 },  // 277
    { 740,  250 },  // MIDI note 78
    { 784,  750 },  // MIDI note 79
    { 659,  500 },  // MIDI note 76
    { 554,  500 },  // MIDI note 73
    { 587,  250 },  // MIDI note 74
    { 659,  250 },  // MIDI note 76
    { 0,    250 },  // rest
    { 440,  250 },  // 285
    { 587,  250 },  // MIDI note 74
    { 659,  250 },  // MIDI note 76
    { 698,  250 },  // MIDI note 77
    { 659,  250 },  // MIDI note 76
    { 587,  250 },  // MIDI note 74
    { 523,  250 },  // MIDI note 72
    { 0,    500 },  // rest
    { 440,  250 },  // 293
    { 466,  250 },  // MIDI note 70
    { 523,  500 },  // MIDI note 72
    { 698,  500 },  // MIDI note 77
    { 659,  250 },  // MIDI note 76
    { 587,  250 },  // MIDI note 74
    { 587,  250 },  // 299
    { 523,  250 },  // MIDI note 72
    { 587,  250 },  // MIDI note 74
    { 523,  250 },  // MIDI note 72
    { 523,  500 },  // MIDI note 72
    { 523,  500 },  // MIDI note 72
    { 440,  250 },  // 305
    { 466,  250 },  // MIDI note 70
    { 523,  500 },  // MIDI note 72
    { 698,  500 },  // MIDI note 77
    { 784,  250 },  // MIDI note 79
    { 698,  250 },  // MIDI note 77
    { 659,  250 },  // 311
    { 587,  250 },  // MIDI note 74
    { 587,  250 },  // MIDI note 74
    { 659,  250 },  // MIDI note 76
    { 698,  500 },  // MIDI note 77
    { 698,  500 },  // MIDI note 77
    { 784,  250 },  // MIDI note 79
    { 880,  250 },  // 318
    { 932,  250 },  // MIDI note 82
    { 932,  250 },  // MIDI note 82
    { 880,  250 },  // MIDI note 81
    { 784,  250 },  // MIDI note 79
    { 784,  500 },  // MIDI note 79
    { 698,  250 },  // MIDI note 77
    { 784,  250 },  // 325
    { 880,  250 },  // MIDI note 81
    { 880,  250 },  // MIDI note 81
    { 784,  250 },  // MIDI note 79
    { 698,  250 },  // MIDI note 77
    { 698,  500 },  // MIDI note 77
    { 587,  250 },  // MIDI note 74
    { 523,  250 },  // 332
    { 587,  250 },  // MIDI note 74
    { 698,  250 },  // MIDI note 77
    { 698,  250 },  // MIDI note 77
    { 659,  500 },  // MIDI note 76
    { 659,  250 },  // 338
    { 740,  250 },  // MIDI note 78
    { 740, 1000 },  // MIDI note 78
    { 0,    750 },  // rest
    { 880,  250 },  // 342
    { 880,  250 },  // MIDI note 81
    { 988,  250 },  // MIDI note 83
    { 880,  250 },  // MIDI note 81
    { 740,  250 },  // MIDI note 78
    { 587,  500 },  // MIDI note 74
    { 784,  250 },  // 348
    { 740,  250 },  // MIDI note 78
    { 740,  500 },  // MIDI note 78
    { 0,   1000 },  // rest
    { 880,  250 },  // 352
    { 880,  250 },  // MIDI note 81
    { 880,  250 },  // MIDI note 81
    { 988,  250 },  // MIDI note 83
    { 880,  250 },  // MIDI note 81
    { 740,  250 },  // MIDI note 78
    { 587,  500 },  // MIDI note 74
    { 784,  250 },  // 359
    { 880,  250 },  // MIDI note 81
    { 880,  500 },  // MIDI note 81
    { 0,   1000 },  // rest
    { 880,  250 },  // 363
    { 880,  250 },  // MIDI note 81
    { 880,  250 },  // MIDI note 81
    { 988,  250 },  // MIDI note 83
    { 880,  250 },  // MIDI note 81
    { 740,  250 },  // MIDI note 78
    { 587,  500 },  // MIDI note 74
    { 784,  250 },  // 370
    { 880,  250 },  // MIDI note 81
    { 880,  500 },  // MIDI note 81
    { 0,   1250 },  // rest
    { 880,  250 },  // MIDI note 81
    { 880,  250 },  // MIDI note 81
    { 988,  250 },  // MIDI note 83
    { 880,  250 },  // MIDI note 81
    { 740,  250 },  // MIDI note 78
    { 587,  500 },  // MIDI note 74
    { 784,  250 },  // 380
    { 880,  250 },  // MIDI note 81
    { 880,  500 },  // MIDI note 81
    { 0,   1000 },  // rest
    { 880,  250 },  // 384
    { 880,  250 },  // MIDI note 81
    { 880,  250 },  // MIDI note 81
    { 988,  250 },  // MIDI note 83
    { 880,  250 },  // MIDI note 81
    { 740,  250 },  // MIDI note 78
    { 587,  500 },  // MIDI note 74
    { 784,  250 },  // 391
    { 880,  250 },  // MIDI note 81
    { 880,  500 },  // MIDI note 81
    { 0,   1000 },  // rest
    { 784,  250 },  // MIDI note 79
    { 880,  250 },  // MIDI note 81
    { 880,  250 },  // MIDI note 81
    { 880,  250 },  // MIDI note 81
    { 0,   1000 },  // rest
    { 784,  250 },  // MIDI note 79
    { 740,  250 },  // MIDI note 78
    { 740,  500 },  // MIDI note 78
    { 0,      0 }   // sentinel
};

static const note_t still_alive_bass[] = {
    { 0,   2000 },  // rest
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 330,  250 },  // MIDI note 64
    { 392,  250 },  // MIDI note 67
    { 330,  250 },  // MIDI note 64
    { 247,  250 },  // MIDI note 59
    { 330,  250 },  // MIDI note 64
    { 392,  250 },  // MIDI note 67
    { 330,  250 },  // MIDI note 64
    { 220,  250 },  // MIDI note 57
    { 277,  250 },  // MIDI note 61
    { 392,  250 },  // MIDI note 67
    { 277,  250 },  // MIDI note 61
    { 220,  250 },  // MIDI note 57
    { 277,  250 },  // MIDI note 61
    { 392,  250 },  // MIDI note 67
    { 277,  250 },  // MIDI note 61
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 330,  250 },  // MIDI note 64
    { 392,  250 },  // MIDI note 67
    { 330,  250 },  // MIDI note 64
    { 247,  250 },  // MIDI note 59
    { 330,  250 },  // MIDI note 64
    { 392,  250 },  // MIDI note 67
    { 330,  250 },  // MIDI note 64
    { 220,  250 },  // MIDI note 57
    { 277,  250 },  // MIDI note 61
    { 392,  250 },  // MIDI note 67
    { 277,  250 },  // MIDI note 61
    { 220,  250 },  // MIDI note 57
    { 277,  250 },  // MIDI note 61
    { 392,  250 },  // MIDI note 67
    { 277,  250 },  // MIDI note 61
    { 233,  250 },  // MIDI note 58
    { 294,  250 },  // MIDI note 62
    { 349,  250 },  // MIDI note 65
    { 440, 1000 },  // MIDI note 69
    { 0,    250 },  // rest
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 262,  250 },  // MIDI note 60
    { 392,  250 },  // MIDI note 67
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 233,  250 },  // MIDI note 58
    { 466,  250 },  // MIDI note 70
    { 233,  250 },  // MIDI note 58
    { 330,  250 },  // MIDI note 64
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 262,  250 },  // MIDI note 60
    { 392,  250 },  // MIDI note 67
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 233,  250 },  // MIDI note 58
    { 466,  250 },  // MIDI note 70
    { 233,  250 },  // MIDI note 58
    { 330,  250 },  // MIDI note 64
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 233,  250 },  // MIDI note 58
    { 392,  250 },  // MIDI note 67
    { 233,  250 },  // MIDI note 58
    { 392,  250 },  // MIDI note 67
    { 262,  250 },  // MIDI note 60
    { 392,  250 },  // MIDI note 67
    { 262,  250 },  // MIDI note 60
    { 392,  250 },  // MIDI note 67
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 294,  250 },  // MIDI note 62
    { 440,  250 },  // MIDI note 69
    { 294,  250 },  // MIDI note 62
    { 440,  250 },  // MIDI note 69
    { 233,  250 },  // MIDI note 58
    { 392,  250 },  // MIDI note 67
    { 233,  250 },  // MIDI note 58
    { 392,  250 },  // MIDI note 67
    { 220,  250 },  // MIDI note 57
    { 392,  250 },  // MIDI note 67
    { 220,  250 },  // MIDI note 57
    { 392,  250 },  // MIDI note 67
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 330,  250 },  // MIDI note 64
    { 392,  250 },  // MIDI note 67
    { 330,  250 },  // MIDI note 64
    { 247,  250 },  // MIDI note 59
    { 330,  250 },  // MIDI note 64
    { 392,  250 },  // MIDI note 67
    { 330,  250 },  // MIDI note 64
    { 220,  250 },  // MIDI note 57
    { 277,  250 },  // MIDI note 61
    { 392,  250 },  // MIDI note 67
    { 277,  250 },  // MIDI note 61
    { 220,  250 },  // MIDI note 57
    { 277,  250 },  // MIDI note 61
    { 392,  250 },  // MIDI note 67
    { 277,  250 },  // MIDI note 61
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 330,  250 },  // MIDI note 64
    { 392,  250 },  // MIDI note 67
    { 330,  250 },  // MIDI note 64
    { 247,  250 },  // MIDI note 59
    { 330,  250 },  // MIDI note 64
    { 392,  250 },  // MIDI note 67
    { 330,  250 },  // MIDI note 64
    { 220,  250 },  // MIDI note 57
    { 277,  250 },  // MIDI note 61
    { 392,  250 },  // MIDI note 67
    { 277,  250 },  // MIDI note 61
    { 220,  250 },  // MIDI note 57
    { 277,  250 },  // MIDI note 61
    { 392,  250 },  // MIDI note 67
    { 277,  250 },  // MIDI note 61
    { 233,  250 },  // MIDI note 58
    { 294,  250 },  // MIDI note 62
    { 440,  250 },  // MIDI note 69
    { 349,  250 },  // MIDI note 65
    { 440,  250 },  // MIDI note 69
    { 349,  250 },  // MIDI note 65
    { 440,  250 },  // MIDI note 69
    { 349,  250 },  // MIDI note 65
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 262,  250 },  // MIDI note 60
    { 392,  250 },  // MIDI note 67
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 233,  250 },  // MIDI note 58
    { 466,  250 },  // MIDI note 70
    { 233,  250 },  // MIDI note 58
    { 330,  250 },  // MIDI note 64
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 262,  250 },  // MIDI note 60
    { 392,  250 },  // MIDI note 67
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 233,  250 },  // MIDI note 58
    { 466,  250 },  // MIDI note 70
    { 233,  250 },  // MIDI note 58
    { 330,  250 },  // MIDI note 64
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 233,  250 },  // MIDI note 58
    { 392,  250 },  // MIDI note 67
    { 233,  250 },  // MIDI note 58
    { 392,  250 },  // MIDI note 67
    { 262,  250 },  // MIDI note 60
    { 392,  250 },  // MIDI note 67
    { 262,  250 },  // MIDI note 60
    { 392,  250 },  // MIDI note 67
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 294,  250 },  // MIDI note 62
    { 440,  250 },  // MIDI note 69
    { 294,  250 },  // MIDI note 62
    { 440,  250 },  // MIDI note 69
    { 233,  250 },  // MIDI note 58
    { 392,  250 },  // MIDI note 67
    { 233,  250 },  // MIDI note 58
    { 392,  250 },  // MIDI note 67
    { 220,  250 },  // MIDI note 57
    { 392,  250 },  // MIDI note 67
    { 220,  250 },  // MIDI note 57
    { 392,  250 },  // MIDI note 67
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 330,  250 },  // MIDI note 64
    { 392,  250 },  // MIDI note 67
    { 330,  250 },  // MIDI note 64
    { 247,  250 },  // MIDI note 59
    { 330,  250 },  // MIDI note 64
    { 392,  250 },  // MIDI note 67
    { 330,  250 },  // MIDI note 64
    { 220,  250 },  // MIDI note 57
    { 277,  250 },  // MIDI note 61
    { 392,  250 },  // MIDI note 67
    { 277,  250 },  // MIDI note 61
    { 220,  250 },  // MIDI note 57
    { 277,  250 },  // MIDI note 61
    { 392,  250 },  // MIDI note 67
    { 277,  250 },  // MIDI note 61
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 330,  250 },  // MIDI note 64
    { 392,  250 },  // MIDI note 67
    { 330,  250 },  // MIDI note 64
    { 247,  250 },  // MIDI note 59
    { 330,  250 },  // MIDI note 64
    { 392,  250 },  // MIDI note 67
    { 330,  250 },  // MIDI note 64
    { 220,  250 },  // MIDI note 57
    { 277,  250 },  // MIDI note 61
    { 392,  250 },  // MIDI note 67
    { 277,  250 },  // MIDI note 61
    { 220,  250 },  // MIDI note 57
    { 277,  250 },  // MIDI note 61
    { 392,  250 },  // MIDI note 67
    { 277,  250 },  // MIDI note 61
    { 233,  250 },  // MIDI note 58
    { 294,  250 },  // MIDI note 62
    { 440,  250 },  // MIDI note 69
    { 349,  250 },  // MIDI note 65
    { 440,  250 },  // MIDI note 69
    { 349,  250 },  // MIDI note 65
    { 440,  250 },  // MIDI note 69
    { 349,  250 },  // MIDI note 65
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 262,  250 },  // MIDI note 60
    { 392,  250 },  // MIDI note 67
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 233,  250 },  // MIDI note 58
    { 466,  250 },  // MIDI note 70
    { 233,  250 },  // MIDI note 58
    { 330,  250 },  // MIDI note 64
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 262,  250 },  // MIDI note 60
    { 392,  250 },  // MIDI note 67
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 233,  250 },  // MIDI note 58
    { 466,  250 },  // MIDI note 70
    { 233,  250 },  // MIDI note 58
    { 330,  250 },  // MIDI note 64
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 262,  250 },  // MIDI note 60
    { 0,    250 },  // rest
    { 233,  250 },  // MIDI note 58
    { 392,  250 },  // MIDI note 67
    { 233,  250 },  // MIDI note 58
    { 392,  250 },  // MIDI note 67
    { 262,  250 },  // MIDI note 60
    { 392,  250 },  // MIDI note 67
    { 262,  250 },  // MIDI note 60
    { 392,  250 },  // MIDI note 67
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 262,  250 },  // MIDI note 60
    { 440,  250 },  // MIDI note 69
    { 294,  250 },  // MIDI note 62
    { 440,  250 },  // MIDI note 69
    { 294,  250 },  // MIDI note 62
    { 440,  250 },  // MIDI note 69
    { 233,  250 },  // MIDI note 58
    { 392,  250 },  // MIDI note 67
    { 233,  250 },  // MIDI note 58
    { 392,  250 },  // MIDI note 67
    { 220,  250 },  // MIDI note 57
    { 392,  250 },  // MIDI note 67
    { 220,  250 },  // MIDI note 57
    { 392,  250 },  // MIDI note 67
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 220,  250 },  // MIDI note 57
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 247,  250 },  // MIDI note 59
    { 294,  250 },  // MIDI note 62
    { 370,  250 },  // MIDI note 66
    { 294,  250 },  // MIDI note 62
    { 0,      0 }   // sentinel
};

static const lyric_t still_alive_lyrics[] = {
    { 0, "\n" },
    { 1, "This was a triumph.\n" },
    { 7, "I'm making a note " },
    { 12, "here:\n" },
    { 14, "HUGE " },
    { 15, "SUCCESS.\n" },
    { 19, "It's hard " },
    { 21, "to " },
    { 22, "overstate\n", 120 },
    { 26, "my ", 60 },
    { 27, "satisfaction.\n", 130 },
    { 32, "Aperture Science\n" },
    { 38, "We do what we must\n" },
    { 44, "because ", 100 },
    { 47, "we can.\n" },
    { 51, "For the good of ", 100 },
    { 56, "all of us.\n", 120 },
    { 60, "Except the ones who are dead.\n\n" },
    { 68, "But there's no sense crying\n" },
    { 74, "over every mistake.\n", 80 },
    { 81, "You just keep on trying\n", 80 },
    { 87, "till you run out of cake.\n", 80 },
    { 94, "And the Science " },
    { 98, "gets done.\n", 80 },
    { 100, "And you make a " },
    { 104, "neat gun.\n", 80 },
    { 106, "For the people who " },
    { 111, "are\n", 70 },
    { 112, "still alive.", 120 },
    { 116, "\n\nDear <<Subject Name Here>>\n\n" },
    { 117, "I'm not even angry.\n" },
    { 124, "I'm being " },
    { 127, "so ", 140 },
    { 128, "sincere right now.\n", 100 },
    { 134, "Even though you broke my heart.\n", 120 },
    { 141, "And killed me.\n", 100 },
    { 145, "And tore me to pieces.\n" },
    { 152, "And threw every piece " },
    { 158, "into ", 100 },
    { 161, "a fire.\n", 100 },
    { 165, "As they burned it hurt because\n", 120 },
    { 173, "I was so happy for you!\n" },
    { 181, "Now these points of data\n" },
    { 187, "make a beautiful line.\n", 80 },
    { 193, "And we're out of beta.\n", 80 },
    { 199, "We're releasing on time.\n" },
    { 205, "So I'm GLaD. I got burned.\n" },
    { 211, "Think of all the things we learned\n"},
    { 218, "for the people who are\n", 80 },
    { 224, "still alive.\n", 120 },
    { 228, "\n\nOne last thing:\n\n", 100 },
    { 229, "Go ahead and leave " },
    { 234, "me.\n", 140 },
    { 236, "I think I " },
    { 240, "prefer to stay ", 100 },
    { 245, "Inside.\n", 120 },
    { 249, "Maybe you'll find someone else\n", 100 },
    { 256, "to help you.\n", 80 },
    { 260, "Maybe Black " },
    { 262, "Mesa...\n", 100 },
    { 266, "THAT WAS A JOKE. " },
    { 274, "FAT CHANCE.\n", 100 },
    { 278, "Anyway, ", 140 },
    { 281, "this cake is great.\n", 80 },
    { 286, "It's so delicious and moist.\n", 80 },
    { 294, "Look at me still talking\n" },
    { 300, "when there's Science to do.\n" },
    { 306, "When I look out there,\n", 80 },
    { 312, "it makes me GLaD I'm not you.\n" },
    { 319, "I've experiments to run.\n" },
    { 326, "There is research to be done.\n" },
    { 333, "On the people who are\n" },
    { 338, "stil alive.\n", 140 },
    { 341, "\n\nPS: " },
    { 342, "And believe me I am\n" },
    { 348, "still alive.\n", 100 },
    { 351, "PPS: " },
    { 352, "I'm doing Science and I'm\n" },
    { 359, "still alive.\n", 100 },
    { 362, "PPPS: " },
    { 363, "I feel FANTASTIC and I'm\n" },
    { 370, "still alive.\n", 100 },
    { 373, "\nFINAL THOUGHT:\n" },
    { 374, "While you're dying I'll be\n" },
    { 380, "still alive.\n", 100 },
    { 383, "\nFINAL THOUGHT PS:\n" },
    { 384, "And when you're dead I will be\n" },
    { 391, "still alive.\n", 100 },
    { 395, "\n\nSTILL ALIVE\n", 100 },
    { 400, "\n\n" },
    { 0xFFFFFFFF, 0 }  /* sentinel */
};

typedef struct {
    u32 beat_index;
    const char *art;
} art_frame_t;

/* Layout (80 cols × 25 rows). Title on row 0, divider on row 1,
 * column labels on row 2, main content rows 3..22, footer row 23+24. */
#define BOX_LYRIC_COL   2
#define BOX_LYRIC_ROW   3
#define BOX_LYRIC_W    37
#define BOX_LYRIC_H    20

#define BOX_ART_COL    42
#define BOX_ART_ROW     3
#define BOX_ART_W      36
#define BOX_ART_H      20

/* --- TODO: Art data. Replace --- */

static const char art_aperture[] =
    "                                \n"
    "                                \n"
    "         _____________          \n"
    "       /               \\        \n"
    "      /     _______     \\       \n"
    "     |    /         \\    |      \n"
    "     |   |     O     |   |      \n"
    "     |    \\_________/    |      \n"
    "      \\                 /       \n"
    "       \\_______________/        \n"
    "                                \n"
    "       A P E R T U R E          \n"
    "        S C I E N C E           \n"
    "                                \n";

static const char art_heart[] =
    "                                \n"
    "                                \n"
    "                                \n"
    "       .-~~-.       .-~~-.      \n"
    "      /      \\     /      \\     \n"
    "     |        '---'        |    \n"
    "      \\                   /     \n"
    "       \\                 /      \n"
    "        \\               /       \n"
    "         \\             /        \n"
    "          \\           /         \n"
    "           \\         /          \n"
    "            \\       /           \n"
    "             \\     /            \n"
    "              \\   /             \n"
    "               \\ /              \n"
    "                V               \n";

static const char art_cube[] =
    "                                \n"
    "                                \n"
    "        +--------------+        \n"
    "       /|             /|        \n"
    "      / |            / |        \n"
    "     +--+-----------+  |        \n"
    "     |  |   _____   |  |        \n"
    "     |  |  /     \\  |  |        \n"
    "     |  | |  <3   | |  +        \n"
    "     |  |  \\_____/  | /         \n"
    "     |  +-----------+/          \n"
    "     | /            |           \n"
    "     |/             |           \n"
    "     +--------------+           \n"
    "                                \n"
    "       companion cube           \n";

static const char art_cake[] =
    "                                \n"
    "                                \n"
    "                                \n"
    "          i   i   i             \n"
    "         _|___|___|_            \n"
    "        /           \\           \n"
    "       /_____________\\          \n"
    "       | o  o  o  o  |          \n"
    "       |_____________|          \n"
    "        \\/\\/\\/\\/\\/\\/            \n"
    "                                \n"
    "                                \n"
    "     the cake is a lie          \n"
    "                                \n";

static const char art_turret[] =
    "                                \n"
    "                                \n"
    "              _____             \n"
    "            .'     '.           \n"
    "           /         \\          \n"
    "          |  o     o  |         \n"
    "           \\         /          \n"
    "            '.  ^  .'           \n"
    "              |||               \n"
    "              |||               \n"
    "            __|||__             \n"
    "           /       \\            \n"
    "          /_________\\           \n"
    "                                \n"
    "      are you still there?      \n";

static const char art_glados[] =
    "                                \n"
    "                                \n"
    "              .---.             \n"
    "           .-'     '-.          \n"
    "          /           \\         \n"
    "         |    .---.    |        \n"
    "         |   | (o) |   |        \n"
    "         |    '---'    |        \n"
    "          \\           /         \n"
    "           '._______.'          \n"
    "               |                \n"
    "               |                \n"
    "           ____|____            \n"
    "          /         \\           \n"
    "                                \n"
    "          G L a D O S           \n";

static const art_frame_t still_alive_frames[] = {
    { 0,   art_aperture },
    { 10,  art_heart    },
    { 30,  art_cube     },
    { 60,  art_cake     },
    { 90,  art_turret   },
    { 120, art_glados   },
    { 0xFFFFFFFF, 0 }
};

/* Round milliseconds up to whole 10 ms PIT ticks, minimum 1. */
static u32 ms_to_ticks(u32 ms) {
    u32 t = (ms + 9) / 10;
    return t == 0 ? 1 : t;
}

/* --- Boxed screen writer --- */

static u32 ly_col = 0;
static u32 ly_row = 0;

static void clear_rect(int col, int row, int w, int h) {
    for (int r = 0; r < h; r++) {
        for (int c = 0; c < w; c++) {
            print_char(' ', col + c, row + r, WHITE_ON_BLACK);
        }
    }
}

static void draw_line_h(int col, int row, int w, char ch) {
    for (int i = 0; i < w; i++) print_char(ch, col + i, row, WHITE_ON_BLACK);
}

static void tint_screen(u8 attr) {
    u8 *vid = (u8*)VIDEO_ADDRESS;
    for (int i = 0; i < MAX_ROWS * MAX_COLS; i++) {
        vid[i * 2 + 1] = attr;
    }
}

static void draw_chrome(void) {
    clear_screen();
    kprint_at("  == APERTURE SCIENCE ==  ", 27, 0);
    draw_line_h(0, 1, MAX_COLS, '-');
    kprint_at("Test Assessment Report",         BOX_LYRIC_COL, 2);
    kprint_at("GLaDOS",  BOX_ART_COL,   2);
    draw_line_h(0, 23, MAX_COLS, '-');
    kprint_at("  [ Ctrl+C to exit ]  ", 29, 24);

    /* Vertical bars: left edge, middle separator between the lyric and
     * art boxes, right edge. Drawn between the horizontal dividers so
     * the frame reads as one enclosed panel. */
    for (int r = 2; r <= 22; r++) {
        print_char('|',  0,            r, WHITE_ON_BLACK);
        print_char('|', 40,            r, WHITE_ON_BLACK);
        print_char('|', MAX_COLS - 1,  r, WHITE_ON_BLACK);
    }
}

/* Scroll the lyric box up by one row, blanking the bottom row. */
static void lyric_box_scroll(void) {
    u8 *vid = (u8*)VIDEO_ADDRESS;
    for (int r = 1; r < BOX_LYRIC_H; r++) {
        int dst_row = BOX_LYRIC_ROW + r - 1;
        int src_row = BOX_LYRIC_ROW + r;
        for (int c = 0; c < BOX_LYRIC_W; c++) {
            int d = 2 * (dst_row * MAX_COLS + BOX_LYRIC_COL + c);
            int s = 2 * (src_row * MAX_COLS + BOX_LYRIC_COL + c);
            vid[d]     = vid[s];
            vid[d + 1] = vid[s + 1];
        }
    }
    for (int c = 0; c < BOX_LYRIC_W; c++) {
        print_char(' ', BOX_LYRIC_COL + c,
                   BOX_LYRIC_ROW + BOX_LYRIC_H - 1, WHITE_ON_BLACK);
    }
}

/* Either wipe the lyric box and restart at the top, or scroll one row,
 * depending on PORTAL_LYRIC_CLEAR_ON_FULL. */
static void lyric_box_overflow(void) {
#if PORTAL_LYRIC_CLEAR_ON_FULL
    clear_rect(BOX_LYRIC_COL, BOX_LYRIC_ROW, BOX_LYRIC_W, BOX_LYRIC_H);
    ly_col = 0;
    ly_row = 0;
#else
    lyric_box_scroll();
    ly_row = BOX_LYRIC_H - 1;
#endif
}

/* Write one character into the lyric box, with wrap + overflow handling. */
static void portal_putc(char c) {
    if (c == '\n') {
        ly_col = 0;
        ly_row++;
    } else {
        if (ly_col >= BOX_LYRIC_W) { ly_col = 0; ly_row++; }
        if (ly_row >= BOX_LYRIC_H) lyric_box_overflow();
        print_char(c, BOX_LYRIC_COL + ly_col,
                   BOX_LYRIC_ROW + ly_row, WHITE_ON_BLACK);
        ly_col++;
    }
    if (ly_row >= BOX_LYRIC_H) lyric_box_overflow();
}

/* Clear the art panel and render an ASCII-art frame into it. */
static void art_draw(const char *art) {
    clear_rect(BOX_ART_COL, BOX_ART_ROW, BOX_ART_W, BOX_ART_H);
    if (art == 0) return;
    int row = 0, col = 0;
    for (int i = 0; art[i] != '\0'; i++) {
        char ch = art[i];
        if (ch == '\n') { row++; col = 0; continue; }
        if (row >= BOX_ART_H) break;
        if (col < BOX_ART_W) {
            print_char(ch, BOX_ART_COL + col,
                       BOX_ART_ROW + row, WHITE_ON_BLACK);
            col++;
        }
    }
}

/*
 * Tick-driven playback loop. Melody and bass advance on independent
 * schedules against the same PIT tick clock. When both voices have a
 * non-zero frequency at the same instant, the speaker rapidly alternates
 * between them (pseudo-polyphony).
 */
void portal_run(void) {
    /* Raw mode swallows keystrokes so the user typing during playback
     * doesn't mangle the lyric display. Ctrl+C is still observed via
     * keyboard_cancel_requested() — see keyboard.c. */
    keyboard_clear_cancel();
    keyboard_set_raw_mode(1);

    /* hlt needs interrupts on so the PIT tick can wake us each 10 ms. */
    __asm__ __volatile__("sti");

    /* Paint the static chrome (title / dividers / column labels / footer),
     * reset the lyric-box cursor, and drop in the opening art frame. */
    ly_col = 0;
    ly_row = 0;
    draw_chrome();
    if (still_alive_frames[0].art != 0) {
        art_draw(still_alive_frames[0].art);
    }
    tint_screen(PORTAL_ORANGE);

    const u32 t0 = get_tick();

    /* Melody (lead) voice state. */
    u32 mel_i = 0;
    u32 mel_expires_at = t0;
    u32 mel_freq = 0;
    int mel_done = 0;

    /* Bass (background) voice state. */
    u32 bass_i = 0;
    u32 bass_expires_at = t0;
    u32 bass_freq = 0;
    int bass_done = 0;

    /* Lyric / typing state (synced to the melody voice's beat index). */
    u32 lyric_i = 0;
    const char *line = 0;
    u32 line_pos = 0;
    u32 next_char_at = 0;
    u32 line_char_ms = PORTAL_CHAR_MS;

    /* Art-frame scheduling state. Frame 0 was drawn above; start at index
     * 1 if it was cued for beat 0, otherwise 0. */
    u32 frame_i = (still_alive_frames[0].art != 0 &&
                   still_alive_frames[0].beat_index == 0) ? 1 : 0;

    /* Interleave toggle for pseudo-polyphony. */
    int toggle = 0;

    while (1) {
        tint_screen(PORTAL_ORANGE);

        if (keyboard_cancel_requested()) break;

        u32 now = get_tick();

        /* --- Advance the melody voice. --- */
        if (!mel_done && now >= mel_expires_at) {
            if (still_alive_notes[mel_i].duration_ms == 0) {
                mel_done = 1;
                mel_freq = 0;
            } else {
                /* Dispatch the lyric cue (if any) for this beat. Flush any
                 * unfinished previous line instantly so the new one starts
                 * clean. */
                if (still_alive_lyrics[lyric_i].text != 0 &&
                    still_alive_lyrics[lyric_i].beat_index == mel_i) {
                    if (line != 0) {
                        while (line[line_pos] != '\0') {
                            portal_putc(line[line_pos]);
                            line_pos++;
                        }
                    }
                    line = still_alive_lyrics[lyric_i].text;
                    line_pos = 0;
                    next_char_at = now;
                    line_char_ms = still_alive_lyrics[lyric_i].char_ms;
                    if (line_char_ms == 0) line_char_ms = PORTAL_CHAR_MS;
                    lyric_i++;
                }

                /* Swap the art panel if this beat has a frame cue. */
                if (still_alive_frames[frame_i].art != 0 &&
                    still_alive_frames[frame_i].beat_index == mel_i) {
                    art_draw(still_alive_frames[frame_i].art);
                    frame_i++;
                }

                const note_t *n = &still_alive_notes[mel_i];
                mel_freq = n->freq_hz;
                mel_expires_at = now + ms_to_ticks(n->duration_ms);
                mel_i++;
            }
        }

        /* --- Advance the bass voice. --- */
        if (!bass_done && now >= bass_expires_at) {
            if (still_alive_bass[bass_i].duration_ms == 0) {
                bass_done = 1;
                bass_freq = 0;
            } else {
                const note_t *n = &still_alive_bass[bass_i];
                bass_freq = n->freq_hz;
                bass_expires_at = now + ms_to_ticks(n->duration_ms);
                bass_i++;
            }
        }

        /* --- Drive the speaker. --- */
        int interleaving = (mel_freq != 0 && bass_freq != 0);
        if (interleaving) {
            toggle ^= 1;
            speaker_play_freq(toggle ? mel_freq : bass_freq);
        } else if (mel_freq != 0) {
            speaker_play_freq(mel_freq);
        } else if (bass_freq != 0) {
            speaker_play_freq(bass_freq);
        } else {
            speaker_stop();
        }

        /* --- Advance the typewriter one character per PORTAL_CHAR_MS. --- */
        if (line != 0 && now >= next_char_at) {
            char c = line[line_pos];
            if (c == '\0') {
                line = 0;  /* line fully typed */
            } else {
                portal_putc(c);
                line_pos++;
                next_char_at = now + ms_to_ticks(line_char_ms);
            }
        }

        /* --- Exit when both voices are done and typing is caught up. --- */
        if (mel_done && bass_done && line == 0) break;

        /* --- Pace the loop. ---
         * When interleaving we must NOT hlt — the whole point is to keep
         * the speaker at the current freq for ~1 ms then switch again, so
         * we burn CPU with a short spin. When only one voice is active
         * there's nothing to fake, so hlt saves power and plays cleanly. */
        if (interleaving) {
            for (volatile u32 i = 0; i < PORTAL_SWITCH_SPINS; i++) { /* spin */ }
        } else {
            __asm__ __volatile__("hlt");
        }
    }

    speaker_stop();
    keyboard_set_raw_mode(0);
    keyboard_clear_cancel();
}
