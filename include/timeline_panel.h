#ifndef VITA_AOE_TIMELINE_PANEL_H
#define VITA_AOE_TIMELINE_PANEL_H

#include <stdint.h>

#include "player_history.h"

/* Patched Rise of Rome 1.0a achievements timeline (tpnl_tml, vtable
   0x0054a134, 0x004fbaa0..0x004fdbe0, 0x52c bytes): the stacked graph of
   every player's share of the population over the game, one column
   group per history sample, with tick labels along the axes and the
   players' history events as marks, icons and captions. Semantic
   storage with the original offsets noted; the draw area, text, shapes,
   palette, strings and the world's players are ops. */

enum { AOE_TIMELINE_TEXT = 52 };

/* A mark on the graph (0xb0 bytes). Kind 3 is an axis tick; the others
   are the history event kinds (0..2 a captioned line, 4..8 an icon,
   frame kind - 4, with a caption for 6 and 8). */
typedef struct AoeTimelineMarkRor {
    int32_t sample;                    /* +0x00 */
    int32_t x, y, length;              /* +0x04, +0x08, +0x0c */
    uint8_t player, color, kind;       /* +0x10, +0x11, +0x12 */
    int32_t value;                     /* +0x14: a tick's total */
    int32_t field_18, field_1c;        /* +0x18, +0x1c */
    int32_t icon_x, icon_y;            /* +0x20, +0x24 */
    int32_t text_x, text_y;            /* +0x28, +0x2c */
    int32_t text_len;                  /* +0x38 */
    char text[AOE_TIMELINE_TEXT];      /* +0x3c */
    int32_t text2_len;                 /* +0x70 */
    char text2[AOE_TIMELINE_TEXT];     /* +0x74 */
    int32_t show;                      /* +0xa8 */
    struct AoeTimelineMarkRor *next;   /* +0xac */
} AoeTimelineMarkRor;

/* A font record (0x004199e0): the handle and its character size. */
typedef struct {
    uintptr_t handle;
    int32_t char_w, char_h;
} AoeTimelineFontRor;

typedef struct {
    void *context;
    /* The base panel: 0x00452750 setup (0 on failure); the base draw
       0x00452d20 with the area / visible test and the clip (vtable
       +0x28(0)), and the clip's end (+0x2c). */
    int (*panel_setup)(void *context, int32_t x, int32_t y, int32_t w, int32_t h);
    int (*draw_begin)(void *context);
    void (*draw_end)(void *context);
    /* The parent's background over our rectangle: +0x3c (framed) or,
       unless the parent's +0x84 is set, +0x34. */
    int (*has_parent)(void *context);
    int (*parent_field_84)(void *context);
    void (*parent_background)(void *context, int framed);
    /* The area: 0x00438730 / 0x004387c0 pixel lock, 0x00438800 /
       0x00438850 device context, each with the caller's name. */
    int (*lock)(void *context, const char *who);
    void (*unlock)(void *context, const char *who);
    int (*dc_lock)(void *context, const char *who);
    void (*dc_unlock)(void *context, const char *who);
    /* 0x00439c90 filled box, 0x00439ad0 outline, 0x00439d90 / 0x00439e40 /
       0x00439f60 one to three rings (colours in call order), 0x00439c00
       `len` pixels down from (x, y). */
    void (*box)(void *context, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color);
    void (*outline)(void *context, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color);
    void (*rings)(void *context, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int count,
                  const uint8_t *colors);
    void (*vline)(void *context, int32_t x, int32_t y, int32_t len, uint8_t color);
    uint8_t (*palette_index)(void *context, uint32_t rgb); /* GetNearestPaletteIndex */
    /* TextOutA with font A (0) or B (1) selected, transparent. */
    void (*text)(void *context, int font, int32_t x, int32_t y, const char *text, int32_t count,
                 uint32_t rgb);
    /* 0x00454270 a string (0 when missing); the game's +0x28 format. */
    int (*string)(void *context, int32_t id, char *out, int32_t size);
    void (*format)(void *context, int32_t kind, int32_t a, int32_t b, char *out, int32_t size);
    /* Shapes: 0x0049f5f0 (name, -1), 0x0049f840, 0x0049fa90 frames,
       0x0049fa00 a frame's size, 0x0049fac0 frame 0's box, 0x0049fe00. */
    void *(*shape_load)(void *context, const char *name);
    void (*shape_free)(void *context, void *shape);
    int32_t (*shape_frames)(void *context, void *shape);
    void (*shape_size)(void *context, void *shape, int32_t frame, int16_t *w, int16_t *h);
    void (*shape_box)(void *context, void *shape, int16_t *left, int16_t *top, int16_t *right,
                      int16_t *bottom);
    void (*shape_draw)(void *context, void *shape, int32_t x, int32_t y, int32_t frame);
    /* 0x00419950: close one of the three +0x520 font slots. */
    void (*font_close)(void *context, uintptr_t *slot);
    /* The world (game +0x3f4): its player count (+0x3c) and, for player
       1.., the history (+0x840), colour (+0x10c +0x26), civilisation
       (+0x109) and name (+0x44). */
    int16_t (*player_count)(void *context);
    const AoePlayerHistoryRor *(*history)(void *context, int32_t player);
    int16_t (*player_color)(void *context, int32_t player);
    uint8_t (*player_civ)(void *context, int32_t player);
    const char *(*player_name)(void *context, int32_t player);
} AoeTimelineOpsRor;

