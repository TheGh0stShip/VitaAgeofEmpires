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
