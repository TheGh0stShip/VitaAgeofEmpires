#ifndef VITA_AOE_UI_EDIT_H
#define VITA_AOE_UI_EDIT_H

#include <stdint.h>

#include "ui_panel.h"

/* Patched Rise of Rome 1.0a edit panel (vtable 0x00545220,
   0x00462050..0x00462e60): a panel hosting a native single- or
   multi-line edit window ("edit"), keeping a copy of its text. The
   native window, its IME and the system calls are AoeUiEditOpsRor (a
   Win32 EDIT in the original; the Vita IME dialog on the target).
   Formats (+0x128) filter typed characters like the text input panel: 1
   number with '.', 2 integer, 3 unsigned, 4 file name, 5 file name
   without '.', 6 path, 7 multi-line text, 8 a percentage. Semantic
   storage; offsets are noted. */

typedef struct {
    void *context;
    /* The native window: create (multi-line for format 7), destroy,
       show, move, invalidate (and update), focus queries. */
    void *(*create)(void *context, AoeUiAreaRor *area, int32_t x, int32_t y,
                    int32_t w, int32_t h, int multiline);
    void (*destroy)(void *context, void *window);
    void (*show)(void *context, void *window, int show);
    void (*move)(void *context, void *window, int32_t x, int32_t y, int32_t w,
                 int32_t h, int repaint);
    void (*invalidate)(void *context, void *window, int update);
    void *(*focus)(void *context);
    void (*set_focus)(void *context, void *window);
    void *(*main_window)(void *context, AoeUiAreaRor *area);
    /* Text and selection (EM_GETSEL / EM_SETSEL), undo, font. */
    void (*set_text)(void *context, void *window, const char *text);
    void (*get_text)(void *context, void *window, char *out, int32_t size);
    void (*get_selection)(void *context, void *window, int32_t *start,
                          int32_t *end);
    void (*set_selection)(void *context, void *window, int32_t start,
                          int32_t end);
    void (*undo)(void *context, void *window);
    void (*set_font)(void *context, void *window, uintptr_t font);
    /* The window's own handling of a message (the subclassed procedure). */
    int32_t (*default_proc)(void *context, void *window, uint32_t message,
                            uint32_t wparam, uint32_t lparam);
    /* Forward a system key to the main window. */
    void (*post_main)(void *context, void *main, uint32_t message,
                      uint32_t wparam, uint32_t lparam);
    /* Fill the window's background with the panel brush. */
    void (*fill_background)(void *context, void *window, uintptr_t brush);
    /* The IME context: obtain (the window's own when enabled, else
       detached), open or close, and release / close it. */
    uintptr_t (*ime_context)(void *context, void *window, int enabled);
    int (*ime_set_open)(void *context, uintptr_t ime, int open);
    void (*ime_release)(void *context, void *window, uintptr_t ime,
                        int enabled);
    int (*key_down)(void *context, int virtual_key);  /* GetKeyState < 0 */
    int (*lead_byte)(void *context, uint8_t c);       /* IsDBCSLeadByte */
    void (*beep)(void *context);                      /* MessageBeep */
    int32_t scrollbar_width;                          /* SM_CXVSCROLL */
    /* The game's software-cursor flag (+0x1d0) and hiding the pointer
       while typing. */
    int (*software_cursor)(void *context);
    void (*hide_pointer)(void *context);
} AoeUiEditOpsRor;

typedef struct {
    AoeUiPanelRor panel;
    const AoeUiEditOpsRor *ops;
    void *window;                 /* +0xf4 */
    int16_t max;                  /* +0xf8: max + 1 characters */
    char *text;                   /* +0xfc */
    int32_t length;               /* +0x100 */
    int32_t hidden;               /* +0x104 */
    uintptr_t brush;              /* +0x108 */
    int32_t field_10c, field_110; /* +0x10c, +0x110 */
    uint32_t color, shadow;       /* +0x114 (0xffffff), +0x118 */
    uint32_t focus_color, focus_shadow; /* +0x11c (0xffffff), +0x120 */
    uint32_t background;          /* +0x124 */
    int32_t format;               /* +0x128 */
    uintptr_t font;               /* +0x12c */
    int32_t field_130;
    AoeUiRectRor inner;           /* +0x134..+0x140 */
    uint32_t text_flags;          /* +0x144: 0x2010 multi-line, else 0x2020 */
    int32_t select_on_focus;      /* +0x148 */
    int32_t selection;            /* +0x14c: selected characters */
    int32_t bevel_type;           /* +0x150, 1 */
    uint8_t bevel[6];             /* +0x154..+0x159 */
    int32_t inset;                /* +0x15c */
    uintptr_t ime;                /* +0x160 */
    int32_t ime_enabled;          /* +0x164 */
    int32_t ime_open;             /* +0x168 */
} AoeUiEditRor;

