#include "live_unit_draw.h"

#include <stdint.h>

static int16_t add_word(int32_t anchor, int16_t offset) {
    uint16_t word = (uint16_t)((uint16_t)anchor + (uint16_t)offset);
    int32_t signed_value = word <= INT16_MAX ? (int32_t)word :
                           (int32_t)word - 65536;
    return (int16_t)signed_value;
}

int aoe_live_unit_draw_ror(const AoeLiveUnitDrawRorInput *input,
                            const AoeLiveUnitDrawRorOps *ops,
                            int *did_draw) {
    const AoeLiveUnitCore *unit;
    int32_t draw_id;
    int16_t left, top, right, bottom;
    int needs_overlay;
    if (!input || !input->unit || !input->placement || !ops) return 0;
    unit = input->unit;
    draw_id = unit->unit_id;
    if (unit->class_code == 25) {
        if (input->type25_hidden_players &
            (UINT32_C(1) << (input->active_player & 31))) {
            if (did_draw) *did_draw = 0;
            return 1;
        }
        draw_id = input->type25_controller ?
            input->type25_controller->unit_id : -1;
    }
    if (!input->placement->tile || unit->state >= 7) {
        if (did_draw) *did_draw = 0;
        return 1;
    }
    needs_overlay = unit->action_flags_36 || input->screen_mode == 3 ||
        ((input->template_draw_flag & 1) && input->screen_submode == 7);
    if (!input->sprites || !ops->sprite ||
        (needs_overlay && !ops->overlay)) return 0;
    left = add_word(input->tile_screen_x, unit->draw_left);
    top = add_word(input->tile_screen_y, unit->draw_top);
    right = add_word(input->tile_screen_x, unit->draw_right);
    bottom = add_word(input->tile_screen_y, unit->draw_bottom);
    if (needs_overlay)
        ops->overlay(ops->context, input->renderer, unit,
                     input->tile_screen_x, input->tile_screen_y);
    if (!ops->sprite(ops->context, input->renderer, unit, input->sprites,
                     input->graphic, input->owner_color,
                     unit->scenario_frame, draw_id,
                     left, top, right, bottom)) return 0;
    if (did_draw) *did_draw = 1;
    return 1;
}
