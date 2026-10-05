#include "map_shore.h"

#include <stddef.h>
#include <string.h>

#include "aoe_ftol.h"

int aoe_shore_is_water_ror(uint8_t terrain) {
    return terrain == 1 || terrain == 22 || terrain == 4;
}

static uint8_t terrain_at(const AoeShoreMapRor *m, int32_t x, int32_t y) {
    return (uint8_t)(m->terrain(m->context, x, y) & 0x1f);
}

static int water_at(const AoeShoreMapRor *m, int32_t x, int32_t y) {
    return aoe_shore_is_water_ror(terrain_at(m, x, y));
}

static void paint(const AoeShoreMapRor *m, int32_t x, int32_t y,
                  uint8_t terrain) {
    if (m->paint) m->paint(m->context, x, y, terrain);
}

static int valid(const AoeShoreMapRor *m) {
    return m && m->terrain && m->width > 0 && m->height > 0;
}

int aoe_shore_flat_ror(const AoeShoreMapRor *m, int32_t x, int32_t y) {
    if (!m || !m->shape) return 0;
    if (x > 0 && m->shape(m->context, x - 1, y)) return 0;
    if (x < m->width - 1 && m->shape(m->context, x + 1, y)) return 0;
    if (y > 0) {
        if (m->shape(m->context, x, y - 1)) return 0;
        if (x > 0 && m->shape(m->context, x - 1, y - 1)) return 0;
        if (x < m->width - 1 && m->shape(m->context, x + 1, y - 1)) return 0;
    }
    if (y < m->height - 1) {
        if (m->shape(m->context, x, y + 1)) return 0;
        if (x > 0 && m->shape(m->context, x - 1, y + 1)) return 0;
        if (x < m->width - 1 && m->shape(m->context, x + 1, y + 1)) return 0;
    }
    return 1;
}

int aoe_shore_paint_area_ror(const AoeShoreMapRor *m, int32_t x, int32_t y,
                             int32_t r, uint8_t terrain) {
    int32_t x0 = x - r, x1 = x + r, y0 = y - r, y1 = y + r;
    if (!valid(m)) return 0;
    if (x0 < 0) x0 = 0;
    if (x1 >= m->width) x1 = m->width - 1;
    if (y0 < 0) y0 = 0;
    if (y1 >= m->height) y1 = m->height - 1;
    for (int32_t ty = y0; ty <= y1; ++ty)
        for (int32_t tx = x0; tx <= x1; ++tx)
            if (!aoe_shore_is_water_ror(terrain) || aoe_shore_flat_ror(m, tx, ty))
                paint(m, tx, ty, terrain);
    return 1;
}

static void clamp(const AoeShoreMapRor *m, int32_t *x0, int32_t *y0,
                  int32_t *x1, int32_t *y1) {
    if (*x0 < 0) *x0 = 0;
    if (*y0 < 0) *y0 = 0;
    if (*x1 >= m->width) *x1 = m->width - 1;
    if (*y1 >= m->height) *y1 = m->height - 1;
}

void aoe_shore_dry_slopes_ror(const AoeShoreMapRor *m, int32_t x0,
                              int32_t y0, int32_t x1, int32_t y1) {
    if (!valid(m)) return;
    clamp(m, &x0, &y0, &x1, &y1);
    for (int32_t y = y0; y <= y1; ++y)
        for (int32_t x = x0; x <= x1; ++x)
            if (water_at(m, x, y) && !aoe_shore_flat_ror(m, x, y))
                paint(m, x, y, 0);
}

/* Water in column x at rows y-1, y, y+1 (within the map). */
static int water_column(const AoeShoreMapRor *m, int32_t x, int32_t y) {
    return water_at(m, x, y) || (y > 0 && water_at(m, x, y - 1)) ||
           (y < m->height - 1 && water_at(m, x, y + 1));
}

void aoe_shore_fix_beaches_ror(const AoeShoreMapRor *m, int32_t x0,
                               int32_t y0, int32_t x1, int32_t y1,
                               uint8_t land) {
    if (!valid(m)) return;
    clamp(m, &x0, &y0, &x1, &y1);
    for (int32_t y = y0; y <= y1; ++y)
        for (int32_t x = x0; x <= x1; ++x) {
            uint8_t t = terrain_at(m, x, y);
            int near = (y > 0 && water_at(m, x, y - 1)) ||
                       (y < m->height - 1 && water_at(m, x, y + 1)) ||
                       (x > 0 && water_column(m, x - 1, y)) ||
                       (x < m->width - 1 && water_column(m, x + 1, y));
            if (t == 2) {
                if (!near) paint(m, x, y, land);
            } else if (!aoe_shore_is_water_ror(t) && near) {
                paint(m, x, y, 2);
            }
        }
}

