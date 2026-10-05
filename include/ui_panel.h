#ifndef VITA_AOE_UI_PANEL_H
#define VITA_AOE_UI_PANEL_H

#include <stddef.h>
#include <stdint.h>

/* Patched Rise of Rome 1.0a panel framework: the panel system (global,
   0x00452030..0x004523c0) and the base panel class (vtable 0x00544ad8,
   0x004523f0..0x00454400). Virtual calls go through AoeUiPanelVtblRor so
   subclasses override slots as in the original; Windows drawing, regions
   and capture go through AoeUiPlatformRor. Semantic storage; offsets are
   noted. */

typedef struct AoeUiPanelRor AoeUiPanelRor;
typedef struct AoeUiSystemRor AoeUiSystemRor;

typedef struct {
    int32_t left, top, right, bottom;
} AoeUiRectRor;

/* A drawing area (+0x20): the window (+4) and its size (+0x18/+0x1c). */
typedef struct {
    void *window;
    int32_t width, height;
} AoeUiAreaRor;

typedef struct {
    void *context;
    /* InvalidateRect(area window, rect, 0). */
    void (*invalidate)(void *context, AoeUiAreaRor *area,
                       const AoeUiRectRor *rect);
    /* The surface flip + ValidateRect. */
    void (*validate)(void *context, AoeUiAreaRor *area,
                     const AoeUiRectRor *rect);
    /* Fill (rect, color). */
    void (*fill)(void *context, const AoeUiRectRor *rect, uint8_t color);
    /* SelectClipRgn(area dc, region of `rect`, or none) and the surface
       clip. */
    void (*clip)(void *context, AoeUiAreaRor *area, const AoeUiRectRor *rect);
    void (*capture)(void *context, AoeUiAreaRor *area); /* SetCapture */
    void (*release)(void *context);                     /* ReleaseCapture */
    uint32_t (*now_ms)(void *context);                  /* timeGetTime */
    int (*key_down)(void *context, int virtual_key);    /* GetKeyState < 0 */
    int (*iconic)(void *context, AoeUiAreaRor *area);   /* IsIconic */
    /* Game vtable +0x20(id, buffer, size): the string table. */
    void (*string)(void *context, int32_t id, char *out, int size);
    /* Palette cache: load (name, id, 1) and DeleteObject. */
    uintptr_t (*create_palette)(void *context, const char *name, int32_t id);
    void (*delete_palette)(void *context, uintptr_t palette);
    /* GetTextExtentPoint32A width of `count` bytes in a font. */
    int32_t (*text_width)(void *context, uintptr_t font, const char *text,
                          int32_t count);
} AoeUiPlatformRor;

typedef struct AoeUiNodeRor {
    AoeUiPanelRor *panel;
    struct AoeUiNodeRor *prev, *next;
} AoeUiNodeRor;

#define AOE_UI_PALETTES 10

struct AoeUiSystemRor {
    AoeUiPanelRor *capture;  /* +0x00 (0x00451ac0) */
    AoeUiPanelRor *keyboard; /* +0x04 */
    AoeUiPanelRor *modal;    /* +0x08 (0x00451ad0) */
    AoeUiPanelRor **panels;  /* +0x10 list, in order */
    int32_t panel_count, panel_capacity;
    char palette_names[AOE_UI_PALETTES][0x104]; /* +0x44 */
    uintptr_t palettes[AOE_UI_PALETTES];        /* +0x1c */
    int32_t palette_refs[AOE_UI_PALETTES];      /* +0xa6c */
    int32_t palette_ids[AOE_UI_PALETTES];       /* +0xa94 */
    int32_t enabled;                      /* +0xac8 */
    int32_t app_active;                   /* game +0x18 */
    const AoeUiPlatformRor *platform;
};

/* Vtable slots by original offset. Mouse arguments are x, y, then the
   control (wParam & 8) and shift (wParam & 4) flags. */
