#include "tech_tree.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"

/* ---- template attribute methods ------------------------------------ */

static int16_t w16(int32_t v) { return (int16_t)(uint16_t)(uint32_t)v; }

/* Root 0x004422d0 / 0x00442370 / 0x00442420. */
static void root_attribute(DatUnit *t, AoeAttributeOp op, uint8_t a,
                           float v) {
    switch (a) {
    case 0:
        if (op == AOE_ATTRIBUTE_SET) t->hit_points = w16(aoe_ftol32(v));
        else if (op == AOE_ATTRIBUTE_ADD)
            t->hit_points = w16(t->hit_points + aoe_ftol32(v));
        else t->hit_points = w16(aoe_ftol32((double)t->hit_points * v));
        break;
    case 1:
        if (op == AOE_ATTRIBUTE_SET) t->line_of_sight = v;
        else if (op == AOE_ATTRIBUTE_ADD)
            t->line_of_sight = (float)((double)v + t->line_of_sight);
        else t->line_of_sight = (float)((double)v * t->line_of_sight);
        break;
    case 2:
        if (op == AOE_ATTRIBUTE_SET)
            t->garrison_capacity = (uint8_t)aoe_ftol32(v);
        else if (op == AOE_ATTRIBUTE_ADD)
            t->garrison_capacity =
                (uint8_t)(t->garrison_capacity + aoe_ftol32(v));
        else t->garrison_capacity =
                 (uint8_t)aoe_ftol32((double)t->garrison_capacity * v);
        break;
    case 3: case 4: {
        float *f = &t->size[a - 3];
        if (op == AOE_ATTRIBUTE_SET) *f = v;
        else if (op == AOE_ATTRIBUTE_ADD) *f = (float)((double)v + *f);
        else *f = (float)((double)v * *f);
        break;
    }
    case 14:
        if (op == AOE_ATTRIBUTE_SET)
            t->resource_capacity = w16(aoe_ftol32(v));
        else if (op == AOE_ATTRIBUTE_ADD)
            t->resource_capacity = w16(t->resource_capacity + aoe_ftol32(v));
        else t->resource_capacity =
                 w16(aoe_ftol32((double)t->resource_capacity * v));
        break;
    default:
        break;
    }
}

static void float_op(float *f, AoeAttributeOp op, float v) {
    if (op == AOE_ATTRIBUTE_SET) *f = v;
    else if (op == AOE_ATTRIBUTE_ADD) *f = (float)((double)v + *f);
    else *f = (float)((double)v * *f);
}

/* Type 20 (0x0043ecd0 family): 5 speed. */
static void t20_attribute(DatUnit *t, AoeAttributeOp op, uint8_t a,
                          float v) {
    if (a == 5) float_op(&t->speed, op, v);
    else root_attribute(t, op, a, v);
}
/* Type 30 (0x00440e90 family): 6 rotation speed. */
static void t30_attribute(DatUnit *t, AoeAttributeOp op, uint8_t a,
                          float v) {
    if (a == 6) float_op(&t->rotation_speed, op, v);
    else t20_attribute(t, op, a, v);
}
/* Type 40 (0x0043e780 family): 13 work rate. */
static void t40_attribute(DatUnit *t, AoeAttributeOp op, uint8_t a,
                          float v) {
    if (a == 13) float_op(&t->work_rate, op, v);
    else t30_attribute(t, op, a, v);
}

/* Type 50 attack/armor lists. */
static void list_set(DatUnitAttackArmor *list, uint16_t count, int32_t v) {
    unsigned index = ((uint32_t)v >> 8) & 0xff;
    if (!list || index >= count) return;
    list[index].amount = (int16_t)(v & 0xff);
}
static void list_add(DatUnitAttackArmor *list, uint16_t count, int32_t v) {
    int16_t cls = (int16_t)(((uint32_t)v >> 8) & 0xff);
    if (!list) return;
    for (uint16_t i = 0; i < count; ++i)
        if (list[i].class_id == cls) {
            list[i].amount = w16(list[i].amount + (v & 0xff));
            return;
        }
}

