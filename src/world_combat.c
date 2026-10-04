#include "world_combat.h"

#include <stdlib.h>
#include <string.h>

#include "sprite_runtime.h"
#include "aoe_ftol.h"
#include "terrain.h"
#include "unit_sprite_binding.h"
#include "unit_task.h"
#include "building_update.h"

/* Every callback context is the AoeWorldCombatRor. */
static AoeWorldMotionRor *motion_of(void *c) {
    return ((AoeWorldCombatRor *)c)->motion;
}

static AoeLiveUnit *resolve(void *c, int32_t id) {
    AoeVisibilityUnitRef *ref = aoe_visibility_registry_resolve(
        &motion_of(c)->runtime->world.view, id);
    return ref ? (AoeLiveUnit *)((char *)ref -
                                 offsetof(AoeLiveUnitCore, position))
               : NULL;
}

static const DatGraphic *graphic_by_id(AoeWorldCombatRor *c, int16_t id) {
    const DatGraphics *g = c->motion->runtime->graphics;
    if (!g || id < 0 || (uint16_t)id >= g->count || !g->graphics[id].present)
        return NULL;
    return &g->graphics[id];
}

AoeWorldCombatUnitRor *aoe_world_combat_unit_ror(AoeWorldCombatRor *c,
                                                 AoeLiveUnit *unit);

static void head_destroy(void *context, void *action) {
    (void)context; (void)action;
}
static void head_activate(void *context, void *action, uint8_t state) {
    (void)context; (void)action; (void)state;
}

/* Action-list callbacks for the move sub-action. */
static void sub_destroy(void *context, void *action) {
    (void)context;
    free(action);
}
static void sub_activate(void *context, void *action, uint8_t state);

AoeWorldCombatUnitRor *aoe_world_combat_unit_ror(AoeWorldCombatRor *c,
                                                 AoeLiveUnit *unit) {
    int64_t slot;
    AoeWorldCombatUnitRor *s;
    if (!c || !c->units || !unit) return NULL;
    slot = (int64_t)unit->core.unit_id + AOE_WORLD_MOTION_ID_OFFSET;
    if (slot < 0 || slot >= AOE_WORLD_MOTION_ID_SLOTS) return NULL;
    s = c->units[slot];
    if (s && s->unit == unit) return s;
    if (!s) {
        s = calloc(1, sizeof(*s));
        if (!s) return NULL;
        c->units[slot] = s;
    } else {
        aoe_action_list_clear(&s->sub_actions);
        aoe_action_list_clear(&s->unit_actions);
        if (s->unit && s->unit->derived_action_list == &s->unit_actions)
            s->unit->derived_action_list = NULL;
        memset(s, 0, sizeof(*s));
    }
    s->unit = unit;
    s->unit_actions.owner_state = &unit->core.state;
    s->unit_actions.ops.context = c;
    s->unit_actions.ops.destroy = head_destroy;
    s->unit_actions.ops.set_state = head_activate;
    if (!unit->derived_action_list &&
        !(unit->core.unit_template && unit->core.unit_template->type == 80))
        unit->derived_action_list = &s->unit_actions;
    s->sub_actions.owner_state = &unit->core.state;
    s->sub_actions.ops.context = c;
    s->sub_actions.ops.destroy = sub_destroy;
    s->sub_actions.ops.set_state = sub_activate;
    return s;
}

/* Base action hooks shared by the move and attack actions. */
static void target_released(void *c, AoeLiveUnit *t, AoeLiveUnit *u) {
    (void)c; (void)t; (void)u;
}
static void target_attached(void *c, AoeLiveUnit *t, AoeLiveUnit *u,
                            int16_t type) {
    (void)c; (void)t; (void)u; (void)type;
}
static void clear_secondary(void *c, AoeMoveActionRor *a) {
    (void)c;
    a->secondary_id = -1;
}
static void unit_notice(void *c, AoeLiveUnit *u, int32_t code, int32_t value,
                        int32_t argument) {
    (void)u; (void)code; (void)value; (void)argument;
    ++((AoeWorldCombatRor *)c)->notices;
}
static void select_graphic(void *c, AoeLiveUnit *u, const void *graphic) {
    AoeWorldCombatRor *w = c;
    const DatUnit *t = u->core.unit_template;
    AoeUnitGraphicTemplate gt;
    gt.standing_graphic = graphic_by_id(w, t->standing_graphic);
    gt.walking_graphic = graphic_by_id(w, t->walking_graphic);
    (void)aoe_unit_graphic_select_runtime(&u->graphic, &gt, &u->sprites,
                                          graphic, AOE_PLAYER_RISE_OF_ROME);
}
static const void *standing_graphic(void *c, AoeLiveUnit *u) {
    return graphic_by_id(c, u->core.unit_template->standing_graphic);
}
static int in_range(void *c, AoeLiveUnit *u, AoeLiveUnit *t, float range) {
    (void)c;
    return aoe_combat_in_range_ror(u, t, range);
}
static float speed(void *c, AoeLiveUnit *u) {
    AoePathWorldOps o;
    aoe_world_motion_path_ops_ror(motion_of(c), u, &o);
    return o.speed(o.context, u);
}
static void order_flag(void *c, AoeLiveUnit *u, int32_t v) {
    (void)c; (void)u; (void)v;
}
static AoeMovementRor *movement_of(void *c, AoeLiveUnit *u) {
    AoeWorldMotionUnitRor *s = aoe_world_motion_unit_ror(motion_of(c), u);
    return s ? &s->movement : NULL;
}
static void set_destination(void *c, AoeLiveUnit *u, float x, float y) {
    AoeMovementRor *m = movement_of(c, u);
    if (m) aoe_movement_set_destination_ror(m, x, y, 1.0f);
}
static int waypoints_stale(void *c, AoeLiveUnit *u) {
    AoeMovementRor *m = movement_of(c, u);
    return m ? m->waypoints_stale : 1;
}
static void record_tile(void *c, AoeLiveUnit *u, const uint8_t tile[3]) {
    AoeMovementRor *m = movement_of(c, u);
    if (m) (void)aoe_movement_record_waypoint_ror(m, tile);
}
static void set_range(void *c, AoeLiveUnit *u, float r) {
    AoeMovementRor *m = movement_of(c, u);
    if (m) aoe_movement_set_range_ror(m, r);
}
static void set_target_id(void *c, AoeLiveUnit *u, int32_t id) {
    AoeMovementRor *m = movement_of(c, u);
    if (m) aoe_movement_set_target_id_ror(m, id);
}
static void set_target_size(void *c, AoeLiveUnit *u, float sx, float sy) {
    AoeMovementRor *m = movement_of(c, u);
    if (m) aoe_movement_set_target_size_ror(m, sx, sy);
}
static int start(void *c, AoeLiveUnit *u) {
    AoeWorldMotionRor *mo = motion_of(c);
    AoeWorldMotionUnitRor *s = aoe_world_motion_unit_ror(mo, u);
    AoeMovementTickOps o;
    int16_t owner = u->runtime_owner_index;
    if (!s || owner < 0 || owner >= AOE_SCENARIO_RUNTIME_PLAYERS) return -1;
    aoe_world_motion_tick_ops_ror(mo, &o);
    return aoe_movement_start_ror(u, &s->movement, &mo->budgets[owner],
                                  &o.step.base);
}
static int status(void *c, AoeLiveUnit *u) {
    return aoe_world_motion_tick_ror(motion_of(c), u,
                                     ((AoeWorldCombatRor *)c)->tick);
}

static void move_ops(AoeWorldCombatRor *c, AoeMoveActionOps *o) {
    memset(o, 0, sizeof(*o));
    o->context = c;
    o->resolve = resolve;
    o->target_released = target_released;
    o->target_attached = target_attached;
    o->clear_secondary = clear_secondary;
    o->unit_notice = unit_notice;
    o->select_graphic = select_graphic;
    o->standing_graphic = standing_graphic;
    o->in_range = in_range;
    o->speed = speed;
    o->order_flag = order_flag;
    o->set_destination = set_destination;
    o->waypoints_stale = waypoints_stale;
    o->record_tile = record_tile;
    o->set_range = set_range;
    o->set_target_id = set_target_id;
    o->set_target_size = set_target_size;
    o->start = start;
    o->status = status;
}

