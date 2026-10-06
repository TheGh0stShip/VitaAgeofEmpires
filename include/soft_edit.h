#ifndef VITA_AOE_SOFT_EDIT_H
#define VITA_AOE_SOFT_EDIT_H

#include <stdint.h>

#include "ui_edit.h"

/* A software stand-in for the Windows EDIT control behind the translated
   edit panel (ui_edit.h): the text with a caret and selection, the
   control's own key handling (characters, Backspace, Delete, Left,
   Right, Home, End; Shift extends the selection), undo of the last
   change, show / hide / move and focus. Integration support. The panel
   draws the text while unfocused; while focused the platform paints the
   control (aoe_soft_edit_paint_ror), as Windows painted the EDIT window. */

enum { AOE_SOFT_EDIT_TEXT = 1024, AOE_SOFT_EDIT_WINDOWS = 16 };

typedef struct {
    int used;
    AoeUiAreaRor *area;
    int32_t x, y, w, h;
    int multiline, shown;
    char text[AOE_SOFT_EDIT_TEXT], undo_text[AOE_SOFT_EDIT_TEXT];
    int32_t sel_start, sel_end;   /* caret at sel_end */
    uintptr_t font;
    AoeUiEditRor *owner;          /* the panel, found when it takes focus */
} AoeSoftEditWindowRor;

typedef struct {
    AoeSoftEditWindowRor windows[AOE_SOFT_EDIT_WINDOWS];
    void *focus;                  /* a window, or the main window */
    void *main_window;
    AoeUiSystemRor *system;
    int (*key_down)(void *context, int vk);
    void *key_context;
    const uint8_t *palette;       /* 256 RGBX entries for nearest_index */
} AoeSoftEditRor;

/* Ops over `e` (the context). */
void aoe_soft_edit_init_ror(AoeSoftEditRor *e, AoeUiEditOpsRor *ops, AoeUiSystemRor *system,
                            void *main_window, int (*key_down)(void *, int), void *key_context,
                            const uint8_t *palette);
/* The focused edit panel (NULL when the main window has the focus). */
AoeUiEditRor *aoe_soft_edit_focused_ror(AoeSoftEditRor *e);
/* The focused window. */
AoeSoftEditWindowRor *aoe_soft_edit_focus_window_ror(AoeSoftEditRor *e);

#endif
