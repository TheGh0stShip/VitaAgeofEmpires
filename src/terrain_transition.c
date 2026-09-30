#include "terrain_transition.h"

static int terrain_contains(const DatTerrain *terrain, int16_t id) {
    for (int i = (int)terrain->terrain_units_used_count - 1; i >= 0; --i)
        if (terrain->terrain_unit_ids[i] == id) return 1;
    return 0;
}

int aoe_terrain_transition_cell_ror(
    AoeRuntimeMap *map, const DatTerrains *terrains, AoeRng *rng,
    uint32_t x, uint32_t y, int16_t filter_terrain,
    int16_t min_template, int16_t max_template,
    const AoeTerrainTransitionOps *ops) {
    const DatTerrain *current;
    AoeObjectUpdateNode *node;
    float spawn_x = 0.0f, spawn_y = 0.0f;
    size_t index;
    if (!map || !map->tiles || !map->objects || !terrains ||
        !terrains->terrains || !rng || !ops || !ops->object_owner_index ||
        !ops->object_template_id || !ops->remove_object || !ops->blocked ||
        !ops->create || x >= map->width || y >= map->height)
        return 0;
    index = (size_t)y * map->width + x;
    if (map->tiles[index].terrain_id >= terrains->count) return 0;
    current = &terrains->terrains[map->tiles[index].terrain_id];
    if (current->terrain_units_used_count > 30) return 0;

    if (current->terrain_units_used_count > 0) {
        const DatTerrain *filter = NULL;
        if (filter_terrain >= 0) {
            if ((uint16_t)filter_terrain >= terrains->count) return 0;
            filter = &terrains->terrains[filter_terrain];
            if (filter->terrain_units_used_count > 30) return 0;
        }
        for (node = map->objects[index].head; node;) {
            AoeObjectUpdateNode *next = node->next;
            void *object = node->object;
            if (object) {
                int16_t id = ops->object_template_id(ops->context, object);
                if (ops->object_owner_index(ops->context, object) ==
                        ops->terrain_owner_index &&
                    (id < min_template || max_template < id) &&
                    (!filter || terrain_contains(filter, id)))
                    ops->remove_object(ops->context, object);
            }
            node = next;
        }
    }

    for (uint16_t i = 0; i < current->terrain_units_used_count; ++i) {
        int16_t unit_id = current->terrain_unit_ids[i];
        int32_t remaining = current->terrain_unit_density[i];
        uint8_t placement = current->terrain_placement_flags[i];
        if (unit_id < 0 || remaining <= 0) continue;
        do {
            if (placement == 0) {
                spawn_x = (float)x + (float)aoe_rng_next(rng) / 32768.0f;
                spawn_y = (float)y + (float)aoe_rng_next(rng) / 32768.0f;
            } else if (placement == 1) {
                spawn_x = (float)x + 0.5f;
                spawn_y = (float)y + 0.5f;
            }
            if (remaining >= 1000 ||
                ((uint32_t)aoe_rng_next(rng) * 1000u) / 32767u <
                    (uint32_t)remaining) {
                if (!ops->blocked(ops->context, unit_id, spawn_x, spawn_y))
                    ops->create(ops->context, unit_id, spawn_x, spawn_y);
            }
            remaining -= 1000;
        } while (remaining > 0);
    }
    return 1;
}