static void sub_activate(void *context, void *action, uint8_t state) {
    AoeMoveActionOps o;
    (void)state;
    move_ops(context, &o);
    (void)aoe_move_action_activate_ror(action, &o);
}

/* Sprite queries on the unit's current graphic (0x004158f0/0x00415920). */
static const AoeSpriteInstance *current_sprite(AoeLiveUnit *u) {
    const void *g = u->graphic.current_graphic;
    for (size_t i = 0;; ++i) {
        const AoeSpriteInstance *s = aoe_sprite_runtime_at(&u->sprites, i);
        if (!s) return NULL;
        if (s->graphic == g) return s;
    }
}

/* Attack action callbacks. */
static int can_attack(void *c, AoeLiveUnit *u, AoeLiveUnit *t, float x,
                      float y) {
    AoeWorldCombatRor *w = c;
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(w, u);
    AoeMovementRor *m = movement_of(c, u);
    const DatGraphic *g = u->graphic.current_graphic;
    AoeMovementSpriteRor sp;
    if (!s || !m) return 0;
    if (t) {
        x = t->core.position.x38;
        y = t->core.position.y3c;
    }
    if (!g)
        return aoe_movement_face_ror(u, m, NULL, x, y, w->world_ms,
                                     &s->last_turn);
    sp.speed_multiplier = g->speed_multiplier;
    sp.facets = (int16_t)g->angle_count;
    sp.flags = g->sequence_type;
    return aoe_movement_face_ror(u, m, &sp, x, y, w->world_ms,
                                 &s->last_turn);
}
static float dist_point(void *c, AoeLiveUnit *u, float x, float y, float z) {
    (void)c; (void)z;
    return (float)aoe_combat_point_distance_ror(u, x, y);
}
static float dist_target(void *c, AoeLiveUnit *u, AoeLiveUnit *t) {
    (void)c;
    return (float)aoe_combat_distance_ror(u, t);
}
static int group_holds(void *c, AoeLiveUnit *u) { (void)c; (void)u; return 0; }
static void group_clear(void *c, AoeLiveUnit *u) { (void)c; (void)u; }
static int16_t owner_id(void *c, AoeLiveUnit *u) {
    (void)c;
    return u->runtime_owner_index;
}
static int refuses_owner(void *c, AoeLiveUnit *u, int16_t other) {
    (void)c; (void)u; (void)other;
    return 0;
}
static uint8_t visibility(void *c, AoeLiveUnit *u, int32_t x, int32_t y) {
    (void)c; (void)u; (void)x; (void)y;
    return 0x0f;
}
static int attached(void *c, AoeLiveUnit *u) {
    (void)c;
    return u->placement.attached_unit != NULL;
}
static uint8_t ready(void *c, AoeLiveUnit *u) {
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(c, u);
    return (uint8_t)(s && aoe_combat_ready_ror(s->reload_timer));
}
static void reset_reload(void *c, AoeLiveUnit *u) {
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(c, u);
    if (s) s->reload_timer = aoe_combat_reload_ror(u->core.unit_template);
}
static uint8_t sprite_done(void *c, AoeLiveUnit *u) {
    (void)c;
    return (uint8_t)aoe_sprite_instance_looped_ror(current_sprite(u));
}
static uint8_t sprite_reached(void *c, AoeLiveUnit *u, int32_t frame) {
    (void)c;
    return (uint8_t)aoe_sprite_instance_reached_ror(current_sprite(u),
                                                    (int16_t)frame);
}
static int16_t sprite_frames(void *c, AoeLiveUnit *u) {
    const DatGraphic *g = u->graphic.current_graphic;
    (void)c;
    return g ? (int16_t)g->frame_count : 0;
}
static uint8_t sprite_flags(void *c, AoeLiveUnit *u) {
    const DatGraphic *g = u->graphic.current_graphic;
    (void)c;
    return g ? g->sequence_type : 0;
}

/* Combat hit callbacks. */
static int32_t crt_rand(void *c) {
    return aoe_rng_next(&motion_of(c)->runtime->rng);
}
static void attack_notice(void *c, AoeLiveUnit *a, int32_t id) {
    (void)a; (void)id;
    ++((AoeWorldCombatRor *)c)->notices;
}
static void blast(void *c, AoeLiveUnit *a, float x, float y, float z,
                  AoeLiveUnit *source, AoeLiveUnit *excluded);
static float hit_multiplier(void *c, AoeLiveUnit *a, AoeLiveUnit *t) {
    (void)c;
    if (a->core.unit_template && a->core.unit_template->type >= 70)
        return aoe_combat_multiplier_tribe_ror(a, t, 0);
    return 1.0f;
}
static void damaged(void *c, AoeLiveUnit *t, AoeLiveUnit *a, int32_t hp,
                    int32_t max) {
    (void)t; (void)a; (void)hp; (void)max;
    ++((AoeWorldCombatRor *)c)->notices;
}
static void credit_kill(void *c, void *player) {
    (void)player;
    ++((AoeWorldCombatRor *)c)->kills;
}
static int spares_class(void *c, AoeLiveUnit *v, int16_t cls) {
    (void)c;
    return aoe_unit_keeps_resource_ror(v, cls); /* 0x004afb90 */
}
/* 0x004aace0(0, 0, 1): carried amount 0, clamped to template +0x7c. */
static void clear_carried(void *c, AoeLiveUnit *v) {
    (void)c;
    aoe_unit_add_carried_ror(v, 0.0f, 0, 1);
}
static void killed_notice(void *c, AoeLiveUnit *v, AoeLiveUnit *a) {
    (void)v; (void)a;
    ++((AoeWorldCombatRor *)c)->kills;
}
static int credit_attribute(void *c, void *player, int16_t attribute) {
    (void)c; (void)player; (void)attribute;
    return 1;
}
static void receive(void *c, AoeLiveUnit *t, const DatUnitAttackArmor *at,
                    int32_t n, float m, AoeLiveUnit *a) {
    AoeWorldCombatRor *w = c;
    AoeCombatReceiveOps ro = {w,           damaged,       owner_id,
                              credit_kill, spares_class,  clear_carried,
                              killed_notice, credit_attribute};
    float terrain = 1.0f; /* terrain defense bonus table: not bound yet */
    uint8_t type = t->core.unit_template ? t->core.unit_template->type : 0;
    ++w->hits;
    if (type == 70 && a)
        (void)aoe_combat_receive_unit_ror(t, terrain, at, n, m, NULL, a,
                                          &ro);
    else if (type == 80)
        (void)aoe_combat_receive_building_ror(t, terrain, at, n, m, NULL, a,
                                              &ro);
    else
        (void)aoe_combat_receive_ror(t, terrain, at, n, m, a, &ro);
}
static void destroy_target(void *c, AoeLiveUnit *t) { (void)c; (void)t; }

static float tick_seconds(void *c);

/* Units on a map tile, in tile-list order (map +0x8d8c rows). */
static size_t tile_units(void *c, int32_t x, int32_t y, AoeLiveUnit **out,
                         size_t capacity) {
    const AoeRuntimeMap *map = &motion_of(c)->runtime->map;
    size_t n = 0;
    if (x < 0 || y < 0 || (uint32_t)x >= map->width ||
        (uint32_t)y >= map->height)
        return 0;
    for (AoeObjectUpdateNode *node =
             map->objects[(size_t)y * map->width + (size_t)x].head;
         node && n < capacity; node = node->next)
        if (node->object) out[n++] = (AoeLiveUnit *)node->object;
    return n;
}
static int same_owner(void *c, AoeLiveUnit *a, AoeLiveUnit *b) {
    (void)c;
    return a->runtime_owner_index == b->runtime_owner_index;
}
static int is_tribe(const AoeLiveUnit *u) {
    return u->core.unit_template && u->core.unit_template->type >= 70;
}
static void blast(void *c, AoeLiveUnit *a, float x, float y, float z,
                  AoeLiveUnit *source, AoeLiveUnit *excluded) {
    AoeWorldCombatRor *w = c;
    const AoeRuntimeMap *map = &w->motion->runtime->map;
    AoeCombatBlastOps b = {w,          crt_rand,  (int32_t)map->width,
                           (int32_t)map->height, tile_units, hit_multiplier,
                           receive,    same_owner, clear_carried};
    ++w->blasts;
    (void)aoe_combat_blast_ror(a, x, y, z, source, excluded, is_tribe(a), &b);
}

