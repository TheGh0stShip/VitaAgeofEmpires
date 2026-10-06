#include "dat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inflate.h"

enum { DAT_CHUNK = 65536, DAT_MAX_SIZE = 16 * 1024 * 1024 };

static uint16_t le16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static float lefloat(const uint8_t *p) {
    uint32_t bits = le32(p);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

void dat_free(DatBlob *blob) {
    if (!blob) return;
    free(blob->bytes);
    memset(blob, 0, sizeof(*blob));
}

/* The file is read whole and decoded with the executable's own stream
   decoder (0x00525b0b, inflate.h), as 0x0046b800 reads it. */
int dat_load(const char *path, DatBlob *blob) {
    FILE *file;
    uint8_t *packed = NULL;
    size_t size = 0, capacity = 0;
    if (!path || !blob) return 0;
    memset(blob, 0, sizeof(*blob));
    file = fopen(path, "rb");
    if (!file) return 0;
    for (;;) {
        size_t n;
        if (size == capacity) {
            uint8_t *grown;
            if (capacity >= DAT_MAX_SIZE) goto fail;
            grown = realloc(packed, capacity ? capacity * 2 : DAT_CHUNK);
            if (!grown) goto fail;
            packed = grown;
            capacity = capacity ? capacity * 2 : DAT_CHUNK;
        }
        n = fread(packed + size, 1, capacity - size, file);
        size += n;
        if (n == 0) {
            if (ferror(file)) goto fail;
            break;
        }
    }
    fclose(file);
    file = NULL;
    if (!aoe_inflate_buffer_ror(packed, size, DAT_MAX_SIZE, &blob->bytes, &blob->size))
        goto fail;
    free(packed);
    if (blob->size < 8 || memcmp(blob->bytes, "VER 3.7\0", 8) != 0) {
        dat_free(blob);
        return 0;
    }
    return 1;
fail:
    if (file) fclose(file);
    free(packed);
    dat_free(blob);
    return 0;
}

void dat_free_intro(DatIntro *intro) {
    if (!intro) return;
    free(intro->terrain_restrictions);
    free(intro->colors);
    memset(intro, 0, sizeof(*intro));
}

int dat_parse_intro(const DatBlob *blob, DatIntro *intro) {
    size_t offset, values;
    if (!intro) return 0;
    memset(intro, 0, sizeof(*intro));
    if (!blob || !blob->bytes || blob->size < 12 ||
        memcmp(blob->bytes, "VER 3.7\0", 8) != 0) return 0;
    intro->restriction_count = le16(blob->bytes + 8);
    intro->terrain_count = le16(blob->bytes + 10);
    if (!intro->restriction_count || intro->restriction_count > 64 ||
        !intro->terrain_count || intro->terrain_count > 256) return 0;
    values = (size_t)intro->restriction_count * intro->terrain_count;
    /* This release stores one 32-bit pointer placeholder per table. */
    offset = 12 + (size_t)intro->restriction_count * 4;
    if (offset > blob->size || values > (blob->size - offset) / 4) return 0;
    intro->terrain_restrictions = malloc(values * sizeof(float));
    if (!intro->terrain_restrictions) goto fail;
    for (size_t i = 0; i < values; ++i) {
        uint32_t bits = le32(blob->bytes + offset + i * 4);
        memcpy(&intro->terrain_restrictions[i], &bits, sizeof(bits));
    }
    offset += values * 4;
    if (blob->size - offset < 2) goto fail;
    intro->color_count = le16(blob->bytes + offset);
    offset += 2;
    if (!intro->color_count || intro->color_count > 64 ||
        intro->color_count > (blob->size - offset) / 36) goto fail;
    intro->colors = calloc(intro->color_count, sizeof(DatColorRecord));
    if (!intro->colors) goto fail;
    for (uint16_t i = 0; i < intro->color_count; ++i) {
        const uint8_t *item = blob->bytes + offset + (size_t)i * 36;
        DatColorRecord *color = &intro->colors[i];
        memcpy(color->name, item, 30);
        color->name[30] = 0;
        if (!memchr(color->name, 0, 30)) goto fail;
        color->ordinal = le16(item + 30);
        color->unknown = le16(item + 32);
        color->value = le16(item + 34);
        if (color->ordinal != i) goto fail;
    }
    intro->next_offset = offset + (size_t)intro->color_count * 36;
    return 1;
fail:
    dat_free_intro(intro);
    return 0;
}

void dat_free_sounds(DatSounds *sounds) {
    if (!sounds) return;
    if (sounds->sounds) {
        for (uint16_t i = 0; i < sounds->count; ++i)
            free(sounds->sounds[i].variants);
    }
    free(sounds->sounds);
    memset(sounds, 0, sizeof(*sounds));
}

int dat_parse_sounds(const DatBlob *blob, const DatIntro *intro, DatSounds *sounds) {
    size_t offset;
    if (!sounds) return 0;
    memset(sounds, 0, sizeof(*sounds));
    if (!blob || !blob->bytes || !intro ||
        intro->next_offset > blob->size || blob->size - intro->next_offset < 2)
        return 0;
    offset = intro->next_offset;
    sounds->count = le16(blob->bytes + offset);
    offset += 2;
    if (!sounds->count || sounds->count > 4096 ||
        sounds->count > (blob->size - offset) / 10)
        goto fail;
    sounds->sounds = calloc(sounds->count, sizeof(DatSound));
    if (!sounds->sounds) goto fail;
    for (uint16_t i = 0; i < sounds->count; ++i) {
        DatSound *sound = &sounds->sounds[i];
        const uint8_t *header;
        if (blob->size - offset < 10) goto fail;
        header = blob->bytes + offset;
        sound->field_0c = le16(header);
        sound->field_00 = le16(header + 2);
        sound->variant_count = le16(header + 4);
        sound->stop_time = le32(header + 6);
        offset += 10;
        if (sound->field_0c != i || sound->variant_count > 64 ||
            sound->variant_count > (blob->size - offset) / 19)
            goto fail;
        if (!sound->variant_count) continue;
        sound->variants = calloc(sound->variant_count, sizeof(DatSoundVariant));
        if (!sound->variants) goto fail;
        for (uint16_t j = 0; j < sound->variant_count; ++j) {
            const uint8_t *item = blob->bytes + offset;
            DatSoundVariant *variant = &sound->variants[j];
            if (!memchr(item, 0, 13)) goto fail;
            memcpy(variant->filename, item, 13);
            variant->resource_id = le32(item + 13);
            variant->chance = le16(item + 17);
            offset += 19;
            ++sounds->variant_count;
        }
    }
    sounds->next_offset = offset;
    return 1;
fail:
    dat_free_sounds(sounds);
    return 0;
}

void dat_free_graphics(DatGraphics *graphics) {
    if (!graphics) return;
    if (graphics->graphics) {
        for (uint16_t i = 0; i < graphics->count; ++i) {
            free(graphics->graphics[i].deltas);
            free(graphics->graphics[i].attack_sounds);
        }
    }
    free(graphics->graphics);
    memset(graphics, 0, sizeof(*graphics));
}

int dat_parse_graphics(const DatBlob *blob, const DatSounds *sounds,
                       DatGraphics *graphics) {
    size_t offset, pointer_offset;
    if (!graphics) return 0;
    memset(graphics, 0, sizeof(*graphics));
    if (!blob || !blob->bytes || !sounds ||
        sounds->next_offset > blob->size || blob->size - sounds->next_offset < 2)
        return 0;
    offset = sounds->next_offset;
    graphics->count = le16(blob->bytes + offset);
    offset += 2;
    if (!graphics->count || graphics->count > 4096 ||
        graphics->count > (blob->size - offset) / 4)
        goto fail;
    pointer_offset = offset;
    offset += (size_t)graphics->count * 4;
    graphics->graphics = calloc(graphics->count, sizeof(DatGraphic));
    if (!graphics->graphics) goto fail;
    for (uint16_t i = 0; i < graphics->count; ++i) {
        DatGraphic *graphic = &graphics->graphics[i];
        const uint8_t *item;
        graphic->pointer_placeholder = le32(blob->bytes + pointer_offset + (size_t)i * 4);
        if (!graphic->pointer_placeholder) continue;
        if (blob->size - offset < 77) goto fail;
        item = blob->bytes + offset;
        if (!memchr(item, 0, 21) || !memchr(item + 21, 0, 13) ||
            le16(item + 74) != i) goto fail;
        graphic->present = 1;
        memcpy(graphic->name, item, 21);
        memcpy(graphic->filename, item + 21, 13);
        graphic->slp_id = (int32_t)le32(item + 34);
        graphic->layer = item[40];
        graphic->player_color = (int16_t)le16(item + 41);
        graphic->slp_loadable = item[38];
        graphic->selection_mode = item[43];
        for (unsigned j = 0; j < 4; ++j)
            graphic->coordinates[j] = (int16_t)le16(item + 44 + j * 2);
        graphic->delta_count = le16(item + 52);
        graphic->sound_id = (int16_t)le16(item + 54);
        uint8_t attack_sound_used = item[56];
        graphic->frame_count = le16(item + 57);
        graphic->angle_count = le16(item + 59);
        graphic->reference_speed = lefloat(item + 61);
        graphic->frame_duration = lefloat(item + 65);
        graphic->loop_delay = lefloat(item + 69);
        graphic->sequence_flags = item[73];
        graphic->mirror_limit = item[76];
        offset += 77;
        if (graphic->delta_count > 256 || graphic->angle_count > 256 ||
            attack_sound_used > 1 || graphic->delta_count > (blob->size - offset) / 16)
            goto fail;
        if (graphic->delta_count) {
            graphic->deltas = calloc(graphic->delta_count, sizeof(DatGraphicDelta));
            if (!graphic->deltas) goto fail;
            for (uint16_t j = 0; j < graphic->delta_count; ++j) {
                const uint8_t *delta = blob->bytes + offset;
                DatGraphicDelta *out = &graphic->deltas[j];
                out->graphic_id = (int16_t)le16(delta);
                out->pointer_placeholder = le32(delta + 4);
                out->offset_x = (int16_t)le16(delta + 8);
                out->offset_y = (int16_t)le16(delta + 10);
                out->display_angle = (int16_t)le16(delta + 12);
                out->unknown = (int16_t)le16(delta + 14);
                offset += 16;
            }
        }
        if (attack_sound_used) {
            graphic->attack_sound_count = (size_t)graphic->angle_count * 3;
            if (graphic->attack_sound_count > (blob->size - offset) / 4)
                goto fail;
            graphic->attack_sounds = calloc(graphic->attack_sound_count,
                                           sizeof(DatGraphicAttackSound));
            if (!graphic->attack_sounds) goto fail;
            for (size_t j = 0; j < graphic->attack_sound_count; ++j) {
                const uint8_t *sound = blob->bytes + offset;
                graphic->attack_sounds[j].delay = (int16_t)le16(sound);
                graphic->attack_sounds[j].sound_id = (int16_t)le16(sound + 2);
                offset += 4;
            }
        }
        ++graphics->present_count;
    }
    /* RoR 0x004a6970 replaces -1 with the owning graphic and every other
       serialized delta graphic ID with its runtime table pointer. */
    for (uint16_t i = 0; i < graphics->count; ++i) {
        DatGraphic *graphic = &graphics->graphics[i];
        if (!graphic->present) continue;
        for (uint16_t j = 0; j < graphic->delta_count; ++j) {
            DatGraphicDelta *delta = &graphic->deltas[j];
            int16_t id = delta->graphic_id;
            if (id == -1)
                delta->graphic = graphic;
            else if (id >= 0 && (uint16_t)id < graphics->count &&
                graphics->graphics[id].present)
                delta->graphic = &graphics->graphics[id];
        }
    }
    graphics->next_offset = offset;
    return 1;
fail:
    dat_free_graphics(graphics);
    return 0;
}

void dat_free_terrains(DatTerrains *terrains) {
    if (!terrains) return;
    free(terrains->terrains);
    memset(terrains, 0, sizeof(*terrains));
}

int dat_parse_terrains(const DatBlob *blob, const DatIntro *intro,
                       const DatGraphics *graphics, DatTerrains *terrains) {
    enum { TERRAIN_HEADER_SIZE = 140, TERRAIN_SIZE = 408, TERRAIN_SLOTS = 32 };
    size_t offset;
    if (!terrains) return 0;
    memset(terrains, 0, sizeof(*terrains));
    if (!blob || !blob->bytes || !intro || !graphics ||
        graphics->next_offset > blob->size ||
        blob->size - graphics->next_offset < TERRAIN_HEADER_SIZE + TERRAIN_SLOTS * TERRAIN_SIZE)
        return 0;
    offset = graphics->next_offset;
    for (unsigned i = 0; i < 6; ++i)
        terrains->map_header[i] = le32(blob->bytes + offset + i * 4);
    offset += 24;
    for (unsigned i = 0; i < 19; ++i) {
        const uint8_t *tile = blob->bytes + offset + i * 6;
        terrains->tile_sizes[i].width = (int16_t)le16(tile);
        terrains->tile_sizes[i].height = (int16_t)le16(tile + 2);
        terrains->tile_sizes[i].elevation_delta = (int16_t)le16(tile + 4);
    }
    offset += 116; /* 19 six-byte tile sizes and a two-byte pad. */
    terrains->count = TERRAIN_SLOTS;
    terrains->terrains = calloc(terrains->count, sizeof(DatTerrain));
    if (!terrains->terrains) return 0;
    for (uint16_t i = 0; i < terrains->count; ++i) {
        const uint8_t *item = blob->bytes + offset + (size_t)i * TERRAIN_SIZE;
        DatTerrain *terrain = &terrains->terrains[i];
        terrain->enabled = item[0];
        terrain->field_01 = item[1];
        if (terrain->enabled > 1 || (terrain->enabled &&
            (!memchr(item + 2, 0, 13) || !memchr(item + 15, 0, 13))))
            goto fail;
        memcpy(terrain->name, item + 2, 13);
        memcpy(terrain->filename, item + 15, 13);
        terrain->slp_id = (int32_t)le32(item + 28);
        terrain->shape_pointer_placeholder = le32(item + 32);
        terrain->sound_id = (int32_t)le32(item + 36);
        memcpy(terrain->map_colors, item + 40, 5);
        terrain->field_2d = (int8_t)item[45];
        terrain->field_2e = (int8_t)item[46];
        terrain->field_2f = item[47];
        terrain->field_30 = (int16_t)le16(item + 48);
        terrain->field_32 = (int16_t)le16(item + 50);
        terrain->field_34 = lefloat(item + 52);
        terrain->field_38 = lefloat(item + 56);
        for (unsigned j = 0; j < 19; ++j) {
            const uint8_t *frame = item + 70 + j * 6;
            terrain->elevation_frames[j].frame_count = (int16_t)le16(frame);
            terrain->elevation_frames[j].angle_count = (int16_t)le16(frame + 2);
            terrain->elevation_frames[j].frame_start = (int16_t)le16(frame + 4);
        }
        terrain->replacement_id = (int16_t)le16(item + 184);
        terrain->frame_grid[0] = (int16_t)le16(item + 186);
        terrain->frame_grid[1] = (int16_t)le16(item + 188);
        for (unsigned j = 0; j < 32; ++j)
            terrain->borders[j] = (int16_t)le16(item + 190 + j * 2);
        for (unsigned j = 0; j < 30; ++j) {
            terrain->object_ids[j] = (int16_t)le16(item + 254 + j * 2);
            terrain->object_amounts[j] = (int16_t)le16(item + 314 + j * 2);
            terrain->object_modes[j] = item[374 + j];
        }
        terrain->object_count = le16(item + 404);
        if (terrain->object_count > 30) goto fail;
        terrains->enabled_count += terrain->enabled;
    }
    if (terrains->enabled_count != intro->terrain_count) goto fail;
    terrains->next_offset = offset + TERRAIN_SLOTS * TERRAIN_SIZE;
    return 1;
fail:
    dat_free_terrains(terrains);
    return 0;
}

void dat_free_borders(DatBorders *borders) {
    if (!borders) return;
    free(borders->borders);
    memset(borders, 0, sizeof(*borders));
}

int dat_parse_borders(const DatBlob *blob, const DatTerrains *terrains,
                      DatBorders *borders) {
    enum { BORDER_SLOTS = 16, BORDER_SIZE = 1440 };
    size_t offset;
    if (!borders) return 0;
    memset(borders, 0, sizeof(*borders));
    if (!blob || !blob->bytes || !terrains ||
        terrains->next_offset > blob->size ||
        BORDER_SLOTS * BORDER_SIZE > blob->size - terrains->next_offset)
        return 0;
    offset = terrains->next_offset;
    borders->count = BORDER_SLOTS;
    borders->borders = calloc(BORDER_SLOTS, sizeof(DatBorder));
    if (!borders->borders) return 0;
    for (uint16_t i = 0; i < BORDER_SLOTS; ++i) {
        const uint8_t *item = blob->bytes + offset + (size_t)i * BORDER_SIZE;
        DatBorder *border = &borders->borders[i];
        border->enabled = item[0];
        border->field_01 = item[1];
        /* Names may occupy all 13 bytes (e.g. Grass/desert2). */
        if (border->enabled > 1) goto fail;
        memcpy(border->name, item + 2, 13);
        memcpy(border->filename, item + 15, 13);
        border->slp_id = (int32_t)le32(item + 28);
        border->shape_pointer_placeholder = le32(item + 32);
        border->sound_id = (int32_t)le32(item + 36);
        memcpy(border->map_colors, item + 40, 3);
        border->field_2b = item[43];
        border->field_2c = (int16_t)le16(item + 44);
        border->field_2e = (int16_t)le16(item + 46);
        border->field_30 = lefloat(item + 48);
        border->field_34 = lefloat(item + 52);
        border->field_38 = (int16_t)le16(item + 56);
        border->field_3a = (int16_t)le16(item + 58);
        border->field_3c = lefloat(item + 60);
        border->field_40 = item[64];
        border->field_41 = item[65];
        for (unsigned shape = 0; shape < 19; ++shape)
            for (unsigned variant = 0; variant < 12; ++variant) {
                const uint8_t *frame = item + 66 + (shape * 12 + variant) * 6;
                DatTerrainFrame *out = &border->frames[shape][variant];
                out->frame_count = (int16_t)le16(frame);
                out->angle_count = (int16_t)le16(frame + 2);
                out->frame_start = (int16_t)le16(frame + 4);
            }
        border->draw_base = (int16_t)le16(item + 1434);
        border->base_terrain = (int16_t)le16(item + 1436);
        border->border_style = (int16_t)le16(item + 1438);
        borders->enabled_count += border->enabled;
    }
    borders->next_offset = offset + BORDER_SLOTS * BORDER_SIZE;
    return 1;
fail:
    dat_free_borders(borders);
    return 0;
}

int dat_parse_map_geometry(const DatBlob *blob, const DatTerrains *terrains,
                           const DatBorders *borders, DatMapGeometry *geometry) {
    const uint8_t *p;
    if (!geometry) return 0;
    memset(geometry, 0, sizeof(*geometry));
    if (!blob || !blob->bytes || !terrains || !borders ||
        borders->next_offset > blob->size ||
        blob->size - borders->next_offset < 20) return 0;
    p = blob->bytes + borders->next_offset;
    geometry->map_row_pointer_placeholder = le32(p);
    geometry->terrains_used = le16(p + 4);
    geometry->borders_used = le16(p + 6);
    geometry->max_terrain = (int16_t)le16(p + 8);
    geometry->tile_width = (int16_t)le16(p + 10);
    geometry->tile_height = (int16_t)le16(p + 12);
    geometry->tile_half_height = (int16_t)le16(p + 14);
    geometry->tile_half_width = (int16_t)le16(p + 16);
    geometry->elevation_height = (int16_t)le16(p + 18);
    if (geometry->terrains_used != terrains->enabled_count ||
        geometry->borders_used != borders->enabled_count ||
        geometry->tile_width <= 0 || geometry->tile_height <= 0 ||
        geometry->tile_half_height <= 0 || geometry->tile_half_width <= 0 ||
        geometry->elevation_height <= 0 ||
        geometry->tile_width != geometry->tile_half_width * 2 ||
        geometry->tile_height != geometry->tile_half_height * 2)
        return 0;
    geometry->next_offset = borders->next_offset + 20;
    return 1;
}

void dat_free_random_maps(DatRandomMaps *maps) {
    if (!maps) return;
    if (maps->maps)
        for (uint32_t i = 0; i < maps->count; ++i) {
            free(maps->maps[i].lands);
            free(maps->maps[i].terrains);
            free(maps->maps[i].objects);
            free(maps->maps[i].elevations);
        }
    free(maps->maps);
    memset(maps, 0, sizeof(*maps));
}

static int32_t lei32(const uint8_t *p) {
    return (int32_t)le32(p);
}

static void decode_random_map_land(const uint8_t *p, DatRandomMapLand *land) {
    land->id = lei32(p);
    land->field_04 = p[0x04];
    land->field_08 = lei32(p + 0x08);
    land->field_0c = lei32(p + 0x0c);
    land->field_10 = p[0x10];
    land->placement_mode = p[0x11];
    land->x = lei32(p + 0x14);
    land->y = lei32(p + 0x18);
    land->area_percent = p[0x1c];
    land->per_player = p[0x1d];
    land->field_20 = lei32(p + 0x20);
    land->field_24 = lei32(p + 0x24);
    land->field_28 = lei32(p + 0x28);
}

static void decode_random_map_terrain(const uint8_t *p,
                                      DatRandomMapTerrain *terrain) {
    terrain->field_00 = lei32(p);
    terrain->field_04 = lei32(p + 0x04);
    terrain->field_08 = lei32(p + 0x08);
    terrain->field_0c = lei32(p + 0x0c);
    terrain->field_10 = lei32(p + 0x10);
    terrain->field_14 = lei32(p + 0x14);
}

static void decode_random_map_object(const uint8_t *p,
                                     DatRandomMapObject *object) {
    object->field_00 = lei32(p);
    object->field_04 = lei32(p + 0x04);
    object->field_08 = p[0x08];
    object->field_09 = p[0x09];
    object->field_0c = lei32(p + 0x0c);
    object->field_10 = lei32(p + 0x10);
    object->field_14 = lei32(p + 0x14);
    object->field_18 = lei32(p + 0x18);
    object->field_1c = lei32(p + 0x1c);
    object->field_20 = lei32(p + 0x20);
    object->field_24 = lei32(p + 0x24);
    object->field_28 = lei32(p + 0x28);
}

static void decode_random_map_elevation(const uint8_t *p,
                                        DatRandomMapElevation *elevation) {
    for (unsigned i = 0; i < 6; ++i)
        elevation->values[i] = lei32(p + 4 * i);
}

int dat_parse_random_maps(const DatBlob *blob, const DatMapGeometry *geometry,
                          DatRandomMaps *maps) {
    static const size_t record_size[4] = {44, 24, 44, 24};
    size_t offset;
    uint32_t count;
    if (!maps) return 0;
    memset(maps, 0, sizeof(*maps));
    if (!blob || !blob->bytes || !geometry ||
        geometry->next_offset > blob->size ||
        blob->size - geometry->next_offset < 56) return 0;
    /* The VER 3.7 terrain block continues 48 bytes past the 20-byte
       geometry fields: six shorts, three words, four flags, five words. */
    offset = geometry->next_offset + 48;
    if (offset > blob->size || blob->size - offset < 8) return 0;
    count = le32(blob->bytes + offset);
    if (count > 1024 || (size_t)count > (blob->size - offset - 8) / 72)
        return 0;
    maps->maps = calloc(count ? count : 1, sizeof(*maps->maps));
    if (!maps->maps) return 0;
    maps->count = count;
    offset += 8; /* count and pointer placeholder */
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t *header = blob->bytes + offset;
        DatRandomMap *map = &maps->maps[i];
        map->map_id = (int32_t)le32(header);
        map->land_count = le32(header + 40);
        map->terrain_count = le32(header + 48);
        map->unit_count = le32(header + 56);
        map->elevation_count = le32(header + 64);
        offset += 72;
    }
    for (uint32_t i = 0; i < count; ++i) {
        DatRandomMap *map = &maps->maps[i];
        const uint32_t expected[4] = {map->land_count, map->terrain_count,
                                      map->unit_count, map->elevation_count};
        const uint8_t *settings;
        if (blob->size - offset < 36) goto fail;
        /* Nine map settings (header +0x04..+0x24); the map ID is omitted on
           pass two and these values replace the first-pass copies. */
        settings = blob->bytes + offset;
        for (unsigned m = 0; m < 4; ++m)
            map->edge_margin[m] = lei32(settings + 4 * m);
        map->field_14 = lei32(settings + 0x10);
        map->land_edge_distance = lei32(settings + 0x14);
        map->field_1c = lei32(settings + 0x18);
        map->land_percent = lei32(settings + 0x1c);
        map->field_24 = lei32(settings + 0x20);
        offset += 36;
        for (unsigned section = 0; section < 4; ++section) {
            const uint8_t *records;
            const size_t stride = record_size[section];
            uint32_t items;
            if (blob->size - offset < 8) goto fail;
            items = le32(blob->bytes + offset);
            if (items != expected[section] ||
                items > (blob->size - offset - 8) / stride)
                goto fail;
            records = blob->bytes + offset + 8;
            switch (section) {
            case 0:
                map->lands = calloc(items ? items : 1, sizeof(*map->lands));
                if (!map->lands) goto fail;
                for (uint32_t r = 0; r < items; ++r)
                    decode_random_map_land(records + (size_t)r * stride,
                                           &map->lands[r]);
                break;
            case 1:
                map->terrains = calloc(items ? items : 1,
                                       sizeof(*map->terrains));
                if (!map->terrains) goto fail;
                for (uint32_t r = 0; r < items; ++r)
                    decode_random_map_terrain(records + (size_t)r * stride,
                                              &map->terrains[r]);
                break;
            case 2:
                map->objects = calloc(items ? items : 1,
                                      sizeof(*map->objects));
                if (!map->objects) goto fail;
                for (uint32_t r = 0; r < items; ++r)
                    decode_random_map_object(records + (size_t)r * stride,
                                             &map->objects[r]);
                break;
            default:
                map->elevations = calloc(items ? items : 1,
                                         sizeof(*map->elevations));
                if (!map->elevations) goto fail;
                for (uint32_t r = 0; r < items; ++r)
                    decode_random_map_elevation(records + (size_t)r * stride,
                                                &map->elevations[r]);
                break;
            }
            offset += 8 + (size_t)items * stride;
        }
    }
    maps->next_offset = offset;
    return 1;
fail:
    dat_free_random_maps(maps);
    return 0;
}

