/* Patched RoR 1.0a shape object 0x0049f5f0..0x004a0370 on synthetic SLP
   and SHP data. */
#include "shape.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void put32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

/* SLP: one 6 x 2 frame, hotspot (2, 1). Row 0: left 1, right 1; runs: 2
   colours (0x08 = 2 << 2 | 0), skip 1 (0x05), 1 colour, end (0x0f). Row 1:
   transparent (left 0x8000). */
static uint8_t slp_data[0x80];
static uint32_t slp_size;
static void build_slp(void) {
    uint8_t *f = slp_data + 0x20;
    memset(slp_data, 0, sizeof(slp_data));
    memcpy(slp_data, "2.0N", 4);
    put32(slp_data + 4, 1);
    put32(f + 0, 0x40);     /* command table */
    put32(f + 4, 0x48);     /* outline */
    put32(f + 0x10, 6);
    put32(f + 0x14, 2);
    put32(f + 0x18, 2);
    put32(f + 0x1c, 1);
    put32(slp_data + 0x40, 0x50);
    put32(slp_data + 0x44, 0x58);
    put16(slp_data + 0x48, 1); put16(slp_data + 0x4a, 1);
    put16(slp_data + 0x4c, 0x8000); put16(slp_data + 0x4e, 0);
    {
        uint8_t row[] = {0x08, 7, 8, 0x05, 0x04, 9, 0x0f};
        memcpy(slp_data + 0x50, row, sizeof(row));
    }
    slp_data[0x58] = 0x0f;
    slp_size = 0x60;
}

/* SHP: "1.10", 1 frame at 0x10: width 4 : height 2, min (-1, -1), max (2, 0).
   Row 0: literal 2 (0x05 a b), skip 1 (0x01 1), end. Row 1: fill 3 (0x06 c),
   end. */
static uint8_t shp_data[0x60];
static uint32_t shp_size;
static void build_shp(void) {
    uint8_t *f = shp_data + 0x10, *p;
    memset(shp_data, 0, sizeof(shp_data));
    memcpy(shp_data, "1.10", 4);
    put32(shp_data + 4, 1);
    put32(shp_data + 8, 0x10);
    put32(f, 4u << 16 | 2u);
    put32(f + 8, (uint32_t)-1);
    put32(f + 0xc, (uint32_t)-1);
    put32(f + 0x10, 2);
    put32(f + 0x14, 0);
    p = f + 0x18;
    {
        uint8_t rows[] = {0x05, 0xa1, 0xb2, 0x01, 1, 0x00, 0x06, 0xc3, 0x00};
        memcpy(p, rows, sizeof(rows));
    }
    shp_size = 0x40;
}

static int which;
static uint8_t *load_file(void *c, const char *path, uint32_t *size) {
    uint8_t *m;
    (void)c;
    if (which == 0 && strcmp(path, "x.SLP") == 0) {
        m = malloc(slp_size); memcpy(m, slp_data, slp_size); *size = slp_size; return m;
    }
    if (which == 1 && strcmp(path, "x.SHP") == 0) {
        m = malloc(shp_size); memcpy(m, shp_data, shp_size); *size = shp_size; return m;
    }
    return NULL;
}
static int released;
static uint8_t *load_res(void *c, uint32_t type, int32_t id, uint32_t *size, int32_t *owned) {
    (void)c; (void)id;
    if (which == 2 && type == 0x73687020u) { *size = shp_size; *owned = 0; return shp_data; }
    return NULL;
}
static void release(void *c, const uint8_t *d, uint32_t n) { (void)c; (void)d; (void)n; ++released; }

