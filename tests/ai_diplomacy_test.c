/* Patched RoR 1.0a Diplomacy AI 0x0040a780..0x0040af80, Emotional AI
   0x0040afc0..0x0040b2d0, InfAI 0x0040b470..0x0040b4c0 and Main Decision
   AI 0x0040b4d0..0x0040be70. */
#include "ai_decision.h"
#include "ai_diplomacy.h"

#include <assert.h>
#include <string.h>

typedef struct {
    uint8_t state[9];
    int allies_us[9];
    int32_t points[9];
    int32_t requested_player, requested_stance;
} Players;

static uint8_t state(void *c, int32_t p) { return ((Players *)c)->state[p]; }
static int allied(void *c, int32_t p) { return ((Players *)c)->allies_us[p]; }
static int32_t points(void *c, int32_t p) { return ((Players *)c)->points[p]; }
static void request(void *c, int32_t p, int32_t s) {
    ((Players *)c)->requested_player = p;
    ((Players *)c)->requested_stance = s;
}

static AoeAiObjectViewRor objects[4];

static int object(void *c, int32_t id, AoeAiObjectViewRor *out) {
    (void)c;
    for (int i = 0; i < 4; ++i)
        if (objects[i].id == id) {
            *out = objects[i];
            return 1;
        }
    return 0;
}

static int accept(void *c, int32_t id, int32_t arg) {
    (void)c;
    return id == arg;
}

