#ifndef VITA_AOE_VIEW_TERRAIN_PASS_H
#define VITA_AOE_VIEW_TERRAIN_PASS_H

#include <stdint.h>

#include "view_lists.h"

/* Patched Rise of Rome 1.0a game view terrain pass (0x00512400) and its
   tile drawer (0x00513ea0). Semantic state with the original offsets in
   the comments; every effect on the world, the span regions, the draw
   rows and the rasterizer goes through the ops. */

/* One 24-byte map tile (map +0x8d8c row table) as the pass reads it. */
typedef struct {
    int16_t x, y;        /* +0x00, +0x02 screen position */
    uint8_t shape;       /* +0x04 */
    uint8_t terrain;     /* +0x05: low 5 bits the terrain, high 3 the elevation shape */
    uint8_t border;      /* +0x06: low 4 bits the border type, high 4 its shape */
    uint8_t last_fog;    /* +0x07 */
    uint8_t edge_visible;/* +0x08 */
    uint8_t fog_9;       /* +0x09 */
    uint8_t flags;       /* +0x0a (0x40: dirty) */
    uint8_t edge_explored; /* +0x0b */
} AoeViewTileRor;

/* The fog classes 0x00512400 gives a tile. */
#define AOE_VIEW_FOG_UNSEEN 0
#define AOE_VIEW_FOG_VISIBLE 0x0f
#define AOE_VIEW_FOG_EXPLORED 0x80

typedef struct {
    int16_t start_x, start_y;      /* +0x12a, +0x12c */
    int16_t cols, rows;            /* +0x116, +0x118: the loops run cols + 5 and rows * 2 + 12 */
    int32_t map_w, map_h;          /* map +0x08, +0x0c */
    int16_t terrain_count;         /* map +0x8d90 */
    uint8_t reveal_all, fog_edges; /* map +0x8db8, +0x8db9 */
    int has_visibility;            /* player (+0x100) +0x38 != 0 */
    uint32_t explored_mask, visible_mask; /* player +0xd4, +0xd8 */
    int32_t origin_x, origin_y;    /* +0x130, +0x134 */
    int16_t tile_w, half_w, half_h, elevation_step; /* +0x108, +0x10c, +0x10e, +0x110 */
    int16_t scroll_x, scroll_y;    /* +0x126, +0x128 */
    uint8_t min_kind;              /* +0x141 (written by the walk) */
    int32_t clip[4];               /* +0x8c, +0x90, +0x94, +0x98 (left, top, right, bottom) */
    int count_terrains;            /* +0xf8 */
    uint8_t incremental;           /* +0x142 */
    int32_t highlight_on, highlight2_on; /* +0x2a0, +0x2a4 */
    int32_t rect1[4], rect2[4];    /* +0x2a8.., +0x2b8.. */
    int32_t timers_active;         /* +0x358 */
    uint8_t drag_kind;             /* +0x168 */
    int32_t press_x, press_y, cur_x, cur_y; /* +0xc4, +0xc8, +0x16c, +0x170 */
    int32_t hover_on, hover_id;    /* +0x33c, +0x340 */
    uint32_t hover_time;           /* +0x344 */
    int32_t hover_messages;        /* +0x34c */
    uint8_t mode;                  /* +0x140 (written) */
    AoeViewMarkRor *marks;         /* +0x350 */
    int32_t last_region;           /* the original's local: -1 none */
} AoeViewTerrainPassRor;

