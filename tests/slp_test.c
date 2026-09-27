#include "slp.h"

#include <assert.h>
#include <string.h>

int main(void) {
    SlpPixel pixels[4] = {
        {1, SLP_COLOR}, {0, SLP_TRANSPARENT},
        {2, SLP_COLOR}, {3, SLP_COLOR}
    };
    SlpFrame frame = {0};
    Palette palette = {0};
    uint8_t rgba[3 * 3 * 4];
    frame.width = 2;
    frame.height = 2;
    frame.pixels = pixels;
    palette.colors[1] = (PaletteColor){10, 20, 30};
    palette.colors[2] = (PaletteColor){40, 50, 60};
    palette.colors[3] = (PaletteColor){70, 80, 90};
    memset(rgba, 7, sizeof(rgba));
    assert(slp_blit_terrain_rgba(&frame, &palette, rgba, sizeof(rgba),
                                 3, 3, 12, 1, 1));
    assert(rgba[16] == 10 && rgba[17] == 20 && rgba[18] == 30 &&
           rgba[19] == 255);
    assert(rgba[20] == 7 && rgba[21] == 7 && rgba[22] == 7 && rgba[23] == 7);
    assert(rgba[28] == 40 && rgba[29] == 50 && rgba[30] == 60 &&
           rgba[31] == 255);
    assert(rgba[32] == 70 && rgba[33] == 80 && rgba[34] == 90 &&
           rgba[35] == 255);
    memset(rgba, 7, sizeof(rgba));
    assert(slp_blit_terrain_rgba(&frame, &palette, rgba, sizeof(rgba),
                                 3, 3, 12, -1, -1));
    assert(rgba[0] == 70 && rgba[1] == 80 && rgba[2] == 90 &&
           rgba[3] == 255);
    pixels[3].kind = SLP_SHADOW;
    memset(rgba, 7, sizeof(rgba));
    assert(!slp_blit_terrain_rgba(&frame, &palette, rgba, sizeof(rgba),
                                  3, 3, 12, 1, 1));
    for (size_t i = 0; i < sizeof(rgba); ++i) assert(rgba[i] == 7);
    pixels[3].kind = SLP_COLOR;
    assert(!slp_blit_terrain_rgba(&frame, &palette, rgba, 8,
                                  3, 3, 12, 1, 1));
    assert(!slp_blit_terrain_rgba(NULL, &palette, rgba, sizeof(rgba),
                                  3, 3, 12, 1, 1));
    return 0;
}
