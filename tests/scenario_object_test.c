/* Patched RoR 1.0a scenario object settings (0x00475800..0x00476250). */
#include "scenario_object.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void str(void *c, int32_t id, char *out, int n) {
    (void)c;
    snprintf(out, (size_t)n, "%s", id == 0x277b ? "Random" : id == 0x2775 ? " <None> " : "");
}
static int temp_ok = 1, writes, write_ok = 1;
static char written[256];
static int temp(void *c, char *out, int n) { (void)c; if (!temp_ok) return 0; snprintf(out, (size_t)n, "T/"); return 1; }
static int wf(void *c, const char *p, const void *d, int32_t sz) { (void)c; (void)d; (void)sz; ++writes; snprintf(written, sizeof written, "%s", p); return write_ok; }

static void names(void *c, int32_t id, char *out, int n) {
    (void)c;
    snprintf(out, (size_t)n, "%s", id == 0x2775 ? " <None> " : "Random");
}
static uint8_t idx_buf[2048];
static AoeOutcomeStream idx_stream;
static size_t idx_size;
static AoeOutcomeStream *idx_open_read(void *c, const char *p) {
    (void)c;
    if (strcmp(p, "list.dat") != 0) return NULL;
    idx_stream.data = idx_buf; idx_stream.size = idx_size; idx_stream.pos = 0; idx_stream.failed = 0;
    return &idx_stream;
}
static AoeOutcomeStream *idx_open_write(void *c, const char *p) {
    (void)c;
    if (strcmp(p, "list.dat") != 0) return NULL;
    idx_stream.data = idx_buf; idx_stream.size = sizeof idx_buf; idx_stream.pos = 0; idx_stream.failed = 0;
    return &idx_stream;
}
static void idx_close(void *c, AoeOutcomeStream *s) { (void)c; if (s->size == sizeof idx_buf) idx_size = s->pos; }


static int ent_obj_a, ent_obj_b, ent_finds;
static void *ent_find(void *c, void *w, int16_t id) { (void)c; (void)w; ++ent_finds; return id == 5 ? (void *)&ent_obj_a : id == 6 ? (void *)&ent_obj_b : NULL; }
static int16_t ent_tag(void *c, void *o) { (void)c; return o == &ent_obj_a ? 40 : 41; }

static void test_scenario_entries(void) {
    AoeScenarioEntriesOpsRor ops = {NULL, ent_find, ent_tag};
    AoeScenarioEntriesRor e, r;
    AoeScenarioEntryRor *a = calloc(1, sizeof *a), *b = calloc(1, sizeof *b);
    uint8_t buf[256];
    AoeOutcomeStream s = {0};
    int world = 0;
    aoe_scenario_entries_init_ror(&e, &world);
    assert(e.world == &world && e.head == NULL && e.count == 0 && e.v10 == -1.0f);
    a->v0 = 0x11223344; a->v4 = 7; a->v6 = -3; a->v8 = 9; a->vc = 1; a->v10 = 2; a->v14 = 3;
    a->v18 = 4; a->id1 = 5; a->object1 = &ent_obj_a; a->id2 = 6; a->object2 = NULL;
    b->id1 = 5; b->object1 = NULL; b->id2 = 6; b->object2 = &ent_obj_b;
    a->next = b; e.head = a; e.count = 2; e.v0e = 12;
    s.data = buf; s.size = sizeof buf;
    aoe_scenario_entries_write_ror(&e, &s, &ops);
    assert(!s.failed && s.pos == 8 + 2 * 30);
    assert(buf[0] == 2 && buf[2] == 12 && buf[8] == 0x44 && buf[11] == 0x11);
    assert(buf[8 + 24] == 40 && buf[8 + 28] == 0xff && buf[8 + 29] == 0xff && buf[38 + 28] == 41); /* tags */
    s.size = s.pos; s.pos = 0;
    ent_finds = 0;
    aoe_scenario_entries_read_ror(&r, &s, &world, &ops);
    assert(!s.failed && r.count == 2 && r.v0e == 12 && r.v10 == -1.0f && ent_finds == 2);
    assert(r.head->v0 == 0x11223344 && r.head->v6 == -3 && r.head->v18 == 4);
    assert(r.head->object1 == &ent_obj_a && r.head->object2 == NULL);
    assert(r.head->next->object1 == NULL && r.head->next->object2 == &ent_obj_b);
    assert(r.head->next->next == NULL);
    s.pos = 0;
    aoe_scenario_entries_read_ror(&e, &s, NULL, &ops); /* no world: no lookups */
    assert(e.head->object1 == NULL && e.head->next->object2 == NULL && ent_finds == 2);
    aoe_scenario_entries_release_ror(&r);
    assert(r.head == NULL && r.count == 0 && r.world == NULL && r.v10 == -1.0f);
    aoe_scenario_entries_release_ror(&e);
    free(a); free(b);
}

