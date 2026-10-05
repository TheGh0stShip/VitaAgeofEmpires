#include "projectile.h"

#include <assert.h>
#include <math.h>
#include <string.h>

static AoeLiveUnit proj, archer, foe, friend_unit;
static DatUnit proj_t, archer_t, foe_t;
static int destroyed, captured, hits, blasts, landed_next;
static AoeLiveUnit *last_striker, *last_victim;
static float facing, tick_value = 1.0f, foe_speed;
static AoeLiveUnit *hit_list[4];
static size_t hit_count;
static float query_step;

static AoeLiveUnit *resolve(void *c, int32_t id) {
    (void)c;
    if (id == archer.core.unit_id && archer.core.unit_template) return &archer;
    if (id == foe.core.unit_id && foe.core.unit_template) return &foe;
    return NULL;
}
static void released(void *c, AoeLiveUnit *t, AoeLiveUnit *u) { (void)c; (void)t; (void)u; }
static void attached(void *c, AoeLiveUnit *t, AoeLiveUnit *u, int16_t type) {
    (void)c; (void)t; (void)u;
    assert(type == 8);
}
static float tick(void *c) { (void)c; return tick_value; }
static float tspeed(void *c, AoeLiveUnit *t) { (void)c; (void)t; return foe_speed; }
static float theading(void *c, AoeLiveUnit *t) { (void)c; (void)t; return 0.0f; }
static void capture(void *c, AoeLiveUnit *p, AoeLiveUnit *l) { (void)c; (void)p; (void)l; ++captured; }
static void destroy(void *c, AoeLiveUnit *p) { (void)c; (void)p; ++destroyed; }
static void set_facing(void *c, AoeLiveUnit *p, float a) { (void)c; (void)p; facing = a; }
static int fly(void *c, AoeLiveUnit *p, float vx, float vy, float vz) {
    (void)c;
    p->core.position.x38 += vx;
    p->core.position.y3c += vy;
    p->core.position.z40 += vz;
    return landed_next;
}
static size_t query(void *c, AoeLiveUnit *p, float step, AoeLiveUnit **out,
                    size_t cap) {
    (void)c; (void)p;
    query_step = step;
    for (size_t i = 0; i < hit_count && i < cap; ++i) out[i] = hit_list[i];
    return hit_count;
}
static void hit(void *c, AoeLiveUnit *s, AoeLiveUnit *v, float x, float y, float z) {
    (void)c; (void)x; (void)y; (void)z;
    ++hits;
    last_striker = s;
    last_victim = v;
}
static void blast(void *c, AoeLiveUnit *s, float x, float y, float z) {
    (void)c; (void)x; (void)y; (void)z;
    ++blasts;
    last_striker = s;
}
static int16_t owner_id(void *c, AoeLiveUnit *u) {
    (void)c;
    return u == &foe ? 2 : 1;
}
static int same_owner(void *c, AoeLiveUnit *a, AoeLiveUnit *b) {
    return owner_id(c, a) == owner_id(c, b);
}

