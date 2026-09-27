#include "scenario_timeline.h"

#include <stddef.h>

#if UINTPTR_MAX == UINT32_MAX
_Static_assert(sizeof(AoeScenarioAction) == 48, "PC/Vita action layout");
_Static_assert(offsetof(AoeScenarioAction, target_a) == 0x1c,
               "PC/Vita target A offset");
_Static_assert(offsetof(AoeScenarioAction, next) == 0x2c,
               "PC/Vita next offset");
#endif

int aoe_scenario_timeline_step(AoeScenarioTimeline *timeline,
                               uint32_t game_time_ms,
                               const AoeScenarioTimelineOps *ops) {
    AoeScenarioAction *action;
    AoeScenarioAction *first_due = 0;
    float now_s;
    if (!timeline || !ops || !ops->object_class || !ops->action_pair ||
        !ops->spawn || !ops->action_position || !ops->action_simple)
        return 0;
    if (!timeline->attached) return 1;

    /* PC x87 uses fildll, fmuls, fstps: the integer is not first rounded
       to float. A double intermediate avoids that premature rounding. */
    now_s = (float)((double)game_time_ms * (double)0.001f);
    for (action = timeline->head; action; action = action->next) {
        if (!first_due && timeline->last_time_s < action->time_s)
            first_due = action;
        if (action->target_a &&
            ops->object_class(ops->user, action->target_a) >= 8)
            action->target_a = 0;
        if (action->target_b &&
            ops->object_class(ops->user, action->target_b) >= 8)
            action->target_b = 0;
    }

    for (action = first_due; action && action->time_s <= now_s;
         action = action->next) {
        switch (action->kind) {
        case 0:
            if (action->target_a && action->target_b)
                ops->action_pair(ops->user, action->target_a,
                                 action->target_b);
            break;
        case 1: {
            void *created = 0;
            if (ops->spawn(ops->user, action->player_index,
                           action->template_index, action->arg_a,
                           action->arg_b, &created)) {
                for (AoeScenarioAction *ref = timeline->head; ref;
                     ref = ref->next) {
                    if (ref->target_a_id == action->target_a_id)
                        ref->target_a = created;
                    if (ref->target_b_id == action->target_a_id)
                        ref->target_b = created;
                }
            }
            break;
        }
        case 2:
            if (action->target_a)
                ops->action_position(ops->user, action->target_a,
                                     action->arg_a, action->arg_b,
                                     action->arg_c);
            break;
        case 4:
            if (action->target_a)
                ops->action_simple(ops->user, action->target_a);
            break;
        }
    }
    timeline->last_time_s = now_s;
    return 1;
}
