#include "live_unit_viewport.h"

#include <string.h>

typedef struct {
    AoeLiveUnitViewportResolveRor resolve;
    void *resolve_context;
    const AoeLiveUnitDrawRorOps *ops;
    AoeLiveUnitViewportResult result;
} DrawContext;

static void draw_one(void *object, const void *graphic, void *renderer,
                     int32_t screen_x, int32_t screen_y, void *opaque) {
    DrawContext *draw = opaque;
    AoeLiveUnit *unit = object;
    AoeLiveUnitViewportStateRor state;
    AoeLiveUnitDrawRorInput input;
    int did_draw = 0;
    ++draw->result.visited;
    memset(&state, 0, sizeof(state));
    if (!unit || !unit->constructed ||
        !draw->resolve(draw->resolve_context, unit, &state)) {
        ++draw->result.failed;
        return;
    }
    memset(&input, 0, sizeof(input));
    input.unit = &unit->core;
    input.placement = &unit->placement;
    input.sprites = &unit->sprites;
    input.graphic = graphic;
    input.type25_controller = state.type25_controller;
    input.renderer = renderer;
    input.tile_screen_x = screen_x;
    input.tile_screen_y = screen_y;
    input.type25_hidden_players = state.type25_hidden_players;
    input.active_player = state.active_player;
    input.owner_color = state.owner_color;
    input.template_draw_flag = state.template_draw_flag;
    input.screen_mode = state.screen_mode;
    input.screen_submode = state.screen_submode;
    if (!aoe_live_unit_draw_ror(&input, draw->ops, &did_draw)) {
        ++draw->result.failed;
        return;
    }
    if (did_draw) ++draw->result.drawn;
}

int aoe_live_unit_viewport_draw_ror(
    const AoeTileObjectDrawEntry *entries, size_t count, int alternate_pass,
    void *renderer, int32_t screen_x, int32_t screen_y,
    AoeLiveUnitViewportResolveRor resolve, void *resolve_context,
    const AoeLiveUnitDrawRorOps *ops, AoeLiveUnitViewportResult *result) {
    DrawContext draw;
    size_t visited;
    if ((!entries && count) || !resolve || !ops || !ops->sprite) return 0;
    memset(&draw, 0, sizeof(draw));
    draw.resolve = resolve;
    draw.resolve_context = resolve_context;
    draw.ops = ops;
    visited = aoe_tile_object_visit_draw_at(
        entries, count, alternate_pass, renderer, screen_x, screen_y,
        draw_one, &draw);
    if (visited != draw.result.visited) ++draw.result.failed;
    if (result) *result = draw.result;
    return draw.result.failed == 0;
}
