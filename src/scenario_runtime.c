#include "scenario_runtime.h"
#include "unit_placement_check.h"
#include "player_resources.h"
#include "aoe_ftol.h"
#include "live_unit_state.h"

#include <stddef.h>

#include <stdlib.h>
#include <string.h>

static int runtime_building_init(AoeScenarioRuntimeRor *runtime,
                                 AoeScenarioRuntimePlayerRor *player,
                                 AoeLiveUnit *unit);
static int runtime_research_init(AoeScenarioRuntimeRor *runtime,
                                 AoeScenarioRuntimePlayerRor *player);
static void runtime_outcome_init(AoeScenarioRuntimeRor *runtime,
                                 AoeScenarioRuntimePlayerRor *player,
                                 const DatCiv *civ);
static int effect_memory_remove(void *context, int player, int32_t unit_id,
                                uint8_t category);
static int effect_doppelganger(void *context, int player, void *unit);
static int runtime_tribe_scenario_load(AoeScenarioRuntimeRor *runtime,
                                       AoeScenarioRuntimePlayerRor *player,
                                       const DatCiv *civ,
                                       const ScnMap *scenario);

static void storage_free_queue(void *context, void *allocation) {
    (void)context;
    free(allocation);
}

void aoe_scenario_runtime_destroy_ror(AoeScenarioRuntimeRor *runtime) {
    if (runtime) aoe_sprite_draw_queue_free(&runtime->sprite_queue);
    if (!runtime) return;
    for (size_t i = 0; i < runtime->building_count; ++i) {
        AoeScenarioBuildingStorageRor *st = runtime->buildings[i];
        if (st && st->unit && st->unit->building_update == &st->building)
            (void)aoe_live_unit_unbind_building_update_ror(
                st->unit, &runtime->queue_status_four, runtime,
                storage_free_queue);
        else if (st)
            free(st->building.queue_entries);
        free(st);
    }
    free(runtime->buildings);
    for (size_t i = runtime->editor_unit_count; i > 0; --i) {
        AoeLiveUnit *unit = runtime->editor_units[i - 1];
        int16_t owner_index = unit ? unit->runtime_owner_index : -1;
        if (unit && unit->constructed && owner_index >= 0 &&
            owner_index < AOE_SCENARIO_RUNTIME_PLAYERS)
            aoe_live_unit_destroy_ror(
                unit, &runtime->players[owner_index].owner, &runtime->world,
                &runtime->players[owner_index].vision_binding);
        free(unit);
    }
    free(runtime->editor_units);
    for (int p = AOE_SCENARIO_RUNTIME_PLAYERS - 1; p >= 0; --p) {
        AoeScenarioRuntimePlayerRor *player = &runtime->players[p];
        if (player->units)
            for (uint32_t i = player->unit_count; i > 0; --i)
                if (player->units[i - 1].constructed)
                    aoe_live_unit_destroy_ror(
                        &player->units[i - 1], &player->owner,
                        &runtime->world, &player->vision_binding);
        free(player->units);
        aoe_research_manager_free_ror(&player->research);
        aoe_outcome_events_free_ror(&player->outcome);
        aoe_player_object_memory_free(&player->object_memory);
        free(player->resource_values);
        aoe_visibility_watch_destroy(&player->owner.visibility_watch40);
        aoe_player_vision_state_destroy(&player->vision);
        aoe_player_templates_free(&player->templates);
    }
    free(runtime->draw_scratch);
    aoe_owned_visibility_registry_destroy(&runtime->world);
    aoe_runtime_map_destroy(&runtime->map);
    memset(runtime, 0, sizeof(*runtime));
    runtime->active_player = -1;
}