/* Type 50: 0x0043f920 / 0x0043fa60 / 0x0043fc40. */
static void t50_attribute(DatUnit *t, AoeAttributeOp op, uint8_t a,
                          float v) {
    switch (op) {
    case AOE_ATTRIBUTE_SET:
        switch (a) {
        case 8: list_set(t->armors, t->armor_count, aoe_ftol32(v)); return;
        case 9: list_set(t->attacks, t->attack_count, aoe_ftol32(v)); return;
        case 10: t->reload_time = v; return;
        case 11: t->accuracy_percent = w16(aoe_ftol32(v)); return;
        case 12: t->max_range = v; return;
        case 15: t->base_armor = (uint8_t)aoe_ftol32(v); return;
        case 16:
            t->projectile_unit_id = (int16_t)(uint8_t)aoe_ftol32(v);
            return;
        case 18: t->terrain_defense_bonus = w16(aoe_ftol32(v)); return;
        default: break;
        }
        break;
    case AOE_ATTRIBUTE_ADD:
        switch (a) {
        case 6: float_op(&t->rotation_speed, op, v); return;
        case 8: list_add(t->armors, t->armor_count, aoe_ftol32(v)); return;
        case 9: list_add(t->attacks, t->attack_count, aoe_ftol32(v)); return;
        case 10: float_op(&t->reload_time, op, v); return;
        case 11:
            t->accuracy_percent = w16(t->accuracy_percent + aoe_ftol32(v));
            return;
        case 12: float_op(&t->max_range, op, v); return;
        case 15:
            t->base_armor = (uint8_t)(t->base_armor + aoe_ftol32(v));
            return;
        case 16:
            t->projectile_unit_id =
                w16(t->projectile_unit_id + aoe_ftol32(v));
            return;
        case 18: t->terrain_defense_bonus = w16(aoe_ftol32(v)); return;
        default: break;
        }
        break;
    case AOE_ATTRIBUTE_MUL:
        switch (a) {
        case 6: float_op(&t->rotation_speed, op, v); return;
        case 8: case 9: return;
        case 10: float_op(&t->reload_time, op, v); return;
        case 11:
            t->accuracy_percent =
                w16(aoe_ftol32((double)t->accuracy_percent * v));
            return;
        case 12: float_op(&t->max_range, op, v); return;
        case 15: /* reads the projectile word (+0x124) */
            t->base_armor =
                (uint8_t)aoe_ftol32((double)t->projectile_unit_id * v);
            return;
        case 16:
            t->projectile_unit_id =
                w16(aoe_ftol32((double)t->projectile_unit_id * v));
            return;
        case 18: t->terrain_defense_bonus = w16(aoe_ftol32(v)); return;
        default: break;
        }
        break;
    }
    t40_attribute(t, op, a, v);
}

/* Type 60: 0x00440810 family. */
static void t60_attribute(DatUnit *t, AoeAttributeOp op, uint8_t a,
                          float v) {
    if (a == 19) t->projectile_lead = (uint8_t)aoe_ftol32(v);
    t50_attribute(t, op, a, v);
}

