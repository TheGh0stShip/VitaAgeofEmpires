#include "live_unit_draw.h"

#include <assert.h>
#include <string.h>

typedef struct {
    char order[4];
    unsigned count;
    int32_t draw_id;
    uint8_t owner_color;
    const AoeLiveUnitCore *unit;
    const AoeSpriteRuntime *sprites;
    const void *graphic;
    void *renderer;
    int32_t tile_x, tile_y;
    int16_t left, top, right, bottom;
    uint8_t frame;
    int sprite_ok;
} Trace;

static void overlay(void *context, void *renderer,
                    const AoeLiveUnitCore *unit, int32_t x, int32_t y) {
    Trace *trace = context;
    trace->order[trace->count++] = 'o';
    trace->unit = unit;
    trace->renderer = renderer;
    trace->tile_x = x;
    trace->tile_y = y;
}

static int sprite(void *context, void *renderer,
                  const AoeLiveUnitCore *unit,
                  const AoeSpriteRuntime *sprites, const void *graphic,
                  uint8_t owner_color, uint8_t frame,
                  int32_t draw_id, int16_t left, int16_t top,
                  int16_t right, int16_t bottom) {
    Trace *trace = context;
    trace->order[trace->count++] = 's';
    trace->unit = unit;
    trace->sprites = sprites;
    trace->graphic = graphic;
    trace->owner_color = owner_color;
    trace->frame = frame;
    trace->draw_id = draw_id;
    trace->renderer = renderer;
    trace->left = left;
    trace->top = top;
    trace->right = right;
    trace->bottom = bottom;
    return trace->sprite_ok;
}

typedef struct { char log[8]; unsigned n; int16_t x, y; uint8_t frame;
                 uint32_t color; int32_t extra; } SlotTrace;
static void slot_push(void *c, char ch) { SlotTrace *t = c; t->log[t->n++] = ch; }
static void s_before(void *c, void *r, int32_t x, int32_t y) { (void)r; (void)x; (void)y; slot_push(c, 'b'); }
static void s_after(void *c, void *r, int32_t x, int32_t y) { (void)r; (void)x; (void)y; slot_push(c, 'a'); }
static void s_normal(void *c, void *r, int32_t x, int32_t y) { (void)r; (void)x; (void)y; slot_push(c, 'n'); }
static void s_box(void *c, void *r, int32_t x, int32_t y) { (void)r; (void)x; (void)y; slot_push(c, 'x'); }
static void s_full(void *c, void *r, int32_t x, int32_t y) { (void)r; (void)x; (void)y; slot_push(c, 'f'); }
static void s_outline(void *c, uint8_t f, int16_t x, int16_t y, uint32_t col, void *r) {
    SlotTrace *t = c; (void)r; t->frame = f; t->x = x; t->y = y; t->color = col; slot_push(c, 'o');
}
static void s_shadow(void *c, uint8_t f, int16_t x, int16_t y, uint32_t col, void *r, int32_t e) {
    SlotTrace *t = c; (void)r; t->frame = f; t->x = x; t->y = y; t->color = col; t->extra = e; slot_push(c, 's');
}

