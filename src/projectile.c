#include "projectile.h"

#include <math.h>
#include <string.h>

#include "aoe_ftol.h"

enum { HIT_CAPACITY = 256 };

static int ops_ok(const AoeProjectileOps *o) {
    return o && o->move.resolve && o->move.target_released &&
           o->move.target_attached && o->tick_seconds && o->target_speed &&
           o->target_heading && o->capture_launcher && o->destroy &&
           o->set_facing && o->fly && o->hit_query && o->hit && o->blast &&
           o->owner_id && o->same_owner;
}

/* 0x004073f0: the secondary-unit setter, used for the launcher. */
static void set_launcher(AoeProjectileRor *p, AoeLiveUnit *launcher,
                         const AoeProjectileOps *ops) {
    void *c = ops->move.context;
    if (p->launcher && p->launcher != launcher) {
        AoeLiveUnit *old = ops->move.resolve(c, p->base.secondary_id);
        if (old) ops->move.target_released(c, old, p->base.unit);
    }
    if (p->launcher == launcher) return;
    p->launcher = launcher;
    if (launcher) {
        p->base.secondary_id = launcher->core.unit_id;
        ops->move.target_attached(c, launcher, p->base.unit, p->base.type);
    }
}

int aoe_projectile_init_ror(AoeProjectileRor *p, AoeLiveUnit *projectile,
                            AoeLiveUnit *launcher, AoeLiveUnit *target,
                            float x, float y, float z, float launch_radius,
                            const AoeProjectileOps *ops) {
    if (!p || !projectile || !ops_ok(ops)) return 0;
    memset(p, 0, sizeof(*p));
    p->base.type = 8;
    p->base.unit = projectile;
    p->base.target_id = -1;
    p->base.secondary_id = -1;
    p->base.x = x;
    p->base.y = y;
    p->base.z = z;
    p->launch_radius = launch_radius;
    if (!aoe_action_set_target_ror(&p->base, target, &ops->move)) return 0;
    set_launcher(p, launcher, ops);
    return 1;
}

static int dying(const AoeLiveUnit *u) {
    return u->core.state >= 3 && u->core.state != 6; /* 0x004aa8c0 */
}

/* 0x00404a50: the aim point. */
static void aim_point(const AoeProjectileRor *p, float *x, float *y,
                      float *z) {
    const AoeLiveUnit *t = p->base.target;
    if (!t) {
        *x = p->base.x;
        *y = p->base.y;
        *z = p->base.z;
        return;
    }
    *x = t->core.position.x38;
    *y = t->core.position.y3c;
    *z = (float)((double)t->core.unit_template->size[2] * 0.5 +
                 t->core.position.z40);
}

