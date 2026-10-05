#include "player_outcome_dispatch.h"

#include <limits.h>

static int64_t signed_32(uint32_t value) {
    return value <= INT32_MAX ? (int64_t)value : (int64_t)value - 0x100000000LL;
}

static int resource_at(AoeOutcomePlayerRef *player, int32_t index, float *value) {
    if (!player || !value || index < 0 || !player->resources50 ||
        (size_t)index >= player->resource_capacity) return 0;
    *value = player->resources50[index];
    return 1;
}

static int resource_whole(AoeOutcomePlayerRef *player, int32_t index,
                          int32_t *whole) {
    float resource;
    if (!whole || !resource_at(player, index, &resource)) return 0;
    /* The original __ftol truncates toward zero before using low 32 bits. */
    if (!(resource >= -2147483648.0f && resource < 2147483648.0f)) return 0;
    *whole = (int32_t)resource;
    return 1;
}

static int event_world(AoeOutcomeManager *manager, AoeOutcomePlayerRef **owner_out,
                       AoeOutcomeWorldRef **world_out) {
    AoeOutcomePlayerRef *owner;
    AoeOutcomeWorldRef *world;
    if (!manager || !owner_out || !world_out || !(owner = manager->owner) ||
        !(world = owner->world3c) || world->player_count <= 0 ||
        owner->index4a < 0 || owner->index4a >= world->player_count ||
        !world->players || world->player_capacity < (size_t)world->player_count)
        return 0;
    *owner_out = owner;
    *world_out = world;
    return 1;
}

int aoe_outcome_event_kind0(AoeOutcomeManager *manager, AoeOutcomeEvent *event) {
    int32_t whole, quotient;
    uint32_t contribution;
    if (!event) return 0;
    if (event->status != 0) return 1;
    if (!manager || !resource_whole(manager->owner, event->resource_index_04,
                                    &whole) || event->divisor_08 == 0) return 0;
    event->sample_14 = (float)whole;
    if (whole == INT32_MIN && event->divisor_08 == -1) return 0;
    quotient = whole / event->divisor_08;
    if (quotient < 0) quotient = 0;
    contribution = (uint32_t)event->weight_0c * (uint32_t)quotient;
    manager->score_sum_1c += contribution - event->contribution_10;
    event->contribution_10 = contribution;
    return 1;
}

int aoe_outcome_event_kind1(AoeOutcomeManager *manager, AoeOutcomeEvent *event) {
    AoeOutcomePlayerRef *owner;
    AoeOutcomeWorldRef *world;
    int32_t whole;
    if (!event) return 0;
    if (event->status != 0) return 1;
    if (!event_world(manager, &owner, &world) ||
        !resource_whole(owner, event->resource_index_04, &whole)) return 0;
    event->sample_14 = (float)whole;
    if (whole < event->target_08) return 1;
    for (int i = 0; i < world->player_count; ++i) {
        float rival;
        if (i == owner->index4a) continue;
        if (!resource_at(world->players[i], event->resource_index_04, &rival)) return 0;
        if ((float)event->target_08 <= rival) {
            event->status = 1;
            return 1;
        }
    }
    manager->score_sum_1c += (uint32_t)event->weight_0c - event->contribution_10;
    event->contribution_10 = (uint32_t)event->weight_0c;
    event->status = 2;
    return 1;
}

int aoe_outcome_event_kind2(AoeOutcomeManager *manager, AoeOutcomeEvent *event) {
    AoeOutcomePlayerRef *owner;
    AoeOutcomeWorldRef *world;
    uint32_t contribution;
    int32_t whole;
    if (!event) return 0;
    if (event->status != 0) return 1;
    if (!event_world(manager, &owner, &world) ||
        !resource_whole(owner, event->resource_index_04, &whole)) return 0;
    event->sample_14 = (float)whole;
    contribution = whole < event->target_08 ? 0 : (uint32_t)event->weight_0c;
    if (whole >= event->target_08) {
        /* The original starts at index 1, excluding neutral player 0. */
        for (int i = 1; i < world->player_count; ++i) {
            float rival;
            if (i == owner->index4a) continue;
            if (!resource_at(world->players[i], event->resource_index_04, &rival)) return 0;
            if (rival > event->sample_14) {
                contribution = 0;
                break;
            }
            if (rival == event->sample_14)
                contribution = event->contribution_10;
        }
    }
    manager->score_sum_1c += contribution - event->contribution_10;
    event->contribution_10 = contribution;
    return 1;
}

