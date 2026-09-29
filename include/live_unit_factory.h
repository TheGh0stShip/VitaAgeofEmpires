#ifndef VITA_AOE_LIVE_UNIT_FACTORY_H
#define VITA_AOE_LIVE_UNIT_FACTORY_H

#include "live_unit_vision.h"
#include "live_unit_state.h"
#include "action_list.h"
#include "building_update.h"
#include "player_update.h"
#include "unit_sprite_binding.h"

/* A portable owned composition of the parts initialized by the common RoR
   live-unit constructor (0x004a7c80/0x004a8200). It deliberately is not the
   PC object ABI. `owner_node` is the player +0x28/+0x2c/+0x30 list link;
   `placement.node` is the distinct map-tile link. */
typedef struct {
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
    size_t dependent_unit_count;
    size_t dependent_unit_capacity;
    /* Semantic equivalent of the type-20 base live field at RoR +0x88.
       Its constructors at 0x00415770/0x00415790 initialize it to zero; live
       action/state methods subsequently change it. */
    float animation_rate;
    int16_t runtime_owner_index; /* Semantic owner for portable scene state. */
    uint8_t select_2c; /* Constructor argument: live +0x49 (HP > 0). */
    uint8_t select_30; /* Constructor argument: live +0x4a (type 25). */
    uint8_t constructed;
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

typedef struct {
    void *context;
    /* Current unit vtable +0x5c. State changes are class-specific, so this
       hook is mandatory when attached_unit->state is above 2. */
    int (*set_state)(void *context, AoeLiveUnit *unit, uint8_t state);
} AoeLiveUnitUpdateTargetOps;

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
   0x004a8f40. `predicate_result` receives the original byte result: zero for
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
} AoeLiveUnitPlayerUpdateBinding;

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
   action list with the translated 0x00403e70 clear. Other types have no work
   here. Call before the common +0x08 cleanup adapter. */
int aoe_live_unit_cleanup_derived_prefix_ror(AoeLiveUnit *unit);

/* Source-backed composition of the common RoR constructor: initialize core
   fields/ID/storage, create a DAT sprite manager, select template +0x18's
   standing graphic, place on map with the +0xd8/+0xdc vision hooks, then use
   player +0xe0 registration. RoR 0x004a8200 passes the initialized live
   bytes +0x49 (positive HP) and +0x4a (type 25) as its two list selectors.
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
    AoeBuildingCompletionWorldOps world;
    AoeLiveUnitRootStateOps root_state;
} AoeBuildingStateTwoRuntimeOps;

/* Full state-2 composition using translated vision and owned sprite state for
   the common completion virtuals. */
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