void aoe_shore_smooth_ror(const AoeShoreMapRor *m, int32_t x0, int32_t y0,
                          int32_t x1, int32_t y1, uint8_t terrain) {
    if (!valid(m)) return;
    clamp(m, &x0, &y0, &x1, &y1);
    for (;;) {
        int changed = 0;
        if (x0 > 0) --x0;
        if (y0 > 0) --y0;
        if (x1 < m->width - 1) ++x1;
        if (y1 < m->height - 1) ++y1;
        for (int pass = 0; pass < 2; ++pass)
            for (int32_t y = y0; y <= y1; ++y)
                for (int32_t x = x0; x <= x1; ++x) {
                    int n = 0, s = 0, w = 0, e = 0, nw = 0, ne = 0, sw = 0,
                        se = 0, pinched = 0, hit = 0;
                    if (water_at(m, x, y)) continue;
                    if (y > 0 && water_at(m, x, y - 1)) n = 1;
                    if (y < m->height - 1 && water_at(m, x, y + 1)) s = 1;
                    if (x > 0 && water_at(m, x - 1, y)) w = 1;
                    if (x < m->width - 1 && water_at(m, x + 1, y)) e = 1;
                    if (y > 0) {
                        nw = x > 0 && water_at(m, x - 1, y - 1);
                        ne = x < m->width - 1 && water_at(m, x + 1, y - 1);
                    }
                    if (y < m->height - 1) {
                        sw = x > 0 && water_at(m, x - 1, y + 1);
                        se = x < m->width - 1 && water_at(m, x + 1, y + 1);
                    }
                    if (pass == 0) {
                        hit = (n && s) || (e && w);
                    } else {
                        if (nw) {
                            if ((ne && !n) || (e && !ne) || (sw && !w) ||
                                (s && !sw))
                                pinched = 1;
                            else if (se && !s && !e)
                                pinched = 1;
                        }
                        if (ne && !pinched &&
                            ((nw && !n) || (w && !nw) || (se && !e) ||
                             (s && !se) || (sw && !w && !s)))
                            pinched = 1;
                        if (se && !pinched &&
                            ((ne && !e) || (n && !ne) || (sw && !s) ||
                             (w && !sw) || (nw && !w && !n)))
                            pinched = 1;
                        if (sw)
                            hit = pinched || (nw && !w) || (n && !nw) ||
                                  (se && !s) || (e && !se) ||
                                  (ne && !e && !n);
                        else
                            hit = pinched;
                    }
                    if (!hit) continue;
                    if (!aoe_shore_is_water_ror(terrain)) {
                        if (nw) paint(m, x - 1, y - 1, terrain);
                        if (n) paint(m, x, y - 1, terrain);
                        if (ne) paint(m, x + 1, y - 1, terrain);
                        if (e) paint(m, x + 1, y, terrain);
                        if (se) paint(m, x + 1, y + 1, terrain);
                        if (s) paint(m, x, y + 1, terrain);
                        if (sw) paint(m, x - 1, y + 1, terrain);
                        if (w) paint(m, x - 1, y, terrain);
                    } else {
                        if (!aoe_shore_flat_ror(m, x, y)) continue;
                        paint(m, x, y, terrain);
                    }
                    changed = 1;
                }
        if (!changed) {
            aoe_shore_fix_beaches_ror(m, x0, y0, x1, y1, terrain);
            if (m->refresh) m->refresh(m->context, x0, y0, x1, y1);
            return;
        }
    }
}

int aoe_shore_paint_line_ror(const AoeShoreMapRor *m, AoeShoreBaseFn base,
                             int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                             int32_t r, uint8_t terrain) {
    int32_t lx, ly, hx, hy;
    if (!m) return 0;
    if (base) base(m->context, x0, y0, x1, y1, r, terrain);
    if (x0 < x1) {
        lx = x0 - r;
        hx = x1;
    } else {
        lx = x1 - r;
        hx = x0;
    }
    if (y0 < y1) {
        ly = y0 - r;
        hy = y1;
    } else {
        ly = y1 - r;
        hy = y0;
    }
    aoe_shore_smooth_ror(m, lx - 2, ly - 2, hx + r + 2, hy + r + 2, terrain);
    return 1;
}

