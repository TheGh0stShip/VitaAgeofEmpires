#include "world_motion.h"

#include <stdlib.h>
#include <string.h>

#include "live_unit_vision.h"

static AoeLiveUnit *resolve(void *context, int32_t unit_id) {
    AoeWorldMotionRor *m = context;
    AoeVisibilityUnitRef *ref =
        aoe_visibility_registry_resolve(&m->runtime->world.view, unit_id);
    /* The core (first member of AoeLiveUnit) embeds the position. */
    return ref ? (AoeLiveUnit *)((char *)ref -
                                 offsetof(AoeLiveUnitCore, position))
               : NULL;
}

AoeWorldMotionUnitRor *aoe_world_motion_unit_ror(AoeWorldMotionRor *m,
                                                 AoeLiveUnit *unit) {
    int64_t slot;
    if (!m || !m->units || !unit) return NULL;
    slot = (int64_t)unit->core.unit_id + AOE_WORLD_MOTION_ID_OFFSET;
    if (slot < 0 || slot >= AOE_WORLD_MOTION_ID_SLOTS) return NULL;
    if (!m->units[slot] || m->units[slot]->unit != unit) {
        AoeWorldMotionUnitRor *s = m->units[slot];
        if (!s) {
            s = calloc(1, sizeof(*s));
            if (!s) return NULL;
            m->units[slot] = s;
        } else {
            aoe_unit_path_free_ror(&s->movement.path);
            aoe_unit_path_free_ror(&s->movement.alternate_path);
            aoe_movement_waypoint_free_ror(&s->movement);
            memset(s, 0, sizeof(*s));
        }
        s->unit = unit;
        s->movement.target_id = -1;
        s->movement.best = 100000.0f;
    }
    return m->units[slot];
}

static const float *restriction_row(const AoeWorldMotionRor *m,
                                    const AoeLiveUnit *unit,
                                    int32_t *count) {
    const DatIntro *intro = m->runtime->intro;
    int16_t r = unit && unit->core.unit_template
                    ? unit->core.unit_template->terrain_restriction
                    : -1;
    if (!intro || !intro->terrain_restrictions || r < 0 ||
        r >= intro->restriction_count) {
        *count = 0;
        return NULL;
    }
    *count = intro->terrain_count;
    return intro->terrain_restrictions + (size_t)r * intro->terrain_count;
}

static const DatGraphic *sprite_of(const AoeLiveUnit *unit) {
    return unit->graphic.current_graphic;
}

/* 0x0044ead0. */
static float unit_speed(void *context, AoeLiveUnit *unit) {
    const DatGraphic *g = sprite_of(unit);
    (void)context;
    if (!unit->core.unit_template) return 0.0f;
    if (g && g->speed_multiplier > 0.0f)
        return (float)((double)unit->core.unit_template->speed *
                       g->speed_multiplier);
    return unit->core.unit_template->speed;
}

static AoeUnitPathRor *unit_path(void *context, AoeLiveUnit *unit) {
    AoeWorldMotionUnitRor *s = aoe_world_motion_unit_ror(context, unit);
    if (!s) return NULL;
    return s->movement.use_alternate_path ? &s->movement.alternate_path
                                          : &s->movement.path;
}

static int tile(void *context, int32_t x, int32_t y, uint8_t *b5,
                uint8_t *b6) {
    AoeWorldMotionRor *m = context;
    const AoeRuntimeMap *map = &m->runtime->map;
    size_t i;
    if (x < 0 || y < 0 || (uint32_t)x >= map->width ||
        (uint32_t)y >= map->height)
        return 0;
    i = (size_t)y * map->width + (size_t)x;
    *b5 = (uint8_t)((map->tiles[i].terrain_id & 0x1f) |
                    (map->tiles[i].elevation << 5));
    *b6 = map->borders ? map->borders[i] : 0;
    return 1;
}

static int16_t overlay_terrain(void *context, uint8_t overlay) {
    AoeWorldMotionRor *m = context;
    const DatBorders *b = m->runtime->terrain_borders;
    if (!b || overlay >= b->count) return -1;
    return b->borders[overlay].underlay_terrain;
}

