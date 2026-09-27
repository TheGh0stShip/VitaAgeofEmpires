#include "dat.h"
#include "player_templates.h"
#include "scenario_editor_catalog.h"

#include <inttypes.h>
#include <stdio.h>

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
    uint64_t visible = 0, hidden = 0;
    int ok = 0;
    if (argc != 2) return 2;
    if (!dat_load(argv[1], &blob) || !dat_parse_intro(&blob, &intro) ||
        !dat_parse_sounds(&blob, &intro, &sounds) ||
        !dat_parse_graphics(&blob, &sounds, &graphics) ||
        !dat_parse_terrains(&blob, &intro, &graphics, &terrains) ||
        !dat_parse_borders(&blob, &terrains, &borders) ||
        !dat_parse_map_geometry(&blob, &terrains, &borders, &geometry) ||
        !dat_parse_random_maps(&blob, &geometry, &random_maps) ||
        !dat_parse_effects(&blob, &random_maps, &effects) ||
        !dat_parse_civilizations(&blob, &effects, &civs)) goto done;
    for (uint16_t c = 0; c < civs.count; ++c) {
        AoePlayerTemplates templates = {0};
        size_t count;
        if (!aoe_player_templates_clone_civ(&civs.civs[c], &templates))
            goto done;
        count = aoe_scenario_editor_unit_count(&templates);
        visible += count;
        for (uint16_t id = 0; id < templates.count; ++id)
            if (templates.slots[id] && templates.slots[id]->hide_in_editor)
                ++hidden;
        for (size_t i = 0; i < count; ++i) {
            AoeScenarioEditorUnitEntry entry;
            if (!aoe_scenario_editor_unit_at(&templates, i, &entry) ||
                entry.object_id >= templates.count ||
                entry.unit != templates.slots[entry.object_id] ||
                entry.unit->hide_in_editor) {
                aoe_player_templates_free(&templates);
                goto done;
            }
        }
        aoe_player_templates_free(&templates);
    }
    printf("%s: editor catalogs expose %" PRIu64
           " civilization-unit templates and hide %" PRIu64 "\n",
           argv[1], visible, hidden);
    ok = visible != 0;
done:
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
