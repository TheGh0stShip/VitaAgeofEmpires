#include "dib_util.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void w32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }

int main(void) {
    uint8_t info[40] = {0};
    uint32_t pitch;
    int32_t h, dir, x, y, w, hh, sx, sy, sw, sh;
    AoeDibSpanRor *spans = NULL;
    uint8_t bits[2 * 8];
    uint8_t table[256];
    int i;

    w32(info, 40);
    w32(info + 4, 5);
    w32(info + 8, (uint32_t)-2);
    aoe_dib_layout_ror(info, &pitch, &h, &dir);
    assert(pitch == 8 && h == 2 && dir == 1);
    w32(info + 8, 2);
    aoe_dib_layout_ror(info, &pitch, &h, &dir);
    assert(dir == -1);

    /* Clip: dest 100 x 50; a 20 x 10 blit at (-5, 45) 1:1. */
    w32(info + 4, 100);
    w32(info + 8, 50);
    x = -5; y = 45; w = 20; hh = 10; sx = 0; sy = 0; sw = 20; sh = 10;
    assert(aoe_dib_clip_ror(info, NULL, &x, &y, &w, &hh, &sx, &sy, &sw, &sh, 0, 0));
    assert(x == 0 && w == 15 && sx == 5 && sw == 15 && y == 45 && hh == 5 && sh == 5 && sy == 0);
    x = 200; w = 5; hh = 5;
    assert(!aoe_dib_clip_ror(info, NULL, &x, &y, &w, &hh, &sx, &sy, &sw, &sh, 0, 0));

    /* Spans of a 5 x 2 bottom-up image: row 0 (stored last) "01020", row 1 "00000". */
    w32(info + 4, 5);
    w32(info + 8, 2);
    memset(bits, 0, sizeof bits);
    bits[8 + 1] = 1;
    bits[8 + 3] = 2;
    assert(aoe_dib_spans_ror(info, &spans, bits) == 1 && spans);
    assert(spans[0].empty == 0 && spans[0].first == 1 && spans[0].last == 3 && spans[0].solid == 0);
    assert(spans[1].empty == 1 && spans[1].first == 5 && spans[1].last == 5);
    free(spans);
    memset(bits, 7, sizeof bits);
    assert(aoe_dib_spans_ror(info, &spans, bits) == 0 && spans == NULL);

    /* Remaps. */
    for (i = 0; i < 256; ++i) table[i] = (uint8_t)(255 - i);
    {
        uint8_t px[2] = {0, 10}, nib[1] = {0x12}, rle[] = {3, 5, 0, 3, 1, 2, 3, 0, 0, 0, 0, 1};
        aoe_dib_remap8_ror(px, 2, table);
        assert(px[0] == 255 && px[1] == 245);
        for (i = 0; i < 16; ++i) table[i] = (uint8_t)(15 - i);
        aoe_dib_remap4_ror(nib, 1, table);
        assert(nib[0] == 0xed);
        for (i = 0; i < 256; ++i) table[i] = (uint8_t)(i + 1);
        aoe_dib_remap_rle8_ror(rle, table);
        assert(rle[1] == 6 && rle[4] == 2 && rle[5] == 3 && rle[6] == 4 && rle[7] == 0);
    }
    {
        uint8_t m[6] = {1, 2, 3, 4, 5, 6};
        aoe_dib_copy_back_ror(m + 2, m, 4);
        assert(memcmp(m, "\1\2\1\2\3\4", 6) == 0);
    }
    /* A BMP file: file header, 40-byte header, 2 colours, 8 x 1 bits. */
    {
        uint8_t f[14 + 40 + 8 + 8];
        size_t n = 0;
        uint8_t *dib;
        memset(f, 0, sizeof f);
        f[0] = 'B'; f[1] = 'M';
        w32(f + 10, 14 + 40 + 8);
        w32(f + 14, 40);
        w32(f + 18, 8);
        w32(f + 22, 1);
        f[26] = 1;
        f[28] = 8;
        w32(f + 46, 2);
        f[54] = 9; f[58] = 10;
        f[62] = 0x42;
        dib = aoe_dib_read_ror(f, sizeof f, &n);
        assert(dib && n == 40 + 8 + 8 && dib[40] == 9 && dib[44] == 10 && dib[48] == 0x42);
        assert(dib[20] == 8); /* image size filled in */
        aoe_dib_free_ror(dib);
        /* An OS/2 core header with 2 triples. */
        memset(f, 0, sizeof f);
        w32(f, 12);
        f[4] = 4; f[6] = 1; f[8] = 1; f[10] = 1;
        f[12] = 1; f[13] = 2; f[14] = 3; f[15] = 4; f[16] = 5; f[17] = 6;
        f[18] = 0xaa;
        dib = aoe_dib_read_ror(f, sizeof f, &n);
        assert(dib && dib[0] == 40 && dib[4] == 4 && dib[8] == 1 && dib[14] == 1);
        assert(dib[40] == 1 && dib[43] == 0 && dib[44] == 4 && dib[48] == 0xaa);
        aoe_dib_free_ror(dib);
    }
    aoe_dib_nothing_ror();
    puts("dib_util_test: ok");
    return 0;
}
