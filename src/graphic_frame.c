#include "graphic_frame.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

int aoe_graphic_select_frame(const DatGraphic *graphic,
                             uint16_t angle, int16_t animation_frame,
                             AoeGraphicFrame *out) {
    if (!graphic || !graphic->present || !out ||
        !graphic->frame_count || !graphic->angle_count ||
        angle >= graphic->angle_count)
        return 0;
    int32_t frame = animation_frame;
    if (frame < 0 || frame >= graphic->frame_count)
        frame = 0;
    AoeGraphicFrame selected = {0};
    uint16_t count = graphic->angle_count;
    uint8_t mirror = graphic->mirror_mode;
    int64_t index;
    if (mirror && count == 2) {
        selected.slp_frame = (uint32_t)frame;
        selected.mirror_horizontal = (uint8_t)(angle & 0xff);
        *out = selected;
        return 1;
    }
    if (mirror && (mirror < angle || angle < count / 4)) {
        int32_t half = count / 2;
        int32_t mirrored_angle = half < angle ? half - angle + count
                                               : half - angle;
        index = (int64_t)(mirrored_angle - count / 4) *
                graphic->frame_count + frame;
        selected.mirror_horizontal = 1;
    } else if (!mirror) {
        index = (int64_t)graphic->frame_count * angle + frame;
    } else {
        index = (int64_t)((int32_t)angle - count / 4) *
                graphic->frame_count + frame;
    }
    if (index < 0 || index > UINT32_MAX)
        return 0;
    selected.slp_frame = (uint32_t)index;
    *out = selected;
    return 1;
}

int aoe_graphic_frame_sounds(const DatGraphic *graphic, int16_t from,
                             int16_t to, int16_t angle,
                             void (*play)(void *context, int16_t sound_id),
                             void *context) {
    int played = 0;
    size_t i;
    if (!graphic || !play || from == to) return 0;
    if (angle >= (int32_t)graphic->angle_count) angle = 0;
    if (graphic->sound_id >= 0 && (from == 0 || (to < from && to != 0))) {
        play(context, graphic->sound_id);
        ++played;
    }
    if (!graphic->attack_sounds || graphic->attack_sound_count == 0 ||
        angle < 0)
        return played;
    for (i = 0; i < 3; ++i) {
        size_t index = (size_t)angle * 3 + i;
        const DatGraphicAttackSound *pair;
        int hit;
        if (index >= graphic->attack_sound_count) break;
        pair = &graphic->attack_sounds[index];
        if (pair->delay < 0) break;
        hit = from < to ? (pair->delay >= from && pair->delay < to)
                        : (pair->delay >= from || pair->delay < to);
        if (hit) {
            play(context, pair->sound_id);
            ++played;
        }
    }
    return played;
}

static int32_t rescale_angle(const DatGraphic *part, const DatGraphic *own,
                             int32_t angle) {
    if (own->angle_count == 0) return 0; /* host guard: original divides */
    return (int32_t)part->angle_count * angle / (int32_t)own->angle_count;
}

int aoe_graphic_hit_test_ror(const DatGraphic *graphic, int32_t x, int32_t y,
                             int32_t angle, int32_t frame,
                             AoeGraphicPixelTestFn pixel, void *context) {
    const int16_t *c;
    uint16_t i;
    if (!graphic || !pixel) return 0;
    c = graphic->coordinates;
    if ((c[0] != 0 || c[2] != 0) && (c[1] != 0 || c[3] != 0) &&
        c[0] <= x && x <= c[2] && c[1] <= y && y <= c[3])
        return 1;
    if (graphic->delta_count > 0 && graphic->deltas) {
        for (i = 0; i < graphic->delta_count; ++i) {
            const DatGraphicDelta *d = &graphic->deltas[i];
            if (!d->graphic) continue;
            if (pixel(context, d->graphic, (int16_t)(x - d->offset_x),
                      (int16_t)(y - d->offset_y),
                      rescale_angle(d->graphic, graphic, angle), frame))
                return 1;
        }
        return 0;
    }
    return pixel(context, graphic, x, y, angle, frame) ? 1 : 0;
}

int aoe_graphic_hit_test_near_ror(const DatGraphic *graphic, int16_t x,
                                  int16_t y, int16_t tolerance,
                                  int32_t angle, int32_t frame,
                                  AoeGraphicPixelTestFn pixel, void *context) {
    int32_t dx, dy;
    if (!graphic) return 0;
    if (aoe_graphic_hit_test_ror(graphic, x, y, angle, frame, pixel, context))
        return 1;
    if (tolerance == 0) return 0;
    for (dx = -tolerance; dx < tolerance; ++dx)
        for (dy = -tolerance; dy < tolerance; ++dy)
            if (aoe_graphic_hit_test_ror(graphic, (int16_t)(x + dx),
                                         (int16_t)(y + dy), angle, frame,
                                         pixel, context))
                return 1;
    return 0;
}

