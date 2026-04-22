#ifndef PORTAL_STILL_ALIVE_H
#define PORTAL_STILL_ALIVE_H

#include <drivers/types.h>

typedef struct {
    u32 freq_hz;      /* 0 = rest (silence for duration_ms) */
    u32 duration_ms;  /* note/rest length */
} note_t;

typedef struct {
    u32 beat_index;       /* index into notes[] that triggers this line */
    const char *text;     /* typed char-by-char (\n and spaces allowed) */
    u32 char_ms;          /* per-char delay; 0 = use PORTAL_CHAR_MS default */
} lyric_t;

typedef struct {
    u32 beat_index;
    const char *art;
} art_frame_t;

/* "Still Alive" data tables, generated from a MIDI file. Both note tables
 * end in a { 0, 0 } sentinel; lyric/frame tables end in a 0xFFFFFFFF beat. */
extern const note_t       still_alive_notes[];
extern const note_t       still_alive_bass[];
extern const lyric_t      still_alive_lyrics[];
extern const art_frame_t  still_alive_frames[];

#endif
