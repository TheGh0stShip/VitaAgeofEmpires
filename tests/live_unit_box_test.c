/* Patched RoR 1.0a draw-list selection box 0x004a76f0. */
#include "live_unit_box.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { int32_t kind; int16_t layer; int32_t v[8]; int32_t prio; uint8_t colour; } Shape;
typedef struct { int32_t frame; int16_t layer; int32_t x, y; } Num;
static Shape shapes[16];
static Num nums[16];
static int nshapes, nnums;

/* A simple projection: x = base_x + 10 x, y = base_y + 10 y - 10 z. */
static void project(void *c, float x, float y, float z, int32_t bx, int32_t by,
                    int16_t *ox, int16_t *oy) {
    (void)c;
    *ox = (int16_t)(bx + (int32_t)(x * 10.0f));
    *oy = (int16_t)(by + (int32_t)(y * 10.0f) - (int32_t)(z * 10.0f));
}
static void shape(void *c, int32_t kind, int16_t layer, const int32_t v[8], int32_t prio, uint8_t col) {
    (void)c;
    shapes[nshapes].kind = kind, shapes[nshapes].layer = layer;
    memcpy(shapes[nshapes].v, v, sizeof shapes[nshapes].v);
    shapes[nshapes].prio = prio, shapes[nshapes].colour = col;
    ++nshapes;
}
static int number(void *c, int32_t f, int16_t layer, int32_t x, int32_t y, int32_t prio) {
    (void)c;
    assert(prio == 0x1e);
    nums[nnums++] = (Num){f, layer, x, y};
    return 1;
}

typedef struct { int32_t x0, y0, x1, y1; } Line;
static Line lines[32];
static int nlines, view_on = 1, black = -1, locks, unlocks;
static int view(void *c) { (void)c; return view_on; }
static void unlock(void *c, void *r, const char *n) { (void)c; (void)r; (void)n; ++unlocks; }
static void lock(void *c, void *r, const char *n) { (void)c; (void)r; (void)n; ++locks; }
static void begin(void *c, void *r, int b) { (void)c; (void)r; black = b; }
static void line(void *c, void *r, int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    (void)c; (void)r;
    lines[nlines++] = (Line){x0, y0, x1, y1};
}

static void brackets(void) {
    static AoeLiveUnitCore u;
    AoeLiveUnitFrame3dInput in = {&u, 2.0f, 4.0f, 0, 0};
    AoeLiveUnitFrame3dOps ops = {0};
    ops.view = view, ops.unlock = unlock, ops.lock = lock, ops.begin = begin;
    ops.line = line, ops.project = project;
    u.draw_left = 10, u.draw_top = 20;
    assert(aoe_live_unit_draw_brackets_ror(&in, 8.0f, NULL, 0, 0, &ops) == 1);
    assert(nlines == 6 && black == 0 && locks == 1 && unlocks == 1);
    /* base (10, 20): the vertical ticks at (ox, -oy) = (30, -20). */
    assert(lines[0].x0 == 30 && lines[0].y0 == -20 && lines[0].x1 == 30 && lines[0].y1 == -40);
    assert(lines[1].y0 == -80 && lines[1].y1 == -100);
    assert(lines[2].x1 == 15 && lines[3].x0 == -10 && lines[3].x1 == 5);
    assert(lines[4].y1 == 10 && lines[5].y0 == 60 && lines[5].y1 == 30);
    /* The logging rule picks the black pen; no view draws nothing. */
    in.logging_on = 1, in.unit_78 = 0;
    nlines = 0;
    aoe_live_unit_draw_brackets_ror(&in, 8.0f, NULL, 0, 0, &ops);
    assert(black == 1);
    /* The full set: 18 strokes, anchor 16 higher. */
    nlines = 0;
    in.logging_on = 0;
    assert(aoe_live_unit_draw_brackets_full_ror(&in, 8.0f, NULL, 0, 0, &ops) == 1);
    assert(nlines == 18 && black == 0);
    assert(lines[0].x0 == 30 && lines[0].y0 == -116 && lines[0].x1 == 15 && lines[0].y1 == -116);
    assert(lines[17].x0 == 30 && lines[17].y0 == -16 && lines[17].x1 == 30 && lines[17].y1 == -36);
    view_on = 0;
    nlines = 0;
    assert(aoe_live_unit_draw_brackets_ror(&in, 8.0f, NULL, 0, 0, &ops) == 0 && nlines == 0);
}

