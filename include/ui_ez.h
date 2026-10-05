#ifndef VITA_AOE_UI_EZ_H
#define VITA_AOE_UI_EZ_H

#include <stdint.h>

#include "ui_button.h"
#include "ui_dropdown.h"
#include "ui_edit.h"
#include "ui_input.h"
#include "ui_list.h"
#include "ui_scrollbar.h"
#include "ui_text.h"

/* Patched Rise of Rome 1.0a "easy" panel (panel_ez, vtable 0x00544bbc,
   0x00454430..0x00457e50): the base of the game's screens and dialogs. It
   reads a screen info file (backgrounds per resolution, palette, mouse
   pointer, shadow, button pictures, popup info, colours and bevel), and
   creates child panels laid out on a 640x480 reference grid scaled to its
   own size. Background drawing and the shadow area are not translated. A zero reference size leaves positions
   unscaled (the original divides by zero). Semantic storage; offsets
   are noted. */

typedef struct AoeUiEzRor AoeUiEzRor;

typedef struct {
    void *context;
    /* The game's font table +0x1e4: font, width, height. 0 when there is
       no table. */
    int (*font)(void *context, int32_t index, uintptr_t *font,
                int32_t *width, int32_t *height);
    /* The game's sound table +0x6c. */
    void *(*sound)(void *context, int32_t id);
    /* A shape from "<file>" or, with no file, resource `id`, checked as
       loaded; NULL when it does not load. free_shape frees it. */
    void *(*load_shape)(void *context, const char *file, int32_t id);
    void (*free_shape)(void *context, void *shape);
    /* The shadow table for a palette and amount, and its deletion. */
    void *(*create_shadow)(void *context, AoeUiAreaRor *area, int32_t amount,
                           uintptr_t palette);
    void (*free_shadow)(void *context, void *shadow);
    /* The info text: "<name>.sin" from disk when loose files are on
       (0x007c0464), else resource 'bina' `id`. A
       NUL-terminated buffer the panel frees with free(), or NULL. */
    char *(*load_info)(void *context, const char *file, int32_t id);
    int loose_files;
    /* The mouse pointer picture (name, id), done only when the game has
       a pointer object (+0x1c0). */
    int (*has_pointer)(void *context);
    void (*set_pointer)(void *context, const char *name, int32_t id);
    /* Palettes: the area's current one, the game default (+0x1c), and
       switching the area (clear, flip, set). */
    uintptr_t (*area_palette)(void *context, AoeUiAreaRor *area);
    uintptr_t (*default_palette)(void *context);
    void (*set_area_palette)(void *context, AoeUiAreaRor *area,
                             uintptr_t palette);
    /* IME association on / off. */
    void (*ime)(void *context, int on);
    /* Pointer shape and mode: LoadCursor(arrow / help) and the game's
       pointer mode; the pointer object's +0x13c. */
    void (*cursor)(void *context, int help, int32_t mode);
    int32_t (*pointer_state)(void *context);
    void (*set_pointer_state)(void *context, int32_t state);
    /* Whether this is a single-player game (game +0x97d), and WinHelp
       by keyword. */
    int (*single_player)(void *context);
    void (*win_help_key)(void *context, const char *keyword);
    /* The help popup; WinHelp for help mode 1. */
    void (*help_popup)(void *context, const char *panel_name,
                       const char *text, int32_t page, int32_t x, int32_t y);
    void (*win_help)(void *context);
    uint8_t *help_mode;                 /* 0x00583bc0: 1 WinHelp, 2 shown */
    /* Child panel callbacks. */
    const AoeUiButtonOpsRor *button_ops;
    const AoeUiInputOpsRor *input_ops;
    uint32_t caret_blink_ms;
    const AoeUiDropdownOpsRor *dropdown_ops;
    const AoeUiEditOpsRor *edit_ops;
    /* The game's idle work, skipped while its +0x1d8 flag is set (the
       message dialog's idle). */
    void (*game_idle)(void *context);
    int (*idle_blocked)(void *context);
    /* Rebuild the shadow area (not translated). */
    void (*shadow_area)(void *context, AoeUiEzRor *ez, int32_t force);
} AoeUiEzOpsRor;

