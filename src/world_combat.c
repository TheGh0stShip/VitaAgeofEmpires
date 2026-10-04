#include "world_combat.h"

#include <stdlib.h>
#include <string.h>

#include "sprite_runtime.h"
#include "unit_sprite_binding.h"

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
        memset(s, 0, sizeof(*s));
    }
    s->unit = unit;
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
                  AoeLiveUnit *source, AoeLiveUnit *excluded) {
    (void)c; (void)a; (void)x; (void)y; (void)z; (void)source;
    (void)excluded;
    /* Blast damage needs the tile scan binding (next step). */
}
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
    (void)c; (void)v; (void)cls;
    return 0; /* victim +0x13c: not translated yet */
}
/* 0x004aace0(0, 0, 1): carried amount 0, clamped to template +0x7c. */
static void clear_carried(void *c, AoeLiveUnit *v) {
    (void)c;
    v->core.storage.carried_amount = 0.0f;
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

static void fire_target(void *c, AoeLiveUnit *u, AoeLiveUnit *t) {
    AoeWorldCombatRor *w = c;
    AoeCombatHitOps h = {w, crt_rand, attack_notice, blast, hit_multiplier,
                         receive, 0, destroy_target};
    const DatUnit *tmpl = u->core.unit_template;
    if (tmpl->projectile_unit_id >= 0) {
        ++w->projectiles; /* projectile flight: next step */
        return;
    }
    if (tmpl->type >= 70) {
        if (aoe_combat_melee_hit_tribe_ror(u, t, u->core.position.x38,
                                           u->core.position.y3c,
                                           u->core.position.z40, &h) != 1)
            ++w->misses;
    } else if (aoe_combat_melee_hit_ror(u, t, u->core.position.x38,
                                        u->core.position.y3c,
                                        u->core.position.z40, &h) != 1) {
        ++w->misses;
    }
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
    if (c->units) {
        for (size_t i = 0; i < AOE_WORLD_MOTION_ID_SLOTS; ++i)
            if (c->units[i]) {
                aoe_action_list_clear(&c->units[i]->sub_actions);
                free(c->units[i]);
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

/* State switch of 0x004a8f40 bound to the runtime. */
typedef struct {
    AoeWorldCombatRor *combat;
    AoeWorldCombatUnitRor *state;
    uint8_t *result;
} StateContext;

static int active_update(void *opaque, AoeLiveUnit *u) {
    StateContext *sc = opaque;
    AoeAttackActionOps o;
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
static int terminal_cleanup(void *opaque, AoeLiveUnit *u) {
    (void)opaque; (void)u;
    return 1; /* vtable +0xb8: not translated yet */
}

static int update_unit(AoeWorldCombatRor *c, AoeLiveUnit *u,
                       AoeWorldCombatUnitRor *s, uint8_t *result) {
    StateContext sc = {c, s, result};
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
        !(s = aoe_world_combat_unit_ror(c, attacker)) || !s->attacking)
        return 0;
    *result = 0;
    c->tick = tick;
    c->world_ms += (uint32_t)(tick * 1000.0f + 0.5f);
    return update_unit(c, attacker, s, result);
}

int aoe_world_combat_update_unit_ror(AoeWorldCombatRor *c, AoeLiveUnit *unit,
                                     float tick) {
    uint8_t result = 0;
    if (!c || !unit) return 0;
    c->tick = tick;
    return update_unit(c, unit, aoe_world_combat_unit_ror(c, unit), &result);
}