static int32_t lift(void *context, AoeLiveUnit *unit) {
    AoeWorldMotionRor *m = context;
    AoeWorldMotionUnitRor *s = aoe_world_motion_unit_ror(m, unit);
    int r;
    if (!s) return 0;
    r = aoe_unit_obstruction_lift_ror(unit, &s->obstruction, &m->path, NULL,
                                      0);
    return r > 0 ? r : 0;
}

static void restore(void *context, AoeLiveUnit *unit) {
    AoeWorldMotionRor *m = context;
    AoeWorldMotionUnitRor *s = aoe_world_motion_unit_ror(m, unit);
    if (s)
        (void)aoe_unit_obstruction_restore_ror(unit, &s->obstruction,
                                               &m->path, NULL, 0);
}

static int fits_cb(void *context, AoeLiveUnit *unit, float x, float y) {
    return aoe_world_motion_fits_ror(context, unit, x, y, 1) == 1;
}

static int32_t retry_count(void *context, AoeLiveUnit *unit) {
    AoeWorldMotionUnitRor *s = aoe_world_motion_unit_ror(context, unit);
    return s ? s->movement.retry_baseline : 0;
}

static uint8_t member_busy(void *context, AoeLiveUnit *unit) {
    (void)context; (void)unit;
    return 0;
}
static float member_rate(void *context, AoeLiveUnit *unit) {
    AoeWorldMotionUnitRor *s = aoe_world_motion_unit_ror(context, unit);
    return s ? s->movement.current_speed : 0.0f;
}
static float member_field_130(void *context, AoeLiveUnit *unit) {
    AoeWorldMotionUnitRor *s = aoe_world_motion_unit_ror(context, unit);
    return s ? s->movement.blocked_wait : 0.0f;
}
static uint8_t member_flag_85(void *context, AoeLiveUnit *unit) {
    AoeWorldMotionUnitRor *s = aoe_world_motion_unit_ror(context, unit);
    return s ? s->obstruction.previous : 0;
}
static int attached(void *context, AoeLiveUnit *unit) {
    (void)context;
    return unit->placement.attached_unit != NULL;
}
static void footprint_adjust(void *context, AoeLiveUnit *unit,
                             int32_t low[2], int32_t high[2]) {
    AoeWorldMotionUnitRor *s = aoe_world_motion_unit_ror(context, unit);
    if (!s) return;
    memcpy(s->footprint_low, low, sizeof(s->footprint_low));
    memcpy(s->footprint_high, high, sizeof(s->footprint_high));
}
static int32_t query(void *context, float radius, int32_t filter, int32_t x,
                     int32_t y, int32_t *count) {
    (void)context; (void)radius; (void)filter; (void)x; (void)y;
    *count = 0;
    return -1;
}
static int16_t owner_id(void *context, AoeLiveUnit *unit) {
    (void)context;
    return unit->runtime_owner_index;
}
static int32_t owner_kind(void *context, AoeLiveUnit *unit) {
    (void)context; (void)unit;
    return 0;
}

void aoe_world_motion_path_ops_ror(AoeWorldMotionRor *m, AoeLiveUnit *unit,
                                   AoePathWorldOps *o) {
    memset(o, 0, sizeof(*o));
    o->context = m;
    o->resolve = resolve;
    o->speed = unit_speed;
    o->unit_path = unit_path;
    o->terrain_costs = restriction_row(m, unit, &o->terrain_cost_count);
    o->free_terrain_a = o->free_terrain_b = -1;
    o->map_width = (int32_t)m->runtime->map.width;
    o->map_height = (int32_t)m->runtime->map.height;
    o->tile = tile;
    o->overlay_terrain = overlay_terrain;
    o->lift_obstruction = lift;
    o->restore_obstruction = restore;
    o->centered = fits_cb;
    o->retry_count = retry_count;
    o->member_busy = member_busy;
    o->member_rate = member_rate;
    o->member_field_130 = member_field_130;
    o->member_flag_85 = member_flag_85;
    o->attached = attached;
    o->footprint_adjust = footprint_adjust;
    o->query = query;
    o->owner_id = owner_id;
    o->owner_kind = owner_kind;
}

