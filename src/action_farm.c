#include "action_farm.h"

#include <string.h>

#include "action_gather.h"

static int ops_ok(const AoeFarmActionOps *o) {
    return o && o->tick_seconds && o->player_attribute && o->deposit &&
           o->notify_depleted && o->destroy;
}

int aoe_farm_action_init_ror(AoeFarmActionRor *a, AoeLiveUnit *unit,
                             const DatUnitTask *task) {
    if (!a || !unit || !task) return 0;
    memset(a, 0, sizeof(*a));
    a->base.type = 0x15;
    a->base.unit = unit;
    a->base.target_id = -1;
    a->base.secondary_id = -1;
    a->task = task;
    return 1;
}

int aoe_farm_action_set_state_ror(AoeFarmActionRor *a, uint8_t state,
                                  const AoeFarmActionOps *o) {
    if (!a || !a->base.unit || !ops_ok(o)) return 0;
    a->base.state = state;
    a->remaining = o->player_attribute(o->context, a->base.unit, 36);
    return 1;
}

int aoe_farm_action_activate_ror(AoeFarmActionRor *a,
                                 const AoeFarmActionOps *o) {
    AoeLiveUnit *u;
    int16_t in;
    if (!a || !a->base.unit || !a->task ||
        !aoe_farm_action_set_state_ror(a, 6, o))
        return 0;
    u = a->base.unit;
    in = a->task->resource_in;
    if (in >= 0 && u->core.storage.carried_resource_type != in)
        aoe_unit_set_carried_ror(u, in, 0.0f);
    return 1;
}

int aoe_farm_action_update_ror(AoeFarmActionRor *a,
                               const AoeFarmActionOps *o, uint8_t *result) {
    AoeLiveUnit *u;
    const DatUnit *t;
    float amount, saved;
    if (!a || !a->base.unit || !a->task || !result || !ops_ok(o) ||
        !(t = a->base.unit->core.unit_template))
        return 0;
    *result = 0;
    u = a->base.unit;
    if (a->base.state == 1) {
        *result = 1;
        return 1;
    }
    if (a->base.state != 6) return 1;
    if (!(a->remaining > 0.0f) &&
        o->player_attribute(o->context, u, 36) > 0.0f) {
        /* fcomp/test ah,0x41: a positive (ordered) load keeps waiting. */
        if (u->core.storage.carried_amount > 0.0f) return 1;
        o->notify_depleted(o->context, u);
        o->destroy(o->context, u);
        return 1;
    }
    amount = (float)((double)o->tick_seconds(o->context) * t->work_rate *
                     a->task->work_value1);
    saved = u->core.storage.carried_amount;
    if (a->task->field_30)
        o->deposit(o->context, u, a->task->resource_in, amount);
    else
        aoe_unit_add_carried_ror(u, amount, 1, 1);
    a->remaining = (float)((double)a->remaining -
                           ((double)u->core.storage.carried_amount - saved));
    return 1;
}
