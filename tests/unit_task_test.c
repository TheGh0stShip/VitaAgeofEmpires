#include "unit_task.h"

#include <assert.h>
#include <math.h>
#include <string.h>

/* Players: 1 = actor, 2 = ally (mutual), 3 = enemy, 4 = one-way friend. */
static int accepts(void *c, int16_t p, int16_t o) {
    (void)c;
    if (p == o) return 1;
    if ((p == 1 && o == 2) || (p == 2 && o == 1)) return 1;
    return p == 1 && o == 4;
}
static int hostile(void *c, int16_t p, int16_t o) {
    (void)c;
    return p == 1 && o == 3;
}
static int16_t owners[8];
static AoeLiveUnit units[8];
static int16_t owner_of(void *c, const AoeLiveUnit *u) {
    (void)c;
    return owners[u - units];
}
static int32_t map_size(void *c, const AoeLiveUnit *a, int axis) {
    (void)c; (void)a;
    return axis ? 20 : 30;
}
static int terrain(void *c, const AoeLiveUnit *a, int16_t x, int16_t y,
                   uint8_t *t) {
    (void)c; (void)a;
    *t = (uint8_t)(x == 5 && y == 6 ? 2 : 0);
    return 1;
}

static AoeLiveUnit *make(int i, const DatUnit *t, int16_t owner,
                         uint8_t state) {
    units[i].constructed = 1;
    units[i].core.unit_id = 100 + i;
    units[i].core.unit_template = t;
    units[i].core.state = state;
    units[i].core.hit_points = t->hit_points;
    units[i].core.storage.carried_resource_type = -1;
    owners[i] = owner;
    return &units[i];
}

