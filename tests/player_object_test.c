#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "player_object.h"

static int members, destroyed, unit_calls, refreshes, releases;
static int view_token, cond_token;

static int create_members(void *c, AoePlayerObjectRor *p) {
    (void)c;
    (void)p;
    members++;
    return 1;
}
static void destroy_members(void *c, AoePlayerObjectRor *p) {
    (void)c;
    (void)p;
    destroyed++;
}
static void unit_68(void *c, AoeSelectableUnitRor *u) {
    (void)c;
    u->state = 68;
    unit_calls++;
}
static void view_refresh(void *c, void *v) {
    (void)c;
    assert(v == &view_token);
    refreshes++;
}
static void view_release(void *c, void *v) {
    (void)c;
    assert(v == &view_token);
    releases++;
}
static void *view_create(void *c, AoePlayerObjectRor *p) {
    (void)c;
    (void)p;
    return &view_token;
}
static void *cond_create(void *c, AoePlayerObjectRor *p, AoeOutcomeStream *in,
                         const int32_t *map, int32_t flag) {
    (void)c;
    (void)p;
    (void)in;
    (void)map;
    assert(flag == 1);
    return &cond_token;
}

static int lists, clones, deleted, cond_new, cond_released, registered;
static int16_t registered_id;
static int tmpl_a, tmpl_b, clone_token;

static void create_lists(void *c, AoePlayerObjectRor *p) {
    (void)c;
    (void)p;
    lists++;
}
static void destroy_lists(void *c, AoePlayerObjectRor *p) {
    (void)c;
    (void)p;
    lists--;
}
static void *clone_template(void *c, void *src) {
    (void)c;
    assert(src == &tmpl_a || src == &tmpl_b);
    clones++;
    return &clone_token;
}
static void delete_template(void *c, void *t) {
    (void)c;
    assert(t == &clone_token);
    deleted++;
}
static void *conditions_new(void *c, AoePlayerObjectRor *p) {
    (void)c;
    (void)p;
    cond_new++;
    return &cond_token;
}
static void conditions_release(void *c, void *t) {
    (void)c;
    assert(t == &cond_token);
    cond_released++;
}
static void register_player(void *c, int16_t id, AoePlayerObjectRor *p) {
    (void)c;
    (void)p;
    registered++;
    registered_id = id;
}

static const AoePlayerObjectOps ops = {
    .create_members = create_members,
    .create_lists = create_lists,
    .destroy_lists = destroy_lists,
    .destroy_members = destroy_members,
    .clone_template = clone_template,
    .delete_template = delete_template,
    .conditions_new = conditions_new,
    .conditions_release = conditions_release,
    .register_player = register_player,
    .unit_68 = unit_68,
    .view_refresh = view_refresh,
    .view_release = view_release,
    .view_create = view_create,
    .conditions_create = cond_create};

