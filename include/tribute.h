#ifndef VITA_AOE_TRIBUTE_H
#define VITA_AOE_TRIBUTE_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a "Diplomacy Dialog" (vtable 0x00543814,
   0x00431470..0x00433230): the tribute and stance logic behind its
   buttons. Resources are 0 food, 1 wood, 2 stone, 3 gold; player slots
   1..8. The dialog keeps what is already promised per resource (+0x610
   food, +0x614 wood, +0x618 gold, +0x61c stone) and per player
   (+0x650, +0x690, +0x670, +0x630 arrays). */
typedef struct {
    float spent[4];
    float given[4][9];
} AoeTributeRor;

/* 0x00432ff0 (gold), 0x00433080 (wood), 0x00433110 (food), 0x004331a0
   (stone): nothing more can be given in game state 5 or when the stock
   less what is promised is at most 0 (each player's button is then
   disabled; the viewed player's row always is). */
int aoe_tribute_exhausted_ror(const AoeTributeRor *t, int resource, float stock, int state5);

/* A click on a player's resource button (+0xb8 0x004323b0): unless
   exhausted, 100 more for that player at a cost of 100 x (1 + fee) (the
   fee is the giver's attribute 46 when positive); what does not cover a
   whole 100 gives the part the remaining stock pays for. 1 when given. */
int aoe_tribute_click_ror(AoeTributeRor *t, int resource, int player, float stock, float fee);

/* The clear button: nothing promised. */
void aoe_tribute_clear_ror(AoeTributeRor *t);

/* OK: each other player with a promise above 0 gets it through the
   giver's +0xf8(player, resource, amount, fee), gold, wood, food then
   stone. */
void aoe_tribute_send_ror(const AoeTributeRor *t, int32_t players, int32_t self, float fee,
                          void (*send)(void *context, int32_t player, int resource, float amount,
                                       float fee),
                          void *context);

/* The stance of a player's three radios: ally 0, neutral 1, else enemy 3. */
uint8_t aoe_diplomacy_stance_ror(int ally_on, int neutral_on);

#endif