int aoe_scenario_runtime_build_ror(
    AoeScenarioRuntimeRor *runtime, const ScnMap *scenario,
    const DatIntro *intro,
    const DatCivilizations *civilizations, const DatGraphics *graphics,
    const DatTerrains *terrains, const DatBorders *borders,
    const DatMapGeometry *geometry, const DatTechs *techs,
    const DatEffects *effects, const AoeGameOptionsRor *options,
    uint32_t rng_seed) {
    if (!runtime || !scenario || !intro || !intro->terrain_restrictions ||
        !civilizations || !graphics || !terrains || !techs || !effects ||
        !borders || !geometry || runtime->initialized || runtime->map.tiles ||
        runtime->world.view.positive || runtime->world.view.negative)
        return 0;
    runtime->active_player = -1;
    runtime->rng.state = rng_seed;
    runtime->techs = techs;
    runtime->effects = effects;
    runtime->options = options ? *options : aoe_game_options_default_ror();
    memcpy(runtime->scenario_options, scenario->options,
           sizeof(runtime->scenario_options));
    runtime->player_count = (int16_t)(scenario->player_count + 1 <
                                              AOE_SCENARIO_RUNTIME_PLAYERS
                                          ? scenario->player_count + 1
                                          : AOE_SCENARIO_RUNTIME_PLAYERS);
    runtime->intro = intro;
    runtime->terrains = terrains;
    runtime->terrain_borders = borders;
    runtime->graphics = graphics;
    runtime->geometry = geometry;
    for (unsigned p = 1; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p) {
        uint16_t count = scenario->player_details[p - 1].diplomacy_count;
        if (count > AOE_SCENARIO_RUNTIME_PLAYERS)
            count = AOE_SCENARIO_RUNTIME_PLAYERS;
        for (uint16_t q = 0; q < count; ++q)
            runtime->diplomacy[p][q] =
                (int32_t)scenario->player_details[p - 1].diplomacy_values[q];
    }
    if (!aoe_runtime_map_from_scenario(&runtime->map, scenario, geometry,
                                        terrains, borders) ||
        !aoe_owned_visibility_registry_init(&runtime->world))
        goto fail;

    /* Completing a building during load can create units (stack units)
       through the editor factory, as the original does inside its load. */
    runtime->initialized = 1;
    runtime->player_effects.context = runtime;
    runtime->player_effects.memory_remove = effect_memory_remove;
    runtime->player_effects.doppelganger = effect_doppelganger;
    runtime->world.player_effects = &runtime->player_effects;
    runtime->outcome_world.player_count = runtime->player_count;
    runtime->outcome_world.players = runtime->outcome_players;
    runtime->outcome_world.player_capacity = AOE_SCENARIO_RUNTIME_PLAYERS;
    /* World +0x98 loads every player's data (+0x68) before any unit. */
    for (unsigned p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p) {
        AoeScenarioRuntimePlayerRor *player = &runtime->players[p];
        const ScnPlayerUnits *records = &scenario->player_units[p];
        uint32_t civ_id = p ? scenario->player_setup[p - 1].civilization_id : 0;
        const DatCiv *civ;
        if (civ_id >= civilizations->count) goto fail;
        civ = &civilizations->civs[civ_id];
        player->player_index = (uint8_t)p;
        player->civilization_id = civ_id;
        player->unit_count = records->count;
        if (!aoe_player_templates_clone_civ(civ, &player->templates) ||
            !aoe_player_vision_state_init(&player->vision,
                                          &runtime->map.masks, (uint8_t)p))
            goto fail;
        player->resource_values = malloc(
            (size_t)(civ->resource_count ? civ->resource_count : 1) *
            sizeof(float));
        player->units = calloc(records->count ? records->count : 1,
                               sizeof(*player->units));
        if (!player->resource_values || !player->units) goto fail;
        if (civ->resource_count)
            memcpy(player->resource_values, civ->resources,
                   (size_t)civ->resource_count * sizeof(float));
        player->resources = (AoePlayerResources){
            (int16_t)civ->resource_count, player->resource_values};
        player->vision_binding.owner_grid = &player->vision.grid;
        player->vision_binding.owner_index = (int16_t)p;
        player->vision_binding.active_index = -1;
        player->vision_binding.explored_ops.owner_context =
            &player->vision.owner_cells58;
        player->vision_binding.explored_ops.record_owner =
            aoe_explored_cells_record;
        runtime_outcome_init(runtime, player, civ);
        /* Player +0x114 (tribe constructor 0x004efb00): five categories.
           Its presence enables the player map update 0x00517060. */
        if (!aoe_player_object_memory_init(&player->object_memory,
                                           AOE_PLAYER_MEMORY_CATEGORIES))
            goto fail;
        player->owner.map_update_flag = 1;
        /* Gaia is a computer player (0x0050a470 -> 0x004f7290), whose
           tribe constructor 0x004efb00 builds the research manager with
           0x004ea91c(..., 1), i.e. with a rebuild (0x004eb38d). The
           scenario reads stocks for players 1..8 only (eight sets), and
           only they run the tribe scenario load, which replaces it. */
        if (!p && (!runtime_research_init(runtime, player) ||
                   !aoe_research_rebuild_ror(&player->research,
                                             runtime->techs,
                                             &player->resources)))
            goto fail;
        if (p && !runtime_tribe_scenario_load(runtime, player, civ, scenario))
            goto fail;
    }
    for (unsigned p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p) {
        AoeScenarioRuntimePlayerRor *player = &runtime->players[p];
        const ScnPlayerUnits *records = &scenario->player_units[p];
        for (uint32_t i = 0; i < records->count; ++i) {
            const ScnUnit *record = &records->units[i];
            DatUnit *definition = aoe_player_templates_get(
                &player->templates, record->object_id);
            int32_t reserved_id;
            memcpy(&reserved_id, &record->spawn_id, sizeof(reserved_id));
            runtime->world.reserved_id = reserved_id;
            runtime->world.special_id_mode = 1;
            if (!definition || !aoe_live_unit_construct_ror(
                    &player->units[i], definition, graphics,
                    &player->resources, &runtime->world, &player->owner,
                    runtime->map.tiles, runtime->map.objects, geometry,
                    runtime->map.width, runtime->map.height,
                    &player->vision_binding, &runtime->rng,
                    record->x, record->y, record->z) ||
                player->vision.owner_cells58.allocation_failed ||
                player->units[i].core.unit_id != reserved_id)
                goto fail;
            player->units[i].runtime_owner_index = (int16_t)p;
            runtime->world.special_id_mode = 0;
            ++runtime->constructed_units;
            if (!runtime_building_init(runtime, player, &player->units[i]))
                goto fail;
            if (record->state <= 2) {
                uint8_t type = definition->type;
                /* Every player uses the tribe spawn 0x004efe70: Gaia is a
                   computer player (RoR world +0x5c 0x0050a470 builds it
                   with 0x004f7290, vtable 0x00549b80, which inherits
                   spawn +0x60). It calls +0xb4 with the rotation only for
                   30 < type < 90 and otherwise stores ftol(rotation) in
                   +0x35 (cliff and other Gaia object facets). */
                if (type <= 30 || type >= 90) {
                    player->units[i].core.scenario_frame =
                        (uint8_t)aoe_ftol32(record->rotation);
                } else if (!aoe_live_unit_scenario_dispatch_ror(
                               &player->units[i], record->state,
                               record->rotation,
                               &(AoeLiveUnitScenarioDispatchOps){0})) {
                    goto fail;
                }
                ++runtime->low_state_units;
                /* 0x004eff01: a finished building record gets 10000 build
                   work (0x004ad7b0), completing it. */
                if (type == 80 && record->state == 2) {
                    int completed = 0;
                    if (!aoe_scenario_runtime_add_build_work_ror(
                            runtime, &player->units[i], 10000.0f,
                            &completed) ||
                        !completed)
                        goto fail;
                }
            } else if (record->state >= 7) {
                if (!aoe_live_unit_scenario_high_root_ror(
                        &player->units[i], &player->resources,
                        &player->owner, &runtime->world,
                        &player->vision_binding)) goto fail;
                ++runtime->high_state_units;
            } else {
                /* No extracted campaign currently uses this branch, but a
                   general runtime may not silently invent its callbacks. */
                goto fail;
            }
        }
    }
    runtime->initialized = 1;
    return 1;
fail:
    aoe_scenario_runtime_destroy_ror(runtime);
    return 0;
}

