#ifndef VITA_AOE_RANDOM_MAP_H
#define VITA_AOE_RANDOM_MAP_H

#include <stdint.h>

#include "aoe_rng.h"

/* Patched Rise of Rome 1.0a random-map generator (0x0046bd00..0x00473500).
   See docs/RANDOM_MAP.md for the class map. The generator works on a tile
   byte per map cell: terrain in the low five bits, elevation in the high
   three (tile +0x05 in the original). Arithmetic follows the original's
   x87 code with 53-bit precision control: intermediates are doubles,
   rounded to float wherever the original stores a float. rand() is the
   CRT generator (AoeRng); every call the original makes, including the
   ones that only feed log lines, is kept so the sequence matches. */

typedef struct AoeRmNodeRor {
    int32_t x, y;                    /* +0x00, +0x04 */
    uint32_t value;                  /* +0x08: an int, or a float's bits */
    float priority;                  /* +0x0c */
    struct AoeRmNodeRor *next, *prev;/* +0x10, +0x14 */
} AoeRmNodeRor;

/* Map hooks for the original RGE_Map calls the modules make. */
typedef struct AoeRmMapRor AoeRmMapRor;
struct AoeRmMapRor {
    int32_t width, height;           /* map +0x08, +0x0c */
    uint8_t *tiles;                  /* row-major tile +0x05 bytes */
    void *context;
    /* Map vtable +0x10: terrain along a line (x0, y0)-(x1, y1). */
    void (*paint_line)(void *context, int32_t x0, int32_t y0, int32_t x1,
                       int32_t y1, int32_t a, int32_t b);
    /* Map vtable +0x20. */
    void (*vt20)(void *context, int32_t x0, int32_t y0, int32_t x1,
                 int32_t y1, int32_t a, int32_t b);
    /* 0x00448270(a, b, x0, y0, x1, y1, terrain, c, d): terrain over a
       rectangle. */
    void (*set_terrain)(void *context, int32_t a, int32_t b, int32_t x0,
                        int32_t y0, int32_t x1, int32_t y1, uint8_t terrain,
                        int32_t c, int32_t d);
    /* 0x00445570(x0, y0, x1, y1, value): elevation clean-up. */
    void (*fix_elevation)(void *context, int32_t x0, int32_t y0, int32_t x1,
                          int32_t y1, int32_t value);
    /* 0x00445b50(x0, y0, x1, y1, terrain): terrain clean-up. */
    void (*fix_terrain)(void *context, int32_t x0, int32_t y0, int32_t x1,
                        int32_t y1, uint8_t terrain);
    /* Map +0xba + terrain * 0x198: the terrain's alias byte. */
    uint8_t (*terrain_alias)(void *context, uint8_t terrain);
    /* The map zone manager (map +0x8dc4): 0x00521a30 / 0x00521c60 the
       zone map for a passability table, 0x00521620 the zone at a tile. */
    void *(*zones)(void *context, const float *table, int32_t count);
    uint8_t (*zone_at)(void *context, void *zones, int32_t x, int32_t y);
};

struct AoeRmModuleRor;
typedef struct AoeRmChildRor {
    struct AoeRmModuleRor *module;
    struct AoeRmChildRor *next;
} AoeRmChildRor;

typedef struct {
    int (*run)(struct AoeRmModuleRor *);       /* slot +0x00 */
    void (*destroy)(struct AoeRmModuleRor *);
} AoeRmModuleVtblRor;

/* Module base (vtable 0x00545cc8): a node in the module tree with the map,
   a priority, and work grids shared with its parent (a byte per tile and
   a search node per tile). */
