#include "graphic_effects.h"

uint8_t aoe_graphic_player_palette_index(uint8_t source_index,
                                          uint8_t player_color) {
    uint8_t bank = (uint8_t)(((unsigned)player_color + 1u) * 16u);
    return (uint8_t)(source_index | bank);
}

int aoe_graphic_player_only(const SlpFrame *frame) {
    if (!frame || !frame->pixels || !frame->width || !frame->height ||
        frame->width > SIZE_MAX / frame->height)
        return 0;
    size_t count = (size_t)frame->width * frame->height;
    for (size_t i = 0; i < count; ++i)
        if (frame->pixels[i].kind > SLP_PLAYER)
            return 0;
    return 1;
}

void aoe_graphic_apply_player(void *context, const SlpPixel *pixel,
                              uint8_t rgba[4]) {
    AoeGraphicPlayerEffects *effects = context;
    if (!effects || !effects->palette || !pixel || !rgba ||
        pixel->kind != SLP_PLAYER)
        return;
    uint8_t index = aoe_graphic_player_palette_index(pixel->index,
                                                       effects->player_color);
    PaletteColor color = effects->palette->colors[index];
    rgba[0] = color.r;
    rgba[1] = color.g;
    rgba[2] = color.b;
    rgba[3] = 255;
}
