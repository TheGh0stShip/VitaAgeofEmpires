/* The view's area (drag-box) selection, patched RoR 1.0a 0x005114d0,
   against fake picks, objects and a 25-slot selection (0x0045dc10). */
#include "main_view.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define OBJECTS 40
#define SLOTS 25

typedef struct {
    int present;
    AoeMainViewAreaObjectRor info;
    int visible;
    uint8_t fog;
    int32_t priority;
} FakeObject;

typedef struct {
    FakeObject objects[OBJECTS];
    AoeViewPickedRor box[AOE_VIEW_PICKED_MAX_ROR];
    int32_t box_n;
    AoeViewPickedRor point[AOE_VIEW_PICKED_MAX_ROR];
    int32_t point_n;
    /* Arguments seen. */
    int box_calls, point_calls;
    int32_t box_args[7];
    uint8_t box_min_kind;
    int32_t point_args[7];
    int32_t fog_x, fog_y, fog_calls, last_class;
    /* The viewed player's selection. */
    void *selection[SLOTS];
    int selected;
    int adds;
} Fake;

static Fake f;

static int32_t box_objects(void *c, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t lo,
                           int32_t hi, int32_t player, uint8_t min_kind,
                           AoeViewPickedRor out[AOE_VIEW_PICKED_MAX_ROR])
{
    (void)c;
    f.box_calls++;
    f.box_args[0] = x0; f.box_args[1] = y0; f.box_args[2] = x1; f.box_args[3] = y1;
    f.box_args[4] = lo; f.box_args[5] = hi; f.box_args[6] = player;
    f.box_min_kind = min_kind;
    memcpy(out, f.box, sizeof f.box);
    return f.box_n;
}

static int32_t point_objects(void *c, int32_t x, int32_t y, int32_t lo, int32_t hi, int32_t slack,
                             int32_t max_class, int positive_only,
                             AoeViewPickedRor out[AOE_VIEW_PICKED_MAX_ROR])
{
    (void)c;
    f.point_calls++;
    f.point_args[0] = x; f.point_args[1] = y; f.point_args[2] = lo; f.point_args[3] = hi;
    f.point_args[4] = slack; f.point_args[5] = max_class; f.point_args[6] = positive_only;
    memcpy(out, f.point, sizeof f.point);
    return f.point_n;
}

static void *object(void *c, int32_t id, AoeMainViewAreaObjectRor *info)
{
    (void)c;
    if (id < 0 || id >= OBJECTS || !f.objects[id].present)
        return NULL;
    *info = f.objects[id].info;
    return &f.objects[id];
}

static int visible(void *c, void *obj) { (void)c; return ((FakeObject *)obj)->visible; }

static uint8_t fog(void *c, int32_t x, int32_t y)
{
    int i;
    (void)c;
    f.fog_calls++;
    f.fog_x = x;
    f.fog_y = y;
    /* The fog of the object standing at (x, y). */
    for (i = 0; i < OBJECTS; i++)
        if (f.objects[i].present && (int32_t)f.objects[i].info.x == x &&
            (int32_t)f.objects[i].info.y == y)
            return f.objects[i].fog;
    return 0;
}

static int32_t priority(void *c, void *obj, int32_t hit_class)
{
    (void)c;
    f.last_class = hit_class;
    return ((FakeObject *)obj)->priority;
}

/* 0x0045dc10 with an empty-slot fill: 0 when all 25 slots are taken. */
static int select_add(void *c, void *obj)
{
    (void)c;
    f.adds++;
    if (f.selected >= SLOTS)
        return 0;
    f.selection[f.selected++] = obj;
    return 1;
}

static AoeMainViewAreaSelectOpsRor ops(void)
{
    AoeMainViewAreaSelectOpsRor o;
    memset(&o, 0, sizeof o);
    o.context = &f;
    o.player = 1;
    o.box_objects = box_objects;
    o.point_objects = point_objects;
    o.object = object;
    o.visible = visible;
    o.fog = fog;
    o.priority = priority;
    o.select_add = select_add;
    return o;
}