typedef struct {
    AoeUiPanelVtblRor base;
    void (*draw_background)(AoeUiEzRor *, int32_t);                 /* +0xe4 */
    void (*set_reference)(AoeUiEzRor *, int32_t, int32_t);          /* +0xe8 */
} AoeUiEzVtblRor;

struct AoeUiEzRor {
    AoeUiPanelRor panel;
    const AoeUiEzOpsRor *ops;
    int32_t ref_w, ref_h;              /* +0xf4 640, +0xf8 480 */
    char info_name[0x104];             /* +0xfc */
    int32_t info_id;                   /* +0x200, -1 */
    void *background;                  /* +0x204 */
    void *background2;                 /* +0x208 */
    uintptr_t palette;                 /* +0x20c */
    char pointer_name[0x104];          /* +0x210 */
    int32_t pointer_id;                /* +0x314, -1 */
    int32_t background_mode;           /* +0x318: 0 at the origin, 1 centred, 2 tiled */
    int32_t bevel_on;                  /* +0x31c */
    uint8_t bevel[6];                  /* +0x320..+0x325 */
    uint32_t text_color, text_shadow;  /* +0x328 0xffffff, +0x32c */
    uint32_t focus_color, focus_shadow;/* +0x330 0xffff, +0x334 */
    uint32_t hot_color, hot_shadow;    /* +0x338 0xffffff, +0x33c */
    char popup_name[0x104];            /* +0x340 */
    int32_t popup_id;                  /* +0x444, -1 */
    void *button_pictures;             /* +0x448 */
    void *shadow_table;                /* +0x44c */
    int32_t shadow_amount;             /* +0x450 */
    uint8_t fill_color, fill_color2;   /* +0x454, +0x455 */
    int32_t ime_toggle;                /* +0x458 */
    uint8_t help_armed;                /* +0x45c */
    int32_t stock_brush;               /* +0x460 */
    uintptr_t brush;                   /* +0x464 */
    int32_t field_468;                 /* +0x468 */
    void *shadow_panel;                /* +0x46c */
    int32_t field_470;                 /* +0x470 */
    int32_t saved_pointer;             /* +0x474 */
};

extern const AoeUiEzVtblRor aoe_ui_ez_vtable_ror;

/* 0x00454430 named constructor, 0x00454b10 default; both reset the
   fields with 0x00454b60. 0x00454c60 destructor (deleting 0x00454480)
   frees the pictures, palette and shadow table (0x00454cb0). */
int aoe_ui_ez_init_named_ror(AoeUiEzRor *ez, AoeUiSystemRor *system,
                             const AoeUiEzOpsRor *ops, const char *name);
void aoe_ui_ez_init_ror(AoeUiEzRor *ez, AoeUiSystemRor *system,
                        const AoeUiEzOpsRor *ops);
void aoe_ui_ez_reset_ror(AoeUiEzRor *ez);
void aoe_ui_ez_free_ror(AoeUiEzRor *ez);
void aoe_ui_ez_release_ror(AoeUiEzRor *ez);
/* 0x004544a0 setup: the info name and ID, full-area or centred (-1)
   placement, the reference size (+0xe8), position mode 1 (fill the
   parent) for a full-area panel, then the info: three background sets (640, 800 and 1024
   wide areas; "none" falls back to the smaller set), palette, pointer,
   shadow, button pictures, popup info, background mode, fill colour,
   bevel and three colour pairs as r g b triples. */
int aoe_ui_ez_setup_ror(AoeUiEzRor *ez, AoeUiAreaRor *area,
                        AoeUiPanelRor *parent, const char *info_name,
                        int32_t info_id, int32_t full, int32_t x, int32_t y,
                        int32_t w, int32_t h, int32_t field_470);