/* Type 70 (tribe): 0x004ecf20 / 0x004ed010 / 0x004ed060. */
static void t70_attribute(DatUnit *t, AoeAttributeOp op, uint8_t a,
                          float v) {
    if (a == 100) {
        for (unsigned i = 0; i < 3; ++i) {
            DatUnitCost *c = &t->costs[i];
            if (!(uint8_t)c->paid) continue;
            if (op == AOE_ATTRIBUTE_SET) c->amount = w16(aoe_ftol32(v));
            else if (op == AOE_ATTRIBUTE_ADD)
                c->amount = w16(c->amount + aoe_ftol32(v));
            else c->amount = w16(aoe_ftol32((double)c->amount * v));
        }
        return;
    }
    if (a == 101 && op == AOE_ATTRIBUTE_SET) {
        for (unsigned i = 0; i < 3; ++i)
            if (!(uint8_t)t->costs[i].paid)
                t->costs[i].amount = w16(-aoe_ftol32(v));
        for (unsigned i = 0; i < 3; ++i)
            if (t->storage[i].resource_type == 4) t->storage[i].amount = v;
        for (unsigned i = 0; i < 3; ++i)
            if (t->storage[i].resource_type == 11) t->storage[i].amount = -v;
        for (unsigned i = 0; i < 3; ++i)
            if (t->storage[i].resource_type == 19) t->storage[i].amount = -v;
        return;
    }
    t50_attribute(t, op, a, v);
}

/* Type 80 (tribe): 0x004ec580 / 0x004ec5b0 / 0x004ec5e0. */
static void t80_attribute(DatUnit *t, AoeAttributeOp op, uint8_t a,
                          float v) {
    if (a == 17 && op == AOE_ATTRIBUTE_SET) {
        t->building_angle = (uint16_t)aoe_ftol32(v);
        return;
    }
    if (a == 17 && op == AOE_ATTRIBUTE_ADD) {
        t->building_angle = (uint16_t)(t->building_angle + aoe_ftol32(v));
        return;
    }
    t70_attribute(t, op, a, v);
}

int aoe_template_attribute_ror(DatUnit *t, AoeAttributeOp op,
                               uint8_t attribute, float value) {
    if (!t || op > AOE_ATTRIBUTE_MUL) return 0;
    switch (t->type) {
    case 10: case 15: case 90: root_attribute(t, op, attribute, value); break;
    case 20: case 25: t20_attribute(t, op, attribute, value); break;
    case 30: t30_attribute(t, op, attribute, value); break;
    case 40: t40_attribute(t, op, attribute, value); break;
    case 50: t50_attribute(t, op, attribute, value); break;
    case 60: t60_attribute(t, op, attribute, value); break;
    case 70: t70_attribute(t, op, attribute, value); break;
    case 80: t80_attribute(t, op, attribute, value); break;
    default: return 0;
    }
    return 1;
}

/* ---- upgrade copy --------------------------------------------------- */

static int class_level(uint8_t type) {
    switch (type) {
    case 20: case 25: return 2;
    case 30: return 3;
    case 40: return 4;
    case 50: return 5;
    case 60: return 6;
    case 70: return 7;
    case 80: return 8;
    default: return 1;
    }
}

static void *dup_array(const void *p, size_t n, size_t size) {
    void *q;
    if (!n) return NULL;
    q = malloc(n * size);
    if (q) memcpy(q, p, n * size);
    return q;
}

