#ifndef VITA_AOE_COMMAND_GAME_APPLY_H
#define VITA_AOE_COMMAND_GAME_APPLY_H

#include <stddef.h>
#include <stdint.h>

#include "live_unit_factory.h" /* AoeLiveUnit, AoeObjectUpdateNode */

/* Patched Rise of Rome 1.0a world cheat 0x0050c570 (__thiscall on the
   world, game +0x3f4; ret 8: int16 player, int16 code). Tribe command
   0x67 sub-code 6 (0x004e8eb0) calls it with the zero-extended byte +0x04
   as the code; the chat parser 0x503cb0 picks the codes (game_ror.h).
   Nothing happens unless the game's cheat setting (byte +0x982,
   0x0041b7a0) is set. The switch (byte table 0x50cd00, jump table
   0x50cc78) covers codes 0..0x18, 0x64..0x67, 0xc8..0xd1, 0xdc, 0xe6 and
   0xe7; every other code does nothing.

   World fields: +0x1e the game-over byte, +0x3c the int16 player count,
   +0x40 the player table, +0x4c the effects object, +0x58 the command
   queue. Player fields: +0x28 the unit list (first node at list +0x04;
   node +0x00 unit, +0x04 next), vtable +0x1c (0x45bd90, ally test) and
   +0x78 (resource add). Unit fields: +0x08 template (its int16 class
   +0x14), +0x48 the state byte, vtable +0x6c. Game fields (0x00580e38):
   +0x97d (0x0041b750), +0x982, +0x9a8, +0x3f4 the world, vtable +0x2c /
   +0x30 the two views.

   Semantic ops, not the PC object ABI: players are world +0x40 indices
   and every original virtual or untranslated callee is a callback. */
typedef struct {
    void *context;
    int cheats_on;         /* game byte +0x982 (0x0041b7a0) */
    int setting_97d;       /* game byte +0x97d (0x0041b750) */
    int16_t player_count;  /* world +0x3c */
    int32_t *game_9a8;     /* game +0x9a8, toggled by code 0x18 */
    uint8_t *world_1e;     /* world +0x1e, the game-over byte */
    /* Global byte 0x007c0324, set by code 100 (the game screen's
       constructor 0x0047d440 clears it); may be NULL. */
    uint8_t *flag_7c0324;
    /* Player +0x28 list's first node (list +0x04); NULL when empty. */
    AoeObjectUpdateNode *(*units)(void *context, int16_t player);
    /* Player +0x28 list 0x451370(template, -1.0f, -1.0f, 1, 2, 0): the
       player's first unit of the template in state 2; NULL for none. */
    AoeLiveUnit *(*find_finished)(void *context, int16_t player,
                                  int16_t template_id);
    /* Unit +0x0c owner's player ID (+0x4a), the train command's byte. */
    int16_t (*owner_id)(void *context, const AoeLiveUnit *unit);
    /* World +0x58 command queue 0x42b6e0(record, size). */
    void (*queue)(void *context, const uint8_t *record, size_t size);
    /* World +0x4c effects vtable +0x08(effect, world +0x40[player])
       (0x004ebb10, tech_tree.h). */
    void (*apply_effect)(void *context, int16_t effect, int16_t player);
    /* Player vtable +0x78(type, amount, 0). */
    void (*resource_add)(void *context, int16_t player, int16_t type,
                         float amount);
    /* Player vtable +0x1c(other) on `player` (0x45bd90: the same player
       or stance 0). */
    int (*is_ally)(void *context, int16_t player, int16_t other);
    /* Unit vtable +0x6c (root 0x004abbc0). The original reads the list
       node's next link after this call, so it must not free the node. */
    void (*kill_unit)(void *context, AoeLiveUnit *unit);
    void (*win)(void *context, int16_t player);  /* player 0x45d460 */
    void (*lose)(void *context, int16_t player); /* player 0x45d440 */
    /* Which 0: the map's +0x8db8 (world +0x28; 0x520690 gives 1 without a
       map); which 1: +0x8db9 (0x5206b0 gives 0 without a map). The set is
       0x41bc10 / 0x41bc30 (-> 0x51b740 / 0x51b760 -> 0x444560 /
       0x444570): nothing without a world or a map. */
    uint8_t (*map_flag)(void *context, int which);
    void (*set_map_flag)(void *context, int which, uint8_t value);
    /* Game vtable +0x2c() object's +0x20(2), then game vtable +0x30()
       object's +0x20(2); both results are used unchecked. */
    void (*redraw_views)(void *context);
} AoeCommandGameCheatOpsRor;

enum {
    AOE_CHEAT_TOWN_CENTER_ROR = 0x6d, /* Town_Center1 (DAT unit 109) */
    AOE_CHEAT_TEMPLE_ROR = 0x68       /* Temple1 (DAT unit 104) */
};

/* 0x0050c570. Codes (game_ror.h names the chat phrases):
   0 / 1 / 0xe / 0x10 / 0x11 (player > 0) and 0xdc (no player test): the
     player's finished town center trains 0x112 / 0xf8 / 0x182 / 0x18d /
     0x18e / 0x174 through train command 0x64 (0x004e9490, request -1);
   0xf (player > 0): the finished temple trains 0x18a;
   2..4, 6..0xd (player > 0): effect 0x7d + code on the player;
   5, 0x12, 0x13: effect 0x82, 0xd8, 0xd9 on player 0;
   0x14 / 0x15 / 0x16 / 0x17: player +0x78(0 / 3 / 2 / 1, 1000.0f, 0);
   0x18: game +0x9a8 = (game +0x9a8 == 0);
   100: 0x007c0324 = 1, then for i = 1..count-1 every unit of i dies
     unless i is the player and both ally tests pass;
   0x65: the player's units die;
   0x66: the player wins (0x45d460), world +0x1e = 0x68;
   0x67: the player loses (0x45d440);
   200..209: the units of player code - 200 die unless it is the player
     and both ally tests pass;
   0xe6 / 0xe7 (game +0x97d set): map +0x8db8 / +0x8db9 inverted, then
     the views redrawn.
   A dying unit is one in state below 3 whose template class is not 1
   (artifact). Port guards: a player index outside 0..count-1, a NULL
   list object or a unit without a template is skipped (the original
   reads them unchecked). Returns -1 without ops or a callback the code
   needs, 0 when the original does nothing, 1 when the code ran. */
int aoe_command_game_cheat_ror(int16_t player, int16_t code,
                               const AoeCommandGameCheatOpsRor *ops);

#endif
