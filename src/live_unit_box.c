#include "live_unit_box.h"

#include "aoe_ftol.h"

#include <stddef.h>

uint8_t aoe_live_unit_box_colour_ror(const AoeLiveUnitBoxInput *in) {
    const AoeLiveUnitCore *u = in->unit;
    uint8_t flags = u->action_flags_36, colour = 0xff;
    if (in->logging_on == 1 && in->unit_78 == u->unit_id && u->unit_id >= 0)
        colour = 0;
    else if (!(flags & 1)) {
        if (in->screen_mode == 3) colour = 0x74;
        else if ((in->template_a8 & 1) && in->game_1b0 == 7) colour = in->template_a9;
    }
    if (flags & 2) colour = 0x4a;
    if (flags & 4) colour = 0x97;
    return colour;
}

static void shape(const AoeLiveUnitBoxOps *ops, int32_t kind, int16_t layer,
                  int32_t a, int32_t b, int32_t c, int32_t d, int32_t e,
                  int32_t f, int32_t g, int32_t h, int32_t priority,
                  uint8_t colour) {
    const int32_t v[8] = {a, b, c, d, e, f, g, h};
    if (ops->shape) ops->shape(ops->context, kind, layer, v, priority, colour);
}

/* The health bar: x0 = bx - 12 to x1 = bx + 11 over rows by - 2..by - 1;
   the fill end (full: x1, no back; else hp * 24 / max + x0, kept below
   x1) and the back start after it. Moves (*px, *py) to (x1, by - 1). */
static void health_bar(const AoeLiveUnitBoxOps *ops, int16_t layer,
                       int32_t hp, int32_t max_hp, int16_t bx, int16_t by,
                       uint8_t colour, int32_t *px, int32_t *py) {
    int32_t x0 = bx - 12, x1 = x0 + 0x17, y1 = by - 1, y0 = by - 2;
    int32_t fill, back, has_fill, has_back;
    if (hp == max_hp) {
        fill = x1, has_fill = 1, back = -1, has_back = 0;
    } else if (hp == 0) {
        fill = -1, has_fill = 0, back = x0, has_back = 1;
    } else {
        fill = hp * 24 / max_hp + x0;
        if ((int16_t)fill >= (int16_t)x1) fill = x1 - 1;
        back = fill + 1, has_fill = has_back = 1;
    }
    shape(ops, 2, layer, (int16_t)x0, (int16_t)y0, (int16_t)x1, (int16_t)y1,
          (int16_t)fill, (int16_t)has_fill, (int16_t)back, (int16_t)has_back,
          0x1e, colour);
    *px = x1;
    *py = y1;
}

/* The unit ID: five digits of |id|, leading ones skipped by the signed ID
   (< 10000: 1, < 1000: 2, < 100: 3, < 10: 4), so a negative ID shows its
   last digit after a minus. */
static void unit_id(const AoeLiveUnitBoxOps *ops, int16_t layer, int32_t id,
                    int32_t px, int32_t py) {
    int32_t digits[5], a = id < 0 ? -id : id, skip = id < 10000, k;
    digits[0] = a / 10000;
    digits[1] = a % 10000 / 1000;
    digits[2] = a % 1000 / 100;
    digits[3] = a % 100 / 10;
    digits[4] = a % 10;
    if (id < 1000) skip = 2;
    if (id < 100) skip = 3;
    if (id < 10) skip = 4;
    if (id < 0) {
        px += 7;
        shape(ops, 4, layer, (int16_t)px + 1, (int16_t)py + 3, (int16_t)px + 5,
              (int16_t)py + 3, 0, 0, 0, 0, 0x1e, 0x97);
    }
    for (k = skip; k < 5; ++k) {
        px += 7;
        if (digits[k] == 0)
            shape(ops, 4, layer, (int16_t)px + 1, (int16_t)py, (int16_t)px + 5,
                  (int16_t)py + 5, 0, 0, 0, 0, 0x1e, 0xff);
        else if (ops->number)
            ops->number(ops->context, digits[k], layer, (int16_t)px, (int16_t)py, 0x1e);
    }
}

void aoe_live_unit_draw_box_ror(const AoeLiveUnitBoxInput *in, int32_t x,
                                int32_t y, const AoeLiveUnitBoxOps *ops) {
    const AoeLiveUnitCore *u;
    float sx, sy;
    int16_t ax, ay, bx, by, cx, cy, dx, dy, layer;
    int32_t base_x, base_y, px, py, hp;
    uint8_t colour;
    if (!in || !in->unit || !ops || !ops->project) return;
    u = in->unit;
    sx = in->size[0];
    sy = in->size[1];
    colour = aoe_live_unit_box_colour_ror(in);
    base_x = (int16_t)(uint16_t)((uint16_t)x + (uint16_t)u->draw_left);
    base_y = (int16_t)(uint16_t)((uint16_t)y + (uint16_t)u->draw_top);
    layer = (int16_t)base_y;
    ops->project(ops->context, sx, -sy, 0.0f, base_x, base_y, &ax, &ay);
    ops->project(ops->context, sx, sy, 0.0f, base_x, base_y, &bx, &by);
    ops->project(ops->context, -sx, sy, 0.0f, base_x, base_y, &cx, &cy);
    ops->project(ops->context, -sx, -sy, 0.0f, base_x, base_y, &dx, &dy);
    shape(ops, 1, layer, ax, ay, bx, by, cx, cy, dx, dy, 10, colour);
    px = bx;
    py = by;
    if (u->action_flags_36 && in->group > 0 && in->group < 10 && in->owner_viewed &&
        ops->number)
        ops->number(ops->context, in->group, layer, (int16_t)(dx - 6),
                    (int16_t)(dy - 8), 0x1e);
    if ((u->action_flags_36 & 1) && !(in->template_a8 & 2) &&
        (hp = aoe_ftol32(u->hit_points)) > 0) {
        int16_t hx, hy;
        ops->project(ops->context, sx, -sy, in->size[2], base_x, base_y, &hx, &hy);
        if (in->max_hp > 0)
            health_bar(ops, layer, hp, in->max_hp, hx, hy, colour, &px, &py);
    }
    if (in->show_ids) unit_id(ops, layer, u->unit_id, px, py);
}

