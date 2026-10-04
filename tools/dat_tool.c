#include "dat.h"
#include "terrain_art.h"
#include "terrain.h"
#include "unit_template_factory.h"
#include "player_templates.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int dump_bytes(const DatBlob *blob, const char *offset_text,
                      const char *length_text) {
    char *end;
    unsigned long long offset, length;
    errno = 0;
    offset = strtoull(offset_text, &end, 0);
    if (errno || end == offset_text || *end || offset > blob->size)
        return 0;
    errno = 0;
    length = strtoull(length_text, &end, 0);
    if (errno || end == length_text || *end || length > blob->size - offset)
        return 0;
    for (unsigned long long i = 0; i < length; i += 16) {
        printf("%08llx:", offset + i);
        for (unsigned long long j = 0; j < 16 && j < length - i; ++j)
            printf(" %02x", blob->bytes[offset + i + j]);
        putchar('\n');
    }
    return 1;
}

static int parse_id(const char *text, uint16_t *value) {
    char *end;
    unsigned long n;
    if (!text || *text == '-') return 0;
    errno = 0;
    n = strtoul(text, &end, 0);
    if (errno || end == text || *end || n > UINT16_MAX) return 0;
    *value = (uint16_t)n;
    return 1;
}

int main(int argc, char **argv) {
    DatBlob blob;
    DatIntro intro;
    DatSounds sounds;
    DatGraphics graphics;
    DatTerrains terrains;
    DatBorders borders;
    DatMapGeometry geometry;
    DatRandomMaps random_maps;
    DatEffects effects;
    DatCivilizations civilizations;
    DatTechs techs;
    unsigned total_unsupported_factory_types = 0;
    unsigned failed_player_clones = 0;
    uint16_t query_civ = 0, query_unit = 0;
    uint16_t query_terrain = 0, query_shape = 0, query_x = 0, query_y = 0;
    uint16_t query_border = 0, query_pattern = 0;
    uint16_t query_draw_terrain = 0;
    int unit_query = argc == 5;
    int factory_audit = argc == 3 &&
        strcmp(argv[2], "--factory-audit") == 0;
    int graphics_all = argc == 3 &&
        strcmp(argv[2], "--graphics-all") == 0;
    int terrain_units = argc == 3 &&
        strcmp(argv[2], "--terrain-units") == 0;
    int art_query = argc == 4 && strcmp(argv[2], "--terrain-art") == 0;
    int border_art_query = argc == 4 && strcmp(argv[2], "--border-art") == 0;
    int draw_terrain_query = argc == 4 &&
        strcmp(argv[2], "--draw-terrain") == 0;
    int frame_query = argc == 7;
    int border_frame_query = argc == 8;
    if ((argc != 2 && argc != 3 && argc != 4 && argc != 5 && argc != 7 && argc != 8) ||
        (argc == 3 && !factory_audit && !graphics_all && !terrain_units) ||
        (unit_query && (strcmp(argv[2], "--unit") != 0 ||
                        !parse_id(argv[3], &query_civ) ||
                        !parse_id(argv[4], &query_unit))) ||
        (frame_query && (strcmp(argv[2], "--terrain-frame") != 0 ||
                         !parse_id(argv[3], &query_terrain) ||
                         !parse_id(argv[4], &query_shape) ||
                         !parse_id(argv[5], &query_x) ||
                         !parse_id(argv[6], &query_y))) ||
        (draw_terrain_query && !parse_id(argv[3], &query_draw_terrain)) ||
        (border_frame_query &&
         (strcmp(argv[2], "--border-frame") != 0 ||
          !parse_id(argv[3], &query_border) ||
          !parse_id(argv[4], &query_shape) ||
          !parse_id(argv[5], &query_pattern) ||
          !parse_id(argv[6], &query_x) ||
          !parse_id(argv[7], &query_y)))) {
        fprintf(stderr, "usage: dat_tool FILE [--factory-audit | --graphics-all | --terrain-units | OFFSET LENGTH | --unit CIV_ID UNIT_ID | --terrain-art TERRAIN.DRS | --border-art BORDER.DRS | --draw-terrain TERRAIN | --terrain-frame TERRAIN SHAPE X Y | --border-frame BORDER SHAPE PATTERN X Y]\n");
        return 2;
    }
    if (!dat_load(argv[1], &blob)) {
        fprintf(stderr, "cannot decompress VER 3.7 game data\n");
        return 1;
    }
    if (argc == 4 && !art_query && !border_art_query &&
        !draw_terrain_query) {
        int ok = dump_bytes(&blob, argv[2], argv[3]);
        dat_free(&blob);
        if (!ok) fprintf(stderr, "invalid dump range\n");
        return ok ? 0 : 2;
    }
    printf("version: %.7s\ndecompressed bytes: %zu\nfirst 32 data bytes:",
           (const char *)blob.bytes, blob.size);
    for (size_t i = 8; i < 40 && i < blob.size; ++i)
        printf(" %02x", blob.bytes[i]);
    putchar('\n');
    if (!dat_parse_intro(&blob, &intro)) {
        fprintf(stderr, "cannot parse game data tables\n");
        dat_free(&blob);
        return 1;
    }
    printf("terrain restrictions: %u x %u\ncolor records: %u\n",
           intro.restriction_count, intro.terrain_count, intro.color_count);
    for (uint16_t i = 0; i < intro.color_count; ++i)
        printf("color %u: %s (value %u)\n", intro.colors[i].ordinal,
               intro.colors[i].name, intro.colors[i].value);
    printf("next section offset: 0x%zx\n", intro.next_offset);
    if (!dat_parse_sounds(&blob, &intro, &sounds)) {
        fprintf(stderr, "cannot parse sound table\n");
        dat_free_intro(&intro);
        dat_free(&blob);
        return 1;
    }
    printf("sounds: %u records / %zu variants\n", sounds.count,
           sounds.variant_count);
    for (uint16_t i = 0; i < sounds.count; ++i) {
        const DatSound *sound = &sounds.sounds[i];
        if (i != 0 && sound->variant_count <= 1 && i != sounds.count - 1) continue;
        printf("sound %u:", sound->ordinal);
        for (uint16_t j = 0; j < sound->variant_count; ++j) {
            const DatSoundVariant *variant = &sound->variants[j];
            printf(" %s=%u (%u)", variant->filename,
                   variant->resource_id, variant->weight);
        }
        putchar('\n');
    }
    printf("next section offset: 0x%zx\n", sounds.next_offset);
    if (!dat_parse_graphics(&blob, &sounds, &graphics)) {
        fprintf(stderr, "cannot parse graphic table\n");
        dat_free_sounds(&sounds);
        dat_free_intro(&intro);
        dat_free(&blob);
        return 1;
    }
    printf("graphics: %u slots / %zu records\n", graphics.count,
           graphics.present_count);
    for (uint16_t i = 0; i < graphics.count; ++i) {
        const DatGraphic *graphic = &graphics.graphics[i];
        if (!graphic->present || (!graphics_all &&
            i >= 8 && i < graphics.count - 1)) continue;
        printf("graphic %u: %s (%s), SLP %d, %u frames x %u angles\n",
               i, graphic->name, graphic->filename, graphic->slp_id,
               graphic->frame_count, graphic->angle_count);
    }
    printf("next section offset: 0x%zx\n", graphics.next_offset);
    if (!dat_parse_terrains(&blob, &intro, &graphics, &terrains)) {
        fprintf(stderr, "cannot parse terrain table\n");
        dat_free_graphics(&graphics);
        dat_free_sounds(&sounds);
        dat_free_intro(&intro);
        dat_free(&blob);
        return 1;
    }
    printf("terrains: %u enabled / %u slots\n", terrains.enabled_count,
           terrains.count);
    if (terrain_units) {
        for (uint16_t i = 0; i < terrains.count; ++i) {
            const DatTerrain *terrain = &terrains.terrains[i];
            printf("terrain %u %s: %u unit entries", i, terrain->name,
                   terrain->terrain_units_used_count);
            for (uint16_t j = 0; j < terrain->terrain_units_used_count; ++j)
                printf(" %d/%d/%u", terrain->terrain_unit_ids[j],
                       terrain->terrain_unit_density[j],
                       terrain->terrain_placement_flags[j]);
            putchar('\n');
        }
    }
    for (uint16_t i = 0; i < terrains.count; ++i) {
        const DatTerrain *terrain = &terrains.terrains[i];
        if (terrain->enabled)
            printf("terrain %u: %s (%s), SLP %d, map color %u, "
                   "replacement %d, frame grid %d/%d, passability %d/%d, "
                   "base frames %d/%d/%d\n",
                   i, terrain->name, terrain->filename, terrain->slp_id,
                   terrain->map_colors[0], terrain->replacement_id,
                   terrain->frame_grid[0], terrain->frame_grid[1],
                   terrain->passable_terrain, terrain->impassable_terrain,
                   terrain->elevation_frames[0].frame_count,
                   terrain->elevation_frames[0].angle_count,
                   terrain->elevation_frames[0].frame_start);
    }
    printf("next section offset: 0x%zx\n", terrains.next_offset);
    if (draw_terrain_query) {
        uint8_t draw_id;
        int ok = query_draw_terrain <= UINT8_MAX &&
                 terrain_resolve_draw_id(&terrains,
                                         (uint8_t)query_draw_terrain,
                                         &draw_id);
        if (ok)
            printf("terrain draw %u: terrain %u\n", query_draw_terrain,
                   draw_id);
        else
            fprintf(stderr, "invalid draw-terrain query\n");
        dat_free_terrains(&terrains);
        dat_free_graphics(&graphics);
        dat_free_sounds(&sounds);
        dat_free_intro(&intro);
        dat_free(&blob);
        return ok ? 0 : 2;
    }
    if (frame_query) {
        uint32_t frame;
        int ok = query_terrain < terrains.count && query_shape <= 16 &&
                 terrain_select_frame(&terrains.terrains[query_terrain],
                                      (uint8_t)query_shape, query_x, query_y,
                                      &frame);
        if (ok)
            printf("terrain frame %u shape %u tile %u,%u: SLP frame %u\n",
                   query_terrain, query_shape, query_x, query_y, frame);
        else
            fprintf(stderr, "invalid terrain frame query\n");
        dat_free_terrains(&terrains);
        dat_free_graphics(&graphics);
        dat_free_sounds(&sounds);
        dat_free_intro(&intro);
        dat_free(&blob);
        return ok ? 0 : 2;
    }
    if (art_query) {
        DrsArchive archive = {0};
        DrsArchive *archives[1] = {&archive};
        TerrainArtCache cache = {0};
        int ok = drs_open(&archive, argv[3]) &&
                 terrain_art_load_direct(&cache, &terrains, archives, 1);
        if (ok) for (uint16_t i = 0; i < cache.count; ++i) {
            const TerrainArtEntry *art = &cache.entries[i];
            if (art->frames)
                printf("terrain art %u: SLP %d, %u decoded frames\n",
                       i, art->slp_id, art->frame_count);
        }
        if (!ok) fprintf(stderr, "cannot load direct terrain art\n");
        terrain_art_free(&cache);
        drs_close(&archive);
        dat_free_terrains(&terrains);
        dat_free_graphics(&graphics);
        dat_free_sounds(&sounds);
        dat_free_intro(&intro);
        dat_free(&blob);
        return ok ? 0 : 1;
    }
    if (!dat_parse_borders(&blob, &terrains, &borders)) {
        fprintf(stderr, "cannot parse border table\n");
        dat_free_terrains(&terrains);
        dat_free_graphics(&graphics);
        dat_free_sounds(&sounds);
        dat_free_intro(&intro);
        dat_free(&blob);
        return 1;
    }
    printf("borders: %u enabled / %u slots\n", borders.enabled_count,
           borders.count);
    for (uint16_t i = 0; i < borders.count; ++i) {
        const DatBorder *border = &borders.borders[i];
        if (border->enabled)
            printf("border %u: %s (%s), SLP %d, style %d, draw terrain %d, underlay %d\n",
                   i, border->name, border->filename, border->slp_id,
                   border->border_style, border->draw_terrain,
                   border->underlay_terrain);
    }
    printf("next section offset: 0x%zx\n", borders.next_offset);
    if (border_art_query) {
        DrsArchive archive = {0};
        DrsArchive *archives[1] = {&archive};
        BorderArtCache cache = {0};
        int ok = drs_open(&archive, argv[3]) &&
                 border_art_load_direct(&cache, &borders, archives, 1);
        if (ok) for (uint16_t i = 0; i < borders.count; ++i) {
            const DatBorder *border = &borders.borders[i];
            const BorderArtEntry *art = &cache.entries[i];
            if (!border->enabled) continue;
            if (art->frames)
                printf("border art %u: SLP %d, %u decoded frames\n",
                       i, art->slp_id, art->frame_count);
            else
                printf("border art %u: SLP %d unavailable\n",
                       i, border->slp_id);
        }
        if (!ok) fprintf(stderr, "cannot load direct border art\n");
        border_art_free(&cache);
        drs_close(&archive);
        dat_free_borders(&borders);
        dat_free_terrains(&terrains);
        dat_free_graphics(&graphics);
        dat_free_sounds(&sounds);
        dat_free_intro(&intro);
        dat_free(&blob);
        return ok ? 0 : 1;
    }
    if (border_frame_query) {
        uint32_t frame;
        int ok = query_border < borders.count && query_shape <= 16 &&
                 query_pattern <= 15 &&
                 terrain_select_border_frame(&borders.borders[query_border],
                                             (uint8_t)query_shape,
                                             (uint8_t)query_pattern,
                                             query_x, query_y, &frame);
        if (ok)
            printf("border frame %u shape %u pattern %u tile %u,%u: SLP frame %u\n",
                   query_border, query_shape, query_pattern, query_x, query_y,
                   frame);
        else
            fprintf(stderr, "invalid or absent border frame\n");
        dat_free_borders(&borders);
        dat_free_terrains(&terrains);
        dat_free_graphics(&graphics);
        dat_free_sounds(&sounds);
        dat_free_intro(&intro);
        dat_free(&blob);
        return ok ? 0 : 2;
    }
    if (!dat_parse_map_geometry(&blob, &terrains, &borders, &geometry)) {
        fprintf(stderr, "cannot parse map geometry\n");
        dat_free_borders(&borders);
        dat_free_terrains(&terrains);
        dat_free_graphics(&graphics);
        dat_free_sounds(&sounds);
        dat_free_intro(&intro);
        dat_free(&blob);
        return 1;
    }
    printf("map geometry: %d x %d tile, %d x %d half-step, "
           "%d elevation step\n", geometry.tile_width, geometry.tile_height,
           geometry.tile_half_width, geometry.tile_half_height,
           geometry.elevation_height);
    printf("next section offset: 0x%zx\n", geometry.next_offset);
    if (!dat_parse_random_maps(&blob, &geometry, &random_maps)) {
        fprintf(stderr, "cannot parse random-map definitions\n");
        dat_free_borders(&borders);
        dat_free_terrains(&terrains);
        dat_free_graphics(&graphics);
        dat_free_sounds(&sounds);
        dat_free_intro(&intro);
        dat_free(&blob);
        return 1;
    }
    printf("random maps: %u\n", random_maps.count);
    for (uint32_t i = 0; i < random_maps.count; ++i) {
        const DatRandomMap *map = &random_maps.maps[i];
        printf("random map %d: %u lands, %u terrains, %u units, %u elevations\n",
               map->map_id, map->land_count, map->terrain_count,
               map->unit_count, map->elevation_count);
        printf("  margins %d %d %d %d, +0x14 %d, land edge distance %d, "
               "+0x1c %d, land percent %d, +0x24 %d\n",
               map->edge_margin[0], map->edge_margin[1], map->edge_margin[2],
               map->edge_margin[3], map->field_14, map->land_edge_distance,
               map->field_1c, map->land_percent, map->field_24);
        for (uint32_t l = 0; l < map->land_count; ++l) {
            const DatRandomMapLand *land = &map->lands[l];
            printf("  land %u: id %d, +0x04 %u, placement %u at %d,%d, "
                   "area %u%%, per player %u, +0x10 %u, +0x20 %d, "
                   "+0x24 %d, +0x28 %d\n",
                   l, land->id, land->field_04, land->placement_mode,
                   land->x, land->y, land->area_percent, land->per_player,
                   land->field_10, land->field_20, land->field_24,
                   land->field_28);
        }
    }
    printf("next section offset: 0x%zx\n", random_maps.next_offset);
    if (!dat_parse_effects(&blob, &random_maps, &effects)) {
        fprintf(stderr, "cannot parse effect definitions\n");
        dat_free_random_maps(&random_maps);
        dat_free_borders(&borders);
        dat_free_terrains(&terrains);
        dat_free_graphics(&graphics);
        dat_free_sounds(&sounds);
        dat_free_intro(&intro);
        dat_free(&blob);
        return 1;
    }
    printf("effects: %u\n", effects.count);
    for (uint32_t i = 0; i < effects.count; ++i) {
        const DatEffect *effect = &effects.effects[i];
        if (i < 3 || i == effects.count - 1)
            printf("effect %u: %s (%u commands)\n", i, effect->name,
                   effect->command_count);
    }
    printf("next section offset: 0x%zx\n", effects.next_offset);
    if (!dat_parse_civilizations(&blob, &effects, &civilizations)) {
        fprintf(stderr, "cannot parse civilization and unit definitions\n");
        dat_free_effects(&effects);
        dat_free_random_maps(&random_maps);
        dat_free_borders(&borders);
        dat_free_terrains(&terrains);
        dat_free_graphics(&graphics);
        dat_free_sounds(&sounds);
        dat_free_intro(&intro);
        dat_free(&blob);
        return 1;
    }
    printf("civilizations: %u\n", civilizations.count);
    for (uint16_t i = 0; i < civilizations.count; ++i) {
        const DatCiv *civ = &civilizations.civs[i];
        unsigned present = 0, mismatched_ids = 0, unsupported = 0;
        unsigned negative_id = 0, elevation_modes[3] = {0};
        for (uint16_t j = 0; j < civ->unit_count; ++j)
            if (civ->units[j].present) {
                AoeUnitTemplateFactorySpec spec;
                ++present;
                negative_id += civ->units[j].negative_id_mode != 0;
                if (civ->units[j].elevation_mode < 2)
                    ++elevation_modes[civ->units[j].elevation_mode];
                else
                    ++elevation_modes[2];
                mismatched_ids += civ->units[j].id != (int16_t)j;
                if (!aoe_unit_template_factory_spec((uint8_t)civ->player_type,
                                                    civ->units[j].type, &spec)) {
                    ++unsupported;
                    ++total_unsupported_factory_types;
                    if (unsupported <= 5)
                        printf("  unhandled unit %u: %s, type %u\n", j,
                               civ->units[j].name, civ->units[j].type);
                }
            }
        printf("civilization %u: %s, type %d, %u/%u units, %u resources, "
               "%u slot/ID mismatches, %u unhandled factory types, "
               "%u negative-ID definitions, elevation modes %u/%u/%u+\n",
               i, civ->name, civ->player_type, present, civ->unit_count,
               civ->resource_count, mismatched_ids, unsupported, negative_id,
               elevation_modes[0], elevation_modes[1], elevation_modes[2]);
        if (factory_audit && !unsupported) {
            AoePlayerTemplates player = {0};
            if (!aoe_player_templates_clone_civ(civ, &player)) {
                ++failed_player_clones;
            } else {
                for (uint16_t j = 0; j < civ->unit_count; ++j) {
                    const DatUnit *copy = aoe_player_templates_get(&player, j);
                    if (!!copy != !!civ->units[j].present ||
                        (copy && (copy->id != civ->units[j].id ||
                                  copy->type != civ->units[j].type))) {
                        ++failed_player_clones;
                        break;
                    }
                }
            }
            aoe_player_templates_free(&player);
        }
    }
    printf("factory audit: %u unhandled definitions\n",
           total_unsupported_factory_types);
    if (factory_audit)
        printf("player-template clone audit: %u failed civilizations\n",
               failed_player_clones);
    if (civilizations.count && civilizations.civs[0].unit_count &&
        civilizations.civs[0].units[0].present)
        printf("Gaia unit 0: %s, type %u, standing graphic %d\n",
               civilizations.civs[0].units[0].name,
               civilizations.civs[0].units[0].type,
               civilizations.civs[0].units[0].standing_graphic);
    if (unit_query) {
        const DatUnit *unit = dat_find_unit(&civilizations, query_civ,
                                            query_unit);
        if (unit)
            printf("unit %u/%u: %s, type %u, class %d, HP %d, "
                   "standing graphic %d, walking graphic %d, speed %.3f, "
                   "%u tasks, %u attacks, %u armors, train time %d, "
                   "size %.3f/%.3f/%.3f, state effects %u/%u/%u, "
                   "death %d sound %d, undead %d mode %u, decay %.3f\n",
                   query_civ, query_unit, unit->name, unit->type,
                   unit->unit_class, unit->hit_points,
                   unit->standing_graphic, unit->walking_graphic,
                   unit->speed, unit->task_count, unit->attack_count,
                   unit->armor_count, unit->train_time,
                   unit->size[0], unit->size[1], unit->size[2],
                   unit->state_notify_mode, unit->state_effect_mode,
                   unit->state_notify_value, unit->dying_graphic,
                   unit->dying_sound, unit->undead_graphic,
                   unit->undead_mode, unit->decay_rate);
        else
            printf("unit %u/%u: absent\n", query_civ, query_unit);
        if (unit && unit->type == 80)
            printf("building: construction graphic %d, adjacent mode %u, "
                   "completed angle %u, disappears %u, stack unit %d, "
                   "foundation/old terrain %d/%d, research %d, sound %d\n",
                   unit->construction_graphic, unit->building_adjacent_mode,
                   unit->building_angle, unit->disappears_when_built,
                   unit->stack_unit_id, unit->foundation_terrain,
                   unit->old_terrain, unit->building_research_id,
                   unit->completion_sound);
    }
    printf("next section offset: 0x%zx\n", civilizations.next_offset);
    if (!dat_parse_techs(&blob, &civilizations, &techs)) {
        fprintf(stderr, "cannot parse technology definitions\n");
        dat_free_civilizations(&civilizations);
        dat_free_effects(&effects);
        dat_free_random_maps(&random_maps);
        dat_free_borders(&borders);
        dat_free_terrains(&terrains);
        dat_free_graphics(&graphics);
        dat_free_sounds(&sounds);
        dat_free_intro(&intro);
        dat_free(&blob);
        return 1;
    }
    printf("technologies: %u\n", techs.count);
    {
        unsigned invalid_effects = 0, invalid_prereqs = 0;
        unsigned invalid_locations = 0;
        for (uint16_t i = 0; i < techs.count; ++i) {
            const DatTech *tech = &techs.techs[i];
            if (tech->effect_id >= 0 &&
                (uint32_t)tech->effect_id >= effects.count)
                ++invalid_effects;
            for (unsigned j = 0; j < 4; ++j)
                if (tech->required_techs[j] >= 0 &&
                    tech->required_techs[j] >= techs.count)
                    ++invalid_prereqs;
            if (tech->research_location >= 0 &&
                !dat_find_unit(&civilizations, 0,
                               (uint16_t)tech->research_location))
                ++invalid_locations;
        }
        printf("technology references: %u invalid effects, %u invalid "
               "prerequisites, %u missing Gaia research locations\n",
               invalid_effects, invalid_prereqs, invalid_locations);
    }
    for (uint16_t i = 0; i < techs.count; ++i)
        if (i < 4 || i == techs.count - 1)
            printf("technology %u: %s, effect %d, time %d, location %d\n",
                   i, techs.techs[i].name, techs.techs[i].effect_id,
                   techs.techs[i].research_time,
                   techs.techs[i].research_location);
    printf("parsed through: 0x%zx of 0x%zx decompressed bytes\n",
           techs.next_offset, blob.size);
    dat_free_techs(&techs);
    dat_free_civilizations(&civilizations);
    dat_free_effects(&effects);
    dat_free_random_maps(&random_maps);
    dat_free_borders(&borders);
    dat_free_terrains(&terrains);
    dat_free_graphics(&graphics);
    dat_free_sounds(&sounds);
    dat_free_intro(&intro);
    dat_free(&blob);
    return factory_audit &&
        (total_unsupported_factory_types || failed_player_clones) ? 1 : 0;
}
