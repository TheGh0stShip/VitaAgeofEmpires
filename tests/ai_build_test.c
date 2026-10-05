/* Patched RoR 1.0a AI module base 0x0040bfd0..0x0040c240 and build
   planning 0x004076b0..0x0040a730. */
#include "ai_build.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char lines[16][160];
    int count, opens, closes;
} Log;

static int log_open(void *c) { ++((Log *)c)->opens; return 1; }
static void log_close(void *c) { ++((Log *)c)->closes; }
static void log_write(void *c, const char *line) {
    Log *l = c;
    if (l->count < 16) {
        strncpy(l->lines[l->count], line, 159);
        l->lines[l->count][159] = '\0';
    }
    ++l->count;
}
static const char *log_time(void *c) { (void)c; return "Sun Oct  4"; }

static int objects(void *c, int32_t type, const char **name, float *rx,
                   float *ry) {
    (void)c;
    if (type != 109) return 0;
    *name = "House";
    *rx = 1.0f;
    *ry = 1.25f;
    return 1;
}

static int ror_updates, ror_hooks;
static int32_t ror_flag;
static void ror_hook(void *c, int32_t a) { (void)c; (void)a; ++ror_hooks; }
static void ror_update(void *c) { (void)c; ++ror_updates; }
static int32_t ror_value(void *c, int32_t id) { (void)c; return id == 0xb1 ? 109 : id == 0x72 ? 4 : 2; }
static int32_t mark_type = -1, mark_x, mark_y;
static void mark_fn(void *c, int32_t t, int32_t x, int32_t y, uint8_t s) {
    (void)c; (void)s; mark_type = t; mark_x = x; mark_y = y;
}
static int fit_fallbacks;
static int fit_fallback(void *c, const AoeAiPlanItemRor *i) { (void)c; (void)i; ++fit_fallbacks; return 1; }
static void cost_tech(void *c, int32_t t, int16_t p[6]) { (void)c; (void)t; p[0] = 0; p[1] = 50; }
static void cost_obj(void *c, int32_t t, int16_t p[6]) { (void)c; p[0] = 1; p[1] = (int16_t)t; p[2] = 2; p[3] = 5; }
static int fail_notes = -1, fail_mark_state = -1;
static float fail_box[4];
static int fail_alive(void *c, int32_t id) { (void)c; return id == 5; }
static int fail_listed(void *c, int32_t id) { (void)c; (void)id; return 1; }
static void fail_avoid(void *c, int32_t id, float a, float b, float d, float e) {
    (void)c; (void)id; fail_box[0] = a; fail_box[1] = b; fail_box[2] = d; fail_box[3] = e;
}
static void fail_note(void *c, float x, float y, int32_t f) { (void)c; (void)x; (void)y; fail_notes = f; }
static void fail_mark(void *c, int32_t t, int32_t x, int32_t y, uint8_t s) {
    (void)c; (void)t; (void)x; (void)y; fail_mark_state = s;
}
static int32_t aug_sn(void *c, int32_t i) { (void)c; return i == 0xb4 ? 1 : 0; }
static int32_t pick_sn(void *c, int32_t i) { (void)c; return i == 0x73 || i == 0x72 ? 10 : i == 0xae ? 5 : 0; }
static int32_t pick_extra(void *c, int32_t t) { (void)c; (void)t; return 0; }
static int pick_yes(void *c, int32_t t) { (void)c; (void)t; return 1; }
static int pick_afford(void *c, const AoeAiCountersRor *k) { (void)c; (void)k; return 1; }
static int pick_fits(void *c, const AoeAiPlanItemRor *i) { (void)c; return i->type == 12; }
static int32_t extra_count(void *c, int32_t type) { (void)c; return type == 0x67 ? 10 : 0; }
static int32_t retry_count(void *c, int32_t type, int32_t category) {
    (void)c; (void)type; (void)category; return 0;
}

