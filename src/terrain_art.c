#include "terrain_art.h"

#include <stdlib.h>
#include <string.h>

/* 1 loaded, 0 missing, -1 invalid resource or allocation failure. The
   caller owns any partially decoded frames left on failure. */
static int load_one(TerrainArtEntry *art, int32_t slp_id,
                    DrsArchive *const *archives, size_t archive_count) {
    DrsMatch match;
    uint8_t *bytes;
    uint32_t frame_count;
    if (!drs_find_first(archives, archive_count, drs_type(" pls"),
                        (uint32_t)slp_id, &match)) return 0;
    bytes = malloc(match.entry->size ? match.entry->size : 1);
    if (!bytes) return -1;
    int ok = drs_read(match.archive, match.entry, bytes,
                      match.entry->size) &&
             slp_frame_count(bytes, match.entry->size, &frame_count);
    if (!ok || frame_count == 0) {
        free(bytes);
        return -1;
    }
    art->frames = calloc(frame_count, sizeof(SlpFrame));
    if (!art->frames) {
        free(bytes);
        return -1;
    }
    art->slp_id = slp_id;
    art->frame_count = frame_count;
    for (uint32_t j = 0; j < frame_count; ++j)
        if (!slp_decode_frame(bytes, match.entry->size, j,
                              &art->frames[j])) {
            free(bytes);
            return -1;
        }
    free(bytes);
    art->plain_frames = malloc(frame_count);
    if (art->plain_frames)
        for (uint32_t j = 0; j < frame_count; ++j) {
            const SlpFrame *f = &art->frames[j];
            size_t n = (size_t)f->width * f->height;
            uint8_t plain = f->pixels != NULL;
            for (size_t k = 0; plain && k < n; ++k)
                plain = f->pixels[k].kind == SLP_COLOR ||
                        f->pixels[k].kind == SLP_TRANSPARENT;
            art->plain_frames[j] = plain;
        }
    return 1;
}

void terrain_art_free(TerrainArtCache *cache) {
    if (!cache) return;
    for (uint16_t i = 0; i < cache->count; ++i) {
        TerrainArtEntry *entry = &cache->entries[i];
        for (uint32_t j = 0; j < entry->frame_count; ++j)
            slp_free_frame(&entry->frames[j]);
        free(entry->frames);
        free(entry->plain_frames);
    }
    free(cache->entries);
    memset(cache, 0, sizeof(*cache));
}

int terrain_art_load_direct(TerrainArtCache *cache, const DatTerrains *terrains,
                            DrsArchive *const *archives, size_t archive_count) {
    if (!cache || !terrains || !terrains->terrains || !archives ||
        !archive_count || !terrains->count) return 0;
    memset(cache, 0, sizeof(*cache));
    cache->entries = calloc(terrains->count, sizeof(TerrainArtEntry));
    if (!cache->entries) return 0;
    cache->count = terrains->count;
    for (uint16_t i = 0; i < cache->count; ++i)
        cache->entries[i].slp_id = -1;
    for (uint16_t i = 0; i < terrains->count; ++i) {
        const DatTerrain *terrain = &terrains->terrains[i];
        TerrainArtEntry *art = &cache->entries[i];
        if (!terrain->enabled || terrain->slp_id < 0) continue;
        if (load_one(art, terrain->slp_id, archives, archive_count) != 1)
            goto fail;
        for (unsigned shape = 0; shape < 17; ++shape) {
            const DatTerrainFrame *span = &terrain->elevation_frames[shape];
            if (span->frame_count <= 0 || span->frame_start < 0 ||
                span->angle_count <= 0 ||
                (uint64_t)span->frame_start +
                    (uint64_t)span->frame_count *
                    (uint64_t)span->angle_count > art->frame_count)
                goto fail;
        }
    }
    return 1;
fail:
    terrain_art_free(cache);
    return 0;
}