static void test_init(void) {
    AoePlayerWorldRor world = {3, NULL, 0, 71, 64, 4.01f, 99, 1};
    void *const templates[3] = {&tmpl_a, NULL, &tmpl_b};
    float res[50];
    AoePlayerSourceRor source = {3, templates, 50, res, 7, 12};
    AoePlayerObjectRor p;
    int i;

    for (i = 0; i < 50; ++i) res[i] = (float)i;
    memset(&p, 0x5a, sizeof(p));
    p.selection.units = NULL;
    p.selection.unit_count = 0;
    assert(aoe_player_object_init_ror(&p, &world, &source, 2, "Bob", 15, 1,
                                      &ops) == 1);
    assert(p.field_04 == 0 && p.field_08 == 99 && p.field_0c == 10);
    assert(p.type == 0 && p.player_id == 2 && p.field_109 == 15);
    assert(strcmp(p.name, "Bob") == 0);
    assert(p.stances[0] == 1 && p.stances[1] == 1 && p.stances[2] == 1);
    assert(p.relations[0] == 0 && p.relations[1] == 3 &&
           p.relations[2] == 1 && p.relations[3] == -1 &&
           p.relations[8] == -1);
    assert(p.template_count == 3 && clones == 2 && p.templates[1] == NULL);
    assert(lists == 1 && cond_new == 1 && p.conditions == &cond_token &&
           p.view == &view_token);
    assert(p.resource_count == 50 && p.resources[3] == 3.0f &&
           p.resources[46] == 0.25f);
    assert(p.field_54 == 7 && p.view_x == 35.0f && p.view_y == 32.0f &&
           p.view_tile_x == 35 && p.view_tile_y == 32);
    assert(p.state == 0 && p.resigned == 0 && p.color == NULL);
    assert(p.field_110 == 12 && registered == 1 && registered_id == 12);
    assert(p.selection.count == 0 && p.selection.field_124[6] == -1);

    destroyed = 0;
    aoe_player_object_destroy_ror(&p, &ops);
    assert(lists == 0 && cond_released == 1 && deleted == 2 &&
           destroyed == 1 && !p.templates && !p.view && !p.conditions);

    /* Not full: no lists, conditions or view; no registration. */
    world.game_ad9 = 0;
    source.field_2a = -1;
    memset(&p, 0, sizeof(p));
    assert(aoe_player_object_init_ror(&p, &world, &source, 1, NULL, 15, 0,
                                      &ops) == 1);
    assert(!p.name && !p.view && !p.conditions && lists == 0 &&
           p.resources[46] == 46.0f && registered == 1);
    aoe_player_object_destroy_ror(&p, &ops);
}

static void test_round_trip(void) {
    AoePlayerColorRor c0 = {0}, c1 = {1}, c2 = {2};
    const AoePlayerColorRor *colors[3] = {&c0, &c1, &c2};
    AoePlayerWorldRor world = {4, colors, 3, 72, 64, 4.01f, 1234};
    AoePlayerObjectRor a, b;
    uint8_t stances[4] = {1, 0, 3, 3};
    float res[3] = {200.0f, 150.5f, -1.0f};
    uint8_t buf[256];
    AoeOutcomeStream out = {buf, sizeof(buf), 0, 0};
    AoeOutcomeStream in;
    int i;

    memset(&a, 0, sizeof(a));
    a.world = &world;
    a.type = 2;
    a.player_id = 1;
    a.stances = stances;
    a.stance_count = 4;
    for (i = 0; i < 9; ++i) a.relations[i] = i;
    a.field_104 = 77;
    a.allied_victory = 1;
    a.name = "Alpha";
    a.resource_count = 3;
    a.resources = res;
    a.view_x = 10.5f;
    a.view_y = 11.25f;
    a.view_tile_x = 10;
    a.view_tile_y = 11;
    a.field_54 = 5;
    a.field_109 = 15;
    a.state = 1;
    a.resigned = 0;
    a.color = &c2;
    a.field_08 = 42;
    a.field_0c = 43;
    assert(aoe_player_object_save_ror(&a, &out));
    assert(buf[0] == 2 && buf[1] == 1 && buf[3] == 3);
    /* type, 4 stances, 36 relations, +0x104, +0x108, name 2+6, count 2,
       12 floats, 4+4+2+2, 5 bytes, 4+4 */
    assert(out.pos == 1 + 4 + 36 + 4 + 1 + 8 + 2 + 12 + 12 + 5 + 8);

    memset(&b, 0, sizeof(b));
    in = (AoeOutcomeStream){buf + 1, out.pos - 1, 0, 0};
    members = 0;
    assert(aoe_player_object_load_ror(&b, &world, 1, &in, &ops) == 1);
    assert(members == 1 && in.pos == out.pos - 1);
    assert(b.player_id == 1 && b.selection.count == 0 &&
           b.selection.field_124[0] == -1);
    assert(memcmp(b.stances, stances, 4) == 0 && b.relations[8] == 8);
    assert(b.field_104 == 77 && b.allied_victory == 1);
    assert(strcmp(b.name, "Alpha") == 0 && b.resource_count == 3 &&
           b.resources[1] == 150.5f);
    assert(b.view_x == 10.5f && b.view_tile_y == 11 && b.field_54 == 5 &&
           b.field_109 == 15 && b.state == 1 && b.color == &c2);
    assert(b.field_08 == 42 && b.field_0c == 43 && b.field_04 == 0);
    destroyed = 0;
    aoe_player_object_delete_ror(&b, &ops);
    assert(destroyed == 1 && !b.name && !b.stances && !b.resources);

    /* Older saves have no +0x104. */
    world.save_version = 4.0f;
    memset(&b, 0, sizeof(b));
    {
        uint8_t old[256];
        size_t n = out.pos - 1;
        memcpy(old, buf + 1, 45);          /* stances + relations + ... */
        memcpy(old + 40, buf + 1 + 44, n - 44); /* drop the +0x104 dword */
        in = (AoeOutcomeStream){old, n - 4, 0, 0};
        assert(aoe_player_object_load_ror(&b, &world, 1, &in, &ops) == 1);
        assert(b.field_104 == 0 && strcmp(b.name, "Alpha") == 0 &&
               b.field_0c == 43 && in.pos == n - 4);
        aoe_player_object_destroy_ror(&b, &ops);
    }
    /* A short stream fails and releases what was allocated. */
    world.save_version = 4.01f;
    memset(&b, 0, sizeof(b));
    in = (AoeOutcomeStream){buf + 1, 60, 0, 0};
    assert(aoe_player_object_load_ror(&b, &world, 1, &in, &ops) == 0);
    assert(!b.name && !b.stances && !b.resources);
}