enum { /* AoeViewTerrainPassOpsRor.step ids (a, b as noted) */
    AOE_VTP_TIMERS,         /* 0x0050f8d0 (timers active) */
    AOE_VTP_MOUSE,          /* 0x0044a850 (only with the mouse global 0x00582edc) */
    AOE_VTP_STEP_BLITS,     /* 0x005104a0(a) (only while +0x2d8) */
    AOE_VTP_ROWS_RESET,     /* 0x00516780 on the +0x338 draw rows (first published to 0x007c0460) */
    AOE_VTP_ROW_WINDOW,     /* 0x00516240(a, b) */
    AOE_VTP_BATCH,          /* 0x007c045c = a */
    AOE_VTP_FOGGED,         /* 0x007c0458 = a */
    AOE_VTP_REGION_BOUNDS,  /* 0x004a3850 twice with rect a: 0 the clip, 1 rect1, 2 rect2 */
    AOE_VTP_TILES_DRAWN,    /* 0x007d2014 += 1 */
    AOE_VTP_DRAG_BOX,       /* 0x00516510(4, 0x14, press - (dx, dy), cur - (dx, dy), 0 x4, 0x28, 0xff, 0) */
    AOE_VTP_PREVIEW,        /* 0x00511080 (edit mode 1, 7, 8) */
    AOE_VTP_MODE_15,        /* vtable +0xfc (edit mode 0x15) */
    AOE_VTP_MARKS_DROP,     /* 0x00515ff0 */
    AOE_VTP_LINK_ROWS,      /* the +0x338 table lists linked into the row heads / tails by +0x10 */
    AOE_VTP_COUNTS_RESET,   /* 0x007d1cf4 = 0x007d1cf0 = 0 */
    AOE_VTP_SHADOW_ROWS,    /* rows 0..5: each kind-0 node through the +0x178 then +0x17c region setups (0x540720, 0x540760, 0x5407a0, 0x5409a0) */
    AOE_VTP_PANEL_BOXES,    /* each active child panel (+0x58 list, +0x6c and +0x38 set): its +0xbc rectangle less (dx, dy) as a kind-3 node in row 0x1e (colour +0xe0) */
    AOE_VTP_DIRTY_ALL,      /* 0x004a3950(0, +0x324's +0x18 - 1, row) for its +0x1c rows */
    AOE_VTP_REGION_OP,      /* 0x004a4200(+0x178, +0x17c) */
    AOE_VTP_DIRTY_SPAN,     /* 0x004a3950(a, b, row c) */
    AOE_VTP_DIFF_ROWS,      /* the +0x338 rows against +0x334: unchanged nodes counted (0x007d1cf0), new / gone kind-0 nodes outlined (0x004a3d10, 0x007d1cf4), others 0x00513a40 */
    AOE_VTP_COPY_SPANS,     /* 0x004395b0(area, clip left, clip top, +0x324's spans, +0x180's +0x3c) with the clip moved by (dx, dy) */
    AOE_VTP_ROW_STARTS,     /* 0x00513980 */
    AOE_VTP_RASTER_SETUP,   /* 0x540720 with +0x32c and region +0x180; the area's +0xd0 = +0x180 */
    AOE_VTP_SPRITE_ROWS,    /* rows 6..0x28: kind-0 nodes rasterized (0x540680 swaps for flags 4 / 8), others 0x00513bc0 */
    AOE_VTP_UNLOCK,         /* 0x004387c0("view::view_function_terrain"); the area's +0xd0 = its +0xcc */
    AOE_VTP_SWAP_ROWS,      /* +0x338 <-> +0x334 */
    AOE_VTP_SET_MOUSE_FRAME,/* 0x0044a6c0(a) */
    AOE_VTP_SHOW_MESSAGE,   /* 0x00454270(a, buf, 0x7f); 0x00465a90(0, buf, 0x70, 0, 0, 0, 0) */
    AOE_VTP_HIDE_MESSAGE    /* 0x00465d10 */
};
typedef struct {
    void *context;
    void (*step)(void *context, int id, int32_t a, int32_t b, int32_t c);
    /* The tile at (x, y) (inside the map) read / written back. */
    void (*tile)(void *context, int32_t x, int32_t y, AoeViewTileRor *t);
    void (*tile_store)(void *context, int32_t x, int32_t y, const AoeViewTileRor *t);
    uint32_t (*visibility)(void *context, int32_t x, int32_t y);      /* 0x007d205c[y][x] */
    int16_t (*shape_height)(void *context, uint8_t shape);            /* map +0x1a + shape * 6 */
    uint32_t (*neighbor_mask)(void *context, int32_t x, int32_t y, uint32_t mask); /* 0x00513d30 */
    int16_t (*terrain_replacement)(void *context, uint8_t terrain);   /* map +0x144 + terrain * 0x198 */
    void (*terrain_counted)(void *context, uint8_t terrain);          /* map +0xd1 + terrain * 0x198 += 1 */
    void (*terrains_uncounted)(void *context);                        /* every one = 0 */
    /* The tile outlines: +0x184[shape] pair `which` 0 / 1 of edge index e, +0x188[shape][e]. */
    const void *(*outline)(void *context, uint8_t shape, uint32_t e, int which);
    const void *(*edge_outline)(void *context, uint8_t shape, uint32_t e);
    void (*region_add)(void *context, const void *outline, int32_t x, int32_t y);    /* 0x004a3f60 */
    void (*region_remove)(void *context, const void *outline, int32_t x, int32_t y); /* 0x004a3fb0 */
    /* 0x00513ea0 (the tile drawer) and 0x00451080 on +0x324 (the tile's objects). */
    void (*draw_tile)(void *context, int32_t x, int32_t y, uint8_t id, int32_t sx, int32_t sy,
                      uint8_t fog, int a, int b);
    void (*draw_objects)(void *context, int32_t sx, int32_t sy, int fogged);
    int (*mouse_global)(void *context);                 /* 0x00582edc */
    int (*blits_active)(void *context);                 /* +0x2d8 */
    int32_t (*game_state)(void *context);               /* game +0x1b0 */
    int32_t (*edit_mode)(void *context);                /* game +0x1b4 */
    int (*messages_enabled)(void *context);             /* game +0xa08 */
    int (*has_area)(void *context);                     /* +0x78 */
    int (*mouse_in_game)(void *context);                /* 0x00449b10 */
    uint32_t (*time_ms)(void *context);
    int32_t (*frame_count)(void *context, int32_t shape); /* 0x0049fa90 */
    /* 0x0049fe00(area +0x20, x - origin, y - origin, frame, (char) field_18, field_2c) with
       0x007c0454 = frames (+0x1c) and 0x007c0444 = -1. */
    void (*draw_mark)(void *context, const AoeViewMarkRor *m);
    void (*free_mark)(void *context, AoeViewMarkRor *m);  /* 0x00525fc2 */
    int (*cursor_in_view)(void *context, int32_t dx, int32_t dy, int32_t *x, int32_t *y); /* 0x00514500 */
    int (*pixel_object)(void *context, int32_t x, int32_t y, int32_t *id); /* 0x00514d70(.., 10, 0x28) */
    int (*object_cursor)(void *context, int32_t id, int32_t *frame, int32_t *string); /* player vtable +0x9c */
} AoeViewTerrainPassOpsRor;

