#include "world_combat.h"

#include "command_place.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "sprite_runtime.h"
#include "aoe_ftol.h"
#include "terrain.h"
#include "unit_sprite_binding.h"
#include "unit_task.h"
#include "building_update.h"
#include "player_costs.h"
#include "unit_placement_check.h"

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

/* Unit +0x184 list entries (context: the unit's record): the record itself
   stands for its adopted gather/attack/build action; anything else is a
   move action owned by the list. */
static int16_t entry_type(const void *action);
static int research_ops(AoeWorldCombatRor *c, int16_t owner,
                        AoeResearchActionOps *o);

/* Heap action classes on +0x184 lists, by action type (the first int16):
   the original's per-class vtable slots +0x00 (destructor), +0x1c
   (activate), +0x28 (update), +0x30 (task), +0x34 (retarget) and the
   target +0x4c. A NULL slot is the base behaviour: destroy only frees,
   no task, retarget refuses (0x004072b0), no target. */
typedef struct {
    int16_t type;
    void (*destroy)(AoeWorldCombatRor *c, void *action);
    void (*activate)(AoeWorldCombatRor *c, void *action, uint8_t state);
    uint8_t (*update)(AoeWorldCombatRor *c, void *action);
    const DatUnitTask *(*task)(void *action);
    int (*retarget)(AoeWorldCombatRor *c, void *action, AoeLiveUnit *t,
                    float x, float y, float z);
    AoeLiveUnit *(*target)(void *action);
} HeapActionClass;
static const HeapActionClass *heap_class(int16_t type);
static void base_destroy(AoeWorldCombatRor *c, AoeMoveActionRor *b);
static void repair_ops(AoeWorldCombatRor *c, AoeRepairActionOps *o);
static void heal_ops(AoeWorldCombatRor *c, AoeHealActionOps *o);
static void convert_ops(AoeWorldCombatRor *c, AoeConvertActionOps *o);
static void trade_ops(AoeWorldCombatRor *c, AoeTradeActionOps *o);
static void object_ops(AoeWorldCombatRor *c, AoeObjectActionOps *o);
static void base_ops(AoeWorldCombatRor *c, AoeBaseActionOps *o);

/* The record (gather/attack/build) clears its flags; heap actions run
   their class destructor, then are freed (train actions free
   themselves). */
static void head_destroy(void *context, void *action) {
    AoeWorldCombatUnitRor *s = context;
    const HeapActionClass *k;
    if (action == s) {
        s->attacking = s->gathering = s->constructing = 0;
        return;
    }
    k = heap_class(entry_type(action));
    if (k && k->destroy) {
        k->destroy(s->world, action);
        return;
    }
    if (k && k->target) base_destroy(s->world, action); /* base layout */
    free(action);
}
static void head_activate(void *context, void *action, uint8_t state);
static void order_adopt(void *c, AoeLiveUnit *u, void *action);
static void train_ops(AoeWorldCombatRor *c);
static void release_building(AoeWorldCombatUnitRor *s);
static void queue_free(void *ctx, void *p);

/* Action-list callbacks for the move sub-action. */
static void move_ops(AoeWorldCombatRor *c, AoeMoveActionOps *o);
/* Base destructor 0x00406ed0: release the target (0x00407380) and the
   secondary (0x004073f0), then destroy the +0x34 list. */
static void base_destroy(AoeWorldCombatRor *c, AoeMoveActionRor *b) {
    AoeMoveActionOps o;
    move_ops(c, &o);
    (void)aoe_action_set_target_ror(b, NULL, &o);
    if (b->secondary_id != -1) {
        AoeLiveUnit *x = o.resolve(c, b->secondary_id);
        if (x) o.target_released(c, x, b->unit);
        b->secondary_id = -1;
    }
    if (b->sub_list) {
        (void)aoe_action_list_clear(b->sub_list);
        free(b->sub_list);
        b->sub_list = NULL;
    }
}
/* Sub-actions are moves (type 1) or, nested in a hunt, attacks (type 9)
   with their own sub-action list. */
static void sub_destroy(void *context, void *action) {
    base_destroy(context, action);
    free(action);
}
static void sub_activate(void *context, void *action, uint8_t state);