void dat_free_effects(DatEffects *effects) {
    if (!effects) return;
    if (effects->effects)
        for (uint32_t i = 0; i < effects->count; ++i)
            free(effects->effects[i].commands);
    free(effects->effects);
    memset(effects, 0, sizeof(*effects));
}

int dat_parse_effects(const DatBlob *blob, const DatRandomMaps *maps,
                      DatEffects *effects) {
    size_t offset;
    uint32_t count;
    if (!effects) return 0;
    memset(effects, 0, sizeof(*effects));
    if (!blob || !blob->bytes || !maps || maps->next_offset > blob->size ||
        blob->size - maps->next_offset < 4) return 0;
    offset = maps->next_offset;
    count = le32(blob->bytes + offset);
    offset += 4;
    if (count > 4096 || (size_t)count > (blob->size - offset) / 33)
        return 0;
    effects->effects = calloc(count ? count : 1, sizeof(*effects->effects));
    if (!effects->effects) return 0;
    effects->count = count;
    for (uint32_t i = 0; i < count; ++i) {
        DatEffect *effect = &effects->effects[i];
        uint16_t commands;
        if (blob->size - offset < 33 ||
            !memchr(blob->bytes + offset, 0, 31)) goto fail;
        memcpy(effect->name, blob->bytes + offset, 31);
        commands = le16(blob->bytes + offset + 31);
        offset += 33;
        if ((size_t)commands > (blob->size - offset) / 11) goto fail;
        effect->commands = calloc(commands ? commands : 1,
                                  sizeof(*effect->commands));
        if (!effect->commands) goto fail;
        effect->command_count = commands;
        for (uint16_t j = 0; j < commands; ++j) {
            const uint8_t *p = blob->bytes + offset + (size_t)j * 11;
            DatEffectCommand *command = &effect->commands[j];
            command->type = (int8_t)p[0];
            command->template_id = (int16_t)le16(p + 1);
            command->class_id = (int16_t)le16(p + 3);
            command->attribute = (int16_t)le16(p + 5);
            command->amount = lefloat(p + 7);
        }
        offset += (size_t)commands * 11;
    }
    effects->next_offset = offset;
    return 1;
fail:
    dat_free_effects(effects);
    return 0;
}
