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

#endif
