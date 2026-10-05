#include "palette.h"

#include <assert.h>
#include <string.h>

/* RoR 1.0a water cycle: ramp at 248..254, rotated by a descending start. */
int main(void) {
    Palette p;
    memset(&p, 0, sizeof(p));
    palette_water_cycle_apply(&p, 0);
    assert(p.colors[248].r == 0x17 && p.colors[248].g == 0x27 &&
           p.colors[248].b == 0x7c);
    assert(p.colors[251].r == 0x57 && p.colors[251].g == 0x7b &&
           p.colors[251].b == 0xb4);
    assert(p.colors[254].r == 0x17 && p.colors[254].b == 0x7b);
    assert(p.colors[247].r == 0 && p.colors[255].r == 0);
    /* start 0 -> 6: entry 0 takes ramp[6], entry 1 ramp[0]. */
    assert(palette_water_cycle_next(0) == 6);
    assert(palette_water_cycle_next(6) == 5);
    palette_water_cycle_apply(&p, 6);
    assert(p.colors[248].b == 0x7b && p.colors[249].b == 0x7c &&
           p.colors[250].b == 0x90);
    palette_water_cycle_apply(&p, 7); /* out of range: unchanged */
    assert(p.colors[248].b == 0x7b);
    return 0;
}
