#ifndef VITA_AOE_LIVE_UNIT_STATE_H
#define VITA_AOE_LIVE_UNIT_STATE_H

#include "live_unit_core.h"

#include <stddef.h>

typedef struct {
    void *context;
    /* Root +0x5c's remaining side-effect boundaries, in call order.
       Callbacks must not modify this unit's state byte. */
    void (*mode_two_effect)(void *context, AoeLiveUnitCore *unit); /* +0x64 */
    void (*pre_terminal)(void *context, AoeLiveUnitCore *unit); /* +0x60 */
    void (*visibility_leave)(void *context, AoeLiveUnitCore *unit,
                             int alternate); /* +0xdc */
    void (*visibility_enter)(void *context, AoeLiveUnitCore *unit,
                             int alternate); /* +0xd8 */
    void (*state_three_actions)(void *context, AoeLiveUnitCore *unit);
    AoePlayerResources *owner_resources; /* player +0x78 calls */
    AoePlayerEdition edition;
    /* These semantic inputs represent original +0x04, +0x74, +0x78 and
       +0x68. The action-list body is still a required callback. */
    const void *default_action;
    const void *current_action;
    int has_action_list;
    size_t action_count;
    int has_map_tile; /* Original unit +0x1c != NULL. */
    void **registration_node; /* Original unit +0x50. */
    AoeLiveUnitRegistrationHooks registration;
} AoeLiveUnitRootStateOps;

/* Root RoR live-unit vtable +0x5c = 0x004abdf0. This composes translated
   field/order rules with explicit callbacks for unported original methods.
   It is not the derived type-70/type-80 setter, nor a complete unit update. */
int aoe_live_unit_root_state_set_ror(AoeLiveUnitCore *unit,
                                      uint8_t new_state,
                                      const AoeLiveUnitRootStateOps *ops);

/* Base root vtable +0x5c = 0x004167f0. Unlike RoR it accepts any changed
   state, runs pre-terminal effects before leaving active state, and has no
   template +0xa6 mode-two call in this setter. */
int aoe_live_unit_root_state_set_base(AoeLiveUnitCore *unit,
                                       uint8_t new_state,
                                       const AoeLiveUnitRootStateOps *ops);

/* RoR scenario-load unit virtual +0xb4. Root types 10/20/25/90 resolve to
   0x00407370, an actual no-op. Types 30/40/50/60/70/80 resolve to 0x0044ca40:
   store rotation in their derived field then run `0x0044c7b0`, which sets
   the low-byte animation frame only for a sequence-type bit-1 graphic with
   a positive frame count. Returns zero for an unsupported type or an
   unrepresentable rotation; it never substitutes a state-byte assignment. */
int aoe_live_unit_low_state_ror(AoeLiveUnitCore *unit,
                                const DatGraphic *current_graphic,
                                float rotation);

typedef struct {
    void *context;
    /* Root high-state optional parent virtual +0xd4. */
    void (*parent_high_state)(void *context, AoeLiveUnitCore *unit);
    /* 0x0045ec70, required only when unit +0x36 bit 0 is set. */
    void (*clear_active_action)(void *context, AoeLiveUnitCore *unit);
    AoeLiveUnitRootStateOps state;
} AoeLiveUnitRootHighStateOps;

/* RoR root unit high-state virtual +0x68 = 0x004abb80. This is used
   directly by types 10/20/25/90; types 40/50/60 prepend their own action
   cleanup and types 70/80 override it. The owner reclassification occurs
   before the root +0x5c state setter, exactly as in the executable. */
int aoe_live_unit_root_high_state_ror(
    AoeLiveUnitCore *unit, const AoeLiveUnitRootHighStateOps *ops);

#endif
