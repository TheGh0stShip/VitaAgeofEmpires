#ifndef VITA_AOE_SCENARIO_INFO_H
#define VITA_AOE_SCENARIO_INFO_H

#include <stdint.h>

#include "player_outcome.h"

/* Patched Rise of Rome 1.0a scenario-info object (`this` of 0x00506e40..
   0x00507cc0): the editable scenario settings that 0x005076c0 applies to
   the players and 0x00507800 turns into victory conditions. Fields carry
   their original offsets; this is semantic storage, not the x86 layout.
   Player slots are 0-based here and in the original (slot p is world
   player p + 1). */

#define AOE_SCENARIO_PLAYERS 16
#define AOE_SCENARIO_CONDITIONS 12

/* One per-player victory-condition record (0x3c bytes at +0x1aa8 +
   (slot x 12 + index) x 0x3c). */
typedef struct {
    int32_t object_class;  /* +0x00 0x005071c0 / 0x00507550 */
    int32_t all_flag;      /* +0x04 0x00507220 / 0x00507590 */
    int32_t player;        /* +0x08 0x005071f0 / 0x00507570 */
    int32_t field_0c;      /* +0x0c, cleared only */
    float area[4];         /* +0x10..+0x1c x0, y0, x1, y1 (0x00507140) */
    int32_t type;          /* +0x20 0x00507280 / 0x005075f0 */
    int32_t amount;        /* +0x24 0x00507330 / 0x005075d0 */
    int32_t subtype;       /* +0x28 0x00507250 / 0x005075b0 */
    int32_t unit_ref;      /* +0x2c unit ID + 1, 0 none (0x005072b0) */
    int32_t target_ref;    /* +0x30 unit ID + 1, 0 none (0x005072f0) */
    int32_t field_34[2];   /* +0x34, +0x38, cleared only */
} AoeScenarioConditionRor;

typedef struct {
    int16_t player_count;  /* world (+8) +0x3c, gaia included */
    uint8_t conquest;      /* +0x0c (0x004072e0) */
    int32_t start_resources[AOE_SCENARIO_PLAYERS][4]; /* +0x1990 */
    /* Global custom goals +0x1a90..+0x1aa4 (0x00506f40 init): conquest
       (1), ruins (attribute 14), artifacts (7), discoveries (13), explore
       percent and gold (3). */
    int32_t custom_conquest, ruins, artifacts, discoveries, explore, gold;
    AoeScenarioConditionRor conditions[AOE_SCENARIO_PLAYERS]
                                      [AOE_SCENARIO_CONDITIONS]; /* +0x1aa8 */
    int32_t diplomacy[AOE_SCENARIO_PLAYERS][AOE_SCENARIO_PLAYERS]; /* +0x47a8 */
    int32_t allied_victory[AOE_SCENARIO_PLAYERS]; /* +0x4ba8 */
    int32_t all_custom_required; /* +0x4be8, 0..1 */
    int32_t selected;            /* +0x4bf0, the edited player slot */
    int32_t tech_toggles[AOE_SCENARIO_PLAYERS][20]; /* +0x4bf4 */
    int32_t options[3];          /* +0x50f4 */
    int32_t starting_age[AOE_SCENARIO_PLAYERS]; /* +0x5100 */
    int32_t victory_type;        /* +0x5140; 4 is custom */
    int32_t score;               /* +0x5144 */
    int32_t time_limit;          /* +0x5148 */
} AoeScenarioInfoRor;

/* 0x00506e40: every tech toggle (0x140 dwords) = 1.
   0x00506fb0: every diplomacy cell (0x100 dwords) = 3.
   0x00506f40: custom conquest 1, the other goals 0, every condition record
   (0xb40 dwords) cleared. */
void aoe_scenario_enable_all_techs_ror(AoeScenarioInfoRor *info);
void aoe_scenario_reset_diplomacy_ror(AoeScenarioInfoRor *info);
void aoe_scenario_reset_goals_ror(AoeScenarioInfoRor *info);

