#ifndef VITA_AOE_TERRAIN_ART_H
#define VITA_AOE_TERRAIN_ART_H

#include "dat.h"
#include "drs.h"
#include "slp.h"

typedef struct {
    int32_t slp_id;
    uint32_t frame_count;
    SlpFrame *frames;
    /* Optional per-frame flag computed at load: 1 when the frame holds only
       color/transparent pixels, the precondition the terrain blitters
       otherwise re-check on every call. NULL means "not computed". */
    uint8_t *plain_frames;
} TerrainArtEntry;

typedef struct {
    uint16_t count;
    TerrainArtEntry *entries; /* Indexed by DAT terrain slot. */
} TerrainArtCache;

typedef TerrainArtEntry BorderArtEntry;
typedef TerrainArtCache BorderArtCache;

/* Patched RoR 1.0a 0x00446ae0 (disc 0x00447900) loads the 32 terrain
   slots' art from the last slot down: a slot whose art cannot be loaded
   (0x0049f5f0 / 0x0049f9e0) is disabled, and its colour table (+8) is
   taken from the table list when not negative (else none); the port
   leaves the colour tables to the renderer.
   Loads only enabled terrains with a direct SLP ID. Archive order follows
   the original DRS lookup precedence. Every DAT-declared shape span must
   fit the decoded sprite. Pass an empty cache; free it before reloading.
   Replacement art is not resolved here. */
int terrain_art_load_direct(TerrainArtCache *cache, const DatTerrains *terrains,
                            DrsArchive *const *archives, size_t archive_count);
void terrain_art_free(TerrainArtCache *cache);

/* Patched RoR 1.0a 0x00446eb0 (disc 0x00447cd0; the 16 border slots, the
   same way): load direct border art for enabled slots. A
   missing resource leaves that slot without frames, matching the original
   loader's disabling of unavailable art (notably border ID 1 / SLP 20005).
   Malformed present resources fail the load. No SHP or loose-file fallback. */
int border_art_load_direct(BorderArtCache *cache, const DatBorders *borders,
                           DrsArchive *const *archives, size_t archive_count);
void border_art_free(BorderArtCache *cache);

#endif