int main(void) {
    static Log log;
    AoeAiLogRor sink = {&log, log_open, log_close, log_write, log_time, 0, -1};
    static AoeAiBuildListRor list, loaded;
    static AoeAiConPlanRor plan, plan2;
    AoeAiBuildItemRor *item;
    AoeAiConItemRor *con;
    AoeAiPlanItemRor like;
    static uint8_t buffer[8192];
    AoeOutcomeStream out = {buffer, sizeof(buffer), 0, 0}, in;

    /* Module base and the shared log. */
    aoe_ai_build_list_init_ror(&list, 2, &sink);
    assert(list.module.id == 0x3e9 && !strcmp(list.module.name, "Build AI"));
    assert(list.module.player == 2 && list.module.field_e8 == 50 &&
           !strcmp(list.module.player_name, "Unknown"));
    aoe_ai_module_set_player_ror(&list.module, 2, NULL);
    assert(!strcmp(list.module.player_name, "Unknown"));
    aoe_ai_log_ror(&list.module, "hidden"); /* log closed */
    assert(log.count == 0);
    aoe_ai_module_set_logging_ror(&list.module, 1);
    assert(log.opens == 1 && log.count == 2);
    assert(!strcmp(log.lines[0],
                   "P#2 Build AI: Common AI History File Started Sun Oct  4\n"));
    assert(!strcmp(log.lines[1], "P#2\n"));
    sink.player_filter = 3;
    aoe_ai_log_ror(&list.module, "filtered");
    assert(log.count == 2);
    sink.player_filter = -1;

    /* Text build list: letters, comments, exclusions and sizes. */
    list.excluded = malloc(sizeof(int32_t));
    list.excluded[0] = 12;
    list.excluded_count = list.excluded_capacity = 1;
    assert(aoe_ai_build_list_load_text_ror(
        &list, "test.ai",
        "// comment\n"
        "B109 House 2 0\n"
        "B12 Barracks 1 0\n"
        "A109 House 1 3 7\n"
        "R4 Tool 1 -1\n"
        "X1 Odd 1 0\n"
        "T83 Villager 1 109 25\n",
        objects, NULL));
    assert(list.count == 5);
    item = list.sentinel.next;
    assert(item->category == 0 && item->plan.type == 109 &&
           item->plan.unique_id == 0 && item->plan.size_x == 2.0f &&
           item->plan.size_y == 2.5f && item->cap == -1 &&
           item->plan.game_id == -1 && item->field_78 == 1 &&
           item->plan.x == -1.0f && item->build_from == 0);
    item = item->next->next;
    assert(item->plan.unique_id == 2 && item->cap == 7 &&
           item->build_from == 3);
    item = item->next;
    assert(item->category == 1 && item->plan.size_x == 3.0f);
    item = item->next;
    assert(item->category == 2 && item->cap == 25 &&
           item->plan.size_x == 3.0f && item->build_from == 109);
    assert(aoe_ai_build_list_has_file_ror(&list));
    assert(aoe_ai_build_list_count_of_ror(&list, 109, 0, AOE_AI_COUNT_ALL) == 3);
    list.sentinel.next->built = 1;
    list.sentinel.next->next->in_progress = 1;
    assert(aoe_ai_build_list_count_of_ror(&list, 109, 0, AOE_AI_COUNT_BUILT) == 1);
    assert(aoe_ai_build_list_count_of_ror(&list, 109, 0,
                                          AOE_AI_COUNT_NOT_BUILT) == 2);
    assert(aoe_ai_build_list_count_of_ror(
               &list, 109, 0, AOE_AI_COUNT_BUILT_OR_IN_PROGRESS) == 2);
    assert(aoe_ai_build_list_count_of_ror(&list, 109, 0,
                                          AOE_AI_COUNT_IN_PROGRESS) == 1);
    assert(aoe_ai_build_list_find_ror(&list, 3)->category == 1);
    assert(!aoe_ai_build_list_find_ror(&list, 9));

    /* Item accessors. */
    item = aoe_ai_build_list_find_ror(&list, 4);
    aoe_ai_build_item_drop_attempt_ror(item);
    assert(item->build_attempts == 0);
    aoe_ai_build_item_add_attempt_ror(item);
    aoe_ai_build_item_add_built_ror(item);
    aoe_ai_build_item_add_skip_cycle_ror(item);
    aoe_ai_build_item_put_ror(item, AOE_AI_BUILD_PERMANENT_SKIP, 1);
    assert(aoe_ai_build_item_get_ror(item, AOE_AI_BUILD_ATTEMPTS) == 1 &&
           aoe_ai_build_item_get_ror(item, AOE_AI_BUILD_COUNT) == 1 &&
           aoe_ai_build_item_get_ror(item, AOE_AI_BUILD_SKIP_CYCLES) == 1 &&
           aoe_ai_build_item_get_ror(item, AOE_AI_BUILD_PERMANENT_SKIP) == 1);
    assert(aoe_ai_build_item_field_90_ror(item, 0) == -1 &&
           aoe_ai_build_item_field_90_ror(item, 3) == -1);
    aoe_ai_plan_item_set_position_ror(&item->plan, AOE_AI_ITEM_Y, 8.5f);
    assert(aoe_ai_plan_item_float_ror(&item->plan, AOE_AI_ITEM_Y) == 8.5f);

    /* Insert after position 1: two copies, reversed, IDs 5 and 6. */
    assert(!aoe_ai_build_list_insert_ror(&list, objects, NULL, 109, 1, 0, 0,
                                         -1, 6));
    assert(aoe_ai_build_list_insert_ror(&list, objects, NULL, 109, 2, 0, 4,
                                        9, 1));
    assert(list.count == 7);
    item = list.sentinel.next->next->next;
    assert(item->plan.unique_id == 6 && item->next->plan.unique_id == 5 &&
           item->cap == 9 && item->plan.size_z == 1.0f &&
           !strcmp(item->plan.name, "House"));
    assert(aoe_ai_build_list_insert_ror(&list, objects, NULL, 5, 1, 6, 0, -1,
                                        0));
    assert(!strcmp(list.sentinel.next->next->plan.name,
                   "SetGatherPercentage"));

    /* Debug lines. */
    log.count = 0;
    aoe_ai_build_list_describe_ror(&list);
    assert(log.count == 9);
    assert(!strncmp(log.lines[1], "P#2 Build AI: BLT Type=109  House", 33));

    /* Save and load at version 3.1 and 2.0 (no cap). */
    strcpy(list.name_4a7, "Custom");
    assert(aoe_ai_build_list_save_ror(&list, &out));
    in = (AoeOutcomeStream){buffer, out.pos, 0, 0};
    assert(aoe_ai_build_list_load_ror(&loaded, 2, &sink, &in, 3.1f));
    assert(in.pos == out.pos && loaded.count == 8 &&
           !strcmp(loaded.file, "test.ai") &&
           !strcmp(loaded.name_4a7, "Custom"));
    for (AoeAiBuildItemRor *a = list.sentinel.next, *b = loaded.sentinel.next;
         a != &list.sentinel; a = a->next, b = b->next) {
        assert(b != &loaded.sentinel);
        assert(a->plan.type == b->plan.type && a->cap == b->cap &&
               a->plan.y == b->plan.y && a->built == b->built &&
               a->permanent_skip == b->permanent_skip &&
               !strcmp(a->plan.name, b->plan.name));
    }
    aoe_ai_build_list_release_ror(&loaded);
    in = (AoeOutcomeStream){buffer, out.pos, 0, 0};
    assert(!aoe_ai_build_list_load_ror(&loaded, 2, &sink, &in, 2.0f) ||
           in.pos != out.pos);
    aoe_ai_build_list_release_ror(&loaded);
    assert(log.closes == 0);
    aoe_ai_build_list_clear_ror(&list);
    assert(list.count == 0 && list.sentinel.next == &list.sentinel);
    assert(aoe_ai_build_list_load_text_ror(&list, "NONE", NULL, objects, NULL));
    assert(!aoe_ai_build_list_has_file_ror(&list));
    assert(!aoe_ai_build_list_load_text_ror(&list, "missing", NULL, objects,
                                            NULL));
    /* RoR build list additions. */
    assert(!aoe_ai_build_list_add_research_ror(&list, objects, NULL, 6, -1));
    assert(aoe_ai_build_list_add_gather_ror(&list, objects, NULL, 0x67, 3, 0));
    list.sentinel.next->built = 1;
    /* One built item: position 1 walks onto the sentinel, so the
       research goes to the front (as 0x00408d90 does). */
    assert(aoe_ai_build_list_add_research_ror(&list, objects, NULL, 7, -1));
    {
        AoeAiBuildItemRor *r = list.sentinel.next, *g = r->next;
        assert(list.count == 2 && r->plan.type == 7 && r->category == 1 && r->build_from == 0x2d);
        assert(g->category == 6 && g->build_from == 3);
        assert(!aoe_ai_build_list_all_started_ror(&list, 1, 9));
        assert(aoe_ai_build_list_all_started_ror(&list, 6, 9));
        assert(aoe_ai_build_list_pending_ror(&list, 7, 2, 1) == r);
        assert(aoe_ai_build_list_pending_ror(&list, 0x67, 6, 6) == NULL);
        g->category = 0;
        g->built = 0;
        g->in_progress = 1;
        assert(aoe_ai_build_list_count_ror2(&list, 0x67, 0, extra_count, NULL) == 1 + 10);
        assert(aoe_ai_build_list_count_ror2(&list, 7, 1, extra_count, NULL) == 0);
        r->build_attempts = 4;
        r->permanent_skip = 1;
        aoe_ai_build_list_retry_ror(&list, retry_count, NULL);
        assert(r->build_attempts == 0 && !r->permanent_skip);
        r->in_progress = 1;
        assert(aoe_ai_build_list_stop_ror(&list, r->plan.unique_id) && r->in_progress == 0);
        assert(aoe_ai_build_list_stop_ror(&list, 999));
    }
    {
        /* 0x004b9050: categories and producers by type. */
        AoeAiBuildItemRor *it;
        int32_t before = list.count;
        assert(!aoe_ai_build_list_add_item_ror(&list, objects, NULL, 2, 1, 0));
        assert(aoe_ai_build_list_add_item_ror(&list, objects, NULL, 0x53, 3, 0));
        for (it = list.sentinel.next; it != &list.sentinel && it->plan.type != 0x53; it = it->next) {
        }
        assert(list.count == before + 1 && it->category == 2 && it->build_from == 0x6d && it->cap == 3);
        assert(aoe_ai_build_list_add_item_ror(&list, objects, NULL, 0x45, 1, 0));
        for (it = list.sentinel.next; it != &list.sentinel && it->plan.type != 0x45; it = it->next) {
        }
        assert(it->category == 0 && it->build_from == -1);
    }
    aoe_ai_build_list_release_ror(&list);
    assert(log.closes == 1 && !list.excluded);
    {
        AoeAiRorBuildListRor rl;
        int owner;
        aoe_ai_ror_build_list_init_ror(&rl, 3, &sink);
        assert(rl.owner == NULL && rl.base.count == 0);
        aoe_ai_ror_build_list_set_owner_ror(&rl, &owner);
        assert(rl.owner == &owner);
        {
            /* 0x004b76f0 and the build events. */
            AoeAiRorBuildOpsRor ro = {NULL, ror_hook, ror_update, ror_value, NULL, NULL, &ror_flag};
            AoeAiBuildItemRor *a, *b;
            assert(aoe_ai_ror_build_list_load_ror2(&rl, "ror.ai",
                                                   "B70 Excluded 1 -1\nB109 House 1 -1\nB12 Barracks 1 -1\n",
                                                   objects, NULL));
            assert(rl.base.excluded_count == 3 && rl.base.excluded[0] == 0x46 &&
                   rl.base.excluded[2] == 0x44);
            assert(rl.base.count == 2);
            a = rl.base.sentinel.next;
            b = a->next;
            assert(a->plan.type == 109 && b->plan.type == 12);
            assert(aoe_ai_ror_build_object_ror(&rl, &ro, 12, 500, 0) && b->built && b->plan.game_id == 500);
            assert(!aoe_ai_ror_build_object_ror(&rl, &ro, 12, 501, 0));
            assert(aoe_ai_ror_build_lost_ror(&rl, &ro, 500) && !b->built && b->plan.game_id == -1);
            assert(aoe_ai_ror_build_placed_ror(&rl, &ro, 109, 77, a->plan.unique_id) && a->built);
            assert(!strcmp(rl.base.name_2a5, "House") && rl.base.field_5a8 == a->plan.unique_id);
            a->built = 0;
            assert(aoe_ai_ror_build_done_ror(&rl, &ro, 1, a->plan.unique_id, 1, 88));
            assert(a->built && a->plan.game_id == 88 && a->built_count == 2 && ror_hooks == 1);
            /* The first unbuilt item (the barracks) is not the wanted type. */
            aoe_ai_ror_build_skip_ror(&rl, &ro);
            assert(b->plan.skip == 1 && b->build_attempts == 4 && b->skip_cycles == 1);
            assert(ror_updates == 3);
            {
                /* 0x004b8960: costs of the pending items. */
                AoeAiCountersRor costs;
                aoe_ai_counters_init_ror(&costs, 4);
                b->plan.skip = 0;
                a->built = 0;
                assert(aoe_ai_ror_build_costs_ror(&rl, &costs, 5, cost_tech, cost_obj, NULL) == 2);
                assert(costs.values[1] == 109 + 12 && costs.values[2] == 10 && costs.values[0] == 0);
                assert(aoe_ai_ror_build_costs_ror(&rl, &costs, 1, cost_tech, cost_obj, NULL) == 1);
                assert(costs.values[1] == 109);
                aoe_ai_counters_release_ror(&costs);
            }
            {
                /* 0x004b8ce0: a failed attempt. */
                AoeAiRorBuildFailOpsRor fo = {NULL, NULL, fail_alive, fail_listed, NULL, fail_avoid,
                                              NULL, fail_note, fail_mark, NULL};
                a->in_progress = 1;
                assert(aoe_ai_ror_build_failed_ror(&rl, &fo, 8, 5, a->plan.unique_id, 10.0f, 4.0f, -1));
                assert(fail_box[0] == 8.0f && fail_box[3] == 6.0f && fail_notes == 1);
                assert(!a->in_progress && fail_mark_state == 2);
                assert(aoe_ai_ror_build_failed_ror(&rl, &fo, 3, 5, a->plan.unique_id, 1.0f, 1.0f, -1));
                assert(fail_notes == 0);
                assert(aoe_ai_ror_build_failed_ror(&rl, &fo, -1, 5, a->plan.unique_id, 1.0f, 1.0f, -1));
                assert(fail_notes == 1 && fail_mark_state == 0);
            }
        }
        {
            /* 0x004b7930: a house once the units outnumber the room. */
            AoeAiRorBuildListRor al;
            int houses = 0;
            aoe_ai_ror_build_list_init_ror(&al, 3, &sink);
            assert(aoe_ai_build_list_load_text_ror(&al.base, "aug.ai",
                                                   "B109 Centre 1 -1\nU83 A 1 -1\nU83 B 1 -1\n"
                                                   "U83 C 1 -1\nU83 D 1 -1\nU83 E 1 -1\n",
                                                   objects, NULL));
            assert(al.base.count == 6);
            aoe_ai_ror_build_list_augment_ror(&al, objects, NULL, aug_sn, NULL, NULL);
            for (AoeAiBuildItemRor *it = al.base.sentinel.next; it != &al.base.sentinel; it = it->next)
                houses += it->plan.type == 0x46;
            assert(houses == 1 && al.base.count == 7);
            aoe_ai_ror_build_list_release_ror(&al);
        }
        {
            /* 0x004b8110: the next building that fits, then the next unit. */
            AoeAiRorBuildListRor pl;
            AoeAiRorPickOpsRor po;
            AoeAiBuildItemRor *got;
            memset(&po, 0, sizeof po);
            po.sn = pick_sn;
            po.count_extra = pick_extra;
            po.usable = pick_yes;
            po.affordable = pick_afford;
            po.plan_fits = pick_fits;
            po.producer_present = pick_yes;
            aoe_ai_ror_build_list_init_ror(&pl, 3, &sink);
            assert(aoe_ai_build_list_load_text_ror(&pl.base, "pick.ai",
                                                   "B109 House 1 -1\nB12 Barracks 1 -1\nU83 Man 1 12\n",
                                                   objects, NULL));
            got = aoe_ai_ror_build_next_ror(&pl, 0, &po);
            assert(got && got->plan.type == 12 && !strcmp(pl.base.name_3a6, "Barracks"));
            assert(pl.base.field_5a8 == got->plan.unique_id && got->build_attempts == 1);
            got = aoe_ai_ror_build_next_ror(&pl, 2, &po);
            assert(got && got->plan.type == 83);
            assert(!aoe_ai_ror_build_next_ror(&pl, 3, &po));
            aoe_ai_ror_build_list_release_ror(&pl);
        }
        aoe_ai_ror_build_list_release_ror(&rl);
    }

    /* Construction plan: origin offsets, duplicates and culling. */
    aoe_ai_con_plan_init_ror(&plan, 1, &sink);
    assert(plan.origin_x == -1.0f && plan.map_width == -1);
    assert(aoe_ai_con_plan_load_text_ror(
        &plan, "town.cp",
        "// x y z sx sy sz type name\n"
        "1 1 0 2 2 1 109 House\n"
        "1 1 0 2 2 1 109 Again\n"
        "-9 0 0 2 2 1 12 Off\n"
        "4 2 0 3 3 1 12 Barracks\n",
        32, 32, 10.0f, 10.0f, 0.0f));
    assert(plan.count == 3);
    con = aoe_ai_con_plan_at_ror(&plan, 11.0f, 11.0f);
    assert(con && !strcmp(con->plan.name, "House") &&
           con->plan.unique_id == -1 && con->plan.type == 109);
    /* 0x004ba000: a planned item at the spot, else a mark. */
    aoe_ai_con_plan_note_ror(&plan, 109, 11.0f, 11.0f, 7, mark_fn, NULL);
    assert(con->field_78 == 7 && mark_type == -1);
    aoe_ai_con_plan_note_ror(&plan, 109, 20.5f, 3.0f, 7, mark_fn, NULL);
    assert(mark_type == 109 && mark_x == 20 && mark_y == 3);
    {
        /* 0x004b9700. */
        AoeAiPlanItemRor want;
        aoe_ai_plan_item_init_ror(&want);
        want.type = 12;
        assert(aoe_ai_con_plan_fits_ror(&plan, &want, 0, fit_fallback, NULL));
        want.type = 99;
        assert(!aoe_ai_con_plan_fits_ror(&plan, &want, 0, fit_fallback, NULL));
        assert(aoe_ai_con_plan_fits_ror(&plan, &want, 1, fit_fallback, NULL) && fit_fallbacks == 1);
        want.type = 0x2d;
        assert(aoe_ai_con_plan_fits_ror(&plan, &want, 0, fit_fallback, NULL));
        aoe_ai_plan_item_release_ror(&want);
    }
    aoe_ai_con_plan_add_ror(&plan, 11.0f, 11.0f, 3);
    aoe_ai_con_plan_drop_ror(&plan, 11.0f, 11.0f, 1);
    assert(con->counter == 2);
    con->plan.unique_id = 44;
    like.x = 11.0f;
    like.y = 11.0f;
    aoe_ai_con_plan_mark_ror(&plan, &like, 5);
    assert(con->field_78 == 5);
    assert(aoe_ai_con_plan_release_item_ror(&plan, &like, 1) == 44);
    assert(con->counter == 1 && con->field_78 == 0 &&
           con->plan.unique_id == -1);
    assert(aoe_ai_con_plan_release_at_ror(&plan, 99.0f, 1.0f, 0) == -1);

    /* Placing at a new origin moves items; leaving the map deletes. */
    aoe_ai_con_plan_place_ror(&plan, 32, 32, 17.0f, 0.5f, 0.0f);
    assert(plan.count == 2); /* the barracks at x 31 + 1.5 leaves */
    assert(aoe_ai_con_plan_at_ror(&plan, 28.0f, 11.5f));
    assert(aoe_ai_con_plan_at_ror(&plan, 18.0f, 10.5f));
    aoe_ai_con_plan_place_ror(&plan, 32, 32, 0.0f, 1.0f, 0.0f);
    assert(plan.count == 0);

    /* Save and load the plan. */
    assert(aoe_ai_con_plan_load_text_ror(&plan, "town.cp",
                                         "1 1 0 2 2 1 109 House\n", 32, 32,
                                         5.0f, 5.0f, 0.0f));
    con = plan.plan.next;
    con->counter = 3;
    con->field_74 = 2;
    {
        AoeAiConItemRor *placed = malloc(sizeof(*placed));
        aoe_ai_con_item_set_ror(placed, 9, 9, 0, 1, 1, 1, 7, "Placed");
        placed->next = &plan.placed;
        placed->prev = &plan.placed;
        plan.placed.next = plan.placed.prev = placed;
        plan.placed_count = 1;
    }
    out = (AoeOutcomeStream){buffer, sizeof(buffer), 0, 0};
    assert(aoe_ai_con_plan_save_ror(&plan, &out));
    in = (AoeOutcomeStream){buffer, out.pos, 0, 0};
    assert(aoe_ai_con_plan_load_ror(&plan2, 1, &sink, &in));
    assert(in.pos == out.pos && plan2.count == 1 && plan2.placed_count == 1 &&
           plan2.origin_x == 5.0f && plan2.map_height == 32);
    assert(plan2.plan.next->counter == 3 && plan2.plan.next->field_74 == 2 &&
           plan2.plan.next->plan.x == 6.0f);
    assert(aoe_ai_con_plan_placed_at_ror(&plan2, 9.0f, 9.0f));
    assert(!aoe_ai_con_plan_placed_at_ror(&plan2, 6.0f, 6.0f));
    aoe_ai_con_plan_release_ror(&plan2);
    aoe_ai_con_plan_release_ror(&plan);
    return 0;
}
