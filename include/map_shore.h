#ifndef VITA_AOE_MAP_SHORE_H
#define VITA_AOE_MAP_SHORE_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a map shoreline and cliff tools (map class
   vtable 0x005499f4, 0x004ed4b0..0x004eee70). Tiles are read through the
   view: terrain = tile byte +5 & 0x1f, shape = tile byte +4. Painting,
   object creation and refreshes go through callbacks. */

typedef struct {
    int32_t width, height;           /* map +0x08, +0x0c */
    void *context;
    uint8_t (*terrain)(void *context, int32_t x, int32_t y);
    uint8_t (*shape)(void *context, int32_t x, int32_t y);
    /* 0x00448160(map +0x8dc0, x, y, terrain, 0, 0). */
    void (*paint)(void *context, int32_t x, int32_t y, uint8_t terrain);
    /* 0x00448270(0, 0, x0, y0, x1, y1, 0, 0, 0). */
    void (*refresh)(void *context, int32_t x0, int32_t y0, int32_t x1,
                    int32_t y1);
} AoeShoreMapRor;

/* 0x004ee960: terrains 1, 22 and 4 count as water. */
int aoe_shore_is_water_ror(uint8_t terrain);
/* 0x004eecd0: no neighbour of (x, y) (8 directions, in the map) has a
   nonzero shape byte. */
int aoe_shore_flat_ror(const AoeShoreMapRor *map, int32_t x, int32_t y);
/* 0x004ed970: paint the square of radius r around (x, y), clamped; water
   only on flat tiles. Returns 1. */
int aoe_shore_paint_area_ror(const AoeShoreMapRor *map, int32_t x, int32_t y,
                             int32_t r, uint8_t terrain);
/* 0x004eedb0: water tiles that are not flat become terrain 0. */
void aoe_shore_dry_slopes_ror(const AoeShoreMapRor *map, int32_t x0,
                              int32_t y0, int32_t x1, int32_t y1);
/* 0x004ee980: beaches (2) with no water around become `land`; other
   non-water tiles next to water become beaches. */
void aoe_shore_fix_beaches_ror(const AoeShoreMapRor *map, int32_t x0,
                               int32_t y0, int32_t x1, int32_t y1,
                               uint8_t land);
/* 0x004eee70: repeat over the rectangle grown by one each round until no
   land tile is pinched: pass 0 for water on opposite sides, pass 1 for the
   diagonal patterns. A pinched tile is resolved by painting `terrain` over
   its water neighbours (land terrain) or onto itself when flat (water
   terrain). Then beaches are fixed and the rectangle refreshed. */
void aoe_shore_smooth_ror(const AoeShoreMapRor *map, int32_t x0, int32_t y0,
                          int32_t x1, int32_t y1, uint8_t terrain);

/* 0x004eda70: the base map's line paint (callback), then smoothing of
   the line's box grown by r + 2. 0x004edb00: the base map's raise
   (callback), then slope drying and smoothing to terrain 0 of the box grown by
   (size & 0xff) + r + 2. Both return 1. */
typedef void (*AoeShoreBaseFn)(void *context, int32_t x0, int32_t y0,
                               int32_t x1, int32_t y1, int32_t r,
                               uint32_t value);
int aoe_shore_paint_line_ror(const AoeShoreMapRor *map, AoeShoreBaseFn base,
                             int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                             int32_t r, uint8_t terrain);
int aoe_shore_raise_ror(const AoeShoreMapRor *map, AoeShoreBaseFn base,
                        int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                        int32_t r, uint32_t size);

/* ---- cliffs ---- */

/* One cliff rule (0x28 bytes from map +0x8df8), keyed by the four side
   deltas a, b, c, d (-1..2): object type and shape byte, an alternative
   pair, the key, and the placement offsets within the 3-tile cell. */
typedef struct {
    int32_t type, shape, alt_type, alt_shape;
    int32_t key[4];
    float offset_x, offset_y;
} AoeCliffRuleRor;

typedef struct {
    AoeCliffRuleRor rules[256];
    /* +0x8de4..+0x8df4: the cliff drawing state (last cell and side). */
    int32_t last_x, last_y, cell_x, cell_y, last_side;
} AoeCliffTableRor;

/* The cliff table part of the map constructor 0x004ed4b0: every rule
   empty (types -1), then the twenty original cliff pieces (types 264..
   272). */
void aoe_cliff_table_init_ror(AoeCliffTableRor *table);
/* 0x004edbc0. */
void aoe_cliff_set_rule_ror(AoeCliffTableRor *table, int32_t a, int32_t b,
                            int32_t c, int32_t d, int32_t type, int32_t shape,
                            int32_t alt_type, int32_t alt_shape, float ox,
                            float oy);
/* The rule for a key (NULL outside -1..2); 0x004ede00: it has a type. */
AoeCliffRuleRor *aoe_cliff_rule_ror(AoeCliffTableRor *table, int32_t a,
                                    int32_t b, int32_t c, int32_t d);
int aoe_cliff_rule_exists_ror(AoeCliffTableRor *table, int32_t a, int32_t b,
                              int32_t c, int32_t d);
/* 0x004edd50: the key of the rule whose type/shape (or alternative)
   matches a cliff object; zeros for none or no object. */
void aoe_cliff_key_of_ror(const AoeCliffTableRor *table, int has_object,
                          int16_t type, uint8_t shape, int32_t key[4]);
