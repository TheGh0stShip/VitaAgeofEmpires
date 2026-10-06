#include "tech_tree.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Patched RoR 1.0a technology: template attributes, upgrade copy, effects
   and research availability. */

static DatUnit *make_unit(uint8_t type, int16_t id, const char *name) {
    DatUnit *u = calloc(1, sizeof(*u));
    assert(u);
    u->present = 1;
    u->type = type;
    u->id = id;
    u->name = malloc(strlen(name) + 1);
    strcpy(u->name, name);
    return u;
}

static int completed_effects[16];
static unsigned completed_count;
static void apply_effect(void *context, int16_t research_id) {
    (void)context;
    completed_effects[completed_count++] = research_id;
}
static const DatTechs *rebuild_techs;
static void rebuild(void *context) {
    AoeResearchCompletionManagerRor *m = context;
    assert(aoe_research_rebuild_ror(m, rebuild_techs, NULL));
}
static void wonder(void *context, int index) { (void)context; (void)index; }

/* Effect target fixture. */
static AoeLiveUnit *units[2];
static size_t unit_count(void *c) { (void)c; return 2; }
static AoeLiveUnit *unit_at(void *c, size_t i) { (void)c; return units[i]; }
static float added[64];
static void resource_add(void *c, int16_t type, float amount) {
    (void)c;
    added[type] += amount;
}
static unsigned vision_resets;
static int32_t last_radius;
static int vision_reset(void *c, AoeLiveUnit *u, int32_t r) {
    (void)c; (void)u;
    ++vision_resets;
    last_radius = r;
    return 1;
}
static int select_graphic(void *c, AoeLiveUnit *u, const DatGraphic *g) {
    (void)c;
    u->graphic.current_graphic = g;
    return 1;
}
static unsigned damage_calls;
static int damage_graphics(void *c, AoeLiveUnit *u) {
    (void)c; (void)u;
    ++damage_calls;
    return 1;
}
static unsigned retasks;
static void retask(void *c, AoeLiveUnit *u, const DatUnit *t) {
    (void)c; (void)u; (void)t;
    ++retasks;
}
static void adjacent(void *c, AoeLiveUnit *u) { (void)c; (void)u; }

