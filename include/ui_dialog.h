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

#endif
