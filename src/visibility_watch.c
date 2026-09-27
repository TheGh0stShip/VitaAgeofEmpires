#include "visibility_watch.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

int aoe_visibility_registry_valid(const AoeVisibilityRegistry *registry) {
    return registry && registry->positive_count >= 0 &&
           registry->negative_count >= 0 &&
           (size_t)registry->positive_count <= registry->positive_capacity &&
           (size_t)registry->negative_count <= registry->negative_capacity &&
           (!registry->positive_count || registry->positive) &&
           (!registry->negative_count || registry->negative);
}

AoeVisibilityUnitRef *aoe_visibility_registry_resolve(
    const AoeVisibilityRegistry *registry, int32_t unit_id) {
    if (!aoe_visibility_registry_valid(registry)) return 0;
    if (unit_id >= 0) {
        if (unit_id >= registry->positive_count) return 0;
        return registry->positive[unit_id];
    }
    if (unit_id == INT32_MIN) return 0;
    int32_t index = -unit_id;
    if (index >= registry->negative_count) return 0;
    return registry->negative[index];
}

int aoe_owned_visibility_registry_init(AoeOwnedVisibilityRegistry *registry) {
    if (!registry) return 0;
    memset(registry, 0, sizeof(*registry));
    registry->view.positive = calloc(10000, sizeof(*registry->view.positive));
    registry->view.negative = calloc(1000, sizeof(*registry->view.negative));
    if (!registry->view.positive || !registry->view.negative) {
        aoe_owned_visibility_registry_destroy(registry);
        return 0;
    }
    registry->view.positive_count = 10000;
    registry->view.positive_capacity = 10000;
    registry->view.negative_count = 1000;
    registry->view.negative_capacity = 1000;
    registry->next_negative_id = -1;
    return 1;
}

void aoe_owned_visibility_registry_destroy(AoeOwnedVisibilityRegistry *registry) {
    if (!registry) return;
    free(registry->view.positive);
    free(registry->view.negative);
    memset(registry, 0, sizeof(*registry));
}

int aoe_owned_visibility_registry_next_positive_id(
    AoeOwnedVisibilityRegistry *registry, int32_t *unit_id) {
    if (!registry || !unit_id ||
        !aoe_visibility_registry_valid(&registry->view)) return 0;
    if (registry->special_id_mode) {
        *unit_id = registry->reserved_id;
        return 1;
    }
    if (registry->next_positive_id < 0) return 0;
    while (registry->next_positive_id < registry->view.positive_count &&
           registry->view.positive[registry->next_positive_id]) {
        if (registry->next_positive_id == INT32_MAX) return 0;
        registry->next_positive_id++;
    }
    if (registry->next_positive_id == INT32_MAX) return 0;
    *unit_id = registry->next_positive_id++;
    return 1;
}

int aoe_owned_visibility_registry_next_negative_id(
    AoeOwnedVisibilityRegistry *registry, int32_t *unit_id) {
    if (!registry || !unit_id ||
        !aoe_visibility_registry_valid(&registry->view)) return 0;
    if (registry->special_id_mode) {
        if (registry->reserved_id < registry->next_negative_id) {
            if (registry->reserved_id == INT32_MIN) return 0;
            registry->next_negative_id = registry->reserved_id - 1;
        }
        *unit_id = registry->reserved_id;
        return 1;
    }
    if (registry->next_negative_id == INT32_MIN) return 0;
    *unit_id = registry->next_negative_id--;
    return 1;
}

int aoe_owned_visibility_registry_clear_id_tables(
    AoeOwnedVisibilityRegistry *registry) {
    if (!registry || !aoe_visibility_registry_valid(&registry->view))
        return 0;
    if (registry->view.positive_count)
        memset(registry->view.positive, 0,
               (size_t)registry->view.positive_count *
                   sizeof(*registry->view.positive));
    if (registry->view.negative_count)
        memset(registry->view.negative, 0,
               (size_t)registry->view.negative_count *
                   sizeof(*registry->view.negative));
    registry->next_positive_id = 0;
    registry->next_negative_id = -1;
    return 1;
}

static int grow_slots(AoeVisibilityUnitRef ***slots, int32_t *count,
                      size_t *capacity, int32_t index) {
    size_t needed = (size_t)index + 1;
    size_t doubled = (size_t)*count * 2;
    size_t scaled = (size_t)((float)index * 1.1f);
    size_t next = doubled > scaled ? doubled : scaled;
    AoeVisibilityUnitRef **new_slots;
    if (next < needed) next = needed;
    if (next > INT32_MAX || next > SIZE_MAX / sizeof(*new_slots)) return 0;
    new_slots = calloc(next, sizeof(*new_slots));
    if (!new_slots) return 0;
    if (*count) memcpy(new_slots, *slots, (size_t)*count * sizeof(*new_slots));
    free(*slots);
    *slots = new_slots;
    *count = (int32_t)next;
    *capacity = next;
    return 1;
}

int aoe_owned_visibility_registry_insert(AoeOwnedVisibilityRegistry *registry,
                                          int32_t unit_id,
                                          AoeVisibilityUnitRef *unit,
                                          AoeVisibilityUnitRef **replaced,
                                          int *needs_reassign) {
    AoeVisibilityUnitRef ***slots;
    int32_t *count;
    size_t *capacity;
    uint32_t *insertions;
    int32_t index;
    if (!registry || !unit || !replaced || !needs_reassign ||
        unit_id == INT32_MIN ||
        !aoe_visibility_registry_valid(&registry->view)) return 0;
    if (unit_id < 0) {
        slots = &registry->view.negative;
        count = &registry->view.negative_count;
        capacity = &registry->view.negative_capacity;
        insertions = &registry->negative_insertions;
        index = -unit_id;
    } else {
        slots = &registry->view.positive;
        count = &registry->view.positive_count;
        capacity = &registry->view.positive_capacity;
        insertions = &registry->positive_insertions;
        index = unit_id;
    }
    if (index >= *count && !grow_slots(slots, count, capacity, index)) return 0;
    *replaced = (*slots)[index];
    *needs_reassign = unit_id >= 0 && *replaced && registry->special_id_mode;
    (*slots)[index] = unit;
    ++*insertions;
    if (*needs_reassign) registry->special_id_mode = 0;
    return 1;
}

