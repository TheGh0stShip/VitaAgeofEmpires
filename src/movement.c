#include "movement.h"

#include <math.h>
#include <stddef.h>
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

void aoe_movement_mark_explored_ror(AoeMovementRor *m) {
    if (m) m->explored = 1;
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

int aoe_movement_update_ror(AoeLiveUnit *unit, AoeMovementRor *m,
                            AoePathBudgetRor *budget,
                            const AoeMovementStepOps *ops) {
    const AoeVisibilityUnitRef *pos;
    const DatUnit *tmpl;
    const AoePathWaypointRor *wp;
    float dxf, ex, dist, radius, bonus = 0.0f;
    double dy, ey;
    int advanced = 1, r;
    if (!unit || !unit->core.unit_template || !m || !budget || !ops ||
        !ops->base.order_flag || !ops->base.find || !ops->base.fits ||
        !ops->base.place || !ops->stop || !ops->pop_waypoint || !ops->zone ||
        !ops->count_repath || !ops->step_to)
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
        r = ops->step_to(ops->base.context, unit, m->dest_x, m->dest_y);
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
    r = ops->step_to(ops->base.context, unit, wp->x, wp->y);
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
