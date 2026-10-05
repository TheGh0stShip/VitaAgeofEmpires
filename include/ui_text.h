#ifndef VITA_AOE_UI_TEXT_H
#define VITA_AOE_UI_TEXT_H

#include <stdint.h>

#include "ui_panel.h"

/* Patched Rise of Rome 1.0a text panel (pnl_txt, vtable 0x00545a90,
   0x004685b0..0x0046a760): a list of lines (text, second column text, ID)
   with a selection, scrolling, an optional scrollbar child and word wrap.
   Text is single-byte here; the original also steps double-byte
   characters with the _mbs* functions, which the port does not model.
   GDI drawing is not translated. */

typedef struct AoeUiLineRor {
    char *text;               /* +0 */
    char *column;             /* +4 */
    int32_t id;               /* +8 */
    struct AoeUiLineRor *next;/* +0xc */
} AoeUiLineRor;

typedef struct AoeUiTextRor AoeUiTextRor;

typedef struct {
    AoeUiPanelVtblRor base;
    int (*set_lines)(AoeUiTextRor *, const char *const *, int16_t);   /* +0xe4 */
    int (*set_text_id)(AoeUiTextRor *, int32_t);                      /* +0xe8 */
    int (*set_text)(AoeUiTextRor *, const char *);                    /* +0xec */
    void (*set_bevel)(AoeUiTextRor *, int32_t, const uint8_t[6]);     /* +0xf0 */
} AoeUiTextVtblRor;

/* The scrollbar child (+0x130) as the text panel drives it. */
typedef struct {
    void *context;
    void (*set_range)(void *context, int32_t max, int32_t position); /* 0x00467310 */
} AoeUiTextScrollOpsRor;

struct AoeUiTextRor {
    AoeUiPanelRor panel;
    AoeUiLineRor *lines;          /* +0xf4 */
    int16_t count;                /* +0xf8 */
    int16_t visible;              /* +0xfa */
    int16_t top, bottom;          /* +0xfc, +0xfe */
    int16_t selected;             /* +0x100 */
    int16_t column_chars;         /* +0x102: characters before column 2 */
    uintptr_t font;               /* +0x104 */
    int32_t font_width;           /* +0x108 */
    int32_t line_height;          /* +0x10c */
    int32_t vertical;             /* +0x110: 3 top, 4 bottom, else centre */
    int32_t align;                /* +0x114: 1 left, 2 right, 5 clip, else centre */
    int32_t wrap;                 /* +0x118 */
    int32_t shadow_style;         /* +0x11c, 2 */
    uint32_t color, shadow;       /* +0x120, +0x124 */
    uint32_t focus_color, focus_shadow; /* +0x128 (0xffff), +0x12c */
    AoeUiPanelRor *scrollbar;     /* +0x130 */
    int32_t has_scrollbar;        /* +0x134 */
    int16_t max_chars;            /* +0x138 */
    int32_t text_width;           /* +0x13c */
    int32_t pitch;                /* +0x140 */
    void *background;             /* +0x144 */
    int16_t background_w, background_h; /* +0x148, +0x14a */
    int32_t fill;                 /* +0x14c */
    uint8_t fill_color;           /* +0x150, 0xff */
    int32_t border;               /* +0x154 */
    uint8_t border_color;         /* +0x158 */
    int32_t sorted;               /* +0x15c */
    int32_t column_x;             /* +0x160 */
    int32_t full_width;           /* +0x164 */
    int32_t scrollbar_width;      /* +0x168 */
    int32_t scrollbar_shown;      /* +0x16c */
    int32_t bevel_type;           /* +0x170 */
    uint8_t bevel[6];             /* +0x174 */
    AoeUiRectRor column_clip;     /* +0x17c: region of the first column */
    int32_t has_column_clip;
    int32_t margin;               /* +0x180 */
    int32_t spacing;              /* +0x184 */
    const AoeUiTextScrollOpsRor *scroll;
};

extern const AoeUiTextVtblRor aoe_ui_text_vtable_ror;

