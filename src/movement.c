#include "movement.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"

int aoe_path_budget_allows_ror(const AoePathBudgetRor *b, int32_t retry) {
    if (!b) return 0;
    if (b->retry_override != -1 && b->retry_override < retry) return 1;
    if (b->max_per_tick == -1) return 1;
    return b->used < b->max_per_tick;
}

void aoe_path_budget_consume_ror(AoePathBudgetRor *b) {
    if (b) ++b->used;
}

void aoe_movement_set_destination_ror(AoeMovementRor *m, float x, float y,
                                      float z) {
    if (!m) return;
    m->dest_x = x;
    m->dest_y = y;
    m->dest_z = z;
}

void aoe_movement_set_range_ror(AoeMovementRor *m, float range) {
    if (m) m->range = range;
}

void aoe_movement_set_target_id_ror(AoeMovementRor *m, int32_t id) {
    if (m) m->target_id = id;
}

void aoe_movement_set_target_size_ror(AoeMovementRor *m, float x, float y) {
    if (!m) return;
    m->target_size_x = x;
    m->target_size_y = y;
}

void aoe_movement_mark_waypoints_stale_ror(AoeMovementRor *m) {
    if (m) m->waypoints_stale = 1;
}

const uint8_t *aoe_movement_waypoint_get_ror(const AoeMovementRor *m,
                                             int32_t index) {
    if (!m || !m->waypoint_tile || index < 0 || index >= m->waypoint_count)
        return NULL;
    return m->waypoint_tile + 3 * index;
}

void aoe_movement_waypoint_clear_ror(AoeMovementRor *m, int32_t arg,
                                     void (*on_clear)(void *context),
                                     void *context) {
    if (!m) return;
    m->waypoint_count = 0;
    if (arg == 1 && on_clear) on_clear(context);
}

int aoe_movement_waypoint_append_ror(AoeMovementRor *m,
                                     const uint8_t entry[3],
                                     int32_t clear_arg,
                                     void (*on_clear)(void *context),
                                     void *context) {
    uint8_t *grown;
    int32_t i;
    if (!m || !entry) return 0;
    if (m->waypoints_stale == 1) {
        aoe_movement_waypoint_clear_ror(m, clear_arg, on_clear, context);
        m->waypoints_stale = 0;
    }
    if (m->waypoint_capacity <= m->waypoint_count) {
        if (m->waypoint_capacity == 0) {
            /* Capacity 0 means no owned storage; like the original, any
               existing pointer is replaced, not freed. */
            grown = malloc(15);
            if (!grown) return 0;
            m->waypoint_capacity = 5;
            m->waypoint_count = 0;
        } else {
            if (m->waypoint_capacity > INT32_MAX / 6) return 0;
            grown = malloc((size_t)m->waypoint_capacity * 6);
            if (!grown) return 0;
            for (i = 0; i < m->waypoint_count; ++i)
                memcpy(grown + 3 * i, m->waypoint_tile + 3 * i, 3);
            m->waypoint_capacity *= 2;
            for (; i < m->waypoint_capacity; ++i) memset(grown + 3 * i, 0, 3);
            free(m->waypoint_tile);
        }
        m->waypoint_tile = grown;
    }
    memcpy(m->waypoint_tile + 3 * m->waypoint_count, entry, 3);
    ++m->waypoint_count;
    return 1;
}

void aoe_movement_waypoint_remove_ror(AoeMovementRor *m, int32_t index) {
    int32_t j;
    if (!m || index < 0 || index >= m->waypoint_count) return;
    /* The original first walks j up to min(index, count), i.e. index. */
    for (j = index; j < m->waypoint_count; ++j)
        if (j) memcpy(m->waypoint_tile + 3 * (j - 1),
                      m->waypoint_tile + 3 * j, 3);
    --m->waypoint_count;
}

int aoe_movement_record_waypoint_ror(AoeMovementRor *m,
                                     const uint8_t tile[3]) {
    if (!m || !tile) return 0;
    if (m->waypoints_stale) return 1;
    if (!aoe_movement_waypoint_append_ror(m, tile, 0, NULL, NULL)) return 0;
    aoe_movement_mark_waypoints_stale_ror(m);
    return 1;
}

void aoe_movement_waypoint_free_ror(AoeMovementRor *m) {
    if (!m) return;
    free(m->waypoint_tile);
    m->waypoint_tile = NULL;
    m->waypoint_count = m->waypoint_capacity = 0;
}

void aoe_unit_path_rewind_ror(AoeUnitPathRor *path) {
    if (path) path->cursor = 0;
}

int aoe_unit_path_advance_ror(AoeUnitPathRor *path) {
    int32_t last;
    if (!path) return 0;
    last = path->count - 1;
    if (path->cursor < last) {
        ++path->cursor;
        return 1;
    }
    path->cursor = last;
    return 0;
}

const AoePathWaypointRor *aoe_unit_path_current_ror(const AoeUnitPathRor *path) {
    if (!path || !path->points || path->cursor < 0 ||
        path->cursor >= path->count)
        return NULL;
    return &path->points[path->cursor];
}

static int off_centre(float value) {
    double frac = (double)value - floor((double)value);
    /* fcomp 0.5; test ah,0x40: equal or unordered counts as centred. */
    return !(isnan(frac) || frac == 0.5);
}

