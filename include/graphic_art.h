#ifndef VITA_AOE_GRAPHIC_ART_H
#define VITA_AOE_GRAPHIC_ART_H

#include "dat.h"
#include "drs.h"
#include "graphic_frame.h"
#include "slp.h"

enum {
    AOE_GRAPHIC_ART_MISSING = 0,
    AOE_GRAPHIC_ART_OK = 1,
    AOE_GRAPHIC_ART_UNSUPPORTED_SHP = -1,
    AOE_GRAPHIC_ART_INVALID = -2,
    AOE_GRAPHIC_ART_CACHE_LIMIT = -3
};

/* Resolves the original archive entry without reading the resource. */
int aoe_graphic_art_find(const DatGraphic *graphic,
                         DrsArchive *const *archives,
                         size_t archive_count, DrsMatch *out_match);

/* Loads one selected original SLP frame. MISSING includes negative DAT SLP
   IDs, which have no direct DRS art and may require a different load path.
   The executable first searches for
   type " phs" and then " pls"; a found SHP is reported as unsupported,
   never silently replaced by an SLP. Archives must be in registration order
   (Rise of Rome overlays before base archives). Pass an empty SlpFrame.
   On success, caller owns out->pixels and must call slp_free_frame.
   On failure, out is unchanged. This direct loader is uncached. */
int aoe_graphic_art_load_frame(const DatGraphic *graphic,
                               const AoeGraphicFrame *selection,
                               DrsArchive *const *archives,
                               size_t archive_count, SlpFrame *out);

/* Reads the original resource's declared frame count without decoding pixels.
   Uses the same SHP-first archive lookup and status codes. */
int aoe_graphic_art_frame_count(const DatGraphic *graphic,
                                DrsArchive *const *archives,
                                size_t archive_count, uint32_t *out_count);

#endif