int aoe_world_motion_fits_ror(AoeWorldMotionRor *m, AoeLiveUnit *unit,
                              float x, float y, int check_terrain) {
    AoePathWorldOps o;
    if (!m || !unit) return -1;
    aoe_world_motion_path_ops_ror(m, unit, &o);
    if (!o.terrain_costs) return -1;
    return aoe_path_fits_ror(&m->path, &o, unit, x, y, check_terrain);
}

/* Movement callbacks. */
static void order_flag(void *context, AoeLiveUnit *unit, int32_t value) {
    (void)context; (void)unit; (void)value;
}
static int32_t find(void *context, const AoePathRequestRor *request) {
    AoeWorldMotionRor *m = context;
    AoePathWorldOps o;
    int32_t returned = 0;
    aoe_world_motion_path_ops_ror(m, request->unit, &o);
    if (!o.terrain_costs) return 0;
    if (aoe_path_find_ror(&m->path, m->distance_table, request, &o,
                          &returned, NULL) == AOE_PATH_RESULT_ERROR_ROR)
        return 0;
    return returned;
}
static void count_search(void *context) {
    ++((AoeWorldMotionRor *)context)->searches;
}
static void count_repath(void *context) {
    ++((AoeWorldMotionRor *)context)->repaths;
}
static void place(void *context, AoeLiveUnit *unit, float x, float y,
                  float z) {
    AoeWorldMotionRor *m = context;
    AoeScenarioRuntimeRor *r = m->runtime;
    int16_t owner = unit->runtime_owner_index;
    AoeWorldMotionUnitRor *s = aoe_world_motion_unit_ror(m, unit);
    int restore_after = 0;
    if (owner < 0 || owner >= AOE_SCENARIO_RUNTIME_PLAYERS) return;
    /* The footprint counts follow the unit: lift, move, restore. */
    if (s && s->obstruction.placed == 1) {
        (void)aoe_unit_obstruction_lift_ror(unit, &s->obstruction, &m->path,
                                            NULL, 0);
        restore_after = 1;
    }
    if (aoe_live_unit_place_ror_with_vision(
            &unit->core, &unit->placement, r->map.tiles, r->map.objects,
            r->geometry, r->map.width, r->map.height, x, y, z,
            &r->players[owner].vision_binding))
        ++m->placements;
    if (restore_after)
        (void)aoe_unit_obstruction_restore_ror(unit, &s->obstruction,
                                               &m->path, NULL, 0);
}
static void stop(void *context, AoeLiveUnit *unit) {
    AoeWorldMotionUnitRor *s = aoe_world_motion_unit_ror(context, unit);
    if (s) aoe_movement_waypoint_clear_ror(&s->movement, 0, NULL, NULL);
}
static void pop_waypoint(void *context, AoeLiveUnit *unit) {
    AoeWorldMotionUnitRor *s = aoe_world_motion_unit_ror(context, unit);
    if (s) aoe_movement_waypoint_remove_ror(&s->movement, 0);
}
static uint8_t zone(void *context, AoeLiveUnit *unit, int32_t x, int32_t y) {
    (void)context; (void)unit; (void)x; (void)y;
    return 0;
}
static const AoeMovementSpriteRor *sprite(void *context, AoeLiveUnit *unit) {
    static _Thread_local AoeMovementSpriteRor info;
    const DatGraphic *g = sprite_of(unit);
    (void)context;
    if (!g) return NULL;
    info.speed_multiplier = g->speed_multiplier;
    info.facets = (int16_t)g->angle_count;
    info.flags = g->sequence_type;
    return &info;
}
static float tick_seconds(void *context) {
    return ((AoeWorldMotionRor *)context)->tick;
}
static int target_position(void *context, int32_t id, float *x, float *y) {
    AoeLiveUnit *t = resolve(context, id);
    if (!t) return 0;
    *x = t->core.position.x38;
    *y = t->core.position.y3c;
    return 1;
}
/* 0x004aa880 on the unit's tile terrain. */
static int terrain_multiplier(void *context, AoeLiveUnit *unit, float *out) {
    AoeWorldMotionRor *m = context;
    const AoeRuntimeMap *map = &m->runtime->map;
    int32_t count, x, y;
    const float *row = restriction_row(m, unit, &count);
    uint8_t terrain;
    if (!row || !unit->placement.tile) return 0;
    x = (int32_t)unit->core.position.x38;
    y = (int32_t)unit->core.position.y3c;
    if (x < 0 || y < 0 || (uint32_t)x >= map->width ||
        (uint32_t)y >= map->height)
        return 0;
    terrain = map->tiles[(size_t)y * map->width + (size_t)x].terrain_id &
              0x1f;
    if (terrain >= count) return 0;
    *out = row[terrain];
    return 1;
}
static int32_t crt_rand(void *context) {
    return aoe_rng_next(&((AoeWorldMotionRor *)context)->runtime->rng);
}

