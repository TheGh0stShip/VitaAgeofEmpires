#ifndef VITA_AOE_MP_WAIT_SCREEN_H
#define VITA_AOE_MP_WAIT_SCREEN_H

#include <stdint.h>

#include "game_ui.h"

/* The "Multiplayer Wait Screen" (vtable 0x00546ea8, 0x0048d3a0..
   0x0048de70; "scr3" / 50053): shown while a networked game loads. A
   title box, a header line and up to 20 setting lines read from the game
   settings. */
typedef struct {
    AoeUiEzRor ez;
    AoeUiTextRor *title;        /* +0x478: "" in font 4 at (20, 40, 380 x 400) */
    int32_t field_47c;
    AoeUiButtonRor *hidden_close; /* +0x480: string 1002, position mode 9 */
    AoeUiTextRor *header;       /* +0x484: (420, 84, 220 x 68), wrapped */
    AoeUiTextRor *line[20];     /* +0x488: (420, 83 + 24 i, 220 x 20), hidden */
    int32_t lines_used;         /* the next free line (the original's ebp) */
    AoeGameUiRor *ui;
} AoeMpWaitScreenRor;

/* 0x0048d3a0 constructor. The header is string 0x25ed formatted with the
   scenario name (settings +0x8fc set; lines start at 3) or with string
   0x2617 / 0x25b5 by option +0xae5 (lines start at 1). Lines, each
   shown as it is set: with 0x0041b710 the map size (0x2973 + size, sizes
   0..5) in 0x25da and the map type (0x296a + type, types 0..8) in
   0x25b6; one line skipped; the victory condition (+0xa8c: 1 -> 0x10e1,
   7 -> 0x10e9 and 8 -> 0x10ea with the +0xa90 value in 0x25be, 9 ->
   0x10ec, others 0x10ec with 0x0041b710 else 0x10e7) in 0x25bc; the
   starting age (0 -> 0x10e7, 1 -> 0x106e, else 0x1067 + age) in 0x25e4;
   the resources (0 -> 0x10e7, else 0x25e5 + level) in 0x25e5; the
   difficulty (0x2bd0 + byte 0x993) in 0x25e0; option +0xad8 (set ->
   0xfa4, else 0xfa3) in 0x25e9; byte 0x985 (set -> 0xfa3, else 0xfa4) in
   0x25b8; option +0xad9 (set -> 0xfa3, else 0xfa4) in 0x25ec; in a
   networked game byte 0x982 (set -> 0xfa3, else 0xfa4) in 0x25bb; the
   path finding (0x260e + byte 0x983 networked, else byte 0xa14) in
   0x25f1; the population limit in 0x25f0. Returns 0 (panel +0xd8 set)
   when the setup fails. */
int aoe_mp_wait_screen_init_ror(AoeMpWaitScreenRor *s, AoeGameUiRor *ui, AoeUiAreaRor *area);
/* The screen's vtable: destroy (0x0048dde0, deleting 0x0048ddc0: every
   panel closed then the screen's 0x00468170) and action (+0xb8
   0x0048de70: a press on the hidden close button closes the window,
   0x00419930; others to the easy panel's 0x00457e50). */
const AoeUiEzVtblRor *aoe_mp_wait_screen_vtable_ror(void);

#endif
