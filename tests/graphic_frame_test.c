#include "graphic_frame.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void synthetic(void) {
    DatGraphic graphic = {.present = 1, .frame_count = 3,
                          .angle_count = 8};
    AoeGraphicFrame result = {99, 99};
    assert(aoe_graphic_select_frame(&graphic, 4, 2, &result));
    assert(result.slp_frame == 14 && result.mirror_horizontal == 0);
    graphic.mirror_limit = 5;
    assert(aoe_graphic_select_frame(&graphic, 0, 2, &result));
    assert(result.slp_frame == 8 && result.mirror_horizontal == 1);
    assert(aoe_graphic_select_frame(&graphic, 2, 1, &result));
    assert(result.slp_frame == 1 && result.mirror_horizontal == 0);
    assert(aoe_graphic_select_frame(&graphic, 6, 0, &result));
    assert(result.slp_frame == 12 && result.mirror_horizontal == 1);
    graphic.angle_count = 2;
    graphic.mirror_limit = 1;
    assert(aoe_graphic_select_frame(&graphic, 1, 2, &result));
    assert(result.slp_frame == 2 && result.mirror_horizontal == 1);
    assert(aoe_graphic_select_frame(&graphic, 0, 99, &result));
    assert(result.slp_frame == 0 && result.mirror_horizontal == 0);
    result = (AoeGraphicFrame){99, 99};
    assert(!aoe_graphic_select_frame(&graphic, 2, 0, &result));
    assert(result.slp_frame == 99 && result.mirror_horizontal == 99);
}

static int real_dat(const char *path) {
    DatBlob blob = {0};
    DatIntro intro = {0};
    DatSounds sounds = {0};
    DatGraphics graphics = {0};
    if (!dat_load(path, &blob)) return 0;
    int ok = dat_parse_intro(&blob, &intro) &&
             dat_parse_sounds(&blob, &intro, &sounds) &&
             dat_parse_graphics(&blob, &sounds, &graphics);
    size_t checked = 0, blank = 0, failed = 0;
    if (ok) {
        for (uint16_t i = 0; i < graphics.count; ++i) {
            const DatGraphic *graphic = &graphics.graphics[i];
            if (!graphic->present) continue;
            if (!graphic->frame_count || !graphic->angle_count) {
                ++blank;
                continue;
            }
            for (uint16_t angle = 0; angle < graphic->angle_count; ++angle) {
                AoeGraphicFrame result;
                if (!aoe_graphic_select_frame(graphic, angle, 0, &result)) {
                    fprintf(stderr, "%s: selector failed for graphic %u angle %u\n",
                            path, i, angle);
                    ++failed;
                } else {
                    ++checked;
                }
            }
        }
        printf("%s: %zu angle selections, %zu blank graphics, %zu failures\n",
               path, checked, blank, failed);
        ok = failed == 0;
    }
    dat_free_graphics(&graphics);
    dat_free_sounds(&sounds);
    dat_free_intro(&intro);
    dat_free(&blob);
    return ok;
}

static int16_t snd_log[8];
static int snd_n;
static void snd_play(void *c, int16_t id) { (void)c; snd_log[snd_n++] = id; }

static void frame_sounds(void) {
    DatGraphicAttackSound pairs[6] = {{2, 101}, {5, 102}, {-1, 103},
                                      {1, 201}, {4, 202}, {6, 203}};
    DatGraphic g = {.present = 1, .frame_count = 8, .angle_count = 2,
                    .sound_id = 50, .attack_sound_count = 6,
                    .attack_sounds = pairs};
    snd_n = 0;
    assert(aoe_graphic_frame_sounds(&g, 3, 3, 0, snd_play, NULL) == 0);
    /* entering from frame 0: own sound, then delay 2 in [0,3) */
    assert(aoe_graphic_frame_sounds(&g, 0, 3, 0, snd_play, NULL) == 2);
    assert(snd_log[0] == 50 && snd_log[1] == 101);
    snd_n = 0;
    /* forward without entering: only delay 5 in [3,6); stops at -1 */
    assert(aoe_graphic_frame_sounds(&g, 3, 6, 0, snd_play, NULL) == 1);
    assert(snd_log[0] == 102);
    snd_n = 0;
    /* wrap 6 -> 2 on angle 1: own sound, delay 1 (<2) and 6 (>=6) */
    assert(aoe_graphic_frame_sounds(&g, 6, 2, 1, snd_play, NULL) == 3);
    assert(snd_log[0] == 50 && snd_log[1] == 201 && snd_log[2] == 203);
    snd_n = 0;
    /* wrap onto frame 0: no own sound; out-of-range angle uses angle 0 */
    assert(aoe_graphic_frame_sounds(&g, 6, 0, 5, snd_play, NULL) == 0);
    g.sound_id = -1;
    g.attack_sound_count = 0;
    assert(aoe_graphic_frame_sounds(&g, 0, 3, 0, snd_play, NULL) == 0);
}

