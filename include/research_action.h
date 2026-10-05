#ifndef VITA_AOE_RESEARCH_ACTION_H
#define VITA_AOE_RESEARCH_ACTION_H

#include "dat.h"
#include "player_resources.h"
#include <stdint.h>

/* Semantic form of each eight-byte research manager entry used by RoR
   004ed8ef: progress at +0 and signed state at +4. */
typedef struct {
    float progress;
    int16_t state;
} AoeResearchProgressEntry;

typedef struct {
    int16_t count;
    const AoeResearchProgressEntry *entries;
    const DatTechs *techs;
} AoeResearchProgressTable;

/* RoR 004ed8ef. Invalid IDs and states other than 2/3 report zero. State 2
   truncates progress*100/research_time toward zero and retains the low signed
   16 bits; state 3 reports 100. Nonfinite or zero-divisor state-2 data is
   rejected by the portable API rather than emulating x87 indefinite values. */
int aoe_research_progress_percent_ror(const AoeResearchProgressTable *table,
                                      int16_t research_id, int16_t *percent);

typedef struct {
    int16_t action_type; /* inherited action +0x04; research action = 103 */
    uint8_t state;       /* inherited action +0x0c */
    int16_t research_id; /* derived action +0x40 */
    const AoeResearchProgressTable *progress_table;
    int32_t request_id;  /* derived action +0x44 */
    /* The building (inherited action +0x08): its ID (+0x04), owner index
       (owner +0x4a), position (+0x38/+0x3c) and template work rate
       (template +0xe4). */
    int32_t building_id;
    int16_t owner_index;
    float building_x, building_y;
    float work_rate;
} AoeResearchAction;

/* Semantic mutable manager used by RoR 004ecd0d. Effect/rebuild/wonder
   callbacks are the three independently recovered callees; their contexts
   and records are portable and make no original pointer-layout claim. */
typedef struct {
    int16_t count;
    AoeResearchProgressEntry *entries;
    void *context;
    void (*apply_effect)(void *context, int16_t research_id);
    void (*rebuild)(void *context);
    void (*wonder_complete)(void *context, int index);
} AoeResearchCompletionManagerRor;

/* RoR 004ecd0d. Valid states 0..2 become 3, then technology effects and the
   manager rebuild run in that order. Research IDs 101..103 additionally
   signal Wonder stage 0..2. Invalid IDs and state -1/3+ return false without
   mutation. */
int aoe_research_complete_ror(AoeResearchCompletionManagerRor *manager,
                              int16_t research_id);

/* Data-return portion of RoR 004b7d60. State 1 reports 100 directly;
   otherwise percentage comes from translated 004ed8ef. The optional localized
   text-buffer branch through 004ed989/00528540 remains outside this API. */
int aoe_research_action_progress_ror(const AoeResearchAction *action,
                                     int16_t *research_id, int16_t *percent);

/* ---- research manager start / progress / stop (owner +0x224) -------- */

/* Manager bindings: the manager (entries +0x00, count +0x04), its tech
   table (+0x08) and the owner's resources (owner +0x0c, then +0x50). */
typedef struct {
    AoeResearchCompletionManagerRor *manager;
    const DatTechs *techs;
    AoePlayerResources *resources;
} AoeResearchOwnerRor;

/* 0x004eaf4c (a player's 0x004f1850 on its manager +0x224): whether
   tech `id` can start. It must be in range and in
   state 1 (available); otherwise *reason = -1. When owner resource 29 is
   at least 1.0, resource 3 (gold) is a reserve: a resource-3 cost first
   draws on the reserve, and any cost resource may fall short by what the
   reserve still holds. For each of the three costs with a nonnegative type
   and a nonzero paid byte, a shortfall the reserve cannot cover fails with
   *reason = the cost's resource type. Returns 1 when it can start, 0 when
   not, -1 for missing bindings or a resource index outside the table. */
int aoe_research_can_start_ror(const AoeResearchOwnerRor *owner, int16_t id,
                               int16_t *reason);

/* 0x004eb100(id, pay, &reason): an out-of-range ID or a failed check
   (0x004eaf4c) sets *reason = -1 and returns 0 (the check's own reason is
   overwritten). With `pay`, each paid cost is deducted in turn: a resource
   below its cost takes the difference from resource 3 and becomes 0.
   The tech then enters state 2 (researching). Returns 1 when started, 0
   when not, -1 for missing bindings. */
