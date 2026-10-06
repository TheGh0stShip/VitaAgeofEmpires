#include "unit_lifecycle.h"

#include <string.h>

#define CALL(ops, fn, ...) do { if ((ops) && (ops)->fn) (ops)->fn((ops)->ctx, __VA_ARGS__); } while (0)

/* -999.0f, the root init's "not yet placed" position (0xc479c000). */
#define AOE_UNIT_LIFE_UNPLACED (-999.0f)

/* 0x004a6590: root load constructor. Clears the runtime fields that the
   record load (0x004a6bb0, unit_record.c) does not supply. 0x004150c0 (type
   20) runs the same defaults before its own load. */
void aoe_unit_life_load_defaults(AoeUnitLife *u)
{
    memset(u, 0, sizeof(*u));
    u->source_id_78 = -1;
    u->source_id_7c = -1;
    u->range_80 = 5.0f;
}

/* 0x004a67d0: root init(template, owner, x, y, z). The random draw around
   the selection sound reseeds the generator with the value it drew, so the
   stream advances by exactly one rand(). */
void aoe_unit_life_init_root(AoeUnitLife *u, const AoeUnitLifeOps *ops,
                             const AoeUnitLifeTemplate *t, AoeUnitLifeOwner *owner,
                             float x, float y, float z)
{
    u->graphic_14 = 0;
    u->ai = NULL;
    u->flag_86 = 0;
    u->graphic_10 = 0;
    u->source_id_78 = -1;
    u->range_80 = 5.0f;
    u->owner = owner;
    u->tmpl = t;
    u->tile_node = NULL;
    u->tile_node2 = NULL;
    u->hit_points = (float)t->hit_points;
    u->alive = u->hit_points > 0.0f;
    u->doppelganger = t->kind == 0x19;
    u->flag_35 = 0;
    u->x = AOE_UNIT_LIFE_UNPLACED;
    u->y = AOE_UNIT_LIFE_UNPLACED;
    u->z = 0.0f;
    memset(u->draw_rect, 0, sizeof(u->draw_rect));
    u->selected_bits = 0;
    u->flag_37 = 0;
    u->flag_4f = 0;
    u->state = 2;
    u->flag_34 = 0;
    u->flag_4b = 0;
    u->owner_node = NULL;
    CALL(ops, storage_reset, u);
    CALL(ops, set_graphic, u, t->standing_graphic);
    u->y = y;
    u->z = z;
    u->x = x;
    u->graphic_14 = 0;
    CALL(ops, place, u, x, y, z);
    if (ops && ops->owner_register)
        u->owner_node = ops->owner_register(ops->ctx, u, u->alive, u->doppelganger);
    {
        int32_t seed = (ops && ops->rand_draw) ? ops->rand_draw(ops->ctx) : 0;
        if (t->has_selection_sound && owner && owner->player_id == owner->viewed_player)
            CALL(ops, play_selection_sound, u);
        CALL(ops, srand_seed, seed);
    }
}

/* 0x004a6640: root destructor (0x004a6570 the deleting form; type 20's
   0x004150b0 is a thunk of it). */
void aoe_unit_life_destroy_root(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    size_t i;
    CALL(ops, sprites_stop, u);
    if (u->owner) {
        if (u->state < 3)
            CALL(ops, tile_drop, u);
        if (u->state < 7)
            CALL(ops, outcome_drop, u);
    }
    if (u->sprite_manager) {
        CALL(ops, sprite_manager_free, u->sprite_manager);
        u->sprite_manager = NULL;
    }
    if (u->owner) {
        if (u->selected_bits & 1)
            CALL(ops, selection_remove, u);
        CALL(ops, owner_unregister, u, u->alive, u->doppelganger, u->owner_node);
        u->owner = NULL;
    }
    if (u->tile_node) {
        CALL(ops, tile_unlink, u, u->tile_node);
        u->tile_node = NULL;
    }
    if (u->tmpl && u->tmpl->footprint_radius > 0.0f)
        CALL(ops, footprint_lift, u);
    if (u->tile_node2) {
        CALL(ops, tile_unlink, u, u->tile_node2);
        u->tile_node2 = NULL;
    }
    if (u->contents) {
        for (i = 0; i < u->content_count; ++i)
            CALL(ops, content_release, u->contents[i]);
        u->contents = NULL;
        u->content_count = 0;
    }
    if (u->ai) {
        CALL(ops, ai_free, u->ai);
        u->ai = NULL;
    }
    u->buf_b = NULL;
    memset(u->buf_b_n, 0, sizeof(u->buf_b_n));
    u->buf_a = NULL;
    memset(u->buf_a_n, 0, sizeof(u->buf_a_n));
}

/* 0x00415170: type-20 init (0x00415020 runs it when its flag is set):
   root init, class 0x14, +0x88 cleared and the unit woken (0x004ac380(0)). */
