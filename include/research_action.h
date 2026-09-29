#ifndef VITA_AOE_RESEARCH_ACTION_H
#define VITA_AOE_RESEARCH_ACTION_H

#include "dat.h"
#include <stdint.h>

/* Semantic form of each eight-byte research manager entry used by RoR
   004ed8ef: progress at +0 and signed state at +4. */
typedef struct {
    float progress;
    int16_t state;
} AoeResearchProgressEntry;

typedef struct {
    int16_t count;
    const AoeResearchProgressEntry *entries;
    const DatTechs *techs;
} AoeResearchProgressTable;

/* RoR 004ed8ef. Invalid IDs and states other than 2/3 report zero. State 2
   truncates progress*100/research_time toward zero and retains the low signed
   16 bits; state 3 reports 100. Nonfinite or zero-divisor state-2 data is
   rejected by the portable API rather than emulating x87 indefinite values. */
int aoe_research_progress_percent_ror(const AoeResearchProgressTable *table,
                                      int16_t research_id, int16_t *percent);

typedef struct {
    int16_t action_type; /* inherited action +0x04; research action = 103 */
    uint8_t state;       /* inherited action +0x0c */
    int16_t research_id; /* derived action +0x40 */
    const AoeResearchProgressTable *progress_table;
} AoeResearchAction;

/* Data-return portion of RoR 004b7d60. State 1 reports 100 directly;
   otherwise percentage comes from translated 004ed8ef. The optional localized
   text-buffer branch through 004ed989/00528540 remains outside this API. */
int aoe_research_action_progress_ror(const AoeResearchAction *action,
                                     int16_t *research_id, int16_t *percent);

#endif
