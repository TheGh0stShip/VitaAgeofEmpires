#ifndef VITA_AOE_PLAYER_GROUP_H
#define VITA_AOE_PLAYER_GROUP_H

#include <stdint.h>

#define AOE_PLAYER_GROUP_MAX_SLOTS 40

typedef enum {
    AOE_GROUP_BASE = 40,
    AOE_GROUP_RISE_OF_ROME = 25
} AoePlayerGroupEdition;

/* Semantic fields corresponding to original object offsets +0x48, +0x20,
   and +0x36. This is a portable model, not the original packed object ABI. */
typedef struct {
    uint8_t code48;
    uint32_t field20;
    uint8_t flag36;
} AoePlayerGroupMember;

/* Original group: leader at player +0x130 (base) / +0x134 (RoR), followed
   by 40 / 25 member pointers and a signed 16-bit count. No members owned. */
typedef struct {
    AoePlayerGroupMember *leader;
    AoePlayerGroupMember *slots[AOE_PLAYER_GROUP_MAX_SLOTS];
    int16_t count;
} AoePlayerGroup;

/* Expansion 0x0045ecc0 / base 0x005186f0. Returns zero for malformed
   portable state; the original does not check those memory-safety bounds. */
int aoe_player_group_remove(AoePlayerGroup *group,
                            AoePlayerGroupEdition edition, int16_t slot);

/* Expansion 0x0045f170 / base 0x00518bf0: one per-frame prune sweep.
   Returns zero for malformed portable state. */
int aoe_player_group_prune(AoePlayerGroup *group,
                           AoePlayerGroupEdition edition);

#endif