/* 0x004685b0 constructor, 0x00468720 destructor (deleting 0x00468700). */
void aoe_ui_text_init_ror(AoeUiTextRor *t, AoeUiSystemRor *system);
void aoe_ui_text_release_ror(AoeUiTextRor *t);
/* Line list: insert 0x0046a1b0 at an index (or sorted by text when
   +0x15c), append 0x0046a150 / 0x0046a190, from a string ID 0x0046a170 /
   0x0046a3d0, insert-and-remove-next 0x0046a420, remove 0x0046a480,
   clear 0x0046a6d0. A line longer than max_chars is cut. */
int aoe_ui_text_insert_ror(AoeUiTextRor *t, int32_t index, const char *text,
                           const char *column, int32_t id);
int aoe_ui_text_append_ror(AoeUiTextRor *t, const char *text, int32_t id);
int aoe_ui_text_append_id_ror(AoeUiTextRor *t, int32_t string_id, int32_t id);
int aoe_ui_text_replace_ror(AoeUiTextRor *t, int32_t index, const char *text,
                            const char *column, int32_t id);
int aoe_ui_text_remove_ror(AoeUiTextRor *t, int32_t index);
void aoe_ui_text_clear_ror(AoeUiTextRor *t);
/* Lookups: 0x0046a0b0 count, 0x0046a0c0 visible, 0x0046a0d0 selected,
   0x0046a550 selected (-1 when empty), 0x0046a570 index of an ID,
   0x0046a5a0 index of a text, 0x0046a610 / 0x0046a600 text of a line /
   the selection ("" past the end), 0x0046a640 column text, 0x0046a690 /
   0x0046a680 ID (-1 past the end). */
int16_t aoe_ui_text_count_ror(const AoeUiTextRor *t);
int16_t aoe_ui_text_visible_ror(const AoeUiTextRor *t);
int16_t aoe_ui_text_selected_ror(const AoeUiTextRor *t);
int32_t aoe_ui_text_selection_ror(const AoeUiTextRor *t);
int32_t aoe_ui_text_index_of_id_ror(const AoeUiTextRor *t, int32_t id);
int32_t aoe_ui_text_index_of_ror(const AoeUiTextRor *t, const char *text);
const char *aoe_ui_text_line_ror(const AoeUiTextRor *t, int32_t index);
const char *aoe_ui_text_selected_text_ror(const AoeUiTextRor *t);
const char *aoe_ui_text_column_ror(const AoeUiTextRor *t, int32_t index);
int32_t aoe_ui_text_id_ror(const AoeUiTextRor *t, int32_t index);
int32_t aoe_ui_text_selected_id_ror(const AoeUiTextRor *t);
/* 0x0046a100 select (and re-lay out; 0x0046a0e0 forwards to it),
   0x0046a120 select by ID. */
void aoe_ui_text_select_ror(AoeUiTextRor *t, int16_t index);
void aoe_ui_text_select_id_ror(AoeUiTextRor *t, int32_t id);
/* 0x00469310: visible lines from the height, then the window and
   selection are clamped; the scrollbar range follows (`update`) and it is
   shown or hidden as lines overflow. */
void aoe_ui_text_layout_ror(AoeUiTextRor *t, int32_t update);
/* 0x00469500 scroll: 0 by delta, 1 to position, 2 down one, 3 up one, 4
   page down, 5 page up, 6 top, 7 bottom. 0x004695c0 keys (PageUp/Down,
   End, Home, Up, Down), passing paging keys to the parent. 0x00469690:
   the scrollbar's actions. */
void aoe_ui_text_scroll_ror(AoeUiTextRor *t, uint8_t how, int16_t value,
                            int32_t update);