int main(void) {
    static AoeLiveUnitCore u;
    AoeLiveUnitBoxInput in;
    AoeLiveUnitBoxOps ops = {0, project, shape, number};
    memset(&in, 0, sizeof in);
    in.unit = &u;
    u.unit_id = 42;
    u.draw_left = 5;
    u.draw_top = -3;
    u.hit_points = 30.0f;
    in.size[0] = 1.0f, in.size[1] = 2.0f, in.size[2] = 1.5f;
    in.max_hp = 40;

    /* Colours. */
    assert(aoe_live_unit_box_colour_ror(&in) == 0xff);
    in.screen_mode = 3;
    assert(aoe_live_unit_box_colour_ror(&in) == 0x74);
    in.screen_mode = 0, in.template_a8 = 1, in.template_a9 = 0x33, in.game_1b0 = 7;
    assert(aoe_live_unit_box_colour_ror(&in) == 0x33);
    u.action_flags_36 = 1;
    assert(aoe_live_unit_box_colour_ror(&in) == 0xff);
    u.action_flags_36 = 1 | 2;
    assert(aoe_live_unit_box_colour_ror(&in) == 0x4a);
    u.action_flags_36 = 1 | 2 | 4;
    assert(aoe_live_unit_box_colour_ror(&in) == 0x97);
    u.action_flags_36 = 0, in.logging_on = 1, in.unit_78 = 42;
    /* logging wins over the own colour */
    assert(aoe_live_unit_box_colour_ror(&in) == 0);
    in.logging_on = 0;

    /* A selected unit in group 3 with a health bar. */
    u.action_flags_36 = 1;
    in.group = 3;
    in.owner_viewed = 1;
    aoe_live_unit_draw_box_ror(&in, 100, 200, &ops);
    assert(nshapes == 2 && nnums == 1);
    /* base (105, 197); corners (sx,-sy) (sx,sy) (-sx,sy) (-sx,-sy) */
    assert(shapes[0].kind == 1 && shapes[0].layer == 197 && shapes[0].prio == 10 &&
           shapes[0].colour == 0xff);
    assert(shapes[0].v[0] == 115 && shapes[0].v[1] == 177 && shapes[0].v[2] == 115 &&
           shapes[0].v[3] == 217 && shapes[0].v[4] == 95 && shapes[0].v[5] == 217 &&
           shapes[0].v[6] == 95 && shapes[0].v[7] == 177);
    assert(nums[0].frame == 3 && nums[0].x == 89 && nums[0].y == 169);
    /* bar at (115, 177 - 15 = 162): x 103..126, rows 160..161, fill
       30 * 24 / 40 + 103 = 121. */
    assert(shapes[1].kind == 2 && shapes[1].prio == 0x1e && shapes[1].v[0] == 103 &&
           shapes[1].v[1] == 160 && shapes[1].v[2] == 126 && shapes[1].v[3] == 161 &&
           shapes[1].v[4] == 121 && shapes[1].v[5] == 1 && shapes[1].v[6] == 122 &&
           shapes[1].v[7] == 1);

    /* Full health: fill to the end, no back; the group needs the viewed owner. */
    nshapes = nnums = 0;
    u.hit_points = 40.0f;
    in.owner_viewed = 0;
    aoe_live_unit_draw_box_ror(&in, 100, 200, &ops);
    assert(nnums == 0 && shapes[1].v[4] == 126 && shapes[1].v[5] == 1 &&
           shapes[1].v[6] == -1 && shapes[1].v[7] == 0);

    /* IDs after the bar's right end; zero digits are kind 4 boxes. */
    nshapes = nnums = 0;
    in.show_ids = 1;
    u.unit_id = 1203;
    aoe_live_unit_draw_box_ror(&in, 100, 200, &ops);
    assert(nnums == 3 && nums[0].frame == 1 && nums[0].x == 133 && nums[0].y == 161 &&
           nums[1].frame == 2 && nums[1].x == 140 && nums[2].frame == 3 && nums[2].x == 154);
    assert(nshapes == 3 && shapes[2].kind == 4 && shapes[2].v[0] == 148 &&
           shapes[2].v[1] == 161 && shapes[2].v[3] == 166 && shapes[2].colour == 0xff);

    /* Without a bar the IDs follow the second corner; a negative ID shows
       a minus and only its last digit. */
    nshapes = nnums = 0;
    in.template_a8 = 2;
    u.unit_id = -57;
    aoe_live_unit_draw_box_ror(&in, 100, 200, &ops);
    assert(nshapes == 2 && shapes[1].kind == 4 && shapes[1].colour == 0x97 &&
           shapes[1].v[0] == 123 && shapes[1].v[1] == 220 && shapes[1].v[3] == 220);
    assert(nnums == 1 && nums[0].frame == 7 && nums[0].x == 129 && nums[0].y == 217);

    brackets();
    puts("live unit box ok");
    return 0;
}
