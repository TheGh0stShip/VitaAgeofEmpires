/* Patched RoR 1.0a RoR scenario binary constructor 0x00506900.
   Evidence (EMPIRESX.EXE): the gates are x87 compares of the base's
   version f32 at +0x1888 with 0x00545e80 = 0x3f90a3d7 (1.13),
   0x00545e4c = 0x3f828f5c (1.02), 0x00545e60 = 0x3f83d70a (1.03),
   0x0054a3e0 = 0x3f851eb8 (1.04), 0x0054a3dc = 0x3f8f5c29 (1.12),
   0x0054a3d8 = 0x3f866666 (1.05) and the raw compare with 0x3f8147ae
   (1.01) at 0x00506b42. 0x00506a92 "test ah, 1" takes the 4 / 900 /
   9000 victory defaults below 1.13; 0x00506bd9 reads three options from
   1.12 up, else one with options 1 and 2 cleared (0x00506bef/0x00506bf5);
   0x00506c5b copies the +0x0c byte into +0x1a90. The writer 0x00506c80
   (aoe_scenario_info_save_ror) produces exactly the 1.15 layout. */
#include "world_ror_scenario.h"

#include <assert.h>
#include <string.h>

static uint8_t buf[40000];
static AoeScenarioInfoRor info, saved;
static AoeScenarioObjectRor obj;
static uint8_t base_conquest;

/* Stand-in for 0x474d40: here its part of the stream is just the version
   (its first field) and the +0x0c byte. */
static float base_read(void *c, AoeOutcomeStream *s, void *world, AoeScenarioObjectRor *o) {
    float v;
    assert(c == &base_conquest);
    assert(world == NULL);
    v = aoe_stream_f32(s);
    o->field_c = aoe_stream_u8(s);
    return v;
}

static void base_save(void *c) {
    AoeOutcomeStream *s = c;
    aoe_stream_put_f32(s, 1.15f); /* 0x00475f80's version */
    aoe_stream_put_u8(s, base_conquest);
}

static void save_write(void *c, const uint8_t *d, int32_t n) {
    aoe_stream_put((AoeOutcomeStream *)c, d, (size_t)n);
}

static AoeOutcomeStream begin(void) {
    AoeOutcomeStream s;
    s.data = buf;
    s.size = sizeof buf;
    s.pos = 0;
    s.failed = 0;
    return s;
}

static AoeOutcomeStream rewind_for_read(const AoeOutcomeStream *w) {
    AoeOutcomeStream s = *w;
    assert(!w->failed);
    s.size = w->pos;
    s.pos = 0;
    return s;
}

static void fill(AoeScenarioInfoRor *x) {
    int p, c, t;
    memset(x, 0, sizeof *x);
    for (p = 0; p < 16; ++p) {
        for (t = 0; t < 4; ++t) x->start_resources[p][t] = p * 10 + t + 1;
        for (t = 0; t < 16; ++t) x->diplomacy[p][t] = (p + t) % 4;
        for (c = 0; c < 12; ++c) {
            AoeScenarioConditionRor *r = &x->conditions[p][c];
            r->object_class = p * 100 + c;
            r->all_flag = c & 1;
            r->player = p + 1;
            r->field_0c = -c;
            r->area[0] = (float)p + 0.5f;
            r->area[1] = (float)c + 0.25f;
            r->area[2] = 3.0f;
            r->area[3] = -1.0f;
            r->type = c % 8;
            r->amount = c * 7;
            r->subtype = p % 7;
            r->unit_ref = p * 12 + c + 1;
            r->target_ref = c;
            r->field_34[0] = 77;
            r->field_34[1] = 88;
        }
        for (t = 0; t < 20; ++t) x->tech_toggles[p][t] = (p + t) & 1;
        x->allied_victory[p] = p & 1;
        x->starting_age[p] = p % 4;
    }
    x->custom_conquest = 11;
    x->ruins = 12;
    x->artifacts = 13;
    x->discoveries = 14;
    x->explore = 15;
    x->gold = 16;
    x->all_custom_required = 1;
    x->victory_type = 2;
    x->score = 1234;
    x->time_limit = 5678;
    x->options[0] = 1;
    x->options[1] = 2;
    x->options[2] = 3;
}