void aoe_world_motion_tick_ops_ror(AoeWorldMotionRor *m,
                                   AoeMovementTickOps *o) {
    memset(o, 0, sizeof(*o));
    o->step.base.context = m;
    o->step.base.order_flag = order_flag;
    o->step.base.owner_kind = owner_kind;
    o->step.base.find = find;
    o->step.base.count_search = count_search;
    o->step.base.fits = fits_cb;
    o->step.base.place = place;
    o->step.stop = stop;
    o->step.pop_waypoint = pop_waypoint;
    o->step.zone = zone;
    o->step.count_repath = count_repath;
    o->sprite = sprite;
    o->tick_seconds = tick_seconds;
    o->target_position = target_position;
    o->terrain_multiplier = terrain_multiplier;
    o->rand = crt_rand;
}

static int restore_all(AoeWorldMotionRor *m) {
    AoeScenarioRuntimeRor *r = m->runtime;
    for (int p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p)
        for (uint32_t i = 0; i < r->players[p].unit_count; ++i) {
            AoeLiveUnit *u = &r->players[p].units[i];
            AoeWorldMotionUnitRor *s;
            if (!u->constructed || u->core.state > 2 ||
                !u->core.unit_template)
                continue;
            s = aoe_world_motion_unit_ror(m, u);
            if (!s) return 0;
            if (aoe_unit_obstruction_restore_ror(u, &s->obstruction, &m->path,
                                                 NULL, 0) < 0)
                return 0;
        }
    return 1;
}

int aoe_world_motion_init_ror(AoeWorldMotionRor *m,
                              AoeScenarioRuntimeRor *runtime) {
    if (!m || !runtime || !runtime->initialized ||
        runtime->map.width > AOE_PATH_GRID_SIDE_ROR ||
        runtime->map.height > AOE_PATH_GRID_SIDE_ROR)
        return 0;
    memset(m, 0, sizeof(*m));
    m->runtime = runtime;
    aoe_path_distance_table_ror(m->distance_table);
    m->path.state = malloc(AOE_PATH_GRID_CELLS_ROR);
    m->path.cost = calloc(AOE_PATH_GRID_CELLS_ROR, sizeof(int32_t));
    m->path.heap_capacity = AOE_PATH_GRID_CELLS_ROR + 1;
    m->path.heap = calloc(m->path.heap_capacity, sizeof(*m->path.heap));
    m->path.occupancy = calloc(1, AOE_PATH_OCCUPANCY_BYTES_ROR +
                                      AOE_PATH_OCCUPANCY_PAD_ROR);
    m->path.penalty = calloc(1, AOE_PATH_GRID_CELLS_ROR);
    m->units = calloc(AOE_WORLD_MOTION_ID_SLOTS, sizeof(*m->units));
    if (!m->path.state || !m->path.cost || !m->path.heap ||
        !m->path.occupancy || !m->path.penalty || !m->units)
        goto fail;
    memset(m->path.state, 0xff, AOE_PATH_GRID_CELLS_ROR);
    aoe_path_set_map_ror(&m->path, 1, (int32_t)runtime->map.width,
                         (int32_t)runtime->map.height, 0, 0);
    m->path.target_id = -1;
    for (int p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p)
        m->budgets[p] = (AoePathBudgetRor){-1, -1, 0};
    if (!restore_all(m)) goto fail;
    return 1;
fail:
    aoe_world_motion_destroy_ror(m);
    return 0;
}

