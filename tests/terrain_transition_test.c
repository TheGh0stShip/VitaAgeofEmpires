#include "terrain_transition.h"

#include <assert.h>

typedef struct { int16_t owner, id; } Object;
typedef struct {
    AoeObjectUpdateList *list;
    unsigned removed, blocked_calls, created;
    int16_t created_ids[3];
    float created_x[3], created_y[3];
} Trace;

static int16_t object_owner(void *context, const void *object) {
    (void)context;
    return ((const Object *)object)->owner;
}
static int16_t object_id(void *context, const void *object) {
    (void)context;
    return ((const Object *)object)->id;
}
static void remove_object(void *context, void *object) {
    Trace *trace = context;
    assert(aoe_object_update_list_detach(trace->list, object, NULL));
    ++trace->removed;
}
static int blocked(void *context, int16_t id, float x, float y) {
    Trace *trace = context;
    (void)id; (void)x; (void)y;
    ++trace->blocked_calls;
    return 0;
}
static void create(void *context, int16_t id, float x, float y) {
    Trace *trace = context;
    assert(trace->created < 3);
    trace->created_ids[trace->created] = id;
    trace->created_x[trace->created] = x;
    trace->created_y[trace->created] = y;
    ++trace->created;
}

int main(void) {
    ScnTile tiles[4] = {{0}};
    AoeObjectUpdateList objects[4] = {{0}};
    AoeRuntimeMap map = {
        .width = 2, .height = 2, .tiles = tiles, .objects = objects
    };
    DatTerrain terrain[2] = {{0}};
    DatTerrains terrains = {.count = 2, .terrains = terrain};
    AoeRng rng = {1};
    Object keep_owner = {1, 10}, keep_id = {0, 11}, remove = {0, 10};
    AoeObjectUpdateNode *attached;
    Trace trace = {.list = &objects[3]};
    AoeTerrainTransitionOps ops = {
        &trace, 0, object_owner, object_id, remove_object, blocked, create
    };
    tiles[3].terrain_id = 1;
    terrain[0].terrain_units_used_count = 1;
    terrain[0].terrain_unit_ids[0] = 10;
    terrain[1].terrain_units_used_count = 2;
    terrain[1].terrain_unit_ids[0] = 20;
    terrain[1].terrain_unit_density[0] = 2000;
    terrain[1].terrain_placement_flags[0] = 1;
    terrain[1].terrain_unit_ids[1] = 21;
    terrain[1].terrain_unit_density[1] = 500;
    terrain[1].terrain_placement_flags[1] = 0;
    assert(aoe_object_update_list_attach(&objects[3], &keep_owner, &attached));
    assert(aoe_object_update_list_attach(&objects[3], &keep_id, &attached));
    assert(aoe_object_update_list_attach(&objects[3], &remove, &attached));
    assert(aoe_terrain_transition_cell_ror(
        &map, &terrains, &rng, 1, 1, 0, -1, -1, &ops));
    assert(trace.removed == 1 && objects[3].count == 2);
    assert(trace.blocked_calls == 3 && trace.created == 3);
    assert(trace.created_ids[0] == 20 && trace.created_ids[1] == 20 &&
           trace.created_ids[2] == 21);
    assert(trace.created_x[0] == 1.5f && trace.created_y[0] == 1.5f &&
           trace.created_x[1] == 1.5f && trace.created_y[1] == 1.5f);
    assert(trace.created_x[2] == 1.0f + 41.0f / 32768.0f &&
           trace.created_y[2] == 1.0f + 18467.0f / 32768.0f &&
           rng.state == 415139642u);
    aoe_object_update_list_clear(&objects[3]);
    return 0;
}
