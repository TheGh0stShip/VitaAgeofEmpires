#ifndef VITA_AOE_COLOR_TABLE_H
#define VITA_AOE_COLOR_TABLE_H

#include <stddef.h>
#include <stdint.h>

#include "dat.h"
#include "drs.h"

#define AOE_COLOR_TABLE_MAX 64

typedef struct {
    size_t count;
    uint8_t present[AOE_COLOR_TABLE_MAX];
    uint8_t loaded[AOE_COLOR_TABLE_MAX];
    uint8_t tables[AOE_COLOR_TABLE_MAX][256];
} AoeColorTableSet;

/* Original .col assets are 256 decimal palette indices separated by ASCII
   whitespace. Leaves out unchanged on malformed input. */
int aoe_color_table_parse(const uint8_t *data, size_t size,
                          uint8_t out[256]);

/* Resolve a text .col table from a registered DRS anib resource. */
int aoe_color_table_load_drs(DrsArchive *const *archives,
                             size_t archive_count, uint32_t resource_id,
                             uint8_t out[256]);

/* Base Interfac.drs anib ID 50909 is byte-for-byte identical to the
   installed data/shadow.col and is shared by Rise of Rome. Archives must be
   in registration order (expansion overlays first). */
int aoe_shadow_table_load(DrsArchive *const *archives,
                          size_t archive_count, uint8_t out[256]);

/* RoR 0x0041d4a0 constructs data/<DAT filename> and scans 256 values. The
   original object remains present with a zeroed table when the file cannot
   be opened. loaded distinguishes that case for diagnostics/packaging. */
int aoe_color_table_set_load_directory(const DatIntro *intro,
                                       const char *directory,
                                       AoeColorTableSet *out);

/* Installed North American media may omit loose .col files while Interfac
   carries the original text tables. Fills only unloaded known VER 3.7
   ordinals: player/fixed colors 0..8, Red_lt 9, and Shadow 10. */
int aoe_color_table_set_load_drs_fallback(
    const DatIntro *intro, DrsArchive *const *archives, size_t archive_count,
    AoeColorTableSet *set);

/* Callback-compatible lookup for AoeSpriteFixedColorTableRor. */
const uint8_t *aoe_color_table_set_resolve(void *context, int16_t color_id);

#endif
