#ifndef VITA_AOE_PLAYER_DIPLOMACY_H
#define VITA_AOE_PLAYER_DIPLOMACY_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a player diplomacy (`this` = player). Stance
   bytes live at +0x84[other] and relation codes at +0x8c[other] (dwords).
   This is semantic caller-owned storage, not the x86 object layout. */
typedef struct {
    int16_t player_id;     /* +0x4a */
    uint8_t *stances;      /* +0x84 */
    int32_t *relations;    /* +0x8c */
    uint32_t count;        /* host bound for both arrays */
    uint8_t changed;       /* +0x88, set by diplomacy commands */
    uint8_t flag_108;      /* +0x108 (0x0045da40 / 0x0045da50) */
} AoePlayerDiplomacyRor;

typedef struct {
    void *context;
    /* Each object of the player's +0x28 list receives vtable +0x94(other,
       stance), in list order. */
    void (*notify_objects)(void *context, const AoePlayerDiplomacyRor *player,
                           int32_t other, uint8_t stance);
} AoePlayerDiplomacyOps;

/* 0x0045da60: the stance byte toward `other` (nonzero is treated as
   hostile by the task predicate 0x004e7f90 and the tribe selector).
   Returns 0 and leaves *stance untouched for an out-of-range host index. */
int aoe_player_stance_ror(const AoePlayerDiplomacyRor *player, int32_t other,
                          uint8_t *stance);

/* 0x0045da70: store the stance, then the relation code: 0 for player 0
   (gaia), 1 for the player itself, 2 for stance 0, 3 for stance 1 and 4
   otherwise; then notify the player's objects. Returns 0 for missing
   dependencies or an out-of-range host index. */
int aoe_player_set_stance_ror(AoePlayerDiplomacyRor *player, int32_t other,
                              uint8_t stance,
                              const AoePlayerDiplomacyOps *ops);

#endif
