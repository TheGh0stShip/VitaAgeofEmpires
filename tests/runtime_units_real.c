#include "dat.h"
#include "live_unit_vision.h"
#include "player_templates.h"
#include "runtime_map.h"
#include "scn.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Audits the original constructor's recovered core and placement/vision
   phase on real records. Later state dispatch, registration, and behavior
   are deliberately not claimed here. */
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
    AoeRuntimeMap map = {0};
    AoeOwnedVisibilityRegistry world = {0};
    uint64_t placed = 0, visible = 0, projected = 0, dual_height = 0;
    int ok = 0;
    if (argc != 3) {
        fprintf(stderr, "usage: runtime_units_real SCENARIO EMPIRES.DAT\n");
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
        !aoe_runtime_map_from_scenario(&map, &scenario, &geometry,
                                        &terrains, &borders) ||
        !aoe_owned_visibility_registry_init(&world)) goto done;

    for (unsigned p = 0; p < 9; ++p) {
        uint32_t civ_id = p ? scenario.player_setup[p - 1].civilization_id : 0;
        const ScnPlayerUnits *records = &scenario.player_units[p];
        AoePlayerTemplates templates = {0};
        AoePlayerVisionState vision = {0};
        AoeLiveUnitCore *units = NULL;
        AoeLiveUnitPlacement *links = NULL;
        float *resource_values = NULL;
        AoePlayerResources resources = {0};
        AoeLiveUnitVisionBinding binding = {0};
        int player_ok = 0;
        if (civ_id >= civs.count) goto player_done;
        const DatCiv *civ = &civs.civs[civ_id];
        if (!aoe_player_templates_clone_civ(civ, &templates) ||
            !aoe_player_vision_state_init(&vision, &map.masks, (uint8_t)p))
            goto player_done;
        resource_values = malloc((size_t)civ->resource_count * sizeof(float));
        units = calloc(records->count ? records->count : 1, sizeof(*units));
        links = calloc(records->count ? records->count : 1, sizeof(*links));
        if (!resource_values || !units || !links) goto player_done;
        memcpy(resource_values, civ->resources,
               (size_t)civ->resource_count * sizeof(float));
        resources = (AoePlayerResources){(int16_t)civ->resource_count,
                                          resource_values};
        binding.owner_grid = &vision.grid;
        binding.owner_index = (int16_t)p;
        binding.active_index = -1; /* No active-player view during this audit. */
        binding.explored_ops.owner_context = &vision.owner_cells58;
        binding.explored_ops.record_owner = aoe_explored_cells_record;
        for (uint32_t i = 0; i < records->count; ++i) {
            const ScnUnit *record = &records->units[i];
            DatUnit *definition = aoe_player_templates_get(&templates,
                                                             record->object_id);
            int32_t reserved_id;
            memcpy(&reserved_id, &record->spawn_id, sizeof(reserved_id));
            world.reserved_id = reserved_id;
            world.special_id_mode = 1;
            if (!definition ||
                !aoe_live_unit_core_begin(definition, &resources,
                                          AOE_PLAYER_RISE_OF_ROME, &world,
                                          record->x, record->y, record->z,
                                          &units[i]) ||
                !aoe_live_unit_place_ror_with_vision(
                    &units[i], &links[i], map.tiles, map.objects, &geometry,
                    map.width, map.height, record->x, record->y,
                    record->z, &binding) ||
                vision.owner_cells58.allocation_failed ||
                units[i].unit_id != reserved_id || !links[i].tile ||
                !links[i].node) {
                fprintf(stderr, "%s: player %u record %u placement failed\n",
                        argv[1], p, i);
                goto player_done;
            }
            world.special_id_mode = 0;
            if ((definition->elevation_mode == 0 &&
                 (units[i].draw_left != units[i].draw_right ||
                  units[i].draw_top != units[i].draw_bottom)) ||
                (definition->elevation_mode == 1 &&
                 units[i].draw_left != units[i].draw_right)) {
                fprintf(stderr, "%s: player %u record %u draw offsets invalid\n",
                        argv[1], p, i);
                goto player_done;
            }
            if (units[i].draw_left || units[i].draw_top ||
                units[i].draw_right || units[i].draw_bottom) ++projected;
            if (definition->elevation_mode == 1 &&
                units[i].draw_top != units[i].draw_bottom) ++dual_height;
            ++placed;
        }
        visible += vision.grid.explored_count;
        player_ok = 1;
player_done:
        if (links && units)
            for (uint32_t i = 0; i < records->count; ++i)
                if (links[i].tile &&
                    !aoe_object_update_list_detach(links[i].tile, &units[i],
                                                   links[i].node)) player_ok = 0;
        free(links);
        free(units);
        free(resource_values);
        aoe_player_vision_state_destroy(&vision);
        aoe_player_templates_free(&templates);
        if (!player_ok) goto done;
    }
    printf("%s: %" PRIu64 " original unit cores/tile links checked, %"
           PRIu64 " projected offsets (%" PRIu64 " dual-height), %"
           PRIu64 " explored cells\n", argv[1], placed, projected,
           dual_height, visible);
    ok = placed != 0 && projected != 0;
done:
    aoe_owned_visibility_registry_destroy(&world);
    aoe_runtime_map_destroy(&map);
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