int aoe_template_upgrade_copy_ror(DatUnit *slot, DatUnit *src) {
    DatUnit keep;
    char *name = NULL;
    DatDamageGraphic *damage = NULL;
    DatUnitAttackArmor *attacks = NULL, *armors = NULL;
    int level;
    if (!slot || !src) return 0;
    if (slot == src) return 1;
    level = class_level(slot->type);
    if (src->name) {
        size_t n = strlen(src->name) + 1;
        if (!(name = malloc(n))) return 0;
        memcpy(name, src->name, n);
    }
    if (src->damage_graphic_count &&
        !(damage = dup_array(src->damage_graphics, src->damage_graphic_count,
                             sizeof(*damage))))
        goto fail;
    if (level >= 5) {
        if (src->attack_count &&
            !(attacks = dup_array(src->attacks, src->attack_count,
                                  sizeof(*attacks))))
            goto fail;
        if (src->armor_count &&
            !(armors = dup_array(src->armors, src->armor_count,
                                 sizeof(*armors))))
            goto fail;
    }
    keep = *slot;
    free(slot->name);
    free(slot->damage_graphics);
    if (level >= 5) {
        free(slot->attacks);
        free(slot->armors);
    }
    /* Root 0x00441f50: every root field. */
    slot->type = src->type;
    slot->unit_class = src->unit_class;
    slot->standing_graphic = src->standing_graphic;
    slot->dying_graphic = src->dying_graphic;
    slot->undead_graphic = src->undead_graphic;
    slot->undead_mode = src->undead_mode;
    slot->hit_points = src->hit_points;
    slot->line_of_sight = src->line_of_sight;
    slot->garrison_capacity = src->garrison_capacity;
    memcpy(slot->size, src->size, sizeof(slot->size));
    slot->dead_unit_id = src->dead_unit_id;
    slot->placement_mode = src->placement_mode;
    slot->placement_overlap_mode = src->placement_overlap_mode;
    memcpy(slot->placement_side_terrain, src->placement_side_terrain,
           sizeof(slot->placement_side_terrain));
    memcpy(slot->placement_terrain, src->placement_terrain,
           sizeof(slot->placement_terrain));
    memcpy(slot->placement_clearance, src->placement_clearance,
           sizeof(slot->placement_clearance));
    slot->placement_class = src->placement_class;
    slot->draw_class = src->draw_class;
    slot->terrain_restriction = src->terrain_restriction;
    slot->elevation_mode = src->elevation_mode;
    memcpy(slot->storage, src->storage, sizeof(slot->storage));
    slot->resource_capacity = src->resource_capacity;
    slot->decay_rate = src->decay_rate;
    slot->blast_defense_level = src->blast_defense_level;
    slot->field_8c = src->field_8c;
    slot->field_8f = src->field_8f;
    slot->negative_id_mode = src->negative_id_mode;
    slot->state_notify_mode = src->state_notify_mode;
    slot->state_effect_mode = src->state_effect_mode;
    slot->state_notify_value = src->state_notify_value;
    slot->selection_sound = src->selection_sound;
    slot->dying_sound = src->dying_sound;
    slot->name = name;
    slot->damage_graphic_count = src->damage_graphic_count;
    slot->damage_graphics = damage;
    /* Kept: ID, enable byte, editor-hide byte; the language name only when
       the source's editor-hide byte is 1. */
    slot->id = keep.id;
    slot->enabled = keep.enabled;
    slot->hide_in_editor = keep.hide_in_editor;
    slot->language_name_id =
        src->hide_in_editor == 1 ? keep.language_name_id
                                 : src->language_name_id;
    src->hide_in_editor = 1;
    if (level >= 2) slot->speed = src->speed;                 /* 0x0043ecb0 */
    if (level >= 3) {                                         /* 0x00440e10 */
        slot->walking_graphic = src->walking_graphic;
        slot->walking_graphic2 = src->walking_graphic2;
        slot->rotation_speed = src->rotation_speed;
    }
    if (level >= 4) {                    /* 0x0043e6f0: not +0xd8 tasks */
        slot->default_task_id = src->default_task_id;
        slot->search_radius = src->search_radius;
        slot->work_rate = src->work_rate;
        memcpy(slot->drop_sites, src->drop_sites, sizeof(slot->drop_sites));
        slot->task_swap_group = src->task_swap_group;
    }
    if (level >= 5) {                                         /* 0x0043f730 */
        slot->attack_graphic = src->attack_graphic;
        slot->base_armor = src->base_armor;
        slot->attack_count = src->attack_count;
        slot->attacks = attacks;
        slot->armor_count = src->armor_count;
        slot->armors = armors;
        slot->terrain_defense_bonus = src->terrain_defense_bonus;
        slot->max_range = src->max_range;
        slot->blast_width = src->blast_width;
        slot->blast_level = src->blast_level;
        slot->reload_time = src->reload_time;
        slot->projectile_unit_id = src->projectile_unit_id;
        slot->accuracy_percent = src->accuracy_percent;
        slot->frame_delay = src->frame_delay;
        memcpy(slot->projectile_offset, src->projectile_offset,
               sizeof(slot->projectile_offset));
        slot->min_range = src->min_range;
        slot->displayed_melee_armor = src->displayed_melee_armor;
        slot->displayed_attack = src->displayed_attack;
        slot->displayed_range = src->displayed_range;
        slot->displayed_reload_time = src->displayed_reload_time;
    }
    if (level == 6) {                                         /* 0x004407b0 */
        slot->projectile_flight_mode = src->projectile_flight_mode;
        slot->projectile_lead = src->projectile_lead;
        slot->projectile_hit_mode = src->projectile_hit_mode;
        slot->projectile_arc = src->projectile_arc;
    }
    if (level >= 7) {                                         /* 0x004ecea0 */
        memcpy(slot->costs, src->costs, sizeof(slot->costs));
        slot->train_time = src->train_time;
        slot->train_location_id = src->train_location_id;
        slot->button_id = src->button_id;
        slot->displayed_pierce_armor = src->displayed_pierce_armor;
    }
    if (level >= 8) {                                         /* 0x004ec4f0 */
        slot->construction_graphic = src->construction_graphic;
        slot->building_adjacent_mode = src->building_adjacent_mode;
        slot->building_angle = src->building_angle;
        slot->disappears_when_built = src->disappears_when_built;
        slot->stack_unit_id = src->stack_unit_id;
        slot->foundation_terrain = src->foundation_terrain;
        slot->building_research_id = src->building_research_id;
    }
    return 1;
fail:
    free(name);
    free(damage);
    free(attacks);
    free(armors);
    return 0;
}

