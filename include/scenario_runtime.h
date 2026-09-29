#ifndef VITA_AOE_SCENARIO_RUNTIME_H
#define VITA_AOE_SCENARIO_RUNTIME_H

#include "live_unit_factory.h"
#include "player_templates.h"
#include "runtime_map.h"
#include "tile_object_draw.h"

enum { AOE_SCENARIO_RUNTIME_PLAYERS = 9 };

typedef struct {
    AoePlayerTemplates templates;
    AoePlayerVisionState vision;
    AoePlayerResources resources;
    AoePlayerUpdateState owner;
    AoeLiveUnitVisionBinding vision_binding;
    float *resource_values;
    AoeLiveUnit *units;
    uint32_t unit_count;
    uint8_t player_index;
    uint32_t civilization_id;
} AoeScenarioRuntimePlayerRor;

/* Portable owned scenario world assembled in the original RoR load order.
   Parsed DAT graphics and scenario/DAT inputs remain caller-owned and must
   outlive the runtime. Player templates, resources, visibility, map storage,
   live units, sprite instances, links, and world ID tables are owned here.
   The runtime must be zero-initialized before its first build. */
typedef struct {
    AoeRuntimeMap map;
    AoeOwnedVisibilityRegistry world;
    AoeScenarioRuntimePlayerRor players[AOE_SCENARIO_RUNTIME_PLAYERS];
    AoeRng rng;
    AoeTileObjectDrawEntry *draw_scratch;
    size_t draw_scratch_capacity;
    AoeLiveUnit **editor_units;
    size_t editor_unit_count;
    size_t editor_unit_capacity;
    const DatGraphics *graphics;
    const DatMapGeometry *geometry;
    int32_t diplomacy[AOE_SCENARIO_RUNTIME_PLAYERS]
                     [AOE_SCENARIO_RUNTIME_PLAYERS];
    uint64_t constructed_units;
    uint64_t low_state_units;
    uint64_t high_state_units;
    int16_t active_player;
    uint8_t initialized;
} AoeScenarioRuntimeRor;

typedef struct {
    uint64_t units_visited;
    uint64_t inherited_tick_units;
    uint64_t sprite_instances;
} AoeScenarioRuntimeSpriteTickResult;

int aoe_scenario_runtime_build_ror(
    AoeScenarioRuntimeRor *runtime, const ScnMap *scenario,
    const DatCivilizations *civilizations, const DatGraphics *graphics,
    const DatTerrains *terrains, const DatBorders *borders,
    const DatMapGeometry *geometry, uint32_t rng_seed);

/* Binds one constructed player's visibility masks/view codes to the runtime
   map and updates every live-unit visibility binding with that player's
   scenario diplomacy row and distinct +0x6c explored-cell destination.
   Player indices use scenario order: 0 Gaia, 1..8 regular players. */
int aoe_scenario_runtime_set_active_player_ror(
    AoeScenarioRuntimeRor *runtime, int16_t player_index);

/* Normal scenario-editor player factory boundary (patched RoR player vtable
   +0x94 from 0x0051a2e0). The unit uses the selected player's cloned DAT
   template and the same recovered common constructor as scenario loading.
   Editor-created units are individually allocated because map/player lists
   retain their addresses. No collision policy is applied here; the original
   template +0x20 validator runs before this factory. */
int aoe_scenario_runtime_editor_create_ror(
    AoeScenarioRuntimeRor *runtime, uint8_t player_index, uint16_t object_id,
    float x, float y, float z, AoeLiveUnit **out_unit);

/* Concrete binding for building completion's template +0x18 stack-unit
   factory call. It selects the completing unit's semantic owner and creates
   the requested cloned player template at the supplied XYZ. Like 004ae500,
   the callback ignores a failed/null factory result. */
void aoe_scenario_runtime_building_spawn_stack_ror(
    void *context, AoeLiveUnit *building, int16_t unit_id,
    float x, float y, float z);

/* Runs only the recovered sprite-manager tail shared by RoR live types
   20..80, in scenario player/record order. It does not stand in for the
   still-incomplete action, movement, combat, or player update chain. */
int aoe_scenario_runtime_tick_sprites_ror(
    AoeScenarioRuntimeRor *runtime, float world_delta,
    AoeScenarioRuntimeSpriteTickResult *result);

void aoe_scenario_runtime_destroy_ror(AoeScenarioRuntimeRor *runtime);

#endif