typedef struct AoeRmModuleRor {
    const AoeRmModuleVtblRor *vt;
    struct AoeRmModuleRor *parent;   /* +0x04 */
    AoeRmChildRor *children;         /* +0x08 */
    AoeRmMapRor *map;                /* +0x0c (+0x10 tile rows) */
    int32_t width, height;           /* +0x14, +0x18 */
    float priority;                  /* +0x1c */
    uint8_t *grid;                   /* +0x20 */
    uint8_t **grid_rows;             /* +0x24 */
    AoeRmNodeRor *nodes;             /* +0x28 */
    AoeRmNodeRor **node_rows;        /* +0x2c */
    AoeRng *rng;
    int owns_grids;
} AoeRmModuleRor;

/* 0x0046bd00: under a parent the module joins its child list and shares
   its grids; a root allocates them (0x0046bfc0) when asked. 0x0046bdb0
   destroys the children, then frees the grids (root) or leaves the
   parent (0x0046bf60). */
void aoe_rm_module_init_ror(AoeRmModuleRor *m, const AoeRmModuleVtblRor *vt,
                            AoeRmMapRor *map, AoeRmModuleRor *parent,
                            int allocate, AoeRng *rng);
void aoe_rm_module_release_ror(AoeRmModuleRor *m);
/* 0x0046bf20 add / 0x0046bf60 remove a child. */
int aoe_rm_module_add_child_ror(AoeRmModuleRor *m, AoeRmModuleRor *child);
int aoe_rm_module_remove_child_ror(AoeRmModuleRor *m, AoeRmModuleRor *child);
/* 0x0046be20 priority; 0x0046be30: sort the children by priority
   (ascending, stable), draw rand() for the start log line, run each, and
   draw rand() again after each that succeeds. 0 if one fails. */
float aoe_rm_module_priority_ror(const AoeRmModuleRor *m);
int aoe_rm_module_run_children_ror(AoeRmModuleRor *m);
/* 0x0046bfc0 allocate, 0x0046c0a0 free, 0x0046c170 share the grids;
   0x0046c120 / 0x0046c100 set the map here and below. */
int aoe_rm_module_alloc_grids_ror(AoeRmModuleRor *m);
void aoe_rm_module_free_grids_ror(AoeRmModuleRor *m);
void aoe_rm_module_set_map_ror(AoeRmModuleRor *m, AoeRmMapRor *map);
/* Tile byte access (tile +0x05). */
uint8_t *aoe_rm_tile_ror(const AoeRmModuleRor *m, int32_t x, int32_t y);

/* Node lists. 0x0046c1b0 init a head, 0x0046c1e0 detach it from its
   nodes, 0x0046c2d0 link a node after another, 0x0046c310 unlink,
   0x0046c200 insert the node of (x, y) ordered by priority (before equal
   ones), 0x0046c290 pop the first, 0x0046c340 unlink
   every grid node, 0x0046c390 shuffle (two passes through 100 random
   buckets, gathered back in pairs). */
void aoe_rm_list_init_ror(AoeRmNodeRor *head);
void aoe_rm_list_detach_ror(AoeRmNodeRor *head);
void aoe_rm_node_link_ror(AoeRmNodeRor *after, AoeRmNodeRor *node);
void aoe_rm_node_unlink_ror(AoeRmNodeRor *node);
void aoe_rm_list_push_ror(AoeRmModuleRor *m, AoeRmNodeRor *head, int32_t x,
                          int32_t y, uint32_t value, float priority);
AoeRmNodeRor *aoe_rm_list_pop_ror(AoeRmNodeRor *head, int32_t *x, int32_t *y,
                                  uint32_t *value);
void aoe_rm_nodes_unlink_all_ror(AoeRmModuleRor *m);
void aoe_rm_list_shuffle_ror(AoeRmModuleRor *m, AoeRmNodeRor *head);
uint32_t aoe_rm_float_bits(float f);
float aoe_rm_bits_float(uint32_t bits);
/* rand() * n / 0x7fff, the original's random scaling. */
int32_t aoe_rm_rand_scaled_ror(AoeRmModuleRor *m, int32_t n);

