#ifndef VITA_AOE_PLAYER_UPDATE_H
#define VITA_AOE_PLAYER_UPDATE_H

#include "object_update_list.h"
#include "player_group.h"
#include "player_outcome.h"
#include "player_queue.h"
#include "visibility_watch.h"

#include <stdint.h>

typedef struct AoePlayerUpdateState {
    /* Player +0x10, +0x20, and +0x114 (RoR) / +0x110 (base). */
    uint32_t field_10;
    uint8_t flag_20;
    uint32_t map_update_flag;
    AoeObjectUpdateList object_list28; /* Player +0x28 primary list. */
    AoeObjectUpdateList object_list2c; /* Player +0x2c alternate list. */
    AoeObjectUpdateList object_list30; /* Player +0x30 follow-up list. */
    AoePlayerGroup group;
    AoeOutcomeManager *outcome_manager; /* Original player +0x34. */
    AoePlayerQueue queue58; /* Original player +0x58. */
    AoeVisibilityWatch visibility_watch40; /* Original player +0x40. */
} AoePlayerUpdateState;

typedef struct {
    AoeObjectUpdateOps object_ops;
    void *user;
    /* Global event interface, called with event code 6 on an unlocked
       transition to player state 2. */
    void (*emit_state_event)(void *user, int code, int16_t player_index);
    /* RoR 0x00519a60 / base 0x00429a60. */
    void (*map_update)(void *user, AoePlayerUpdateState *player);
} AoePlayerUpdateOps;

/* Player vtable +0x04: RoR 0x0045cb90, base 0x00516480. */
int aoe_player_state_set(AoeOutcomePlayerRef *player, uint8_t state,
                         const AoePlayerUpdateOps *ops);

/* RoR player vtable +0x7c at 0x0045df70; base counterpart 0x005178f0.
   Composes only translated original subroutines. Required callbacks have no
   substitute implementation, and this semantic struct is not the PC ABI. */
int aoe_player_update_step(AoePlayerUpdateState *player,
                           AoePlayerGroupEdition edition,
                           const AoePlayerUpdateOps *ops);

/* Player vtable +0xdc/+0xe0: RoR 0x0045f810/0x0045f860, base
   0x00519350/0x005193a0. A nonzero first selector chooses list +0x2c;
   otherwise a nonzero second selector chooses +0x30; else +0x28.
   Registering also inserts in the world ID table. Removal always attempts
   to clear the world slot after list unlink, as in the original. Any
   needs_reassign result requires the displaced unit's original virtual
   methods before simulation can continue. */
int aoe_player_unit_register(AoePlayerUpdateState *player,
                             AoeOwnedVisibilityRegistry *world,
                             AoeVisibilityUnitRef *unit, int32_t unit_id,
                             int select_2c, int select_30,
                             AoeObjectUpdateNode **attached,
                             AoeVisibilityUnitRef **replaced,
                             int *needs_reassign);
int aoe_player_unit_remove(AoePlayerUpdateState *player,
                           AoeOwnedVisibilityRegistry *world,
                           AoeVisibilityUnitRef *unit, int32_t unit_id,
                           int select_2c, int select_30,
                           AoeObjectUpdateNode *node,
                           int *list_removed);

#endif