int aoe_scenario_runtime_set_active_player_ror(
    AoeScenarioRuntimeRor *runtime, int16_t player_index) {
    if (!runtime || !runtime->initialized || player_index < 0 ||
        player_index >= AOE_SCENARIO_RUNTIME_PLAYERS)
        return 0;
    if (runtime->active_player >= 0 &&
        !aoe_runtime_map_bind_player_vision(
            &runtime->map, &runtime->players[runtime->active_player].vision,
            0)) return 0;
    if (!aoe_runtime_map_bind_player_vision(
            &runtime->map, &runtime->players[player_index].vision, 1))
        return 0;
    for (unsigned p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p) {
        AoeLiveUnitVisionBinding *binding =
            &runtime->players[p].vision_binding;
        binding->active_index = player_index;
        binding->active_relations = runtime->diplomacy[player_index];
        binding->relation_count = AOE_SCENARIO_RUNTIME_PLAYERS;
        binding->explored_ops.active_context =
            &runtime->players[player_index].vision.active_cells6c;
        binding->explored_ops.record_active = aoe_explored_cells_record;
    }
    runtime->active_player = player_index;
    return 1;
}

static void storage_list_destroy(void *context, void *action) {
    (void)context; (void)action;
}
static void storage_list_state(void *context, void *action, uint8_t state) {
    (void)context; (void)action; (void)state;
}
void aoe_scenario_runtime_building_list_ops_ror(AoeActionListOps *out) {
    if (out)
        *out = (AoeActionListOps){NULL, storage_list_destroy,
                                  storage_list_state};
}

AoeLiveUnit *aoe_scenario_runtime_building_unit_ror(
    const AoeBuildingUpdate *building) {
    if (!building) return NULL;
    return ((const AoeScenarioBuildingStorageRor *)(const void *)(
                (const char *)building -
                offsetof(AoeScenarioBuildingStorageRor, building)))
        ->unit;
}

/* Type-80 construction: owned storage bound to the unit, then the
   foundation body 0x004ac8a0. */
static int runtime_building_init(AoeScenarioRuntimeRor *runtime,
                                 AoeScenarioRuntimePlayerRor *player,
                                 AoeLiveUnit *unit) {
    AoeScenarioBuildingStorageRor *st;
    AoeActionListOps ops;
    if (!unit->core.unit_template || unit->core.unit_template->type != 80)
        return 1;
    if (runtime->building_count == runtime->building_capacity) {
        size_t next = runtime->building_capacity
                          ? runtime->building_capacity * 2 : 64;
        AoeScenarioBuildingStorageRor **grown;
        if (next > SIZE_MAX / sizeof(*grown)) return 0;
        grown = realloc(runtime->buildings, next * sizeof(*grown));
        if (!grown) return 0;
        runtime->buildings = grown;
        runtime->building_capacity = next;
    }
    st = calloc(1, sizeof(*st));
    if (!st) return 0;
    runtime->buildings[runtime->building_count++] = st;
    st->unit = unit;
    aoe_scenario_runtime_building_list_ops_ror(&ops);
    return aoe_live_unit_bind_building_update_ror(
               unit, &st->building, &st->live, ops, &st->queue, ops,
               player->resources, (int16_t)player->player_index, 0) &&
           aoe_live_unit_building_foundation_ror(
               unit, &player->resources, &player->owner, &runtime->world,
               &player->vision_binding, runtime->graphics);
}

static void runtime_refresh_world(void *context) { (void)context; }

void aoe_scenario_runtime_owner_add_ror(AoeScenarioRuntimeRor *runtime,
                                        AoeLiveUnit *unit, int16_t type,
                                        float amount, int flag) {
    AoeScenarioRuntimePlayerRor *p;
    AoePlayerResourceTargetRor target;
    int16_t o = unit ? unit->runtime_owner_index : -1;
    if (!runtime || o < 0 || o >= AOE_SCENARIO_RUNTIME_PLAYERS) return;
    p = &runtime->players[o];
    target.resources = &p->resources;
    target.derived = 1;
    target.field_104 = &p->field_104;
    target.context = runtime;
    target.refresh_world = runtime_refresh_world;
    (void)aoe_player_resource_add_virtual_ror(&target, type, amount, flag);
}

static void complete_add(void *context, AoeLiveUnit *unit, int16_t type,
                         float amount) {
    aoe_scenario_runtime_owner_add_ror(context, unit, type, amount, 0);
}
/* +0x260 = 0x004acaf0. */
static void complete_storage(void *context, AoeLiveUnit *unit) {
    (void)aoe_live_unit_building_storage_ror(unit, 1, complete_add, context);
}
static void complete_local(void *context, AoeLiveUnit *unit, int value) {
    (void)context; (void)unit; (void)value;
}

/* World player-effect hooks for root +0x60/+0x64. */
static int effect_memory_remove(void *context, int player, int32_t unit_id,
                                uint8_t category) {
    AoeScenarioRuntimeRor *r = context;
    if (player < 0 || player >= AOE_SCENARIO_RUNTIME_PLAYERS) return 0;
    return aoe_player_object_memory_remove(&r->players[player].object_memory,
                                           unit_id, category);
}

/* Doppelganger creation (template 243 +0x38 -> 0x00436030 with the source
   unit, 0x004364b0) is not translated yet: report it instead of
   inventing a fog image. */
static int effect_doppelganger(void *context, int player, void *unit) {
    AoeScenarioRuntimeRor *r = context;
    (void)player; (void)unit;
    ++r->doppelgangers_requested;
    return 0;
}

/* Wall adjacency (patched RoR 1.0a). Only units whose current graphic
   (+0x10) has exactly three angles take part; others get facet 0. */

/* 0x00451370(template ID, -1, -1, 0, 0, 0) over one tile's object list,
   then the 0x004ad380 owner test. */
