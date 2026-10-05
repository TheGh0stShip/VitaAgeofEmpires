/* Patched RoR 1.0a AI group templates 0x0040c2f0..0x0040cca0. */
#include "ai_group.h"

#include <assert.h>
#include <string.h>

int main(void) {
    static AoeAiGroupRor group;
    AoeAiActionRor action;
    AoeAiTriggerRor trigger;
    AoeAiRuleRor *rule;
    AoeAiCountRangeRor *range;

    assert(aoe_ai_action_code_ror("Move") == 1 &&
           aoe_ai_action_code_ror("Wait") == 8 &&
           aoe_ai_action_code_ror("Gather") == 0);
    assert(aoe_ai_trigger_code_ror("Gather") == 1 &&
           aoe_ai_trigger_code_ror("Time") == 11 &&
           aoe_ai_trigger_code_ror("Move") == 0);

    aoe_ai_group_init_ror(&group);
    assert(group.field_42 == 0xff && group.mode == 1 && group.field_51 == 50);
    assert(group.slots_44[4] == 5 && group.slots_49[0] == 5);
    range = aoe_ai_group_range_ror(&group, 4);
    assert(range && aoe_ai_range_max_ror(range, 5) == 0xfe &&
           aoe_ai_range_max_ror(range, 6) == 0xff &&
           aoe_ai_range_min_ror(range, -1) == 0);
    aoe_ai_range_set_min_ror(range, 2, 3);
    aoe_ai_range_set_max_ror(range, 2, 9);
    assert(range->min[2] == 3 && range->max[2] == 9);
    assert(!aoe_ai_group_range_ror(&group, 5));

    rule = aoe_ai_group_rule_ror(&group, 9);
    assert(rule && !aoe_ai_group_rule_ror(&group, 10));
    assert(rule->triggers[2].a == -1 && aoe_ai_rule_action_ror(rule, 4) &&
           !aoe_ai_rule_action_ror(rule, 5) && !aoe_ai_rule_trigger_ror(rule, 3));
    aoe_ai_action_set_named_ror(&action, 2, "Retreat", 1, 2, 3);
    assert(action.code == 4 && action.field_0 == 2 && action.c == 3);
    for (int i = 0; i < 5; ++i) assert(aoe_ai_rule_add_action_ror(rule, &action));
    assert(!aoe_ai_rule_add_action_ror(rule, &action));
    assert(aoe_ai_group_gather_value_ror(&group, 9) == 2);
    aoe_ai_trigger_set_named_ror(&trigger, "Death", 4, 5, 6, 7, 8);
    assert(aoe_ai_rule_add_trigger_ror(rule, &trigger));
    aoe_ai_trigger_set_named_ror(&trigger, "Gather", 30, 0, 0, 0, 0);
    assert(aoe_ai_rule_add_trigger_ror(rule, &trigger));
    assert(aoe_ai_group_gather_value_ror(&group, 9) == 30);
    assert(aoe_ai_group_gather_value_ror(&group, 11) == 2);
    aoe_ai_trigger_clear_ror(&rule->triggers[1]);
    assert(rule->triggers[1].a == -1 && rule->triggers[1].e == 0);
    aoe_ai_rule_clear_ror(rule);
    assert(!rule->actions[0].code && rule->actions[0].a == 1);

    aoe_ai_group_set_name_ror(&group, "HP Defenders");
    assert(aoe_ai_group_is_hp_ror(&group) && !strcmp(group.name, "HP Defenders"));
    aoe_ai_group_set_name_ror(&group, NULL);
    assert(aoe_ai_group_is_hp_ror(&group));
    for (int i = 0; i < 5; ++i) assert(aoe_ai_group_add_slot_44_ror(&group, 1));
    assert(!aoe_ai_group_add_slot_44_ror(&group, 1));
    group.slots_49[2] = 0;
    group.slots_49[0] = group.slots_49[1] = 3;
    assert(aoe_ai_group_add_slot_49_ror(&group, 4) &&
           aoe_ai_group_slot_49_ror(&group, 2) == 4 &&
           aoe_ai_group_slot_49_ror(&group, 5) == 0 &&
           aoe_ai_group_slot_44_ror(&group, 0) == 1);
    return 0;
}