typedef struct { const DatGraphic *g; int32_t x, y, angle, frame; int calls; int hit_x, hit_y; } PixCtx;
static int pix(void *c, const DatGraphic *g, int32_t x, int32_t y, int32_t angle, int32_t frame) {
    PixCtx *p = c;
    p->g = g; p->x = x; p->y = y; p->angle = angle; p->frame = frame; ++p->calls;
    return x == p->hit_x && y == p->hit_y;
}
static int draws;
static int32_t draw_x, draw_angle;
static void d20(void *c, const DatGraphic *g, int32_t angle, int32_t x, int32_t y) {
    (void)c; (void)g; (void)y; ++draws; draw_x = x; draw_angle = angle;
}

static void hit_tests(void) {
    DatGraphic part = {.present = 1, .angle_count = 16, .layer = 20};
    DatGraphicDelta deltas[2] = {{.graphic = &part, .offset_x = 10, .offset_y = 4, .display_angle = -1},
                                 {.graphic = &part, .offset_x = 1, .offset_y = 1, .display_angle = 3}};
    DatGraphic g = {.present = 1, .angle_count = 8, .box = {-5, -20, 5, 0}};
    PixCtx p = {.hit_x = 1000};
    /* box: inclusive, no pixel test */
    assert(aoe_graphic_hit_test_ror(&g, 5, 0, 2, 1, pix, &p) == 1 && p.calls == 0);
    assert(aoe_graphic_hit_test_ror(&g, 6, 0, 2, 1, pix, &p) == 0 && p.calls == 1);
    assert(p.g == &g && p.angle == 2 && p.frame == 1);
    g.delta_count = 2; g.deltas = deltas;
    p.calls = 0; p.hit_x = 20; p.hit_y = 6;
    assert(aoe_graphic_hit_test_ror(&g, 30, 10, 3, 2, pix, &p) == 1);
    assert(p.calls == 1 && p.g == &part && p.angle == 6 && p.frame == 2);
    /* tolerance: (30,10) misses; offset (-1,-1) lands on (19,5)+... */
    p.calls = 0; p.hit_x = 18; p.hit_y = 5;  /* local (28, 9) via delta 0 */
    assert(aoe_graphic_hit_test_near_ror(&g, 30, 10, 0, 3, 2, pix, &p) == 0);
    assert(aoe_graphic_hit_test_near_ror(&g, 30, 10, 2, 3, 2, pix, &p) == 1);
    p.hit_x = 22;  /* would need dx = +2: outside [-2, 2) */
    assert(aoe_graphic_hit_test_near_ror(&g, 30, 10, 2, 3, 2, pix, &p) == 0);
    /* layer-20 draw: delta 0 always, delta 1 only at angle 3 */
    draws = 0;
    assert(aoe_graphic_draw_layer20_ror(&g, 3, 100, 0, d20, NULL) == 2);
    assert(draw_x == 101 && draw_angle == 6);
    assert(aoe_graphic_draw_layer20_ror(&g, 9, 100, 0, d20, NULL) == 1);
    assert(draw_x == 110 && draw_angle == 0);
    g.delta_count = 0;
    assert(aoe_graphic_draw_layer20_ror(&g, 0, 0, 0, d20, NULL) == 0);
    part.delta_count = 0;
    assert(aoe_graphic_draw_layer20_ror(&part, 0, 0, 0, d20, NULL) == 1);
    /* lower layers: below 20 and not the skipped layer */
    assert(aoe_graphic_draw_lower_layers_ror(&part, 0, 0, 0, 5, d20, NULL) == 0);
    part.layer = 5;
    assert(aoe_graphic_draw_lower_layers_ror(&part, 0, 0, 0, 5, d20, NULL) == 0);
    assert(aoe_graphic_draw_lower_layers_ror(&part, 0, 0, 0, 6, d20, NULL) == 1);
    g.delta_count = 2;
    assert(aoe_graphic_draw_lower_layers_ror(&g, 3, 100, 0, 6, d20, NULL) == 2);
    assert(aoe_graphic_draw_layer20_ror(&g, 3, 100, 0, d20, NULL) == 0);
}

static uint32_t bounds_frame;
static int fb(void *c, const DatGraphic *g, uint32_t f, int32_t *x0, int32_t *y0,
              int32_t *x1, int32_t *y1) {
    (void)c; (void)g; bounds_frame = f;
    *x0 = -4; *y0 = -30; *x1 = 10; *y1 = 2; return 1;
}

