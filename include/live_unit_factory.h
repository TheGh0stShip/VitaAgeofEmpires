#ifndef VITA_AOE_LIVE_UNIT_FACTORY_H
#define VITA_AOE_LIVE_UNIT_FACTORY_H

#include "live_unit_vision.h"
#include "live_unit_state.h"
#include "action_list.h"
#include "building_update.h"
#include "player_update.h"
#include "research_action.h"
#include "unit_sprite_binding.h"

/* A portable owned composition of the parts initialized by the common RoR
   live-unit constructor (0x004a7c80/0x004a8200). It deliberately is not the
   PC object ABI. `owner_node` is the player +0x28/+0x2c/+0x30 list link;
   `placement.node` is the distinct map-tile link. */
typedef struct AoeLiveUnit {
    AoeLiveUnitCore core;
    AoeLiveUnitPlacement placement;
    AoeUnitGraphicState graphic;
    AoeSpriteRuntime sprites;
    AoeObjectUpdateNode *owner_node;
    /* Optional portable owner for the type-40..70 derived cleanup list
       (original live +0x184). Its nodes/actions have class-owned lifetime. */
    AoeActionList *derived_action_list;
    /* Type-80 fields at live +0x1c0..+0x1d2, including queue actions +0x1cc. */
    AoeBuildingUpdate *building_update;
    /* Type-80 live +0x1e0 building-manager association. Semantic pointer;
       never serialized or truncated to the original x86 token width. */
    void *building_manager_link;
    /* Type-80 live +0x1dd. Set before completion visual/world effects. */
    uint8_t building_completed;
    /* Type-80 live +0x1d8 value passed to owner completion virtual +0x104. */
    uint32_t building_completion_value;
    /* Original live +0x64 pointer, +0x68 active count, +0x70 capacity.
       Entries are world unit IDs pruned by the common +0x24 update. */
    int32_t *dependent_unit_ids;
    uint32_t dependent_unit_count;
    uint32_t dependent_unit_capacity;
    int32_t dependent_source_id; /* original live +0x78 */
    uint32_t dependent_context_bits; /* opaque original 32-bit +0x80 */
    /* Command group vector +0x54 (IDs), +0x58 (count), +0x60 (capacity),
       written by 0x004ac280 and emptied by 0x004ac320. */
    int32_t *group_unit_ids;
    uint32_t group_unit_count;
    uint32_t group_unit_capacity;
    /* Semantic equivalent of the type-20 base live field at RoR +0x88.
       Its constructors at 0x00415770/0x00415790 initialize it to zero; live
       action/state methods subsequently change it. */
    float animation_rate;
    int16_t runtime_owner_index; /* Semantic owner for portable scene state. */
    /* Registration selector: live +0x49, the dormancy flag (initialised to
       HP > 0; dormant units sit in player list +0x2c, which no update
       traverses). */
    uint8_t select_2c;
    uint8_t select_30; /* Constructor argument: live +0x4a (type 25). */
    uint8_t constructed;
    /* Type 70 live +0x1bc set by a change of owner (0x004aeac0): the unit
       runs on its own copy of the template (template vtable +0x1c), which
       the unit owns and frees. */
    DatUnit *private_template;
    /* Live +0x24: the units inside this one (garrison, transport), most
       recent first; each has this unit as its placement.attached_unit
       (+0x20). */
    AoeObjectUpdateList contents;
    /* Type-25 doppelganger fields (RoR 0x00436030, live +0x8c..+0xac). */
    struct {
        struct AoeLiveUnit *source;     /* +0x8c, NULL once it died */
        const DatUnit *source_template; /* +0x94 */
        int16_t source_owner;           /* +0x98 (also the +0x90 colour) */
        int32_t source_id;              /* +0xac, -1 once it died */
        const uint32_t *tile_mask;      /* +0x9c: shared mask at its tile */
        uint32_t last_mask;             /* +0xa0 */
        uint32_t hidden_players;        /* +0xa4 */
    } doppel;
    uint8_t byte_86; /* live +0x86, read and written through +0xe8 / +0xec */
} AoeLiveUnit;

/* Source-backed tail of the RoR type-20/25 update predicate at 0x00415820.
   Types 30..80 reach the same tail through their inherited update chain.
   It passes world delta and live +0x88 to sprite-manager virtual +0x08.
   Root type 10 and type 90 use 0x004aa790 directly and do not take this
   path. This is deliberately not claimed as the complete unit update. */
int aoe_live_unit_tick_sprites_ror(AoeLiveUnit *unit, float world_delta);

/* Head of patched RoR 1.0a common live update predicate 0x004a8f40
   (disc 0x004aa790). A pending positive-health registration is applied in
   original remove/register order and terminates this update (`handled=1`).
   Otherwise dependent IDs are stably pruned when world lookup fails or the
   referenced unit state is above 2 (`handled=0`). */
int aoe_live_unit_update_prelude_ror(
    AoeLiveUnit *unit, AoePlayerUpdateState *owner,
    AoeOwnedVisibilityRegistry *world, int *handled);

/* Patched RoR 004abc70/004abc80. Replaces the +0x64 dependency-ID vector,
   ignoring negative and duplicate IDs, while retaining its allocation for
   later calls. `source_id` and `context_bits` preserve the exact 32-bit
   fields written at +0x78/+0x80; context bits are not a host pointer. A
   negative count follows the executable's empty-success branch. */
int aoe_live_unit_has_dependencies_ror(const AoeLiveUnit *unit);
int aoe_live_unit_set_dependencies_ror(
    AoeLiveUnit *unit, const int32_t *unit_ids, int32_t count,
    int32_t source_id, uint32_t context_bits);