static AoeLiveUnit *wall_neighbor(AoeScenarioRuntimeRor *r,
                                  const AoeLiveUnit *u, int32_t x, int32_t y) {
    const AoeObjectUpdateNode *node;
    int16_t id = u->core.unit_template->id;
    for (node = r->map.objects[(size_t)y * r->map.width + (uint32_t)x].head;
         node; node = node->next) {
        AoeLiveUnit *o = node->object;
        if (!o || !o->constructed || !o->core.unit_template ||
            o->core.unit_template->id != id)
            continue;
        return o->runtime_owner_index == u->runtime_owner_index ? o : NULL;
    }
    return NULL;
}

/* 0x004ad380: n[0] (x, y-1), n[1] (x-1, y), n[2] (x+1, y), n[3] (x, y+1),
   each only inside the map. */
static void wall_neighbors(AoeScenarioRuntimeRor *r, const AoeLiveUnit *u,
                           AoeLiveUnit *n[4]) {
    int32_t x = aoe_ftol32(u->core.position.x38);
    int32_t y = aoe_ftol32(u->core.position.y3c);
    int32_t w = (int32_t)r->map.width, h = (int32_t)r->map.height;
    n[0] = n[1] = n[2] = n[3] = NULL;
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    if (y > 0) n[0] = wall_neighbor(r, u, x, y - 1);
    if (x > 0) n[1] = wall_neighbor(r, u, x - 1, y);
    if (x < w - 1) n[2] = wall_neighbor(r, u, x + 1, y);
    if (y < h - 1) n[3] = wall_neighbor(r, u, x, y + 1);
}

/* 0x0044f090(delta): rotation += delta * 2pi / angles (double 2pi at
   0x00544a38), wrapped into [0, 2pi) with the float 2pi at 0x00544a30,
   then the unit's +0xb4 low-state method with the new rotation. */
static void wall_turn(AoeLiveUnit *u, const DatGraphic *g, int32_t delta) {
    double v = (double)delta * 6.2831853 / (double)g->angle_count +
               (double)u->core.scenario_rotation;
    if (v < 0.0)
        do v += (double)6.2831854820251465f; while (v < 0.0);
    while (!(v < 6.2831853)) v -= (double)6.2831854820251465f;
    (void)aoe_live_unit_low_state_ror(&u->core, g, (float)v);
}

/* 0x004ad620 (and the tail of 0x004ad550): facet 1 between walls at
   n[0] and n[3] only, facet 0 between n[1] and n[2] only, else 2. */
static void wall_refresh(AoeScenarioRuntimeRor *r, AoeLiveUnit *u,
                         AoeLiveUnit *n[4]) {
    const DatGraphic *g = u->graphic.current_graphic;
    uint8_t facet = u->core.scenario_frame;
    if (!g) return;
    if (g->angle_count != 3) {
        u->core.scenario_frame = 0;
        return;
    }
    wall_neighbors(r, u, n);
    if (n[0] && n[3] && !n[1] && !n[2])
        wall_turn(u, g, 1 - facet);
    else if (n[1] && n[2] && !n[0] && !n[3])
        wall_turn(u, g, -(int32_t)facet);
    else
        wall_turn(u, g, 2 - facet);
}

/* 0x004ad550: recompute the unit's facet from its rotation (0x0044bbd0),
   refresh it, then refresh each neighbour wall found (0x004ad620). */
static void wall_adjacency(AoeScenarioRuntimeRor *r, AoeLiveUnit *u) {
    AoeLiveUnit *n[4], *m[4];
    const DatGraphic *g;
    if (!r || !u || !u->constructed || !u->core.unit_template) return;
    g = u->graphic.current_graphic;
    if (!g) return;
    if (g->angle_count != 3) {
        u->core.scenario_frame = 0;
        return;
    }
    (void)aoe_live_unit_low_state_ror(&u->core, g,
                                      u->core.scenario_rotation);
    wall_refresh(r, u, n);
    for (int i = 0; i < 4; ++i)
        if (n[i]) wall_refresh(r, n[i], m);
    ++r->adjacency_refreshes;
}

static void complete_adjacent(void *context, AoeLiveUnit *unit) {
    wall_adjacency(context, unit);
}
static void complete_disappear(void *context, AoeLiveUnit *unit) {
    (void)context;
    unit->core.hit_points = 0.0f;
}

int aoe_scenario_runtime_complete_building_ror(AoeScenarioRuntimeRor *runtime,
                                               AoeLiveUnit *unit) {
    AoeScenarioRuntimePlayerRor *p;
    AoeBuildingCompletionVisualRuntimeOps visual;
    AoeBuildingCompletionWorldRuntimeOps world;
    int16_t o = unit ? unit->runtime_owner_index : -1;
    if (!runtime || o < 0 || o >= AOE_SCENARIO_RUNTIME_PLAYERS) return 0;
    p = &runtime->players[o];
    memset(&visual, 0, sizeof(visual));
    visual.context = runtime;
    visual.rng = &runtime->rng;
    visual.graphics = runtime->graphics;
    visual.vision = &p->vision_binding;
    visual.local_completion = complete_local;
    visual.building_pre_complete = complete_storage;
    visual.complete_adjacent = complete_adjacent;
    visual.disappear = complete_disappear;
    if (!aoe_scenario_runtime_building_completion_world_ops_ror(
            runtime, &p->research, NULL, &world) ||
        !aoe_live_unit_scenario_building_complete_ror(
            unit, &visual, &world, &p->resources, &p->owner, &runtime->world,
            &p->vision_binding))
        return 0;
    ++runtime->completed_buildings;
    return 1;
}

static int work_set_state(void *context, AoeLiveUnit *unit, uint8_t state) {
    return state == 2 &&
           aoe_scenario_runtime_complete_building_ror(context, unit);
}

int aoe_scenario_runtime_add_build_work_ror(AoeScenarioRuntimeRor *runtime,
                                            AoeLiveUnit *unit, float amount,
                                            int *completed) {
    AoeLiveUnitSetStateOps ops = {runtime, work_set_state};
    if (!runtime || !unit) return 0;
    return aoe_live_unit_add_build_work_ror(unit, amount, &ops, completed);
}

/* ---- technology ---------------------------------------------------- */

