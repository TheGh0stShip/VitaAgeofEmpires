#include "player_costs.h"

#include <assert.h>
#include <math.h>
#include <string.h>

static float values[40];
static AoePlayerResources r = {40, values};

static DatUnit unit(int16_t t0, int16_t a0, int16_t t1, int16_t a1,
                    int16_t t2, int16_t a2) {
    DatUnit u;
    memset(&u, 0, sizeof(u));
    u.type = 70;
    u.costs[0] = (DatUnitCost){t0, a0, 1};
    u.costs[1] = (DatUnitCost){t1, a1, 1};
    u.costs[2] = (DatUnitCost){t2, a2, 1};
    return u;
}

static DatUnit bound_unit;

static const DatUnit *lookup(void *context, int16_t index) {
    (void)context;
    return index == 83 ? &bound_unit : NULL;
}

static void binding_tests(void) {
    AoePlayerCostBindingRor binding = {&r, 0, lookup};
    int16_t failing = 0;
    memset(values, 0, sizeof(values));
    bound_unit = unit(0, 50, 4, 1, -1, 0);
    values[0] = 120.0f;
    values[4] = 0.0f; /* pop flag 1: short population is skipped */
    assert(aoe_player_cost_queue_afford_ror(&binding, 83, &failing) &&
           failing == -1);
    aoe_player_cost_queue_pay_ror(&binding, 83);
    aoe_player_cost_queue_pay_ror(&binding, 83);
    assert(values[0] == 20.0f && values[4] == 0.0f);
    assert(!aoe_player_cost_queue_afford_ror(&binding, 83, &failing) &&
           failing == 0);
    aoe_player_cost_queue_pay_ror(&binding, 83); /* unaffordable: no-op */
    assert(values[0] == 20.0f);
    aoe_player_cost_queue_refund_ror(&binding, 83);
    assert(values[0] == 70.0f && values[4] == 1.0f);
    assert(!aoe_player_cost_queue_afford_ror(&binding, 12, &failing) &&
           failing == -1);
}

