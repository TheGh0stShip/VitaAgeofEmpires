#ifndef VITA_AOE_SCN_H
#define VITA_AOE_SCN_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t terrain_id;
    uint8_t elevation;
    uint8_t unused;
} ScnTile;

typedef struct {
    float x, y, z;
    uint32_t spawn_id;
    uint16_t object_id;
    uint8_t state;
    float rotation;
} ScnUnit;

typedef struct {
    float food, wood, gold, stone;
} ScnPlayerResources;

typedef struct {
    char name[257]; /* Original 256-byte field, NUL-terminated on load. */
    uint32_t enabled;
    uint32_t is_human;
    uint32_t civilization_id;
    uint32_t unknown;
} ScnPlayerSetup;

typedef struct {
    uint32_t count;
    ScnUnit *units;
} ScnPlayerUnits;

typedef struct {
    uint8_t type;
    int32_t object_type, target_player;
    float x0, y0, x1, y1;
    int32_t number, count, object, target;
    uint8_t victory_group, ally_flag, state;
} ScnVictoryCondition;

typedef struct {
    char name[257];
    float camera_x, camera_y;
    int16_t camera_x2, camera_y2;
    uint8_t allied_victory;
    uint16_t diplomacy_count;
    uint8_t diplomacy[16];
    uint32_t diplomacy_values[16];
    float victory_version;
    uint32_t victory_condition_count;
    uint8_t victory;
    ScnVictoryCondition *victory_conditions;
    uint32_t point_condition_count, total_victory_points;
} ScnPlayerDetail;

typedef struct {
    uint32_t width, height;
    size_t dib_size;
    uint8_t *dib; /* BITMAPINFOHEADER, 256-color palette, padded pixel rows. */
} ScnBriefingBitmap;

typedef struct {
    char version[5];
    int32_t save_type;
    uint32_t last_save_time;
    uint32_t victory_type;
    uint32_t player_count;
    uint32_t next_unit_id;
    float player_data_version;
    char *instructions;
    ScnBriefingBitmap briefing_bitmap;
    size_t player_data_fixed_offset; /* 1.15 final fixed-array block. */
    uint32_t width;
    uint32_t height;
    ScnTile *tiles; /* Scenario order. For square maps this is runtime
                       row[y][x] order; see scn_runtime_tile_order_supported. */
    size_t map_offset; /* Offset within the decompressed scenario body. */
    ScnPlayerSetup player_setup[16];
    ScnPlayerResources resources[8];
    ScnPlayerUnits player_units[9]; /* Index 0 is the neutral/Gaia player. */
    ScnPlayerDetail player_details[8];
    /* 1.15 fixed block (scenario-info object, reader 0x005069c0 region),
       indexed by player - 1:
       - +0x1990 starting resources, four int32 per player; the getters
         0x005073e0/0x00507400/0x00507420/0x00507440 hand fields 0/1/2/3
         to resources 3 (gold), 1 (wood), 0 (food) and 2 (stone);
       - +0x4bf4 per-player tech toggles (20 int32, 16 used by 0x00507e50);
       - +0x50f4 three scenario options (0x005076a0);
       - +0x5100 starting age per player (1..4: Tool, Bronze, Iron,
         Post-Iron). */
    int32_t start_resources[16][4];
    int32_t tech_toggles[16][20];
    int32_t options[3];
    int32_t starting_age[16];
} ScnMap;

/* Loads original 1.10/1.11 scenarios with a 1.15 player-data body. */
int scn_load_map(const char *path, uint8_t terrain_count, ScnMap *map);
void scn_free_map(ScnMap *map);

/* RoR map reader 0x00445aa0 (base 0x00424cb0) consumes triples with outer
   y and inner x, writing runtime row[y][x]; runtime rows have width stride.
   Both original readers use height as the inner-loop bound too, so this
   direct scenario-to-runtime mapping is verified only for square maps. */
int scn_runtime_tile_order_supported(const ScnMap *map);
const ScnTile *scn_runtime_tile_at(const ScnMap *map,
                                    uint32_t x, uint32_t y);

#endif
