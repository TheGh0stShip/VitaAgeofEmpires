#include "unit_order.h"
#include <stdint.h>

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

/* The template among the unit's own and its swap group's that has a task
   for the target (`own` when none). 0 for a host failure. */
static int swap_template(const AoeUnitOrderOps *ops, AoeLiveUnit *unit, AoeLiveUnit *target,
                         float x, float y, const DatUnit **chosen, const DatUnitTask **task) {
    const DatUnit *own = unit->core.unit_template;
    *chosen = own;
    *task = NULL;
    if (!select_task(ops, own, unit, target, x, y, task)) return 0;
    if (!*task && own->task_swap_group) {
        int16_t count = ops->owner_template_count(ops->context, unit);
        for (int16_t i = 0; i < count; ++i) {
            const DatUnit *other = ops->owner_template(ops->context, unit, i);
            if (!other || other->unit_class != own->unit_class || other->type != own->type ||
                other->task_swap_group != own->task_swap_group)
                continue;
            if (!select_task(ops, other, unit, target, x, y, task)) return 0;
            if (*task) {
                *chosen = other;
                break;
            }
        }
    }
    return 1;
}

int aoe_unit_select_task_swap_ror(AoeLiveUnit *unit, AoeLiveUnit *target, float x, float y,
                                  float z, const AoeUnitOrderOps *ops,
                                  const DatUnitTask **task) {
    const DatUnit *chosen;
    (void)z;
    if (!task) return 0;
    *task = NULL;
    if (!unit || !ops || !unit->core.unit_template || !ops->tribe_template ||
        !ops->owner_template_count || !ops->owner_template || !ops->change_template)
        return 0;
    if (!swap_template(ops, unit, target, x, y, &chosen, task)) return 0;
    if (chosen != unit->core.unit_template) ops->change_template(ops->context, unit, chosen);
    return 1;
}

