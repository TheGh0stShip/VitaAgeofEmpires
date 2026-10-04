#ifndef VITA_AOE_SCENARIO_SCENE_H
#define VITA_AOE_SCENARIO_SCENE_H

#include "runtime_viewport.h"
#include "scenario_runtime.h"
#include "sprite_draw.h"

/* Caller-owned original art and frame surfaces. The runtime supplies the
   live map, active-player visibility masks, tile +10 flags, and object lists.
   Unit/player color and unresolved class-specific entry state remain explicit
   callbacks because they are original game state, not rendering defaults. */
typedef struct {
    const DatMapGeometry *geometry;
    const DatTerrains *terrains;
    const DatBorders *borders;
    const TerrainArtCache *terrain_art;
    const BorderArtCache *border_art;
    const TerrainViewCamera *camera;
    const TerrainViewIndexedSurface *surface;
    const TileEdgeIndex *tile_edges;
    const TileEdgeIndex *black_edges;
    TileEdgeCoveragePlane first_clip;
    TileEdgeCoveragePlane second_clip;
    int fog_enabled;
    int bypass_exploration_gate;
    AoeLiveUnitViewportResolveRor resolve_unit;
    void *resolve_unit_context;
    AoeRuntimeViewportEntryResolveRor resolve_entry;
    void *resolve_entry_context;
    const AoeLiveUnitDrawRorOps *draw_ops;
    /* Nonzero draws sprites during the tile walk instead of through the
       original deferred sprite list (diagnostics only). */
    int immediate_sprites;
} AoeScenarioSceneDrawRor;

typedef struct {
    size_t terrain_tiles;
    size_t object_tiles;
    AoeLiveUnitViewportResult units;
} AoeScenarioSceneResultRor;

/* Draws one authentic indexed scene: the recovered viewport tile walk draws
   each tile's terrain/borders and visits that tile's linked live objects,
   whose sprites are queued in the view's sprite list and drawn after all
   terrain in layer/row/queue order (see AoeSpriteDrawQueue). */
int aoe_scenario_runtime_draw_scene_ror(
    AoeScenarioRuntimeRor *runtime, const AoeScenarioSceneDrawRor *draw,
    AoeScenarioSceneResultRor *result);

#endif
