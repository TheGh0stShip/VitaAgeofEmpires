#ifndef VITA_AOE_SCENARIO_EDITOR_PLAYER_H
#define VITA_AOE_SCENARIO_EDITOR_PLAYER_H

#include <stdint.h>

#include "scenario_info.h"
#include "scenario_object.h"

/* The scenario editor's player page values (0x00492780). Resources are
   in world attribute order: food (edit +0x5d8, info field 2), wood
   (+0x5dc, field 1), stone (+0x5e0, field 3) and gold (+0x5e4, field
   0). */
typedef struct {
    const char *resource[4];
    const char *name;        /* edit +0x5e8 */
    int32_t kind_line;       /* list +0x5f8 line: value_18cc */
    int32_t value_line;      /* list +0x5fc line: value_190c - 1 */
    const char *ai;          /* dropdown +0x600 text (0x00475940) */
    const char *city;        /* dropdown +0x604 text (0x004759b0) */
    const char *per;         /* dropdown +0x608 text (0x00475a20) */
} AoeEditorPlayerFormRor;

typedef struct {
    char resource[4][16];    /* "%d" of the info fields */
    const char *name;        /* 0x00476200 */
    int32_t kind_line;       /* 0x00476210 */
    int32_t value_line;      /* 0x00476230 - 1 */
    const char *ai, *city, *per; /* 0x00475b90 / 0x00475d80 / 0x00475f90 (p, 0) */
} AoeEditorPlayerShownRor;

/* 0x00492780(player, store) leaving slot = editor +0x4b8 - 1 (at least
   0), after the viewed player's selection is cleared: with `store` the
   form goes to the scenario info (start resources), the world player's
   resources (atoi, as floats, into `world_resources` when not NULL) and
   the scenario object (name, the two values, the AI, city-plan and
   personality names unstripped). Host-bounded: slots 0..15. */
void aoe_editor_store_player_ror(AoeScenarioInfoRor *info,
                                 AoeScenarioObjectRor *obj, int32_t slot,
                                 const AoeEditorPlayerFormRor *form,
                                 float world_resources[4]);

/* Then the editor's player becomes `player` (+0x4b8); unless 0 the
   player list selects line player - 1, the game views that player (+0x14)
   and, with `store` exactly 1, the page shows slot player - 1: these
   values, each file name selected in its dropdown (appended with ID 0
   first when missing). Returns 0 for an unavailable slot. */
int aoe_editor_show_player_ror(const AoeScenarioInfoRor *info,
                               AoeScenarioObjectRor *obj, int32_t slot,
                               const AoeScenarioFileOpsRor *file_ops,
                               AoeEditorPlayerShownRor *out);

/* 0x0049a350: the scenario's player count changed to n = list +0x5b8
   line + 1. Each slot below the world's player count - 1 gets value_194c
   (0x00475ad0) slot < n. The player lists +0x73c, +0x740, +0x7cc,
   +0x894 and +0x5b4 are refilled with strings 0x2865.. (n of them) and
   +0x618 with 0x2776 then those. When n <= the editor's player the page
   switches to player n unstored (0x00492780(n, 0, 0)); then +0x73c,
   +0x7cc, +0x894 and +0x5b4 select line player - 1 and +0x618 line
   player. Returns n; `switched` reports the 0x00492780 call. */
int32_t aoe_editor_set_player_count_ror(AoeScenarioObjectRor *obj,
                                        int32_t world_players,
                                        int32_t count_line,
                                        int16_t *current_player,
                                        int *switched);

/* 0x00506dc0 (from the RoR scenario constructor 0x00506830): every
   slot's defaults: kind 4, value_18cc 0 (slot 0 then 1), value_190c the
   slot + 1, value_194c 0; start resources gold 0, wood 200, food 200,
   stone 150 (fields 0..3); allied victory and starting age 0. */
void aoe_scenario_player_defaults_ror(AoeScenarioInfoRor *info,
                                      AoeScenarioObjectRor *obj);

/* 0x00507e00: whether any active slot (value_194c, 0x00476250) has a
   condition record with a type (0x005075f0); each active slot is
   selected (0x00506f10) on the way, the last one examined staying
   selected. */
int aoe_scenario_has_conditions_ror(AoeScenarioInfoRor *info,
                                    const AoeScenarioObjectRor *obj);

/* The RoR scenario record (vtable 0x0054a3c0): the base record and two
   dwords, whether the scenario has conditions (+0x14) and its active
   player count (+0x18). 0x00506710 builds one from a scenario (the base
   0x00474a40, then 0x00507e00 and 0x00476a90); 0x00506770 reads one (the
   base 0x00474ae0, then the two dwords unless the base failed); the tribe
   game's 0x00500210 / 0x005001b0 allocate them. Size 0x005067f0 and write
   0x00506800 (the size written first is this record's, the base +8). */
typedef struct {
    AoeScenarioRecordRor base;
    int32_t has_conditions, players;
} AoeScenarioRecordExRor;
void aoe_scenario_record_ex_init_ror(AoeScenarioRecordExRor *r, const char *text, int32_t now,
                                     AoeScenarioInfoRor *info, const AoeScenarioObjectRor *obj);
void aoe_scenario_record_ex_read_ror(AoeScenarioRecordExRor *r, AoeOutcomeStream *in);
int32_t aoe_scenario_record_ex_size_ror(const AoeScenarioRecordExRor *r);
void aoe_scenario_record_ex_write_ror(const AoeScenarioRecordExRor *r, AoeOutcomeStream *out);

#endif
