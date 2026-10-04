#include "dat.h"
#include "scenario_runtime.h"
#include "scn.h"
#include "world_combat.h"

#include <stdio.h>
#include <string.h>

/* Builds, starts and ticks every scenario given on the command line with
   the patched RoR 1.0a rules: scenario load (players, units, including
   high-state records), game start, world motion/combat binding and a few
   world updates. */
int main(int argc, char **argv) {
    static DatBlob blob;
    static DatIntro intro;
    static DatSounds sounds;
    static DatGraphics graphics;
    static DatTerrains terrains;
    static DatBorders borders;
    static DatMapGeometry geometry;
    static DatRandomMaps random_maps;
    static DatEffects effects;
    static DatTechs techs;
    static DatCivilizations civs;
    int failures = 0;
    if (argc < 3) return 2;
    if (!dat_load(argv[1], &blob) || !dat_parse_intro(&blob, &intro) ||
        !dat_parse_sounds(&blob, &intro, &sounds) ||
        !dat_parse_graphics(&blob, &sounds, &graphics) ||
        !dat_parse_terrains(&blob, &intro, &graphics, &terrains) ||
        !dat_parse_borders(&blob, &terrains, &borders) ||
        !dat_parse_map_geometry(&blob, &terrains, &borders, &geometry) ||
        !dat_parse_random_maps(&blob, &geometry, &random_maps) ||
        !dat_parse_effects(&blob, &random_maps, &effects) ||
        !dat_parse_civilizations(&blob, &effects, &civs) ||
        !dat_parse_techs(&blob, &civs, &techs))
        return 1;
    for (int a = 2; a < argc; ++a) {
        static ScnMap scenario;
        static AoeScenarioRuntimeRor runtime;
        static AoeWorldMotionRor motion;
        static AoeWorldCombatRor combat;
        int ok;
        memset(&scenario, 0, sizeof(scenario));
        memset(&runtime, 0, sizeof(runtime));
        ok = scn_load_map(argv[a], (uint8_t)intro.terrain_count, &scenario) &&
             aoe_scenario_runtime_build_ror(&runtime, &scenario, &intro,
                                            &civs, &graphics, &terrains,
                                            &borders, &geometry, &techs,
                                            &effects, NULL, 1) &&
             aoe_scenario_runtime_start_game_ror(&runtime) &&
             aoe_scenario_runtime_set_active_player_ror(&runtime, 1) &&
             aoe_world_motion_init_ror(&motion, &runtime) &&
             aoe_world_combat_init_ror(&combat, &motion);
        for (int t = 0; ok && t < 20; ++t)
            ok = aoe_world_combat_update_world_ror(&combat, 0.05f);
        printf("%s: %s, %llu units, %u doppelganger requests\n", argv[a],
               ok ? "ok" : "FAILED",
               (unsigned long long)runtime.constructed_units,
               runtime.doppelgangers_requested);
        failures += !ok;
        if (combat.units) aoe_world_combat_destroy_ror(&combat);
        if (motion.units) aoe_world_motion_destroy_ror(&motion);
        aoe_scenario_runtime_destroy_ror(&runtime);
        scn_free_map(&scenario);
    }
    return failures ? 1 : 0;
}
