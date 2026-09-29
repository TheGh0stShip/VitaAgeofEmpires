#include "research_action.h"

#include <math.h>

int aoe_research_progress_percent_ror(const AoeResearchProgressTable *table,
                                      int16_t research_id, int16_t *percent) {
    const AoeResearchProgressEntry *entry;
    long double value;
    int64_t truncated;
    if (!table || !percent || research_id < 0 || research_id >= table->count) {
        if (percent) *percent = 0;
        return table && percent;
    }
    if (!table->entries) return 0;
    entry = &table->entries[research_id];
    if (entry->state == 3) {
        *percent = 100;
        return 1;
    }
    if (entry->state != 2) {
        *percent = 0;
        return 1;
    }
    if (!table->techs || !table->techs->techs ||
        (uint16_t)research_id >= table->techs->count ||
        table->techs->techs[research_id].research_time == 0 ||
        !isfinite(entry->progress)) return 0;
    value = (long double)entry->progress * 100.0L /
            table->techs->techs[research_id].research_time;
    if (!isfinite(value) || value >= 9223372036854775808.0L ||
        value < -9223372036854775808.0L) return 0;
    truncated = (int64_t)value;
    *percent = (int16_t)(uint16_t)truncated;
    return 1;
}

int aoe_research_action_progress_ror(const AoeResearchAction *action,
                                     int16_t *research_id, int16_t *percent) {
    if (!action || !research_id || !percent) return 0;
    *research_id = action->research_id;
    if (action->state == 1) {
        *percent = 100;
        return 1;
    }
    return aoe_research_progress_percent_ror(action->progress_table,
                                             action->research_id, percent);
}
