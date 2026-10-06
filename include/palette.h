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


/* 0x0042f6a0: the game palette installed over the system palette. The
   system's 256 entries are read (GetSystemPaletteEntries) and the game
   palette's (GetPaletteEntries); entries 10..245 of the result take the
   game colours with PC_RESERVED (flag 1). When either read failed, the
   Windows static colours (indexes 0..9 and 246..255: black, maroon,
   green, olive, navy, purple, teal, silver, 0xc0dcc0, 0xf0caa6, 0xf0fbff,
   0xa4a0a0, grey, red, lime, yellow, blue, fuchsia, aqua, white) are
   written with flag 0. The palette is resized to 256 and every entry
   set (ResizePalette, SetPaletteEntries). `system` and `game` are RGBA
   quads; the result goes to `out`. */
void palette_install_system_ror(const uint8_t system[256][4], const uint8_t game[256][4],
                                int got_system, int got_game, uint8_t out[256][4]);


/* 0x0042eb90: a JASC text palette from a file (the name, when the
   0x007c0464 flag allows files) or the "bina" resource `id`, parsed with
   strtok over " \n": the "JASC-PAL" and "0100" tokens, the count, then
   count RGB triples (flags 0). Only a fully read table counts; a table of
   256 entries takes the Windows static colours (palette_install_system_ror's
   table) and, with `install` and the game window's +0x478 / +0x479 flags,
   is installed over the system palette (0x0042f6a0: the caller's step).
   Here the text is given; returns 1 with `out` filled, 0 otherwise. */
int palette_parse_text_ror(const char *text, Palette *out, int *count_out, int *complete);
void palette_apply_static_colors_ror(Palette *p);

#endif