static void poison(void) {
    memset(&info, 0x55, sizeof info);
    memset(&obj, 0, sizeof obj);
}

static float read_back(AoeOutcomeStream *s) {
    AoeRorScenarioReadOpsRor ops = {&base_conquest, base_read};
    return aoe_ror_scenario_read_ror(&info, &obj, s, NULL, &ops);
}

/* Old-format pieces, in 0x00506900's order. */
static void put_names(AoeOutcomeStream *s) {
    char name[0x100];
    int p;
    for (p = 0; p < 16; ++p) {
        memset(name, 0, sizeof name);
        name[0] = (char)('A' + p);
        name[0xff] = 'z'; /* raw 0x100 bytes, not terminated */
        aoe_stream_put(s, name, sizeof name);
    }
}

static void put_old_players(AoeOutcomeStream *s) {
    int p, t;
    for (p = 0; p < 16; ++p) {
        aoe_stream_put_i32(s, 1);                  /* +0x194c */
        for (t = 0; t < 4; ++t) aoe_stream_put_i32(s, saved.start_resources[p][t]);
        aoe_stream_put_i32(s, p == 0);             /* +0x18cc */
        aoe_stream_put_i32(s, p + 1);              /* +0x190c */
        aoe_stream_put_i32(s, 4);                  /* +0x188c */
    }
}

static void put_middle(AoeOutcomeStream *s, int marker, int victory) {
    int p, q, c, i;
    if (marker) aoe_stream_put_i32(s, -99);
    aoe_stream_put_i32(s, saved.custom_conquest);
    aoe_stream_put_i32(s, saved.ruins);
    aoe_stream_put_i32(s, saved.artifacts);
    aoe_stream_put_i32(s, saved.discoveries);
    aoe_stream_put_i32(s, saved.explore);
    aoe_stream_put_i32(s, saved.gold);
    aoe_stream_put_i32(s, saved.all_custom_required);
    if (victory) {
        aoe_stream_put_i32(s, saved.victory_type);
        aoe_stream_put_i32(s, saved.score);
        aoe_stream_put_i32(s, saved.time_limit);
    }
    for (p = 0; p < 16; ++p)
        for (q = 0; q < 16; ++q) aoe_stream_put_i32(s, saved.diplomacy[p][q]);
    for (p = 0; p < 16; ++p)
        for (c = 0; c < 12; ++c) {
            const AoeScenarioConditionRor *r = &saved.conditions[p][c];
            aoe_stream_put_i32(s, r->object_class);
            aoe_stream_put_i32(s, r->all_flag);
            aoe_stream_put_i32(s, r->player);
            aoe_stream_put_i32(s, r->field_0c);
            for (i = 0; i < 4; ++i) aoe_stream_put_f32(s, r->area[i]);
            aoe_stream_put_i32(s, r->type);
            aoe_stream_put_i32(s, r->amount);
            aoe_stream_put_i32(s, r->subtype);
            aoe_stream_put_i32(s, r->unit_ref);
            aoe_stream_put_i32(s, r->target_ref);
            aoe_stream_put_i32(s, r->field_34[0]);
            aoe_stream_put_i32(s, r->field_34[1]);
        }
    if (marker) aoe_stream_put_i32(s, -99);
}

static void put_allied(AoeOutcomeStream *s, int32_t base) {
    int p;
    for (p = 0; p < 16; ++p) aoe_stream_put_i32(s, base + p);
}

static void check_common(void) {
    assert(memcmp(info.start_resources, saved.start_resources, sizeof info.start_resources) == 0);
    assert(memcmp(info.diplomacy, saved.diplomacy, sizeof info.diplomacy) == 0);
    assert(memcmp(info.conditions, saved.conditions, sizeof info.conditions) == 0);
    assert(info.ruins == 12 && info.artifacts == 13 && info.discoveries == 14);
    assert(info.explore == 15 && info.gold == 16 && info.all_custom_required == 1);
    assert(info.selected == 0);
    /* +0x1a90 ends as the +0x0c byte, not the stored goal. */
    assert(info.custom_conquest == base_conquest && info.conquest == base_conquest);
}