/* 0x0046c4c0 (0x0046c470 with a local open list): best-first search from
   (x, y) to (*goal_x, *goal_y) over tiles whose terrain cost
   (costs[terrain]) is positive. The byte grid records how each tile was
   reached (4..11: from +x, -x, +y, -y and the four diagonals); 1 marks
   blocked tiles, 2 the goal, 3 the start. Diagonals cost 1.36x and need
   both adjacent straight steps open. 1 when the goal is reached. */
int aoe_rm_search_ror(AoeRmModuleRor *m, AoeRmNodeRor *open, int32_t x,
                      int32_t y, int32_t *goal_x, int32_t *goal_y,
                      const float *costs);
int aoe_rm_find_path_ror(AoeRmModuleRor *m, int32_t x, int32_t y,
                         int32_t *goal_x, int32_t *goal_y,
                         const float *costs);





/* ---- Map records (the DAT / text tables, 0x48 bytes per map). ---- */

typedef struct {               /* 0x2c bytes */
    int32_t id;                /* +0x00: copied to the object lands */
    uint8_t terrain;           /* +0x04 */
    int32_t f08;               /* +0x08: land work +0x50 */
    int32_t radius;            /* +0x0c: the land's starting square */
    uint8_t player;            /* +0x10: grid id (land work +0x54) */
    uint8_t placement;         /* +0x11: 0 random, 1 fixed, 2 spaced */
    int32_t fixed_x, fixed_y;  /* +0x14, +0x18 */
    uint8_t area_percent;      /* +0x1c */
    uint8_t per_player;        /* +0x1d: 0 one land, 1/other one per player */
    int32_t zone_radius;       /* +0x20: < 0 no terrain/elevation zone */
    int32_t zone_weight;       /* +0x24 */
    int32_t f28;               /* +0x28: growth weight (text default 8) */
} AoeRmLandRecRor;

typedef struct {               /* 0x18 bytes */
    int32_t percent;           /* +0x00: of the land area */
    int32_t terrain;           /* +0x04 */
    int32_t clumps;            /* +0x08 */
    int32_t f0c, f10, f14;     /* +0x14: text default 20 */
} AoeRmTerrainRecRor;

typedef struct {               /* 0x2c bytes */
    int32_t unit;              /* +0x00 */
    int32_t f04;
    uint8_t f08, f09;
    int32_t f0c, f10, f14, f18, f1c;
    int32_t land;              /* +0x20: -1 any, -2 by map size */
    int32_t f24, f28;
} AoeRmObjectRecRor;

typedef struct {
    int32_t v[6];              /* 0x18 bytes, never read by the generator */
} AoeRmElevationRecRor;

typedef struct {               /* 0x48 bytes */
    int32_t id;                /* +0x00 */
    int32_t margin[4];         /* +0x04..+0x10 */
    int32_t f14;               /* +0x14: low byte to land work +0x6c */
    int32_t edge;              /* +0x18: placement edge distance */
    int32_t f1c;               /* +0x1c: low byte to controller +0x145c */
    int32_t land_percent;      /* +0x20 */
    int32_t f24;               /* +0x24 */
    int32_t land_count;        /* +0x28 */
    uint32_t land_ptr;         /* +0x2c pointer placeholder */
    int32_t terrain_count;     /* +0x30 */
    uint32_t terrain_ptr;
    int32_t object_count;      /* +0x38 */
    uint32_t object_ptr;
    int32_t elevation_count;   /* +0x40 */
    uint32_t elevation_ptr;
    AoeRmLandRecRor *lands;
    AoeRmTerrainRecRor *terrains;
    AoeRmObjectRecRor *objects;
    AoeRmElevationRecRor *elevations;
} AoeRmMapRecRor;

/* ---- Controller work tables (copied into the modules). ---- */

