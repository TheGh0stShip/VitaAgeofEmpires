#include "live_unit_viewport.h"
#include "live_unit_sprite_backend.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    unsigned resolved;
    unsigned sprites;
    const void *graphics[2];
} Trace;

static int color_source(void *context, const void *graphic,
                        uint8_t fallback, uint8_t *color,
                        const uint8_t **table) {
    unsigned *calls = context;
    static const uint8_t selected_table[256] = {0};
    assert(graphic == (void *)(uintptr_t)0x1234 && fallback == 3);
    ++*calls;
    *color = 6;
    *table = selected_table;
    return 1;
}

static int resolve(void *context, const AoeLiveUnit *unit,
                   AoeLiveUnitViewportStateRor *state) {
    Trace *trace = context;
    assert(unit->constructed);
    ++trace->resolved;
    state->owner_color = (uint8_t)(unit->core.unit_id + 1);
    return 1;
}

static int sprite(void *context, void *renderer,
                  const AoeLiveUnitCore *unit,
                  const AoeSpriteRuntime *sprites, const void *graphic,
                  uint8_t owner_color, uint8_t frame, int32_t draw_id,
                  int16_t left, int16_t top, int16_t right, int16_t bottom) {
    Trace *trace = context;
    (void)sprites;
    assert(renderer == (void *)0x99);
    assert(owner_color == (uint8_t)(unit->unit_id + 1));
    assert(frame == (uint8_t)(6 + unit->unit_id));
    assert(draw_id == unit->unit_id);
    assert(left == 91 + unit->unit_id && top == 202 + unit->unit_id);
    assert(right == 103 + unit->unit_id && bottom == 204 + unit->unit_id);
    trace->graphics[trace->sprites++] = graphic;
    return 1;
}

int main(void) {
    AoeLiveUnit units[2];
    AoeObjectUpdateList tile = {0};
    AoeTileObjectDrawEntry entries[2];
    Trace trace = {0};
    AoeLiveUnitViewportResult result = {0};
    AoeLiveUnitDrawRorOps ops = {.context = &trace, .sprite = sprite};
    memset(units, 0, sizeof(units));
    memset(entries, 0, sizeof(entries));
    for (int i = 0; i < 2; ++i) {
        units[i].constructed = 1;
        units[i].core.unit_id = i;
        units[i].core.state = 2;
        units[i].core.class_code = 10;
        units[i].core.scenario_frame = (uint8_t)(6 + i);
        units[i].core.draw_left = (int16_t)(-9 + i);
        units[i].core.draw_top = (int16_t)(2 + i);
        units[i].core.draw_right = (int16_t)(3 + i);
        units[i].core.draw_bottom = (int16_t)(4 + i);
        units[i].placement.tile = &tile;
        entries[i].object = &units[i].core;
        entries[i].normal_graphic = (void *)(uintptr_t)(0x100 + i);
        entries[i].draw_class = 5;
    }
    assert(aoe_live_unit_viewport_draw_ror(
        entries, 2, 0, (void *)0x99, 100, 200,
        resolve, &trace, &ops, &result));
    assert(result.visited == 2 && result.drawn == 2 && result.failed == 0);
    assert(trace.resolved == 2 && trace.sprites == 2);
    assert(trace.graphics[0] == (void *)0x100 &&
           trace.graphics[1] == (void *)0x101);

    units[1].constructed = 0;
    memset(&trace, 0, sizeof(trace));
    assert(!aoe_live_unit_viewport_draw_ror(
        entries, 2, 0, (void *)0x99, 100, 200,
        resolve, &trace, &ops, &result));
    assert(result.visited == 2 && result.drawn == 1 && result.failed == 1);

    {
        uint8_t pixel = 0;
        unsigned color_calls = 0;
        DrsArchive *archives[1] = {(DrsArchive *)(uintptr_t)1};
        AoeSpriteIndexSurface surface = {&pixel, 1, 1, 1, 1};
        AoeLiveUnitIndexedBackend backend = {
            .archives = archives, .archive_count = 1, .last_status = -99,
            .resolve_color_source = color_source,
            .color_source_context = &color_calls
        };
        AoeSpriteRuntime empty = {0};
        assert(aoe_live_unit_sprite_indexed_ror(
            &backend, &surface, &units[0].core, &empty,
            (void *)(uintptr_t)0x1234, 3, 0, 0, 0, 0, 0, 0));
        assert(backend.last_status == AOE_GRAPHIC_ART_OK && color_calls == 1);
    }
    return 0;
}