int aoe_movement_start_ror(AoeLiveUnit *unit, AoeMovementRor *m,
                           AoePathBudgetRor *budget,
                           const AoeMovementOps *ops) {
    const AoeVisibilityUnitRef *pos;
    AoePathRequestRor req;
    AoePathWaypointRor wp = {0};
    int32_t returned;
    if (!unit || !unit->core.unit_template || !m || !budget || !ops ||
        !ops->order_flag || !ops->owner_kind || !ops->find ||
        !ops->count_search || !ops->fits || !ops->place)
        return -1;
    pos = &unit->core.position;
    if (!(unit->core.unit_template->size[2] > 0.0f)) {
        ops->order_flag(ops->context, unit, 0);
        m->start_x = pos->x38;
        m->start_y = pos->y3c;
        m->start_z = pos->z40;
        unit->group_unit_count = 0;
        aoe_unit_path_clear_ror(&m->path);
        wp.x = m->dest_x; wp.y = m->dest_y; wp.z = m->dest_z;
        wp.direction = m->dest_direction;
        if (!aoe_unit_path_prepend_ror(&m->path, &wp)) return -1;
        wp.x = m->start_x; wp.y = m->start_y; wp.z = m->start_z;
        wp.direction = m->start_direction;
        if (!aoe_unit_path_prepend_ror(&m->path, &wp)) return -1;
        aoe_unit_path_rewind_ror(&m->path);
        (void)aoe_unit_path_advance_ror(&m->path);
        m->best = 100000.0f;
        return 0;
    }
    if (!aoe_path_budget_allows_ror(budget, m->retry)) {
        ++m->retry;
        return 2;
    }
    aoe_path_budget_consume_ror(budget);
    m->retry = 0;
    ops->order_flag(ops->context, unit, 0);
    m->retry_baseline = m->waypoint_count +
                        (ops->owner_kind(ops->context, unit) == 0 ? 25 : 15);
    m->start_x = pos->x38;
    m->start_y = pos->y3c;
    m->start_z = pos->z40;
    m->best = 100000.0f;
    ops->count_search(ops->context);
    req.x1 = aoe_ftol32(m->start_x);
    req.y1 = aoe_ftol32(m->start_y);
    if (m->waypoint_count > 0) {
        if (!m->waypoint_tile) return -1;
        req.x2 = m->waypoint_tile[0];
        req.y2 = m->waypoint_tile[1];
    } else {
        req.x2 = aoe_ftol32(m->dest_x);
        req.y2 = aoe_ftol32(m->dest_y);
    }
    req.unit = unit;
    req.range = m->range;
    req.target_id = m->target_id;
    req.mode = 1;
    req.budget_limited = 1;
    req.argument_30 = 1;
    req.check_occupancy = 1;
    req.step = 1;
    {
        uint32_t bits = UINT32_MAX; /* pushed -1 as the float argument */
        float radius;
        memcpy(&radius, &bits, sizeof(radius));
        req.query_radius = radius;
    }
    req.query_filter = -1;
    req.budget_base = 2500;
    returned = ops->find(ops->context, &req);
    if (returned == 0 && m->path.count == 1) {
        /* floor() + 0.5f in x87 then fstp to float. */
        float cx = (float)(floor((double)pos->x38) + 0.5);
        float cy = (float)(floor((double)pos->y3c) + 0.5);
        if (ops->fits(ops->context, unit, cx, cy) &&
            (off_centre(pos->x38) || off_centre(pos->y3c))) {
            ops->place(ops->context, unit, cx, cy, pos->z40);
            return 2;
        }
    }
    aoe_unit_path_rewind_ror(&m->path);
    (void)aoe_unit_path_advance_ror(&m->path);
    return 0;
}

/* 0x0044c2d0 stop tail at 0x0044c9e5 (with +0x88 cleared first at
   0x0044c5f4). */
static int stop_moving(AoeLiveUnit *unit, AoeMovementRor *m,
                       const AoeMovementStepOps *ops, int clear_speed) {
    if (clear_speed) m->current_speed = 0.0f;
    m->status = 1;
    ops->stop(ops->base.context, unit);
    unit->group_unit_count = 0;
    return 2;
}

/* Tails at 0x0044c87d/0x0044cd70/0x0044cd9e. */
static int give_up(AoeMovementRor *m) {
    m->current_speed = 0.0f;
    m->status = 2;
    if (m->waypoint_count <= 1) return 0;
    m->retry_baseline += 5;
    return 1;
}

static double path_distance(const AoeVisibilityUnitRef *pos,
                            const AoePathWaypointRor *wp) {
    double dx = (float)((double)wp->x - pos->x38);
    double dy = (double)wp->y - pos->y3c;
    return sqrt(dy * dy + dx * dx);
}

/* Loops at 0x0044c61f and 0x0044ca5f: drop intermediate waypoints already
   within reach. Returns 0 when none remain. */
static int drop_reached(AoeLiveUnit *unit, AoeMovementRor *m, float radius,
                        const AoeMovementStepOps *ops) {
    const AoeVisibilityUnitRef *pos = &unit->core.position;
    for (;;) {
        double dx, dy, dist, reach;
        float dyf;
        ops->pop_waypoint(ops->base.context, unit);
        if (m->waypoint_count <= 0) return 0;
        if (!m->waypoint_tile) return -1;
        dx = (double)m->waypoint_tile[0] - pos->x38;
        dy = (double)m->waypoint_tile[1] - pos->y3c;
        dyf = (float)dy;
        dist = sqrt(dy * dyf + dx * dx);
        reach = (double)radius + m->range + (double)0.3f;
        if (reach < dist || isnan(reach) || isnan(dist)) return 1;
    }
}

/* Re-path at 0x0044c70c / 0x0044cb04 after the budget passed. `variant`
   1 fails when the snap test does not fit; variant 2 fails only when the
   unit is already centred. Returns 1 to continue, 0 to give up, -1 on
   error. */
static int repath(AoeLiveUnit *unit, AoeMovementRor *m,
                  AoePathBudgetRor *budget, const AoeMovementStepOps *ops,
                  int variant) {
    const AoeVisibilityUnitRef *pos = &unit->core.position;
    AoePathRequestRor req;
    uint32_t bits = UINT32_MAX;
    int32_t returned;
    aoe_path_budget_consume_ror(budget);
    m->start_x = pos->x38;
    m->start_y = pos->y3c;
    m->start_z = pos->z40;
    m->retry = 0;
    ops->count_repath(ops->base.context);
    req.x1 = aoe_ftol32(m->start_x);
    req.y1 = aoe_ftol32(m->start_y);
    if (m->waypoint_count > 0) {
        if (!m->waypoint_tile) return -1;
        req.x2 = m->waypoint_tile[0];
        req.y2 = m->waypoint_tile[1];
    } else {
        req.x2 = aoe_ftol32(m->dest_x);
        req.y2 = aoe_ftol32(m->dest_y);
    }
    req.unit = unit;
    req.range = m->range;
    req.target_id = m->target_id;
    req.mode = 1;
    req.budget_limited = 1;
    req.argument_30 = 1;
    req.check_occupancy = 1;
    req.step = 1;
    memcpy(&req.query_radius, &bits, sizeof(bits));
    req.query_filter = -1;
    req.budget_base = 2500;
    returned = ops->base.find(ops->base.context, &req);
    unit->group_unit_count = 0;
    if (returned == 0 && m->path.count == 1) {
        float cx = (float)(floor((double)pos->x38) + 0.5);
        float cy = (float)(floor((double)pos->y3c) + 0.5);
        int fits = ops->base.fits(ops->base.context, unit, cx, cy);
        int off = off_centre(pos->x38) || off_centre(pos->y3c);
        if (fits && off)
            ops->base.place(ops->base.context, unit, cx, cy, pos->z40);
        else if (variant == 1 || fits)
            return 0;
    }
    aoe_unit_path_rewind_ror(&m->path);
    return 1;
}