typedef struct {               /* 0x34 bytes, controller +0x3c */
    int32_t area;              /* +0x00 */
    int32_t terrain;           /* +0x04 */
    int32_t x, y;              /* +0x08, +0x0c */
    int32_t radius;            /* +0x10 (land +0x0c) */
    int32_t f14;               /* +0x14 (land +0x08) */
    uint8_t id;                /* +0x18: grid id / player */
    int32_t weight;            /* +0x1c (land +0x28) */
    int32_t bounds[4];         /* +0x20: x0, y0, x1, y1 */
    uint8_t avoid;             /* +0x30 (header +0x14) */
} AoeRmLandWorkRor;

typedef struct {               /* 0x20 bytes, controller +0x1474 */
    int32_t tiles, f04, terrain, clumps, f10, f14, f18, flag;
} AoeRmTerrainWorkRor;

typedef struct {               /* 16 bytes */
    int32_t x, y, radius, weight;
} AoeRmZoneRor;

typedef struct {               /* 0x30 bytes, controller +0x270c */
    AoeRmObjectRecRor rec;
    int32_t flag;              /* +0x2c */
} AoeRmObjectWorkRor;

typedef struct {               /* 16 bytes, controller +0x39a0 */
    int32_t x, y, id, player;
} AoeRmObjectLandRor;

typedef struct {               /* 0x18 bytes, controller +0x3fd4 */
    int32_t tiles, height, f08, f0c, f10, f14;
} AoeRmLayerRor;

enum { AOE_RM_SLOTS = 99 };

typedef struct {
    void *context;
    /* Unit type `unit` of `player` (player +0x24): its size (+0x30,
       +0x34); 0 when the player has no such type. */
    int (*unit_size)(void *context, int32_t player, int32_t unit, float *sx,
                     float *sy);
    /* Unit type vtable +0x20(player, x, y, 0, 0, 1, 1, 1, flag, 1):
       nonzero when the spot is refused. */
    int (*blocked)(void *context, int32_t player, int32_t unit, float x,
                   float y, int32_t flag);
    /* Player vtable +0x94(unit, x, y, 0, 1). */
    void (*create)(void *context, int32_t player, int32_t unit, float x,
                   float y);
    /* Unit type vtable +0x24(&x, &y, world, 1). */
    void (*adjust)(void *context, int32_t player, int32_t unit, float *x,
                   float *y);
    /* 0x00521b50 with the unit's terrain restriction (+0x66): its zone
       map; zone_at reads it (0x00521620). */
    void *(*unit_zones)(void *context, int32_t player, int32_t unit);
    uint8_t (*zone_at)(void *context, void *zones, int32_t x, int32_t y);
    /* 0x00448270(gaia, world, 0, 0, w - 1, h - 1, 1, 0, 0) and the zone
       manager reset 0x00521c80. */
    void (*clear_all)(void *context);
    void (*zones_reset)(void *context);
} AoeRmWorldOpsRor;

typedef struct {
    void *context;
    /* Player (1-based) stance toward another (0x0045da60): 0 is allied. */
    uint8_t (*stance)(void *context, int32_t player, int32_t other);
    /* Game +0xae0 (0x005054c0). */
    int32_t (*game_mode)(void *context);
} AoeRmGameOpsRor;