/* Settings: 0x004553f0 info name, 0x00455450 reference size (+0xe8),
   0x00455470 / 0x004555e0 the two backgrounds and 0x00455a50 the button
   pictures ("<name>.shp" unless it has a dot; "none" or no name loads
   resource `id` unless it is -1), 0x00455750 fill colour (and the stock
   black brush), 0x004557a0 palette (and the shadow table), 0x004558e0
   pointer, 0x00455930 bevel on, 0x00455940 background mode, 0x00455950
   bevel colours, 0x00455990 / 0x004559b0 / 0x004559d0 text, focus and
   hot colours, 0x004559f0 popup info, 0x00455bc0 shadow amount (-1 is 0;
   the table is rebuilt; an active panel redraws). Getters 0x00455cc0
   info name, 0x00455cd0 info ID, 0x00455ce0 palette, 0x00455cf0 popup
   name, 0x00455d00 popup ID. */
void aoe_ui_ez_set_info_ror(AoeUiEzRor *ez, const char *name, int32_t id);
int aoe_ui_ez_set_background_ror(AoeUiEzRor *ez, const char *name,
                                 int32_t id);
int aoe_ui_ez_set_background2_ror(AoeUiEzRor *ez, const char *name,
                                  int32_t id);
void aoe_ui_ez_set_button_pictures_ror(AoeUiEzRor *ez, const char *name,
                                       int32_t id);
void aoe_ui_ez_set_fill_color_ror(AoeUiEzRor *ez, uint8_t color);
int aoe_ui_ez_set_palette_ror(AoeUiEzRor *ez, const char *name, int32_t id);
int aoe_ui_ez_set_pointer_ror(AoeUiEzRor *ez, const char *name, int32_t id);
void aoe_ui_ez_set_bevel_on_ror(AoeUiEzRor *ez, int32_t on);
void aoe_ui_ez_set_background_mode_ror(AoeUiEzRor *ez, int32_t mode);
void aoe_ui_ez_set_bevel_colors_ror(AoeUiEzRor *ez, const uint8_t colors[6]);
void aoe_ui_ez_set_text_colors_ror(AoeUiEzRor *ez, uint32_t color,
                                   uint32_t shadow);
void aoe_ui_ez_set_focus_colors_ror(AoeUiEzRor *ez, uint32_t color,
                                    uint32_t shadow);
void aoe_ui_ez_set_hot_colors_ror(AoeUiEzRor *ez, uint32_t color,
                                  uint32_t shadow);
void aoe_ui_ez_set_popup_ror(AoeUiEzRor *ez, const char *name, int32_t id);
void aoe_ui_ez_set_shadow_amount_ror(AoeUiEzRor *ez, int32_t amount);
const char *aoe_ui_ez_info_name_ror(const AoeUiEzRor *ez);
int32_t aoe_ui_ez_info_id_ror(const AoeUiEzRor *ez);
uintptr_t aoe_ui_ez_palette_ror(const AoeUiEzRor *ez);
const char *aoe_ui_ez_popup_name_ror(const AoeUiEzRor *ez);
int32_t aoe_ui_ez_popup_id_ror(const AoeUiEzRor *ez);

/* Creators (vtable +0xec..+0x128). Positions and sizes are in reference
   units, scaled by width / ref_w and height / ref_h (truncating). A
   failure marks the panel (+0xd8). Font index < 0 is font 10.
   +0xf0 0x00455db0 button (draw type 3; sound ID < 0 none; second line;
   colours on all nine states, state 1 hot), +0xec 0x00455d10 the same
   from two string IDs (the second line is always set, possibly ""); +0xf4 0x00455fe0 check box (draw type 6 cycling
   two states over the button pictures 0 and 2, or a text "X" in draw
   type 3); +0xf8 0x00456240 radio button (draw type 2; pictures 4 and 6,
   or the same "X"); +0x104 0x00456530 text (left-aligned unless centred;
   top unless middle), +0xfc 0x004564a0 from a string ID; +0x100 0x004566d0 lines;
   +0x108 0x00456870 input (border 1 colour 0xff, caret 0xff); +0x110
   0x00456bf0 drop-down (one " " line; button and scrollbar 18 units
   wide; the button pictures at frames 14 and -1, 8, 10, 12); +0x114
   0x00456f20 list (one " " line, first selected; it reports success
   even when creation failed); +0x118 0x00457170
   scrollbar (width in pixels, unscaled; attached to a text panel);
   +0x11c 0x004572e0 a scrollbar for a text panel's parent, shown;
   +0x120 0x00457330 / +0x124 0x004574a0 vertical / horizontal value
   scrollbars; +0x128
   0x00457610 a child's scaled fixed position. */
