#ifndef VITA_AOE_SHELL_OSK_H
#define VITA_AOE_SHELL_OSK_H

#include <stdint.h>

/* The console's on-screen keyboard for the game's text fields (the
   player name, chat, scenario names). Port integration, not an original
   feature: the PC game read the keyboard. The shell shows it while an
   EDIT control has the focus and turns its keys into the window
   messages a keyboard would send (WM_CHAR, Backspace, Enter, the arrow
   keys).

   Five rows of ten units: digits, three letter rows (with - , . _), then
   Shift (2), Space (4), Delete (2) and Done (2). */

enum { AOE_OSK_ROWS = 5, AOE_OSK_UNITS = 10 };

typedef enum {
    AOE_OSK_NONE,
    AOE_OSK_CHAR,       /* `code` is the character */
    AOE_OSK_SHIFT,
    AOE_OSK_BACKSPACE,
    AOE_OSK_ENTER
} AoeOskKeyKind;

typedef struct {
    int visible;
    int shift;               /* the next letter is a capital */
    int row, col;            /* the highlighted key (col in key order) */
    int32_t x, y, w, h;      /* the keyboard in the output */
    int32_t unit_w, row_h;
} AoeOskRor;

/* Places the keyboard along the bottom of a `out_w` x `out_h` output,
   highlighting the first letter, with Shift on. */
void aoe_osk_init_ror(AoeOskRor *k, int32_t out_w, int32_t out_h);
/* The number of keys in a row. */
int aoe_osk_row_keys_ror(int row);
/* The key under an output point: 1 with its row and column, else 0. */
int aoe_osk_hit_ror(const AoeOskRor *k, int32_t x, int32_t y, int *row, int *col);
/* What key (row, col) does; for a character, `code` with Shift applied
   to letters. */
AoeOskKeyKind aoe_osk_key_ror(const AoeOskRor *k, int row, int col, int32_t *code);
/* Moves the highlight by whole keys (rows keep the nearest column by
   position; it wraps). */
void aoe_osk_move_ror(AoeOskRor *k, int dx, int dy);
/* A key's rectangle in the output (x, y, w, h). */
void aoe_osk_key_rect_ror(const AoeOskRor *k, int row, int col, int32_t r[4]);
/* Draws the keyboard into an A8B8G8R8 frame (row stride in pixels),
   over a darkened band. */
void aoe_osk_draw_ror(const AoeOskRor *k, uint32_t *out, int32_t stride, int32_t out_w,
                      int32_t out_h);

#endif
