#include "live_unit_owner.h"

#include "player_templates.h"

static int ops_ok(const AoeLiveUnitOwnerOps *o) {
    return o && o->owner_template && o->move_lists && o->vision &&
           o->resource_add && o->graphic && o->select_graphic && o->notice &&
           o->queue_cancel && o->clear_actions && o->create &&
           o->complete_research && o->status_four_count && o->stop &&
           o->group_changed;
}

/* 0x004a7200 (sign -1, mode 2) / 0x004a71a0 (sign +1, modes 1..2) and the
   type-80 mode >= 4 loops: template storage (+0x6a type, +0x70 amount,
   +0x88 mode) for one owner. */
static void storage(AoeLiveUnit *u, int16_t owner, int sign, uint8_t lo,
                    uint8_t hi, const AoeLiveUnitOwnerOps *o) {
    const DatUnit *t = u->core.unit_template;
    for (int i = 0; i < 3; ++i) {
        uint8_t mode = (uint8_t)t->storage[i].paid;
        if (mode < lo || mode > hi || t->storage[i].resource_type < 0)
            continue;
        o->resource_add(o->context, owner, t->storage[i].resource_type,
                        sign < 0 ? -t->storage[i].amount
                                 : t->storage[i].amount);
    }
}

/* Root 0x004a9ea0. */
static int change_root(AoeLiveUnit *u, int16_t to,
                       const AoeLiveUnitOwnerOps *o) {
    const DatUnit *t = u->core.unit_template, *n;
    int16_t from = u->runtime_owner_index;
    const void *cur = u->graphic.current_graphic, *g;
    float ratio;
    if (!(n = o->owner_template(o->context, to, t->id))) return 1;
    ratio = (float)((double)u->core.hit_points / t->hit_points);
    storage(u, from, -1, 2, 2, o);
    if (u->core.state == 2) o->vision(o->context, u, from, 0, 0);
    if (!o->move_lists(o->context, u, from, to)) return 0;
    /* The current graphic's slot (template +0x18 standing, +0x1c dying,
       +0x20 undead) in the new template; anything else becomes standing. */
    if (cur == o->graphic(o->context, t->standing_graphic))
        g = o->graphic(o->context, n->standing_graphic);
    else if (cur == o->graphic(o->context, t->dying_graphic))
        g = o->graphic(o->context, n->dying_graphic);
    else if (cur == o->graphic(o->context, t->undead_graphic))
        g = o->graphic(o->context, n->undead_graphic);
    else
        g = o->graphic(o->context, n->standing_graphic);
    if (g) o->select_graphic(o->context, u, g);
    u->runtime_owner_index = to;
    u->core.unit_template = n;
    u->core.hit_points = (float)((double)n->hit_points * ratio);
    if (u->core.state == 2) o->vision(o->context, u, to, 1, 0);
    storage(u, to, 1, 1, 2, o);
    o->notice(o->context, u, to);
    return 1;
}

/* Type 70 0x004aeac0. */
static int change_tribe(AoeLiveUnit *u, int16_t to,
                        const AoeLiveUnitOwnerOps *o) {
    const DatUnit *t = u->core.unit_template;
    int16_t from = u->runtime_owner_index;
    if (t->unit_class == 1) {
        int ok;
        o->stop(o->context, u);
        ok = change_root(u, to, o);
        o->clear_actions(o->context, u); /* +0x208(NULL) */
        return ok;
    }
    if (!o->owner_template(o->context, to, t->id)) return 1;
    storage(u, from, -1, 2, 2, o);
    if (!o->move_lists(o->context, u, from, to)) return 0;
    if (u->core.state == 2) o->vision(o->context, u, from, 0, 0);
    u->runtime_owner_index = to;
    if (!u->private_template) {
        DatUnit *copy = aoe_template_clone_ror(t);
        if (!copy) return 0;
        u->private_template = copy;
        u->core.unit_template = copy;
    }
    if (u->core.state == 2) o->vision(o->context, u, to, 1, 0);
    storage(u, to, 1, 1, 2, o);
    o->group_changed(o->context, u);
    o->notice(o->context, u, to);
    return 1;
}

/* Type 80 0x004ad190. */
static int change_building(AoeLiveUnit *u, int16_t to,
                           const AoeLiveUnitOwnerOps *o) {
    AoeBuildingUpdate *b = u->building_update;
    int16_t from = u->runtime_owner_index;
    const DatUnit *t;
    if (u->core.state == 2) storage(u, from, -1, 4, 255, o);
    if (u->core.state == 0) o->vision(o->context, u, from, 0, 1);
    if (b && b->queue_count > 0 && b->queue_entries) {
        int16_t count = b->queue_entries[0].quantity;
        if (b->queue_status == 4) {
            uint32_t *n = o->status_four_count(o->context, from);
            b->queue_status = -1;
            if (n) --*n;
        } else {
            count = (int16_t)(count - 1);
        }
        o->queue_cancel(o->context, u, b->queue_entries[0].unit_id, count);
    }
    o->clear_actions(o->context, u);
    if (!change_tribe(u, to, o)) return 0;
    if (u->core.state == 0) o->vision(o->context, u, to, 1, 1);
    if (u->core.state != 2) return 1;
    t = u->core.unit_template;
    if (t->stack_unit_id > -1)
        o->create(o->context, u->runtime_owner_index, t->stack_unit_id,
                  u->core.position.x38, u->core.position.y3c,
                  u->core.position.z40);
    if (t->building_research_id > -1)
        o->complete_research(o->context, u->runtime_owner_index,
                             t->building_research_id);
    if (u->core.state == 2) storage(u, u->runtime_owner_index, 1, 4, 255, o);
    return 1;
}

int aoe_live_unit_change_owner_ror(AoeLiveUnit *u, int16_t to,
                                   const AoeLiveUnitOwnerOps *o) {
    const DatUnit *t;
    if (!u || !u->constructed || !(t = u->core.unit_template) || !ops_ok(o))
        return 0;
    if (t->type == 80) return change_building(u, to, o);
    if (t->type == 70) return change_tribe(u, to, o);
    return change_root(u, to, o);
}