typedef struct {
    AoeRmModuleRor base;
    int32_t object_arg;                        /* +0x30 */
    int32_t map_index;                         /* +0x34 */
    int32_t players;                           /* +0x38 */
    AoeRmLandWorkRor lands[AOE_RM_SLOTS];      /* +0x3c */
    int32_t land_count;                        /* +0x1458 */
    uint8_t f145c;                             /* +0x145c */
    int32_t bounds[4];                         /* +0x1460 */
    uint8_t avoid;                             /* +0x1470 */
    AoeRmTerrainWorkRor terrains[AOE_RM_SLOTS];/* +0x1474 */
    int32_t terrain_count;                     /* +0x20d4 */
    AoeRmZoneRor terrain_zones[AOE_RM_SLOTS];  /* +0x20d8 */
    int32_t terrain_zone_count;                /* +0x2708 */
    AoeRmObjectWorkRor objects[AOE_RM_SLOTS];  /* +0x270c */
    int32_t object_count;                      /* +0x399c */
    AoeRmObjectLandRor object_lands[AOE_RM_SLOTS]; /* +0x39a0 */
    int32_t object_land_count;                 /* +0x3fd0 */
    AoeRmLayerRor layers[AOE_RM_SLOTS];        /* +0x3fd4 */
    int32_t layer_count;                       /* +0x491c */
    AoeRmZoneRor elevation_zones[AOE_RM_SLOTS];/* +0x4920 */
    int32_t elevation_zone_count;              /* +0x4f50 */
    int32_t map_count;                         /* +0x4f54 */
    uint32_t maps_ptr;
    AoeRmMapRecRor *maps;                      /* +0x4f58 */
    int ror;                                   /* vtable 0x00545d18 */
    const AoeRmGameOpsRor *game;
    const AoeRmWorldOpsRor *world;             /* the +0x30 world's calls */
} AoeRmControllerRor;

#include "player_outcome.h" /* AoeOutcomeStream */

/* 0x0046da10 load from the DAT stream (RoR controller 0x00471d20): count
   and pointer, the 0x48-byte headers, then per map header +0x04..+0x2f
   again (only +0x00 keeps the first pass) and each table with its count
   and pointer pair. 0x0046dcc0 writes the same layout. 0x0046dbe0 frees. */
int aoe_rm_controller_load_ror(AoeRmControllerRor *c, AoeOutcomeStream *in,
                               int ror, AoeRng *rng,
                               const AoeRmGameOpsRor *game);
int aoe_rm_controller_save_ror(const AoeRmControllerRor *c,
                               AoeOutcomeStream *out);
void aoe_rm_controller_release_ror(AoeRmControllerRor *c);
/* 0x0046d580 (RoR 0x00471cf0): the four text tables (fscanf "%d"). The
   header file lists the maps' IDs; the land file gives per listed map
   its land count and header +0x04..+0x20, then 12 values per land (land
   +0x28 = 8); the terrain file 5 values per terrain (+0x14 = 20); the
   object file 12 values per object. Missing files leave tables empty. */
int aoe_rm_controller_load_text_ror(AoeRmControllerRor *c,
                                    const char *lands, const char *terrains,
                                    const char *objects, const char *header,
                                    int ror, AoeRng *rng,
                                    const AoeRmGameOpsRor *game);
/* 0x0046dff0: positions for `count` lands from `first` (see the source:
   mode 0 random inside the edge, 1 fixed, 2 random outside a central box
   and apart from the earlier lands, moving the previous one when it is
   the one too close). */
void aoe_rm_place_lands_ror(AoeRmControllerRor *c, int32_t first,
                            int32_t edge, int32_t count,
                            const AoeRmLandRecRor *land);
/* Setup slots: 0x0046e3a0 lands (with `teams`, allied players' lands are
   regrouped), 0x0046ead0 terrains, 0x0046ec90 objects, 0x0046ede0
   elevation layers; RoR wrappers 0x00471f60, 0x00472370, 0x00472520,
   0x00472c20 adjust them per map index and size. */
void aoe_rm_setup_lands_ror(AoeRmControllerRor *c, int teams);
void aoe_rm_setup_terrains_ror(AoeRmControllerRor *c);
void aoe_rm_setup_objects_ror(AoeRmControllerRor *c);
void aoe_rm_setup_elevation_ror(AoeRmControllerRor *c);
void aoe_rm_ror_setup_lands_ror(AoeRmControllerRor *c, int teams);
void aoe_rm_ror_setup_terrains_ror(AoeRmControllerRor *c);
void aoe_rm_ror_setup_objects_ror(AoeRmControllerRor *c);
void aoe_rm_ror_setup_elevation_ror(AoeRmControllerRor *c);
/* 0x0046df30: store the object argument, map index (low byte) and player
   count, take the map, allocate the grids and run the four setup slots.
   0x0046df90 cleanup (the modules go, the grids are freed; RoR
   0x00471f50 is the same). */
