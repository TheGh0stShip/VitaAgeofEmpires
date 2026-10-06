#include "live_unit_draw.h"

#include <stdint.h>

#include "aoe_ftol.h"

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

int aoe_live_unit_draw_shadow_ror(const AoeLiveUnitCore *unit, int placed,
                                  uint32_t owner_color, void *renderer,
                                  int32_t x, int32_t y, int32_t extra,
                                  const AoeLiveUnitDrawSlotOps *ops) {
    if (!unit || !ops || !ops->shadow || !placed || unit->state >= 7)
        return 0;
    ops->shadow(ops->context, unit->scenario_frame,
                add_word(x, unit->draw_right), add_word(y, unit->draw_bottom),
                owner_color, renderer, extra);
    return 1;
}

int aoe_live_unit_draw_outline_ror(const AoeLiveUnitCore *unit, int placed,
                                   uint32_t owner_color, uint8_t screen_mode,
                                   void *renderer, int32_t x, int32_t y,
                                   const AoeLiveUnitDrawSlotOps *ops) {
    int bracket;
    if (!unit || !ops || !ops->outline || !placed || unit->state >= 7)
        return 0;
    bracket = unit->action_flags_36 || screen_mode == 3;
    if (bracket && ops->before) ops->before(ops->context, renderer, x, y);
    ops->outline(ops->context, unit->scenario_frame,
                 add_word(x, unit->draw_left), add_word(y, unit->draw_top),
                 owner_color, renderer);
    if (bracket && ops->after) ops->after(ops->context, renderer, x, y);
    return 1;
}

int aoe_live_unit_draw_frame_dispatch_ror(int32_t game_full_frame,
                                          uint8_t screen_mode, void *renderer,
                                          int32_t x, int32_t y,
                                          const AoeLiveUnitDrawSlotOps *ops) {
    if (!ops) return 0;
    if (game_full_frame == 1 || screen_mode == 1) {
        if (ops->frame_full) ops->frame_full(ops->context, renderer, x, y);
        return 3;
    }
    if (screen_mode == 0) {
        if (ops->frame_normal) ops->frame_normal(ops->context, renderer, x, y);
        return 1;
    }
    if (screen_mode == 2 || screen_mode == 3) {
        if (ops->frame_box) ops->frame_box(ops->context, renderer, x, y);
        return 2;
    }
    return 0;
}

int aoe_live_unit_sprite_bounds_ror(const void *graphic, uint8_t frame,
                                    const AoeLiveUnitSelectionOps *ops,
                                    int16_t *x0, int16_t *y0, int16_t *x1,
                                    int16_t *y1) {
    if (!graphic || !ops || !ops->bounds) return 0;
    return ops->bounds(ops->context, graphic, frame, x0, y0, x1, y1) ? 1 : 0;
}

int aoe_live_unit_draw_selection_ror(const AoeLiveUnitCore *unit,
                                     const void *graphic, void *renderer,
                                     int32_t x, int32_t y,
                                     const AoeLiveUnitSelectionOps *ops) {
    int16_t bx0 = 0, by0 = 0, bx1 = 0, by1 = 0;
    int32_t left, top, hp, max_hp;
    if (!unit || !graphic || !ops || !ops->rect || !ops->fill) return 0;
    if (ops->prepare) ops->prepare(ops->context, graphic, unit->scenario_frame);
    if (!aoe_live_unit_sprite_bounds_ror(graphic, unit->scenario_frame, ops,
                                         &bx0, &by0, &bx1, &by1))
        return 0;
    left = bx0 + x + unit->draw_left - 1;
    top = by0 + y + unit->draw_top - 1;
    ops->rect(ops->context, renderer, left, top,
              bx1 + x + unit->draw_left + 1, by1 + y + unit->draw_top + 1,
              0xff);
    if (unit->state > 2 || !unit->unit_template) return 1;
    hp = aoe_ftol32(unit->hit_points);
    if (hp < 1) hp = 0;
    max_hp = unit->unit_template->hit_points;
    if (max_hp <= 0) return 1;
    ops->rect(ops->context, renderer, left, top - 6, left + 26, top - 1, 0xff);
    ops->fill(ops->context, renderer, left + 1, top - 5, left + 24, top - 3,
              0x55);
    ops->fill(ops->context, renderer, left + 1, top - 5,
              hp * 25 / max_hp + left, top - 2, 0x25);
    return 1;
}