static void reset(void)
{
    int i;
    memset(&f, 0, sizeof f);
    for (i = 0; i < OBJECTS; i++) {
        f.objects[i].present = 1;
        f.objects[i].info.state = 2;
        f.objects[i].info.kind = 70;
        f.objects[i].info.x = (float)i + 0.75f;
        f.objects[i].info.y = 3.5f;
        f.objects[i].visible = 1;
        f.objects[i].fog = 0x0f;
        f.objects[i].priority = 1;
    }
}

static void box_add(int32_t id)
{
    f.box[f.box_n].id = id;
    f.box[f.box_n].count = 1;
    f.box[f.box_n].field_6 = 1;
    f.box_n++;
}

static void point_add(int32_t id, int16_t hit_class)
{
    f.point[f.point_n].id = id;
    f.point[f.point_n].count = hit_class;
    f.point_n++;
}

/* The boxed objects are added in the box pick's order; -1 IDs, missing
   objects and those in state 7 or above are skipped; the 25-slot
   selection stops the pass at the first refusal. */
static void test_box_order_and_limit(void)
{
    AoeMainViewAreaSelectOpsRor o = ops();
    int32_t i;

    reset();
    o.player = 3;
    box_add(5);
    box_add(-1);
    box_add(2);
    f.objects[7].present = 0;
    box_add(7);
    f.objects[9].info.state = 7;
    box_add(9);
    for (i = 10; i < 38; i++)
        box_add(i);
    assert(aoe_main_view_area_select_ror(&o, 4, 100, 50, 300, 90) == 1);
    assert(f.box_calls == 1 && f.point_calls == 0);
    assert(f.box_args[0] == 100 && f.box_args[1] == 50 && f.box_args[2] == 300 &&
           f.box_args[3] == 90);
    assert(f.box_args[4] == 10 && f.box_args[5] == 0x14 && f.box_args[6] == 3);
    assert(f.box_min_kind == 4);
    assert(f.selected == SLOTS);
    assert(f.selection[0] == &f.objects[5]);
    assert(f.selection[1] == &f.objects[2]);
    for (i = 2; i < SLOTS; i++)
        assert(f.selection[i] == &f.objects[10 + i - 2]);
    /* 25 accepted, then the refusal ends the pass. */
    assert(f.adds == SLOTS + 1);

    /* The low byte of the kind goes to the box pick. */
    reset();
    box_add(1);
    assert(aoe_main_view_area_select_ror(&o, 0x104, 0, 0, 10, 10) == 1);
    assert(f.box_min_kind == 4);
}

/* A kept (Shift) selection is not cleared: the boxed objects follow it,
   and a full one refuses the first and falls back to the scoring, whose
   winner is refused as well. */
static void test_kept_selection(void)
{
    AoeMainViewAreaSelectOpsRor o = ops();
    int i;

    reset();
    f.selection[0] = &f.objects[30];
    f.selected = 1;
    box_add(4);
    box_add(6);
    assert(aoe_main_view_area_select_ror(&o, 4, 0, 0, 20, 20) == 1);
    assert(f.selected == 3);
    assert(f.selection[0] == &f.objects[30] && f.selection[1] == &f.objects[4] &&
           f.selection[2] == &f.objects[6]);

    reset();
    for (i = 0; i < SLOTS; i++)
        f.selection[i] = &f.objects[i];
    f.selected = SLOTS;
    box_add(30);
    box_add(31);
    point_add(32, 3);
    assert(aoe_main_view_area_select_ror(&o, 4, 0, 0, 20, 20) == 0);
    assert(f.point_calls == 1);
    /* One refusal in the box pass, one for the scored winner. */
    assert(f.adds == 2);
    assert(f.selected == SLOTS);
}

/* An empty box (or one whose objects all fail) scores the objects under
   its centre: the first strictly highest priority, +5 for the viewed
   player's own, among the seen objects of kind 2 or more in a state
   below 7. */