void aoe_unit_life_init_type20(AoeUnitLife *u, const AoeUnitLifeOps *ops,
                               const AoeUnitLifeTemplate *t, AoeUnitLifeOwner *owner,
                               float x, float y, float z)
{
    aoe_unit_life_init_root(u, ops, t, owner, x, y, z);
    u->class_code = 0x14;
    u->type20_88 = 0;
    CALL(ops, wake, u);
}

/* 0x004b0d20: the type-90 constructor runs the root init only (its class
   byte stays the root's). */
void aoe_unit_life_init_type90(AoeUnitLife *u, const AoeUnitLifeOps *ops,
                               const AoeUnitLifeTemplate *t, AoeUnitLifeOwner *owner,
                               float x, float y, float z)
{
    aoe_unit_life_init_root(u, ops, t, owner, x, y, z);
}

/* The live tail of 0x00405b90: class 0x28 and a fresh action list
   (vtable +0x21c). */
void aoe_unit_life_init_type40(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    u->class_code = 0x28;
    u->type40_180 = 0;
    u->action_list = (ops && ops->action_list_make) ? ops->action_list_make(ops->ctx, u) : NULL;
    u->type40_188 = 0;
}

/* 0x00405b60: type-30 reinit (0x0044b070) then +0x180 cleared. */
void aoe_unit_life_reinit_type40(AoeUnitLife *u)
{
    u->type40_180 = 0;
}

/* 0x00405cc0 (0x00449000 is a thunk of it): root post-load fixup
   then the action list relinked (0x00403df0). */
void aoe_unit_life_post_load_type40(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    CALL(ops, post_load_fixup, u);
    if (u->action_list)
        CALL(ops, action_list_relink, u->action_list);
}

/* 0x00405d20: type-30 reinit (0x0044b940) then the actions cleared. */
void aoe_unit_life_reset_type40(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    if (u->action_list)
        CALL(ops, action_list_clear, u->action_list);
}

/* 0x00405ad0's live half: the action list destroyed, then down the chain
   (type 30's record half is unit_record.c) to the root destructor. */
void aoe_unit_life_destroy_type40(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    if (u->action_list) {
        CALL(ops, action_list_free, u->action_list);
        u->action_list = NULL;
    }
    aoe_unit_life_destroy_root(u, ops);
}

/* 0x00405b40: cleanup slot - actions cleared, then root cleanup 0x004a6920. */
static void cleanup_type40(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    if (u->action_list)
        CALL(ops, action_list_clear, u->action_list);
    CALL(ops, root_cleanup, u);
}

void aoe_unit_life_init_type60(AoeUnitLife *u)
{
    u->class_code = 0x3c;
    u->type60_1c0 = 0;
}

/* 0x00448e50 (0x00448dc0 the deleting form): combat release 0x004496a0,
   then the type-50 destructor chain. */
void aoe_unit_life_destroy_type60(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    CALL(ops, combat_release, u);
    aoe_unit_life_destroy_type40(u, ops);
}

/* 0x00448ea0: combat release then the type-40 cleanup 0x00405b40. */
void aoe_unit_life_cleanup_type60(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    CALL(ops, combat_release, u);
    cleanup_type40(u, ops);
}

static void population_adjust(const AoeUnitLife *u, float delta)
{
    const AoeUnitLifeTemplate *t = u->tmpl;
    float *res;
    size_t idx;
    if (!t || !u->owner || !u->owner->resources)
        return;
    res = u->owner->resources;
    if (t->class_code == 4 || t->class_code == 0x15 || t->class_code == 2)
        idx = AOE_UNIT_LIFE_RES_POP_A;
    else if ((t->class_code != 0x12 && t->kind == 'F') ||
             t->id == 0x4f || t->id == 0x45 || t->id == 199)
        idx = AOE_UNIT_LIFE_RES_POP_B;
    else
        return;
    if (idx < u->owner->resource_count)
        res[idx] += delta;
}

/* 0x004ae4e0: type-70 init tail (after the type-50 init 0x00426140;
   0x004ae2d0 runs it when its flag is set; the load constructor 0x004ae380
   runs the type-50 load constructor and the record load 0x004ae5c0). Player 0 (gaia) units are
   flagged at +0x1b8. Classes 2/4/0x15 count into resource 37; other 'F'
   kinds (not class 0x12) and ids 0x45/0x4f/199 count into resource 40.
   Class 0x12 starts carrying 100 of resource 0x22. */
void aoe_unit_life_init_type70(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    if (u->owner && u->owner->player_id == 0)
        u->gaia_owned = 1;
    u->class_code = 0x46;
    if (ops && ops->ai_make)
        u->ai = ops->ai_make(ops->ctx, u);
    u->private_template = 0;
    population_adjust(u, 1.0f);
    if (u->tmpl && u->tmpl->class_code == 0x12) {
        u->carried_amount = 100.0f;
        u->carried_type = 0x22;
    }
}

/* 0x004ae3f0 (0x004ae360 the deleting form): a private template is freed
   and replaced by the owner's shared one of the same id before the counter
   is undone, then the type-50 destructor chain runs. */