/* ---- live hooks ----------------------------------------------------- */

static const DatGraphic *graphic_of(const AoeLiveAttributeOps *o,
                                    int16_t id) {
    const DatGraphics *g = o->graphics;
    if (!g || id < 0 || (uint16_t)id >= g->count || !g->graphics[id].present)
        return NULL;
    return &g->graphics[id];
}

static int placed_active(const AoeLiveUnit *u) {
    return u->placement.tile != NULL && u->core.state == 2;
}

/* Root 0x004a9fd0 / 0x004aa050 / 0x004aa0e0. */
static int root_live(AoeLiveUnit *u, AoeAttributeOp op, uint8_t a, float v,
                     const AoeLiveAttributeOps *o) {
    const DatUnit *t = u->core.unit_template;
    if (a == 0) {
        double hp = u->core.hit_points;
        int32_t r;
        if (op == AOE_ATTRIBUTE_SET)
            r = aoe_ftol32((double)v * hp / t->hit_points);
        else if (op == AOE_ATTRIBUTE_ADD)
            r = aoe_ftol32(((double)t->hit_points + v) * hp / t->hit_points);
        else
            r = aoe_ftol32((double)v * hp);
        u->core.hit_points = (float)(int16_t)r;
        return 1;
    }
    if (a == 1 && placed_active(u)) {
        int32_t radius;
        if (op == AOE_ATTRIBUTE_SET) radius = aoe_ftol32(v);
        else if (op == AOE_ATTRIBUTE_ADD)
            radius = aoe_ftol32((double)v + t->line_of_sight);
        else radius = aoe_ftol32((double)v * t->line_of_sight);
        return o->vision_reset(o->context, u, radius);
    }
    return 1;
}

