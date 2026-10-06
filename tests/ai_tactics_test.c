/* Patched RoR 1.0a tactical AI pieces 0x0040d150..0x0040ede0. */
#include "ai_tactics.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int unit(void *c, int32_t id, uint8_t *state, float *hp) {
    (void)c;
    if (id == 3) return 0;
    *state = id == 4 ? 3 : 2;
    *hp = (float)id + 0.9f;
    return 1;
}

/* Units 100..: category = id % 10 (8 and 9 refused). */
static int32_t fill_category(void *c, int32_t id) {
    (void)c;
    return id % 10 >= 6 ? -1 : id % 10;
}

static void fill_tests(void) {
    static AoeAiGroupRor play;
    static AoeAiMembersRor m;
    const int32_t ids[] = {100, 110, 120, 101, 111, 102};
    const int32_t bad[] = {100, 108};
    aoe_ai_group_init_ror(&play);
    aoe_ai_members_init_ror(&m);
    assert(m.field_190 == -1 && m.field_1b8 == 0xfd && m.slots[49].id == -1 &&
           m.slots[0].role == 0xff && m.totals[4] == -1 && m.shortfall == 0);
    /* Two ranges: range 0 wants at least 2 of category 0 (at most 3),
       range 1 at least 1 of category 1 and 2 of category 2 (max 2 each). */
    play.range_count = 2;
    play.field_42 = 50;
    aoe_ai_range_set_min_ror(&play.ranges[0], 0, 2);
    aoe_ai_range_set_max_ror(&play.ranges[0], 0, 3);
    aoe_ai_range_set_max_ror(&play.ranges[0], 1, 0);
    aoe_ai_range_set_max_ror(&play.ranges[0], 2, 0);
    aoe_ai_range_set_min_ror(&play.ranges[1], 1, 1);
    aoe_ai_range_set_min_ror(&play.ranges[1], 2, 2);
    aoe_ai_range_set_max_ror(&play.ranges[1], 0, 0);
    aoe_ai_range_set_max_ror(&play.ranges[1], 1, 2);
    aoe_ai_range_set_max_ror(&play.ranges[1], 2, 2);
    assert(aoe_ai_members_fill_ror(&m, 1, &play, ids, 6, fill_category, NULL) == 1);
    /* 100, 110 (minimum) and 120 (room) in range 0; 101, 111 and 102 in
       range 1; one category-2 member short. */
    assert(aoe_ai_members_count_role_ror(&m, 0, 0) == 3 &&
           aoe_ai_members_count_role_ror(&m, 1, 1) == 2 &&
           aoe_ai_members_count_role_ror(&m, 1, 2) == 1 && m.shortfall == 1);
    /* A refused unit clears the result; a size of 1 stops at once. */
    aoe_ai_members_init_ror(&m);
    play.field_42 = 1;
    assert(aoe_ai_members_fill_ror(&m, 2, &play, bad, 2, fill_category, NULL) == 0);
    assert(aoe_ai_members_count_ror(&m) == 1 && m.shortfall == 4);
    /* No range 6: 0. */
    aoe_ai_members_init_ror(&m);
    play.range_count = 7;
    play.field_42 = 50;
    assert(aoe_ai_members_fill_ror(&m, 2, &play, ids, 6, fill_category, NULL) == 0);
    assert(aoe_ai_members_fill_ror(&m, 2, NULL, ids, 6, fill_category, NULL) == 0);
}

static void point_tests(void) {
    AoeAiPointRor p, o, out;
    aoe_ai_point_zero_ror(&p);
    assert(p.x == 0 && p.y == 0 && p.z == 0);
    aoe_ai_point_set_ror(&p, 12, 10, 3);
    aoe_ai_point_set_ror(&o, 10, 10, 0);
    aoe_ai_point_rotate_ror(&p, &o, &out, 0);
    assert(out.x == 12 && out.y == 10 && out.z == 3);
    /* A quarter turn (two eighths): the offset (2, 0) becomes (0, 2),
       without the origin added back. */
    aoe_ai_point_rotate_ror(&p, &o, &out, 2);
    assert(out.x == 0 && out.y == 2 && out.z == 3);
}


/* 0x0040d840: an in-memory play file. */
typedef struct {
    const char *const *lines;
    int pos, opened, rewound, closed;
} PlayFile;