void aoe_unit_life_destroy_type70(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    if (u->private_template) {
        const AoeUnitLifeTemplate *old = u->tmpl;
        int16_t id = old ? old->id : 0;
        if (old)
            CALL(ops, template_free, old);
        u->tmpl = (u->owner && u->owner->templates && id >= 0 &&
                   (size_t)id < u->owner->template_count)
                      ? u->owner->templates[id] : NULL;
    }
    population_adjust(u, -1.0f);
    aoe_unit_life_destroy_type40(u, ops);
}

static void type80_zero(AoeUnitLife *u)
{
    u->pairs = NULL;
    u->pair_count = 0;
    u->pair_cap = 0;
    u->flag_1c8 = 1;
    u->queue = NULL;
    u->flag_1d0 = 0;
    u->flag_1d1 = 0;
    u->status = -1;
    u->watch = NULL;
}

static void type80_watch(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    if (ops && ops->watch_add)
        u->watch = ops->watch_add(ops->ctx, u, (int32_t)u->x, (int32_t)u->y);
}

/* 0x004ac3f0 (completion value given) / 0x004ac550 (completion -1):
   the type-70 constructor without init, the building fields, the type-80
   init 0x004ac8a0 when asked, the default task for template 0x114 and the
   visibility watch on the unit's tile. */
void aoe_unit_life_construct_type80(AoeUnitLife *u, const AoeUnitLifeOps *ops,
                                    int32_t completion_value, int run_init)
{
    u->completion_value = completion_value;
    type80_zero(u);
    if (run_init)
        CALL(ops, building_init, u);
    if (u->tmpl && u->tmpl->id == 0x114)
        CALL(ops, default_task, u, u->tmpl->default_task);
    type80_watch(u, ops);
    memset(u->tail_1e4, 0, sizeof(u->tail_1e4));
}

/* 0x004ac670: load constructor - the same zeros, the record load
   (0x004ac960) by the caller, then the watch. +0x1e4.. are left alone. */
void aoe_unit_life_load_type80(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    type80_zero(u);
    type80_watch(u, ops);
}

static void type80_drop_pairs(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    CALL(ops, pairs_free, u->pairs);
    u->pairs = NULL;
}

static void type80_status4_release(AoeUnitLife *u)
{
    if (u->status == 4) {
        u->status = -1;
        if (u->owner && u->owner->status4_counter)
            *u->owner->status4_counter -= 1;
    }
}

/* 0x004ac740 (0x004ac530 the deleting form). The pair counts are not reset
   here, only in the cleanup slot. */
void aoe_unit_life_destroy_type80(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    CALL(ops, watch_remove, u);
    if (u->queue) {
        CALL(ops, queue_free, u->queue);
        u->queue = NULL;
    }
    if (u->pairs) {
        type80_drop_pairs(u, ops);
        type80_status4_release(u);
    }
    aoe_unit_life_destroy_type70(u, ops);
}

/* 0x004ac810: cleanup slot. */
void aoe_unit_life_cleanup_type80(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    if (u->queue)
        CALL(ops, action_list_clear, u->queue);
    if (u->pairs) {
        type80_drop_pairs(u, ops);
        u->pair_count = 0;
        u->pair_cap = 0;
        u->flag_1c8 = 1;
        u->flag_1d0 = 0;
        u->flag_1d1 = 0;
        type80_status4_release(u);
    }
    cleanup_type40(u, ops);
}

void aoe_unit_life_reset_queue_type80(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    if (u->status != -1) {
        type80_status4_release(u);
        u->status = -1;
    }
    u->pair_cap = 0;
    if (u->queue)
        CALL(ops, action_list_clear, u->queue);
    u->flag_1d0++;
}

void aoe_unit_life_post_load_type80(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    aoe_unit_life_post_load_type40(u, ops);
    if (u->queue)
        CALL(ops, action_list_relink, u->queue);
}

void aoe_unit_life_remove_type80(AoeUnitLife *u, const AoeUnitLifeOps *ops)
{
    float fraction = 0.0f;
    if (u->state >= 2) {
        CALL(ops, remove_finished, u);
        return;
    }
    if (u->selected_bits & 1)
        CALL(ops, selection_remove, u);
    if (u->hit_points > 0.0f && u->tmpl) {
        float time = (float)u->tmpl->build_time;
        fraction = (float)((double)((time - u->build_progress) / time) * 0.5);
    }
    if (fraction > 1.0f)
        fraction = 1.0f;
    if (u->tmpl) {
        for (int i = 0; i < 3; ++i) {
            if (u->tmpl->build_cost[i].resource >= 0 && u->tmpl->build_cost[i].enabled)
                CALL(ops, owner_add_attribute, u, u->tmpl->build_cost[i].resource,
                     (float)u->tmpl->build_cost[i].amount * fraction);
        }
    }
    u->hit_points = 0.0f;
}
