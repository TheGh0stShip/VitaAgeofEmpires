#ifndef VITA_AOE_UI_DROPDOWN_H
#define VITA_AOE_UI_DROPDOWN_H

#include <stdint.h>

#include "ui_button.h"
#include "ui_list.h"
#include "ui_scrollbar.h"
#include "ui_text.h"

/* Patched Rise of Rome 1.0a drop-down panel (pnl_drop, vtable 0x00545050,
   0x00460a00..0x00461f50): a text panel showing the selection, a button
   that opens the list, the list itself and its scrollbar. State 1 is
   closed, 2 open; opening makes the panel modal and grows it over the
   list (upward when the list would leave the drawing area). The parent
   hears action 0 when a line is chosen, 1 when it opens and 2 when it
   closes. Drawing is the base panel's (0x00460e70 forwards to it).
   Semantic storage; offsets are noted. */

typedef struct {
    void *context;
    /* The panel system's modal stack (0x005830e8): 0x00451da0 returns to
       the previous modal panel (0 when there is none), 0x00451ce0 makes a
       panel (or none) modal. */
    int (*pop_modal)(void *context);
    void (*set_modal)(void *context, AoeUiPanelRor *panel);
} AoeUiDropdownOpsRor;

typedef struct AoeUiDropdownRor AoeUiDropdownRor;

/* The opening button (vtable 0x00545134): a button that knows its
   drop-down (+0x2b8). */
typedef struct {
    AoeUiButtonRor button;
    AoeUiDropdownRor *owner; /* +0x2b8 */
} AoeUiDropdownButtonRor;

struct AoeUiDropdownRor {
    AoeUiPanelRor panel;
    const AoeUiDropdownOpsRor *ops;
    AoeUiTextRor *text;                   /* +0xf4 */
    AoeUiDropdownButtonRor *button;       /* +0xf8 */
    AoeUiListRor *list;                   /* +0xfc */
    AoeUiScrollbarRor *scrollbar;         /* +0x100 */
    int32_t state;                        /* +0x104: 1 closed, 2 open */
    int16_t selected;                     /* +0x108 */
    int32_t text_w, text_h;               /* +0x10c, +0x110 */
    int32_t button_w, button_h;           /* +0x114, +0x118 */
    int32_t list_w, list_h;               /* +0x11c, +0x120 */
    int32_t scroll_w;                     /* +0x124 */
    int32_t scroll_field_11c, scroll_thumb; /* +0x128, +0x12c */
    uint8_t opened_up;                    /* +0x130 */
    int32_t line_height;                  /* +0x134 */
    int32_t border_color;                 /* +0x138 */
    int32_t button_left;                  /* +0x13c: 1 puts the button first */
    int32_t border;                       /* +0x140, 1 */
    AoeUiTextScrollOpsRor list_scroll;    /* the list drives its scrollbar */
};

extern const AoeUiPanelVtblRor aoe_ui_dropdown_vtable_ror;

/* 0x00460a00 constructor; 0x00460a80 destructor (deleting 0x00460a60)
   destroys the button, list, scrollbar and text. */
void aoe_ui_dropdown_init_ror(AoeUiDropdownRor *d, AoeUiSystemRor *system,
                              const AoeUiDropdownOpsRor *ops);
void aoe_ui_dropdown_release_ror(AoeUiDropdownRor *d);
/* 0x00460b00 setup: the panel at (x, y, w, h); the text (w - button_w - 1
   wide) showing lines[selected]; the button (draw type 6, notify); the
   list (hover-select, hidden) with `count` lines; the scrollbar beside the
   list (range `count`), then closed. The original also takes an unused
   argument between the button height and the list width. */
int aoe_ui_dropdown_setup_ror(
    AoeUiDropdownRor *d, AoeUiAreaRor *area, AoeUiPanelRor *parent,
    uintptr_t font, int32_t font_width, int32_t line_height, int32_t fill,
    uint8_t fill_color, int32_t border, uint32_t border_color, int32_t x,
    int32_t y, int32_t w, int32_t h, int16_t max_chars, int16_t selected,
    int32_t button_w, int32_t button_h, int32_t list_w, int32_t list_h,
    const char *const *lines, int16_t count, int32_t scroll_w,
    int32_t scroll_field_11c, int32_t scroll_thumb);
