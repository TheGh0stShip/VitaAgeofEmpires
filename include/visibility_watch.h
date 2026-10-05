#ifndef VITA_AOE_VISIBILITY_WATCH_H
#define VITA_AOE_VISIBILITY_WATCH_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    AOE_VISIBILITY_BASE = 0,
    AOE_VISIBILITY_RISE_OF_ROME = 1
} AoeVisibilityEdition;

/* Semantic object fields read by the original +0x40 player follow-up. */
typedef struct {
    float x38;
    float y3c;
    float z40;
} AoeVisibilityUnitRef;

/* One slot from the four original parallel arrays at watch +0xc/+0x10/
   +0x14/+0x18. This is portable state, not the packed PC object ABI. */
typedef struct {
    AoeVisibilityUnitRef *unit;
    const uint32_t *live_flags;
    uint32_t previous_flags;
    int32_t unit_id;
} AoeVisibilityWatchEntry;

typedef struct {
    AoeVisibilityWatchEntry *entries;
    int32_t count;
    size_t capacity;
} AoeVisibilityWatch;

/* The player's +0x40 object. Patched RoR 1.0a 0x00436850 constructs it
   with room for `capacity` (100) entries in each of its four arrays and
   0x004368a0 frees them (aoe_visibility_watch_destroy). */
int aoe_visibility_watch_init(AoeVisibilityWatch *watch, size_t capacity);

/* Patched RoR 1.0a 0x004368f0 (noted earlier as RoR 0x00436dd0) / base
   0x004f9470: grow in 32-entry increments (0x00436b30 with 0x20; the
   original grows a full list before the owner test), reject a different
   owner or duplicate unit, and snapshot the pointed live flags. The
   live_flags storage must outlive the watch entry. */
int aoe_visibility_watch_add(AoeVisibilityWatch *watch,
                             int16_t watch_owner_index,
                             int16_t unit_owner_index,
                             AoeVisibilityUnitRef *unit,
                             const uint32_t *live_flags, int32_t unit_id);

/* Semantic world object tables at world +0x88/+0x90 (nonnegative IDs) and
   +0x94/+0x9c (negative IDs). Negative ID -1 uses negative[1], not [0]. */
typedef struct {
    AoeVisibilityUnitRef **positive;
    int32_t positive_count;
    size_t positive_capacity;
    AoeVisibilityUnitRef **negative;
    int32_t negative_count;
    size_t negative_capacity;
} AoeVisibilityRegistry;

/* Per-player effects of the root +0x60/+0x64 state methods, through the
   world's player list (+0x40): player +0x114 object-memory removal
   (0x00517220) and the doppelganger template (243) factory +0x38. Index 0
   is Gaia. A callback a unit needs but that is missing fails the state
   change rather than skipping it. */
typedef struct AoeLiveUnitPlayerEffects {
    void *context;
    int (*memory_remove)(void *context, int player, int32_t unit_id,
                         uint8_t category);
    int (*doppelganger)(void *context, int player, void *unit_core);
} AoeLiveUnitPlayerEffects;

/* Owned form of the original world +0x88..+0x9c unit tables. The counts in
   view are allocated slot counts, not the number of occupied slots. */
typedef struct {
    AoeVisibilityRegistry view;
    uint32_t positive_insertions;
    uint32_t negative_insertions;
    int32_t next_positive_id; /* world +0x68 */
    int32_t next_negative_id; /* world +0x6c */
    int32_t reserved_id;      /* world +0x70 */
    uint8_t special_id_mode;  /* world +0x74 */
    const AoeLiveUnitPlayerEffects *player_effects; /* world +0x40 list */
} AoeOwnedVisibilityRegistry;

typedef struct {
    void *user;
    /* Original recipient player +0x24/+0x3cc virtual +0x38 action. */
    void (*notify_lost)(void *user, int player_index,
                        const AoeVisibilityUnitRef *unit);
} AoeVisibilityWatchOps;

/* RoR 0x005230d0 / base 0x005062f0. Invalid or absent IDs return null.
   The portable form rejects malformed bounds and INT32_MIN safely. */
int aoe_visibility_registry_valid(const AoeVisibilityRegistry *registry);
AoeVisibilityUnitRef *aoe_visibility_registry_resolve(
    const AoeVisibilityRegistry *registry, int32_t unit_id);

/* RoR 0x00523190 / base 0x005063c0 register the object at its signed ID.
   Replacing an occupied positive slot can trigger an edition-specific world
   rebase when a separate world flag is set. In that case insertion clears
   special_id_mode and sets needs_reassign, and replaced receives the old
   object so the caller can perform that still-unported unit method. The
   caller must not continue simulation before handling needs_reassign.
   Negative-slot replacement has no such side effect. */
/* Constructor table sizes: 10,000 positive and 1,000 negative slots.
   Returns zero on allocation failure. */
int aoe_owned_visibility_registry_init(AoeOwnedVisibilityRegistry *registry);
void aoe_owned_visibility_registry_destroy(AoeOwnedVisibilityRegistry *registry);
/* RoR 0x00523120/0x00523160; base 0x00506340/0x00506390.
   The positive cursor skips occupied slots; the negative cursor descends.
   In special-ID mode both return reserved_id with the original cursor rules.
   Overflow is rejected instead of wrapping a signed integer. */
int aoe_owned_visibility_registry_next_positive_id(
    AoeOwnedVisibilityRegistry *registry, int32_t *unit_id);
int aoe_owned_visibility_registry_next_negative_id(
    AoeOwnedVisibilityRegistry *registry, int32_t *unit_id);
/* ID-table portion of world reset: RoR 0x0051f400 / base 0x00502390.
   Call only after the original unit teardown; this does not free objects,
   alter insertion counters, or represent the whole world reset. */
int aoe_owned_visibility_registry_clear_id_tables(
    AoeOwnedVisibilityRegistry *registry);
int aoe_owned_visibility_registry_insert(AoeOwnedVisibilityRegistry *registry,
                                          int32_t unit_id,
                                          AoeVisibilityUnitRef *unit,
                                          AoeVisibilityUnitRef **replaced,
                                          int *needs_reassign);
/* RoR 0x00523360 / base 0x005065a0 clear a valid slot. As in the original,
   this does not reduce either insertion counter. */
int aoe_owned_visibility_registry_clear(AoeOwnedVisibilityRegistry *registry,
                                         int32_t unit_id);

/* Patched RoR 1.0a 0x00436980 (noted earlier as 0x00436e60) / base
   0x004f9520: swap-last removal by unit pointer. */
int aoe_visibility_watch_remove(AoeVisibilityWatch *watch,
                                const AoeVisibilityUnitRef *unit);
void aoe_visibility_watch_destroy(AoeVisibilityWatch *watch);

/* Patched RoR 1.0a 0x00436a20 (noted earlier as 0x00436f20) / base
   0x004f95d0. Both notify recipients 1..15 for
   newly cleared bits 1..15, skipping the owner. RoR removes each invalid
   entry immediately and rechecks a swapped-in slot; base removes only the
   last non-null invalid entry after the scan. Returns zero on invalid
   portable state or missing original-game bindings. */
int aoe_visibility_watch_step(AoeVisibilityWatch *watch,
                              AoeVisibilityEdition edition,
                              int16_t owner_index,
                              const AoeVisibilityRegistry *registry,
                              const AoeVisibilityWatchOps *ops);

#endif