/* Patched 004abdf0/004ac030/004ac1b0 single-ID operations. Add rejects any
   call once active count is 25, even a duplicate; its second stack argument
   is present in the executable ABI but unread. Remove deliberately searches
   allocated capacity rather than active count and shifts the whole retained
   allocation, matching the original stale-slot behavior. Clear retains it. */
int aoe_live_unit_add_dependency_ror(
    AoeLiveUnit *unit, int32_t unit_id, uint32_t unread_argument);
int aoe_live_unit_remove_dependency_ror(
    AoeLiveUnit *unit, int32_t unit_id);
int aoe_live_unit_clear_dependencies_ror(AoeLiveUnit *unit);

typedef struct {
    void *context;
    AoeLiveUnit *(*resolve)(void *context, int32_t unit_id);
} AoeLiveUnitDependencyOps;

/* Patched 004abea0 group propagation. The target must resolve and the caller
   must have fewer than 25 active IDs. Every resolved non-self current member
   receives target through 004abdf0; then the caller is rebuilt from its prior
   IDs plus target with caller ID/context stored at +0x78/+0x80. Nested add
   results are ignored by the executable. */
int aoe_live_unit_propagate_dependency_ror(
    AoeLiveUnit *unit, int32_t target_unit_id, uint32_t context_bits,
    const AoeLiveUnitDependencyOps *ops);

/* Patched 004ac0a0/004ac1d0 group removal. Ordinary removal deletes the ID
   from resolved non-self members using 004ac0a0's inline capacity scan (which
   does not clear their +0x78), then calls 004ac030 on the caller. Removing
   the caller's own source ID instead clears every resolved non-self member
   and the caller, as does the explicit group-clear entry. */
int aoe_live_unit_remove_dependency_group_ror(
    AoeLiveUnit *unit, int32_t unit_id,
    const AoeLiveUnitDependencyOps *ops);
int aoe_live_unit_clear_dependency_group_ror(
    AoeLiveUnit *unit, const AoeLiveUnitDependencyOps *ops);

/* Patched 004abd80 group replacement. The caller is used only to reach the
   world lookup (unit +0x0c -> owner +0x3c). A null list or zero count
   returns 0 and a negative count returns 1 without lookups. Otherwise every
   listed ID that resolves, including the caller's, is rebuilt through
   004abc80 from the whole list with the given +0x78/+0x80 values; nested
   results are ignored and the call returns 1. `unit_ids` must not alias a
   member's own vector, which the rebuild may reallocate. */
int aoe_live_unit_set_dependency_group_ror(
    AoeLiveUnit *unit, const int32_t *unit_ids, int32_t count,
    int32_t source_id, uint32_t context_bits,
    const AoeLiveUnitDependencyOps *ops);

typedef struct {
    void *context;
    /* Current unit vtable +0x5c. State changes are class-specific, so this
       hook is mandatory when attached_unit->state is above 2. */
    int (*set_state)(void *context, AoeLiveUnit *unit, uint8_t state);
} AoeLiveUnitUpdateTargetOps;

/* Unit vtable +0xa4 "stop" in patched RoR 1.0a, by class:
   - ROOT (root vtable 0x00547da8): 0x0045bcc0, a bare `ret`;
   - ACTION (0x00542748): 0x004064e0. Unless the head action of the
     +0x184 list accepts cancellation (0x00404530 = head vtable +0x2c,
     returning nonzero), the whole list is cleared (0x00403e50);
   - GROUP (0x0054324c, 0x005445e8, 0x0054820c): 0x00426250, ACTION, then
     the +0x74 group object's vtable +0x58(1) when it exists;
   - BUILDING (type-80 vtable 0x00547fa0): 0x004ae290, GROUP, then the
     production queue is drained one item at a time through 0x004add50. */
typedef enum {
    AOE_LIVE_UNIT_STOP_ROOT_ROR,
    AOE_LIVE_UNIT_STOP_ACTION_ROR,
    AOE_LIVE_UNIT_STOP_GROUP_ROR,
    AOE_LIVE_UNIT_STOP_BUILDING_ROR
} AoeLiveUnitStopClassRor;

typedef struct {
    void *context;
    int (*cancel_head)(void *context, void *action); /* head vtable +0x2c */
    int (*has_group_object)(void *context, const AoeLiveUnit *unit);
    void (*stop_group)(void *context, AoeLiveUnit *unit); /* +0x74 +0x58(1) */
    AoeBuildingQueueCancelOps queue_cancel;
    uint32_t *owner_status_four_count;
} AoeLiveUnitStopOps;

/* Returns 0 for missing dependencies, an invalid action list, or a queue
   drain that fails or stops making progress (host guard). */
int aoe_live_unit_stop_ror(AoeLiveUnit *unit, AoeLiveUnitStopClassRor kind,
                           const AoeLiveUnitStopOps *ops);

/* Patched RoR 1.0a 0x004ac280: append an ID to the +0x54 group vector
   unless already present among the active entries. There is no count
   limit; the allocation grows by exactly one element when full and is kept
   across clears. Returns 0 only for missing/inconsistent storage or
   allocation failure. */
int aoe_live_unit_add_group_member_ror(AoeLiveUnit *unit, int32_t unit_id);

/* Patched RoR 1.0a 0x004ac320: empties the group (count = 0) when the
   owner player's +0x04 (0x00454150) equals 1; otherwise does nothing. */
int aoe_live_unit_reset_group_ror(AoeLiveUnit *unit, int32_t owner_kind);

