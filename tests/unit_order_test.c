#include "unit_order.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const DatUnitTask *task;
    AoeLiveUnit *target;
    int16_t type;
    int retarget_ok;
} FakeAction;

static DatUnit villager, builder, tree, foundation_t;
static DatUnitTask villager_tasks[1], builder_tasks[1];
static AoeLiveUnit unit, tree_unit, site;
static AoeActionList list;
static FakeAction head_action, built;
static char trace[256];
static int construct_ok = 1;

static void mark(const char *s) {
    strncat(trace, s, sizeof(trace) - strlen(trace) - 1);
}
static void expect(const char *s) {
    if (strcmp(trace, s)) {
        fprintf(stderr, "%s != %s\n", trace, s);
        assert(0);
    }
    trace[0] = 0;
}
static int16_t owner_id(void *c, const AoeLiveUnit *u) { (void)c; return u == &tree_unit ? 0 : 1; }
static int accepts(void *c, int16_t p, int16_t o) { (void)c; return p == o; }
static int hostile(void *c, int16_t p, int16_t o) { (void)c; (void)p; return o != 0; }
static int32_t map_size(void *c, const AoeLiveUnit *a, int axis) { (void)c; (void)a; (void)axis; return 64; }
static int terrain(void *c, const AoeLiveUnit *a, int16_t x, int16_t y, uint8_t *t) {
    (void)c; (void)a; (void)x; (void)y;
    *t = 0;
    return 1;
}
static int tribe(void *c, const DatUnit *t) { (void)c; (void)t; return 1; }
static int16_t template_count(void *c, const AoeLiveUnit *u) { (void)c; (void)u; return 3; }
static const DatUnit *owner_template(void *c, const AoeLiveUnit *u, int16_t i) {
    (void)c; (void)u;
    return i == 0 ? &tree : i == 1 ? &villager : &builder;
}
static void change_template(void *c, AoeLiveUnit *u, const DatUnit *t) {
    (void)c;
    u->core.unit_template = t;
    mark(t == &builder ? "swap->builder " : "swap ");
}
static const DatUnitTask *action_task(void *c, void *a) { (void)c; return ((FakeAction *)a)->task; }
static int retarget(void *c, void *a, AoeLiveUnit *t, float x, float y, float z) {
    (void)c; (void)t; (void)x; (void)y; (void)z;
    mark("retarget ");
    return ((FakeAction *)a)->retarget_ok;
}
static int16_t action_type(void *c, void *a) { (void)c; return ((FakeAction *)a)->type; }
static AoeLiveUnit *action_target(void *c, void *a) { (void)c; return ((FakeAction *)a)->target; }
static void *construct(void *c, AoeLiveUnit *u, const AoeActionRouteRor *r,
                       const DatUnitTask *task, AoeLiveUnit *t, float x,
                       float y, float z) {
    char s[32];
    (void)c; (void)u; (void)x; (void)y; (void)z;
    snprintf(s, sizeof(s), "ctor%x/%x/%d ", r->constructor, r->size, r->shape);
    mark(s);
    built.task = task;
    built.target = t;
    built.type = task->action_type;
    return construct_ok ? &built : NULL;
}
static void adopt(void *c, AoeLiveUnit *u, void *a) {
    (void)c; (void)u;
    assert(a == &built);
    mark("adopt ");
}
static void move(void *c, AoeLiveUnit *u, AoeLiveUnit *t, float x, float y,
                 float z) {
    (void)c; (void)u; (void)t; (void)z;
    assert(x == 5.0f && y == 6.0f);
    mark("move ");
}
static int has_group(void *c, const AoeLiveUnit *u) { (void)c; (void)u; return 1; }
static void ack(void *c, AoeLiveUnit *u, int32_t a, int32_t b, int32_t id,
                int32_t cls, float x, float y, float z) {
    char s[48];
    (void)c; (void)u; (void)x; (void)y; (void)z;
    snprintf(s, sizeof(s), "ack%x/%x/%d/%d", a, b, id, cls);
    mark(s);
}
static void destroy(void *c, void *a) { (void)c; (void)a; }
static void state(void *c, void *a, uint8_t s) { (void)c; (void)a; (void)s; }