int aoe_outcome_event_kind3(AoeOutcomeManager *manager, AoeOutcomeEvent *event) {
    AoeOutcomePlayerRef *owner;
    AoeOutcomeWorldRef *world;
    int32_t whole;
    uint32_t contribution;
    if (!event) return 0;
    if (event->status != 0) return 1;
    if (!event_world(manager, &owner, &world) ||
        !resource_whole(owner, event->resource_index_04, &whole)) return 0;
    if (event->sample_14 < (float)whole) event->sample_14 = (float)whole;
    /* The original excludes neutral player 0 and the owner. */
    for (int i = 1; i < world->player_count; ++i) {
        float rival;
        if (i == owner->index4a) continue;
        if (!resource_at(world->players[i], event->resource_index_04, &rival)) return 0;
        if ((float)event->rival_threshold_08 < rival) {
            int32_t rival_whole;
            if (!resource_whole(world->players[i], event->resource_index_04,
                                &rival_whole)) return 0;
            event->rival_threshold_08 = rival_whole;
        }
    }
    if ((float)event->rival_threshold_08 < event->sample_14)
        contribution = (uint32_t)event->weight_0c;
    else if (whole == event->rival_threshold_08)
        contribution = event->contribution_10;
    else
        contribution = 0;
    manager->score_sum_1c += contribution - event->contribution_10;
    event->contribution_10 = contribution;
    return 1;
}

int aoe_outcome_event_kind4(AoeOutcomeManager *manager, AoeOutcomeEvent *event) {
    float resource;
    int32_t whole, quotient;
    long double ratio;
    uint32_t contribution;
    if (!event) return 0;
    if (event->status != 0) return 1;
    if (!manager || !resource_at(manager->owner, event->resource_index_04,
                                  &resource) || event->divisor_08 == 0) return 0;
    if (!(resource >= -2147483648.0f && resource < 2147483648.0f)) return 0;
    whole = (int32_t)resource;
    if (event->sample_14 < (float)whole) event->sample_14 = (float)whole;
    ratio = (long double)event->sample_14 / (long double)event->divisor_08;
    if (!(ratio >= -2147483648.0L && ratio < 2147483648.0L)) return 0;
    quotient = (int32_t)ratio;
    contribution = (uint32_t)quotient * (uint32_t)event->weight_0c;
    manager->score_sum_1c += contribution - event->contribution_10;
    event->contribution_10 = contribution;
    return 1;
}

int aoe_outcome_dispatch_event(void *user, AoeOutcomeEvent *event) {
    AoeOutcomeDispatcher *dispatch = user;
    if (!dispatch || !event) return 0;
    if (event->kind >= 5) return 1;
    if (event->kind == 0) return aoe_outcome_event_kind0(dispatch->manager, event);
    if (event->kind == 1) return aoe_outcome_event_kind1(dispatch->manager, event);
    if (event->kind == 2) return aoe_outcome_event_kind2(dispatch->manager, event);
    if (event->kind == 3) return aoe_outcome_event_kind3(dispatch->manager, event);
    if (event->kind == 4) return aoe_outcome_event_kind4(dispatch->manager, event);
    return 1;
}

