/* Patched RoR 1.0a outcome manager lifecycle: adders 0x0050e4b0..
   0x0050e770, removal, lookups, totals, text 0x0050e800, save 0x0050d750
   and load 0x0050d380. */
#include "player_outcome.h"

#include <assert.h>
#include <string.h>

static AoeOutcomeUnitRef *units[4];

static AoeOutcomeUnitRef *find(void *context, AoeOutcomePlayerRef *player,
                               int32_t id) {
    int *calls = context;
    ++*calls;
    for (int i = 0; i < 4; ++i)
        if (units[i] && units[i]->owner0c == player && units[i]->id04 == id)
            return units[i];
    return NULL;
}

int main(void) {
    static const char *names[] = {"Villager", "Archer", NULL, "Temple"};
    AoeOutcomeWorldRef world = {0};
    AoeOutcomePlayerRef p0 = {0}, p1 = {0}, p2 = {0};
    AoeOutcomePlayerRef *players[3] = {&p0, &p1, &p2};
    AoeOutcomeUnitRef temple = {.id04 = 40, .type_name = "Temple",
                                .owner0c = &p2};
    AoeOutcomeUnitRef hero = {.id04 = 7, .type_name = "Hero", .owner0c = &p1};
    AoeOutcomeManager m = {0}, loaded;
    AoeOutcomeCondition *c;
    AoeOutcomeEvent *e;
    uint8_t buffer[1024];
    AoeOutcomeStream out = {buffer, sizeof(buffer), 0, 0};
    AoeOutcomeStream in;
    char text[128];
    int calls = 0;

    world.player_count = 3;
    world.players = players;
    world.player_capacity = 3;
    p0.index4a = 0;
    p1.index4a = 1;
    p2.index4a = 2;
    p0.world3c = p1.world3c = p2.world3c = &world;
    p2.type_names = names;
    p2.type_count = 4;
    m.owner = &p1;

    /* Adders push to the front and count. */
    assert(aoe_outcome_add_destroy_count_ror(&m, 1, 3, 5, &p2));
    assert(aoe_outcome_add_bring_to_unit_ror(&m, 2, &hero, &temple));
    assert(aoe_outcome_add_explore_ror(&m, 3, 250, 1));
    assert(aoe_outcome_add_create_in_area_ror(&m, 4, 1, 2, 1.0f, 2.0f, 3.0f,
                                              4.5f, 1));
    assert(m.condition_count == 4);
    c = aoe_outcome_condition_at_ror(&m, 0);
    assert(c->kind == 8 && c->group == 4 && c->include_allies_2d == 1 &&
           c->min_x == 1.0f && c->min_y == 2.0f && c->max_x == 3.0f &&
           c->max_y == 4.5f && c->unit_target_count_1c == 2);
    c = aoe_outcome_condition_at_ror(&m, 1);
    assert(c->kind == 7 && c->progress_target_20 == 100);
    c = aoe_outcome_condition_at_ror(&m, 2);
    assert(c->kind == 5 && c->unit_24 == &hero && c->unit_28 == &temple &&
           c->player_08 == &p2 && c->include_allies_2d == 0);
    assert(aoe_outcome_condition_text_ror(c, text, sizeof(text)));
    assert(!strcmp(text, "Bring Hero to Temple"));
    c = aoe_outcome_condition_at_ror(&m, 3);
    assert(c->kind == 3 && c->threshold_1c == 5 && c->unit_class_04 == 3);
    assert(aoe_outcome_condition_text_ror(c, text, sizeof(text)));
    assert(!strcmp(text, "Destroy 5 Temple of player 2"));
    assert(!aoe_outcome_condition_at_ror(&m, 4));
    /* Class 2 has no name: empty text instead of a NULL dereference. */
    c->unit_class_04 = 2;
    assert(aoe_outcome_condition_text_ror(c, text, sizeof(text)));
    assert(text[0] == '\0');
    c->unit_class_04 = 3;
    {
        AoeOutcomeCondition explore = {.kind = 7, .progress_target_20 = 40};
        assert(aoe_outcome_condition_text_ror(&explore, text, sizeof(text)));
        assert(!strcmp(text, "explore 40 percent of the map"));
    }

    /* Events and the points totals. */
    e = aoe_outcome_event_add_ror(&m, 0, 2, 5, 3, 10, 4);
    e->contribution_10 = 30;
    e->sample_14 = 7.9f;
    e = aoe_outcome_event_add_ror(&m, 1, 1, 6, 4, 0, 2);
    e->contribution_10 = 12;
    assert(aoe_outcome_points_ror(&m, -1) == 42);
    assert(aoe_outcome_points_ror(&m, 2) == 30);
    assert(aoe_outcome_event_points_ror(&m, 6) == 12);
    assert(aoe_outcome_event_sample_ror(&m, 5) == 7);
    assert(aoe_outcome_event_sample_ror(&m, 9) == 0);
    m.score_sum_1c = 42;
    m.outcome = 2;
    assert(aoe_outcome_won_ror(&m));

    /* Save, then load: conditions keep order, events reverse, the hero
       (unit +0x24) is found in player 1's list and the temple (unit +0x28)
       misses the owner's list and is found under saved player 2. */
    assert(aoe_outcome_manager_save_ror(&m, &out));
    assert(out.pos == 4 + 4 + 1 + 4 * 44 + 4 + 4 + 2 * 24);
    units[0] = &hero;
    units[1] = &temple;
    in = (AoeOutcomeStream){buffer, out.pos, 0, 0};
    assert(aoe_outcome_manager_load_ror(&loaded, &p1, &in, 1, NULL, 0, find,
                                        &calls));
    assert(in.pos == out.pos);
    assert(loaded.condition_count == 4 && loaded.outcome == 2 &&
           loaded.owner == &p1 && loaded.score_sum_1c == 42);
    c = aoe_outcome_condition_at_ror(&loaded, 2);
    assert(c->kind == 5 && c->unit_24 == &hero && c->unit_28 == &temple &&
           c->player_08 == &p2 && c->status == 0);
    c = aoe_outcome_condition_at_ror(&loaded, 0);
    assert(c->kind == 8 && c->max_y == 4.5f && c->player_08 == NULL);
    assert(loaded.event_count == 2 && loaded.events->index_02 == 5 &&
           loaded.events->next->index_02 == 6);
    assert(loaded.events->sample_14 == 7.9f);

    /* Dropping the hero removes the condition that referenced it. */
    aoe_outcome_conditions_drop_unit_ror(&loaded, &hero);
    assert(loaded.condition_count == 3);
    assert(aoe_outcome_condition_at_ror(&loaded, 2)->kind == 3);
    aoe_outcome_manager_release_ror(&loaded);
    assert(!loaded.conditions && !loaded.events && !loaded.condition_count);

    /* Missing units and a remapped-away player mark status 3; a negative
       points sum clears event weights and is stored as 0. */
    units[0] = NULL;
    m.score_sum_1c = (uint32_t)-5;
    out = (AoeOutcomeStream){buffer, sizeof(buffer), 0, 0};
    assert(aoe_outcome_manager_save_ror(&m, &out));
    in = (AoeOutcomeStream){buffer, out.pos, 0, 0};
    {
        const int32_t map[3] = {0, 1, -1};
        assert(aoe_outcome_manager_load_ror(&loaded, &p1, &in, 1, map, 3,
                                            find, &calls));
    }
    assert(aoe_outcome_condition_at_ror(&loaded, 2)->status == 3);
    assert(aoe_outcome_condition_at_ror(&loaded, 3)->status == 3);
    assert(aoe_outcome_condition_at_ror(&loaded, 1)->status == 0);
    assert(loaded.score_sum_1c == 0 && loaded.events->weight_0c == 0);
    aoe_outcome_manager_release_ror(&loaded);

    /* A short stream fails cleanly. */
    in = (AoeOutcomeStream){buffer, 20, 0, 0};
    assert(!aoe_outcome_manager_load_ror(&loaded, &p1, &in, 1, NULL, 0, find,
                                         &calls));
    assert(!loaded.conditions);

    aoe_outcome_condition_remove_ror(&m, m.conditions);
    assert(m.condition_count == 3);
    aoe_outcome_event_remove_ror(&m, m.events);
    assert(m.event_count == 1);
    aoe_outcome_manager_release_ror(&m);
    assert(!m.conditions && !m.events && !m.owner);
    return 0;
}