int aoe_owned_visibility_registry_clear(AoeOwnedVisibilityRegistry *registry,
                                         int32_t unit_id) {
    int32_t index;
    if (!registry || !aoe_visibility_registry_valid(&registry->view) ||
        unit_id == INT32_MIN) return 0;
    if (unit_id < 0) {
        index = -unit_id;
        if (index >= registry->view.negative_count) return 0;
        registry->view.negative[index] = 0;
    } else {
        index = unit_id;
        if (index >= registry->view.positive_count) return 0;
        registry->view.positive[index] = 0;
    }
    return 1;
}

static int valid_watch(const AoeVisibilityWatch *watch) {
    if (!watch || watch->count < 0 || (size_t)watch->count > watch->capacity)
        return 0;
    if (watch->count && !watch->entries) return 0;
    for (int32_t i = 0; i < watch->count; ++i)
        if (!watch->entries[i].live_flags) return 0;
    return 1;
}

int aoe_visibility_watch_add(AoeVisibilityWatch *watch,
                             int16_t watch_owner_index,
                             int16_t unit_owner_index,
                             AoeVisibilityUnitRef *unit,
                             const uint32_t *live_flags, int32_t unit_id) {
    AoeVisibilityWatchEntry *grown;
    size_t capacity;
    if (!valid_watch(watch) || !unit || !live_flags ||
        watch_owner_index != unit_owner_index) return 0;
    for (int32_t i = 0; i < watch->count; ++i)
        if (watch->entries[i].unit == unit) return 0;
    if ((size_t)watch->count == watch->capacity) {
        if (watch->capacity > SIZE_MAX - 32) return 0;
        capacity = watch->capacity + 32;
        if (capacity > SIZE_MAX / sizeof(*grown)) return 0;
        grown = realloc(watch->entries, capacity * sizeof(*grown));
        if (!grown) return 0;
        memset(grown + watch->capacity, 0,
               (capacity - watch->capacity) * sizeof(*grown));
        watch->entries = grown;
        watch->capacity = capacity;
    }
    watch->entries[watch->count++] = (AoeVisibilityWatchEntry){
        unit, live_flags, *live_flags, unit_id};
    return 1;
}

int aoe_visibility_watch_remove(AoeVisibilityWatch *watch,
                                const AoeVisibilityUnitRef *unit) {
    if (!valid_watch(watch)) return 0;
    for (int32_t i = 0; i < watch->count; ++i) {
        if (watch->entries[i].unit != unit) continue;
        watch->count--;
        if (i < watch->count)
            watch->entries[i] = watch->entries[watch->count];
        memset(&watch->entries[watch->count], 0,
               sizeof(watch->entries[watch->count]));
        return 1;
    }
    return 0;
}

void aoe_visibility_watch_destroy(AoeVisibilityWatch *watch) {
    if (!watch) return;
    free(watch->entries);
    memset(watch, 0, sizeof(*watch));
}

static void notify_cleared(uint32_t old_flags, uint32_t new_flags,
                           int16_t owner_index,
                           const AoeVisibilityUnitRef *unit,
                           const AoeVisibilityWatchOps *ops) {
    uint32_t lost = ((old_flags & ~new_flags) >> 1) & 0x7fffu;
    for (int player_index = 1; lost; ++player_index, lost >>= 1)
        if ((lost & 1u) && player_index != owner_index)
            ops->notify_lost(ops->user, player_index, unit);
}

int aoe_visibility_watch_step(AoeVisibilityWatch *watch,
                              AoeVisibilityEdition edition,
                              int16_t owner_index,
                              const AoeVisibilityRegistry *registry,
                              const AoeVisibilityWatchOps *ops) {
    AoeVisibilityUnitRef *last_invalid = 0;
    if (!valid_watch(watch) || !aoe_visibility_registry_valid(registry) ||
        !ops || !ops->notify_lost ||
        (edition != AOE_VISIBILITY_BASE &&
         edition != AOE_VISIBILITY_RISE_OF_ROME)) return 0;

    for (int32_t i = 0; i < watch->count;) {
        AoeVisibilityWatchEntry *entry = &watch->entries[i];
        uint32_t current = *entry->live_flags;
        if (current != entry->previous_flags) {
            if (entry->unit &&
                aoe_visibility_registry_resolve(registry, entry->unit_id) ==
                    entry->unit) {
                notify_cleared(entry->previous_flags, current, owner_index,
                               entry->unit, ops);
                entry->previous_flags = current;
            } else if (edition == AOE_VISIBILITY_RISE_OF_ROME) {
                AoeVisibilityUnitRef *invalid = entry->unit;
                if (!aoe_visibility_watch_remove(watch, invalid)) return 0;
                continue; /* Recheck a swapped-in entry at this index. */
            } else {
                last_invalid = entry->unit;
                entry->previous_flags = current;
            }
        }
        ++i;
    }
    if (edition == AOE_VISIBILITY_BASE && last_invalid)
        return aoe_visibility_watch_remove(watch, last_invalid);
    return 1;
}