int aoe_outcome_condition_kind0(AoeOutcomeManager *manager,
                                AoeOutcomeCondition *condition) {
    AoeOutcomeUnitRef *unit;
    AoeOutcomePlayerRef *owner, *holder;
    if (!condition) return 0;
    unit = condition->unit_28;
    if (!unit) return 1;
    if (unit->code48 >= 3) {
        condition->unit_28 = 0;
        if (condition->status == 0) condition->status = 1;
        return 1;
    }
    if (condition->status != 0 && condition->status != 2) return 1;
    if (!manager || !(owner = manager->owner) || !(holder = unit->owner0c)) return 0;
    if (holder == owner) {
        if (condition->status == 0) condition->status = 2;
        return 1;
    }
    if (condition->status == 2) {
        /* The original reversion tests only the owner's diplomacy byte. */
        if (!condition->include_allies_2d) {
            condition->status = 0;
            return 1;
        }
        if (holder->index4a < 0 || !owner->eligible84 ||
            (size_t)holder->index4a >= owner->eligible_capacity) return 0;
        if (owner->eligible84[holder->index4a]) condition->status = 0;
        return 1;
    }
    if (condition->include_allies_2d && owner->team_mode) {
        if (holder->index4a < 0 || owner->index4a < 0 ||
            !owner->eligible84 || !holder->eligible84 ||
            (size_t)holder->index4a >= owner->eligible_capacity ||
            (size_t)owner->index4a >= holder->eligible_capacity) return 0;
        if (!owner->eligible84[holder->index4a] &&
            !holder->eligible84[owner->index4a] && holder->team_mode)
            condition->status = 2;
    }
    return 1;
}

int aoe_outcome_condition_kind2(AoeOutcomeCondition *condition) {
    if (!condition) return 0;
    if (condition->unit_28 && condition->unit_28->code48 == 7) {
        condition->status = 2;
        condition->unit_28 = 0;
    }
    return 1;
}

static int count_code7_class(AoeOutcomePlayerRef *player, int32_t class_id,
                             uint32_t *count) {
    AoeOutcomeUnitNode *slow, *fast;
    if (!player || !count) return 0;
    slow = player->units28;
    fast = player->units28;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return 0;
    }
    for (AoeOutcomeUnitNode *node = player->units28; node; node = node->next) {
        if (!node->unit) return 0;
        if ((int32_t)node->unit->class12 == class_id && node->unit->code48 == 7)
            ++*count;
    }
    return 1;
}

static int count_code2_class(AoeOutcomePlayerRef *player,
                             AoeOutcomeCondition *condition,
                             int within_region, uint32_t *count) {
    AoeOutcomeUnitNode *slow, *fast;
    if (!player || !condition || !count) return 0;
    slow = player->units28;
    fast = player->units28;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return 0;
    }
    for (AoeOutcomeUnitNode *node = player->units28; node; node = node->next) {
        AoeOutcomeUnitRef *unit = node->unit;
        if (!unit) return 0;
        if ((int32_t)unit->class12 != condition->unit_class_04 ||
            unit->code48 != 2) continue;
        if (within_region &&
            !(condition->min_x <= unit->x38 && unit->x38 <= condition->max_x &&
              condition->min_y <= unit->y3c && unit->y3c <= condition->max_y))
            continue;
        ++*count;
    }
    return 1;
}

int aoe_outcome_condition_kind1_or_8(AoeOutcomeManager *manager,
                                     AoeOutcomeCondition *condition,
                                     int within_region) {
    AoeOutcomePlayerRef *owner;
    AoeOutcomeWorldRef *world;
    uint32_t count = 0;
    if (!manager || !condition || !(owner = manager->owner)) return 0;
    if (!count_code2_class(owner, condition, within_region, &count)) return 0;
    if (condition->include_allies_2d && owner->team_mode) {
        world = owner->world3c;
        if (!world || world->player_count < 0 || owner->index4a < 0 ||
            owner->index4a >= world->player_count) return 0;
        if (world->player_count > 1 &&
            (!world->players || !owner->eligible84 ||
             world->player_capacity < (size_t)world->player_count ||
             owner->eligible_capacity < (size_t)world->player_count)) return 0;
        /* Both editions start at player 1, excluding neutral player 0. */
        for (int i = 1; i < world->player_count; ++i) {
            AoeOutcomePlayerRef *other;
            if (owner->eligible84[i] || i == owner->index4a) continue;
            other = world->players[i];
            if (!other || !other->eligible84 ||
                other->eligible_capacity <= (size_t)owner->index4a) return 0;
            if (other->eligible84[owner->index4a] || !other->team_mode) continue;
            if (!count_code2_class(other, condition, within_region, &count)) return 0;
        }
    }
    condition->status = signed_32(count) >= condition->unit_target_count_1c ? 2 : 0;
    return 1;
}

