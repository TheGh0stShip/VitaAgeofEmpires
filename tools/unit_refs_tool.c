#include "dat.h"
#include "scn.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    DatBlob blob = {0};
    DatIntro intro = {0};
    DatSounds sounds = {0};
    DatGraphics graphics = {0};
    DatTerrains terrains = {0};
    DatBorders borders = {0};
    DatMapGeometry geometry = {0};
    DatRandomMaps maps = {0};
    DatEffects effects = {0};
    DatCivilizations civs = {0};
    ScnMap scenario = {0};
    unsigned long checked = 0, missing = 0;
    uint64_t state_by_type[3][256] = {{0}};
    int state_summary = argc == 4 && strcmp(argv[3], "--spawn-states") == 0;
    int ok = 0;
    if (argc != 3 && !state_summary) {
        fprintf(stderr, "usage: unit_refs_tool SCENARIO EMPIRES.DAT [--spawn-states]\n");
        return 2;
    }
    if (!dat_load(argv[2], &blob) ||
        !dat_parse_intro(&blob, &intro) ||
        !dat_parse_sounds(&blob, &intro, &sounds) ||
        !dat_parse_graphics(&blob, &sounds, &graphics) ||
        !dat_parse_terrains(&blob, &intro, &graphics, &terrains) ||
        !dat_parse_borders(&blob, &terrains, &borders) ||
        !dat_parse_map_geometry(&blob, &terrains, &borders, &geometry) ||
        !dat_parse_random_maps(&blob, &geometry, &maps) ||
        !dat_parse_effects(&blob, &maps, &effects) ||
        !dat_parse_civilizations(&blob, &effects, &civs) ||
        !scn_load_map(argv[1], (uint8_t)intro.terrain_count, &scenario)) {
        fprintf(stderr, "cannot load scenario or unit definitions\n");
        goto done;
    }
    for (unsigned player = 0; player < 9; ++player) {
        uint32_t civilization = player ?
            scenario.player_setup[player - 1].civilization_id : 0;
        const ScnPlayerUnits *placements = &scenario.player_units[player];
        for (uint32_t i = 0; i < placements->count; ++i) {
            const ScnUnit *placement = &placements->units[i];
            const DatUnit *unit = civilization <= UINT16_MAX ?
                dat_find_unit(&civs, (uint16_t)civilization,
                              placement->object_id) : NULL;
            ++checked;
            if (!unit || unit->id != (int16_t)placement->object_id) {
                if (missing < 8)
                    printf("missing player %u civ %u unit %u at %.1f, %.1f\n",
                           player, civilization, placement->object_id,
                           placement->x, placement->y);
                ++missing;
            } else {
                unsigned branch = placement->state >= 7 ? 2 :
                                  placement->state >= 3 ? 1 : 0;
                state_by_type[branch][unit->type]++;
            }
        }
    }
    printf("%s: %lu placed units, %lu missing definitions\n", argv[1],
           checked, missing);
    if (state_summary) {
        for (unsigned type = 0; type < 256; ++type) {
            uint64_t low = state_by_type[0][type];
            uint64_t middle = state_by_type[1][type];
            uint64_t high = state_by_type[2][type];
            if (low || middle || high)
                printf("type %u: 0-2=%" PRIu64 " 3-6=%" PRIu64
                       " 7+=%" PRIu64 "\n", type, low, middle, high);
        }
    }
    ok = missing == 0;
done:
    scn_free_map(&scenario);
    dat_free_civilizations(&civs);
    dat_free_effects(&effects);
    dat_free_random_maps(&maps);
    dat_free_borders(&borders);
    dat_free_terrains(&terrains);
    dat_free_graphics(&graphics);
    dat_free_sounds(&sounds);
    dat_free_intro(&intro);
    dat_free(&blob);
    return ok ? 0 : 1;
}
