#include "ai_diplomacy.h"

#include <string.h>

#include "aoe_stream.h"

void aoe_ai_diplomacy_init_ror(AoeAiDiplomacyRor *ai, int32_t player,
                               AoeAiLogRor *log) {
    if (!ai) return;
    memset(ai, 0, sizeof(*ai));
    aoe_ai_module_init_ror(&ai->module, "Diplomacy AI", 0x3eb, player, log);
    for (int i = 0; i < AOE_AI_DIPLOMACY_SLOTS; ++i) {
        ai->dislike[i] = 49;
        ai->like[i] = 49;
        ai->changeable[i] = 1;
    }
}

int aoe_ai_diplomacy_load_ror(AoeAiDiplomacyRor *ai, int32_t player,
                              AoeAiLogRor *log, AoeOutcomeStream *in) {
    if (!ai || !in || !in->data) return 0;
    memset(ai, 0, sizeof(*ai));
    aoe_ai_module_init_ror(&ai->module, "Diplomacy AI", 0x3eb, player, log);
    for (int i = 0; i < AOE_AI_DIPLOMACY_SLOTS; ++i) {
        ai->dislike[i] = aoe_stream_i32(in);
        ai->like[i] = aoe_stream_i32(in);
        ai->changeable[i] = aoe_stream_u8(in);
    }
    return !in->failed;
}

int aoe_ai_diplomacy_save_ror(const AoeAiDiplomacyRor *ai,
                              AoeOutcomeStream *out) {
    if (!ai || !out || !out->data) return 0;
    for (int i = 0; i < AOE_AI_DIPLOMACY_SLOTS; ++i) {
        aoe_stream_put_i32(out, ai->dislike[i]);
        aoe_stream_put_i32(out, ai->like[i]);
        aoe_stream_put_u8(out, ai->changeable[i]);
    }
    return !out->failed;
}

void aoe_ai_diplomacy_release_ror(AoeAiDiplomacyRor *ai) {
    if (ai) aoe_ai_module_release_ror(&ai->module);
}

/* 0 < player < count, within the ten slots. */
static int other(const AoeAiDiplomacyOps *ops, int32_t player) {
    return ops && player > 0 && player < ops->player_count &&
           player < AOE_AI_DIPLOMACY_SLOTS;
}

static int32_t magnitude(int32_t v) { return v < 0 ? -v : v; }

int32_t aoe_ai_diplomacy_get_ror(const AoeAiDiplomacyRor *ai,
                                 const AoeAiDiplomacyOps *ops,
                                 int32_t player, int kind) {
    int32_t v;
    if (!ai || !other(ops, player)) return -1;
    switch (kind) {
    case AOE_AI_DISLIKE: return ai->dislike[player];
    case AOE_AI_NEUTRALITY:
        v = 100 - magnitude(49 - ai->like[player]) -
            magnitude(49 - ai->dislike[player]);
        return v < 0 ? 0 : v;
    case AOE_AI_LIKE: return ai->like[player];
    default: return -1;
    }
}

static void store(AoeAiDiplomacyRor *ai, int32_t player, int kind,
                  int32_t value) {
    if (kind == AOE_AI_DISLIKE) {
        int32_t low = ai->changeable[player] ? 0 : 50;
        ai->dislike[player] = value < low ? low : value > 100 ? 100 : value;
    } else if (kind == AOE_AI_LIKE) {
        int32_t high = ai->changeable[player] ? 100 : 48;
        ai->like[player] = value < 0 ? 0 : value > high ? high : value;
    }
}

void aoe_ai_diplomacy_add_ror(AoeAiDiplomacyRor *ai,
                              const AoeAiDiplomacyOps *ops, int32_t player,
                              int kind, int32_t delta) {
    if (!ai || !other(ops, player)) return;
    if (kind == AOE_AI_DISLIKE)
        store(ai, player, kind, ai->dislike[player] + delta);
    else if (kind == AOE_AI_LIKE)
        store(ai, player, kind, ai->like[player] + delta);
}

