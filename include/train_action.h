#ifndef VITA_AOE_TRAIN_ACTION_H
#define VITA_AOE_TRAIN_ACTION_H
#include "dat.h"
#include "player_resources.h"
#include "action_list.h"
#include "live_unit_registry.h"
#include "aoe_rng.h"
#include <stdint.h>

/* Semantic subset needed by RoR training action +0x28. Not a complete
   original action object, constructor, or allocation factory. */
typedef struct AoeTrainAction {
    int16_t action_type; /* inherited action +0x04; training constructor=102 */
    uint8_t state; /* +0x0c; 6 trains, 1 returns complete */
    int16_t unit_id; /* +0x40 */
    float progress; /* +0x44 */
    int32_t request_id; /* +0x48 */
    uint8_t prepaid; /* +0x4c: nonzero skips resource charge on activation */
    const DatUnit *building_template;
    const DatUnit *trained_template;
    AoePlayerResources *owner_resources;
    int16_t owner_id;
    int32_t building_id;
    float building_x, building_y;
    void *primary_target;   /* inherited action +0x10 */
    void *secondary_target; /* inherited action +0x14 */
    int32_t primary_target_id;   /* inherited action +0x18, -1 when null */
    int32_t secondary_target_id; /* inherited action +0x1c, -1 when null */
    AoeActionList *owned_actions; /* inherited action +0x34 */
    const AoeLiveUnitRegistry *unit_registry; /* owner world +0x3c */
    float candidate_x, candidate_y; /* inherited action +0x20/+0x24 */
} AoeTrainAction;

typedef struct {
    int16_t unit_id;
    int32_t request_id;
    uint8_t prepaid;
    const DatUnit *building_template;
    const DatUnit *trained_template; /* current player slot; may be NULL */
    AoePlayerResources *owner_resources;
    int16_t owner_id;
    int32_t building_id;
    float building_x, building_y;
    AoeActionListOps owned_action_ops;
    const AoeLiveUnitRegistry *unit_registry;
} AoeTrainActionCreate;

typedef struct AoeTrainSpawnOps AoeTrainSpawnOps;

/* Recovered direction reductions used by the two executables' spawn loops.
   Exposed for exhaustive cross-edition verification; callers normally use
   the edition-specific spawn functions below. */
int16_t aoe_train_spawn_direction_ror(uint16_t random, int late_attempt);
int16_t aoe_train_spawn_direction_base(uint16_t random, int late_attempt);

typedef struct {
    void *context;
    AoeRng *rng;
    const AoeTrainSpawnOps *spawn; /* translated 004b6b40 dependencies */
    /* global +0x40(event,owner,unit_id,x,y) */
    void (*notify)(void *, uint32_t event, int16_t owner, int16_t unit_id,
                    int32_t x, int32_t y);
    /* owner +0x10c(0,unit_id,building_id,request_id) */
    void (*failed)(void *, AoeTrainAction *);
    /* owner +0x110(unit_id,building_id,request_id,created_unit_id) */
    void (*completed)(void *, AoeTrainAction *, int32_t created_unit_id);
    /* building +0x148(building_id,building_id,code,619,0,0) */
    void (*signal)(void *, AoeTrainAction *, uint32_t code);
} AoeTrainActionOps;

typedef struct {
    void *context;
    /* Owner 004f3ae0(template,1.0,0). It checks/deducts the template's
       three costs atomically according to the original resource rules. */
    int (*charge)(void *, AoeTrainAction *);
    /* Owner +0x10c(0,unit_id,building_id,request_id). */
    void (*failed)(void *, AoeTrainAction *);
    /* Building +0x148(building_id,building_id,505,619,0,0). */
    void (*signal_failure)(void *, AoeTrainAction *);
} AoeTrainActivationOps;

/* 004b68c0–004b6995, vtable +0x1c. `active == 0` is an exact no-op.
   A negative unit ID or absent trained-template slot only reports failure.
   A non-prepaid valid unit must pass the real resource charge; charge failure
   reports failure, signals 505, then enters state 1. Otherwise enter state 6.
   Returns 0 only for missing portable dependencies; prior callback effects
   are not rolled back. */