static void test_draw_slots(void) {
    AoeLiveUnitCore unit = {0};
    SlotTrace t;
    AoeLiveUnitDrawSlotOps ops = {&t, s_before, s_after, s_outline, s_shadow,
                                  s_normal, s_box, s_full};
    unit.state = 2; unit.scenario_frame = 4;
    unit.draw_left = -10; unit.draw_top = 5; unit.draw_right = 20; unit.draw_bottom = 30;
    memset(&t, 0, sizeof t);
    assert(aoe_live_unit_draw_shadow_ror(&unit, 1, 0x77, NULL, 100, 200, 9, &ops));
    assert(t.x == 120 && t.y == 230 && t.frame == 4 && t.color == 0x77 && t.extra == 9);
    assert(!aoe_live_unit_draw_shadow_ror(&unit, 0, 0x77, NULL, 0, 0, 0, &ops));
    memset(&t, 0, sizeof t);
    assert(aoe_live_unit_draw_outline_ror(&unit, 1, 0x55, 0, NULL, 100, 200, &ops));
    assert(!strcmp(t.log, "o") && t.x == 90 && t.y == 205);
    memset(&t, 0, sizeof t);
    assert(aoe_live_unit_draw_outline_ror(&unit, 1, 0x55, 3, NULL, 0, 0, &ops));
    assert(!strcmp(t.log, "boa"));
    memset(&t, 0, sizeof t);
    unit.action_flags_36 = 1;
    assert(aoe_live_unit_draw_outline_ror(&unit, 1, 0x55, 0, NULL, 0, 0, &ops));
    assert(!strcmp(t.log, "boa"));
    unit.state = 7;
    assert(!aoe_live_unit_draw_outline_ror(&unit, 1, 0x55, 0, NULL, 0, 0, &ops));
    memset(&t, 0, sizeof t);
    assert(aoe_live_unit_draw_frame_dispatch_ror(0, 0, NULL, 0, 0, &ops) == 1);
    assert(aoe_live_unit_draw_frame_dispatch_ror(0, 1, NULL, 0, 0, &ops) == 3);
    assert(aoe_live_unit_draw_frame_dispatch_ror(0, 2, NULL, 0, 0, &ops) == 2);
    assert(aoe_live_unit_draw_frame_dispatch_ror(0, 3, NULL, 0, 0, &ops) == 2);
    assert(aoe_live_unit_draw_frame_dispatch_ror(1, 0, NULL, 0, 0, &ops) == 3);
    assert(aoe_live_unit_draw_frame_dispatch_ror(0, 4, NULL, 0, 0, &ops) == 0);
    assert(!strcmp(t.log, "nfxxf"));
}

typedef struct { int32_t r[4][5]; unsigned n; } SelTrace;
static int sel_bounds(void *c, const void *g, uint8_t f, int16_t *x0, int16_t *y0,
                      int16_t *x1, int16_t *y1) {
    (void)c; (void)g; assert(f == 4);
    *x0 = -10; *y0 = -40; *x1 = 12; *y1 = 2; return 1;
}
static void sel_rec(void *c, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t col) {
    SelTrace *t = c; int32_t *r = t->r[t->n++];
    r[0] = x0; r[1] = y0; r[2] = x1; r[3] = y1; r[4] = col;
}
static void sel_rect(void *c, void *rd, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t col) {
    (void)rd; sel_rec(c, x0, y0, x1, y1, col);
}

typedef struct { int32_t kind, row, depth, f1c; int16_t c[8]; } QNode;
static QNode qn[4]; static unsigned qn_n;
static void q_add(void *c, int32_t kind, int32_t row, const int16_t co[8],
                  int32_t depth, int32_t f1c) {
    QNode *n = &qn[qn_n++]; (void)c;
    n->kind = kind; n->row = row; n->depth = depth; n->f1c = f1c;
    memcpy(n->c, co, sizeof(n->c));
}

static void test_queue_selection_and_touch(void) {
    static int graphic;
    DatUnit tmpl = {0};
    AoeLiveUnitCore unit = {0};
    SelTrace t = {0};
    AoeLiveUnitSelectionOps ops = {&t, NULL, sel_bounds, sel_rect, sel_rect, q_add};
    tmpl.hit_points = 50;
    unit.unit_template = &tmpl;
    unit.scenario_frame = 4; unit.state = 2; unit.hit_points = 200.0f;
    unit.draw_left = 3; unit.draw_top = 5;
    /* 0x004a75c0: box 92,164,116,208; hp over max clamps the fill. */
    qn_n = 0;
    assert(aoe_live_unit_queue_selection_ror(&unit, &graphic, 100, 200, &ops) && qn_n == 2);
    assert(qn[0].kind == 4 && qn[0].row == 208 && qn[0].depth == 10 && qn[0].f1c == 0xff &&
           qn[0].c[0] == 92 && qn[0].c[1] == 164 && qn[0].c[2] == 116 && qn[0].c[3] == 208 &&
           qn[0].c[4] == 0);
    assert(qn[1].kind == 2 && qn[1].row == 208 && qn[1].depth == 0x14 &&
           qn[1].c[0] == 92 && qn[1].c[1] == 158 && qn[1].c[2] == 118 && qn[1].c[3] == 163 &&
           qn[1].c[4] == 117 && qn[1].c[6] == 118);
    unit.hit_points = 20.0f; qn_n = 0;
    assert(aoe_live_unit_queue_selection_ror(&unit, &graphic, 100, 200, &ops) &&
           qn[1].c[4] == 102);
    unit.state = 3; qn_n = 0;
    assert(aoe_live_unit_queue_selection_ror(&unit, &graphic, 100, 200, &ops) && qn_n == 1);

    /* 0x004ab0c0: box (93,165)-(115,207). */
    assert(aoe_live_unit_box_touches_ror(&unit, &graphic, 100, 200, 0, 0, 93, 165, &ops));
    assert(aoe_live_unit_box_touches_ror(&unit, &graphic, 100, 200, 100, 170, 101, 171, &ops));
    assert(!aoe_live_unit_box_touches_ror(&unit, &graphic, 100, 200, 0, 0, 92, 300, &ops));
    /* A thin query crossing the box horizontally (no corner inside). */
    assert(aoe_live_unit_box_touches_ror(&unit, &graphic, 100, 200, 50, 180, 200, 190, &ops));
    assert(aoe_live_unit_box_touches_ror(&unit, &graphic, 100, 200, 100, 100, 110, 300, &ops));
    assert(aoe_live_unit_box_touches_ror(&unit, &graphic, 100, 200, 50, 207, 200, 300, &ops));
    assert(!aoe_live_unit_box_touches_ror(&unit, &graphic, 100, 200, 50, 208, 200, 300, &ops));
    assert(!aoe_live_unit_box_touches_ror(&unit, NULL, 100, 200, 0, 0, 999, 999, &ops));
}