typedef struct {
    AoeLiveUnitDependencyOps world;
    void *context;
    /* 0x00407a60: the member's +0x74 object is non-null. */
    int (*has_group_object)(void *context, const AoeLiveUnit *unit);
    /* (new_leader +0x74)->0x00413050((source +0x74)->+0x12c): lazily build
       the leader's 0x1c8-byte +0x12c record (0x0040d2f0) and copy the old
       source's record into it (0x0040d670) when both exist. */
    void (*adopt_group)(void *context, AoeLiveUnit *new_leader,
                        AoeLiveUnit *old_source);
} AoeLiveUnitDependencyHandoffOps;

/* State-3 group handoff body inside patched RoR 1.0a root setter
   0x004aa550. Patched AoE 1.0c 0x004a9430 has the same body (getter
   0x004aa8b0, adopt 0x00412e40, lookup 0x00519f60). The setter runs it after
   storing state 3 when the unit is its own source (+0x04 == +0x78), its
   +0x74 object exists and count +0x68 > 1. Over the source's IDs in order
   (count re-read each step), the source's own ID is skipped, and each
   member that resolves with state <= 2 and a group object gets
   +0x78 = the first such member's ID. That first member alone then adopts
   the group through `adopt_group`. The source's own +0x78 and list are
   unchanged. Returns 0 for missing dependencies or a source that does not
   meet the branch condition. */
int aoe_live_unit_dependency_handoff_ror(
    AoeLiveUnit *unit, const AoeLiveUnitDependencyHandoffOps *ops);

typedef struct {
    void *context;
    int (*set_state)(void *context, AoeLiveUnit *unit,
                     uint8_t state); /* current unit vtable +0x5c */
} AoeLiveUnitSetStateOps;

/* Patched RoR 1.0a 0x004ad7b0, building construction progress. A unit
   whose +0x48 state is nonzero reports `completed=1` untouched. Otherwise,
   with T = template +0x15a (train_time) and H = template +0x26 (hit
   points):
   - if amount + elapsed(+0x1d4) > T, amount becomes (float)(T - elapsed);
   - hp(+0x30) += H * amount / T; if the sum exceeds H the stored hp
     becomes H (the original compares the unrounded sum, which is
     equivalent because H is exact and float rounding is monotonic);
   - elapsed += amount; if the unrounded total is >= T, vtable +0x5c sets
     state 2 and `completed=1`, otherwise `completed=0` (also for NaN).
   Unrounded x87 stack values are modelled as double, assuming the MSVC
   default 53-bit precision control; values are rounded to float only where
   the original stores them. Returns 0 only for missing dependencies. */
int aoe_live_unit_add_build_work_ror(
    AoeLiveUnit *unit, float amount, const AoeLiveUnitSetStateOps *ops,
    int *completed);

/* Attached-unit branch of patched RoR 1.0a 0x004a8f40. A non-null original
   live +0x20 suppresses the normal state switch. If the attached unit is in
   state 3 or higher, its state is propagated to the current unit through
   the current class's virtual state setter. `handled` reports whether the
   normal switch must be skipped. */
int aoe_live_unit_update_attached_target_ror(
    AoeLiveUnit *unit, const AoeLiveUnitUpdateTargetOps *ops, int *handled);

typedef struct {
    void *context;
    int (*active_update)(void *context, AoeLiveUnit *unit); /* vtable +0x28 */
    int (*set_state)(void *context, AoeLiveUnit *unit,
                     uint8_t state); /* vtable +0x5c */
    int (*select_graphic)(void *context, AoeLiveUnit *unit,
                          int16_t graphic_id); /* vtable +0x38 */
    void (*play_dying_sound)(void *context, AoeLiveUnit *unit,
                             int16_t sound_id);
    int (*terminal_cleanup)(void *context,
                            AoeLiveUnit *unit); /* vtable +0xb8 */
    float world_delta; /* world +0x3c +0x84 */
    float comparison_epsilon; /* patched binary global +0x00542458 */
} AoeLiveUnitUpdateStateOps;

/* Normal (no attached +0x20 unit) state switch of patched RoR 1.0a
   0x004a8f40. States 0/2 die (state 3, dying graphic template +0x1c,
   dying sound) below 1.0 hit points. The sound callback must reproduce the
   original rand()/srand(that value) bracket around the local-player-only
   sound so every player's RNG advances identically. `predicate_result` receives the original byte result: zero for
   ordinary transitions, or 1/2 in state 8 according to template +0xa4. */
int aoe_live_unit_update_normal_state_ror(
    AoeLiveUnit *unit, const AoeLiveUnitUpdateStateOps *ops,
    uint8_t *predicate_result);

/* Composes the common predicate in patched RoR 1.0a player-list vtable
   +0x24 order: deferred registration/dependency pruning, attached target,
   then the ordinary state switch. This is the method result consumed by the
   player's object-list dispatcher; callers still supply derived virtuals. */
int aoe_live_unit_update_predicate_ror(
    AoeLiveUnit *unit, AoePlayerUpdateState *owner,
    AoeOwnedVisibilityRegistry *world,
    const AoeLiveUnitUpdateTargetOps *target_ops,
    const AoeLiveUnitUpdateStateOps *state_ops,
    uint8_t *predicate_result);

/* Tail of the patched RoR 1.0a type-20 update predicate 0x00415200, run
   after its root call 0x004a8f40 (types 20..80 inherit it). State 3 moves
   through the class setter to 4 when the template has a dying graphic
   (+0x1c), else to 5. State 4 waits on the sprite manager's +0x18
   (0x00415890: the current graphic's sprite +0x20, its frame; static or
   missing sprites answer 0): when the frame's low byte is 0 the state byte
   becomes 5 directly and +0x88 is cleared. Every path then ticks the
   sprites (aoe_live_unit_tick_sprites_ror). */
