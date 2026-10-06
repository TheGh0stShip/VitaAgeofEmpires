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

/* Text border table (one record per border slot 0..15). */
typedef struct {
    int16_t count, size, offset; /* offset: running sum of count*size */
} BorderTextShape;

typedef struct {
    uint8_t used;
    char name[13];       /* underscores become spaces */
    char file_name[13];
    int32_t slp_id;
    int32_t color_table; /* index into the caller's table list, -1 none */
    uint8_t b28, b29, b2a, b2b;
    int16_t w2c;
    int16_t value_whole; /* truncated `value`; 0 when value is 0.0 */
    float value;
    float value2;
    BorderTextShape shapes[19][12];
    uint8_t b59a;
    int16_t w59c, w59e;
} BorderTextRecord;

/* 0x00446f80: the 16 border records are cleared, then the text
   "count" followed by count records of
   "%hd %s %s %d %hd %hd %hd %hd %hd %hd %hd %f %f %hd %hd %hd" (id, name,
   file, SLP, unused, +0x29, +0x28, +0x2a, colour table, +0x2b, +0x2c,
   value, value2, +0x59a, +0x59c, +0x59e) each followed by 19x12
   "%hd %hd" shape pairs is parsed. A record whose id is outside 0..15 is
   ignored. Returns 0 when `text` is NULL (the file did not open). */
int border_text_load(BorderTextRecord records[16], const char *text);

#endif
