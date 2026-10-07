#include "command_game_apply.h"

#include <assert.h>
#include <string.h>

/* Patched RoR 1.0a 0x0050c570 (world cheat), evidence from
   build/patch_1_0c/EMPIRESX.EXE:
   - 0x50c57c: 0x41b7a0 (game byte +0x982) zero returns at once;
   - byte table 0x50cd00: codes 0..0x18 -> cases 0..0x18, 0x64..0x67 ->
     0x19..0x1c, 0xc8..0xd1 -> 0x1d, 0xdc -> 0x1e, 0xe6 / 0xe7 -> 0x1f /
     0x20, all else 0x21 (0x50cc6f, return);
   - 0x50c5a8: player > 0, players[p] +0x28 0x451370(0x6d, -1.0f, -1.0f,
     1, 2, 0), world +0x58 0x4e9490(found, 0x112);
   - 0x50c644: world +0x4c vtable +8(0x7f, players[p]); 0x50c6c5: (0x82,
     players[0]);
   - 0x50c9ab / 0x50c9eb / 0x50ca0b / 0x50c9cb (codes 0x14..0x17): player
     +0x78(0 / 3 / 2 / 1, 0x447a0000 = 1000.0f, 0);
   - 0x50ca2b: game +0x9a8 = sete(+0x9a8 == 0);
   - 0x50ca4a: byte 0x7c0324 = 1, i from 1 below world +0x3c; 0x50ca65:
     i != p kills, else players[p] +0x1c(i) and players[i] +0x1c(p);
     0x50ca9b: unit byte +0x48 < 3 and template +0x14 != 1 -> +0x6c;
   - 0x50cb05: player 0x45d460, world byte +0x1e = 0x68;
   - 0x50cbe5: 0x41b750 (game +0x97d), 0x520690 sete -> 0x41bc10, then
     game +0x2c() +0x20(2) and +0x30() +0x20(2).
   DAT (EMPIRES.DAT, civ 1): 109 Town_Center1, 104 Temple1, 274
   Soldier_Laser, 248 Soldier_Nuker, 394 Saint_Francis. */

static DatUnit soldier_t, artifact_t;
static AoeLiveUnit units[6];
static AoeObjectUpdateNode nodes[6];
static AoeObjectUpdateNode *heads[3];
static AoeLiveUnit town_center, temple;

static uint8_t record[32];
static size_t record_size;
static int queued, effects, adds, wins, losses, redraws;
static int16_t last_find, effect_id, effect_player, add_player, add_type;
static float add_amount;
static int16_t killed[8];
static int kill_count;
static int self_ally = 1;
static uint8_t map_bytes[2];
static int last_set_which = -1;

static AoeObjectUpdateNode *units_of(void *c, int16_t p) {
    (void)c;
    assert(p >= 0 && p < 3);
    return heads[p];
}
static AoeLiveUnit *find_finished(void *c, int16_t p, int16_t t) {
    (void)c;
    last_find = t;
    if (p == 0 || p == 1)
        return t == AOE_CHEAT_TOWN_CENTER_ROR ? &town_center
               : t == AOE_CHEAT_TEMPLE_ROR    ? &temple
                                              : NULL;
    return NULL;
}
static int16_t owner_id(void *c, const AoeLiveUnit *u) {
    (void)c;
    return u->runtime_owner_index;
}
static void queue(void *c, const uint8_t *r, size_t n) {
    (void)c;
    assert(n <= sizeof record);
    memcpy(record, r, n);
    record_size = n;
    ++queued;
}
static void apply_effect(void *c, int16_t e, int16_t p) {
    (void)c;
    effect_id = e;
    effect_player = p;
    ++effects;
}
static void resource_add(void *c, int16_t p, int16_t t, float a) {
    (void)c;
    add_player = p;
    add_type = t;
    add_amount = a;
    ++adds;
}
/* 0x45bd90: the same player is always an ally; here the self test is
   switchable to reach the other branch. */
static int is_ally(void *c, int16_t p, int16_t o) {
    (void)c;
    return p == o ? self_ally : 0;
}
static void kill_unit(void *c, AoeLiveUnit *u) {
    (void)c;
    assert(kill_count < 8);
    killed[kill_count++] = (int16_t)u->core.unit_id;
    u->core.state = 3; /* dying; the node stays linked */
}
static void win(void *c, int16_t p) { (void)c; assert(p == 1); ++wins; }
static void lose(void *c, int16_t p) { (void)c; assert(p == 2); ++losses; }
static uint8_t map_flag(void *c, int w) { (void)c; return map_bytes[w]; }
static void set_map_flag(void *c, int w, uint8_t v) {
    (void)c;
    map_bytes[w] = v;
    last_set_which = w;
}
static void redraw_views(void *c) { (void)c; ++redraws; }

