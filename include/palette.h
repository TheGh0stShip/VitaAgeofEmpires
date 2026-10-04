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

/* Patched RoR 1.0a water colour cycle. Palette 50500 leaves indices
   248..254 black; the game fills them with a 7-entry ramp (screen
   constructor 0x0047e8a3 stores it at +0x69e; 0x004fee50 also sets it) and
   rotates it every 200 ms (+0x690 = 0xc8): 0x004843d0 writes entry i =
   ramp[(start + i) % 7] then steps start down (0 wraps to 6, 0x00484462). */
enum {
    AOE_WATER_CYCLE_FIRST = 248,
    AOE_WATER_CYCLE_COUNT = 7,
    AOE_WATER_CYCLE_INTERVAL_MS = 200
};
void palette_water_cycle_apply(Palette *palette, int start);
int palette_water_cycle_next(int start);

#endif
