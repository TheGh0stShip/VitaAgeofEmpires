#include "unit_task.h"

#include <math.h>
#include <stddef.h>

#include "aoe_ftol.h"

static int ops_valid(const AoeUnitTaskOps *ops) {
    return ops && ops->owner_id && ops->owner_accepts && ops->owner_hostile &&
           ops->map_size && ops->terrain_at;
}

static const DatUnit *tmpl(const AoeLiveUnit *unit) {
    return unit ? unit->core.unit_template : NULL;
}

int aoe_unit_task_allowed_ror(const DatUnitTask *task,
                              const AoeLiveUnit *actor,
                              const AoeLiveUnit *target,
                              const AoeUnitTaskOps *ops, int *allowed) {
    int16_t actor_owner, target_owner;
    if (!task || !actor || !target || !allowed || !ops_valid(ops) ||
        !tmpl(actor))
        return 0;
    *allowed = 0;
    if (task->requires_carried_resource == 1 &&
        (target->core.storage.carried_resource_type == -1 ||
         !(target->core.storage.carried_amount > 0.0f)))
        return 1;
    actor_owner = ops->owner_id(ops->context, actor);
    target_owner = ops->owner_id(ops->context, target);
    switch (task->owner_mode) {
    case 1:
        if (target_owner != actor_owner) return 1;
        break;
    case 2:
        if (target_owner == actor_owner || target_owner == 0 ||
            !ops->owner_hostile(ops->context, actor_owner, target_owner))
            return 1;
        break;
    case 3:
        if (target_owner != 0) return 1;
        break;
    case 4:
        if (target_owner == actor_owner) {
            *allowed = 1;
            return 1;
        }
        if (target_owner == 0 ||
            !ops->owner_accepts(ops->context, actor_owner, target_owner) ||
            !ops->owner_accepts(ops->context, target_owner, actor_owner))
            return 1;
        *allowed = 1;
        return 1;
    case 5:
        if (target_owner == actor_owner) return 1;
        if (target_owner != 0 &&
            !ops->owner_hostile(ops->context, actor_owner, target_owner))
            return 1;
        if (target_owner == 0 && !tmpl(actor)->field_8c) return 1;
        break;
    default:
        break;
    }
    *allowed = actor != target;
    return 1;
}

typedef enum { MATCH_UNBUILT, MATCH_UNIT, MATCH_CLASS, MATCH_TARGETING } Pass;

static int pass(const DatUnit *a, const AoeLiveUnit *actor,
                const AoeLiveUnit *target, Pass kind,
                const AoeUnitTaskOps *ops, const DatUnitTask **task) {
    const DatUnit *t = tmpl(target);
    for (uint16_t i = 0; i < a->task_count; ++i) {
        const DatUnitTask *candidate = &a->tasks[i];
        int match = 0, allowed;
        switch (kind) {
        case MATCH_UNBUILT: match = candidate->unbuilt_target == 1; break;
        case MATCH_UNIT: match = candidate->target_template == t->id; break;
        case MATCH_CLASS: match = candidate->target_class == t->unit_class; break;
        case MATCH_TARGETING: match = candidate->any_flagged_target != 0; break;
        }
        if (!match) continue;
        if (!aoe_unit_task_allowed_ror(candidate, actor, target, ops,
                                       &allowed))
            return 0;
        if (allowed) {
            *task = candidate;
            return 1;
        }
    }
    return 1;
}

int aoe_unit_task_select_base_with_ror(const DatUnit *a,
                                       const AoeLiveUnit *actor,
                                       const AoeLiveUnit *target, float x,
                                       float y, const AoeUnitTaskOps *ops,
                                       const DatUnitTask **task) {
    if (!a || !tmpl(actor) || !task || !ops_valid(ops) || (a->task_count && !a->tasks) ||
        (target && !tmpl(target)))
        return 0;
    *task = NULL;
    if (!a->task_count) return 1;
    if (target) {
        if (target->core.state == 0 &&
            (!pass(a, actor, target, MATCH_UNBUILT, ops, task) || *task))
            return 1;
        if (!pass(a, actor, target, MATCH_UNIT, ops, task) || *task) return 1;
        if (!pass(a, actor, target, MATCH_CLASS, ops, task) || *task) return 1;
        if (target->core.state < 3 && tmpl(target)->field_8c &&
            (!pass(a, actor, target, MATCH_TARGETING, ops, task) || *task))
            return 1;
    }
    /* fcomp; test ah,0x40: equal or unordered skips the terrain pass. */
    if (isnan(x) || isnan(y) || x == -1.0f || y == -1.0f) return 1;
    {
        int16_t tx = (int16_t)aoe_ftol32(x), ty = (int16_t)aoe_ftol32(y);
        uint8_t terrain;
        if (tx < 0 || tx >= ops->map_size(ops->context, actor, 0) ||
            ty < 0 || ty >= ops->map_size(ops->context, actor, 1))
            return 1;
        if (!ops->terrain_at(ops->context, actor, tx, ty, &terrain)) return 0;
        for (uint16_t i = 0; i < a->task_count; ++i)
            if ((uint16_t)a->tasks[i].target_terrain == terrain) {
                *task = &a->tasks[i];
                return 1;
            }
    }
    return 1;
}