int main(void) {
    /* --- template attributes ---------------------------------------- */
    {
        DatUnit *u = make_unit(70, 5, "Archer");
        DatUnitAttackArmor attacks[2] = {{3, 4}, {4, 2}};
        DatUnitAttackArmor armors[2] = {{3, 1}, {4, 0}};
        u->attacks = attacks;
        u->attack_count = 2;
        u->armors = armors;
        u->armor_count = 2;
        u->hit_points = 35;
        u->line_of_sight = 7.0f;
        u->speed = 1.2f;
        u->work_rate = 1.0f;
        u->projectile_unit_id = 300;
        u->costs[0] = (DatUnitCost){0, 40, 1};
        u->costs[1] = (DatUnitCost){4, 1, 0};
        u->storage[0] = (DatUnitStorage){11, 1.0f, 2};
        /* Root, type 20, type 40 through the class chain. */
        assert(aoe_template_attribute_ror(u, AOE_ATTRIBUTE_ADD, 0, 5.9f));
        assert(u->hit_points == 40);
        assert(aoe_template_attribute_ror(u, AOE_ATTRIBUTE_MUL, 1, 1.5f));
        assert(u->line_of_sight == 10.5f);
        assert(aoe_template_attribute_ror(u, AOE_ATTRIBUTE_ADD, 5, 0.3f));
        assert(u->speed == (float)(0.3f + (double)1.2f));
        assert(aoe_template_attribute_ror(u, AOE_ATTRIBUTE_MUL, 13, 1.2f));
        assert(u->work_rate == 1.2f);
        /* Attack set indexes entries by value >> 8; add searches classes. */
        assert(aoe_template_attribute_ror(u, AOE_ATTRIBUTE_SET, 9,
                                          (float)(1 * 256 + 7)));
        assert(attacks[1].amount == 7 && attacks[0].amount == 4);
        assert(aoe_template_attribute_ror(u, AOE_ATTRIBUTE_ADD, 9,
                                          (float)(3 * 256 + 2)));
        assert(attacks[0].amount == 6);
        assert(aoe_template_attribute_ror(u, AOE_ATTRIBUTE_ADD, 8,
                                          (float)(9 * 256 + 2)));
        assert(armors[0].amount == 1 && armors[1].amount == 0);
        assert(aoe_template_attribute_ror(u, AOE_ATTRIBUTE_MUL, 9, 2.0f));
        assert(attacks[0].amount == 6);
        /* Projectile set keeps only the low byte; multiply of base armor
           reads the projectile word. */
        assert(aoe_template_attribute_ror(u, AOE_ATTRIBUTE_SET, 16, 300.0f));
        assert(u->projectile_unit_id == 44);
        assert(aoe_template_attribute_ror(u, AOE_ATTRIBUTE_MUL, 15, 2.0f));
        assert(u->base_armor == 88);
        /* Attribute 18 add/multiply store. */
        assert(aoe_template_attribute_ror(u, AOE_ATTRIBUTE_MUL, 18, 3.0f));
        assert(u->terrain_table == 3);
        /* Tribe 100: deductible costs; 101: the rest and storage. */
        assert(aoe_template_attribute_ror(u, AOE_ATTRIBUTE_MUL, 100, 0.5f));
        assert(u->costs[0].amount == 20 && u->costs[1].amount == 1);
        assert(aoe_template_attribute_ror(u, AOE_ATTRIBUTE_SET, 101, 2.0f));
        assert(u->costs[1].amount == -2 && u->costs[0].amount == 20 &&
               u->storage[0].amount == -2.0f);
        u->attacks = NULL;
        u->armors = NULL;
        free(u->name);
        free(u);
    }

    /* --- upgrade copy ------------------------------------------------- */
    {
        DatUnit *slot = make_unit(70, 4, "Clubman");
        DatUnit *src = make_unit(70, 6, "Axeman");
        DatUnitTask task = {.action_type = 7};
        DatUnitAttackArmor a = {4, 5};
        slot->tasks = &task;
        slot->task_count = 1;
        slot->enabled = 1;
        slot->hide_in_editor = 0;
        slot->language_name_id = 100;
        src->enabled = 0;
        src->hide_in_editor = 1;
        src->language_name_id = 200;
        src->hit_points = 50;
        src->attacks = &a;
        src->attack_count = 1;
        src->train_time = 30;
        /* 0x00441f50 also takes the picture (+0x4c), creation text
           (+0x0e), map mode and colour (+0x8e / +0x90) and the help, tree
           and hotkey IDs (+0x98..+0xa0): an age upgrade changes the
           object panel's picture. */
        slot->icon = 33;
        src->icon = 36;
        src->language_creation_id = 6200;
        src->map_mode = 1;
        src->map_color = 9;
        src->language_help_id = 105200;
        src->language_tree_id = 155200;
        src->hotkey_id = 16200;
        assert(aoe_template_upgrade_copy_ror(slot, src));
        assert(slot->icon == 36 && slot->language_creation_id == 6200 && slot->map_mode == 1 &&
               slot->map_color == 9 && slot->language_help_id == 105200 &&
               slot->language_tree_id == 155200 && slot->hotkey_id == 16200);
        assert(slot->id == 4 && slot->enabled == 1 &&
               slot->hide_in_editor == 0 && slot->language_name_id == 200 &&
               slot->hit_points == 50 && slot->train_time == 30 &&
               !strcmp(slot->name, "Axeman") && slot->name != src->name &&
               slot->attack_count == 1 && slot->attacks != &a &&
               slot->attacks[0].amount == 5 && slot->tasks == &task);
        src->hide_in_editor = 0;
        src->language_name_id = 201;
        assert(aoe_template_upgrade_copy_ror(slot, src));
        assert(slot->language_name_id == 201 && src->hide_in_editor == 1);
        slot->tasks = NULL;
        free(slot->name);
        free(slot->attacks);
        free(slot);
        src->attacks = NULL;
        free(src->name);
        free(src);
    }

    /* --- effects ------------------------------------------------------ */
    {
        AoePlayerTemplates templates = {0};
        DatUnit *slots[4];
        float values[64] = {0};
        AoePlayerResources resources = {64, values};
        AoeLiveUnit u0, u1;
        DatGraphic graphics_list[2] = {{.present = 1}, {.present = 1}};
        DatGraphics graphics = {.count = 2, .graphics = graphics_list};
        AoeLiveAttributeOps live = {NULL, &graphics, vision_reset,
                                    select_graphic, damage_graphics, retask,
                                    adjacent, &resources};
        AoeResearchProgressEntry entries[4] = {0};
        AoeResearchCompletionManagerRor research = {4, entries};
        AoeEffectTargetOps o = {NULL, &templates, &resources, unit_count,
                                unit_at, resource_add, &research, &live};
        DatEffectCommand commands[7] = {
            {0, 1, -1, 0, 60.0f},  /* set HP of template 1 */
            {4, -1, 6, 1, 2.0f},   /* add LOS to class 6 */
            {1, 3, 0, 0, 150.0f},  /* set resource 3 */
            {1, 0, 1, 0, 25.0f},   /* add resource 0 */
            {2, 2, 1, 0, 0.0f},    /* enable template 2 */
            {(int8_t)102, 0, 0, 0, 2.0f}, /* disable tech 2 */
            {3, 1, 3, 0, 0.0f}};   /* upgrade 1 -> 3 */
        DatEffect effect = {"test", 7, commands};
        DatEffects effects = {1, &effect, 0};
        templates.count = 4;
        templates.slots = slots;
        slots[0] = make_unit(70, 0, "a");
        slots[1] = make_unit(70, 1, "b");
        slots[2] = make_unit(70, 2, "c");
        slots[3] = make_unit(70, 3, "d");
        slots[1]->hit_points = 30;
        slots[1]->unit_class = 6;
        slots[1]->line_of_sight = 4.0f;
        slots[1]->standing_graphic = 0;
        slots[3]->hit_points = 90;
        slots[3]->standing_graphic = 1;
        slots[3]->line_of_sight = 9.0f;
        memset(&u0, 0, sizeof(u0));
        memset(&u1, 0, sizeof(u1));
        u0.constructed = u1.constructed = 1;
        u0.core.unit_template = slots[1];
        u0.core.hit_points = 15.0f;
        u0.core.state = 2;
        u0.graphic.current_graphic = &graphics_list[0];
        u1.core.unit_template = slots[0];
        u1.core.hit_points = 10.0f;
        units[0] = &u0;
        units[1] = &u1;
        assert(aoe_effect_apply_ror(&effects, 0, &o));
        /* HP set: the live unit is rescaled against the old maximum 30
           (15 * 60 / 30), then the template; LOS add by class. */
        assert(slots[1]->name && !strcmp(slots[1]->name, "d"));
        assert(slots[1]->id == 1);
        assert(values[3] == 150.0f && added[0] == 25.0f);
        assert(slots[2]->enabled == 1);
        assert(entries[2].state == -1);
        /* Upgrade: HP ratio 30/60 kept against 90, standing graphic
           remapped, damage graphics run before and after. */
        assert(u0.core.hit_points == 45.0f &&
               u0.graphic.current_graphic == &graphics_list[1] &&
               damage_calls == 2 && retasks == 1 && u0.core.unit_template ==
               slots[1] && slots[1]->hit_points == 90);
        assert(u1.core.hit_points == 10.0f);
        assert(aoe_effect_apply_ror(&effects, 5, &o));
        for (unsigned i = 0; i < 4; ++i) {
            free(slots[i]->name);
            free(slots[i]);
        }
    }

    /* --- research availability ---------------------------------------- */
    {
        DatTech techs_list[4];
        DatTechs techs = {4, techs_list, 0};
        AoeResearchCompletionManagerRor m = {0};
        float values[40] = {0};
        AoePlayerResources res = {40, values};
        memset(techs_list, 0, sizeof(techs_list));
        for (unsigned i = 0; i < 4; ++i)
            for (unsigned j = 0; j < 4; ++j)
                techs_list[i].required_techs[j] = -1;
        /* 0: zero time, no prerequisites -> completes at once;
           1: needs 0, time 0 -> completes in the cascade;
           2: needs 1, time 30 -> available; 3: needs 2 -> unavailable. */
        techs_list[1].required_techs[0] = 0;
        techs_list[1].required_count = 1;
        techs_list[2].required_techs[0] = 1;
        techs_list[2].required_count = 1;
        techs_list[2].research_time = 30;
        techs_list[3].required_techs[0] = 2;
        techs_list[3].required_count = 1;
        techs_list[3].research_time = 10;
        assert(aoe_research_manager_init_ror(&m, &techs) && m.count == 4);
        m.context = &m;
        m.apply_effect = apply_effect;
        m.rebuild = rebuild;
        m.wonder_complete = wonder;
        rebuild_techs = &techs;
        assert(aoe_research_rebuild_ror(&m, &techs, &res));
        assert(m.entries[0].state == 3 && m.entries[1].state == 3 &&
               m.entries[2].state == 1 && m.entries[3].state == 0 &&
               completed_count == 2 && completed_effects[0] == 0 &&
               completed_effects[1] == 1 && values[39] == 0.0f);
        /* Disable/enable. */
        assert(aoe_research_set_enabled_ror(&m, 2, 0) &&
               m.entries[2].state == -1);
        assert(aoe_research_set_enabled_ror(&m, 0, 0) &&
               m.entries[0].state == 3);
        assert(aoe_research_set_enabled_ror(&m, 2, 1) &&
               m.entries[2].state == 0);
        /* Nothing left pending: attribute 39 flips to 1. */
        m.entries[2].state = -1;
        m.entries[3].state = -1;
        assert(aoe_research_rebuild_ror(&m, &techs, &res) &&
               values[39] == 1.0f);
        aoe_research_manager_free_ror(&m);
    }
    return 0;
}