int aoe_live_unit_update_type20_tail_ror(AoeLiveUnit *unit,
                                         const AoeLiveUnitUpdateStateOps *ops,
                                         int has_dying_graphic);

typedef struct {
    void *context;
    /* Unit +0x1fc = 0x004063d0: build the template's default task (+0xdc)
       through the action factory and adopt it (+0x208). */
    int (*default_task)(void *context, AoeLiveUnit *unit, int16_t task_id);
    /* List vtable +0x08, used instead of the update while an attached unit
       (+0x20) is present. */
    int (*attached_update)(void *context, AoeLiveUnit *unit,
                           AoeActionList *list);
    const AoeActionTickOps *tick; /* list vtable +0x0c = 0x004043b0 */
} AoeLiveUnitActionListOps;

/* Action-list part of the type-40 update 0x00406500, after its 0x00415200
   call. Above state 2 a non-empty +0x184 list is cleared (0x00403e50). An
   empty list in state 2 builds the template default task when it is not
   -1. Otherwise the list updates (0x004043b0, which already pops the head
   on result 1) and the head is popped again (0x004044d0) for result 1 or
   3, as in the executable. A NULL list does nothing. */
int aoe_live_unit_update_action_list_ror(AoeLiveUnit *unit,
                                         AoeActionList *list,
                                         const AoeLiveUnitActionListOps *ops);

typedef struct {
    AoePlayerUpdateState *owner;
    AoeOwnedVisibilityRegistry *world;
    AoeLiveUnit *const *units;
    size_t unit_count;
    AoeLiveUnitUpdateTargetOps target_ops;
    AoeLiveUnitUpdateStateOps state_ops;
    void *context;
    /* Patched common cleanup body 0x004a6920, used directly by root classes
       and after the translated 40..70 prefix. */
    int (*common_cleanup)(void *context, AoeLiveUnit *unit);
    /* Type-80 global +0xa34 + 4*owner_id status counter resolver. */
    uint32_t *(*queue_status_four_count)(void *context, int16_t owner_id);
    /* Matching allocator release for AoeBuildingUpdate.queue_entries. */
    void (*deallocate_queue_entries)(void *context, void *allocation);
    /* The list also contains objects outside this portable live-unit
       composition. These callbacks preserve their original vtable dispatch. */
    int (*other_predicate)(void *context, void *object);
    void (*other_action)(void *context, void *object);
    int (*cooperative_pending)(void *context);
    void (*cooperative_pump)(void *context);
    int failed;
    /* Optional. Resolves list objects missing from `units` (for example
       units created during this traversal); NULL means "not a live unit". */
    AoeLiveUnit *(*resolve_live)(void *context, void *object);
    /* Optional derived +0x24 layers around the common predicate: `before`
       runs ahead of it (e.g. the type-50 reload countdown) and `after`
       receives its byte result (e.g. the 0x00415200 tail and 0x00406500
       action list) and may replace it. */
    int (*before_update)(void *context, AoeLiveUnit *unit);
    int (*after_update)(void *context, AoeLiveUnit *unit, uint8_t *result);
    /* Optional. A unit's whole +0x24 when the class wraps the common
       predicate itself (type 80's 0x004acf80); return -1 to fall back to
       before/predicate/after, 0 on failure, 1 with *result written. */
    int (*live_update)(void *context, AoeLiveUnit *unit, uint8_t *result);
} AoeLiveUnitPlayerUpdateBinding;

/* Doppelganger constructor tail 0x00436030 (after the common constructor
   built template 243 for the observing player at the source position)
   and 0x004364b0: source links and template (+0x8c/+0x94/+0x98/+0xac),
   the source's current graphic (+0x10), a clone of its sprite list
   (0x004155e0 -> 0x00415620), its facet (+0x35), class 0x19 (+0x4e) and
   the tile-mask pointer (+0x9c). A source already in state 7+ is not
   kept. The +0xa8/+0xa9 display bytes (source template +0x8e / +0x90) are
   not parsed yet. */
int aoe_live_unit_doppelganger_init_ror(AoeLiveUnit *unit,
                                        AoeLiveUnit *source,
                                        const uint32_t *tile_mask);

/* The same bound per-object update (predicate, then +0x08 cleanup when it
   reports done) over another player list: the world loop's second pass
   over each player's +0x30 list (type-25 doppelgangers). */
int aoe_live_unit_player_list_update_ror(
    AoeLiveUnitPlayerUpdateBinding *binding, AoeObjectUpdateList *list);

/* Installs callbacks into a copy of AoePlayerUpdateOps and runs the actual
   Rise of Rome player +0x7c update. It matches list object pointers against
   the supplied owned units' +0x38 fields, and routes every other object
   through its original dynamic-class adapter. */
int aoe_live_unit_player_update_step_ror(
    AoeLiveUnitPlayerUpdateBinding *binding, AoePlayerUpdateState *owner,
    const AoePlayerUpdateOps *ops);

/* Pool branch condition used by the common RoR +0x08 cleanup: template byte
   +0xa4 must be set and type-specific world pool dispatch 0x00523400 must
   accept the type. This reports eligibility; it does not enqueue the unit. */
int aoe_live_unit_cleanup_uses_reuse_pool_ror(const AoeLiveUnit *unit);

/* Derived cleanup prefix 0x00405a70: types 40..70 and 80 drain their +0x184
   action list with the translated clear (patched 0x00403e50, disc 0x00403e70). Other types have no work
   here. Call before the common +0x08 cleanup adapter. */