/* 0x00506e60 / 0x00506e90: clamped to 0..1. */
void aoe_scenario_set_all_custom_required_ror(AoeScenarioInfoRor *info,
                                              int32_t value);
int32_t aoe_scenario_all_custom_required_ror(const AoeScenarioInfoRor *info);
/* 0x00506ea0 / 0x00506ec0: a non-custom (not 4) type also sets conquest. */
void aoe_scenario_set_victory_type_ror(AoeScenarioInfoRor *info,
                                       int32_t type);
int32_t aoe_scenario_victory_type_ror(const AoeScenarioInfoRor *info);
/* 0x00506ed0 / 0x00506ee0 and 0x00506ef0 / 0x00506f00. */
void aoe_scenario_set_score_ror(AoeScenarioInfoRor *info, int32_t score);
int32_t aoe_scenario_score_ror(const AoeScenarioInfoRor *info);
void aoe_scenario_set_time_limit_ror(AoeScenarioInfoRor *info, int32_t time);
int32_t aoe_scenario_time_limit_ror(const AoeScenarioInfoRor *info);
/* 0x00506f10: negative selects 0 and above 16 selects 15 (16 itself is
   kept, one past the last slot; the record accessors then refuse it). */
void aoe_scenario_select_player_ror(AoeScenarioInfoRor *info, int32_t slot);

/* 0x00506fd0 / 0x00507010: stance 0, 1 or 3 between slots 0..15; the
   getter returns -1 out of range. */
void aoe_scenario_set_diplomacy_ror(AoeScenarioInfoRor *info, int32_t from,
                                    int32_t to, int32_t stance);
int32_t aoe_scenario_diplomacy_ror(const AoeScenarioInfoRor *info,
                                   int32_t from, int32_t to);

/* Starting resources by slot 0..15 (others ignored / 0):
   fields 0..3 = setters 0x00507040/0x00507060/0x00507080/0x005070a0 and
   getters 0x005073e0/0x00507400/0x00507420/0x00507440. */
void aoe_scenario_set_start_resource_ror(AoeScenarioInfoRor *info,
                                         int32_t slot, int field,
                                         int32_t value);
int32_t aoe_scenario_start_resource_ror(const AoeScenarioInfoRor *info,
                                        int32_t slot, int field);

/* Global goals: setters 0x005070c0 (custom conquest; also the conquest
   byte), 0x005070f0 ruins, 0x00507100 artifacts, 0x00507110 discoveries,
   0x00507120 explore, 0x00507130 gold; getters 0x00507460, 0x00507470,
   0x00507480, 0x00507490, 0x005074a0 and 0x005074b0. */
enum {
    AOE_SCENARIO_GOAL_CONQUEST,
    AOE_SCENARIO_GOAL_RUINS,
    AOE_SCENARIO_GOAL_ARTIFACTS,
    AOE_SCENARIO_GOAL_DISCOVERIES,
    AOE_SCENARIO_GOAL_EXPLORE,
    AOE_SCENARIO_GOAL_GOLD
};
void aoe_scenario_set_goal_ror(AoeScenarioInfoRor *info, int goal,
                               int32_t value);
int32_t aoe_scenario_goal_ror(const AoeScenarioInfoRor *info, int goal);

/* Records of the selected slot (index 0..11; host-bounded, the original
   does not check): clear 0x00506f80; area 0x00507140 / 0x005074c0;
   object class, all flag, player, type, amount and subtype as listed on
   AoeScenarioConditionRor; unit references 0x005072b0 / 0x005072f0 (a
   NULL unit leaves the field) and their resolution 0x00507640 /
   0x00507610 through `units` (0x00507ca0). Returns NULL for an
   unavailable record. */
AoeScenarioConditionRor *aoe_scenario_condition_ror(AoeScenarioInfoRor *info,
                                                    int32_t index);
void aoe_scenario_clear_condition_ror(AoeScenarioInfoRor *info,
                                      int32_t index);
void aoe_scenario_set_condition_area_ror(AoeScenarioInfoRor *info,
                                         int32_t index, float x0, float y0,
                                         float x1, float y1);
