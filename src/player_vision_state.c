#include "player_vision_state.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int aoe_explored_cells_init(AoeExploredCellVector *vector) {
    if (!vector) return 0;
    memset(vector, 0, sizeof(*vector));
    vector->cells = calloc(400, sizeof(*vector->cells));
    if (!vector->cells) return 0;
    vector->capacity = 400;
    return 1;
}

int aoe_explored_cells_append(AoeExploredCellVector *vector,
                              int32_t x, int32_t y) {
    if (!vector || !vector->cells || vector->allocation_failed ||
        vector->count > vector->capacity) return 0;
    if (vector->count == vector->capacity) {
        size_t next = vector->capacity * 2;
        AoeExploredCell *cells;
        if (next <= vector->capacity ||
            next > SIZE_MAX / sizeof(*cells)) {
            vector->allocation_failed = 1;
            return 0;
        }
        cells = malloc(next * sizeof(*cells));
        if (!cells) {
            vector->allocation_failed = 1;
            return 0;
        }
        memcpy(cells, vector->cells,
               vector->capacity * sizeof(*cells));
        free(vector->cells);
        vector->cells = cells;
        vector->capacity = next;
    }
    vector->cells[vector->count++] = (AoeExploredCell){x, y};
    vector->total_appends++;
    return 1;
}

void aoe_explored_cells_reset(AoeExploredCellVector *vector) {
    if (!vector) return;
    vector->count = 0;
    vector->total_appends = 0;
    vector->allocation_failed = 0;
}

void aoe_explored_cells_destroy(AoeExploredCellVector *vector) {
    if (!vector) return;
    free(vector->cells);
    memset(vector, 0, sizeof(*vector));
}

void aoe_explored_cells_record(void *context, int32_t x, int32_t y) {
    AoeExploredCellVector *vector = context;
    if (vector && !aoe_explored_cells_append(vector, x, y))
        vector->allocation_failed = 1;
}

int aoe_visibility_mask_map_init(AoeVisibilityMaskMap *map,
                                 uint32_t width, uint32_t height) {
    if (!map || !width || !height || width > INT32_MAX || height > 256 ||
        (size_t)width > SIZE_MAX / height ||
        (size_t)width * height > SIZE_MAX / sizeof(*map->masks)) return 0;
    memset(map, 0, sizeof(*map));
    map->masks = calloc((size_t)width * height, sizeof(*map->masks));
    if (!map->masks) return 0;
    map->width = width;
    map->height = height;
    return 1;
}

void aoe_visibility_mask_map_destroy(AoeVisibilityMaskMap *map) {
    if (!map) return;
    free(map->masks);
    memset(map, 0, sizeof(*map));
}

int aoe_player_vision_state_init(AoePlayerVisionState *state,
                                 AoeVisibilityMaskMap *shared,
                                 uint8_t owner_index) {
    size_t cells;
    uint32_t owner_bit;
    if (!state || !shared || !shared->masks || !shared->width ||
        !shared->height || owner_index > 15) return 0;
    memset(state, 0, sizeof(*state));
    cells = (size_t)shared->width * shared->height;
    state->owned_counts = malloc(cells);
    if (!state->owned_counts) return 0;
    memset(state->owned_counts, 0xff, cells);
    if (!aoe_explored_cells_init(&state->owner_cells58) ||
        !aoe_explored_cells_init(&state->active_cells6c)) {
        aoe_player_vision_state_destroy(state);
        return 0;
    }
    owner_bit = 1u << owner_index;
    state->grid.width = shared->width;
    state->grid.height = shared->height;
    state->grid.counts = state->owned_counts;
    state->grid.tile_masks = shared->masks;
    state->grid.visible_mask = owner_bit;
    state->grid.clear_visible_mask = ~owner_bit;
    state->grid.explored_mask = owner_bit << 16;
    return 1;
}

void aoe_player_vision_state_destroy(AoePlayerVisionState *state) {
    if (!state) return;
    aoe_explored_cells_destroy(&state->owner_cells58);
    aoe_explored_cells_destroy(&state->active_cells6c);
    free(state->owned_counts);
    memset(state, 0, sizeof(*state));
}