typedef struct {
    void (*destroy)(AoeUiPanelRor *);                                 /* 0x00 */
    int (*setup)(AoeUiPanelRor *, AoeUiAreaRor *, AoeUiPanelRor *, int32_t,
                 int32_t, int32_t, int32_t, uint8_t);                 /* 0x04 */
    void (*set_rect_ltrb)(AoeUiPanelRor *, int32_t, int32_t, int32_t,
                          int32_t);                                   /* 0x08 */
    void (*set_rect)(AoeUiPanelRor *, int32_t, int32_t, int32_t,
                     int32_t);                                        /* 0x0c */
    void (*set_color)(AoeUiPanelRor *, uint8_t);                      /* 0x10 */
    void (*set_active)(AoeUiPanelRor *, int32_t);                     /* 0x14 */
    void (*set_position_mode)(AoeUiPanelRor *, int32_t, int32_t, int32_t,
                              int32_t, int32_t, int32_t, int32_t,
                              int32_t, int32_t, AoeUiPanelRor *,
                              AoeUiPanelRor *, AoeUiPanelRor *,
                              AoeUiPanelRor *);                       /* 0x18 */
    void (*set_fixed_position)(AoeUiPanelRor *, int32_t, int32_t, int32_t,
                               int32_t);                              /* 0x1c */
    void (*set_redraw)(AoeUiPanelRor *, int32_t);                     /* 0x20 */
    void (*redraw_overlaps)(AoeUiPanelRor *, AoeUiPanelRor *,
                            AoeUiPanelRor *, int32_t);                /* 0x24 */
    void (*begin_draw)(AoeUiPanelRor *, int32_t);                     /* 0x28 */
    void (*end_draw)(AoeUiPanelRor *);                                /* 0x2c */
    void (*draw)(AoeUiPanelRor *);                                    /* 0x30 */
    void (*draw_rect)(AoeUiPanelRor *, const AoeUiRectRor *);         /* 0x34 */
    void (*draw_offset)(AoeUiPanelRor *, int32_t, int32_t,
                        const AoeUiRectRor *);                        /* 0x38 */
    void (*draw_parent_rect)(AoeUiPanelRor *, const AoeUiRectRor *);  /* 0x3c */
    void (*draw_parent_offset)(AoeUiPanelRor *, int32_t, int32_t,
                               const AoeUiRectRor *);                 /* 0x40 */
    void (*validate)(AoeUiPanelRor *);                                /* 0x44 */
    int32_t (*handle_message)(AoeUiPanelRor *, void *, uint32_t, uint32_t,
                              uint32_t);                              /* 0x48 */
    int32_t (*handle_idle)(AoeUiPanelRor *);                          /* 0x4c */
    int32_t (*handle_size)(AoeUiPanelRor *, int32_t, int32_t);        /* 0x50 */
    int32_t (*handle_paint)(AoeUiPanelRor *);                         /* 0x54 */
    int32_t (*key_down)(AoeUiPanelRor *, int32_t, int32_t, int32_t,
                        int32_t, int32_t);                            /* 0x58 */
    int32_t (*char_input)(AoeUiPanelRor *, int32_t, int32_t);         /* 0x5c */
    int32_t (*command)(AoeUiPanelRor *, uint32_t, uint32_t);          /* 0x60 */
    int32_t (*user)(AoeUiPanelRor *, uint32_t, uint32_t);             /* 0x64 */
    int32_t (*timer)(AoeUiPanelRor *, uint32_t, uint32_t);            /* 0x68 */
    int32_t (*scroll)(AoeUiPanelRor *, uint32_t, uint32_t);           /* 0x6c */
    int32_t (*mouse_down)(AoeUiPanelRor *, uint8_t, int32_t, int32_t,
                          int32_t, int32_t);                          /* 0x70 */
    int32_t (*mouse_move)(AoeUiPanelRor *, int32_t, int32_t, int32_t,
                          int32_t);                                   /* 0x74 */
    int32_t (*mouse_up)(AoeUiPanelRor *, uint8_t, int32_t, int32_t,
                        int32_t, int32_t);                            /* 0x78 */
    int32_t (*mouse_dbl)(AoeUiPanelRor *, uint8_t, int32_t, int32_t,
                         int32_t, int32_t);                           /* 0x7c */
    /* Actions 0x80..0xa8 (x, y, ctrl, shift): hover, left down, hold,
       drag, up, double; right down, hold, drag, up, double. */
    int32_t (*actions[11])(AoeUiPanelRor *, int32_t, int32_t, int32_t,
                           int32_t);                                  /* 0x80 */
    int32_t (*key_action)(AoeUiPanelRor *, int32_t, int32_t, int32_t,
                          int32_t, int32_t);                          /* 0xac */
    int32_t (*slot_b0)(AoeUiPanelRor *);                              /* 0xb0 */
    int32_t (*char_action)(AoeUiPanelRor *, int32_t, int32_t);        /* 0xb4 */
    int32_t (*action)(AoeUiPanelRor *, AoeUiPanelRor *, int32_t, int32_t,
                      int32_t);                                       /* 0xb8 */
    void (*get_true_rect)(AoeUiPanelRor *, AoeUiRectRor *);           /* 0xbc */
    int32_t (*is_inside)(AoeUiPanelRor *, int32_t, int32_t);          /* 0xc0 */
    void (*set_focus)(AoeUiPanelRor *, int32_t);                      /* 0xc4 */
    void (*set_tab_order)(AoeUiPanelRor *, AoeUiPanelRor *,
                          AoeUiPanelRor *);                           /* 0xc8 */
    int (*get_help)(AoeUiPanelRor *, char *, int32_t *, int32_t,
                    int32_t);                                         /* 0xd0 */
    void (*stop_sound)(AoeUiPanelRor *);                              /* 0xd4 */
    int32_t (*restart_sound)(AoeUiPanelRor *);                        /* 0xd8 */
    void (*slot_e0)(AoeUiPanelRor *);                                 /* 0xe0 */
} AoeUiPanelVtblRor;

