/* Patched RoR 1.0a object list 0x00450ef0..0x004516c0. */
#include "object_list.h"

#include <assert.h>
#include <string.h>

#include "aoe_stream.h"

typedef struct { int32_t id; uint8_t flag, key; int16_t order; int dirty, updates, destroyed; } Obj;
typedef struct { AoeObjectListRor *list; int pumps; Obj made[4]; int n; int32_t seen[8]; int seen_n; } Ctx;

static int32_t id(void *c, const void *o) { (void)c; return ((const Obj *)o)->id; }
static uint8_t flag(void *c, const void *o) { (void)c; return ((const Obj *)o)->flag; }
static uint8_t key(void *c, const void *o) { (void)c; return ((const Obj *)o)->key; }
static int16_t order(void *c, const void *o) { (void)c; return ((const Obj *)o)->order; }
static int needs(void *c, void *o) { (void)c; return ((Obj *)o)->dirty; }
static void update(void *c, void *o) { (void)c; ((Obj *)o)->updates++; }
static void pump(void *c) { ((Ctx *)c)->pumps++; }
static void destroy(void *c, void *o) {
    Ctx *x = c;
    ((Obj *)o)->destroyed++;
    aoe_object_list_remove_ror(x->list, o, NULL);
}
static void save(void *c, void *o, AoeOutcomeStream *s) {
    (void)c;
    aoe_stream_put_u8(s, 7);
    aoe_stream_put_i32(s, ((Obj *)o)->id);
}
static void load(void *c, AoeObjectListRor *l, uint8_t type, AoeOutcomeStream *s, int32_t arg) {
    Ctx *x = c;
    Obj *o = &x->made[x->n++];
    assert(type == 7 && arg == 5);
    o->id = aoe_stream_i32(s);
    aoe_object_list_append_ror(l, o);
}
static uint8_t dstate(void *c, const void *o) { (void)c; return (uint8_t)((const Obj *)o)->order; }
static uint8_t dkind(void *c, const void *o) { (void)c; return ((const Obj *)o)->key; }
static int32_t ocol(void *c, const void *o) { (void)c; return ((const Obj *)o)->id; }
static int32_t own(void *c, const void *o) { (void)c; return -((const Obj *)o)->id; }
static int sees(void *c, const void *o) { (void)c; return ((const Obj *)o)->dirty; }
static void ddraw(void *c, void *o, int32_t a, int32_t b, int32_t cc, int32_t col) {
    Ctx *x = c; (void)o; (void)a; (void)b; (void)cc; x->seen[x->seen_n++] = col;
}
static void record(void *c, void *o) { Ctx *x = c; x->seen[x->seen_n++] = ((Obj *)o)->id; }

static void test_find_any(void) {
    /* 0x004a94d0: owner first, then other players in index order */
    AoeObjectListOpsRor ops = {NULL, NULL, id, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL};
    AoeObjectListRor l[9];
    const AoeObjectListRor *lists[9];
    Obj a = {.id = 5}, b = {.id = 5}, c = {.id = 9}, d = {.id = 7};
    int i;
    for (i = 0; i < 9; ++i) { aoe_object_list_init_ror(&l[i]); lists[i] = &l[i]; }
    assert(aoe_object_list_append_ror(&l[1], &a));  /* player 0 list 2 */
    assert(aoe_object_list_append_ror(&l[8], &b));  /* player 2 list 3 */
    assert(aoe_object_list_append_ror(&l[5], &c));  /* player 1 list 3 */
    assert(aoe_object_list_append_ror(&l[6], &d));  /* player 2 list 1 */
    assert(aoe_object_lists_find_any_ror(lists, 3, 2, &ops, 5) == &b);
    assert(aoe_object_lists_find_any_ror(lists, 3, 1, &ops, 5) == &a);
    assert(aoe_object_lists_find_any_ror(lists, 3, 0, &ops, 9) == &c);
    assert(aoe_object_lists_find_any_ror(lists, 3, 0, &ops, 7) == &d);
    assert(aoe_object_lists_find_any_ror(lists, 3, 0, &ops, 8) == NULL);
    assert(aoe_object_lists_find_any_ror(lists, 3, 0, &ops, -1) == NULL);
    for (i = 0; i < 9; ++i) {
        while (l[i].head) aoe_object_list_remove_ror(&l[i], l[i].head->object, NULL);
    }
}