static void *pf_open(void *c, const char *path) {
    PlayFile *f = c;
    if (strcmp(path, "plays.ai")) return NULL;
    f->opened++;
    f->pos = 0;
    return f;
}
static int pf_gets(void *c, void *file, char *buf, int32_t size) {
    PlayFile *f = c;
    assert(file == f);
    if (!f->lines[f->pos]) return 0;
    strncpy(buf, f->lines[f->pos++], (size_t)size - 1);
    buf[size - 1] = '\0';
    return 1;
}
static void pf_rewind(void *c, void *file) {
    PlayFile *f = c;
    (void)file;
    f->pos = 0;
    f->rewound++;
}
static void pf_close(void *c, void *file) {
    (void)file;
    ((PlayFile *)c)->closed++;
}
static int32_t pf_type(void *c, const char *name) {
    (void)c;
    return !strcmp(name, "Tower") ? 7 : 0;
}
static int32_t pf_char(void *c, const char *name) {
    (void)c;
    return !strcmp(name, "Weak") ? 3 : 0;
}
static int32_t pf_category(void *c, const char *name) {
    (void)c;
    return !strcmp(name, "Archer") ? 2 : -1;
}

static void play_load_tests(void) {
    static const char *const lines[] = {
        "// a comment\n",
        "\n",
        "   \n",
        "Play \"Rush\"\n",
        "PlayType Flank\n",
        "MinUnits 4\n",
        "MaxUnits 12\n",
        "Power 9\n",
        "Overflow FirstGroup\n",
        "DeathPercentage 130\n",
        "Intelligence -5\n",
        "TargetType Tower\n",
        "TargetType Wall\n",
        "TargetChar Weak\n",
        "NumberGroups 2\n",
        "Group 1 Archer 3 - 70\n",
        "Group 0 Archer 5 - -1\n",
        "Group 0 Cavalry 5 - 9\n",
        "Group 7 Archer 5 - 9\n",
        "Phase 2\n",
        "Group 1 Move 10 0 5\n",
        "Group 2 Attack 1 2 3\n",
        "Group 6 Attack 1 2 3\n",
        "Trigger 4 Death 6 7\n",
        "RTrigger 1 Time 8 9 10\n",
        "EndPlay\n",
        "Play \"HPHold\"\n",
        "MinUnits 2\n",
        "EndPlay\n",
        NULL};
    static const char *const empty[] = {NULL};
    PlayFile f = {lines, 0, 0, 0, 0};
    AoeAiPlayLoadOpsRor ops = {&f, pf_open, pf_gets, pf_rewind, pf_close,
                               pf_type, pf_char, pf_category};
    AoeAiPlayListRor list;
    AoeAiGroupRor *p, *q;
    AoeAiRuleRor *r;
    int k;

    aoe_ai_play_list_init_ror(&list);
    /* NULL and NONE only name the list. */
    assert(aoe_ai_play_list_load_ror(&list, NULL, &ops) == 0 &&
           !strcmp(list.name, "NULL") && f.opened == 0);
    assert(aoe_ai_play_list_load_ror(&list, "NONE", &ops) == 0 &&
           !strcmp(list.name, "NONE") && f.opened == 0);
    assert(aoe_ai_play_list_load_ror(&list, "missing.ai", &ops) == 0 &&
           !strcmp(list.name, "missing.ai") && !list.plays);

    assert(aoe_ai_play_list_load_ror(&list, "plays.ai", &ops) == 1);
    assert(f.opened == 1 && f.rewound == 1 && f.closed == 1);
    /* Two Play lines reserve 16; the HP play gives back seven. */
    assert(list.count == 9);
    p = (AoeAiGroupRor *)aoe_ai_play_list_get_ror(&list, 0);
    assert(!strcmp(p->name, "Rush") && p->field_43 == 3 &&
           p->field_41 == 4 && p->field_42 == 12 && p->field_4e == 9 &&
           p->mode == 2 && p->field_51 == 100 && p->field_50 == 0 &&
           p->range_count == 2);
    assert(p->slots_44[0] == 7 && p->slots_44[1] == 5 &&
           p->slots_49[0] == 3 && p->slots_49[1] == 5);
    assert(p->ranges[1].min[2] == 3 && p->ranges[1].max[2] == 50);
    assert(p->ranges[0].min[2] == 0 && p->ranges[0].max[2] == 0xff);
    r = &p->rules[2];
    assert(r->actions[0].field_0 == 1 && r->actions[0].code == 1 &&
           r->actions[0].a == 10 && r->actions[0].c == 5);
    assert(r->actions[1].code == 2 && r->actions[1].b == 2 &&
           r->actions[2].code == 0);
    assert(r->triggers[0].code == 2 && r->triggers[0].a == 6 &&
           r->triggers[0].b == 7 && r->triggers[0].c == 2 &&
           r->triggers[0].d == 4 && r->triggers[0].e == 0);
    assert(r->triggers[1].code == 11 && r->triggers[1].a == 8 &&
           r->triggers[1].b == 9 && r->triggers[1].c == 2 &&
           r->triggers[1].d == 1 && r->triggers[1].e == 10);
    /* Seven rotated copies RushF1..RushF7. */
    for (k = 1; k < 8; k++) {
        char name[16];
        q = (AoeAiGroupRor *)aoe_ai_play_list_get_ror(&list, k);
        snprintf(name, sizeof name, "RushF%d", k);
        assert(!strcmp(q->name, name) && q->field_43 == 3 &&
               q->mode == 2 && q->field_42 == 12 && q->range_count == 2 &&
               q->slots_44[0] == 7 && q->slots_49[0] == 3 &&
               q->ranges[1].max[2] == 50 &&
               q->rules[2].triggers[1].e == 10 &&
               q->rules[2].actions[1].a == 1 && q->rules[2].actions[1].b == 2);
        assert(q->rules[2].actions[0].c == 5);
    }
    q = (AoeAiGroupRor *)aoe_ai_play_list_get_ror(&list, 2); /* quarter turn */
    assert(q->rules[2].actions[0].a == 0 && q->rules[2].actions[0].b == 10);
    q = (AoeAiGroupRor *)aoe_ai_play_list_get_ror(&list, 4);
    /* Half turn: cos(4 * (float)(pi / 4)) is just above -1, truncated. */
    assert(q->rules[2].actions[0].a == -9 && q->rules[2].actions[0].b == 0);
    q = (AoeAiGroupRor *)aoe_ai_play_list_get_ror(&list, 1); /* eighth */
    assert(q->rules[2].actions[0].a == 7 && q->rules[2].actions[0].b == 7);
    /* The HP play is not expanded. */
    q = (AoeAiGroupRor *)aoe_ai_play_list_get_ror(&list, 8);
    assert(!strcmp(q->name, "HPHold") && q->field_41 == 2);

    /* Reloading frees the old plays; an empty file gives no plays. */
    f.lines = empty;
    assert(aoe_ai_play_list_load_ror(&list, "plays.ai", &ops) == 1 &&
           list.count == 0 && list.plays);
    aoe_ai_play_list_release_ror(&list);
}

