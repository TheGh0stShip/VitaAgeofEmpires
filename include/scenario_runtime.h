#ifndef VITA_AOE_SCENARIO_RUNTIME_H
#define VITA_AOE_SCENARIO_RUNTIME_H

#include "live_unit_factory.h"
#include "player_templates.h"
#include "runtime_map.h"
#include "terrain_transition.h"
#include "tile_object_draw.h"
#include "tech_tree.h"

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
    int32_t field_104; /* derived player +0x104, used by +0x78 */
    /* Player +0x224 (new-game constructor 0x004ea91c with rebuild). */
    AoeResearchCompletionManagerRor research;
} AoeScenarioRuntimePlayerRor;

/* Owned type-80 storage: the building view (+0x1c0..+0x1f8) with its +0x184
   live and +0x1cc queue action lists, bound to the unit at construction. */
typedef struct {
    AoeBuildingUpdate building;
    AoeActionList live, queue;
    AoeLiveUnit *unit;
} AoeScenarioBuildingStorageRor;

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
    const DatIntro *intro;
    const DatTerrains *terrains;
    const DatBorders *terrain_borders;
    int32_t diplomacy[AOE_SCENARIO_RUNTIME_PLAYERS]
                     [AOE_SCENARIO_RUNTIME_PLAYERS];
    uint64_t constructed_units;
    uint64_t low_state_units;
    uint64_t high_state_units;
    const DatTechs *techs;     /* tech tree (world +0x104 object) */
    const DatEffects *effects; /* world +0x4c effects */
    /* Optional world hook for upgrades: the +0x184 list's +0x14 retask. */
    void *retask_context;
    void (*retask)(void *context, AoeLiveUnit *unit,
                   const DatUnit *new_template);
    uint32_t wonder_signals, adjacency_refreshes;
    AoeScenarioBuildingStorageRor **buildings;
    size_t building_count, building_capacity;
    uint32_t queue_status_four; /* teardown sink for status-4 counts */
    uint64_t completed_buildings;
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
    const DatIntro *intro,
    const DatCivilizations *civilizations, const DatGraphics *graphics,
    const DatTerrains *terrains, const DatBorders *borders,
    const DatMapGeometry *geometry, const DatTechs *techs,
    const DatEffects *effects, uint32_t rng_seed);

/* Applies effect `effect_id` (0x004ebb10) to a player of the runtime: its
   templates, resources, research and awake (+0x28) units. */
int aoe_scenario_runtime_apply_effect_ror(AoeScenarioRuntimeRor *runtime,
                                          int16_t player_index,
                                          int16_t effect_id);

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

/* Concrete building-completion binding for map method 00445760. */
void aoe_scenario_runtime_building_invalidate_foundation_ror(
    void *context, AoeLiveUnit *building,
    int32_t x0, int32_t y0, int32_t x1, int32_t y1);

/* Owned bindings for 004481b0 object fields and its virtual +0x68 removal.
   They accept the map-list object pointer (`AoeLiveUnitCore *`) and verify it
   belongs to this runtime before accessing the enclosing semantic unit. */
int16_t aoe_scenario_runtime_terrain_object_owner_ror(
    void *context, const void *object);
int16_t aoe_scenario_runtime_terrain_object_template_ror(
    void *context, const void *object);
int aoe_scenario_runtime_terrain_remove_object_ror(
    void *context, void *object);

/* Builds the complete owned callback set consumed by
   aoe_terrain_transition_cell_ror. Placement uses 004481b0's exact template
   +0x20 argument set; creation uses Gaia's cloned template +0x18 factory. */
int aoe_scenario_runtime_terrain_transition_ops_ror(
    AoeScenarioRuntimeRor *runtime, AoeTerrainTransitionOps *ops);

/* Complete owned composition of completion's 00449110 call, including its
   004481b0 per-cell object transitions, border refresh, and invalidation. */
int aoe_scenario_runtime_paint_foundation_ror(
    AoeScenarioRuntimeRor *runtime,
    const AoeBuildingFoundationBounds *bounds, uint8_t terrain_id);

/* Supplies the owned stack factory, full terrain painter, and post-paint map
   invalidator to the translated type-80 completion continuation. Research
   and owner UI remain caller-selected because they are gameplay/UI state. */
int aoe_scenario_runtime_building_completion_world_ops_ror(
    AoeScenarioRuntimeRor *runtime,
    AoeResearchCompletionManagerRor *research,
    AoeBuildingCompletionUiRor *owner_ui,
    AoeBuildingCompletionWorldRuntimeOps *ops);

/* Runs only the recovered sprite-manager tail shared by RoR live types
   20..80, in scenario player/record order. It does not stand in for the
   still-incomplete action, movement, combat, or player update chain. */
int aoe_scenario_runtime_tick_sprites_ror(
    AoeScenarioRuntimeRor *runtime, float world_delta,
    AoeScenarioRuntimeSpriteTickResult *result);

/* The live unit whose owned storage holds `building`. */
AoeLiveUnit *aoe_scenario_runtime_building_unit_ror(
    const AoeBuildingUpdate *building);

/* The inert list callbacks bound with runtime building storage (a world
   binding may replace them and must restore them before it goes away). */
void aoe_scenario_runtime_building_list_ops_ror(AoeActionListOps *out);

/* Owner player +0x78(type, amount, flag) through the derived player
   override 0x004f6c60. */
void aoe_scenario_runtime_owner_add_ror(AoeScenarioRuntimeRor *runtime,
                                        AoeLiveUnit *unit, int16_t type,
                                        float amount, int flag);

/* Type-80 setter request 2 (completion) on the owned runtime: the
   translated visual and world continuations with the +0x260 storage add;
   the local-completion signal, wall adjacency (complete_adjacent) and owner
   research/UI are not bound. */
int aoe_scenario_runtime_complete_building_ror(AoeScenarioRuntimeRor *runtime,
                                               AoeLiveUnit *unit);

/* 0x004ad7b0 add-build-work on a runtime building, completing it through
   aoe_scenario_runtime_complete_building_ror. */
int aoe_scenario_runtime_add_build_work_ror(AoeScenarioRuntimeRor *runtime,
                                            AoeLiveUnit *unit, float amount,
                                            int *completed);

void aoe_scenario_runtime_destroy_ror(AoeScenarioRuntimeRor *runtime);

#endif