static void check_old_players(void) {
    int p;
    for (p = 0; p < 16; ++p) {
        assert(obj.player_name[p][0] == (char)('A' + p));
        assert(obj.player_name[p][0xff] == 'z');
        assert(obj.value_194c[p] == 1);
        assert(obj.value_18cc[p] == (p == 0));
        assert(obj.value_190c[p] == p + 1);
        assert(obj.player_kind[p] == 4);
    }
}

static void check_all_techs_enabled(void) {
    int p, t;
    for (p = 0; p < 16; ++p)
        for (t = 0; t < 20; ++t) assert(info.tech_toggles[p][t] == 1);
}

static void test_round_trip_115(void) {
    AoeOutcomeStream w = begin(), r;
    AoeScenarioSaveOpsRor sops;
    fill(&saved);
    base_conquest = 1;
    sops.context = &w;
    sops.base_save = base_save;
    sops.write = save_write;
    aoe_scenario_info_save_ror(&saved, NULL, &sops);
    r = rewind_for_read(&w);
    poison();
    assert(read_back(&r) == 1.15f);
    assert(!r.failed && r.pos == r.size);
    check_common();
    assert(info.victory_type == 2 && info.score == 1234 && info.time_limit == 5678);
    assert(memcmp(info.allied_victory, saved.allied_victory, sizeof info.allied_victory) == 0);
    assert(memcmp(info.tech_toggles, saved.tech_toggles, sizeof info.tech_toggles) == 0);
    assert(memcmp(info.options, saved.options, sizeof info.options) == 0);
    assert(memcmp(info.starting_age, saved.starting_age, sizeof info.starting_age) == 0);
    /* Above 1.13 the names and the per-player values stay the base's. */
    assert(obj.player_name[3][0] == 0 && obj.value_190c[3] == 0);
}

/* 1.01 exactly: names, old players, no markers, victory defaults, the
   allied victory read sixteen times (the last kept), nothing later. */
static void test_version_101(void) {
    AoeOutcomeStream w = begin(), r;
    int i;
    fill(&saved);
    base_conquest = 0;
    aoe_stream_put_f32(&w, 1.01f);
    aoe_stream_put_u8(&w, base_conquest);
    put_names(&w);
    put_old_players(&w);
    put_middle(&w, 0, 0);
    for (i = 0; i < 16; ++i) put_allied(&w, i * 100);
    r = rewind_for_read(&w);
    poison();
    assert(read_back(&r) == 1.01f);
    assert(!r.failed && r.pos == r.size);
    check_common();
    check_old_players();
    assert(info.victory_type == 4 && info.score == 900 && info.time_limit == 9000);
    for (i = 0; i < 16; ++i) assert(info.allied_victory[i] == 1500 + i);
    check_all_techs_enabled();
    /* Options and starting ages are left as they were. */
    assert(info.options[1] == 0x55555555 && info.starting_age[0] == 0x55555555);
}

/* Below 1.01: not even the allied victory. */
static void test_version_100(void) {
    AoeOutcomeStream w = begin(), r;
    fill(&saved);
    base_conquest = 1;
    aoe_stream_put_f32(&w, 1.0f);
    aoe_stream_put_u8(&w, base_conquest);
    put_names(&w);
    put_old_players(&w);
    put_middle(&w, 0, 0);
    r = rewind_for_read(&w);
    poison();
    assert(read_back(&r) == 1.0f);
    assert(!r.failed && r.pos == r.size);
    check_common();
    assert(info.allied_victory[5] == 0x55555555);
}

/* 1.10: markers, the allied victory once, the techs, one option (the
   other two cleared) and the starting ages; victory still defaulted. */