static const DatGraphic *runtime_graphic(const DatGraphics *g, int16_t id) {
    if (!g || id < 0 || (uint16_t)id >= g->count || !g->graphics[id].present)
        return NULL;
    return &g->graphics[id];
}

static AoeScenarioRuntimeRor *runtime_of_player(
    AoeScenarioRuntimePlayerRor *player) {
    return (AoeScenarioRuntimeRor *)(void *)(
        (char *)(player - player->player_index) -
        offsetof(AoeScenarioRuntimeRor, players));
}

static AoeLiveUnit *unit_from_ref(AoeScenarioRuntimeRor *r,
                                  const void *object) {
    uintptr_t o = (uintptr_t)object;
    for (unsigned p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p) {
        AoeScenarioRuntimePlayerRor *pl = &r->players[p];
        uintptr_t lo = (uintptr_t)pl->units, hi;
        size_t i;
        if (!pl->units || !pl->unit_count) continue;
        hi = (uintptr_t)(pl->units + pl->unit_count);
        if (o < lo || o >= hi) continue;
        i = (o - lo) / sizeof(*pl->units);
        return &pl->units[i].core.position == object ? &pl->units[i] : NULL;
    }
    for (size_t i = 0; i < r->editor_unit_count; ++i)
        if (r->editor_units[i] &&
            &r->editor_units[i]->core.position == object)
            return r->editor_units[i];
    return NULL;
}

typedef struct {
    AoeScenarioRuntimeRor *runtime;
    AoeScenarioRuntimePlayerRor *player;
    AoeLiveUnit **units;
    size_t count;
} EffectContext;

static size_t effect_unit_count(void *c) { return ((EffectContext *)c)->count; }
static AoeLiveUnit *effect_unit_at(void *c, size_t i) {
    return ((EffectContext *)c)->units[i];
}
static void effect_resource_add(void *c, int16_t type, float amount) {
    EffectContext *e = c;
    AoePlayerResourceTargetRor target;
    target.resources = &e->player->resources;
    target.derived = 1;
    target.field_104 = &e->player->field_104;
    target.context = e->runtime;
    target.refresh_world = runtime_refresh_world;
    (void)aoe_player_resource_add_virtual_ror(&target, type, amount, 0);
}

static AoeLiveUnitVisionBinding *vision_of(AoeScenarioRuntimeRor *r,
                                           AoeLiveUnit *u) {
    int16_t o = u->runtime_owner_index;
    return o >= 0 && o < AOE_SCENARIO_RUNTIME_PLAYERS
               ? &r->players[o].vision_binding
               : NULL;
}
/* +0xdc(owner, 0, -1) then +0xd8(owner, 0, radius). */
static int live_vision_reset(void *c, AoeLiveUnit *u, int32_t radius) {
    AoeLiveUnitVisionBinding *v = vision_of(c, u);
    return v && aoe_live_unit_vision_radius_ror(&u->core, v, 0, 0, -1) &&
           aoe_live_unit_vision_radius_ror(&u->core, v, 1, 0, radius);
}
static int live_select_graphic(void *c, AoeLiveUnit *u,
                               const DatGraphic *g) {
    AoeScenarioRuntimeRor *r = c;
    const DatUnit *t = u->core.unit_template;
    AoeUnitGraphicTemplate gt;
    gt.standing_graphic = runtime_graphic(r->graphics, t->standing_graphic);
    gt.walking_graphic = runtime_graphic(r->graphics, t->walking_graphic);
    return aoe_unit_graphic_select_runtime(&u->graphic, &gt, &u->sprites, g,
                                           AOE_PLAYER_RISE_OF_ROME);
}
static int live_damage_graphics(void *c, AoeLiveUnit *u) {
    AoeScenarioRuntimeRor *r = c;
    AoeLiveUnitDamageGraphicOps o = {r, live_select_graphic, r->graphics,
                                     r->geometry, &r->rng};
    return aoe_live_unit_damage_graphics_ror(u, &o);
}
static void live_retask(void *c, AoeLiveUnit *u, const DatUnit *t) {
    AoeScenarioRuntimeRor *r = c;
    if (r->retask) r->retask(r->retask_context, u, t);
}
static void live_adjacent(void *c, AoeLiveUnit *u) {
    wall_adjacency(c, u); /* 0x004ad550 */
}

typedef enum { TARGET_EFFECT, TARGET_ATTRIBUTE } TargetKind;
typedef struct {
    TargetKind kind;
    int16_t effect_id;
    AoeAttributeOp op;
    int16_t unit, unit_class;
    uint8_t attribute;
    float amount;
} TargetCall;
static int runtime_target_call(AoeScenarioRuntimeRor *runtime,
                               int16_t player_index, const TargetCall *call);

int aoe_scenario_runtime_attribute_ror(AoeScenarioRuntimeRor *runtime,
                                       int16_t player_index,
                                       AoeAttributeOp op, int16_t unit,
                                       int16_t unit_class, uint8_t attribute,
                                       float amount) {
    TargetCall call = {TARGET_ATTRIBUTE, -1, op, unit, unit_class, attribute,
                       amount};
    return runtime_target_call(runtime, player_index, &call);
}

int aoe_scenario_runtime_apply_effect_ror(AoeScenarioRuntimeRor *runtime,
                                          int16_t player_index,
                                          int16_t effect_id) {
    TargetCall call = {TARGET_EFFECT, effect_id, AOE_ATTRIBUTE_SET, -1, -1, 0,
                       0.0f};
    return runtime_target_call(runtime, player_index, &call);
}