/* 0x004613d0: open (2) or close (1) the list, laying the parts out. */
void aoe_ui_dropdown_set_state_ror(AoeUiDropdownRor *d, int32_t state);
/* 0x00461130 text colours; 0x00461150 button side (re-lays out the
   current state); 0x00461180 border; 0x004611c0 bevel (the text maps
   types 2/3/4 to 5/6/7); 0x00461370 the button picture (none: frame -1)
   and the scrollbar pictures. */
void aoe_ui_dropdown_set_colors_ror(AoeUiDropdownRor *d, uint32_t color,
                                    uint32_t shadow);
void aoe_ui_dropdown_set_button_left_ror(AoeUiDropdownRor *d, int32_t left);
void aoe_ui_dropdown_set_border_ror(AoeUiDropdownRor *d, int32_t border);
void aoe_ui_dropdown_set_bevel_ror(AoeUiDropdownRor *d, int32_t type,
                                   const uint8_t colors[6]);
void aoe_ui_dropdown_set_pictures_ror(AoeUiDropdownRor *d, void *picture,
                                      int16_t frame, void *scroll1,
                                      void *scroll2, void *scroll3,
                                      void *scroll4);
/* List access (0 / nothing without a list): 0x00461c50 selected line,
   0x00461c70 / 0x00461d50 select (the text follows), 0x00461c80 select by
   ID, 0x00461ce0 / 0x00461eb0 selected text, 0x00461d30 sorted,
   0x00461da0 append, 0x00461dd0 append a string ID, 0x00461e00 insert,
   0x00461e30 remove, 0x00461e50 selection, 0x00461e70 index of an ID,
   0x00461e90 index of a text, 0x00461ed0 selected ID, 0x00461ef0 ID of a
   line, 0x00461f10 clear. */
int16_t aoe_ui_dropdown_selected_ror(const AoeUiDropdownRor *d);
void aoe_ui_dropdown_select_ror(AoeUiDropdownRor *d, int16_t index);
void aoe_ui_dropdown_select_id_ror(AoeUiDropdownRor *d, int32_t id);
const char *aoe_ui_dropdown_selected_text_ror(const AoeUiDropdownRor *d);
void aoe_ui_dropdown_set_sorted_ror(AoeUiDropdownRor *d, int32_t sorted);
int aoe_ui_dropdown_append_ror(AoeUiDropdownRor *d, const char *text,
                               int32_t id);
int aoe_ui_dropdown_append_id_ror(AoeUiDropdownRor *d, int32_t string_id,
                                  int32_t id);
int aoe_ui_dropdown_insert_ror(AoeUiDropdownRor *d, int32_t index,
                               const char *text, int32_t id);
int aoe_ui_dropdown_remove_ror(AoeUiDropdownRor *d, int32_t index);
int32_t aoe_ui_dropdown_selection_ror(const AoeUiDropdownRor *d);
int32_t aoe_ui_dropdown_index_of_id_ror(const AoeUiDropdownRor *d,
                                        int32_t id);
int32_t aoe_ui_dropdown_index_of_ror(const AoeUiDropdownRor *d,
                                     const char *text);
int32_t aoe_ui_dropdown_selected_id_ror(const AoeUiDropdownRor *d);
int32_t aoe_ui_dropdown_id_ror(const AoeUiDropdownRor *d, int32_t index);
void aoe_ui_dropdown_clear_ror(AoeUiDropdownRor *d);
/* Slots: +0x0c 0x004610a0 (the parts follow the new rect), +0x58
   0x00460f80 (Up/Down open, Escape closes, with focus), +0x70 0x00460e80
   (a press outside the open list closes it and restores the selection),
   +0x74 0x00460f30 / +0x78 0x00460f50 (the base move and release), +0xb8
   0x00460ff0 (the button opens; a chosen line closes and tells the
   parent), +0xc8 0x00461d00 (the text leaves the tab order).
   The button: 0x00461f20 constructor, 0x00461f40 / 0x004f74f0
   destructors, +0x74 0x00461f50 (dragging into the list or its scrollbar
   hands the press to the list). */

#endif
