#include "dat.h"
#include "scenario_runtime.h"
#include "scn.h"
#include "world_motion.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Drives translated movement start/update/step and the A* search on a real
   Rise of Rome campaign map. For each terrain restriction class present
   among player 1's mobile units, one unit is ordered (range 0.4, as the
   move action constructor 0x00405250 substitutes for a non-positive range)
   to a free tile centre 6..16 steps away and ticked until it stops. With
   range 0 the 8-facet steps can circle a point indefinitely. */
static int drive(AoeWorldMotionRor *motion, AoeScenarioRuntimeRor *runtime,
                 AoeLiveUnit *u) {
    float gx = 0, gy = 0, start_d, d, ex, ey, edge;
    int found = 0, ticks, r = -1;
    for (int dy = -8; dy <= 8 && !found; ++dy)
        for (int dx = -8; dx <= 8 && !found; ++dx) {
            float x = floorf(u->core.position.x38) + dx + 0.5f;
            float y = floorf(u->core.position.y3c) + dy + 0.5f;
            if (abs(dx) + abs(dy) < 6 || x < 1 || y < 1 ||
                x >= runtime->map.width - 1 || y >= runtime->map.height - 1)
                continue;
            if (aoe_world_motion_fits_ror(motion, u, x, y, 1) == 1) {
                found = 1;
                gx = x;
                gy = y;
            }
        }
    if (!found) return -1; /* boxed in: not a failure of movement */
    start_d = hypotf(gx - u->core.position.x38, gy - u->core.position.y3c);
    if (aoe_world_motion_move_to_ror(motion, u, gx, gy, 0.4f, -1) != 0)
        return 0;
    for (ticks = 0; ticks < 4000; ++ticks)
        if ((r = aoe_world_motion_tick_ror(motion, u, 0.05f)) != 1) break;
    d = hypotf(gx - u->core.position.x38, gy - u->core.position.y3c);
    ex = fabsf(gx - u->core.position.x38) - u->core.unit_template->size[0];
    ey = fabsf(gy - u->core.position.y3c) - u->core.unit_template->size[1];
    edge = hypotf(ex > 0 ? ex : 0, ey > 0 ? ey : 0);
    printf("unit %d (template %d, restriction %d): %.2f tiles, %d ticks, "
           "result %d, edge distance %.3f\n",
           u->core.unit_id, u->core.unit_template->id,
           u->core.unit_template->terrain_restriction, start_d, ticks, r,
           edge);
    return r == 2 && edge <= 0.4f && d < start_d;
}

int main(int argc, char **argv) {
    DatBlob blob = {0};
    DatIntro intro = {0};
    DatSounds sounds = {0};
    DatGraphics graphics = {0};
    DatTerrains terrains = {0};
    DatBorders borders = {0};
    DatMapGeometry geometry = {0};
    DatRandomMaps random_maps = {0};
    DatEffects effects = {0}; DatTechs techs = {0};
    DatCivilizations civs = {0};
    ScnMap scenario = {0};
    AoeScenarioRuntimeRor runtime = {0};
    AoeWorldMotionRor motion;
    int16_t seen[16];
    int seen_count = 0, driven = 0, ok = 0;
    memset(&motion, 0, sizeof(motion));
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
        !aoe_scenario_runtime_set_active_player_ror(&runtime, 1)) {
        fprintf(stderr, "load failed\n");
        return 1;
    }
    if (!aoe_world_motion_init_ror(&motion, &runtime)) goto done;
    ok = 1;
    for (uint32_t i = 0; i < runtime.players[1].unit_count && ok; ++i) {
        AoeLiveUnit *u = &runtime.players[1].units[i];
        const DatUnit *t = u->core.unit_template;
        int known = 0, r;
        if (!u->constructed || u->core.state != 2 || !t || t->type < 30 ||
            !(t->speed > 0.0f) || !(t->size[2] > 0.0f))
            continue;
        for (int k = 0; k < seen_count; ++k)
            known |= seen[k] == t->terrain_restriction;
        if (known || seen_count == 16) continue;
        r = drive(&motion, &runtime, u);
        if (r < 0) continue;
        seen[seen_count++] = t->terrain_restriction;
        ++driven;
        ok = r == 1;
    }
    printf("%d restriction classes driven, %u searches\n", driven,
           motion.searches);
    ok = ok && driven >= 2;
done:
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
