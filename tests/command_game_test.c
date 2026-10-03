#include "command_dispatch.h"

#include <assert.h>
#include <string.h>

static uint8_t stances[9];
static int32_t relations[9];
static AoePlayerDiplomacyRor player2 = {2, stances, relations, 9, 0, 0};
static int notified, refreshes, events, cheats, syncs, world_notes;
static int32_t event_args[3];
static float values[60];
static AoePlayerResources stock = {60, values};
static AoePlayerResourceTargetRor resources = {&stock, 0, 0, 0, 0};

static AoePlayerDiplomacyRor *diplomacy(void *c, int16_t p) {
    (void)c;
    return p == 2 ? &player2 : NULL;
}
static void notify(void *c, const AoePlayerDiplomacyRor *p, int32_t o,
                   uint8_t s) {
    (void)c;
    assert(p == &player2 && o == 3 && s == p->stances[3]);
    ++notified;
}
static const AoePlayerResourceTargetRor *res(void *c, int16_t p) {
    (void)c;
    return p == 2 ? &resources : NULL;
}
static void refresh(void *c) { (void)c; ++refreshes; }
static void game_event(void *c, uint32_t e, int32_t a, int32_t b, int32_t d) {
    (void)c;
    assert(e == 8);
    event_args[0] = a; event_args[1] = b; event_args[2] = d;
    ++events;
}
static void cheat(void *c, int16_t p, uint8_t code) {
    (void)c;
    assert(p == 2 && code == 9);
    ++cheats;
}
static void world_note(void *c, int16_t p) { (void)c; assert(p == 2); ++world_notes; }
static void sync_error(void *c) { (void)c; ++syncs; }

int main(void) {
    int32_t world_18 = 0, game_9a8 = 0;
    AoeCommandGameOps ops = {0, diplomacy, {0, notify}, res, refresh,
                             game_event, &world_18, &game_9a8, cheat,
                             world_note, sync_error};
    AoeCommandTribeDispatchRor tribe;
    AoeRng rng;
    uint8_t r[16];
    uint8_t stance;
    memset(&tribe, 0, sizeof(tribe));
    aoe_rng_seed(&rng, 1);
    tribe.rng = &rng;
    aoe_command_bind_game_ror(&tribe, &ops);

    /* Diplomacy: stance from __ftol(3.9f) = 3 -> relation 4 (other). */
    assert(aoe_command_encode_game_ror(r, sizeof(r), 0, 2, 3, 0x40799999,
                                       7) == 16);
    assert(aoe_command_dispatch_tribe_ror(&tribe, r, 16) ==
           AOE_COMMAND_ROUTE_HANDLED);
    assert(player2.changed == 1 && stances[3] == 3 && relations[3] == 4 &&
           notified == 1 && refreshes == 1 && events == 1 &&
           event_args[0] == 2 && event_args[1] == 3 && event_args[2] == 7);
    assert(aoe_player_stance_ror(&player2, 3, &stance) && stance == 3);
    /* Relation codes: stance 0 -> 2, stance 1 -> 3; self -> 1; gaia -> 0. */
    {
        AoePlayerDiplomacyOps quiet = {0, notify};
        assert(aoe_player_set_stance_ror(&player2, 3, 0, &quiet) &&
               relations[3] == 2);
        assert(aoe_player_set_stance_ror(&player2, 3, 1, &quiet) &&
               relations[3] == 3);
        assert(!aoe_player_set_stance_ror(&player2, 9, 1, &quiet));
    }
    stances[2] = 0;
    stances[0] = 5;
    relations[0] = 7;
    /* Codes 1, 4, 5, 6, 7, 8 and an unknown code. */
    assert(aoe_command_encode_game_ror(r, sizeof(r), 1, 2, 0, 0xdeadbeef, 0) &&
           aoe_command_execute_game_ror(r, 16, &ops) == 1 &&
           world_18 == (int32_t)0xdeadbeef);
    assert(aoe_command_encode_game_ror(r, sizeof(r), 4, -3, 0, 0, 0) &&
           aoe_command_execute_game_ror(r, 16, &ops) == 1 && game_9a8 == -3);
    assert(aoe_command_encode_game_ror(r, sizeof(r), 5, 2, 0x1ab, 0, 0) &&
           aoe_command_execute_game_ror(r, 16, &ops) == 1 &&
           player2.flag_108 == 0xab);
    assert(aoe_command_encode_game_ror(r, sizeof(r), 6, 2, 9, 0, 0) &&
           aoe_command_execute_game_ror(r, 16, &ops) == 1 && cheats == 1);
    assert(aoe_command_encode_game_ror(r, sizeof(r), 7, 2, 0, 0, 0) &&
           aoe_command_execute_game_ror(r, 16, &ops) == 1 && world_notes == 1);
    assert(aoe_command_encode_game_ror(r, sizeof(r), 8, 2, 0, 0, 0) &&
           aoe_command_execute_game_ror(r, 16, &ops) == 1 && syncs == 1);
    assert(aoe_command_encode_game_ror(r, sizeof(r), 3, 2, 0, 0, 0) &&
           aoe_command_execute_game_ror(r, 16, &ops) == 1);
    /* Code 2: resource add through player +0x78. */
    assert(aoe_command_encode_game_ror(r, sizeof(r), 2, 2, 4, 0x41200000, 0) &&
           aoe_command_execute_game_ror(r, 16, &ops) == 1 && values[4] == 10.0f);
    /* Unknown player. */
    assert(aoe_command_encode_game_ror(r, sizeof(r), 0, 5, 3, 0, 0) &&
           aoe_command_execute_game_ror(r, 16, &ops) == -1);
    return 0;
}
