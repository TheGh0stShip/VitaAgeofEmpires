#include "palette_fade.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #cond);     \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

static uint8_t applied[1024];
static int sets;
static uint32_t clock_ms;

static int32_t palette_set(void *c, void *p, int32_t s, int32_t n,
                           const uint8_t *in) {
    (void)c;
    (void)p;
    memcpy(applied + s * 4, in, (size_t)n * 4);
    ++sets;
    return 0;
}
static uint32_t tick(void *c) {
    (void)c;
    clock_ms += 100;
    return clock_ms;
}

static char written[64];
static uint8_t *bmp;
static uint32_t bmp_size;
static int exists(void *c, const char *path) {
    (void)c;
    return strcmp(path, "C:\\AOE_000.BMP") == 0;
}
static int write_file(void *c, const char *path, const uint8_t *d,
                      uint32_t n) {
    (void)c;
    snprintf(written, sizeof written, "%s", path);
    bmp = malloc(n);
    memcpy(bmp, d, n);
    bmp_size = n;
    return 1;
}

int main(void) {
    static const AoeDrawOpsRor ops = {.palette_set = palette_set};
    static const AoeSnapshotOpsRor snap = {NULL, exists, write_file};
    uint8_t src[1024], dst[1024];
    AoeDrawSystemRor d;
    AoeDrawSurfaceRor s;
    AoePaletteFadeRor f;
    uint8_t pixels[3 * 2] = {1, 2, 3, 4, 5, 6};
    uint8_t *rows[2] = {pixels, pixels + 3};
    int32_t counter = 0, i;

    memset(src, 0, sizeof src);
    src[4] = 200;  /* entry 1: r 200 */
    src[5] = 100;
    src[6] = 50;
    src[7] = 9;
    src[8] = 77;   /* entry 2 kept */
    aoe_palette_blend_ror(src, dst, 0x0000ff, 50, 2, -1);
    /* light = (100 + 50) * (255 - 200) / 512 + 200 = 216; red target 216 */
    CHECK(dst[4] == 200 + (216 - 200) * 50 / 100);
    CHECK(dst[5] == (uint8_t)(100 + (0 - 100) * 50 / 100) && dst[6] == 25);
    CHECK(dst[7] == 9 && dst[8] == 77);
    aoe_palette_blend_ror(src, dst, 0x0000ff, 0, -1, -1);
    CHECK(memcmp(src, dst, 1024) == 0);

    aoe_draw_clear_ror(&d, &ops);
    d.type = 2;
    d.palette_object = &d;
    memcpy(d.palette, src, sizeof src);
    aoe_palette_fade_ror(&d, 0, 0.5f, 1, NULL, -1, -1, tick, NULL);
    /* Faded fully to black; ends forced black and white. */
    CHECK(applied[4] == 0 && applied[0x3fc] == 0xff && sets >= 2);
    memcpy(d.palette, src, sizeof src);
    sets = 0;
    aoe_palette_fade_ror(&d, 0, 0.5f, 0, src, -1, -1, tick, NULL);
    CHECK(applied[4] == 200 && sets >= 2);

    aoe_palette_fade_start_ror(&f, &d, 0, 1.0f, 1, src, -1, -1, 1000);
    CHECK(f.percent == 0);
    CHECK(!aoe_palette_fade_step_ror(&f, 1500) && f.percent == 25);
    CHECK(aoe_palette_fade_step_ror(&f, 2100) && f.percent == 100);
    aoe_palette_fade_start_ror(&f, &d, 0, 1.0f, 0, src, -1, -1, 0);
    CHECK(f.percent == 100);
    CHECK(!aoe_palette_fade_step_ror(&f, 500) && f.percent == 25);

    memset(&s, 0, sizeof s);
    s.system = &d;
    d.type = 1;
    s.view.width = 3;
    s.view.height = 2;
    s.rows = rows;
    for (i = 0; i < 1024; ++i) d.palette[i] = (uint8_t)i;
    CHECK(aoe_surface_snapshot_ror(&s, NULL, &counter, &snap));
    CHECK(strcmp(written, "C:\\AOE_001.BMP") == 0 && counter == 1);
    CHECK(bmp_size == 0x436 + 2 * 4 && bmp[0] == 'B' && bmp[0x16] == 2);
    /* Palette swapped to BGR; rows bottom-up. */
    CHECK(bmp[0x36 + 4] == 6 && bmp[0x36 + 6] == 4);
    CHECK(bmp[0x436] == 4 && bmp[0x43a] == 1);
    free(bmp);

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    puts("palette_fade_test: ok");
    return 0;
}