int aoe_ui_ez_create_button_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                                AoeUiButtonRor **out, const char *text,
                                const char *text2, int32_t x, int32_t y,
                                int32_t w, int32_t h, int32_t font,
                                int32_t sound_id, int32_t command);
int aoe_ui_ez_create_button_id_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                                   AoeUiButtonRor **out, int32_t text_id,
                                   int32_t text2_id, int32_t x, int32_t y,
                                   int32_t w, int32_t h, int32_t font,
                                   int32_t sound_id, int32_t command);
int aoe_ui_ez_create_check_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                               AoeUiButtonRor **out, int32_t x, int32_t y,
                               int32_t w, int32_t h, int32_t sound_id,
                               int32_t command);
int aoe_ui_ez_create_radio_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                               AoeUiButtonRor **out, int32_t x, int32_t y,
                               int32_t w, int32_t h, int32_t sound_id,
                               int32_t command);
int aoe_ui_ez_create_text_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                              AoeUiTextRor **out, const char *text, int32_t x,
                              int32_t y, int32_t w, int32_t h, int32_t font,
                              int32_t centred, int32_t middle, int32_t wrap);
int aoe_ui_ez_create_text_id_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                                 AoeUiTextRor **out, int32_t text_id,
                                 int32_t x, int32_t y, int32_t w, int32_t h,
                                 int32_t font, int32_t centred,
                                 int32_t middle, int32_t wrap);
int aoe_ui_ez_create_lines_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                               AoeUiTextRor **out, const char *const *lines,
                               int16_t count, int32_t x, int32_t y, int32_t w,
                               int32_t h, int32_t font, int32_t centred,
                               int32_t middle);
int aoe_ui_ez_create_input_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                               AoeUiInputRor **out, const char *text,
                               int16_t max, int32_t format, int32_t x,
                               int32_t y, int32_t w, int32_t h, int32_t font);
int aoe_ui_ez_create_dropdown_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                                  AoeUiDropdownRor **out, int32_t list_w,
                                  int32_t list_h, int32_t x, int32_t y,
                                  int32_t w, int32_t h, int32_t font);
int aoe_ui_ez_create_list_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                              AoeUiListRor **out, int32_t x, int32_t y,
                              int32_t w, int32_t h, int32_t font);
int aoe_ui_ez_create_scrollbar_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                                   AoeUiScrollbarRor **out,
                                   AoeUiTextRor *target, int32_t range,
                                   int32_t x, int32_t y, int32_t width,
                                   int32_t h);
int aoe_ui_ez_create_text_scrollbar_ror(AoeUiEzRor *ez,
                                        AoeUiScrollbarRor **out,
                                        AoeUiTextRor *target, int32_t width);
int aoe_ui_ez_create_value_scrollbar_ror(AoeUiEzRor *ez,
                                         AoeUiPanelRor *parent,
                                         AoeUiValueScrollbarRor **out,
                                         int32_t horizontal, int32_t range,
                                         int32_t position, int32_t step,
                                         int32_t x, int32_t y, int32_t w,
                                         int32_t h);
/* +0x10c 0x00456a30: an edit panel (ui_edit.h) with the text, limit and
   format; the height is in pixels; selecting all on focus unless
   multi-line; bevel 3 and the text colours; the IME settings. */
int aoe_ui_ez_create_edit_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                              AoeUiEditRor **out, const char *text,
                              int16_t max, int32_t format, int32_t x,
                              int32_t y, int32_t w, int32_t h, int32_t font,
                              int32_t ime_enabled, int32_t ime_open);