typedef int32_t (*StepFn)(void *context, AoeLiveUnit *unit, float x,
                          float y);

static int update_impl(AoeLiveUnit *unit, AoeMovementRor *m,
                       AoePathBudgetRor *budget,
                       const AoeMovementStepOps *ops, StepFn step,
                       void *step_context) {
    const AoeVisibilityUnitRef *pos;
    const DatUnit *tmpl;
    const AoePathWaypointRor *wp;
    float dxf, ex, dist, radius, bonus = 0.0f;
    double dy, ey;
    int advanced = 1, r;
    if (!unit || !unit->core.unit_template || !m || !budget || !ops ||
        !ops->base.order_flag || !ops->base.find || !ops->base.fits ||
        !ops->base.place || !ops->stop || !ops->pop_waypoint || !ops->zone ||
        !ops->count_repath || !step)
        return -1;
    pos = &unit->core.position;
    tmpl = unit->core.unit_template;
    ops->base.order_flag(ops->base.context, unit, 0);

    /* Footprint-adjusted distance to the destination. */
    dxf = (float)((double)m->dest_x - pos->x38);
    dy = (double)m->dest_y - pos->y3c;
    if (dxf < 0.0f || isnan(dxf)) dxf = -dxf;
    if (dy < 0.0 || isnan(dy)) dy = -dy;
    ex = (float)((double)dxf - ((double)tmpl->size[0] + m->target_size_x));
    ey = dy - ((double)tmpl->size[1] + m->target_size_y);
    if (ex < 0.0f || isnan(ex)) ex = 0.0f;
    if (ey < 0.0 || isnan(ey)) ey = 0.0;
    dist = (float)sqrt(ey * ey + (double)ex * ex);

    if (m->range > 0.0f && !(dist > m->range)) {
        m->current_speed = 0.0f;
        return stop_moving(unit, m, ops, 0);
    }
    if (!((double)dist >= 0.5)) {
        double d;
        wp = aoe_unit_path_current_ror(&m->path);
        d = wp ? path_distance(pos, wp) : dist;
        if (d == 0.0 || isnan(d)) {
            unit->group_unit_count = 0;
            return 2;
        }
        r = step(step_context, unit, m->dest_x, m->dest_y);
        switch (r) {
        case 5: return stop_moving(unit, m, ops, 0);
        case 0: m->status = 2; return 1;
        case 1: case 6: m->current_speed = 0.0f; m->status = 2; return 1;
        case 2: case 3: case 4: return stop_moving(unit, m, ops, 1);
        default: break;
        }
    }

    radius = (float)sqrt((double)tmpl->size[1] * tmpl->size[1] +
                         (double)tmpl->size[0] * tmpl->size[0]);
    wp = aoe_unit_path_current_ror(&m->path);
    if (!wp) {
        advanced = 0;
    } else {
        double d = path_distance(pos, wp);
        if (d == 0.0 || isnan(d)) {
            if (ops->base.fits(ops->base.context, unit, wp->x, wp->y))
                ops->base.place(ops->base.context, unit, wp->x, wp->y, wp->z);
            advanced = aoe_unit_path_advance_ror(&m->path);
            m->best = 100000.0f;
        }
    }
    if (m->waypoint_count > 1 && dist > 4.0f) {
        if (!m->waypoint_tile) return -1;
        if (ops->zone(ops->base.context, unit, m->waypoint_tile[0],
                      m->waypoint_tile[1]) ==
            ops->zone(ops->base.context, unit, m->waypoint_tile[3],
                      m->waypoint_tile[4]))
            bonus = 3.0f;
    }

    if (!advanced) {
        double reach = (double)bonus + m->range + radius;
        if (!(reach < dist || isnan(reach) || isnan(dist)))
            return stop_moving(unit, m, ops, 1);
        r = drop_reached(unit, m, radius, ops);
        if (r <= 0) return r;
        if (--m->retry_baseline <= 0) {
            m->current_speed = 0.0f;
            m->status = 2;
            if (m->waypoint_count <= 1) return 0;
            m->retry_baseline += 5;
            return 1;
        }
        if (!aoe_path_budget_allows_ror(budget, m->retry)) {
            m->current_speed = 0.0f;
            ++m->retry;
            return 1;
        }
        r = repath(unit, m, budget, ops, 1);
        if (r < 0) return r;
        if (!r) return give_up(m);
        if (!aoe_unit_path_advance_ror(&m->path)) return give_up(m);
        m->best = 100000.0f;
    }

    wp = aoe_unit_path_current_ror(&m->path);
    if (!wp) return give_up(m);
    r = step(step_context, unit, wp->x, wp->y);
    switch (r) {
    case 5: return stop_moving(unit, m, ops, 0);
    case 0: m->status = 2; return 1;
    case 1: m->current_speed = 0.0f; m->status = 2; return 1;
    case 2: break;
    default: return give_up(m);
    }
    if (aoe_unit_path_advance_ror(&m->path)) {
        m->status = 2;
        return 1;
    }
    r = drop_reached(unit, m, radius, ops);
    if (r <= 0) return r;
    if (!aoe_path_budget_allows_ror(budget, m->retry)) {
        m->current_speed = 0.0f;
        ++m->retry;
        return 1;
    }
    r = repath(unit, m, budget, ops, 2);
    if (r < 0) return r;
    if (!r) return give_up(m);
    if (!aoe_unit_path_advance_ror(&m->path)) return give_up(m);
    m->status = 2;
    return 1;
}

