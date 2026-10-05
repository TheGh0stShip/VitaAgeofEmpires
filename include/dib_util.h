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

#endif
