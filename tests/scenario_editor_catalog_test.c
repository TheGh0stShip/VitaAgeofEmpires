#include "scenario_editor_catalog.h"

#include <assert.h>

typedef struct {
    const DatUnit *adjusted;
    float x, y, z;
    uint16_t object_id;
    uint8_t placement_class;
    int variant, blocked, normal_calls, special_calls, nonzero_layer, redraw;
} PlacementTrace;

static int adjust(void *opaque, const DatUnit *unit, float *x, float *y,
                  int variant) {
    PlacementTrace *trace = opaque;
    trace->adjusted = unit;
    trace->variant = variant;
    *x += 0.25f;
    *y += 0.5f;
    return 1;
}

static int blocked(void *opaque, const DatUnit *unit, float x, float y,
                   uint8_t placement_class) {
    PlacementTrace *trace = opaque;
    assert(unit == trace->adjusted);
    trace->x = x;
    trace->y = y;
    trace->placement_class = placement_class;
    return trace->blocked;
}

static void *create(void *opaque, uint16_t object_id, float x, float y,
                    float z, int editor_create) {
    PlacementTrace *trace = opaque;
    assert(editor_create == 1 && x == trace->x && y == trace->y);
    trace->object_id = object_id;
    trace->z = z;
    ++trace->normal_calls;
    return trace;
}

static int relocate(void *opaque, void *selected, float x, float y, float z) {
    PlacementTrace *trace = opaque;
    assert(selected == trace);
    assert(x == trace->x && y == trace->y);
    trace->z = z;
    ++trace->special_calls;
    return 1;
}

static int nonzero_layer(void *opaque, void *unit) {
    PlacementTrace *trace = opaque;
    assert(unit == trace);
    return trace->nonzero_layer;
}

static void redraw(void *opaque, int status) {
    PlacementTrace *trace = opaque;
    trace->redraw = status;
}

int main(void) {
    DatUnit visible0 = {.present = 1, .id = 0, .type = 10};
    DatUnit hidden = {.present = 1, .id = 2, .type = 80,
                      .hide_in_editor = 1};
    DatUnit visible3 = {.present = 1, .id = 3, .type = 30,
                        .placement_class = 7};
    DatUnit *slots[] = {&visible0, 0, &hidden, &visible3};
    AoePlayerTemplates templates = {4, slots};
    AoeScenarioEditorUnitEntry entry = {0};

    assert(aoe_scenario_editor_unit_count(&templates) == 2);
    assert(aoe_scenario_editor_unit_at(&templates, 0, &entry));
    assert(entry.object_id == 0 && entry.unit == &visible0);
    assert(aoe_scenario_editor_unit_at(&templates, 1, &entry));
    assert(entry.object_id == 3 && entry.unit == &visible3);
    assert(!aoe_scenario_editor_unit_at(&templates, 2, &entry));
    assert(!aoe_scenario_editor_unit_select(&templates, 2, &entry));
    assert(aoe_scenario_editor_unit_select(&templates, 3, &entry));
    assert(entry.object_id == 3 && entry.unit == &visible3);

    PlacementTrace trace = {.nonzero_layer = 1};
    AoeScenarioEditorPlacementOps ops = {
        adjust, blocked, create, relocate, &trace,
        nonzero_layer, redraw, &trace};
    void *created = 0;
    assert(aoe_scenario_editor_place_ror(&templates, 3, 8, 0,
                                         10.0f, 20.0f,
                                         &ops, &created));
    assert(created == &trace && trace.adjusted == &visible3 &&
           trace.variant == 1 && trace.x == 10.25f && trace.y == 20.5f &&
           trace.z == 0.0f && trace.object_id == 3 &&
           trace.placement_class == 7 && trace.normal_calls == 1 &&
           trace.special_calls == 0 && trace.redraw == 1);

    trace.nonzero_layer = 0;
    assert(aoe_scenario_editor_place_ror(&templates, 3, 8, 1,
                                         1.0f, 2.0f,
                                         &ops, &created));
    assert(trace.special_calls == 1 && trace.normal_calls == 1 &&
           trace.redraw == 2);
    trace.blocked = 1;
    assert(!aoe_scenario_editor_place_ror(&templates, 3, 8, 3,
                                          5.0f, 6.0f,
                                          &ops, &created));
    assert(created == 0 && trace.variant == 0 && trace.normal_calls == 1);
    trace.blocked = 0;
    /* Hidden templates cannot be selected through the catalog, but the
       original click handler itself only requires a non-null template. */
    assert(aoe_scenario_editor_place_ror(&templates, 2, 8, 0,
                                         0, 0, &ops, &created));
    assert(trace.object_id == 2);
    assert(!aoe_scenario_editor_place_ror(&templates, 1, 8, 0,
                                          0, 0, &ops, &created));
    assert(!aoe_scenario_editor_place_ror(&templates, 3, 7, 0,
                                          0, 0, &ops, &created));

    visible3.size[0] = 0.5f;
    visible3.size[1] = 1.0f;
    float x = -3.0f, y = -2.0f;
    assert(aoe_scenario_editor_adjust_position_ror(
        &visible3, 20, 10, 0, &x, &y));
    assert(x == 0.5f && y == 1.0f);
    x = 30.0f;
    y = 30.0f;
    assert(aoe_scenario_editor_adjust_position_ror(
        &visible3, 20, 10, 0, &x, &y));
    assert(x < 19.5f && x > 19.49f && y < 9.0f && y > 8.99f);

    /* Half-tile footprints already align and remain unchanged. */
    x = 7.5f;
    y = 6.0f;
    assert(aoe_scenario_editor_adjust_position_ror(
        &visible3, 20, 10, 1, &x, &y));
    assert(x == 7.5f && y == 6.0f);
    /* An off-grid center moves to the nearer footprint-edge alignment. */
    x = 7.7f;
    y = 6.2f;
    assert(aoe_scenario_editor_adjust_position_ror(
        &visible3, 20, 10, 1, &x, &y));
    assert(x == 7.5f && y == 6.0f);
    assert(!aoe_scenario_editor_adjust_position_ror(
        &visible3, 40000, 10, 0, &x, &y));
    return 0;
}