int aoe_outcome_condition_kind3(AoeOutcomeManager *manager,
                                AoeOutcomeCondition *condition) {
    uint32_t count = 0;
    int64_t signed_counter;
    AoeOutcomePlayerRef *owner;
    AoeOutcomeWorldRef *world;
    if (!condition) return 0;
    if (condition->status != 0) return 1;
    if (condition->player_08) {
        if (!count_code7_class(condition->player_08,
                               condition->unit_class_04, &count)) return 0;
    } else {
        if (!manager || !(owner = manager->owner) ||
            !(world = owner->world3c) || world->player_count < 0) return 0;
        if (world->player_count > 0 &&
            (!world->players || !owner->eligible84 ||
             world->player_capacity < (size_t)world->player_count ||
             owner->eligible_capacity < (size_t)world->player_count)) return 0;
        for (int i = 0; i < world->player_count; ++i) {
            if (!owner->eligible84[i]) continue;
            if (!count_code7_class(world->players[i], condition->unit_class_04,
                                   &count)) return 0;
        }
    }
    condition->counter_20 += count; /* Original x86 addition wraps at 32 bits. */
    signed_counter = signed_32(condition->counter_20);
    if (signed_counter >= condition->threshold_1c) condition->status = 2;
    return 1;
}

int aoe_outcome_condition_kind4(AoeOutcomeCondition *condition) {
    AoeOutcomeUnitRef *unit;
    if (!condition) return 0;
    unit = condition->unit_24;
    if (!unit) {
        condition->status = 3;
        return 1;
    }
    if (unit->code48 > 2) {
        if (condition->status == 0) condition->status = 1;
        condition->unit_24 = 0;
        return 1;
    }
    if (condition->min_x <= unit->x38 && unit->x38 <= condition->max_x &&
        condition->min_y <= unit->y3c && unit->y3c <= condition->max_y)
        condition->status = 2;
    else
        condition->status = 0;
    return 1;
}

int aoe_outcome_condition_kind5(AoeOutcomeCondition *condition) {
    AoeOutcomeUnitRef *a, *b;
    float dx, dy;
    if (!condition) return 0;
    a = condition->unit_24;
    b = condition->unit_28;
    if (!a || !b) {
        condition->status = 3;
        return 1;
    }
    if (a->code48 >= 3 || b->code48 >= 3) {
        condition->unit_24 = 0;
        condition->unit_28 = 0;
        if (condition->status == 0) condition->status = 1;
        return 1;
    }
    dx = a->x38 - b->x38;
    dy = a->y3c - b->y3c;
    if (dx < 0.0f) dx = -dx;
    if (dy < 0.0f) dy = -dy;
    if (dx <= a->extent_x30 + b->extent_x30 + 1.0f &&
        dy <= a->extent_y34 + b->extent_y34 + 1.0f)
        condition->status = 2;
    else
        condition->status = 0;
    return 1;
}

int aoe_outcome_condition_kind6(AoeOutcomeManager *manager,
                                AoeOutcomeCondition *condition) {
    AoeOutcomePlayerRef *owner;
    AoeOutcomeWorldRef *world;
    float total, other_value;
    int32_t index;
    if (!condition || !manager) return 0;
    owner = manager->owner;
    if (!owner) return 1; /* The original leaves status unchanged. */
    index = condition->resource_index_1c;
    if (!resource_at(owner, index, &total)) return 0;
    if (condition->include_allies_2d && owner->team_mode) {
        world = owner->world3c;
        if (!world || world->player_count < 0 || owner->index4a < 0 ||
            owner->index4a >= world->player_count) return 0;
        if (world->player_count > 0 &&
            (!world->players || !owner->eligible84 ||
             world->player_capacity < (size_t)world->player_count ||
             owner->eligible_capacity < (size_t)world->player_count)) return 0;
        for (int i = world->player_count - 1; i >= 0; --i) {
            AoeOutcomePlayerRef *other;
            if (owner->eligible84[i] || i == owner->index4a) continue;
            other = world->players[i];
            if (!other || !other->eligible84 ||
                other->eligible_capacity <= (size_t)owner->index4a) return 0;
            if (other->eligible84[owner->index4a] || !other->team_mode) continue;
            if (!resource_at(other, index, &other_value)) return 0;
            total += other_value;
        }
    }
    condition->status = (float)condition->resource_target_20 <= total ? 2 : 0;
    return 1;
}

