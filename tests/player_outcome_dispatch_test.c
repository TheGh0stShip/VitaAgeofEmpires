#include "player_outcome_dispatch.h"

#include <assert.h>
#include <string.h>

int main(void) {
    AoeOutcomeDispatcher dispatch = {0};
    AoeOutcomeEvent event = {0};
    AoeOutcomeCondition condition = {0};
    AoeOutcomeManager manager = {0};
    AoeOutcomeOps ops = {&dispatch, aoe_outcome_dispatch_event,
                         aoe_outcome_dispatch_condition};
    uint8_t result = 0;
    dispatch.manager = &manager;

    for (unsigned kind = 0; kind < 5; ++kind) {
        event.kind = (uint8_t)kind;
        event.status = 1;
        assert(aoe_outcome_dispatch_event(&dispatch, &event));
    }
    event.kind = 5;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));

    AoeOutcomePlayerRef empty_target = {0};
    for (unsigned kind = 0; kind <= 12; ++kind) {
        memset(&condition, 0, sizeof(condition));
        condition.kind = (uint8_t)kind;
        condition.player_08 = kind == 3 || kind == 9 ? &empty_target : 0;
        manager.owner = kind == 1 || kind == 8 ? &empty_target : 0;
        assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    }
    manager.owner = 0;
    condition.kind = 9;
    assert(!aoe_outcome_dispatch_condition(&dispatch, &condition));
    condition.kind = 3;
    condition.status = 0;
    assert(!aoe_outcome_dispatch_condition(&dispatch, &condition));
    condition.kind = 13;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));

    AoeOutcomeUnitRef unit = {.code48 = 7};
    AoeOutcomePlayerRef player = {2};
    condition.kind = 2;
    condition.status = 0;
    condition.unit_28 = &unit;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2 && condition.unit_28 == 0);
    condition.kind = 10;
    condition.status = 0;
    condition.player_08 = &player;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2);
    player.state80 = 1;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);

    AoeOutcomeUnitRef located = {.code48 = 0, .x38 = 5.0f, .y3c = 7.0f};
    condition.kind = 4;
    condition.unit_24 = &located;
    condition.min_x = 0.0f;
    condition.max_x = 10.0f;
    condition.min_y = 3.0f;
    condition.max_y = 8.0f;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2);
    located.x38 = 11.0f;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);
    located.code48 = 3;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 1 && condition.unit_24 == 0);
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 3);

    AoeOutcomeUnitRef a = {.code48 = 0, .x38 = 0.0f, .y3c = 0.0f,
                           .extent_x30 = 1.0f, .extent_y34 = 1.0f};
    AoeOutcomeUnitRef b = {.code48 = 0, .x38 = 3.0f, .y3c = 3.0f,
                           .extent_x30 = 1.0f, .extent_y34 = 1.0f};
    condition.kind = 5;
    condition.status = 0;
    condition.unit_24 = &a;
    condition.unit_28 = &b;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2); /* Inclusive sum of extents plus one. */
    b.x38 = 3.01f;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);
    b.code48 = 3;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 1 && !condition.unit_24 && !condition.unit_28);
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 3);

    AoeOutcomeUnitRef class_unit = {.class12 = 5};
    AoeOutcomeUnitNode class_node = {&class_unit, 0};
    AoeOutcomePlayerRef target = {.units28 = &class_node};
    condition.kind = 9;
    condition.unit_class_04 = 5;
    condition.player_08 = &target;
    condition.status = 2;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);
    target.units28 = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2);

    AoeOutcomeUnitRef counted_a = {.class12 = 5, .code48 = 7};
    AoeOutcomeUnitRef counted_b = {.class12 = 5, .code48 = 7};
    AoeOutcomeUnitRef not_counted = {.class12 = 5, .code48 = 2};
    AoeOutcomeUnitNode counted_node_c = {&not_counted, 0};
    AoeOutcomeUnitNode counted_node_b = {&counted_b, &counted_node_c};
    AoeOutcomeUnitNode counted_node_a = {&counted_a, &counted_node_b};
    target.units28 = &counted_node_a;
    condition.kind = 3;
    condition.status = 0;
    condition.player_08 = &target;
    condition.unit_class_04 = 5;
    condition.threshold_1c = 3;
    condition.counter_20 = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0 && condition.counter_20 == 2);
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2 && condition.counter_20 == 4);
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.counter_20 == 4); /* Completed conditions do not recount. */
    condition.status = 0;
    condition.counter_20 = UINT32_MAX;
    condition.threshold_1c = 1;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.counter_20 == 1 && condition.status == 2);
    counted_node_c.next = &counted_node_a;
    condition.status = 0;
    assert(!aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.counter_20 == 1);
    counted_node_c.next = 0;

    AoeOutcomePlayerRef owner = {.index4a = 0};
    AoeOutcomePlayerRef other = {.index4a = 1, .units28 = &class_node};
    AoeOutcomePlayerRef *players[2] = {&owner, &other};
    uint8_t eligible[2] = {1, 1};
    AoeOutcomeWorldRef world = {2, players, 2};
    owner.world3c = &world;
    owner.eligible84 = eligible;
    owner.eligible_capacity = 2;
    manager.owner = &owner;
    dispatch.manager = &manager;
    condition.kind = 9;
    condition.player_08 = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);
    eligible[1] = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2);

    owner.units28 = &counted_node_a;
    other.units28 = 0;
    condition.kind = 3;
    condition.status = 0;
    condition.counter_20 = 0;
    condition.threshold_1c = 2;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.counter_20 == 2 && condition.status == 2);
    eligible[0] = 0;
    condition.status = 0;
    condition.counter_20 = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.counter_20 == 0 && condition.status == 0);

    float owner_resources[2] = {0.0f, 100.0f};
    float other_resources[2] = {0.0f, 50.0f};
    uint8_t other_eligible[2] = {0, 0};
    owner.resources50 = owner_resources;
    owner.resource_capacity = 2;
    owner.team_mode = 1;
    other.resources50 = other_resources;
    other.resource_capacity = 2;
    other.team_mode = 1;
    other.eligible84 = other_eligible;
    other.eligible_capacity = 2;
    event.kind = 0;
    event.status = 0;
    event.resource_index_04 = 1;
    event.divisor_08 = 10;
    event.weight_0c = 3;
    event.contribution_10 = 0;
    owner_resources[1] = 100.75f;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.sample_14 == 100.0f && event.contribution_10 == 30);
    assert(manager.score_sum_1c == 30);
    owner_resources[1] = 85.5f;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.sample_14 == 85.0f && event.contribution_10 == 24);
    assert(manager.score_sum_1c == 24); /* Replace prior contribution. */
    owner_resources[1] = -3.5f;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.sample_14 == -3.0f && event.contribution_10 == 0);
    assert(manager.score_sum_1c == 0);
    event.status = 1;
    owner_resources[1] = 100.0f;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(manager.score_sum_1c == 0); /* Completed event is not sampled. */
    event.status = 0;
    event.divisor_08 = 0;
    assert(!aoe_outcome_dispatch_event(&dispatch, &event));
    event.divisor_08 = 10;
    event.kind = 4;
    event.status = 0;
    event.weight_0c = 2;
    event.contribution_10 = 0;
    event.sample_14 = 0.0f;
    owner_resources[1] = 85.5f;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.sample_14 == 85.0f && event.contribution_10 == 16);
    assert(manager.score_sum_1c == 16);
    owner_resources[1] = 50.0f;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.sample_14 == 85.0f && manager.score_sum_1c == 16);
    owner_resources[1] = 100.75f;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.sample_14 == 100.0f && event.contribution_10 == 20);
    assert(manager.score_sum_1c == 20);
    owner_resources[1] = 100.0f;
    memset(&event, 0, sizeof(event));
    manager.score_sum_1c = 0;
    event.kind = 1;
    event.resource_index_04 = 1;
    event.divisor_08 = 90;
    event.weight_0c = 7;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.status == 2 && event.sample_14 == 100.0f);
    assert(event.contribution_10 == 7 && manager.score_sum_1c == 7);
    other_resources[1] = 100.0f;
    event.status = 0;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.status == 1 && manager.score_sum_1c == 7);
    other_resources[1] = 50.0f;
    memset(&event, 0, sizeof(event));
    manager.score_sum_1c = 0;
    event.kind = 2;
    event.resource_index_04 = 1;
    event.divisor_08 = 90;
    event.weight_0c = 10;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.status == 0 && event.contribution_10 == 10);
    assert(manager.score_sum_1c == 10);
    other_resources[1] = 100.0f;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.contribution_10 == 10 && manager.score_sum_1c == 10);
    other_resources[1] = 100.5f;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.contribution_10 == 0 && manager.score_sum_1c == 0);
    owner_resources[1] = 85.0f;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.sample_14 == 85.0f && event.contribution_10 == 0);
    owner_resources[1] = 100.0f;
    other_resources[1] = 50.0f;
    memset(&event, 0, sizeof(event));
    manager.score_sum_1c = 0;
    event.kind = 3;
    event.resource_index_04 = 1;
    event.rival_threshold_08 = 90;
    event.weight_0c = 10;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.sample_14 == 100.0f && event.rival_threshold_08 == 90);
    assert(event.contribution_10 == 10 && manager.score_sum_1c == 10);
    other_resources[1] = 120.75f;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.rival_threshold_08 == 120 && event.contribution_10 == 0);
    assert(manager.score_sum_1c == 0);
    owner_resources[1] = 130.0f;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.sample_14 == 130.0f && event.contribution_10 == 10);
    assert(manager.score_sum_1c == 10);
    owner_resources[1] = 110.0f;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.sample_14 == 130.0f && manager.score_sum_1c == 10);
    other_resources[1] = 130.0f;
    assert(aoe_outcome_dispatch_event(&dispatch, &event));
    assert(event.rival_threshold_08 == 130 && event.contribution_10 == 0);
    /* Patched 1.0a: a best sample equal to the threshold earns the weight. */
    assert(aoe_outcome_event_kind3_patched_ror(&manager, &event));
    assert(event.rival_threshold_08 == 130 && event.contribution_10 == 10);
    other_resources[1] = 140.0f;
    owner_resources[1] = 140.0f;
    assert(aoe_outcome_event_kind3_patched_ror(&manager, &event));
    assert(event.rival_threshold_08 == 140 && event.contribution_10 == 10); /* the weight on a tie */
    owner_resources[1] = 100.0f;
    other_resources[1] = 50.0f;
    AoeOutcomeManager other_manager = {0};
    other.outcome34 = &other_manager;
    manager.score_sum_1c = 100;
    other_manager.score_sum_1c = 200;
    condition.kind = 12;
    condition.include_allies_2d = 1;
    condition.score_target_20 = 150;
    condition.status = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2); /* (100 + 200) / 2. */
    condition.score_target_20 = 151;
    condition.status = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);
    condition.include_allies_2d = 0;
    condition.status = 2;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2); /* Falling below target does not reset. */
    assert(aoe_outcome_condition_kind12_patched_ror(&manager, &condition));
    assert(condition.status == 0); /* The patched body resets it. */
    condition.score_target_20 = 100;
    assert(aoe_outcome_condition_kind12_patched_ror(&manager, &condition) && condition.status == 2);
    condition.score_target_20 = 151;
    condition.status = 2;
    manager.score_sum_1c = 0;
    condition.kind = 6;
    condition.resource_index_1c = 1;
    condition.resource_target_20 = 125;
    condition.include_allies_2d = 1;
    condition.status = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2); /* 100 + mutually allied 50. */
    other_eligible[0] = 1;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0); /* One-way alliance does not count. */
    other_eligible[0] = 0;
    other.team_mode = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);
    other.team_mode = 1;
    condition.include_allies_2d = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);
    condition.resource_target_20 = 100;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2); /* Inclusive threshold and re-evaluation. */
    condition.resource_index_1c = 2;
    assert(!aoe_outcome_dispatch_condition(&dispatch, &condition));

    AoeOutcomeUnitRef owner_inside = {.class12 = 5, .code48 = 2,
                                      .x38 = 3.0f, .y3c = 4.0f};
    AoeOutcomeUnitRef owner_outside = {.class12 = 5, .code48 = 2,
                                       .x38 = 11.0f, .y3c = 4.0f};
    AoeOutcomeUnitRef other_inside = {.class12 = 5, .code48 = 2,
                                      .x38 = 10.0f, .y3c = 10.0f};
    AoeOutcomeUnitNode owner_outside_node = {&owner_outside, 0};
    AoeOutcomeUnitNode owner_inside_node = {&owner_inside, &owner_outside_node};
    AoeOutcomeUnitNode other_inside_node = {&other_inside, 0};
    owner.units28 = &owner_inside_node;
    other.units28 = &other_inside_node;
    eligible[1] = 0;
    condition.unit_class_04 = 5;
    condition.unit_target_count_1c = 3;
    condition.include_allies_2d = 1;
    condition.kind = 1;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2);
    other_eligible[0] = 1;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);
    other_eligible[0] = 0;
    condition.kind = 8;
    condition.min_x = 0.0f;
    condition.max_x = 10.0f;
    condition.min_y = 0.0f;
    condition.max_y = 10.0f;
    condition.unit_target_count_1c = 2;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2); /* Inclusive map bounds, one unit per player. */
    other_inside.x38 = 10.01f;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);
    condition.include_allies_2d = 0;
    condition.unit_target_count_1c = 1;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2);
    owner_inside_node.next = &owner_inside_node;
    assert(!aoe_outcome_dispatch_condition(&dispatch, &condition));
    owner_inside_node.next = &owner_outside_node;

    AoeOutcomeUnitRef tracked = {.code48 = 2, .owner0c = &owner};
    condition.kind = 0;
    condition.unit_28 = &tracked;
    condition.status = 0;
    condition.include_allies_2d = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2);
    tracked.owner0c = &other;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);
    condition.include_allies_2d = 1;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2); /* Mutually eligible team-mode holder. */
    other.team_mode = 0;
    other_eligible[0] = 1;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2); /* Reversion ignores holder's flags. */
    eligible[1] = 1;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);
    eligible[1] = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0); /* Acquisition requires mutual eligibility. */
    other.team_mode = 1;
    other_eligible[0] = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2);
    tracked.code48 = 3;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2 && condition.unit_28 == 0);
    condition.status = 0;
    condition.unit_28 = &tracked;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 1 && condition.unit_28 == 0);

    AoeOutcomeProgressRef owner_progress = {20, 100};
    AoeOutcomeProgressRef other_progress = {30, 100};
    owner.progress38 = &owner_progress;
    other.progress38 = &other_progress;
    condition.kind = 7;
    condition.status = 0;
    condition.progress_target_20 = 25;
    condition.include_allies_2d = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);
    condition.include_allies_2d = 1;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2); /* Allied player reached 30%. */
    condition.status = 0;
    other.team_mode = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);
    other.team_mode = 1;
    other_eligible[0] = 1;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);
    other_eligible[0] = 0;
    eligible[1] = 1;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 0);
    eligible[1] = 0;
    assert(aoe_outcome_dispatch_condition(&dispatch, &condition));
    assert(condition.status == 2);
    condition.status = 0;
    condition.include_allies_2d = 0;
    owner_progress.denominator_20 = 0;
    assert(!aoe_outcome_dispatch_condition(&dispatch, &condition));
    owner_progress.denominator_20 = 100;
    owner_progress.numerator_1c = 30;
    condition.group = 1;
    manager.conditions = &condition;
    assert(aoe_outcome_manager_update(&manager, &ops, &result));
    assert(result == 2 && condition.status == 2);
    return 0;
}
