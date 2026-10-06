#include "scn.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

enum { SCN_CHUNK = 65536, SCN_MAX_SIZE = 16 * 1024 * 1024,
       SCN_MAX_HEADER = 1024 * 1024, SCN_MAX_DIMENSION = 256,
       SCN_MAX_UNITS_PER_PLAYER = 20000, SCN_UNIT_BYTES = 23,
       SCN_VICTORY_BYTES = 44, SCN_MAX_VICTORY_CONDITIONS = 100 };

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t le16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static float lefloat(const uint8_t *p) {
    uint32_t bits = le32(p);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

typedef struct {
    const uint8_t *data;
    size_t size;
    size_t offset;
} ScnCursor;

static int skip_bytes(ScnCursor *cursor, size_t count) {
    if (count > cursor->size - cursor->offset) return 0;
    cursor->offset += count;
    return 1;
}

static int skip_string(ScnCursor *cursor) {
    int16_t length;
    if (cursor->size - cursor->offset < 2) return 0;
    length = (int16_t)le16(cursor->data + cursor->offset);
    cursor->offset += 2;
    return length >= 0 && skip_bytes(cursor, (size_t)length);
}

/* A 16-bit length-prefixed string kept as a NUL-terminated copy (NULL
   when empty). */
static int read_string(ScnCursor *cursor, char **out) {
    int16_t length;
    if (cursor->size - cursor->offset < 2) return 0;
    length = (int16_t)le16(cursor->data + cursor->offset);
    if (length < 0 || (size_t)length > cursor->size - cursor->offset - 2) return 0;
    cursor->offset += 2;
    if (length > 0 && cursor->data[cursor->offset]) {
        *out = malloc((size_t)length + 1);
        if (!*out) return 0;
        memcpy(*out, cursor->data + cursor->offset, (size_t)length);
        (*out)[length] = 0;
    }
    cursor->offset += (size_t)length;
    return 1;
}

/* Empires.exe 0x0045ddf0 and 0x0051dd50, for the installed 1.15 body.
   The latter is the scenario object's +0xf4 reader, immediately before
   the map reader at 0x00424cb0. Unsupported versions/embedded blobs fail
   closed instead of searching for a tile-shaped byte sequence. */
static int skip_player_data_115(ScnCursor *cursor, ScnMap *map) {
    uint16_t record_count;
    if (cursor->size - cursor->offset < 4096 + 256 + 1) return 0;
    for (int i = 0; i < 16; ++i) {
        ScnPlayerSetup *setup = &map->player_setup[i];
        const uint8_t *p = cursor->data + cursor->offset + i * 256;
        memcpy(setup->name, p, 256);
        setup->name[256] = 0;
    }
    cursor->offset += 4096;
    for (int i = 0; i < 16; ++i) {
        ScnPlayerSetup *setup = &map->player_setup[i];
        const uint8_t *p = cursor->data + cursor->offset + i * 16;
        setup->enabled = le32(p);
        setup->is_human = le32(p + 4);
        setup->civilization_id = le32(p + 8);
        setup->unknown = le32(p + 12);
    }
    cursor->offset += 256;
    if (!skip_bytes(cursor, 1) || cursor->size - cursor->offset < 8)
        return 0;
    record_count = le16(cursor->data + cursor->offset);
    if (record_count > 4096 ||
        !skip_bytes(cursor, 8 + (size_t)record_count * 30)) return 0;
    for (int i = 0; i < SCN_TEXTS; ++i)
        if (!read_string(cursor, &map->texts[i])) return 0;
    if (cursor->size - cursor->offset < 14) return 0;
    /* 0x004a33a0 reads a 14-byte wrapper. If dimensions are nonzero,
       0x004a37d0 rounds width up to a four-byte row stride, then reads
       height * stride + 0x428 bytes: an 8-bit DIB with 256-color palette. */
    {
        const uint8_t *p = cursor->data + cursor->offset;
        uint32_t width = le32(p + 4), height = le32(p + 8);
        if (!skip_bytes(cursor, 14)) return 0;
        if (width || height) {
            size_t stride, image_size, dib_size;
            const uint8_t *dib;
            if (!width || !height || width > 4096 || height > 4096)
                return 0;
            stride = ((size_t)width + 3) & ~(size_t)3;
            image_size = stride * height;
            dib_size = 0x428 + image_size;
            if (dib_size > SCN_MAX_SIZE ||
                dib_size > cursor->size - cursor->offset) return 0;
            dib = cursor->data + cursor->offset;
            if (le32(dib) != 40 || le32(dib + 4) != width ||
                le32(dib + 8) != height || le16(dib + 12) != 1 ||
                le16(dib + 14) != 8 || le32(dib + 16) != 0 ||
                (le32(dib + 20) != 0 && le32(dib + 20) != image_size))
                return 0;
            map->briefing_bitmap.dib = malloc(dib_size);
            if (!map->briefing_bitmap.dib) return 0;
            memcpy(map->briefing_bitmap.dib, dib, dib_size);
            map->briefing_bitmap.width = width;
            map->briefing_bitmap.height = height;
            map->briefing_bitmap.dib_size = dib_size;
            if (!skip_bytes(cursor, dib_size)) return 0;
        }
    }
    for (int i = 0; i < 48; ++i)
        if (!skip_string(cursor)) return 0;
    for (int player = 0; player < 16; ++player) {
        uint32_t lengths[3];
        if (cursor->size - cursor->offset < 12) return 0;
        for (int i = 0; i < 3; ++i) {
            lengths[i] = le32(cursor->data + cursor->offset + i * 4);
            if (lengths[i] > SCN_MAX_SIZE) return 0;
        }
        cursor->offset += 12;
        for (int i = 0; i < 3; ++i)
            if (!skip_bytes(cursor, lengths[i])) return 0;
    }
    /* Final field, then the 1.15-only fixed player arrays/settings. */
    map->player_data_fixed_offset = cursor->offset;
    {
        /* Offsets within the block follow the reader's order: final field
           (4), resources +0x1990 (16 x 16), 4, +0x1a90 (24), +0x4be8 (4),
           +0x5140..+0x5148 (12), +0x47a8 (1024), sixteen 0x2d0-byte records
           at +0x1aa8, 4, +0x4ba8 (64), toggles +0x4bf4 (0x500), options
           +0x50f4 (12), ages +0x5100 (64), 4. */
        enum {
            RESOURCES = 4,
            TOGGLES = 4 + 16 * 16 + 4 + 24 + 4 + 12 + 1024 + 16 * 0x2d0 +
                      4 + 64,
            OPTIONS = TOGGLES + 0x500,
            AGES = OPTIONS + 12,
            END = AGES + 64 + 4
        };
        const uint8_t *b = cursor->data + cursor->offset;
        if (cursor->size - cursor->offset < END) return 0;
        for (int i = 0; i < 16; ++i) {
            for (int k = 0; k < 4; ++k)
                map->start_resources[i][k] =
                    (int32_t)le32(b + RESOURCES + i * 16 + k * 4);
            for (int k = 0; k < 20; ++k)
                map->tech_toggles[i][k] =
                    (int32_t)le32(b + TOGGLES + i * 0x50 + k * 4);
            map->starting_age[i] = (int32_t)le32(b + AGES + i * 4);
        }
        for (int k = 0; k < 3; ++k)
            map->options[k] = (int32_t)le32(b + OPTIONS + k * 4);
        return skip_bytes(cursor, END);
    }
}

void scn_free_map(ScnMap *map) {
    if (!map) return;
    free(map->instructions);
    for (int i = 0; i < SCN_TEXTS; ++i) free(map->texts[i]);
    free(map->briefing_bitmap.dib);
    free(map->tiles);
    for (int i = 0; i < 9; ++i) free(map->player_units[i].units);
    for (int i = 0; i < 8; ++i)
        free(map->player_details[i].victory_conditions);
    memset(map, 0, sizeof(*map));
}

static int read_player_details(ScnCursor *cursor, ScnMap *map) {
    if (cursor->size - cursor->offset < 4 ||
        le32(cursor->data + cursor->offset) != 9 ||
        !skip_bytes(cursor, 4)) return 0;
    for (int player = 0; player < 8; ++player) {
        ScnPlayerDetail *detail = &map->player_details[player];
        const uint8_t *p;
        uint16_t name_length;
        if (cursor->size - cursor->offset < 2) return 0;
        name_length = le16(cursor->data + cursor->offset);
        cursor->offset += 2;
        if (name_length > 256 ||
            cursor->size - cursor->offset < (size_t)name_length + 15)
            return 0;
        memcpy(detail->name, cursor->data + cursor->offset, name_length);
        detail->name[name_length] = 0;
        cursor->offset += name_length;
        p = cursor->data + cursor->offset;
        detail->camera_x = lefloat(p);
        detail->camera_y = lefloat(p + 4);
        detail->camera_x2 = (int16_t)le16(p + 8);
        detail->camera_y2 = (int16_t)le16(p + 10);
        detail->allied_victory = p[12];
        detail->diplomacy_count = le16(p + 13);
        cursor->offset += 15;
        if (detail->diplomacy_count > 16 ||
            cursor->size - cursor->offset <
                (size_t)detail->diplomacy_count * 5 + 9)
            return 0;
        memcpy(detail->diplomacy, cursor->data + cursor->offset,
               detail->diplomacy_count);
        cursor->offset += detail->diplomacy_count;
        for (uint16_t i = 0; i < detail->diplomacy_count; ++i) {
            detail->diplomacy_values[i] = le32(cursor->data + cursor->offset);
            cursor->offset += 4;
        }
        detail->victory_version = lefloat(cursor->data + cursor->offset);
        cursor->offset += 4;
        detail->victory_condition_count = le32(cursor->data + cursor->offset);
        cursor->offset += 4;
        detail->victory = cursor->data[cursor->offset++];
        if (detail->victory_condition_count > SCN_MAX_VICTORY_CONDITIONS ||
            detail->victory_condition_count >
                (cursor->size - cursor->offset) / SCN_VICTORY_BYTES)
            return 0;
        if (detail->victory_condition_count) {
            detail->victory_conditions = calloc(detail->victory_condition_count,
                                                sizeof(ScnVictoryCondition));
            if (!detail->victory_conditions) return 0;
        }
        for (uint32_t i = 0; i < detail->victory_condition_count; ++i) {
            ScnVictoryCondition *condition = &detail->victory_conditions[i];
            p = cursor->data + cursor->offset;
            condition->type = p[0];
            condition->object_type = (int32_t)le32(p + 1);
            condition->target_player = (int32_t)le32(p + 5);
            condition->x0 = lefloat(p + 9);
            condition->y0 = lefloat(p + 13);
            condition->x1 = lefloat(p + 17);
            condition->y1 = lefloat(p + 21);
            condition->number = (int32_t)le32(p + 25);
            condition->count = (int32_t)le32(p + 29);
            condition->object = (int32_t)le32(p + 33);
            condition->target = (int32_t)le32(p + 37);
            condition->victory_group = p[41];
            condition->ally_flag = p[42];
            condition->state = p[43];
            cursor->offset += SCN_VICTORY_BYTES;
        }
        if (detail->victory_version >= 1.0f) {
            if (cursor->size - cursor->offset < 8) return 0;
            detail->point_condition_count = le32(cursor->data + cursor->offset);
            detail->total_victory_points = le32(cursor->data + cursor->offset + 4);
            cursor->offset += 8;
        }
        if (detail->victory_version > 1.0f && !skip_bytes(cursor, 8))
            return 0;
    }
    return cursor->offset == cursor->size;
}

/* 0x00505da0 reads eight four-float resource sets, then nine groups of
   units. Each 23-byte unit record is read as 3 floats, ID, type, state,
   and rotation before dispatching to the player's +0x60 constructor. */
static int read_units(ScnCursor *cursor, ScnMap *map) {
    uint32_t player_count;
    if (cursor->size - cursor->offset < 4) return 0;
    player_count = le32(cursor->data + cursor->offset);
    if (player_count != 9 || !skip_bytes(cursor, 4)) return 0;
    for (int i = 0; i < 8; ++i) {
        const uint8_t *p;
        if (cursor->size - cursor->offset < 16) return 0;
        p = cursor->data + cursor->offset;
        map->resources[i].food = lefloat(p);
        map->resources[i].wood = lefloat(p + 4);
        map->resources[i].gold = lefloat(p + 8);
        map->resources[i].stone = lefloat(p + 12);
        cursor->offset += 16;
    }
    for (int player = 0; player < 9; ++player) {
        ScnPlayerUnits *group = &map->player_units[player];
        if (cursor->size - cursor->offset < 4) return 0;
        group->count = le32(cursor->data + cursor->offset);
        cursor->offset += 4;
        if (group->count > SCN_MAX_UNITS_PER_PLAYER ||
            group->count > (cursor->size - cursor->offset) / SCN_UNIT_BYTES)
            return 0;
        if (!group->count) continue;
        group->units = calloc(group->count, sizeof(ScnUnit));
        if (!group->units) return 0;
        for (uint32_t i = 0; i < group->count; ++i) {
            const uint8_t *p = cursor->data + cursor->offset;
            ScnUnit *unit = &group->units[i];
            unit->x = lefloat(p);
            unit->y = lefloat(p + 4);
            unit->z = lefloat(p + 8);
            unit->spawn_id = le32(p + 12);
            unit->object_id = le16(p + 16);
            unit->state = p[18];
            unit->rotation = lefloat(p + 19);
            cursor->offset += SCN_UNIT_BYTES;
        }
    }
    return read_player_details(cursor, map);
}

static int inflate_body(FILE *file, uint8_t **out, size_t *out_size) {
    z_stream stream = {0};
    uint8_t input[SCN_CHUNK];
    size_t capacity = 0;
    int status = Z_OK, reached_eof = 0;
    *out = NULL;
    *out_size = 0;
    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) return 0;
    while (status != Z_STREAM_END) {
        if (!stream.avail_in && !reached_eof) {
            size_t n = fread(input, 1, sizeof(input), file);
            if (n < sizeof(input)) {
                if (ferror(file)) goto fail;
                reached_eof = 1;
            }
            stream.next_in = input;
            stream.avail_in = (uInt)n;
        }
        if (*out_size == capacity) {
            size_t next = capacity ? capacity * 2 : SCN_CHUNK;
            uint8_t *grown;
            if (next > SCN_MAX_SIZE) goto fail;
            grown = realloc(*out, next);
            if (!grown) goto fail;
            *out = grown;
            capacity = next;
        }
        stream.next_out = *out + *out_size;
        stream.avail_out = (uInt)(capacity - *out_size);
        status = inflate(&stream, Z_NO_FLUSH);
        *out_size = capacity - stream.avail_out;
        if (status != Z_OK && status != Z_STREAM_END) goto fail;
        if (reached_eof && !stream.avail_in && status != Z_STREAM_END) goto fail;
    }
    inflateEnd(&stream);
    return 1;
fail:
    inflateEnd(&stream);
    free(*out);
    *out = NULL;
    *out_size = 0;
    return 0;
}

