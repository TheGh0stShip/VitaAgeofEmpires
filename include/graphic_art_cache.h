#ifndef VITA_AOE_GRAPHIC_ART_CACHE_H
#define VITA_AOE_GRAPHIC_ART_CACHE_H

#include "graphic_art.h"

typedef struct AoeGraphicArtCacheEntry AoeGraphicArtCacheEntry;

typedef struct {
    AoeGraphicArtCacheEntry *entries;
    size_t capacity, max_pixel_bytes, used_pixel_bytes;
    uint64_t stamp, hits, misses, evictions;
} AoeGraphicArtCache;

/* Fixed-count, byte-bounded LRU of decoded original SLP frames. Initialize
   an empty cache; clear it before closing/replacing any source DRS archive.
   Pixel budget excludes slot metadata. */
int aoe_graphic_art_cache_init(AoeGraphicArtCache *cache,
                               size_t capacity, size_t max_pixel_bytes);
void aoe_graphic_art_cache_clear(AoeGraphicArtCache *cache);

/* Returns a borrowed frame pointer valid until the next cache miss or clear.
   CACHE_LIMIT means the selected frame exceeds the configured pixel budget;
   callers can use aoe_graphic_art_load_frame as an uncached fallback. */
int aoe_graphic_art_cache_get(AoeGraphicArtCache *cache,
                              const DatGraphic *graphic,
                              const AoeGraphicFrame *selection,
                              DrsArchive *const *archives,
                              size_t archive_count,
                              const SlpFrame **out_frame);

#endif