/* Striker vtable +0x22c: tribe 0x004aeca0 or base 0x004267b0. */
static void melee_hit(AoeWorldCombatRor *w, AoeLiveUnit *striker,
                      AoeLiveUnit *victim, float x, float y, float z) {
    AoeCombatHitOps h = {w, crt_rand, attack_notice, blast, hit_multiplier,
                         receive, 0, destroy_target};
    int r = is_tribe(striker)
                ? aoe_combat_melee_hit_tribe_ror(striker, victim, x, y, z, &h)
                : aoe_combat_melee_hit_ror(striker, victim, x, y, z, &h);
    if (r != 1) ++w->misses;
}

/* Projectile flight callbacks. */
static AoeWorldProjectileRor *flight_of(AoeWorldCombatRor *w,
                                        const AoeLiveUnit *p) {
    for (size_t i = 0; i < w->flight_count; ++i)
        if (w->flights[i].unit == p) return &w->flights[i];
    return NULL;
}
static float target_speed(void *c, AoeLiveUnit *t) {
    AoeWorldMotionUnitRor *s = aoe_world_motion_unit_ror(motion_of(c), t);
    return s ? s->movement.current_speed : 0.0f; /* +0xf4 = +0x88 */
}
static float target_heading(void *c, AoeLiveUnit *t) {
    AoeWorldMotionUnitRor *s = aoe_world_motion_unit_ror(motion_of(c), t);
    return s ? s->movement.facing : 0.0f; /* +0xf8 = +0x9c */
}
/* 0x004495b0: keep the launcher's attack data in a private template. */
static void capture_launcher(void *c, AoeLiveUnit *p, AoeLiveUnit *l) {
    AoeWorldProjectileRor *f = flight_of(c, p);
    const DatUnit *lt = l->core.unit_template;
    DatUnit *copy;
    if (!f || f->captured || !p->core.unit_template || !lt) return;
    copy = malloc(sizeof(*copy));
    if (!copy) return;
    *copy = *p->core.unit_template;
    copy->attack_count = lt->attack_count;
    copy->attacks = NULL;
    if (lt->attack_count) {
        f->captured_attacks = malloc(lt->attack_count * sizeof(*lt->attacks));
        if (!f->captured_attacks) {
            free(copy);
            return;
        }
        memcpy(f->captured_attacks, lt->attacks,
               lt->attack_count * sizeof(*lt->attacks));
        copy->attacks = f->captured_attacks;
    }
    copy->blast_width = lt->blast_width;
    copy->blast_level = lt->blast_level;
    copy->terrain_defense_bonus = lt->terrain_defense_bonus;
    f->captured = copy;
    f->original = p->core.unit_template;
    p->core.unit_template = copy;
}
/* Projectile +0x6c = 0x004aa320: the hit points drop to 0 (its health flag
   follows at the next state change) and the state switch retires it. */
static void destroy_projectile(void *c, AoeLiveUnit *p) {
    (void)c;
    p->core.hit_points = 0.0f;
}
static void set_facing(void *c, AoeLiveUnit *p, float angle) {
    AoeMovementRor *m = movement_of(c, p);
    const DatGraphic *g = p->graphic.current_graphic;
    if (!m) return;
    m->facing = angle; /* +0xb4 = 0x0044be40 */
    if (g) {
        AoeMovementSpriteRor sp = {g->speed_multiplier,
                                   (int16_t)g->angle_count, g->sequence_type};
        aoe_movement_update_facet_ror(p, m, &sp);
    }
}
/* 0x0044be10 + 0x004491d0 + placement 0x004a98c0: move by the velocity,
   destroy off-map, land when the placed z equals the terrain surface. */
static int fly(void *c, AoeLiveUnit *p, float vx, float vy, float vz) {
    AoeWorldCombatRor *w = c;
    AoeScenarioRuntimeRor *r = w->motion->runtime;
    AoeMovementRor *m = movement_of(c, p);
    float x, y, z, surface;
    int16_t o = p->runtime_owner_index;
    if (m) {
        m->velocity_x = vx;
        m->velocity_y = vy;
        m->velocity_z = vz;
    }
    x = (float)((double)vx + p->core.position.x38);
    y = (float)((double)vy + p->core.position.y3c);
    z = (float)((double)vz + p->core.position.z40);
    if (x < 0.0f || !((float)r->map.width > x) || y < 0.0f ||
        !((float)r->map.height > y)) {
        destroy_projectile(c, p);
        return 0;
    }
    if (o < 0 || o >= AOE_SCENARIO_RUNTIME_PLAYERS ||
        !aoe_live_unit_place_ror_with_vision(
            &p->core, &p->placement, r->map.tiles, r->map.objects, r->geometry,
            r->map.width, r->map.height, x, y, z,
            &r->players[o].vision_binding) ||
        !terrain_surface_height_at(r->map.tiles, r->map.width, r->map.height,
                                   p->core.position.x38,
                                   p->core.position.y3c, &surface))
        return 0;
    return surface == p->core.position.z40;
}
/* 0x00449280: tiles ftol(x -/+ 3) by ftol(y -/+ 3), clamped. */
static size_t hit_query(void *c, AoeLiveUnit *p, float step,
                        AoeLiveUnit **out, size_t capacity) {
    const AoeRuntimeMap *map = &motion_of(c)->runtime->map;
    int32_t x0 = aoe_ftol32((double)p->core.position.x38 - 3.0);
    int32_t y0 = aoe_ftol32((double)p->core.position.y3c - 3.0);
    int32_t x1 = aoe_ftol32((double)p->core.position.x38 + 3.0);
    int32_t y1 = aoe_ftol32((double)p->core.position.y3c + 3.0);
    size_t n = 0;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= (int32_t)map->width) x1 = (int32_t)map->width - 1;
    if (y1 >= (int32_t)map->height) y1 = (int32_t)map->height - 1;
    for (int32_t y = y0; y <= y1; ++y)
        for (int32_t x = x0; x <= x1; ++x) {
            AoeLiveUnit *units[64];
            size_t k = tile_units(c, x, y, units, 64);
            for (size_t i = 0; i < k && n < capacity; ++i)
                if (aoe_projectile_overlaps_ror(p, units[i], step))
                    out[n++] = units[i];
        }
    return n;
}
static void projectile_hit(void *c, AoeLiveUnit *striker,
                           AoeLiveUnit *victim, float x, float y, float z) {
    melee_hit(c, striker, victim, x, y, z);
}
static void projectile_blast(void *c, AoeLiveUnit *striker, float x, float y,
                             float z) {
    blast(c, striker, x, y, z, striker, NULL);
}

static void projectile_ops(AoeWorldCombatRor *c, AoeProjectileOps *o) {
    memset(o, 0, sizeof(*o));
    move_ops(c, &o->move);
    o->tick_seconds = tick_seconds;
    o->target_speed = target_speed;
    o->target_heading = target_heading;
    o->capture_launcher = capture_launcher;
    o->destroy = destroy_projectile;
    o->set_facing = set_facing;
    o->fly = fly;
    o->hit_query = hit_query;
    o->hit = projectile_hit;
    o->blast = projectile_blast;
    o->owner_id = owner_id;
    o->same_owner = same_owner;
}

/* 0x00426520 for a ranged unit: launch a projectile unit at the rotated
   spawn offsets (0x00449070 at a target). */