static int read_map(const uint8_t *body, size_t size, uint8_t terrain_count,
                    ScnMap *map) {
    ScnCursor cursor = {body, size, 8};
    uint32_t width, height;
    size_t count;
    const uint8_t *tiles;
    if (size < 12 || !terrain_count) return 0;
    map->next_unit_id = le32(body);
    uint32_t player_version_bits = le32(body + 4);
    memcpy(&map->player_data_version, &player_version_bits,
           sizeof(map->player_data_version));
    if (player_version_bits != 0x3f933333 ||
        !skip_player_data_115(&cursor, map) ||
        size - cursor.offset < 8) return 0;
    width = le32(body + cursor.offset);
    height = le32(body + cursor.offset + 4);
    if (width < 32 || height < 32 || width > SCN_MAX_DIMENSION ||
        height > SCN_MAX_DIMENSION) return 0;
    count = (size_t)width * height;
    if (count > (size - cursor.offset - 8) / 3) return 0;
    map->map_offset = cursor.offset;
    map->width = width;
    map->height = height;
    tiles = body + cursor.offset + 8;
    for (size_t i = 0; i < count; ++i)
        if (tiles[i * 3] >= terrain_count || tiles[i * 3 + 1] > 15 ||
            tiles[i * 3 + 2] != 0) return 0;
    map->tiles = calloc(count, sizeof(ScnTile));
    if (!map->tiles) return 0;
    for (size_t i = 0; i < count; ++i) {
        map->tiles[i].terrain_id = tiles[i * 3];
        map->tiles[i].elevation = tiles[i * 3 + 1];
        map->tiles[i].unused = tiles[i * 3 + 2];
    }
    unsigned enabled = 0;
    for (int i = 0; i < 16; ++i)
        if (map->player_setup[i].enabled) ++enabled;
    if (enabled != map->player_count) return 0;
    cursor.offset += 8 + count * 3;
    return read_units(&cursor, map);
}