int main(void) {
    binding_tests();
    /* Villager-like: food 50, population 1. */
    DatUnit villager = unit(0, 50, 4, 1, -1, 0);
    DatUnit legion = unit(0, 60, 3, 20, 4, 1);
    int16_t failing;
    int ok, paid;

    memset(values, 0, sizeof(values));
    values[0] = 49.0f; values[3] = 100.0f; values[4] = 10.0f;
    values[11] = 4.0f; values[32] = 5.0f;
    assert(aoe_player_cost_check_ror(&r, &villager, 1.0f, 1, &failing, &ok) &&
           !ok && failing == 0);
    values[0] = 50.0f;
    assert(aoe_player_cost_check_ror(&r, &villager, 1.0f, 1, &failing, &ok) &&
           ok && failing == -1);
    /* Population cap: pop_flag 0 tests +0x2c >= +0x80 first. */
    values[11] = 5.0f;
    assert(aoe_player_cost_check_ror(&r, &villager, 1.0f, 0, &failing, &ok) &&
           !ok && failing == 0x20);
    /* pop_flag set skips a short population resource. */
    values[4] = 0.0f;
    assert(aoe_player_cost_check_ror(&r, &villager, 1.0f, 1, &failing, &ok) &&
           ok);
    /* Templates below type 70 or missing fail with -1. */
    villager.type = 60;
    assert(aoe_player_cost_check_ror(&r, &villager, 1.0f, 1, &failing, &ok) &&
           !ok && failing == -1);
    assert(aoe_player_cost_check_ror(&r, NULL, 1.0f, 1, &failing, &ok) &&
           !ok && failing == -1);
    villager.type = 70;

    /* Pay: food 50 deducted, population skipped when pop_flag set. */
    values[0] = 80.0f; values[4] = 3.0f;
    assert(aoe_player_cost_pay_ror(&r, &villager, 1.0f, 1, &paid) && paid &&
           values[0] == 30.0f && values[4] == 3.0f);
    /* pop_flag 0 fails the cap check (values[11] >= values[32]): the
       re-check makes pay charge nothing. */
    assert(aoe_player_cost_pay_ror(&r, &villager, 1.0f, 0, &paid) && !paid &&
           values[0] == 30.0f && values[4] == 3.0f);
    values[11] = 0.0f;
    values[0] = 80.0f; values[4] = 3.0f;
    assert(aoe_player_cost_pay_ror(&r, &villager, 1.0f, 0, &paid) && paid &&
           values[0] == 30.0f && values[4] == 2.0f);

    /* Refund adds (float)amount back, population included, no multiplier. */
    assert(aoe_player_cost_refund_ror(&r, &villager) &&
           values[0] == 80.0f && values[4] == 3.0f);
    villager.costs[1].paid = 0x100; /* only the low byte counts */
    assert(aoe_player_cost_refund_ror(&r, &villager) &&
           values[0] == 130.0f && values[4] == 3.0f);
    villager.costs[1].paid = 1;

    /* Gold substitution, enabled by values[29] >= 1.0: 40 food short is
       covered by gold after the legion's own 20 gold. */
    memset(values, 0, sizeof(values));
    values[0] = 20.0f; values[3] = 50.0f; values[4] = 1.0f;
    values[32] = 10.0f;
    assert(aoe_player_cost_check_ror(&r, &legion, 1.0f, 0, &failing, &ok) &&
           !ok && failing == 0);
    values[29] = 1.0f;
    /* Costs run in order: food's 40 shortfall leaves G = 10, then the
       legion's own 20 gold drives G negative, failing with 3. */
    assert(aoe_player_cost_check_ror(&r, &legion, 1.0f, 0, &failing, &ok) &&
           !ok && failing == 3);
    values[3] = 60.0f;
    assert(aoe_player_cost_check_ror(&r, &legion, 1.0f, 0, &failing, &ok) &&
           ok);
    /* Pay: food 60 > 20 takes 40 from gold and zeroes food; then gold 20. */
    assert(aoe_player_cost_pay_ror(&r, &legion, 1.0f, 0, &paid) && paid &&
           values[0] == 0.0f && values[3] == 0.0f && values[4] == 0.0f);
    /* Gold short of its own cost fails with 3 when substitution is on. */
    values[3] = 19.0f; values[0] = 60.0f; values[4] = 1.0f;
    assert(aoe_player_cost_check_ror(&r, &legion, 1.0f, 0, &failing, &ok) &&
           !ok && failing == 3);
    /* A cost without the deduct byte cannot be substituted. */
    values[3] = 500.0f; values[0] = 0.0f;
    legion.costs[0].paid = 0;
    assert(aoe_player_cost_check_ror(&r, &legion, 1.0f, 0, &failing, &ok) &&
           !ok && failing == 0);
    legion.costs[0].paid = 1;

    /* The multiplier is rounded to float in the check. */
    memset(values, 0, sizeof(values));
    values[0] = 16.5f;
    villager.costs[1].resource_type = -1;
    assert(aoe_player_cost_check_ror(&r, &villager, 0.33f, 0, &failing,
                                     &ok) &&
           ok == (16.5f >= (float)(50.0 * (double)0.33f)));
    /* NaN resource is "short" and cannot be covered. */
    values[0] = NAN;
    assert(aoe_player_cost_check_ror(&r, &villager, 1.0f, 0, &failing, &ok) &&
           !ok && failing == 0);

    /* Host guards: indices outside the allocation. */
    r.count = 29;
    assert(!aoe_player_cost_check_ror(&r, &villager, 1.0f, 0, &failing, &ok));
    r.count = 40;
    villager.costs[0].resource_type = 40;
    assert(!aoe_player_cost_check_ror(&r, &villager, 1.0f, 0, &failing, &ok));
    assert(!aoe_player_cost_refund_ror(&r, &villager));
    return 0;
}