int main(void) {
    static AoeScenarioObjectRor s;
    static char desc[64], ai[3][32], city[3][32], per[3][32];
    s.description = desc;
    aoe_scenario_set_description_ror(&s, "Hello");
    assert(strcmp(desc, "Hello") == 0);
    aoe_scenario_set_description_ror(&s, NULL);
    assert(desc[0] == 0);
    aoe_scenario_set_text_ror(&s, 2, "Win");
    assert(strcmp(aoe_scenario_text_ror(&s, 2), "Win") == 0 && aoe_scenario_text_ror(&s, 2) == s.text_16c8);
    aoe_scenario_set_player_name_ror(&s, 3, "Bob");
    assert(strcmp(aoe_scenario_player_name_ror(&s, 3), "Bob") == 0);
    /* Four characters are always cut: ".cty" goes, ".ai" takes one more. */
    aoe_scenario_set_file_name_ror(ai[1], "Plan.cty", 1);
    assert(strcmp(ai[1], "Plan") == 0);
    aoe_scenario_set_file_name_ror(ai[1], "Tough.ai", 1);
    assert(strcmp(ai[1], "Toug") == 0);
    aoe_scenario_set_file_name_ror(ai[1], "Tough.ai", 0);
    assert(strcmp(ai[1], "Tough.ai") == 0);
    aoe_scenario_set_value_ror(s.value_190c, 15, 7);
    aoe_scenario_set_value_ror(s.value_190c, 16, 9);
    assert(aoe_scenario_value_ror(s.value_190c, 15) == 7 && aoe_scenario_value_ror(s.value_190c, 16) == 0);
    s.player_kind[2] = 5;
    s.player_kind[3] = 6;
    assert(aoe_scenario_player_kind_ror(&s, 2) == 5 && aoe_scenario_player_kind_ror(&s, 3) == 0 &&
           aoe_scenario_player_kind_ror(&s, 9) == 0);
    assert(aoe_scenario_version_ror() == 1.15f);

    /* Files. */
    for (int i = 0; i < 3; ++i) s.ai_name[i] = ai[i], s.city_name[i] = city[i], s.per_name[i] = per[i];
    strcpy(ai[1], "Tough");
    strcpy(city[1], "Plan");
    strcpy(per[1], "Mean");
    {
        AoeScenarioFileOpsRor o = {0, 7, str, temp, wf, "AI/"};
        assert(aoe_scenario_player_file_ror(&s, AOE_SCENARIO_FILE_AI, 1, 1, &o) == ai[1]);
        o.game_state = 4;
        assert(strcmp(aoe_scenario_player_file_ror(&s, AOE_SCENARIO_FILE_AI, 1, 1, &o), "AI/Tough.ai") == 0);
        assert(strcmp(aoe_scenario_player_file_ror(&s, AOE_SCENARIO_FILE_CITY, 1, 1, &o), "AI/Plan.cty") == 0);
        assert(strcmp(aoe_scenario_player_file_ror(&s, AOE_SCENARIO_FILE_PER, 1, 0, &o), "Mean") == 0);
        /* Embedded data goes to a temp file. */
        s.per_data[1] = "x";
        s.per_size[1] = 1;
        assert(strcmp(aoe_scenario_player_file_ror(&s, AOE_SCENARIO_FILE_PER, 1, 1, &o), "T/temp_AiRules1.tmp") == 0 && writes == 1);
        temp_ok = 0;
        write_ok = 0;
        assert(strcmp(aoe_scenario_player_file_ror(&s, AOE_SCENARIO_FILE_PER, 1, 1, &o), "AI/Mean.per") == 0 &&
               strcmp(written, ".temp_AiRules1.tmp") == 0);
        /* "Random" (tested on the AI name for all three) keeps the plain name. */
        strcpy(ai[1], "Random");
        assert(strcmp(aoe_scenario_player_file_ror(&s, AOE_SCENARIO_FILE_CITY, 1, 1, &o), "Plan") == 0);
    }
    /* The info record round trip. */
    {
        AoeScenarioRecordRor r, back;
        uint8_t buf[64];
        AoeOutcomeStream out = {buf, sizeof buf, 0, 0}, in = {buf, sizeof buf, 0, 0};
        aoe_scenario_record_init_ror(&r, "abc", 1234);
        assert(r.version == 2 && r.stamp == 1234 && aoe_scenario_record_size_ror(&r) == 16);
        aoe_scenario_record_write_ror(&r, &out);
        assert(out.pos == 20 && buf[0] == 16 && buf[4] == 2 && buf[12] == 4 && buf[16] == 'a');
        aoe_scenario_record_read_ror(&back, &in);
        assert(back.version == 2 && back.stamp == 1234 && strcmp(back.text, "abc") == 0 && !back.error);
        aoe_scenario_record_free_ror(&r);
        aoe_scenario_record_free_ror(&back);
        aoe_scenario_record_init_ror(&r, NULL, 0);
        assert(aoe_scenario_record_size_ror(&r) == 12 && !r.text);
    }
    puts("scenario object ok");
    {
        /* Defaults, messages and counts (0x004755b0, 0x00476b40, ...). */
        static AoeScenarioObjectRor d;
        memset(&d, 0, sizeof d);
        assert(aoe_scenario_reset_ror(&d, names, NULL));
        assert(strcmp(d.text_17a8, " <None> ") == 0 && strcmp(d.ai_name[15], "Random") == 0);
        assert(strcmp(d.city_name[0], " <None> ") == 0 && strcmp(d.per_name[3], "Random") == 0);
        assert(d.description && d.description[0] == 0 && d.message[2] == NULL);
        aoe_scenario_set_message_ror(&d, "win", 2);
        aoe_scenario_set_message_ror(&d, "lose", 2);
        aoe_scenario_set_message_ror(&d, "x", 7);
        assert(strcmp(aoe_scenario_message_ror(&d, 2), "lose") == 0 && aoe_scenario_message_ror(&d, 5) == NULL);
        aoe_scenario_append_message_ror(&d, "ab");
        aoe_scenario_append_message_ror(&d, "cd");
        assert(strcmp(aoe_scenario_message_ror(&d, 0), "abcd") == 0);
        d.value_194c[1] = 1; d.value_194c[4] = 1; d.value_18cc[4] = 1; d.value_18cc[2] = 1;
        assert(aoe_scenario_active_players_ror(&d) == 2 && aoe_scenario_human_players_ror(&d) == 1);
        aoe_scenario_release_ror(&d);
        assert(d.ai_name[0] == NULL && d.message[0] == NULL);
    }
    {
        /* The scenario index file. */
        AoeScenarioIndexRor x, y;
        AoeScenarioIndexOpsRor iops = {NULL, idx_open_read, idx_close, idx_open_write, idx_close, NULL};
        memset(&x, 0, sizeof x);
        snprintf(x.path, sizeof x.path, "list.dat");
        x.count = 2;
        x.entries = calloc(2, sizeof *x.entries);
        x.entries[0].id = 7;
        snprintf(x.entries[0].name, sizeof x.entries[0].name, "one.scn");
        x.entries[0].record = calloc(1, sizeof *x.entries[0].record);
        aoe_scenario_record_init_ror(x.entries[0].record, "desc", 99);
        x.entries[1].id = 8;
        snprintf(x.entries[1].name, sizeof x.entries[1].name, "two.scn");
        x.entries[1].record = calloc(1, sizeof *x.entries[1].record);
        aoe_scenario_record_init_ror(x.entries[1].record, "", 5);
        aoe_scenario_index_close_ror(&x, &iops); /* saves, then frees */
        assert(x.count == 0 && x.entries == NULL);
        assert(aoe_scenario_index_load_ror(&y, "list.dat", &iops) && y.count == 2);
        assert(strcmp(aoe_scenario_index_name_ror(&y, 1), "two.scn") == 0 && y.entries[0].id == 7);
        assert(strcmp(aoe_scenario_index_record_ror(&y, 0)->text, "desc") == 0);
        assert(aoe_scenario_index_name_ror(&y, 2) == NULL && aoe_scenario_index_record_ror(&y, -1) == NULL);
        snprintf(y.path, sizeof y.path, "other.dat");
        aoe_scenario_index_close_ror(&y, &iops);
        assert(!aoe_scenario_index_load_ror(&y, "missing", &iops) && y.count == 0);
    }
    test_scenario_entries();
    return 0;
}