static int launch(AoeWorldCombatRor *w, AoeLiveUnit *u, AoeLiveUnit *t) {
    AoeScenarioRuntimeRor *r = w->motion->runtime;
    const DatUnit *tmpl = u->core.unit_template;
    AoeMovementRor *m = movement_of(w, u);
    AoeLiveUnit *p = NULL;
    AoeWorldProjectileRor *f;
    AoeProjectileOps o;
    float x, y, z, radius;
    int16_t owner = u->runtime_owner_index;
    if (!m || owner < 0 || owner >= AOE_SCENARIO_RUNTIME_PLAYERS) return 0;
    aoe_combat_projectile_origin_ror(u, m->facing, &x, &y, &z, &radius);
    if (!aoe_scenario_runtime_editor_create_ror(
            r, (uint8_t)owner, (uint16_t)tmpl->projectile_unit_id, x, y, z,
            &p) || !p)
        return 0;
    if (w->flight_count == w->flight_capacity) {
        size_t cap = w->flight_capacity ? w->flight_capacity * 2 : 16;
        AoeWorldProjectileRor *grown =
            realloc(w->flights, cap * sizeof(*grown));
        if (!grown) return 0;
        w->flights = grown;
        w->flight_capacity = cap;
    }
    f = &w->flights[w->flight_count++];
    memset(f, 0, sizeof(*f));
    f->unit = p;
    f->active = 1;
    projectile_ops(w, &o);
    if (!aoe_projectile_init_ror(&f->flight, p, u, t, t->core.position.x38,
                                 t->core.position.y3c, t->core.position.z40,
                                 radius, &o) ||
        !aoe_projectile_activate_ror(&f->flight, 1, &o)) {
        f->active = 0;
        return 0;
    }
    ++w->projectiles;
    ++w->notices; /* attacker +0x148(.., 0x200, 0x258, -1, 0) */
    return 1;
}

static void fire_target(void *c, AoeLiveUnit *u, AoeLiveUnit *t) {
    AoeWorldCombatRor *w = c;
    const DatUnit *tmpl = u->core.unit_template;
    if (tmpl->projectile_unit_id >= 0) {
        /* 0x004aec50: alert a non-Gaia target owner first (counted). */
        if (is_tribe(u) && t && t->runtime_owner_index > 0) ++w->alerts;
        (void)launch(w, u, t);
        return;
    }
    melee_hit(w, u, t, u->core.position.x38, u->core.position.y3c,
              u->core.position.z40);
}
static void fire_point(void *c, AoeLiveUnit *u, float x, float y, float z) {
    (void)u; (void)x; (void)y; (void)z;
    ++((AoeWorldCombatRor *)c)->projectiles;
}

/* Sub-action list: 0x004052d0 / 0x00405250, push 0x00404400, quiet. */
static int start_move(void *c, AoeAttackActionRor *a) {
    AoeWorldCombatRor *w = c;
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(w, a->base.unit);
    AoeMoveActionOps o;
    AoeMoveActionRor *mv;
    const DatUnit *t = a->base.unit->core.unit_template;
    const void *walk = graphic_by_id(w, t->walking_graphic);
    int ok;
    if (!s) return 0;
    mv = calloc(1, sizeof(*mv));
    if (!mv) return 0;
    move_ops(w, &o);
    ok = a->base.target
             ? aoe_move_action_init_target_ror(mv, a->base.unit,
                                               a->base.target, a->range,
                                               a->move_graphic, walk, &o)
             : aoe_move_action_init_point_ror(mv, a->base.unit, a->base.x,
                                              a->base.y, a->base.z, a->range,
                                              a->move_graphic, walk);
    if (!ok) {
        free(mv);
        return 0;
    }
    mv->quiet = 1; /* 0x004076a0(1) after the push */
    if (!aoe_action_list_push(&s->sub_actions, mv)) {
        free(mv);
        return 0;
    }
    return 1;
}
static uint8_t update_head(void *c, void *action) {
    AoeMoveActionOps o;
    uint8_t r = 0;
    move_ops(c, &o);
    if (!aoe_move_action_update_ror(action, &o, &r)) return 0;
    return r;
}
static void update_waiting(void *c, void *action) { (void)c; (void)action; }
static uint8_t sub_update(void *c, AoeAttackActionRor *a) {
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(c, a->base.unit);
    AoeActionTickOps t = {c, update_head, update_waiting};
    uint8_t r = 0;
    if (!s || !aoe_action_list_update_ror(&s->sub_actions, &t, &r)) return 0;
    return r;
}
static void sub_clear(void *c, AoeAttackActionRor *a) {
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(c, a->base.unit);
    if (s) (void)aoe_action_list_clear(&s->sub_actions);
}
static float tick_seconds(void *c) { return ((AoeWorldCombatRor *)c)->tick; }

static void attack_ops(AoeWorldCombatRor *c, AoeAttackActionOps *o) {
    memset(o, 0, sizeof(*o));
    move_ops(c, &o->move);
    o->can_attack = can_attack;
    o->distance_to_point = dist_point;
    o->distance_to_target = dist_target;
    o->group_holds = group_holds;
    o->group_clear_target = group_clear;
    o->owner_id = owner_id;
    o->refuses_owner = refuses_owner;
    o->visibility = visibility;
    o->attached = attached;
    o->ready = ready;
    o->reset_reload = reset_reload;
    o->sprite_done = sprite_done;
    o->sprite_reached = sprite_reached;
    o->sprite_frames = sprite_frames;
    o->sprite_flags = sprite_flags;
    o->fire_target = fire_target;
    o->fire_point = fire_point;
    o->start_move = start_move;
    o->sub_update = sub_update;
    o->sub_clear = sub_clear;
    o->tick_seconds = tick_seconds;
}

int aoe_world_combat_init_ror(AoeWorldCombatRor *c, AoeWorldMotionRor *m) {
    if (!c || !m || !m->runtime) return 0;
    memset(c, 0, sizeof(*c));
    c->motion = m;
    c->units = calloc(AOE_WORLD_MOTION_ID_SLOTS, sizeof(*c->units));
    return c->units != NULL;
}

void aoe_world_combat_destroy_ror(AoeWorldCombatRor *c) {
    if (!c) return;
    for (size_t i = 0; i < c->flight_count; ++i) {
        AoeWorldProjectileRor *f = &c->flights[i];
        if (f->captured && f->unit &&
            f->unit->core.unit_template == f->captured)
            f->unit->core.unit_template = f->original;
        free(f->captured_attacks);
        free(f->captured);
    }
    free(c->flights);
    if (c->units) {
        for (size_t i = 0; i < AOE_WORLD_MOTION_ID_SLOTS; ++i)
            if (c->units[i]) {
                AoeWorldCombatUnitRor *s = c->units[i];
                if (s->unit && s->unit->building_update == &s->building_storage)
                    (void)aoe_live_unit_unbind_building_update_ror(
                        s->unit, NULL, NULL, NULL);
                aoe_action_list_clear(&s->sub_actions);
                aoe_action_list_clear(&s->unit_actions);
                if (s->unit && s->unit->derived_action_list == &s->unit_actions)
                    s->unit->derived_action_list = NULL;
                free(s);
            }
        free(c->units);
    }
    memset(c, 0, sizeof(*c));
}

int aoe_world_combat_attack_ror(AoeWorldCombatRor *c, AoeLiveUnit *attacker,
                                AoeLiveUnit *target) {
    AoeWorldCombatUnitRor *s;
    AoeAttackActionOps o;
    AoeAttackParamsRor p = {0}, d = {0};
    const DatUnit *t;
    const DatUnitTask *task = NULL;
    if (!c || !attacker || !target || !attacker->core.unit_template ||
        attacker->core.unit_template->type < 50 ||
        !(s = aoe_world_combat_unit_ror(c, attacker)))
        return 0;
    t = attacker->core.unit_template;
    for (uint16_t i = 0; i < t->task_count && !task; ++i)
        if (t->tasks[i].action_type == 7) task = &t->tasks[i];
    if (task) {
        p.move_graphic = graphic_by_id(c, task->moving_graphic);
        p.attack_graphic = graphic_by_id(c, task->proceeding_graphic);
        p.range = task->work_range;
    }
    p.min_range = t->min_range;
    p.projectile_id = t->projectile_unit_id;
    p.fire_frame = t->frame_delay;
    d.move_graphic = graphic_by_id(c, t->walking_graphic);
    d.attack_graphic = graphic_by_id(c, t->attack_graphic);
    d.idle_graphic = graphic_by_id(c, t->standing_graphic);
    attack_ops(c, &o);
    if (!aoe_attack_action_init_target_ror(&s->attack, attacker, target, &p,
                                           &d, t->max_range, &o))
        return 0;
    s->attacking = 1;
    return aoe_attack_action_activate_ror(&s->attack, &o);
}

