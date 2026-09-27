#ifndef VITA_AOE_PALETTE_H
#define VITA_AOE_PALETTE_H

#include <stddef.h>
#include <stdint.h>

typedef struct { uint8_t r, g, b; } PaletteColor;
typedef struct { PaletteColor colors[256]; } Palette;

int palette_parse_jasc(const uint8_t *data, size_t size, Palette *palette);

/* Port boundary: expand an opaque indexed frame into RGBA8888 for display.
   Source and destination buffers must not overlap. Validates both complete
   surfaces before writing; destination alpha is always 255. */
int palette_expand_indexed_rgba(const Palette *palette,
                                const uint8_t *indices, size_t indices_size,
                                uint32_t width, uint32_t height,
                                size_t index_stride,
                                uint8_t *rgba, size_t rgba_size,
                                size_t rgba_stride);

#endif