int aoe_movement_update_ror(AoeLiveUnit *unit, AoeMovementRor *m,
                            AoePathBudgetRor *budget,
                            const AoeMovementStepOps *ops) {
    if (!ops) return -1;
    return update_impl(unit, m, budget, ops, ops->step_to, ops->base.context);
}

/* Constants of 0x0044cde0 and its helpers (floats unless noted). */
#define TWO_PI_F 6.2831854820251465   /* 0x00544a30 */
#define INV_TWO_PI_F 0.15915493667125702 /* 0x00544a4c */
#define PI4_F 0.7853981852531433      /* 0x00544a60 */
#define PI8_F 0.39269909262657166     /* 0x00544aa0 */
#define PI_D 3.14159265               /* doubles 0x00544a50/0x00544a58 */
#define TWO_PI_D 6.2831853            /* double 0x00544a38 */

void aoe_movement_snap_angle_ror(float *angle, int32_t facets) {
    double v, step;
    float best_diff;
    int32_t best = -1;
    if (!angle || isnan(*angle)) return;
    v = *angle;
    if (v < 0.0) {
        do v += TWO_PI_F; while (v < 0.0);
        *angle = (float)v;
    } else if (v > TWO_PI_F) {
        do {
            v = (double)*angle - TWO_PI_F;
            *angle = (float)v;
        } while (v > TWO_PI_F);
    }
    if (facets == -1) return;
    step = facets == 8 ? PI4_F : PI8_F;
    best_diff = (float)step;
    for (int32_t i = 0; i <= facets; ++i) {
        /* One stack slot holds both the facet step and the best difference
           so far, so later candidates are i * best_diff. Exact facet
           multiples still snap to themselves. */
        double diff = (double)*angle - (double)i * best_diff;
        if (diff < 0.0) diff = -diff;
        if (diff < best_diff) {
            best_diff = (float)diff;
            best = i;
        }
        if (best_diff < 0.1) break;
    }
    *angle = (float)((double)best * (facets == 8 ? PI4_F : PI8_F));
}

void aoe_movement_update_facet_ror(AoeLiveUnit *unit, const AoeMovementRor *m,
                                   const AoeMovementSpriteRor *sprite) {
    double f, v;
    if (!unit || !m || !sprite || !(sprite->flags & 2) || sprite->facets <= 0)
        return;
    f = (float)sprite->facets;
    v = ((double)m->facing - PI4_F) * f * INV_TWO_PI_F + 0.5;
    if (v < 0.0 || isnan(v)) v += f;
    unit->core.scenario_frame = (uint8_t)aoe_ftol32(v);
}

/* 0x0044ce0c / 0x0044d9db: template speed scaled by a positive sprite
   speed multiplier. */
static float base_speed(const DatUnit *tmpl, const AoeMovementSpriteRor *sp) {
    if (sp && sp->speed_multiplier > 0.0f)
        return (float)((double)tmpl->speed * sp->speed_multiplier);
    return tmpl->speed;
}

static float tile_centre(float v) {
    return (float)(floor((double)v) + 0.5);
}

/* The 0x0044cffc / 0x0044d693 turning loops: while the turn timer allows,
   step the facing one facet toward the heading. */
static void turn(AoeMovementRor *m, const DatUnit *tmpl, double fraction,
                 float delta, int32_t facets, double threshold) {
    while (fraction > threshold || fraction < -threshold || isnan(fraction)) {
        double step = TWO_PI_D / facets;
        m->turn_timer = (float)((double)tmpl->rotation_speed + m->turn_timer);
        if (delta < 0.0f || isnan(delta)) {
            m->facing = (float)((double)m->facing - step);
            fraction += 1.0;
        } else {
            m->facing = (float)(step + m->facing);
            fraction -= 1.0;
        }
        /* NaN keeps the original looping until the timer turns positive,
           which never happens with a zero rotation speed; stop after one
           step instead (host-only guard). */
        if (m->turn_timer > 0.0f || isnan(fraction)) break;
    }
}

/* 0x0044d115 / 0x0044d790: start a blocked wait of 0.0..0.5 s plus half a
   tile's travel time. */
static int32_t start_wait(AoeMovementRor *m, const AoeMovementTickOps *ops) {
    double r;
    if (m->current_speed > 0.0f) {
        r = (double)(ops->rand(ops->step.base.context) % 6);
        m->blocked_wait = (float)(r * (double)0.1f +
                                  (double)0.5f / m->current_speed);
    } else {
        r = (double)(ops->rand(ops->step.base.context) % 6);
        m->blocked_wait = (float)(r * (double)0.1f + 0.5);
    }
    m->current_speed = 0.0f;
    return 0;
}