int aoe_train_action_activate_ror(AoeTrainAction *action, int active,
                                   const AoeTrainActivationOps *ops);
/* Base 004a51e0–004a52aa has no +0x4c prepaid byte: every valid active
   action charges resources before entering state 6. */
int aoe_train_action_activate_base(AoeTrainAction *action, int active,
                                   const AoeTrainActivationOps *ops);
/* Owner 004f3b90(unit_id): for type >=70, add each enabled nonnegative DAT
   cost amount back to its resource. The original tests only the low paid
   byte; this preserves that detail. Returns 0 for invalid portable bindings. */
int aoe_train_action_refund_ror(AoeTrainAction *action);

typedef struct {
    void *context;
    /* Global +0x40(103, owner, unit_id, 0, 0). */
    void (*notify_cancel)(void *, AoeTrainAction *);
    /* Building +0x148(building_id, building_id, 505, 619, 0, 0). */
    void (*signal_cancel)(void *, AoeTrainAction *);
} AoeTrainCancelOps;

/* Training vtable +0x2c, RoR 004b6d90–004b6dfc. If state is not 1, invoke
   inherited state setter +0x5c (a direct state-1 store in this vtable), then
   refund when prepaid byte is zero. Regardless of prior state/prepaid flag,
   emit event 103 and building signal 505/619. Return 0 only for absent
   semantic dependencies; no callback effects are rolled back. */
int aoe_train_action_cancel_ror(AoeTrainAction *action,
                                const AoeTrainCancelOps *ops);
/* Base-game counterpart 004a5690–004a56f3 has no prepaid flag and refunds
   whenever it transitions a non-state-1 action. */
int aoe_train_action_cancel_base(AoeTrainAction *action,
                                 const AoeTrainCancelOps *ops);

/* RoR 004b6e00–004b6e89 training-action helper, data-return portion.
   Always returns the queued unit ID. State 1 reports 100 directly; otherwise
   it returns trunc_toward_zero(progress * 100 / signed DAT train_time), then
   retains the original low 16 bits. The executable's optional text-buffer
   mutation through 00528540 is deliberately excluded until that callee is
   translated. */
int aoe_train_action_progress_ror(const AoeTrainAction *action,
                                  int16_t *unit_id, int16_t *percent);
/* Base-game counterpart 004a5700 follows the same returned-ID and truncating
   percent contract as the RoR helper. */
int aoe_train_action_progress_base(const AoeTrainAction *action,
                                   int16_t *unit_id, int16_t *percent);

typedef struct {
    void *context;
    /* Inherited vtable +0x14 (00407220), before derived fields. */
    int (*serialize_base)(void *, const AoeTrainAction *);
    /* Equivalent of each 0046cc40(stream, field, byte_count) call. */
    int (*write)(void *, const void *bytes, size_t count);
} AoeTrainActionSerializeOps;

/* Training vtable +0x14, RoR 004b6870–004b68bc. Serialize inherited state,
   then unit ID (+40, 2 bytes), progress (+44, 4), request ID (+48, 4), and
   prepaid flag (+4c, 1), in that order. */
int aoe_train_action_serialize_ror(const AoeTrainAction *action,
                                   const AoeTrainActionSerializeOps *ops);
/* Base vtable +0x14, 004a5190–004a51ce: inherited state, unit ID (2),
   progress (4), request ID (4). There is no prepaid-field write. */
int aoe_train_action_serialize_base(const AoeTrainAction *action,
                                    const AoeTrainActionSerializeOps *ops);

typedef struct AoeTrainTargetOps AoeTrainTargetOps;

typedef struct {
    void *context;
    uint8_t (*target_state)(void *, const void *target); /* target +0x48 */
    const AoeTrainTargetOps *targets; /* translated +0x54/+0x58 dependencies */
} AoeTrainWaitingOps;

/* Inherited action waiting method 004073d0–00407402, used as training
   vtable +0x24. Clear each nonnull target whose live state is >=7, primary
   before secondary, rereading the secondary after the first callback.
   Original return byte is always 1; portable result 0 reports a missing
   required dependency. */