/* 0x00472cb0: the RoR shallows module over the lands' positions
   (aoe_rm_shallows_create_ror builds module 0x004716d0). */
int aoe_rm_shallows_create_ror(AoeRmControllerRor *c);
void aoe_rm_ror_shallows_ror(AoeRmControllerRor *c);
int aoe_rm_controller_setup_ror(AoeRmControllerRor *c, AoeRmMapRor *map,
                                int32_t object_arg, int32_t map_index,
                                int32_t players, int teams);
void aoe_rm_controller_cleanup_ror(AoeRmControllerRor *c);

/* ---- Generator modules ---- */

/* Path module (vtable 0x00545cd8, 0x0046cdb0..0x0046d490, priority 1.75):
   on a grid of 3x3 blocks, random walks over blocks of plain terrain
   (0 or 6) at one elevation, painted with map vtable +0x10 / +0x20
   (terrain 0x10, cleared by the RoR generator afterwards). */
typedef struct {
    AoeRmModuleRor base;
    int32_t count;             /* +0x30 walks */
    int32_t f34;
    int32_t length, variance;  /* +0x38, +0x3c */
    int32_t water_clear;       /* +0x40 */
    int32_t path_clear;        /* +0x44 */
    int32_t zone_x[AOE_RM_SLOTS], zone_y[AOE_RM_SLOTS], zone_r[AOE_RM_SLOTS]; /* +0x48 */
    int32_t zone_count;        /* +0x4ec */
    AoeRmNodeRor open;         /* +0x4f0 */
} AoeRmPathModuleRor;

/* 0x0046cdb0 constructor; slot 0 0x0046cdf0 runs the setup, draws one
   rand() and walks `count` times. */
AoeRmPathModuleRor *aoe_rm_path_module_create_ror(AoeRmControllerRor *c,
                                                   int32_t count,
                                                   int32_t length,
                                                   int32_t variance);
/* 0x0046ce20 block setup, 0x0046d0e0 one walk, 0x0046d3b0 one step
   (direction 0 up, 1 right, 2 down, 3 left, folded the original's way),
   0x0046d490 clear blocks (the bottom row clamps against the right edge
   test, as in the original). */
void aoe_rm_path_setup_ror(AoeRmPathModuleRor *p);
void aoe_rm_path_walk_ror(AoeRmPathModuleRor *p);
int aoe_rm_path_step_ror(AoeRmPathModuleRor *p, int32_t dir, int32_t *x,
                         int32_t *y, uint32_t value);
void aoe_rm_path_clear_ror(AoeRmPathModuleRor *p, int32_t x, int32_t y,
                           int32_t r);

/* Shallows module (vtable 0x00545d14, 0x004716d0..0x00471860, priority
   1.1): joins every pair of land positions, then two random pairs of dry
   tiles, by a search with water costs (grass 1, water 7, beach 1,
   shallows 2, deep water 14) and turns the water crossed into shallows. */
typedef struct {
    AoeRmModuleRor base;
    int32_t xy[AOE_RM_SLOTS][2];/* +0x30 */
    int32_t count;              /* +0x348 */
} AoeRmShallowsModuleRor;

/* 0x00471710 run (every pair, then two random dry pairs), 0x00471860
   connect: search, then walk the path back widening the crossed water. */
void aoe_rm_shallows_connect_ror(AoeRmShallowsModuleRor *s, int32_t a,
                                 int32_t b);

/* Elevation module (vtable 0x00545d00, 0x0046f030..0x0046f620, priority
   1.5): raises hills layer by layer from random seeds. */
