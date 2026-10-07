#ifndef VITA_AOE_WORLD_BASE_MAP_H
#define VITA_AOE_WORLD_BASE_MAP_H

#include <stdint.h>

#include "tile_map.h" /* AoeTileMapRor */

/* Patched Rise of Rome 1.0a base map (0x8dd0 bytes, vtable 0x005445a0):
   what its constructor 0x00443d80 sets. The world makes one from its text
   names (0x0051bf90: operator new 0x8dd0, then this with the world's
   first, third and fourth names, the three counts and the sounds); the
   RoR map's constructor 0x4ed800 runs it and then installs
   vtable 0x005499f4. The port has no vtable word: the type is the class.
   Semantic storage (host pointers stay host pointers); offsets noted. */

/* One 0x198-byte terrain record (32 of them from map +0x8c): the parts
   the constructor clears. Its used byte (+0x00), names, +0xb8 and the
   object modes (+0x176, 30 bytes) are left to the terrain text loader. */
typedef struct {
    void *shape;                /* +0x20: its art (0x00444110 frees it) */
    void *sound;                /* +0x24: the caller's sound entry (the
                                   data save 0x00446a20 writes its ID) */
    int16_t field_ba, field_bc; /* +0xba, +0xbc */
    int16_t table_be[32];       /* +0xbe: per-terrain values (16 dwords) */
    int16_t object_ids[30];     /* +0xfe: terrain object template IDs */
    int16_t object_density[30]; /* +0x13a: per mille */
    int16_t object_count;       /* +0x194 */
} AoeBaseMapTerrainRor;

/* One 0x5a0-byte border record (16 of them from map +0x338c). */
typedef struct {
    void *shape;                /* +0x20 (0x00444110 frees it) */
    void *sound;                /* +0x24 (the data save writes its ID) */
} AoeBaseMapBorderRor;

typedef struct {
    /* +0x04 the tile block and +0x8d8c its row pointers (one contiguous
       array here, `tile.tiles`), +0x08 width, +0x0c height, +0x18 the 19
       shapes, +0x8d96 tile width, +0x8d98 tile height, +0x8d9a half
       height, +0x8d9c half width, +0x8d9e elevation step. */
    AoeTileMapRor tile;
    int32_t pixel_w, pixel_h;          /* +0x10, +0x14 (0x4445e0: tile
                                          size times map size) */
    AoeBaseMapTerrainRor terrains[32]; /* +0x8c */
    AoeBaseMapBorderRor borders[16];   /* +0x338c */
    int16_t terrain_count;             /* +0x8d90 (the terrain list's) */
    int16_t field_8d94;                /* +0x8d94 */
    int16_t field_8da0, field_8da2;    /* +0x8da0, +0x8da2 */
    int16_t field_8da4, field_8da6;    /* +0x8da4, +0x8da6 */
    int16_t field_8da8, field_8daa;    /* +0x8da8, +0x8daa */
    uint8_t field_8dac;                /* +0x8dac */
    void *raw8db0;                     /* +0x8db0 (a plain allocation) */
    void *raw8db4;                     /* +0x8db4 (a plain allocation) */
    uint8_t reveal_all;                /* +0x8db8 */
    uint8_t fog_edges;                 /* +0x8db9 (1) */
    void *random_map;                  /* +0x8dbc */
    void *world;                       /* +0x8dc0: the game's world */
    void *zones;                       /* +0x8dc4 */
    void *masks;                       /* +0x8dc8 */
    void *v8dcc;                       /* +0x8dcc */
} AoeBaseMapRor;

typedef struct {
    void *context;
    /* The game's (global 0x00580e38) world, its +0x3f4. */
    void *(*game_world)(void *context);
    /* 0x00446bb0 (name, sounds): the terrain text table (terrain_art.h's
       terrain_text_load parses it; the count lands in +0x8d90). */
    void (*terrain_text)(void *context, AoeBaseMapRor *m, const char *name, void **sounds);
    /* 0x00446f80 (name, sounds): the border text table (terrain_art.h's
       border_text_load). */
    void (*border_text)(void *context, AoeBaseMapRor *m, const char *name, void **sounds);
} AoeBaseMapOpsRor;

/* 0x00443d80 (thiscall, 7 stack arguments, ret 0x1c): the base map
   constructor. In order: +0x8dc4 and +0x8dbc cleared, +0x8dc0 the game's
   world; the tile width and height with their halves (signed, toward
   zero) and the elevation step; width, height and +0x10 / +0x14 zeroed;
   the 19 shapes (0x00447a70) as 0x00444240 lays them out but with
   offsets -100 / 100 instead of -8 / 8: 0 the tile; 1, 5, 7, 9, 13 the
   elevation step (-100); 2, 6, 8, 10, 14 tile plus elevation (100); 3, 4,
   11, 12 the tile (-100); 15, 16 the tile (100); 17, 18 half width by
   tile height (0). Then the shorts +0x8da0, +0x8da2, +0x8d90, +0x8d94,
   +0x8da4..+0x8daa and the byte +0x8dac zeroed, +0x8db8 0 and +0x8db9 1;
   each terrain record's +0xba, +0xbc, +0xbe table, object IDs,
   densities, count, shape and sound zeroed, each border's shape and
   sound (from the last down); then the terrain list (`terrain_name`)
   and the border list (`border_name`) are loaded with the sounds; and
   only after them the tile block (+0x04, +0x8d8c), +0x8db0, +0x8db4,
   +0x8dc8 and +0x8dcc are cleared. `unused_name` (the third argument)
   is never read. Returns `m`. */
AoeBaseMapRor *aoe_base_map_init_ror(AoeBaseMapRor *m, const char *border_name,
                                     const char *terrain_name, const char *unused_name,
                                     int16_t tile_w, int16_t tile_h, int16_t elevation_h,
                                     void **sounds, const AoeBaseMapOpsRor *ops);

#endif