int aoe_scenario_condition_area_ror(AoeScenarioInfoRor *info, int32_t index,
                                    float *x0, float *y0, float *x1,
                                    float *y1);
void aoe_scenario_set_condition_unit_ror(AoeScenarioInfoRor *info,
                                         int32_t index,
                                         const AoeOutcomeUnitRef *unit);
void aoe_scenario_set_condition_target_ror(AoeScenarioInfoRor *info,
                                           int32_t index,
                                           const AoeOutcomeUnitRef *unit);

/* World object lookup 0x005206d0 for an object ID. */
typedef AoeOutcomeUnitRef *(*AoeScenarioUnitFn)(void *context, int32_t id);
/* 0x00507ca0: a reference above 0 resolves ID ref - 1; else NULL. */
AoeOutcomeUnitRef *aoe_scenario_resolve_ror(int32_t ref,
                                            AoeScenarioUnitFn units,
                                            void *context);
AoeOutcomeUnitRef *aoe_scenario_condition_unit_ror(AoeScenarioInfoRor *info,
                                                   int32_t index,
                                                   AoeScenarioUnitFn units,
                                                   void *context);
AoeOutcomeUnitRef *aoe_scenario_condition_target_ror(AoeScenarioInfoRor *info,
                                                     int32_t index,
                                                     AoeScenarioUnitFn units,
                                                     void *context);

/* Per-slot arrays: allied victory 0x00507360 / 0x00507670, tech toggles
   0x00507380 / 0x00507680 (slot, tech 0..19), options 0x005073a0 (0..2)
   and starting age 0x005073c0 / 0x005076b0. Host-bounded. */
void aoe_scenario_set_allied_victory_ror(AoeScenarioInfoRor *info,
                                         int32_t slot, int32_t value);
int32_t aoe_scenario_allied_victory_ror(const AoeScenarioInfoRor *info,
                                        int32_t slot);
void aoe_scenario_set_tech_ror(AoeScenarioInfoRor *info, int32_t slot,
                               int32_t tech, int32_t value);
int32_t aoe_scenario_tech_ror(const AoeScenarioInfoRor *info, int32_t slot,
                              int32_t tech);
void aoe_scenario_set_option_ror(AoeScenarioInfoRor *info, int32_t index,
                                 int32_t value);
void aoe_scenario_set_starting_age_ror(AoeScenarioInfoRor *info,
                                       int32_t slot, int32_t age);
int32_t aoe_scenario_starting_age_ror(const AoeScenarioInfoRor *info,
                                      int32_t slot);

typedef struct {
    void *context;
    /* Player +0x70 (0x0045cfc0): set resource `attribute`. */
    void (*set_resource)(void *context, int32_t player, int16_t attribute,
                         float value);
    /* 0x0045da70: the stance byte toward `other`. */
    void (*set_stance)(void *context, int32_t player, int32_t other,
                       uint8_t stance);
    /* 0x0045da40: player +0x108, allied victory. */
    void (*set_allied_victory)(void *context, int32_t player, uint8_t value);
} AoeScenarioApplyOps;

/* 0x005076c0: for each slot below player_count - 1, world player slot + 1
   gets resources 0, 2, 3, 1 from fields 2, 3, 0, 1 (food, stone, gold,
   wood), a stance (low byte) toward every other non-gaia player from the
   diplomacy row, then its allied-victory byte. */
int aoe_scenario_apply_players_ror(const AoeScenarioInfoRor *info,
                                   const AoeScenarioApplyOps *ops);

typedef struct {
    void *context;
    AoeScenarioApplyOps apply;
    /* World player p (+0x3c +0x40[p]) and its outcome manager +0x34. */
    AoeOutcomePlayerRef *(*player)(void *context, int32_t p);
    AoeOutcomeManager *(*manager)(void *context, int32_t p);
    AoeScenarioUnitFn units;
} AoeScenarioInstallOps;