typedef struct {
    AoeRmModuleRor base;
    AoeRmLayerRor layers[AOE_RM_SLOTS];       /* +0x30 */
    int32_t layer_count;                      /* +0x978 */
    AoeRmZoneRor zones[AOE_RM_SLOTS];         /* +0x97c */
    int32_t zone_count;                       /* +0xfac */
} AoeRmElevationModuleRor;

/* 0x0046f130 all tiles within r are terrain `terrain` at elevation >=
   `min`; 0x0046f210 the same test, then the tiles at `height` in the 5x5
   around (x, y); 0x0046f3b0 the zone mask (sum of weight * (radius -
   distance), capped at 101); 0x0046f4d0 the seed list; 0x0046f5a0 unlink
   seeds (rows y - r up to the clamped x + r); 0x0046f620 grow a layer. */
int aoe_rm_elev_check_ror(AoeRmModuleRor *m, int32_t x, int32_t y,
                          uint8_t terrain, uint8_t min, int32_t r);
int aoe_rm_elev_count_ror(AoeRmModuleRor *m, uint8_t height, int32_t x,
                          int32_t y, int32_t r, uint8_t terrain, uint8_t min);
void aoe_rm_zone_mask_ror(AoeRmModuleRor *m, const AoeRmZoneRor *zones,
                          int32_t count);
int aoe_rm_elev_grow_ror(AoeRmElevationModuleRor *e, const AoeRmLayerRor *l);
/* 0x0046f030 constructor; 0x0046f070 run: base elevation, the zone mask,
   then each layer and a map elevation clean-up (0x00445570). Seeds score
   250 - 15 * count (0x0046f4b0). */
AoeRmElevationModuleRor *aoe_rm_elevation_module_create_ror(AoeRmControllerRor *c);

/* Land module (vtable 0x00545d04, 0x0046fae0..0x0046fe00, priority
   1.0): paints the base terrain, then grows each land from its starting
   square. Grid cells hold the land id, the land count for free tiles and
   0xff for tiles refused by the bounds penalty. */
typedef struct {
    AoeRmModuleRor base;
    AoeRmLandWorkRor lands[AOE_RM_SLOTS];     /* +0x30 */
    int32_t land_count;                       /* +0x144c */
    uint8_t base_terrain;                     /* +0x1450 */
    int32_t bounds[4];                        /* +0x1454 */
    uint8_t avoid;                            /* +0x1464 */
} AoeRmLandModuleRor;

/* 0x0046fae0 constructor; 0x0046fb20 run: fill the grid with the land
   count, paint the base terrain, grow, clean up (0x00445b50). */
AoeRmLandModuleRor *aoe_rm_land_module_create_ror(AoeRmControllerRor *c);
/* 0x0046fbb0: tiles of this land's id in the 5x5 around (x, y), 0 when
   another land is within the land's +0x14 square or the tile is taken;
   0x0046fd60: the bounds penalty (0..99, or 101); 0x0046fe00 the growth. */
int aoe_rm_land_score_ror(AoeRmLandModuleRor *l, int32_t land, int32_t x,
                          int32_t y);
int aoe_rm_land_penalty_ror(AoeRmLandModuleRor *l, int32_t x, int32_t y,
                            int32_t land);
void aoe_rm_land_grow_ror(AoeRmLandModuleRor *l);

/* Terrain module (vtable 0x00545d4c, 0x00472df0..0x00473500, priority
   2.0): terrain clumps grown over a base terrain, then beaches. */
typedef struct {
    AoeRmModuleRor base;
    AoeRmTerrainWorkRor terrains[AOE_RM_SLOTS]; /* +0x30 */
    int32_t terrain_count;                       /* +0xc90 */
    AoeRmZoneRor zones[AOE_RM_SLOTS];            /* +0xc94 */
    int32_t zone_count;                          /* +0x12c4 */
    void *zone_map;                              /* +0x12c8 */
} AoeRmTerrainModuleRor;

