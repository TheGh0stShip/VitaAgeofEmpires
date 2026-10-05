#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "player_body.h"

/* Opaque tokens: templates are ints holding their type, lists count their
   loads. */
static int templates[8], ntemplates;
static int view_tok, mem_tok, unit_tok, node_tok;
static int lists[3], nlists, list_loads[3], list_saves, finished[3];
static int u68, u6c, ub4, ub4_value, added, removed_id, global, lupd, tupd;
static void *last_list;
static int cond_flag = -1;

static void *load_template(void *c, uint8_t type, AoeOutcomeStream *in) {
    (void)c;
    assert(aoe_stream_u8(in) == 0xee); /* the template's own byte */
    templates[ntemplates] = type;
    return &templates[ntemplates++];
}
static void save_template(void *c, void *t, AoeOutcomeStream *out) {
    (void)c;
    aoe_stream_put_u8(out, (uint8_t)*(int *)t);
    aoe_stream_put_u8(out, 0xee);
}
static void *view_load(void *c, AoePlayerObjectRor *p, AoeOutcomeStream *in) {
    (void)c;
    (void)p;
    assert(aoe_stream_u8(in) == 0x77);
    return &view_tok;
}
static void view_save(void *c, void *v, AoeOutcomeStream *out) {
    (void)c;
    assert(v == &view_tok);
    aoe_stream_put_u8(out, 0x77);
}
static void *memory_load(void *c, AoePlayerObjectRor *p,
                         AoeOutcomeStream *in) {
    (void)c;
    (void)p;
    assert(aoe_stream_u8(in) == 0x66);
    return &mem_tok;
}
static void memory_save(void *c, void *m, AoeOutcomeStream *out) {
    (void)c;
    assert(m == &mem_tok);
    aoe_stream_put_u8(out, 0x66);
}
static void *list_new(void *c) {
    (void)c;
    assert(nlists < 3);
    return &lists[nlists++];
}
static void list_load(void *c, void *l, AoeOutcomeStream *in) {
    (void)c;
    assert(aoe_stream_u8(in) == 0x55);
    list_loads[(int *)l - lists]++;
}
static void list_save(void *c, void *l, AoeOutcomeStream *out) {
    (void)c;
    (void)l;
    list_saves++;
    aoe_stream_put_u8(out, 0x55);
}
static void list_finish(void *c, void *l) {
    (void)c;
    finished[(int *)l - lists]++;
}
static void global_update(void *c) {
    (void)c;
    global++;
}
static void list_update(void *c, void *l) {
    (void)c;
    assert(l == &lists[2]);
    lupd++;
}
static void table_update(void *c, void *t) {
    (void)c;
    (void)t;
    tupd++;
}
static void *list_add(void *c, void *l, void *u) {
    (void)c;
    assert(u == &unit_tok);
    last_list = l;
    return &node_tok;
}
static void list_remove(void *c, void *l, void *u, void *n) {
    (void)c;
    assert(u == &unit_tok && n == &node_tok);
    last_list = l;
}
static void world_added(void *c, void *u) {
    (void)c;
    (void)u;
    added++;
}
static void world_removed(void *c, int32_t id) {
    (void)c;
    removed_id = id;
}
static int32_t unit_id(void *c, void *u) {
    (void)c;
    (void)u;
    return 4321;
}
static void *create(void *c, void *t, AoePlayerObjectRor *p, float x,
                    float y, float z) {
    (void)c;
    (void)p;
    assert(*(int *)t == 30 && x == 1.0f && y == 2.0f && z == 3.0f);
    return &unit_tok;
}
static void unit_68(void *c, void *u) {
    (void)c;
    (void)u;
    u68++;
}
static void unit_6c(void *c, void *u) {
    (void)c;
    (void)u;
    u6c++;
}
static void unit_b4(void *c, void *u, int32_t v) {
    (void)c;
    (void)u;
    ub4++;
    ub4_value = v;
}
static void load_conditions(void *c, AoePlayerObjectRor *p,
                            AoeOutcomeStream *in, const int32_t *map,
                            int32_t flag) {
    (void)c;
    (void)p;
    (void)in;
    assert(map == NULL);
    cond_flag = flag;
}