int main(void) {
    AoeUnitOrderOps ops = {
        {0, owner_id, accepts, hostile, map_size, terrain}, 0, tribe,
        template_count, owner_template, change_template, action_task,
        retarget, action_type, action_target, construct, adopt, move,
        has_group, ack};
    AoeActionRouteRor r;
    int16_t code;
    int32_t a, b;

    /* Routing table spot checks. */
    aoe_action_route_ror(1, 0x65, 1, &r);
    assert(r.constructor == 0x004b13c0 && r.size == 0x40 &&
           r.shape == AOE_ACTION_SHAPE_TARGET_ROR);
    aoe_action_route_ror(1, 0x65, 0, &r);
    assert(r.constructor == 0x004b1450 && r.shape == AOE_ACTION_SHAPE_POINT_ROR);
    aoe_action_route_ror(1, 5, 1, &r); /* deferred to the base factory */
    assert(r.constructor == 0x00402ef0 && r.size == 0x44);
    aoe_action_route_ror(0, 0x65, 1, &r);
    assert(r.shape == AOE_ACTION_SHAPE_NONE_ROR);
    aoe_action_route_ror(0, 7, 0, &r);
    assert(r.constructor == 0x00401290 && r.size == 0x5c &&
           r.shape == AOE_ACTION_SHAPE_ATTACK_POINT_ROR);
    aoe_action_route_ror(1, 3, 0, &r);
    assert(r.constructor == 0x00402440 && r.shape == AOE_ACTION_SHAPE_TARGET_ROR);
    aoe_action_route_ror(1, 0x78, 1, &r);
    assert(r.shape == AOE_ACTION_SHAPE_UNIT_ROR);
    aoe_order_acknowledgement_ror(9, &a, &b);
    assert(a == 0x2bc && b == 0x258);
    aoe_order_acknowledgement_ror(7, &a, &b);
    assert(a == -1 && b == -1);

    /* Templates: a villager without a gather task and a builder (same
       class/type/swap group) with a build task for unbuilt targets. */
    villager.type = builder.type = 70;
    villager.unit_class = builder.unit_class = 4;
    villager.task_swap_group = builder.task_swap_group = 1;
    villager_tasks[0] = (DatUnitTask){.action_type = 5, .target_class = 15,
                                      .target_unit = -1, .target_terrain = -1};
    builder_tasks[0] = (DatUnitTask){.action_type = 0x65, .target_class = -1,
                                     .target_unit = -1, .target_terrain = -1,
                                     .unbuilt_target = 1};
    villager.tasks = villager_tasks; villager.task_count = 1;
    builder.tasks = builder_tasks; builder.task_count = 1;
    tree.unit_class = 15; tree.id = 9;
    foundation_t.type = 80; foundation_t.unit_class = 3; foundation_t.id = 70;
    tree.type = 30; tree.unit_class = 15;
    unit.constructed = 1; unit.core.unit_id = 1; unit.core.state = 2;
    unit.core.unit_template = &villager;
    unit.core.storage.carried_resource_type = -1;
    tree_unit.constructed = 1; tree_unit.core.unit_id = 2;
    tree_unit.core.unit_template = &tree; tree_unit.core.state = 2;
    tree_unit.core.storage.carried_resource_type = -1;
    site.constructed = 1; site.core.unit_id = 3; site.core.state = 0;
    site.core.unit_template = &foundation_t;
    site.core.storage.carried_resource_type = -1;
    list = (AoeActionList){&unit.core.state, NULL, {0, destroy, state}};
    unit.derived_action_list = &list;

    /* 0x00406300: the villager's own template has no task for the site;
       its swap group's builder has. The tree is the villager's own. */
    assert(aoe_unit_task_template_ror(&unit, &site, 5, 6, 0, &ops) == &builder);
    assert(aoe_unit_task_template_ror(&unit, &tree_unit, 5, 6, 0, &ops) == &villager);
    assert(aoe_unit_task_template_ror(&unit, &unit, 5, 6, 0, &ops) == &villager);
    expect("");

    /* Gather a tree: task, tribe list defers 5 to the base target ctor. */
    assert(aoe_unit_order_ror(&unit, &tree_unit, 5, 6, 0, &ops, &code) &&
           code == 5);
    expect("ctor402ef0/44/1 adopt ack2c5/261/2/15");
    /* Order the villager onto a foundation: no task of its own, so it
       switches to the builder template (own owner -> smart build). */
    assert(aoe_unit_order_ror(&unit, &site, 5, 6, 0, &ops, &code) &&
           code == 0x65 && unit.core.unit_template == &builder);
    expect("swap->builder ctor4b13c0/40/1 adopt ack2be/25a/3/3");
    /* A head action from the same task is retargeted instead. */
    head_action = (FakeAction){&builder_tasks[0], &tree_unit, 0x65, 1};
    assert(aoe_action_list_push(&list, &head_action));
    assert(aoe_unit_order_ror(&unit, &site, 5, 6, 0, &ops, &code) &&
           code == 0x65);
    expect("retarget ack2be/25a/3/3");
    /* Failed retarget: a new action is built. */
    head_action.retarget_ok = 0;
    assert(aoe_unit_order_ror(&unit, &site, 5, 6, 0, &ops, &code) &&
           code == 0x65);
    expect("retarget ctor4b13c0/40/1 adopt ack2be/25a/3/3");
    /* Type-9 head already aimed at the target is retargeted and skips
       creation even without a matching task. */
    head_action = (FakeAction){NULL, NULL, 9, 1};
    assert(aoe_unit_order_ror(&unit, NULL, 5, 6, 0, &ops, &code) &&
           code == 9);
    expect("retarget ack2bc/258/-1/-1");
    /* 0x00406670: a swap happens through the select alone too. */
    {
        const DatUnitTask *t = NULL;
        unit.core.unit_template = &villager;
        assert(aoe_unit_select_task_swap_ror(&unit, &site, 5, 6, 0, &ops, &t) &&
               t == &builder_tasks[0] && unit.core.unit_template == &builder);
        expect("swap->builder ");
    }
    /* Ground point with no task: move, result 1. */
    head_action.type = 2;
    assert(aoe_unit_order_ror(&unit, NULL, 5, 6, 0, &ops, &code) && code == 1);
    expect("move ack2c6/262/-1/-1");
    /* Construction failure falls back to moving too. */
    construct_ok = 0;
    head_action.type = 2;
    head_action.task = NULL;
    assert(aoe_unit_order_ror(&unit, &site, 5, 6, 0, &ops, &code) && code == 1);
    expect("ctor4b13c0/40/1 move ack2c6/262/3/3");
    /* Ordering a unit onto itself does nothing. */
    assert(aoe_unit_order_ror(&unit, &unit, 5, 6, 0, &ops, &code) &&
           code == -1);
    expect("");
    /* 0x00406420: the builder's task ID 4 builds a point action, adopted;
       an unknown ID and -1 do nothing. */
    builder_tasks[0].id = 4;
    construct_ok = 1;
    unit.core.unit_template = &builder;
    trace[0] = 0;
    assert(aoe_unit_start_task_ror(&unit, 4, &ops) == 1);
    assert(strstr(trace, "adopt ") != NULL);
    trace[0] = 0;
    assert(aoe_unit_start_task_ror(&unit, 5, &ops) == 0 &&
           aoe_unit_start_task_ror(&unit, -1, &ops) == 0);
    expect("");
    aoe_action_list_clear(&list);
    return 0;
}
