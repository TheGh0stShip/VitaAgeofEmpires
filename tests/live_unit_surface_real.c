#include "dat.h"
#include "live_unit_core.h"
#include "player_templates.h"
#include "scn.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Real-data path audit, not an alternate game/runtime. It uses only the
   original loader's verified square-map scenario-to-runtime tile order. */
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
    AoeOwnedVisibilityRegistry world = {0};
    uint64_t checked = 0, adjusted_z = 0, negative_ids = 0;
    int ok = 0;
    if (argc != 3) {
        fprintf(stderr, "usage: live_unit_surface_real SCENARIO EMPIRES.DAT\n");
        return 2;
    }
    if (!dat_load(argv[2], &blob) ||
        !dat_parse_intro(&blob, &intro) ||
        !dat_parse_sounds(&blob, &intro, &sounds) ||
        !dat_parse_graphics(&blob, &sounds, &graphics) ||
        !dat_parse_terrains(&blob, &intro, &graphics, &terrains) ||
        !dat_parse_borders(&blob, &terrains, &borders) ||
        !dat_parse_map_geometry(&blob, &terrains, &borders, &geometry) ||
        !dat_parse_random_maps(&blob, &geometry, &random_maps) ||
        !dat_parse_effects(&blob, &random_maps, &effects) ||
        !dat_parse_civilizations(&blob, &effects, &civs) ||
        !scn_load_map(argv[1], (uint8_t)intro.terrain_count, &scenario) ||
        !scn_runtime_tile_order_supported(&scenario) ||
        !aoe_owned_visibility_registry_init(&world))
        goto done;
    for (unsigned player = 0; player < 9; ++player) {
        uint32_t civ_id = player ?
            scenario.player_setup[player - 1].civilization_id : 0;
        AoePlayerTemplates templates = {0};
        float *resource_values = NULL;
        AoePlayerResources resources;
        const DatCiv *civ;
        const ScnPlayerUnits *placements = &scenario.player_units[player];
        if (civ_id >= civs.count) goto done;
        civ = &civs.civs[civ_id];
        resource_values = malloc((size_t)civ->resource_count * sizeof(float));
        if (!resource_values) goto done;
        memcpy(resource_values, civ->resources,
               (size_t)civ->resource_count * sizeof(float));
        resources = (AoePlayerResources){(int16_t)civ->resource_count,
                                          resource_values};
        if (!aoe_player_templates_clone_civ(civ, &templates)) {
            free(resource_values);
            goto done;
        }
        for (uint32_t i = 0; i < placements->count; ++i) {
            const ScnUnit *placement = &placements->units[i];
            DatUnit *unit = aoe_player_templates_get(&templates,
                                                      placement->object_id);
            AoeLiveUnitCore core;
            int32_t reserved_id;
            memcpy(&reserved_id, &placement->spawn_id, sizeof(reserved_id));
            world.reserved_id = reserved_id;
            world.special_id_mode = 1; /* Original scenario-load wrapper. */
            if (!unit || unit->id != (int16_t)placement->object_id ||
                !aoe_live_unit_core_begin(unit, &resources,
                                          AOE_PLAYER_RISE_OF_ROME, &world,
                                          placement->x, placement->y,
                                          placement->z, &core) ||
                core.unit_id != reserved_id ||
                !aoe_live_unit_core_apply_surface(&core, scenario.tiles,
                                                   scenario.width,
                                                   scenario.height)) {
                fprintf(stderr, "cannot initialize player %u placement %u\n",
                        player, i);
                aoe_player_templates_free(&templates);
                free(resource_values);
                goto done;
            }
            world.special_id_mode = 0;
            ++checked;
            adjusted_z += core.position.z40 != placement->z;
            negative_ids += core.unit_id < 0;
        }
        aoe_player_templates_free(&templates);
        free(resource_values);
    }
    printf("%s: %" PRIu64 " real placements checked, %" PRIu64
           " Z adjustments, %" PRIu64 " negative IDs\n",
           argv[1], checked, adjusted_z, negative_ids);
    ok = checked != 0;
done:
    aoe_owned_visibility_registry_destroy(&world);
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
