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
