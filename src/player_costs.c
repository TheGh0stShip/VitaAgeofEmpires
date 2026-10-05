#include "player_costs.h"

#include <stddef.h>

static int index_ok(const AoePlayerResources *r, int32_t index) {
    return index >= 0 && index < r->count;
}

static int costs_in_range(const AoePlayerResources *r, const DatUnit *t,
                          int pop_flag) {
    for (int i = 0; i < 3; ++i) {
        int16_t type = t->costs[i].resource_type;
        if (type < 0) continue;
        if (!index_ok(r, type)) return 0;
        if (type == 4 && !pop_flag && (!index_ok(r, 11) || !index_ok(r, 32)))
            return 0;
    }
    return 1;
}

int aoe_player_cost_check_ror(const AoePlayerResources *r,
                              const DatUnit *t, float mult, int pop_flag,
                              int16_t *failing, int *affordable) {
    const float *v;
    double gold;
    int enabled;
    if (!r || !r->values || !affordable || !index_ok(r, 3) ||
        !index_ok(r, 29))
        return 0;
    v = r->values;
    if (failing) *failing = -1;
    *affordable = 0;
    enabled = (double)v[29] >= 1.0;
    gold = enabled ? (double)v[3] : 0.0;
    if (!t || t->type < 70) return 1;
    if (!costs_in_range(r, t, pop_flag)) return 0;
    for (int i = 0; i < 3; ++i) {
        int16_t type = t->costs[i].resource_type;
        float cost;
        if (type < 0) continue;
        cost = (float)((double)t->costs[i].amount * mult);
        if (type == 4 && !pop_flag && v[11] >= v[32]) {
            if (failing) *failing = 0x20;
            return 1;
        }
        if (enabled && type == 3) {
            gold -= cost;
            if (!(gold >= 0.0)) {
                if (failing) *failing = 3;
                return 1;
            }
        }
        if (v[type] >= cost) continue;
        if (type == 4 && pop_flag) continue;
        if (!(uint8_t)t->costs[i].paid || !(gold + v[type] >= cost)) {
            if (failing) *failing = type;
            return 1;
        }
        gold -= (double)cost - v[type];
    }
    *affordable = 1;
    return 1;
}

int aoe_player_cost_pay_ror(AoePlayerResources *r, const DatUnit *t,
                            float mult, int pop_flag, int *paid) {
    int affordable;
    if (!paid ||
        !aoe_player_cost_check_ror(r, t, mult, pop_flag, NULL, &affordable))
        return 0;
    *paid = 0;
    if (!affordable) return 1;
    for (int i = 0; i < 3; ++i) {
        int16_t type = t->costs[i].resource_type;
        double cost;
        if (type < 0 || !(uint8_t)t->costs[i].paid ||
            (pop_flag && type == 4))
            continue;
        cost = (double)t->costs[i].amount * mult;
        /* fcom; test ah,0x41: greater (ordered) takes the gold path. */
        if (cost > r->values[type]) {
            r->values[3] = (float)(r->values[3] - (cost - r->values[type]));
            r->values[type] = 0.0f;
        } else {
            r->values[type] = (float)(r->values[type] - cost);
        }
    }
    *paid = 1;
    return 1;
}

int aoe_player_cost_refund_ror(AoePlayerResources *r, const DatUnit *t) {
    if (!r || !r->values || !t) return 0;
    if ((uint8_t)t->type < 70) return 1;
    for (int i = 0; i < 3; ++i)
        if (t->costs[i].resource_type >= 0 &&
            !index_ok(r, t->costs[i].resource_type))
            return 0;
    for (int i = 0; i < 3; ++i) {
        int16_t type = t->costs[i].resource_type;
        if (type < 0 || !(uint8_t)t->costs[i].paid) continue;
        r->values[type] = (float)((double)t->costs[i].amount + r->values[type]);
    }
    return 1;
}

static const DatUnit *bound_template(const AoePlayerCostBindingRor *b,
                                     int16_t index) {
    return b && b->unit_template ? b->unit_template(b->context, index) : NULL;
}

int aoe_player_cost_queue_afford_ror(void *binding, int16_t unit_id,
                                     int16_t *failing) {
    AoePlayerCostBindingRor *b = binding;
    int affordable;
    if (!b || !aoe_player_cost_check_ror(b->resources,
                                         bound_template(b, unit_id), 1.0f, 1,
                                         failing, &affordable)) {
        if (failing) *failing = -1;
        return 0;
    }
    return affordable;
}

void aoe_player_cost_queue_pay_ror(void *binding, int16_t unit_id) {
    AoePlayerCostBindingRor *b = binding;
    int paid;
    if (b)
        (void)aoe_player_cost_pay_ror(b->resources, bound_template(b, unit_id),
                                      1.0f, 1, &paid);
}

void aoe_player_cost_queue_refund_ror(void *binding, int16_t unit_id) {
    AoePlayerCostBindingRor *b = binding;
    if (b)
        (void)aoe_player_cost_refund_ror(b->resources,
                                         bound_template(b, unit_id));
}