static void test_fallback_scoring(void)
{
    AoeMainViewAreaSelectOpsRor o = ops();

    reset();
    /* The box lists one dying object only. */
    f.objects[1].info.state = 7;
    box_add(1);
    point_add(-3, 1);              /* negative ID */
    point_add(2, 2);  f.objects[2].priority = 50; f.objects[2].info.kind = 1; /* kind 1 */
    point_add(3, 2);  f.objects[3].priority = 50; f.objects[3].info.state = 8; /* gone */
    point_add(4, 3);  f.objects[4].priority = 50; f.objects[4].visible = 0;
                      f.objects[4].fog = 0x80;      /* explored, not seen */
    point_add(5, 4);  f.objects[5].priority = 9;    /* enemy, 9 */
    point_add(6, 3);  f.objects[6].priority = 4;    /* own: 4 + 5 = 9, not above */
    f.objects[6].info.owned = 1;
    point_add(7, 1);  f.objects[7].priority = 6;    /* own: 11 */
    f.objects[7].info.owned = 1; f.objects[7].visible = 0;   /* seen by the fog */
    point_add(8, 2);  f.objects[8].priority = 11;   /* ties 11: the first wins */
    assert(aoe_main_view_area_select_ror(&o, 4, 100, 50, 300, 90) == 1);
    assert(f.box_calls == 1 && f.point_calls == 1);
    /* w 200, h 40: centre (200, 70), slack min(w, h) 40. */
    assert(f.point_args[0] == 200 && f.point_args[1] == 70);
    assert(f.point_args[2] == 10 && f.point_args[3] == 0x14);
    assert(f.point_args[4] == 40 && f.point_args[5] == 4 && f.point_args[6] == 1);
    assert(f.selected == 1 && f.selection[0] == &f.objects[7]);
    assert(f.adds == 1);
    /* The last priority asked was object 8's with its hit class 2. */
    assert(f.last_class == 2);
    /* The fog was read at object 7's truncated position (7.75, 3.5). */
    assert(f.fog_x == 7 && f.fog_y == 3);
    assert(f.fog_calls == 2);

    /* A revealed map ignores the fog: object 4 (50) now wins. */
    reset();
    point_add(4, 3);  f.objects[4].priority = 50; f.objects[4].visible = 0;
                      f.objects[4].fog = 0;
    point_add(5, 4);  f.objects[5].priority = 9;
    o.reveal_all = 1;
    assert(aoe_main_view_area_select_ror(&o, 4, 10, 20, 210, 220) == 1);
    assert(f.selected == 1 && f.selection[0] == &f.objects[4]);
    assert(f.fog_calls == 0);
    /* 200 x 200: centre (110, 120), slack capped at 50. */
    assert(f.point_args[0] == 110 && f.point_args[1] == 120 && f.point_args[4] == 50);
    o.reveal_all = 0;

    /* Odd sizes truncate the half: 0..7 x 0..3 centre (3, 1), slack 3. */
    reset();
    point_add(5, 4);
    assert(aoe_main_view_area_select_ror(&o, 4, 0, 0, 7, 3) == 1);
    assert(f.point_args[0] == 3 && f.point_args[1] == 1 && f.point_args[4] == 3);
}

/* No positive score, or nothing under the centre: 0, nothing added. */
static void test_fallback_nothing(void)
{
    AoeMainViewAreaSelectOpsRor o = ops();

    reset();
    point_add(5, 4);  f.objects[5].priority = 0;
    point_add(6, 4);  f.objects[6].priority = -10;
    f.objects[6].info.owned = 1;   /* -5 */
    assert(aoe_main_view_area_select_ror(&o, 4, 0, 0, 40, 40) == 0);
    assert(f.adds == 0 && f.selected == 0);

    reset();
    assert(aoe_main_view_area_select_ror(&o, 4, 0, 0, 40, 40) == 0);
    assert(f.box_calls == 1 && f.point_calls == 1 && f.adds == 0);
}

int main(void)
{
    test_box_order_and_limit();
    test_kept_selection();
    test_fallback_scoring();
    test_fallback_nothing();
    puts("main_view_area_select_test: ok");
    return 0;
}
