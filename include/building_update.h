#ifndef VITA_AOE_BUILDING_UPDATE_H
#define VITA_AOE_BUILDING_UPDATE_H
#include <stdint.h>
#include "action_list.h"
#include "dat.h"
#include "player_resources.h"
#include <stddef.h>

typedef struct {
    int16_t unit_id;
    int16_t quantity;
} AoeBuildingQueueEntry;

/* Semantic storage, not the original packed x86 object layout. */
typedef struct AoeBuildingUpdate {
    const DatUnit *unit_template; /* +0x08; semantic pointer, not x86 layout */
    uint8_t state;              /* +0x48 */
    uint8_t angle;              /* +0x35 */
    int16_t graphic_angles;     /* current graphic +0x60 */
    int16_t build_time;         /* template +0x15a */
    float elapsed_build_time;  /* +0x1d4; this update does not increment it */
    uint32_t current_1b0;
    uint32_t history_e4, history_e8, history_ec;
    uint32_t history_f0, history_f4, history_f8;
    int16_t queue_count;       /* +0x1c6 */
    int16_t queue_aux_count;   /* +0x1c4 */
    uint8_t queue_revision;    /* +0x1d0 */
    uint8_t queue_aux_state;   /* +0x1d1 */
    int16_t queue_status;      /* +0x1d2; 4 has a global per-player counter */
    AoeActionList *live_actions;  /* +0x184, distinct general action list */
    AoeActionList *queue_actions; /* +0x1cc */
    uint8_t queue_enabled;        /* +0x1c8 */
    AoeBuildingQueueEntry *queue_entries; /* +0x1c0 */
    size_t queue_capacity; /* portable bounds, not an original field */
    AoePlayerResources owner_resources; /* owner +0x50 */
    int16_t owner_id; /* owner +0x4a */
    float carried_amount;       /* +0x44 */
    int16_t carried_resource_type; /* +0x4c */
} AoeBuildingUpdate;

typedef struct {
    void *context;
    /* 004af680: cancel the selected unit id, quantity -1. Must reduce the
       queue count when called by the code-102/103 cancellation loop. */
    void (*cancel)(void *, AoeBuildingUpdate *, int16_t unit_id, int16_t quantity);
    /* Allocation plus 004b6820(building,unit_id,-1,1). NULL is an original
       allocation failure; the original still updates the action list. */
    void *(*create_action)(void *, AoeBuildingUpdate *, int16_t unit_id);
    const AoeActionTickOps *action_tick; /* translated list virtual +0xc */
    /* Global virtual +0x40(event,owner_id,reason,0,0). */
    void (*notify)(void *, uint32_t event, int16_t owner_id, uint32_t reason);
} AoeBuildingProductionOps;

/* RoR 004af1a0 with its optional text output omitted, matching production's
   call with null text and zero length. Read the first action from building
   +0x184, write its common +0x04 type, dispatch 102 to 004b6e00 and 103 to
   004b7d60. Empty/other types return false with ID -1 and progress zero.
   Portable training/research action records put action_type first so no host
   pointer-token or original x86 object-layout assumption is required. */
int aoe_building_active_action_query_ror(const AoeBuildingUpdate *building,
                                          int16_t *action_type,
                                          int16_t *item_id,
                                          int16_t *progress);

typedef struct {
    void *context;
    void (*set_state)(void *, AoeBuildingUpdate *, uint8_t); /* virtual +0x5c */
    /* Root object update 004272e0. The translated 004b0100 prelude invokes
       this after applying its class-18 carried-resource behavior. */
    uint8_t (*parent_update)(void *, AoeBuildingUpdate *);
    float world_delta; /* owner +0x3c -> world +0x84 */
    const AoeBuildingProductionOps *production;           /* 004af900 */
    /* Selected original global +0xa34 + 4*owner_id, required since parent
       can change state/status. Raw unsigned storage preserves DEC wrapping. */
    uint32_t *owner_queue_status_four_count;
} AoeBuildingUpdateOps;

/* RoR 004b0100. Class-18 buildings in state 2 add owner resource 35 times
   world delta to carried amount, clamp it to 100, set carried type 34, then
   all paths tail-call root update 004272e0. The root body remains an explicit
   integration boundary. Finite arithmetic is required by the portable API. */
int aoe_building_parent_update_ror(AoeBuildingUpdate *building,
                                    float world_delta, void *context,
                                    uint8_t (*root_update)(void *,
                                                           AoeBuildingUpdate *),
                                    uint8_t *result);

/* RoR 004af240. Nonzero state or zero signed build time reports 100;
   otherwise truncate elapsed/build-time percent toward zero and retain the
   original low 16 bits. This is a query and does not mutate the building. */
int aoe_building_construction_percent_ror(const AoeBuildingUpdate *building,
                                           int16_t *percent);

/* Complete control/field flow of RoR 004ae880, with untranslated callees
   required explicitly. Callbacks may change state and fields, but must keep
   queue_actions, its owner-state binding and method dependencies valid. Returns 0
   without mutation for preflight dependency/numeric failures. A failure in
   production after callbacks can leave earlier effects applied. On success
   writes the original AL result to result. Numeric support
   covers finite nonnegative build accumulation and positive signed DAT times
   and angle counts. Not a complete building simulation or base-game port. */
int aoe_building_update_ror(AoeBuildingUpdate *building, uint32_t global_7d74d0,
                            const AoeBuildingUpdateOps *ops, uint8_t *result);
/* RoR 004af890, including translated action-list clear. No refund or queue
   array deallocation occurs in this function. The action destructors remain
   responsible for their original class-specific side effects. */
int aoe_building_terminal_cleanup_ror(AoeBuildingUpdate *building,
                                       uint32_t *owner_status_four_count);
/* Type-80 live vtable +0x08 prefix 004ae110. Clears +0x1cc actions, releases
   the +0x1c0 queue allocation and resets +0x1c4/+0x1c6/+0x1c8/+0x1d0/+0x1d1;
   status 4 is cleared and decrements the per-owner counter. The caller
   supplies the original allocator/deallocator and owner-indexed counter. */
int aoe_building_cleanup_type80_ror(
    AoeBuildingUpdate *building, uint32_t *owner_status_four_count,
    void *deallocator_context, void (*deallocate)(void *, void *));
/* 004af7f0: consume one item, shift exhausted head entry, notify event 124.
   Does not clear the now-unused trailing entry, matching the original. */
int aoe_building_queue_consume_ror(AoeBuildingUpdate *building,
                                    const AoeBuildingProductionOps *ops);
/* 004af900–004afb3a. Directly composes list push/pop and queue consumption.
   Valid finite owner resources are required on the empty-action gate.
   Failure after a callback/allocation may leave preceding effects applied. */
int aoe_building_production_ror(AoeBuildingUpdate *building, uint32_t *count,
                                const AoeBuildingProductionOps *ops);
int aoe_building_production_ops_valid(const AoeBuildingProductionOps *ops);
#endif