/* 0x00512400(mode, dx, dy): the diagonal walk over cols + 5 tiles of
   rows * 2 + 12 rows from (+0x12a, +0x12c) (odd rows step y, even rows
   step x back). Each on-map tile is classed by its visibility word
   against the player masks: visible 0x0f; explored (or the whole map
   seen) 0x80 when the map has fog edges, else 0x0f; unseen 0 (its +0x07
   cleared, not drawn). A drawn tile must meet the clip rectangle. With
   +0x142 an unchanged tile (same fog, same edge indices) is skipped unless
   a highlight rectangle covers it (region 1 / 2). Its outlines (pair by
   the visible-edge index; the explored-edge outline removed) go to the
   span region, bounded by the clip or highlight rectangle when the
   region changes; the tile remembers fog and edges (+0x0a bit 0x40
   cleared) and is drawn (0x00513ea0, 0x007d2014 counted). Any tile with
   bit 0x40 is redrawn with both passes. Every tile's objects are drawn
   batched (0x007c045c) with 0x007c0458 for fog, in rows 0..5 for unseen
   tiles. The pass then runs the steps in the order of their ids,
   animating the +0x350 marks on the way (a mark of kind 1 / 2 whose
   interval elapsed advances its frame, wrapping at 0x0049fa90; a wrapped
   kind 2 is unlinked and freed and 0x00515ff0 runs at the end) and
   tracking the hovered object in game state 4 (0x00514500, 0x00514d70;
   a new object asks the player's +0x9c for a cursor frame and message).
   Returns 0. */
int aoe_game_view_terrain_pass_ror(AoeViewTerrainPassRor *v, const AoeViewTerrainPassOpsRor *o,
                                   uint8_t mode, int32_t dx, int32_t dy);

