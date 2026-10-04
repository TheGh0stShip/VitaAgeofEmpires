/* Random-map DAT records: second-pass header settings and the land, terrain,
   object and elevation records read by patched RoR 1.0a 0x0046da10 (base
   1.0c 0x0046e5e0), with field widths from text loader 0x0046d580. */
#include "dat.h"

#include <assert.h>
#include <string.h>

static uint8_t bytes[1024];
static size_t cursor;

static void put32_at(size_t at, uint32_t value) {
    bytes[at] = (uint8_t)value;
    bytes[at + 1] = (uint8_t)(value >> 8);
    bytes[at + 2] = (uint8_t)(value >> 16);
    bytes[at + 3] = (uint8_t)(value >> 24);
}

static void put32(uint32_t value) {
    put32_at(cursor, value);
    cursor += 4;
}

/* Fill a record with padding poison, then write its fields. */
static size_t record(size_t size) {
    size_t at = cursor;
    memset(bytes + at, 0xcc, size);
    cursor += size;
    return at;
}

static void put_section_count(uint32_t count) {
    put32(count);
    put32(0xdeadbeefu); /* serialized pointer placeholder */
}

static void put_settings(const int32_t settings[9]) {
    for (unsigned i = 0; i < 9; ++i) put32((uint32_t)settings[i]);
}

