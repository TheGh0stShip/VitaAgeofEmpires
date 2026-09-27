#include "graphic_art.h"

#include <stdlib.h>

static int load_slp_bytes(const DatGraphic *graphic,
                          DrsArchive *const *archives, size_t archive_count,
                          uint8_t **out_bytes, size_t *out_size) {
    if (!out_bytes || !out_size)
        return AOE_GRAPHIC_ART_INVALID;
    DrsMatch match;
    int status = aoe_graphic_art_find(graphic, archives, archive_count,
                                      &match);
    if (status != AOE_GRAPHIC_ART_OK)
        return status;
    uint8_t *bytes = malloc(match.entry->size ? match.entry->size : 1);
    if (!bytes)
        return AOE_GRAPHIC_ART_INVALID;
    if (!drs_read(match.archive, match.entry, bytes, match.entry->size)) {
        free(bytes);
        return AOE_GRAPHIC_ART_INVALID;
    }
    *out_bytes = bytes;
    *out_size = match.entry->size;
    return AOE_GRAPHIC_ART_OK;
}

int aoe_graphic_art_find(const DatGraphic *graphic,
                         DrsArchive *const *archives,
                         size_t archive_count, DrsMatch *out_match) {
    if (!graphic || !graphic->present || !archives || !archive_count ||
        !out_match)
        return AOE_GRAPHIC_ART_INVALID;
    if (graphic->slp_id < 0)
        return AOE_GRAPHIC_ART_MISSING;
    uint32_t id = (uint32_t)graphic->slp_id;
    if (drs_find_first(archives, archive_count, drs_type(" phs"), id,
                       out_match))
        return AOE_GRAPHIC_ART_UNSUPPORTED_SHP;
    if (!drs_find_first(archives, archive_count, drs_type(" pls"), id,
                        out_match))
        return AOE_GRAPHIC_ART_MISSING;
    return AOE_GRAPHIC_ART_OK;
}

int aoe_graphic_art_load_frame(const DatGraphic *graphic,
                               const AoeGraphicFrame *selection,
                               DrsArchive *const *archives,
                               size_t archive_count, SlpFrame *out) {
    if (!selection || !out || out->pixels)
        return AOE_GRAPHIC_ART_INVALID;
    uint8_t *bytes = NULL;
    size_t size = 0;
    int status = load_slp_bytes(graphic, archives, archive_count,
                                &bytes, &size);
    if (status != AOE_GRAPHIC_ART_OK)
        return status;
    uint32_t frame_count = 0;
    SlpFrame decoded = {0};
    int ok = slp_frame_count(bytes, size, &frame_count) &&
             selection->slp_frame < frame_count &&
             slp_decode_frame(bytes, size,
                              selection->slp_frame, &decoded);
    free(bytes);
    if (!ok) {
        slp_free_frame(&decoded);
        return AOE_GRAPHIC_ART_INVALID;
    }
    *out = decoded;
    return AOE_GRAPHIC_ART_OK;
}

int aoe_graphic_art_frame_count(const DatGraphic *graphic,
                                DrsArchive *const *archives,
                                size_t archive_count, uint32_t *out_count) {
    if (!out_count)
        return AOE_GRAPHIC_ART_INVALID;
    uint8_t *bytes = NULL;
    size_t size = 0;
    int status = load_slp_bytes(graphic, archives, archive_count,
                                &bytes, &size);
    if (status != AOE_GRAPHIC_ART_OK)
        return status;
    uint32_t count;
    int ok = slp_frame_count(bytes, size, &count);
    free(bytes);
    if (!ok)
        return AOE_GRAPHIC_ART_INVALID;
    *out_count = count;
    return AOE_GRAPHIC_ART_OK;
}
