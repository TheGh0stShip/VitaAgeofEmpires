#include "building_update.h"
#include <math.h>
#include <stdlib.h>

int aoe_building_terminal_cleanup_ror(AoeBuildingUpdate *b, uint32_t *count) {
    if (!b || !aoe_action_list_valid(b->queue_actions) ||
        (b->queue_status == 4 && !count)) return 0;
    if (b->queue_status != -1) {
        if (b->queue_status == 4) --*count;
        b->queue_status = -1;
    }
    b->queue_count = 0;
    aoe_action_list_clear(b->queue_actions);
    b->queue_revision = (uint8_t)(b->queue_revision + 1);
    return 1;
}

int aoe_building_cleanup_type80_ror(
    AoeBuildingUpdate *b, uint32_t *count,
    void *deallocator_context, void (*deallocate)(void *, void *)) {
    if (!b) return 0;
    if (b->queue_entries &&
        ((!deallocate) || (b->queue_status == 4 && !count))) return 0;
    if (b->queue_actions && !aoe_action_list_valid(b->queue_actions)) return 0;
    if (b->queue_actions && !aoe_action_list_clear(b->queue_actions)) return 0;
    if (!b->queue_entries) return 1;

    deallocate(deallocator_context, b->queue_entries);
    b->queue_entries = NULL;
    b->queue_capacity = 0;
    b->queue_aux_count = 0;
    b->queue_count = 0;
    b->queue_enabled = 1;
    b->queue_revision = 0;
    b->queue_aux_state = 0;
    if (b->queue_status == 4) {
        b->queue_status = -1;
        --*count;
    }
    return 1;
}

int aoe_building_update_ror(AoeBuildingUpdate *b, uint32_t global_word,
                            const AoeBuildingUpdateOps *ops, uint8_t *result) {
    uint8_t angle = 0, parent_result;
    int complete = 0;
    if (!b || !ops || !result || !ops->parent_update ||
        !aoe_building_production_ops_valid(ops->production) ||
        !ops->owner_queue_status_four_count ||
        !aoe_action_list_valid(b->queue_actions)) return 0;
    if (b->state == 0) {
        long double quotient;
        if (b->build_time <= 0 || b->graphic_angles <= 0 ||
            !isfinite(b->elapsed_build_time) || b->elapsed_build_time < 0)
            return 0;
        quotient = (long double)b->graphic_angles * b->elapsed_build_time /
                   b->build_time;
        /* Original helper truncates to signed 64-bit; only AL is used. */
        if (quotient >= 9223372036854775808.0L) return 0;
        angle = (uint8_t)(int64_t)quotient;
        complete = b->elapsed_build_time >= b->build_time;
        if (complete && !ops->set_state) return 0;
        if (angle < b->graphic_angles && angle != b->angle) b->angle = angle;
        if (complete) ops->set_state(ops->context, b, 2);
    }
    b->history_ec = b->history_e8;
    b->history_e8 = b->history_e4;
    b->history_f8 = b->history_f4;
    b->history_f4 = b->history_f0;
    b->history_f0 = global_word;
    parent_result = ops->parent_update(ops->context, b);
    if (b->state > 2)
        aoe_building_terminal_cleanup_ror(b, ops->owner_queue_status_four_count);
    else if (!aoe_building_production_ror(b, ops->owner_queue_status_four_count,
                                         ops->production)) return 0;
    b->history_e4 = b->current_1b0;
    *result = parent_result;
    return 1;
}