/* Gather action bindings. */
static const void *graphic_id_cb(void *c, int16_t id) {
    return graphic_by_id(c, id);
}
static int32_t owner_kind_cb(void *c, AoeLiveUnit *u) {
    (void)c; (void)u;
    return 0;
}
static int face_target(void *c, AoeLiveUnit *u, AoeLiveUnit *t) {
    return can_attack(c, u, t, 0.0f, 0.0f);
}
static float distance_cb(void *c, AoeLiveUnit *u, AoeLiveUnit *t) {
    (void)c;
    return (float)aoe_combat_distance_ror(u, t);
}
static AoeScenarioRuntimePlayerRor *player_of(void *c, AoeLiveUnit *u) {
    int16_t o = u->runtime_owner_index;
    if (o < 0 || o >= AOE_SCENARIO_RUNTIME_PLAYERS) return NULL;
    return &motion_of(c)->runtime->players[o];
}
static float player_attribute(void *c, AoeLiveUnit *u, int16_t index) {
    AoeScenarioRuntimePlayerRor *p = player_of(c, u);
    if (!p || index < 0 || index >= p->resources.count) return 0.0f;
    return p->resources.values[index];
}
static void refresh_world(void *c) { (void)c; }
/* Player +0x78(type, amount, 1) through the derived 0x004f6c60, then the
   +0x5c deposit notice (counted). */
static void deposit_cb(void *c, AoeLiveUnit *u, int16_t type, float amount) {
    AoeWorldCombatRor *w = c;
    AoeScenarioRuntimePlayerRor *p = player_of(c, u);
    AoePlayerResourceTargetRor target;
    if (!p) return;
    target.resources = &p->resources;
    target.derived = 1;
    target.field_104 = &w->player_field_104[u->runtime_owner_index];
    target.context = w;
    target.refresh_world = refresh_world;
    (void)aoe_player_resource_add_virtual_ror(&target, type, amount, 1);
    ++w->deposits;
}
/* 0x004514d0 over the owner's units with state filter 2. Without a zone
   lookup every candidate is in the same zone. */
static AoeLiveUnit *find_drop_site(void *c, AoeLiveUnit *u, int16_t id1,
                                   int16_t id2, float x, float y,
                                   AoeLiveUnit *zone_unit) {
    AoeScenarioRuntimeRor *r = motion_of(c)->runtime;
    AoeScenarioRuntimePlayerRor *p = player_of(c, u);
    AoeLiveUnit *best = NULL;
    double best_d2 = -1.0;
    (void)zone_unit;
    if (!p) return NULL;
    for (size_t pass = 0; pass < 2; ++pass) {
        size_t n = pass ? r->editor_unit_count : p->unit_count;
        for (size_t i = 0; i < n; ++i) {
            AoeLiveUnit *o = pass ? r->editor_units[i] : &p->units[i];
            int16_t id;
            double dx, dy, d2;
            if (pass && o->runtime_owner_index != u->runtime_owner_index)
                continue;
            if (!o->constructed || !o->core.unit_template) continue;
            id = o->core.unit_template->id;
            if ((id != id1 && id != id2) || o->core.state != 2) continue;
            if (x == -1.0f && y == -1.0f) return o;
            dx = (double)x - o->core.position.x38;
            dy = (double)y - o->core.position.y3c;
            d2 = dy * dy + dx * dx;
            if (best_d2 == -1.0 || best_d2 > d2) {
                best_d2 = (float)d2;
                best = o;
            }
        }
    }
    return best;
}
static void play_sound_cb(void *c, int16_t id) { (void)c; (void)id; }
static int gather_start_move(void *c, AoeGatherActionRor *a, AoeLiveUnit *t,
                             float range, const void *graphic,
                             int quiet_first) {
    AoeWorldCombatRor *w = c;
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(w, a->base.unit);
    AoeMoveActionOps o;
    AoeMoveActionRor *mv;
    const DatUnit *tt = a->base.unit->core.unit_template;
    int ok;
    if (!s) return 0;
    mv = calloc(1, sizeof(*mv));
    if (!mv) return 0;
    move_ops(w, &o);
    ok = t ? aoe_move_action_init_target_ror(
                 mv, a->base.unit, t, range, graphic,
                 graphic_by_id(w, tt->walking_graphic), &o)
           : aoe_move_action_init_point_ror(
                 mv, a->base.unit, a->base.x, a->base.y, a->base.z, range,
                 graphic, graphic_by_id(w, tt->walking_graphic));
    if (!ok) {
        free(mv);
        return 0;
    }
    if (quiet_first) mv->quiet = 1;
    if (!aoe_action_list_push(&s->sub_actions, mv)) {
        free(mv);
        return 0;
    }
    mv->quiet = 1;
    return 1;
}
static uint8_t gather_sub_update(void *c, AoeGatherActionRor *a) {
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(c, a->base.unit);
    AoeActionTickOps t = {c, update_head, update_waiting};
    uint8_t r = 0;
    if (!s || !aoe_action_list_update_ror(&s->sub_actions, &t, &r)) return 0;
    return r;
}
static void gather_sub_clear(void *c, AoeGatherActionRor *a) {
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(c, a->base.unit);
    if (s) (void)aoe_action_list_clear(&s->sub_actions);
}

static void gather_ops(AoeWorldCombatRor *c, AoeGatherActionOps *o) {
    memset(o, 0, sizeof(*o));
    move_ops(c, &o->move);
    o->graphic = graphic_id_cb;
    o->tick_seconds = tick_seconds;
    o->owner_kind = owner_kind_cb;
    o->face = face_target;
    o->distance = distance_cb;
    o->player_attribute = player_attribute;
    o->deposit = deposit_cb;
    o->find_drop_site = find_drop_site;
    o->play_sound = play_sound_cb;
    o->start_move = gather_start_move;
    o->sub_update = gather_sub_update;
    o->sub_clear = gather_sub_clear;
}

/* Task selection owners (see the header). */
static int16_t task_owner(void *c, const AoeLiveUnit *u) {
    (void)c;
    return u->runtime_owner_index;
}
static int task_accepts(void *c, int16_t a, int16_t b) {
    (void)c;
    return a == b;
}
static int task_hostile(void *c, int16_t a, int16_t b) {
    (void)c;
    return a != b && b != 0;
}
static int32_t task_map_size(void *c, const AoeLiveUnit *u, int axis) {
    const AoeRuntimeMap *m = &motion_of(c)->runtime->map;
    (void)u;
    return (int32_t)(axis ? m->height : m->width);
}
static int task_terrain(void *c, const AoeLiveUnit *u, int16_t x, int16_t y,
                        uint8_t *terrain) {
    const AoeRuntimeMap *m = &motion_of(c)->runtime->map;
    (void)u;
    if (x < 0 || y < 0 || (uint32_t)x >= m->width ||
        (uint32_t)y >= m->height)
        return 0;
    *terrain = m->tiles[(size_t)y * m->width + (size_t)x].terrain_id;
    return 1;
}

int aoe_world_combat_gather_ror(AoeWorldCombatRor *c, AoeLiveUnit *unit,
                                AoeLiveUnit *target) {
    AoeWorldCombatUnitRor *s;
    AoeGatherActionOps o;
    AoeUnitTaskOps to = {c, task_owner, task_accepts, task_hostile,
                         task_map_size, task_terrain};
    const DatUnitTask *task = NULL;
    if (!c || !unit || !target || !(s = aoe_world_combat_unit_ror(c, unit)))
        return 0;
    if (!aoe_unit_task_select_tribe_ror(unit, target, -1.0f, -1.0f, &to,
                                        &task) ||
        !task || task->action_type != 5)
        return 0;
    gather_ops(c, &o);
    if (!aoe_gather_action_init_target_ror(&s->gather, unit, task, target,
                                           &o))
        return 0;
    s->attacking = 0;
    s->gathering = 1;
    return aoe_gather_action_activate_ror(&s->gather, 1, &o);
}

