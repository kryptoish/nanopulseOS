#include <kernel/portal.h>
#include <drivers/speaker.h>
#include <drivers/screen.h>
#include <drivers/keyboard.h>
#include <drivers/types.h>
#include <interrupts/timer.h>

#include "still_alive.h"

/*
 * Typing cadence
 */
#define PORTAL_CHAR_MS 60

/*
 * Pseudo-polyphony: Adust for real hardware vs QEMU test
 */
#define PORTAL_SWITCH_SPINS 600000

/* VGA brown (#AA5500) - stock-palette orange. */
#define PORTAL_ORANGE 0x06

/* 1 = wipe lyric box and restart at top when full; 0 = scroll up. */
#define PORTAL_LYRIC_CLEAR_ON_FULL 1

/* Layout (80 cols × 25 rows). Title on row 0, divider on row 1,
 * column labels on row 2, main content rows 3..22, footer row 23+24. */
#define BOX_LYRIC_COL   2
#define BOX_LYRIC_ROW   3
#define BOX_LYRIC_W    36
#define BOX_LYRIC_H    20

#define BOX_ART_COL    39
#define BOX_ART_ROW     3
#define BOX_ART_W      40
#define BOX_ART_H      20

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
    kprint_at("  == APERTURE SCIENCE ==  ", 25, 0);
    draw_line_h(0, 1, MAX_COLS, '-');
    kprint_at("Test Assessment Report",         BOX_LYRIC_COL, 2);
    kprint_at(" GLaDOS",  BOX_ART_COL,   2);
    draw_line_h(0, 23, MAX_COLS, '-');
    kprint_at("  [ Ctrl+C to exit ]  ", 27, 24);

    /* Vertical bars: left edge, middle separator between the lyric and
     * art boxes, right edge. Drawn between the horizontal dividers so
     * the frame reads as one enclosed panel. */
    for (int r = 2; r <= 22; r++) {
        print_char('|',  0,            r, WHITE_ON_BLACK);
        print_char('|', 38,            r, WHITE_ON_BLACK);
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
     * keyboard_cancel_requested() - see keyboard.c. */
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
         * When interleaving we must NOT hlt - the whole point is to keep
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