enum {
    AOE_UI_HOVER, AOE_UI_LEFT_DOWN, AOE_UI_LEFT_HOLD, AOE_UI_LEFT_DRAG,
    AOE_UI_LEFT_UP, AOE_UI_LEFT_DBL, AOE_UI_RIGHT_DOWN, AOE_UI_RIGHT_HOLD,
    AOE_UI_RIGHT_DRAG, AOE_UI_RIGHT_UP, AOE_UI_RIGHT_DBL
};

struct AoeUiPanelRor {
    const AoeUiPanelVtblRor *vt;
    AoeUiSystemRor *system;
    int32_t field_4, field_8;              /* +0x04, +0x08 */
    int32_t x, y, width, height;           /* +0x0c..+0x18 */
    char *name;                            /* +0x1c */
    AoeUiAreaRor *area;                    /* +0x20 */
    AoeUiRectRor clip;                     /* +0x24 */
    int32_t position_mode;                 /* +0x34 */
    int32_t redraw;                        /* +0x38: 0, 1 or 2 */
    AoeUiPanelRor *current;                /* +0x3c: the focused child */
    AoeUiPanelRor *parent;                 /* +0x40 */
    AoeUiPanelRor *anchor[4];              /* +0x44..+0x50 l, t, r, b */
    AoeUiNodeRor *node;                    /* +0x54 */
    AoeUiNodeRor *first, *last;            /* +0x58, +0x5c children */
    AoeUiPanelRor *tab_prev, *tab_next;    /* +0x60, +0x64 */
    int32_t captured;                      /* +0x68 */
    int32_t active;                        /* +0x6c */
    int32_t visible;                       /* +0x70: clip not empty */
    int32_t tab_stop;                      /* +0x74 */
    int32_t has_focus;                     /* +0x78 */
    int32_t overlap_redraw;                /* +0x7c */
    int32_t handles_mouse;                 /* +0x80 */
    int32_t painting;                      /* +0x84 */
    int32_t region;                        /* +0x88: a region exists */
    AoeUiRectRor rect;                     /* +0x8c */
    int32_t offset[4];                     /* +0x9c..+0xa8 l, t, r, b */
    int32_t min_w, max_w, min_h, max_h;    /* +0xac..+0xb8 */
    int32_t hold_ms;                       /* +0xbc, 250 */
    int32_t drag_slack;                    /* +0xc0 */
    int32_t down_x, down_y, down_ctrl, down_shift; /* +0xc4..+0xd0 */
    uint32_t down_ms;                      /* +0xd4 */
    int32_t field_d8;
    int32_t z;                             /* +0xdc */
    int32_t field_e0;
    int32_t help_id, help_page;            /* +0xe4, +0xe8 */
    uint8_t mouse_state;                   /* +0xec: 0, 1 down, 2 held, 3 drag */
    uint8_t mouse_button;                  /* +0xed */
    uint8_t field_ee, color;               /* +0xee, +0xef */
    uint8_t own_fill;                      /* +0xf0 */
    uint8_t clip_to_parent;                /* +0xf1 */
    uint8_t parent_rect;                   /* +0xf2 */
    uint8_t need_restore;                  /* +0xf3 */
};

extern const AoeUiPanelVtblRor aoe_ui_panel_vtable_ror;

/* ---- panel system ---- */
/* 0x00452030: a palette by id (when not -1) or name ("<name>.pal" without a
   dot) from the ten-slot cache, created into the first free slot; each
   use is counted. 0 when the cache is full or creation fails.
   0x004521f0: release one use; the last deletes it. */