static int runtime_target_call(AoeScenarioRuntimeRor *runtime,
                               int16_t player_index, const TargetCall *call) {
    int16_t effect_id = call->effect_id;
    AoeScenarioRuntimePlayerRor *p;
    EffectContext e;
    AoeLiveAttributeOps live;
    AoeEffectTargetOps ops;
    size_t n = 0;
    int ok;
    if (!runtime || !runtime->effects || player_index < 0 ||
        player_index >= AOE_SCENARIO_RUNTIME_PLAYERS)
        return 0;
    p = &runtime->players[player_index];
    /* Player list +0x28, in order. */
    for (AoeObjectUpdateNode *node = p->owner.object_list28.head; node;
         node = node->next)
        ++n;
    e = (EffectContext){runtime, p, n ? calloc(n, sizeof(AoeLiveUnit *))
                                      : NULL,
                        0};
    if (n && !e.units) return 0;
    for (AoeObjectUpdateNode *node = p->owner.object_list28.head; node;
         node = node->next) {
        AoeLiveUnit *u = unit_from_ref(runtime, node->object);
        if (u) e.units[e.count++] = u;
    }
    live = (AoeLiveAttributeOps){runtime, runtime->graphics,
                                 live_vision_reset, live_select_graphic,
                                 live_damage_graphics, live_retask,
                                 live_adjacent, &p->resources};
    ops = (AoeEffectTargetOps){&e, &p->templates, &p->resources,
                               effect_unit_count, effect_unit_at,
                               effect_resource_add,
                               p->research.entries ? &p->research : NULL,
                               &live};
    ok = call->kind == TARGET_EFFECT
             ? aoe_effect_apply_ror(runtime->effects, effect_id, &ops)
             : aoe_effect_attribute_ror(&ops, call->op, call->unit,
                                        call->unit_class, call->attribute,
                                        call->amount);
    free(e.units);
    return ok;
}

/* Research manager callbacks (context: the player). */
static void research_apply(void *c, int16_t research_id) {
    AoeScenarioRuntimePlayerRor *p = c;
    AoeScenarioRuntimeRor *r = runtime_of_player(p);
    if (research_id < 0 || (uint32_t)research_id >= r->techs->count) return;
    /* 0x004ea3bc: the tech's effect (+0x20) when nonnegative. */
    if (r->techs->techs[research_id].effect_id >= 0)
        (void)aoe_scenario_runtime_apply_effect_ror(
            r, (int16_t)p->player_index,
            r->techs->techs[research_id].effect_id);
}
static void research_rebuild(void *c) {
    AoeScenarioRuntimePlayerRor *p = c;
    (void)aoe_research_rebuild_ror(&p->research, runtime_of_player(p)->techs,
                                   &p->resources);
}
static void research_wonder(void *c, int index) {
    (void)index;
    ++runtime_of_player(c)->wonder_signals;
}

static int runtime_research_init(AoeScenarioRuntimeRor *runtime,
                                 AoeScenarioRuntimePlayerRor *player) {
    if (!aoe_research_manager_init_ror(&player->research, runtime->techs))
        return 0;
    player->research.context = player;
    player->research.apply_effect = research_apply;
    player->research.rebuild = research_rebuild;
    player->research.wonder_complete = research_wonder;
    return 1;
}

static void runtime_outcome_init(AoeScenarioRuntimeRor *runtime,
                                 AoeScenarioRuntimePlayerRor *player,
                                 const DatCiv *civ) {
    AoeOutcomePlayerRef *ref = &player->outcome_player;
    (void)civ;
    memset(ref, 0, sizeof(*ref));
    ref->index4a = (int16_t)player->player_index;
    ref->world3c = &runtime->outcome_world;
    ref->resources50 = player->resource_values;
    ref->resource_capacity = (size_t)player->resources.count;
    ref->outcome34 = &player->outcome;
    player->outcome.owner = ref;
    player->owner.outcome_manager = &player->outcome;
    runtime->outcome_players[player->player_index] = ref;
}

/* Scenario-info 0x00507e50 for one tribe player (index = player - 1). */
static int runtime_scenario_player_settings(
    AoeScenarioRuntimeRor *runtime, AoeScenarioRuntimePlayerRor *player,
    const ScnMap *scenario) {
    static const int16_t techs[16] = {0x58, 0x59, 0x5a, 0x5b, 0x5e, 0x5f,
                                      0x61, 0x62, 0x5d, 0x60, 0x5c, 0x65,
                                      0x66, 0x67, 0x73, 0x74};
    int slot = player->player_index - 1;
    AoeResearchCompletionManagerRor *m = &player->research;
    if (slot < 0) return 1;
    for (int k = 0; k < 16; ++k) /* 0x004f1870(tech, flag byte) */
        (void)aoe_research_set_enabled_ror(
            m, techs[k], (uint8_t)scenario->tech_toggles[slot][k] != 0);
    if (!aoe_research_rebuild_ror(m, runtime->techs, &player->resources))
        return 0;
    if (scenario->options[0] &&
        !aoe_scenario_runtime_apply_effect_ror(
            runtime, (int16_t)player->player_index, 100))
        return 0;
    if (runtime->options.mode_1b0 != 7) {
        static const int steps[4] = {25, 23, 24, 1};
        int32_t age = scenario->starting_age[slot];
        if (age >= 1 && age <= 4 &&
            !aoe_research_advance_age_ror(m, runtime->techs,
                                          &player->resources,
                                          steps[age - 1]))
            return 0;
    }
    /* fild toggle 14; fcomp 0.0; equal -> resource 48 = 1.0. */
    if (scenario->tech_toggles[slot][14] == 0 &&
        player->resources.count > 48)
        player->resource_values[48] = 1.0f;
    return 1;
}

/* Tribe player scenario load 0x004f02d0 after the base load: every
   template disabled, the scenario's starting stocks (version 1.11 > 1.06),
   resources 30 and 21 cleared, a research manager without rebuild (1.11 >
   1.03), the civ tech tree's tech-disable commands (effect 210's when the
   tree is missing or option +0xad9 / scenario option 2 is set), the
   scenario player settings (0x00507e50, outside the editor) and a
   rebuild. */
