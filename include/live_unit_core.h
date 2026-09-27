#ifndef VITA_AOE_LIVE_UNIT_CORE_H
#define VITA_AOE_LIVE_UNIT_CORE_H

#include "live_unit_id.h"
#include "player_resources.h"
#include "terrain.h"
#include "unit_storage.h"
#include "visibility_watch.h"

/* Directly recoverable semantic fields of RoR 0x004a8200 / base
   0x00412c20. This is not either original 32-bit object layout. */
typedef struct {
    const DatUnit *unit_template; /* original unit +0x08 */
    int32_t unit_id;              /* +0x04 */
    AoeVisibilityUnitRef position;/* +0x38/+0x3c/+0x40 */
    int16_t draw_left;            /* +0x28 */
    int16_t draw_top;             /* +0x2a */
    int16_t draw_right;           /* +0x2c */
    int16_t draw_bottom;          /* +0x2e */
    float hit_points;             /* +0x30 */
    uint8_t action_flags_36;      /* +0x36, bit 0 gates action cleanup */
    AoeUnitStorageState storage;  /* +0x44/+0x4c */
    uint8_t state;                /* +0x48, initially 2 */
    uint8_t positive_hit_points;  /* +0x49 */
    uint8_t type_25;             /* +0x4a */
    uint8_t registration_pending; /* +0x4b */
    uint8_t class_code;          /* +0x4e, initially 10 */
    /* Type-30..80 derived live field +0x9c, written by scenario low-state
       virtual 0x0044ca40. It is not present in the root object. */
    float scenario_rotation;
    /* Type-30..80 current-animation frame byte +0x35, set by 0x0044c7b0. */
    uint8_t scenario_frame;
} AoeLiveUnitCore;

typedef struct {
    void *context;
    /* Original player vtable +0xe0 then +0xdc. The node is unit +0x50;
       these methods are required when a live unit changes list category. */
    void (*remove)(void *context, AoeLiveUnitCore *unit,
                   uint8_t was_positive, uint8_t type_25, void *node);
    void *(*register_unit)(void *context, AoeLiveUnitCore *unit,
                           uint8_t is_positive, uint8_t type_25);
} AoeLiveUnitRegistrationHooks;

/* Translates only the direct-field/storage/ID portion of the original
   constructor. Calls for sprite/list objects, standing graphic, placement,
   owner registration, and post-registration effects remain unported. As in
   the original call order, storage can change owner resources before a
   later ID allocation failure. `out` is written only on success. */
int aoe_live_unit_core_begin(const DatUnit *unit_template,
                             AoePlayerResources *owner_resources,
                             AoePlayerEdition edition,
                             AoeOwnedVisibilityRegistry *registry,
                             float x, float y, float z,
                             AoeLiveUnitCore *out);

/* Terrain-height portion of placement vtable +0x34. The caller must still
   perform the original tile linkage and owner/world method calls. */
int aoe_live_unit_core_apply_surface(AoeLiveUnitCore *unit,
                                      const ScnTile *tiles,
                                      uint32_t width, uint32_t height);

/* RoR 0x004adc70 / base 0x00418890. Changing a live positive-health
   category to zero either
   clears the deferred flag or removes and re-registers the unit in the
   original order. A zero-to-one change is deferred. No owner effects are
   silently skipped: immediate reclassification requires both callbacks.
   `registration_node` is the semantic unit +0x50 field. */
int aoe_live_unit_health_flag_set(AoeLiveUnitCore *unit, uint8_t is_positive,
                                   void **registration_node,
                                   const AoeLiveUnitRegistrationHooks *hooks);

typedef struct {
    void *context;
    void (*clear_active_action)(void *context, AoeLiveUnitCore *unit);
} AoeLiveUnitMiddleActionHooks;

/* Root RoR unit vtable +0x6c = 0x004abbc0; base equivalent 0x004166c0.
   Both conditionally clear an active action before health/HP changes.
   Derived classes override this slot; callers must select the original
   class method before using it. */
int aoe_live_unit_root_middle_state(AoeLiveUnitCore *unit,
                                     void **registration_node,
                                     const AoeLiveUnitRegistrationHooks *hooks,
                                     const AoeLiveUnitMiddleActionHooks *action);

#endif
