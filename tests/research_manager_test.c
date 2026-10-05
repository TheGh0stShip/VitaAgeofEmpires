#include "research_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #cond);     \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

static int rebuilds;
static void rebuild(void *c) {
    (void)c;
    ++rebuilds;
}

int main(void) {
    DatTech tech[2];
    DatTechs techs = {2, tech, 0};
    AoeResearchProgressEntry entries[2];
    AoeResearchCompletionManagerRor m;
    AoeResearchCompletionManagerRor back;
    float values[30];
    AoePlayerResources res = {30, values};
    AoeResearchOwnerRor owner;
    AoeOutcomeStream s;
    uint8_t buf[64];
    int16_t types[3], amounts[3];

    memset(tech, 0, sizeof tech);
    tech[0].name = "Wheel";
    tech[0].research_time = 40;
    tech[0].language_help = 100;
    tech[0].language_tree = 200;
    tech[0].hotkey = 300;
    tech[0].language_name = 7;
    tech[0].button_id = 4;
    tech[0].costs[0].resource_type = 0; /* food 80 */
    tech[0].costs[0].amount = 80;
    tech[0].costs[0].paid = 1;
    tech[0].costs[1].resource_type = 1; /* wood 50 */
    tech[0].costs[1].amount = 50;
    tech[0].costs[1].paid = 1;
    tech[0].costs[2].resource_type = -1;
    tech[1] = tech[0];
    tech[1].name = "Other";

    CHECK(aoe_tech_help_ror(&techs, 0) == 100);
    CHECK(aoe_tech_hotkey_ror(&techs, 2) == -1);
    CHECK(aoe_tech_tree_text_ror(&techs, -1) == -1);

    memset(&m, 0, sizeof m);
    m.count = 2;
    m.entries = entries;
    entries[0].progress = 10.0f;
    entries[0].state = 2;
    entries[1].progress = 0.0f;
    entries[1].state = 3;
    CHECK(aoe_research_state_ror(&m, 1) == 3);
    CHECK(aoe_research_state_ror(&m, 2) == -1);
    CHECK(aoe_research_percent_ror(&m, &techs, 0) == 25);
    CHECK(aoe_research_percent_ror(&m, &techs, 1) == 100);
    CHECK(strcmp(aoe_research_name_ror(&m, &techs, 1), "Other") == 0);
    CHECK(aoe_research_name_ror(&m, &techs, 5) == NULL);
    {
        int16_t id, pct;
        char nm[4];
        aoe_research_action_readout_ror(&m, &techs, 0, 0, &id, &pct, NULL, 0);
        CHECK(id == 0 && pct == 25);
        aoe_research_action_readout_ror(&m, &techs, 1, 1, &id, &pct, nm, sizeof nm);
        CHECK(id == 1 && pct == 100 && strcmp(nm, "Oth") == 0);
    }
    CHECK(aoe_research_language_name_ror(&m, &techs, 0) == 7);
    CHECK(aoe_research_button_ror(&m, &techs, 0) == 4);
    CHECK(aoe_research_help_ror(&m, &techs, 1) == 100);
    CHECK(aoe_research_hotkey_ror(&m, &techs, -3) == -1);

    s.data = buf;
    s.size = sizeof buf;
    s.pos = 0;
    s.failed = 0;
    aoe_research_manager_save_ror(&m, &s);
    CHECK(!s.failed && s.pos == 2 + 2 * 6);
    s.size = s.pos;
    s.pos = 0;
    memset(&back, 0, sizeof back);
    back.rebuild = rebuild;
    CHECK(aoe_research_manager_load_ror(&back, &s, 7.24f, 0));
    CHECK(back.count == 2 && back.entries[0].progress == 10.0f &&
          back.entries[1].state == 3 && rebuilds == 0);
    aoe_research_manager_release_ror(&back);
    CHECK(back.entries == NULL);

    /* An old save: two-byte progress values. */
    {
        uint8_t old[] = {1, 0, 0x34, 0x12, 2, 0};
        s.data = old;
        s.size = sizeof old;
        s.pos = 0;
        s.failed = 0;
        CHECK(aoe_research_manager_load_ror(&back, &s, 7.0f, 0));
        CHECK(back.count == 1 && back.entries[0].state == 2);
        {
            uint8_t raw[4] = {0x34, 0x12, 0, 0};
            float f;
            memcpy(&f, raw, 4);
            CHECK(memcmp(&back.entries[0].progress, &f, 4) == 0);
        }
        aoe_research_manager_release_ror(&back);
    }

    /* Costs: plain display, then the gold reserve. */
    memset(values, 0, sizeof values);
    owner.manager = &m;
    owner.techs = &techs;
    owner.resources = &res;
    entries[0].state = 1;
    values[0] = 30.0f;
    values[1] = 100.0f;
    values[3] = 500.0f;
    aoe_research_costs_ror(&owner, 0, types, amounts);
    CHECK(types[0] == 0 && amounts[0] == 80 && types[1] == 1 &&
          amounts[1] == 50 && types[2] == -1);
    values[29] = 1.0f;
    aoe_research_costs_ror(&owner, 0, types, amounts);
    /* Food shows what is held; the 50 short is added as gold in slot 3. */
    CHECK(types[0] == 0 && amounts[0] == 30);
    CHECK(types[1] == 1 && amounts[1] == 50);
    CHECK(types[2] == 3 && amounts[2] == 50);
    aoe_research_costs_ror(&owner, 9, types, amounts);
    CHECK(types[0] == -1 && types[2] == -1);

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    puts("research_manager_test: ok");
    return 0;
}
