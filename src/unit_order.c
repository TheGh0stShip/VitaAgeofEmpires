#include "unit_order.h"

#include <stddef.h>

typedef struct {
    int16_t type;
    uint32_t target_ctor, point_ctor;
    uint16_t size;
} Pair;

static const Pair tribe_pairs[] = {
    {0x65, 0x004b13c0, 0x004b1450, 0x40}, {0x68, 0x004b1cb0, 0x004b1d40, 0x48},
    {0x69, 0x004b2c00, 0x004b2c80, 0x40}, {0x6a, 0x004b55c0, 0x004b5650, 0x44},
    {0x6b, 0x004b0df0, 0x004b0e20, 0x40}, {0x6c, 0x004b2880, 0x004b2900, 0x44},
    {0x6e, 0x004b32c0, 0x004b3350, 0x44}, {0x6f, 0x004b6520, 0x004b65b0, 0x48}};

static void set(AoeActionRouteRor *r, uint32_t ctor, uint16_t size,
                AoeActionShapeRor shape) {
    r->constructor = ctor;
    r->size = size;
    r->shape = shape;
}

void aoe_action_route_ror(int tribe_list, int16_t action_type, int has_target,
                          AoeActionRouteRor *route) {
    if (!route) return;
    set(route, 0, 0, AOE_ACTION_SHAPE_NONE_ROR);
    if (tribe_list) {
        for (size_t i = 0; i < sizeof(tribe_pairs) / sizeof(tribe_pairs[0]);
             ++i)
            if (tribe_pairs[i].type == action_type) {
                if (has_target)
                    set(route, tribe_pairs[i].target_ctor, tribe_pairs[i].size,
                        AOE_ACTION_SHAPE_TARGET_ROR);
                else
                    set(route, tribe_pairs[i].point_ctor, tribe_pairs[i].size,
                        AOE_ACTION_SHAPE_POINT_ROR);
                return;
            }
        switch (action_type) {
        case 3:
            set(route, 0x00402440, 0x40, AOE_ACTION_SHAPE_TARGET_ROR);
            return;
        case 0x0c:
            set(route, 0x004067d0, 0x40, AOE_ACTION_SHAPE_POINT_ROR);
            return;
        case 0x78:
            set(route, 0x004b7220, 0x40, AOE_ACTION_SHAPE_UNIT_ROR);
            return;
        default:
            break; /* 0x004b4600 defers everything else to 0x004040b0 */
        }
    }
    switch (action_type) {
    case 4:
        set(route, 0x00402b80, 0x40, AOE_ACTION_SHAPE_POINT_ROR);
        break;
    case 5:
        if (has_target)
            set(route, 0x00402ef0, 0x44, AOE_ACTION_SHAPE_TARGET_ROR);
        else
            set(route, 0x00402f80, 0x44, AOE_ACTION_SHAPE_POINT_ROR);
        break;
    case 7:
        if (has_target)
            set(route, 0x00401150, 0x5c, AOE_ACTION_SHAPE_ATTACK_TARGET_ROR);
        else
            set(route, 0x00401290, 0x5c, AOE_ACTION_SHAPE_ATTACK_POINT_ROR);
        break;
    case 0x0a:
        set(route, 0x004020e0, 0x40, AOE_ACTION_SHAPE_POINT_ROR);
        break;
    case 0x15:
        set(route, 0x00404750, 0x40, AOE_ACTION_SHAPE_TASK_ROR);
        break;
    default:
        break;
    }
}

void aoe_order_acknowledgement_ror(int16_t code, int32_t *id_6c0,
                                   int32_t *id_6d0) {
    static const struct {
        int16_t code;
        int16_t a, b;
    } table[] = {{1, 0x2c6, 0x262},    {3, 0x2cd, 0x269},
                 {4, 0x2c1, 0x25d},    {5, 0x2c5, 0x261},
                 {9, 0x2bc, 0x258},    {0x0c, 0x2ca, 0x266},
                 {0x65, 0x2be, 0x25a}, {0x68, 0x2c0, 0x25c},
                 {0x69, 0x2bf, 0x25b}, {0x6a, 0x2ce, 0x26a},
                 {0x6e, 0x2c9, 0x265}, {0x6f, 0x2cb, 0x267}};
    int32_t a = -1, b = -1;
    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); ++i)
        if (table[i].code == code) {
            a = table[i].a;
            b = table[i].b;
            break;
        }
    if (id_6c0) *id_6c0 = a;
    if (id_6d0) *id_6d0 = b;
}