int aoe_live_unit_queue_selection_ror(const AoeLiveUnitCore *unit,
                                      const void *graphic, int32_t x,
                                      int32_t y,
                                      const AoeLiveUnitSelectionOps *ops) {
    int16_t bx0 = 0, by0 = 0, bx1 = 0, by1 = 0;
    int32_t left, top, right, bottom, hp, max_hp, fill;
    if (!unit || !graphic || !ops || !ops->add_node) return 0;
    if (ops->prepare) ops->prepare(ops->context, graphic, unit->scenario_frame);
    if (!aoe_live_unit_sprite_bounds_ror(graphic, unit->scenario_frame, ops,
                                         &bx0, &by0, &bx1, &by1))
        return 0;
    left = bx0 + x + unit->draw_left - 1;
    top = by0 + y + unit->draw_top - 1;
    right = bx1 + x + unit->draw_left + 1;
    bottom = by1 + y + unit->draw_top + 1;
    {
        const int16_t box[8] = {(int16_t)left, (int16_t)top, (int16_t)right,
                                (int16_t)bottom, 0, 0, 0, 0};
        ops->add_node(ops->context, 4, bottom, box, 10, 0xff);
    }
    if (unit->state > 2 || !unit->unit_template) return 1;
    hp = aoe_ftol32(unit->hit_points);
    if (hp < 1) hp = 0;
    max_hp = unit->unit_template->hit_points;
    if (max_hp <= 0) return 1;
    fill = hp * 25 / max_hp + left;
    if (fill >= left + 26) fill = left + 25;
    {
        const int16_t bar[8] = {(int16_t)left, (int16_t)(top - 6),
                                (int16_t)(left + 26), (int16_t)(top - 1),
                                (int16_t)fill, 0, (int16_t)(fill + 1), 0};
        ops->add_node(ops->context, 2, bottom, bar, 0x14, 0xff);
    }
    return 1;
}

static int in_range(int16_t v, int16_t lo, int16_t hi) {
    return lo <= v && v <= hi;
}
static int strictly(int16_t v, int16_t lo, int16_t hi) {
    return lo < v && v < hi;
}

int aoe_live_unit_box_touches_ror(const AoeLiveUnitCore *unit,
                                  const void *graphic, int32_t x, int32_t y,
                                  int16_t l, int16_t t, int16_t r, int16_t b,
                                  const AoeLiveUnitSelectionOps *ops) {
    int16_t bx0 = 0, by0 = 0, bx1 = 0, by1 = 0;
    int16_t left, top, right, bottom;
    if (!unit || !graphic || !ops) return 0;
    if (ops->prepare) ops->prepare(ops->context, graphic, unit->scenario_frame);
    if (!aoe_live_unit_sprite_bounds_ror(graphic, unit->scenario_frame, ops,
                                         &bx0, &by0, &bx1, &by1))
        return 0;
    /* The original sums in 16-bit registers. */
    left = (int16_t)(uint16_t)((uint32_t)bx0 + (uint32_t)x + (uint32_t)unit->draw_left);
    top = (int16_t)(uint16_t)((uint32_t)by0 + (uint32_t)y + (uint32_t)unit->draw_top);
    right = (int16_t)(uint16_t)((uint32_t)bx1 + (uint32_t)x + (uint32_t)unit->draw_left);
    bottom = (int16_t)(uint16_t)((uint32_t)by1 + (uint32_t)y + (uint32_t)unit->draw_top);
    if (in_range(left, l, r) && in_range(top, t, b)) return 1;
    if (in_range(right, l, r) && in_range(bottom, t, b)) return 1;
    if (in_range(l, left, right) && in_range(t, top, bottom)) return 1;
    if (in_range(r, left, right) && in_range(b, top, bottom)) return 1;
    if (l < left && r > right &&
        (strictly(t, top, bottom) || strictly(b, top, bottom))) return 1;
    if (t < top && b > bottom &&
        (strictly(l, left, right) || strictly(r, left, right))) return 1;
    return 0;
}

