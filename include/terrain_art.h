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


/* 0x00446bb0: the terrain text table (32 records of 0x198 bytes at map
   +0x8c): every record's used flag cleared, the file opened ("r"), the
   count (%hd) into map +0x8d90, then per record "%hd %s %s %d %hd %hd
   %hd %hd %hd %hd %hd %f %f %hd %hd %hd %hd %hd %hd %hd %hd": id, name,
   file name, +0x1c, (an unused short), bytes +0x29, +0x28, +0x2a, the
   colour table index (+0x24 takes the caller's table entry, 0 for a
   negative index), byte +0x2f, +0x30, the float +0x34 (its truncation
   at +0x32; 0 for 0.0), the float +0x38, +0xb8, +0xba, +0xbc, bytes
   +0x2b, +0x2c, +0x2e, +0x2d and the pair count; then 19 "%hd %hd"
   shape pairs (count, size; offsets a running sum) and `pairs` "%hd %hd"
   (index, value) into the +0xbe table. Underscores in the name become
   spaces; +0x40, +0x3c, +0x3e, +0x44 and +0x45 are cleared, used set.
   Records with an id outside 0..31 are skipped. */
typedef struct {
    uint8_t used;
    char name[13];       /* +0x02 */
    char file_name[13];  /* +0x0f */
    int32_t value_1c;
    int32_t color_table; /* +0x24: the caller's entry or 0 */
    uint8_t b28, b29, b2a, b2b, b2c, b2d, b2e, b2f;
    int16_t w30;
    int16_t value_whole; /* +0x32 */
    float value;         /* +0x34 */
    float value2;        /* +0x38 */
    int16_t w3c, w3e;
    int32_t field_40;
    uint8_t b44, b45;
    BorderTextShape shapes[19];  /* +0x46: count, size, offset */
    int16_t w_b8, w_ba, w_bc;
    int16_t table_be[0x6d];      /* +0xbe */
} TerrainTextRecord;
int terrain_text_load(TerrainTextRecord records[32], const char *text, const int32_t *color_tables,
                      int16_t *count_out);

#endif