int scn_load_map(const char *path, uint8_t terrain_count, ScnMap *map) {
    FILE *file = NULL;
    uint8_t prefix[8], *header = NULL, *body = NULL;
    uint32_t header_size, instructions_size;
    size_t body_size = 0;
    int ok = 0;
    if (!path || !map) return 0;
    memset(map, 0, sizeof(*map));
    file = fopen(path, "rb");
    if (!file || fread(prefix, 1, 8, file) != 8 ||
        (memcmp(prefix, "1.11", 4) != 0 &&
         memcmp(prefix, "1.10", 4) != 0)) goto done;
    memcpy(map->version, prefix, 4);
    header_size = le32(prefix + 4);
    if (header_size < 20 || header_size > SCN_MAX_HEADER) goto done;
    header = malloc(header_size);
    if (!header || fread(header, 1, header_size, file) != header_size) goto done;
    instructions_size = le32(header + 8);
    if (instructions_size != header_size - 20) goto done;
    map->record_version = (int32_t)le32(header);
    map->last_save_time = le32(header + 4);
    map->has_conditions = le32(header + 12 + instructions_size);
    map->player_count = le32(header + 16 + instructions_size);
    if (map->player_count < 1 || map->player_count > 16) goto done;
    map->instructions = malloc((size_t)instructions_size + 1);
    if (!map->instructions) goto done;
    memcpy(map->instructions, header + 12, instructions_size);
    map->instructions[instructions_size] = 0;
    if (!inflate_body(file, &body, &body_size) ||
        !read_map(body, body_size, terrain_count, map)) goto done;
    ok = 1;
done:
    if (file) fclose(file);
    free(header);
    free(body);
    if (!ok) scn_free_map(map);
    return ok;
}

int scn_runtime_tile_order_supported(const ScnMap *map) {
    return map && map->tiles && map->width && map->height &&
           map->width == map->height && map->height <= SCN_MAX_DIMENSION;
}

const ScnTile *scn_runtime_tile_at(const ScnMap *map,
                                    uint32_t x, uint32_t y) {
    if (!scn_runtime_tile_order_supported(map) ||
        x >= map->width || y >= map->height) return NULL;
    return &map->tiles[(size_t)y * map->width + x];
}
