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

int main(void) {
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
