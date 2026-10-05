#ifndef VITA_AOE_UI_DIALOG_H
#define VITA_AOE_UI_DIALOG_H

#include <stdint.h>

#include "ui_ez.h"

/* Patched Rise of Rome 1.0a dialog panel (vtable 0x00544f1c,
   0x00460730..0x004609f0): an easy panel centred over its parent (or the
   drawing area) that becomes modal and takes its parent's focus, giving
   it back when destroyed. Semantic storage; offsets are noted. */

typedef struct {
    AoeUiEzRor ez;
    int32_t field_478;              /* +0x478 */
    AoeUiPanelRor *saved_focus;     /* +0x47c: the parent's focus before */
    int32_t field_480, field_484, field_488; /* +0x480..+0x488, 11 each */
    int32_t field_48c;              /* +0x48c */
} AoeUiDialogRor;

extern const AoeUiEzVtblRor aoe_ui_dialog_vtable_ror;

/* 0x00460730 constructor (a named easy panel); 0x00460970 destructor
   (deleting 0x00460780): the modal stack goes back a step (or to none),
   and the parent redraws and gets its focus back. */
int aoe_ui_dialog_init_ror(AoeUiDialogRor *d, AoeUiSystemRor *system,
                           const AoeUiEzOpsRor *ops, const char *name);
void aoe_ui_dialog_release_ror(AoeUiDialogRor *d);
/* 0x004607a0: setup from an info file, w x h centred on the parent (or
   area); the parent's focus moves to the dialog, which comes to the front
   and becomes modal. 0x00460890: the same without an info file and with
   a fill colour, made modal before taking the focus. +0xc4 0x004609f0 is
   the easy panel's focus slot. */
int aoe_ui_dialog_setup_ror(AoeUiDialogRor *d, AoeUiAreaRor *area,
                            AoeUiPanelRor *parent, int32_t w, int32_t h,
                            const char *info_name, int32_t info_id,
                            int32_t field_470);
int aoe_ui_dialog_setup_color_ror(AoeUiDialogRor *d, AoeUiAreaRor *area,
                                  AoeUiPanelRor *parent, int32_t w,
                                  int32_t h, uint8_t color,
                                  int32_t field_470);
/* For subclasses: the dialog destructor's own part. */
void aoe_ui_dialog_destroy_slot_ror(AoeUiPanelRor *p);

/* The message dialog (vtable 0x00543cec, 0x00434550..0x00434990): a
   wrapped, centred text over one to three buttons. Types: 0 OK, 1
   OK/Cancel, 2 Yes/No, 3 two-button (strings 4005 / 4007), 4
   Yes/No/Cancel, 5 help (OK opens WinHelp). The last button answers
   Escape. Pressing button n tells the parent action n. */
typedef struct {
    AoeUiDialogRor dialog;
    uint8_t type;                   /* +0x490 */
    AoeUiTextRor *text;             /* +0x494 */
    AoeUiButtonRor *buttons[3];     /* +0x498..+0x4a0 */
} AoeUiMessageDialogRor;

extern const AoeUiEzVtblRor aoe_ui_message_dialog_vtable_ror;

/* 0x00434550 constructor; 0x004345b0 destructor (deleting 0x00434590)
   destroys the text and buttons. 0x00434630 setup: the dialog (w x h, the
   info file, field 1), the text (3, 3 to w - 3 by h - button_h - 6, font
   11, centred, wrapped), the buttons spread evenly along y = h -
   button_h - 7 with their labels, a tab cycle, the first focused. 0 for
   an unknown type. */
int aoe_ui_message_dialog_init_ror(AoeUiMessageDialogRor *m,
                                   AoeUiSystemRor *system,
                                   const AoeUiEzOpsRor *ops,
                                   const char *name);
int aoe_ui_message_dialog_setup_ror(AoeUiMessageDialogRor *m,
                                    AoeUiPanelRor *parent,
                                    const char *info_name, int32_t info_id,
                                    int32_t w, int32_t h, uint8_t type,
                                    const char *text, int32_t button_w,
                                    int32_t button_h);
/* Slots: +0x4c 0x00487220 (the game's idle hook unless paused, then the
   base idle), +0xac 0x00434990 (no key action), +0xb8 0x004348d0 (a
   button's click answers the parent). */