static void union_bounds(void) {
    DatGraphic g = {.present = 1, .frame_count = 3, .angle_count = 8};
    int16_t x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    /* frame 5 wraps to 0; angle 2 unmirrored -> slp frame 6 */
    assert(aoe_graphic_union_part_bounds_ror(&g, 2, 5, 1, 2, fb, NULL, &x0, &y0, &x1, &y1));
    assert(bounds_frame == 6 && x0 == -3 && y0 == -28 && x1 == 11 && y1 == 4);
    /* mirrored angle negates and swaps x */
    g.mirror_limit = 1;
    x0 = y0 = x1 = y1 = 0;
    assert(aoe_graphic_union_part_bounds_ror(&g, 7, 1, 0, 0, fb, NULL, &x0, &y0, &x1, &y1));
    assert(x0 == -10 && x1 == 4 && y0 == -30 && y1 == 2);
}

static void graphic_bounds(void) {
    DatGraphic part = {.present = 1, .frame_count = 1, .angle_count = 4};
    DatGraphicDelta d = {.graphic = &part, .offset_x = 5, .offset_y = -1};
    DatGraphic g = {.present = 1, .frame_count = 1, .angle_count = 8};
    int16_t x0 = 9, y0 = 9, x1 = 9, y1 = 9;
    /* own box wins */
    g.box[0] = -1; g.box[1] = -2; g.box[2] = 3; g.box[3] = 4;
    assert(aoe_graphic_bounds_ror(&g, 0, 0, fb, NULL, &x0, &y0, &x1, &y1));
    assert(x0 == -1 && y0 == -2 && x1 == 3 && y1 == 4);
    /* empty box: grown from 0 by the delta at its offset, angle 4 -> 2 */
    g.box[2] = 0; g.box[0] = 0;
    g.delta_count = 1; g.deltas = &d;
    /* selection mode 0 skips every delta (0x004a5d60 +0x2c gate) */
    assert(!aoe_graphic_bounds_ror(&g, 4, 0, fb, NULL, &x0, &y0, &x1, &y1));
    g.selection_mode = 1;
    assert(aoe_graphic_bounds_ror(&g, 4, 0, fb, NULL, &x0, &y0, &x1, &y1));
    assert(bounds_frame == 2 && x0 == 0 && y0 == -31 && x1 == 15 && y1 == 1);
    /* nothing resolvable: degenerate */
    d.graphic = NULL;
    assert(!aoe_graphic_bounds_ror(&g, 4, 0, fb, NULL, &x0, &y0, &x1, &y1));
    assert(x0 == 0 && y1 == 0);
}


/* SLP: one 6 x 2 frame, hotspot (2, 1); row 0 drawn at x 1, 2, 4. */
static void put32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static uint8_t slp[0x60];
static void build_slp(void) {
    uint8_t *f = slp + 0x20;
    uint8_t row[] = {0x08, 7, 8, 0x05, 0x04, 9, 0x0f};
    memcpy(slp, "2.0N", 4);
    put32(slp + 4, 1);
    put32(f + 0, 0x40); put32(f + 4, 0x48);
    put32(f + 0x10, 6); put32(f + 0x14, 2); put32(f + 0x18, 2); put32(f + 0x1c, 1);
    put32(slp + 0x40, 0x50); put32(slp + 0x44, 0x58);
    put16(slp + 0x48, 1); put16(slp + 0x4a, 1);
    put16(slp + 0x4c, 0x8000); put16(slp + 0x4e, 0);
    memcpy(slp + 0x50, row, sizeof(row));
    slp[0x58] = 0x0f;
}
static char loaded_path[32];
static int32_t loaded_id;
static uint8_t *ld_file(void *c, const char *path, uint32_t *size) {
    (void)c; (void)path; (void)size; return NULL;
}
static uint8_t *ld_res(void *c, uint32_t type, int32_t id, uint32_t *size, int32_t *owned) {
    (void)c;
    if (type != 0x736c7020u) return NULL;
    loaded_id = id; *size = sizeof slp; *owned = 0; return slp;
}
static void ld_release(void *c, const uint8_t *d, uint32_t n) { (void)c; (void)d; (void)n; }
static AoeGraphicShapeRor slots[2];
static const DatGraphic *slot_owner;
static AoeGraphicShapeRor *slot_of(void *c, const DatGraphic *g) {
    (void)c; return g == slot_owner ? &slots[0] : &slots[1];
}
static uint32_t clock_ms;
static uint32_t now_ms(void *c) { (void)c; return clock_ms += 5; }
static int32_t stat_slot = -1; static uint32_t stat_ms;
static void rec(void *c, int32_t slot, uint32_t ms) { (void)c; stat_slot = slot; stat_ms = ms; }