int aoe_shore_raise_ror(const AoeShoreMapRor *m, AoeShoreBaseFn base,
                        int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                        int32_t r, uint32_t size) {
    int32_t grow = (int32_t)(size & 0xff) + r, lx, ly, hx, hy;
    if (!m) return 0;
    if (base) base(m->context, x0, y0, x1, y1, r, size);
    if (x0 < x1) {
        lx = x0 - grow;
        hx = x1 + grow;
    } else {
        lx = x1 - grow;
        hx = x0 + grow;
    }
    if (y0 < y1) {
        ly = y0 - grow;
        hy = y1 + grow;
    } else {
        ly = y1 - grow;
        hy = y0 + grow;
    }
    aoe_shore_dry_slopes_ror(m, lx - 2, ly - 2, hx + 2, hy + 2);
    aoe_shore_smooth_ror(m, lx - 2, ly - 2, hx + 2, hy + 2, 0);
    return 1;
}

/* ---- cliffs ---- */

static int key_ok(int32_t v) { return v >= -1 && v <= 2; }

static int32_t rule_index(int32_t a, int32_t b, int32_t c, int32_t d) {
    return d + 0x55 + (c + (b + a * 4) * 4) * 4;
}

AoeCliffRuleRor *aoe_cliff_rule_ror(AoeCliffTableRor *t, int32_t a,
                                    int32_t b, int32_t c, int32_t d) {
    if (!t || !key_ok(a) || !key_ok(b) || !key_ok(c) || !key_ok(d))
        return NULL;
    return &t->rules[rule_index(a, b, c, d)];
}

void aoe_cliff_set_rule_ror(AoeCliffTableRor *t, int32_t a, int32_t b,
                            int32_t c, int32_t d, int32_t type, int32_t shape,
                            int32_t alt_type, int32_t alt_shape, float ox,
                            float oy) {
    AoeCliffRuleRor *r = aoe_cliff_rule_ror(t, a, b, c, d);
    if (!r) return;
    r->type = type;
    r->shape = shape;
    r->alt_type = alt_type;
    r->alt_shape = alt_shape;
    r->offset_x = ox;
    r->offset_y = oy;
    r->key[0] = a;
    r->key[1] = b;
    r->key[2] = c;
    r->key[3] = d;
}

void aoe_cliff_table_init_ror(AoeCliffTableRor *t) {
    static const struct {
        int8_t a, b, c, d;
        int16_t type, shape, alt_type, alt_shape;
        float ox, oy;
    } pieces[20] = {
        {1, 0, 0, 0, 0x108, 0x10, -1, -1, 1.5f, 1.5f},
        {0, 1, 0, 0, 0x108, 0x12, -1, -1, 1.5f, 1.5f},
        {0, 0, 1, 0, 0x108, 0x11, -1, -1, 1.5f, 1.5f},
        {0, 0, 0, 1, 0x108, 0x13, -1, -1, 1.5f, 1.5f},
        {-1, 0, 0, 0, 0x10a, 0x14, -1, -1, 1.5f, 1.5f},
        {0, -1, 0, 0, 0x109, 0x16, -1, -1, 1.5f, 1.5f},
        {0, 0, -1, 0, 0x10a, 0x15, -1, -1, 1.5f, 1.5f},
        {0, 0, 0, -1, 0x109, 0x17, -1, -1, 1.5f, 1.5f},
        {-1, 1, 0, 0, 0x10e, 0, -1, -1, 1.5f, 2.0f},
        {0, -1, -1, 0, 0x110, 9, -1, -1, 1.0f, 2.0f},
        {0, 0, 1, -1, 0x10c, 6, -1, -1, 1.0f, 1.5f},
        {1, 0, 0, 1, 0x108, 3, -1, -1, 1.5f, 1.5f},
        {1, -1, 0, 0, 0x10b, 0xc, -1, -1, 2.0f, 1.5f},
        {0, 1, 1, 0, 0x108, 0xf, -1, -1, 1.5f, 1.5f},
        {0, 0, -1, 1, 0x10d, 0xe, -1, -1, 1.5f, 1.0f},
        {-1, 0, 0, -1, 0x10f, 0xd, -1, -1, 2.0f, 1.0f},
        {1, 0, 1, 0, 0x108, 4, 0x108, 5, 1.5f, 1.5f},
        {0, 1, 0, 1, 0x108, 1, 0x108, 2, 1.5f, 1.5f},
        {-1, 0, -1, 0, 0x10a, 0xb, 0x10a, 10, 1.5f, 1.5f},
        {0, -1, 0, -1, 0x109, 8, 0x109, 7, 1.5f, 1.5f}};
    if (!t) return;
    t->last_x = t->last_y = t->cell_x = t->cell_y = -1;
    for (int i = 0; i < 256; ++i) {
        AoeCliffRuleRor *r = &t->rules[i];
        r->type = r->shape = r->alt_type = r->alt_shape = -1;
        r->key[0] = r->key[1] = r->key[2] = r->key[3] = 0;
        r->offset_x = r->offset_y = 0.0f;
    }
    for (int i = 0; i < 20; ++i)
        aoe_cliff_set_rule_ror(t, pieces[i].a, pieces[i].b, pieces[i].c,
                               pieces[i].d, pieces[i].type, pieces[i].shape,
                               pieces[i].alt_type, pieces[i].alt_shape,
                               pieces[i].ox, pieces[i].oy);
}