static void test_version_110(void) {
    AoeOutcomeStream w = begin(), r;
    int p, t;
    fill(&saved);
    base_conquest = 1;
    aoe_stream_put_f32(&w, 1.10f);
    aoe_stream_put_u8(&w, base_conquest);
    put_names(&w);
    put_old_players(&w);
    put_middle(&w, 1, 0);
    put_allied(&w, 40);
    for (p = 0; p < 16; ++p)
        for (t = 0; t < 20; ++t) aoe_stream_put_i32(&w, saved.tech_toggles[p][t]);
    aoe_stream_put_i32(&w, 9);
    put_allied(&w, 60); /* starting ages */
    aoe_stream_put_i32(&w, -99);
    r = rewind_for_read(&w);
    poison();
    assert(read_back(&r) == 1.10f);
    assert(!r.failed && r.pos == r.size);
    check_common();
    check_old_players();
    assert(info.victory_type == 4 && info.score == 900 && info.time_limit == 9000);
    assert(info.allied_victory[15] == 55);
    assert(memcmp(info.tech_toggles, saved.tech_toggles, sizeof info.tech_toggles) == 0);
    assert(info.options[0] == 9 && info.options[1] == 0 && info.options[2] == 0);
    assert(info.starting_age[2] == 62);
}

/* 1.04: techs (above 1.03) but no options and no starting ages. */
static void test_version_104(void) {
    AoeOutcomeStream w = begin(), r;
    int p, t;
    fill(&saved);
    base_conquest = 0;
    aoe_stream_put_f32(&w, 1.04f);
    aoe_stream_put_u8(&w, base_conquest);
    put_names(&w);
    put_old_players(&w);
    put_middle(&w, 1, 0);
    put_allied(&w, 0);
    for (p = 0; p < 16; ++p)
        for (t = 0; t < 20; ++t) aoe_stream_put_i32(&w, 0);
    aoe_stream_put_i32(&w, -99);
    r = rewind_for_read(&w);
    poison();
    assert(read_back(&r) == 1.04f);
    assert(!r.failed && r.pos == r.size);
    check_common();
    assert(info.tech_toggles[7][3] == 0);
    assert(info.options[0] == 0x55555555 && info.starting_age[0] == 0x55555555);
}

/* 1.13 itself: still the old names and players (not above 1.13), but
   the victory settings (at least 1.13) and three options (at least 1.12). */
static void test_version_113(void) {
    AoeOutcomeStream w = begin(), r;
    int p, t;
    fill(&saved);
    base_conquest = 1;
    aoe_stream_put_f32(&w, 1.13f);
    aoe_stream_put_u8(&w, base_conquest);
    put_names(&w);
    put_old_players(&w);
    put_middle(&w, 1, 1);
    put_allied(&w, 0);
    for (p = 0; p < 16; ++p)
        for (t = 0; t < 20; ++t) aoe_stream_put_i32(&w, saved.tech_toggles[p][t]);
    aoe_stream_put_i32(&w, 7);
    aoe_stream_put_i32(&w, 8);
    aoe_stream_put_i32(&w, 9);
    put_allied(&w, 0);
    aoe_stream_put_i32(&w, -99);
    r = rewind_for_read(&w);
    poison();
    assert(read_back(&r) == 1.13f);
    assert(!r.failed && r.pos == r.size);
    check_common();
    check_old_players();
    assert(info.victory_type == 2 && info.score == 1234 && info.time_limit == 5678);
    assert(info.options[0] == 7 && info.options[1] == 8 && info.options[2] == 9);
}

static void test_missing(void) {
    AoeOutcomeStream w = begin();
    AoeRorScenarioReadOpsRor ops = {NULL, NULL};
    poison();
    assert(aoe_ror_scenario_read_ror(&info, &obj, &w, NULL, &ops) == 0.0f);
    assert(aoe_ror_scenario_read_ror(&info, &obj, &w, NULL, NULL) == 0.0f);
    assert(w.pos == 0 && info.score == 0x55555555);
}

int main(void) {
    uint32_t u;
    float f = 1.01f;
    memcpy(&u, &f, 4);
    assert(u == 0x3f8147aeu);
    f = 1.13f;
    memcpy(&u, &f, 4);
    assert(u == 0x3f90a3d7u);
    test_round_trip_115();
    test_version_101();
    test_version_100();
    test_version_110();
    test_version_104();
    test_version_113();
    test_missing();
    return 0;
}