static int draw_parts(const DatGraphic *graphic, int32_t angle, int32_t x,
                      int32_t y, int lower, uint8_t skip_layer,
                      void (*draw)(void *context, const DatGraphic *graphic,
                                   int32_t angle, int32_t x, int32_t y),
                      void *context) {
    int drawn = 0;
    uint16_t i;
    if (!graphic || !draw) return 0;
    if (angle >= (int32_t)graphic->angle_count) angle = 0;
#define PART_OK(layer) (lower ? ((layer) != skip_layer && (layer) < 20) : (layer) == 20)
    if (graphic->delta_count > 0 && graphic->deltas) {
        for (i = 0; i < graphic->delta_count; ++i) {
            const DatGraphicDelta *d = &graphic->deltas[i];
            if (!d->graphic || !PART_OK(d->graphic->layer)) continue;
            if (d->display_angle >= 0 && d->display_angle != angle) continue;
            draw(context, d->graphic, rescale_angle(d->graphic, graphic, angle),
                 x + d->offset_x, y + d->offset_y);
            ++drawn;
        }
        return drawn;
    }
    if (PART_OK(graphic->layer)) {
        draw(context, graphic, angle, x, y);
        ++drawn;
    }
#undef PART_OK
    return drawn;
}

int aoe_graphic_draw_layer20_ror(const DatGraphic *graphic, int32_t angle,
                                 int32_t x, int32_t y,
                                 void (*draw)(void *context,
                                              const DatGraphic *graphic,
                                              int32_t angle, int32_t x,
                                              int32_t y),
                                 void *context) {
    return draw_parts(graphic, angle, x, y, 0, 0, draw, context);
}

int aoe_graphic_draw_lower_layers_ror(const DatGraphic *graphic,
                                      int32_t angle, int32_t x, int32_t y,
                                      uint8_t skip_layer,
                                      void (*draw)(void *context,
                                                   const DatGraphic *graphic,
                                                   int32_t angle, int32_t x,
                                                   int32_t y),
                                      void *context) {
    return draw_parts(graphic, angle, x, y, 1, skip_layer, draw, context);
}

int aoe_graphic_union_part_bounds_ror(const DatGraphic *graphic,
                                      int32_t angle, int32_t frame,
                                      int16_t dx, int16_t dy,
                                      AoeGraphicFrameBoundsFn bounds,
                                      void *context, int16_t *x0, int16_t *y0,
                                      int16_t *x1, int16_t *y1) {
    AoeGraphicFrame selected = {0, 0};
    int32_t bx0, by0, bx1, by1;
    if (!graphic || !bounds || !x0 || !y0 || !x1 || !y1) return 0;
    if (frame >= (int32_t)graphic->frame_count) frame = 0;
    if (!aoe_graphic_select_frame(graphic, (uint16_t)angle, (int16_t)frame,
                                  &selected))
        return 0;
    if (!bounds(context, graphic, selected.slp_frame, &bx0, &by0, &bx1, &by1))
        return 0;
    if (selected.mirror_horizontal) {
        int32_t t = -bx0;
        bx0 = -bx1;
        bx1 = t;
    }
    bx0 += dx; bx1 += dx; by0 += dy; by1 += dy;
    if (bx0 < *x0) *x0 = (int16_t)bx0;
    if (by0 < *y0) *y0 = (int16_t)by0;
    if (*x1 < bx1) *x1 = (int16_t)bx1;
    if (*y1 < by1) *y1 = (int16_t)by1;
    return 1;
}