/* ---- 0x00513ea0: one tile ---- */
typedef struct {
    void *context;
    int16_t terrain_count;                                   /* map +0x8d90 */
    int (*terrain_enabled)(void *context, uint8_t terrain);  /* map +0x8c + terrain * 0x198 */
    const void *(*terrain_shape)(void *context, uint8_t terrain); /* +0xac */
    /* The border record (map +0x338c + type * 0x5a0): enabled (+0x00), shape (+0x20),
       per-edge (+0x59e) and whether the terrain under it is drawn (+0x59a). */
    int (*border_enabled)(void *context, uint8_t type);
    const void *(*border_shape)(void *context, uint8_t type);
    int (*border_per_edge)(void *context, uint8_t type);
    uint8_t (*border_draws_terrain)(void *context, uint8_t type);
    int16_t (*border_frame)(void *context, uint8_t type, uint8_t shape, uint8_t bshape,
                            int16_t x, int16_t y);                    /* 0x514fd0 */
    int (*border_frames)(void *context, uint8_t type, uint8_t shape, uint8_t bshape, int16_t x,
                         int16_t y, int32_t out[4]);                  /* 0x005150c0 */
    int16_t (*terrain_frame)(void *context, uint8_t terrain, uint8_t shape, int16_t x,
                             int16_t y);                              /* 0x00514ee0 */
    int32_t grid;                                                     /* 0x007d200c */
    int32_t game_state;                                               /* game +0x1b0 */
    uint32_t (*visibility)(void *context, int16_t x, int16_t y);      /* 0x007d205c */
    uint32_t explored_mask, visible_mask;                             /* player +0xd4, +0xd8 */
    int32_t *sync_explored, *sync_visible;                            /* 0x007d2020, 0x007d201c */
    void (*blit)(void *context, int32_t sx, int32_t sy, const void *shape, int32_t frame,
                 uint8_t fog, uint8_t flags, int a, int b);           /* 0x00514320 */
    void (*mark_dirty)(void *context, int16_t x, int16_t y);         /* tile +0x0a |= 0x40 */
    int32_t map_h;
} AoeViewTileDrawOpsRor;
/* Returns 1 when the terrain was blitted. Needs a known, enabled terrain.
   A border (type != 0, enabled, with a shape) with a frame draws its
   terrain only when its +0x59a says so (per-edge borders always). In
   the editor (game state 7) with a grid, a tile of odd checker parity
   without other flags gets flag 0x20. The terrain blit (shape and frame
   present) counts a fog-edge mismatch into the sync globals when `fog`
   is set. The border frames follow (per-edge: frames 0, 2, 1, 3 in that
   order). The tile to the left (x > 0) and the one below (y < h - 1)
   are marked dirty. */
int aoe_game_view_draw_tile_ror(const AoeViewTileDrawOpsRor *o, const AoeViewTileRor *t,
                                uint8_t terrain, int16_t sx, int16_t sy, int16_t x, int16_t y,
                                uint8_t fog, int a, int b);