int main(void) {
    AoeShapeRor s;
    int32_t loads = 0, a, b, cx, cy;
    int16_t w, h;
    AoeShapeOpsRor ops = {NULL, 1, &loads, load_file, load_res, release};
    uint8_t px[4 * 4];
    AoeShapeSurfaceRor surf = {px, 4, 4, 4, 1, 0, 0, 3, 3};
    static const uint8_t table[256] = {[0xa1] = 1, [0xb2] = 2, [0xc3] = 3, [0x11] = 0x22};

    build_slp();
    build_shp();
    /* SLP from a loose file: "x.shp" becomes "x.SLP". */
    which = 0;
    aoe_shape_load_ror(&s, "x.shp", -1, &ops);
    assert(s.slp && s.owned == 1 && loads == 1 && aoe_shape_loaded_ror(&s));
    assert(aoe_shape_count_ror(&s) == 1 && !aoe_shape_bad_ror(&s, 0) &&
           aoe_shape_bad_ror(&s, 1));
    assert(aoe_shape_size_ror(&s, 0, &w, &h) && w == 6 && h == 2);
    assert(aoe_shape_bounds_ror(&s, 0, &a, &b, &cx, &cy) && a == -2 && b == -1 &&
           cx == 3 && cy == 0);
    /* Row 0 (y = -1): columns 1, 2 drawn, 3 skipped, 4 drawn; x is relative
       to the hotspot (2). */
    assert(!aoe_shape_hit_ror(&s, -2, -1, 0));
    assert(aoe_shape_hit_ror(&s, -1, -1, 0) && aoe_shape_hit_ror(&s, 0, -1, 0));
    assert(!aoe_shape_hit_ror(&s, 1, -1, 0) && aoe_shape_hit_ror(&s, 2, -1, 0));
    assert(!aoe_shape_hit_ror(&s, 0, 0, 0) && !aoe_shape_hit_ror(&s, 9, -1, 0));
    aoe_shape_free_ror(&s, &ops);
    assert(!aoe_shape_loaded_ror(&s));

    /* SHP from a file. */
    which = 1;
    aoe_shape_load_ror(&s, "x.shp", -1, &ops);
    assert(s.shp && s.shp_table == s.shp + 8 && !s.slp);
    assert(aoe_shape_size_ror(&s, 0, &w, &h) && w == 4 && h == 2);
    assert(aoe_shape_bounds_ror(&s, 0, &a, &b, &cx, &cy) && a == -1 && b == -1 &&
           cx == 2 && cy == 0);
    /* Row -1: x -1, 0 literal; 1 skipped; 2 past the end. Row 0: -1..1 fill. */
    assert(aoe_shape_hit_ror(&s, -1, -1, 0) && aoe_shape_hit_ror(&s, 0, -1, 0) &&
           !aoe_shape_hit_ror(&s, 1, -1, 0) && !aoe_shape_hit_ror(&s, 2, -1, 0));
    assert(aoe_shape_hit_ror(&s, 1, 0, 0) && !aoe_shape_hit_ror(&s, 2, 0, 0));
    /* Blitting the frame looked up last at (0, 1). */
    memset(px, 0x11, sizeof(px));
    assert(aoe_shape_blit_ror(&s, &surf, 0, 1));
    assert(px[4] == 0xa1 && px[5] == 0xb2 && px[6] == 0x11 &&
           px[8] == 0xc3 && px[10] == 0xc3 && px[11] == 0x11);
    memset(px, 0x11, sizeof(px));
    assert(aoe_shape_blit_xlat_ror(&s, &surf, 0, 1, table));
    assert(px[4] == 1 && px[5] == 2 && px[8] == 3 && px[11] == 0x11);
    memset(px, 0x11, sizeof(px));
    assert(aoe_shape_blit_shadow_ror(&s, &surf, 0, 1, table));
    assert(px[4] == 0x22 && px[6] == 0x11 && px[10] == 0x22 && px[11] == 0x11);
    /* Bottom-up surfaces count rows from the end. */
    memset(px, 0x11, sizeof(px));
    surf.direction = -1;
    assert(aoe_shape_blit_ror(&s, &surf, 0, 1));
    assert(px[8] == 0xa1 && px[4] == 0xc3);
    /* Drawing: hotspot (1, 1) puts the frame's top-left at (0, 0);
       clipping to column 0..0 and row 1 leaves only the fill's first
       pixel. */
    surf.direction = 1;
    memset(px, 0x11, sizeof(px));
    assert(aoe_shape_draw_ror(&s, &surf, 1, 1, 0, 0, NULL, NULL));
    assert(px[0] == 0xa1 && px[1] == 0xb2 && px[4] == 0xc3 && px[6] == 0xc3);
    memset(px, 0x11, sizeof(px));
    surf.clip_right = 0;
    surf.clip_top = 1;
    assert(aoe_shape_draw_ror(&s, &surf, 1, 1, 0, 1, table, NULL));
    assert(px[0] == 0x11 && px[4] == 3 && px[5] == 0x11);
    /* Clipped literal runs start mid-run. */
    memset(px, 0x11, sizeof(px));
    surf.clip_left = 1; surf.clip_right = 3; surf.clip_top = 0;
    assert(aoe_shape_draw_ror(&s, &surf, 1, 1, 0, 0, NULL, NULL));
    assert(px[0] == 0x11 && px[1] == 0xb2 && px[4] == 0x11 && px[5] == 0xc3);
    /* Mirrored: hotspot x 2 flips the box to 0..3; the literal pair lands
       at 3 and 2, the fill at 3..1. */
    memset(px, 0x11, sizeof(px));
    surf.clip_left = 0; surf.clip_right = 3; surf.clip_top = 0; surf.clip_bottom = 3;
    assert(aoe_shape_draw_mirrored_ror(&s, &surf, 2, 1, 0, 0, NULL, NULL));
    assert(px[3] == 0xa1 && px[2] == 0xb2 && px[1] == 0x11 &&
           px[7] == 0xc3 && px[5] == 0xc3 && px[4] == 0x11);
    memset(px, 0x11, sizeof(px));
    surf.clip_right = 2;
    assert(aoe_shape_draw_mirrored_ror(&s, &surf, 2, 1, 0, 1, table, NULL));
    assert(px[3] == 0x11 && px[2] == 2 && px[7] == 0x11 && px[6] == 3);
    surf.clip_right = 3;
    /* Mode 3 draws nothing; a box wholly outside draws nothing. */
    assert(!aoe_shape_draw_ror(&s, &surf, 1, 1, 0, 3, table, NULL));
    assert(!aoe_shape_draw_ror(&s, &surf, 40, 40, 0, 0, NULL, NULL));
    aoe_shape_free_ror(&s, &ops);

    /* An SHP resource (mapped, released on free). */
    which = 2;
    aoe_shape_load_ror(&s, "x.shp", 5, &ops);
    assert(s.shp == shp_data && s.owned == 0 && s.shp_header);
    aoe_shape_free_ror(&s, &ops);
    assert(released == 1);
    /* A wrong version keeps the data without tables. */
    memcpy(shp_data, "1.00", 4);
    aoe_shape_load_ror(&s, "x.shp", 5, &ops);
    assert(s.shp && !s.shp_header && aoe_shape_bad_ror(&s, 0) &&
           aoe_shape_count_ror(&s) == 0);
    return 0;
}