int border_art_load_direct(BorderArtCache *cache, const DatBorders *borders,
                           DrsArchive *const *archives, size_t archive_count) {
    if (!cache || !borders || !borders->borders || !archives ||
        !archive_count || !borders->count) return 0;
    memset(cache, 0, sizeof(*cache));
    cache->entries = calloc(borders->count, sizeof(BorderArtEntry));
    if (!cache->entries) return 0;
    cache->count = borders->count;
    for (uint16_t i = 0; i < cache->count; ++i)
        cache->entries[i].slp_id = -1;
    for (uint16_t i = 0; i < borders->count; ++i) {
        const DatBorder *border = &borders->borders[i];
        BorderArtEntry *art = &cache->entries[i];
        if (!border->enabled || border->slp_id < 0) continue;
        int loaded = load_one(art, border->slp_id, archives, archive_count);
        if (loaded < 0) goto fail;
        if (loaded == 0) continue;
        for (unsigned shape = 0; shape < 17; ++shape)
            for (unsigned pattern = 1; pattern <= 15; ++pattern) {
                unsigned flat = shape * 12 + pattern - 1;
                const DatTerrainFrame *span =
                    &border->frames[flat / 12][flat % 12];
                if (span->frame_count == 0) continue;
                if (span->frame_count < 0 || span->angle_count <= 0 ||
                    span->frame_start < 0 ||
                    (uint64_t)span->frame_start +
                        (uint64_t)span->frame_count *
                        (uint64_t)span->angle_count > art->frame_count)
                    goto fail;
            }
    }
    return 1;
fail:
    terrain_art_free(cache);
    return 0;
}

void border_art_free(BorderArtCache *cache) {
    terrain_art_free(cache);
}

#include <stdio.h>
#include <stdlib.h>

static int border_text_token(const char **cursor, char *out, size_t cap) {
    const char *p = *cursor;
    size_t n = 0;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
    if (!*p) { *cursor = p; return 0; }
    while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
        if (n + 1 < cap) out[n++] = *p;
        ++p;
    }
    out[n] = 0;
    *cursor = p;
    return 1;
}

static int16_t border_text_hd(const char **cursor) {
    char tok[64];
    if (!border_text_token(cursor, tok, sizeof tok)) return 0;
    return (int16_t)strtol(tok, NULL, 10);
}

/* 0x00446f80 */
int border_text_load(BorderTextRecord records[16], const char *text) {
    char tok[64];
    int16_t count;
    for (int i = 0; i < 16; ++i) records[i].used = 0;
    if (!text) return 0;
    const char *cur = text;
    count = border_text_hd(&cur);
    for (int r = 0; r < count; ++r) {
        char name[64], file[64];
        if (!border_text_token(&cur, tok, sizeof tok)) break;
        int16_t id = (int16_t)strtol(tok, NULL, 10);
        border_text_token(&cur, name, sizeof name);
        border_text_token(&cur, file, sizeof file);
        int32_t slp = (int32_t)strtol(border_text_token(&cur, tok, sizeof tok) ? tok : "0", NULL, 10);
        int16_t f[7];
        for (int k = 0; k < 7; ++k) f[k] = border_text_hd(&cur);
        float v1 = border_text_token(&cur, tok, sizeof tok) ? strtof(tok, NULL) : 0.0f;
        float v2 = border_text_token(&cur, tok, sizeof tok) ? strtof(tok, NULL) : 0.0f;
        int16_t g[3];
        for (int k = 0; k < 3; ++k) g[k] = border_text_hd(&cur);
        if (id < 0 || id >= 16) continue;
        BorderTextRecord *b = &records[id];
        int16_t offset = 0;
        for (int row = 0; row < 19; ++row)
            for (int c = 0; c < 12; ++c) {
                BorderTextShape *s = &b->shapes[row][c];
                s->count = border_text_hd(&cur);
                s->size = border_text_hd(&cur);
                s->offset = offset;
                offset = (int16_t)(offset + s->size * s->count);
            }
        b->color_table = f[4];
        b->b2a = (uint8_t)f[3];
        b->b29 = (uint8_t)f[1];
        b->b28 = (uint8_t)f[2];
        for (char *p = name; *p; ++p) if (*p == '_') *p = ' ';
        name[sizeof b->name - 1] = 0;
        memcpy(b->name, name, sizeof b->name);
        file[sizeof b->file_name - 1] = 0;
        memcpy(b->file_name, file, sizeof b->file_name);
        b->used = 1;
        b->slp_id = slp;
        b->b2b = (uint8_t)f[5];
        b->w2c = f[6];
        b->value_whole = v1 == 0.0f ? 0 : (int16_t)(int32_t)v1;
        b->value = v1;
        b->value2 = v2;
        b->b59a = (uint8_t)g[0];
        b->w59c = g[1];
        b->w59e = g[2];
    }
    return 1;
}