static void test_selection(void) {
    static int graphic;
    DatUnit tmpl = {0};
    AoeLiveUnitCore unit = {0};
    SelTrace t = {0};
    AoeLiveUnitSelectionOps ops = {&t, NULL, sel_bounds, sel_rect, sel_rect};
    tmpl.hit_points = 50;
    unit.unit_template = &tmpl;
    unit.scenario_frame = 4; unit.state = 2; unit.hit_points = 20.7f;
    unit.draw_left = 3; unit.draw_top = 5;
    assert(aoe_live_unit_draw_selection_ror(&unit, &graphic, NULL, 100, 200, &ops));
    assert(t.n == 4);
    /* left = -10+100+3-1 = 92, top = -40+200+5-1 = 164 */
    assert(t.r[0][0] == 92 && t.r[0][1] == 164 && t.r[0][2] == 116 && t.r[0][3] == 208 && t.r[0][4] == 0xff);
    assert(t.r[1][0] == 92 && t.r[1][1] == 158 && t.r[1][2] == 118 && t.r[1][3] == 163);
    assert(t.r[2][0] == 93 && t.r[2][1] == 159 && t.r[2][2] == 116 && t.r[2][3] == 161 && t.r[2][4] == 0x55);
    assert(t.r[3][2] == 20 * 25 / 50 + 92 && t.r[3][3] == 162 && t.r[3][4] == 0x25);
    t.n = 0; unit.state = 3;
    assert(aoe_live_unit_draw_selection_ror(&unit, &graphic, NULL, 100, 200, &ops) && t.n == 1);
    assert(!aoe_live_unit_draw_selection_ror(&unit, NULL, NULL, 0, 0, &ops));
}


/* 0x004a7c60 / 0x004a7e40 GDI frame halves with an identity-like projection
   (x * 10 + base_x, y * 10 + base_y - z). */
static struct { int view, dc, unlocks, locks, black, lines, fills;
                int32_t l[4][4]; int32_t f[2][5]; const char *name; } f3;