static void slp_tests(void) {
    int32_t loads = 0, a, b, c, d;
    int16_t x0, y0, x1, y1;
    AoeShapeOpsRor sops = {NULL, 0, &loads, ld_file, ld_res, ld_release};
    AoeGraphicShapeOpsRor ops = {NULL, slot_of, &sops, now_ms, rec};
    DatGraphic g = {.present = 1, .frame_count = 1, .angle_count = 8,
                    .slp_id = 77, .filename = "x"};
    build_slp();
    slot_owner = &g;
    /* not loadable and nothing loaded: no shape, every test misses */
    assert(!aoe_graphic_frame_bounds_slp_ror(&ops, &g, 0, &a, &b, &c, &d));
    assert(!slots[0].loaded);
    g.slp_loadable = 1;
    /* 0x004a5c60: first use loads (resource 77), records statistic 11 */
    assert(aoe_graphic_frame_bounds_slp_ror(&ops, &g, 0, &a, &b, &c, &d));
    assert(slots[0].loaded && loaded_id == 77 && loads == 1);
    assert(stat_slot == 11 && stat_ms == 5);
    assert(a == -2 && b == -1 && c == 3 && d == 0);
    assert(!aoe_graphic_frame_bounds_slp_ror(&ops, &g, 1, &a, &b, &c, &d));
    assert(loads == 1);
    (void)loaded_path;
    /* 0x004a6150 mode 0: miss */
    assert(!aoe_graphic_pixel_test_slp_ror(&ops, &g, -1, -1, 0, 0));
    /* mode 1: SLP pixels; angle 0 unmirrored */
    g.selection_mode = 1;
    assert(aoe_graphic_pixel_test_slp_ror(&ops, &g, -1, -1, 0, 0));
    assert(!aoe_graphic_pixel_test_slp_ror(&ops, &g, 1, -1, 0, 0));
    assert(aoe_graphic_pixel_test_slp_ror(&ops, &g, 2, -1, 0, 1)); /* frame 1 = count: kept, selects 0 */
    assert(aoe_graphic_pixel_test_slp_ror(&ops, &g, -1, -1, 0, 5)); /* past count: 0 */
    /* mirrored (two angles with mirror: angle 1 is frame 0 flipped): x negated */
    g.mirror_limit = 1; g.angle_count = 2;
    assert(aoe_graphic_pixel_test_slp_ror(&ops, &g, 1, -1, 1, 0));
    assert(!aoe_graphic_pixel_test_slp_ror(&ops, &g, -1, -1, 1, 0));
    /* mode 2: the box -2..3 x -1..0, mirrored -3..2 */
    g.selection_mode = 2;
    assert(aoe_graphic_pixel_test_slp_ror(&ops, &g, -3, 0, 1, 0));
    assert(!aoe_graphic_pixel_test_slp_ror(&ops, &g, 3, 0, 1, 0));
    g.mirror_limit = 0; g.angle_count = 8;
    assert(aoe_graphic_pixel_test_slp_ror(&ops, &g, 3, 0, 0, 0));
    assert(!aoe_graphic_pixel_test_slp_ror(&ops, &g, -3, 0, 0, 0));
    assert(!aoe_graphic_pixel_test_slp_ror(&ops, &g, 0, 1, 0, 0));
    g.selection_mode = 3;
    assert(!aoe_graphic_pixel_test_slp_ror(&ops, &g, 0, 0, 0, 0));
    /* 0x004a5d60 with SLP: grows from the frame box */
    assert(aoe_graphic_bounds_slp_ror(&g, 0, 0, &ops, &x0, &y0, &x1, &y1));
    assert(x0 == -2 && y0 == -1 && x1 == 3 && y1 == 0);
    /* an owning graphic without a shape: 0 even with a coordinate box */
    {
        DatGraphic h = {.present = 1, .frame_count = 1, .angle_count = 1,
                        .box = {-1, -1, 1, 1}};
        assert(!aoe_graphic_bounds_slp_ror(&h, 0, 0, &ops, &x0, &y0, &x1, &y1));
        assert(!slots[1].loaded);
    }
    aoe_shape_free_ror(&slots[0].shape, &sops);
}

int main(int argc, char **argv) {
    slp_tests();
    union_bounds();
    graphic_bounds();
    hit_tests();
    frame_sounds();
    synthetic();
    for (int i = 1; i < argc; ++i)
        if (!real_dat(argv[i])) return 1;
    return 0;
}
