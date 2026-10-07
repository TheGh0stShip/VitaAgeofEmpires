#include "command_game_apply.h"

#include "command_production.h"

static int valid_player(const AoeCommandGameCheatOpsRor *ops, int16_t p) {
    return p >= 0 && p < ops->player_count;
}

/* 0x004e9490(building, unit) on world +0x58: the 0x10-byte train record
   (opcode 0x64, building ID +0x04, owner byte +0x08, unit +0x0a, request
   -1 +0x0c) handed to the queue 0x42b6e0; nothing for a NULL building. */
static void train(const AoeCommandGameCheatOpsRor *ops,
                  const AoeLiveUnit *building, int16_t unit) {
    uint8_t record[AOE_COMMAND_PRODUCTION_SIZE_ROR];
    size_t n;
    if (!building) return;
    n = aoe_command_encode_production_ror(
        record, sizeof record, AOE_COMMAND_TRAIN_ROR,
        building->core.unit_id,
        (uint8_t)ops->owner_id(ops->context, building), unit, -1);
    if (n) ops->queue(ops->context, record, n);
}

/* Inlined in 0x0050c570: the player's +0x28 list's 0x451370(template,
   -1.0f, -1.0f, 1, 2, 0) unit trains `unit`; 0 when there is none. */
static int train_at(const AoeCommandGameCheatOpsRor *ops, int16_t player,
                    int16_t template_id, int16_t unit) {
    AoeLiveUnit *b;
    if (!ops->find_finished || !ops->owner_id || !ops->queue) return -1;
    if (!valid_player(ops, player)) return 0;
    b = ops->find_finished(ops->context, player, template_id);
    if (!b) return 0;
    train(ops, b, unit);
    return 1;
}

/* Inlined in 0x0050c570 (codes 100, 0x65, 200..209): every unit on the
   player's +0x28 list in state (+0x48) below 3 whose template class
   (+0x08 -> +0x14) is not 1 gets vtable +0x6c; the next link is read
   after the call. */
static void kill_units(const AoeCommandGameCheatOpsRor *ops, int16_t player) {
    AoeObjectUpdateNode *node;
    if (!valid_player(ops, player)) return;
    for (node = ops->units(ops->context, player); node; node = node->next) {
        AoeLiveUnit *u = node->object;
        if (!u || !u->core.unit_template) continue; /* port guard */
        if (u->core.state < 3 && u->core.unit_template->unit_class != 1)
            ops->kill_unit(ops->context, u);
    }
}

/* Codes 100 and 200..209: `target`'s units die unless it is `player` and
   players[player] +0x1c(target) and players[target] +0x1c(player) both
   hold. */
static void kill_unless_allied_self(const AoeCommandGameCheatOpsRor *ops,
                                    int16_t player, int16_t target) {
    if (target == player && valid_player(ops, player) &&
        ops->is_ally(ops->context, player, target) &&
        ops->is_ally(ops->context, target, player))
        return;
    kill_units(ops, target);
}

/* 0x0050c570 */
int aoe_command_game_cheat_ror(int16_t player, int16_t code,
                               const AoeCommandGameCheatOpsRor *ops) {
    if (!ops) return -1;
    if (!ops->cheats_on) return 0; /* 0x0041b7a0 */
    switch (code) {
    case 0:
        return player > 0 ? train_at(ops, player, AOE_CHEAT_TOWN_CENTER_ROR, 0x112) : 0;
    case 1:
        return player > 0 ? train_at(ops, player, AOE_CHEAT_TOWN_CENTER_ROR, 0xf8) : 0;
    case 0xe:
        return player > 0 ? train_at(ops, player, AOE_CHEAT_TOWN_CENTER_ROR, 0x182) : 0;
    case 0xf:
        return player > 0 ? train_at(ops, player, AOE_CHEAT_TEMPLE_ROR, 0x18a) : 0;
    case 0x10:
        return player > 0 ? train_at(ops, player, AOE_CHEAT_TOWN_CENTER_ROR, 0x18d) : 0;
    case 0x11:
        return player > 0 ? train_at(ops, player, AOE_CHEAT_TOWN_CENTER_ROR, 0x18e) : 0;
    case 0xdc:
        return train_at(ops, player, AOE_CHEAT_TOWN_CENTER_ROR, 0x174);
    case 2: case 3: case 4: case 6: case 7: case 8:
    case 9: case 10: case 11: case 12: case 13:
        if (!ops->apply_effect) return -1;
        if (player <= 0 || !valid_player(ops, player)) return 0;
        ops->apply_effect(ops->context, (int16_t)(0x7d + code), player);
        return 1;
    case 5:
    case 0x12:
    case 0x13:
        if (!ops->apply_effect) return -1;
        if (!valid_player(ops, 0)) return 0;
        ops->apply_effect(ops->context,
                          code == 5 ? 0x82 : code == 0x12 ? 0xd8 : 0xd9, 0);
        return 1;
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17: {
        /* Food, gold, stone, wood: 1000.0f (0x447a0000). */
        static const int16_t type[4] = {0, 3, 2, 1};
        if (!ops->resource_add) return -1;
        if (!valid_player(ops, player)) return 0;
        ops->resource_add(ops->context, player, type[code - 0x14], 1000.0f);
        return 1;
    }
    case 0x18:
        if (!ops->game_9a8) return -1;
        *ops->game_9a8 = *ops->game_9a8 == 0;
        return 1;
    case 100: {
        int16_t i;
        if (!ops->units || !ops->kill_unit || !ops->is_ally) return -1;
        if (ops->flag_7c0324) *ops->flag_7c0324 = 1;
        for (i = 1; i < ops->player_count; ++i)
            kill_unless_allied_self(ops, player, i);
        return 1;
    }
    case 0x65:
        if (!ops->units || !ops->kill_unit) return -1;
        kill_units(ops, player);
        return 1;
    case 0x66:
        if (!ops->win || !ops->world_1e) return -1;
        if (!valid_player(ops, player)) return 0;
        ops->win(ops->context, player); /* 0x45d460 */
        *ops->world_1e = 0x68;
        return 1;
    case 0x67:
        if (!ops->lose) return -1;
        if (!valid_player(ops, player)) return 0;
        ops->lose(ops->context, player); /* 0x45d440 */
        return 1;
    case 200: case 201: case 202: case 203: case 204:
    case 205: case 206: case 207: case 208: case 209:
        if (!ops->units || !ops->kill_unit || !ops->is_ally) return -1;
        kill_unless_allied_self(ops, player, (int16_t)(code - 200));
        return 1;
    case 0xe6:
    case 0xe7: {
        int which = code - 0xe6;
        if (!ops->map_flag || !ops->set_map_flag || !ops->redraw_views)
            return -1;
        if (!ops->setting_97d) return 0; /* 0x0041b750 */
        /* 0x520690 / 0x5206b0, then 0x41bc10 / 0x41bc30(sete). */
        ops->set_map_flag(ops->context, which,
                          (uint8_t)(ops->map_flag(ops->context, which) == 0));
        ops->redraw_views(ops->context);
        return 1;
    }
    default:
        return 0;
    }
}