int aoe_cliff_rule_exists_ror(AoeCliffTableRor *t, int32_t a, int32_t b,
                              int32_t c, int32_t d) {
    AoeCliffRuleRor *r = aoe_cliff_rule_ror(t, a, b, c, d);
    return r && r->type >= 0;
}

void aoe_cliff_key_of_ror(const AoeCliffTableRor *t, int has_object,
                          int16_t type, uint8_t shape, int32_t key[4]) {
    if (!key) return;
    key[0] = key[1] = key[2] = key[3] = 0;
    if (!t || !has_object) return;
    for (int i = 0; i < 256; ++i) {
        const AoeCliffRuleRor *r = &t->rules[i];
        if ((r->type == type && r->shape == shape) ||
            (r->alt_type == type && r->alt_shape == shape)) {
            memcpy(key, r->key, sizeof(r->key));
            return;
        }
    }
}

int32_t aoe_cliff_opposite_ror(int32_t side) {
    return side + 2 > 3 ? side - 2 : side + 2;
}

static int cliff_on(const AoeCliffOpsRor *ops, int32_t x, int32_t y,
                    AoeCliffObjectRor *out) {
    AoeCliffObjectRor o;
    for (int32_t i = 0; ops->object_at(ops->context, x, y, i, &o); ++i)
        if (o.type > 0x107 && o.type < 0x112) {
            if (out) *out = o;
            return 1;
        }
    return 0;
}

int aoe_cliff_at_ror(const AoeShoreMapRor *map, const AoeCliffOpsRor *ops,
                     int32_t gx, int32_t gy, AoeCliffObjectRor *out) {
    int32_t x = gx * 3 + 1, y = gy * 3 + 1;
    if (!map || !ops || !ops->object_at || gx * 3 + 2 > map->width ||
        gy * 3 + 2 > map->height)
        return 0;
    return cliff_on(ops, x, y, out) || cliff_on(ops, x, y + 1, out) ||
           cliff_on(ops, x + 1, y, out) || cliff_on(ops, x + 1, y + 1, out);
}

void *aoe_cliff_place_ror(AoeCliffTableRor *t, const AoeCliffOpsRor *ops,
                          int32_t gx, int32_t gy, int32_t a, int32_t b,
                          int32_t c, int32_t d) {
    AoeCliffRuleRor *r = aoe_cliff_rule_ror(t, a, b, c, d);
    float fx, fy;
    int32_t type, shape, tx, ty;
    void *object;
    if (!r || !ops || !ops->create) return NULL;
    fx = (float)(gx * 3) + r->offset_x;
    fy = (float)(gy * 3) + r->offset_y;
    type = r->type;
    shape = r->shape;
    if (r->alt_type > -1 && ops->rand && ops->rand(ops->context) * 2 > 0x7fff) {
        type = r->alt_type;
        shape = r->alt_shape;
    }
    if (type < 0) return NULL;
    object = ops->create(ops->context, type, fx, fy);
    ty = aoe_ftol32(fy);
    tx = aoe_ftol32(fx);
    for (int32_t dx = -1; dx < 2; ++dx)
        for (int32_t dy = -1; dy < 2; ++dy)
            if (ops->touch) ops->touch(ops->context, tx + dx, ty + dy);
    if (object && ops->set_shape) ops->set_shape(ops->context, object, (uint8_t)shape);
    if (ops->placed) ops->placed(ops->context, type, fx, fy);
    return object;
}