int32_t aoe_movement_step_to_ror(AoeLiveUnit *unit, AoeMovementRor *m,
                                 AoePathBudgetRor *budget,
                                 const AoeMovementTickOps *ops, float tx,
                                 float ty) {
    const AoeVisibilityUnitRef *pos;
    const DatUnit *tmpl;
    const AoeMovementSpriteRor *sp;
    const AoeMovementStepOps *so;
    void *ctx;
    const AoePathWaypointRor *wp;
    int first = 1;
    int32_t facets;
    float move, heading, delta_f, nx, ny, vy, d_now, mult;
    double tick, dy, delta, fraction, facets_f, vx, z0;
    if (!unit || !unit->core.unit_template || !m || !budget || !ops ||
        !ops->sprite || !ops->tick_seconds || !ops->target_position ||
        !ops->terrain_multiplier || !ops->rand ||
        !ops->step.base.find || !ops->step.base.fits ||
        !ops->step.base.place || !ops->step.base.owner_kind ||
        !ops->step.count_repath)
        return -1;
    so = &ops->step;
    ctx = so->base.context;
    pos = &unit->core.position;
    tmpl = unit->core.unit_template;
    sp = ops->sprite(ctx, unit);

    if (m->current_speed == 0.0f || isnan(m->current_speed)) {
        first = 0;
        m->current_speed = base_speed(tmpl, sp);
    }
    tick = ops->tick_seconds(ctx);
    move = (float)(tick * m->current_speed);
    if (m->blocked_wait > 0.0f) {
        double left = (double)m->blocked_wait - move;
        m->blocked_wait = (float)left;
        if (left > 0.0) {
            m->current_speed = 0.0f;
            return 0;
        }
        m->blocked_wait = -1.0f;
    }
    if (tmpl->rotation_speed > 0.0f) {
        if (m->turn_timer < 0.0f || isnan(m->turn_timer))
            m->turn_timer = 0.0f;
        else
            m->turn_timer = (float)((double)m->turn_timer - tick);
    }
    facets = sp ? sp->facets : 8;

    /* Heading from the unit (or its tile centre when starting). */
    {
        float dxf;
        if (first) {
            dxf = (float)((double)tx - pos->x38);
            dy = (double)ty - pos->y3c;
        } else {
            dxf = (float)((double)tx - tile_centre(pos->x38));
            dy = (double)ty - tile_centre(pos->y3c);
        }
        heading = (float)atan2(dy, (double)dxf);
    }
    aoe_movement_snap_angle_ror(&heading, -1);
    delta = (double)heading - m->facing;
    delta_f = (float)delta;
    /* NaN would loop forever in the original; it is left as is here. */
    if (delta < -PI_D) {
        do delta += TWO_PI_F; while (delta < -PI_D);
        delta_f = (float)delta;
    }
    if (delta > PI_D) {
        do delta -= TWO_PI_F; while (delta > PI_D);
        delta_f = (float)delta;
    }
    facets_f = (float)facets;
    fraction = (float)(facets_f * delta * INV_TWO_PI_F);
    aoe_movement_snap_angle_ror(&m->facing, facets);
    if (!(m->turn_timer > 0.0f))
        turn(m, tmpl, fraction, delta_f, facets, 0.8);
    aoe_movement_snap_angle_ror(&m->facing, facets);
    if (sp) aoe_movement_update_facet_ror(unit, m, sp);
    vx = cos((double)m->facing) * move;
    m->velocity_x = (float)vx;
    vy = (float)(sin((double)m->facing) * move);
    m->velocity_y = vy;
    nx = (float)(vx + pos->x38);
    ny = (float)((double)vy + pos->y3c);

    if (tmpl->size[2] > 0.0f && !so->base.fits(ctx, unit, nx, ny)) {
        float cx, cy, target_x, target_y;
        AoePathRequestRor req;
        uint32_t bits = UINT32_MAX;
        int32_t returned;
        if (m->blocked_wait == 0.0f || isnan(m->blocked_wait))
            return start_wait(m, ops);
        /* A wait already expired: re-centre and try another route. */
        m->blocked_wait = 0.0f;
        cx = tile_centre(pos->x38);
        cy = tile_centre(pos->y3c);
        if (so->base.fits(ctx, unit, cx, cy))
            so->base.place(ctx, unit, cx, cy, pos->z40);
        if (m->target_id != -1 &&
            ops->target_position(ctx, m->target_id, &target_x, &target_y)) {
            double ddx = (double)target_x - pos->x38;
            double ddy = (double)target_y - pos->y3c;
            double d = sqrt(ddy * ddy + ddx * ddx);
            double r = sqrt((double)tmpl->size[0] * tmpl->size[0] +
                            (double)tmpl->size[1] * tmpl->size[1]);
            if (r + m->range + (double)0.3f > d) return 5;
        }
        if (!so->base.fits(ctx, unit, tx, ty)) return 2;
        if (--m->retry_baseline <= 0) return 4;
        if (!aoe_path_budget_allows_ror(budget, m->retry)) {
            ++m->retry;
            return 1;
        }
        aoe_path_budget_consume_ror(budget);
        m->start_x = pos->x38;
        m->start_y = pos->y3c;
        m->start_z = pos->z40;
        m->retry = 0;
        so->count_repath(ctx);
        req.x1 = aoe_ftol32(m->start_x);
        req.y1 = aoe_ftol32(m->start_y);
        if (m->waypoint_count > 0) {
            if (!m->waypoint_tile) return -1;
            req.x2 = m->waypoint_tile[0];
            req.y2 = m->waypoint_tile[1];
        } else {
            req.x2 = aoe_ftol32(m->dest_x);
            req.y2 = aoe_ftol32(m->dest_y);
        }
        req.unit = unit;
        req.range = m->range;
        req.target_id = m->target_id;
        req.mode = 1;
        req.budget_limited = 1;
        req.argument_30 = 1;
        req.check_occupancy = 1;
        req.step = 1;
        memcpy(&req.query_radius, &bits, sizeof(bits));
        req.query_filter = -1;
        req.budget_base = 2500;
        returned = so->base.find(ctx, &req);
        if (returned == 0 && m->path.count == 1) {
            cx = tile_centre(pos->x38);
            cy = tile_centre(pos->y3c);
            if (so->base.fits(ctx, unit, cx, cy)) {
                if (!(off_centre(pos->x38) || off_centre(pos->y3c))) return 4;
                so->base.place(ctx, unit, cx, cy, pos->z40);
            }
        }
        aoe_unit_path_rewind_ror(&m->path);
        if (!aoe_unit_path_advance_ror(&m->path)) return 4;
        m->best = 100000.0f;
        wp = aoe_unit_path_current_ror(&m->path);
        if (!wp) return -1;
        {
            float dxf;
            if (first) {
                dxf = (float)((double)wp->x - pos->x38);
                dy = (double)wp->y - pos->y3c;
            } else {
                dxf = (float)((double)wp->x - tile_centre(pos->x38));
                dy = (double)wp->y - tile_centre(pos->y3c);
            }
            heading = (float)atan2(dy, (double)dxf);
        }
        aoe_movement_snap_angle_ror(&heading, -1);
        delta_f = (float)((double)heading - m->facing);
        fraction = (double)(float)facets_f * delta_f * INV_TWO_PI_F;
        if (m->turn_timer > 0.0f) {
            m->current_speed = 0.0f;
            return 0;
        }
        turn(m, tmpl, fraction, delta_f, facets, 0.9);
        aoe_movement_snap_angle_ror(&m->facing, facets);
        if (sp) aoe_movement_update_facet_ror(unit, m, sp);
        vx = cos((double)m->facing) * move;
        m->velocity_x = (float)vx;
        vy = (float)(sin((double)m->facing) * move);
        m->velocity_y = vy;
        nx = (float)(vx + pos->x38);
        ny = (float)((double)vy + pos->y3c);
        if (!so->base.fits(ctx, unit, nx, ny)) {
            if (so->base.owner_kind(ctx, unit) == 1) return 4;
            return start_wait(m, ops);
        }
    }

    /* Move (0x0044d80a). */
    z0 = pos->z40;
    wp = aoe_unit_path_current_ror(&m->path);
    if (wp) {
        float dxf = (float)((double)wp->x - pos->x38);
        double wdy = (double)wp->y - pos->y3c;
        float ndx = (float)((double)wp->x -
                            ((double)pos->x38 + m->velocity_x));
        double ndy = (double)wp->y - ((double)m->velocity_y + pos->y3c);
        double d_next = sqrt(ndy * ndy + (double)ndx * ndx);
        int snap = 0;
        d_now = (float)sqrt(wdy * wdy + (double)dxf * dxf);
        /* Overshooting within a quarter tile: 0x0044d8af compares
           against 0x00544a78 as a float (0.25). */
        if (d_next > m->best && d_now < 0.25f)
            snap = 1;
        else if (m->facet_history[1] == unit->core.scenario_frame &&
                 m->facet_history[1] != m->facet_history[0] &&
                 !(d_now >= 0.5f))
            snap = 1; /* oscillating between two facets near it */
        if (snap) {
            if (!so->base.fits(ctx, unit, wp->x, wp->y)) return 2;
            so->base.place(ctx, unit, wp->x, wp->y, wp->z);
        } else {
            so->base.place(ctx, unit, nx, ny,
                           (float)((double)m->velocity_z + pos->z40));
        }
    } else {
        double ddx = (double)tx - nx;
        double ddy = (double)ty - ny;
        float ddyf = (float)ddy;
        d_now = (float)sqrt(ddy * ddyf + ddx * ddx);
        so->base.place(ctx, unit, nx, ny,
                       (float)((double)m->velocity_z + pos->z40));
    }

    /* Slope and terrain speed (0x0044d97b). */
    {
        double dz = z0 - pos->z40;
        float terrain;
        m->best = d_now;
        m->facet_history[1] = m->facet_history[0];
        m->facet_history[0] = unit->core.scenario_frame;
        mult = 1.0f;
        if (dz < -0.02 || isnan(dz))
            mult = 0.9f;
        else if (dz > 0.02)
            mult = 1.03f;
        else
            m->current_speed = base_speed(tmpl, sp);
        if (ops->terrain_multiplier(ctx, unit, &terrain))
            mult = (float)((double)terrain * mult);
        if (mult > 0.0f)
            m->current_speed = (float)((double)mult * m->current_speed);
    }
    return 0;
}

