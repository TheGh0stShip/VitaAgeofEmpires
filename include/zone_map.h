#ifndef VITA_AOE_ZONE_MAP_H
#define VITA_AOE_ZONE_MAP_H

#include <stddef.h>
#include <stdint.h>

/* Patched Rise of Rome 1.0a terrain-restriction zones (world map
   +0x8dc4 manager). A zone map gives every tile a byte zone: tiles of the
   same passability under one terrain-restriction table (passable: entry
   > 0 for the tile's terrain, low 5 bits) that touch along edges share a
   zone. Zones are numbered in scan order and wrap after 255
   (0x00521210 / 0x00521330). Maps here are square in the original; rows
   are y, columns x. */
typedef struct {
    int32_t width, height;
    uint8_t *zones;          /* +0x404, rows at +0x408 */
    int32_t counts[255];     /* +0x000: tile count per zone, 0 = not yet */
    float *table;            /* +0x510 copy of the restriction row */
    int32_t table_count;     /* +0x514 */
} AoeZoneMapRor;

/* 0x00521060: copy the table and flood-fill `terrain` (row-major terrain
   bytes, low 5 bits used). Returns 0 on allocation failure. */
int aoe_zone_map_build_ror(AoeZoneMapRor *map, int32_t width, int32_t height,
                           const uint8_t *terrain, const float *table,
                           int32_t table_count);
void aoe_zone_map_free_ror(AoeZoneMapRor *map);
/* 0x00521620: the zone at a tile (caller bounds the tile). */
uint8_t aoe_zone_at_ror(const AoeZoneMapRor *map, int32_t x, int32_t y);
/* 0x00521680: a tile of `zone` within Euclidean distance `distance` of
   (x, y), searched over the square of half-side ftol(distance) clamped to
   the map. */
int aoe_zone_near_ror(const AoeZoneMapRor *map, uint8_t zone, int32_t x,
                      int32_t y, float distance);
/* 0x00521770: the tile count of a zone (cached once positive). */
int32_t aoe_zone_size_ror(AoeZoneMapRor *map, uint8_t zone);
/* 0x005215c0: the same passability (> 0) for every table entry. */
int aoe_zone_table_matches_ror(const AoeZoneMapRor *map, const float *table,
                               int32_t table_count);

/* The manager (0x00521b50 / 0x00521c60): one zone map per distinct
   passability table, built on first use. */
typedef struct {
    AoeZoneMapRor **maps;
    int32_t count;
} AoeZoneManagerRor;

/* 0x00521b50: the index of the map for `table`, building it (from the
   current terrain) when no map has the same passability; -1 on failure. */
int32_t aoe_zone_manager_index_ror(AoeZoneManagerRor *manager,
                                   int32_t width, int32_t height,
                                   const uint8_t *terrain, const float *table,
                                   int32_t table_count);
AoeZoneMapRor *aoe_zone_manager_map_ror(AoeZoneManagerRor *manager,
                                        int32_t index); /* 0x00521c60 */
void aoe_zone_manager_free_ror(AoeZoneManagerRor *manager);

#endif