static int progress_percent(AoeOutcomePlayerRef *player, long double *percent) {
    AoeOutcomeProgressRef *progress;
    if (!player || !percent || !(progress = player->progress38) ||
        progress->denominator_20 == 0) return 0;
    *percent = ((long double)progress->numerator_1c /
                (long double)progress->denominator_20) * 100.0L;
    return 1;
}

int aoe_outcome_condition_kind7(AoeOutcomeManager *manager,
                                AoeOutcomeCondition *condition) {
    AoeOutcomePlayerRef *owner;
    AoeOutcomeWorldRef *world;
    long double percent;
    if (!condition || !manager) return 0;
    if (condition->status != 0 || !(owner = manager->owner)) return 1;
    if (!condition->include_allies_2d || !owner->team_mode) {
        if (!progress_percent(owner, &percent)) return 0;
        if ((long double)condition->progress_target_20 <= percent)
            condition->status = 2;
        return 1;
    }
    world = owner->world3c;
    if (!world || world->player_count < 0 || owner->index4a < 0 ||
        owner->index4a >= world->player_count || !world->players ||
        world->player_capacity < (size_t)world->player_count ||
        !owner->eligible84 ||
        owner->eligible_capacity < (size_t)world->player_count) return 0;
    for (int i = world->player_count - 1; i >= 0; --i) {
        AoeOutcomePlayerRef *other = world->players[i];
        if (!other) return 0;
        if (i != owner->index4a) {
            if (owner->eligible84[i]) continue;
            if (!other->eligible84 ||
                other->eligible_capacity <= (size_t)owner->index4a) return 0;
            if (other->eligible84[owner->index4a] || !other->team_mode) continue;
        }
        if (!progress_percent(other, &percent)) return 0;
        if ((long double)condition->progress_target_20 <= percent) {
            condition->status = 2;
            return 1;
        }
    }
    return 1;
}

static int player_has_class(AoeOutcomePlayerRef *player, int32_t class_id,
                            int *found) {
    AoeOutcomeUnitNode *slow, *fast;
    if (!player || !found) return 0;
    slow = player->units28;
    fast = player->units28;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return 0;
    }
    *found = 0;
    for (AoeOutcomeUnitNode *node = player->units28; node; node = node->next) {
        if (!node->unit) return 0;
        if ((int32_t)node->unit->class12 == class_id) {
            *found = 1;
            return 1;
        }
    }
    return 1;
}

int aoe_outcome_condition_kind9(AoeOutcomeManager *manager,
                                AoeOutcomeCondition *condition) {
    int found;
    AoeOutcomePlayerRef *owner;
    AoeOutcomeWorldRef *world;
    if (!condition) return 0;
    if (condition->player_08) {
        if (!player_has_class(condition->player_08,
                              condition->unit_class_04, &found)) return 0;
        condition->status = found ? 0 : 2;
        return 1;
    }
    if (!manager || !(owner = manager->owner) ||
        !(world = owner->world3c) || world->player_count < 0) return 0;
    if (world->player_count > 0 &&
        (!world->players || !owner->eligible84 ||
         world->player_capacity < (size_t)world->player_count ||
         owner->eligible_capacity < (size_t)world->player_count)) return 0;
    for (int i = 0; i < world->player_count; ++i) {
        if (!owner->eligible84[i] || i == owner->index4a) continue;
        if (!player_has_class(world->players[i], condition->unit_class_04,
                              &found)) return 0;
        if (found) {
            condition->status = 0;
            return 1;
        }
    }
    condition->status = 2;
    return 1;
}

int aoe_outcome_condition_kind10(AoeOutcomeCondition *condition) {
    if (!condition) return 0;
    if (condition->player_08)
        condition->status = condition->player_08->state80 == 2 ? 2 : 0;
    return 1;
}

