#include <assert.h>
#include <string.h>

#include "player_diplomacy.h"
#include "player_victory.h"

typedef struct {
    int events, event, event_player, local_lost;
    int session, local;
    uint8_t left[9];
} Game;

static void game_event(void *c, int32_t event, int32_t player) {
    Game *g = c;
    g->events++;
    g->event = event;
    g->event_player = player;
}
static int32_t slot_player(void *c, int32_t slot) {
    (void)c;
    return slot == 2 || slot == 5 ? 3 : 1;
}
static int session(void *c) { return ((Game *)c)->session; }
static int32_t local_slot(void *c) { return ((Game *)c)->local; }
static void local_lost(void *c) { ((Game *)c)->local_lost++; }

static void test_set_state(void) {
    Game g;
    AoePlayerStateOps ops;
    AoePlayerVictoryRor p;

    memset(&g, 0, sizeof(g));
    ops = (AoePlayerStateOps){&g, game_event, slot_player, g.left,
                              session, local_slot, local_lost};
    memset(&p, 0, sizeof(p));
    p.player_id = 3;
    aoe_player_set_state_ror(&p, 1, &ops);
    assert(p.state == 1 && g.events == 0);
    g.session = 1;
    g.local = 5;
    aoe_player_set_state_ror(&p, 2, &ops);
    assert(p.state == 2 && g.events == 1 && g.event == 6 &&
           g.event_player == 3);
    assert(g.left[2] == 1 && g.left[5] == 1 && g.left[1] == 0);
    assert(g.local_lost == 1);
    aoe_player_set_state_ror(&p, 2, &ops); /* unchanged: nothing */
    assert(g.events == 1);

    /* A resigned player only accepts state 2, silently. */
    memset(&p, 0, sizeof(p));
    p.player_id = 3;
    p.resigned = 1;
    aoe_player_set_state_ror(&p, 1, &ops);
    assert(p.state == 0);
    aoe_player_set_state_ror(&p, 2, &ops);
    assert(p.state == 2 && g.events == 1);
}

static void test_victory(void) {
    /* Players 1..3; 1 and 2 allied, 3 enemy. */
    uint8_t s1[4] = {0, 0, 0, 3}, s2[4] = {0, 0, 0, 3}, s3[4] = {0, 3, 3, 0};
    AoePlayerVictoryRor p0, p1, p2, p3;
    AoePlayerVictoryRor *list[4] = {&p0, &p1, &p2, &p3};
    AoePlayerVictoryWorldRor world = {list, 4, 0};
    AoePlayerStateOps ops;
    Game g;
    int32_t count;

    memset(&g, 0, sizeof(g));
    ops = (AoePlayerStateOps){&g, game_event, slot_player, g.left,
                              session, local_slot, local_lost};
    memset(&p0, 0, sizeof(p0));
    p1 = (AoePlayerVictoryRor){1, 0, 0, 1, s1, 2};
    p2 = (AoePlayerVictoryRor){2, 0, 0, 1, s2, 0};
    p3 = (AoePlayerVictoryRor){3, 0, 0, 1, s3, 0};

    assert(aoe_player_lost_or_conditions_met_ror(&p1));
    assert(!aoe_player_lost_or_conditions_met_ror(&p2));

    count = -1;
    assert(aoe_player_allies_agree_ror(&p1, &world, &count) == 1);
    assert(count == 3);
    assert(aoe_player_allies_agree_ror(&p1, &world, &count) == 1);
    count = 2; /* a different expected count fails */
    assert(aoe_player_allies_agree_ror(&p1, &world, &count) == 0);
    p2.allied_victory = 0;
    count = -1;
    assert(aoe_player_allies_agree_ror(&p1, &world, &count) == 0);
    assert(count == 1);
    p2.allied_victory = 1;
    p2.state = 2; /* a lost player keeps the count */
    count = 7;
    assert(aoe_player_allies_agree_ror(&p2, &world, &count) == 1);
    assert(count == 7);
    p2.state = 0;

    /* Ally 2 has not met its conditions: no win. */
    assert(aoe_player_check_victory_ror(&p1, &world, &ops) == 0);
    assert(world.game_over == 0);
    p2.condition_state = 2;
    assert(aoe_player_check_victory_ror(&p1, &world, &ops) == 1);
    assert(p1.state == 1 && world.game_over == 2 && g.events == 0);

    /* Without allied victory the allies do not matter. */
    p1.state = 0;
    p2.condition_state = 0;
    p1.allied_victory = 0;
    world.game_over = 0;
    assert(aoe_player_check_victory_ror(&p1, &world, &ops) == 1);
    /* Conditions not met: unchanged. */
    p3.condition_state = 0;
    assert(aoe_player_check_victory_ror(&p3, &world, &ops) == 0);
    /* An ally that disagrees blocks the win. */
    p1.state = 0;
    p1.allied_victory = 1;
    p2.condition_state = 2;
    s2[1] = 1;
    assert(aoe_player_check_victory_ror(&p1, &world, &ops) == 0);
}

static void test_stance_predicates(void) {
    uint8_t stances[5] = {0, 0, 3, 1, 1};
    int32_t relations[5] = {0};
    AoePlayerDiplomacyRor p = {1, stances, relations, 5, 0, 0};

    assert(aoe_player_is_enemy_ror(&p, 5, 2) == 1);
    assert(aoe_player_is_enemy_ror(&p, 2, 2) == 0); /* beyond the count */
    assert(aoe_player_is_neutral_ror(&p, 5, 3) == 1);
    assert(aoe_player_is_neutral_ror(&p, 5, 0) == 0);
    assert(aoe_player_is_ally_ror(&p, 5, 1) == 1);  /* itself */
    assert(aoe_player_is_ally_ror(&p, 5, 0) == 0);  /* gaia */
    assert(aoe_player_is_ally_ror(&p, 5, 2) == 0);
    assert(aoe_player_all_neutral_ror(&p, 5) == 0);
    stances[2] = 1;
    assert(aoe_player_all_neutral_ror(&p, 5) == 1);
    assert(aoe_player_all_neutral_ror(&p, 1) == 1);
}

int main(void) {
    test_set_state();
    test_victory();
    test_stance_predicates();
    return 0;
}