int aoe_live_unit_attribute_ror(AoeLiveUnit *u, AoeAttributeOp op,
                                uint8_t a, float v,
                                const AoeLiveAttributeOps *o) {
    const DatUnit *t;
    if (!u || !u->constructed || !(t = u->core.unit_template) || !o ||
        !o->vision_reset || op > AOE_ATTRIBUTE_MUL)
        return 0;
    if (op == AOE_ATTRIBUTE_SET && t->type == 80 && a == 17) {
        u->core.scenario_frame = (uint8_t)aoe_ftol32(v);   /* 0x004ad350 */
        return 1;
    }
    if (op == AOE_ATTRIBUTE_SET && (t->type == 70 || t->type == 80) &&
        a == 101) {                                        /* 0x004ae820 */
        AoePlayerResources *r = o->owner_resources;
        if (!r || !r->values || r->count <= 19) return 0;
        for (unsigned i = 0; i < 3; ++i) {
            int16_t type = t->storage[i].resource_type;
            float amount = t->storage[i].amount;
            if (type == 4) {
                r->values[4] = (float)((double)r->values[4] - amount);
                r->values[4] = (float)((double)v + r->values[4]);
            }
            if (type == 11) {
                r->values[11] = (float)((double)r->values[11] - amount);
                r->values[11] = (float)((double)r->values[11] - v);
            }
            if (type == 19) {
                r->values[19] = (float)((double)r->values[19] - amount);
                r->values[19] = (float)((double)r->values[19] - v);
            }
        }
    }
    return root_live(u, op, a, v, o);
}

static int remap(AoeLiveUnit *u, const AoeLiveAttributeOps *o,
                 int16_t old_id, int16_t new_id, int require_new) {
    const DatGraphic *cur = u->graphic.current_graphic;
    const DatGraphic *old = graphic_of(o, old_id);
    const DatGraphic *next;
    if (cur != old) return 1;
    next = graphic_of(o, new_id);
    if (require_new && !next) return 1;
    return o->select_graphic(o->context, u, next);
}

/* Root 0x004aa210. */
static int root_upgrade(AoeLiveUnit *u, const DatUnit *n,
                        const AoeLiveAttributeOps *o) {
    const DatUnit *t = u->core.unit_template;
    const DatGraphic *cur = u->graphic.current_graphic, *next = NULL;
    float ratio;
    if (placed_active(u) &&
        !o->vision_reset(o->context, u, aoe_ftol32(n->line_of_sight)))
        return 0;
    ratio = (float)((double)u->core.hit_points / t->hit_points);
    u->core.hit_points = (float)t->hit_points;
    if (u->core.state == 2 && !o->damage_graphics(o->context, u)) return 0;
    if (cur == graphic_of(o, t->standing_graphic))
        next = graphic_of(o, n->standing_graphic);
    else if (cur == graphic_of(o, t->dying_graphic))
        next = graphic_of(o, n->dying_graphic);
    else if (cur == graphic_of(o, t->undead_graphic))
        next = graphic_of(o, n->undead_graphic);
    u->core.hit_points = (float)((double)n->hit_points * ratio);
    if (next && !o->select_graphic(o->context, u, next)) return 0;
    if (u->core.state == 2) {
        int ok;
        u->core.unit_template = n;
        ok = o->damage_graphics(o->context, u);
        u->core.unit_template = t;
        if (!ok) return 0;
    }
    return 1;
}

int aoe_live_unit_upgrade_ror(AoeLiveUnit *u, const DatUnit *n,
                              const AoeLiveAttributeOps *o) {
    const DatUnit *t;
    uint8_t type;
    if (!u || !u->constructed || !(t = u->core.unit_template) || !n || !o ||
        !o->vision_reset || !o->select_graphic || !o->damage_graphics ||
        !o->retask || !o->adjacent)
        return 0;
    type = t->type;
    if (type == 80 &&
        !remap(u, o, t->construction_graphic, n->construction_graphic, 0))
        return 0;
    if (type >= 50 && type <= 80 &&
        !remap(u, o, t->attack_graphic, n->attack_graphic, 1))
        return 0;
    if (type >= 40 && type <= 80) o->retask(o->context, u, n);
    if (type >= 30 && type <= 80) {
        const DatGraphic *cur = u->graphic.current_graphic, *next = NULL;
        int hit = 0;
        if (cur == graphic_of(o, t->walking_graphic)) {
            next = graphic_of(o, n->walking_graphic);
            hit = 1;
        } else if (cur == graphic_of(o, t->walking_graphic2)) {
            next = graphic_of(o, n->walking_graphic2);
            hit = 1;
        }
        if (hit && next && !o->select_graphic(o->context, u, next)) return 0;
    }
    if (!root_upgrade(u, n, o)) return 0;
    if (type == 80 && t->building_adjacent_mode) o->adjacent(o->context, u);
    return 1;
}