void aoe_world_motion_release_ror(AoeWorldMotionRor *m,
                                  const AoeLiveUnit *unit) {
    int64_t slot;
    AoeWorldMotionUnitRor *s;
    if (!m || !m->units || !unit) return;
    slot = (int64_t)unit->core.unit_id + AOE_WORLD_MOTION_ID_OFFSET;
    if (slot < 0 || slot >= AOE_WORLD_MOTION_ID_SLOTS) return;
    s = m->units[slot];
    if (!s || s->unit != unit) return;
    aoe_unit_path_free_ror(&s->movement.path);
    aoe_unit_path_free_ror(&s->movement.alternate_path);
    aoe_movement_waypoint_free_ror(&s->movement);
    free(s);
    m->units[slot] = NULL;
}

void aoe_world_motion_destroy_ror(AoeWorldMotionRor *m) {
    if (!m) return;
    if (m->units) {
        for (size_t i = 0; i < AOE_WORLD_MOTION_ID_SLOTS; ++i) {
            AoeWorldMotionUnitRor *s = m->units[i];
            if (!s) continue;
            aoe_unit_path_free_ror(&s->movement.path);
            aoe_unit_path_free_ror(&s->movement.alternate_path);
            aoe_movement_waypoint_free_ror(&s->movement);
            free(s);
        }
        free(m->units);
    }
    free(m->path.state);
    free(m->path.cost);
    free(m->path.heap);
    free(m->path.occupancy);
    free(m->path.penalty);
    free(m->path.nearby_ids);
    memset(m, 0, sizeof(*m));
}

static AoePathBudgetRor *budget_of(AoeWorldMotionRor *m, AoeLiveUnit *u) {
    int16_t owner = u->runtime_owner_index;
    if (owner < 0 || owner >= AOE_SCENARIO_RUNTIME_PLAYERS) return NULL;
    return &m->budgets[owner];
}

int aoe_world_motion_move_to_ror(AoeWorldMotionRor *m, AoeLiveUnit *unit,
                                 float x, float y, float range,
                                 int32_t target_id) {
    AoeWorldMotionUnitRor *s;
    AoeMovementTickOps o;
    AoePathBudgetRor *budget;
    if (!m || !unit || !(s = aoe_world_motion_unit_ror(m, unit)) ||
        !(budget = budget_of(m, unit)))
        return -1;
    aoe_movement_set_destination_ror(&s->movement, x, y, 1.0f);
    aoe_movement_set_range_ror(&s->movement, range);
    aoe_movement_set_target_id_ror(&s->movement, target_id);
    {
        AoeLiveUnit *t = target_id != -1 ? resolve(m, target_id) : NULL;
        if (t && t->core.unit_template)
            aoe_movement_set_target_size_ror(&s->movement,
                                             t->core.unit_template->size[0],
                                             t->core.unit_template->size[1]);
        else
            aoe_movement_set_target_size_ror(&s->movement, 0.0f, 0.0f);
    }
    aoe_world_motion_tick_ops_ror(m, &o);
    return aoe_movement_start_ror(unit, &s->movement, budget, &o.step.base);
}

int aoe_world_motion_tick_ror(AoeWorldMotionRor *m, AoeLiveUnit *unit,
                              float tick) {
    AoeWorldMotionUnitRor *s;
    AoeMovementTickOps o;
    AoePathBudgetRor *budget;
    int r;
    if (!m || !unit || !(s = aoe_world_motion_unit_ror(m, unit)) ||
        !(budget = budget_of(m, unit)))
        return -1;
    m->tick = tick;
    aoe_world_motion_tick_ops_ror(m, &o);
    r = aoe_movement_tick_ror(unit, &s->movement, budget, &o);
    /* Live +0x88 is both the movement speed and the sprite speed. */
    unit->animation_rate = s->movement.current_speed;
    return r;
}
