#ifndef VITA_AOE_UI_LIST_H
#define VITA_AOE_UI_LIST_H

#include <stdint.h>

#include "ui_text.h"

/* Patched Rise of Rome 1.0a list panel (pnl_lst, vtable 0x005453f8,
   0x00464a50..0x004656e0): a text panel whose selection follows the mouse
   and keyboard, auto-scrolls while a drag holds above or below it, and
   reports to its parent (action 1 selected, 2 released, 3 chosen).
   Drawing 0x00465340 / 0x00465470 is not translated. */
typedef struct {
    AoeUiTextRor text;
    int32_t hover_select;     /* +0x188 */
    uint32_t scroll_ms;       /* +0x18c */
    uint32_t scroll_interval; /* +0x190, 50 */
    int32_t scroll_up;        /* +0x194 */
    int32_t scroll_down;      /* +0x198 */
    int32_t highlight;        /* +0x19c, 1 */
} AoeUiListRor;

extern const AoeUiTextVtblRor aoe_ui_list_vtable_ror;

/* 0x00464a50 constructor; 0x00464ae0 / 0x00464ac0 destructors. */
void aoe_ui_list_init_ror(AoeUiListRor *l, AoeUiSystemRor *system);
void aoe_ui_list_release_ror(AoeUiListRor *l);
/* 0x00464e60: the line under y (rows of pitch - 1 + 2 * spacing, or the
   pitch without spacing, from the top margin). */
int32_t aoe_ui_list_row_at_ror(const AoeUiListRor *l, int32_t y);
/* 0x004650c0: move the selection. 1 to `value` (clamped, scrolling it
   into view), 2 down, 3 up (scrolling at the window edge), 4 page down
   and 5 page up (to the window edge, then a page), 6 first, 7 last. The
   parent hears action 1 when the selection changed. */
void aoe_ui_list_move_ror(AoeUiListRor *l, uint8_t how, int16_t value,
                          int32_t update);
/* 0x00464eb0: select the line under y. */
void aoe_ui_list_select_at_ror(AoeUiListRor *l, int32_t x, int32_t y);
/* 0x004656d0 highlight bar on/off. */
void aoe_ui_list_set_highlight_ror(AoeUiListRor *l, int32_t on);
/* Slots: +0x4c 0x00464b90 idle (auto-scroll every interval), +0x80
   0x00464c40 hover, +0x84 0x00464cb0 press, +0x8c 0x00464ce0 drag
   (outside the panel arms auto-scroll), +0x90 0x00464db0 release, +0x94
   0x00464e10 double click, +0xac 0x00464ee0 keys (Enter, PageUp/Down,
   End, Home, Up, Down), +0xb4 0x00464ff0 type-ahead on the first letter
   (case-insensitive, after the selection then wrapping), +0xb8 0x004650a0
   (the text action), +0xc4 0x004656e0 focus (reports the selection),
   +0xf0 0x00464af0 bevel (spacing 1, or 2 for types 3/6 and 3 for 4/7;
   a zero margin becomes 1). */

#endif
