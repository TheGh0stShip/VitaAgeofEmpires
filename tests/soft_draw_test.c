/* The translated draw system over the software DirectDraw stand-in. */
#include "soft_draw.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    static AoeSoftDisplayRor display;
    static AoeDrawOpsRor ops;
    static AoeDrawSystemRor d;
    static uint32_t frame[640 * 480];
    uint8_t pal[1024], *bits;
    int32_t rect[4] = {10, 20, 30, 40}, pitch;
    const uint8_t *prim;
    int i;

    aoe_soft_draw_init_ror(&display, &ops, 640, 480);
    memset(&d, 0, sizeof d);
    d.ops = &ops;
    /* Full screen, DirectDraw type: primary, back buffer and palette. */
    assert(aoe_draw_open_ror(&d, NULL, NULL, NULL, 2, 2, 640, 480, 0) == 1);
    assert(d.primary_surface && d.back && d.back->surface && display.primary);
    /* A palette: entry i = (i, 255 - i, i / 2). */
    for (i = 0; i < 256; ++i) {
        pal[4 * i] = (uint8_t)i;
        pal[4 * i + 1] = (uint8_t)(255 - i);
        pal[4 * i + 2] = (uint8_t)(i / 2);
        pal[4 * i + 3] = 0;
    }
    aoe_draw_set_palette_ror(&d, 0, 256, pal);
    assert(display.palette[4 * 200] == 200 && display.palette_changes >= 1);
    /* Fill the back buffer (the draw system's rectangles are inclusive),
       present, read the primary. */
    aoe_surface_fill_ror(d.back, NULL, 7);
    aoe_surface_fill_ror(d.back, rect, 200);
    aoe_draw_present_ror(&d, NULL);
    prim = aoe_soft_draw_primary_ror(&display, &pitch);
    assert(prim && pitch >= 640);
    assert(prim[0] == 7 && prim[20 * pitch + 10] == 200 && prim[40 * pitch + 30] == 200 &&
           prim[41 * pitch + 10] == 7 && prim[20 * pitch + 31] == 7 && prim[19 * pitch + 10] == 7);
    /* A locked back buffer writes straight through. */
    bits = aoe_surface_lock_ror(d.back, 1);
    assert(bits);
    bits[5] = 99;
    aoe_surface_unlock_ror(d.back);
    aoe_draw_present_ror(&d, NULL);
    assert(prim[5] == 99);
    /* 32-bit output: R in the low byte. */
    assert(aoe_soft_draw_present_abgr_ror(&display, frame, 640));
    assert(frame[20 * 640 + 10] == (0xff000000u | (100u << 16) | (55u << 8) | 200u));
    aoe_draw_shutdown_ror(&d);
    assert(display.primary == NULL);
    puts("soft draw ok");
    return 0;
}