/* 0x004ee130: the opposite side (0..3). */
int32_t aoe_cliff_opposite_ror(int32_t side);

typedef struct {
    void *object;
    int16_t type;
    uint8_t shape;
    float x, y;          /* +0x38, +0x3c */
} AoeCliffObjectRor;

typedef struct {
    void *context;
    /* Objects of tile (x, y) in list order; nonzero while one exists. */
    int (*object_at)(void *context, int32_t x, int32_t y, int32_t index,
                     AoeCliffObjectRor *out);
    int32_t (*rand)(void *context);              /* 0x0052605d */
    /* Gaia vtable +0x94(type, x, y, 0, 0); NULL on failure. */
    void *(*create)(void *context, int32_t type, float x, float y);
    void (*set_shape)(void *context, void *object, uint8_t shape); /* +0x35 */
    /* Viewed player +0x6c, 0x0045eac0(first, second). */
    void (*touch)(void *context, int32_t first, int32_t second);
    /* 0x0050a600(0, type, x, y, 1) on map +0x8dc0. */
    void (*placed)(void *context, int32_t type, float x, float y);
    void (*remove)(void *context, void *object);  /* vtable +0(1) */
    /* Base map tools 0x004463e0 (nonzero: handled) and 0x004463f0. */
    int (*base_drag)(void *context, int32_t x, int32_t y, int32_t a,
                     uint8_t erase);
    void (*base_press)(void *context, int32_t x, int32_t y, int32_t a,
                       int32_t b, int32_t c, int32_t d);
} AoeCliffOpsRor;

/* 0x004edc50: the first cliff object (types 264..273) on the four
   centre tiles of cell (gx, gy): (3gx+1, 3gy+1), (3gx+1, 3gy+2),
   (3gx+2, 3gy+1), (3gx+2, 3gy+2). 0 when none or off the map. */
int aoe_cliff_at_ror(const AoeShoreMapRor *map, const AoeCliffOpsRor *ops,
                     int32_t gx, int32_t gy, AoeCliffObjectRor *out);
/* 0x004ede40: place the rule's piece for cell (gx, gy) at (3gx + ox,
   3gy + oy), taking the alternative when it exists and rand() * 2 >
   0x7fff; touch the 3 x 3 tiles around it, set its shape and report it.
   Returns the object (NULL for an empty rule). */
void *aoe_cliff_place_ror(AoeCliffTableRor *table, const AoeCliffOpsRor *ops,
                          int32_t gx, int32_t gy, int32_t a, int32_t b,
                          int32_t c, int32_t d);

/* 0x004edfc0: the cliff in the neighbouring cell on `side` (0 x+1, 1 y+1,
   2 x-1, 3 y-1; nothing off the map) is removed (its 3 x 3 tiles touched)
   and replaced by the piece for its key with the side facing back
   cleared. */
void aoe_cliff_drop_side_ror(AoeCliffTableRor *table,
                             const AoeShoreMapRor *map,
                             const AoeCliffOpsRor *ops, int32_t gx,
                             int32_t gy, int32_t side);
/* 0x004ee640: erase the cliff of a cell, first detaching every joined
   neighbour. */
void aoe_cliff_erase_ror(AoeCliffTableRor *table, const AoeShoreMapRor *map,
                         const AoeCliffOpsRor *ops, int32_t gx, int32_t gy);
/* 0x004ee150: join cell (gx, gy) on `side` with delta `value` (0 picks
   1, or -1 when 1 has no rule), keeping at most one other side (`keep`,
   or any two when negative), dropping neighbours whose sides no longer
   fit, and placing the new piece. Returns the delta used (an existing
   side's delta when it already matches or value is 0). */
int32_t aoe_cliff_join_ror(AoeCliffTableRor *table, const AoeShoreMapRor *map,
                           const AoeCliffOpsRor *ops, int32_t gx, int32_t gy,
                           int32_t side, int32_t value, int32_t keep);
/* 0x004ee720: a cliff drag to tile (x, y): after the base tool, moves to
   a new cell along one axis only (a move inside the previous cell's
   column or row snaps to it), erases with `erase`, starts a line on the
   first cell, and otherwise joins the previous and new cells. Returns 0
   when nothing changed. 0x004ee8e0: a press outside the remembered cell
   resets the drag state, then the base tool runs. */
int aoe_cliff_drag_ror(AoeCliffTableRor *table, const AoeShoreMapRor *map,
                       const AoeCliffOpsRor *ops, int32_t x, int32_t y,
                       int32_t a, uint8_t erase);
int aoe_cliff_press_ror(AoeCliffTableRor *table, const AoeCliffOpsRor *ops,
                        int32_t x, int32_t y, int32_t a, int32_t b,
                        int32_t c, int32_t d);


/* RoR map fields past the base record (map +0x8dd4, +0x8dd8, +0x8de0). */
typedef struct {
    int32_t v8dd4, v8dd8, v8de0;
} AoeMapExtraRor;

/* 0x004ed860 (map vtable +0x24, the base's empty 0x00446540): stores
   `a` at +0x8dd4, `b` at +0x8dd8 and `d` at +0x8de0; `c` goes to the
   empty 0x004ed890 and is dropped. */
void aoe_map_extra_set_ror(AoeMapExtraRor *m, int32_t a, int32_t b, int32_t c, int32_t d);

#endif