int main(void) {
    point_tests();
    play_load_tests();
    static AoeAiGroupRor group;
    static AoeAiMembersRor m, m2;
    AoeAiCountersRor c;
    AoeAiPlayListRor plays;
    AoeAiTradeRor trade;

    fill_tests();
    AoeAiModuleRor strat;
    int32_t ids[50];
    uint8_t buffer[1024];
    AoeOutcomeStream out = {buffer, sizeof(buffer), 0, 0}, in;

    aoe_ai_group_init_ror(&group);
    aoe_ai_action_set_ror(&group.rules[2].actions[3], 1, 4, 0, 0, 0);
    assert(aoe_ai_group_find_action_ror(&group, 1, 4, 2) == 3);
    assert(aoe_ai_group_find_action_ror(&group, 1, 4, 10) == -1);
    assert(aoe_ai_group_find_action_ror(&group, 5, 4, 2) == -1);
    assert(aoe_ai_attack_kind_ror("Annoy") == 2 &&
           aoe_ai_attack_kind_ror("Recon") == 4 &&
           aoe_ai_attack_kind_ror("x") == 0);
    assert(aoe_ai_speed_kind_ror("Stationary") == 1 &&
           aoe_ai_speed_kind_ror("Any") == 5 && aoe_ai_any_kind_ror("Fast") == 0);

    for (int i = 0; i < AOE_AI_MEMBERS; ++i) m.slots[i].id = -1;
    assert(aoe_ai_members_add_ror(&m, 3, 1, 0) && aoe_ai_members_add_ror(&m, 4, 1, 2) &&
           aoe_ai_members_add_ror(&m, 7, 1, 2) && aoe_ai_members_add_ror(&m, 9, 0, 2));
    assert(aoe_ai_members_count_ror(&m) == 4 &&
           aoe_ai_members_count_category_ror(&m, 1) == 3 &&
           aoe_ai_members_count_role_ror(&m, 1, 2) == 2 &&
           aoe_ai_members_category_ror(&m, 9) == 0 &&
           aoe_ai_members_category_ror(&m, 8) == 0xff);
    m.slots[1].pad[0] = 0xaa;
    m.totals[4] = 12;
    m.field_1b8 = 5;
    assert(aoe_ai_members_save_ror(&m, &out) && out.pos == 20 + 20 + 1 + 400 + 8);
    in = (AoeOutcomeStream){buffer, out.pos, 0, 0};
    assert(aoe_ai_members_load_ror(&m2, &in) && !memcmp(&m2.slots, &m.slots, sizeof(m.slots)) &&
           m2.totals[4] == 12 && m2.field_1b8 == 5);
    aoe_ai_members_purge_ror(&m, unit, NULL);
    assert(aoe_ai_members_count_ror(&m) == 2 && m.slots[0].category == 0xff);
    assert(aoe_ai_members_ids_ror(&m, ids) == 2 && ids[0] == 7 && ids[1] == 9);
    aoe_ai_members_sum_hp_ror(&m, 1, unit, NULL);
    assert(aoe_ai_members_total_ror(&m, 1) == 7 &&
           aoe_ai_members_total_ror(&m, 5) == -1);
    for (int i = 0; i < AOE_AI_MEMBERS; ++i) aoe_ai_members_add_ror(&m, 100 + i, 0, 0);
    assert(!aoe_ai_members_add_ror(&m, 1, 0, 0));

    aoe_ai_counters_init_ror(&c, 3);
    assert(c.values[2] == 0 && c.values[3] == -1 && c.second[0] == -1);
    aoe_ai_counters_add_ror(&c, 1, 5);
    aoe_ai_counters_set_ror(&c, 3, 9); /* ignored */
    assert(aoe_ai_counters_get_ror(&c, 1) == 5 && aoe_ai_counters_get_ror(&c, 3) == -1 &&
           aoe_ai_counters_count_ror(&c) == 3);
    aoe_ai_counters_fill_ror(&c, 2);
    assert(c.values[7] == 2);
    {
        const int16_t ty[3] = {0, -1, 2}, am[3] = {10, 99, 4};
        aoe_ai_counters_add_costs_ror(&c, ty, am);
        assert(c.values[0] == 12 && c.values[1] == 2 && c.values[2] == 6);
    }

    aoe_ai_play_list_init_ror(&plays);
    assert(!aoe_ai_play_list_get_ror(&plays, 0));
    plays.plays = calloc(2, sizeof(AoeAiPlayRor));
    plays.count = 2;
    assert(aoe_ai_play_list_get_ror(&plays, 1) == &plays.plays[1] &&
           !aoe_ai_play_list_get_ror(&plays, 2));
    aoe_ai_play_list_release_ror(&plays);
    assert(!plays.plays && !plays.count);

    aoe_ai_strategy_init_ror(&strat, 2, NULL);
    assert(strat.id == 0x3f0 && !strcmp(strat.name, "Strategy AI"));
    aoe_ai_strategy_release_ror(&strat);
    aoe_ai_tac_init_ror(&strat, 2, NULL);
    assert(strat.id == 0x3f1 && !strcmp(strat.name, "TacAI"));
    aoe_ai_tac_release_ror(&strat);
    aoe_ai_trade_load_ror(&trade, 4, NULL);
    assert(trade.module.id == 0x3f2 && trade.module.player == 4 && !trade.field_f0);
    aoe_ai_trade_release_ror(&trade);
    assert(aoe_ai_unit_default_c0_ror() == -1);
    return 0;
}