static int runtime_tribe_scenario_load(AoeScenarioRuntimeRor *runtime,
                                       AoeScenarioRuntimePlayerRor *player,
                                       const DatCiv *civ,
                                       const ScnMap *scenario) {
    const ScnPlayerResources *start = &scenario->resources[player->player_index - 1];
    float *r = player->resource_values;
    int16_t tree = civ->tech_tree_id;
    player->tech_tree_id = tree;
    for (uint16_t i = 0; i < player->templates.count; ++i) {
        DatUnit *t = aoe_player_templates_get(&player->templates, i);
        if (t) t->enabled = 0;
    }
    if (player->resources.count <= 30) return 0;
    r[0] = start->food;
    r[1] = start->wood;
    r[3] = start->gold;
    r[2] = start->stone;
    r[30] = 0.0f;
    r[21] = 0.0f;
    if (!runtime_research_init(runtime, player)) return 0;
    if (tree < 0 || runtime->options.option_ad9 || scenario->options[2])
        tree = 210;
    if (!aoe_effect_apply_disables_ror(runtime->effects, tree,
                                       &player->research))
        return 0;
    if (runtime->options.mode_1b4 != 7 &&
        !runtime_scenario_player_settings(runtime, player, scenario))
        return 0;
    return aoe_research_rebuild_ror(&player->research, runtime->techs,
                                    &player->resources);
}

int aoe_scenario_runtime_editor_create_ror(
    AoeScenarioRuntimeRor *runtime, uint8_t player_index, uint16_t object_id,
    float x, float y, float z, AoeLiveUnit **out_unit) {
    AoeScenarioRuntimePlayerRor *player;
    AoeLiveUnit *unit;
    AoeLiveUnit **grown;
    size_t next_capacity;
    DatUnit *definition;
    if (out_unit) *out_unit = NULL;
    if (!runtime || !runtime->initialized ||
        player_index >= AOE_SCENARIO_RUNTIME_PLAYERS || !runtime->graphics ||
        !runtime->geometry)
        return 0;
    player = &runtime->players[player_index];
    definition = aoe_player_templates_get(&player->templates, object_id);
    if (!definition) return 0;
    if (runtime->editor_unit_count == runtime->editor_unit_capacity) {
        next_capacity = runtime->editor_unit_capacity
            ? runtime->editor_unit_capacity * 2 : 16;
        if (next_capacity < runtime->editor_unit_capacity ||
            next_capacity > SIZE_MAX / sizeof(*grown))
            return 0;
        grown = realloc(runtime->editor_units,
                        next_capacity * sizeof(*grown));
        if (!grown) return 0;
        runtime->editor_units = grown;
        runtime->editor_unit_capacity = next_capacity;
    }
    unit = calloc(1, sizeof(*unit));
    if (!unit) return 0;
    runtime->world.special_id_mode = 0;
    if (!aoe_live_unit_construct_ror(
            unit, definition, runtime->graphics, &player->resources,
            &runtime->world, &player->owner, runtime->map.tiles,
            runtime->map.objects, runtime->geometry, runtime->map.width,
            runtime->map.height, &player->vision_binding, &runtime->rng,
            x, y, z) || player->vision.owner_cells58.allocation_failed) {
        if (unit->constructed)
            aoe_live_unit_destroy_ror(unit, &player->owner, &runtime->world,
                                      &player->vision_binding);
        free(unit);
        return 0;
    }
    unit->runtime_owner_index = player_index;
    runtime->editor_units[runtime->editor_unit_count++] = unit;
    ++runtime->constructed_units;
    if (!runtime_building_init(runtime, player, unit)) return 0;
    if (out_unit) *out_unit = unit;
    return 1;
}

void aoe_scenario_runtime_building_spawn_stack_ror(
    void *context, AoeLiveUnit *building, int16_t unit_id,
    float x, float y, float z) {
    AoeScenarioRuntimeRor *runtime = context;
    int16_t owner_index;
    if (!runtime || !building || unit_id < 0) return;
    owner_index = building->runtime_owner_index;
    if (owner_index < 0 || owner_index >= AOE_SCENARIO_RUNTIME_PLAYERS)
        return;
    (void)aoe_scenario_runtime_editor_create_ror(
        runtime, (uint8_t)owner_index, (uint16_t)unit_id,
        x, y, z, NULL);
}

void aoe_scenario_runtime_building_invalidate_foundation_ror(
    void *context, AoeLiveUnit *building,
    int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    AoeScenarioRuntimeRor *runtime = context;
    (void)building;
    if (!runtime) return;
    (void)aoe_runtime_map_invalidate_ror(
        &runtime->map, x0, y0, x1, y1, 0);
}

static AoeLiveUnit *runtime_find_core(AoeScenarioRuntimeRor *runtime,
                                     const void *object) {
    if (!runtime || !runtime->initialized || !object) return NULL;
    for (unsigned p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p)
        for (uint32_t i = 0; i < runtime->players[p].unit_count; ++i)
            if (&runtime->players[p].units[i].core == object)
                return &runtime->players[p].units[i];
    for (size_t i = 0; i < runtime->editor_unit_count; ++i)
        if (runtime->editor_units[i] &&
            &runtime->editor_units[i]->core == object)
            return runtime->editor_units[i];
    return NULL;
}

int16_t aoe_scenario_runtime_terrain_object_owner_ror(
    void *context, const void *object) {
    AoeLiveUnit *unit = runtime_find_core(context, object);
    return unit ? unit->runtime_owner_index : -1;
}

int16_t aoe_scenario_runtime_terrain_object_template_ror(
    void *context, const void *object) {
    AoeLiveUnit *unit = runtime_find_core(context, object);
    return unit && unit->core.unit_template ? unit->core.unit_template->id : -1;
}

int aoe_scenario_runtime_terrain_remove_object_ror(
    void *context, void *object) {
    AoeScenarioRuntimeRor *runtime = context;
    AoeLiveUnit *unit = runtime_find_core(runtime, object);
    int16_t owner_index;
    if (!unit) return 0;
    owner_index = unit->runtime_owner_index;
    if (owner_index < 0 || owner_index >= AOE_SCENARIO_RUNTIME_PLAYERS)
        return 0;
    return aoe_live_unit_scenario_high_root_ror(
        unit, &runtime->players[owner_index].resources,
        &runtime->players[owner_index].owner, &runtime->world,
        &runtime->players[owner_index].vision_binding);
}