AoeWorldCombatUnitRor *aoe_world_combat_unit_ror(AoeWorldCombatRor *c,
                                                 AoeLiveUnit *unit) {
    int64_t slot;
    AoeWorldCombatUnitRor *s;
    if (!c || !c->units || !unit) return NULL;
    slot = (int64_t)unit->core.unit_id + AOE_WORLD_MOTION_ID_OFFSET;
    if (!aoe_world_id_table_reserve(&c->units, &c->unit_slots,
                                    slot))
        return NULL;
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
    s->action_tag = AOE_WORLD_COMBAT_RECORD_TAG;
    s->unit = unit;
    s->world = c;
    s->unit_actions.owner_state = &unit->core.state;
    s->unit_actions.ops.context = s;
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

/* The unit's record if it has one (no allocation). */
static AoeWorldCombatUnitRor *peek_unit(AoeWorldCombatRor *c,
                                        const AoeLiveUnit *unit) {
    int64_t slot;
    AoeWorldCombatUnitRor *s;
    if (!c || !c->units || !unit) return NULL;
    slot = (int64_t)unit->core.unit_id + AOE_WORLD_MOTION_ID_OFFSET;
    if (slot < 0 || slot >= (int64_t)c->unit_slots) return NULL;
    s = c->units[slot];
    return s && s->unit == unit ? s : NULL;
}

/* Base action hooks shared by the move and attack actions: the target's
   +0xc4 / +0xc0 (reference count +0x4f and dormancy). */
static AoeScenarioRuntimePlayerRor *owner_of(AoeWorldCombatRor *c,
                                             const AoeLiveUnit *u) {
    int16_t o = u ? u->runtime_owner_index : -1;
    if (o < 0 || o >= AOE_SCENARIO_RUNTIME_PLAYERS) return NULL;
    return &c->motion->runtime->players[o];
}
static void target_released(void *c, AoeLiveUnit *t, AoeLiveUnit *u) {
    AoeWorldCombatRor *w = c;
    AoeScenarioRuntimePlayerRor *p = owner_of(w, t);
    int notice = 0;
    (void)u;
    if (p && t->constructed &&
        aoe_live_unit_target_release_ror(t, &p->owner,
                                         &w->motion->runtime->world,
                                         &notice) &&
        notice)
        ++w->notices; /* +0x148(..., 0x2bb, ...) */
}
static void target_attached(void *c, AoeLiveUnit *t, AoeLiveUnit *u,
                            int16_t type) {
    AoeWorldCombatRor *w = c;
    AoeScenarioRuntimePlayerRor *p = owner_of(w, t);
    (void)u; (void)type;
    if (p && t->constructed)
        (void)aoe_live_unit_target_attach_ror(t, &p->owner,
                                              &w->motion->runtime->world);
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

static void attack_ops(AoeWorldCombatRor *c, AoeAttackActionOps *o);
static void sub_activate(void *context, void *action, uint8_t state) {
    AoeMoveActionOps o;
    (void)state;
    if (((AoeMoveActionRor *)action)->type == 9) {
        AoeAttackActionOps ao;
        attack_ops(context, &ao);
        (void)aoe_attack_action_activate_ror(action, &ao);
        return;
    }
    move_ops(context, &o);
    (void)aoe_move_action_activate_ror(action, &o);
}
static int16_t entry_type(const void *action) {
    int16_t t;
    memcpy(&t, action, sizeof(t));
    return t;
}
/* An action's sub-action list (+0x34): its own when it has one (an action
   nested in another, such as a hunt's attack), else the unit's shared
   list. */
static AoeActionList *sub_list_of(AoeWorldCombatRor *w,
                                  AoeMoveActionRor *base) {
    AoeWorldCombatUnitRor *s;
    if (base->sub_list) return base->sub_list;
    s = aoe_world_combat_unit_ror(w, base->unit);
    return s ? &s->sub_actions : NULL;
}
static void farm_ops(AoeWorldCombatRor *c, AoeFarmActionOps *o);
static void head_activate(void *context, void *action, uint8_t state) {
    AoeWorldCombatUnitRor *s = context;
    const HeapActionClass *k = heap_class(entry_type(action));
    if (k && k->activate) k->activate(s->world, action, state);
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
    sp.reference_speed = g->reference_speed;
    sp.facets = (int16_t)g->angle_count;
    sp.flags = g->sequence_flags;
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
    return g ? g->sequence_flags : 0;
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
    copy->terrain_table = lt->terrain_table;
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
        AoeMovementSpriteRor sp = {g->reference_speed,
                                   (int16_t)g->angle_count, g->sequence_flags};
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
    if (!aoe_action_list_push(sub_list_of(w, &a->base), mv)) {
        free(mv);
        return 0;
    }
    return 1;
}
static uint8_t update_head(void *c, void *action) {
    AoeMoveActionOps o;
    uint8_t r = 0;
    if (((AoeMoveActionRor *)action)->type == 9) {
        AoeAttackActionOps ao;
        attack_ops(c, &ao);
        return aoe_attack_action_update_ror(action, &ao, &r) ? r : 0;
    }
    move_ops(c, &o);
    if (!aoe_move_action_update_ror(action, &o, &r)) return 0;
    return r;
}
static void update_waiting(void *c, void *action) { (void)c; (void)action; }
static uint8_t sub_update(void *c, AoeAttackActionRor *a) {
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(c, a->base.unit);
    AoeActionTickOps t = {c, update_head, update_waiting};
    uint8_t r = 0;
    if (!s || !aoe_action_list_update_ror(sub_list_of(c, &a->base), &t, &r))
        return 0;
    return r;
}
static void sub_clear(void *c, AoeAttackActionRor *a) {
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(c, a->base.unit);
    if (s) (void)aoe_action_list_clear(sub_list_of(c, &a->base));
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
    for (int i = 0; i < 10; ++i) aoe_settings_clear_wonder_ror(&c->settings, i);
    c->units = calloc(AOE_WORLD_MOTION_ID_SLOTS, sizeof(*c->units));
    c->unit_slots = c->units ? AOE_WORLD_MOTION_ID_SLOTS : 0;
    train_ops(c);
    return c->units != NULL;
}

void aoe_world_combat_destroy_ror(AoeWorldCombatRor *c) {
    if (!c) return;
    c->tearing_down = 1; /* game +0x3f4 +0x1d: destructors stay quiet */
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
        for (size_t i = 0; i < c->unit_slots; ++i)
            if (c->units[i]) {
                AoeWorldCombatUnitRor *s = c->units[i];
                release_building(s);
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
        p.move_graphic = graphic_by_id(c, task->approach_graphic);
        p.attack_graphic = graphic_by_id(c, task->action_graphic);
        p.range = task->action_range;
    }
    p.min_range = t->min_range;
    p.projectile_id = t->projectile_unit_id;
    p.fire_frame = t->fire_frame;
    d.move_graphic = graphic_by_id(c, t->walking_graphic);
    d.attack_graphic = graphic_by_id(c, t->attack_graphic);
    d.idle_graphic = graphic_by_id(c, t->standing_graphic);
    attack_ops(c, &o);
    if (!aoe_attack_action_init_target_ror(&s->attack, attacker, target, &p,
                                           &d, t->max_range, &o))
        return 0;
    s->pending = 9;
    order_adopt(c, attacker, s);
    return s->attacking;
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
static void refresh_world(void *c) {
    aoe_scenario_runtime_refresh_shared_vision_ror(
        ((AoeWorldCombatRor *)c)->motion->runtime);
}
/* Player +0x78(type, amount, 1) through the derived 0x004f6c60, then the
   +0x5c deposit notice (counted). */
static void deposit_cb(void *c, AoeLiveUnit *u, int16_t type, float amount,
                       int flag) {
    AoeWorldCombatRor *w = c;
    AoeScenarioRuntimePlayerRor *p = player_of(c, u);
    AoePlayerResourceTargetRor target;
    if (!p) return;
    target.resources = &p->resources;
    target.derived = 1;
    target.field_104 = &p->field_104;
    target.context = w;
    target.refresh_world = refresh_world;
    (void)aoe_player_resource_add_virtual_ror(&target, type, amount, flag);
    ++w->deposits;
}
/* Farm action bindings: player +0x78(type, amount, 0); the 0x7e event is
   counted; +0x6c drops the hit points to 0 (0x004aa320 also wakes the
   unit, which every type-80 unit already is). */
/* Owner player +0x78(type, amount, 0). */
static void owner_add(void *c, AoeLiveUnit *u, int16_t type, float amount) {
    AoeWorldCombatRor *w = c;
    AoeScenarioRuntimePlayerRor *p = player_of(c, u);
    AoePlayerResourceTargetRor target;
    if (!p) return;
    target.resources = &p->resources;
    target.derived = 1;
    target.field_104 = &p->field_104;
    target.context = w;
    target.refresh_world = refresh_world;
    (void)aoe_player_resource_add_virtual_ror(&target, type, amount, 0);
}
static void farm_depleted(void *c, AoeLiveUnit *u) {
    (void)u;
    ++((AoeWorldCombatRor *)c)->farms_depleted;
}
static void farm_destroy(void *c, AoeLiveUnit *u) {
    (void)c;
    u->core.hit_points = 0.0f;
}
static void farm_ops(AoeWorldCombatRor *c, AoeFarmActionOps *o) {
    o->context = c;
    o->tick_seconds = tick_seconds;
    o->player_attribute = player_attribute;
    o->deposit = owner_add;
    o->notify_depleted = farm_depleted;
    o->destroy = farm_destroy;
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
    if (!aoe_action_list_push(sub_list_of(w, &a->base), mv)) {
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
    if (!s || !aoe_action_list_update_ror(sub_list_of(c, &a->base), &t, &r))
        return 0;
    return r;
}
static void gather_sub_clear(void *c, AoeGatherActionRor *a) {
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(c, a->base.unit);
    if (s) (void)aoe_action_list_clear(sub_list_of(c, &a->base));
}

static int attack_params(AoeWorldCombatRor *c, const DatUnit *t,
                         const DatUnitTask *task, AoeAttackParamsRor *p,
                         AoeAttackParamsRor *d);
/* Hunting state 12: 0x00401150 with its own sub-action list, pushed on
   the hunt's list, then quiet. */
static int gather_start_attack(void *c, AoeGatherActionRor *a,
                               const void *move_graphic,
                               const void *attack_graphic, float range) {
    AoeWorldCombatRor *w = c;
    AoeLiveUnit *u = a->base.unit;
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(w, u);
    AoeAttackActionRor *at;
    AoeAttackActionOps o;
    AoeAttackParamsRor p, d;
    AoeActionList *own, *list;
    if (!s || !a->base.target || !(list = sub_list_of(w, &a->base)))
        return 0;
    at = calloc(1, sizeof(*at));
    own = calloc(1, sizeof(*own));
    if (!at || !own) {
        free(at);
        free(own);
        return 0;
    }
    own->owner_state = &u->core.state;
    own->ops = s->sub_actions.ops;
    attack_params(w, u->core.unit_template, NULL, &p, &d);
    p.move_graphic = move_graphic;
    p.attack_graphic = attack_graphic;
    p.idle_graphic = NULL;
    p.range = range;
    attack_ops(w, &o);
    if (!aoe_attack_action_init_target_ror(at, u, a->base.target, &p, &d,
                                           u->core.unit_template->max_range,
                                           &o)) {
        free(at);
        free(own);
        return 0;
    }
    at->base.sub_list = own;
    if (!aoe_action_list_push(list, at)) {
        sub_destroy(w, at);
        return 0;
    }
    at->base.quiet = 1;
    return 1;
}
static void gather_ops(AoeWorldCombatRor *c, AoeGatherActionOps *o) {
    memset(o, 0, sizeof(*o));
    o->start_attack = gather_start_attack;
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
    s->pending = 5;
    order_adopt(c, unit, s);
    return s->gathering;
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
/* Takes over the runtime-owned building lists: +0x184 entries are this
   world's actions, +0x1cc entries training actions. */
static int bind_building(AoeWorldCombatRor *w, AoeLiveUnit *t) {
    AoeWorldCombatUnitRor *s;
    AoeBuildingUpdate *b = t->building_update;
    if (!b || !b->live_actions || !b->queue_actions ||
        !(s = aoe_world_combat_unit_ror(w, t)))
        return 0;
    if (!s->adopted_building) {
        b->live_actions->ops = (AoeActionListOps){s, head_destroy,
                                                  head_activate};
        aoe_train_action_adapter_list_ops(&w->train,
                                          &b->queue_actions->ops);
        s->adopted_building = 1;
    }
    return 1;
}
/* Clears the adopted lists and gives their callbacks back to the runtime. */
static void release_building(AoeWorldCombatUnitRor *s) {
    AoeBuildingUpdate *b;
    AoeActionListOps ops;
    if (!s->adopted_building || !s->unit || !(b = s->unit->building_update))
        return;
    (void)aoe_action_list_clear(b->live_actions);
    (void)aoe_action_list_clear(b->queue_actions);
    aoe_scenario_runtime_building_list_ops_ror(&ops);
    b->live_actions->ops = ops;
    b->queue_actions->ops = ops;
    s->adopted_building = 0;
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
/* +0x260 = 0x004acaf0: storage entries of mode >= 4 reach the owner. */
static void owner_add(void *c, AoeLiveUnit *u, int16_t type, float amount);
static void storage_complete(void *c, AoeLiveUnit *u) {
    (void)aoe_live_unit_building_storage_ror(u, 1, owner_add, c);
}
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
    visual.building_pre_complete = storage_complete;
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
    if (!aoe_action_list_push(sub_list_of(w, &a->base), mv)) {
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
    if (!s || !aoe_action_list_update_ror(sub_list_of(c, &a->base), &t, &r))
        return 0;
    return r;
}
static void build_sub_clear(void *c, AoeBuildActionRor *a) {
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(c, a->base.unit);
    if (s) (void)aoe_action_list_clear(sub_list_of(c, &a->base));
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
/* Heads other than the record are heap actions (HeapActionClass). */
static const DatUnitTask *order_action_task(void *c, void *action) {
    AoeWorldCombatUnitRor *s = action;
    (void)c;
    if (entry_type(action) != AOE_WORLD_COMBAT_RECORD_TAG) {
        const HeapActionClass *k = heap_class(entry_type(action));
        return k && k->task ? k->task(action) : NULL;
    }
    return s->constructing ? s->build.task
                           : s->gathering ? s->gather.task : NULL;
}
static int order_action_retarget(void *c, void *action, AoeLiveUnit *t,
                                 float x, float y, float z) {
    AoeWorldCombatUnitRor *s = action;
    if (entry_type(action) != AOE_WORLD_COMBAT_RECORD_TAG) {
        const HeapActionClass *k = heap_class(entry_type(action));
        return k && k->retarget ? k->retarget(c, action, t, x, y, z) : 0;
    }
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
    if (entry_type(action) != AOE_WORLD_COMBAT_RECORD_TAG)
        return entry_type(action);
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
    if (entry_type(action) != AOE_WORLD_COMBAT_RECORD_TAG) {
        const HeapActionClass *k = heap_class(entry_type(action));
        return k && k->target ? k->target(action) : NULL;
    }
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
        p->move_graphic = graphic_by_id(c, task->approach_graphic);
        p->attack_graphic = graphic_by_id(c, task->action_graphic);
        p->range = task->action_range;
    }
    p->min_range = t->min_range;
    p->projectile_id = t->projectile_unit_id;
    p->fire_frame = t->fire_frame;
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
    case 0x00402440: case 0x00402b80: case 0x004020e0: case 0x004067d0: {
        AoeBaseActionRor *a = calloc(1, sizeof(*a));
        AoeBaseActionOps o;
        int ok;
        base_ops(w, &o);
        if (!a) return NULL;
        switch (route->constructor) {
        case 0x00402440:
            ok = aoe_enter_action_init_ror(a, u, task, t, &o);
            break;
        case 0x00402b80:
            ok = aoe_explore_action_init_ror(a, u, task, x, y, z);
            break;
        case 0x004020e0:
            ok = aoe_fly_action_init_ror(a, u, task, x, y, z);
            break;
        default:
            ok = aoe_transport_action_init_ror(a, u, task, x, y, z);
            break;
        }
        if (!ok) {
            free(a);
            a = NULL;
        }
        return a;
    }
    case 0x004b0df0: case 0x004b0e20: {
        AoeArtifactActionRor *a = calloc(1, sizeof(*a));
        if (a && !(route->constructor == 0x004b0df0
                       ? aoe_artifact_action_init_ror(a, u, task, t)
                       : aoe_artifact_action_init_point_ror(a, u, task, x, y,
                                                            z))) {
            free(a);
            a = NULL;
        }
        return a;
    }
    case 0x004b2880: case 0x004b2900: {
        AoeDiscoveryActionRor *a = calloc(1, sizeof(*a));
        AoeObjectActionOps o;
        object_ops(w, &o);
        if (a && !aoe_discovery_action_init_ror(
                     a, u, task, route->constructor == 0x004b2880 ? t : NULL,
                     &o)) {
            aoe_discovery_action_free_ror(a);
            free(a);
            a = NULL;
        }
        if (a && route->constructor == 0x004b2900) {
            a->base.x = x;
            a->base.y = y;
            a->base.z = z;
        }
        return a;
    }
    case 0x004b7220: {
        AoeWonderActionRor *a = calloc(1, sizeof(*a));
        if (a && !aoe_wonder_action_init_ror(a, u)) {
            free(a);
            a = NULL;
        }
        return a;
    }
    case 0x004b6520: case 0x004b65b0: {
        AoeTradeActionRor *a = calloc(1, sizeof(*a));
        AoeMoveActionOps o;
        move_ops(w, &o);
        if (a && !(route->constructor == 0x004b6520
                       ? t && aoe_trade_action_init_target_ror(a, u, task, t,
                                                               &o)
                       : aoe_trade_action_init_point_ror(a, u, task, x, y,
                                                         z))) {
            free(a);
            a = NULL;
        }
        return a;
    }
    case 0x004b32c0: case 0x004b3350: {
        AoeGatherActionRor *a = calloc(1, sizeof(*a));
        AoeGatherActionOps o;
        gather_ops(w, &o);
        if (a && !(route->constructor == 0x004b32c0
                       ? aoe_hunt_action_init_target_ror(a, u, task, t, &o)
                       : aoe_hunt_action_init_point_ror(a, u, task, x, y,
                                                        z))) {
            free(a);
            a = NULL;
        }
        return a;
    }
    case 0x004b1cb0: case 0x004b1d40: {
        AoeConvertActionRor *a = calloc(1, sizeof(*a));
        AoeMoveActionOps o;
        move_ops(w, &o);
        if (a && !(route->constructor == 0x004b1cb0
                       ? aoe_convert_action_init_target_ror(a, u, task, t,
                                                            &o)
                       : aoe_convert_action_init_point_ror(a, u, task, x, y,
                                                           z))) {
            free(a);
            a = NULL;
        }
        return a;
    }
    case 0x004b2c00: case 0x004b2c80: {
        AoeHealActionRor *a = calloc(1, sizeof(*a));
        AoeMoveActionOps o;
        move_ops(w, &o);
        if (a && !(route->constructor == 0x004b2c00
                       ? t && aoe_heal_action_init_target_ror(a, u, task, t,
                                                              &o)
                       : aoe_heal_action_init_point_ror(a, u, task, x, y,
                                                        z))) {
            free(a);
            a = NULL;
        }
        return a;
    }
    case 0x004b55c0: case 0x004b5650: {
        AoeRepairActionRor *a = calloc(1, sizeof(*a));
        AoeRepairActionOps o;
        repair_ops(w, &o);
        if (a && !(route->constructor == 0x004b55c0
                       ? aoe_repair_action_init_target_ror(a, u, task, t, &o)
                       : aoe_repair_action_init_point_ror(a, u, task, x, y,
                                                          z))) {
            free(a);
            a = NULL;
        }
        return a;
    }
    case 0x00404750: {
        AoeFarmActionRor *f = calloc(1, sizeof(*f));
        if (f && !aoe_farm_action_init_ror(f, u, task)) {
            free(f);
            f = NULL;
        }
        return f;
    }
    default:
        return NULL; /* other action classes are not translated yet */
    }
}
/* +0x208 = 0x00406490: clear the +0x184 list (0x00403e50), then push the
   action (0x00404400, which activates it). */
static AoeActionList *own_list(AoeWorldCombatUnitRor *s, AoeLiveUnit *u) {
    if (u->derived_action_list == &s->unit_actions ||
        (s->adopted_building && u->building_update &&
         u->derived_action_list == u->building_update->live_actions))
        return u->derived_action_list;
    return NULL;
}
static void order_adopt(void *c, AoeLiveUnit *u, void *action) {
    AoeWorldCombatUnitRor *s;
    AoeActionList *list;
    if (entry_type(action) != AOE_WORLD_COMBAT_RECORD_TAG) {
        /* A heap action (move, farm) owned by the list from here on. */
        s = aoe_world_combat_unit_ror(c, u);
        if (!s || !(list = own_list(s, u))) {
            free(action);
            return;
        }
        (void)aoe_action_list_clear(list);
        if (!aoe_action_list_push(list, action)) free(action);
        return;
    }
    s = action;
    if ((list = own_list(s, u))) {
        (void)aoe_action_list_clear(list);
        if (!aoe_action_list_push(list, s) || !list->head ||
            list->head->action != s) {
            s->pending = 0; /* a terminal unit destroys the offer */
            return;
        }
    }
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
/* +0x9c = 0x00405db0: a self-target is ignored; the head action's +0x30
   (0x00404550) may absorb the move (gather 0x00403b80, build 0x004b5db0;
   base and attack 0x004072b0 refuse); otherwise a type-1 move action
   (0x004052d0 / 0x00405250, range 0.4, default graphic) is adopted. The
   group notice and the +0x188 count are not modelled. */
static void order_move(void *c, AoeLiveUnit *u, AoeLiveUnit *t, float x,
                       float y, float z) {
    AoeWorldCombatRor *w = c;
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(w, u);
    AoeMoveActionOps o;
    AoeMoveActionRor *mv;
    const void *walk;
    int ok;
    if (t == u || !s || u->derived_action_list != &s->unit_actions) return;
    if (s->unit_actions.head && s->unit_actions.head->action == s) {
        if (s->constructing) {
            AoeBuildActionOps bo;
            build_ops(w, &bo);
            if (aoe_build_action_walk_ror(&s->build, x, y, z, &bo)) return;
        } else if (s->gathering) {
            AoeGatherActionOps go;
            gather_ops(w, &go);
            if (aoe_gather_action_deliver_ror(&s->gather, t, x, y, z, &go))
                return;
        }
    }
    mv = calloc(1, sizeof(*mv));
    if (!mv) return;
    move_ops(w, &o);
    walk = graphic_by_id(w, u->core.unit_template->walking_graphic);
    if (!walk) walk = graphic_by_id(w, u->core.unit_template->standing_graphic);
    ok = t ? aoe_move_action_init_target_ror(mv, u, t, 0.4f, NULL, walk, &o)
           : aoe_move_action_init_point_ror(mv, u, x, y, z, 0.4f, NULL, walk);
    if (!ok) {
        free(mv);
        return;
    }
    (void)aoe_action_list_clear(&s->unit_actions);
    if (!aoe_action_list_push(&s->unit_actions, mv)) free(mv);
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
    int ok;
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
    ok = aoe_unit_order_ror(u, t, x, y, z, &o, code);
    /* The order entry points (+0x98/+0xa0 around +0x1f8) count orders
       in unit byte +0x188. */
    if ((s = peek_unit(c, u))) ++s->order_serial;
    return ok;
}

/* Per-unit update chain bound to the runtime. */

/* The adopted gather/attack/build action's +0x28 update. */
static uint8_t record_update(AoeWorldCombatUnitRor *s) {
    AoeWorldCombatRor *c = s->world;
    uint8_t r = 1;
    if (s->constructing) {
        AoeBuildActionOps o;
        build_ops(c, &o);
        if (!aoe_build_action_update_ror(&s->build, &o, &r)) return 0;
    } else if (s->gathering) {
        AoeGatherActionOps o;
        gather_ops(c, &o);
        if (!aoe_gather_action_update_ror(&s->gather, &o, &r)) return 0;
    } else if (s->attacking) {
        AoeAttackActionOps o;
        attack_ops(c, &o);
        if (!aoe_attack_action_update_ror(&s->attack, &o, &r)) return 0;
    }
    return r;
}
/* +0x184 list head update (context: the record). */
static uint8_t unit_update_head(void *context, void *action) {
    AoeWorldCombatUnitRor *s = context;
    uint8_t r;
    r = 0;
    if (action == s) {
        r = record_update(s);
    } else {
        const HeapActionClass *k = heap_class(entry_type(action));
        r = k && k->update ? k->update(s->world, action) : 0;
    }
    s->last_result = r;
    return r;
}
static uint8_t foreign_head(void *context, void *action) {
    (void)context; (void)action;
    return 0;
}

/* Type-80 setter bindings. The building manager (world +0x40) is not
   modelled; queue refunds and cancellations are counted until production
   is bound. */
static int no_manager(void *c, AoeLiveUnit *u) {
    (void)c; (void)u;
    return 1;
}
static int probe_template(void *c, AoeLiveUnit *u, int16_t id) {
    AoeScenarioRuntimeRor *r = ((AoeWorldCombatRor *)c)->motion->runtime;
    int16_t o = u->runtime_owner_index;
    AoeScenarioRuntimePlayerRor *p = &r->players[o];
    for (uint32_t i = 0; i < p->unit_count; ++i) {
        AoeLiveUnit *x = &p->units[i];
        if (x->constructed && x->core.state == 2 && x->core.unit_template &&
            x->core.unit_template->id == id)
            return 1;
    }
    for (size_t i = 0; i < r->editor_unit_count; ++i) {
        AoeLiveUnit *x = r->editor_units[i];
        if (x && x->constructed && x->runtime_owner_index == o &&
            x->core.state == 2 && x->core.unit_template &&
            x->core.unit_template->id == id)
            return 1;
    }
    return 0;
}
/* Owner +0x224 0x004ea9e4: the building's techs drop back to state 0 and
   the manager rebuilds. */
static void remove_research(void *c, AoeLiveUnit *u, int16_t id) {
    AoeScenarioRuntimePlayerRor *p = owner_of(c, u);
    ++((AoeWorldCombatRor *)c)->research_removals;
    if (p && p->research.entries)
        (void)aoe_research_reset_ror(&p->research, id);
}
static void refund_one(void *c, int16_t unit_id) {
    (void)unit_id;
    ++((AoeWorldCombatRor *)c)->queue_refunds;
}
static void cancel_action(void *c, void *action) { (void)c; (void)action; }
static void queue_notify(void *c, uint32_t e, int16_t o, uint32_t r) {
    (void)c; (void)e; (void)o; (void)r;
}

static int set_state(void *opaque, AoeLiveUnit *u, uint8_t state) {
    AoeWorldCombatRor *c = opaque;
    AoeScenarioRuntimeRor *r = c->motion->runtime;
    int16_t o = u->runtime_owner_index;
    if (o < 0 || o >= AOE_SCENARIO_RUNTIME_PLAYERS) return 0;
    if (u->core.unit_template->type == 80) {
        AoeBuildingSetterRuntimeOps bo;
        BuildStateContext sc = {c};
        if (!bind_building(c, u)) return 0;
        if (state == 2) return building_set_state(&sc, u, 2);
        bo.context = c;
        bo.storage_add = owner_add;
        bo.remove_building_manager = no_manager;
        bo.completion_probe = probe_template;
        bo.remove_research = remove_research;
        bo.owner_status_four_count = &c->queue_status_four[o];
        bo.queue_cancel = (AoeBuildingQueueCancelOps){c, refund_one,
                                                      cancel_action,
                                                      queue_notify};
        return aoe_live_unit_scenario_building_state_set_ror(
            u, state, &r->players[o].resources, &r->players[o].owner,
            &r->world, &r->players[o].vision_binding, &bo);
    }
    return aoe_live_unit_scenario_state_set_ror(
        u, state, &r->players[o].resources, &r->players[o].owner, &r->world,
        &r->players[o].vision_binding);
}
/* State 2 +0x28 = 0x004a9240: damage graphics by hit-point level. */
static int damage_select(void *opaque, AoeLiveUnit *u, const DatGraphic *g) {
    select_graphic(opaque, u, g);
    return 1;
}
static int damage_graphics(void *opaque, AoeLiveUnit *u) {
    AoeWorldCombatRor *c = opaque;
    AoeScenarioRuntimeRor *r = c->motion->runtime;
    AoeLiveUnitDamageGraphicOps o = {c, damage_select, r->graphics,
                                     r->geometry, &r->rng};
    return aoe_live_unit_damage_graphics_ror(u, &o);
}
static int select_graphic_id(void *opaque, AoeLiveUnit *u, int16_t id) {
    select_graphic(opaque, u, graphic_by_id(opaque, id));
    return 1;
}
/* The dying sound plays only for the local player; the original brackets
   it with r = rand() ... srand(r) so every player's RNG matches. */
static void dying_sound(void *opaque, AoeLiveUnit *u, int16_t sound) {
    AoeRng *rng = &((AoeWorldCombatRor *)opaque)->motion->runtime->rng;
    uint16_t r = aoe_rng_next(rng);
    (void)u; (void)sound;
    aoe_rng_seed(rng, r);
}
/* Vtable +0xb8 = 0x0044b910 (types 30..80; root 0x004a9450): create the
   template's dead unit (+0x48) at the unit's position through the owner's
   template factory; corpses of type >= 30 take the unit's facing (+0xb4 =
   0x0044be40: facing, then the facet frame). */
static int terminal_cleanup(void *opaque, AoeLiveUnit *u) {
    AoeWorldCombatRor *w = opaque;
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
static void state_ops(AoeWorldCombatRor *c, AoeLiveUnitUpdateStateOps *o) {
    memset(o, 0, sizeof(*o));
    o->context = c;
    o->active_update = damage_graphics;
    o->set_state = set_state;
    o->select_graphic = select_graphic_id;
    o->play_dying_sound = dying_sound;
    o->terminal_cleanup = terminal_cleanup;
    o->world_delta = c->tick;
    o->comparison_epsilon = 0.0f;
}

/* Derived +0x24 prefixes: type 70/80 class-18 faith (0x004ae7d0: carried
   += owner attribute 35 x tick, capped at 100, carried type 34), then the
   type-50 reload countdown (0x00426bf0). Type 80 binds its storage first
   so its +0x08 cleanup has it. */
static int unit_before(void *opaque, AoeLiveUnit *u) {
    AoeWorldCombatRor *c = opaque;
    const DatUnit *t = u->core.unit_template;
    AoeWorldCombatUnitRor *s;
    if (t->type == 80 && !bind_building(c, u)) return 0;
    if (t->type == 70 && t->unit_class == 0x12 && u->core.state == 2) {
        float v = (float)((double)player_attribute(c, u, 35) * c->tick +
                          u->core.storage.carried_amount);
        u->core.storage.carried_amount =
            (v < 100.0f || isnan(v)) ? v : 100.0f;
        u->core.storage.carried_resource_type = 0x22;
    }
    if (t->type >= 50 && t->type <= 80 && (s = peek_unit(c, u)) &&
        s->reload_timer > 0.0f) {
        s->reload_timer = (float)((double)s->reload_timer - c->tick);
        if (s->reload_timer < 0.0f) s->reload_timer = 0.0f;
    }
    return 1;
}

/* +0x1fc = 0x004063d0: 0x0043e670 indexes the template's task list
   (negative indices other than -1 would read before it in the original);
   the list's task factory +0x04 (0x004040b0 / tribe 0x004b4600) builds the
   action with no target at (-1, -1, -1), and a built action is adopted
   (+0x208). Action classes not translated yet are counted. */
static int default_task(void *opaque, AoeLiveUnit *u, int16_t index) {
    AoeWorldCombatRor *c = opaque;
    const DatUnit *t = u->core.unit_template;
    const DatUnitTask *task;
    AoeActionRouteRor route;
    void *action;
    if (index < 0 || (uint16_t)index >= t->task_count) return 1;
    task = &t->tasks[index];
    aoe_action_route_ror(order_tribe_template(c, t), task->action_type, 0,
                         &route);
    action = order_construct(c, u, &route, task, NULL, -1.0f, -1.0f, -1.0f);
    if (action)
        order_adopt(c, u, action);
    else if (route.constructor)
        ++c->default_tasks; /* e.g. farm 21, transport 12 */
    return 1;
}
static int attached_update(void *opaque, AoeLiveUnit *u, AoeActionList *l) {
    (void)u; (void)l;
    ++((AoeWorldCombatRor *)opaque)->attached_updates;
    return 1;
}

/* Derived +0x24 tails after the common predicate: 0x00415200 (dying
   states, sprites) for types 20..80, then 0x00406500's action list for
   types 40..80. A projectile's list holds its flight action. */
/* Doppelganger +0x24 tail 0x00436630 (after 0x00415200): a source in
   state 7+ is dropped (+0x8c = 0, +0xac = -1). When the shared mask at its
   tile changes: a doppelganger below state 3 whose player (with shared
   vision, +0xd8) now sees the tile leaves (+0x68); for each player bit in
   the owner's indirect mask (+0xdc) the owner's via mask (+0xe0 + 4i) is
   added to the hidden players (+0xa4). (A source whose slot no longer holds
   the same unit is also treated as gone: the runtime reuses slots.) */
static int doppel_update(AoeWorldCombatRor *c, AoeLiveUnit *u) {
    AoeScenarioRuntimeRor *r = c->motion->runtime;
    AoeScenarioRuntimePlayerRor *owner;
    AoeLiveUnit *src = u->doppel.source;
    uint32_t mask, bits;
    if (src && (!src->constructed ||
                src->core.unit_id != u->doppel.source_id ||
                src->core.state >= 7))
        u->doppel.source = NULL;
    if (!u->doppel.source) u->doppel.source_id = -1;
    if (!u->doppel.tile_mask) return 1;
    mask = *u->doppel.tile_mask;
    if (mask == u->doppel.last_mask) return 1;
    owner = owner_of(c, u);
    if (!owner) return 0;
    if ((owner->shared.visible & mask) && u->core.state < 3 &&
        !aoe_live_unit_scenario_high_root_ror(u, &owner->resources,
                                              &owner->owner, &r->world,
                                              &owner->vision_binding))
        return 0;
    bits = owner->shared.indirect & mask;
    for (int i = 0; bits && i < AOE_SHARED_VISION_PLAYERS; ++i, bits >>= 1)
        if (bits & 1) u->doppel.hidden_players |= owner->shared.via[i];
    u->doppel.last_mask = mask;
    return 1;
}

static int unit_after(void *opaque, AoeLiveUnit *u, uint8_t *result) {
    AoeWorldCombatRor *c = opaque;
    const DatUnit *t = u->core.unit_template;
    AoeLiveUnitUpdateStateOps so;
    AoeWorldProjectileRor *f;
    AoeWorldCombatUnitRor *s;
    AoeActionList empty, *list;
    AoeActionTickOps tick;
    AoeLiveUnitActionListOps lo;
    (void)result;
    if (t->type < 20 || t->type > 80) return 1;
    state_ops(c, &so);
    if (!aoe_live_unit_update_type20_tail_ror(
            u, &so, graphic_by_id(c, t->dying_graphic) != NULL))
        return 0;
    if (t->type == 25 && u->core.class_code == 0x19)
        return doppel_update(c, u);
    if (t->type < 40) return 1;
    if ((f = flight_of(c, u)) && f->active) {
        if (u->core.state > 2) {
            f->active = 0;
        } else {
            AoeProjectileOps po;
            int r;
            size_t i = (size_t)(f - c->flights);
            projectile_ops(c, &po);
            r = aoe_projectile_update_ror(&f->flight, &po);
            if (r < 0) return 0;
            /* The flights array may have grown during the update. */
            if (r == 1) c->flights[i].active = 0;
        }
        return 1;
    }
    s = peek_unit(c, u);
    memset(&empty, 0, sizeof(empty));
    list = u->derived_action_list ? u->derived_action_list : &empty;
    if (s && (list == &s->unit_actions ||
              (s->adopted_building && u->building_update &&
               list == u->building_update->live_actions)))
        tick = (AoeActionTickOps){s, unit_update_head, update_waiting};
    else
        tick = (AoeActionTickOps){c, foreign_head, update_waiting};
    lo = (AoeLiveUnitActionListOps){c, default_task, attached_update, &tick};
    return aoe_live_unit_update_action_list_ror(u, list, &lo);
}

/* Type 80 (0x004acf80) around the chain. The building view mirrors the
   live unit's state, frame byte +0x35 and carried resource across the
   call; its parent (0x004ae7d0 -> 0x00426bf0 -> 0x00406500) is the reload
   countdown, common predicate and tails, after aoe_building_parent_update_ror
   has applied the class-18 generation. */
typedef struct {
    AoeWorldCombatRor *c;
    AoeLiveUnit *u;
    int failed;
} BuildingTick;

static void building_to_view(AoeLiveUnit *u, AoeBuildingUpdate *b) {
    const DatGraphic *g = u->graphic.current_graphic;
    b->unit_template = u->core.unit_template;
    b->state = u->core.state;
    b->angle = u->core.scenario_frame;
    b->graphic_angles = g ? (int16_t)g->angle_count : 0;
    b->carried_amount = u->core.storage.carried_amount;
    b->carried_resource_type = u->core.storage.carried_resource_type;
}
static void view_to_building(AoeBuildingUpdate *b, AoeLiveUnit *u) {
    u->core.scenario_frame = b->angle;
    u->core.storage.carried_amount = b->carried_amount;
    u->core.storage.carried_resource_type = b->carried_resource_type;
}
static void bt_set_state(void *ctx, AoeBuildingUpdate *b, uint8_t state) {
    BuildingTick *t = ctx;
    view_to_building(b, t->u);
    if (!set_state(t->c, t->u, state)) t->failed = 1;
    building_to_view(t->u, b);
}
static uint8_t bt_parent(void *ctx, AoeBuildingUpdate *b) {
    BuildingTick *t = ctx;
    AoeWorldCombatRor *c = t->c;
    AoeLiveUnit *u = t->u;
    AoeScenarioRuntimePlayerRor *p = owner_of(c, u);
    AoeLiveUnitUpdateStateOps so;
    AoeLiveUnitUpdateTargetOps to = {c, set_state};
    AoeWorldCombatUnitRor *s = peek_unit(c, u);
    uint8_t predicate = 0;
    view_to_building(b, u);
    if (s && s->reload_timer > 0.0f) {
        s->reload_timer = (float)((double)s->reload_timer - c->tick);
        if (s->reload_timer < 0.0f) s->reload_timer = 0.0f;
    }
    state_ops(c, &so);
    if (!p ||
        !aoe_live_unit_update_predicate_ror(u, &p->owner,
                                            &c->motion->runtime->world, &to,
                                            &so, &predicate) ||
        !unit_after(c, u, &predicate))
        t->failed = 1;
    if (u->building_update == b) building_to_view(u, b);
    return predicate;
}
static void bt_refund(void *ctx, int16_t unit_id) {
    BuildingTick *t = ctx;
    AoeScenarioRuntimePlayerRor *p = owner_of(t->c, t->u);
    const DatUnit *tt;
    ++t->c->queue_refunds;
    if (p && unit_id >= 0 &&
        (tt = aoe_player_templates_get(&p->templates, (uint16_t)unit_id)))
        (void)aoe_player_cost_refund_ror(&p->resources, tt);
}
static void bt_cancel_notify(void *ctx, AoeTrainAction *a) {
    (void)a;
    ++((BuildingTick *)ctx)->c->production_events;
}
/* Queue-head cancel (head +0x2c): the training action's disc 0x004b6d90 (patched 0x004b5450). */
static void bt_cancel(void *ctx, void *action) {
    BuildingTick *t = ctx;
    AoeTrainCancelOps o = {t, bt_cancel_notify, bt_cancel_notify};
    if (!aoe_train_action_cancel_ror(action, &o)) t->failed = 1;
}
static void bt_notify(void *ctx, uint32_t e, int16_t o, uint32_t r) {
    (void)e; (void)o; (void)r;
    ++((BuildingTick *)ctx)->c->production_events;
}
static void *bt_create(void *ctx, AoeBuildingUpdate *b, int16_t unit_id) {
    return aoe_train_action_adapter_create(&((BuildingTick *)ctx)->c->train,
                                           b, unit_id);
}

static int building_update(AoeWorldCombatRor *c, AoeLiveUnit *u,
                           uint8_t *result) {
    AoeScenarioRuntimePlayerRor *p = owner_of(c, u);
    AoeBuildingUpdate *b;
    BuildingTick t = {c, u, 0};
    AoeBuildingProductionOps prod;
    AoeBuildingUpdateOps ops;
    uint8_t r = 0;
    int16_t o;
    if (!p || !bind_building(c, u) || !(b = u->building_update)) return 0;
    o = u->runtime_owner_index;
    building_to_view(u, b);
    b->owner_resources = p->resources;
    prod = (AoeBuildingProductionOps){&t, bt_refund, bt_cancel, bt_create,
                                      &c->train_tick, bt_notify};
    ops = (AoeBuildingUpdateOps){&t, bt_set_state, bt_parent, c->tick, &prod,
                                 &c->queue_status_four[o]};
    aoe_train_action_adapter_set_timing(&c->train, c->tick, 0);
    c->train.failed = 0;
    if (!aoe_building_update_ror(b, c->update_serial, &ops, &r)) return 0;
    if (u->building_update == b) view_to_building(b, u);
    if (t.failed || c->train.failed) return 0;
    if (result) *result = r;
    return 1;
}

/* Training bindings (adapter context: the world). */
static AoeLiveUnit *unit_of_view(AoeBuildingUpdate *b) {
    return aoe_scenario_runtime_building_unit_ror(b);
}
static int train_prepare(void *ctx, AoeBuildingUpdate *b, int16_t unit_id,
                         AoeTrainActionCreate *out) {
    AoeWorldCombatRor *c = ctx;
    AoeLiveUnit *u = unit_of_view(b);
    AoeScenarioRuntimePlayerRor *p = owner_of(c, u);
    if (!p) return 0;
    memset(out, 0, sizeof(*out));
    out->unit_id = unit_id;
    out->request_id = -1;
    out->building_template = u->core.unit_template;
    out->trained_template =
        unit_id >= 0 ? aoe_player_templates_get(&p->templates,
                                                 (uint16_t)unit_id)
                     : NULL;
    out->owner_resources = &p->resources;
    out->owner_id = u->runtime_owner_index;
    out->building_id = u->core.unit_id;
    out->building_x = u->core.position.x38;
    out->building_y = u->core.position.y3c;
    out->owned_action_ops = (AoeActionListOps){c, list_destroy_noop,
                                               list_activate_noop};
    out->unit_registry = &c->registry_view;
    return 1;
}
static int train_charge(void *ctx, AoeTrainAction *a) {
    int paid = 0;
    (void)ctx;
    return aoe_player_cost_pay_ror(a->owner_resources, a->trained_template,
                                   1.0f, 1, &paid) && paid;
}
static void train_failed(void *ctx, AoeTrainAction *a) {
    (void)a;
    ++((AoeWorldCombatRor *)ctx)->train_failures;
}
static void train_signal(void *ctx, AoeTrainAction *a, uint32_t code) {
    (void)a; (void)code;
    ++((AoeWorldCombatRor *)ctx)->production_events;
}
static void train_signal_failure(void *ctx, AoeTrainAction *a) {
    train_signal(ctx, a, 505);
}
static void train_notify(void *ctx, uint32_t e, int16_t o, int16_t id,
                         int32_t x, int32_t y) {
    (void)e; (void)o; (void)id; (void)x; (void)y;
    ++((AoeWorldCombatRor *)ctx)->production_events;
}
static void train_completed(void *ctx, AoeTrainAction *a, int32_t id) {
    (void)a; (void)id;
    ++((AoeWorldCombatRor *)ctx)->trained;
}
int aoe_world_combat_placement_ror(AoeWorldCombatRor *c, int16_t owner, const DatUnit *t,
                                   float x, float y, const int32_t flags[6],
                                   int32_t *blocker) {
    AoeScenarioRuntimeRor *r = c->motion->runtime;
    AoeUnitPlacementCheckContext check;
    int32_t b = -1;
    if (!t || !r->intro || t->terrain_restriction < 0 ||
        (uint16_t)t->terrain_restriction >= r->intro->restriction_count) {
        if (blocker) *blocker = -1;
        return AOE_PLACEMENT_OUT_OF_BOUNDS;
    }
    check = (AoeUnitPlacementCheckContext){
        &r->map,
        r->intro->terrain_restrictions +
            (size_t)(uint16_t)t->terrain_restriction *
                r->intro->terrain_count,
        r->intro->terrain_count, 0, NULL, 0, 0};
    if (owner >= 0 && owner < AOE_SCENARIO_RUNTIME_PLAYERS) {
        const AoeVisibilityFootprintGrid *g = &r->players[owner].vision.grid;
        check.owner_tile_masks = g->tile_masks;
        check.owner_visible_mask = g->visible_mask;
        check.owner_explored_mask = g->explored_mask;
    }
    {
        int code = aoe_unit_placement_check_ror(t, &check, x, y, &b, flags[0], (uint8_t)flags[1],
                                                flags[2], flags[3], flags[4], flags[5]);
        if (blocker) *blocker = b;
        return code;
    }
}
/* Template +0x20(owner, x, y, 0, 1, 1, 1, 0, overlap, 1): nonzero when
   the point is refused. */
static int placement_refused(AoeWorldCombatRor *c, const DatUnit *t,
                             int16_t owner, float x, float y,
                             int honor_overlap) {
    const int32_t flags[6] = {1, 1, 1, 0, honor_overlap, 1};
    return aoe_world_combat_placement_ror(c, owner, t, x, y, flags, NULL) != AOE_PLACEMENT_OK;
}
/* Trained template +0x20(owner, x, y, 0, 1, 1, 1, 0, 0, 1). */
static int train_blocked(void *ctx, AoeTrainAction *a, float x, float y) {
    AoeScenarioRuntimeRor *r = ((AoeWorldCombatRor *)ctx)->motion->runtime;
    const DatUnit *t = a->trained_template;
    AoeUnitPlacementCheckContext check;
    int32_t blocker = -1;
    if (!t || !r->intro || t->terrain_restriction < 0 ||
        (uint16_t)t->terrain_restriction >= r->intro->restriction_count)
        return 1;
    check = (AoeUnitPlacementCheckContext){
        &r->map,
        r->intro->terrain_restrictions +
            (size_t)(uint16_t)t->terrain_restriction *
                r->intro->terrain_count,
        r->intro->terrain_count, 0, NULL, 0, 0};
    if (a->owner_id >= 0 && a->owner_id < AOE_SCENARIO_RUNTIME_PLAYERS) {
        const AoeVisibilityFootprintGrid *g =
            &r->players[a->owner_id].vision.grid;
        check.owner_tile_masks = g->tile_masks;
        check.owner_visible_mask = g->visible_mask;
        check.owner_explored_mask = g->explored_mask;
    }
    return aoe_unit_placement_check_ror(t, &check, x, y, &blocker, 1, 1, 1, 0,
                                        0, 1) != AOE_PLACEMENT_OK;
}
/* Trained template +0x18(owner, x, y, world). */
static int train_create(void *ctx, AoeTrainAction *a, float x, float y,
                        int32_t *id) {
    AoeScenarioRuntimeRor *r = ((AoeWorldCombatRor *)ctx)->motion->runtime;
    AoeLiveUnit *u = NULL;
    if (!a->trained_template || a->owner_id < 0 ||
        a->owner_id >= AOE_SCENARIO_RUNTIME_PLAYERS ||
        !aoe_scenario_runtime_editor_create_ror(
            r, (uint8_t)a->owner_id, (uint16_t)a->trained_template->id, x, y,
            0.0f, &u) ||
        !u)
        return 0;
    *id = u->core.unit_id;
    return 1;
}
static AoeLiveUnit *unit_of_ref(const void *ref) {
    return (AoeLiveUnit *)((char *)ref - offsetof(AoeLiveUnit, core) -
                           offsetof(AoeLiveUnitCore, position));
}
static uint8_t train_target_state(void *ctx, const void *target) {
    (void)ctx;
    return unit_of_ref(target)->core.state;
}
static int32_t train_target_id(void *ctx, const void *target) {
    (void)ctx;
    return unit_of_ref(target)->core.unit_id;
}
static void train_detach(void *ctx, void *target, AoeTrainAction *a) {
    (void)a;
    target_released(ctx, unit_of_ref(target), NULL);
}
static void train_attach(void *ctx, void *target, AoeTrainAction *a,
                         int16_t type) {
    (void)a;
    target_attached(ctx, unit_of_ref(target), NULL, type);
}
static void train_ops(AoeWorldCombatRor *c) {
    AoeScenarioRuntimeRor *r = c->motion->runtime;
    c->train_activation = (AoeTrainActivationOps){c, train_charge,
                                                  train_failed,
                                                  train_signal_failure};
    c->train_spawn = (AoeTrainSpawnOps){c, train_blocked, train_create};
    c->train_update = (AoeTrainActionOps){c, &r->rng, &c->train_spawn,
                                          train_notify, train_failed,
                                          train_completed, train_signal};
    c->train_targets = (AoeTrainTargetOps){c, train_target_id, train_detach,
                                           train_attach};
    c->train_waiting = (AoeTrainWaitingOps){c, train_target_state,
                                            &c->train_targets};
    memset(&c->train, 0, sizeof(c->train));
    c->train.context = c;
    c->train.prepare = train_prepare;
    c->train.activation = &c->train_activation;
    c->train.update = &c->train_update;
    c->train.waiting = &c->train_waiting;
    c->train.targets = &c->train_targets;
    aoe_train_action_adapter_tick_ops(&c->train, &c->train_tick);
}
static void refresh_registry(AoeWorldCombatRor *c) {
    const AoeVisibilityRegistry *v = &c->motion->runtime->world.view;
    c->registry_view = (AoeLiveUnitRegistry){
        (void *const *)v->positive, v->positive_count,
        (void *const *)v->negative, v->negative_count};
}

/* Queue add bindings. */
typedef struct {
    AoeWorldCombatRor *c;
    AoeScenarioRuntimePlayerRor *p;
} QueueContext;
static const DatUnit *queue_template(void *ctx, int16_t index) {
    QueueContext *q = ctx;
    return index < 0 ? NULL
                     : aoe_player_templates_get(&q->p->templates,
                                                (uint16_t)index);
}
static void *queue_calloc(void *ctx, size_t n, size_t size) {
    (void)ctx;
    return calloc(n, size);
}
static void queue_free(void *ctx, void *p) {
    (void)ctx;
    free(p);
}
static void queue_notify_event(void *ctx, uint32_t e, int16_t o,
                               uint32_t r) {
    QueueContext *q = ((AoePlayerCostBindingRor *)ctx)->context;
    (void)e; (void)o; (void)r;
    ++q->c->production_events;
}

int aoe_world_combat_train_ror(AoeWorldCombatRor *c, AoeLiveUnit *u,
                               int16_t unit_id, int16_t count) {
    AoeScenarioRuntimePlayerRor *p;
    QueueContext q;
    AoePlayerCostBindingRor cost;
    AoeBuildingQueueAddOps ops;
    BuildingTick t;
    if (!c || !u || !u->core.unit_template ||
        u->core.unit_template->type != 80 || !(p = owner_of(c, u)) ||
        !bind_building(c, u))
        return 0;
    q = (QueueContext){c, p};
    cost = (AoePlayerCostBindingRor){&p->resources, &q, queue_template};
    t = (BuildingTick){c, u, 0};
    building_to_view(u, u->building_update);
    u->building_update->owner_resources = p->resources;
    memset(&ops, 0, sizeof(ops));
    ops.context = &cost;
    ops.can_afford = aoe_player_cost_queue_afford_ror;
    ops.pay = aoe_player_cost_queue_pay_ror;
    ops.notify = queue_notify_event;
    ops.allocate_zeroed = queue_calloc;
    ops.deallocate = queue_free;
    ops.cancel = (AoeBuildingQueueCancelOps){&t, bt_refund, bt_cancel,
                                             bt_notify};
    ops.owner_status_four_count = &c->queue_status_four[u->runtime_owner_index];
    return aoe_building_queue_add_ror(u->building_update, unit_id, count,
                                      &ops) &&
           !t.failed;
}

/* Research action bindings (context: the world). */
static void research_notify(void *ctx, uint32_t e, int16_t o, int16_t id,
                            int32_t x, int32_t y) {
    (void)e; (void)o; (void)id; (void)x; (void)y;
    ++((AoeWorldCombatRor *)ctx)->production_events;
}
static void research_failed(void *ctx, const AoeResearchAction *a,
                            int16_t reason) {
    (void)a; (void)reason;
    ++((AoeWorldCombatRor *)ctx)->research_stopped;
}
static void research_completed(void *ctx, const AoeResearchAction *a) {
    (void)a;
    ++((AoeWorldCombatRor *)ctx)->researched;
}
static void research_signal(void *ctx, const AoeResearchAction *a,
                            uint32_t code) {
    (void)a; (void)code;
    ++((AoeWorldCombatRor *)ctx)->production_events;
}
static int research_ops(AoeWorldCombatRor *c, int16_t owner,
                        AoeResearchActionOps *o) {
    AoeScenarioRuntimeRor *r = c->motion->runtime;
    AoeScenarioRuntimePlayerRor *p;
    if (owner < 0 || owner >= AOE_SCENARIO_RUNTIME_PLAYERS || !r->techs)
        return 0;
    p = &r->players[owner];
    memset(o, 0, sizeof(*o));
    o->context = c;
    o->owner = (AoeResearchOwnerRor){&p->research, r->techs, &p->resources};
    o->notify = research_notify;
    o->failed = research_failed;
    o->completed = research_completed;
    o->signal = research_signal;
    o->tick = c->tick;
    return 1;
}

/* Command 0x64/0x65 bindings. */
static AoeLiveUnit *command_resolve(void *c, int32_t id) {
    return resolve(c, id);
}
static int16_t command_busy(void *c, const AoeLiveUnit *u) {
    const AoeActionList *list = u->derived_action_list;
    const void *head;
    (void)c;
    if (!list || !list->head || !(head = list->head->action)) return 0;
    return entry_type(head) == AOE_WORLD_COMBAT_RECORD_TAG ? 0
                                                           : entry_type(head);
}
static const DatUnit *command_template(void *c, const AoeLiveUnit *b,
                                       int16_t index) {
    AoeScenarioRuntimePlayerRor *p = owner_of(c, (AoeLiveUnit *)b);
    return p && index >= 0
               ? aoe_player_templates_get(&p->templates, (uint16_t)index)
               : NULL;
}
static int16_t command_research_location(void *c, const AoeLiveUnit *b,
                                         int16_t tech) {
    const DatTechs *t = ((AoeWorldCombatRor *)c)->motion->runtime->techs;
    (void)b;
    return t && tech >= 0 && (uint16_t)tech < t->count
               ? t->techs[tech].research_location
               : -1;
}
typedef struct {
    AoeWorldCombatRor *c;
    int failed;
} CommandContext;
/* Building +0x208 = 0x00406490 on the adopted +0x184 list. */
static void command_adopt(AoeWorldCombatRor *c, AoeLiveUnit *b,
                          void *action, int *failed) {
    AoeActionList *list = b->derived_action_list;
    if (!action) {
        *failed = 1;
        return;
    }
    if (!bind_building(c, b) || !list ||
        list != b->building_update->live_actions) {
        AoeWorldCombatUnitRor *s = peek_unit(c, b);
        if (s) head_destroy(s, action);
        else free(action);
        *failed = 1;
        return;
    }
    (void)aoe_action_list_clear(list);
    if (!aoe_action_list_push(list, action)) {
        list->ops.destroy(list->ops.context, action);
        *failed = 1;
    }
}
static void command_train(void *ctx, AoeLiveUnit *b, int16_t unit,
                          int32_t request) {
    CommandContext *k = ctx;
    AoeTrainActionCreate create;
    AoeTrainAction *a = NULL;
    if (!bind_building(k->c, b)) {
        k->failed = 1;
        return;
    }
    building_to_view(b, b->building_update);
    aoe_train_action_adapter_set_timing(&k->c->train, k->c->tick, 0);
    if (train_prepare(k->c, b->building_update, unit, &create)) {
        create.request_id = request;
        create.prepaid = 0;
        a = aoe_train_action_create_ror(&create);
    }
    command_adopt(k->c, b, a, &k->failed);
}
static void command_research(void *ctx, AoeLiveUnit *b, int16_t tech,
                             int32_t request) {
    CommandContext *k = ctx;
    AoeResearchAction *a = calloc(1, sizeof(*a));
    if (a) {
        aoe_research_action_init_ror(a, tech, request);
        a->building_id = b->core.unit_id;
        a->owner_index = b->runtime_owner_index;
        a->building_x = b->core.position.x38;
        a->building_y = b->core.position.y3c;
        a->work_rate = b->core.unit_template->work_rate;
    }
    command_adopt(k->c, b, a, &k->failed);
}
static int command_no_group(void *c, const AoeLiveUnit *u) {
    (void)c; (void)u;
    return 0;
}
static void command_group(void *c, AoeLiveUnit *b, int32_t request) {
    (void)c; (void)b; (void)request;
}
static void command_player_train(void *ctx, uint8_t player, int16_t unit,
                                 int32_t building, int32_t request) {
    (void)player; (void)unit; (void)building; (void)request;
    ++((CommandContext *)ctx)->c->production_fallbacks;
}
static void command_player_research(void *ctx, uint8_t player, int16_t tech,
                                    int32_t building, int32_t request) {
    (void)player; (void)tech; (void)building; (void)request;
    ++((CommandContext *)ctx)->c->production_fallbacks;
}
static int16_t command_busy_ctx(void *ctx, const AoeLiveUnit *u) {
    return command_busy(((CommandContext *)ctx)->c, u);
}
static const DatUnit *command_template_ctx(void *ctx, const AoeLiveUnit *b,
                                           int16_t index) {
    return command_template(((CommandContext *)ctx)->c, b, index);
}
static int16_t command_location_ctx(void *ctx, const AoeLiveUnit *b,
                                    int16_t tech) {
    return command_research_location(((CommandContext *)ctx)->c, b, tech);
}

int aoe_world_combat_command_production_ror(AoeWorldCombatRor *c,
                                            const uint8_t *command,
                                            size_t size) {
    CommandContext k = {c, 0};
    AoeCommandProductionOps ops;
    int result;
    if (!c || !c->motion || !c->motion->runtime) return -1;
    refresh_registry(c);
    memset(&ops, 0, sizeof(ops));
    ops.world.context = c;
    ops.world.resolve = command_resolve;
    ops.context = &k;
    ops.active_action_type = command_busy_ctx;
    ops.owner_unit_template = command_template_ctx;
    ops.research_location = command_location_ctx;
    ops.start_train = command_train;
    ops.start_research = command_research;
    ops.has_group_object = command_no_group;
    ops.group_production = command_group;
    ops.player_train = command_player_train;
    ops.player_research = command_player_research;
    result = aoe_command_execute_production_ror(command, size, &ops);
    return k.failed ? -1 : result;
}

/* ---- 0x66, building placement ---- */
static int place_player_present(void *ctx, uint8_t player) {
    AoeScenarioRuntimeRor *r = ((CommandContext *)ctx)->c->motion->runtime;
    return player < AOE_SCENARIO_RUNTIME_PLAYERS && (int)player < r->player_count;
}
static const DatUnit *place_template(void *ctx, uint8_t player, int16_t index) {
    AoeScenarioRuntimeRor *r = ((CommandContext *)ctx)->c->motion->runtime;
    if (index < 0 || player >= AOE_SCENARIO_RUNTIME_PLAYERS) return NULL;
    return aoe_player_templates_get(&r->players[player].templates, (uint16_t)index);
}
static AoePlayerResources *place_resources(void *ctx, uint8_t player) {
    AoeScenarioRuntimeRor *r = ((CommandContext *)ctx)->c->motion->runtime;
    return player < AOE_SCENARIO_RUNTIME_PLAYERS ? &r->players[player].resources : NULL;
}
static uint8_t place_check(void *ctx, uint8_t player, const DatUnit *t, float x, float y,
                           uint32_t *out, const int32_t flags[6]) {
    int32_t blocker = -1;
    int code = aoe_world_combat_placement_ror(((CommandContext *)ctx)->c, (int16_t)player, t, x,
                                              y, flags, &blocker);
    if (out) *out = (uint32_t)blocker;
    return (uint8_t)code;
}
/* Player +0x28 list 0x00451370(id, ...): the player's own object. */
static AoeLiveUnit *place_find(void *ctx, uint8_t player, int16_t id) {
    AoeLiveUnit *u = resolve(((CommandContext *)ctx)->c, id);
    return u && u->runtime_owner_index == (int16_t)player ? u : NULL;
}
static void place_failed(void *ctx, uint8_t player, uint8_t code, uint32_t out, int16_t index,
                         uint32_t x_bits, uint32_t y_bits, uint32_t value_10,
                         int32_t builder_id) {
    (void)player; (void)code; (void)out; (void)index; (void)x_bits; (void)y_bits;
    (void)value_10; (void)builder_id;
    ++((CommandContext *)ctx)->c->placement_failures;
}
/* Template +0x3c(player, x, y, 0, +0x10): the unit made as the editor
   makes one; a type-80 unit starts as a foundation (0x004ac8a0). */
static AoeLiveUnit *place_create(void *ctx, uint8_t player, const DatUnit *t, float x, float y,
                                 uint32_t value_10) {
    AoeWorldCombatRor *c = ((CommandContext *)ctx)->c;
    AoeLiveUnit *u = NULL;
    (void)value_10;
    if (!t || !aoe_scenario_runtime_editor_create_ror(c->motion->runtime, player, (uint16_t)t->id,
                                                      x, y, 0.0f, &u) ||
        !u)
        return NULL;
    ++c->placements;
    return u;
}
static int32_t place_owner_kind(void *ctx, const AoeLiveUnit *u) {
    return owner_kind_cb(((CommandContext *)ctx)->c, (AoeLiveUnit *)u);
}
/* Unit +0x98(target, x, y, 0): the builder's default order on the foundation. */
static void place_order(void *ctx, AoeLiveUnit *u, AoeLiveUnit *target, float x, float y) {
    int16_t code = 0;
    (void)aoe_world_combat_order_ror(((CommandContext *)ctx)->c, u, target, x, y, 0.0f, &code);
}

int aoe_world_combat_command_place_ror(AoeWorldCombatRor *c, const uint8_t *command,
                                       size_t size) {
    CommandContext k = {c, 0};
    AoeCommandPlaceOps ops;
    if (!c || !c->motion || !c->motion->runtime) return -1;
    refresh_registry(c);
    memset(&ops, 0, sizeof ops);
    ops.world.context = c;
    ops.world.resolve = command_resolve;
    ops.context = &k;
    ops.player_present = place_player_present;
    ops.player_template = place_template;
    ops.resources = place_resources;
    ops.placement_check = place_check;
    ops.find_object = place_find;
    ops.placement_failed = place_failed;
    ops.create = place_create;
    ops.owner_kind = place_owner_kind;
    ops.order = place_order;
    return aoe_command_execute_place_ror(command, size, &ops);
}

/* One unit through the whole chain, outside the player list. */
static int update_unit(AoeWorldCombatRor *c, AoeLiveUnit *u,
                       uint8_t *result) {
    AoeScenarioRuntimeRor *r = c->motion->runtime;
    AoeScenarioRuntimePlayerRor *p = owner_of(c, u);
    AoeLiveUnitUpdateStateOps so;
    AoeLiveUnitUpdateTargetOps to = {c, set_state};
    AoeWorldCombatUnitRor *s = peek_unit(c, u);
    uint8_t predicate = 0;
    if (!p) return 0;
    if (s) s->last_result = 0;
    if (u->core.unit_template->type == 80) {
        uint8_t r8 = 0;
        if (!building_update(c, u, &r8)) return 0;
        if (result) *result = (s = peek_unit(c, u)) ? s->last_result : 0;
        return 1;
    }
    state_ops(c, &so);
    if (!unit_before(c, u) ||
        !aoe_live_unit_update_predicate_ror(u, &p->owner, &r->world, &to, &so,
                                            &predicate) ||
        !unit_after(c, u, &predicate))
        return 0;
    if (result) *result = s ? s->last_result : 0;
    return 1;
}

int aoe_world_combat_tick_ror(AoeWorldCombatRor *c, AoeLiveUnit *attacker,
                              float tick, uint8_t *result) {
    if (!c || !attacker || !result || !aoe_world_combat_unit_ror(c, attacker))
        return 0;
    *result = 0;
    c->tick = tick;
    c->world_ms += (uint32_t)(tick * 1000.0f + 0.5f);
    return update_unit(c, attacker, result);
}

int aoe_world_combat_update_projectiles_ror(AoeWorldCombatRor *c) {
    if (!c) return 0;
    for (size_t i = 0; i < c->flight_count; ++i) {
        AoeLiveUnit *u = c->flights[i].unit;
        if (!c->flights[i].active) continue;
        if (!update_unit(c, u, NULL)) return 0;
    }
    return 1;
}

int aoe_world_combat_update_unit_ror(AoeWorldCombatRor *c, AoeLiveUnit *unit,
                                     float tick) {
    if (!c || !unit) return 0;
    c->tick = tick;
    return update_unit(c, unit, NULL);
}

/* The world loop's list binding. */
static AoeLiveUnit *resolve_live(void *opaque, void *object) {
    AoeScenarioRuntimeRor *r = ((AoeWorldCombatRor *)opaque)->motion->runtime;
    uintptr_t o = (uintptr_t)object;
    for (unsigned p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p) {
        AoeScenarioRuntimePlayerRor *pl = &r->players[p];
        uintptr_t lo = (uintptr_t)pl->units, hi;
        size_t i;
        if (!pl->units || !pl->unit_count) continue;
        hi = (uintptr_t)(pl->units + pl->unit_count);
        if (o < lo || o >= hi) continue;
        i = (o - lo) / sizeof(*pl->units);
        return &pl->units[i].core.position == object ? &pl->units[i] : NULL;
    }
    for (size_t i = 0; i < r->editor_unit_count; ++i)
        if (r->editor_units[i] &&
            &r->editor_units[i]->core.position == object)
            return r->editor_units[i];
    return NULL;
}
static int live_update(void *opaque, AoeLiveUnit *u, uint8_t *result) {
    if (u->core.unit_template->type != 80) return -1;
    return building_update(opaque, u, result);
}
static int other_predicate(void *opaque, void *object) {
    (void)object;
    ++((AoeWorldCombatRor *)opaque)->foreign_objects;
    return 0;
}
static void other_action(void *opaque, void *object) {
    (void)opaque; (void)object;
}
static int no_cooperative(void *opaque) { (void)opaque; return 0; }
static void no_pump(void *opaque) { (void)opaque; }
static uint32_t *status_four(void *opaque, int16_t owner) {
    AoeWorldCombatRor *c = opaque;
    if (owner < 0 || owner >= AOE_SCENARIO_RUNTIME_PLAYERS) return NULL;
    return &c->queue_status_four[owner];
}
static void free_queue(void *opaque, void *allocation) {
    (void)opaque;
    free(allocation);
}

/* Forgets everything this world holds for a unit about to be removed. */
static void release_unit(AoeWorldCombatRor *c, AoeLiveUnit *u) {
    AoeWorldCombatUnitRor *s = peek_unit(c, u);
    AoeWorldProjectileRor *f = flight_of(c, u);
    if (f) {
        f->active = 0;
        /* 0x004496a0 (the projectile's destructor): a private template copy
           (+0x1c0) gives way to the owner's template of its ID; the copy
           itself is freed with the flight. */
        if (f->captured && u->core.unit_template == f->captured)
            u->core.unit_template = f->original;
        f->unit = NULL;
    }
    if (s) {
        int64_t slot = (int64_t)u->core.unit_id + AOE_WORLD_MOTION_ID_OFFSET;
        aoe_action_list_clear(&s->sub_actions);
        aoe_action_list_clear(&s->unit_actions);
        if (u->derived_action_list == &s->unit_actions)
            u->derived_action_list = NULL;
        release_building(s);
        free(s);
        c->units[slot] = NULL;
    }
    aoe_world_motion_release_ror(c->motion, u);
}

/* +0x08 after the derived prefixes: the portable owner/world/tile/sprite
   removal (see aoe_live_unit_destroy_ror for what it still omits). */
static int common_cleanup(void *opaque, AoeLiveUnit *u) {
    AoeWorldCombatRor *c = opaque;
    AoeScenarioRuntimeRor *r = c->motion->runtime;
    AoeScenarioRuntimePlayerRor *p = owner_of(c, u);
    if (!p) return 0;
    release_unit(c, u);
    aoe_live_unit_destroy_ror(u, &p->owner, &r->world, &p->vision_binding);
    ++c->removed;
    return 1;
}

static void no_state_event(void *user, int code, int16_t player) {
    (void)user; (void)code; (void)player;
}
/* Player map update 0x00517060: for each cell newly explored by the
   player since the last world tick (player +0x58, 0x0045ebc0), every
   object on the tile whose template has a notify mode (+0xa5) is added to
   the player's +0x114 memory under +0xa7 by 0x00517130, which looks the
   unit up in the world's positive-ID table and stores its tile x/y.
   (Negative IDs would index before that table in the original; such
   objects are skipped.) */
static void map_update(void *user, AoePlayerUpdateState *owner) {
    AoeWorldCombatRor *c = user;
    AoeScenarioRuntimeRor *r = c->motion->runtime;
    AoeScenarioRuntimePlayerRor *pl = NULL;
    for (unsigned p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p)
        if (&r->players[p].owner == owner) pl = &r->players[p];
    if (!pl || !pl->object_memory.category_count) return;
    for (size_t i = 0; i < pl->vision.owner_cells58.count; ++i) {
        const AoeExploredCell *cell = &pl->vision.owner_cells58.cells[i];
        const AoeObjectUpdateNode *node;
        if (cell->x < 0 || cell->y < 0 ||
            (uint32_t)cell->x >= r->map.width ||
            (uint32_t)cell->y >= r->map.height)
            continue;
        for (node = r->map.objects[(size_t)cell->y * r->map.width +
                                   (uint32_t)cell->x].head;
             node; node = node->next) {
            const AoeLiveUnit *u = node->object;
            const AoeVisibilityUnitRef *ref;
            AoePlayerMemoryEntry entry;
            if (!u || !u->core.unit_template ||
                !u->core.unit_template->state_notify_mode ||
                u->core.unit_id < 0)
                continue;
            ref = aoe_visibility_registry_resolve(&r->world.view,
                                                  u->core.unit_id);
            if (!ref) continue;
            memset(&entry, 0, sizeof(entry));
            entry.unit_id = u->core.unit_id;
            entry.x = (uint8_t)aoe_ftol32(ref->x38);
            entry.y = (uint8_t)aoe_ftol32(ref->y3c);
            if (!aoe_player_object_memory_add(
                    &pl->object_memory,
                    u->core.unit_template->state_notify_value, &entry))
                ++c->memory_add_failures;
        }
    }
}

int aoe_world_combat_update_world_ror(AoeWorldCombatRor *c, float tick) {
    AoeScenarioRuntimeRor *r;
    if (!c || !c->motion || !(r = c->motion->runtime) || !isfinite(tick) ||
        tick < 0.0f)
        return 0;
    c->tick = tick;
    c->world_ms += (uint32_t)(tick * 1000.0f + 0.5f);
    ++c->update_serial; /* 0x0051da32: global 0x7d2460 */
    refresh_registry(c);
    for (unsigned p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p) {
        AoeScenarioRuntimePlayerRor *pl = &r->players[p];
        AoeLiveUnitPlayerUpdateBinding b;
        AoePlayerUpdateOps po;
        if (!pl->owner.outcome_manager) {
            c->outcome_player[p].index4a = (int16_t)p;
            c->outcome_player[p].outcome34 = &c->outcome[p];
            c->outcome[p].owner = &c->outcome_player[p];
            pl->owner.outcome_manager = &c->outcome[p];
        }
        memset(&b, 0, sizeof(b));
        b.owner = &pl->owner;
        b.world = &r->world;
        b.target_ops = (AoeLiveUnitUpdateTargetOps){c, set_state};
        state_ops(c, &b.state_ops);
        b.context = c;
        b.common_cleanup = common_cleanup;
        b.queue_status_four_count = status_four;
        b.deallocate_queue_entries = free_queue;
        b.other_predicate = other_predicate;
        b.other_action = other_action;
        b.cooperative_pending = no_cooperative;
        b.cooperative_pump = no_pump;
        b.resolve_live = resolve_live;
        b.before_update = unit_before;
        b.after_update = unit_after;
        b.live_update = live_update;
        memset(&po, 0, sizeof(po));
        po.user = c;
        po.emit_state_event = no_state_event;
        po.map_update = map_update;
        if (!aoe_live_unit_player_update_step_ror(&b, &pl->owner, &po))
            return 0;
        /* The world tick then resets the player +0x58 queue (0x0045fad0
           disc, see player_queue.c). */
        aoe_explored_cells_reset(&pl->vision.owner_cells58);
    }
    /* Second pass over every player's +0x30 list (type-25 doppelgangers),
       bracketed by r = rand() ... srand(r) (around 0x0051daae). */
    {
        uint32_t reseed = aoe_rng_next(&r->rng);
        for (unsigned p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p) {
            AoeScenarioRuntimePlayerRor *pl = &r->players[p];
            AoeLiveUnitPlayerUpdateBinding b;
            if (!pl->owner.object_list30.head) continue;
            memset(&b, 0, sizeof(b));
            b.owner = &pl->owner;
            b.world = &r->world;
            b.target_ops = (AoeLiveUnitUpdateTargetOps){c, set_state};
            state_ops(c, &b.state_ops);
            b.context = c;
            b.common_cleanup = common_cleanup;
            b.queue_status_four_count = status_four;
            b.deallocate_queue_entries = free_queue;
            b.other_predicate = other_predicate;
            b.other_action = other_action;
            b.cooperative_pending = no_cooperative;
            b.cooperative_pump = no_pump;
            b.resolve_live = resolve_live;
            b.before_update = unit_before;
            b.after_update = unit_after;
            b.live_update = live_update;
            if (!aoe_live_unit_player_list_update_ror(
                    &b, &pl->owner.object_list30))
                return 0;
        }
        aoe_rng_seed(&r->rng, reseed);
    }
    return 1;
}

/* ---- repair action (0x6a) bindings ---------------------------------- */

/* Sub-action walks for heap actions, as build_start_move. */
static int heap_start_move(AoeWorldCombatRor *w, AoeMoveActionRor *base,
                           AoeLiveUnit *t, float range, const void *graphic,
                           int quiet_first) {
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(w, base->unit);
    AoeMoveActionOps o;
    AoeMoveActionRor *mv;
    const DatUnit *tt = base->unit->core.unit_template;
    int ok;
    if (!s || !(mv = calloc(1, sizeof(*mv)))) return 0;
    move_ops(w, &o);
    ok = t ? aoe_move_action_init_target_ror(
                 mv, base->unit, t, range, graphic,
                 graphic_by_id(w, tt->walking_graphic), &o)
           : aoe_move_action_init_point_ror(
                 mv, base->unit, base->x, base->y, base->z, range, graphic,
                 graphic_by_id(w, tt->walking_graphic));
    if (!ok) {
        free(mv);
        return 0;
    }
    if (quiet_first) mv->quiet = 1;
    if (!aoe_action_list_push(sub_list_of(w, base), mv)) {
        free(mv);
        return 0;
    }
    mv->quiet = 1;
    return 1;
}
static uint8_t heap_sub_update(AoeWorldCombatRor *c, AoeMoveActionRor *base) {
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(c, base->unit);
    AoeActionTickOps t = {c, update_head, update_waiting};
    uint8_t r = 0;
    if (!s || !aoe_action_list_update_ror(sub_list_of(c, base), &t, &r))
        return 0;
    return r;
}
static void heap_sub_clear(AoeWorldCombatRor *c, AoeMoveActionRor *base) {
    AoeWorldCombatUnitRor *s = aoe_world_combat_unit_ror(c, base->unit);
    if (s) (void)aoe_action_list_clear(sub_list_of(c, base));
}
static int repair_start_move(void *c, AoeRepairActionRor *a, AoeLiveUnit *t,
                             float range, const void *graphic,
                             int quiet_first) {
    return heap_start_move(c, &a->base, t, range, graphic, quiet_first);
}
static uint8_t repair_sub_update(void *c, AoeRepairActionRor *a) {
    return heap_sub_update(c, &a->base);
}
static void repair_sub_clear(void *c, AoeRepairActionRor *a) {
    heap_sub_clear(c, &a->base);
}
static double repair_distance(void *c, AoeLiveUnit *u, AoeLiveUnit *t) {
    (void)c;
    return aoe_combat_distance_ror(u, t);
}
/* Owner +0x1c = 0x0045bd90: the owner itself, or a player 1..n-1 whose
   stance byte (+0x84) is 0. */
static int repair_allowed(void *c, AoeLiveUnit *u, AoeLiveUnit *t) {
    AoeScenarioRuntimeRor *r = ((AoeWorldCombatRor *)c)->motion->runtime;
    int16_t o = u->runtime_owner_index, k = t->runtime_owner_index;
    if (k == o) return 1;
    return o >= 0 && o < AOE_SCENARIO_RUNTIME_PLAYERS && k > 0 &&
           k < r->player_count && r->diplomacy[o][k] == 0;
}
/* Owner 0x004f1750(template ID, multiplier, population flag). */
static int repair_pay(void *c, AoeLiveUnit *u, float mult, int pop_flag) {
    AoeScenarioRuntimePlayerRor *p = owner_of(c, u);
    const DatUnit *t;
    int paid = 0;
    if (!p || !(t = aoe_player_templates_get(
                    &p->templates, (uint16_t)u->core.unit_template->id)))
        return 0;
    return aoe_player_cost_pay_ror(&p->resources, t, mult, pop_flag, &paid) &&
           paid;
}
static uint8_t repair_work(void *c, AoeLiveUnit *t, float amount) {
    int r = aoe_live_unit_repair_ror(t, amount, repair_pay, c);
    if (r < 0) {
        ++((AoeWorldCombatRor *)c)->repair_failures;
        return 0;
    }
    if (r == 1) ++((AoeWorldCombatRor *)c)->repairs;
    return (uint8_t)r;
}
static void repair_event(void *c, uint32_t e, int16_t o, int32_t id) {
    (void)e; (void)o; (void)id;
    ++((AoeWorldCombatRor *)c)->repair_unpaid;
}
static uint8_t repair_order_serial(void *c, AoeLiveUnit *u) {
    AoeWorldCombatUnitRor *s = peek_unit(c, u);
    return s ? s->order_serial : 0;
}
/* Owner list +0x28 (the player's primary object list) in order; its
   objects are unit position references (see resolve). */
static AoeLiveUnit *repair_owner_unit(void *c, AoeLiveUnit *u, uint32_t i) {
    AoeScenarioRuntimePlayerRor *p = owner_of(c, u);
    const AoeObjectUpdateNode *n;
    if (!p) return NULL;
    for (n = p->owner.object_list28.head; n; n = n->next)
        if (n->object && i-- == 0)
            return (AoeLiveUnit *)((char *)n->object -
                                   offsetof(AoeLiveUnit, core) -
                                   offsetof(AoeLiveUnitCore, position));
    return NULL;
}
static void *head_of(AoeLiveUnit *u) {
    const AoeActionList *l = u->derived_action_list;
    return l && l->head ? l->head->action : NULL;
}
/* +0xcc = 0x00406650: the head action's +0x4c target. */
static AoeLiveUnit *repair_head_target(void *c, AoeLiveUnit *u) {
    void *h = head_of(u);
    return h ? order_action_target(c, h) : NULL;
}
/* +0x134 = 0x00406740: the head action's state, 0 without one. */
static uint8_t repair_head_state(void *c, AoeLiveUnit *u) {
    void *h = head_of(u);
    AoeWorldCombatUnitRor *s;
    (void)c;
    if (!h) return 0;
    if (entry_type(h) != AOE_WORLD_COMBAT_RECORD_TAG)
        return ((AoeMoveActionRor *)h)->state;
    s = h;
    return s->constructing ? s->build.base.state
           : s->gathering  ? s->gather.base.state
           : s->attacking  ? s->attack.base.state : 0;
}
/* 0x00407670 / 0x00407680: the head action's +0x20/+0x24. */
static void repair_head_destination(void *c, AoeLiveUnit *u, float *x,
                                    float *y) {
    void *h = head_of(u);
    (void)c;
    if (h && entry_type(h) != AOE_WORLD_COMBAT_RECORD_TAG) {
        *x = ((AoeMoveActionRor *)h)->x;
        *y = ((AoeMoveActionRor *)h)->y;
    }
}
/* 0x00447de0; off-map tiles match no terrain. */
static int repair_terrain(void *c, int32_t x, int32_t y) {
    uint8_t t = 0;
    if (x < INT16_MIN || x > INT16_MAX || y < INT16_MIN || y > INT16_MAX ||
        !task_terrain(c, NULL, (int16_t)x, (int16_t)y, &t))
        return -1;
    return t;
}
/* Unit +0x114 = 0x0044da60(x, y, 0): 0x0045a4f0(unit, x, y, 1). */
static int repair_fits(void *c, AoeLiveUnit *u, float x, float y) {
    return aoe_world_motion_fits_ror(((AoeWorldCombatRor *)c)->motion, u, x,
                                     y, 1) == 1;
}
/* Unit +0x154 = 0x004272b0 (types 50..80; root 0x00405a00 refuses).
   Unforced, a head move (type 1) already bound for (x, y, z) is kept;
   a head that accepts the walk (+0x30) is kept; otherwise a new move
   action (0x00405250, graphic template +0xc0 or +0xbc) replaces the list
   (+0x208). */
static int unit_move_to(void *c, AoeLiveUnit *u, float x, float y, float z,
                        float range, int force) {
    AoeWorldCombatRor *w = c;
    const DatUnit *t = u->core.unit_template;
    void *h = head_of(u);
    AoeMoveActionRor *mv;
    int16_t g;
    if (!t || t->type < 50) return 0;
    if (!force && h && entry_type(h) == 1 &&
        ((AoeMoveActionRor *)h)->x == x && ((AoeMoveActionRor *)h)->y == y &&
        ((AoeMoveActionRor *)h)->z == z)
        return 1;
    if (h && entry_type(h) == 0x6a) {
        AoeRepairActionOps o;
        repair_ops(w, &o);
        if (aoe_repair_action_walk_ror(h, x, y, z, &o)) return 1;
    }
    g = t->walking_graphic2 >= 0 ? t->walking_graphic2 : t->walking_graphic;
    if (!(mv = calloc(1, sizeof(*mv)))) return 0;
    if (!aoe_move_action_init_point_ror(mv, u, x, y, z, range,
                                        graphic_by_id(w, g),
                                        graphic_by_id(w, t->walking_graphic))) {
        free(mv);
        return 0;
    }
    order_adopt(w, u, mv);
    if ((h = peek_unit(w, u))) ++((AoeWorldCombatUnitRor *)h)->order_serial;
    return 1;
}
static void repair_ops(AoeWorldCombatRor *c, AoeRepairActionOps *o) {
    memset(o, 0, sizeof(*o));
    move_ops(c, &o->move);
    o->graphic = graphic_id_cb;
    o->tick_seconds = tick_seconds;
    o->distance = repair_distance;
    o->face = face_target;
    o->may_repair = repair_allowed;
    o->repair = repair_work;
    o->event = repair_event;
    o->play_sound = play_sound_cb;
    o->start_move = repair_start_move;
    o->sub_update = repair_sub_update;
    o->sub_clear = repair_sub_clear;
    o->order_serial = repair_order_serial;
    o->owner_unit_at = repair_owner_unit;
    o->head_target = repair_head_target;
    o->head_state = repair_head_state;
    o->head_destination = repair_head_destination;
    o->terrain_at = repair_terrain;
    o->fits = repair_fits;
    o->move_to = unit_move_to;
}

/* ---- heal action (0x69) bindings ------------------------------------ */

static int heal_start_move(void *c, AoeMoveActionRor *a, AoeLiveUnit *t,
                           float range, const void *graphic,
                           int quiet_first) {
    return heap_start_move(c, a, t, range, graphic, quiet_first);
}
static uint8_t heal_sub_update(void *c, AoeMoveActionRor *a) {
    return heap_sub_update(c, a);
}
static void heal_sub_clear(void *c, AoeMoveActionRor *a) {
    heap_sub_clear(c, a);
}
static uint8_t heal_work(void *c, AoeLiveUnit *t, float amount) {
    AoeWorldCombatRor *w = c;
    int r = aoe_live_unit_repair_ror(t, amount, repair_pay, c);
    if (r < 0) {
        ++w->heal_failures;
        return 0;
    }
    if (r == 1) ++w->heals;
    return (uint8_t)r;
}
static float heal_rate(void *c, AoeLiveUnit *u) {
    AoeScenarioRuntimePlayerRor *p = owner_of(c, u);
    return p && p->resources.values && p->resources.count > 56
               ? p->resources.values[56]
               : 0.0f;
}
static void heal_ops(AoeWorldCombatRor *c, AoeHealActionOps *o) {
    memset(o, 0, sizeof(*o));
    move_ops(c, &o->move);
    o->graphic = graphic_id_cb;
    o->tick_seconds = tick_seconds;
    o->distance = repair_distance;
    o->face = face_target;
    o->allowed = repair_allowed;
    o->heal = heal_work;
    o->heal_rate = heal_rate;
    o->play_sound = play_sound_cb;
    o->start_move = heal_start_move;
    o->sub_update = heal_sub_update;
    o->sub_clear = heal_sub_clear;
}

/* ---- change of owner (unit +0x44) ----------------------------------- */

static AoeScenarioRuntimePlayerRor *player_at(AoeWorldCombatRor *c,
                                              int16_t o) {
    return o >= 0 && o < AOE_SCENARIO_RUNTIME_PLAYERS
               ? &c->motion->runtime->players[o]
               : NULL;
}
static const DatUnit *co_template(void *c, int16_t owner, int16_t id) {
    AoeScenarioRuntimePlayerRor *p = player_at(c, owner);
    return p && id >= 0 ? aoe_player_templates_get(&p->templates,
                                                   (uint16_t)id)
                        : NULL;
}
/* Old owner +0xe0, new owner +0xdc (0x0045f860 / 0x0045f810). */
static int co_move_lists(void *c, AoeLiveUnit *u, int16_t from,
                         int16_t to) {
    AoeScenarioRuntimeRor *r = ((AoeWorldCombatRor *)c)->motion->runtime;
    AoeScenarioRuntimePlayerRor *a = player_at(c, from), *b = player_at(c, to);
    AoeObjectUpdateNode *node = NULL;
    AoeVisibilityUnitRef *replaced = NULL;
    int removed = 0, reassign = 0;
    if (!a || !b) return 0;
    if (u->owner_node &&
        !aoe_player_unit_remove(&a->owner, &r->world, &u->core.position,
                                u->core.unit_id, u->select_2c, u->select_30,
                                u->owner_node, &removed))
        return 0;
    u->owner_node = NULL;
    if (!aoe_player_unit_register(&b->owner, &r->world, &u->core.position,
                                  u->core.unit_id, u->select_2c,
                                  u->select_30, &node, &replaced,
                                  &reassign) ||
        replaced || reassign || !node)
        return 0;
    u->owner_node = node;
    return 1;
}
static void co_vision(void *c, AoeLiveUnit *u, int16_t owner, int add,
                      int foundation) {
    AoeScenarioRuntimePlayerRor *p = player_at(c, owner);
    if (p && u->placement.tile)
        (void)aoe_live_unit_vision_change_ror(&u->core, &p->vision_binding,
                                              add, foundation);
}
static void co_resource(void *c, int16_t owner, int16_t type, float amount) {
    AoeScenarioRuntimePlayerRor *p = player_at(c, owner);
    AoePlayerResourceTargetRor target;
    if (!p) return;
    target.resources = &p->resources;
    target.derived = 1;
    target.field_104 = &p->field_104;
    target.context = c;
    target.refresh_world = refresh_world;
    (void)aoe_player_resource_add_virtual_ror(&target, type, amount, 0);
}
static const void *co_graphic(void *c, int16_t id) {
    return graphic_by_id(c, id);
}
static void co_select(void *c, AoeLiveUnit *u, const void *g) {
    select_graphic(c, u, g);
}
static void co_notice(void *c, AoeLiveUnit *u, int16_t owner) {
    (void)u; (void)owner;
    ++((AoeWorldCombatRor *)c)->conversions;
}
static void co_queue_cancel(void *c, AoeLiveUnit *u, int16_t item,
                            int16_t count) {
    AoeWorldCombatRor *w = c;
    BuildingTick t = {w, u, 0};
    AoeBuildingQueueCancelOps ops = {&t, bt_refund, bt_cancel, bt_notify};
    uint32_t *n = &w->queue_status_four[u->runtime_owner_index];
    if (!u->building_update || !bind_building(w, u)) return;
    building_to_view(u, u->building_update);
    (void)aoe_building_queue_cancel_ror(u->building_update, item, count, n,
                                        &ops);
}
static void co_clear_actions(void *c, AoeLiveUnit *u) {
    (void)c;
    if (u->derived_action_list) (void)aoe_action_list_clear(u->derived_action_list);
}
static void co_create(void *c, int16_t owner, int16_t id, float x, float y,
                      float z) {
    AoeLiveUnit *made = NULL;
    if (owner >= 0 && id >= 0)
        (void)aoe_scenario_runtime_editor_create_ror(
            ((AoeWorldCombatRor *)c)->motion->runtime, (uint8_t)owner,
            (uint16_t)id, x, y, z, &made);
}
static void co_research(void *c, int16_t owner, int16_t tech) {
    AoeScenarioRuntimePlayerRor *p = player_at(c, owner);
    if (p && p->research.entries)
        (void)aoe_research_complete_ror(&p->research, tech);
}
static uint32_t *co_status_four(void *c, int16_t owner) {
    return owner >= 0 && owner < AOE_SCENARIO_RUNTIME_PLAYERS
               ? &((AoeWorldCombatRor *)c)->queue_status_four[owner]
               : NULL;
}
static void co_stop(void *c, AoeLiveUnit *u) { co_clear_actions(c, u); }
static void co_group(void *c, AoeLiveUnit *u) { (void)c; (void)u; }
int aoe_world_combat_change_owner_ror(AoeWorldCombatRor *c, AoeLiveUnit *u,
                                      int16_t owner) {
    AoeLiveUnitOwnerOps o = {c,           co_template,   co_move_lists,
                             co_vision,   co_resource,   co_graphic,
                             co_select,   co_notice,     co_queue_cancel,
                             co_clear_actions, co_create, co_research,
                             co_status_four,   co_stop,   co_group};
    if (!c || !u || !player_at(c, owner)) return 0;
    return aoe_live_unit_change_owner_ror(u, owner, &o);
}

/* ---- convert action (0x68) bindings --------------------------------- */

static float cv_resource(void *c, AoeLiveUnit *u, int16_t type) {
    AoeScenarioRuntimePlayerRor *p = owner_of(c, u);
    return p && p->resources.values && type >= 0 && type < p->resources.count
               ? p->resources.values[type]
               : 0.0f;
}
static void cv_event(void *c, uint32_t e, int32_t a, int32_t b, int32_t x,
                     int32_t y) {
    (void)e; (void)a; (void)b; (void)x; (void)y;
    ++((AoeWorldCombatRor *)c)->production_events;
}
static uint16_t cv_rand(void *c) {
    return aoe_rng_next(&((AoeWorldCombatRor *)c)->motion->runtime->rng);
}
static uint8_t cv_civ(void *c, AoeLiveUnit *u) {
    AoeScenarioRuntimePlayerRor *p = owner_of(c, u);
    return p ? (uint8_t)p->civilization_id : 0;
}
static int cv_change_owner(void *c, AoeLiveUnit *t, int16_t owner) {
    return aoe_world_combat_change_owner_ror(c, t, owner);
}
static void cv_resource_add(void *c, AoeLiveUnit *u, int16_t type,
                            float amount) {
    co_resource(c, u->runtime_owner_index, type, amount);
}
/* +0x80 = 0x004aacc0. */
static void cv_set_carried(void *c, AoeLiveUnit *u, int16_t type,
                           float amount) {
    (void)c;
    u->core.storage.carried_resource_type = type;
    u->core.storage.carried_amount = amount;
}
/* Target owner +0x94 = 0x004f0b50 (0x0045db00): create template 112. */
static void cv_flare(void *c, AoeLiveUnit *t, float x, float y, float z) {
    co_create(c, t->runtime_owner_index, 0x70, x, y, z);
}
static void cv_target_notice(void *c, AoeLiveUnit *t, AoeLiveUnit *u) {
    (void)c; (void)t; (void)u;
}
static void convert_ops(AoeWorldCombatRor *c, AoeConvertActionOps *o) {
    memset(o, 0, sizeof(*o));
    move_ops(c, &o->move);
    o->graphic = graphic_id_cb;
    o->tick_seconds = tick_seconds;
    o->face = face_target;
    o->allied = repair_allowed;
    o->owner_kind = owner_kind_cb;
    o->resource = cv_resource;
    o->event = cv_event;
    o->rand = cv_rand;
    o->civ = cv_civ;
    o->change_owner = cv_change_owner;
    o->resource_add = cv_resource_add;
    o->set_carried = cv_set_carried;
    o->flare = cv_flare;
    o->target_notice = cv_target_notice;
    o->play_sound = play_sound_cb;
    o->start_move = heal_start_move;
    o->sub_update = heal_sub_update;
    o->sub_clear = heal_sub_clear;
}

/* ---- trade action (0x6f) bindings ----------------------------------- */

static void tr_store(void *c, AoeLiveUnit *u, int16_t type, float value) {
    AoeScenarioRuntimePlayerRor *p = owner_of(c, u);
    if (p && p->resources.values && type >= 0 && type < p->resources.count)
        p->resources.values[type] = value;
}
static void tr_add(void *c, AoeLiveUnit *u, int16_t type, float amount) {
    AoeScenarioRuntimePlayerRor *p = owner_of(c, u);
    AoePlayerResourceTargetRor target;
    if (!p) return;
    target.resources = &p->resources;
    target.derived = 1;
    target.field_104 = &p->field_104;
    target.context = c;
    target.refresh_world = refresh_world;
    (void)aoe_player_resource_add_virtual_ror(&target, type, amount, 1);
    if (type == 3 && amount > 0.0f) ++((AoeWorldCombatRor *)c)->trade_payouts;
}
static void trade_ops(AoeWorldCombatRor *c, AoeTradeActionOps *o) {
    memset(o, 0, sizeof(*o));
    move_ops(c, &o->move);
    o->graphic = graphic_id_cb;
    o->tick_seconds = tick_seconds;
    o->face = face_target;
    o->resource = cv_resource;
    o->resource_store = tr_store;
    o->resource_add = tr_add;
    o->civ = cv_civ;
    o->find_home = find_drop_site;
    o->play_sound = play_sound_cb;
    o->start_move = heal_start_move;
    o->sub_update = heal_sub_update;
    o->sub_clear = heal_sub_clear;
}

/* ---- artifact / discovery / wonder bindings ------------------------- */

static int16_t ob_players(void *c) {
    return ((AoeWorldCombatRor *)c)->motion->runtime->player_count;
}
static int32_t ob_width(void *c) {
    return (int32_t)((AoeWorldCombatRor *)c)->motion->runtime->map.width;
}
static int32_t ob_height(void *c) {
    return (int32_t)((AoeWorldCombatRor *)c)->motion->runtime->map.height;
}
static AoeLiveUnit *ob_tile_object(void *c, int32_t x, int32_t y,
                                   uint32_t i) {
    const AoeRuntimeMap *m = &((AoeWorldCombatRor *)c)->motion->runtime->map;
    const AoeObjectUpdateNode *n;
    if (x < 0 || y < 0 || (uint32_t)x >= m->width || (uint32_t)y >= m->height)
        return NULL;
    n = m->objects ? m->objects[(size_t)y * m->width + (size_t)x].head
                   : NULL;
    for (; n; n = n->next)
        if (n->object && i-- == 0) return (AoeLiveUnit *)n->object;
    return NULL;
}
/* Artifact walks: 0x00405250 at the point, range 0, pushed as is. */
static int ob_start_move(void *c, AoeMoveActionRor *base, const void *g) {
    AoeWorldCombatRor *w = c;
    AoeActionList *list = sub_list_of(w, base);
    AoeMoveActionRor *mv;
    const DatUnit *tt = base->unit->core.unit_template;
    if (!list || !(mv = calloc(1, sizeof(*mv)))) return 0;
    if (!aoe_move_action_init_point_ror(mv, base->unit, base->x, base->y,
                                        base->z, 0.0f, g,
                                        graphic_by_id(w, tt->walking_graphic)) ||
        !aoe_action_list_push(list, mv)) {
        free(mv);
        return 0;
    }
    return 1;
}
static int ob_change_owner(void *c, AoeLiveUnit *u, int16_t owner) {
    return aoe_world_combat_change_owner_ror(c, u, owner);
}
static void ob_event(void *c, uint32_t e, int16_t o, int32_t x, int32_t y) {
    (void)e; (void)o; (void)x; (void)y;
    ++((AoeWorldCombatRor *)c)->production_events;
}
/* Game 0x0041c0d0 / 0x0041c0b0 on the settings' +0x9d8 countdowns. */
static void ob_countdown_set(void *c, int16_t owner, int32_t years) {
    AoeWorldCombatRor *w = c;
    if (owner >= 0 && owner < AOE_SCENARIO_RUNTIME_PLAYERS) {
        aoe_settings_set_wonder_ror(&w->settings, owner, years);
        w->wonder_countdown[owner] = w->settings.wonder_years[owner];
    }
}
static void ob_countdown_clear(void *c, int16_t owner) {
    AoeWorldCombatRor *w = c;
    if (owner >= 0 && owner < AOE_SCENARIO_RUNTIME_PLAYERS) {
        aoe_settings_clear_wonder_ror(&w->settings, owner);
        w->wonder_countdown[owner] = -1;
    }
}
static void ob_no_event(void *user, int code, int16_t player) {
    (void)user; (void)code; (void)player;
}
/* Player 0x0045d460: the owner wins (0x0045d420, state 1 unless its
   state is locked by an earlier outcome); every other player wins too when
   both have team victory (+0x108) and are mutual allies (stance 0, the
   scenario diplomacy here), else loses (0x0045d440, state 2). */
static void ob_win(void *c, int16_t owner) {
    AoeWorldCombatRor *w = c;
    AoeScenarioRuntimeRor *r = w->motion->runtime;
    AoePlayerUpdateOps po;
    memset(&po, 0, sizeof(po));
    po.user = c;
    po.emit_state_event = ob_no_event;
    if (owner < 0 || owner >= AOE_SCENARIO_RUNTIME_PLAYERS) return;
    if (!w->outcome_player[owner].state80)
        (void)aoe_player_state_set(&w->outcome_player[owner], 1, &po);
    for (int16_t j = 1; j < r->player_count && j < AOE_SCENARIO_RUNTIME_PLAYERS;
         ++j) {
        int team;
        if (j == owner) continue;
        team = w->outcome_player[owner].team_mode &&
               r->diplomacy[owner][j] == 0 && r->diplomacy[j][owner] == 0 &&
               w->outcome_player[j].team_mode;
        if (!w->outcome_player[j].state80)
            (void)aoe_player_state_set(&w->outcome_player[j], team ? 1 : 2,
                                       &po);
    }
    ++w->wonder_victories;
}
static void object_ops(AoeWorldCombatRor *c, AoeObjectActionOps *o) {
    memset(o, 0, sizeof(*o));
    move_ops(c, &o->move);
    o->tick_seconds = tick_seconds;
    o->player_count = ob_players;
    o->map_width = ob_width;
    o->map_height = ob_height;
    o->tile_object = ob_tile_object;
    o->start_move = ob_start_move;
    o->sub_update = heal_sub_update;
    o->sub_clear = heal_sub_clear;
    o->graphic = graphic_id_cb;
    o->change_owner = ob_change_owner;
    o->create = co_create;
    o->vision = co_vision;
    o->event = ob_event;
    o->countdown_set = ob_countdown_set;
    o->countdown_clear = ob_countdown_clear;
    o->win = ob_win;
    o->countdown_disabled = c->wonder_countdown_hidden;
    o->tearing_down = c->tearing_down;
    o->game_over = &c->game_over;
}

/* ---- garrison, enter / explore / fly / transport bindings ----------- */

static void ga_vision_off(void *c, AoeLiveUnit *u) {
    co_vision(c, u, u->runtime_owner_index, 0, 0);
}
static void ga_leave_tile(void *c, AoeLiveUnit *u) {
    (void)c;
    (void)aoe_object_update_list_detach(u->placement.tile, &u->core,
                                        u->placement.node);
    u->placement.tile = NULL;
    u->placement.node = NULL;
}
static void ga_lift(void *c, AoeLiveUnit *u) {
    aoe_world_motion_lift_ror(((AoeWorldCombatRor *)c)->motion, u);
}
static void ga_record(void *c, AoeLiveUnit *u) {
    (void)u;
    ++((AoeWorldCombatRor *)c)->garrison_records;
}
static int ga_zone_at(void *c, AoeLiveUnit *u, int32_t x, int32_t y) {
    return aoe_world_motion_zone_ror(((AoeWorldCombatRor *)c)->motion, u, x,
                                     y);
}
static int ga_placement(void *c, AoeLiveUnit *u, float x, float y) {
    return placement_refused(c, u->core.unit_template,
                             u->runtime_owner_index, x, y, 1);
}
static void ga_place(void *c, AoeLiveUnit *u, float x, float y, float z) {
    aoe_world_motion_place_ror(((AoeWorldCombatRor *)c)->motion, u, x, y, z);
}
static void garrison_ops(AoeWorldCombatRor *c, AoeGarrisonOps *o) {
    memset(o, 0, sizeof(*o));
    o->context = c;
    o->vision_off = ga_vision_off;
    o->leave_tile = ga_leave_tile;
    o->lift = ga_lift;
    o->record = ga_record;
    o->rand = cv_rand;
    o->map_width = ob_width;
    o->map_height = ob_height;
    o->zone_at = ga_zone_at;
    o->placement = ga_placement;
    o->fits = repair_fits;
    o->place = ga_place;
}
/* Unit +0xd0 (root 0x004aa940, types 50+ 0x00426b80). */
int aoe_world_combat_enter_ror(AoeWorldCombatRor *c, AoeLiveUnit *u,
                               AoeLiveUnit *container) {
    AoeGarrisonOps o;
    if (!c || !u || !container || !u->core.unit_template) return -1;
    garrison_ops(c, &o);
    return aoe_unit_enter_ror(u, container,
                              u->core.unit_template->type >= 50, &o);
}
/* 0x004ab280. */
int aoe_world_combat_unload_ror(AoeWorldCombatRor *c, AoeLiveUnit *t,
                                int zone, uint8_t *count) {
    AoeGarrisonOps o;
    if (!c) return 0;
    garrison_ops(c, &o);
    return aoe_unit_unload_ror(t, zone, &o, count);
}

static int ba_start_move(void *c, AoeMoveActionRor *base, float range,
                         const void *graphic, int quiet) {
    AoeWorldCombatRor *w = c;
    AoeActionList *list = sub_list_of(w, base);
    AoeMoveActionRor *mv;
    const DatUnit *tt = base->unit->core.unit_template;
    if (!list || !(mv = calloc(1, sizeof(*mv)))) return 0;
    if (!aoe_move_action_init_point_ror(mv, base->unit, base->x, base->y,
                                        base->z, range, graphic,
                                        graphic_by_id(w, tt->walking_graphic))) {
        free(mv);
        return 0;
    }
    if (quiet == 1) mv->quiet = 1;
    if (!aoe_action_list_push(list, mv)) {
        free(mv);
        return 0;
    }
    if (quiet == 0) mv->quiet = 1;
    return 1;
}
static void ba_place(void *c, AoeLiveUnit *u, float x, float y, float z) {
    aoe_world_motion_place_ror(((AoeWorldCombatRor *)c)->motion, u, x, y, z);
}
static double ba_dist_point(void *c, AoeLiveUnit *u, float x, float y,
                            float z) {
    (void)c; (void)z;
    return aoe_combat_point_distance_ror(u, x, y);
}
static int32_t ba_head_secondary(void *c, AoeLiveUnit *u) {
    void *h = head_of(u);
    (void)c;
    if (!h || entry_type(h) == AOE_WORLD_COMBAT_RECORD_TAG) return -1;
    if (entry_type(h) == 0x0c) return ((AoeBaseActionRor *)h)->unload_flag;
    return ((AoeMoveActionRor *)h)->secondary_id;
}
static int ba_enter(void *c, AoeLiveUnit *u, AoeLiveUnit *t) {
    return aoe_world_combat_enter_ror(c, u, t) == 1;
}
static int ba_zone_near(void *c, AoeLiveUnit *u, int zone, float d) {
    return aoe_world_motion_zone_near_ror(((AoeWorldCombatRor *)c)->motion,
                                          u, zone, d);
}
static uint8_t ba_unload(void *c, AoeLiveUnit *u, int zone) {
    uint8_t n = 0;
    (void)aoe_world_combat_unload_ror(c, u, zone, &n);
    return n;
}
static uint16_t ba_contents(void *c, AoeLiveUnit *u) {
    (void)c;
    return u->contents.count;
}
static void ba_owner_notice(void *c, AoeLiveUnit *u, int32_t code) {
    (void)u; (void)code;
    ++((AoeWorldCombatRor *)c)->notices;
}
static void base_ops(AoeWorldCombatRor *c, AoeBaseActionOps *o) {
    memset(o, 0, sizeof(*o));
    move_ops(c, &o->move);
    o->graphic = graphic_id_cb;
    o->tick_seconds = tick_seconds;
    o->rand = cv_rand;
    o->map_width = ob_width;
    o->map_height = ob_height;
    o->start_move = ba_start_move;
    o->sub_update = heal_sub_update;
    o->sub_clear = heal_sub_clear;
    o->place = ba_place;
    o->distance = repair_distance;
    o->distance_to_point = ba_dist_point;
    o->allied = repair_allowed;
    o->owner_kind = owner_kind_cb;
    o->head_state = repair_head_state;
    o->head_secondary = ba_head_secondary;
    o->head_destination = repair_head_destination;
    o->move_to = unit_move_to;
    o->enter = ba_enter;
    o->line = (AoeLineSearchOps){c, repair_terrain, repair_fits};
    o->zone_at = ga_zone_at;
    o->zone_near = ba_zone_near;
    o->terrain_at = repair_terrain;
    o->unload = ba_unload;
    o->contents = ba_contents;
    o->owner_notice = ba_owner_notice;
}

/* ---- heap action classes -------------------------------------------- */

static void move_activate(AoeWorldCombatRor *c, void *a, uint8_t st) {
    sub_activate(c, a, st);
}
static uint8_t move_update(AoeWorldCombatRor *c, void *a) {
    AoeMoveActionOps o;
    uint8_t r = 0;
    move_ops(c, &o);
    return aoe_move_action_update_ror(a, &o, &r) ? r : 0;
}
static AoeLiveUnit *base_target(void *a) {
    return ((AoeMoveActionRor *)a)->target;
}
static void farm_activate(AoeWorldCombatRor *c, void *a, uint8_t st) {
    AoeFarmActionOps o;
    (void)st;
    farm_ops(c, &o);
    (void)aoe_farm_action_activate_ror(a, &o);
}
static uint8_t farm_update(AoeWorldCombatRor *c, void *a) {
    AoeFarmActionOps o;
    uint8_t r = 0;
    farm_ops(c, &o);
    return aoe_farm_action_update_ror(a, &o, &r) ? r : 0;
}
static const DatUnitTask *farm_task(void *a) {
    return ((AoeFarmActionRor *)a)->task;
}
static void train_destroy(AoeWorldCombatRor *c, void *a) {
    aoe_train_action_adapter_destroy(&c->train, a);
}
static void train_activate(AoeWorldCombatRor *c, void *a, uint8_t st) {
    aoe_train_action_adapter_state(&c->train, a, st);
}
static uint8_t train_update(AoeWorldCombatRor *c, void *a) {
    return aoe_train_action_adapter_head(&c->train, a);
}
static void research_destroy(AoeWorldCombatRor *c, void *action) {
    AoeResearchAction *a = action;
    AoeResearchActionOps o;
    if (!research_ops(c, a->owner_index, &o) ||
        !aoe_research_action_destroy_ror(a, &o))
        ++c->research_failures;
    free(a);
}
static void research_activate(AoeWorldCombatRor *c, void *action,
                              uint8_t st) {
    AoeResearchAction *a = action;
    AoeResearchActionOps o;
    if (!research_ops(c, a->owner_index, &o) ||
        !aoe_research_action_activate_ror(a, st, &o))
        ++c->research_failures;
}
static uint8_t research_update(AoeWorldCombatRor *c, void *action) {
    AoeResearchAction *a = action;
    AoeResearchActionOps o;
    uint8_t r = 0;
    if (!research_ops(c, a->owner_index, &o) ||
        !aoe_research_action_update_ror(a, &o, &r)) {
        ++c->research_failures;
        return 0;
    }
    return r;
}
static void repair_activate(AoeWorldCombatRor *c, void *a, uint8_t st) {
    AoeRepairActionOps o;
    repair_ops(c, &o);
    if (!aoe_repair_action_activate_ror(a, st, &o)) ++c->repair_failures;
}
static uint8_t repair_update(AoeWorldCombatRor *c, void *a) {
    AoeRepairActionOps o;
    uint8_t r = 0;
    repair_ops(c, &o);
    if (!aoe_repair_action_update_ror(a, &o, &r)) {
        ++c->repair_failures;
        return 0;
    }
    return r;
}
static const DatUnitTask *repair_task(void *a) {
    return ((AoeRepairActionRor *)a)->task;
}
static int repair_retarget(AoeWorldCombatRor *c, void *a, AoeLiveUnit *t,
                           float x, float y, float z) {
    AoeRepairActionOps o;
    repair_ops(c, &o);
    return aoe_repair_action_retarget_ror(a, t, x, y, z, &o);
}

static void heal_activate(AoeWorldCombatRor *c, void *a, uint8_t st) {
    AoeHealActionOps o;
    (void)st;
    heal_ops(c, &o);
    if (!aoe_heal_action_activate_ror(a, &o)) ++c->heal_failures;
}
static uint8_t heal_update(AoeWorldCombatRor *c, void *a) {
    AoeHealActionOps o;
    uint8_t r = 0;
    heal_ops(c, &o);
    if (!aoe_heal_action_update_ror(a, &o, &r)) {
        ++c->heal_failures;
        return 0;
    }
    return r;
}
static const DatUnitTask *heal_task(void *a) {
    return ((AoeHealActionRor *)a)->task;
}
static int heal_retarget(AoeWorldCombatRor *c, void *a, AoeLiveUnit *t,
                         float x, float y, float z) {
    AoeHealActionOps o;
    int accepted = 0;
    (void)x; (void)y; (void)z;
    heal_ops(c, &o);
    return aoe_heal_action_retarget_ror(a, t, &o, &accepted) && accepted;
}

static void convert_activate(AoeWorldCombatRor *c, void *a, uint8_t st) {
    AoeConvertActionOps o;
    convert_ops(c, &o);
    if (!aoe_convert_action_activate_ror(a, st, &o)) ++c->convert_failures;
}
static uint8_t convert_update(AoeWorldCombatRor *c, void *a) {
    AoeConvertActionOps o;
    uint8_t r = 0;
    convert_ops(c, &o);
    if (!aoe_convert_action_update_ror(a, &o, &r)) {
        ++c->convert_failures;
        return 0;
    }
    return r;
}
static const DatUnitTask *convert_task(void *a) {
    return ((AoeConvertActionRor *)a)->task;
}
static int convert_retarget(AoeWorldCombatRor *c, void *a, AoeLiveUnit *t,
                            float x, float y, float z) {
    AoeConvertActionOps o;
    int accepted = 0;
    (void)x; (void)y; (void)z;
    convert_ops(c, &o);
    return aoe_convert_action_retarget_ror(a, t, &o, &accepted) && accepted;
}

/* Hunt (0x6e): activation 0x004b33a0 is shared with gathering. */
static void hunt_activate(AoeWorldCombatRor *c, void *a, uint8_t st) {
    AoeGatherActionOps o;
    gather_ops(c, &o);
    if (!aoe_gather_action_activate_ror(a, st, &o)) ++c->hunt_failures;
}
static uint8_t hunt_update(AoeWorldCombatRor *c, void *a) {
    AoeGatherActionOps o;
    uint8_t r = 0;
    gather_ops(c, &o);
    if (!aoe_hunt_action_update_ror(a, &o, &r)) {
        ++c->hunt_failures;
        return 0;
    }
    return r;
}
static const DatUnitTask *hunt_task(void *a) {
    return ((AoeGatherActionRor *)a)->task;
}
static int hunt_retarget(AoeWorldCombatRor *c, void *a, AoeLiveUnit *t,
                         float x, float y, float z) {
    AoeGatherActionOps o;
    gather_ops(c, &o);
    return aoe_hunt_action_retarget_ror(a, t, x, y, z, &o);
}

static void trade_activate(AoeWorldCombatRor *c, void *a, uint8_t st) {
    AoeTradeActionOps o;
    trade_ops(c, &o);
    if (!aoe_trade_action_activate_ror(a, st, &o)) ++c->trade_failures;
}
static uint8_t trade_update(AoeWorldCombatRor *c, void *a) {
    AoeTradeActionOps o;
    uint8_t r = 0;
    trade_ops(c, &o);
    if (!aoe_trade_action_update_ror(a, &o, &r)) {
        ++c->trade_failures;
        return 0;
    }
    return r;
}
static const DatUnitTask *trade_task(void *a) {
    return ((AoeTradeActionRor *)a)->task;
}
static int trade_retarget(AoeWorldCombatRor *c, void *a, AoeLiveUnit *t,
                          float x, float y, float z) {
    AoeTradeActionOps o;
    trade_ops(c, &o);
    return aoe_trade_action_retarget_ror(a, t, x, y, z, &o);
}

static void artifact_destroy(AoeWorldCombatRor *c, void *action) {
    AoeArtifactActionRor *a = action;
    a->base.target = NULL; /* stored raw, never attached */
    base_destroy(c, &a->base);
    free(a);
}
static void artifact_activate(AoeWorldCombatRor *c, void *a, uint8_t st) {
    AoeObjectActionOps o;
    object_ops(c, &o);
    if (!aoe_artifact_action_activate_ror(a, st, &o)) ++c->object_failures;
}
static uint8_t artifact_update(AoeWorldCombatRor *c, void *a) {
    AoeObjectActionOps o;
    uint8_t r = 0;
    object_ops(c, &o);
    if (!aoe_artifact_action_update_ror(a, &o, &r)) {
        ++c->object_failures;
        return 0;
    }
    return r;
}
static const DatUnitTask *artifact_task(void *a) {
    return ((AoeArtifactActionRor *)a)->task;
}
static int artifact_retarget(AoeWorldCombatRor *c, void *a, AoeLiveUnit *t,
                             float x, float y, float z) {
    AoeObjectActionOps o;
    object_ops(c, &o);
    return aoe_artifact_action_retarget_ror(a, t, x, y, z, &o);
}
static void discovery_destroy(AoeWorldCombatRor *c, void *action) {
    AoeDiscoveryActionRor *a = action;
    base_destroy(c, &a->base);
    aoe_discovery_action_free_ror(a);
    free(a);
}
static void discovery_activate(AoeWorldCombatRor *c, void *a, uint8_t st) {
    (void)c;
    (void)aoe_discovery_action_activate_ror(a, st);
}
static uint8_t discovery_update(AoeWorldCombatRor *c, void *a) {
    AoeObjectActionOps o;
    uint8_t r = 0;
    object_ops(c, &o);
    if (!aoe_discovery_action_update_ror(a, &o, &r)) {
        ++c->object_failures;
        return 0;
    }
    return r;
}
static const DatUnitTask *discovery_task(void *a) {
    return ((AoeDiscoveryActionRor *)a)->task;
}
/* Another standing (state >= 2) wonder, template 276, of the owner. */
static int other_wonder(AoeWorldCombatRor *c, AoeLiveUnit *u) {
    AoeScenarioRuntimePlayerRor *p = owner_of(c, u);
    const AoeObjectUpdateNode *n;
    if (!p) return 0;
    for (n = p->owner.object_list28.head; n; n = n->next) {
        AoeLiveUnit *x = (AoeLiveUnit *)((char *)n->object -
                                         offsetof(AoeLiveUnitCore, position));
        if (n->object && x != u && x->core.unit_template &&
            x->core.unit_template->id == 0x114 && x->core.state >= 2)
            return 1;
    }
    return 0;
}
static void wonder_destroy(AoeWorldCombatRor *c, void *action) {
    AoeWonderActionRor *a = action;
    AoeObjectActionOps o;
    object_ops(c, &o);
    if (!aoe_wonder_action_destroy_ror(a, &o, other_wonder(c, a->base.unit)))
        ++c->object_failures;
    base_destroy(c, &a->base);
    free(a);
}
static void wonder_activate(AoeWorldCombatRor *c, void *a, uint8_t st) {
    AoeObjectActionOps o;
    object_ops(c, &o);
    if (!aoe_wonder_action_activate_ror(a, st, &o)) ++c->object_failures;
}
static uint8_t wonder_update(AoeWorldCombatRor *c, void *a) {
    AoeObjectActionOps o;
    uint8_t r = 0;
    object_ops(c, &o);
    if (!aoe_wonder_action_update_ror(a, &o, &r)) {
        ++c->object_failures;
        return 0;
    }
    return r;
}

static void enter_activate(AoeWorldCombatRor *c, void *a, uint8_t st) {
    AoeBaseActionOps o;
    (void)st;
    base_ops(c, &o);
    if (!aoe_enter_action_activate_ror(a, &o)) ++c->object_failures;
}
static uint8_t enter_update(AoeWorldCombatRor *c, void *a) {
    AoeBaseActionOps o;
    uint8_t r = 0;
    base_ops(c, &o);
    if (!aoe_enter_action_update_ror(a, &o, &r)) {
        ++c->object_failures;
        return 0;
    }
    return r;
}
static void explore_activate(AoeWorldCombatRor *c, void *a, uint8_t st) {
    AoeBaseActionOps o;
    (void)st;
    base_ops(c, &o);
    if (!aoe_explore_action_activate_ror(a, &o)) ++c->object_failures;
}
static uint8_t explore_update(AoeWorldCombatRor *c, void *a) {
    AoeBaseActionOps o;
    uint8_t r = 0;
    base_ops(c, &o);
    if (!aoe_explore_action_update_ror(a, &o, &r)) {
        ++c->object_failures;
        return 0;
    }
    return r;
}
static int explore_retarget(AoeWorldCombatRor *c, void *a, AoeLiveUnit *t,
                            float x, float y, float z) {
    AoeBaseActionOps o;
    (void)t;
    base_ops(c, &o);
    return aoe_explore_action_retarget_ror(a, x, y, z, &o);
}
static void fly_activate(AoeWorldCombatRor *c, void *a, uint8_t st) {
    AoeBaseActionOps o;
    base_ops(c, &o);
    if (!aoe_fly_action_activate_ror(a, st, &o)) ++c->object_failures;
}
static uint8_t fly_update(AoeWorldCombatRor *c, void *a) {
    AoeBaseActionOps o;
    uint8_t r = 0;
    base_ops(c, &o);
    if (!aoe_fly_action_update_ror(a, &o, &r)) {
        ++c->object_failures;
        return 0;
    }
    return r;
}
static void transport_activate(AoeWorldCombatRor *c, void *a, uint8_t st) {
    AoeBaseActionOps o;
    base_ops(c, &o);
    if (!aoe_transport_action_activate_ror(a, st, &o)) ++c->object_failures;
}
static uint8_t transport_update(AoeWorldCombatRor *c, void *a) {
    AoeBaseActionOps o;
    uint8_t r = 0;
    base_ops(c, &o);
    if (!aoe_transport_action_update_ror(a, &o, &r)) {
        ++c->object_failures;
        return 0;
    }
    return r;
}
static int transport_retarget(AoeWorldCombatRor *c, void *a, AoeLiveUnit *t,
                              float x, float y, float z) {
    AoeBaseActionOps o;
    base_ops(c, &o);
    return aoe_transport_action_retarget_ror(a, t, x, y, z, &o);
}
static const DatUnitTask *base_task(void *a) {
    return ((AoeBaseActionRor *)a)->task;
}

static const HeapActionClass heap_classes[] = {
    {3, NULL, enter_activate, enter_update, base_task, NULL, base_target},
    {4, NULL, explore_activate, explore_update, base_task, explore_retarget,
     base_target},
    {0x0a, NULL, fly_activate, fly_update, base_task, NULL, base_target},
    {0x0c, NULL, transport_activate, transport_update, base_task,
     transport_retarget, base_target},
    {0x6b, artifact_destroy, artifact_activate, artifact_update,
     artifact_task, artifact_retarget, NULL},
    {0x6c, discovery_destroy, discovery_activate, discovery_update,
     discovery_task, NULL, base_target},
    {0x78, wonder_destroy, wonder_activate, wonder_update, NULL, NULL, NULL},
    {0x6f, NULL, trade_activate, trade_update, trade_task, trade_retarget,
     base_target},
    {0x6e, NULL, hunt_activate, hunt_update, hunt_task, hunt_retarget,
     base_target},
    {0x68, NULL, convert_activate, convert_update, convert_task,
     convert_retarget, base_target},
    {0x69, NULL, heal_activate, heal_update, heal_task, heal_retarget,
     base_target},
    {1, NULL, move_activate, move_update, NULL, NULL, base_target},
    {0x15, NULL, farm_activate, farm_update, farm_task, NULL, base_target},
    {0x66, train_destroy, train_activate, train_update, NULL, NULL, NULL},
    {0x67, research_destroy, research_activate, research_update, NULL, NULL,
     NULL},
    {0x6a, NULL, repair_activate, repair_update, repair_task,
     repair_retarget, base_target},
};
static const HeapActionClass *heap_class(int16_t type) {
    for (size_t i = 0; i < sizeof(heap_classes) / sizeof(heap_classes[0]);
         ++i)
        if (heap_classes[i].type == type) return &heap_classes[i];
    return NULL;
}
