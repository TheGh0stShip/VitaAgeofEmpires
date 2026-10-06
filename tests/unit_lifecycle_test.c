#include "unit_lifecycle.h"

#include <assert.h>
#include <string.h>

typedef struct Log {
    char seq[64];
    int n;
    int sound;
    int32_t seed;
    int unregister_node_ok;
    int watch_tx, watch_ty;
    int16_t task;
    int freed_template;
} Log;

static Log g;
static int ai_token, list_token, watch_token, node_token;
static void push(char c) { g.seq[g.n++] = c; g.seq[g.n] = 0; }

static void set_graphic(void *c, AoeUnitLife *u, int32_t gr) { (void)c; (void)u; (void)gr; push('G'); }
static void place(void *c, AoeUnitLife *u, float x, float y, float z)
{ (void)c; assert(u->x == x && u->y == y && u->z == z); push('P'); }
static void storage_reset(void *c, AoeUnitLife *u) { (void)c; (void)u; push('S'); }
static void *owner_register(void *c, AoeUnitLife *u, uint8_t a, uint8_t d)
{ (void)c; (void)u; (void)a; (void)d; push('R'); return &node_token; }
static void owner_unregister(void *c, AoeUnitLife *u, uint8_t a, uint8_t d, void *n)
{ (void)c; (void)u; (void)a; (void)d; g.unregister_node_ok = n == &node_token; push('U'); }
static int32_t rand_draw(void *c) { (void)c; return 1234; }
static void srand_seed(void *c, int32_t s) { (void)c; g.seed = s; }
static void sound(void *c, AoeUnitLife *u) { (void)c; (void)u; g.sound++; }
static void sprites_stop(void *c, AoeUnitLife *u) { (void)c; (void)u; push('s'); }
static void tile_drop(void *c, AoeUnitLife *u) { (void)c; (void)u; push('t'); }
static void outcome_drop(void *c, AoeUnitLife *u) { (void)c; (void)u; push('o'); }
static void footprint(void *c, AoeUnitLife *u) { (void)c; (void)u; push('f'); }
static void *ai_make(void *c, AoeUnitLife *u) { (void)c; (void)u; return &ai_token; }
static void ai_free(void *c, void *ai) { (void)c; assert(ai == &ai_token); push('a'); }
static void *list_make(void *c, AoeUnitLife *u) { (void)c; (void)u; return &list_token; }
static void list_free(void *c, void *l) { (void)c; (void)l; push('l'); }
static void list_clear(void *c, void *l) { (void)c; (void)l; push('c'); }
static void *watch_add(void *c, AoeUnitLife *u, int32_t tx, int32_t ty)
{ (void)c; (void)u; g.watch_tx = tx; g.watch_ty = ty; return &watch_token; }
static void watch_remove(void *c, AoeUnitLife *u) { (void)c; (void)u; push('w'); }
static void default_task(void *c, AoeUnitLife *u, int16_t t) { (void)c; (void)u; g.task = t; }
static void template_free(void *c, const AoeUnitLifeTemplate *t) { (void)c; (void)t; g.freed_template++; }
static void pairs_free(void *c, void *p) { (void)c; (void)p; push('p'); }
static void queue_free(void *c, void *q) { (void)c; (void)q; push('q'); }
static void root_cleanup(void *c, AoeUnitLife *u) { (void)c; (void)u; push('C'); }

static AoeUnitLifeOps ops(void)
{
    AoeUnitLifeOps o;
    memset(&o, 0, sizeof(o));
    o.set_graphic = set_graphic; o.place = place; o.storage_reset = storage_reset;
    o.owner_register = owner_register; o.owner_unregister = owner_unregister;
    o.rand_draw = rand_draw; o.srand_seed = srand_seed; o.play_selection_sound = sound;
    o.sprites_stop = sprites_stop; o.tile_drop = tile_drop; o.outcome_drop = outcome_drop;
    o.footprint_lift = footprint; o.ai_make = ai_make; o.ai_free = ai_free;
    o.action_list_make = list_make; o.action_list_free = list_free;
    o.action_list_clear = list_clear; o.watch_add = watch_add; o.watch_remove = watch_remove;
    o.default_task = default_task; o.template_free = template_free;
    o.pairs_free = pairs_free; o.queue_free = queue_free; o.root_cleanup = root_cleanup;
    return o;
}