static void reset_units(void) {
    /* Player 1: units[0] (soldier). Player 2: units[1] soldier, units[2]
       artifact (class 1), units[3] in state 3, units[4] in state 0. */
    static const int owner[5] = {1, 2, 2, 2, 2};
    static const uint8_t state[5] = {2, 2, 2, 3, 0};
    int i;
    memset(units, 0, sizeof units);
    memset(nodes, 0, sizeof nodes);
    heads[0] = heads[1] = heads[2] = NULL;
    for (i = 4; i >= 0; --i) {
        units[i].core.unit_id = 10 + i;
        units[i].core.state = state[i];
        units[i].core.unit_template = i == 2 ? &artifact_t : &soldier_t;
        units[i].runtime_owner_index = (int16_t)owner[i];
        nodes[i].object = &units[i];
        nodes[i].next = heads[owner[i]];
        heads[owner[i]] = &nodes[i];
    }
    kill_count = 0;
}

static int32_t le32(const uint8_t *b) {
    return (int32_t)((uint32_t)b[0] | (uint32_t)b[1] << 8 |
                     (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24);
}

int main(void) {
    int32_t game_9a8 = 0;
    uint8_t world_1e = 0, flag_7c0324 = 0;
    AoeCommandGameCheatOpsRor ops;
    int c;

    soldier_t.unit_class = 6;
    artifact_t.unit_class = 1;
    town_center.core.unit_id = 77;
    town_center.runtime_owner_index = 1;
    temple.core.unit_id = 78;
    temple.runtime_owner_index = 1;
    reset_units();

    memset(&ops, 0, sizeof ops);
    ops.cheats_on = 0;
    ops.setting_97d = 0;
    ops.player_count = 3;
    ops.game_9a8 = &game_9a8;
    ops.world_1e = &world_1e;
    ops.flag_7c0324 = &flag_7c0324;
    ops.units = units_of;
    ops.find_finished = find_finished;
    ops.owner_id = owner_id;
    ops.queue = queue;
    ops.apply_effect = apply_effect;
    ops.resource_add = resource_add;
    ops.is_ally = is_ally;
    ops.kill_unit = kill_unit;
    ops.win = win;
    ops.lose = lose;
    ops.map_flag = map_flag;
    ops.set_map_flag = set_map_flag;
    ops.redraw_views = redraw_views;

    assert(aoe_command_game_cheat_ror(1, 0, NULL) == -1);
    /* Cheats off (0x41b7a0 == 0): nothing, not even 0x18. */
    assert(aoe_command_game_cheat_ror(1, 0x18, &ops) == 0);
    assert(game_9a8 == 0 && queued == 0);
    ops.cheats_on = 1;

    /* Code 0: the town center (0x6d) trains 0x112 (Soldier_Laser) as
       record 0x64: building ID +4, owner +8, unit +0xa, request -1. */
    assert(aoe_command_game_cheat_ror(1, 0, &ops) == 1);
    assert(queued == 1 && record_size == 16 && last_find == 0x6d);
    assert(record[0] == 0x64 && le32(record + 4) == 77 && record[8] == 1);
    assert(record[10] == 0x12 && record[11] == 0x01);
    assert(le32(record + 12) == -1);
    /* Player 0 (and below) is refused for code 0; 0xdc has no test. */
    assert(aoe_command_game_cheat_ror(0, 0, &ops) == 0 && queued == 1);
    assert(aoe_command_game_cheat_ror(0, 0xdc, &ops) == 1 && queued == 2);
    assert(record[10] == 0x74 && record[11] == 0x01);
    /* Codes 1 / 0xe / 0x10 / 0x11 and the temple (0x68) for 0xf. */
    assert(aoe_command_game_cheat_ror(1, 1, &ops) == 1);
    assert(record[10] == 0xf8 && record[11] == 0);
    assert(aoe_command_game_cheat_ror(1, 0xe, &ops) == 1 && record[10] == 0x82);
    assert(aoe_command_game_cheat_ror(1, 0x10, &ops) == 1 && record[10] == 0x8d);
    assert(aoe_command_game_cheat_ror(1, 0x11, &ops) == 1 && record[10] == 0x8e);
    assert(aoe_command_game_cheat_ror(1, 0xf, &ops) == 1);
    assert(last_find == 0x68 && le32(record + 4) == 78);
    assert(record[10] == 0x8a && record[11] == 0x01);
    /* No finished building: nothing queued. */
    assert(aoe_command_game_cheat_ror(2, 0, &ops) == 0 && queued == 7);

    /* Effects: 2..4, 6..0xd -> 0x7d + code on the player (> 0). */
    assert(aoe_command_game_cheat_ror(2, 2, &ops) == 1);
    assert(effect_id == 0x7f && effect_player == 2);
    assert(aoe_command_game_cheat_ror(1, 0xd, &ops) == 1);
    assert(effect_id == 0x8a && effect_player == 1);
    assert(aoe_command_game_cheat_ror(0, 3, &ops) == 0 && effects == 2);
    /* 5, 0x12, 0x13 go to player 0 whoever asks. */
    assert(aoe_command_game_cheat_ror(2, 5, &ops) == 1);
    assert(effect_id == 0x82 && effect_player == 0);
    assert(aoe_command_game_cheat_ror(1, 0x12, &ops) == 1 && effect_id == 0xd8);
    assert(aoe_command_game_cheat_ror(1, 0x13, &ops) == 1 && effect_id == 0xd9);
    assert(effect_player == 0 && effects == 5);

    /* Resources: food, gold, stone, wood, 1000 each. */
    {
        static const int16_t want[4] = {0, 3, 2, 1};
        for (c = 0x14; c <= 0x17; ++c) {
            assert(aoe_command_game_cheat_ror(2, (int16_t)c, &ops) == 1);
            assert(add_player == 2 && add_type == want[c - 0x14]);
            assert(add_amount == 1000.0f);
        }
        assert(adds == 4);
    }

    /* 0x18: game +0x9a8 = (+0x9a8 == 0). */
    assert(aoe_command_game_cheat_ror(1, 0x18, &ops) == 1 && game_9a8 == 1);
    assert(aoe_command_game_cheat_ror(1, 0x18, &ops) == 1 && game_9a8 == 0);
    game_9a8 = 5;
    assert(aoe_command_game_cheat_ror(1, 0x18, &ops) == 1 && game_9a8 == 0);

    /* 100: player 2's soldier and state-0 unit die, not its artifact or
       its state-3 unit, nor player 1's own (self ally). */
    assert(aoe_command_game_cheat_ror(1, 100, &ops) == 1);
    assert(flag_7c0324 == 1);
    assert(kill_count == 2 && killed[0] == 11 && killed[1] == 14);
    /* A failing self test kills the player's own units as well. */
    reset_units();
    self_ally = 0;
    assert(aoe_command_game_cheat_ror(1, 100, &ops) == 1);
    assert(kill_count == 3 && killed[0] == 10 && killed[1] == 11 &&
           killed[2] == 14);
    self_ally = 1;

    /* 0x65: the player's own units. */
    reset_units();
    assert(aoe_command_game_cheat_ror(2, 0x65, &ops) == 1);
    assert(kill_count == 2 && killed[0] == 11 && killed[1] == 14);

    /* 200 + n: player n's units, unless n is the asking player. */
    reset_units();
    assert(aoe_command_game_cheat_ror(1, 202, &ops) == 1 && kill_count == 2);
    reset_units();
    assert(aoe_command_game_cheat_ror(1, 201, &ops) == 1 && kill_count == 0);
    assert(aoe_command_game_cheat_ror(2, 201, &ops) == 1 && kill_count == 1);
    assert(killed[0] == 10);
    /* Player 9 is beyond the count: the port skips it. */
    assert(aoe_command_game_cheat_ror(1, 209, &ops) == 1 && kill_count == 1);

    /* 0x66 wins and sets the game-over byte to 0x68; 0x67 loses. */
    assert(aoe_command_game_cheat_ror(1, 0x66, &ops) == 1);
    assert(wins == 1 && world_1e == 0x68);
    assert(aoe_command_game_cheat_ror(2, 0x67, &ops) == 1 && losses == 1);

    /* 0xe6 / 0xe7 need game +0x97d (0x41b750). */
    assert(aoe_command_game_cheat_ror(1, 0xe6, &ops) == 0 && redraws == 0);
    ops.setting_97d = 1;
    assert(aoe_command_game_cheat_ror(1, 0xe6, &ops) == 1);
    assert(map_bytes[0] == 1 && last_set_which == 0 && redraws == 1);
    assert(aoe_command_game_cheat_ror(1, 0xe6, &ops) == 1 && map_bytes[0] == 0);
    map_bytes[1] = 7;
    assert(aoe_command_game_cheat_ror(1, 0xe7, &ops) == 1);
    assert(map_bytes[1] == 0 && last_set_which == 1 && redraws == 3);

    /* Codes outside the table do nothing. */
    {
        static const int16_t none[] = {0x19, 0x63, 0x68, 0xc7, 0xd2,
                                       0xdb, 0xdd, 0xe5, 0xe8, 0xff, -1};
        size_t i;
        int before = queued + effects + adds + wins + losses + redraws;
        for (i = 0; i < sizeof none / sizeof none[0]; ++i)
            assert(aoe_command_game_cheat_ror(1, none[i], &ops) == 0);
        assert(before == queued + effects + adds + wins + losses + redraws);
    }

    /* A missing callback for the code: -1. */
    ops.apply_effect = NULL;
    assert(aoe_command_game_cheat_ror(1, 2, &ops) == -1);
    ops.queue = NULL;
    assert(aoe_command_game_cheat_ror(1, 0, &ops) == -1);
    return 0;
}
