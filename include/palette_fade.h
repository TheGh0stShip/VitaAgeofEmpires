#ifndef VITA_AOE_PALETTE_FADE_H
#define VITA_AOE_PALETTE_FADE_H

#include <stdint.h>

#include "draw_system.h"

/* Patched Rise of Rome 1.0a palette fades and the screen snapshot.
   Palettes are 256 RGBX entries (1024 bytes) as the draw system keeps
   them. */

/* 0x0041cac0: each entry of `src` moved `percent` (0..100) of the way to
   `color` (0x00BBGGRR) scaled by the entry's lightness (its brightest
   component plus half the other two weighted by what the brightest lacks,
   over 512), into `dst`; entries `keep_a` and `keep_b` are copied. */
void aoe_palette_blend_ror(const uint8_t src[1024], uint8_t dst[1024],
                           uint32_t color, int32_t percent, int32_t keep_a,
                           int32_t keep_b);

/* 0x0041d090(unused, percent, color, palette), the computed colour
   table's constructor (vtable 0x0054320c): its ID (+0x22) and the bytes
   +0x24 / +0x25 are cleared and its map is made from `palette` (or the
   system palette when NULL) blended `percent` towards `color` (0 for
   NULL; 0x0041cac0 with no kept entries): each blended entry maps to the
   palette entry nearest by squared RGB distance below 20000 (the first
   on ties; 0 when none is that near). */
void aoe_palette_remap_ror(const uint8_t palette[1024], uint32_t color, int32_t percent,
                           uint8_t map[256]);

/* 0x0043a460: the surface's system palette. 0x0043a490: set a whole
   palette with entry 0 black and entry 255 white. */
void aoe_surface_palette_ror(const AoeDrawSurfaceRor *s, uint8_t out[1024]);
void aoe_palette_apply_ror(AoeDrawSystemRor *d, uint8_t entries[1024]);

/* 0x0041cc90: fade the palette (`base`, or the back buffer's when NULL)
   towards `color` over `seconds` (direction 1 towards it, otherwise back
   from it), easing with the square of the elapsed time; blocks until
   done, applying each step. `now_ms` is the millisecond clock. */
void aoe_palette_fade_ror(AoeDrawSystemRor *d, uint32_t color, float seconds,
                          int direction, const uint8_t *base, int32_t keep_a,
                          int32_t keep_b, uint32_t (*now_ms)(void *),
                          void *clock);

/* The credits' fade without blocking: 0x0047d1c0 starts it (the base
   palette copied or read from the back buffer; percent 0 towards the
   colour, else 100) and 0x0047d2d0 applies the next step, returning 1
   once it has finished. */
typedef struct {
    AoeDrawSystemRor *system;
    uint32_t color;
    float seconds, seconds_sq;
    uint8_t direction;
    int32_t keep_a, keep_b;
    uint32_t start_ms;
    int32_t percent;
    uint8_t base[1024], work[1024];
} AoePaletteFadeRor;

void aoe_palette_fade_start_ror(AoePaletteFadeRor *f, AoeDrawSystemRor *d,
                                uint32_t color, float seconds, int direction,
                                const uint8_t *base, int32_t keep_a,
                                int32_t keep_b, uint32_t now_ms);
int aoe_palette_fade_step_ror(AoePaletteFadeRor *f, uint32_t now_ms);

/* 0x0043a4d0: write the surface as an 8-bit bottom-up BMP to the first
   name of `format` ("C:\AOE_%03d.BMP" when NULL) from *counter on that
   does not exist yet (up to 1000 tries; the counter advances past
   existing names). */
typedef struct {
    void *context;
    int (*exists)(void *context, const char *path);
    int (*write)(void *context, const char *path, const uint8_t *data,
                 uint32_t size);
} AoeSnapshotOpsRor;

int aoe_surface_snapshot_ror(AoeDrawSurfaceRor *s, const char *format,
                             int32_t *counter, const AoeSnapshotOpsRor *ops);

#endif
