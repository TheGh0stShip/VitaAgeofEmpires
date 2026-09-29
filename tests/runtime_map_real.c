#include "dat.h"
#include "runtime_map.h"
#include "scn.h"
#include "terrain.h"

#include <inttypes.h>
#include <stdio.h>

/* Real installed scenario/DAT audit, not an alternate game runtime. */
int main(int argc, char **argv) {
    DatBlob blob = {0};
    DatIntro intro = {0};
    DatSounds sounds = {0};
    DatGraphics graphics = {0};
    DatTerrains terrains = {0};
    DatBorders borders = {0};
    DatMapGeometry geometry = {0};
    ScnMap scenario = {0};
    AoeRuntimeMap runtime = {0};
    AoePlayerVisionState vision = {0};
    uint64_t checked = 0;
    int ok = 0;
    if (argc != 3) {
        fprintf(stderr, "usage: runtime_map_real SCENARIO EMPIRES.DAT\n");
        return 2;
    }
    if (!dat_load(argv[2], &blob) ||
        !dat_parse_intro(&blob, &intro) ||
        !dat_parse_sounds(&blob, &intro, &sounds) ||
        !dat_parse_graphics(&blob, &sounds, &graphics) ||
        !dat_parse_terrains(&blob, &intro, &graphics, &terrains) ||
        !dat_parse_borders(&blob, &terrains, &borders) ||
        !dat_parse_map_geometry(&blob, &terrains, &borders, &geometry) ||
        !scn_load_map(argv[1], (uint8_t)intro.terrain_count, &scenario) ||
        !aoe_runtime_map_from_scenario(&runtime, &scenario, &geometry,
                                        &terrains, &borders) ||
        !aoe_player_vision_state_init(&vision, &runtime.masks, 0) ||
        !aoe_runtime_map_bind_player_vision(&runtime, &vision, 1))
        goto done;

    for (uint32_t y = 0; y < runtime.height; ++y) {
        for (uint32_t x = 0; x < runtime.width; ++x) {
            size_t i = (size_t)y * runtime.width + x;
            const ScnTile *source = scn_runtime_tile_at(&scenario, x, y);
            int32_t projected_x, projected_y;
            if (!source ||
                runtime.tiles[i].terrain_id != (source->terrain_id & 0x1f) ||
                runtime.tiles[i].elevation != (source->elevation & 7) ||
                runtime.shapes[i] != terrain_elevation_shape(
                    scenario.tiles, scenario.width, scenario.height, x, y) ||
                runtime.borders[i] != terrain_border_code(
                    scenario.tiles, scenario.width, scenario.height,
                    x, y, &terrains, &borders) ||
                !terrain_project_tile(scenario.tiles, scenario.width,
                                      scenario.height, x, y, &geometry,
                                      &projected_x, &projected_y) ||
                runtime.projected_x[i] != projected_x ||
                runtime.projected_y[i] != projected_y ||
                runtime.objects[i].count != 0 ||
                runtime.view_codes[i] != 0 ||
                runtime.cache_flags[i] != 0 ||
                runtime.draw_flags[i] != 0 ||
                runtime.masks.masks[i] != 0 ||
                vision.grid.view_codes != runtime.view_codes ||
                vision.grid.tile_masks != runtime.masks.masks) {
                fprintf(stderr, "%s: runtime map mismatch at (%u,%u)\n",
                        argv[1], x, y);
                goto done;
            }
            ++checked;
        }
    }
    printf("%s: %" PRIu64 " runtime tiles checked\n", argv[1], checked);
    ok = checked != 0;
done:
    aoe_player_vision_state_destroy(&vision);
    aoe_runtime_map_destroy(&runtime);
    scn_free_map(&scenario);
    dat_free_borders(&borders);
    dat_free_terrains(&terrains);
    dat_free_graphics(&graphics);
    dat_free_sounds(&sounds);
    dat_free_intro(&intro);
    dat_free(&blob);
    return ok ? 0 : 1;
}
