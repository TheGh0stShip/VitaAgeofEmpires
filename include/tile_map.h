#ifndef VITA_AOE_TILE_MAP_H
#define VITA_AOE_TILE_MAP_H

#include <stdint.h>

#include "player_outcome.h" /* AoeOutcomeStream */

/* Patched Rise of Rome 1.0a map tiles (the map object, vtable
   0x005445a0): the tile shape sizes, screen positions, elevation steps
   and smoothing, square terrain / elevation brushes, invalidation and
   the old map-data load. Semantic storage (one contiguous row-major
   tile array for the original row pointers at +0x8d8c); offsets noted. */

typedef struct {
    int16_t screen_x, screen_y; /* +0x00, +0x02 */
    uint8_t shape;              /* +0x04 */
    uint8_t terrain_elevation;  /* +0x05: terrain low 5 bits, elevation high 3 */
    uint8_t border;             /* +0x06 */
    uint8_t dirty;              /* +0x07 (0xcc when invalidated) */
    uint8_t field_8, view;      /* +0x08, +0x09 */
    uint8_t draw_flag;          /* +0x0a */
} AoeMapTileRor;

typedef struct {
    int16_t width, height, offset; /* +0x18 + shape * 6 */
} AoeTileShapeRor;

typedef struct {
    int32_t width, height;         /* +0x08, +0x0c */
    AoeMapTileRor *tiles;          /* +0x8d8c rows */
    AoeTileShapeRor shapes[19];    /* +0x18 */
    int16_t tile_w, tile_h;        /* +0x8d96, +0x8d98 */
    int16_t half_h, half_w;        /* +0x8d9a, +0x8d9c */
    int16_t elevation_h;           /* +0x8d9e */
} AoeTileMapRor;

static inline uint8_t aoe_tile_elevation_ror(const AoeMapTileRor *t) { return t->terrain_elevation >> 5; }
static inline uint8_t aoe_tile_terrain_ror(const AoeMapTileRor *t) { return t->terrain_elevation & 0x1f; }

typedef struct {
    void *context;
    /* 0x004445e0: a new map of this size (the tiles reallocated). */
    int (*resize)(void *context, AoeTileMapRor *map, int32_t w, int32_t h);
    /* 0x00447e40(x0, y0, x1, y1, 0, 0, 0): the shapes and borders. */
    void (*refresh_shapes)(void *context, AoeTileMapRor *map, int32_t x0, int32_t y0, int32_t x1,
                           int32_t y1);
    /* 0x00448270(a, b, x0, y0, x1, y1, terrain, c, d). */
    void (*paint_rect)(void *context, AoeTileMapRor *map, void *a, int32_t b, int32_t x0, int32_t y0,
                       int32_t x1, int32_t y1, int32_t terrain, int32_t c, int32_t d);
    /* 0x00448160(map +0x8dc0, x, y, terrain, 0, 0). */
    void (*paint_cell)(void *context, int32_t x, int32_t y, int32_t terrain);
    /* World 0x0051b6d0 / 0x0051c950 around a world paint. */
    void (*world_begin)(void *context, int32_t world);
    void (*world_end)(void *context, int32_t world);
    /* The terrain pair border (terrain +0x14a + other * 2) and whether a
       border is in use (+0x338c + border * 0x5a0). */
    int16_t (*border_between)(void *context, uint8_t terrain, uint8_t other);
    int (*border_used)(void *context, int16_t border);
} AoeTileMapOpsRor;

/* 0x00447a70: shape `i`'s size and offset. 0x00444240: the default 64x32
   tiles (half 32x16, elevation 16) and the 19 shapes: 0 64x32; the flat
   and the steep slopes width 64 by 16, 32 or 48 with offset -8 / 8;
   15 and 16 64x32 (8); 17 and 18 32x32 (0). */