int aoe_train_action_waiting_ror(AoeTrainAction *action,
                                  const AoeTrainWaitingOps *ops,
                                  uint8_t *result);

struct AoeTrainTargetOps {
    void *context;
    int32_t (*unit_id)(void *, const void *target); /* target +0x04 */
    /* Old target virtual +0xc4(building); new target +0xc0(building,type,0). */
    void (*detach)(void *, void *registered_target, AoeTrainAction *);
    void (*attach)(void *, void *new_target, AoeTrainAction *, int16_t type);
};

/* Inherited virtual +0x54/+0x58 at 00407460/004074d0. The old target is
   detached through its cached-ID registry lookup before the live pointer is
   reread. If a detach callback already changed it to `target`, no assignment
   or attach occurs. Otherwise write pointer, cache target ID (or -1), then
   register a nonnull target. */
int aoe_train_action_set_primary_ror(AoeTrainAction *action, void *target,
                                      const AoeTrainTargetOps *ops);
int aoe_train_action_set_secondary_ror(AoeTrainAction *action, void *target,
                                        const AoeTrainTargetOps *ops);

/* Semantic allocation form of 004b0020 -> 004b5ac0 -> 004b6820 and its
   inherited 00407110 initialization. It preserves recovered initial values
   and allocates the inherited +0x34 action list. Runtime object/vtable ABI is
   intentionally replaced by portable callbacks and semantic bindings. */
AoeTrainAction *aoe_train_action_create_ror(const AoeTrainActionCreate *create);
/* Base constructor 004a50c0–004a5125 initializes unit ID, zero progress, and
   request ID only. The portable record retains `prepaid` for RoR layout, but
   this entry point always clears it before construction. */
AoeTrainAction *aoe_train_action_create_base(const AoeTrainActionCreate *create);
/* Derived 004b6860 -> inherited 00406f80 teardown: detach primary, detach
   secondary, clear/free owned +0x34 list, then free the action. Returns 0
   without freeing when a required target dependency is unavailable. */
int aoe_train_action_destroy_ror(AoeTrainAction *action,
                                  const AoeTrainTargetOps *targets);

struct AoeTrainSpawnOps {
    void *context;
    /* Trained-template virtual +0x20(owner,x,y,0,1,1,1,0,0,1): nonzero
       means blocked, zero accepts this candidate. */
    int (*blocked)(void *, AoeTrainAction *, float x, float y);
    /* Trained-template virtual +0x18(owner,x,y,world): return the created
       live unit's semantic ID. */
    int (*create)(void *, AoeTrainAction *, float x, float y,
                  int32_t *created_unit_id);
};

/* 004b6b40–004b6d75. Probe at most 60 candidates in each of three expanding
   perimeter rings using the original RNG calls and integer direction
   reduction. `found` is original AL; API return 0 denotes invalid portable
   inputs/callback failure. Candidate fields retain the last probe. */
int aoe_train_action_spawn_ror(AoeTrainAction *action, AoeRng *rng,
                                const AoeTrainSpawnOps *ops,
                                int32_t *created_unit_id, uint8_t *found);
/* Base 004a5450 has the same three-ring semantic spawn interface. */
int aoe_train_action_spawn_base(AoeTrainAction *action, AoeRng *rng,
                                const AoeTrainSpawnOps *ops,
                                int32_t *created_unit_id, uint8_t *found);

/* 004b69a0–004b6b35, vtable 0054db00 +0x28. Delta is owner->world +0x84;
   fast_train is global +0x9a8. Fast mode assigns 1000, not infinity.
   Return 0 for unsupported inputs/dependencies; success writes original AL.
   Spawn and notification methods remain required real integration work.
   Callbacks preserve template/position bindings and must not destroy action. */
int aoe_train_action_update_ror(AoeTrainAction *action, float delta,
                                 int fast_train, const AoeTrainActionOps *ops,
                                 uint8_t *result);
/* Base 004a52c0–004a5443 has the same action-state/update semantics. */
int aoe_train_action_update_base(AoeTrainAction *action, float delta,
                                 int fast_train, const AoeTrainActionOps *ops,
                                 uint8_t *result);
#endif