/* The list dialog (vtable 0x00543a7c, 0x00433890..0x00433c20): an
   optional title, a list with its scrollbar and OK / Cancel (Escape).
   OK or a double-click (action 3) on the list keeps the selection and
   its text and answers the parent 1; Cancel answers 2. */
typedef struct {
    AoeUiDialogRor dialog;
    AoeUiTextRor *title;            /* +0x490 */
    AoeUiListRor *list;             /* +0x494 */
    AoeUiScrollbarRor *scrollbar;   /* +0x498 */
    AoeUiButtonRor *ok, *cancel;    /* +0x49c, +0x4a0 */
    int16_t selected;               /* +0x4a4, -1 */
    const char *selected_text;      /* +0x4a8 */
} AoeUiListDialogRor;

extern const AoeUiEzVtblRor aoe_ui_list_dialog_vtable_ror;

/* 0x00433890 constructor; 0x00433900 destructor (deleting 0x004338e0).
   0x004339a0 setup: the dialog, the title (font 4, centred, wrapped) when
   given, the list (font 11) and its 20-wide scrollbar, the lines, the
   first selection, OK (string 4001) and Cancel (4002) of the same size on
   one row; the list has the focus. 0x00433c10 / 0x00433c20 the chosen
   line and text. */
int aoe_ui_list_dialog_init_ror(AoeUiListDialogRor *d, AoeUiSystemRor *system,
                                const AoeUiEzOpsRor *ops, const char *name);
int aoe_ui_list_dialog_setup_ror(
    AoeUiListDialogRor *d, AoeUiPanelRor *parent, const char *info_name,
    int32_t info_id, int32_t w, int32_t h, int32_t title_x, int32_t title_y,
    int32_t title_w, int32_t title_h, const char *title, int32_t list_x,
    int32_t list_y, int32_t list_w, int32_t list_h, const char *const *lines,
    int16_t count, int16_t selected, int32_t ok_x, int32_t cancel_x,
    int32_t button_y, int32_t button_w, int32_t button_h);
int16_t aoe_ui_list_dialog_selected_ror(const AoeUiListDialogRor *d);
const char *aoe_ui_list_dialog_text_ror(const AoeUiListDialogRor *d);

/* The help dialog "Help Dialog" (vtable 0x00543948,
   0x00433310..0x00433830): help text in a 340-wide box placed beside the
   point (x, y) inside the parent, an OK button (Escape) and, with a help
   file and a help page string, a "more help" button (string 9277). */
typedef struct {
    AoeUiDialogRor dialog;
    int32_t page;                   /* +0x490: the help page string ID */
    char owner[0x104];              /* +0x494: the screen to go back to */
    void *field_594;                /* +0x594 */
    AoeUiTextRor *text;             /* +0x598 */
    AoeUiButtonRor *ok, *more;      /* +0x59c, +0x5a0 */
} AoeUiHelpDialogRor;

extern const AoeUiEzVtblRor aoe_ui_help_dialog_vtable_ror;

/* 0x00433310 constructor with its setup 0x00433410: the dialog over
   `parent` (340 x 400, its popup info), the text (font 11, wrapped), then
   the height from the text's lines: (font height + 1) * (bottom + 2) +
   55; x right of the point when it fits (|px - 5 - x|), else left
   (|x - px - 345|), else centred less 170; y above the point when it fits
   (|y - h - py - 10|), else below (|y - py + 10|), else centred; both at
   least 0. The buttons sit at font height * lines + 20, OK at 110 (195
   with "more help" at 25), 120 x 25, font 11. 0x00433380 destructor
   (deleting 0x004332f0). */
int aoe_ui_help_dialog_init_ror(AoeUiHelpDialogRor *d, AoeUiSystemRor *system,
                                const AoeUiEzOpsRor *ops,
                                AoeUiPanelRor *parent, const char *owner,
                                const char *text, int32_t page, int32_t x,
                                int32_t y);
/* Slots: +0xb8 0x00433770 (OK goes back to the owner screen and closes
   the dialog; "more help" also opens WinHelp at the page's keyword),
   +0x70 0x00433830 (a press outside the dialog is OK). */

#endif