void aoe_ai_diplomacy_set_ror(AoeAiDiplomacyRor *ai,
                              const AoeAiDiplomacyOps *ops, int32_t player,
                              int kind, int32_t value) {
    if (ai && other(ops, player)) store(ai, player, kind, value);
}

int aoe_ai_diplomacy_changeable_ror(const AoeAiDiplomacyRor *ai,
                                    const AoeAiDiplomacyOps *ops,
                                    int32_t player) {
    return ai && other(ops, player) && ai->changeable[player] == 1;
}

void aoe_ai_diplomacy_set_changeable_ror(AoeAiDiplomacyRor *ai,
                                         const AoeAiDiplomacyOps *ops,
                                         int32_t player, uint8_t value) {
    if (ai && other(ops, player)) ai->changeable[player] = value;
}

static int stance_is(const AoeAiDiplomacyOps *ops, int32_t player,
                     uint8_t stance) {
    return ops && ops->stances && player > 0 && player <= ops->player_count &&
           player != ops->self && ops->stances[player] == stance;
}

int aoe_ai_diplomacy_is_enemy_ror(const AoeAiDiplomacyOps *ops,
                                  int32_t player) {
    return stance_is(ops, player, 3);
}

int aoe_ai_diplomacy_is_neutral_ror(const AoeAiDiplomacyOps *ops,
                                    int32_t player) {
    return stance_is(ops, player, 1);
}

int aoe_ai_diplomacy_is_ally_ror(const AoeAiDiplomacyOps *ops,
                                 int32_t player) {
    if (!ops) return 0;
    if (player == ops->self) return 1;
    return ops->stances && player > 0 && player <= ops->player_count &&
           ops->stances[player] == 0;
}

int aoe_ai_diplomacy_allied_by_ror(const AoeAiDiplomacyOps *ops,
                                   int32_t player) {
    return ops && ops->considers_ally && player > 0 &&
           player < ops->player_count &&
           ops->considers_ally(ops->context, player) != 0;
}

int aoe_ai_diplomacy_update_ror(AoeAiDiplomacyRor *ai,
                                const AoeAiDiplomacyOps *ops) {
    if (!ai || !ops) return 0;
    for (int32_t p = 1; p < ops->player_count; ++p) {
        if (p == ops->self) continue;
        if (aoe_ai_diplomacy_is_ally_ror(ops, p) &&
            !aoe_ai_diplomacy_allied_by_ror(ops, p))
            aoe_ai_diplomacy_add_ror(ai, ops, p, AOE_AI_DISLIKE, 1);
        if (!aoe_ai_diplomacy_is_ally_ror(ops, p) &&
            aoe_ai_diplomacy_allied_by_ror(ops, p))
            aoe_ai_diplomacy_add_ror(ai, ops, p, AOE_AI_LIKE, 1);
    }
    return 1;
}

int32_t aoe_ai_diplomacy_most_disliked_ror(const AoeAiDiplomacyRor *ai,
                                           const AoeAiDiplomacyOps *ops,
                                           int32_t use_spare, int32_t spare,
                                           int32_t mode, int32_t divisor) {
    int32_t best = -1, best_value = -1;
    if (!ai || !ops || !ops->state || !ops->points || ops->player_count < 2)
        return -1;
    for (int32_t p = 1; p < ops->player_count &&
                        p < AOE_AI_DIPLOMACY_SLOTS; ++p) {
        int32_t value, points;
        if (p == ops->self || ops->state(ops->context, p) == 2) continue;
        if (use_spare > 0 && spare == p && aoe_ai_diplomacy_is_ally_ror(ops, p))
            continue;
        points = ops->points(ops->context, p);
        value = ai->dislike[p];
        if (divisor > 1) {
            if (mode == 1) value += points / divisor;
            else value -= points / divisor;
        }
        if (best_value < value) {
            best_value = value;
            best = p;
        }
    }
    return best;
}