/* Settings: 0x00469010 font, width, line height; 0x00469050 wrap;
   0x00469070 / 0x00469090 colours; 0x004690b0 background picture (via
   the loader); 0x00469190 border (bevel type 1 when on); 0x004691e0 bevel (+0xf0); 0x004692a0
   spacing; 0x0046a740 sorted; 0x00468a30 scrollbar. Slots: +0x0c
   0x00468910 (remember the full width, re-place the scrollbar, lay out,
   and with a second column clip the first one to end 10 pixels before
   it),
   +0x14 0x004692c0 (the scrollbar follows), +0x84 0x0046a090 (left press
   notifies the parent, action 1). */
void aoe_ui_text_set_font_ror(AoeUiTextRor *t, uintptr_t font, int32_t width,
                              int32_t line_height);
void aoe_ui_text_set_wrap_ror(AoeUiTextRor *t, int32_t wrap);
void aoe_ui_text_set_colors_ror(AoeUiTextRor *t, uint32_t color,
                                uint32_t shadow);
void aoe_ui_text_set_focus_colors_ror(AoeUiTextRor *t, uint32_t color,
                                      uint32_t shadow);
void aoe_ui_text_set_background_ror(AoeUiTextRor *t, void *picture,
                                    int16_t width, int16_t height);
void aoe_ui_text_set_border_ror(AoeUiTextRor *t, int32_t border,
                                uint8_t color);
void aoe_ui_text_set_bevel_ror(AoeUiTextRor *t, int32_t type,
                               const uint8_t colors[6]);
void aoe_ui_text_set_spacing_ror(AoeUiTextRor *t, int32_t spacing);
void aoe_ui_text_set_sorted_ror(AoeUiTextRor *t, int32_t sorted);
void aoe_ui_text_set_scrollbar_ror(AoeUiTextRor *t, AoeUiPanelRor *scrollbar,
                                   int32_t shown);
/* Text: 0x00468ac0 (split at newlines, or wrapped when wrapping or align
   6), 0x00468ba0 word wrap to the width - 10 (breaks at a space, keeps
   it; a newline becomes " "), 0x00468fc0 lines from an array,
   0x00468f80 from a string ID. */
int aoe_ui_text_set_text_ror(AoeUiTextRor *t, const char *text);
int aoe_ui_text_wrap_ror(AoeUiTextRor *t, const char *text);
int aoe_ui_text_set_lines_ror(AoeUiTextRor *t, const char *const *lines,
                              int16_t count);
int aoe_ui_text_set_text_id_ror(AoeUiTextRor *t, int32_t string_id);
/* 0x00469e60: the screen rect of a line slot `row` showing `line`, and
   the column-2 offset; 0x0046a760 for row 0. */
void aoe_ui_text_line_rect_ror(AoeUiTextRor *t, int16_t row, int16_t line,
                               AoeUiRectRor *out, int32_t *column_offset);
/* 0x00468790 setup: the panel, fill, border, line limit, the text (slot
   +0xec), the font, alignment 0/0 (0x004f7cf0) and the background picture
   (0x004690b0 loads it by name; here it is passed loaded with its size). */
int aoe_ui_text_setup_ror(AoeUiTextRor *t, AoeUiAreaRor *area,
                          AoeUiPanelRor *parent, int32_t x, int32_t y,
                          int32_t w, int32_t h, uintptr_t font,
                          int32_t font_width, int32_t line_height,
                          void *background, int16_t background_w,
                          int16_t background_h, int32_t fill,
                          uint8_t fill_color, int32_t border,
                          uint8_t border_color, int16_t max_chars,
                          const char *text);
/* 0x00468850 setup from a line array: the panel, fill, border, line limit,
   the lines (slot +0xe4), the font, alignment 3/1 (top, left) and the
   background picture. */
int aoe_ui_text_setup_lines_ror(AoeUiTextRor *t, AoeUiAreaRor *area,
                                AoeUiPanelRor *parent, int32_t x, int32_t y,
                                int32_t w, int32_t h, uintptr_t font,
                                int32_t font_width, int32_t line_height,
                                void *background, int16_t background_w,
                                int16_t background_h, int32_t fill,
                                uint8_t fill_color, int32_t border,
                                uint8_t border_color, int16_t max_chars,
                                const char *const *lines, int16_t count);