int main(void) {
    static const int32_t map0_settings[9] = {
        1, 2, 3, 4, 0x101, 25, -1, 60, 0x12345678};
    static const int32_t map1_settings[9] = {5, 6, 7, 8, 0, 20, 9, 30, -4};
    DatMapGeometry geometry = {0};
    DatRandomMaps maps;
    DatBlob blob = {bytes, 0};
    size_t end, at;

    memset(bytes, 0, sizeof(bytes));
    cursor = 48; /* the section follows the geometry block's 48-byte tail */
    put_section_count(2);

    /* First pass: whole 72-byte headers. Settings here are stale copies the
       second pass replaces; only the map ID at +0x00 survives. */
    at = record(72);
    put32_at(at, 11);
    for (unsigned i = 1; i < 10; ++i) put32_at(at + 4 * i, 0x7777);
    put32_at(at + 0x28, 2);  /* lands */
    put32_at(at + 0x30, 1);  /* terrains */
    put32_at(at + 0x38, 1);  /* objects */
    put32_at(at + 0x40, 1);  /* elevations */
    at = record(72);
    put32_at(at, (uint32_t)-3);
    put32_at(at + 0x28, 0);
    put32_at(at + 0x30, 0);
    put32_at(at + 0x38, 0);
    put32_at(at + 0x40, 0);

    /* Second pass, map 0. */
    put_settings(map0_settings);
    put_section_count(2);
    at = record(44);
    put32_at(at, (uint32_t)-7);
    bytes[at + 0x04] = 0xfe;
    put32_at(at + 0x08, 0x11);
    put32_at(at + 0x0c, (uint32_t)-0x22);
    bytes[at + 0x10] = 0x80;
    bytes[at + 0x11] = 2;
    put32_at(at + 0x14, 40);
    put32_at(at + 0x18, 41);
    bytes[at + 0x1c] = 0xff;
    bytes[at + 0x1d] = 1;
    put32_at(at + 0x20, (uint32_t)-1);
    put32_at(at + 0x24, 5);
    put32_at(at + 0x28, 8);
    at = record(44);
    put32_at(at, 3);
    bytes[at + 0x04] = 1;
    put32_at(at + 0x08, 0);
    put32_at(at + 0x0c, 0);
    bytes[at + 0x10] = 0;
    bytes[at + 0x11] = 1;
    put32_at(at + 0x14, 0);
    put32_at(at + 0x18, 0);
    bytes[at + 0x1c] = 0;
    bytes[at + 0x1d] = 0;
    put32_at(at + 0x20, 0);
    put32_at(at + 0x24, 0);
    put32_at(at + 0x28, 0);
    put_section_count(1);
    at = record(24);
    for (unsigned i = 0; i < 6; ++i) put32_at(at + 4 * i, 100 + i);
    put32_at(at, (uint32_t)-100);
    put_section_count(1);
    at = record(44);
    put32_at(at, 200);
    put32_at(at + 0x04, (uint32_t)-201);
    bytes[at + 0x08] = 0xab;
    bytes[at + 0x09] = 0x01;
    for (unsigned i = 0; i < 8; ++i)
        put32_at(at + 0x0c + 4 * i, 300 + i);
    put_section_count(1);
    at = record(24);
    for (unsigned i = 0; i < 6; ++i)
        put32_at(at + 4 * i, (uint32_t)(-(int32_t)i - 1));

    /* Second pass, map 1: empty sections. */
    put_settings(map1_settings);
    for (unsigned section = 0; section < 4; ++section) put_section_count(0);
    end = cursor;
    blob.size = end;

    assert(dat_parse_random_maps(&blob, &geometry, &maps));
    assert(maps.count == 2 && maps.next_offset == end);
    {
        const DatRandomMap *m = &maps.maps[0];
        const DatRandomMapLand *l = &m->lands[0];
        assert(m->map_id == 11);
        assert(m->land_count == 2 && m->terrain_count == 1);
        assert(m->unit_count == 1 && m->elevation_count == 1);
        for (unsigned i = 0; i < 4; ++i)
            assert(m->edge_margin[i] == map0_settings[i]);
        assert(m->field_14 == 0x101 && m->land_edge_distance == 25);
        assert(m->field_1c == -1 && m->land_percent == 60);
        assert(m->field_24 == 0x12345678);

        assert(l->id == -7 && l->field_04 == 0xfe);
        assert(l->field_08 == 0x11 && l->field_0c == -0x22);
        assert(l->field_10 == 0x80 && l->placement_mode == 2);
        assert(l->x == 40 && l->y == 41);
        assert(l->area_percent == 0xff && l->per_player == 1);
        assert(l->field_20 == -1 && l->field_24 == 5 && l->field_28 == 8);
        l = &m->lands[1];
        assert(l->id == 3 && l->field_04 == 1 && l->placement_mode == 1);
        assert(l->per_player == 0 && l->area_percent == 0);

        assert(m->terrains[0].field_00 == -100);
        assert(m->terrains[0].field_04 == 101);
        assert(m->terrains[0].field_10 == 104);
        assert(m->terrains[0].field_14 == 105);

        assert(m->objects[0].field_00 == 200);
        assert(m->objects[0].field_04 == -201);
        assert(m->objects[0].field_08 == 0xab && m->objects[0].field_09 == 1);
        assert(m->objects[0].field_0c == 300);
        assert(m->objects[0].field_18 == 303);
        assert(m->objects[0].field_28 == 307);

        for (unsigned i = 0; i < 6; ++i)
            assert(m->elevations[0].values[i] == -(int32_t)i - 1);
    }
    {
        const DatRandomMap *m = &maps.maps[1];
        assert(m->map_id == -3);
        assert(m->land_count == 0 && m->terrain_count == 0);
        assert(m->unit_count == 0 && m->elevation_count == 0);
        for (unsigned i = 0; i < 4; ++i)
            assert(m->edge_margin[i] == map1_settings[i]);
        assert(m->land_edge_distance == 20 && m->land_percent == 30);
        assert(m->field_24 == -4);
    }
    dat_free_random_maps(&maps);
    assert(maps.count == 0 && maps.maps == NULL && maps.next_offset == 0);

    /* Every truncation fails cleanly, including mid-record. */
    for (size_t size = 48; size < end; ++size) {
        blob.size = size;
        assert(!dat_parse_random_maps(&blob, &geometry, &maps));
        assert(maps.count == 0 && maps.maps == NULL);
    }
    blob.size = end;

    /* Counts must agree between passes in every section. */
    {
        static const size_t second_pass_counts[4] = {
            56 + 2 * 72 + 36,
            56 + 2 * 72 + 36 + 8 + 2 * 44,
            56 + 2 * 72 + 36 + 8 + 2 * 44 + 8 + 24,
            56 + 2 * 72 + 36 + 8 + 2 * 44 + 8 + 24 + 8 + 44};
        static const uint32_t original[4] = {2, 1, 1, 1};
        for (unsigned section = 0; section < 4; ++section) {
            put32_at(second_pass_counts[section], original[section] + 1);
            assert(!dat_parse_random_maps(&blob, &geometry, &maps));
            put32_at(second_pass_counts[section], original[section]);
        }
        assert(dat_parse_random_maps(&blob, &geometry, &maps));
        dat_free_random_maps(&maps);
    }
    return 0;
}