static void test_view_and_color(void) {
    AoePlayerColorRor c0 = {9};
    const AoePlayerColorRor *colors[1] = {&c0};
    AoePlayerWorldRor world = {2, colors, 1, 72, 64, 4.01f, 0};
    AoePlayerObjectRor p;

    memset(&p, 0, sizeof(p));
    p.world = &world;
    aoe_player_set_view_ror(&p, -0.5f, 100.0f);
    assert(p.view_x == 0.0f && p.view_y == 63.0f);
    aoe_player_set_view_ror(&p, 71.0f, 63.5f);
    assert(p.view_x == 71.0f && p.view_y == 63.0f);
    aoe_player_set_view_ror(&p, NAN, 2.0f);
    assert(p.view_x == 0.0f && p.view_y == 2.0f);
    aoe_player_set_view_tile_ror(&p, 3, -4);
    assert(p.view_tile_x == 3 && p.view_tile_y == -4);
    aoe_player_set_color_ror(&p, 0x100);
    assert(p.color == &c0);
    aoe_player_set_color_ror(&p, 1);
    assert(p.color == NULL);
}

static void test_members(void) {
    AoePlayerObjectRor p;
    AoeSelectableUnitRor u[2];
    AoeSelectableUnitRor *list[2] = {&u[0], &u[1]};
    uint8_t byte = 0;
    AoeOutcomeStream in = {&byte, 1, 0, 0};
    int32_t map[1] = {0};

    releases = refreshes = 0;
    memset(&p, 0, sizeof(p));
    memset(u, 0, sizeof(u));
    p.selection.units = list;
    p.selection.unit_count = 2;
    aoe_player_units_68_ror(&p, &ops);
    assert(unit_calls == 2 && u[1].state == 68);

    aoe_player_view_refresh_ror(&p, &ops); /* no view: skipped */
    assert(refreshes == 0);
    aoe_player_view_reset_ror(&p, &ops);
    assert(p.view == &view_token && releases == 0);
    aoe_player_view_reset_ror(&p, &ops);
    assert(releases == 1);
    aoe_player_view_refresh_ror(&p, &ops);
    assert(refreshes == 1);
    aoe_player_conditions_create_ror(&p, &in, map, 1, &ops);
    assert(p.conditions == &cond_token);
}

int main(void) {
    test_init();
    test_round_trip();
    test_view_and_color();
    test_members();
    return 0;
}