static int aim(AoeProjectileRor *p, const AoeProjectileOps *ops) {
    void *c = ops->move.context;
    AoeLiveUnit *u = p->base.unit;
    const DatUnit *tmpl = u->core.unit_template;
    const AoeVisibilityUnitRef *pos = &u->core.position;
    float tx, ty, tz, dx, dy, dzf, dz2, dist;
    if (p->base.target && !p->base.target->core.unit_template) return 0;
    aim_point(p, &tx, &ty, &tz);
    dx = (float)((double)tx - pos->x38);
    dy = (float)((double)ty - pos->y3c);
    {
        double dz = (double)tz - pos->z40;
        dzf = (float)dz;
        dz2 = (float)(dz * dzf);
    }
    dist = (float)sqrt((double)dy * dy + (double)dx * dx + dz2);
    if (tmpl->projectile_lead == 1 && p->base.target) {
        float ts = ops->target_speed(c, p->base.target);
        if (ts > 0.0f) {
            double h, lx, ly, nx, ny, d2, denom;
            if (p->launcher && p->launcher->core.unit_template) {
                int16_t id = p->launcher->core.unit_template->id;
                if ((id == 0x23 || id == 0x24 || id == 0x118) && ts > 1.0f)
                    ts = 1.0f;
            }
            h = ops->target_heading(c, p->base.target);
            lx = (float)(cos(h) * ts);
            ly = sin(h) * ts;
            nx = lx + dx;
            ny = ly + dy;
            d2 = sqrt(ny * ny + nx * nx + dz2);
            denom = ((double)dist - d2) + tmpl->speed;
            if (denom > (double)tmpl->speed * 0.5) {
                double t = dist / denom;
                dx = (float)(t * lx + dx);
                dy = (float)(t * ly + dy);
                dist = (float)sqrt((double)dy * dy + (double)dx * dx + dz2);
            }
        }
    }
    switch (tmpl->projectile_flight_mode) {
    case 0: {
        double ux = (double)dx / dist, uy = (double)dy / dist;
        p->dir_x = (float)ux;
        p->dir_y = (float)uy;
        p->dir_z = (float)((double)dzf / dist);
        ops->set_facing(c, u, (float)atan2(uy, ux));
        p->traveled = 0.0f;
        break;
    }
    case 1: case 2:
        if (!aoe_projectile_set_state_ror(p, 4, ops)) return 0;
        break;
    case 3:
        p->dir_x = dx;
        p->dir_y = dy;
        p->dir_z = dzf;
        ops->set_facing(c, u, (float)atan2((double)dy, (double)dx));
        break;
    default:
        break;
    }
    if (tmpl->projectile_arc > 0.0f) {
        double half = (tmpl->speed > 0.0f ? (double)dist / tmpl->speed
                                          : 0.0) * 0.5;
        double g = -((double)dist * tmpl->projectile_arc) * 2.0 /
                   (half * half);
        p->gravity = (float)g;
        p->climb = (float)-(g * half);
    } else {
        p->climb = 0.0f;
        p->gravity = 0.0f;
    }
    return aoe_projectile_set_state_ror(p, 4, ops);
}

int aoe_projectile_set_state_ror(AoeProjectileRor *p, uint8_t state,
                                 const AoeProjectileOps *ops) {
    if (!p || !p->base.unit || !p->base.unit->core.unit_template ||
        !ops_ok(ops))
        return 0;
    p->base.state = state;
    if (state == 1) {
        ops->destroy(ops->move.context, p->base.unit);
        return 1;
    }
    if (state == 3) return aim(p, ops);
    return 1;
}

int aoe_projectile_activate_ror(AoeProjectileRor *p, int aim_now,
                                const AoeProjectileOps *ops) {
    uint32_t bx, by;
    if (!p) return 0;
    if (!aim_now) return aoe_projectile_set_state_ror(p, 4, ops);
    memcpy(&bx, &p->base.x, sizeof(bx));
    memcpy(&by, &p->base.y, sizeof(by));
    if (!p->base.target && (bx == 0xbf800000u || by == 0xbf800000u))
        return aoe_projectile_set_state_ror(p, 1, ops);
    return aoe_projectile_set_state_ror(p, 3, ops);
}

int aoe_projectile_overlaps_ror(const AoeLiveUnit *projectile,
                                const AoeLiveUnit *other, float step) {
    const DatUnit *pt, *ot;
    double dz, low, high, ax, ay;
    float dxf, gx, gy;
    if (!projectile || !other || other == projectile) return 0;
    pt = projectile->core.unit_template;
    ot = other->core.unit_template;
    if (!pt || !ot || !(pt->size[0] > 0.0f) || !(pt->size[1] > 0.0f) ||
        !(ot->size[1] > 0.0f) || !(ot->size[0] > 0.0f))
        return 0;
    dz = (double)other->core.position.z40 - projectile->core.position.z40;
    low = -((double)ot->size[2] + (double)0.1f);
    high = (double)pt->size[2] + (double)0.1f;
    if (!(dz >= low) || dz > high) return 0;
    dxf = (float)((double)other->core.position.x38 -
                  projectile->core.position.x38);
    ay = (double)other->core.position.y3c - projectile->core.position.y3c;
    ax = dxf < 0.0f || isnan(dxf) ? -dxf : dxf;
    if (ay < 0.0 || isnan(ay)) ay = -ay;
    gx = (float)(ax - (((double)ot->size[0] + pt->size[0]) + step));
    gy = (float)(ay - (((double)ot->size[1] + pt->size[1]) + step));
    return !(gx > 0.0f) && !(gy > 0.0f);
}