uintptr_t aoe_ui_palette_ror(AoeUiSystemRor *system, const char *name,
                          int32_t id);
void aoe_ui_palette_release_ror(AoeUiSystemRor *system, uintptr_t palette);
/* 0x004522b0 / 0x004522c0 / 0x004522d0: the input-enabled flag. */
int32_t aoe_ui_enabled_ror(const AoeUiSystemRor *system);
void aoe_ui_enable_ror(AoeUiSystemRor *system);
void aoe_ui_disable_ror(AoeUiSystemRor *system);
/* 0x004522e0 every panel +0xd4; 0x00452310 every panel +0xd8 (0 if any
   fails); 0x004523c0 every panel needs restoring. 0x00451ac0 /
   0x00451ad0: the capture and modal panels. */
void aoe_ui_stop_sounds_ror(AoeUiSystemRor *system);
int32_t aoe_ui_restart_sounds_ror(AoeUiSystemRor *system);
void aoe_ui_mark_restore_ror(AoeUiSystemRor *system);
AoeUiPanelRor *aoe_ui_capture_panel_ror(const AoeUiSystemRor *system);
AoeUiPanelRor *aoe_ui_modal_panel_ror(const AoeUiSystemRor *system);

/* ---- base panel ---- */
/* Default constructor 0x00452580 and named 0x004523f0 (registers a named
   panel with the system); destructor 0x004526b0 (deleting 0x00452560). */
void aoe_ui_panel_init_ror(AoeUiPanelRor *panel, AoeUiSystemRor *system);
int aoe_ui_panel_init_named_ror(AoeUiPanelRor *panel, AoeUiSystemRor *system,
                                const char *name);
void aoe_ui_panel_release_ror(AoeUiPanelRor *panel);
/* 0x00454180 (NULL or "" clears it). */
int aoe_ui_panel_set_name_ror(AoeUiPanelRor *panel, const char *name);
/* Child list of `parent`: insert 0x00452820 (no reference: at the end;
   else before it, or after it with `after`), remove 0x004528c0. */
void aoe_ui_insert_child_ror(AoeUiPanelRor *parent, AoeUiNodeRor *node,
                             AoeUiNodeRor *ref, int after);
void aoe_ui_remove_child_ror(AoeUiPanelRor *parent, AoeUiNodeRor *node);

/* Base slot implementations (vtable 0x00544ad8): +0x04 setup 0x00452750
   (area, parent, x, y, w, h, color), +0x08 0x00452a40, +0x0c 0x00452920
   (clip to the parent when +0xf1, to 0 and the area), +0x10 0x00452a70,
   +0x14 0x00452a90, +0x18 0x00452ad0, +0x1c 0x00452b80, +0x20 0x00452bb0
   (InvalidateRect), +0x24 0x00452bf0, +0x28 0x00452ee0, +0x2c 0x00452f30,
   +0x30 0x00452d20, +0x34 0x00452d80, +0x38 0x00452de0, +0x3c 0x00452e80,
   +0x40 0x00452eb0, +0x44 0x00452f60, +0x48 0x00452fa0 (keys 0x100/0x102/
   0x104 and mouse 0x200..0x206 to the capture, keyboard or modal panel;
   other messages to children last first, then scroll 0x114/0x115,
   command 0x111, timer 0x113, user 0x400), +0x4c 0x00453360, +0x50
   0x004533f0 (layout modes: 0 fixed, 2 centred, 3/7/8 keep the left edge
   and 4/7/9 the top edge when capping), +0x54 0x00453640, +0x58
   0x00453700 (Tab moves to the tab neighbour), +0x5c 0x004537b0, +0x70
   0x00453810, +0x74 0x00453950 (left drags past the +0xc0 slack, right
   past 10 pixels), +0x78 0x00453af0, +0x7c 0x00453bf0, +0xb8 0x00453cc0,
   +0xbc 0x00454400, +0xc0 0x00453df0, +0xc4 0x00453f00, +0xc8 0x00454130,
   +0xd0 0x004542e0. The Windows-only IME and system-colour helpers
   of the panel system are not translated. */
void aoe_ui_panel_destroy_ror(AoeUiPanelRor *p);
int aoe_ui_panel_setup_ror(AoeUiPanelRor *p, AoeUiAreaRor *area,
                           AoeUiPanelRor *parent, int32_t x, int32_t y,
                           int32_t w, int32_t h, uint8_t color);