int32_t aoe_ai_diplomacy_most_liked_ror(const AoeAiDiplomacyRor *ai,
                                        const AoeAiDiplomacyOps *ops,
                                        int32_t use_spare, int32_t spare) {
    int32_t best = -1, best_value = -1;
    if (!ai || !ops || !ops->state || ops->player_count < 2) return -1;
    for (int32_t p = 1; p < ops->player_count &&
                        p < AOE_AI_DIPLOMACY_SLOTS; ++p) {
        if (p == ops->self || ops->state(ops->context, p) == 2) continue;
        if (use_spare >= 1 && spare == p &&
            aoe_ai_diplomacy_is_enemy_ror(ops, p)) continue;
        if (best_value < ai->like[p]) {
            best_value = ai->like[p];
            best = p;
        }
    }
    return best;
}

void aoe_ai_diplomacy_request_ror(const AoeAiDiplomacyRor *ai,
                                  const AoeAiDiplomacyOps *ops,
                                  int32_t player, int32_t stance) {
    if (ai && other(ops, player) && ai->changeable[player] && ops->request)
        ops->request(ops->context, player, stance);
}

/* ---- Emotional AI ---- */

static const char *const trait_names[AOE_AI_TRAITS] = {
    "Aggressive", "Compassionate", "Defensive", "Friendly", "Passive",
    "Vengeful"};

static void emotional_base(AoeAiEmotionalRor *ai, int32_t player,
                           AoeAiLogRor *log) {
    memset(ai, 0, sizeof(*ai));
    aoe_ai_module_init_ror(&ai->module, "Emotional AI", 0x3ec, player, log);
    for (int i = 0; i < AOE_AI_TRAITS; ++i)
        strcpy(ai->names[i], trait_names[i]);
}

void aoe_ai_emotional_init_ror(AoeAiEmotionalRor *ai, int32_t player,
                               AoeAiLogRor *log) {
    if (!ai) return;
    emotional_base(ai, player, log);
    for (int i = 0; i < AOE_AI_TRAITS; ++i) ai->traits[i] = 49;
}

int aoe_ai_emotional_load_ror(AoeAiEmotionalRor *ai, int32_t player,
                              AoeAiLogRor *log, AoeOutcomeStream *in) {
    if (!ai || !in || !in->data) return 0;
    emotional_base(ai, player, log);
    for (int i = 0; i < AOE_AI_TRAITS; ++i) ai->traits[i] = aoe_stream_i32(in);
    return !in->failed;
}

int aoe_ai_emotional_save_ror(const AoeAiEmotionalRor *ai,
                              AoeOutcomeStream *out) {
    if (!ai || !out || !out->data) return 0;
    for (int i = 0; i < AOE_AI_TRAITS; ++i)
        aoe_stream_put_i32(out, ai->traits[i]);
    return !out->failed;
}

void aoe_ai_emotional_release_ror(AoeAiEmotionalRor *ai) {
    if (ai) aoe_ai_module_release_ror(&ai->module);
}

void aoe_ai_emotional_preset_ror(AoeAiEmotionalRor *ai, int32_t preset) {
    /* Aggressive, compassionate, defensive, friendly, passive, vengeful. */
    static const int32_t presets[6][AOE_AI_TRAITS] = {
        {100, 10, 0, 10, 10, 70},  {10, 100, 50, 80, 50, 0},
        {20, 50, 100, 50, 60, 10}, {10, 70, 50, 100, 50, 20},
        {20, 70, 60, 50, 100, 0},  {90, 0, 50, 30, 10, 100}};
    if (!ai || preset < 0 || preset > 5) return;
    memcpy(ai->traits, presets[preset], sizeof(ai->traits));
}

/* ---- InfAI ---- */

void aoe_ai_inf_init_ror(AoeAiModuleRor *ai, int32_t player,
                         AoeAiLogRor *log) {
    aoe_ai_module_init_ror(ai, "InfAI", 0x3ed, player, log);
}

void aoe_ai_inf_release_ror(AoeAiModuleRor *ai) {
    aoe_ai_module_release_ror(ai);
}
