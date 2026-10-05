#include "dib_picture.h"

#include <stdlib.h>
#include <string.h>

#include "le_bytes.h"
#include "unit_class.h"

#define DIB_HEADER_AND_PALETTE 0x428u

static void clear_fields(AoeDibPictureRor *p) {
    p->dib = NULL;
    p->info = NULL;
    p->bits = NULL;
    p->spans = NULL;
    p->mode = 0;
    p->width = 0;
    p->height = 0;
    p->orientation = 1;
    p->dib_size = 0;
}

/* The pixel rows follow the header and either three masks (compression
   3, BI_BITFIELDS) or clrUsed palette entries. Port: an offset beyond the
   block yields NULL. */
static uint8_t *bits_of(uint8_t *dib, size_t size) {
    uint32_t header, offset;
    if (!dib || size < 0x24) return NULL;
    header = aoe_le_u32(dib);
    if (aoe_le_u32(dib + 0x10) == 3)
        offset = header + 0xc;
    else
        offset = header + aoe_le_u32(dib + 0x20) * 4u;
    return offset <= size ? dib + offset : NULL;
}

void aoe_dib_picture_load_ror(AoeDibPictureRor *picture, const char *name,
                              int32_t resource_id, int32_t spans,
                              void *remap_table, int32_t remap_extra,
                              const AoeDibPictureOps *ops) {
    size_t size = 0;
    int32_t height;

    if (!picture) return;
    picture->spans = NULL;
    picture->mode = 1;
    if (ops && ops->prefer_files && ops->load_dib)
        picture->dib = ops->load_dib(ops->context, name, &size);
    if (!picture->dib && resource_id != -1 && ops && ops->resource_file) {
        char path[260];
        memset(path, 0, sizeof(path));
        if (ops->resource_file(ops->context, 0x62696e61u, resource_id,
                               path)) {
            if (ops->load_dib)
                picture->dib = ops->load_dib(ops->context, path, &size);
            if (ops->remove_file) ops->remove_file(ops->context, path);
        }
    }
    if (ops && !ops->prefer_files && !picture->dib && ops->load_dib)
        picture->dib = ops->load_dib(ops->context, name, &size);
    if (!picture->dib) {
        clear_fields(picture);
        return;
    }
    picture->dib_size = size;
    picture->info = picture->dib;
    picture->bits = bits_of(picture->dib, size);
    picture->width = size >= 0xc ? aoe_le_i32(picture->dib + 4) : 0;
    height = size >= 0xc ? aoe_le_i32(picture->dib + 8) : 0;
    picture->height = height;
    if (height < 0) {
        picture->orientation = 1;
        picture->height = -height;
    } else {
        picture->orientation = -1;
    }
    if (remap_table && ops && ops->remap)
        ops->remap(ops->context, picture->dib, remap_table, spans,
                   remap_extra);
    if (!spans) return;
    picture->spans = ops && ops->build_spans
                         ? ops->build_spans(ops->context, picture->dib,
                                            bits_of(picture->dib, size))
                         : NULL;
    if (picture->spans) return;
    if (ops && ops->release_dib) ops->release_dib(ops->context, picture->dib);
    picture->dib = NULL;
    picture->info = NULL;
    picture->mode = 0;
    picture->width = 0;
    picture->height = 0;
    picture->orientation = 1;
    picture->dib_size = 0;
}

void aoe_dib_picture_init_ror(AoeDibPictureRor *picture, const char *name,
                              int32_t resource_id, int32_t spans,
                              void *remap_table, int32_t remap_extra,
                              const AoeDibPictureOps *ops) {
    if (!picture) return;
    clear_fields(picture);
    aoe_dib_picture_load_ror(picture, name, resource_id, spans, remap_table,
                             remap_extra, ops);
}

static int16_t stream_i16(AoeOutcomeStream *s) {
    const uint8_t *p = aoe_stream_take(s, 2);
    return p ? (int16_t)(uint16_t)(p[0] | (p[1] << 8)) : 0;
}