int aoe_live_unit_cleanup_derived_prefix_ror(AoeLiveUnit *unit);

/* Root state setter 0x004aa550 (types 10..70) composed with the owned
   scenario runtime: owner registration lists, visibility leave/enter and
   the mode-2 visibility watch. Refuses (returns 0) type 80 (setter
   0x004acbf0), type-70 class-1 units leaving the active states (0x004aeed0),
   the state-3 dependency leadership handoff, and transitions reaching the
   template +0xa6 == 2 method or the pre-terminal +0x60 outside their
   translated zero high-mask case. */
int aoe_live_unit_scenario_state_set_ror(
    AoeLiveUnit *unit, uint8_t new_state, AoePlayerResources *owner_resources,
    AoePlayerUpdateState *owner, AoeOwnedVisibilityRegistry *world,
    AoeLiveUnitVisionBinding *vision);

typedef struct {
    void *context;
    /* Unit +0x38: select a graphic as the unit's current one. */
    int (*select_graphic)(void *context, AoeLiveUnit *unit,
                          const DatGraphic *graphic);
    const DatGraphics *graphics;
    const DatMapGeometry *geometry; /* map +0x8d9c/+0x8d9a half tile */
    AoeRng *rng;                    /* the game's rand() */
} AoeLiveUnitDamageGraphicOps;

/* Root state-2 update +0x28 = 0x004a9240: damage graphics. With template
   HP > 0 and damage entries (+0x93/+0x94), the damage level is 100 minus
   ftol(HP x 100 / template HP), as a byte. An unchanged level does
   nothing; a level >= 100 (unsigned, so also negative damage) is only
   stored. Otherwise the last entry whose percent is below the new level
   and the last below the old level (+0x34) are chosen; when they differ
   the new entry is applied - mode 0 adds its sprite (priority 0x5a), mode
   1 adds it at a random offset (two rand() calls: +/- template size around
   the unit's position in its tile, projected by 0x00448cc0), mode 2 makes
   it the unit's graphic - and then the old one is undone: modes 0/1
   remove their sprite, mode 2 restores the standing graphic unless the
   new entry is also mode 2. The new level is stored in +0x34. */
int aoe_live_unit_damage_graphics_ror(AoeLiveUnit *unit,
                                      const AoeLiveUnitDamageGraphicOps *ops);

/* 0x00448cc0: tile-space offset to screen offset. x = ftol((x + y) x half
   width) + base_x, y = ftol((y - x + 1 - z) x half height) + base_y - 16,
   both kept as int16. */
void aoe_map_screen_offset_ror(const DatMapGeometry *geometry, float x,
                               float y, float z, int32_t base_x,
                               int32_t base_y, int16_t *out_x,
                               int16_t *out_y);

/* Patched RoR 1.0a 0x004ac380 (disc 0x004adc70) on a constructed unit with
   its owner lists. Waking (0) a dormant unit removes it from +0x2c and
   registers it in +0x28 (or +0x30 for type 25), unless a deferred
   dormancy (+0x4b) is pending, which is simply cancelled. Making a unit
   dormant (1) is deferred to the next update prelude. Callers include
   the type-20 constructor, target attach/release (+0xc0 0x004aad90 /
   +0xc4 0x004aadb0) and the state setter. */
int aoe_live_unit_set_dormant_ror(AoeLiveUnit *unit,
                                  AoePlayerUpdateState *owner,
                                  AoeOwnedVisibilityRegistry *world,
                                  uint8_t dormant);

/* Target attach +0xc0: root 0x004aad90 (types 50..80 through 0x004152c0)
   counts +0x4f up to 0xfa and wakes the unit. */
int aoe_live_unit_target_attach_ror(AoeLiveUnit *unit,
                                    AoePlayerUpdateState *owner,
                                    AoeOwnedVisibilityRegistry *world);
/* Target release +0xc4: root 0x004aadb0 counts +0x4f down; at zero a unit
   in state 2 with hit points >= 0 (or NaN), or in state 6, becomes dormant
   (deferred). Types 20..40 (0x004152e0) and 50..80 (0x00426b40) wake it
   again at once, and 50..80 then notice 0x2bb (*notice set to 1). */
int aoe_live_unit_target_release_ror(AoeLiveUnit *unit,
                                     AoePlayerUpdateState *owner,
                                     AoeOwnedVisibilityRegistry *world,
                                     int *notice);

/* Type-80 constructor body 0x004ac8a0 (after the inherited constructors)
   on a bound building: clears +0x1dd (completed), sets class code 0x50,
   enters state 0 through the setter (root 0x004aa550 path), selects the
   construction graphic (template +0x168) when the build time (+0x15a) is
   positive, zeroes the elapsed build time (+0x1d4), sets HP to 1.0 and
   +0x35 to the template's building angle (+0x16e). Not modelled here: the
   +0x1dc owner byte, world 0x0050a600 and the +0x21c queue-list creation
   (the bound storage supplies the list). Every type-80 unit starts as a
   foundation this way, also when a scenario places it finished. */
int aoe_live_unit_building_foundation_ror(
    AoeLiveUnit *unit, AoePlayerResources *owner_resources,
    AoePlayerUpdateState *owner, AoeOwnedVisibilityRegistry *world,
    AoeLiveUnitVisionBinding *vision, const DatGraphics *graphics);

/* Type-80 storage effects over template +0x6a/+0x70/+0x88: entries with
   mode >= 4 and type >= 0 call owner +0x78(type, sign x amount, 0) -
   +0x260 = 0x004acaf0 on completion (+1) and +0x25c = 0x004aca90 when a
   finished building starts dying (-1), e.g. a house's population room. */