static const DatUnitTask *first_action(const DatUnit *a, int16_t action) {
    for (uint16_t i = 0; i < a->task_count; ++i)
        if (a->tasks[i].action_type == action) return &a->tasks[i];
    return NULL;
}

static int damaged(const AoeLiveUnit *unit) {
    return (double)unit->core.hit_points <
           (double)tmpl(unit)->hit_points;
}

int aoe_unit_task_select_tribe_with_ror(const DatUnit *list,
                                        const AoeLiveUnit *actor,
                                        const AoeLiveUnit *target, float x,
                                        float y, const AoeUnitTaskOps *ops,
                                        const DatUnitTask **task) {
    const DatUnit *a = tmpl(actor), *t;
    int16_t carried, actor_owner, target_owner;
    int same, accepted, mutual;
    if (!a || !list || !task || !ops_valid(ops) ||
        (list->task_count && !list->tasks) ||
        (target && !tmpl(target)))
        return 0;
    *task = NULL;
    t = tmpl(target);
    carried = actor->core.storage.carried_resource_type;
    actor_owner = ops->owner_id(ops->context, actor);
    target_owner = target ? ops->owner_id(ops->context, target) : 0;

    if (target && carried != -1 &&
        actor->core.storage.carried_amount > 0.0f &&
        target->core.state == 2 &&
        (a->unit_class == 4 || a->unit_class == 21)) {
        int16_t first = -1, second = -1;
        if (a->unit_class == 4) {
            if (carried == 1 || carried == 2 || carried == 3 ||
                carried == 15 || carried == 17) {
                first = 109; second = 103;
            } else if (carried == 16) {
                first = 109; second = 68;
            }
        } else if (carried == 9 || carried == 17) {
            first = 45;
        }
        if ((t->id == first || t->id == second) &&
            target_owner == actor_owner)
            for (uint16_t i = 0; i < list->task_count; ++i) {
                const DatUnitTask *c = &list->tasks[i];
                if (c->carry_resource == carried &&
                    (c->action_type == 5 || c->action_type == 0x6e ||
                     c->action_type == 0x6f)) {
                    *task = c;
                    return 1;
                }
            }
    }

    if (target) {
        same = target_owner == actor_owner;
        accepted = same ||
                   ops->owner_accepts(ops->context, actor_owner, target_owner);
        mutual = same ||
                 (ops->owner_accepts(ops->context, actor_owner, target_owner) &&
                  ops->owner_accepts(ops->context, target_owner, actor_owner));
        if (same && t->type == 80 && target->core.state == 0) {
            *task = first_action(list, 0x65);
            return 1;
        }
        if (mutual && target->core.state < 3 && t->field_8f == 7) {
            *task = first_action(list, 3);
            return 1;
        }
        if (accepted && t->type == 80 && damaged(target) &&
            target->core.state < 3 && t->id != 50) {
            for (uint16_t i = 0; i < list->task_count; ++i)
                if (list->tasks[i].action_type == 0x6a &&
                    (list->tasks[i].target_class == -1 ||
                     list->tasks[i].target_class == 0)) {
                    *task = &list->tasks[i];
                    break;
                }
            return 1;
        }
        if (mutual &&
            (t->unit_class == 2 || t->unit_class == 20 ||
             t->unit_class == 21 || t->unit_class == 22) &&
            damaged(target) && target->core.state < 3) {
            for (uint16_t i = 0; i < list->task_count; ++i)
                if (list->tasks[i].action_type == 0x6a &&
                    list->tasks[i].target_class == t->unit_class) {
                    *task = &list->tasks[i];
                    break;
                }
            return 1;
        }
    }

    if (a->unit_class == 18) {
        if (target) {
            int16_t c = t->unit_class;
            if (target_owner != actor_owner && target->core.state < 3 &&
                target_owner > 0 &&
                !ops->owner_accepts(ops->context, actor_owner, target_owner) &&
                c != 1) {
                *task = first_action(list, 0x68);
                return 1;
            }
            if ((target_owner == actor_owner ||
                 ops->owner_accepts(ops->context, actor_owner, target_owner)) &&
                damaged(target) && target->core.state < 3 && c != 21 &&
                c != 2 && c != 20 && c != 22 && c != 1 && c != 27 && c != 3)
                *task = first_action(list, 0x69);
        }
        return 1;
    }
    return aoe_unit_task_select_base_with_ror(list, actor, target, x, y, ops,
                                             task);
}

int aoe_unit_task_select_base_ror(const AoeLiveUnit *actor,
                                  const AoeLiveUnit *target, float x,
                                  float y, const AoeUnitTaskOps *ops,
                                  const DatUnitTask **task) {
    return aoe_unit_task_select_base_with_ror(tmpl(actor), actor, target, x,
                                              y, ops, task);
}

int aoe_unit_task_select_tribe_ror(const AoeLiveUnit *actor,
                                   const AoeLiveUnit *target, float x,
                                   float y, const AoeUnitTaskOps *ops,
                                   const DatUnitTask **task) {
    return aoe_unit_task_select_tribe_with_ror(tmpl(actor), actor, target, x,
                                               y, ops, task);
}