typedef struct {
    int32_t x, y, width, height;        /* panel +0x0c..+0x18 */
    AoeTimelineFontRor font_a, font_b;  /* +0x478, +0x484 */
    int32_t field_490;                  /* 2 */
    uint32_t color_494, color_498, color_49c; /* 0xffffff, 0, 0xffff */
    void *background;                   /* +0x4ac */
    int16_t background_w, background_h; /* +0x4b0, +0x4b2 */
    int32_t fill;                       /* +0x4b4 */
    uint8_t fill_color;                 /* +0x4b8 (0xff) */
    int32_t outline;                    /* +0x4bc */
    uint8_t outline_color;              /* +0x4c0 */
    int32_t border;                     /* +0x4c8: 0 none, 1 outline, 2..7 rings */
    uint8_t border_colors[6];           /* +0x4cc..+0x4d1 */
    void *icons;                        /* +0x4d4 */
    int16_t icon_w, icon_h;             /* +0x4d8, +0x4da */
    int32_t left, top;                  /* +0x4dc, +0x4e0 */
    int32_t cur_x, cur_y;               /* +0x4e4, +0x4e8 */
    int32_t graph_w, graph_h;           /* +0x4ec, +0x4f0 */
    int32_t axis_w, axis_h;             /* +0x4f4, +0x4f8 */
    float scale;                        /* +0x4fc: columns per sample */
    int32_t tick_every;                 /* +0x500 (12) */
    float skip_ratio;                   /* +0x504 */
    int32_t skip_total;                 /* +0x508 */
    uint8_t compress;                   /* +0x50c */
    int32_t columns;                    /* +0x510 */
    int32_t samples;                    /* +0x514 */
    AoeTimelineMarkRor *marks;          /* +0x518 */
    uint8_t enabled;                    /* +0x51c (1) */
    uintptr_t fonts[3];                 /* +0x520 */
    int32_t redraw;                     /* panel +0x38 */
    const AoeTimelineOpsRor *ops;
} AoeTimelineRor;

/* 0x004fbaa0 constructor (after the base panel's 0x00454b10); 0x004fbbe0
   destructor (deleting 0x004fbbc0): the marks, both shapes and the three
   font slots. */
void aoe_timeline_init_ror(AoeTimelineRor *t, const AoeTimelineOpsRor *ops);
void aoe_timeline_release_ror(AoeTimelineRor *t);
/* 0x004fbe30: the base setup, the fill (+0x4b4 / +0x4b8) and outline
   (+0x4bc / +0x4c0; an outline makes the border style 1), the two
   fonts, the background and icon shapes, then a black fill (flag 1). */
int aoe_timeline_setup_ror(AoeTimelineRor *t, int32_t x, int32_t y, int32_t w, int32_t h,
                           const AoeTimelineFontRor *font_a, const AoeTimelineFontRor *font_b,
                           const char *background, const char *icons, int32_t fill,
                           uint8_t fill_color, int32_t outline, uint8_t outline_color);
/* 0x004fbcb0 (the parent's factory): a new panel at the rectangle
   scaled from a base_w x base_h layout to the parent's size, with the
   two fonts (records fetched by the caller; negative indexes mean 11),
   set up plain and laid out. NULL on failure (the parent's +0xd8 is
   then set by the caller). */
