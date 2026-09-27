#include "color_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int white(uint8_t c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
           c == '\v' || c == '\f';
}

static void ascii_lower(char *text) {
    for (; *text; ++text)
        if (*text >= 'A' && *text <= 'Z')
            *text = (char)(*text + ('a' - 'A'));
}

int aoe_color_table_parse(const uint8_t *data, size_t size,
                          uint8_t out[256]) {
    if (!data || !out) return 0;
    uint8_t values[256];
    size_t pos = 0;
    for (size_t i = 0; i < 256; ++i) {
        while (pos < size && white(data[pos])) ++pos;
        if (pos == size || data[pos] < '0' || data[pos] > '9')
            return 0;
        unsigned value = 0;
        do {
            value = value * 10u + (unsigned)(data[pos] - '0');
            if (value > 255) return 0;
            ++pos;
        } while (pos < size && data[pos] >= '0' && data[pos] <= '9');
        if (pos < size && !white(data[pos])) return 0;
        values[i] = (uint8_t)value;
    }
    while (pos < size && white(data[pos])) ++pos;
    if (pos != size) return 0;
    memcpy(out, values, sizeof(values));
    return 1;
}

int aoe_color_table_load_drs(DrsArchive *const *archives,
                             size_t archive_count, uint32_t resource_id,
                             uint8_t out[256]) {
    if (!archives || !archive_count || !out) return 0;
    DrsMatch match;
    if (!drs_find_first(archives, archive_count, drs_type("anib"),
                        resource_id, &match) || !match.entry->size ||
        match.entry->size > 16384)
        return 0;
    uint8_t *bytes = malloc(match.entry->size);
    if (!bytes) return 0;
    int ok = drs_read(match.archive, match.entry, bytes,
                      match.entry->size) &&
             aoe_color_table_parse(bytes, match.entry->size, out);
    free(bytes);
    return ok;
}

int aoe_shadow_table_load(DrsArchive *const *archives,
                          size_t archive_count, uint8_t out[256]) {
    return aoe_color_table_load_drs(archives, archive_count, 50909, out);
}

int aoe_color_table_set_load_directory(const DatIntro *intro,
                                       const char *directory,
                                       AoeColorTableSet *out) {
    AoeColorTableSet *next;
    if (!intro || !intro->colors || !intro->color_count ||
        intro->color_count > AOE_COLOR_TABLE_MAX || !directory || !out)
        return 0;
    next = calloc(1, sizeof(*next));
    if (!next) return 0;
    next->count = intro->color_count;
    for (uint16_t i = 0; i < intro->color_count; ++i) {
        uint16_t ordinal = intro->colors[i].ordinal;
        const char *name = intro->colors[i].name;
        char path[1024];
        int written;
        FILE *file;
        written = snprintf(path, sizeof(path), "%s/%s", directory, name);
        if (ordinal >= AOE_COLOR_TABLE_MAX || ordinal >= intro->color_count ||
            next->present[ordinal] || !name[0] || strchr(name, '/') ||
            strchr(name, '\\') || strstr(name, "..") || written < 0 ||
            (size_t)written >= sizeof(path)) {
            free(next);
            return 0;
        }
        next->present[ordinal] = 1;
        file = fopen(path, "rb");
        /* The original runs on a case-insensitive Windows filesystem. Game
           media commonly stores shadow.col while the DAT names Shadow.col. */
        if (!file) {
            char folded[1024];
            memcpy(folded, path, (size_t)written + 1);
            ascii_lower(folded + strlen(directory) + 1);
            file = fopen(folded, "rb");
        }
        if (file) {
            uint8_t bytes[16384];
            size_t size = fread(bytes, 1, sizeof(bytes), file);
            int complete = feof(file) && !ferror(file);
            fclose(file);
            if (complete && aoe_color_table_parse(bytes, size,
                                                   next->tables[ordinal]))
                next->loaded[ordinal] = 1;
        }
    }
    memcpy(out, next, sizeof(*out));
    free(next);
    return 1;
}

int aoe_color_table_set_load_drs_fallback(
    const DatIntro *intro, DrsArchive *const *archives, size_t archive_count,
    AoeColorTableSet *set) {
    if (!intro || !archives || !archive_count || !set ||
        set->count != intro->color_count || intro->color_count != 11)
        return 0;
    for (uint16_t ordinal = 0; ordinal < intro->color_count; ++ordinal) {
        uint32_t resource_id;
        if (set->loaded[ordinal]) continue;
        if (ordinal <= 8) resource_id = 50900u + ordinal;
        else if (ordinal == 9) resource_id = 50910;
        else resource_id = 50909;
        if (!aoe_color_table_load_drs(archives, archive_count, resource_id,
                                      set->tables[ordinal]))
            return 0;
        set->loaded[ordinal] = 1;
    }
    return 1;
}

const uint8_t *aoe_color_table_set_resolve(void *context, int16_t color_id) {
    const AoeColorTableSet *set = context;
    if (!set || color_id < 0 || (size_t)color_id >= set->count ||
        color_id >= AOE_COLOR_TABLE_MAX || !set->present[color_id]) return NULL;
    return set->tables[color_id];
}