int aoe_dib_picture_load_saved_ror(AoeDibPictureRor *picture,
                                   AoeOutcomeStream *in, int32_t spans,
                                   const AoeDibPictureOps *ops) {
    if (!picture || !in) return 0;
    clear_fields(picture);
    picture->mode = aoe_stream_i32(in);
    picture->width = aoe_stream_i32(in);
    picture->height = aoe_stream_i32(in);
    picture->orientation = stream_i16(in);
    if (picture->mode != 0) picture->mode = 2;
    if (picture->width < 1 || picture->height < 1) {
        picture->dib = NULL;
        picture->info = NULL;
        picture->bits = NULL;
    } else {
        uint64_t size = (uint64_t)aoe_dib_stride_ror(picture->width) *
                            (uint64_t)picture->height +
                        DIB_HEADER_AND_PALETTE;
        const uint8_t *src;
        if (size > in->size) return 0; /* port: cannot be in the stream */
        src = aoe_stream_take(in, (size_t)size);
        if (!src) return 0;
        picture->dib = malloc((size_t)size);
        if (!picture->dib) return 0;
        memcpy(picture->dib, src, (size_t)size);
        picture->dib_size = (size_t)size;
        picture->info = picture->dib;
        picture->bits = picture->dib + DIB_HEADER_AND_PALETTE;
    }
    picture->spans = NULL;
    if (spans && picture->dib) {
        picture->spans =
            ops && ops->build_spans
                ? ops->build_spans(ops->context, picture->dib,
                                   bits_of(picture->dib, picture->dib_size))
                : NULL;
        if (!picture->spans) {
            free(picture->dib);
            picture->dib = NULL;
            picture->info = NULL;
            picture->mode = 0;
            picture->width = 0;
            picture->height = 0;
            picture->orientation = 1;
            picture->dib_size = 0;
        }
    }
    return !in->failed;
}

static void put_i16(AoeOutcomeStream *s, int16_t v) {
    uint8_t b[2];
    b[0] = (uint8_t)((uint16_t)v & 0xff);
    b[1] = (uint8_t)((uint16_t)v >> 8);
    aoe_stream_put(s, b, 2);
}

int aoe_dib_picture_save_ror(const AoeDibPictureRor *picture,
                             AoeOutcomeStream *out) {
    if (!picture || !out) return 0;
    aoe_stream_put_i32(out, picture->mode);
    aoe_stream_put_i32(out, picture->width);
    aoe_stream_put_i32(out, picture->height);
    put_i16(out, picture->orientation);
    if (picture->width > 0 && picture->height > 0) {
        uint64_t rows = (uint64_t)aoe_dib_stride_ror(picture->width) *
                        (uint64_t)picture->height;
        if (!picture->info || !picture->bits || !picture->dib) return 0;
        if ((size_t)(picture->bits - picture->dib) + rows >
                picture->dib_size ||
            DIB_HEADER_AND_PALETTE > picture->dib_size)
            return 0; /* port: the original reads past a short block */
        aoe_stream_put(out, picture->info, DIB_HEADER_AND_PALETTE);
        aoe_stream_put(out, picture->bits, (size_t)rows);
    }
    return !out->failed;
}

void aoe_dib_picture_destroy_ror(AoeDibPictureRor *picture,
                                 const AoeDibPictureOps *ops) {
    if (!picture || picture->mode == 0) return;
    if (picture->spans && ops && ops->release_spans)
        ops->release_spans(ops->context, picture->spans);
    picture->spans = NULL;
    if (picture->dib) {
        if (picture->mode == 2)
            free(picture->dib);
        else if (ops && ops->release_dib)
            ops->release_dib(ops->context, picture->dib);
    }
    picture->dib = NULL;
    picture->info = NULL;
    picture->bits = NULL;
}

void aoe_dib_picture_draw_ror(const AoeDibPictureRor *picture, void *surface,
                              int32_t x, int32_t y, int32_t a, int32_t b,
                              const AoeDibPictureOps *ops) {
    if (!picture || !ops || !ops->draw) return;
    ops->draw(ops->context, surface, picture, x, y, picture->spans != NULL,
              a, b);
}