/* ---- effects -------------------------------------------------------- */

/* Live hook on each awake unit of the template, then the template. */
static int attribute_one(const AoeEffectTargetOps *o, DatUnit *t,
                         AoeAttributeOp op, uint8_t attribute, float amount) {
    for (size_t k = 0; k < o->unit_count(o->context); ++k) {
        AoeLiveUnit *u = o->unit_at(o->context, k);
        if (u && u->core.unit_template == t &&
            !aoe_live_unit_attribute_ror(u, op, attribute, amount, o->live))
            return 0;
    }
    return aoe_template_attribute_ror(t, op, attribute, amount);
}

/* 0x0045d6a0 / 0x0045d7a0 / 0x0045d8a0. */
static int attribute_command(const AoeEffectTargetOps *o, AoeAttributeOp op,
                             int16_t unit, int16_t cls, uint8_t attribute,
                             float amount) {
    AoePlayerTemplates *ts = o->templates;
    if (unit >= 0 && (uint16_t)unit < ts->count) {
        DatUnit *t = aoe_player_templates_get(ts, (uint16_t)unit);
        return t ? attribute_one(o, t, op, attribute, amount) : 1;
    }
    if (cls < 0) return 1;
    for (uint16_t i = 0; i < ts->count; ++i) {
        DatUnit *t = aoe_player_templates_get(ts, i);
        if (t && t->unit_class == cls &&
            !attribute_one(o, t, op, attribute, amount))
            return 0;
    }
    return 1;
}

int aoe_effect_apply_ror(const DatEffects *effects, int16_t effect_id,
                         const AoeEffectTargetOps *o) {
    const DatEffect *e;
    if (!effects || !o || !o->templates || !o->resources || !o->unit_count ||
        !o->unit_at || !o->resource_add || !o->live)
        return 0;
    if (effect_id < 0 || (uint32_t)effect_id >= effects->count) return 1;
    e = &effects->effects[effect_id];
    for (uint16_t i = 0; i < e->command_count; ++i)
        if ((uint8_t)e->commands[i].type == 102 && o->research)
            (void)aoe_research_set_enabled_ror(
                o->research, (int16_t)aoe_ftol32(e->commands[i].amount), 0);
    for (uint16_t i = 0; i < e->command_count; ++i) {
        const DatEffectCommand *c = &e->commands[i];
        switch ((uint8_t)c->type) {
        case 0:
            if (!attribute_command(o, AOE_ATTRIBUTE_SET, c->target_unit,
                                   c->unit_class, (uint8_t)c->attribute,
                                   c->amount))
                return 0;
            break;
        case 4:
            if (!attribute_command(o, AOE_ATTRIBUTE_ADD, c->target_unit,
                                   c->unit_class, (uint8_t)c->attribute,
                                   c->amount))
                return 0;
            break;
        case 5:
            if (!attribute_command(o, AOE_ATTRIBUTE_MUL, c->target_unit,
                                   c->unit_class, (uint8_t)c->attribute,
                                   c->amount))
                return 0;
            break;
        case 1: /* 0x0045cfc0 set, or +0x78(a, amount, 0) */
            if (c->target_unit > -1) {
                if (c->unit_class == 0) {
                    if (c->target_unit < o->resources->count)
                        o->resources->values[c->target_unit] = c->amount;
                } else {
                    o->resource_add(o->context, c->target_unit, c->amount);
                }
            }
            break;
        case 2: { /* 0x0045d010 -> template 0x00443130 */
            DatUnit *t;
            if (c->target_unit > -1 &&
                (uint16_t)c->target_unit < o->templates->count &&
                (t = aoe_player_templates_get(o->templates,
                                              (uint16_t)c->target_unit)))
                t->enabled = (uint8_t)c->unit_class;
            break;
        }
        case 3: { /* 0x0045d9a0 */
            int16_t a = c->target_unit, b = c->unit_class;
            DatUnit *ta, *tb;
            if (a < 0 || (uint16_t)a >= o->templates->count || b < 0 ||
                (uint16_t)b >= o->templates->count)
                break; /* the original would read past the table */
            ta = aoe_player_templates_get(o->templates, (uint16_t)a);
            tb = aoe_player_templates_get(o->templates, (uint16_t)b);
            if (!ta || !tb) break;
            for (size_t k = 0; k < o->unit_count(o->context); ++k) {
                AoeLiveUnit *u = o->unit_at(o->context, k);
                if (u && u->core.unit_template == ta &&
                    !aoe_live_unit_upgrade_ror(u, tb, o->live))
                    return 0;
            }
            if (!aoe_template_upgrade_copy_ror(ta, tb)) return 0;
            break;
        }
        default:
            break;
        }
    }
    return 1;
}