void aoe_ui_panel_set_rect_ltrb_ror(AoeUiPanelRor *p, int32_t l, int32_t t,
                                    int32_t r, int32_t b);
void aoe_ui_panel_set_rect_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                               int32_t w, int32_t h);
void aoe_ui_panel_set_color_ror(AoeUiPanelRor *p, uint8_t color);
void aoe_ui_panel_set_active_ror(AoeUiPanelRor *p, int32_t active);
void aoe_ui_panel_set_position_mode_ror(
    AoeUiPanelRor *p, int32_t mode, int32_t left, int32_t top, int32_t right,
    int32_t bottom, int32_t min_w, int32_t max_w, int32_t min_h, int32_t max_h,
    AoeUiPanelRor *anchor_l, AoeUiPanelRor *anchor_t, AoeUiPanelRor *anchor_r,
    AoeUiPanelRor *anchor_b);
void aoe_ui_panel_set_fixed_position_ror(AoeUiPanelRor *p, int32_t x,
                                         int32_t y, int32_t w, int32_t h);
void aoe_ui_panel_set_redraw_ror(AoeUiPanelRor *p, int32_t level);
void aoe_ui_panel_redraw_overlaps_ror(AoeUiPanelRor *p, AoeUiPanelRor *of,
                                      AoeUiPanelRor *skip, int32_t level);
void aoe_ui_panel_begin_draw_ror(AoeUiPanelRor *p, int32_t clear);
void aoe_ui_panel_end_draw_ror(AoeUiPanelRor *p);
void aoe_ui_panel_draw_ror(AoeUiPanelRor *p);
void aoe_ui_panel_draw_rect_ror(AoeUiPanelRor *p, const AoeUiRectRor *r);
void aoe_ui_panel_draw_offset_ror(AoeUiPanelRor *p, int32_t dx, int32_t dy,
                                  const AoeUiRectRor *r);
void aoe_ui_panel_draw_parent_rect_ror(AoeUiPanelRor *p,
                                       const AoeUiRectRor *r);
void aoe_ui_panel_draw_parent_offset_ror(AoeUiPanelRor *p, int32_t dx,
                                         int32_t dy, const AoeUiRectRor *r);
void aoe_ui_panel_validate_ror(AoeUiPanelRor *p);
int32_t aoe_ui_panel_handle_message_ror(AoeUiPanelRor *p, void *window,
                                        uint32_t message, uint32_t wparam,
                                        uint32_t lparam);
int32_t aoe_ui_panel_handle_idle_ror(AoeUiPanelRor *p);
int32_t aoe_ui_panel_handle_size_ror(AoeUiPanelRor *p, int32_t w, int32_t h);
int32_t aoe_ui_panel_handle_paint_ror(AoeUiPanelRor *p);
int32_t aoe_ui_panel_key_down_ror(AoeUiPanelRor *p, int32_t key,
                                  int32_t count, int32_t alt, int32_t ctrl,
                                  int32_t shift);
int32_t aoe_ui_panel_char_ror(AoeUiPanelRor *p, int32_t key, int32_t count);
int32_t aoe_ui_panel_mouse_down_ror(AoeUiPanelRor *p, uint8_t button,
                                    int32_t x, int32_t y, int32_t ctrl,
                                    int32_t shift);
int32_t aoe_ui_panel_mouse_move_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                    int32_t ctrl, int32_t shift);
int32_t aoe_ui_panel_mouse_up_ror(AoeUiPanelRor *p, uint8_t button,
                                  int32_t x, int32_t y, int32_t ctrl,
                                  int32_t shift);
int32_t aoe_ui_panel_mouse_dbl_ror(AoeUiPanelRor *p, uint8_t button,
                                   int32_t x, int32_t y, int32_t ctrl,
                                   int32_t shift);
int32_t aoe_ui_panel_action_ror(AoeUiPanelRor *p, AoeUiPanelRor *from,
                                int32_t a, int32_t b, int32_t c);
void aoe_ui_panel_get_true_rect_ror(AoeUiPanelRor *p, AoeUiRectRor *out);
int32_t aoe_ui_panel_is_inside_ror(AoeUiPanelRor *p, int32_t x, int32_t y);
void aoe_ui_panel_set_focus_ror(AoeUiPanelRor *p, int32_t focus);
void aoe_ui_panel_set_tab_order_ror(AoeUiPanelRor *p, AoeUiPanelRor *prev,
                                    AoeUiPanelRor *next);
int aoe_ui_panel_get_help_ror(AoeUiPanelRor *p, char *text, int32_t *page,
                              int32_t x, int32_t y);