static int runtime_terrain_blocked(void *context, int16_t unit_id,
                                   float x, float y) {
    AoeScenarioRuntimeRor *runtime = context;
    DatUnit *unit;
    const float *restriction;
    AoeUnitPlacementCheckContext check;
    int32_t blocker = -1;
    if (!runtime || !runtime->initialized || !runtime->intro || unit_id < 0 ||
        !(unit = aoe_player_templates_get(
              &runtime->players[0].templates, (uint16_t)unit_id)) ||
        unit->terrain_restriction < 0 ||
        (uint16_t)unit->terrain_restriction >=
            runtime->intro->restriction_count)
        return 1;
    restriction = runtime->intro->terrain_restrictions +
        (size_t)(uint16_t)unit->terrain_restriction *
            runtime->intro->terrain_count;
    check = (AoeUnitPlacementCheckContext){
        &runtime->map, restriction, runtime->intro->terrain_count, 0, NULL,
        0, 0};
    return aoe_unit_placement_check_ror(
        unit, &check, x, y, &blocker,
        0, 1, 0, 1, 0, 1) != AOE_PLACEMENT_OK;
}

static int runtime_terrain_create(void *context, int16_t unit_id,
                                  float x, float y) {
    if (unit_id < 0) return 0;
    return aoe_scenario_runtime_editor_create_ror(
        context, 0, (uint16_t)unit_id, x, y, 0.0f, NULL);
}

int aoe_scenario_runtime_terrain_transition_ops_ror(
    AoeScenarioRuntimeRor *runtime, AoeTerrainTransitionOps *ops) {
    if (!runtime || !runtime->initialized || !ops) return 0;
    *ops = (AoeTerrainTransitionOps){
        runtime, 0,
        aoe_scenario_runtime_terrain_object_owner_ror,
        aoe_scenario_runtime_terrain_object_template_ror,
        aoe_scenario_runtime_terrain_remove_object_ror,
        runtime_terrain_blocked,
        runtime_terrain_create
    };
    return 1;
}

typedef struct {
    AoeScenarioRuntimeRor *runtime;
    AoeTerrainTransitionOps ops;
} RuntimeTerrainPaintContext;

static int runtime_paint_transition(void *opaque, uint32_t x, uint32_t y,
                                    uint8_t old_terrain,
                                    int16_t filter_terrain) {
    RuntimeTerrainPaintContext *context = opaque;
    (void)old_terrain;
    return aoe_terrain_transition_cell_ror(
        &context->runtime->map, context->runtime->terrains,
        &context->runtime->rng, x, y, filter_terrain, -1, -1,
        &context->ops);
}

int aoe_scenario_runtime_paint_foundation_ror(
    AoeScenarioRuntimeRor *runtime,
    const AoeBuildingFoundationBounds *bounds, uint8_t terrain_id) {
    RuntimeTerrainPaintContext context;
    AoeRuntimeTerrainPaintOps paint;
    if (!runtime || !runtime->initialized || !bounds || !runtime->terrains ||
        !runtime->terrain_borders ||
        !aoe_scenario_runtime_terrain_transition_ops_ror(
            runtime, &context.ops))
        return 0;
    context.runtime = runtime;
    paint = (AoeRuntimeTerrainPaintOps){&context, runtime_paint_transition};
    return aoe_runtime_map_paint_terrain_ror(
        &runtime->map, runtime->terrains, runtime->terrain_borders,
        bounds->x0, bounds->y0, bounds->x1, bounds->y1,
        terrain_id, 0, &paint);
}

static int runtime_building_paint_foundation(
    void *context, AoeLiveUnit *unit,
    const AoeBuildingFoundationBounds *bounds, uint8_t terrain_id) {
    (void)unit;
    return aoe_scenario_runtime_paint_foundation_ror(
        context, bounds, terrain_id);
}

int aoe_scenario_runtime_building_completion_world_ops_ror(
    AoeScenarioRuntimeRor *runtime,
    AoeResearchCompletionManagerRor *research,
    AoeBuildingCompletionUiRor *owner_ui,
    AoeBuildingCompletionWorldRuntimeOps *ops) {
    if (!runtime || !runtime->initialized || !ops) return 0;
    *ops = (AoeBuildingCompletionWorldRuntimeOps){
        .context = runtime,
        .spawn_stack_unit = aoe_scenario_runtime_building_spawn_stack_ror,
        .paint_foundation = runtime_building_paint_foundation,
        .invalidate_foundation =
            aoe_scenario_runtime_building_invalidate_foundation_ror,
        .research = research,
        .owner_ui = owner_ui
    };
    return 1;
}

int aoe_scenario_runtime_tick_sprites_ror(
    AoeScenarioRuntimeRor *runtime, float world_delta,
    AoeScenarioRuntimeSpriteTickResult *result) {
    AoeScenarioRuntimeSpriteTickResult next = {0};
    if (!runtime || !runtime->initialized || !result)
        return 0;
    for (unsigned p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p) {
        AoeScenarioRuntimePlayerRor *player = &runtime->players[p];
        for (uint32_t i = 0; i < player->unit_count; ++i) {
            AoeLiveUnit *unit = &player->units[i];
            uint8_t type;
            ++next.units_visited;
            if (!unit->constructed || !unit->core.unit_template)
                return 0;
            type = unit->core.unit_template->type;
            if (type != 10 && type != 90) {
                ++next.inherited_tick_units;
                next.sprite_instances +=
                    aoe_sprite_list_count(&unit->sprites.list);
            }
            if (!aoe_live_unit_tick_sprites_ror(unit, world_delta))
                return 0;
        }
    }
    for (size_t i = 0; i < runtime->editor_unit_count; ++i) {
        AoeLiveUnit *unit = runtime->editor_units[i];
        uint8_t type;
        ++next.units_visited;
        if (!unit || !unit->constructed || !unit->core.unit_template)
            return 0;
        type = unit->core.unit_template->type;
        if (type != 10 && type != 90) {
            ++next.inherited_tick_units;
            next.sprite_instances += aoe_sprite_list_count(&unit->sprites.list);
        }
        if (!aoe_live_unit_tick_sprites_ror(unit, world_delta)) return 0;
    }
    *result = next;
    return 1;
}
