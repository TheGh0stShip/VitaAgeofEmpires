#include "command_place.h"
#include "dat.h"
#include "player_costs.h"
#include "scenario_runtime.h"
#include "scn.h"
#include "unit_placement_check.h"
#include "world_combat.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { WALL_SMALL = 72, WALL_PIECES = 4 };

/* Wall line placement (tribe command 0x69, 0x004e8b00 with its run
   0x004e8d00) on a real Rise of Rome map: a player-1 villager is told to
   build a 4-cell Wall_Small (72, template +0x16c adjacent mode) line
   near its town center. Every cell is checked at its centre, paid and
   gets a type-80 foundation (0x004ac8a0: state 0, construction graphic,
   facet +0x35 = template +0x16e); the villager is ordered onto the first
   piece and builds it; completion selects the three-angle standing
   graphic. */
int main(int argc, char **argv) {
    DatBlob blob = {0};
    DatIntro intro = {0};
    DatSounds sounds = {0};
    DatGraphics graphics = {0};
    DatTerrains terrains = {0};
    DatBorders borders = {0};
    DatMapGeometry geometry = {0};
    DatRandomMaps random_maps = {0};
    DatEffects effects = {0};
    DatTechs techs = {0};
    DatCivilizations civs = {0};
    ScnMap scenario = {0};
    AoeScenarioRuntimeRor runtime = {0};
    AoeWorldMotionRor motion;
    AoeWorldCombatRor combat;
    AoeLiveUnit *tc = NULL, *villager = NULL, *first = NULL;
    AoeLiveUnit *pieces[WALL_PIECES] = {NULL};
    const DatUnit *wall_t;
    int32_t x1 = 0, y1 = 0;
    int found = 0, ok = 0;
    memset(&motion, 0, sizeof(motion));
    memset(&combat, 0, sizeof(combat));
    if (argc != 3) return 2;
    if (!dat_load(argv[2], &blob) || !dat_parse_intro(&blob, &intro) ||
        !dat_parse_sounds(&blob, &intro, &sounds) ||
        !dat_parse_graphics(&blob, &sounds, &graphics) ||
        !dat_parse_terrains(&blob, &intro, &graphics, &terrains) ||
        !dat_parse_borders(&blob, &terrains, &borders) ||
        !dat_parse_map_geometry(&blob, &terrains, &borders, &geometry) ||
        !dat_parse_random_maps(&blob, &geometry, &random_maps) ||
        !dat_parse_effects(&blob, &random_maps, &effects) ||
        !dat_parse_civilizations(&blob, &effects, &civs) ||
        !dat_parse_techs(&blob, &civs, &techs) ||
        !scn_load_map(argv[1], (uint8_t)intro.terrain_count, &scenario) ||
        !aoe_scenario_runtime_build_ror(&runtime, &scenario, &intro, &civs,
                                        &graphics, &terrains, &borders,
                                        &geometry, &techs, &effects, NULL, 1) ||
        !aoe_scenario_runtime_set_active_player_ror(&runtime, 1))
        return 1;
    if (!aoe_world_motion_init_ror(&motion, &runtime) ||
        !aoe_world_combat_init_ror(&combat, &motion))
        goto done;
    for (uint32_t i = 0; i < runtime.players[1].unit_count; ++i) {
        AoeLiveUnit *u = &runtime.players[1].units[i];
        if (!u->constructed || u->core.state != 2 || !u->core.unit_template)
            continue;
        if (!tc && u->core.unit_template->id == 109) tc = u;
        if (!villager && u->core.unit_template->unit_class == 4) villager = u;
    }
    wall_t = aoe_player_templates_get(&runtime.players[1].templates, WALL_SMALL);
    if (!tc || !villager || !wall_t || wall_t->type != 80 ||
        !wall_t->building_adjacent_mode) {
        fprintf(stderr, "no town center, villager or adjacent-mode wall\n");
        goto done;
    }
    /* A free straight run of four cells near the town center, each passing
       the run's own check +0x20(1, x, y, 0, 1, template +0x64, 0, 1, 1, 1)
       before anything is placed. */
    for (int r = 3; r <= 14 && !found; ++r)
        for (int k = 0; k < 8 && !found; ++k) {
            const int32_t flags[6] = {1, wall_t->placement_class, 0, 1, 1, 1};
            int32_t sx = (int32_t)floorf(tc->core.position.x38) +
                         (k & 1 ? r : -r - 3) * (k < 4 ? 1 : 0);
            int32_t sy = (int32_t)floorf(tc->core.position.y3c) +
                         (k & 2 ? r : -r) * (k >= 4 ? 1 : 0);
            int free_cells = 0;
            if (sx < 0 || sy < 0 || sx + WALL_PIECES - 1 > 255 || sy > 255 ||
                (uint32_t)(sx + WALL_PIECES) > runtime.map.width ||
                (uint32_t)sy >= runtime.map.height)
                continue;
            for (int i = 0; i < WALL_PIECES; ++i)
                free_cells += aoe_world_combat_placement_ror(
                                  &combat, 1, wall_t, (float)(sx + i) + 0.5f,
                                  (float)sy + 0.5f, flags, NULL) ==
                              AOE_PLACEMENT_OK;
            if (free_cells == WALL_PIECES) {
                x1 = sx;
                y1 = sy;
                found = 1;
            }
        }
    if (!found) {
        fprintf(stderr, "no free wall line\n");
        goto done;
    }
    /* Attribute 30 is set to 500 by the world game start (0x0050aaa0, not
       run by this test); resources 0..3 are raised to cover four pieces
       (0x004f1750 pays each one). */
    if (runtime.players[1].resources.count <= 30) goto done;
    runtime.players[1].resources.values[30] = 500.0f;
    for (int i = 0; i < 4; ++i)
        if (runtime.players[1].resources.values[i] < 200.0f)
            runtime.players[1].resources.values[i] = 200.0f;
    {
        uint8_t cmd[AOE_COMMAND_WALL_HEADER_ROR + 4];
        float before[4], expect[4];
        int32_t id = villager->core.unit_id;
        size_t editors0 = runtime.editor_unit_count;
        uint32_t placements0 = combat.placements;
        const DatGraphic *building_g =
            wall_t->construction_graphic >= 0 &&
                    (uint16_t)wall_t->construction_graphic < graphics.count
                ? &graphics.graphics[wall_t->construction_graphic]
                : NULL;
        size_t n;
        int r, count = 0, ordered;
        for (int i = 0; i < 4; ++i)
            before[i] = expect[i] = runtime.players[1].resources.values[i];
        for (int k = 0; k < 3; ++k) {
            const DatUnitCost *cost = &wall_t->costs[k];
            if (cost->resource_type >= 0 && cost->resource_type < 4 &&
                (uint8_t)cost->paid)
                expect[cost->resource_type] -=
                    (float)WALL_PIECES * (float)cost->amount;
            printf("wall cost %d: resource %d amount %d deduct %d\n", k,
                   cost->resource_type, cost->amount, (uint8_t)cost->paid);
        }
        n = aoe_command_encode_wall_ror(cmd, sizeof cmd, 1, (uint8_t)x1,
                                        (uint8_t)y1,
                                        (uint8_t)(x1 + WALL_PIECES - 1),
                                        (uint8_t)y1, WALL_SMALL, 0, &id, 1);
        r = n ? aoe_world_combat_command_wall_ror(&combat, cmd, n) : -1;
        for (size_t i = editors0; i < runtime.editor_unit_count; ++i) {
            AoeLiveUnit *u = runtime.editor_units[i];
            int cell;
            if (!u || !u->core.unit_template ||
                u->core.unit_template->id != WALL_SMALL)
                continue;
            cell = (int)floorf(u->core.position.x38) - x1;
            if (cell < 0 || cell >= WALL_PIECES ||
                (int)floorf(u->core.position.y3c) != y1 || pieces[cell]) {
                fprintf(stderr, "wall piece off the line at (%.2f,%.2f)\n",
                        u->core.position.x38, u->core.position.y3c);
                goto done;
            }
            pieces[cell] = u;
            ++count;
        }
        first = pieces[0];
        ordered = villager->derived_action_list &&
                  villager->derived_action_list->head != NULL;
        printf("wall (%d,%d)-(%d,%d): result %d placements +%u pieces %d "
               "failures %u ordered %d stone %.0f -> %.0f\n",
               x1, y1, x1 + WALL_PIECES - 1, y1, r,
               combat.placements - placements0, count,
               combat.placement_failures, ordered, before[2],
               runtime.players[1].resources.values[2]);
        if (r != 1 || count != WALL_PIECES ||
            combat.placements != placements0 + WALL_PIECES || !ordered) {
            fprintf(stderr, "wall placement failed\n");
            goto done;
        }
        for (int i = 0; i < 4; ++i)
            if (fabsf(runtime.players[1].resources.values[i] - expect[i]) >
                0.001f) {
                fprintf(stderr, "resource %d: %.2f, expected %.2f\n", i,
                        runtime.players[1].resources.values[i], expect[i]);
                goto done;
            }
        /* Each piece is a foundation of the wall: owner 1, state 0, at its
           cell centre, the construction graphic, and the facet +0x35 (and
           building +0x1dc angle) from template +0x16e. */
        for (int i = 0; i < WALL_PIECES; ++i) {
            AoeLiveUnit *u = pieces[i];
            printf("piece %d: unit %d at (%.2f,%.2f) state %d facet %u "
                   "graphic %s\n",
                   i, u->core.unit_id, u->core.position.x38,
                   u->core.position.y3c, (int)u->core.state,
                   u->core.scenario_frame,
                   u->graphic.current_graphic == (const void *)building_g
                       ? "construction" : "other");
            if (u->runtime_owner_index != 1 || u->core.state != 0 ||
                u->building_completed || !u->building_update ||
                fabsf(u->core.position.x38 - ((float)(x1 + i) + 0.5f)) > 0.01f ||
                fabsf(u->core.position.y3c - ((float)y1 + 0.5f)) > 0.01f ||
                u->core.scenario_frame != (uint8_t)wall_t->building_frame ||
                u->building_update->angle !=
                    (uint8_t)wall_t->building_frame ||
                (wall_t->train_time > 0 &&
                 u->graphic.current_graphic != (const void *)building_g)) {
                fprintf(stderr, "piece %d is not a wall foundation\n", i);
                goto done;
            }
        }
    }
    /* The villager walks to the first piece and builds it (0x004b13c0,
       work through 0x004ad7b0); completion selects the standing graphic,
       whose three angles are the wall facets. */
    {
        const DatGraphic *standing =
            wall_t->standing_graphic >= 0 &&
                    (uint16_t)wall_t->standing_graphic < graphics.count
                ? &graphics.graphics[wall_t->standing_graphic]
                : NULL;
        const DatGraphic *cg;
        int ticks = 0;
        uint8_t res = 0;
        while (ticks < 6000 && first->core.state == 0) {
            (void)aoe_world_combat_tick_ror(&combat, villager, 0.05f, &res);
            (void)aoe_world_combat_update_unit_ror(&combat, first, 0.05f);
            ++ticks;
        }
        printf("first piece built after %d ticks (train time %d s): state %d "
               "completed %u hp %.0f of %d facet %u, adjacency refreshes %u\n",
               ticks, wall_t->train_time, (int)first->core.state,
               first->building_completed, first->core.hit_points,
               (int)wall_t->hit_points, first->core.scenario_frame,
               runtime.adjacency_refreshes);
        cg = (const DatGraphic *)first->graphic.current_graphic;
        printf("first piece graphic: %s, %u angles\n",
               cg == standing ? "standing" : "other",
               cg ? (unsigned)cg->angle_count : 0u);
        if (first->core.state == 0 || !first->building_completed ||
            !standing || standing->angle_count != 3 || !cg ||
            cg->angle_count != 3) {
            fprintf(stderr, "the first wall piece was never completed\n");
            goto done;
        }
        for (int i = 1; i < WALL_PIECES; ++i)
            if (pieces[i]->core.state != 0 || pieces[i]->building_completed) {
                fprintf(stderr, "piece %d completed without work\n", i);
                goto done;
            }
    }
    ok = 1;
done:
    aoe_world_combat_destroy_ror(&combat);
    aoe_world_motion_destroy_ror(&motion);
    aoe_scenario_runtime_destroy_ror(&runtime);
    scn_free_map(&scenario);
    dat_free_civilizations(&civs);
    dat_free_techs(&techs);
    dat_free_effects(&effects);
    dat_free_random_maps(&random_maps);
    dat_free_borders(&borders);
    dat_free_terrains(&terrains);
    dat_free_graphics(&graphics);
    dat_free_sounds(&sounds);
    dat_free_intro(&intro);
    dat_free(&blob);
    return ok ? 0 : 1;
}