void aoe_ui_ez_place_ror(AoeUiEzRor *ez, AoeUiPanelRor *child, int32_t x,
                         int32_t y, int32_t w, int32_t h);

/* Dialogs: 0x00457790 help popup (WinHelp in help mode 1; once), 0x004578d0
   / 0x00457920 OK, 0x00457a30 / 0x00457a80 yes/no, 0x00457b40 /
   0x00457b90 yes/no/cancel (the default names "OKDialog", "YesNoDialog",
   "YesNoCancelDialog"; an OK or help dialog of the same name is closed
   first); 0x00457740 the help popup from a string ID. Each creates a
   registered message dialog (ui_dialog.h) of w x h with the popup info
   and buttons 90 x 30 (120 x 25 for yes/no/cancel). */
void aoe_ui_ez_help_dialog_ror(AoeUiEzRor *ez, const char *text,
                               const char *name, int32_t w, int32_t h);
void aoe_ui_ez_help_dialog_id_ror(AoeUiEzRor *ez, int32_t text_id,
                                  const char *name, int32_t w, int32_t h);
void aoe_ui_ez_ok_dialog_ror(AoeUiEzRor *ez, const char *text,
                             const char *name, int32_t w, int32_t h);
void aoe_ui_ez_ok_dialog_id_ror(AoeUiEzRor *ez, int32_t text_id,
                                const char *name, int32_t w, int32_t h);
void aoe_ui_ez_yes_no_dialog_ror(AoeUiEzRor *ez, const char *text,
                                 const char *name, int32_t w, int32_t h);
void aoe_ui_ez_yes_no_dialog_id_ror(AoeUiEzRor *ez, int32_t text_id,
                                    const char *name, int32_t w, int32_t h);
void aoe_ui_ez_yes_no_cancel_dialog_ror(AoeUiEzRor *ez, const char *text,
                                        const char *name, int32_t w,
                                        int32_t h);
void aoe_ui_ez_yes_no_cancel_dialog_id_ror(AoeUiEzRor *ez, int32_t text_id,
                                           const char *name, int32_t w,
                                           int32_t h);
/* Help mode: 0x00457cd0 arms it (help pointer, mode 6, the pointer state
   saved and cleared), 0x00457d30 disarms it; 0x00457d80 the next press
   disarms it and a left press shows the panel's help (+0xd0) popup. */
void aoe_ui_ez_help_mode_on_ror(AoeUiEzRor *ez);
void aoe_ui_ez_help_mode_off_ror(AoeUiEzRor *ez);
int32_t aoe_ui_ez_help_press_ror(AoeUiEzRor *ez, uint8_t button, int32_t x,
                                 int32_t y);
/* The screen subclass (vtable 0x0054579c): 0x00468150 constructor,
   0x00468170 / 0x00468130 destructors, 0x00468180 setup (full area, no
   parent), +0x54 0x004681b0 (paint, then 0), +0xb0 0x00468340 (the help
   popup, string 120055 at 450, 100). Its fading focus and drawing are
   not translated. */
extern const AoeUiEzVtblRor aoe_ui_screen_vtable_ror;
int aoe_ui_screen_init_ror(AoeUiEzRor *ez, AoeUiSystemRor *system,
                           const AoeUiEzOpsRor *ops, const char *name);
int aoe_ui_screen_setup_ror(AoeUiEzRor *ez, AoeUiAreaRor *area,
                            const char *info_name, int32_t info_id,
                            int32_t field_470);

/* Slots: +0x28 0x00454f70 (the base begin_draw), +0x30 0x00454f80
   (restore marks the shadow rebuilt; draws the background, the parent's
   rect flag passed), +0x70 0x004553a0 (help mode first), +0xb8
   0x00457e50 (an OK / yes-no / yes-no-cancel dialog's buttons close it),
   +0xc4 0x00457670 (focus: the pointer, the palette, the IME). The
   shadow area is the `shadow_area` callback. */

#endif
