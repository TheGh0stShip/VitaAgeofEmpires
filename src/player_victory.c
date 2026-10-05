#include "player_victory.h"

#include <stddef.h>

void aoe_player_set_state_ror(AoePlayerVictoryRor *player, uint8_t state,
                              const AoePlayerStateOps *ops) {
    int32_t slot;

    if (!player || state == player->state) return;
    if (player->resigned && state != 2) return;
    player->state = state;
    if (state != 2 || player->resigned) return;
    if (!ops) return;
    if (ops->game_event) ops->game_event(ops->context, 6, player->player_id);
    for (slot = 0; slot < 9; ++slot) {
        if (!ops->slot_player ||
            ops->slot_player(ops->context, slot) != player->player_id)
            continue;
        if (ops->slot_left) ops->slot_left[slot] = 1;
        if (ops->session && ops->session(ops->context) && ops->local_slot &&
            slot == ops->local_slot(ops->context) && ops->local_lost)
            ops->local_lost(ops->context);
    }
}

int aoe_player_lost_or_conditions_met_ror(const AoePlayerVictoryRor *player) {
    if (!player) return 0;
    if (player->state == 2) return 1;
    return player->condition_state == 2;
}

int aoe_player_allies_agree_ror(const AoePlayerVictoryRor *player,
                                const AoePlayerVictoryWorldRor *world,
                                int32_t *count) {
    int32_t start;
    int result = 1;
    int32_t i;

    if (!player || !world || !count) return 0;
    start = *count;
    if (player->state != 2) {
        *count = 0;
        for (i = 1; i < world->player_count; ++i) {
            const AoePlayerVictoryRor *other;
            if (!player->stances) {
                result = 0;
                break;
            }
            if (player->stances[i] == 0) {
                other = world->players ? world->players[i] : NULL;
                if (!other || !other->stances || player->player_id < 0 ||
                    player->player_id >= world->player_count ||
                    other->stances[player->player_id] != 0 ||
                    other->allied_victory == 0) {
                    result = 0;
                    break;
                }
            }
            ++*count;
        }
    }
    if (start >= 0 && *count != start) return 0;
    return result;
}

uint8_t aoe_player_check_victory_ror(AoePlayerVictoryRor *player,
                                     AoePlayerVictoryWorldRor *world,
                                     const AoePlayerStateOps *ops) {
    int32_t agreed = -1;
    int disagree = 0;
    int32_t i;

    if (!player || !world) return player ? player->state : 0;
    if (player->state == 2 || player->condition_state != 2)
        return player->state;
    if (player->allied_victory) {
        for (i = 1; i < world->player_count; ++i) {
            const AoePlayerVictoryRor *other;
            if (i != player->player_id &&
                (!player->stances || player->stances[i] != 0))
                continue;
            other = world->players ? world->players[i] : NULL;
            if (!aoe_player_allies_agree_ror(other, world, &agreed)) {
                disagree = 1;
                break;
            }
        }
        if (disagree) return player->state;
        for (i = 1; i < world->player_count; ++i) {
            const AoePlayerVictoryRor *other;
            if (!player->stances || player->stances[i] != 0) continue;
            other = world->players ? world->players[i] : NULL;
            if (!aoe_player_lost_or_conditions_met_ror(other))
                return player->state;
        }
    }
    aoe_player_set_state_ror(player, 1, ops);
    world->game_over = 2;
    return player->state;
}
