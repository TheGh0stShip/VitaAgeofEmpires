#ifndef VITA_AOE_VIEW_LISTS_H
#define VITA_AOE_VIEW_LISTS_H

#include <stdint.h>

#include "span_region.h" /* AoeNodePoolRor */

/* Patched Rise of Rome 1.0a game view lists: the view's marks (+0x350,
   0x30-byte nodes such as the flashing order flag 0x00509150 adds with
   the world's flag shape, frame count 0xf, kind 2 and 125 ms) and the
   row-bucketed draw list (64-byte nodes from a node pool, row heads and
   tails, a row window and a clip rectangle). Semantic storage. */

typedef struct AoeViewMarkRor {
    struct AoeViewMarkRor *next, *prev; /* +0x00, +0x04 */
    int32_t shape;                      /* +0x08 */
    int32_t field_c;                    /* +0x0c */
    int32_t x, y;                       /* +0x10, +0x14 */
    int32_t field_18;                   /* +0x18 */
    int32_t frames;                     /* +0x1c */
    int32_t kind;                       /* +0x20 (1: dropped by 0x00515ff0) */
    int32_t frame;                      /* +0x24, 0 */
    int32_t interval;                   /* +0x28 */
    int32_t field_2c;                   /* +0x2c */
} AoeViewMarkRor;

/* 0x00516050 prepends a mark (nothing when out of memory); 0x00515fb0
   frees them all; 0x00515ff0 unlinks and frees those of kind 1. */
void aoe_view_mark_add_ror(AoeViewMarkRor **list, int32_t shape, int32_t field_c, int32_t x,
                           int32_t y, int32_t field_18, int32_t frames, int32_t field_2c,
                           int32_t kind, int32_t interval);
void aoe_view_marks_free_ror(AoeViewMarkRor **list);
void aoe_view_marks_drop_once_ror(AoeViewMarkRor **list);

typedef struct AoeDrawRowNodeRor {
    void *free_link;                  /* +0x00 (the pool's) */
    struct AoeDrawRowNodeRor *next;   /* +0x04 */
    int32_t id;                       /* +0x08 */
    uint8_t body[0x20];
    int32_t key;                      /* +0x2c */
    uint8_t tail[0x10];
} AoeDrawRowNodeRor;

typedef struct {
    AoeNodePoolRor pool;              /* +0x00..+0x38 */
    int32_t clip_left, clip_right;    /* +0x3c, +0x40 */
    int32_t clip_top, clip_bottom;    /* +0x44, +0x48 */
    int32_t rows;                     /* +0x4c */
    int32_t row_lo, row_hi;           /* +0x50, +0x54 */
    void **table;                     /* +0x58 (+0x68 entries) */
    AoeDrawRowNodeRor **heads;        /* +0x5c (rows + 1) */
    AoeDrawRowNodeRor **tails;        /* +0x60 (rows + 1) */
    int32_t table_count;              /* +0x68 */
} AoeDrawRowsRor;

/* 0x00516360: the clip rectangle, only when left <= right and top <=
   bottom. 0x00516240: the row window (either order, clamped to 0 and
   the row count). 0x005165f0: the first node in rows y0..y1 (either
   order, clamped) with this ID and key, or NULL. 0x00516780 empties
   it (the table, heads and tails zeroed, the pool reset). 0x005162f0 is
   the pool take (aoe_pool_take_ror with 64-byte nodes). 0x005161c0 frees
   the pool's blocks and the three tables. */
void aoe_draw_rows_set_clip_ror(AoeDrawRowsRor *d, int32_t l, int32_t t, int32_t r, int32_t b);
void aoe_draw_rows_set_window_ror(AoeDrawRowsRor *d, int32_t a, int32_t b);
AoeDrawRowNodeRor *aoe_draw_rows_find_ror(const AoeDrawRowsRor *d, int32_t id, int32_t y0,
                                          int32_t y1, int32_t key);
void aoe_draw_rows_reset_ror(AoeDrawRowsRor *d);
AoeDrawRowNodeRor *aoe_draw_rows_take_ror(AoeDrawRowsRor *d);
void aoe_draw_rows_free_ror(AoeDrawRowsRor *d);

/* 0x0051b070: a map point kept on the map (+0x104's +0x08 / +0x0c
   size): below 0 becomes 0, at or past the edge the size less 1.0. */
void aoe_view_clamp_point_ror(float point[2], int32_t map_w, int32_t map_h);

/* The view's timers (+0x354, +0x35c entries of 0x14 bytes). */
typedef struct {
    int32_t active;   /* +0x00 */
    int32_t field_4;
    int32_t time;     /* +0x08 */
    int32_t period;   /* +0x0c */
    int32_t kind;     /* +0x10 */
} AoeViewTimerRor;

/* 0x0051b0f0: each active timer of `kind` has its period taken off its
   time. */
void aoe_view_timers_rewind_ror(AoeViewTimerRor *timers, int32_t count, int32_t kind);

/* 0x0050f970: a flashing timer for object `index` (`duration` ms,
   `flag` bits) in the last free slot (nothing when none is free; the
   active count +0x358 grows); on the way every timer whose flag equals
   `match` (when nonzero) has its period taken off its start time.
   0x0050f8d0: each active timer whose object is gone or whose time is up
   is freed (the bits cleared on time-up); otherwise the object's +0x36
   gets the bits for the first 256 ms of every 512 and loses them for the
   rest. Here `time` is the start, `period` the duration and `kind` the
   flag. */
void aoe_view_timer_add_ror(AoeViewTimerRor *timers, int32_t count, int32_t *active,
                            int32_t index, int32_t duration, int32_t flag, int32_t match,
                            uint32_t now);
void aoe_view_timers_tick_ror(AoeViewTimerRor *timers, int32_t count, int32_t *active,
                              uint8_t *(*object_flags)(void *context, int32_t index),
                              void *context, uint32_t now);

/* The game view's tile geometry (0x0050fde0): with a map and a tile size,
   the tiles across and down the view and the centre offsets; else all 0. */
typedef struct {
    int has_map;                      /* +0x70 */
    int32_t width, height;            /* +0x14, +0x18 */
    int16_t tile_w, tile_h;           /* +0x108, +0x10a */
    int16_t half_tile_w, half_tile_h; /* +0x10c, +0x10e */
    int16_t origin_x, origin_y;       /* +0x8c, +0x90 */
    int16_t across, down;             /* +0x116, +0x118 */
    int16_t centre_x, centre_y;       /* +0x11a, +0x11c */
    int16_t field_11e, field_120;     /* +0x11e, +0x120 */
    int16_t field_122, field_124;     /* +0x122, +0x124 */
} AoeViewGeometryRor;
void aoe_view_geometry_ror(AoeViewGeometryRor *g);

#endif
