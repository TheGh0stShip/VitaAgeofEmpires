#include "palette.h"

#include <string.h>

static int next_uint(const char **cursor, const char *end, unsigned *value) {
    unsigned n = 0;
    while (*cursor < end && (**cursor == ' ' || **cursor == '\r' || **cursor == '\n'))
        ++*cursor;
    if (*cursor == end || **cursor < '0' || **cursor > '9') return 0;
    do {
        n = n * 10 + (unsigned)(**cursor - '0');
        if (n > 255) return 0;
        ++*cursor;
    } while (*cursor < end && **cursor >= '0' && **cursor <= '9');
    *value = n;
    return 1;
}

int palette_parse_jasc(const uint8_t *data, size_t size, Palette *palette) {
    const char *cursor, *end;
    if (!data || !palette || size < 21 ||
        memcmp(data, "JASC-PAL\r\n0100\r\n256\r\n", 21) != 0) return 0;
    cursor = (const char *)data + 21;
    end = (const char *)data + size;
    for (int i = 0; i < 256; ++i) {
        unsigned r, g, b;
        if (!next_uint(&cursor, end, &r) ||
            !next_uint(&cursor, end, &g) ||
            !next_uint(&cursor, end, &b)) return 0;
        palette->colors[i] = (PaletteColor){(uint8_t)r, (uint8_t)g, (uint8_t)b};
    }
    return 1;
}

int palette_expand_indexed_rgba(const Palette *palette,
                                const uint8_t *indices, size_t indices_size,
                                uint32_t width, uint32_t height,
                                size_t index_stride,
                                uint8_t *rgba, size_t rgba_size,
                                size_t rgba_stride) {
    if (!palette || !indices || !rgba || !width || !height ||
        (size_t)width > index_stride ||
        (size_t)width > rgba_stride / 4 ||
        (size_t)width > indices_size ||
        (size_t)(height - 1) >
            (indices_size - (size_t)width) / index_stride ||
        (size_t)width * 4 > rgba_size ||
        (size_t)(height - 1) >
            (rgba_size - (size_t)width * 4) / rgba_stride)
        return 0;
    for (uint32_t y = 0; y < height; ++y) {
        const uint8_t *src = indices + (size_t)y * index_stride;
        uint8_t *dst = rgba + (size_t)y * rgba_stride;
        for (uint32_t x = 0; x < width; ++x) {
            PaletteColor color = palette->colors[src[x]];
            dst[(size_t)x * 4 + 0] = color.r;
            dst[(size_t)x * 4 + 1] = color.g;
            dst[(size_t)x * 4 + 2] = color.b;
            dst[(size_t)x * 4 + 3] = 255;
        }
    }
    return 1;
}

static const PaletteColor water_ramp[AOE_WATER_CYCLE_COUNT] = {
    {0x17, 0x27, 0x7c}, {0x27, 0x3f, 0x90}, {0x3f, 0x5f, 0x9f},
    {0x57, 0x7b, 0xb4}, {0x3f, 0x5f, 0xa0}, {0x27, 0x3f, 0x91},
    {0x17, 0x27, 0x7b}};

void palette_water_cycle_apply(Palette *palette, int start) {
    if (!palette || start < 0 || start >= AOE_WATER_CYCLE_COUNT) return;
    for (int i = 0; i < AOE_WATER_CYCLE_COUNT; ++i)
        palette->colors[AOE_WATER_CYCLE_FIRST + i] =
            water_ramp[(start + i) % AOE_WATER_CYCLE_COUNT];
}

int palette_water_cycle_next(int start) {
    return start <= 0 ? AOE_WATER_CYCLE_COUNT - 1 : start - 1;
}

/* 0x00557108: the static colours, (index, 0x00bbggrr) pairs ending at -1. */
static const uint32_t static_colors[20][2] = {
    {0, 0}, {1, 0x80}, {2, 0x8000}, {3, 0x8080}, {4, 0x800000}, {5, 0x800080}, {6, 0x808000},
    {7, 0xc0c0c0}, {8, 0xc0dcc0}, {9, 0xf0caa6}, {0xf6, 0xf0fbff}, {0xf7, 0xa4a0a0},
    {0xf8, 0x808080}, {0xf9, 0xff}, {0xfa, 0xff00}, {0xfb, 0xffff}, {0xfc, 0xff0000},
    {0xfd, 0xff00ff}, {0xfe, 0xffff00}, {0xff, 0xffffff}};

void palette_install_system_ror(const uint8_t system[256][4], const uint8_t game[256][4],
                                int got_system, int got_game, uint8_t out[256][4]) {
    int i;
    memcpy(out, system, 256 * 4);
    for (i = 10; i < 246; ++i) { /* 0x3b0 / 4 = 236 entries from 10 */
        out[i][0] = game[i][0];
        out[i][1] = game[i][1];
        out[i][2] = game[i][2];
        out[i][3] = 1;
    }
    if (!got_system || !got_game) {
        for (i = 0; i < 20; ++i) {
            uint32_t idx = static_colors[i][0], c = static_colors[i][1];
            out[idx][0] = (uint8_t)(c & 0xff);
            out[idx][1] = (uint8_t)((c >> 8) & 0xff);
            out[idx][2] = (uint8_t)((c >> 16) & 0xff);
            out[idx][3] = 0;
        }
    }
}