static void test_root_init_and_destroy(void)
{
    AoeUnitLifeOps o = ops();
    AoeUnitLifeTemplate t = {0};
    AoeUnitLifeOwner owner = {0};
    AoeUnitLife u;
    t.hit_points = 25; t.kind = 0x19; t.has_selection_sound = 1; t.footprint_radius = 1.0f;
    owner.player_id = 2; owner.viewed_player = 2;
    memset(&g, 0, sizeof(g));
    aoe_unit_life_load_defaults(&u);
    assert(u.source_id_78 == -1 && u.source_id_7c == -1 && u.range_80 == 5.0f);
    aoe_unit_life_init_root(&u, &o, &t, &owner, 3.5f, 4.5f, 1.0f);
    assert(strcmp(g.seq, "SGPR") == 0);
    assert(u.hit_points == 25.0f && u.alive == 1 && u.doppelganger == 1);
    assert(u.state == 2 && u.x == 3.5f && u.y == 4.5f && u.z == 1.0f);
    assert(u.owner_node == &node_token && g.sound == 1 && g.seed == 1234);

    /* Not the viewed player: no sound. Zero hp: not alive. */
    owner.viewed_player = 1; t.hit_points = 0; t.kind = 'F';
    memset(&g, 0, sizeof(g));
    aoe_unit_life_init_root(&u, &o, &t, &owner, 0, 0, 0);
    assert(g.sound == 0 && u.alive == 0 && u.doppelganger == 0);

    /* Destroy: state 2 drops from tile and outcomes; footprint lifted. */
    u.ai = &ai_token;
    memset(&g, 0, sizeof(g));
    aoe_unit_life_destroy_root(&u, &o);
    assert(strcmp(g.seq, "stoUfa") == 0);
    assert(g.unregister_node_ok && u.owner == NULL && u.ai == NULL);
    /* state 7 after the owner is gone: only the sprite stop. */
    u.state = 7;
    memset(&g, 0, sizeof(g));
    aoe_unit_life_destroy_root(&u, &o);
    assert(strcmp(g.seq, "sf") == 0);
}

static void test_type70_counters(void)
{
    AoeUnitLifeOps o = ops();
    float res[64] = {0};
    AoeUnitLifeTemplate villager = {0}, fish = {0}, ram = {0}, priv = {0};
    const AoeUnitLifeTemplate *templates[0x100] = {0};
    AoeUnitLifeOwner owner = {0};
    AoeUnitLife u;
    owner.resources = res; owner.resource_count = 64;
    owner.templates = templates; owner.template_count = 0x100;
    villager.class_code = 4;
    fish.class_code = 0x12; fish.kind = 'F';
    ram.class_code = 0x0d; ram.id = 0x4f;
    memset(&u, 0, sizeof(u));
    u.owner = &owner; u.tmpl = &villager;
    aoe_unit_life_init_type70(&u, &o);
    assert(u.class_code == 0x46 && u.gaia_owned == 1 && u.ai == &ai_token);
    assert(res[AOE_UNIT_LIFE_RES_POP_A] == 1.0f && res[AOE_UNIT_LIFE_RES_POP_B] == 0.0f);

    owner.player_id = 1;
    memset(&u, 0, sizeof(u));
    u.owner = &owner; u.tmpl = &fish;
    aoe_unit_life_init_type70(&u, &o);
    assert(u.gaia_owned == 0 && u.carried_amount == 100.0f && u.carried_type == 0x22);
    assert(res[AOE_UNIT_LIFE_RES_POP_B] == 0.0f); /* class 0x12 'F' does not count */

    memset(&u, 0, sizeof(u));
    u.owner = &owner; u.tmpl = &ram;
    aoe_unit_life_init_type70(&u, &o);
    assert(res[AOE_UNIT_LIFE_RES_POP_B] == 1.0f);

    /* Private template swapped for the shared one before the decrement. */
    priv = villager; priv.id = 0x4f; priv.class_code = 0x0d;
    templates[0x4f] = &villager;
    u.tmpl = &priv; u.private_template = 1;
    memset(&g, 0, sizeof(g));
    aoe_unit_life_destroy_type70(&u, &o);
    assert(g.freed_template == 1 && u.tmpl == &villager);
    assert(res[AOE_UNIT_LIFE_RES_POP_A] == 0.0f && res[AOE_UNIT_LIFE_RES_POP_B] == 1.0f);
}