/* ---- research manager ----------------------------------------------- */

int aoe_research_manager_init_ror(AoeResearchCompletionManagerRor *m,
                                  const DatTechs *techs) {
    if (!m || !techs || techs->count > INT16_MAX) return 0;
    m->count = (int16_t)techs->count;
    m->entries = m->count ? calloc((size_t)m->count, sizeof(*m->entries))
                          : NULL;
    return !m->count || m->entries != NULL;
}

void aoe_research_manager_free_ror(AoeResearchCompletionManagerRor *m) {
    if (!m) return;
    free(m->entries);
    m->entries = NULL;
    m->count = 0;
}

int aoe_research_rebuild_ror(AoeResearchCompletionManagerRor *m,
                             const DatTechs *techs, AoePlayerResources *res) {
    int16_t pending = 0;
    if (!m || !techs || (m->count && !m->entries) ||
        (uint32_t)m->count > techs->count)
        return 0;
    for (int16_t i = 0; i < m->count; ++i) {
        const DatTech *tech = &techs->techs[i];
        int16_t state = m->entries[i].state;
        if (state < 2 && state > -1) {
            uint8_t met = 0;
            ++pending;
            for (int j = 0; j < 4; ++j) {
                int16_t r = tech->required_techs[j];
                if (r >= 0 && r < m->count && m->entries[r].state == 3) ++met;
            }
            if (met >= tech->required_count) {
                /* fild time; fcomp progress; test ah,0x41 */
                if (!((double)tech->research_time >
                      m->entries[i].progress)) {
                    if (!aoe_research_complete_ror(m, i)) return 0;
                    --pending;
                } else if (m->entries[i].state < 1) {
                    m->entries[i].state = 1;
                }
            } else if (m->entries[i].state > 0) {
                m->entries[i].state = 0;
            }
        } else if (state == 2) {
            ++pending;
        }
    }
    if (pending == 0 && res && res->values && res->count > 39 &&
        res->values[39] == 0.0f)
        res->values[39] = 1.0f;
    return 1;
}

int aoe_research_set_enabled_ror(AoeResearchCompletionManagerRor *m,
                                 int16_t id, int enable) {
    if (!m || id < 0 || id >= m->count || !m->entries) return 1;
    if (enable) {
        if (m->entries[id].state == -1) m->entries[id].state = 0;
    } else if (m->entries[id].state != 3) {
        m->entries[id].state = -1;
    }
    return 1;
}