int aoe_graphic_bounds_ror(const DatGraphic *graphic, int32_t angle,
                           int32_t frame, AoeGraphicFrameBoundsFn bounds,
                           void *context, int16_t *x0, int16_t *y0,
                           int16_t *x1, int16_t *y1) {
    const int16_t *c;
    uint16_t i;
    if (!graphic || !x0 || !y0 || !x1 || !y1) return 0;
    c = graphic->coordinates;
    if ((c[0] != 0 || c[2] != 0) && (c[1] != 0 || c[3] != 0)) {
        *x0 = c[0]; *y0 = c[1]; *x1 = c[2]; *y1 = c[3];
        return 1;
    }
    *x0 = *y0 = *x1 = *y1 = 0;
    if (graphic->delta_count > 0 && graphic->deltas) {
        for (i = 0; i < graphic->delta_count; ++i) {
            const DatGraphicDelta *d = &graphic->deltas[i];
            if (!graphic->selection_mode || !d->graphic) continue;
            aoe_graphic_union_part_bounds_ror(
                d->graphic, rescale_angle(d->graphic, graphic, angle), frame,
                d->offset_x, d->offset_y, bounds, context, x0, y0, x1, y1);
        }
    } else {
        aoe_graphic_union_part_bounds_ror(graphic, angle, frame, 0, 0, bounds,
                                          context, x0, y0, x1, y1);
    }
    if ((*x0 == 0 && *x1 == 0) || (*y0 == 0 && *y1 == 0)) return 0;
    return 1;
}

int aoe_graphic_shape_ensure_ror(const DatGraphic *graphic,
                                 const AoeGraphicShapeOpsRor *ops,
                                 AoeShapeRor **out) {
    AoeGraphicShapeRor *slot;
    char path[sizeof graphic->filename + 8];
    uint32_t start;
    if (out) *out = NULL;
    if (!graphic || !ops || !ops->slot) return 0;
    slot = ops->slot(ops->context, graphic);
    if (!slot) return 0;
    if (!slot->loaded) {
        if (!graphic->slp_loadable || !ops->shape_ops) return 0;
        snprintf(path, sizeof path, "%.*s.shp",
                 (int)strnlen(graphic->filename, sizeof graphic->filename),
                 graphic->filename);
        start = ops->now ? ops->now(ops->context) : 0;
        memset(&slot->shape, 0, sizeof slot->shape);
        aoe_shape_load_ror(&slot->shape, path, graphic->slp_id,
                           ops->shape_ops);
        slot->loaded = 1;
        if (ops->record)
            ops->record(ops->context, 11,
                        (ops->now ? ops->now(ops->context) : 0) - start);
    }
    if (out) *out = &slot->shape;
    return 1;
}

int aoe_graphic_frame_bounds_slp_ror(void *ops, const DatGraphic *graphic,
                                     uint32_t slp_frame, int32_t *x0,
                                     int32_t *y0, int32_t *x1, int32_t *y1) {
    AoeShapeRor *shape;
    if (!aoe_graphic_shape_ensure_ror(graphic, ops, &shape)) return 0;
    return aoe_shape_bounds_ror(shape, (int32_t)slp_frame, x0, y0, x1, y1)
               ? 1 : 0;
}

int aoe_graphic_pixel_test_slp_ror(void *ops, const DatGraphic *graphic,
                                   int32_t x, int32_t y, int32_t angle,
                                   int32_t frame) {
    AoeShapeRor *shape;
    AoeGraphicFrame sel = {0, 0};
    int32_t bx0, by0, bx1, by1;
    int16_t sx = (int16_t)x, sy = (int16_t)y;
    if (!aoe_graphic_shape_ensure_ror(graphic, ops, &shape)) return 0;
    if (frame > (int32_t)graphic->frame_count) frame = 0;
    if (!aoe_graphic_select_frame(graphic, (uint16_t)angle, (int16_t)frame,
                                  &sel))
        return 0;
    if (graphic->selection_mode == 1)
        return aoe_shape_hit_ror(shape, sel.mirror_horizontal ? -sx : sx, sy,
                                 (int32_t)sel.slp_frame) ? 1 : 0;
    if (graphic->selection_mode != 2) return 0;
    if (!aoe_shape_bounds_ror(shape, (int32_t)sel.slp_frame, &bx0, &by0,
                              &bx1, &by1))
        return 0;
    if (sel.mirror_horizontal) {
        int32_t t = -bx0;
        bx0 = -bx1;
        bx1 = t;
    }
    return sx >= bx0 && sx <= bx1 && sy >= by0 && sy <= by1;
}

int aoe_graphic_bounds_slp_ror(const DatGraphic *graphic, int32_t angle,
                               int32_t frame,
                               const AoeGraphicShapeOpsRor *ops,
                               int16_t *x0, int16_t *y0, int16_t *x1,
                               int16_t *y1) {
    if (!aoe_graphic_shape_ensure_ror(graphic, ops, NULL)) return 0;
    return aoe_graphic_bounds_ror(graphic, angle, frame,
                                  aoe_graphic_frame_bounds_slp_ror,
                                  (void *)ops, x0, y0, x1, y1);
}