/* 0x00507800: optionally apply the players first, then clear the
   conditions of players 1.. and install the scenario's goals:
   - custom victory (type 4): each player gets explore (0x0050e720, allies)
     and attribute goals ruins 14, artifacts 7, discoveries 13 (without
     allies) and gold 3, in group 8 when all are required, otherwise in
     groups 9, 10, 11 and 12 (explore keeps group 8);
   - then each slot's 12 records go to player slot + 1 in group 1 by
     type: 1 destroy (class and all flag: every class unit; else a player
     with or without a class; else the referenced unit), 2 create (in the
     area when x1 > 0), 3 bring the unit to the target or to the area,
     4 subtype 6 explore or a resource (0..5 = attributes 3, 1, 2, 0, 11,
     6), 5 the unit (kind 0, allies), 6 an attribute, 7 kind 100.
   Returns 0 for missing dependencies. */
int aoe_scenario_install_ror(const AoeScenarioInfoRor *info, int apply,
                             const AoeScenarioInstallOps *ops);

/* Scenario editor victory pages (editor vtable 0x00547360). The controls
   are read by the caller; these carry their values. */
typedef struct {
    int32_t tool_victory;     /* editor +0x4ac, 1..4 standard, else custom */
    int32_t radio[4];         /* +0x6e0..+0x6ec states: conquest, score,
                                 time limit, ruins/artifacts standard */
    int32_t score;            /* +0x6f8 selected ID */
    int32_t time_limit;       /* +0x700 selected ID */
    int32_t all_required;     /* +0x684 state */
    int32_t conquest_off;     /* +0x668 state */
    int32_t conquest_alt;     /* +0x6f0 state */
    /* Explore, ruins, artifacts, discoveries and gold: check +0x66c..
       +0x67c and edit +0x6b8..+0x6c8. */
    int32_t goal_check[5];
    const char *goal_text[5];
} AoeScenarioGoalsFormRor;

/* 0x004954c0: victory type from the tool (1..4 -> 0..3, else 4), then
   again from the first set radio (else 4); score and time limit (each
   stored twice in the original); all-required = state != 0; custom
   conquest 0 only when +0x668 is clear and +0x6f0 set; each goal is
   atoi(text) when checked, else 0, explore clamped to 1..100. */
void aoe_scenario_store_goals_ror(AoeScenarioInfoRor *info,
                                  const AoeScenarioGoalsFormRor *form);

/* Condition page values (0x004957e0). */
typedef struct {
    const char *amount_text;  /* edit +0x76c */
    int32_t kind;             /* list +0x734 line, 1..18 */
    int32_t player_line;      /* list +0x740 line (player = line + 1) */
    int32_t object_class;     /* dropdown +0x738 selected ID */
    int32_t subtype_id;       /* dropdown +0x744 selected ID (kind 17) */
    int32_t tech_line;        /* list +0x748 line (kind 15) */
    int32_t relic_id;         /* dropdown +0x74c selected ID (kind 18) */
    const AoeOutcomeUnitRef *unit;   /* editor +0x480 */
    const AoeOutcomeUnitRef *target; /* editor +0x484 */
    int32_t area[4];          /* view tiles (0x00510070) */
} AoeScenarioConditionFormRor;

/* 0x004957e0: all-required and custom conquest as in the goals store;
   for index >= 0 the area becomes floats in `area_out` (editor +0x488..
   +0x494; may be NULL), the record is cleared and filled per kind:
   1 type 3 unit+target (both needed), 2 type 3 unit+area, 3 type 2
   class+amount, 4 that plus area, 5 type 1 player 1..8 + class + nonzero
   amount, 6 type 1 unit, 7 type 1 player + class + all flag, 8 type 1
   player, 9 type 5 unit, 10..14 type 4 subtypes 0,3,1,2,4 + amount,
   15 type 4 subtype 5 amount tech_line + 1, 16 type 4 subtype 6 + amount,
   17 type 6 subtype_id + amount, 18 type 7 subtype relic_id. A record
   whose requirements fail stays cleared. */
void aoe_scenario_store_condition_ror(AoeScenarioInfoRor *info,
                                      int32_t index,
                                      const AoeScenarioGoalsFormRor *goals,
                                      const AoeScenarioConditionFormRor *form,
                                      float area_out[4]);