void aoe_tilemap_set_shape_ror(AoeTileMapRor *m, uint8_t i, int16_t w, int16_t h, int16_t offset);
void aoe_tilemap_default_shapes_ror(AoeTileMapRor *m);
/* 0x00444e00: the tiles. */
AoeMapTileRor *aoe_tilemap_tiles_ror(const AoeTileMapRor *m);
/* 0x00444980: normalise and clamp the rectangle; each tile's +0x07 to
   0xcc and +0x0a to `flag`. */
void aoe_tilemap_invalidate_ror(AoeTileMapRor *m, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                                uint8_t flag);
/* 0x004ac340 (a unit's sprites changed): tiles x - 2 .. x + 1 and
   y - 2 .. y + 1 around its truncated position, flag 0. */
void aoe_tilemap_invalidate_around_ror(AoeTileMapRor *m, float x, float y);
/* 0x00444aa0 (0x00444a80: the whole map, 0, 0 .. width - 1, height -
   1): each tile's screen position from the clamped rectangle:
   x (x + y) * half width, y (y - x) * half height less the elevation
   (one more step for shapes 2, 6, 8, 10, 14, 15 and 16). */
void aoe_tilemap_positions_ror(AoeTileMapRor *m, int32_t x0, int32_t y0, int32_t x1, int32_t y1);
/* 0x00444e10: over the rectangle grown by one, one step towards
   `target` for each tile whose eight neighbours differ by more than one
   in that direction (in place, rows then columns); whether any changed. */
int aoe_tilemap_step_ror(AoeTileMapRor *m, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                         uint8_t target);
/* 0x00445180: repeated (growing the rectangle by one each pass) until
   nothing changes: a tile lower than both its opposite neighbours
   (north and south or east and west) is raised by one below `limit`,
   else its higher neighbours are brought down to it. */
void aoe_tilemap_smooth_ror(AoeTileMapRor *m, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                            uint8_t limit);
/* 0x00445fc0 terrain and 0x004461a0 elevation over the clamped square
   of radius `r` around (x, y); both return 1. */
int aoe_tilemap_paint_square_ror(AoeTileMapRor *m, int32_t x, int32_t y, int32_t r, int32_t terrain,
                                 const AoeTileMapOpsRor *ops);
int aoe_tilemap_raise_square_ror(AoeTileMapRor *m, int32_t x, int32_t y, int32_t r, uint8_t elevation);
/* Line brushes (vtable +0x10 0x00446050 terrain, +0x18 0x00446250
   elevation): `width` is halved (arithmetic shift) into the square radius;
   the start is clamped up to 0 and the end down to the last row/column.
   Steps = ftol(sqrt(dx^2 + dy^2)); the brush (vtable +0x0c / +0x14, the
   squares above) is applied at the start, at ftol of the double position
   after each of the steps (dx/steps, dy/steps), and at the end unless the
   last position already equals it exactly. The elevation line then runs
   the clean-up 0x00445570 over the line's bounding box widened by the
   radius (`cleanup`, may be NULL). Both return 1. */
int aoe_tilemap_paint_line_ror(AoeTileMapRor *m, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                               int32_t width, int32_t terrain, const AoeTileMapOpsRor *ops);
int aoe_tilemap_raise_line_ror(AoeTileMapRor *m, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                               int32_t width, int32_t elevation,
                               void (*cleanup)(void *context, int32_t x0, int32_t y0,
                                               int32_t x1, int32_t y1, int32_t value),
                               void *context);
/* 0x004433c0 (template method: world, x, y, then eight short outputs):
   the screen offsets of a template footprint's four corners relative to
   the tile holding (x, y). With fx = x - (int16)ftol(x) + size_x
   (template +0x30) and fy likewise with size_y (+0x34), all rounded to
   float, and the map's half_w / half_h: corner (fx, fy), then fx - 2
   size_x, then fy - 2 size_y, then fx + 2 size_x again; each corner gives
   x = ftol((fy + fx) * half_w) and y = ftol((fy + 1 - fx) * half_h).
   `out` receives x0, y0, x1, y1, x2, y2, x3, y3. Returns 1. */