int terrain_text_load(TerrainTextRecord records[32], const char *text, const int32_t *color_tables,
                      int16_t *count_out) {
    const char *p = text;
    int i, consumed;
    short count;
    for (i = 0; i < 32; ++i) records[i].used = 0;
    if (!text) return 0;
    if (sscanf(p, " %hd%n", &count, &consumed) < 1) return 1;
    p += consumed;
    if (count_out) *count_out = count;
    for (i = 0; i < count; ++i) {
        short id, b29, b28, b2a, color, b2f, w30, w_b8, w_ba, w_bc, b2b, b2c, b2e, b2d, pairs, unused;
        int v1c;
        float f34, f38;
        char name[64], file[64];
        TerrainTextRecord *r;
        int n = sscanf(p, " %hd %63s %63s %d %hd %hd %hd %hd %hd %hd %hd %f %f %hd %hd %hd %hd %hd %hd %hd %hd%n",
                       &id, name, file, &v1c, &unused, &b29, &b28, &b2a, &color, &b2f, &w30, &f34,
                       &f38, &w_b8, &w_ba, &w_bc, &b2b, &b2c, &b2e, &b2d, &pairs, &consumed);
        if (n < 21) return 1;
        p += consumed;
        if (id < 0 || id >= 32) {
            int k;
            short a, b;
            for (k = 0; k < 19 + pairs; ++k) {
                if (sscanf(p, " %hd %hd%n", &a, &b, &consumed) < 2) return 1;
                p += consumed;
            }
            continue;
        }
        r = &records[id];
        memset(r, 0, sizeof *r);
        {
            short running = 0, k;
            for (k = 0; k < 19; ++k) {
                if (sscanf(p, " %hd %hd%n", &r->shapes[k].count, &r->shapes[k].size, &consumed) < 2)
                    return 1;
                p += consumed;
                r->shapes[k].offset = running;
                running = (short)(running + r->shapes[k].count * r->shapes[k].size);
            }
            for (k = 0; k < pairs; ++k) {
                short idx, val;
                if (sscanf(p, " %hd %hd%n", &idx, &val, &consumed) < 2) return 1;
                p += consumed;
                if (idx >= 0 && idx < 0x6d) r->table_be[idx] = val;
            }
        }
        r->color_table = color < 0 || !color_tables ? 0 : color_tables[color];
        r->b2a = (uint8_t)b2a; r->b29 = (uint8_t)b29; r->b28 = (uint8_t)b28;
        r->b2b = (uint8_t)b2b; r->b2c = (uint8_t)b2c; r->b2e = (uint8_t)b2e; r->b2d = (uint8_t)b2d;
        {
            char *u;
            for (u = name; *u; ++u) if (*u == '_') *u = ' ';
        }
        snprintf(r->name, sizeof r->name, "%.12s", name);
        snprintf(r->file_name, sizeof r->file_name, "%.12s", file);
        r->value_1c = v1c;
        r->used = 1;
        r->w_ba = w_ba; r->w_bc = w_bc; r->w_b8 = w_b8;
        r->b2f = (uint8_t)b2f;
        r->w30 = w30;
        r->value_whole = f34 == 0.0f ? 0 : (int16_t)f34;
        r->value = f34;
        r->value2 = f38;
    }
    return 1;
}
