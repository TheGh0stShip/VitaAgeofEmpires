#include "unit_garrison.h"

#include "aoe_ftol.h"

int aoe_unit_has_room_ror(const AoeLiveUnit *c) {
    return c && c->core.unit_template &&
           (int16_t)c->contents.count <
               (int16_t)c->core.unit_template->garrison_capacity;
}

void aoe_unit_leave_ror(AoeLiveUnit *u) {
    AoeLiveUnit *c;
    if (!u || !u->placement.attached_unit) return;
    c = (AoeLiveUnit *)u->placement.attached_unit;
    (void)aoe_object_update_list_detach(&c->contents, u, NULL);
    u->placement.attached_unit = NULL;
}

int aoe_unit_enter_ror(AoeLiveUnit *u, AoeLiveUnit *c, int type50,
                       const AoeGarrisonOps *o) {
    AoeObjectUpdateNode *node;
    if (!u || !c || !o || !o->vision_off || !o->leave_tile || !o->lift)
        return -1;
    if (!aoe_unit_has_room_ror(c)) return 0;
    /* The original calls +0xd4 on the old container itself. */
    if (u->placement.attached_unit)
        aoe_unit_leave_ror((AoeLiveUnit *)u->placement.attached_unit);
    if (u->placement.tile) {
        o->vision_off(o->context, u);
        o->leave_tile(o->context, u);
    }
    o->lift(o->context, u);
    if (!aoe_object_update_list_attach(&c->contents, u, &node)) return -1;
    u->placement.attached_unit = &c->core;
    u->core.position.x38 = c->core.position.x38;
    u->core.position.y3c = c->core.position.y3c;
    if (type50 && u->core.unit_id >= 0 && o->record) o->record(o->context, u);
    return 1;
}

static float rnd(const AoeGarrisonOps *o) {
    return (float)o->rand(o->context);
}

int aoe_unit_unload_spot_ror(AoeLiveUnit *t, int zone, AoeLiveUnit *p,
                             float cx, float cy, float w, float h, float *x,
                             float *y, const AoeGarrisonOps *o) {
    const DatUnit *pt = p->core.unit_template;
    const double k = (double)3.0518509447574615e-05f; /* 1 / 32767 */
    float hw = (float)((double)w + pt->size[0]);
    float hh = (float)((double)h + pt->size[1]);
    float ww = (float)(((double)w + pt->size[0]) * 2.0);
    float wh = (float)(((double)h + pt->size[1]) * 2.0);
    /* The ring counter starts at 1 and stops at 2: one ring of 50 tries;
       the growth below is computed but never used. */
    for (int ring = 1; ring < 2; ++ring) {
        for (int tries = 0; tries < 50; ++tries) {
            int side = (int)(((int32_t)o->rand(o->context) * 4) / 32767);
            switch (side) {
            case 0:
                *x = (float)((double)cx + hw);
                *y = (float)((double)rnd(o) * wh * k - hh + cy);
                break;
            case 1:
                *x = (float)((double)cx - hw);
                *y = (float)((double)rnd(o) * wh * k - hh + cy);
                break;
            case 2:
                *y = (float)((double)cy + hh);
                *x = (float)((double)rnd(o) * ww * k - hw + cx);
                break;
            case 3:
                *y = (float)((double)cy - hh);
                *x = (float)((double)rnd(o) * ww * k - hw + cx);
                break;
            default:
                break; /* rand() == 32767: the previous point again */
            }
            if (*x < 0.0f || *y < 0.0f ||
                !((double)o->map_width(o->context) > *x) ||
                !((double)o->map_height(o->context) > *y))
                continue;
            if (zone != -1 &&
                o->zone_at(o->context, t, aoe_ftol32(*x), aoe_ftol32(*y)) !=
                    zone)
                continue;
            if (o->placement(o->context, p, *x, *y) != 0) continue;
            if (o->fits(o->context, p, *x, *y) == 1) return 1;
        }
        hw = (float)((double)pt->size[0] * 2.0 + hw);
        hh = (float)((double)pt->size[1] * 2.0 + hh);
        ww = (float)((double)pt->size[0] * 4.0 + ww);
        wh = (float)((double)pt->size[1] * 4.0 + wh);
    }
    return 0;
}

int aoe_unit_unload_ror(AoeLiveUnit *t, int zone, const AoeGarrisonOps *o,
                        uint8_t *count) {
    float cx[10], cy[10], cw[10], ch[10];
    int n = 1;
    AoeObjectUpdateNode *node, *next;
    if (!t || !t->core.unit_template || !o || !count || !o->rand ||
        !o->map_width || !o->map_height || !o->zone_at || !o->placement ||
        !o->fits || !o->place)
        return 0;
    *count = 0;
    cx[0] = t->core.position.x38;
    cy[0] = t->core.position.y3c;
    cw[0] = (float)((double)t->core.unit_template->size[0] * 2.0);
    ch[0] = (float)((double)t->core.unit_template->size[1] * 2.0);
    for (node = t->contents.head; node; node = next) {
        AoeLiveUnit *p = node->object;
        float x = 0.0f, y = 0.0f;
        int k = n;
        next = node->next;
        while (k > 0 &&
               !aoe_unit_unload_spot_ror(t, zone, p, cx[k - 1], cy[k - 1],
                                         cw[k - 1], ch[k - 1], &x, &y, o)) {
            --k;
            if (k > 0) {
                cx[k - 1] = cx[k];
                cy[k - 1] = cy[k];
                cw[k - 1] = cw[k];
                ch[k - 1] = ch[k];
            }
        }
        n = k;
        if (k <= 0) continue;
        p->core.position.x38 = t->core.position.x38; /* 0x004ab260 */
        p->core.position.y3c = t->core.position.y3c;
        p->core.position.z40 = t->core.position.z40;
        aoe_unit_leave_ror(p);
        o->place(o->context, p, x, y, t->core.position.z40);
        ++*count;
        if (n < 9) {
            cx[n] = x;
            cy[n] = y;
            cw[n] = (float)((double)p->core.unit_template->size[0] * 2.0);
            ch[n] = (float)((double)p->core.unit_template->size[1] * 2.0);
            ++n;
        }
    }
    return 1;
}