/* Build action bindings. */
static int wall_at(void *c, AoeLiveUnit *u, int32_t x, int32_t y) {
    AoeLiveUnit *units[64];
    size_t n = tile_units(c, x, y, units, 64);
    for (size_t i = 0; i < n; ++i)
        if (units[i]->runtime_owner_index == u->runtime_owner_index &&
            units[i]->core.unit_template &&
            units[i]->core.unit_template->unit_class == 0x1b &&
            units[i]->core.state <= 2)
            return 1;
    return 0;
}
static void list_destroy_noop(void *c, void *a) { (void)c; (void)a; }
static void list_activate_noop(void *c, void *a, uint8_t s) {
    (void)c; (void)a; (void)s;
}
static int bind_building(AoeWorldCombatRor *w, AoeLiveUnit *t) {
    AoeWorldCombatUnitRor *s;
    AoeScenarioRuntimePlayerRor *p;
    AoeActionListOps lo = {w, list_destroy_noop, list_activate_noop};
    if (t->building_update) return 1;
    if (!(s = aoe_world_combat_unit_ror(w, t)) || !(p = player_of(w, t)))
        return 0;
    if (t->derived_action_list == &s->unit_actions)
        t->derived_action_list = NULL;
    return aoe_live_unit_bind_building_update_ror(
        t, &s->building_storage, &s->building_live, lo, &s->building_queue,
        lo, p->resources, t->runtime_owner_index, 0);
}
static void no_unit_hook(void *c, AoeLiveUnit *u) { (void)c; (void)u; }
static void no_local(void *c, AoeLiveUnit *u, int v) { (void)c; (void)u; (void)v; }
static void disappear(void *c, AoeLiveUnit *u) {
    (void)c;
    u->core.hit_points = 0.0f;
}
typedef struct {
    AoeWorldCombatRor *combat;
} BuildStateContext;
static int building_set_state(void *opaque, AoeLiveUnit *t, uint8_t state) {
    AoeWorldCombatRor *w = ((BuildStateContext *)opaque)->combat;
    AoeScenarioRuntimeRor *r = w->motion->runtime;
    AoeScenarioRuntimePlayerRor *p = player_of(w, t);
    AoeBuildingCompletionVisualRuntimeOps visual;
    AoeBuildingCompletionWorldRuntimeOps world;
    if (!p || state != 2) return 0;
    memset(&visual, 0, sizeof(visual));
    visual.context = w;
    visual.rng = &r->rng;
    visual.graphics = r->graphics;
    visual.vision = &p->vision_binding;
    visual.local_completion = no_local;
    visual.building_pre_complete = no_unit_hook;
    visual.complete_adjacent = no_unit_hook;
    visual.disappear = disappear;
    if (!aoe_scenario_runtime_building_completion_world_ops_ror(r, NULL, NULL,
                                                                &world))
        return 0;
    if (!aoe_live_unit_scenario_building_complete_ror(
            t, &visual, &world, &p->resources, &p->owner, &r->world,
            &p->vision_binding))
        return 0;
    ++w->completions;
    return 1;
}
static int add_build_work(void *c, AoeLiveUnit *t, float amount,
                          int *completed) {
    BuildStateContext sc = {c};
    AoeLiveUnitSetStateOps so = {&sc, building_set_state};
    if (!t->core.unit_template || t->core.unit_template->type != 80 ||
        !bind_building(c, t))
        return 0;
    return aoe_live_unit_add_build_work_ror(t, amount, &so, completed);
}
static void reset_group_cb(void *c, AoeLiveUnit *u) {
    (void)aoe_live_unit_reset_group_ror(u, owner_kind_cb(c, u));
}
static void request_repair(void *c, AoeLiveUnit *u, AoeLiveUnit *t) {
    (void)u; (void)t;
    ++((AoeWorldCombatRor *)c)->repairs;
}
int aoe_world_combat_order_ror(AoeWorldCombatRor *c, AoeLiveUnit *u,
                               AoeLiveUnit *t, float x, float y, float z,
                               int16_t *code);
static void order_onto(void *c, AoeLiveUnit *u, AoeLiveUnit *t) {
    int16_t code = -1;
    (void)aoe_world_combat_order_ror(c, u, t, t->core.position.x38,
                                     t->core.position.y3c, 0.0f, &code);
}
static int build_start_move(void *c, AoeBuildActionRor *a, AoeLiveUnit *t,
                            float range, const void *graphic,
                            int quiet_first) {
    AoeWorldCombatRor *w = c;
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(w, a->base.unit);
    AoeMoveActionOps o;
    AoeMoveActionRor *mv;
    const DatUnit *tt = a->base.unit->core.unit_template;
    int ok;
    if (!s) return 0;
    mv = calloc(1, sizeof(*mv));
    if (!mv) return 0;
    move_ops(w, &o);
    ok = t ? aoe_move_action_init_target_ror(
                 mv, a->base.unit, t, range, graphic,
                 graphic_by_id(w, tt->walking_graphic), &o)
           : aoe_move_action_init_point_ror(
                 mv, a->base.unit, a->base.x, a->base.y, a->base.z, range,
                 graphic, graphic_by_id(w, tt->walking_graphic));
    if (!ok) {
        free(mv);
        return 0;
    }
    if (quiet_first) mv->quiet = 1;
    if (!aoe_action_list_push(&s->sub_actions, mv)) {
        free(mv);
        return 0;
    }
    mv->quiet = 1;
    return 1;
}
static uint8_t build_sub_update(void *c, AoeBuildActionRor *a) {
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(c, a->base.unit);
    AoeActionTickOps t = {c, update_head, update_waiting};
    uint8_t r = 0;
    if (!s || !aoe_action_list_update_ror(&s->sub_actions, &t, &r)) return 0;
    return r;
}
static void build_sub_clear(void *c, AoeBuildActionRor *a) {
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(c, a->base.unit);
    if (s) (void)aoe_action_list_clear(&s->sub_actions);
}
static void build_ops(AoeWorldCombatRor *c, AoeBuildActionOps *o) {
    memset(o, 0, sizeof(*o));
    move_ops(c, &o->move);
    o->graphic = graphic_id_cb;
    o->tick_seconds = tick_seconds;
    o->owner_kind = owner_kind_cb;
    o->face = face_target;
    o->wall_at = wall_at;
    o->add_build_work = add_build_work;
    o->reset_group = reset_group_cb;
    o->request_repair = request_repair;
    o->order_onto = order_onto;
    o->play_sound = play_sound_cb;
    o->start_move = build_start_move;
    o->sub_update = build_sub_update;
    o->sub_clear = build_sub_clear;
}

/* 0x004aa160 bound to the runtime. */
int aoe_world_combat_change_template_ror(AoeWorldCombatRor *c,
                                         AoeLiveUnit *u, const DatUnit *t) {
    AoeScenarioRuntimeRor *r;
    AoeWorldMotionUnitRor *ms;
    AoeLiveUnitVisionBinding *vision;
    float ratio;
    int16_t o;
    int on_map;
    if (!c || !u || !t || !u->core.unit_template) return 0;
    if (u->core.state > 2) return 1;
    r = c->motion->runtime;
    o = u->runtime_owner_index;
    if (o < 0 || o >= AOE_SCENARIO_RUNTIME_PLAYERS) return 0;
    vision = &r->players[o].vision_binding;
    ms = aoe_world_motion_unit_ror(c->motion, u);
    ratio = (float)((double)u->core.hit_points /
                    u->core.unit_template->hit_points);
    on_map = u->placement.tile != NULL && u->core.state == 2;
    if (on_map && !aoe_live_unit_vision_change_ror(&u->core, vision, 0, 0))
        return 0;
    if (ms)
        (void)aoe_unit_obstruction_lift_ror(u, &ms->obstruction,
                                            &c->motion->path, NULL, 0);
    u->core.unit_template = t;
    if (on_map && !aoe_live_unit_vision_change_ror(&u->core, vision, 1, 0))
        return 0;
    if (ms)
        (void)aoe_unit_obstruction_restore_ror(u, &ms->obstruction,
                                               &c->motion->path, NULL, 0);
    select_graphic(c, u, graphic_by_id(c, t->standing_graphic));
    u->core.hit_points = (float)((double)t->hit_points * ratio);
    u->core.state = 0;
    return aoe_live_unit_scenario_state_set_ror(
        u, 2, &r->players[o].resources, &r->players[o].owner, &r->world,
        vision);
}