int main(void) {
    static AoeAiDiplomacyRor ai, loaded;
    static AoeAiEmotionalRor emo, emo2;
    static AoeAiDecisionRor dec, dec2;
    AoeAiModuleRor inf;
    Players players = {{0}};
    /* We are player 2 of 5 (gaia + 4); toward 1 ally, 3 enemy, 4 neutral. */
    uint8_t stances[6] = {0, 0, 0, 3, 1, 3};
    AoeAiDiplomacyOps ops = {&players, 2, 5, stances, state, allied, points,
                             request};
    uint8_t buffer[512];
    AoeOutcomeStream out = {buffer, sizeof(buffer), 0, 0}, in;
    AoeAiObjectViewRor view;

    aoe_ai_diplomacy_init_ror(&ai, 2, NULL);
    assert(ai.module.id == 0x3eb && ai.dislike[9] == 49 && ai.changeable[0]);
    assert(aoe_ai_diplomacy_get_ror(&ai, &ops, 1, AOE_AI_NEUTRALITY) == 100);
    assert(aoe_ai_diplomacy_get_ror(&ai, &ops, 5, AOE_AI_LIKE) == -1);
    assert(aoe_ai_diplomacy_get_ror(&ai, &ops, 0, AOE_AI_LIKE) == -1);

    /* Stance tests; enemy/neutral accept player == count. */
    assert(aoe_ai_diplomacy_is_ally_ror(&ops, 2) &&
           aoe_ai_diplomacy_is_ally_ror(&ops, 1));
    assert(aoe_ai_diplomacy_is_enemy_ror(&ops, 3) &&
           aoe_ai_diplomacy_is_enemy_ror(&ops, 5));
    assert(aoe_ai_diplomacy_is_neutral_ror(&ops, 4) &&
           !aoe_ai_diplomacy_is_enemy_ror(&ops, 2));

    /* Update: ally 1 does not return it -> dislike; enemy 3 likes us. */
    players.allies_us[3] = 1;
    assert(aoe_ai_diplomacy_update_ror(&ai, &ops));
    assert(ai.dislike[1] == 50 && ai.like[3] == 50 && ai.like[1] == 49);

    /* Clamps for fixed and changeable players. */
    aoe_ai_diplomacy_set_changeable_ror(&ai, &ops, 4, 0);
    assert(!aoe_ai_diplomacy_changeable_ror(&ai, &ops, 4));
    aoe_ai_diplomacy_set_ror(&ai, &ops, 4, AOE_AI_DISLIKE, 10);
    assert(ai.dislike[4] == 50);
    aoe_ai_diplomacy_set_ror(&ai, &ops, 4, AOE_AI_LIKE, 90);
    assert(ai.like[4] == 48);
    aoe_ai_diplomacy_add_ror(&ai, &ops, 3, AOE_AI_DISLIKE, -80);
    assert(ai.dislike[3] == 0);
    aoe_ai_diplomacy_add_ror(&ai, &ops, 3, AOE_AI_LIKE, 80);
    assert(ai.like[3] == 100);
    assert(aoe_ai_diplomacy_get_ror(&ai, &ops, 3, AOE_AI_NEUTRALITY) == 0);

    /* Most disliked: 4 (50) vs 1 (50, ally) vs 3 (0); points shift it. */
    ai.dislike[1] = 70;
    assert(aoe_ai_diplomacy_most_disliked_ror(&ai, &ops, 0, 0, 0, 0) == 1);
    assert(aoe_ai_diplomacy_most_disliked_ror(&ai, &ops, 1, 1, 0, 0) == 4);
    players.points[3] = 400;
    assert(aoe_ai_diplomacy_most_disliked_ror(&ai, &ops, 0, 0, 1, 4) == 3);
    players.state[1] = 2;
    assert(aoe_ai_diplomacy_most_disliked_ror(&ai, &ops, 0, 0, 0, 0) == 4);
    assert(aoe_ai_diplomacy_most_liked_ror(&ai, &ops, 0, 0) == 3);
    assert(aoe_ai_diplomacy_most_liked_ror(&ai, &ops, 1, 3) == 4);

    aoe_ai_diplomacy_request_ror(&ai, &ops, 4, 0); /* fixed: ignored */
    assert(players.requested_player == 0);
    aoe_ai_diplomacy_request_ror(&ai, &ops, 3, 0);
    assert(players.requested_player == 3 && players.requested_stance == 0);

    assert(aoe_ai_diplomacy_save_ror(&ai, &out) && out.pos == 90);
    in = (AoeOutcomeStream){buffer, out.pos, 0, 0};
    assert(aoe_ai_diplomacy_load_ror(&loaded, 2, NULL, &in));
    assert(!memcmp(loaded.dislike, ai.dislike, sizeof(ai.dislike)) &&
           !memcmp(loaded.like, ai.like, sizeof(ai.like)) &&
           !memcmp(loaded.changeable, ai.changeable, sizeof(ai.changeable)));
    aoe_ai_diplomacy_release_ror(&loaded);
    aoe_ai_diplomacy_release_ror(&ai);

    /* Emotional AI. */
    aoe_ai_emotional_init_ror(&emo, 1, NULL);
    assert(emo.module.id == 0x3ec && emo.traits[5] == 49 &&
           !strcmp(emo.names[AOE_AI_VENGEFUL], "Vengeful"));
    aoe_ai_emotional_preset_ror(&emo, 5);
    assert(emo.traits[AOE_AI_AGGRESSIVE] == 90 &&
           emo.traits[AOE_AI_VENGEFUL] == 100 &&
           emo.traits[AOE_AI_COMPASSIONATE] == 0);
    aoe_ai_emotional_preset_ror(&emo, 6);
    assert(emo.traits[AOE_AI_FRIENDLY] == 30);
    out = (AoeOutcomeStream){buffer, sizeof(buffer), 0, 0};
    assert(aoe_ai_emotional_save_ror(&emo, &out) && out.pos == 24);
    in = (AoeOutcomeStream){buffer, out.pos, 0, 0};
    assert(aoe_ai_emotional_load_ror(&emo2, 1, NULL, &in));
    assert(!memcmp(emo2.traits, emo.traits, sizeof(emo.traits)) &&
           !strcmp(emo2.names[0], "Aggressive"));
    aoe_ai_emotional_release_ror(&emo2);
    aoe_ai_emotional_release_ror(&emo);
    aoe_ai_inf_init_ror(&inf, 3, NULL);
    assert(inf.id == 0x3ed && !strcmp(inf.name, "InfAI"));
    aoe_ai_inf_release_ror(&inf);

    /* Main Decision AI object list. */
    aoe_ai_decision_init_ror(&dec, 1, NULL, NULL);
    assert(dec.module.id == 1000);
    assert(!aoe_ai_decision_add_ror(&dec, 0, 5));
    assert(aoe_ai_decision_add_ror(&dec, 1, 10));
    assert(aoe_ai_decision_add_ror(&dec, 1, 11));
    assert(aoe_ai_decision_add_ror(&dec, 1, 10) && dec.count == 2);
    assert(aoe_ai_decision_add_ror(&dec, 1, 12) && dec.count == 3);
    objects[0] = (AoeAiObjectViewRor){.id = 10, .type = 83, .unit_class = 4,
                                      .max_hp = 25, .hp = 12.9f,
                                      .field_44 = 3.0f, .state = 2,
                                      .method_90 = 1, .has_ai = 1,
                                      .ai_order = 7, .ai_34 = 1, .ai_38 = 2};
    objects[1] = (AoeAiObjectViewRor){.id = 11, .type = 83, .unit_class = 4,
                                      .max_hp = 25, .hp = 20.0f, .state = 2,
                                      .method_90 = 1, .has_ai = 1,
                                      .ai_order = 9};
    objects[2] = (AoeAiObjectViewRor){.id = 12, .type = 75, .unit_class = 0,
                                      .has_ai = 0};
    assert(aoe_ai_decision_object_ror(&(AoeAiDecisionOps){NULL, object, accept},
                                      12, &view) && view.type == 75);
    {
        AoeAiDecisionOps dops = {NULL, object, accept};
        assert(aoe_ai_decision_count_order_ror(&dec, &dops, 7) == 1);
        assert(aoe_ai_decision_find_ror(&dec, &dops, -1, 83, 4, -1, 11, -1, -1,
                                        -1, -1, &view) && view.id == 11);
        assert(aoe_ai_decision_find_ror(&dec, &dops, -1, -1, -1, 7, -1, 1, 2,
                                        2, 3, &view) && view.id == 10);
        assert(!aoe_ai_decision_find_ror(&dec, &dops, -1, -1, -1, 7, -1, -1,
                                         -1, -1, 2, &view));
        assert(!aoe_ai_decision_find_ror(&dec, &dops, 12, -1, -1, -1, -1, -1,
                                         -1, -1, -1, &view)); /* no AI */
        assert(aoe_ai_decision_find_other_ror(&dec, &dops, -1, 83, -1, 7, 8,
                                              -1, &view) && view.id == 11);
        assert(aoe_ai_decision_healthiest_damaged_ror(&dec, &dops, 4, -1,
                                                      &view) && view.id == 11);
        objects[1].hp = 25.0f; /* undamaged */
        assert(aoe_ai_decision_healthiest_damaged_ror(&dec, &dops, 4, 83,
                                                      &view) && view.id == 10);
    }
    out = (AoeOutcomeStream){buffer, sizeof(buffer), 0, 0};
    assert(aoe_ai_decision_save_ror(&dec, &out) && out.pos == 16);
    buffer[0] = 4; /* claim four IDs, append a duplicate 10 */
    memcpy(buffer + 16, buffer + 4, 4);
    in = (AoeOutcomeStream){buffer, 20, 0, 0};
    assert(aoe_ai_decision_load_ror(&dec2, 1, NULL, NULL, &in));
    assert(dec2.count == 3 && dec2.ids[2] == 12);
    assert(aoe_ai_decision_remove_ror(&dec2, 11) && dec2.count == 2 &&
           dec2.ids[1] == 12);
    assert(!aoe_ai_decision_remove_ror(&dec2, 99));
    assert(aoe_ai_unknown_action_ror(&dec2.module, 77) == -1);
    aoe_ai_decision_release_ror(&dec2);
    aoe_ai_decision_release_ror(&dec);
    assert(!dec.ids && !dec.capacity);
    return 0;
}