int aoe_live_unit_building_storage_ror(
    AoeLiveUnit *unit, int sign,
    void (*add)(void *context, AoeLiveUnit *unit, int16_t type,
                float amount),
    void *context);

typedef struct {
    void *context;
    /* Owner player +0x78(type, amount, 0). */
    void (*storage_add)(void *context, AoeLiveUnit *unit, int16_t type,
                        float amount);
    /* World +0x40 0x00436980; clears semantic +0x1e0. */
    int (*remove_building_manager)(void *context, AoeLiveUnit *unit);
    /* Owner list +0x28 0x00451370(template ID, -1, -1, 1, 2, 0): another
       state-2 unit of the template remains. */
    int (*completion_probe)(void *context, AoeLiveUnit *unit,
                            int16_t template_id);
    /* Owner +0x224 0x004ea9e4(research ID). */
    void (*remove_research)(void *context, AoeLiveUnit *unit,
                            int16_t research_id);
    uint32_t *owner_status_four_count; /* global +0xa34 + 4 x owner */
    AoeBuildingQueueCancelOps queue_cancel;
} AoeBuildingSetterRuntimeOps;

/* Type-80 setter 0x004acbf0 for every requested state but 2 (completion,
   see aoe_live_unit_scenario_building_complete_ror) on the owned scenario
   runtime. 3 from 2 runs the state-three branch (+0x25c storage removal,
   building-manager removal, queue cancellation, root setter, research
   removal unless the probe finds another); any other state runs the queue
   transition and the root setter. The building view's state follows. */
int aoe_live_unit_scenario_building_state_set_ror(
    AoeLiveUnit *unit, uint8_t new_state, AoePlayerResources *owner_resources,
    AoePlayerUpdateState *owner, AoeOwnedVisibilityRegistry *world,
    AoeLiveUnitVisionBinding *vision,
    const AoeBuildingSetterRuntimeOps *ops);

/* Source-backed composition of the common RoR constructor: initialize core
   fields/ID/storage, create a DAT sprite manager, select template +0x18's
   standing graphic, place on map with the +0xd8/+0xdc vision hooks, then use
   player +0xe0 registration. RoR 0x004a8200 passes the initialized live
   bytes +0x49 (dormancy, initially HP > 0) and +0x4a (type 25) as its two
   list selectors. Types 20..80 then run the type-20 constructor tail
   0x00415130: +0x88 is cleared and the unit is woken (0x004ac380(0)), so
   only types 10 and 90 stay dormant.
   On failure no player/world/tile link remains. Resource effects already
   performed by the original-order core
   initializer are intentionally not rolled back. */
int aoe_live_unit_construct_ror(
    AoeLiveUnit *out, const DatUnit *unit_template, const DatGraphics *graphics,
    AoePlayerResources *owner_resources, AoeOwnedVisibilityRegistry *world,
    AoePlayerUpdateState *owner, const ScnTile *terrain_tiles,
    AoeObjectUpdateList *tile_lists, const DatMapGeometry *geometry,
    uint32_t width, uint32_t height,
    AoeLiveUnitVisionBinding *vision, AoeRng *rng,
    float x, float y, float z);

/* Releases only links and sprite instances owned by this portable
   composition, including the matching state-2 visibility leave. It never
   frees the DAT template, map, player, world, or RNG. */
void aoe_live_unit_destroy_ror(AoeLiveUnit *unit, AoePlayerUpdateState *owner,
                               AoeOwnedVisibilityRegistry *world,
                               AoeLiveUnitVisionBinding *vision);

/* Bind caller-owned type-80 semantic storage after live construction. This
   translates the direct initialization fields of disc constructors
   004adcf0/004ade50/004adf70 and initialized body 004ae1a0. The three storage
   objects and action callback contexts must outlive the live unit or be
   detached after their original-order cleanup. */
int aoe_live_unit_bind_building_update_ror(
    AoeLiveUnit *unit, AoeBuildingUpdate *building,
    AoeActionList *live_actions, AoeActionListOps live_action_ops,
    AoeActionList *queue_actions, AoeActionListOps queue_action_ops,
    AoePlayerResources owner_resources, int16_t owner_id,
    uint32_t completion_value);

/* Symmetric type-80 teardown for caller-owned semantic storage. Clears the
   production list and queue allocation through translated 004ae110, then
   clears the inherited live-action list before detaching both live pointers.
   A queue allocation requires the matching caller deallocator; status 4
   requires its owner counter. Storage itself remains caller-owned. */
int aoe_live_unit_unbind_building_update_ror(
    AoeLiveUnit *unit, uint32_t *owner_queue_status_four_count,
    void *deallocator_context, void (*deallocate)(void *, void *));

typedef struct {
    AoeLiveUnitRootHighStateOps root_high;
    AoeLiveUnitMiddleActionHooks root_middle_action;
    AoeLiveUnitRegistrationHooks registration;
    void **registration_node;
    /* Type 70/80 middle state +0x6c = 0x004b02a0. The caller supplies the
       world timer at player+0x50+0xe4 and the 0x004045c0 lookup boundary. */
    float derived_middle_time;
    float derived_middle_threshold;
    int (*derived_middle_lookup)(void *context, int16_t *kind);
    void (*derived_middle_kind_68)(void *context, AoeLiveUnit *unit);
    /* Type 40/50/60/70 high state +0x68 = 0x00405c80. */
    void (*clear_derived_actions)(void *context, AoeLiveUnit *unit);
    /* Remaining type-80 high-state effects around translated inherited work. */
    void (*building_pre_high)(void *context, AoeLiveUnit *unit); /* +0x258 */
    int (*remove_building_manager)(void *context, AoeLiveUnit *unit);
    void *context;
} AoeLiveUnitScenarioDispatchOps;