void aoe_ui_panel_stop_sound_ror(AoeUiPanelRor *p);
int32_t aoe_ui_panel_restart_sound_ror(AoeUiPanelRor *p);
void aoe_ui_panel_slot_e0_ror(AoeUiPanelRor *p);
/* 0x00453800 (command, user, timer and scroll) and 0x004059e0 /
   0x004059d0 (the action defaults): 0. */
int32_t aoe_ui_panel_ignore_ror(AoeUiPanelRor *p, uint32_t a, uint32_t b);
int32_t aoe_ui_panel_no_action_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                   int32_t ctrl, int32_t shift);
/* A slot returning 0 (+0xb0 default). */
int32_t aoe_ui_panel_zero_ror(AoeUiPanelRor *p);
int32_t aoe_ui_panel_no_char_action_ror(AoeUiPanelRor *p, int32_t a,
                                        int32_t b);

/* Helpers. */
/* 0x00453cf0: mouse lParam/wParam decode (coordinates above 60000 wrap). */
void aoe_ui_decode_mouse_ror(uint32_t wparam, uint32_t lparam, int32_t xy[2],
                             int32_t *ctrl, int32_t *shift);
/* 0x00453d50 / 0x00453db0: mouse capture (refused while another panel
   holds it). */
int aoe_ui_panel_capture_ror(AoeUiPanelRor *p);
void aoe_ui_panel_release_capture_ror(AoeUiPanelRor *p);
/* 0x00453e30: point in rect (inclusive); 0x00453e60: pull a point into the
   clip rect inset by one (1 when moved). */
int aoe_ui_point_in_rect_ror(int32_t x, int32_t y, const AoeUiRectRor *r);
int aoe_ui_panel_clamp_point_ror(AoeUiPanelRor *p, int32_t *x, int32_t *y);
/* 0x00453eb0: the focused child changes (focus moves when the panel has
   it); 0x00453f30: has_focus. */
void aoe_ui_panel_set_current_ror(AoeUiPanelRor *p, AoeUiPanelRor *child);
int32_t aoe_ui_panel_has_focus_ror(const AoeUiPanelRor *p);
/* 0x00453f40 / 0x00453f90: z-order (mode 0 at z, shifting others up; 1
   front; 2 back), re-sorting the parent's child list. */
void aoe_ui_panel_set_z_ror(AoeUiPanelRor *p, uint8_t mode, int16_t z);
void aoe_ui_panel_set_child_z_ror(AoeUiPanelRor *parent, AoeUiPanelRor *child,
                                  uint8_t mode, int16_t z);
/* 0x004540e0: a cyclic tab order over `count` panels. */
void aoe_ui_panels_tab_cycle_ror(AoeUiPanelRor **panels, int16_t count);
/* 0x00454150 +4, 0x00454160 set +8, 0x00454170 name, 0x00454230 y,
   0x00454240 width, 0x00454250 height, 0x00454260 area; 0x004542c0 help
   ids. 0x00454270 / 0x004542a0: a string from the table (nonzero when not
   empty; the latter into a 0x200 static buffer). */
int32_t aoe_ui_panel_field4_ror(const AoeUiPanelRor *p);
void aoe_ui_panel_set_field8_ror(AoeUiPanelRor *p, int32_t v);
const char *aoe_ui_panel_name_ror(const AoeUiPanelRor *p);
int32_t aoe_ui_panel_y_ror(const AoeUiPanelRor *p);
int32_t aoe_ui_panel_width_ror(const AoeUiPanelRor *p);
int32_t aoe_ui_panel_height_ror(const AoeUiPanelRor *p);
AoeUiAreaRor *aoe_ui_panel_area_ror(const AoeUiPanelRor *p);
void aoe_ui_panel_set_help_ror(AoeUiPanelRor *p, int32_t id, int32_t page);
int aoe_ui_string_ror(const AoeUiSystemRor *system, int32_t id, char *out,
                      int size);
const char *aoe_ui_string_static_ror(const AoeUiSystemRor *system,
                                     int32_t id);
/* Children broadcasts 0x00454390 (+0xd4), 0x004543b0 (+0xd8, 0 if any
   fails), 0x004543e0 (+0xe0). */
void aoe_ui_children_stop_sound_ror(AoeUiPanelRor *p);
int32_t aoe_ui_children_restart_sound_ror(AoeUiPanelRor *p);
void aoe_ui_children_e0_ror(AoeUiPanelRor *p);

#endif