typedef struct {
    AoeMovementRor *m;
    AoePathBudgetRor *budget;
    const AoeMovementTickOps *ops;
} TickBinding;

static int32_t bound_step(void *context, AoeLiveUnit *unit, float x,
                          float y) {
    TickBinding *b = context;
    return aoe_movement_step_to_ror(unit, b->m, b->budget, b->ops, x, y);
}

int aoe_movement_tick_ror(AoeLiveUnit *unit, AoeMovementRor *m,
                          AoePathBudgetRor *budget,
                          const AoeMovementTickOps *ops) {
    TickBinding b = {m, budget, ops};
    if (!ops) return -1;
    return update_impl(unit, m, budget, &ops->step, bound_step, &b);
}

double aoe_movement_facet_offset_ror(const AoeLiveUnit *unit,
                                     const AoeMovementRor *m,
                                     const AoeMovementSpriteRor *sp,
                                     float x, float y) {
    double a;
    if (!unit || !m || !sp || !(sp->flags & 2) || sp->facets <= 0)
        return 0.0;
    a = atan2((double)y - unit->core.position.y3c,
              (double)x - unit->core.position.x38);
    if (a < 0.0 || isnan(a)) a += TWO_PI_F;
    a -= m->facing;
    if (a > PI_D) a -= TWO_PI_F;
    if (a < -PI_D || isnan(a)) a += TWO_PI_F;
    return a * ((double)sp->facets * INV_TWO_PI_F);
}

int aoe_movement_face_ror(AoeLiveUnit *unit, AoeMovementRor *m,
                          const AoeMovementSpriteRor *sp, float x, float y,
                          uint32_t world_ms, float *last_turn) {
    float f;
    double step, now;
    if (!unit || !unit->core.unit_template || !m || !last_turn) return 0;
    f = (float)aoe_movement_facet_offset_ror(unit, m, sp, x, y);
    if (!(f > 0.5f) && !(f < -0.5f || isnan(f))) return 1;
    /* The comparison uses the unrounded product; +0xa0 keeps the float. */
    now = (double)world_ms * (double)0.001f;
    if (!(now - *last_turn >= unit->core.unit_template->rotation_speed))
        return 0;
    /* A facet offset needs a facet sprite, so facets > 0 here. */
    step = TWO_PI_D / sp->facets;
    if (f < 0.0f || isnan(f))
        m->facing = (float)((double)m->facing - step);
    else
        m->facing = (float)(step + m->facing);
    if (m->facing < 0.0f || isnan(m->facing))
        m->facing = (float)((double)m->facing + TWO_PI_F);
    else if (m->facing > TWO_PI_F)
        m->facing = (float)((double)m->facing - TWO_PI_F);
    aoe_movement_update_facet_ror(unit, m, sp);
    *last_turn = (float)now;
    f = (float)aoe_movement_facet_offset_ror(unit, m, sp, x, y);
    return !(f > 0.5f) && !(f < -0.5f || isnan(f));
}

void aoe_movement_reinit_ror(AoeMovementRor *m) {
    if (!m) return;
    m->velocity_x = m->velocity_y = m->velocity_z = 0.0f;
    m->blocked_wait = 0.0f;
    m->turn_timer = 0.0f;
    m->field_a0 = 0;
}

int aoe_movement_moving_ror(const AoeMovementRor *m) {
    return m && m->current_speed > 0.0f;
}

uint8_t aoe_movement_hold_ror(const AoeMovementRor *m, uint8_t unit_state) {
    if (!m || unit_state == 6 || unit_state == 7) return 0;
    return m->hold_155;
}

void aoe_movement_set_hold_ror(AoeMovementRor *m, uint8_t value) {
    if (m) m->hold_155 = value;
}

