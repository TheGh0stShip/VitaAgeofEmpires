#include "dat.h"
#include "live_unit_vision.h"
#include "scenario_runtime.h"
#include "scn.h"
#include "world_combat.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A player-1 melee unit attacks a player-2 unit on a real Rise of Rome
   map through the translated attack action, move sub-action, movement,
   A* search, turn-to-face, reload, melee hit roll and tribe damage. The
   target is first relocated (through the original placement routine) to a
   free tile 3..6 tiles from the attacker so the fight starts nearby. */
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
    DatCivilizations civs = {0};
    ScnMap scenario = {0};
    AoeScenarioRuntimeRor runtime = {0};
    AoeWorldMotionRor motion;
    AoeWorldCombatRor combat;
    AoeLiveUnit *attacker = NULL, *target = NULL;
    float start_hp = 0;
    int ok = 0, ticks = 0, placed = 0;
    uint8_t result = 0;
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
        !scn_load_map(argv[1], (uint8_t)intro.terrain_count, &scenario) ||
        !aoe_scenario_runtime_build_ror(&runtime, &scenario, &intro, &civs,
                                        &graphics, &terrains, &borders,
                                        &geometry, 1) ||
        !aoe_scenario_runtime_set_active_player_ror(&runtime, 1))
        return 1;
    if (!aoe_world_motion_init_ror(&motion, &runtime) ||
        !aoe_world_combat_init_ror(&combat, &motion))
        goto done;
    for (uint32_t i = 0; i < runtime.players[1].unit_count && !attacker; ++i) {
        AoeLiveUnit *u = &runtime.players[1].units[i];
        const DatUnit *t = u->core.unit_template;
        int has_attack = 0;
        if (!u->constructed || u->core.state != 2 || !t || t->type < 70 ||
            t->type == 80 || !(t->speed > 0.0f) || t->projectile_unit_id >= 0)
            continue;
        for (uint16_t k = 0; k < t->task_count; ++k)
            has_attack |= t->tasks[k].action_type == 7;
        if (has_attack && t->attack_count > 0) attacker = u;
    }
    for (int p = 2; p < AOE_SCENARIO_RUNTIME_PLAYERS && !target; ++p)
        for (uint32_t i = 0; i < runtime.players[p].unit_count && !target;
             ++i) {
            AoeLiveUnit *u = &runtime.players[p].units[i];
            const DatUnit *t = u->core.unit_template;
            if (u->constructed && u->core.state == 2 && t && t->type == 70 &&
                u->core.hit_points > 0.0f)
                target = u;
        }
    if (!attacker || !target) {
        fprintf(stderr, "no attacker/target pair\n");
        goto done;
    }
    /* Relocate the target near the attacker, keeping its footprint count. */
    {
        AoeWorldMotionUnitRor *ts = aoe_world_motion_unit_ror(&motion, target);
        (void)aoe_unit_obstruction_lift_ror(target, &ts->obstruction,
                                            &motion.path, NULL, 0);
        for (int r = 3; r <= 6 && !placed; ++r)
            for (int dy = -r; dy <= r && !placed; ++dy)
                for (int dx = -r; dx <= r && !placed; ++dx) {
                    float x = floorf(attacker->core.position.x38) + dx + 0.5f;
                    float y = floorf(attacker->core.position.y3c) + dy + 0.5f;
                    if (abs(dx) != r && abs(dy) != r) continue;
                    if (aoe_world_motion_fits_ror(&motion, target, x, y, 1) !=
                        1)
                        continue;
                    placed = aoe_live_unit_place_ror_with_vision(
                        &target->core, &target->placement, runtime.map.tiles,
                        runtime.map.objects, runtime.geometry,
                        runtime.map.width, runtime.map.height, x, y,
                        target->core.position.z40,
                        &runtime.players[target->runtime_owner_index]
                             .vision_binding);
                }
        (void)aoe_unit_obstruction_restore_ror(target, &ts->obstruction,
                                               &motion.path, NULL, 0);
    }
    if (!placed) goto done;
    start_hp = target->core.hit_points;
    printf("attacker %d (template %d, attack %d) vs target %d (template %d, "
           "%.1f hp) %.2f tiles apart\n",
           attacker->core.unit_id, attacker->core.unit_template->id,
           attacker->core.unit_template->attacks[0].amount,
           target->core.unit_id, target->core.unit_template->id, start_hp,
           hypotf(target->core.position.x38 - attacker->core.position.x38,
                  target->core.position.y3c - attacker->core.position.y3c));
    if (!aoe_world_combat_attack_ror(&combat, attacker, target)) goto done;
    for (ticks = 0; ticks < 6000; ++ticks) {
        if (!aoe_world_combat_tick_ror(&combat, attacker, 0.05f, &result))
            break;
        if (result || target->core.hit_points < start_hp - 0.001f) break;
    }
    printf("after %d ticks: result %u, state %u, target hp %.2f, hits %u, "
           "misses %u, %.2f tiles apart\n",
           ticks, result, combat.units[attacker->core.unit_id + 1000]
                              ->attack.base.state,
           target->core.hit_points, combat.hits, combat.misses,
           hypotf(target->core.position.x38 - attacker->core.position.x38,
                  target->core.position.y3c - attacker->core.position.y3c));
    ok = target->core.hit_points < start_hp && combat.hits > 0;
    /* Keep fighting until the tribe receive clamps the target to 0. The
       reload (template +0x120) resets as each shot starts while the hit
       lands at the fire frame of an animation that is not restarted, so
       single gaps may be shorter than the reload but the average is not.
       25 hp at 3 damage (armor 0) takes 9 hits. */
    if (ok) {
        uint32_t last_hits = combat.hits;
        int last_tick = ticks, first_tick = ticks, min_gap = 1 << 30;
        for (int t = 0; t < 6000 && target->core.hit_points > 0.0f; ++t) {
            ++ticks;
            if (!aoe_world_combat_tick_ror(&combat, attacker, 0.05f,
                                           &result))
                break;
            if (combat.hits != last_hits) {
                if (ticks - last_tick < min_gap) min_gap = ticks - last_tick;
                last_tick = ticks;
                last_hits = combat.hits;
            }
        }
        float average = combat.hits > 1
                            ? (last_tick - first_tick) * 0.05f /
                                  (float)(combat.hits - 1)
                            : 0.0f;
        printf("fight over after %d ticks: target hp %.2f, %u hits, %u "
               "misses, gaps %.2f s shortest / %.2f s average (reload "
               "%.2f s)\n",
               ticks, target->core.hit_points, combat.hits, combat.misses,
               min_gap * 0.05f, average,
               attacker->core.unit_template->reload_time);
        ok = target->core.hit_points == 0.0f && combat.hits == 9 &&
             !combat.misses &&
             average + 0.05f >= attacker->core.unit_template->reload_time;
    }
    /* Death: the target's own state switch (0x004a8f40) enters dying
       (state 3) below 1.0 hit points, and the attacker's action then drops
       its dead target (result 5). */
    if (ok) {
        uint8_t states[8], n = 0, last = target->core.state;
        int attacker_result = -1;
        states[n++] = last;
        for (int t = 0; t < 400 && n < 8; ++t) {
            if (!aoe_world_combat_update_unit_ror(&combat, target, 0.05f))
                break;
            if (target->core.state != last) {
                last = target->core.state;
                states[n++] = last;
            }
            if (attacker_result < 0) {
                (void)aoe_world_combat_tick_ror(&combat, attacker, 0.05f,
                                                &result);
                if (result) attacker_result = result;
            }
        }
        printf("target states:");
        for (uint8_t i = 0; i < n; ++i) printf(" %u", states[i]);
        printf("; attacker result %d, kills %u\n", attacker_result,
               combat.kills);
        ok = n >= 2 && states[0] == 2 && states[1] == 3 &&
             attacker_result == 5 && combat.kills == 1;
    }
done:
    aoe_world_combat_destroy_ror(&combat);
    aoe_world_motion_destroy_ror(&motion);
    aoe_scenario_runtime_destroy_ror(&runtime);
    scn_free_map(&scenario);
    dat_free_civilizations(&civs);
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
