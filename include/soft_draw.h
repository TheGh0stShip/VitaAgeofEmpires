#ifndef VITA_AOE_SOFT_DRAW_H
#define VITA_AOE_SOFT_DRAW_H

#include <stdint.h>

#include "draw_system.h"

/* A portable stand-in for DirectDraw beneath the translated draw system
   (draw_system.h): 8-bit heap surfaces, palette objects, rectangle fills,
   (colour-keyed) copies and a primary surface that the platform layer
   presents (on Vita through the GPU, on the host to an image). It is
   integration support, not an original function. There is no GDI: device
   contexts are refused, which the draw system treats as a failed lock of
   the DC path. */

typedef struct {
    int32_t width, height, pitch;
    uint8_t *bits;
    int32_t keyed;
    uint8_t key;
    uint8_t *palette;          /* the attached palette object's entries, or NULL */
    int primary;
} AoeSoftSurfaceRor;

typedef struct {
    int32_t width, height, bits;   /* the display mode */
    AoeSoftSurfaceRor *primary;
    uint8_t palette[1024];         /* the primary's palette (RGBX) */
    uint32_t palette_changes;      /* bumped whenever an attached palette changes */
} AoeSoftDisplayRor;

/* The ops (context: the display) and the display a platform presents.
   `width` x `height` is the only mode offered (8 bits). */
void aoe_soft_draw_init_ror(AoeSoftDisplayRor *display, AoeDrawOpsRor *ops, int32_t width,
                            int32_t height);
/* The primary's pixels and pitch (NULL before the draw system made it). */
const uint8_t *aoe_soft_draw_primary_ror(const AoeSoftDisplayRor *display, int32_t *pitch);
/* The primary as 32-bit A8B8G8R8 (R in the low byte) into `out` with a
   row stride of `stride` pixels; 0 without a primary. */
int aoe_soft_draw_present_abgr_ror(const AoeSoftDisplayRor *display, uint32_t *out,
                                   int32_t stride);

#endif