static int f3_view(void *c) { (void)c; return f3.view; }
static void f3_unlock(void *c, void *r, const char *n) { (void)c; (void)r; f3.name = n; ++f3.unlocks; }
static void f3_lock(void *c, void *r, const char *n) { (void)c; (void)r; assert(n == f3.name); ++f3.locks; }
static int f3_dc(void *c, void *r, const char *n) { (void)c; (void)r; (void)n; return f3.dc; }
static void f3_rel(void *c, void *r, const char *n) { (void)c; (void)r; (void)n; }
static void f3_begin(void *c, void *r, int black) { (void)c; (void)r; f3.black = black; }
static void f3_line(void *c, void *r, int32_t a, int32_t b, int32_t x, int32_t y) {
    (void)c; (void)r; f3.l[f3.lines][0] = a; f3.l[f3.lines][1] = b; f3.l[f3.lines][2] = x; f3.l[f3.lines][3] = y; ++f3.lines;
}
static void f3_end(void *c, void *r) { (void)c; (void)r; }
static void f3_proj(void *c, float x, float y, float z, int32_t bx, int32_t by, int16_t *ox, int16_t *oy) {
    (void)c; *ox = (int16_t)(x * 10 + bx); *oy = (int16_t)(y * 10 + by - z);
}
static void f3_fill(void *c, void *r, int32_t a, int32_t b, int32_t x, int32_t y, uint8_t col) {
    (void)c; (void)r; f3.f[f3.fills][0] = a; f3.f[f3.fills][1] = b; f3.f[f3.fills][2] = x; f3.f[f3.fills][3] = y; f3.f[f3.fills][4] = col; ++f3.fills;
}
static void frame_3d_tests(void) {
    DatUnit tmpl;
    AoeLiveUnitCore u;
    AoeLiveUnitFrame3dOps ops = {NULL, f3_view, f3_unlock, f3_lock, f3_dc, f3_rel,
                                 f3_begin, f3_line, f3_end, f3_proj, f3_fill};
    AoeLiveUnitFrame3dInput in;
    memset(&tmpl, 0, sizeof tmpl); memset(&u, 0, sizeof u); memset(&f3, 0, sizeof f3);
    tmpl.hit_points = 40; tmpl.size[2] = 3.0f;
    u.unit_template = &tmpl; u.unit_id = 7; u.draw_left = 1; u.draw_top = 2; u.hit_points = 10.9f;
    in = (AoeLiveUnitFrame3dInput){&u, 1.0f, 2.0f, 7, 1};
    /* no view: nothing */
    assert(!aoe_live_unit_draw_frame_back_ror(&in, NULL, 100, 200, &ops));
    assert(f3.unlocks == 0);
    f3.view = 1; f3.dc = 1;
    assert(aoe_live_unit_draw_frame_back_ror(&in, NULL, 100, 200, &ops));
    assert(f3.unlocks == 1 && f3.locks == 1 && f3.black == 1 && f3.lines == 2 && f3.fills == 0);
    assert(strcmp(f3.name, "stat_obj::draw_frame_3d_square_back") == 0);
    /* (1,-2)->(1,2), (1,-2)->(-1,-2) around (101, 202) */
    assert(f3.l[0][0] == 111 && f3.l[0][1] == 182 && f3.l[0][2] == 111 && f3.l[0][3] == 222);
    assert(f3.l[1][0] == 111 && f3.l[1][1] == 182 && f3.l[1][2] == 91 && f3.l[1][3] == 182);
    /* front: white pen when +0x78 differs; bar at (1,-2,3): (111, 179) */
    f3.lines = 0; in.unit_78 = 8;
    assert(aoe_live_unit_draw_frame_front_ror(&in, NULL, 100, 200, &ops));
    assert(f3.black == 0 && f3.lines == 2);
    assert(f3.l[0][0] == 91 && f3.l[0][1] == 182 && f3.l[0][2] == 91 && f3.l[0][3] == 222);
    assert(f3.l[1][0] == 111 && f3.l[1][1] == 222 && f3.l[1][2] == 91 && f3.l[1][3] == 222);
    assert(f3.fills == 2);
    assert(f3.f[0][0] == 99 && f3.f[0][1] == 177 && f3.f[0][2] == 123 && f3.f[0][3] == 178 && f3.f[0][4] == 0x97);
    assert(f3.f[1][0] == 99 && f3.f[1][2] == 10 * 24 / 40 - 12 + 111 && f3.f[1][4] == 0x4a);
    /* no device context: no lines, still relocked and the bar drawn */
    f3.dc = 0; f3.lines = 0; f3.fills = 0;
    assert(aoe_live_unit_draw_frame_front_ror(&in, NULL, 100, 200, &ops));
    assert(f3.lines == 0 && f3.fills == 2 && f3.locks == 3);
    /* HP below 1: no bar */
    u.hit_points = 0.5f; f3.fills = 0;
    assert(aoe_live_unit_draw_frame_front_ror(&in, NULL, 100, 200, &ops));
    assert(f3.fills == 0);
}