/* ---- 0x005118c0: the view's generic tile walk (draw and pick modes) ---- */
enum { /* AoeViewWalkOpsRor.step ids */
    AOE_VW_EXPLORED_QUERY,  /* 0x0045ebc0 on the player's +0x58 list (result unused) */
    AOE_VW_D0_RESET,        /* every terrain's +0xd0 byte = 0 (mode 10, at the end) */
    /* The tile's object list (+0x0c) drawn into +0xf4 at (a = sx, b = sy), c = fogged:
       0x1e / 0x1f: 0x00451160(.., 1 / 10, fogged) twice, the target's +0xd0 first +0x17c with
       0x007c0458 = 1 then +0x178 with 0 (restored to its +0xcc); 0x20: 0x00451160(.., 0, fogged)
       and 0x15: 0x004511e0(.., fogged), both with 0x007c0458 set only for a fogged (0x80) tile
       of a map with fog edges, cleared after. */
    AOE_VW_OBJECTS_1E, AOE_VW_OBJECTS_1F, AOE_VW_OBJECTS_20, AOE_VW_OBJECTS_15
};
typedef struct {
    void *context;
    void (*step)(void *context, int id, int32_t x, int32_t y, int32_t a, int32_t b, int32_t c);
    void (*tile)(void *context, int32_t x, int32_t y, AoeViewTileRor *t);
    void (*tile_store)(void *context, int32_t x, int32_t y, const AoeViewTileRor *t);
    void *(*tile_ref)(void *context, int32_t x, int32_t y);          /* the tile's address */
    uint8_t (*fog_value)(void *context, int32_t x, int32_t y);       /* 0x00517670 */
    void (*shape_size)(void *context, uint8_t shape, int16_t *w, int16_t *h); /* map +0x18 / +0x1a + shape * 6 */
    int16_t (*terrain_replacement)(void *context, uint8_t terrain);
    void (*terrain_counted)(void *context, uint8_t terrain);          /* +0xd1 += 1 */
    void (*terrains_uncounted)(void *context);                        /* +0xd1 = 0 */
    int (*area)(void *context, int32_t out[4]);                       /* 0x00510070(x0, y0, x1, y1, 1) */
    /* 0x005152f0(tile, sx, sy, x, y, point, arg, out_sx, out_sy, preferred, fogged). */
    void *(*pick_object)(void *context, void *tile, int32_t sx, int32_t sy, int32_t x, int32_t y,
                         const int16_t *point, void *arg, int16_t *out_sx, int16_t *out_sy,
                         void *preferred, int fogged);
    uint8_t (*object_kind)(void *context, void *object);              /* template (+0x08) +0x8d */
    void (*object_point)(void *context, void *object, float *x, float *y); /* +0x38, +0x3c */
    int (*terrain_hit)(void *context, void *tile, int32_t sx, int32_t sy, int32_t x, int32_t y,
                       const int16_t *point);                         /* 0x005151d0 */
    void (*elevation_point)(void *context, int32_t dx, int32_t dy, void *tile, float *x,
                            float *y);                                /* 0x00447ab0 */
} AoeViewWalkOpsRor;
/* 0x005118c0(mode, min_kind, point, arg, result, out_x, out_y, out_sx,
   out_sy) on the view state `v` (the same fields as the terrain pass plus
   the scroll +0x126 / +0x128 and the half sizes). Modes: 10 counts the
   terrains (and clears their +0xd0 after), 11 copies each visible tile's
   +0x09 to +0x07, 14 limits the walk to the 0x00510070 area, 0x1e /
   0x1f / 0x20 / 0x15 draw tile objects, '(' picks the terrain under
   `point` ('3': the tile and its map point, the elevation taken into
   account), '<' the tile at the truncated *out_x / *out_y ('3'), ')' /
   '*' / '+' / ',' pick objects ('4'; ',' adds every hit; '*' falls back
   to the terrain while nothing was found; the best object is the one of
   the highest kind, a lower kind only replacing a best that is the
   preferred one by another object; meeting the preferred object again
   returns it at once). The walk covers (+0x116 + 4) tiles of
   (+0x118 + 6) * 2 rows alternately stepping y and x back; tiles are
   clipped against the panel rectangle (and, in mode 14, the area). Off
   the map only '(' and '*' continue with no tile; a fogged-out tile
   continues only in modes 14, '(', '<', '*'. Returns '2' when nothing
   was hit. */
uint8_t aoe_main_view_walk_ror(AoeViewTerrainPassRor *v, const AoeViewWalkOpsRor *o, uint8_t mode,
                               uint8_t min_kind, const int16_t *point, void *arg, void **result,
                               float *out_x, float *out_y, int16_t *out_sx, int16_t *out_sy);


/* ---- 0x00514fd0: a border's frame ---- */
/* A map border record (map +0x338c + type * 0x5a0): its shape (+0x20),
   the per-edge flag (+0x59e) and the frame table at +0x3c: entry
   shape * 12 + border shape, three int16 (count, step, base). */
typedef struct {
    int has_shape;
    int16_t per_edge;
    const int16_t *table;
} AoeViewBorderRor;
/* -1 without a shape or an empty entry. With two or more frames the
   index is y % count for border shapes 11 / 12 (per-edge 0) or 3 / 2
   (per-edge 1), x % count for 9 / 10 (per-edge 0) or 1 / 4 (per-edge 1),
   else 0 (also when it exceeds count - 1). Returns base + step * index. */
int16_t aoe_main_view_border_frame_ror(const AoeViewBorderRor *b, uint8_t shape, uint8_t bshape,
                                       int16_t x, int16_t y);

#endif
