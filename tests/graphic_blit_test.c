#include "graphic_effects.h"

#include <assert.h>
#include <string.h>

typedef struct { unsigned calls; } Trace;

static void apply_special(void *context, const SlpPixel *pixel,
                          uint8_t rgba[4]) {
    Trace *trace = context;
    ++trace->calls;
    assert(pixel->kind == SLP_PLAYER);
    rgba[0] = pixel->index;
    rgba[1] = 42;
    rgba[2] = 43;
    rgba[3] = 255;
}

static const uint8_t *at(const uint8_t *rgba, unsigned x, unsigned y) {
    return rgba + (y * 6 + x) * 4;
}

int main(void) {
    SlpPixel pixels[6] = {
        {1, SLP_COLOR}, {0, SLP_TRANSPARENT}, {2, SLP_COLOR},
        {3, SLP_PLAYER}, {2, SLP_COLOR}, {1, SLP_COLOR},
    };
    SlpFrame frame = {.width = 3, .height = 2,
                      .hotspot_x = 1, .hotspot_y = 1,
                      .pixels = pixels};
    Palette palette = {0};
    palette.colors[1] = (PaletteColor){10, 11, 12};
    palette.colors[2] = (PaletteColor){20, 21, 22};
    uint8_t rgba[6 * 4 * 4];
    Trace trace = {0};

    memset(rgba, 7, sizeof(rgba));
    assert(!slp_blit_graphic_rgba(&frame, &palette, rgba, sizeof(rgba),
                                  6, 4, 24, 3, 2, 0, NULL, NULL));
    for (size_t i = 0; i < sizeof(rgba); ++i) assert(rgba[i] == 7);

    assert(slp_blit_graphic_rgba(&frame, &palette, rgba, sizeof(rgba),
                                 6, 4, 24, 3, 2, 0,
                                 apply_special, &trace));
    assert(at(rgba, 2, 1)[0] == 10 && at(rgba, 2, 1)[3] == 255);
    assert(at(rgba, 3, 1)[0] == 7); /* Transparent. */
    assert(at(rgba, 4, 1)[0] == 20);
    assert(at(rgba, 2, 2)[0] == 3 && at(rgba, 2, 2)[1] == 42);
    assert(trace.calls == 1);

    memset(rgba, 7, sizeof(rgba));
    assert(slp_blit_graphic_rgba(&frame, &palette, rgba, sizeof(rgba),
                                 6, 4, 24, 3, 2, 1,
                                 apply_special, &trace));
    assert(at(rgba, 1, 1)[0] == 20); /* Original source x=2. */
    assert(at(rgba, 2, 1)[0] == 7);
    assert(at(rgba, 3, 1)[0] == 10); /* Original source x=0. */
    assert(at(rgba, 3, 2)[0] == 3);  /* Mirrored player pixel. */
    assert(trace.calls == 2);

    memset(rgba, 7, sizeof(rgba));
    assert(slp_blit_graphic_rgba(&frame, &palette, rgba, sizeof(rgba),
                                 6, 4, 24, -3, -2, 0,
                                 apply_special, &trace));
    for (size_t i = 0; i < sizeof(rgba); ++i) assert(rgba[i] == 7);

    pixels[0].kind = 99;
    assert(!slp_blit_graphic_rgba(&frame, &palette, rgba, sizeof(rgba),
                                  6, 4, 24, 3, 2, 0,
                                  apply_special, &trace));
    for (size_t i = 0; i < sizeof(rgba); ++i) assert(rgba[i] == 7);

    pixels[0].kind = SLP_COLOR;
    assert(aoe_graphic_player_only(&frame));
    assert(aoe_graphic_player_palette_index(3, 0) == 0x13);
    assert(aoe_graphic_player_palette_index(3, 7) == 0x83);
    assert(aoe_graphic_player_palette_index(0xf3, 7) == 0xf3);
    palette.colors[0x83] = (PaletteColor){81, 82, 83};
    AoeGraphicPlayerEffects effects = {&palette, 7};
    memset(rgba, 7, sizeof(rgba));
    assert(slp_blit_graphic_rgba(&frame, &palette, rgba, sizeof(rgba),
                                 6, 4, 24, 3, 2, 0,
                                 aoe_graphic_apply_player, &effects));
    assert(at(rgba, 2, 2)[0] == 81 && at(rgba, 2, 2)[1] == 82 &&
           at(rgba, 2, 2)[2] == 83 && at(rgba, 2, 2)[3] == 255);
    pixels[0].kind = SLP_SHADOW;
    assert(!aoe_graphic_player_only(&frame));

    pixels[0].kind = SLP_COLOR;
    pixels[2].kind = SLP_SHADOW;
    uint8_t indices[6 * 4], shadow[256];
    for (unsigned i = 0; i < 256; ++i)
        shadow[i] = (uint8_t)(i + 1);
    memset(indices, 5, sizeof(indices));
    assert(!slp_blit_graphic_indexed(&frame, indices, sizeof(indices),
                                     6, 4, 6, 3, 2, 0, 7, NULL));
    for (size_t i = 0; i < sizeof(indices); ++i)
        assert(indices[i] == 5);
    assert(slp_blit_graphic_indexed(&frame, indices, sizeof(indices),
                                    6, 4, 6, 3, 2, 0, 7, shadow));
    assert(indices[1 * 6 + 2] == 1);
    assert(indices[1 * 6 + 3] == 5);
    assert(indices[1 * 6 + 4] == 6); /* Shadow transforms destination 5. */
    assert(indices[2 * 6 + 2] == 0x83);
    memset(indices, 5, sizeof(indices));
    assert(slp_blit_graphic_indexed(&frame, indices, sizeof(indices),
                                    6, 4, 6, 3, 2, 1, 7, shadow));
    assert(indices[1 * 6 + 1] == 6); /* Mirrored shadow. */
    assert(indices[2 * 6 + 3] == 0x83);
    palette.colors[6] = (PaletteColor){60, 61, 62};
    memset(rgba, 7, sizeof(rgba));
    assert(!palette_expand_indexed_rgba(&palette, indices,
              sizeof(indices), 6, 4, 6, rgba, sizeof(rgba) - 1, 24));
    for (size_t i = 0; i < sizeof(rgba); ++i)
        assert(rgba[i] == 7);
    assert(palette_expand_indexed_rgba(&palette, indices,
              sizeof(indices), 6, 4, 6, rgba, sizeof(rgba), 24));
    assert(at(rgba, 1, 1)[0] == 60 && at(rgba, 1, 1)[1] == 61 &&
           at(rgba, 1, 1)[2] == 62 && at(rgba, 1, 1)[3] == 255);
    assert(at(rgba, 3, 2)[0] == 81 && at(rgba, 3, 2)[3] == 255);
    pixels[2].kind = SLP_OUTLINE1;
    memset(indices, 5, sizeof(indices));
    assert(!slp_blit_graphic_indexed(&frame, indices, sizeof(indices),
                                     6, 4, 6, 3, 2, 0, 7, shadow));
    for (size_t i = 0; i < sizeof(indices); ++i)
        assert(indices[i] == 5);
    return 0;
}