AoeTimelineRor *aoe_timeline_new_ror(const AoeTimelineOpsRor *ops, int32_t parent_w,
                                     int32_t parent_h, int32_t x, int32_t y, int32_t w, int32_t h,
                                     const AoeTimelineFontRor *font_a,
                                     const AoeTimelineFontRor *font_b, int32_t base_w,
                                     int32_t base_h);
/* 0x004fbf00 the border style and its six colours; 0x004fc170 /
   0x004fc1a0 font A / B (the caller redraws: vtable +0x20(1), `redraw`
   here); 0x004fc1d0 the fill flag and colour. 0x004fbf50 / 0x004fc060
   replace the background / icon shape (none for an empty name: 0) and
   take its frame 0 box as the tile / icon size. */
void aoe_timeline_set_border_ror(AoeTimelineRor *t, int32_t style, uint8_t c0, uint8_t c1,
                                 uint8_t c2, uint8_t c3, uint8_t c4, uint8_t c5);
void aoe_timeline_set_font_ror(AoeTimelineRor *t, int which, const AoeTimelineFontRor *font);
void aoe_timeline_set_fill_ror(AoeTimelineRor *t, int32_t fill, uint8_t color);
int aoe_timeline_set_background_ror(AoeTimelineRor *t, const char *name);
int aoe_timeline_set_icons_ror(AoeTimelineRor *t, const char *name);
/* 0x004fc3e0: the layout. The sample count is the longest history (none:
   1 and the panel is disabled). The graph is width - 49 columns wide:
   scale = that / samples, and below 1.0 the graph compresses (scale
   1.0, columns = the width, samples - width to skip at width / skipped
   per kept sample). axis_w = columns * scale + 4 (truncated), axis_h =
   height - 4 * font A height - 26, left = x + 27, top = y + 15 + 2 *
   font A height, the graph 2 narrower and 3 shorter than the axes. A
   tick every (columns - 1) / 12 samples from 12 columns up, else every
   one. */
void aoe_timeline_layout_ror(AoeTimelineRor *t);
/* 0x004fc5a0: a tick mark (kind 3) for `sample` at column x with the
   total. 0x004fc620: a mark for the player's event at `sample`, if its
   history has one (the event's kind). Both are pushed on the list. */
void aoe_timeline_add_tick_ror(AoeTimelineRor *t, int32_t sample, int32_t total, int32_t x,
                               int32_t show);
void aoe_timeline_add_event_ror(AoeTimelineRor *t, uint8_t player,
                                const AoePlayerHistoryRor *history, uint8_t color,
                                int32_t sample, int32_t x, int32_t y, int32_t length);
/* 0x004fc6c0: where a mark's icon (frame `frame`) and caption go, kept
   inside the graph; whether the icon fits (then +0xa8 is set). */
int aoe_timeline_place_ror(AoeTimelineRor *t, AoeTimelineMarkRor *m, int16_t frame);
/* 0x004fdbe0: a mark's texts (strings 0x106a..0x106c for kinds 0..2; a
   tick's total and its time "h:mm" at 15 seconds a sample; "%ld/%1d"
   for kind 8), its placement, then its line (kinds 0..2, in black) or
   icon. */
void aoe_timeline_draw_mark_ror(AoeTimelineRor *t, AoeTimelineMarkRor *m, int16_t frame);
/* The draw (+0x30 0x004fcb00) and its parts: 0x004fd150 background (the
   tiled shape, the fill, or the parent's), 0x004fc1f0 border, 0x004fcb60
   axes and their titles (strings 0x2c57, 0x2c56), 0x004fd590 the graph
   (which adds the marks), 0x004fce60 the marks and captions, 0x004fd2e0
   "name - civilisation" beside each player's first band. */
void aoe_timeline_draw_ror(AoeTimelineRor *t);
void aoe_timeline_draw_background_ror(AoeTimelineRor *t);
void aoe_timeline_draw_border_ror(AoeTimelineRor *t);
void aoe_timeline_draw_axis_ror(AoeTimelineRor *t);
void aoe_timeline_draw_graph_ror(AoeTimelineRor *t);
void aoe_timeline_draw_marks_ror(AoeTimelineRor *t);
void aoe_timeline_draw_names_ror(AoeTimelineRor *t);

#endif