int32_t aoe_unit_path_coord_sum_ror(AoeUnitPathRor *path) {
    float sum = 0.0f;
    int32_t saved, i;
    if (!path) return 0;
    saved = aoe_unit_path_cursor_ror(path);
    aoe_unit_path_rewind_ror(path);
    for (i = 0; i < aoe_unit_path_count_ror(path); ++i) {
        const AoePathWaypointRor *w = aoe_unit_path_current_ror(path);
        if (w) sum = (float)((double)w->y + w->x + sum);
        aoe_unit_path_advance_ror(path);
    }
    aoe_unit_path_set_cursor_ror(path, saved);
    return aoe_ftol32(sum);
}

/* 0x0044e350 */
int32_t aoe_movement_path_to_ror(AoeLiveUnit *unit, AoeMovementRor *m,
                                 int32_t x2, int32_t y2, float range,
                                 int32_t target_id, int32_t budget_limited,
                                 int32_t free_terrain_a,
                                 int32_t free_terrain_b, int second,
                                 int32_t query_filter, int32_t budget_base,
                                 const AoeMovementPathToOps *ops) {
    AoePathRequestRor req;
    uint32_t bits = 1u; /* pushed 1 as the float argument */
    int32_t result;
    if (!unit || !m || !ops || !ops->find || !ops->count_search) return -1;
    if (x2 < 0 || y2 < 0 || x2 >= ops->map_width || y2 >= ops->map_height)
        return 0;
    m->free_terrain_b = free_terrain_b;
    m->free_terrain_a = free_terrain_a;
    m->use_alternate_path = 1;
    ops->count_search(ops->context, second);
    memset(&req, 0, sizeof(req));
    req.x1 = aoe_ftol32(unit->core.position.x38);
    req.y1 = aoe_ftol32(unit->core.position.y3c);
    req.x2 = x2;
    req.y2 = y2;
    req.unit = unit;
    req.range = range;
    req.target_id = target_id;
    req.mode = 1;
    req.budget_limited = budget_limited;
    req.argument_30 = 1;
    req.check_occupancy = 1;
    req.step = 1;
    memcpy(&req.query_radius, &bits, sizeof(bits));
    req.query_filter = query_filter;
    req.budget_base = budget_base;
    result = ops->find(ops->context, second, &req);
    m->use_alternate_path = 0;
    m->free_terrain_a = -1;
    m->free_terrain_b = -1;
    return result;
}

/* 0x0044ede0 */
AoeUnitPathRor *aoe_movement_reach_path_ror(AoeLiveUnit *unit,
                                            AoeMovementRor *m,
                                            int32_t *zone_index,
                                            const int32_t goal[2],
                                            float range, int32_t target_id,
                                            const AoeMovementReachOps *ops) {
    const AoeZoneMapRor *map;
    AoePathRequestRor req;
    uint32_t bits = 1u; /* pushed 1 as the float argument */
    int32_t ux, uy;
    if (!unit || !m || !zone_index || !goal || !ops || !ops->zone_index ||
        !ops->zone_map || !ops->find || !ops->clear_penalty)
        return NULL;
    if (*zone_index == -1) *zone_index = ops->zone_index(ops->context, unit);
    map = ops->zone_map(ops->context, *zone_index);
    if (!map) return NULL;
    ux = aoe_ftol32(unit->core.position.x38);
    uy = aoe_ftol32(unit->core.position.y3c);
    if (!map->zones || ux < 0 || uy < 0 || ux >= map->width ||
        uy >= map->height || goal[0] < 0 || goal[1] < 0 ||
        goal[0] >= map->width || goal[1] >= map->height)
        return NULL;
    if (aoe_zone_at_ror(map, goal[0], goal[1]) != aoe_zone_at_ror(map, ux, uy) &&
        !aoe_zone_near_tile_ror(map, ux, uy, goal[0], goal[1], range))
        return NULL;
    memset(&req, 0, sizeof(req));
    req.x1 = ux;
    req.y1 = uy;
    req.x2 = goal[0];
    req.y2 = goal[1];
    req.unit = unit;
    req.range = range;
    req.target_id = target_id;
    req.mode = 1;
    req.budget_limited = 0;
    req.argument_30 = 0;
    req.check_occupancy = 1;
    req.step = 1;
    memcpy(&req.query_radius, &bits, sizeof(bits));
    req.query_filter = -1;
    req.budget_base = -1;
    m->use_alternate_path = 1;
    (void)ops->find(ops->context, &req);
    m->use_alternate_path = 0;
    ops->clear_penalty(ops->context);
    return &m->alternate_path;
}

/* 0x0044e7a0 (and the body of 0x0044e600) */
int32_t aoe_movement_line_passable_ror(AoeLiveUnit *unit, int32_t x1,
                                       int32_t y1, int32_t x2, int32_t y2,
                                       float range, int32_t arg,
                                       AoeMovementPassableFn passable,
                                       void *context, int32_t blocked[2]) {
    int32_t dx = x2 - x1, dy = y2 - y1, ax, ay, steps, last_x = -999,
            last_y = -999;
    float sx, sy, fx, fy;
    if (!passable) return -1;
    if (dx == 0 && dy == 0) return 0;
    ax = dx < 0 ? -dx : dx;
    ay = dy < 0 ? -dy : dy;
    steps = 2 * (ax < ay ? ay : ax);
    sx = (float)((double)dx / (double)steps);
    sy = (float)((double)dy / (double)steps);
    fx = (float)x1;
    fy = (float)y1;
    for (int32_t i = 0; i < steps; ++i) {
        int32_t tx, ty;
        double ex, ey;
        fx = fx + sx;
        fy = fy + sy;
        tx = aoe_ftol32(fx);
        ty = aoe_ftol32(fy);
        if (tx == last_x && ty == last_y) continue;
        last_x = tx;
        last_y = ty;
        if (!passable(context, unit, fx, fy, arg)) {
            if (blocked) {
                blocked[0] = tx;
                blocked[1] = ty;
            }
            return 0;
        }
        ex = (double)x2 - (double)fx;
        ey = (double)y2 - (double)fy;
        if (!(sqrt(ey * ey + ex * ex) > (double)range)) return 1;
    }
    return 1;
}

