#include "ai_group.h"

#include <string.h>

static uint8_t lookup(const char *name, const char *const *names, int count) {
    if (!name) return 0;
    for (int i = 0; i < count; ++i)
        if (!strcmp(name, names[i])) return (uint8_t)(i + 1);
    return 0;
}

uint8_t aoe_ai_action_code_ror(const char *name) {
    static const char *const names[] = {
        "Move", "Attack", "AttackSavedTarget", "Retreat", "Heal",
        "ResetHitPoints", "ResetAliveCount", "Wait"};
    return lookup(name, names, 8);
}

uint8_t aoe_ai_trigger_code_ror(const char *name) {
    static const char *const names[] = {
        "Gather", "Death", "HealthLost", "DamageToGroup", "DamageToAnyGroup",
        "Level1EnemySighted", "Level2EnemySighted", "DeathOfGroup",
        "HealOfGroup", "Immediate", "Time"};
    return lookup(name, names, 11);
}

void aoe_ai_action_clear_ror(AoeAiActionRor *action) {
    if (action) aoe_ai_action_set_ror(action, 0, 0, 0, 0, 0);
}

void aoe_ai_action_set_ror(AoeAiActionRor *action, uint8_t field_0,
                           uint8_t code, int32_t a, int32_t b, int32_t c) {
    if (!action) return;
    action->field_0 = field_0;
    action->code = code;
    action->a = a;
    action->b = b;
    action->c = c;
}

void aoe_ai_action_set_named_ror(AoeAiActionRor *action, uint8_t field_0,
                                 const char *name, int32_t a, int32_t b,
                                 int32_t c) {
    aoe_ai_action_set_ror(action, field_0, aoe_ai_action_code_ror(name), a,
                          b, c);
}

void aoe_ai_trigger_clear_ror(AoeAiTriggerRor *trigger) {
    if (!trigger) return;
    trigger->code = 0;
    trigger->a = trigger->b = -1;
    trigger->c = trigger->d = 0;
}

void aoe_ai_trigger_set_named_ror(AoeAiTriggerRor *trigger, const char *name,
                                  int32_t a, int32_t b, uint8_t c, uint8_t d,
                                  int32_t e) {
    if (!trigger) return;
    trigger->code = aoe_ai_trigger_code_ror(name);
    trigger->a = a;
    trigger->b = b;
    trigger->c = c;
    trigger->d = d;
    trigger->e = e;
}

void aoe_ai_rule_init_ror(AoeAiRuleRor *rule) {
    if (!rule) return;
    memset(rule, 0, sizeof(*rule));
    for (int i = 0; i < AOE_AI_RULE_ACTIONS; ++i)
        aoe_ai_action_clear_ror(&rule->actions[i]);
    for (int i = 0; i < AOE_AI_RULE_TRIGGERS; ++i)
        aoe_ai_trigger_clear_ror(&rule->triggers[i]);
    aoe_ai_rule_clear_ror(rule);
}

void aoe_ai_rule_clear_ror(AoeAiRuleRor *rule) {
    if (!rule) return;
    for (int i = 0; i < AOE_AI_RULE_ACTIONS; ++i) rule->actions[i].code = 0;
    for (int i = 0; i < AOE_AI_RULE_TRIGGERS; ++i) rule->triggers[i].code = 0;
}

AoeAiActionRor *aoe_ai_rule_action_ror(AoeAiRuleRor *rule, int32_t index) {
    if (!rule || index < 0 || index >= AOE_AI_RULE_ACTIONS) return NULL;
    return &rule->actions[index];
}

AoeAiTriggerRor *aoe_ai_rule_trigger_ror(AoeAiRuleRor *rule, int32_t index) {
    if (!rule || index < 0 || index >= AOE_AI_RULE_TRIGGERS) return NULL;
    return &rule->triggers[index];
}

int aoe_ai_rule_add_action_ror(AoeAiRuleRor *rule,
                               const AoeAiActionRor *action) {
    if (!rule || !action) return 0;
    for (int i = 0; i < AOE_AI_RULE_ACTIONS; ++i)
        if (!rule->actions[i].code) {
            rule->actions[i] = *action;
            return 1;
        }
    return 0;
}

int aoe_ai_rule_add_trigger_ror(AoeAiRuleRor *rule,
                                const AoeAiTriggerRor *trigger) {
    if (!rule || !trigger) return 0;
    for (int i = 0; i < AOE_AI_RULE_TRIGGERS; ++i)
        if (!rule->triggers[i].code) {
            rule->triggers[i] = *trigger;
            return 1;
        }
    return 0;
}