/* Disc type-80 high state 004ae450 / patched 1.0a 004acb40. Runs the
   building-specific +0x258 prelude, translated derived-list clear and root
   high transition, removes the unit from its building manager, then clears
   semantic live +0x1e0. */
int aoe_live_unit_building_high_state_ror(
    AoeLiveUnit *unit, const AoeLiveUnitScenarioDispatchOps *ops);

typedef struct {
    void *context;
    void (*building_pre_state_three)(void *context, AoeLiveUnit *unit);
    int (*remove_building_manager)(void *context, AoeLiveUnit *unit);
    /* Disc 004522e0 / patched 00451370, called with template ID,
       -1.0, -1.0, 1, 2, 0. Nonzero suppresses cleanup IDs. */
    int (*completion_probe)(void *context, int16_t template_id);
    /* Disc 004eccb4 / patched 004ea9e4. */
    void (*remove_research)(void *context, int16_t research_id);
    uint32_t *owner_status_four_count;
    AoeBuildingQueueCancelOps queue_cancel;
    AoeLiveUnitRootStateOps root_state;
} AoeBuildingStateThreeOps;

/* State-3 branch of disc type-80 setter 004ae500 / patched 004acbf0.
   Requires the semantic live/building views to represent the same original
   object at state 2. This excludes the large state-2 completion branch. */
int aoe_live_unit_building_state_three_ror(
    AoeLiveUnit *unit, const AoeBuildingStateThreeOps *ops);

typedef struct {
    void *context;
    AoeRng *rng;
    /* Caller-resolved original template +0x164 non-null and owner-is-local
       condition. That unrecovered pointer is not fabricated in DatUnit. */
    uint8_t signal_local_completion;
    void (*local_completion)(void *context, AoeLiveUnit *unit, int value);
    void (*building_pre_complete)(void *context, AoeLiveUnit *unit);
    /* Original type-80 vtable +0xdc(owner,1,-1), then +0xd8(owner,0,-1). */
    int (*visibility_dc)(void *context, AoeLiveUnit *unit);
    int (*visibility_d8)(void *context, AoeLiveUnit *unit);
    int (*select_graphic)(void *context, AoeLiveUnit *unit,
                          int16_t graphic_id);
    void (*complete_adjacent)(void *context, AoeLiveUnit *unit);
    void (*disappear)(void *context, AoeLiveUnit *unit);
} AoeBuildingCompletionVisualOps;

/* Source-backed prefix of requested-state 2 in disc 004ae500 / patched
   004acbf0, ending after the disappear or completed visual/adjacency branch.
   It does not perform stack spawning, foundation painting, research, event
   105, owner finalization, queue handling, or the inherited state setter. */
int aoe_live_unit_building_completion_visual_ror(
    AoeLiveUnit *unit, const AoeBuildingCompletionVisualOps *ops);

typedef struct {
    void *context;
    AoeRng *rng;
    const DatGraphics *graphics;
    AoeLiveUnitVisionBinding *vision;
    uint8_t signal_local_completion;
    void (*local_completion)(void *context, AoeLiveUnit *unit, int value);
    void (*building_pre_complete)(void *context, AoeLiveUnit *unit);
    void (*complete_adjacent)(void *context, AoeLiveUnit *unit);
    void (*disappear)(void *context, AoeLiveUnit *unit);
} AoeBuildingCompletionVisualRuntimeOps;

/* Binds the completion prefix's +0xdc leave, +0xd8 enter and +0x38 graphic
   virtuals to translated owned runtime state. The remaining class-specific
   effects stay explicit. */
int aoe_live_unit_building_completion_visual_runtime_ror(
    AoeLiveUnit *unit, const AoeBuildingCompletionVisualRuntimeOps *ops);

typedef struct {
    int16_t x0, y0, x1, y1;
} AoeBuildingFoundationBounds;

/* Exact finite/in-range rectangle calculation around disc 004ae60d..004ae6c3.
   Upper edges subtract the executable's double 0.001 constant before x87
   truncation; each edge is narrowed to the signed 16-bit value consumed by
   the terrain routines. */
int aoe_live_unit_building_foundation_bounds_ror(
    const AoeLiveUnit *unit, AoeBuildingFoundationBounds *bounds);

/* Semantic view of the selected-object entries visited by derived RoR player
   vtable 0054ee68 +0x104 -> 004f4fc0 -> 004ba540. These are portable fields,
   not the original GUI-node layout. */
typedef struct AoeBuildingCompletionUiEntryRor {
    struct AoeBuildingCompletionUiEntryRor *next;
    int16_t template_id;
    int32_t completion_key;
    int32_t object_id;
    const char *display_name;
    uint8_t active;
    uint8_t completed;
} AoeBuildingCompletionUiEntryRor;

typedef struct {
    AoeBuildingCompletionUiEntryRor *head;
    uint32_t *wonder_completed;
    char selected_name[63];
    int32_t selected_completion_key;
    void *refresh_context;
    void (*refresh)(void *context);
} AoeBuildingCompletionUiRor;

/* Exact data/control portion of 004ba540. A null UI matches the outer
   player's null +0x23c branch and has no effect. A present UI updates only
   the first matching incomplete entry, then always requests the original
   summary rebuild through `refresh`. `selected_name` deliberately has the
   original 63-byte strncpy semantics and may not be NUL-terminated. */