/* 0x0044e600 */
int32_t aoe_movement_line_first_block_ror(AoeLiveUnit *unit,
                                          const int32_t from[2],
                                          const int32_t to[2], float range,
                                          int32_t blocked[2], int32_t arg,
                                          AoeMovementPassableFn passable,
                                          void *context) {
    if (!from || !to || !blocked) return -1;
    return aoe_movement_line_passable_ror(unit, from[0], from[1], to[0],
                                          to[1], range, arg, passable,
                                          context, blocked);
}

/* 0x0044da90 */
int32_t aoe_movement_zone_path_ror(AoeLiveUnit *unit, int32_t *zone_index,
                                   int32_t x2, int32_t y2, float range,
                                   int32_t target_id, int32_t budget_limited,
                                   int second, int32_t query_filter,
                                   int32_t budget_base,
                                   const AoeMovementZonePathOps *ops) {
    const AoeZoneMapRor *map;
    AoePathRequestRor req;
    uint32_t bits = 1u; /* pushed 1 as the float argument */
    int32_t ux, uy;
    if (!unit || !zone_index || !ops || !ops->zone_index || !ops->zone_map ||
        !ops->path.find || !ops->path.count_search)
        return -1;
    if (x2 < 0 || y2 < 0 || x2 >= ops->path.map_width ||
        y2 >= ops->path.map_height)
        return 0;
    if (*zone_index == -1) *zone_index = ops->zone_index(ops->path.context, unit);
    map = ops->zone_map(ops->path.context, *zone_index);
    if (!map) return 0;
    ux = aoe_ftol32(unit->core.position.x38);
    uy = aoe_ftol32(unit->core.position.y3c);
    if (!map->zones || ux < 0 || uy < 0 || ux >= map->width ||
        uy >= map->height || x2 >= map->width || y2 >= map->height)
        return -1; /* host-only: the original reads unchecked */
    if (aoe_zone_at_ror(map, x2, y2) != aoe_zone_at_ror(map, ux, uy) &&
        !aoe_zone_near_tile_ror(map, ux, uy, x2, y2, range))
        return 0;
    ops->path.count_search(ops->path.context, second);
    memset(&req, 0, sizeof(req));
    req.x1 = ux;
    req.y1 = uy;
    req.x2 = x2;
    req.y2 = y2;
    req.unit = unit;
    req.range = range;
    req.target_id = target_id;
    req.mode = 0;
    req.budget_limited = budget_limited;
    req.argument_30 = 1;
    req.check_occupancy = 1;
    req.step = 1;
    memcpy(&req.query_radius, &bits, sizeof(bits));
    req.query_filter = query_filter;
    req.budget_base = budget_base;
    return ops->path.find(ops->path.context, second, &req);
}

static int32_t object_path_common(AoeLiveUnit *unit, AoeMovementRor *m, int32_t *zone_index,
                                  const AoeLiveUnit *target, int32_t target_id, float range,
                                  int32_t budget_limited, int second, int32_t query_filter,
                                  int32_t budget_base, int32_t mode,
                                  const AoeMovementZonePathOps *ops) {
    const AoeZoneMapRor *map;
    AoePathRequestRor req;
    uint32_t bits = 1u; /* pushed 1 as the float argument */
    int32_t ux, uy, tx, ty, r;
    if (!unit || !zone_index || !ops || !ops->zone_index || !ops->zone_map ||
        !ops->path.find || !ops->path.count_search)
        return -1;
    if (!target) return 0;
    if (*zone_index == -1) *zone_index = ops->zone_index(ops->path.context, unit);
    map = ops->zone_map(ops->path.context, *zone_index);
    if (!map) return 0;
    ux = aoe_ftol32(unit->core.position.x38);
    uy = aoe_ftol32(unit->core.position.y3c);
    tx = aoe_ftol32(target->core.position.x38);
    ty = aoe_ftol32(target->core.position.y3c);
    if (!map->zones || ux < 0 || uy < 0 || tx < 0 || ty < 0 || ux >= map->width ||
        uy >= map->height || tx >= map->width || ty >= map->height)
        return -1; /* host-only: the original reads unchecked */
    if (aoe_zone_at_ror(map, tx, ty) != aoe_zone_at_ror(map, ux, uy) &&
        !aoe_zone_near_tile_ror(map, ux, uy, tx, ty, range))
        return 0;
    if (m) m->use_alternate_path = 1;
    ops->path.count_search(ops->path.context, second);
    memset(&req, 0, sizeof(req));
    req.x1 = ux;
    req.y1 = uy;
    req.x2 = tx;
    req.y2 = ty;
    req.unit = unit;
    req.range = range;
    req.target_id = target_id;
    req.mode = mode;
    req.budget_limited = budget_limited;
    req.argument_30 = 1;
    req.check_occupancy = 1;
    req.step = 1;
    memcpy(&req.query_radius, &bits, sizeof(bits));
    req.query_filter = query_filter;
    req.budget_base = budget_base;
    r = ops->path.find(ops->path.context, second, &req);
    return r;
}

int32_t aoe_movement_object_path_ror(AoeLiveUnit *unit, int32_t *zone_index,
                                     const AoeLiveUnit *target, int32_t target_id, float range,
                                     int32_t budget_limited, int second, int32_t query_filter,
                                     int32_t budget_base, const AoeMovementZonePathOps *ops) {
    return object_path_common(unit, NULL, zone_index, target, target_id, range, budget_limited,
                              second, query_filter, budget_base, 0, ops);
}

int32_t aoe_movement_object_path_alt_ror(AoeLiveUnit *unit, AoeMovementRor *m,
                                         int32_t *zone_index, const AoeLiveUnit *target,
                                         int32_t target_id, float range,
                                         int32_t budget_limited, int second,
                                         int32_t query_filter, int32_t budget_base, void *met,
                                         AoeMovementCollectFn collect,
                                         const AoeMovementZonePathOps *ops) {
    int32_t r;
    if (!m) return -1;
    m->use_alternate_path = 0;
    r = object_path_common(unit, m, zone_index, target, target_id, range, budget_limited, second,
                           query_filter, budget_base, 1, ops);
    if (!m->use_alternate_path) return r; /* no search was run */
    if (budget_base != -1 && collect) collect(ops->path.context, second, met);
    m->use_alternate_path = 0;
    return r;
}