static int select_task(const AoeUnitOrderOps *ops, const DatUnit *list,
                       AoeLiveUnit *unit, AoeLiveUnit *target, float x,
                       float y, const DatUnitTask **task) {
    return ops->tribe_template(ops->context, list)
               ? aoe_unit_task_select_tribe_with_ror(list, unit, target, x, y,
                                                     &ops->task, task)
               : aoe_unit_task_select_base_with_ror(list, unit, target, x, y,
                                                    &ops->task, task);
}

int aoe_unit_order_ror(AoeLiveUnit *unit, AoeLiveUnit *target, float x,
                       float y, float z, const AoeUnitOrderOps *ops,
                       int16_t *code) {
    const DatUnit *own;
    const DatUnitTask *task = NULL;
    AoeActionList *list;
    void *head;
    int handled = 0;
    if (!unit || !code || !ops || !ops->tribe_template ||
        !ops->owner_template_count || !ops->owner_template ||
        !ops->change_template || !ops->action_task || !ops->action_retarget ||
        !ops->action_type || !ops->action_target || !ops->construct ||
        !ops->adopt || !ops->move || !ops->has_group_object ||
        !ops->acknowledge || !(own = unit->core.unit_template) ||
        !(list = unit->derived_action_list) || !aoe_action_list_valid(list))
        return 0;
    *code = -1;
    if (target == unit) return 1;
    if (!select_task(ops, own, unit, target, x, y, &task)) return 0;
    if (!task && own->task_swap_group) {
        int16_t count = ops->owner_template_count(ops->context, unit);
        for (int16_t i = 0; i < count; ++i) {
            const DatUnit *other = ops->owner_template(ops->context, unit, i);
            if (!other || other->unit_class != own->unit_class ||
                other->type != own->type ||
                other->task_swap_group != own->task_swap_group)
                continue;
            if (!select_task(ops, other, unit, target, x, y, &task)) return 0;
            if (task) {
                ops->change_template(ops->context, unit, other);
                break;
            }
        }
    }
    head = list->head ? list->head->action : NULL;
    if (head && task &&
        ops->action_task(ops->context, head) == task &&
        ops->action_retarget(ops->context, head, target, x, y, z)) {
        *code = ops->action_type(ops->context, head);
        handled = 1;
    }
    head = list->head ? list->head->action : NULL;
    if (head && ops->action_target(ops->context, head) == target &&
        ops->action_type(ops->context, head) == 9 &&
        ops->action_retarget(ops->context, head, target, x, y, z)) {
        *code = ops->action_type(ops->context, head);
        handled = 1;
    } else if (!handled) {
        void *action = NULL;
        if (task) {
            AoeActionRouteRor route;
            aoe_action_route_ror(ops->tribe_template(ops->context,
                                                     unit->core.unit_template),
                                 task->action_type, target != NULL, &route);
            if (route.shape != AOE_ACTION_SHAPE_NONE_ROR)
                action = ops->construct(ops->context, unit, &route, task,
                                        target, x, y, z);
        }
        if (action) {
            *code = ops->action_type(ops->context, action);
            ops->adopt(ops->context, unit, action);
        } else {
            ops->move(ops->context, unit, target, x, y, z);
            *code = 1;
        }
    }
    if (ops->has_group_object(ops->context, unit)) {
        int32_t a, b;
        aoe_order_acknowledgement_ror(*code, &a, &b);
        ops->acknowledge(ops->context, unit, a, b,
                         target ? target->core.unit_id : -1,
                         target && target->core.unit_template
                             ? target->core.unit_template->unit_class
                             : -1,
                         x, y, z);
    }
    return 1;
}