int main(void) {
    AoeUnitTaskOps ops = {0, owner_of, accepts, hostile, map_size, terrain};
    DatUnitTask villager_tasks[8], priest_tasks[2], soldier_tasks[3];
    DatUnit villager, priest, soldier, town_center, house, tree, deer, ship,
        transport;
    const DatUnitTask *task;
    int allowed;
    memset(villager_tasks, 0, sizeof(villager_tasks));
    memset(priest_tasks, 0, sizeof(priest_tasks));
    memset(soldier_tasks, 0, sizeof(soldier_tasks));
    memset(&villager, 0, sizeof(DatUnit) * 0 + sizeof(villager));
    memset(&priest, 0, sizeof(priest)); memset(&soldier, 0, sizeof(soldier));
    memset(&town_center, 0, sizeof(town_center)); memset(&house, 0, sizeof(house));
    memset(&tree, 0, sizeof(tree)); memset(&deer, 0, sizeof(deer));
    memset(&ship, 0, sizeof(ship)); memset(&transport, 0, sizeof(transport));

    /* Villager tasks: gather wood (class 15 target), deposit wood (action
       5, carry_resource 1), build (0x65), repair (0x6a, class -1), hunt with
       a carried-resource requirement, terrain task for terrain 2, an
       any_flagged_target attack, and repair ships (class 22). */
    villager_tasks[0] = (DatUnitTask){.action_type = 5, .target_class = 15,
                                      .target_template = -1, .owner_mode = 3};
    villager_tasks[1] = (DatUnitTask){.action_type = 5, .carry_resource = 1,
                                      .target_class = -1, .target_template = -1};
    villager_tasks[2] = (DatUnitTask){.action_type = 0x65, .target_class = -1,
                                      .target_template = -1, .unbuilt_target = 1,
                                      .owner_mode = 1};
    villager_tasks[3] = (DatUnitTask){.action_type = 0x6a, .target_class = -1,
                                      .target_template = -1};
    villager_tasks[4] = (DatUnitTask){.action_type = 5, .target_class = 11,
                                      .target_template = -1,
                                      .requires_carried_resource = 1};
    villager_tasks[5] = (DatUnitTask){.action_type = 1, .target_class = -1,
                                      .target_template = -1, .target_terrain = 2};
    villager_tasks[6] = (DatUnitTask){.action_type = 7, .target_class = -1,
                                      .target_template = -1, .any_flagged_target = 1,
                                      .owner_mode = 5};
    villager_tasks[7] = (DatUnitTask){.action_type = 0x6a, .target_class = 22,
                                      .target_template = -1};
    villager.type = 70; villager.unit_class = 4; villager.hit_points = 25;
    villager.tasks = villager_tasks; villager.task_count = 8;
    priest_tasks[0] = (DatUnitTask){.action_type = 0x68, .target_terrain = -1};
    priest_tasks[1] = (DatUnitTask){.action_type = 0x69, .target_terrain = -1};
    priest.type = 70; priest.unit_class = 18; priest.hit_points = 25;
    priest.tasks = priest_tasks; priest.task_count = 2;
    soldier_tasks[0] = (DatUnitTask){.action_type = 7, .target_class = -1,
                                     .target_template = -1, .any_flagged_target = 1,
                                     .owner_mode = 2, .target_terrain = -1};
    soldier.type = 70; soldier.unit_class = 6; soldier.hit_points = 40;
    soldier.field_8c = 1;
    soldier.tasks = soldier_tasks; soldier.task_count = 1;
    town_center.type = 80; town_center.id = 109; town_center.unit_class = 3;
    town_center.hit_points = 600;
    house.type = 80; house.id = 70; house.unit_class = 3; house.hit_points = 75;
    tree.type = 30; tree.unit_class = 15; tree.hit_points = 25;
    deer.type = 70; deer.unit_class = 11; deer.hit_points = 5;
    deer.field_8c = 1;
    ship.type = 70; ship.unit_class = 22; ship.hit_points = 100;
    transport.type = 80; transport.id = 99; transport.field_8f = 7;
    transport.hit_points = 100;

    AoeLiveUnit *v = make(0, &villager, 1, 2);
    AoeLiveUnit *tc = make(1, &town_center, 1, 2);
    AoeLiveUnit *tr = make(2, &tree, 0, 2);
    AoeLiveUnit *found = make(3, &house, 1, 0);
    AoeLiveUnit *enemy = make(4, &soldier, 3, 2);
    AoeLiveUnit *p = make(5, &priest, 1, 2);
    AoeLiveUnit *boat = make(6, &ship, 2, 2);
    AoeLiveUnit *dr = make(7, &deer, 0, 2);

    /* Base: class pass picks "gather wood" on a gaia tree (mode 3). */
    assert(aoe_unit_task_select_tribe_ror(v, tr, -1.0f, -1.0f, &ops, &task) &&
           task == &villager_tasks[0]);
    /* Mode 3 rejects a tree owned by a player. */
    owners[2] = 2;
    assert(aoe_unit_task_allowed_ror(&villager_tasks[0], v, tr, &ops,
                                     &allowed) && !allowed);
    owners[2] = 0;
    /* Smart order: own foundation -> build; damaged house -> repair. */
    assert(aoe_unit_task_select_tribe_ror(v, found, -1, -1, &ops, &task) &&
           task == &villager_tasks[2]);
    found->core.state = 2;
    found->core.hit_points = 10.0f;
    assert(aoe_unit_task_select_tribe_ror(v, found, -1, -1, &ops, &task) &&
           task == &villager_tasks[3]);
    /* Drop-off: carrying wood (1) onto own live town center (id 109). */
    v->core.storage.carried_resource_type = 1;
    v->core.storage.carried_amount = 10.0f;
    assert(aoe_unit_task_select_tribe_ror(v, tc, -1, -1, &ops, &task) &&
           task == &villager_tasks[1]);
    /* An empty basket (amount 0) does not deposit: no other task fits. */
    v->core.storage.carried_amount = 0.0f;
    assert(aoe_unit_task_select_tribe_ror(v, tc, -1, -1, &ops, &task) &&
           task == NULL);
    /* Damaged allied ship of class 22 -> class-matched repair. */
    boat->core.hit_points = 50.0f;
    assert(aoe_unit_task_select_tribe_ror(v, boat, -1, -1, &ops, &task) &&
           task == &villager_tasks[7]);
    /* Requires-carried-resource task: deer must carry food. */
    /* Not carrying: the class-11 task is rejected, and the mode-5
       targeting task rejects a gaia target for an actor without
       field_8c. */
    assert(aoe_unit_task_select_base_ror(v, dr, -1, -1, &ops, &task) &&
           task == NULL);
    dr->core.storage.carried_resource_type = 0;
    dr->core.storage.carried_amount = 100.0f;
    assert(aoe_unit_task_select_base_ror(v, dr, -1, -1, &ops, &task) &&
           task == &villager_tasks[4]);
    /* Mode 5 against gaia needs actor field_8c: villager lacks it, so the
       any_flagged_target pass finds nothing for a hostile-free gaia deer. */
    dr->core.storage.carried_resource_type = -1;
    villager.task_count = 7; /* hide ship repair; keeps indices stable */
    assert(aoe_unit_task_allowed_ror(&villager_tasks[6], v, dr, &ops,
                                     &allowed) && !allowed);
    villager.field_8c = 1;
    assert(aoe_unit_task_allowed_ror(&villager_tasks[6], v, dr, &ops,
                                     &allowed) && allowed);
    villager.field_8c = 0;
    /* Terrain pass: ground point (5.9, 6.2) on terrain 2. NaN skips it. */
    assert(aoe_unit_task_select_tribe_ror(v, NULL, 5.9f, 6.2f, &ops, &task) &&
           task == &villager_tasks[5]);
    assert(aoe_unit_task_select_base_ror(v, NULL, NAN, 6.2f, &ops, &task) &&
           task == NULL);
    assert(aoe_unit_task_select_base_ror(v, NULL, 35.0f, 6.0f, &ops,
                                         &task) && task == NULL);

    /* Soldier: mode 2 needs a hostile non-gaia owner. */
    AoeLiveUnit *s = enemy; /* reuse slot as attacker owned by player 1 */
    owners[4] = 1;
    AoeLiveUnit *target = make(1, &town_center, 3, 2);
    assert(aoe_unit_task_select_tribe_ror(s, target, -1, -1, &ops, &task) &&
           task == NULL); /* town center lacks field_8c: no targeting pass */
    town_center.field_8c = 1;
    assert(aoe_unit_task_select_tribe_ror(s, target, -1, -1, &ops, &task) &&
           task == &soldier_tasks[0]);
    owners[1] = 2; /* ally is not hostile */
    assert(aoe_unit_task_select_tribe_ror(s, target, -1, -1, &ops, &task) &&
           task == NULL);
    /* Mutual ally transport (field_8f 7) -> action 3; none here. */
    {
        AoeLiveUnit *t = make(1, &transport, 2, 2);
        assert(aoe_unit_task_select_tribe_ror(s, t, -1, -1, &ops, &task) &&
               task == NULL);
    }

    /* Priest: convert an enemy, heal a damaged own villager, never fall
       back to the base selector. */
    owners[4] = 3;
    assert(aoe_unit_task_select_tribe_ror(p, enemy, -1, -1, &ops, &task) &&
           task == &priest_tasks[0]);
    owners[4] = 4; /* one-way accepted: no conversion, not damaged */
    assert(aoe_unit_task_select_tribe_ror(p, enemy, -1, -1, &ops, &task) &&
           task == NULL);
    v->core.hit_points = 3.0f;
    assert(aoe_unit_task_select_tribe_ror(p, v, -1, -1, &ops, &task) &&
           task == &priest_tasks[1]);
    assert(aoe_unit_task_select_tribe_ror(p, NULL, 5.0f, 6.0f, &ops, &task) &&
           task == NULL);
    /* Self is never a valid predicate target. */
    assert(aoe_unit_task_allowed_ror(&villager_tasks[3], v, v, &ops,
                                     &allowed) && !allowed);
    return 0;
}