static void strike(AoeProjectileRor *p, const AoeProjectileOps *ops,
                   AoeLiveUnit *victim, int blast) {
    AoeLiveUnit *u = p->base.unit;
    AoeLiveUnit *striker = p->launcher ? p->launcher : u;
    const AoeVisibilityUnitRef *pos = &u->core.position;
    if (blast)
        ops->blast(ops->move.context, striker, pos->x38, pos->y3c, pos->z40);
    else
        ops->hit(ops->move.context, striker, victim, pos->x38, pos->y3c,
                 pos->z40);
}

int aoe_projectile_update_ror(AoeProjectileRor *p,
                              const AoeProjectileOps *ops) {
    void *c;
    AoeLiveUnit *u;
    const DatUnit *tmpl;
    double tick;
    float step, vx = 0.0f, vy = 0.0f, vz = 0.0f;
    int landed;
    if (!p || !p->base.unit || !p->base.unit->core.unit_template ||
        !ops_ok(ops))
        return -1;
    c = ops->move.context;
    u = p->base.unit;
    tmpl = u->core.unit_template;
    tick = ops->tick_seconds(c);
    step = (float)(tick * tmpl->speed);
    if (p->base.target_id != -1 && !ops->move.resolve(c, p->base.target_id))
        if (!aoe_action_set_target_ror(&p->base, NULL, &ops->move)) return -1;
    if (p->base.secondary_id != -1 &&
        !ops->move.resolve(c, p->base.secondary_id))
        set_launcher(p, NULL, ops);
    if (p->base.target && dying(p->base.target))
        if (!aoe_action_set_target_ror(&p->base, NULL, &ops->move)) return -1;
    if (p->launcher && dying(p->launcher)) {
        ops->capture_launcher(c, u, p->launcher);
        set_launcher(p, NULL, ops);
    }
    if (p->base.state == 1) return 1;
    if (p->base.state != 4) return 0;

    if (tmpl->projectile_flight_mode == 0) {
        vx = (float)((double)step * p->dir_x);
        vy = (float)((double)step * p->dir_y);
        vz = (float)((double)step * p->dir_z);
        p->traveled = (float)((double)step + p->traveled);
        if (p->traveled >= p->launch_radius &&
            !aoe_projectile_set_state_ror(p, 1, ops))
            return -1;
    } else if (tmpl->projectile_flight_mode == 3) {
        vx = p->dir_x;
        vy = p->dir_y;
        vz = p->dir_z;
    }
    if (tmpl->projectile_arc > 0.0f) {
        double a = tick * p->gravity;
        vz = (float)((a * 0.5 + p->climb) * tick + vz);
        p->climb = (float)(a + p->climb);
    }
    landed = ops->fly(c, u, vx, vy, vz);

    if (tmpl->projectile_flight_mode == 3) {
        strike(p, ops, p->base.target, 0);
        if (!aoe_projectile_set_state_ror(p, 1, ops)) return -1;
    } else if (tmpl->projectile_flight_mode <= 2) {
        AoeLiveUnit *hits[HIT_CAPACITY];
        size_t n = ops->hit_query(c, u, step, hits, HIT_CAPACITY);
        for (size_t i = 0; i < n && i < HIT_CAPACITY; ++i) {
            AoeLiveUnit *h = hits[i];
            int foreign = !ops->same_owner(c, h, u) &&
                          ops->owner_id(c, h) != 0;
            switch (tmpl->projectile_hit_mode) {
            case 0:
                if (h != p->base.target) continue;
                strike(p, ops, h, 0);
                break;
            case 1:
                if (!foreign) continue;
                strike(p, ops, h, 0);
                break;
            case 2:
                if (p->launcher && h == p->launcher) continue;
                strike(p, ops, h, !foreign);
                break;
            default:
                continue;
            }
            if (!aoe_projectile_set_state_ror(p, 1, ops)) return -1;
        }
    }
    if (landed && p->base.state != 1) {
        strike(p, ops, NULL, 1);
        if (!aoe_projectile_set_state_ror(p, 1, ops)) return -1;
    }
    return 0;
}
