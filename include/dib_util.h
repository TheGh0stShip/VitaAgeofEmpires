#ifndef VITA_AOE_DIB_UTIL_H
#define VITA_AOE_DIB_UTIL_H

#include <stddef.h>
#include <stdint.h>

/* Patched Rise of Rome 1.0a DIB helpers (0x0042e810..0x0042f7c0): blit
   clipping, row pitch and orientation, the transparent-span table, BMP
   header and palette reading, and palette remapping of 8-bit, 4-bit and
   RLE8 pixels. DIBs are packed little-endian BITMAPINFO blocks. */

/* 0x0042e9f0: the row pitch (width rounded up to 4), the height made
   positive, and the direction: 1 for a top-down DIB (negative height),
   else -1. */
void aoe_dib_layout_ror(const uint8_t *info, uint32_t *pitch, int32_t *height,
                        int32_t *direction);

/* 0x0042e810: clip a blit of (x, y, w, h) at source (sx, sy) scaled to
   (sw, sh) against the destination (its whole area, or `area` x0, y0,
   x1, y1 inclusive); the source moves with the clipped edge unless that
   axis is flipped (then the far edge moves it). 1 when something is
   left. */
int aoe_dib_clip_ror(const uint8_t *dest_info, const int32_t *area, int32_t *x, int32_t *y,
                     int32_t *w, int32_t *h, int32_t *sx, int32_t *sy, int32_t *sw,
                     int32_t *sh, int flip_x, int flip_y);

/* 0x0042ea30: the transparency spans of an 8-bit DIB (colour 0 is
   transparent): per row, empty, solid between its first and last opaque
   pixels, and those two columns (empty rows: the width for both). The
   table is kept (`*spans`) only when some pixel is transparent; returns
   whether one is. */
typedef struct {
    int32_t empty, solid, first, last;
} AoeDibSpanRor;
int aoe_dib_spans_ror(const uint8_t *info, AoeDibSpanRor **spans, const uint8_t *bits);

/* 0x0042ef70: read a DIB's header and palette from a byte source (a
   BITMAPFILEHEADER optional; an OS/2 core header converted to 40 bytes,
   its triples widened); the colours default to 2^bits below 9 and the
   image size to the rows' size. NULL on a short read. `offset` gets the
   bits' position (0 without a file header). 0x0042ee40 then appends the
   image (`aoe_dib_read_ror`). 0x0042f7c0 frees one. */
uint8_t *aoe_dib_read_header_ror(const uint8_t *data, size_t size, size_t *pos,
                                 size_t *offset, size_t *block_size);
uint8_t *aoe_dib_read_ror(const uint8_t *data, size_t size, size_t *block_size);
void aoe_dib_free_ror(uint8_t *dib);

/* Palette remaps through a 256-entry table: 0x0042f580 8-bit pixels,
   0x0042f5b0 4-bit pairs, 0x0042f5f0 RLE8 data (runs and literals mapped,
   escapes kept). 0x0042f660 does nothing; 0x0042f670 copies backwards
   (overlap safe towards higher addresses). */
void aoe_dib_remap8_ror(uint8_t *p, int32_t n, const uint8_t *table);
void aoe_dib_remap4_ror(uint8_t *p, int32_t n, const uint8_t *table);
void aoe_dib_remap_rle8_ror(uint8_t *p, const uint8_t *table);
void aoe_dib_nothing_ror(void);
void aoe_dib_copy_back_ror(uint8_t *dst, const uint8_t *src, int32_t n);


/* 0x0042e460 (18 arguments): an 8-bit DIB blit with scaling, flips and
   a transparent colour. With a span table whose first dword is 0 the
   unscaled 0x42e1f0 runs instead. A negative transparent colour becomes
   0. Unless `clip` is 0 the blit is clipped first (0x0042e810; nothing
   left returns). Both DIBs give their pitch and direction (0x0042e9f0);
   the destination row is dest_y (or height - 1 - dest_y bottom-up), the
   source row sy (flipped: height - 1 - sy), each advancing by the
   pitch times the direction. The source column of each destination
   column is sx + i * (sw << 8) / w >> 8 (reversed when flipped in x);
   rows advance by (sh << 8) / h. With a span table and no x flip, each
   source row's span (empty rows skipped) is copied: whole when solid
   and unscaled (memcpy), else pixel by pixel skipping the transparent
   colour; without one every pixel is copied unless transparent. */
typedef struct {
    const uint8_t *dest_info; uint8_t *dest_bits;
    int32_t dest_x, dest_y, w, h;
    const uint8_t *src_info; const uint8_t *src_bits;
    int32_t sx, sy, sw, sh;
    const AoeDibSpanRor *spans;   /* NULL for none */
    int clip, flip_x, flip_y;
    int32_t transparent;
} AoeDibBlitRor;
int aoe_dib_blit_scaled_ror(AoeDibBlitRor *bl);

/* 0x0042f190 (dib, palette, remap_transparent, from_last): a DIB's
   colours matched to a palette. The palette's size and the DIB's colour
   count (2^bits below 9 when unset), image size and bits offset (12 for
   compression 3) are taken. With `remap_transparent` the transparent
   colour index is the first pixel of the last row (or 0 with
   `from_last`), and a replacement index is sought: the first colour's
   RGB moved by +1, then -1 per step until the palette index found is
   not 0. Each colour maps to its nearest palette index (0x0042f...),
   except the transparent one (0) and ones that land on 0 (the
   replacement). The DIB's colour count becomes the palette's; a larger
   palette grows the block (GlobalReAlloc) and moves the bits up. The
   pixels are remapped (0x0042f580 8-bit, 0x0042f5b0 4-bit, 0x0042f5f0
   RLE8, 0x0042f660 RLE4), then the DIB's colours are rewritten from the
   palette entries. */
typedef struct {
    void *context;
    int32_t palette_size;
    uint8_t (*nearest)(void *context, uint8_t r, uint8_t g, uint8_t b);     /* GetNearestPaletteIndex */
    void (*entry)(void *context, int32_t i, uint8_t *r, uint8_t *g, uint8_t *b); /* GetPaletteEntries */
    /* The block regrown to `size` bytes (GlobalReAlloc); the new pointer. */
    uint8_t *(*regrow)(void *context, uint8_t *dib, size_t size);
} AoeDibPaletteOpsRor;
uint8_t *aoe_dib_match_palette_ror(uint8_t *dib, const AoeDibPaletteOpsRor *o, int remap_transparent,
                                   int from_last);

#endif
