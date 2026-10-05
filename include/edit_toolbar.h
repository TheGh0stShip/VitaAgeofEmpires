#ifndef VITA_AOE_EDIT_TOOLBAR_H
#define VITA_AOE_EDIT_TOOLBAR_H

#include <stdint.h>

#include "ui_button.h"

/* Patched Rise of Rome 1.0a game-screen edit toolbar (vtable 0x00545d50,
   0x00473d70..0x00474a40): the in-game editing buttons "Game" ("<Game>"
   pressed), "Obj", "Move", "Del", "Terr", "Elev" (a radio group, 50 x 30
   at x 0, 52, 104, 156, 208, 260; Terr and Elev start inactive) and the
   two-state "Cmd" at 314. They switch the game's edit mode (game vtable
   +0x10): 0 play, 1 place an object (picked in the "Object List
   Dialog", string 9002), 2 paint terrain (the "Terrain List Dialog"),
   3 elevation (PageUp / PageDown raise and lower the view's tiles), 0xe
   delete, 0x12 move (Delete removes the selected object). Cmd pauses
   (game state 6) or resumes (4) the game. */

typedef struct {
    void *context;
    /* Game vtable +0x10 (edit mode, value) and +0x0c (state); its +0x1b0
       state and +0x1b4 edit mode; the comm value (0x004199c0, nonzero
       while waiting on the network). */
    void (*set_edit_mode)(void *context, int32_t mode, int32_t value);
    void (*set_state)(void *context, int32_t state);
    int32_t (*state)(void *context);
    int32_t (*edit_mode)(void *context);
    int32_t (*comm_value)(void *context);
    /* World +0x1c = 1 when pausing. */
    void (*mark_world)(void *context);
    /* The viewed player's selection cleared (0x0045dcb0); the main view's
       tile highlight cleared (0x0050ffd0(-1, -1, -1, -1)). */
    void (*clear_selection)(void *context);
    void (*clear_highlight)(void *context);
    /* The object list dialog opened over `parent` (0x0046a890 / 0x0046a8d0
       with string 0x232a), NULL when it fails; a list dialog's name and
       chosen entry (its vtable +0x134); closing it (0x00419950). */
    AoeUiPanelRor *(*open_object_list)(void *context, AoeUiPanelRor *parent);
    const char *(*dialog_name)(void *context, AoeUiPanelRor *dialog);
    int16_t (*dialog_choice)(void *context, AoeUiPanelRor *dialog);
    void (*close_dialog)(void *context, AoeUiPanelRor *dialog);
    /* Game +0x3fc (object to place) and +0x3fe (terrain to paint). */
    void (*set_place_object)(void *context, int16_t id);
    void (*set_paint_terrain)(void *context, int16_t id);
    /* The main view's tile rectangle (0x00510070) raised or lowered
       (0x00447e40(.., 0, delta, 1) then 0x00445570(.., 8)). */
    int (*view_tiles)(void *context, int32_t *x0, int32_t *y0, int32_t *x1,
                      int32_t *y1);
    void (*adjust_elevation)(void *context, int32_t x0, int32_t y0, int32_t x1,
                             int32_t y1, int32_t delta);
    /* The viewed player's selected object (+0x134) removed through its
       vtable +0x68, returning the code the original computes first (a
       type-dependent check; 0x14 when the object has neither link), or -1
       with no selection. */
    int32_t (*remove_selected)(void *context);
} AoeEditToolbarOpsRor;

typedef struct {
    AoeUiPanelRor panel;
    AoeUiPanelRor *screen;       /* +0xf4: the dialogs' parent */
    AoeUiPanelRor *main_view;    /* +0xf8 */
    AoeUiPanelRor *map;          /* +0xfc */
    AoeUiButtonRor *game, *obj, *del, *move, *terr, *elev, *cmd; /* +0x100.. */
    AoeUiPanelRor *dialog;       /* +0x11c */
    const AoeEditToolbarOpsRor *ops;
} AoeEditToolbarRor;

const AoeUiPanelVtblRor *aoe_edit_toolbar_vtable_ror(void);

/* 0x00473d70: setup (area, parent, x, y, w, h), the buttons (font, its
   width and height, the sound), then the buttons for the edit mode;
   0 when anything fails (or there is no main view). 0x00474330
   destructor (deleting 0x00474310). */
int aoe_edit_toolbar_init_ror(AoeEditToolbarRor *t, AoeUiSystemRor *system,
                              const AoeEditToolbarOpsRor *ops,
                              const AoeUiButtonOpsRor *button_ops,
                              AoeUiAreaRor *area, AoeUiPanelRor *parent,
                              int32_t x, int32_t y, int32_t w, int32_t h,
                              uintptr_t font, int32_t font_w, int32_t font_h,
                              void *sound, AoeUiPanelRor *screen,
                              AoeUiPanelRor *main_view, AoeUiPanelRor *map);
/* 0x004748f0: leaving modes 1 and 0x12 clears the selection, 2 and 3
   the highlight; the new mode's button goes down (2 and 3 also clear the
   selection). */
void aoe_edit_toolbar_sync_ror(AoeEditToolbarRor *t, int32_t old_mode,
                               int32_t new_mode);
/* Slots: +0x30 0x00474a00 (fills black), +0xac 0x00474700 keys, +0xb8
   0x004743f0 actions (1 when handled; 0 only without a sender). */
int32_t aoe_edit_toolbar_key_ror(AoeUiPanelRor *p, int32_t key, int32_t a,
                                 int32_t b, int32_t c, int32_t d);
int32_t aoe_edit_toolbar_action_ror(AoeUiPanelRor *p, AoeUiPanelRor *from,
                                    int32_t code, int32_t value, int32_t extra);

#endif
