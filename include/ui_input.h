#ifndef VITA_AOE_UI_INPUT_H
#define VITA_AOE_UI_INPUT_H

#include <stdint.h>

#include "ui_text.h"

/* Patched Rise of Rome 1.0a text input panel (pnl_inp, vtable 0x00545304,
   0x00463650..0x00464910): a text panel holding an editable string with a
   blinking caret. Edits copy between two buffers (+0x1a4 / +0x1a8; +0x1a0
   points at the current one). The caret is a character offset (+0x1b4)
   kept in step with the text panel's line (+0x100) and column (+0x102).
   Formats (+0x19c): 1 number with '.', 2 integer, 3 unsigned integer,
   4 file name, 5 file name without '.', 6 path, 7 multi-line text, 8 a
   percentage up to 100; anything else is free text. Clipboard and beeps
   are callbacks. */
typedef struct {
    void *context;
    /* The sound object's play when a sound is set, else MessageBeep. */
    void (*beep)(void *context, void *sound);
    /* CF_TEXT clipboard text (at most `size` - 1 bytes); length or -1. */
    int32_t (*clipboard)(void *context, char *out, int32_t size);
} AoeUiInputOpsRor;

typedef struct {
    AoeUiTextRor text;
    uint8_t caret_color;      /* +0x188 */
    uint32_t blink_ms;        /* +0x18c, GetCaretBlinkTime */
    uint32_t blink_at;        /* +0x190 */
    int32_t caret_on;         /* +0x194 */
    void *sound;              /* +0x198 */
    int32_t format;           /* +0x19c */
    char *current;            /* +0x1a0 */
    char *buffers[2];         /* +0x1a4, +0x1a8: 2 * max + 1 bytes each */
    int16_t length, max;      /* +0x1ac, +0x1ae */
    int32_t insert;           /* +0x1b0, 1 (toggled, never consulted) */
    int16_t cursor;           /* +0x1b4 */
    const AoeUiInputOpsRor *ops;
} AoeUiInputRor;

extern const AoeUiTextVtblRor aoe_ui_input_vtable_ror;

/* +0x30 0x00464140: the text, then while focused with the caret on and
   the caret's line visible a two-pixel caret in the caret colour after
   the line's first `column_chars` characters (inside the panel). */
void aoe_ui_input_draw_slot_ror(AoeUiPanelRor *p);

/* 0x00463650 constructor (blink_ms from the platform, 530 by default);
   0x00463700 / 0x004636e0 destructors. */
void aoe_ui_input_init_ror(AoeUiInputRor *in, AoeUiSystemRor *system,
                           const AoeUiInputOpsRor *ops, uint32_t blink_ms);
void aoe_ui_input_release_ror(AoeUiInputRor *in);
/* 0x00463790: the text setup (no line limit for format 7), caret colour,
   the buffers for `max` characters, beep sound, text and format (7:
   top/left aligned and wrapping; else clipped). 0 on failure. */
int aoe_ui_input_setup_ror(AoeUiInputRor *in, AoeUiAreaRor *area,
                           AoeUiPanelRor *parent, int32_t x, int32_t y,
                           int32_t w, int32_t h, uintptr_t font,
                           int32_t font_width, int32_t line_height,
                           void *background, int16_t background_w,
                           int16_t background_h, int32_t fill,
                           uint8_t fill_color, int32_t border,
                           uint8_t border_color, uint8_t caret_color,
                           int16_t max, void *sound, const char *text,
                           int32_t format);
/* +0xec 0x00464290: replace the text (the caret goes to the end unless
   multi-line). */
int aoe_ui_input_set_text_ror(AoeUiTextRor *t, const char *text);
const char *aoe_ui_input_text_ror(const AoeUiInputRor *in);
/* 0x00464310 insert a character at the caret (beeps when full),
   0x00464420 delete at the caret (a CR LF pair goes together),
   0x004645b0 backspace, 0x004645d0 paste. Return 1 when changed. */
int aoe_ui_input_insert_ror(AoeUiInputRor *in, char ch);
int aoe_ui_input_delete_ror(AoeUiInputRor *in);
int aoe_ui_input_backspace_ror(AoeUiInputRor *in);
int aoe_ui_input_paste_ror(AoeUiInputRor *in);
/* 0x00464790 re-split the text keeping the scroll; 0x004647f0 caret from
   line and column; 0x00464910 line and column from the caret. Both skip
   back over a newline just before the caret and keep it in view. */
void aoe_ui_input_relayout_ror(AoeUiInputRor *in);
void aoe_ui_input_caret_from_line_ror(AoeUiInputRor *in);
void aoe_ui_input_line_from_caret_ror(AoeUiInputRor *in);
/* Slots: +0x4c 0x00463920 idle (caret blink), +0xac 0x00463bd0 keys,
   +0xb4 0x00463980 characters (format filter, repeated `count` times),
   +0xc4 0x004638f0 focus (caret on). */

#endif