int main(void) {
    test_find_any();
    static Ctx ctx;
    AoeObjectListOpsRor ops = {&ctx, destroy, id, flag, key, order, needs, update, pump, 1, save, load};
    AoeObjectListRor l, back;
    Obj objs[12];
    uint8_t buf[256];
    AoeOutcomeStream s = {0};
    memset(objs, 0, sizeof objs);
    aoe_object_list_init_ror(&l);
    ctx.list = &l;
    for (int i = 0; i < 4; i++) {
        objs[i].id = 10 + i;
        assert(aoe_object_list_append_ror(&l, &objs[i]));
    }
    assert(l.count == 4 && l.head->object == &objs[3]);
    assert(aoe_object_list_find_ror(&l, &ops, 11) == &objs[1]);
    assert(!aoe_object_list_find_ror(&l, &ops, 99));
    aoe_object_list_remove_ror(&l, &objs[2], NULL);
    assert(l.count == 3 && !aoe_object_list_find_ror(&l, &ops, 12));
    aoe_object_list_remove_ror(&l, &objs[2], NULL);
    assert(l.count == 3);
    aoe_object_list_reverse_ror(&l);
    assert(l.head->object == &objs[0] && !l.head->prev && l.head->next->prev == l.head);

    /* Sorted: key descending, then order ascending, else list order. */
    objs[0].key = 1; objs[0].order = 5;
    objs[1].key = 3; objs[1].order = 9; objs[1].flag = 1;
    objs[3].key = 1; objs[3].order = 2;
    ctx.seen_n = 0;
    aoe_object_list_each_sorted_ror(&l, &ops, 0, record, &ctx);
    assert(ctx.seen_n == 3 && ctx.seen[0] == 11 && ctx.seen[1] == 13 && ctx.seen[2] == 10);
    ctx.seen_n = 0;
    aoe_object_list_each_ror(&l, &ops, 1, record, &ctx);
    assert(ctx.seen_n == 1 && ctx.seen[0] == 11);

    /* Update and the pump after ten objects. */
    for (int i = 4; i < 12; i++) aoe_object_list_append_ror(&l, &objs[i]);
    objs[1].dirty = 1;
    aoe_object_list_update_ror(&l, &ops);
    assert(objs[1].updates == 1 && objs[0].updates == 0 && ctx.pumps == 1);

    /* Save and load keep the order. */
    s.data = buf; s.size = sizeof buf;
    aoe_object_list_destroy_ror(&l, &ops);
    assert(!l.head && objs[1].destroyed == 1);
    aoe_object_list_init_ror(&l);
    aoe_object_list_append_ror(&l, &objs[0]);
    aoe_object_list_append_ror(&l, &objs[1]);
    aoe_object_list_save_ror(&l, &ops, &s);
    assert(s.pos == 11 && buf[10] == 0);
    s.size = s.pos; s.pos = 0;
    aoe_object_list_init_ror(&back);
    aoe_object_list_load_ror(&back, &ops, &s, 5);
    assert(back.count == 2 && ((Obj *)back.head->object)->id == 11);
    ctx.list = &back;
    aoe_object_list_clear_ror(&back, &ops);
    assert(!back.head && back.count == 0);
    ctx.list = &l;
    aoe_object_list_destroy_ror(&l, &ops);
    {
        /* 0x00451080 the fog-filtered draw. */
        Obj d[4] = {{1, 0, 0, 0, 0, 0, 0}, {2, 2, 0, 3, 0, 0, 0}, {3, 3, 0, 0, 1, 0, 0},
                    {4, 9, 0, 0, 0, 0, 0}};
        AoeObjectListDrawOpsRor dops = {&ctx, flag, dstate, dkind, ocol, own, sees, ddraw};
        AoeObjectListRor dl;
        aoe_object_list_init_ror(&dl);
        for (int i = 0; i < 4; i++) aoe_object_list_append_ror(&dl, &d[i]);
        ctx.seen_n = 0;
        aoe_object_list_draw_ror(&dl, &dops, 0, 0, 0, 1);
        assert(ctx.seen_n == 2 && ctx.seen[0] == 4 && ctx.seen[1] == -3);
        ctx.seen_n = 0;
        aoe_object_list_draw_ror(&dl, &dops, 0, 0, 0, 0);
        assert(ctx.seen_n == 3);
        aoe_object_list_free_nodes_ror(dl.head);
    }
    return 0;
}