static const AoePlayerBodyOps ops = {
    .load_template = load_template, .save_template = save_template,
    .view_load = view_load, .view_save = view_save,
    .memory_load = memory_load, .memory_save = memory_save,
    .list_new = list_new, .list_load = list_load, .list_save = list_save,
    .list_finish = list_finish, .global_update = global_update,
    .list_update = list_update, .table_update = table_update,
    .list_add = list_add, .list_remove = list_remove,
    .world_unit_added = world_added, .world_unit_removed = world_removed,
    .unit_id = unit_id, .create_from_template = create, .unit_68 = unit_68,
    .unit_6c = unit_6c, .unit_b4 = unit_b4,
    .load_conditions = load_conditions};

int main(void) {
    AoePlayerWorldRor world = {2, NULL, 0, 64, 64, 7.09f, 0, 0};
    AoePlayerObjectRor a, b;
    int t30 = 30, t60 = 60;
    void *table_a[4] = {&t30, NULL, &t60, NULL};
    uint8_t buf[128];
    AoeOutcomeStream out = {buf, sizeof(buf), 0, 0}, in;

    memset(&a, 0, sizeof(a));
    a.world = &world;
    a.template_count = 4;
    a.templates = table_a;
    a.view = &view_tok;
    a.memory = &mem_tok;
    assert(aoe_player_body_save_ror(&a, &out, &ops));
    assert(list_saves == 3);
    assert(out.pos == 2 + 16 + 4 + 1 + 1 + 3);

    memset(&b, 0, sizeof(b));
    b.world = &world;
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    assert(aoe_player_body_load_ror(&b, &in, &ops));
    assert(in.pos == out.pos && b.template_count == 4);
    assert(b.templates[0] && *(int *)b.templates[0] == 30);
    assert(b.templates[1] == NULL && *(int *)b.templates[2] == 60 &&
           b.templates[3] == NULL);
    assert(b.view == &view_tok && b.memory == &mem_tok);
    assert(b.lists[0] == &lists[0] && b.lists[2] == &lists[2]);
    assert(list_loads[0] == 1 && list_loads[1] == 1 && list_loads[2] == 1);

    free(b.templates);
    /* Before 7.09 the third list is not read. */
    world.save_version = 7.08f;
    nlists = 0;
    memset(list_loads, 0, sizeof(list_loads));
    in = (AoeOutcomeStream){buf, out.pos - 1, 0, 0};
    memset(&b, 0, sizeof(b));
    b.world = &world;
    assert(aoe_player_body_load_ror(&b, &in, &ops));
    assert(in.pos == out.pos - 1 && list_loads[2] == 0);

    /* Unknown template types leave the slot empty. */
    aoe_player_template_load_ror(&b, &in, 0, 11, &ops);
    assert(b.templates[0] == NULL);

    aoe_player_body_finish_load_ror(&b, &in, &ops);
    assert(cond_flag == 1 && finished[0] == 1 && finished[2] == 1);
    aoe_player_body_update_ror(&b, &ops);
    assert(global == 1 && lupd == 1 && tupd == 1);

    /* Creation from template 0 of `a` (type 30). */
    assert(aoe_player_create_unit_ror(&a, 1, 2, 3, 0, 7, 0, &ops) ==
           &unit_tok && u68 == 1);
    assert(aoe_player_create_unit_ror(&a, 1, 2, 3, 0, 3, 0, &ops) &&
           u6c == 1);
    assert(aoe_player_create_unit_ror(&a, 1, 2, 3, 0, 2, 9, &ops) &&
           ub4 == 1 && ub4_value == 9);
    assert(aoe_player_create_unit_ror(&a, 1, 2, 3, 1, 2, 9, &ops) == NULL);
    assert(aoe_player_create_unit_ror(&a, 1, 2, 3, 4, 2, 9, &ops) == NULL);

    a.lists[0] = &lists[0];
    a.lists[1] = &lists[1];
    a.lists[2] = &lists[2];
    assert(aoe_player_add_unit_ror(&a, &unit_tok, 1, 1, &ops) == &node_tok);
    assert(last_list == &lists[1] && added == 1);
    aoe_player_add_unit_ror(&a, &unit_tok, 0, 1, &ops);
    assert(last_list == &lists[2]);
    aoe_player_remove_unit_ror(&a, &unit_tok, 0, 0, &node_tok, &ops);
    assert(last_list == &lists[0] && removed_id == 4321);
    free(b.templates);
    return 0;
}
