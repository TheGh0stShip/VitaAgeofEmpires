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

#endif