/* 0x00472df0 constructor; 0x00472e30 run: zone mask (0x00473260, as
   0x0046f3b0), the zone map for a 0..98 table, each entry, beaches. The
   seed list (0x00473380) attaches its head and picks (w * h) / 8 nodes;
   0x00473460 unlinks a clamped square; 0x00473360 scores 250 - count *
   weight. */
AoeRmTerrainModuleRor *aoe_rm_terrain_module_create_ror(AoeRmControllerRor *c);
/* 0x00472ee0 water (1, 4, 22); 0x00472f00 beaches (2) next to water;
   0x004730a0 1 + tiles of `terrain` in the 5x5, or 0 when a tile within
   r is not the base, its alias or `terrain`; 0x00473500 grow one entry. */
int aoe_rm_is_water_ror(uint8_t terrain);
void aoe_rm_beaches_ror(AoeRmModuleRor *m);
int aoe_rm_terrain_check_ror(AoeRmModuleRor *m, uint8_t terrain, int32_t x,
                             int32_t y, int32_t r, uint8_t base);
int aoe_rm_terrain_grow_ror(AoeRmTerrainModuleRor *t,
                            const AoeRmTerrainWorkRor *e);

/* Object module (vtable 0x00545d10, 0x004704d0..0x00471350, priority
   3.0): places each object record's units on random free tiles, near a
   land, or in clusters. */
typedef struct {
    AoeRmModuleRor base;
    const AoeRmWorldOpsRor *world;            /* +0x30 */
    AoeRmObjectWorkRor objects[AOE_RM_SLOTS]; /* +0x34 */
    int32_t object_count;                     /* +0x12c4 */
    AoeRmObjectLandRor lands[AOE_RM_SLOTS];   /* +0x12c8 */
    int32_t land_count;                       /* +0x18f8 */
    int32_t counters[8];                      /* +0x18fc */
    uint8_t clear;                            /* +0x191c */
} AoeRmObjectModuleRor;

/* 0x004704d0 constructor; 0x00470640 run: clear the world when asked,
   place every record, reset the zone manager. */
AoeRmObjectModuleRor *aoe_rm_object_module_create_ror(AoeRmControllerRor *c,
                                                       int with_work,
                                                       uint8_t clear);
/* 0x00470530: append a record (unit, count, flag) with the defaults. */
void aoe_rm_object_add_ror(AoeRmObjectModuleRor *o, int32_t unit,
                           int32_t count, int32_t flag);
/* 0x00470790: chain the grid's open tiles in the square (all the map
   when x, y or r is negative) after `head`, then put (w-1)(h-1)/4 random
   open ones in front; 0x00470950 fill a square of the grid. */
void aoe_rm_object_gather_ror(AoeRmObjectModuleRor *o, AoeRmNodeRor *head,
                              int32_t x, int32_t y, int32_t r);
void aoe_rm_object_fill_ror(AoeRmObjectModuleRor *o, int32_t x, int32_t y,
                            int32_t r, uint8_t value);
/* 0x004706d0 one record: scaled counts (+0x09), then 0x00470c10
   anywhere (land -1), 0x004709d0 away from the lands (-2), or 0x00470e20
   around each land with that id; clusters 0x00471180 / 0x00471350. */
void aoe_rm_object_place_ror(AoeRmObjectModuleRor *o, AoeRmObjectWorkRor *e);

/* Base generate 0x0046de20 (terrain, land, object and elevation modules,
   then 0x0046be30) and the RoR generate 0x00472d80 (then terrain 0x10
   becomes 0). */
int aoe_rm_generate_ror(AoeRmControllerRor *c);
/* 0x00471d40 RoR setup: the base setup, then (rand() >= 0x2000) a path
   module sized by the map width and adjusted per map index. */
int aoe_rm_ror_setup_ror(AoeRmControllerRor *c, AoeRmMapRor *map,
                         int32_t object_arg, int32_t map_index,
                         int32_t players, int teams);

#endif
