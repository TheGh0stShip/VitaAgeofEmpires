#include "draw_prims.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #cond);     \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

static uint8_t px[8 * 8];

static AoeShapeSurfaceRor surface(int32_t direction) {
    AoeShapeSurfaceRor s;
    memset(&s, 0, sizeof s);
    memset(px, 0, sizeof px);
    s.bits = px;
    s.width = s.height = s.pitch = 8;
    s.direction = direction;
    s.clip_right = s.clip_bottom = 7;
    return s;
}

/* The pixel at (x, y) whichever way the rows run. */
static uint8_t at(const AoeShapeSurfaceRor *s, int x, int y) {
    return px[(s->direction == 1 ? y : 7 - y) * 8 + x];
}

static void test_lines(void) {
    AoeShapeSurfaceRor s = surface(1);
    int x, y, n = 0;
    s.clip_left = 2;
    s.clip_right = 5;
    aoe_draw_hline_ror(&s, 0, 1, 8, 9);
    for (x = 0; x < 8; ++x) CHECK(at(&s, x, 1) == (x >= 2 && x <= 5 ? 9 : 0));
    aoe_draw_hline_ror(&s, 0, 2, 2, 9); /* ends left of the clip */
    for (x = 0; x < 8; ++x) n += at(&s, x, 2);
    CHECK(n == 0);

    s = surface(-1);
    s.clip_top = 1;
    s.clip_bottom = 4;
    aoe_draw_vline_ror(&s, 3, 0, 8, 7);
    for (y = 0; y < 8; ++y) CHECK(at(&s, 3, y) == (y >= 1 && y <= 4 ? 7 : 0));
    CHECK(px[7 * 8 + 3] == 0 && px[6 * 8 + 3] == 7); /* bottom-up rows */

    s = surface(1);
    aoe_draw_fill_ror(&s, 6, 5, 1, 2, 4);
    for (y = 0; y < 8; ++y)
        for (x = 0; x < 8; ++x)
            CHECK(at(&s, x, y) == (x >= 1 && x <= 6 && y >= 2 && y <= 5 ? 4 : 0));
    s.bits = NULL;
    aoe_draw_fill_ror(&s, 0, 0, 7, 7, 1); /* nothing without bits */

    s = surface(1);
    aoe_draw_rect_ror(&s, 1, 1, 4, 3, 5);
    CHECK(at(&s, 1, 1) == 5 && at(&s, 4, 3) == 5 && at(&s, 2, 2) == 0);
    CHECK(at(&s, 4, 2) == 5 && at(&s, 1, 2) == 5 && at(&s, 5, 1) == 0);
}

static void test_bevels(void) {
    static const uint8_t two[4] = {1, 2, 3, 4};
    static const uint8_t three[6] = {1, 2, 3, 4, 5, 6};
    AoeShapeSurfaceRor s = surface(1);

    aoe_draw_bevel1_ror(&s, 0, 0, 3, 3, 1, 2);
    /* The bottom/left colour owns the top-left and bottom-right corners. */
    CHECK(at(&s, 0, 0) == 2 && at(&s, 3, 3) == 2 && at(&s, 3, 0) == 1);
    CHECK(at(&s, 1, 0) == 1 && at(&s, 3, 2) == 1 && at(&s, 0, 2) == 2);
    CHECK(at(&s, 1, 1) == 0);

    s = surface(1);
    aoe_draw_bevel2_ror(&s, 0, 0, 5, 5, two);
    CHECK(at(&s, 5, 0) == 1 && at(&s, 0, 5) == 4 && at(&s, 0, 0) == 4);
    CHECK(at(&s, 4, 1) == 2 && at(&s, 2, 1) == 2 && at(&s, 1, 1) == 3);
    CHECK(at(&s, 1, 4) == 3 && at(&s, 4, 4) == 3 && at(&s, 2, 2) == 0);

    s = surface(1);
    aoe_draw_bevel3_ror(&s, 0, 0, 7, 7, three);
    CHECK(at(&s, 7, 0) == 1 && at(&s, 6, 1) == 2 && at(&s, 5, 2) == 3);
    CHECK(at(&s, 0, 7) == 6 && at(&s, 1, 6) == 5 && at(&s, 2, 5) == 4);
    CHECK(at(&s, 3, 3) == 0);

    s = surface(1);
    aoe_draw_bevel2_flat_ror(&s, 0, 0, 5, 5, two);
    CHECK(at(&s, 1, 1) == 2 && at(&s, 4, 4) == 2 && at(&s, 1, 2) == 0);
    CHECK(at(&s, 0, 0) == 4 && at(&s, 5, 0) == 1);

    s = surface(1);
    aoe_draw_bevel3_flat_ror(&s, 0, 0, 7, 7, three);
    CHECK(at(&s, 2, 2) == 3 && at(&s, 5, 2) == 3 && at(&s, 1, 6) == 5);
    CHECK(at(&s, 2, 5) == 0); /* no third bottom/left ring */
}

static void test_shade(void) {
    uint8_t table[256];
    AoeShapeSurfaceRor s = surface(-1);
    int i, x, y;
    for (i = 0; i < 256; ++i) table[i] = (uint8_t)(i + 10);
    aoe_draw_shade_ror(&s, 0, 0, 7, 7); /* no table: nothing */
    CHECK(px[0] == 0);
    aoe_draw_set_shade_ror(&s, table);
    s.clip_left = 1;
    aoe_draw_shade_ror(&s, -3, 6, 20, 30);
    for (y = 0; y < 8; ++y)
        for (x = 0; x < 8; ++x)
            CHECK(at(&s, x, y) == (x >= 1 && y >= 6 ? 10 : 0));
    aoe_draw_shade_ror(&s, 5, 7, 4, 7); /* empty */
    CHECK(at(&s, 4, 7) == 10);
}

int main(void) {
    test_lines();
    test_bevels();
    test_shade();
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    puts("draw_prims_test: ok");
    return 0;
}