int aoe_research_start_ror(const AoeResearchOwnerRor *owner, int16_t id,
                           int pay, int16_t *reason);

/* 0x004eab23(id, amount): a researching (state 2) tech gains `amount`
   progress; while research_time > progress it reports 1 (continue),
   otherwise it completes (0x004eaa3d) and reports 0. Other states and
   out-of-range IDs report 0. Returns 0 for missing bindings, else 1 with
   *researching set. */
int aoe_research_add_progress_ror(const AoeResearchOwnerRor *owner,
                                  int16_t id, float amount,
                                  int *researching);

/* 0x004eb27b(id, refund): only a researching (state 2) tech stops; with
   `refund` each paid cost is added back. It returns to state 1 with zero
   progress. Returns 1 when stopped, 0 otherwise, -1 for missing
   bindings. */
int aoe_research_stop_ror(const AoeResearchOwnerRor *owner, int16_t id,
                          int refund);

/* 0x004ea9e4(id): a tech in state 1 or above (including done) drops to 0
   and the manager rebuilds (0x004eb38d). Returns 1 when reset. */
int aoe_research_reset_ror(AoeResearchCompletionManagerRor *manager,
                           int16_t id);

/* ---- research action (type 0x67, vtable 0x00548990) ----------------- */

typedef struct {
    void *context;
    AoeResearchOwnerRor owner;
    /* Game +0x40(event, owner index, tech, x, y). */
    void (*notify)(void *context, uint32_t event, int16_t owner, int16_t id,
                   int32_t x, int32_t y);
    /* Owner +0x118(reason, tech, building ID, request): not started. */
    void (*failed)(void *context, const AoeResearchAction *action,
                   int16_t reason);
    /* Owner +0x11c(tech, building ID, request): researched. */
    void (*completed)(void *context, const AoeResearchAction *action);
    /* Building +0x148(building ID, building ID, code, 0x26c, 0, 0). */
    void (*signal)(void *context, const AoeResearchAction *action,
                   uint32_t code);
    float tick;          /* world +0x84 */
    int fast_research;   /* game +0x9a8: 1000 progress per update */
    int quiet_teardown;  /* game +0x3f4 byte +0x1d: destructor stays quiet */
} AoeResearchActionOps;

/* Constructor 0x004b60b0(building, tech, request): base action init
   (0x00406ea0 -> 0x00407050, state 0), type 0x67. The caller fills the
   building fields. */
void aoe_research_action_init_ror(AoeResearchAction *action, int16_t id,
                                  int32_t request);

/* Vtable +0x1c (0x004b6200): with `active` and a nonnegative tech, start
   it with payment (0x004eb100). Started: state 6. Otherwise owner
   +0x118(reason), signal 0x1f9, state 1. Returns 0 for missing
   bindings. */
int aoe_research_action_activate_ror(AoeResearchAction *action, int active,
                                     const AoeResearchActionOps *ops);

/* Vtable +0x28 (0x004b6290): state 1 reports 1 (done); state 6 adds
   1000 (fast research) or tick x building work rate (0x004eab23). While
   that continues it reports 0; at completion: state 1, event 0x64 at the
   building's truncated position, owner +0x11c, signal 0x1fa, report 1.
   Other states report 0. Returns 0 for missing bindings. */
int aoe_research_action_update_ror(AoeResearchAction *action,
                                   const AoeResearchActionOps *ops,
                                   uint8_t *result);

/* Vtable +0x2c (0x004b6390): state 1, stop with refund (0x004eb27b),
   event 0x65 (owner, tech, 0, 0), owner +0x118(-1), signal 0x1f9.
   Returns 0 for missing bindings. */
int aoe_research_action_cancel_ror(AoeResearchAction *action,
                                   const AoeResearchActionOps *ops);

/* Destructor 0x004b60f0 (deleting 0x004b6090), only for an action not in state 1: stop without
   refund, then (unless quiet) event 0x65, owner +0x118(-1) and signal
   0x1f9. The resources stay spent. Returns 0 for missing bindings. */
int aoe_research_action_destroy_ror(AoeResearchAction *action,
                                    const AoeResearchActionOps *ops);

#endif
