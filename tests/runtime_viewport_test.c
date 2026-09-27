#include "runtime_viewport.h"

#include <assert.h>
#include <string.h>

typedef struct {
    size_t resolved, sprites;
    int32_t x, y;
} Trace;

static int resolve_unit(void *opaque, const AoeLiveUnit *unit,
                        AoeLiveUnitViewportStateRor *state) {
    Trace *trace = opaque;
    assert(unit->constructed);
    memset(state, 0, sizeof(*state));
    state->owner_color = 2;
    ++trace->resolved;
    return 1;
}

static int draw_sprite(void *opaque, void *renderer,
                       const AoeLiveUnitCore *unit,
                       const AoeSpriteRuntime *sprites, const void *graphic,
                       uint8_t owner_color, uint8_t frame, int32_t draw_id,
                       int16_t left, int16_t top, int16_t right,
                       int16_t bottom) {
    Trace *trace = opaque;
    (void)renderer;
    (void)sprites;
    (void)graphic;
    (void)owner_color;
    (void)frame;
    (void)draw_id;
    (void)right;
    (void)bottom;
    trace->x = left - unit->draw_left;
    trace->y = top - unit->draw_top;
    ++trace->sprites;
    return 1;
}

int main(void) {
    AoeRuntimeMap map = {.width = 1, .height = 1};
    AoeObjectUpdateList list = {0};
    AoeLiveUnit unit = {0};
    DatUnit unit_template = {.draw_class = 5};
    uint8_t view_code = 0;
    AoeTileObjectDrawEntry scratch[1];
    Trace trace = {0};
    AoeLiveUnitDrawRorOps ops = {.context = &trace, .sprite = draw_sprite};
    AoeRuntimeViewportRor viewport = {
        .map = &map,
        .scratch = scratch,
        .scratch_capacity = 1,
        .renderer = (void *)1,
        .resolve_unit = resolve_unit,
        .resolve_unit_context = &trace,
        .draw_ops = &ops
    };
    map.objects = &list;
    map.view_codes = &view_code;
    unit.constructed = 1;
    unit.core.unit_template = &unit_template;
    unit.core.state = 2;
    unit.core.class_code = 10;
    unit.core.draw_left = -2;
    unit.core.draw_top = 3;
    unit.core.draw_right = 4;
    unit.core.draw_bottom = 5;
    unit.graphic.current_graphic = (void *)2;
    unit.placement.tile = &list;
    assert(aoe_object_update_list_attach(&list, &unit.core,
                                         &unit.placement.node));
    assert(aoe_runtime_viewport_draw_tile_ror(
        &viewport, 0, 0, 123, 234, TERRAIN_VIEW_VISIBLE));
    assert(viewport.tiles == 1 && viewport.nonempty_tiles == 1);
    assert(viewport.result.visited == 1 && viewport.result.drawn == 1 &&
           viewport.result.failed == 0);
    assert(trace.resolved == 1 && trace.sprites == 1 &&
           trace.x == 123 && trace.y == 234);

    /* Tile +9 value 0x80 selects the alternate pass; class zero is omitted. */
    unit_template.draw_class = 0;
    view_code = 0x80;
    memset(&viewport.result, 0, sizeof(viewport.result));
    trace = (Trace){0};
    assert(aoe_runtime_viewport_draw_tile_ror(
        &viewport, 0, 0, 123, 234, TERRAIN_VIEW_EXPLORED_FOG));
    assert(viewport.result.visited == 0 && viewport.result.drawn == 0 &&
           trace.sprites == 0);

    viewport.scratch_capacity = 0;
    assert(!aoe_runtime_viewport_draw_tile_ror(
        &viewport, 0, 0, 123, 234, TERRAIN_VIEW_VISIBLE));
    aoe_object_update_list_clear(&list);
    return 0;
}
