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

/* 0x004f0830 */
int aoe_player_cost_shown_ror(const AoePlayerResources *r, const DatUnit *t, float mult,
                              int16_t types[3], int16_t amounts[3]) {
    int affordable = 0;
    int16_t extra = 0;
    int i;
    for (i = 0; i < 3; ++i) {
        types[i] = -1;
        amounts[i] = 0;
    }
    if (!aoe_player_cost_check_ror(r, t, 1.0f, 0, NULL, &affordable)) return 0;
    if (!t || t->type < 70) return 1;
    for (i = 0; i < 3; ++i) {
        int16_t type = t->costs[i].resource_type;
        int16_t amount;
        if (!(uint8_t)t->costs[i].paid) continue;
        types[i] = type;
        if (type < 0) continue;
        if (!index_ok(r, type)) return 0;
        amount = (int16_t)(int32_t)((double)t->costs[i].amount * mult);
        amounts[i] = amount;
        if ((double)r->values[type] < (double)amount && affordable) {
            int16_t held = (int16_t)(int32_t)r->values[type];
            extra = (int16_t)(extra + (amount - held));
            amounts[i] = held;
        }
    }
    if (extra > 0) {
        if (types[0] == 3 || types[0] == -1) {
            types[0] = 3;
            amounts[0] = (int16_t)(amounts[0] + extra);
        } else if (types[1] == 3 || types[1] == -1) {
            types[1] = 3;
            amounts[1] = (int16_t)(amounts[1] + extra);
        } else if (types[2] == 3 || types[2] == -1) {
            types[2] = 3;
            amounts[2] = extra;
        }
    }
    for (i = 0; i < 3; ++i)
        if (amounts[i] < 1) types[i] = -1;
    return 1;
}

/* 0x004f1ff0 */
int32_t aoe_player_cost_shown_total_ror(const AoePlayerResources *r, const DatUnit *t) {
    int16_t types[3], amounts[3];
    aoe_player_cost_shown_ror(r, t, 1.0f, types, amounts);
    return (int32_t)amounts[0] + amounts[1] + amounts[2];
}