/* The shared body of the bracket draws: the view, unlock, device
   context and pen (black under the logging rule, else white), the strokes
   (from x, y, z -> to x, y, z) around the anchor plus (+0x28, +0x2a +
   dy) in 16-bit arithmetic, then relock. */
static int strokes_3d(const AoeLiveUnitFrame3dInput *in, void *renderer,
                      int32_t x, int32_t y, int16_t dy,
                      const AoeLiveUnitFrame3dOps *ops,
                      const float (*strokes)[6], int n) {
    const AoeLiveUnitCore *u = in->unit;
    int i;
    if (!ops->view(ops->context)) return 0;
    if (ops->unlock) ops->unlock(ops->context, renderer, NULL);
    if (!ops->get_dc || ops->get_dc(ops->context, renderer, NULL)) {
        int16_t ax, ay, bx, by;
        x = (int16_t)(uint16_t)((uint16_t)x + (uint16_t)u->draw_left);
        y = (int16_t)(uint16_t)((uint16_t)y + (uint16_t)(u->draw_top + dy));
        if (ops->begin)
            ops->begin(ops->context, renderer,
                       in->logging_on == 1 && in->unit_78 == u->unit_id &&
                           u->unit_id >= 0);
        for (i = 0; i < n; ++i) {
            const float *s = strokes[i];
            ops->project(ops->context, s[0], s[1], s[2], x, y, &ax, &ay);
            ops->project(ops->context, s[3], s[4], s[5], x, y, &bx, &by);
            ops->line(ops->context, renderer, ax, ay, bx, by);
        }
        if (ops->end) ops->end(ops->context, renderer);
        if (ops->release_dc) ops->release_dc(ops->context, renderer, NULL);
    }
    if (ops->lock) ops->lock(ops->context, renderer, NULL);
    return 1;
}

int aoe_live_unit_draw_brackets_ror(const AoeLiveUnitFrame3dInput *in,
                                    float outline_z, void *renderer,
                                    int32_t x, int32_t y,
                                    const AoeLiveUnitFrame3dOps *ops) {
    float ox, oy, oz, qx, qy, qz, tz;
    if (!in || !in->unit || !ops || !ops->view || !ops->project || !ops->line)
        return 0;
    ox = in->outline_x, oy = in->outline_y, oz = outline_z;
    qx = ox * 0.25f, qy = oy * 0.25f, qz = oz * 0.25f, tz = oz * 0.75f;
    {
        const float strokes[6][6] = {
            {ox, -oy, 0.0f, ox, -oy, qz},   {ox, -oy, tz, ox, -oy, oz},
            {ox, -oy, 0.0f, qx, -oy, 0.0f}, {-ox, -oy, 0.0f, -qx, -oy, 0.0f},
            {ox, -oy, 0.0f, ox, -qy, 0.0f}, {ox, oy, 0.0f, ox, qy, 0.0f}};
        return strokes_3d(in, renderer, x, y, 0, ops, strokes, 6);
    }
}

int aoe_live_unit_draw_brackets_full_ror(const AoeLiveUnitFrame3dInput *in,
                                         float outline_z, void *renderer,
                                         int32_t x, int32_t y,
                                         const AoeLiveUnitFrame3dOps *ops) {
    float X, Y, Z, qx, qy, qz, tz;
    if (!in || !in->unit || !ops || !ops->view || !ops->project || !ops->line)
        return 0;
    X = in->outline_x, Y = in->outline_y, Z = outline_z;
    qx = X * 0.25f, qy = Y * 0.25f, qz = Z * 0.25f, tz = Z * 0.75f;
    {
        const float strokes[18][6] = {
            /* top face corners */
            {X, -Y, Z, qx, -Y, Z},     {-X, -Y, Z, -qx, -Y, Z},
            {X, Y, Z, qx, Y, Z},       {-X, Y, Z, -qx, Y, Z},
            {X, Y, Z, X, qy, Z},       {X, -Y, Z, X, -qy, Z},
            {-X, Y, Z, -X, qy, Z},     {-X, -Y, Z, -X, -qy, Z},
            /* bottom face corners */
            {-X, -Y, 0.0f, -X, -qy, 0.0f}, {-X, Y, 0.0f, -X, qy, 0.0f},
            {X, Y, 0.0f, qx, Y, 0.0f},     {-X, Y, 0.0f, -qx, Y, 0.0f},
            /* vertical ticks */
            {-X, Y, 0.0f, -X, Y, qz},  {-X, Y, tz, -X, Y, Z},
            {-X, -Y, 0.0f, -X, -Y, qz}, {-X, -Y, tz, -X, -Y, Z},
            {X, Y, 0.0f, X, Y, qz},    {X, Y, tz, X, Y, Z}};
        return strokes_3d(in, renderer, x, y, -0x10, ops, strokes, 18);
    }
}