int main(void) {
    AoeProjectileOps ops;
    AoeProjectileRor p;
    memset(&ops, 0, sizeof(ops));
    ops.move.resolve = resolve;
    ops.move.target_released = released;
    ops.move.target_attached = attached;
    ops.tick_seconds = tick; ops.target_speed = tspeed;
    ops.target_heading = theading; ops.capture_launcher = capture;
    ops.destroy = destroy; ops.set_facing = set_facing; ops.fly = fly;
    ops.hit_query = query; ops.hit = hit; ops.blast = blast;
    ops.owner_id = owner_id; ops.same_owner = same_owner;

    proj.core.unit_template = &proj_t;
    archer.core.unit_template = &archer_t;
    archer.core.unit_id = 1;
    foe.core.unit_template = &foe_t;
    foe.core.unit_id = 2;
    friend_unit.core.unit_template = &foe_t;
    friend_unit.core.unit_id = 3;
    proj_t.speed = 2.0f;
    proj_t.size[0] = proj_t.size[1] = 0.1f;
    foe_t.size[0] = foe_t.size[1] = 0.2f;
    foe_t.size[2] = 2.0f;
    foe.core.position.x38 = 10.0f;
    foe.core.position.y3c = 4.0f;

    /* Construction and aiming (mode 0, no arc): the direction is the unit
       vector to the target's centre height (z + 1.0). */
    assert(aoe_projectile_init_ror(&p, &proj, &archer, &foe, 0, 0, 0, 20.0f,
                                   &ops));
    assert(p.base.type == 8 && p.launcher == &archer &&
           p.base.secondary_id == 1 && p.base.target == &foe);
    proj.core.position.y3c = 4.0f;
    proj.core.position.z40 = 1.0f;
    assert(aoe_projectile_activate_ror(&p, 1, &ops) && p.base.state == 4 &&
           p.dir_x == 1.0f && p.dir_y == 0.0f && p.dir_z == 0.0f &&
           facing == 0.0f && p.traveled == 0.0f);

    /* One tick: move 2 along x; no hit, not landed. */
    assert(aoe_projectile_update_ror(&p, &ops) == 0 &&
           proj.core.position.x38 == 2.0f && p.traveled == 2.0f &&
           query_step == 2.0f && !hits);

    /* Hit mode 0 strikes only the target, through the launcher. */
    hit_list[0] = &friend_unit;
    hit_list[1] = &foe;
    hit_count = 2;
    assert(aoe_projectile_update_ror(&p, &ops) == 0 && hits == 1 &&
           last_striker == &archer && last_victim == &foe &&
           p.base.state == 1 && destroyed == 1);
    assert(aoe_projectile_update_ror(&p, &ops) == 1);

    /* Hit mode 2 with a dead launcher: the projectile strikes foreign units
       and blasts at its own side's; the launcher's data is captured. */
    proj_t.projectile_hit_mode = 2;
    assert(aoe_projectile_init_ror(&p, &proj, &archer, &foe, 0, 0, 0, 20.0f,
                                   &ops));
    proj.core.position.x38 = 0.0f;
    assert(aoe_projectile_activate_ror(&p, 1, &ops));
    archer.core.state = 3;
    hits = blasts = 0;
    assert(aoe_projectile_update_ror(&p, &ops) == 0 && captured == 1 &&
           !p.launcher && hits == 1 && blasts == 1 &&
           last_striker == &proj);
    archer.core.state = 0;
    hit_count = 0;

    /* Range expiry: traveled reaches the launch radius. */
    proj_t.projectile_hit_mode = 0;
    assert(aoe_projectile_init_ror(&p, &proj, &archer, &foe, 0, 0, 0, 3.0f,
                                   &ops));
    proj.core.position.x38 = 0.0f;
    assert(aoe_projectile_activate_ror(&p, 1, &ops));
    destroyed = 0;
    assert(aoe_projectile_update_ror(&p, &ops) == 0 && !destroyed);
    assert(aoe_projectile_update_ror(&p, &ops) == 0 && destroyed == 1 &&
           p.base.state == 1);

    /* Landing blasts and stops. */
    assert(aoe_projectile_init_ror(&p, &proj, &archer, NULL, 5, 4, 1, 50.0f,
                                   &ops));
    assert(aoe_projectile_activate_ror(&p, 1, &ops));
    landed_next = 1;
    blasts = 0;
    assert(aoe_projectile_update_ror(&p, &ops) == 0 && blasts == 1 &&
           last_striker == &archer && p.base.state == 1);
    landed_next = 0;

    /* A point marked -1 does not aim. */
    assert(aoe_projectile_init_ror(&p, &proj, &archer, NULL, -1, 4, 1, 50.0f,
                                   &ops));
    assert(aoe_projectile_activate_ror(&p, 1, &ops) && p.base.state == 1);

    /* Arc: distance 4 at speed 2 -> half time 1; arc 0.5 gives gravity
       -4 and climb 4; one tick adds (-4 * 0.5 + 4) = 2 to z. */
    proj_t.projectile_arc = 0.5f;
    proj.core.position.x38 = 6.0f;
    proj.core.position.z40 = 1.0f;
    assert(aoe_projectile_init_ror(&p, &proj, &archer, &foe, 0, 0, 0, 50.0f,
                                   &ops));
    assert(aoe_projectile_activate_ror(&p, 1, &ops) && p.gravity == -4.0f &&
           p.climb == 4.0f);
    assert(aoe_projectile_update_ror(&p, &ops) == 0 &&
           proj.core.position.z40 == 3.0f && p.climb == 0.0f);
    proj_t.projectile_arc = 0.0f;

    /* Leading: a target moving at 1 along x, 4 away, projectile speed 2:
       d2 = 5, denom = 4 - 5 + 2 = 1 > 1? no (needs > speed / 2 = 1). */
    foe_speed = 1.0f;
    proj_t.projectile_lead = 1;
    proj.core.position.x38 = 6.0f;
    proj.core.position.z40 = 1.0f;
    assert(aoe_projectile_init_ror(&p, &proj, &archer, &foe, 0, 0, 0, 50.0f,
                                   &ops));
    assert(aoe_projectile_activate_ror(&p, 1, &ops) && p.dir_x == 1.0f);
    /* Toward the projectile instead: d2 = 3, denom = 3, t = 4/3, the aim
       moves back 4/3 * 1, still along +x. */
    foe.core.position.x38 = 2.0f;
    assert(aoe_projectile_init_ror(&p, &proj, &archer, &foe, 0, 0, 0, 50.0f,
                                   &ops));
    assert(aoe_projectile_activate_ror(&p, 1, &ops) && p.dir_x == -1.0f);

    /* Mode 3 strikes the target at once. */
    proj_t.projectile_flight_mode = 3;
    foe_speed = 0.0f;
    hits = 0;
    assert(aoe_projectile_init_ror(&p, &proj, &archer, &foe, 0, 0, 0, 50.0f,
                                   &ops));
    assert(aoe_projectile_activate_ror(&p, 1, &ops) && p.dir_x == -4.0f);
    assert(aoe_projectile_update_ror(&p, &ops) == 0 && hits == 1 &&
           proj.core.position.x38 == 2.0f && p.base.state == 1);

    /* Overlap test. */
    proj.core.position.x38 = 2.25f;
    proj.core.position.z40 = foe.core.position.z40 = 0.0f;
    assert(aoe_projectile_overlaps_ror(&proj, &foe, 0.0f));
    proj.core.position.x38 = 2.5f;
    assert(!aoe_projectile_overlaps_ror(&proj, &foe, 0.0f) &&
           aoe_projectile_overlaps_ror(&proj, &foe, 0.25f));
    proj.core.position.z40 = 2.2f; /* above the 2.0-high target + 0.1 */
    assert(!aoe_projectile_overlaps_ror(&proj, &foe, 1.0f));
    return 0;
}