int aoe_outcome_condition_kind12(AoeOutcomeManager *manager,
                                 AoeOutcomeCondition *condition) {
    AoeOutcomePlayerRef *owner;
    AoeOutcomeWorldRef *world;
    uint32_t total, members = 1;
    int64_t average;
    if (!condition || !manager) return 0;
    owner = manager->owner;
    if (!owner) return 1; /* Original leaves status unchanged. */
    total = manager->score_sum_1c;
    if (condition->include_allies_2d && owner->team_mode) {
        world = owner->world3c;
        if (!world || world->player_count < 0 || owner->index4a < 0 ||
            owner->index4a >= world->player_count) return 0;
        if (world->player_count > 0 &&
            (!world->players || !owner->eligible84 ||
             world->player_capacity < (size_t)world->player_count ||
             owner->eligible_capacity < (size_t)world->player_count)) return 0;
        for (int i = world->player_count - 1; i >= 0; --i) {
            AoeOutcomePlayerRef *other;
            if (owner->eligible84[i] || i == owner->index4a) continue;
            other = world->players[i];
            if (!other || !other->eligible84 ||
                other->eligible_capacity <= (size_t)owner->index4a) return 0;
            if (other->eligible84[owner->index4a] || !other->team_mode) continue;
            if (!other->outcome34) return 0;
            total += other->outcome34->score_sum_1c;
            ++members;
        }
    }
    average = signed_32(total) / (int64_t)members;
    if (average >= condition->score_target_20) condition->status = 2;
    return 1;
}

int aoe_outcome_dispatch_condition(void *user, AoeOutcomeCondition *condition) {
    AoeOutcomeDispatcher *dispatch = user;
    unsigned kind;
    if (!dispatch || !condition) return 0;
    kind = condition->kind;
    if (kind > 12 || kind == 11) return 1;
    if (kind == 0) return aoe_outcome_condition_kind0(dispatch->manager, condition);
    if (kind == 1) return aoe_outcome_condition_kind1_or_8(dispatch->manager, condition, 0);
    if (kind == 2) return aoe_outcome_condition_kind2(condition);
    if (kind == 3) return aoe_outcome_condition_kind3(dispatch->manager, condition);
    if (kind == 4) return aoe_outcome_condition_kind4(condition);
    if (kind == 5) return aoe_outcome_condition_kind5(condition);
    if (kind == 6) return aoe_outcome_condition_kind6(dispatch->manager, condition);
    if (kind == 7) return aoe_outcome_condition_kind7(dispatch->manager, condition);
    if (kind == 8) return aoe_outcome_condition_kind1_or_8(dispatch->manager, condition, 1);
    if (kind == 9) return aoe_outcome_condition_kind9(dispatch->manager, condition);
    if (kind == 10) return aoe_outcome_condition_kind10(condition);
    if (kind == 12) return aoe_outcome_condition_kind12(dispatch->manager, condition);
    return 1;
}

int aoe_outcome_event_kind3_patched_ror(AoeOutcomeManager *manager, AoeOutcomeEvent *event) {
    AoeOutcomePlayerRef *owner;
    AoeOutcomeWorldRef *world;
    int32_t whole;
    uint32_t contribution;
    if (!event) return 0;
    if (event->status != 0) return 1;
    if (!event_world(manager, &owner, &world) ||
        !resource_whole(owner, event->resource_index_04, &whole)) return 0;
    if (event->sample_14 < (float)whole) event->sample_14 = (float)whole;
    for (int i = 1; i < world->player_count; ++i) {
        float rival;
        if (i == owner->index4a) continue;
        if (!resource_at(world->players[i], event->resource_index_04, &rival)) return 0;
        if ((float)event->rival_threshold_08 <= rival) {
            int32_t rival_whole;
            if (!resource_whole(world->players[i], event->resource_index_04,
                                &rival_whole)) return 0;
            event->rival_threshold_08 = rival_whole;
        }
    }
    if (event->sample_14 < (float)event->rival_threshold_08)
        contribution = whole == event->rival_threshold_08 ? event->contribution_10 : 0;
    else
        contribution = (uint32_t)event->weight_0c;
    manager->score_sum_1c += contribution - event->contribution_10;
    event->contribution_10 = contribution;
    return 1;
}

int aoe_outcome_condition_kind12_patched_ror(AoeOutcomeManager *manager,
                                             AoeOutcomeCondition *condition) {
    uint8_t before;
    int r;
    if (!condition || !manager) return 0;
    if (!manager->owner) return 1;
    before = condition->status;
    condition->status = 0;
    r = aoe_outcome_condition_kind12(manager, condition);
    if (!r) condition->status = before;
    return r;
}