/* Globals shared by edit panels: 0x007bfab4 (a Tab passed on), 0x007bfab8
   (a refused lead byte swallows the next character), 0x007bfab0 (the
   last IME open result). */
typedef struct {
    int32_t passing_tab;
    int32_t skip_char;
    int32_t ime_status;
} AoeUiEditGlobalsRor;
extern AoeUiEditGlobalsRor aoe_ui_edit_globals_ror;

extern const AoeUiPanelVtblRor aoe_ui_edit_vtable_ror;

/* 0x00462050 constructor (hidden, mouse handled, bevel 1); 0x00462120
   destructor (deleting 0x00462100): hide, free the text, release the IME
   and destroy the window. */
void aoe_ui_edit_init_ror(AoeUiEditRor *e, AoeUiSystemRor *system,
                          const AoeUiEditOpsRor *ops);
void aoe_ui_edit_release_ror(AoeUiEditRor *e);
/* 0x004621e0 setup: the panel (colour 0xff), the window inside it by one
   pixel, a buffer of max + 2 (max is one more than given when positive),
   the text, colours, font and text flags. 0 when the window fails. */
int aoe_ui_edit_setup_ror(AoeUiEditRor *e, AoeUiAreaRor *area,
                          AoeUiPanelRor *parent, int32_t x, int32_t y,
                          int32_t w, int32_t h, uintptr_t font, int16_t max,
                          const char *text, int32_t format,
                          int32_t select_on_focus);
/* 0x004623e0 set the text (cut to max - 1); 0x004624a0 read it back from
   the window (an over-long paste is cut and beeps) and the selection;
   0x00462560 the text after reading it back. */
void aoe_ui_edit_set_text_ror(AoeUiEditRor *e, const char *text);
void aoe_ui_edit_sync_ror(AoeUiEditRor *e);
const char *aoe_ui_edit_text_ror(AoeUiEditRor *e);
/* 0x00462860 colours; 0x00462880 bevel (inset 1 for types 1, 2, 5; 2
   for 3, 6; 3 for 4, 7); 0x00462950 IME enabled / open. */
void aoe_ui_edit_set_colors_ror(AoeUiEditRor *e, uint32_t color,
                                uint32_t shadow);
void aoe_ui_edit_set_bevel_ror(AoeUiEditRor *e, int32_t type,
                               const uint8_t colors[6]);
void aoe_ui_edit_set_ime_ror(AoeUiEditRor *e, int32_t enabled, int32_t open);
/* 0x00462e60: may character c be typed? */
int aoe_ui_edit_accept_char_ror(AoeUiEditRor *e, uint32_t c);
/* 0x00462a70 the window's message procedure (reached through
   0x004623a0): Tab, Enter (parent action 0) and Escape (action 1, else
   undo) keys, the character filter, system keys to the main window, the
   pointer hidden while typing, a repaint when the selection moves,
   mouse buttons passed to the screen in screen coordinates, and the
   text re-read after a paste. */
int32_t aoe_ui_edit_window_proc_ror(AoeUiEditRor *e, uint32_t message,
                                    uint32_t wparam, uint32_t lparam);
/* 0x004623a0: the window class procedure: 0 for a window without its
   panel, else the panel's procedure. */
int32_t aoe_ui_edit_class_proc_ror(AoeUiEditRor *owner, uint32_t message,
                                   uint32_t wparam, uint32_t lparam);
/* Slots: +0x0c 0x00462710 (the window and the inner text rectangle
   follow), +0x14 0x00462570 (inactive hides the window), +0x20
   0x00462820 (the window repaints too), +0x48 0x00462970 (focus,
   edit-change and colour messages), +0xc4 0x004625b0 (keyboard focus and
   IME, selecting all on entry when asked, re-reading on exit, hiding
   unless a Tab is passing). */

#endif