int aoe_tilemap_footprint_corners_ror(const AoeTileMapRor *m, float size_x, float size_y,
                                      float x, float y, int16_t out[8]);
/* 0x00444cb0: the old map data: i32 width, height, then per tile
   terrain, elevation and an ignored byte (rows and columns both counted
   by the height, as in the original), then the shapes and a terrain
   repaint of the whole map. */
int aoe_tilemap_load_old_ror(AoeTileMapRor *m, AoeOutcomeStream *s, const AoeTileMapOpsRor *ops);
/* 0x00444580: a new w x h map painted with one terrain (inside the
   world's begin / end when `world` is set). */
int aoe_tilemap_new_filled_ror(AoeTileMapRor *m, void *a, int32_t world, int32_t terrain, int32_t w,
                               int32_t h, const AoeTileMapOpsRor *ops);
/* 0x00448c50: `*cell` becomes `terrain` unless a usable border (0..15)
   joins the two. */
void aoe_tilemap_pick_terrain_ror(uint8_t terrain, uint8_t *cell, const AoeTileMapOpsRor *ops);
/* 0x00446540: an empty map method. */
void aoe_tilemap_nothing_ror(AoeTileMapRor *m);

/* 0x00446550 (map +0x28): a random map. The world's +0x1d becomes 3
   for the duration; the seed is the game's +0x2c (or rand() when -1),
   kept at +0x34 and logged ("Random Map Seed = %d"); a size below 0
   keeps the map's. With srand(seed) before each: a new map of terrain 0
   (0x00444580), the controller's +0x14(map, world, a, b, 1), then its
   +0x00 and +0x18; then "After Random Map Generation random number is
   %d" with the next rand(). */
typedef struct {
    void *context;
    int32_t (*game_seed)(void *context);
    void (*set_used_seed)(void *context, int32_t seed);
    int32_t (*rand)(void *context);
    void (*srand)(void *context, uint32_t seed);
    void (*log)(void *context, const char *line);
    uint8_t (*world_flag)(void *context);
    void (*set_world_flag)(void *context, uint8_t v);
    void (*rm_setup)(void *context, AoeTileMapRor *map, int32_t world, int32_t a, int32_t b);
    void (*rm_generate)(void *context);
    void (*rm_finish)(void *context);
} AoeTileMapGenOpsRor;
int aoe_tilemap_generate_ror(AoeTileMapRor *m, int32_t world, int32_t w, int32_t h, int32_t a,
                             int32_t b, const AoeTileMapOpsRor *ops,
                             const AoeTileMapGenOpsRor *gen);

/* 0x00446a20 (map +0x2c; the RoR map's 0x004ed850 forwards to it): the
   data-file save. The whole 0x8dd0-byte map record is written with its
   sound pointers replaced by sound IDs: for each of the 32 terrains
   (0x198 bytes from +0x8c) that is enabled (its first byte), the slot at
   record +0xb0 + i * 0x198 becomes the sound's ID or -1 when it has
   none; for each of the 16 borders (0x5a0 bytes from +0x338c) the slot
   at +0x33b0 + i * 0x5a0 becomes the ID, or -1 when the border is
   disabled or has no sound. Then the random map controller's save
   (0x0046dcc0). Here `image` is the record in its 32-bit disk layout
   (the other pointer slots as they are, which the original also writes
   raw), and `sound_id` gives a terrain's (border 0) or border's (1)
   sound ID, -1 for none. */
enum {
    AOE_MAP_RECORD_SIZE = 0x8dd0,
    AOE_MAP_TERRAINS = 32,
    AOE_MAP_BORDERS = 16
};
int aoe_tilemap_save_data_ror(const uint8_t image[AOE_MAP_RECORD_SIZE],
                              int32_t (*sound_id)(void *context, int border, int index),
                              void (*controller_save)(void *context, AoeOutcomeStream *s),
                              void *context, AoeOutcomeStream *s);