void aoe_ai_range_reset_ror(AoeAiCountRangeRor *range) {
    if (!range) return;
    memset(range->min, 0, sizeof(range->min));
    memset(range->max, 0xfe, sizeof(range->max));
}

uint8_t aoe_ai_range_min_ror(const AoeAiCountRangeRor *range, int32_t i) {
    return range && i >= 0 && i < 6 ? range->min[i] : 0;
}

void aoe_ai_range_set_min_ror(AoeAiCountRangeRor *range, int32_t i,
                              uint8_t value) {
    if (range && i >= 0 && i < 6) range->min[i] = value;
}

uint8_t aoe_ai_range_max_ror(const AoeAiCountRangeRor *range, int32_t i) {
    return range && i >= 0 && i < 6 ? range->max[i] : 0xff;
}

void aoe_ai_range_set_max_ror(AoeAiCountRangeRor *range, int32_t i,
                              uint8_t value) {
    if (range && i >= 0 && i < 6) range->max[i] = value;
}

void aoe_ai_group_init_ror(AoeAiGroupRor *group) {
    if (!group) return;
    memset(group, 0, sizeof(*group));
    group->field_41 = 1;
    group->field_43 = 1;
    group->mode = 1;
    group->field_42 = 0xff;
    group->field_4e = 0;
    group->field_50 = 50;
    group->field_51 = 50;
    for (int i = 0; i < AOE_AI_GROUP_RANGES; ++i)
        aoe_ai_range_reset_ror(&group->ranges[i]);
    for (int i = 0; i < AOE_AI_GROUP_RULES; ++i)
        aoe_ai_rule_init_ror(&group->rules[i]);
    memset(group->slots_44, 5, sizeof(group->slots_44));
    memset(group->slots_49, 5, sizeof(group->slots_49));
}

void aoe_ai_group_set_name_ror(AoeAiGroupRor *group, const char *name) {
    size_t n = 0;
    if (!group || !name) return;
    /* strncpy(name, 0x40): no terminator for 64+ characters; byte +0x40
       stays as it was. */
    while (n < 0x40 && name[n]) ++n;
    memcpy(group->name, name, n);
    if (n < 0x40) memset(group->name + n, 0, 0x40 - n);
}

int aoe_ai_group_is_hp_ror(const AoeAiGroupRor *group) {
    return group && group->name[0] == 'H' && group->name[1] == 'P';
}

uint8_t aoe_ai_group_slot_44_ror(const AoeAiGroupRor *group, int32_t i) {
    return group && i >= 0 && i < 5 ? group->slots_44[i] : 0;
}

uint8_t aoe_ai_group_slot_49_ror(const AoeAiGroupRor *group, int32_t i) {
    return group && i >= 0 && i < 5 ? group->slots_49[i] : 0;
}

static int add_slot(uint8_t slots[5], uint8_t value) {
    for (int i = 0; i < 5; ++i)
        if (slots[i] == 0 || slots[i] == 5) {
            slots[i] = value;
            return 1;
        }
    return 0;
}

int aoe_ai_group_add_slot_44_ror(AoeAiGroupRor *group, uint8_t value) {
    return group && add_slot(group->slots_44, value);
}

int aoe_ai_group_add_slot_49_ror(AoeAiGroupRor *group, uint8_t value) {
    return group && add_slot(group->slots_49, value);
}

AoeAiCountRangeRor *aoe_ai_group_range_ror(AoeAiGroupRor *group, int32_t i) {
    return group && i >= 0 && i < AOE_AI_GROUP_RANGES ? &group->ranges[i]
                                                      : NULL;
}

AoeAiRuleRor *aoe_ai_group_rule_ror(AoeAiGroupRor *group, int32_t i) {
    return group && i >= 0 && i < AOE_AI_GROUP_RULES ? &group->rules[i]
                                                     : NULL;
}

int32_t aoe_ai_group_gather_value_ror(AoeAiGroupRor *group, int32_t rule) {
    AoeAiRuleRor *r = aoe_ai_group_rule_ror(group, rule);
    if (r)
        for (int i = 0; i < AOE_AI_RULE_TRIGGERS; ++i)
            if (r->triggers[i].code == 1) return r->triggers[i].a;
    return 2;
}
