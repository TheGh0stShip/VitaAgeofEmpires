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