static void test_type80(void)
{
    AoeUnitLifeOps o = ops();
    AoeUnitLifeTemplate t = {0};
    int32_t counter = 3;
    float res[64] = {0};
    AoeUnitLifeOwner owner = {0};
    AoeUnitLife u;
    int pairs;
    owner.status4_counter = &counter; owner.resources = res; owner.resource_count = 64;
    t.id = 0x114; t.default_task = 77; t.class_code = 3;
    memset(&u, 0, sizeof(u));
    u.tmpl = &t; u.owner = &owner; u.x = 7.9f; u.y = 2.2f;
    memset(&g, 0, sizeof(g));
    aoe_unit_life_construct_type80(&u, &o, -1, 0);
    assert(u.status == -1 && u.flag_1c8 == 1 && u.completion_value == -1);
    assert(g.task == 77 && u.watch == &watch_token && g.watch_tx == 7 && g.watch_ty == 2);

    /* Cleanup resets pair state and releases the status-4 count. */
    u.pairs = &pairs; u.pair_count = 2; u.flag_1c8 = 0; u.status = 4; u.queue = &list_token;
    memset(&g, 0, sizeof(g));
    aoe_unit_life_cleanup_type80(&u, &o);
    assert(strcmp(g.seq, "cpC") == 0);
    assert(u.pairs == NULL && u.pair_count == 0 && u.flag_1c8 == 1 && u.status == -1 && counter == 2);

    /* Destructor: no pair-count reset, but the status-4 count is released. */
    u.pairs = &pairs; u.pair_count = 2; u.status = 4; u.owner = &owner; u.state = 9;
    memset(&g, 0, sizeof(g));
    aoe_unit_life_destroy_type80(&u, &o);
    assert(strncmp(g.seq, "wqp", 3) == 0 && counter == 1 && u.pair_count == 2);

    /* 0x004adf50: queue reset; only status 4 releases the count. */
    u.status = 4; u.pair_cap = 5; u.flag_1d0 = 1; u.queue = &list_token; u.owner = &owner;
    memset(&g, 0, sizeof(g));
    aoe_unit_life_reset_queue_type80(&u, &o);
    assert(strcmp(g.seq, "c") == 0 && u.status == -1 && counter == 0 &&
           u.pair_cap == 0 && u.flag_1d0 == 2 && u.pair_count == 2);
    u.status = 2;
    aoe_unit_life_reset_queue_type80(&u, &o);
    assert(u.status == -1 && counter == 0 && u.flag_1d0 == 3);
}

static float refunds[4];
static int refund_n;
static void relink(void *c, void *l) { (void)c; (void)l; push('r'); }
static void fixup(void *c, AoeUnitLife *u) { (void)c; (void)u; push('F'); }
static void sel_remove(void *c, AoeUnitLife *u) { (void)c; (void)u; push('x'); }
static void removed(void *c, AoeUnitLife *u) { (void)c; (void)u; push('D'); }
static void add_attr(void *c, AoeUnitLife *u, int16_t r, float a)
{ (void)c; (void)u; refunds[refund_n++] = a + 1000.0f * (float)r; }

static void test_type80_post_load_and_remove(void)
{
    AoeUnitLifeOps o = ops();
    AoeUnitLifeTemplate t;
    AoeUnitLife u;
    int pairs_token;
    o.action_list_relink = relink; o.post_load_fixup = fixup;
    o.selection_remove = sel_remove; o.remove_finished = removed;
    o.owner_add_attribute = add_attr;
    memset(&u, 0, sizeof(u)); memset(&t, 0, sizeof(t));
    /* 0x004acf60: base fixup, action list, then the queue. */
    u.action_list = &list_token; u.queue = &pairs_token;
    memset(&g, 0, sizeof(g));
    aoe_unit_life_post_load_type80(&u, &o);
    assert(strcmp(g.seq, "Frr") == 0);

    /* 0x004ad6d0: half the unbuilt share refunded per enabled cost. */
    t.build_time = 40;
    t.build_cost[0].resource = 0; t.build_cost[0].amount = 100; t.build_cost[0].enabled = 1;
    t.build_cost[1].resource = 2; t.build_cost[1].amount = 50; t.build_cost[1].enabled = 0;
    t.build_cost[2].resource = -1; t.build_cost[2].amount = 10; t.build_cost[2].enabled = 1;
    u.tmpl = &t; u.state = 1; u.hit_points = 5.0f; u.build_progress = 10.0f;
    u.selected_bits = 1; refund_n = 0;
    memset(&g, 0, sizeof(g));
    aoe_unit_life_remove_type80(&u, &o);
    assert(strcmp(g.seq, "x") == 0 && refund_n == 1 && refunds[0] == 37.5f &&
           u.hit_points == 0.0f);
    /* Dead (hp 0) still calls with a zero amount; state 2 defers. */
    refund_n = 0; u.selected_bits = 0;
    aoe_unit_life_remove_type80(&u, &o);
    assert(refund_n == 1 && refunds[0] == 0.0f);
    u.state = 2; refund_n = 0; memset(&g, 0, sizeof(g));
    aoe_unit_life_remove_type80(&u, &o);
    assert(strcmp(g.seq, "D") == 0 && refund_n == 0);
}

static void test_type40_type20(void)
{
    AoeUnitLifeOps o = ops();
    AoeUnitLife u;
    memset(&u, 0, sizeof(u));
    aoe_unit_life_init_type40(&u, &o);
    assert(u.class_code == 0x28 && u.action_list == &list_token);
    memset(&g, 0, sizeof(g));
    aoe_unit_life_cleanup_type60(&u, &o);
    assert(strcmp(g.seq, "cC") == 0);
    memset(&g, 0, sizeof(g));
    aoe_unit_life_destroy_type40(&u, &o);
    assert(g.seq[0] == 'l' && u.action_list == NULL);
}

int main(void)
{
    test_root_init_and_destroy();
    test_type70_counters();
    test_type80();
    test_type40_type20();
    test_type80_post_load_and_remove();
    return 0;
}
