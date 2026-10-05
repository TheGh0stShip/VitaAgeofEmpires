#ifndef VITA_AOE_DIB_PICTURE_H
#define VITA_AOE_DIB_PICTURE_H

#include <stddef.h>
#include <stdint.h>

#include "aoe_stream.h"

/* Patched Rise of Rome 1.0a device-independent bitmap holder (0x20
   bytes): a packed BITMAPINFO block (little-endian header, palette, then
   the pixel rows) loaded from a file, a 'bina' resource or a save, plus
   an optional transparency span table used when drawing. Semantic
   storage; original offsets noted. */
typedef struct {
    uint8_t *dib;          /* +0x00, the whole block */
    const uint8_t *info;   /* +0x04, the header (== dib) */
    uint8_t *bits;         /* +0x08 */
    void *spans;           /* +0x0c, built by the span builder */
    int32_t mode;          /* +0x10: 0 none, 1 file/resource, 2 save */
    int32_t width;         /* +0x14 */
    int32_t height;        /* +0x18, made positive */
    int16_t orientation;   /* +0x1c: 1 top-down (negative height), -1 */
    size_t dib_size;       /* host size of `dib` */
} AoeDibPictureRor;

typedef struct {
    void *context;
    /* The global "files first" flag. */
    int prefer_files;
    /* A packed DIB loaded from a file (or, failing that, an executable
       bitmap resource) of that name; NULL when neither exists. Released
       through `release_dib`. Sets *size to the block size. */
    uint8_t *(*load_dib)(void *context, const char *name, size_t *size);
    void (*release_dib)(void *context, uint8_t *dib);
    /* Resource lookup: extract resource `id` of type 'bina' to a
       temporary file (path written to `path`, its handle closed); then
       remove that file. */
    int (*resource_file)(void *context, uint32_t type, int32_t id,
                         char path[260]);
    void (*remove_file)(void *context, const char *path);
    /* The palette remap applied to a freshly loaded block. */
    void (*remap)(void *context, uint8_t *dib, void *table, int32_t spans,
                  int32_t extra);
    /* A new span table for the block and its rows (NULL only when the
       0x18-byte table cannot be allocated) and its release. */
    void *(*build_spans)(void *context, const uint8_t *dib,
                         const uint8_t *bits);
    void (*release_spans)(void *context, void *spans);
    /* Draws onto a drawing surface at x/y; with spans the transparent
       blitter, otherwise the opaque one (whole picture, both trailing
       arguments passed through). */
    void (*draw)(void *context, void *surface, const AoeDibPictureRor *p,
                 int32_t x, int32_t y, int with_spans, int32_t a,
                 int32_t b);
} AoeDibPictureOps;

/* 0x0045b1b0: load `name`. With prefer_files the file is tried first;
   otherwise (or when it is missing) resource `resource_id` (unless -1)
   is extracted to a temporary file and loaded from there; without
   prefer_files the file is tried last. The bits follow the header and
   either three masks (compression 3) or clrUsed palette entries. A
   negative height means top-down (orientation 1); otherwise -1. With a
   remap table the block is remapped; with `spans` a span table is built.
   Without a block, or when the span table cannot be allocated (the block
   is then released, the bits pointer left stale), the mode, size and
   orientation reset to 0, 0, 0 and 1. */
void aoe_dib_picture_load_ror(AoeDibPictureRor *picture, const char *name,
                              int32_t resource_id, int32_t spans,
                              void *remap_table, int32_t remap_extra,
                              const AoeDibPictureOps *ops);

/* 0x0045af10: the constructor (cleared fields, orientation 1) then the
   load above. */
void aoe_dib_picture_init_ror(AoeDibPictureRor *picture, const char *name,
                              int32_t resource_id, int32_t spans,
                              void *remap_table, int32_t remap_extra,
                              const AoeDibPictureOps *ops);

/* 0x0045af60: the save constructor: mode (any non-zero becomes 2), width,
   height (int32 each), orientation (int16), then for a positive size the
   0x428-byte header and palette and stride * height pixel bytes (stride
   0x0045b360). With `spans` a span table is built; failing that the block
   is dropped. Returns 0 for a short stream or allocation failure. Port:
   that drop frees the heap block (the original passes it to the file
   loader's release). */
int aoe_dib_picture_load_saved_ror(AoeDibPictureRor *picture,
                                   AoeOutcomeStream *in, int32_t spans,
                                   const AoeDibPictureOps *ops);

/* 0x0045b130: the fields read above and, for a positive size, the 0x428
   header bytes and the pixel rows. Returns 0 when the stream fills up or
   the block is shorter than that. */
int aoe_dib_picture_save_ror(const AoeDibPictureRor *picture,
                             AoeOutcomeStream *out);

/* 0x0045b0d0: for a non-zero mode, release the span table and the block
   (heap blocks of mode 2 with free, others through release_dib). */
void aoe_dib_picture_destroy_ror(AoeDibPictureRor *picture,
                                 const AoeDibPictureOps *ops);

/* 0x0045b370: draw at x/y (transparent when a span table exists). */
void aoe_dib_picture_draw_ror(const AoeDibPictureRor *picture, void *surface,
                              int32_t x, int32_t y, int32_t a, int32_t b,
                              const AoeDibPictureOps *ops);

#endif