/* 0x00495d90's choice of the condition list line for a record: type 0
   -> 0; type 1 -> 5 with an amount, 6 with a unit, else 7 with the all
   flag or 8; type 2 -> 4 when x1 > 0 else 3; type 3 -> 1 with a target
   else 2; type 4 -> 10,12,13,11,14,15,16 by subtype 0..6; type 5 -> 9
   with a unit; type 6 -> 17; type 7 -> 18. -1 means the original leaves
   the list unchanged (unknown values, or type 5 without a unit). The
   original tests the resolved unit and target; this tests the stored
   references (equal for references that resolve). */
int32_t aoe_scenario_condition_kind_ror(const AoeScenarioConditionRor *c);

/* 0x00496140's victory radio for the stored type: 0..3 -> tool 1..4,
   else 5. Its score and time defaults are 900 and 9000 when 0. */
int32_t aoe_scenario_victory_tool_ror(const AoeScenarioInfoRor *info);

/* 0x00496420: for slot >= 0 and targets 0..7 the first of the ally,
   neutral and enemy buttons whose state is 1 stores stance 0, 1 or 3
   (none leaves the cell); then allied victory from the slot's button. */
void aoe_scenario_store_diplomacy_page_ror(AoeScenarioInfoRor *info,
                                           int32_t slot,
                                           const int32_t ally[8],
                                           const int32_t neutral[8],
                                           const int32_t enemy[8],
                                           int32_t allied_victory);

/* 0x004964c0: for slot >= 0 and targets 0..7, the radio to select from
   the stored stance (0 ally, 1 neutral, 3 enemy -> radio 0, 1, 2 of the
   target's group at editor +0x8a8 + target x 0xc; other stances -1, no
   selection) and the allied-victory button states (+0x908[8]): all 0
   but the slot's own, which shows the slot's allied victory. Slots above
   7 are host-guarded (the original indexes past the eight buttons).
   Returns 0 for a negative slot, which leaves the outputs alone. */
int aoe_scenario_diplomacy_page_ror(const AoeScenarioInfoRor *info,
                                    int32_t slot, int32_t radio[8],
                                    int32_t allied_buttons[8]);

/* 0x00507cd0: for each world player 1..player_count - 1 (slot = player
   - 1): with option 0 (+0x50f4) set, the world's effects (+0x4c) apply
   effect 100 to it (vtable +8); then its techs 0x58, 0x59, 0x5a, 0x5b,
   0x5e, 0x5f, 0x61, 0x62, 0x5d, 0x60, 0x5c, 0x65, 0x66, 0x67, 0x73, 0x74
   are enabled or disabled (0x004f1870) by the low byte of toggles 0..15.
   Nothing without a world (+8). */
typedef struct {
    void *context;
    int has_world;
    void (*apply_effect)(void *context, int32_t player, int16_t effect);
    void (*set_tech)(void *context, int32_t player, int16_t tech, uint8_t enabled);
} AoeScenarioTechOpsRor;
void aoe_scenario_apply_techs_ror(const AoeScenarioInfoRor *info,
                                  const AoeScenarioTechOpsRor *ops);

/* 0x00506c80(stream): 0x00507cd0 first, the base scenario save
   (0x00476280), then (0x0046ba40, little-endian): the start resources
   (16 x 4 int32), the marker -99, the six custom goals, +0x4be8,
   victory type, score, time limit, the diplomacy (16 x 16), every
   condition record (16 x 12 x 0x3c bytes), -99, the allied victory
   (16), the tech toggles (16 x 20), the three options, the starting ages
   (16) and -99. */
typedef struct {
    void *context;
    void (*base_save)(void *context);                               /* 0x00476280 */
    void (*write)(void *context, const uint8_t *data, int32_t n);   /* 0x0046ba40 */
} AoeScenarioSaveOpsRor;
void aoe_scenario_info_save_ror(const AoeScenarioInfoRor *info,
                                const AoeScenarioTechOpsRor *techs,
                                const AoeScenarioSaveOpsRor *ops);

#endif
