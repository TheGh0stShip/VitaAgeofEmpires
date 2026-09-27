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