int main(void) {
    frame_3d_tests();
    test_selection();
    test_queue_selection_and_touch();
    test_draw_slots();
    AoeLiveUnitCore unit = {0}, controller = {0};
    AoeLiveUnitPlacement placement = {0};
    AoeObjectUpdateList tile = {0};
    AoeSpriteRuntime sprites = {0};
    Trace trace = {.sprite_ok = 1};
    AoeLiveUnitDrawRorInput input = {0};
    AoeLiveUnitDrawRorOps ops = {&trace, overlay, sprite};
    int drew = -1;
    unit.unit_id = 42;
    unit.state = 2;
    unit.class_code = 10;
    unit.scenario_frame = 6;
    unit.draw_left = -12;
    unit.draw_top = 7;
    unit.draw_right = 25;
    unit.draw_bottom = 31;
    controller.unit_id = 101;
    placement.tile = &tile;
    input.unit = &unit;
    input.placement = &placement;
    input.sprites = &sprites;
    input.graphic = (void *)0x5678;
    input.renderer = (void *)0x1234;
    input.tile_screen_x = 320;
    input.tile_screen_y = -40;
    input.owner_color = 4;
    assert(aoe_live_unit_draw_ror(&input, &ops, &drew));
    assert(drew == 1 && trace.count == 1 && trace.order[0] == 's');
    assert(trace.draw_id == 42 && trace.owner_color == 4);
    assert(trace.frame == 6);
    assert(trace.unit == &unit && trace.sprites == &sprites);
    assert(trace.graphic == (void *)0x5678);
    assert(trace.renderer == (void *)0x1234);
    assert(trace.left == 308 && trace.top == -33);
    assert(trace.right == 345 && trace.bottom == -9);

    memset(&trace, 0, sizeof(trace));
    trace.sprite_ok = 1;
    unit.action_flags_36 = 1;
    assert(aoe_live_unit_draw_ror(&input, &ops, &drew));
    assert(drew == 1 && trace.count == 2 &&
           trace.order[0] == 'o' && trace.order[1] == 's');
    assert(trace.renderer == (void *)0x1234 &&
           trace.tile_x == 320 && trace.tile_y == -40);
    ops.overlay = NULL;
    trace.count = 0;
    assert(!aoe_live_unit_draw_ror(&input, &ops, &drew));
    assert(trace.count == 0);
    ops.overlay = overlay;
    unit.action_flags_36 = 0;

    input.screen_mode = 3;
    trace.count = 0;
    assert(aoe_live_unit_draw_ror(&input, &ops, &drew));
    assert(trace.count == 2 && trace.order[0] == 'o');
    input.screen_mode = 0;
    input.template_draw_flag = 1;
    input.screen_submode = 7;
    trace.count = 0;
    assert(aoe_live_unit_draw_ror(&input, &ops, &drew));
    assert(trace.count == 2 && trace.order[0] == 'o');
    input.template_draw_flag = 0;
    input.screen_submode = 0;

    unit.class_code = 25;
    input.active_player = 3;
    input.type25_hidden_players = 1u << 3;
    trace.count = 0;
    assert(aoe_live_unit_draw_ror(&input, &ops, &drew));
    assert(drew == 0 && trace.count == 0);
    input.type25_hidden_players = 0;
    input.type25_controller = &controller;
    assert(aoe_live_unit_draw_ror(&input, &ops, &drew));
    assert(drew == 1 && trace.draw_id == 101);
    input.type25_controller = NULL;
    assert(aoe_live_unit_draw_ror(&input, &ops, &drew));
    assert(trace.draw_id == -1);

    unit.state = 7;
    trace.count = 0;
    assert(aoe_live_unit_draw_ror(&input, &ops, &drew));
    assert(drew == 0 && trace.count == 0);
    unit.state = 2;
    placement.tile = NULL;
    assert(aoe_live_unit_draw_ror(&input, &ops, &drew));
    assert(drew == 0 && trace.count == 0);
    placement.tile = &tile;
    input.active_player = 35;
    input.type25_hidden_players = 1u << 3;
    assert(aoe_live_unit_draw_ror(&input, &ops, &drew));
    assert(drew == 0);
    input.active_player = 0;
    input.type25_hidden_players = 0;
    ops.sprite = NULL;
    assert(!aoe_live_unit_draw_ror(&input, &ops, &drew));
    ops.sprite = sprite;
    input.tile_screen_x = 32767;
    unit.draw_left = 1;
    trace.count = 0;
    assert(aoe_live_unit_draw_ror(&input, &ops, &drew));
    assert(trace.count == 1 && trace.left == -32768);
    return 0;
}
