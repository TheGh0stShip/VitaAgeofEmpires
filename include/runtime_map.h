#ifndef VITA_AOE_RUNTIME_MAP_H
#define VITA_AOE_RUNTIME_MAP_H

#include "dat.h"
#include "object_update_list.h"
#include "player_vision_state.h"
#include "scn.h"

/* Semantic, owned form of the fields initialized by RoR 0x004453c0 and
   filled by scenario reader 0x00445aa0. Arrays use runtime row[y][x]
   order, but this does not claim the original 0x18-byte PC tile ABI. */
typedef struct {
    uint32_t width, height;
    ScnTile *tiles;                  /* packed +5 terrain/elevation split */
    uint8_t *shapes;                /* tile +4 */
    uint8_t *borders;               /* tile +6 */
    int32_t *projected_x, *projected_y; /* tile +0/+2, widened */
    uint8_t *view_codes;            /* tile +9 */
    uint8_t *cache_flags;           /* tile +7 invalidation byte */
    uint8_t *draw_flags;            /* tile +10 */
    AoeObjectUpdateList *objects;   /* tile +0x0c list */
    AoeVisibilityMaskMap masks;     /* map +0x8dc8 */
} AoeRuntimeMap;

/* Requires the original reader's verified square scenario order and parsed
   DAT geometry/terrain/border tables. The caller tears down live units
   before destroying this map. */
int aoe_runtime_map_from_scenario(AoeRuntimeMap *out,
                                   const ScnMap *scenario,
                                   const DatMapGeometry *geometry,
                                   const DatTerrains *terrains,
                                   const DatBorders *borders);
void aoe_runtime_map_destroy(AoeRuntimeMap *map);

/* Connects an owned player vision grid to this runtime map's masks and
   active-player view bytes. The original world changes active player at
   runtime; callers must repeat this binding when it changes. */
int aoe_runtime_map_bind_player_vision(AoeRuntimeMap *map,
                                       AoePlayerVisionState *player,
                                       int is_active_player);

/* RoR 00445760: normalize and clamp an inclusive rectangle, mark tile +7
   with 0xcc, and write the requested tile +10 draw flag. Coordinates are
   signed runtime values; this semantic map does not expose the packed PC
   tile layout. A wholly out-of-map rectangle succeeds with no effect. */
int aoe_runtime_map_invalidate_ror(AoeRuntimeMap *map,
                                   int32_t x0, int32_t y0,
                                   int32_t x1, int32_t y1,
                                   uint8_t draw_flag);

typedef struct {
    void *context;
    /* 004481b0, invoked once for every admitted cell after its terrain byte
       is written. `filter_terrain` is completion's final zero argument.
       Return zero if a required dynamic removal or creation failed. */
    int (*transition_cell)(void *context, uint32_t x, uint32_t y,
                           uint8_t old_terrain,
                           int16_t filter_terrain);
} AoeRuntimeTerrainPaintOps;

/* Deterministic map portion of RoR 00449110 for its write-enabled path.
   Bounds are narrowed to signed 16-bit as in the executable, then normalized
   and clipped. Admitted cells require a positive target-terrain frame count
   for their existing elevation shape. The mandatory transition hook keeps
   004481b0 object removal/spawning explicit. Borders and invalidation are
   recomputed over the one-cell-expanded painted rectangle. A failed hook is
   propagated after the admitted terrain write, before border/invalidation. */
int aoe_runtime_map_paint_terrain_ror(
    AoeRuntimeMap *map, const DatTerrains *terrains,
    const DatBorders *borders, int32_t x0, int32_t y0,
    int32_t x1, int32_t y1, uint8_t terrain_id,
    int16_t filter_terrain, const AoeRuntimeTerrainPaintOps *ops);

#endif
