#include "player_diplomacy.h"

#include <stddef.h>

static int in_range(const AoePlayerDiplomacyRor *p, int32_t other) {
    return p && p->stances && p->relations && other >= 0 &&
           (uint32_t)other < p->count;
}

int aoe_player_stance_ror(const AoePlayerDiplomacyRor *player, int32_t other,
                          uint8_t *stance) {
    if (!stance || !in_range(player, other)) return 0;
    *stance = player->stances[other];
    return 1;
}

int aoe_player_set_stance_ror(AoePlayerDiplomacyRor *player, int32_t other,
                              uint8_t stance,
                              const AoePlayerDiplomacyOps *ops) {
    if (!in_range(player, other) || !ops || !ops->notify_objects) return 0;
    player->stances[other] = stance;
    if (other == 0) player->relations[0] = 0;
    else if (other == player->player_id) player->relations[other] = 1;
    else if (stance == 0) player->relations[other] = 2;
    else if (stance == 1) player->relations[other] = 3;
    else player->relations[other] = 4;
    ops->notify_objects(ops->context, player, other, stance);
    return 1;
}

static int stance_is(const AoePlayerDiplomacyRor *player,
                     int16_t player_count, int32_t other, uint8_t stance) {
    if (!player || other == player->player_id || other <= 0 ||
        other >= player_count)
        return 0;
    if (!player->stances || (uint32_t)other >= player->count) return 0;
    return player->stances[other] == stance;
}

int aoe_player_is_enemy_ror(const AoePlayerDiplomacyRor *player,
                            int16_t player_count, int32_t other) {
    return stance_is(player, player_count, other, 3);
}

int aoe_player_is_ally_ror(const AoePlayerDiplomacyRor *player,
                           int16_t player_count, int32_t other) {
    if (player && other == player->player_id) return 1;
    return stance_is(player, player_count, other, 0);
}

int aoe_player_is_neutral_ror(const AoePlayerDiplomacyRor *player,
                              int16_t player_count, int32_t other) {
    return stance_is(player, player_count, other, 1);
}

int aoe_player_all_neutral_ror(const AoePlayerDiplomacyRor *player,
                               int16_t player_count) {
    int32_t i;
    if (!player) return 0;
    for (i = 1; i < player_count; ++i) {
        if (i == player->player_id) continue;
        if (!aoe_player_is_neutral_ror(player, player_count, i)) return 0;
    }
    return 1;
}
