#include "action_farm.h"

#include <assert.h>
#include <math.h>
#include <string.h>

/* Patched RoR 1.0a farm action 0x0054261c (type 0x15). */

typedef struct {
    float tick, attribute36;
    float deposited;
    int16_t deposit_type;
    unsigned depleted, destroyed;
} Fake;

static float tick_seconds(void *c) { return ((Fake *)c)->tick; }
static float attribute(void *c, AoeLiveUnit *u, int16_t index) {
    (void)u;
    assert(index == 36);
    return ((Fake *)c)->attribute36;
}
static void deposit(void *c, AoeLiveUnit *u, int16_t type, float amount) {
    (void)u;
    ((Fake *)c)->deposit_type = type;
    ((Fake *)c)->deposited += amount;
}
static void depleted(void *c, AoeLiveUnit *u) {
    (void)u;
    ++((Fake *)c)->depleted;
}
static void destroy(void *c, AoeLiveUnit *u) {
    ++((Fake *)c)->destroyed;
    u->core.hit_points = 0.0f;
}

int main(void) {
    Fake f = {.tick = 0.5f, .attribute36 = 3.0f};
    AoeFarmActionOps ops = {&f, tick_seconds, attribute, deposit, depleted,
                            destroy};
    DatUnit farm = {.present = 1, .type = 80, .id = 50, .work_rate = 2.0f,
                    .resource_capacity = 15};
    DatUnitTask task = {.action_type = 0x15, .carry_resource = 16,
                        .work_amount = 1.5f};
    AoeLiveUnit u;
    AoeFarmActionRor a;
    uint8_t r = 9;
    memset(&u, 0, sizeof(u));
    u.core.unit_template = &farm;
    u.core.hit_points = 10.0f;
    u.core.state = 2;
    u.core.storage.carried_resource_type = 0;
    u.core.storage.carried_amount = 15.0f;

    assert(aoe_farm_action_init_ror(&a, &u, &task));
    assert(a.base.type == 0x15 && a.base.target_id == -1 && a.task == &task);
    /* Activation: state 6, +0x2c from attribute 36, resource-in at 0. */
    assert(aoe_farm_action_activate_ror(&a, &ops));
    assert(a.base.state == 6 && a.remaining == 3.0f &&
           u.core.storage.carried_resource_type == 16 &&
           u.core.storage.carried_amount == 0.0f);

    /* Production 0.5 x 2.0 x 1.5 = 1.5 per update, into the unit. */
    assert(aoe_farm_action_update_ror(&a, &ops, &r) && r == 0);
    assert(u.core.storage.carried_amount == 1.5f && a.remaining == 1.5f);
    assert(aoe_farm_action_update_ror(&a, &ops, &r));
    assert(u.core.storage.carried_amount == 3.0f && a.remaining == 0.0f);

    /* Allowance used: it waits while it still holds food. */
    assert(aoe_farm_action_update_ror(&a, &ops, &r));
    assert(u.core.storage.carried_amount == 3.0f && !f.depleted);
    /* Emptied by gatherers: event 0x7e, then +0x6c. */
    u.core.storage.carried_amount = 0.0f;
    assert(aoe_farm_action_update_ror(&a, &ops, &r) && r == 0);
    assert(f.depleted == 1 && f.destroyed == 1 && u.core.hit_points == 0.0f);

    /* A non-positive allowance keeps producing (no depletion). */
    f.attribute36 = 0.0f;
    assert(aoe_farm_action_set_state_ror(&a, 6, &ops) && a.remaining == 0.0f);
    assert(aoe_farm_action_update_ror(&a, &ops, &r));
    assert(u.core.storage.carried_amount == 1.5f && f.depleted == 1 &&
           a.remaining == -1.5f);

    /* The carried amount is clamped to template +0x7c, and +0x2c falls by
       what was actually gained. */
    f.attribute36 = 100.0f;
    assert(aoe_farm_action_set_state_ror(&a, 6, &ops));
    u.core.storage.carried_amount = 14.0f;
    assert(aoe_farm_action_update_ror(&a, &ops, &r));
    assert(u.core.storage.carried_amount == 15.0f && a.remaining == 99.0f);

    /* Task +0x30 credits the owner directly; the unit gains nothing and
       +0x2c is unchanged. */
    task.field_30 = 1;
    assert(aoe_farm_action_update_ror(&a, &ops, &r));
    assert(f.deposit_type == 16 && f.deposited == 1.5f &&
           u.core.storage.carried_amount == 15.0f && a.remaining == 99.0f);

    /* State 1 reports done. */
    a.base.state = 1;
    assert(aoe_farm_action_update_ror(&a, &ops, &r) && r == 1);
    assert(!aoe_farm_action_update_ror(&a, NULL, &r));
    return 0;
}