int aoe_live_unit_owner_complete_ui_ror(
    AoeBuildingCompletionUiRor *ui, const AoeLiveUnit *unit,
    uint32_t completion_value);

typedef struct {
    void *context;
    void (*spawn_stack_unit)(void *context, AoeLiveUnit *unit,
                             int16_t unit_id, float x, float y, float z);
    void (*paint_foundation)(void *context, AoeLiveUnit *unit,
                             const AoeBuildingFoundationBounds *bounds,
                             uint8_t terrain_id);
    void (*invalidate_foundation)(void *context, AoeLiveUnit *unit,
                                  int32_t x0, int32_t y0,
                                  int32_t x1, int32_t y1);
    void (*apply_research)(void *context, AoeLiveUnit *unit,
                           int16_t research_id);
    void (*notify_completion)(void *context, uint32_t event,
                              int16_t owner_id, int16_t unit_id,
                              int32_t x, int32_t y);
    void (*owner_complete)(void *context, AoeLiveUnit *unit,
                           uint32_t completion_value);
} AoeBuildingCompletionWorldOps;

/* Continuation of requested-state 2 in disc 004ae500 / patched 004acbf0,
   from optional stack-unit creation through owner virtual +0x104. Requires
   the visual prefix to have set semantic +0x1dd first. */
int aoe_live_unit_building_completion_world_ror(
    AoeLiveUnit *unit, const AoeBuildingCompletionWorldOps *ops);

typedef struct {
    void *context;
    void (*spawn_stack_unit)(void *context, AoeLiveUnit *unit,
                             int16_t unit_id, float x, float y, float z);
    int (*paint_foundation)(void *context, AoeLiveUnit *unit,
                            const AoeBuildingFoundationBounds *bounds,
                            uint8_t terrain_id);
    void (*invalidate_foundation)(void *context, AoeLiveUnit *unit,
                                  int32_t x0, int32_t y0,
                                  int32_t x1, int32_t y1);
    AoeResearchCompletionManagerRor *research;
    AoeBuildingCompletionUiRor *owner_ui; /* derived player +0x23c; optional */
} AoeBuildingCompletionWorldRuntimeOps;

/* Disc global application vtable 00547de8 +0x40 -> 00417910. The apparent
   event-105 dispatch is an unconditional no-op in Rise of Rome. */
void aoe_live_unit_building_completion_event_ror(
    uint32_t event, int16_t owner_id, int16_t unit_id, int32_t x, int32_t y);

/* Binds the inert event slot and derived player +0x104 to their translated
   routines. A null owner_ui is the original headless branch. Unlike the
   original void world callback, the runtime paint adapter reports failure so
   an unsupported owned removal/creation cannot be accepted silently. */
int aoe_live_unit_building_completion_world_runtime_ror(
    AoeLiveUnit *unit, const AoeBuildingCompletionWorldRuntimeOps *ops);

typedef struct {
    AoeBuildingCompletionVisualOps visual;
    AoeBuildingCompletionWorldOps world;
    AoeLiveUnitRootStateOps root_state;
} AoeBuildingStateTwoOps;

/* Complete requested-state-2 branch and common inherited setter tail of disc
   004ae500 / patched 004acbf0. The class/world method bodies exposed by the
   two completion operation sets remain explicit integration boundaries. */
int aoe_live_unit_building_state_two_ror(
    AoeLiveUnit *unit, const AoeBuildingStateTwoOps *ops);

typedef struct {
    AoeBuildingCompletionVisualRuntimeOps visual;
    AoeBuildingCompletionWorldRuntimeOps world;
    AoeLiveUnitRootStateOps root_state;
} AoeBuildingStateTwoRuntimeOps;

/* Requested-state-2 completion of a type-80 foundation (patched setter
   0x004acbf0) on the owned scenario runtime: the translated visual and
   world continuations with the root setter tail composed like
   aoe_live_unit_scenario_state_set_ror. */
int aoe_live_unit_scenario_building_complete_ror(
    AoeLiveUnit *unit, const AoeBuildingCompletionVisualRuntimeOps *visual,
    const AoeBuildingCompletionWorldRuntimeOps *world_ops,
    AoePlayerResources *owner_resources, AoePlayerUpdateState *owner,
    AoeOwnedVisibilityRegistry *world, AoeLiveUnitVisionBinding *vision);

/* Full state-2 composition using translated vision and owned sprite state for
   the common completion virtuals and the derived-player owner finalizer. */
int aoe_live_unit_building_state_two_runtime_ror(
    AoeLiveUnit *unit, const AoeBuildingStateTwoRuntimeOps *ops);

/* Player scenario-spawn dispatch after construction (RoR player +0x60,
   0x0045d1c0). It selects the recovered class vtable family for the record
   state: 0..2 low (including rotation), 3..6 middle, 7+ high. Unsupported
   derived methods are mandatory hooks; no branch assigns `core.state` as a
   stand-in for the original virtual call. */
int aoe_live_unit_scenario_dispatch_ror(
    AoeLiveUnit *unit, uint8_t scenario_state, float rotation,
    const AoeLiveUnitScenarioDispatchOps *ops);

/* Source-complete root high-state composition for common units whose
   template +0xa5/+0xa6 effects are both zero. This binds the translated
   player reclassification and unit vision hooks to persistent factory
   ownership. Types with derived high methods or remaining template effects
   fail before changing the unit. */
int aoe_live_unit_scenario_high_root_ror(
    AoeLiveUnit *unit, AoePlayerResources *owner_resources,
    AoePlayerUpdateState *owner, AoeOwnedVisibilityRegistry *world,
    AoeLiveUnitVisionBinding *vision);

#endif
