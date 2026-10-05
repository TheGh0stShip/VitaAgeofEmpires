#ifndef VITA_AOE_UI_SCROLLBAR_H
#define VITA_AOE_UI_SCROLLBAR_H

#include <stdint.h>

#include "ui_panel.h"

/* Patched Rise of Rome 1.0a scrollbar panel (pnl_scbr, vtable
   0x005456b8, 0x00466e30..0x00467af0). Parts: 1 back arrow, 2 forward
   arrow, 3 thumb, 4 page back, 5 page forward. Scroll requests go to the
   target panel's +0xb8 (codes 0 line back, 1 line forward, 2 page back,
   3 page forward, 4 to a position); without a target the bar moves
   itself and tells its parent (code 4). Drawing is not translated. */

typedef struct {
    AoeUiPanelRor panel;
    AoeUiPanelRor *target;        /* +0xf4 */
    int32_t range;                /* +0xf8 */
    int32_t line_step, page_step; /* +0xfc, +0x100 */
    int32_t position;             /* +0x104 */
    void *pictures[5];            /* +0x108..+0x118 */
    int32_t field_11c;            /* +0x11c */
    int32_t thumb_size;           /* +0x120 */
    int32_t thumb_y, thumb_min_y, thumb_max_y, grab_y; /* +0x124..+0x130 */
    int32_t thumb_x, thumb_min_x, thumb_max_x, grab_x; /* +0x134..+0x140 */
    uint8_t part;                 /* +0x144 */
    int32_t repeating;            /* +0x148 */
    int32_t mouse_x, mouse_y;     /* +0x14c, +0x150 */
    uint8_t repeat_state;         /* +0x154: 1 waiting, 2 repeating */
    uint32_t repeat_ms;           /* +0x158 */
    uint32_t repeat_interval;     /* +0x15c, 100 */
    uint32_t repeat_delay;        /* +0x160, 250 */
    AoeUiRectRor back, forward, thumb, page_back, page_forward; /* +0x164.. */
    int32_t horizontal;           /* +0x1b4 */
    int32_t bevel_type;           /* +0x1b8, 1 */
    uint8_t bevel[6];             /* +0x1bc..+0x1c1 */
} AoeUiScrollbarRor;

extern const AoeUiPanelVtblRor aoe_ui_scrollbar_vtable_ror;

/* 0x00466e30 constructor, 0x00466fa0 destructor (deleting 0x00466f80);
   0x00466fb0 setup (the four picture names are ignored here). */
void aoe_ui_scrollbar_init_ror(AoeUiScrollbarRor *s, AoeUiSystemRor *system);
void aoe_ui_scrollbar_release_ror(AoeUiScrollbarRor *s);
int aoe_ui_scrollbar_setup_ror(AoeUiScrollbarRor *s, AoeUiAreaRor *area,
                               AoeUiPanelRor *parent, int32_t x, int32_t y,
                               int32_t w, int32_t h, int32_t field_11c,
                               int32_t thumb_size, AoeUiPanelRor *target,
                               int32_t range, int32_t horizontal);
/* +0x0c 0x00467010: the arrow, thumb-track and page rectangles;
   0x00467180: the thumb and page rectangles from the thumb position. */
void aoe_ui_scrollbar_place_parts_ror(AoeUiScrollbarRor *s);
/* 0x00467220 position (the thumb follows; the last position pins the
   vertical thumb to the end), 0x00467310 range and position, 0x00467330
   pictures, 0x00467370 bevel. */
void aoe_ui_scrollbar_set_position_ror(AoeUiScrollbarRor *s, int32_t position);
void aoe_ui_scrollbar_set_range_ror(AoeUiScrollbarRor *s, int32_t range,
                                    int32_t position);
void aoe_ui_scrollbar_set_pictures_ror(AoeUiScrollbarRor *s,
                                       void *const pictures[5]);
void aoe_ui_scrollbar_set_bevel_ror(AoeUiScrollbarRor *s, int32_t type,
                                    const uint8_t colors[6]);
/* 0x00467af0: a scroll request (to the target, or clamp and move). */
void aoe_ui_scrollbar_scroll_ror(AoeUiScrollbarRor *s, int32_t code,
                                 int32_t position);
/* Slots: +0x4c 0x004673c0 (auto-repeat: the delay, then every interval
   while the pointer stays on a page area), +0x84 0x00467520 (press a
   part), +0x8c 0x004676a0 (drag: arrows and pages re-arm, the thumb maps
   to a position), +0x90 0x00467aa0 (release: tell the parent, code 5). */

#endif
