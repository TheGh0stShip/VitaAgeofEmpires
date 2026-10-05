/* Patched RoR 1.0a achievements pages (0x00478200). */
#include "achievements_page.h"

#include <stdio.h>
#include <string.h>

static const AoeAchievementsPageRor pages[7] = {
    {0x26d9, 7, {0x26ac, 0x26ad, 0x26ae, 0x26af, 0x26d6, 0x26d7, 0x26b1}},
    {0x26ac, 6, {0x26b6, 0x26b7, 0x26b9, 0x26b8, 0x26ba, 0x26b5, 0}},
    {0x26ad, 7, {0x26be, 0x26bf, 0x26c0, 0x26c1, 0x26c2, 0x26c3, 0x26b5}},
    {0x26ae, 7, {0x26c6, 0x26c7, 0x26c8, 0x26c9, 0x26ca, 0x26dc, 0x26b5}},
    {0x26af, 5, {0x26ce, 0x26d1, 0x26cf, 0x26d0, 0x26b5, 0, 0}},
    {0x26b2, 0, {0}},
    {0x26d8, 0, {0}},
};

int aoe_achievements_page_ror(int32_t page, AoeAchievementsPageRor *out) {
    if (page < 0 || page > 6 || !out) return 0;
    *out = pages[page];
    return 1;
}

static AoeAchievementsCellRor num(int32_t v) {
    AoeAchievementsCellRor c = {AOE_ACH_CELL_NUMBER, v};
    return c;
}

static AoeAchievementsCellRor yes_if(const AoeOutcomeManager *m, int8_t index) {
    AoeAchievementsCellRor c = {AOE_ACH_CELL_EMPTY, 0};
    if (aoe_outcome_event_points_ror(m, index) > 0) c.kind = AOE_ACH_CELL_YES;
    return c;
}

int aoe_achievements_row_ror(int32_t page, const AoeOutcomeManager *m,
                             const float *attributes, AoeAchievementsCellRor cells[7]) {
    int i;
    if (!m || !cells || page < 0 || page > 4) return 0;
    for (i = 0; i < 7; ++i) cells[i].kind = AOE_ACH_CELL_EMPTY, cells[i].value = 0;
    switch (page) {
    case 0:
        for (i = 0; i < 4; ++i) cells[i] = num(aoe_outcome_points_ror(m, (int8_t)i));
        cells[4] = yes_if(m, 0x16);
        cells[5] = yes_if(m, 0x17);
        cells[6] = num((int32_t)m->score_sum_1c);
        break;
    case 1:
        cells[0] = num(aoe_outcome_event_sample_ror(m, 0));
        cells[1] = num(aoe_outcome_event_sample_ror(m, 1));
        cells[2] = num(attributes ? (int32_t)(attributes[19] - attributes[11]) : 0);
        cells[3] = num(aoe_outcome_event_sample_ror(m, 2));
        cells[4] = yes_if(m, 3);
        cells[5] = num(aoe_outcome_points_ror(m, 0));
        break;
    case 2:
        cells[0] = num(aoe_outcome_event_sample_ror(m, 4));
        cells[1] = num(aoe_outcome_event_sample_ror(m, 5));
        cells[2] = yes_if(m, 6);
        cells[3].kind = AOE_ACH_CELL_PERCENT;
        cells[3].value = aoe_outcome_event_sample_ror(m, 7);
        cells[4] = yes_if(m, 8);
        cells[5] = num(aoe_outcome_event_sample_ror(m, 9));
        cells[6] = num(aoe_outcome_points_ror(m, 1));
        break;
    case 3:
        cells[0] = num(aoe_outcome_event_sample_ror(m, 10));
        cells[1] = yes_if(m, 0xb);
        cells[2] = num(aoe_outcome_event_sample_ror(m, 0xc));
        cells[3] = num(aoe_outcome_event_sample_ror(m, 0xd));
        cells[4] = num(aoe_outcome_event_sample_ror(m, 0xe));
        cells[5] = num(aoe_outcome_event_points_ror(m, 0x10) + aoe_outcome_event_points_ror(m, 0xf));
        cells[6] = num(aoe_outcome_points_ror(m, 2));
        break;
    default:
        cells[0] = num(aoe_outcome_event_sample_ror(m, 0x11));
        cells[1] = yes_if(m, 0x12);
        cells[2] = yes_if(m, 0x13);
        cells[3] = yes_if(m, 0x14);
        cells[4] = num(aoe_outcome_points_ror(m, 3));
        break;
    }
    return 1;
}

void aoe_achievements_cell_text_ror(const AoeAchievementsCellRor *c, const char *yes,
                                    char *out, int size) {
    if (!out || size <= 0) return;
    switch (c ? c->kind : AOE_ACH_CELL_EMPTY) {
    case AOE_ACH_CELL_NUMBER: snprintf(out, (size_t)size, "%d", (int)c->value); break;
    case AOE_ACH_CELL_PERCENT: snprintf(out, (size_t)size, "%d%%", (int)c->value); break;
    case AOE_ACH_CELL_YES: snprintf(out, (size_t)size, "%s", yes ? yes : ""); break;
    default: out[0] = '\0'; break;
    }
}