static void touch_around(const AoeCliffOpsRor *ops,
                         const AoeCliffObjectRor *o) {
    int32_t tx = aoe_ftol32(o->x), ty = aoe_ftol32(o->y);
    for (int32_t dx = -1; dx < 2; ++dx)
        for (int32_t dy = -1; dy < 2; ++dy)
            if (ops->touch) ops->touch(ops->context, tx + dx, ty + dy);
}

static void remove_object(const AoeCliffOpsRor *ops, void *object) {
    if (ops->remove) ops->remove(ops->context, object);
}

void aoe_cliff_drop_side_ror(AoeCliffTableRor *t, const AoeShoreMapRor *map,
                             const AoeCliffOpsRor *ops, int32_t gx,
                             int32_t gy, int32_t side) {
    AoeCliffObjectRor o;
    int32_t key[4];
    if (!t || !map || !ops) return;
    switch (side) {
    case 0: if (++gx >= map->width) return; break;
    case 1: if (++gy >= map->height) return; break;
    case 2: if (gx <= 0) return; --gx; break;
    case 3: if (gy <= 0) return; --gy; break;
    default: break;
    }
    if (!aoe_cliff_at_ror(map, ops, gx, gy, &o)) return;
    aoe_cliff_key_of_ror(t, 1, o.type, o.shape, key);
    touch_around(ops, &o);
    remove_object(ops, o.object);
    key[aoe_cliff_opposite_ror(side)] = 0;
    aoe_cliff_place_ror(t, ops, gx, gy, key[0], key[1], key[2], key[3]);
}

void aoe_cliff_erase_ror(AoeCliffTableRor *t, const AoeShoreMapRor *map,
                         const AoeCliffOpsRor *ops, int32_t gx, int32_t gy) {
    AoeCliffObjectRor o;
    int32_t key[4];
    if (!t || !ops || !aoe_cliff_at_ror(map, ops, gx, gy, &o)) return;
    aoe_cliff_key_of_ror(t, 1, o.type, o.shape, key);
    for (int32_t i = 0; i < 4; ++i)
        if (key[i]) aoe_cliff_drop_side_ror(t, map, ops, gx, gy, i);
    touch_around(ops, &o);
    remove_object(ops, o.object);
}

static int fits(AoeCliffTableRor *t, const int32_t k[4]) {
    return aoe_cliff_rule_exists_ror(t, k[0], k[1], k[2], k[3]);
}

