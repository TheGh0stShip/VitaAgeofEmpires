#include "palette.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* RoR 1.0a water cycle: ramp at 248..254, rotated by a descending start. */

static void test_install_system(void) {
    uint8_t sys[256][4], game[256][4], out[256][4];
    int i;
    for (i = 0; i < 256; ++i) {
        sys[i][0] = 1; sys[i][1] = 2; sys[i][2] = 3; sys[i][3] = 9;
        game[i][0] = (uint8_t)i; game[i][1] = 7; game[i][2] = 8; game[i][3] = 5;
    }
    palette_install_system_ror(sys, game, 1, 1, out);
    assert(out[9][0] == 1 && out[9][3] == 9 && out[10][0] == 10 && out[10][3] == 1 &&
           out[245][0] == 245 && out[245][3] == 1 && out[246][0] == 1 && out[246][3] == 9);
    palette_install_system_ror(sys, game, 0, 1, out);
    assert(out[0][0] == 0 && out[0][3] == 0 && out[1][0] == 0x80 && out[1][2] == 0 && out[7][0] == 0xc0 &&
           out[9][0] == 0xa6 && out[9][1] == 0xca && out[9][2] == 0xf0 && out[0xf9][0] == 0xff &&
           out[0xf9][1] == 0 && out[0xff][2] == 0xff && out[10][0] == 10 && out[0xf5][3] == 1);
}

static void test_parse_text(void) {
    Palette p; int n = -1, complete = -1;
    memset(&p, 0, sizeof p);
    assert(palette_parse_text_ror("JASC-PAL\n0100\n3\n1 2 3\n4 5 6\n7 8 9\n", &p, &n, &complete) == 1 && n == 3 && complete == 1 &&
           p.colors[0].r == 1 && p.colors[2].b == 9);
    assert(palette_parse_text_ror("JASC-PAL\n0100\n3\n1 2 3\n4 5", &p, &n, &complete) == 0 && complete == 0);
    assert(palette_parse_text_ror("JASC-PAL 0100", &p, &n, &complete) == 0 && palette_parse_text_ror(NULL, &p, &n, &complete) == 0);
    {
        static char big[8192]; size_t len = (size_t)snprintf(big, sizeof big, "JASC-PAL\n0100\n256\n"); int i;
        for (i = 0; i < 256; ++i) len += (size_t)snprintf(big + len, sizeof big - len, "%d %d %d\n", i, i, i);
        assert(palette_parse_text_ror(big, &p, &n, &complete) == 1 && n == 256 && p.colors[1].r == 0x80 && p.colors[100].r == 100 && p.colors[255].b == 0xff);
    }
}
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
    test_install_system();
    test_parse_text();
    return 0;
}