/* What the map owns at destruction. */
typedef struct {
    void *zones;              /* +0x8dc4 (dtor 0x00521c80) */
    void *random_map;         /* +0x8dbc (dtor 0x0046dbe0) */
    void *raw04, *raw8db0, *rows8d8c, *raw8db4; /* plain allocations */
    void *terrain_shapes[32]; /* terrain i's shape (+0x3214 - (31 - i) * 0x198) */
    void *border_shapes[16];  /* border i's shape (+0x880c - (15 - i) * 0x5a0) */
    void *masks;              /* +0x8dc8 (dtor 0x00517f30) */
    void *v8dcc;              /* +0x8dcc (dtor 0x005168d0) */
} AoeMapOwnedRor;

enum {
    AOE_MAP_OWNED_ZONES, AOE_MAP_OWNED_RANDOM_MAP, AOE_MAP_OWNED_SHAPE,
    AOE_MAP_OWNED_MASKS, AOE_MAP_OWNED_8DCC
};

typedef struct {
    void *context;
    /* The member's destructor, then delete. */
    void (*destroy)(void *context, int kind, void *object);
    void (*free_raw)(void *context, void *p);
} AoeMapOwnedOpsRor;

/* 0x00444110 (base map destructor, deleting form 0x00443d60; the RoR map's 0x004ed840 resets its
   vtable and calls it, 0x004ed7e0 also deletes): zones, random map
   (both cleared), the four plain blocks (not cleared), the 32 terrain
   shapes then the 16 border shapes, each from the last down (cleared),
   then the masks and +0x8dcc (left set). */
void aoe_map_release_owned_ror(AoeMapOwnedRor *m, const AoeMapOwnedOpsRor *ops);

/* Template slot 0x00443140 (all template classes' vtables): keep a unit
   footprint (half sizes template +0x30 / +0x34) centred on (*x, *y)
   inside the map (world +0x28 width / height less 1e-5, 0x00544578): a
   low edge below 0 or a high edge at or past the limit shifts the centre
   (x first; the high-y test uses the low-y-shifted bottom edge only for
   the shift amount). With `snap`, the footprint is then aligned to tile
   edges using each edge's fractional part (|edge| less its int16
   truncation; the high edges use 1 - fraction): x moves left by the high
   fraction when the low fraction is smaller, else right by 1 - low
   fraction, unless the two together are under one tile; y moves down by
   1 - low fraction when that fraction is larger, else up by the high
   fraction (ties go up for y, right for x). Returns 1. */
int aoe_map_clamp_footprint_ror(float half_w, float half_h, int32_t map_w,
                                int32_t map_h, float *x, float *y, int snap);


/* 0x00447670 (map method): the objects on tile (x, y) swept (the terrain
   byte 0x00447de0 is read first, unused). Each object whose template ID
   lies in lo..hi stays; otherwise it goes (vtable +0x68 when `delete` is
   0, else deleted) when `player` is negative, when the player's class
   table (+0x220 + player * 0x198 count, +0x18a + player * 0x198 entries)
   lists the ID, or when its template's placement check (+0x20 at the
   object's position with flags 0, 0, 1, 1, 1, 0, 0) answers nonzero.
   Returns the last template ID considered (-1 for none). */
typedef struct {
    void *context;
    int32_t (*first)(void *context, int32_t x, int32_t y);     /* an object or 0 */
    int32_t (*next)(void *context, int32_t object);
    int16_t (*template_id)(void *context, int32_t object);
    int16_t (*player_class_count)(void *context, int32_t player);
    int16_t (*player_class)(void *context, int32_t player, int16_t i);
    int (*can_place)(void *context, int32_t object);
    void (*remove)(void *context, int32_t object);             /* +0x68 */
    void (*destroy)(void *context, int32_t object);            /* delete */
} AoeTileSweepOpsRor;
int32_t aoe_tilemap_sweep_tile_ror(const AoeTileSweepOpsRor *o, int32_t x, int32_t y,
                                   int32_t player, int32_t delete_them, int32_t lo, int32_t hi);

#endif
