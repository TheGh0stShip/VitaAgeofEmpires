#include "graphic_art_cache.h"

#include <stdlib.h>
#include <string.h>

struct AoeGraphicArtCacheEntry {
    const DatGraphic *graphic;
    DrsArchive *archive;
    const DrsEntry *entry;
    uint32_t frame_index;
    SlpFrame frame;
    size_t pixel_bytes;
    uint64_t stamp;
};

int aoe_graphic_art_cache_init(AoeGraphicArtCache *cache,
                               size_t capacity, size_t max_pixel_bytes) {
    if (!cache || !capacity || !max_pixel_bytes || capacity > UINT16_MAX ||
        capacity > SIZE_MAX / sizeof(AoeGraphicArtCacheEntry))
        return 0;
    AoeGraphicArtCache next = {0};
    next.entries = calloc(capacity, sizeof(*next.entries));
    if (!next.entries)
        return 0;
    next.capacity = capacity;
    next.max_pixel_bytes = max_pixel_bytes;
    *cache = next;
    return 1;
}

void aoe_graphic_art_cache_clear(AoeGraphicArtCache *cache) {
    if (!cache)
        return;
    for (size_t i = 0; i < cache->capacity; ++i)
        slp_free_frame(&cache->entries[i].frame);
    free(cache->entries);
    memset(cache, 0, sizeof(*cache));
}

static void evict(AoeGraphicArtCache *cache, size_t slot) {
    AoeGraphicArtCacheEntry *entry = &cache->entries[slot];
    cache->used_pixel_bytes -= entry->pixel_bytes;
    slp_free_frame(&entry->frame);
    memset(entry, 0, sizeof(*entry));
    ++cache->evictions;
}

static size_t index_of(const DatGraphic *graphic, uint32_t frame) {
    uintptr_t h = (uintptr_t)graphic >> 3;
    h ^= (uintptr_t)frame * 2654435761u;
    h ^= h >> 13;
    return (size_t)(h % AOE_GRAPHIC_ART_CACHE_INDEX);
}

int aoe_graphic_art_cache_get(AoeGraphicArtCache *cache,
                              const DatGraphic *graphic,
                              const AoeGraphicFrame *selection,
                              DrsArchive *const *archives,
                              size_t archive_count,
                              const SlpFrame **out_frame) {
    if (!cache || !cache->entries || !selection || !out_frame)
        return AOE_GRAPHIC_ART_INVALID;
    size_t hint = index_of(graphic, selection->slp_frame);
    if (graphic && cache->index[hint]) {
        AoeGraphicArtCacheEntry *slot =
            &cache->entries[cache->index[hint] - 1u];
        if (slot->frame.pixels && slot->graphic == graphic &&
            slot->frame_index == selection->slp_frame) {
            slot->stamp = ++cache->stamp;
            ++cache->hits;
            *out_frame = &slot->frame;
            return AOE_GRAPHIC_ART_OK;
        }
    }
    DrsMatch match;
    int status = aoe_graphic_art_find(graphic, archives, archive_count,
                                      &match);
    if (status != AOE_GRAPHIC_ART_OK)
        return status;
    for (size_t i = 0; i < cache->capacity; ++i) {
        AoeGraphicArtCacheEntry *slot = &cache->entries[i];
        if (slot->frame.pixels && slot->archive == match.archive &&
            slot->entry == match.entry &&
            slot->frame_index == selection->slp_frame) {
            slot->stamp = ++cache->stamp;
            ++cache->hits;
            if (slot->graphic == graphic)
                cache->index[hint] = (uint16_t)(i + 1);
            *out_frame = &slot->frame;
            return AOE_GRAPHIC_ART_OK;
        }
    }
    ++cache->misses;
    uint8_t *bytes = malloc(match.entry->size ? match.entry->size : 1);
    if (!bytes)
        return AOE_GRAPHIC_ART_INVALID;
    uint32_t frame_count = 0;
    SlpFrame frame = {0};
    int ok = drs_read(match.archive, match.entry, bytes, match.entry->size) &&
             slp_frame_count(bytes, match.entry->size, &frame_count) &&
             selection->slp_frame < frame_count &&
             slp_decode_frame(bytes, match.entry->size,
                              selection->slp_frame, &frame);
    free(bytes);
    if (!ok) {
        slp_free_frame(&frame);
        return AOE_GRAPHIC_ART_INVALID;
    }
    size_t pixel_bytes = (size_t)frame.width * frame.height *
                         sizeof(SlpPixel);
    if (pixel_bytes > cache->max_pixel_bytes) {
        slp_free_frame(&frame);
        return AOE_GRAPHIC_ART_CACHE_LIMIT;
    }
    size_t slot = cache->capacity;
    for (size_t i = 0; i < cache->capacity; ++i)
        if (!cache->entries[i].frame.pixels) {
            slot = i;
            break;
        }
    while (slot == cache->capacity ||
           pixel_bytes > cache->max_pixel_bytes - cache->used_pixel_bytes) {
        size_t oldest = cache->capacity;
        for (size_t i = 0; i < cache->capacity; ++i)
            if (cache->entries[i].frame.pixels &&
                (oldest == cache->capacity ||
                 cache->entries[i].stamp < cache->entries[oldest].stamp))
                oldest = i;
        if (oldest == cache->capacity) {
            slp_free_frame(&frame);
            return AOE_GRAPHIC_ART_INVALID;
        }
        evict(cache, oldest);
        if (slot == cache->capacity)
            slot = oldest;
    }
    AoeGraphicArtCacheEntry *dest = &cache->entries[slot];
    *dest = (AoeGraphicArtCacheEntry){
        graphic, match.archive, match.entry, selection->slp_frame, frame,
        pixel_bytes, ++cache->stamp,
    };
    if (graphic) cache->index[hint] = (uint16_t)(slot + 1);
    cache->used_pixel_bytes += pixel_bytes;
    *out_frame = &dest->frame;
    return AOE_GRAPHIC_ART_OK;
}
