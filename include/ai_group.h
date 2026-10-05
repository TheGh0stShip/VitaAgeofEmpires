#ifndef VITA_AOE_AI_GROUP_H
#define VITA_AOE_AI_GROUP_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a AI group templates (0x0040c2f0..0x0040cca0):
   named groups with per-category count ranges and ten rules, each rule
   holding up to five actions and three triggers. Semantic storage; the
   original offsets are noted. */

/* Action, 16 bytes. Codes from names (0x0040c380): Move 1, Attack 2,
   AttackSavedTarget 3, Retreat 4, Heal 5, ResetHitPoints 6,
   ResetAliveCount 7, Wait 8; anything else 0 (an empty slot). */
typedef struct {
    uint8_t field_0; /* +0 */
    uint8_t code;    /* +1 */
    int32_t a, b, c; /* +4, +8, +0xc */
} AoeAiActionRor;

/* Trigger, 20 bytes. Codes from names (0x0040c600): Gather 1, Death 2,
   HealthLost 3, DamageToGroup 4, DamageToAnyGroup 5, Level1EnemySighted
   6, Level2EnemySighted 7, DeathOfGroup 8, HealOfGroup 9, Immediate 10,
   Time 11; anything else 0 (an empty slot). */
typedef struct {
    uint8_t code;       /* +0 */
    int32_t a, b;       /* +4, +8 (-1 when cleared) */
    uint8_t c, d;       /* +0xc, +0xd */
    int32_t e;          /* +0x10 */
} AoeAiTriggerRor;

#define AOE_AI_RULE_ACTIONS 5
#define AOE_AI_RULE_TRIGGERS 3

typedef struct {
    AoeAiActionRor actions[AOE_AI_RULE_ACTIONS];    /* +0 */
    AoeAiTriggerRor triggers[AOE_AI_RULE_TRIGGERS]; /* +0x50 */
} AoeAiRuleRor;

/* Six per-category count ranges, 12 bytes: minimum (0) and maximum
   (0xfe). */
typedef struct {
    uint8_t min[6], max[6];
} AoeAiCountRangeRor;

#define AOE_AI_GROUP_RANGES 5
#define AOE_AI_GROUP_RULES 10

typedef struct {
    char name[0x41];                 /* +0, 0x40 copied */
    uint8_t field_41, field_42, field_43; /* 1, 0xff, 1 */
    uint8_t slots_44[5];             /* +0x44, 5 = free */
    uint8_t slots_49[5];             /* +0x49, 5 = free */
    uint8_t field_4e, mode;          /* +0x4e 0, +0x4f 1 */
    uint8_t field_50, field_51;      /* 50, 50 */
    uint8_t range_count;             /* +0x52 */
    AoeAiCountRangeRor ranges[AOE_AI_GROUP_RANGES]; /* +0x53 */
    AoeAiRuleRor rules[AOE_AI_GROUP_RULES];         /* +0x90, 0x8c apart */
} AoeAiGroupRor;

/* Actions: clear 0x0040c2f0, set 0x0040c310, set by name 0x0040c340 (the
   name gives the code); name lookup 0x0040c380. */
void aoe_ai_action_clear_ror(AoeAiActionRor *action);
void aoe_ai_action_set_ror(AoeAiActionRor *action, uint8_t field_0,
                           uint8_t code, int32_t a, int32_t b, int32_t c);
void aoe_ai_action_set_named_ror(AoeAiActionRor *action, uint8_t field_0,
                                 const char *name, int32_t a, int32_t b,
                                 int32_t c);
uint8_t aoe_ai_action_code_ror(const char *name);
/* Triggers: clear 0x0040c5a0 (+0x10 kept), set by name 0x0040c5c0; name
   lookup 0x0040c600. */
void aoe_ai_trigger_clear_ror(AoeAiTriggerRor *trigger);
void aoe_ai_trigger_set_named_ror(AoeAiTriggerRor *trigger, const char *name,
                                  int32_t a, int32_t b, uint8_t c, uint8_t d,
                                  int32_t e);
uint8_t aoe_ai_trigger_code_ror(const char *name);

/* Rules: init 0x0040c8e0 (all slots cleared), clear 0x0040ca10 (the codes
   only), action(i) 0x0040c920 (0..4) and trigger(i) 0x0040c940 (0..2),
   NULL out of range; add into the first empty slot 0x0040c960 (action)
   and 0x0040c9b0 (trigger), 0 when full. */
void aoe_ai_rule_init_ror(AoeAiRuleRor *rule);
void aoe_ai_rule_clear_ror(AoeAiRuleRor *rule);
AoeAiActionRor *aoe_ai_rule_action_ror(AoeAiRuleRor *rule, int32_t index);
AoeAiTriggerRor *aoe_ai_rule_trigger_ror(AoeAiRuleRor *rule, int32_t index);
int aoe_ai_rule_add_action_ror(AoeAiRuleRor *rule,
                               const AoeAiActionRor *action);
int aoe_ai_rule_add_trigger_ror(AoeAiRuleRor *rule,
                                const AoeAiTriggerRor *trigger);

/* Count ranges: init 0x0040ca40 / reset 0x0040cad0 (min 0, max 0xfe);
   minimum 0x0040ca50 / 0x0040ca70 (0 out of range), maximum 0x0040ca90
   (0xff out of range) / 0x0040cab0, categories 0..5. */
void aoe_ai_range_reset_ror(AoeAiCountRangeRor *range);
uint8_t aoe_ai_range_min_ror(const AoeAiCountRangeRor *range, int32_t i);
void aoe_ai_range_set_min_ror(AoeAiCountRangeRor *range, int32_t i,
                              uint8_t value);
uint8_t aoe_ai_range_max_ror(const AoeAiCountRangeRor *range, int32_t i);
void aoe_ai_range_set_max_ror(AoeAiCountRangeRor *range, int32_t i,
                              uint8_t value);

/* Groups: init 0x0040caf0; name 0x0040cb60 (NULL keeps it); 0x0040cb80
   the name starts "HP"; slot lists +0x44 / +0x49: get 0x0040cba0 /
   0x0040cbf0 (0 out of range), add into the first 0 or 5 slot 0x0040cbc0
   / 0x0040cc10 (0 when full); range(i) 0x0040cc80, rule(i) 0x0040cca0;
   0x0040cc40: the rule's first trigger with code 1 gives its +4, else 2
   (also 2 for no rule). */
void aoe_ai_group_init_ror(AoeAiGroupRor *group);
void aoe_ai_group_set_name_ror(AoeAiGroupRor *group, const char *name);
int aoe_ai_group_is_hp_ror(const AoeAiGroupRor *group);
uint8_t aoe_ai_group_slot_44_ror(const AoeAiGroupRor *group, int32_t i);
int aoe_ai_group_add_slot_44_ror(AoeAiGroupRor *group, uint8_t value);
uint8_t aoe_ai_group_slot_49_ror(const AoeAiGroupRor *group, int32_t i);
int aoe_ai_group_add_slot_49_ror(AoeAiGroupRor *group, uint8_t value);
AoeAiCountRangeRor *aoe_ai_group_range_ror(AoeAiGroupRor *group, int32_t i);
AoeAiRuleRor *aoe_ai_group_rule_ror(AoeAiGroupRor *group, int32_t i);
int32_t aoe_ai_group_gather_value_ror(AoeAiGroupRor *group, int32_t rule);

#endif