/* Order step bindings. */
static int order_tribe_template(void *c, const DatUnit *t) {
    (void)c;
    return t->type >= 70;
}
static int16_t order_template_count(void *c, const AoeLiveUnit *u) {
    AoeScenarioRuntimePlayerRor *p = player_of(c, (AoeLiveUnit *)u);
    return p ? (int16_t)p->templates.count : 0;
}
static const DatUnit *order_template(void *c, const AoeLiveUnit *u,
                                     int16_t i) {
    AoeScenarioRuntimePlayerRor *p = player_of(c, (AoeLiveUnit *)u);
    return p && i >= 0 && (uint16_t)i < p->templates.count
               ? p->templates.slots[i]
               : NULL;
}
static void order_change_template(void *c, AoeLiveUnit *u, const DatUnit *t) {
    (void)aoe_world_combat_change_template_ror(c, u, t);
}
/* The head action is the unit's gather or attack action. */
static const DatUnitTask *order_action_task(void *c, void *action) {
    AoeWorldCombatUnitRor *s = action;
    (void)c;
    return s->constructing ? s->build.task
                           : s->gathering ? s->gather.task : NULL;
}
static int order_action_retarget(void *c, void *action, AoeLiveUnit *t,
                                 float x, float y, float z) {
    AoeWorldCombatUnitRor *s = action;
    if (s->constructing) {
        AoeBuildActionOps o;
        build_ops(c, &o);
        return aoe_build_action_retarget_ror(&s->build, t, x, y, z, &o);
    }
    if (s->gathering) {
        AoeGatherActionOps o;
        gather_ops(c, &o);
        return aoe_gather_action_retarget_ror(&s->gather, t, x, y, z, &o);
    }
    if (s->attacking) {
        AoeAttackActionOps o;
        attack_ops(c, &o);
        return t ? aoe_attack_action_retarget_ror(&s->attack, t, &o) : 0;
    }
    return 0;
}
static int16_t order_action_type(void *c, void *action) {
    AoeWorldCombatUnitRor *s = action;
    (void)c;
    if (s->pending == 0x65) return s->build.base.type;
    if (s->pending == 5) return s->gather.base.type;
    if (s->pending == 9) return s->attack.base.type;
    if (s->constructing) return s->build.base.type;
    return s->gathering ? s->gather.base.type
                        : s->attacking ? s->attack.base.type : -1;
}
static AoeLiveUnit *order_action_target(void *c, void *action) {
    AoeWorldCombatUnitRor *s = action;
    (void)c;
    if (s->constructing) return s->build.base.target;
    return s->gathering ? s->gather.base.target
                        : s->attacking ? s->attack.base.target : NULL;
}
static int attack_params(AoeWorldCombatRor *c, const DatUnit *t,
                         const DatUnitTask *task, AoeAttackParamsRor *p,
                         AoeAttackParamsRor *d) {
    memset(p, 0, sizeof(*p));
    memset(d, 0, sizeof(*d));
    if (task) {
        p->move_graphic = graphic_by_id(c, task->moving_graphic);
        p->attack_graphic = graphic_by_id(c, task->proceeding_graphic);
        p->range = task->work_range;
    }
    p->min_range = t->min_range;
    p->projectile_id = t->projectile_unit_id;
    p->fire_frame = t->frame_delay;
    d->move_graphic = graphic_by_id(c, t->walking_graphic);
    d->attack_graphic = graphic_by_id(c, t->attack_graphic);
    d->idle_graphic = graphic_by_id(c, t->standing_graphic);
    return 1;
}
static void *order_construct(void *c, AoeLiveUnit *u,
                             const AoeActionRouteRor *route,
                             const DatUnitTask *task, AoeLiveUnit *t,
                             float x, float y, float z) {
    AoeWorldCombatRor *w = c;
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(w, u);
    if (!s) return NULL;
    switch (route->constructor) {
    case 0x004b13c0: case 0x004b1450: {
        AoeBuildActionOps o;
        build_ops(w, &o);
        if (route->constructor == 0x004b13c0
                ? !t || !aoe_build_action_init_target_ror(&s->build, u, task,
                                                          t, &o)
                : !aoe_build_action_init_point_ror(&s->build, u, task, x, y,
                                                   z))
            return NULL;
        s->pending = 0x65;
        return s;
    }
    case 0x00402ef0: case 0x00402f80: {
        AoeGatherActionOps o;
        gather_ops(w, &o);
        if (route->constructor == 0x00402ef0
                ? !aoe_gather_action_init_target_ror(&s->gather, u, task, t,
                                                     &o)
                : !aoe_gather_action_init_point_ror(&s->gather, u, task, x, y,
                                                    z))
            return NULL;
        s->pending = 5;
        return s;
    }
    case 0x00401150: case 0x00401290: {
        AoeAttackActionOps o;
        AoeAttackParamsRor p, d;
        const DatUnit *ut = u->core.unit_template;
        attack_params(w, ut, task, &p, &d);
        attack_ops(w, &o);
        if (route->constructor == 0x00401150
                ? !aoe_attack_action_init_target_ror(&s->attack, u, t, &p, &d,
                                                     ut->max_range, &o)
                : !aoe_attack_action_init_point_ror(&s->attack, u, x, y, z,
                                                    &p, &d, ut->max_range))
            return NULL;
        s->pending = 9;
        return s;
    }
    default:
        return NULL; /* other action classes are not translated yet */
    }
}
/* +0x208: the action list push activates the new head action. */
static void order_adopt(void *c, AoeLiveUnit *u, void *action) {
    AoeWorldCombatUnitRor *s = action;
    if (u->derived_action_list == &s->unit_actions &&
        (!s->unit_actions.head || s->unit_actions.head->action != s))
        (void)aoe_action_list_push(&s->unit_actions, s);
    if (s->pending == 0x65) {
        AoeBuildActionOps o;
        build_ops(c, &o);
        s->attacking = s->gathering = 0;
        s->constructing = 1;
        (void)aoe_build_action_activate_ror(&s->build, 1, &o);
    } else if (s->pending == 5) {
        AoeGatherActionOps o;
        gather_ops(c, &o);
        s->attacking = s->constructing = 0;
        s->gathering = 1;
        (void)aoe_gather_action_activate_ror(&s->gather, 1, &o);
    } else if (s->pending == 9) {
        AoeAttackActionOps o;
        attack_ops(c, &o);
        s->gathering = s->constructing = 0;
        s->attacking = 1;
        (void)aoe_attack_action_activate_ror(&s->attack, &o);
    }
    s->pending = 0;
}
static void order_move(void *c, AoeLiveUnit *u, AoeLiveUnit *t, float x,
                       float y, float z) {
    (void)z;
    if (t) {
        x = t->core.position.x38;
        y = t->core.position.y3c;
    }
    (void)aoe_world_motion_move_to_ror(motion_of(c), u, x, y, 0.4f,
                                       t ? t->core.unit_id : -1);
}
static int order_has_group(void *c, const AoeLiveUnit *u) {
    (void)c; (void)u;
    return 0;
}
static void order_acknowledge(void *c, AoeLiveUnit *u, int32_t a, int32_t b,
                              int32_t id, int32_t cls, float x, float y,
                              float z) {
    (void)c; (void)u; (void)a; (void)b; (void)id; (void)cls; (void)x;
    (void)y; (void)z;
}