int32_t aoe_cliff_join_ror(AoeCliffTableRor *t, const AoeShoreMapRor *map,
                           const AoeCliffOpsRor *ops, int32_t gx, int32_t gy,
                           int32_t side, int32_t value, int32_t keep) {
    AoeCliffObjectRor o;
    int has, first = -1, second = -1;
    int32_t key[4], first_value = 0, second_value = 0;
    if (!t || !ops || side < 0 || side > 3) return 0;
    has = aoe_cliff_at_ror(map, ops, gx, gy, &o);
    aoe_cliff_key_of_ror(t, has, has ? o.type : 0, has ? o.shape : 0, key);
    if (key[side]) {
        if (value == 0) return key[side];
        if (value == key[side]) return value;
    }
    if (has) {
        touch_around(ops, &o);
        remove_object(ops, o.object);
    }
    if (keep == side || keep < 0) {
        int others = 0;
        for (int i = 0; i < 4; ++i) others += key[i] && i != side;
        if (others == 2)
            for (int i = 0; i < 4; ++i) {
                if (!key[i] || i == side) continue;
                if (first < 0) {
                    first = i;
                    first_value = key[i];
                } else {
                    second = i;
                    second_value = key[i];
                }
                key[i] = 0;
            }
    } else {
        for (int i = 0; i < 4; ++i)
            if (i != keep) {
                aoe_cliff_drop_side_ror(t, map, ops, gx, gy, i);
                key[i] = 0;
            }
    }
    if (value != 0) {
        key[side] = value;
        if (first >= 0) {
            key[first] = first_value;
            if (!fits(t, key)) {
                key[first] = 0;
                aoe_cliff_drop_side_ror(t, map, ops, gx, gy, first);
            }
        }
        if (second >= 0) {
            key[second] = second_value;
            if (!fits(t, key)) {
                key[second] = 0;
                aoe_cliff_drop_side_ror(t, map, ops, gx, gy, second);
            }
        }
        if (!fits(t, key))
            for (int i = 0; i < 4; ++i)
                if (i != side && key[i]) {
                    aoe_cliff_drop_side_ror(t, map, ops, gx, gy, i);
                    key[i] = 0;
                }
        goto place;
    }
    value = 1;
    key[side] = 1;
    if (first >= 0) {
        key[first] = first_value;
        if (fits(t, key)) goto drop_second;
        key[first] = 0;
    }
    if (second >= 0) {
        key[second] = second_value;
        if (fits(t, key)) {
            second_value = 0;
            goto drop_first;
        }
        key[second] = 0;
    }
    if (!fits(t, key)) {
        key[side] = -1;
        value = -1;
        if (first >= 0) {
            key[first] = first_value;
            if (fits(t, key)) goto drop_second;
            key[first] = 0;
        }
        if (second >= 0) {
            key[second] = second_value;
            if (fits(t, key)) {
                second_value = 0;
                goto drop_first;
            }
            key[second] = 0;
        }
        if (!fits(t, key))
            for (int i = 0; i < 4; ++i)
                if (i != side && key[i]) {
                    aoe_cliff_drop_side_ror(t, map, ops, gx, gy, i);
                    key[i] = 0;
                }
    }
drop_first:
    if (first_value) aoe_cliff_drop_side_ror(t, map, ops, gx, gy, first);
drop_second:
    if (second_value) aoe_cliff_drop_side_ror(t, map, ops, gx, gy, second);
place:
    aoe_cliff_place_ror(t, ops, gx, gy, key[0], key[1], key[2], key[3]);
    return value;
}

int aoe_cliff_drag_ror(AoeCliffTableRor *t, const AoeShoreMapRor *map,
                       const AoeCliffOpsRor *ops, int32_t x, int32_t y,
                       int32_t a, uint8_t erase) {
    int32_t gx = x / 3, gy = y / 3, last_x, nx, ny, side, v;
    if (!t || !map || !ops) return 0;
    if (ops->base_drag && ops->base_drag(ops->context, x, y, a, erase))
        return 1;
    last_x = t->last_x;
    t->cell_x = gx;
    t->cell_y = gy;
    if ((last_x == gx && t->last_y == gy) || map->width <= gx * 3 + 2 ||
        map->height <= gy * 3 + 2 || gx < 0 || gy < 0)
        return 0;
    nx = gx;
    ny = gy;
    if (last_x != -1 && t->last_y != -1) {
        int32_t dx = x - last_x * 3, dy = y - t->last_y * 3;
        if (dx >= 0 && dx < 3) {
            nx = last_x;
            if (dy >= 0 && dy < 3) return 0;
        } else {
            ny = t->last_y;
        }
    }
    if (nx == last_x && ny == t->last_y) return 1;
    if (erase) {
        aoe_cliff_erase_ror(t, map, ops, nx, ny);
        return 1;
    }
    if (last_x == -1) {
        t->last_x = nx;
        t->last_y = ny;
        t->last_side = -1;
        return 1;
    }
    if (last_x < nx) side = 0;
    else if (nx < last_x) side = 2;
    else side = ny <= t->last_y ? 3 : 1;
    v = aoe_cliff_join_ror(t, map, ops, last_x, t->last_y, side, 0,
                           t->last_side);
    t->last_side = aoe_cliff_opposite_ror(side);
    t->last_x = nx;
    t->last_y = ny;
    aoe_cliff_join_ror(t, map, ops, nx, ny, t->last_side, v, -1);
    return 1;
}

int aoe_cliff_press_ror(AoeCliffTableRor *t, const AoeCliffOpsRor *ops,
                        int32_t x, int32_t y, int32_t a, int32_t b,
                        int32_t c, int32_t d) {
    if (!t) return 0;
    if (t->cell_x != x / 3 || t->cell_y != y / 3)
        t->last_x = t->last_y = t->cell_x = t->cell_y = t->last_side = -1;
    if (ops && ops->base_press) ops->base_press(ops->context, x, y, a, b, c, d);
    return 1;
}