static const char frame_back_name[] = "stat_obj::draw_frame_3d_square_back";
static const char frame_front_name[] = "stat_obj::draw_frame_3d_square_front";

static int frame_3d(const AoeLiveUnitFrame3dInput *in, void *renderer,
                    int32_t x, int32_t y, const AoeLiveUnitFrame3dOps *ops,
                    int front) {
    const char *name = front ? frame_front_name : frame_back_name;
    const AoeLiveUnitCore *u;
    float ox, oy;
    int16_t ax, ay, bx, by;
    int i;
    if (!in || !in->unit || !ops || !ops->view || !ops->project ||
        !ops->line)
        return 0;
    if (!ops->view(ops->context)) return 0;
    u = in->unit;
    ox = in->outline_x;
    oy = in->outline_y;
    x += u->draw_left;
    y += u->draw_top;
    if (ops->unlock) ops->unlock(ops->context, renderer, name);
    if (!ops->get_dc || ops->get_dc(ops->context, renderer, name)) {
        /* two edges, each (from, to) in tile units */
        const float back[2][4] = {{ox, -oy, ox, oy}, {ox, -oy, -ox, -oy}};
        const float fore[2][4] = {{-ox, -oy, -ox, oy}, {ox, oy, -ox, oy}};
        const float (*e)[4] = front ? fore : back;
        if (ops->begin)
            ops->begin(ops->context, renderer,
                       in->logging_on == 1 && in->unit_78 == u->unit_id &&
                           u->unit_id >= 0);
        for (i = 0; i < 2; ++i) {
            ops->project(ops->context, e[i][0], e[i][1], 0.0f, x, y, &ax, &ay);
            ops->project(ops->context, e[i][2], e[i][3], 0.0f, x, y, &bx, &by);
            ops->line(ops->context, renderer, ax, ay, bx, by);
        }
        if (ops->end) ops->end(ops->context, renderer);
        if (ops->release_dc) ops->release_dc(ops->context, renderer, name);
    }
    if (ops->lock) ops->lock(ops->context, renderer, name);
    if (front && ops->fill) {
        int32_t hp = aoe_ftol32(u->hit_points), max_hp;
        if (hp > 0 && u->unit_template) {
            ops->project(ops->context, ox, -oy, u->unit_template->size[2], x,
                         y, &ax, &ay);
            max_hp = u->unit_template->hit_points;
            if (max_hp > 0) {
                ops->fill(ops->context, renderer, ax - 12, ay - 2, ax + 12,
                          ay - 1, 0x97);
                ops->fill(ops->context, renderer, ax - 12, ay - 2,
                          hp * 24 / max_hp - 12 + ax, ay - 1, 0x4a);
            }
        }
    }
    return 1;
}

int aoe_live_unit_draw_frame_back_ror(const AoeLiveUnitFrame3dInput *in,
                                      void *renderer, int32_t x, int32_t y,
                                      const AoeLiveUnitFrame3dOps *ops) {
    return frame_3d(in, renderer, x, y, ops, 0);
}

int aoe_live_unit_draw_frame_front_ror(const AoeLiveUnitFrame3dInput *in,
                                       void *renderer, int32_t x, int32_t y,
                                       const AoeLiveUnitFrame3dOps *ops) {
    return frame_3d(in, renderer, x, y, ops, 1);
}