int aoe_world_combat_order_ror(AoeWorldCombatRor *c, AoeLiveUnit *u,
                               AoeLiveUnit *t, float x, float y, float z,
                               int16_t *code) {
    AoeWorldCombatUnitRor *s;
    AoeUnitOrderOps o;
    if (!c || !u || !code || !(s = aoe_world_combat_unit_ror(c, u))) return 0;
    memset(&o, 0, sizeof(o));
    o.task = (AoeUnitTaskOps){c, task_owner, task_accepts, task_hostile,
                              task_map_size, task_terrain};
    o.context = c;
    o.tribe_template = order_tribe_template;
    o.owner_template_count = order_template_count;
    o.owner_template = order_template;
    o.change_template = order_change_template;
    o.action_task = order_action_task;
    o.action_retarget = order_action_retarget;
    o.action_type = order_action_type;
    o.action_target = order_action_target;
    o.construct = order_construct;
    o.adopt = order_adopt;
    o.move = order_move;
    o.has_group_object = order_has_group;
    o.acknowledge = order_acknowledge;
    return aoe_unit_order_ror(u, t, x, y, z, &o, code);
}

/* State switch of 0x004a8f40 bound to the runtime. */
typedef struct {
    AoeWorldCombatRor *combat;
    AoeWorldCombatUnitRor *state;
    uint8_t *result;
    AoeWorldProjectileRor *flight;
} StateContext;

static int active_update(void *opaque, AoeLiveUnit *u) {
    StateContext *sc = opaque;
    AoeAttackActionOps o;
    if (sc->flight) {
        AoeProjectileOps po;
        projectile_ops(sc->combat, &po);
        return aoe_projectile_update_ror(&sc->flight->flight, &po) >= 0;
    }
    if (sc->state && sc->state->constructing) {
        AoeBuildActionOps bo;
        build_ops(sc->combat, &bo);
        return aoe_build_action_update_ror(&sc->state->build, &bo,
                                           sc->result);
    }
    if (sc->state && sc->state->gathering) {
        AoeGatherActionOps go;
        gather_ops(sc->combat, &go);
        return aoe_gather_action_update_ror(&sc->state->gather, &go,
                                            sc->result);
    }
    if (!sc->state || !sc->state->attacking) return 1;
    attack_ops(sc->combat, &o);
    return aoe_attack_action_update_ror(&sc->state->attack, &o, sc->result);
    (void)u;
}
static int set_state(void *opaque, AoeLiveUnit *u, uint8_t state) {
    StateContext *sc = opaque;
    AoeScenarioRuntimeRor *r = sc->combat->motion->runtime;
    int16_t o = u->runtime_owner_index;
    if (o < 0 || o >= AOE_SCENARIO_RUNTIME_PLAYERS) return 0;
    return aoe_live_unit_scenario_state_set_ror(
        u, state, &r->players[o].resources, &r->players[o].owner, &r->world,
        &r->players[o].vision_binding);
}
static int select_graphic_id(void *opaque, AoeLiveUnit *u, int16_t id) {
    StateContext *sc = opaque;
    select_graphic(sc->combat, u, graphic_by_id(sc->combat, id));
    return 1;
}
/* The dying sound plays only for the local player; the original brackets
   it with r = rand() ... srand(r) so every player's RNG matches. */
static void dying_sound(void *opaque, AoeLiveUnit *u, int16_t sound) {
    StateContext *sc = opaque;
    AoeRng *rng = &sc->combat->motion->runtime->rng;
    uint16_t r = aoe_rng_next(rng);
    (void)u; (void)sound;
    aoe_rng_seed(rng, r);
}
/* Vtable +0xb8 = 0x0044b910 (types 30..80; root 0x004a9450): create the
   template's dead unit (+0x48) at the unit's position through the owner's
   template factory; corpses of type >= 30 take the unit's facing (+0xb4 =
   0x0044be40: facing, then the facet frame). */
static int terminal_cleanup(void *opaque, AoeLiveUnit *u) {
    StateContext *sc = opaque;
    AoeWorldCombatRor *w = sc->combat;
    AoeLiveUnit *corpse = NULL;
    int16_t dead = u->core.unit_template->dead_unit_id;
    int16_t o = u->runtime_owner_index;
    if (dead < 0) return 1;
    if (o < 0 || o >= AOE_SCENARIO_RUNTIME_PLAYERS ||
        !aoe_scenario_runtime_editor_create_ror(
            w->motion->runtime, (uint8_t)o, (uint16_t)dead,
            u->core.position.x38, u->core.position.y3c, u->core.position.z40,
            &corpse))
        return 1; /* the original ignores a failed factory */
    ++w->corpses;
    if (corpse && corpse->core.unit_template &&
        corpse->core.unit_template->type >= 30 &&
        u->core.unit_template->type >= 30) {
        AoeMovementRor *from = movement_of(w, u);
        if (from) set_facing(w, corpse, from->facing);
    }
    return 1;
}

static int update_unit(AoeWorldCombatRor *c, AoeLiveUnit *u,
                       AoeWorldCombatUnitRor *s, uint8_t *result) {
    StateContext sc = {c, s, result, NULL};
    if (!s) sc.flight = flight_of(c, u);
    AoeLiveUnitUpdateStateOps ops;
    uint8_t predicate = 0;
    memset(&ops, 0, sizeof(ops));
    ops.context = &sc;
    ops.active_update = active_update;
    ops.set_state = set_state;
    ops.select_graphic = select_graphic_id;
    ops.play_dying_sound = dying_sound;
    ops.terminal_cleanup = terminal_cleanup;
    ops.world_delta = c->tick;
    ops.comparison_epsilon = 0.0f;
    if (s && s->reload_timer > 0.0f) {
        /* 0x00426bf0: reload countdown before the base update. */
        s->reload_timer = (float)((double)s->reload_timer - c->tick);
        if (s->reload_timer < 0.0f) s->reload_timer = 0.0f;
    }
    (void)aoe_live_unit_tick_sprites_ror(u, c->tick);
    return aoe_live_unit_update_normal_state_ror(u, &ops, &predicate);
}

int aoe_world_combat_tick_ror(AoeWorldCombatRor *c, AoeLiveUnit *attacker,
                              float tick, uint8_t *result) {
    AoeWorldCombatUnitRor *s;
    if (!c || !attacker || !result ||
        !(s = aoe_world_combat_unit_ror(c, attacker)) ||
        (!s->attacking && !s->gathering && !s->constructing))
        return 0;
    *result = 0;
    c->tick = tick;
    c->world_ms += (uint32_t)(tick * 1000.0f + 0.5f);
    return update_unit(c, attacker, s, result);
}

int aoe_world_combat_update_projectiles_ror(AoeWorldCombatRor *c) {
    if (!c) return 0;
    for (size_t i = 0; i < c->flight_count; ++i) {
        AoeWorldProjectileRor *f = &c->flights[i];
        uint8_t result = 0;
        StateContext sc;
        if (!f->active) continue;
        sc = (StateContext){c, NULL, &result, f};
        {
            AoeLiveUnitUpdateStateOps ops;
            uint8_t predicate = 0;
            memset(&ops, 0, sizeof(ops));
            ops.context = &sc;
            ops.active_update = active_update;
            ops.set_state = set_state;
            ops.select_graphic = select_graphic_id;
            ops.play_dying_sound = dying_sound;
            ops.terminal_cleanup = terminal_cleanup;
            ops.world_delta = c->tick;
            if (!aoe_live_unit_update_normal_state_ror(f->unit, &ops,
                                                       &predicate))
                return 0;
        }
        /* The flights array may have grown during the update. */
        f = &c->flights[i];
        if (f->unit->core.state >= 7) f->active = 0;
    }
    return 1;
}

int aoe_world_combat_update_unit_ror(AoeWorldCombatRor *c, AoeLiveUnit *unit,
                                     float tick) {
    uint8_t result = 0;
    if (!c || !unit) return 0;
    c->tick = tick;
    return update_unit(c, unit, aoe_world_combat_unit_ror(c, unit), &result);
}
