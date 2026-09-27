#include "scenario_editor_catalog.h"

#include <math.h>

static float abs_float(float value) {
    return value < 0.0f ? -value : value;
}

static float short_fraction(float value) {
    return value - (float)(int16_t)value;
}

int aoe_scenario_editor_adjust_position_ror(
    const DatUnit *unit, uint32_t map_width, uint32_t map_height,
    int snap_to_grid, float *x, float *y) {
    const float edge_epsilon = 0.00001f; /* release bits 0x3727c5ac */
    float left, bottom, right, top;
    float left_fraction, bottom_fraction, right_complement, top_complement;
    if (!unit || !x || !y || !map_width || !map_height ||
        map_width > INT16_MAX || map_height > INT16_MAX ||
        !isfinite(*x) || !isfinite(*y) ||
        !isfinite(unit->size[0]) || !isfinite(unit->size[1]) ||
        unit->size[0] < 0.0f || unit->size[1] < 0.0f)
        return 0;

    left = *x - unit->size[0];
    bottom = *y - unit->size[1];
    right = *x + unit->size[0];
    top = *y + unit->size[1];
    if (left < 0.0f) {
        *x += -left;
        right += -left;
        left = 0.0f;
    }
    if (bottom < 0.0f) {
        *y += -bottom;
        top += -bottom;
        bottom = 0.0f;
    }
    if ((float)map_width - edge_epsilon <= right) {
        float delta = ((float)map_width - edge_epsilon) - right;
        *x += delta;
        left += delta;
        right += delta;
    }
    if ((float)map_height - edge_epsilon <= top) {
        float delta = ((float)map_height - edge_epsilon) - top;
        *y += delta;
        bottom += delta;
        top += delta;
    }
    if (!snap_to_grid) return 1;

    left = abs_float(left);
    bottom = abs_float(bottom);
    right = abs_float(right);
    top = abs_float(top);
    left_fraction = short_fraction(left);
    bottom_fraction = short_fraction(bottom);
    right_complement = 1.0f - short_fraction(right);
    top_complement = 1.0f - short_fraction(top);
    if (right_complement + left_fraction >= 1.0f) {
        if (left_fraction > right_complement)
            *x += 1.0f - left_fraction;
        else
            *x -= 1.0f - right_complement;
    }
    if (top_complement + bottom_fraction >= 1.0f) {
        if (bottom_fraction > top_complement)
            *y += 1.0f - bottom_fraction;
        else
            *y -= 1.0f - top_complement;
    }
    return 1;
}

static int editor_visible(const DatUnit *unit) {
    return unit && !unit->hide_in_editor;
}

size_t aoe_scenario_editor_unit_count(const AoePlayerTemplates *templates) {
    size_t count = 0;
    if (!templates || (templates->count && !templates->slots)) return 0;
    for (uint16_t id = 0; id < templates->count; ++id)
        if (editor_visible(templates->slots[id])) ++count;
    return count;
}

int aoe_scenario_editor_unit_at(const AoePlayerTemplates *templates,
                                size_t visible_index,
                                AoeScenarioEditorUnitEntry *entry) {
    if (!templates || !entry || (templates->count && !templates->slots))
        return 0;
    for (uint16_t id = 0; id < templates->count; ++id) {
        DatUnit *unit = templates->slots[id];
        if (!editor_visible(unit)) continue;
        if (visible_index--) continue;
        *entry = (AoeScenarioEditorUnitEntry){id, unit};
        return 1;
    }
    return 0;
}

int aoe_scenario_editor_unit_select(const AoePlayerTemplates *templates,
                                    uint16_t object_id,
                                    AoeScenarioEditorUnitEntry *entry) {
    DatUnit *unit;
    if (!templates || !entry) return 0;
    unit = aoe_player_templates_get(templates, object_id);
    if (!editor_visible(unit)) return 0;
    *entry = (AoeScenarioEditorUnitEntry){object_id, unit};
    return 1;
}

int aoe_scenario_editor_place_ror(
    const AoePlayerTemplates *templates, uint16_t object_id,
    int editor_mode, int editor_submode, float map_x, float map_y,
    const AoeScenarioEditorPlacementOps *ops, void **out_unit) {
    DatUnit *selected;
    void *created;
    int placement_variant;
    if (out_unit) *out_unit = 0;
    if (!ops || !ops->adjust_position || !ops->placement_blocked ||
        !ops->created_nonzero_layer || !ops->set_redraw_status ||
        editor_mode != 8)
        return 0;
    /* The UI catalog prevents choosing +0x4e-hidden templates, but the click
       routine itself only validates ID and a non-null player-template slot. */
    selected = aoe_player_templates_get(templates, object_id);
    if (!selected) return 0;
    placement_variant = editor_submode == 3 ? 0 : 1;
    if (!ops->adjust_position(ops->context, selected, &map_x, &map_y,
                              placement_variant))
        return 0;
    if (ops->placement_blocked(ops->context, selected, map_x, map_y,
                               selected->placement_class))
        return 0;
    if (editor_submode == 1) {
        if (!ops->relocate_selected || !ops->selected_unit ||
            !ops->relocate_selected(ops->context, ops->selected_unit,
                                    map_x, map_y, 0.0f))
            return 0;
        created = ops->selected_unit;
    } else {
        if (!ops->create) return 0;
        created = ops->create(ops->context, object_id, map_x, map_y, 0.0f, 1);
    }
    if (!created) return 0;
    ops->set_redraw_status(ops->context,
        ops->created_nonzero_layer(ops->context, created) ? 1 : 2);
    if (out_unit) *out_unit = created;
    return 1;
}