const DatUnit *aoe_unit_task_template_ror(AoeLiveUnit *unit, AoeLiveUnit *target, float x,
                                          float y, float z, const AoeUnitOrderOps *ops) {
    const DatUnit *chosen;
    const DatUnitTask *task;
    (void)z;
    if (!unit || !unit->core.unit_template) return NULL;
    if (target == unit || !unit->core.unit_template->task_swap_group || !ops ||
        !ops->tribe_template || !ops->owner_template_count || !ops->owner_template)
        return unit->core.unit_template;
    if (!swap_template(ops, unit, target, x, y, &chosen, &task)) return unit->core.unit_template;
    return chosen;
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
    if (!aoe_unit_select_task_swap_ror(unit, target, x, y, z, ops, &task)) return 0;
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

int aoe_unit_start_task_ror(AoeLiveUnit *unit, int16_t id, const AoeUnitOrderOps *ops) {
    const DatUnit *t;
    const DatUnitTask *task = NULL;
    AoeActionRouteRor route;
    void *action;
    int i;
    if (!unit || !ops || !ops->construct || !ops->adopt || !ops->tribe_template || id == -1)
        return 0;
    t = unit->core.unit_template;
    if (!t) return 0;
    for (i = 0; i < t->task_count; ++i)
        if (t->tasks[i].id == id) {
            task = &t->tasks[i];
            break;
        }
    if (!task) return 0;
    aoe_action_route_ror(ops->tribe_template(ops->context, t), task->action_type, 0, &route);
    if (route.shape == AOE_ACTION_SHAPE_NONE_ROR) return 0;
    action = ops->construct(ops->context, unit, &route, task, NULL, -1.0f, -1.0f, -1.0f);
    if (!action) return 0;
    ops->adopt(ops->context, unit, action);
    return 1;
}

int aoe_unit_order_class_action_ror(AoeLiveUnit *unit, AoeLiveUnit *target,
                                    int force, int16_t action_type,
                                    const AoeUnitOrderOps *ops) {
    AoeActionList *list;
    const DatUnitTask *task = NULL;
    AoeActionRouteRor route;
    void *head, *action;
    float x, y, z;
    if (!unit || !target || !ops || !ops->action_type || !ops->action_target ||
        !ops->action_retarget || !ops->construct || !ops->adopt ||
        !(list = unit->derived_action_list))
        return 0;
    x = target->core.position.x38;
    y = target->core.position.y3c;
    z = target->core.position.z40;
    head = list->head ? list->head->action : NULL;
    if (!force && head && ops->action_target(ops->context, head) == target &&
        ops->action_type(ops->context, head) == action_type)
        return 1;
    if (head && ops->action_type(ops->context, head) == action_type) {
        (void)ops->action_retarget(ops->context, head, target, x, y, z);
        return 1;
    }
    if (action_type == 0x6a && ops->owner_template && ops->change_template &&
        (!ops->owner_template_count ||
         ops->owner_template_count(ops->context, unit) > 156)) {
        const DatUnit *repairer = ops->owner_template(ops->context, unit, 156);
        if (repairer) ops->change_template(ops->context, unit, repairer);
    }
    if (!aoe_unit_select_task_swap_ror(unit, target, x, y, z, ops, &task) ||
        !task)
        return 0;
    aoe_action_route_ror(1, action_type, 1, &route);
    if (route.shape == AOE_ACTION_SHAPE_NONE_ROR) return 0;
    action = ops->construct(ops->context, unit, &route, task, target, x, y, z);
    if (!action) return 0;
    (void)aoe_action_list_clear(list);
    ops->adopt(ops->context, unit, action);
    return 1;
}

int aoe_unit_order_repair_ror(AoeLiveUnit *unit, AoeLiveUnit *target,
                              const AoeUnitOrderOps *ops, int16_t *code) {
    const DatUnit *t, *tt;
    const DatUnitTask *task = NULL;
    AoeActionList *list;
    void *head, *action;
    float x, y, z;
    int16_t i;
    if (!unit || !code || !ops || !ops->owner_template ||
        !ops->change_template || !ops->action_task || !ops->action_retarget ||
        !ops->action_type || !ops->construct || !ops->adopt ||
        !ops->has_group_object || !ops->acknowledge ||
        !(list = unit->derived_action_list) || !unit->core.unit_template)
        return 0;
    *code = 0;
    if (!target || target == unit) return 1;
    if (!(tt = target->core.unit_template)) return 0;
    x = target->core.position.x38;
    y = target->core.position.y3c;
    z = target->core.position.z40;
    if (unit->core.unit_template->id != 156)
        ops->change_template(ops->context, unit,
                             ops->owner_template(ops->context, unit, 156));
    if (!(t = unit->core.unit_template)) return 0;
    for (i = 0; t->tasks && i < t->task_count; ++i) {
        const DatUnitTask *k = &t->tasks[i];
        if (k->action_type == 0x6a &&
            ((tt->type == 80 && (k->target_class == -1 || k->target_class == 0)) ||
             k->target_class == tt->unit_class)) {
            task = k;
            break;
        }
    }
    head = list->head ? list->head->action : NULL;
    if (head && task && ops->action_task(ops->context, head) == task &&
        ops->action_retarget(ops->context, head, target, x, y, z)) {
        *code = ops->action_type(ops->context, head);
    } else if (task) {
        AoeActionRouteRor route;
        aoe_action_route_ror(ops->tribe_template
                                 ? ops->tribe_template(ops->context, t) : 1,
                             task->action_type, 1, &route);
        action = route.shape == AOE_ACTION_SHAPE_NONE_ROR ? NULL
                 : ops->construct(ops->context, unit, &route, task, target,
                                  x, y, z);
        if (action) {
            *code = ops->action_type(ops->context, action);
            ops->adopt(ops->context, unit, action);
        }
    }
    if (ops->has_group_object(ops->context, unit))
        ops->acknowledge(ops->context, unit, *code == 0x6a ? 0x2ce : -1,
                         *code == 0x6a ? 0x26a : -1, target->core.unit_id,
                         tt->unit_class, x, y, z);
    return 1;
}

static int32_t iabs32(int32_t v) { return v < 0 ? -v : v; }

void aoe_unit_order_by_code_ror(AoeUnitOrderCodeStateRor *st, const AoeUnitOrderCodeOpsRor *o,
                                uint8_t code) {
    if (!st || !o) return;
    o->clear_actions(o->context);
    st->code = code;
    switch (code) {
    case 1:
    case 0xd:
        o->set_graphic(o->context, o->standing_graphic);
        break;
    case 2:
        o->set_graphic(o->context, o->standing_graphic);
        o->slot_54(o->context, 0);
        o->slot_58(o->context, 0);
        st->saved_x = st->saved_y = st->saved_z = -1.0f;
        break;
    case 3:
        if (o->game_mode(o->context) == 1) {
            o->notify(o->context, o->owner, o->owner, 0x1fa, 0, 0, 0);
            o->slot_5c(o->context, 1);
        } else {
            o->notify(o->context, o->owner, o->owner, 0x202, 0x25a, 0, 0);
        }
        o->set_graphic(o->context, o->standing_graphic);
        break;
    case 4:
        if (o->has_target) {
            st->saved_x = o->target_x;
            st->saved_y = o->target_y;
            st->saved_z = o->target_z;
        }
        if (!o->make_move_to_target(o->context, o->record_24, o->record_34)) {
            o->slot_5c(o->context, 0xd);
        } else {
            o->push_action(o->context);
            o->action_hook(o->context, 1);
        }
        break;
    case 6:
        o->set_graphic(o->context, o->record_34);
        break;
    case 7: {
        int32_t tx, ty, ux, uy, dx, dy;
        if (!o->has_target) {
            o->slot_5c(o->context, 0xd);
            return;
        }
        tx = (int32_t)o->target_x; ty = (int32_t)o->target_y;
        ux = (int32_t)o->unit_x; uy = (int32_t)o->unit_y;
        if ((float)(int32_t)o->target_size_x == o->target_size_x &&
            (float)(int32_t)o->target_size_y == o->target_size_y) {
            if ((ty < uy && ux < tx) || (uy < ty && tx < ux)) uy -= 1;
        }
        dx = tx - ux; dy = ty - uy;
        if (iabs32(dx) == iabs32(dy)) {
            int32_t ax, ay, bx, by;
            if (dx > 0 && dy > 0) { ax = tx; ay = ty - 1; bx = tx - 1; by = ty; }
            else if (dx < 0 && dy > 0) { ax = tx - 1; ay = ty - 1; bx = tx + 1; by = ty; }
            else if (dx < 0 && dy == 0) { ax = tx + 1; ay = ty; bx = tx; by = ty + 1; }
            else { ax = tx - 1; ay = ty; bx = tx; by = ty + 1; }
            if (o->find_unit(o->context, o->owner, 0x1b, ax, ay) != -1 &&
                o->find_unit(o->context, o->owner, 0x1b, bx, by) != -1) {
                o->slot_5c(o->context, 0xd);
                return;
            }
        }
        o->set_graphic(o->context, o->record_38);
        if (o->record_has_sound) o->play_sound(o->context);
        break;
    }
    case 0xb:
        if (!o->make_move_to_point(o->context, st->saved_x, st->saved_y, st->saved_z, o->record_34)) {
            o->slot_5c(o->context, 0xd);
        } else {
            o->action_hook(o->context, 1);
            o->push_action(o->context);
        }
        break;
    default:
        break;
    }
}