/* 0x004f7cf0: vertical (+0x110) and horizontal (+0x114) alignment. */
void aoe_ui_text_set_alignment_ror(AoeUiTextRor *t, int32_t vertical,
                                   int32_t align);
/* +0x84 0x0046a090: a left press notifies the parent (action 1). */
int32_t aoe_ui_text_left_down_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                  int32_t c, int32_t s);
/* Drawing (+0x30 0x00469750): the background (0x00469860: tiled picture,
   which the original only tiles when the panel is no taller than a tile;
   or a fill; or the parent's rect), then the visible lines (0x00469be0:
   left-aligned 5 pixels in except alignments 0 and 6, a second column at
   column_x, shadow style 1 or 2 drawing a copy one pixel down-left), in
   the focus colours when the panel has focus or is captured, then the
   border (0x004699f0: bevel types 1..7). 0x0046a0f0 and 0x0046a6c0 are
   thunks to the line text 0x0046a600 and the clear 0x0046a6d0. */
void aoe_ui_text_draw_slot_ror(AoeUiPanelRor *p);
/* The pieces, for subclasses: background, one line (inside the platform's
   text_begin / text_end), border. */
void aoe_ui_text_draw_background_ror(AoeUiTextRor *t);
void aoe_ui_text_draw_line_ror(AoeUiTextRor *t, int16_t row, int16_t line,
                               uint32_t color, uint32_t shadow);
void aoe_ui_text_draw_border_ror(AoeUiTextRor *t);
/* Text vtable slots shared with subclasses: +0x0c 0x00468910, +0x14
   0x004692c0, +0xac 0x004695c0 keys, +0xb8 0x00469690 action, +0xe4
   0x00468fc0 lines, +0xf0 0x004691e0 bevel. */
void aoe_ui_text_set_rect_slot_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                   int32_t w, int32_t h);
void aoe_ui_text_set_active_slot_ror(AoeUiPanelRor *p, int32_t active);
int32_t aoe_ui_text_key_action_ror(AoeUiPanelRor *p, int32_t key,
                                   int32_t count, int32_t a, int32_t b,
                                   int32_t c);
int32_t aoe_ui_text_action_ror(AoeUiPanelRor *p, AoeUiPanelRor *from,
                               int32_t code, int32_t value, int32_t extra);
int aoe_ui_text_lines_slot_ror(AoeUiTextRor *t, const char *const *lines,
                               int16_t count);
void aoe_ui_text_bevel_slot_ror(AoeUiTextRor *t, int32_t type,
                                const uint8_t colors[6]);

/* 0x00468a80: 2 for a double-byte lead byte (callback), 1 for half-width
   katakana 0xa1..0xdf, else 0. */
int aoe_ui_text_char_kind_ror(const uint8_t *text,
                              int (*dbcs_lead)(uint8_t byte));

/* The campaign editor screen's two lists (+0x48c, +0x490):
   0x0047a170 / 0x0047a1f0 move the selected line from one list to the
   other (appended there and selected, removed here, then the target
   scrolls to it through 0x004650c0(1, selection, 1)); 0x0047a270 moves
   the selected line of +0x490 up one place. Each marks the screen
   changed (+0x478), the move up only when something was selected past
   the first line. */
/* 0x00479c90: when every line of +0x490 is also in +0x48c, those lines
   are removed from +0x48c (1); else nothing changes (0). */
int aoe_ui_text_take_chosen_ror(AoeUiTextRor *available, const AoeUiTextRor *chosen);
typedef void (*AoeUiTextRevealFn)(void *context, AoeUiTextRor *t, int32_t index);
void aoe_ui_text_move_selected_ror(AoeUiTextRor *from, AoeUiTextRor *to, AoeUiTextRevealFn reveal,
                                   void *context, uint8_t *changed);
void aoe_ui_text_move_up_ror(AoeUiTextRor *t, AoeUiTextRevealFn reveal, void *context,
                             uint8_t *changed);

#endif
