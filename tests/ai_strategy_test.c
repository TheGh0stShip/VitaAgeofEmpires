/* Patched RoR 1.0a strategy AI, strategic numbers and resource AI
   (0x004c6880..0x004cc5c0). */
#include "ai_strategy.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* An in-memory text file for the SN loader. */
static const char *file_text;
static size_t file_pos;
static char removed[64];
static char opened[64];
static void *f_open(void *c, const char *path) {
    (void)c;
    snprintf(opened, sizeof opened, "%s", path);
    file_pos = 0;
    return strcmp(path, "missing") ? (void *)&file_pos : NULL;
}
static int f_gets(void *c, void *f, char *line, int size) {
    size_t n = 0;
    (void)c; (void)f;
    if (!file_text[file_pos]) return 0;
    while (file_text[file_pos] && n + 1 < (size_t)size) {
        char ch = file_text[file_pos++];
        line[n++] = ch;
        if (ch == '\n') break;
    }
    line[n] = 0;
    return 1;
}
static void f_close(void *c, void *f) { (void)c; (void)f; }
static void f_remove(void *c, const char *path) {
    (void)c;
    snprintf(removed, sizeof removed, "%s", path);
}

static int32_t tac[AOE_AI_SN_COUNT], tac_sets;
static void tac_set(void *c, int32_t i, int32_t v) {
    (void)c;
    tac[i] = v;
    ++tac_sets;
}

static int has_condition = 1;
static int first_condition(void *c, AoeAiVictoryTargetRor *t) {
    (void)c;
    if (!has_condition) return 0;
    memset(t, 0, sizeof *t);
    t->kind = 3;
    t->has_unit_28 = 1;
    t->unit_28_id = 77;
    t->unit_28_type = 109;
    t->area[0] = 1.5f; t->area[1] = 2.5f; t->area[2] = 3.5f; t->area[3] = 4.5f;
    t->field_1c = 5;
    t->field_20 = 6;
    return 1;
}

static double range_value;
static int range_asked;
static double range(void *c) { (void)c; ++range_asked; return range_value; }

int main(void) {
    static AoeAiStrategicNumbersRor sn;
    static AoeAiStrategyRor ai, back;
    static uint8_t buf[4096];
    AoeAiTextFileOpsRor files = {NULL, f_open, f_gets, f_close, f_remove};
    AoeAiStrategyOpsRor ops = {NULL, first_condition, tac_set, &files};
    AoeOutcomeStream out = {buf, sizeof buf, 0, 0}, in;
    AoeAiIntListRor list = {0};
    const int16_t *numbers;
    int32_t i;

    /* Growable list. */
    assert(aoe_ai_int_list_add_unique_ror(&list, 5));
    assert(!aoe_ai_int_list_add_unique_ror(&list, 5));
    assert(aoe_ai_int_list_add_unique_ror(&list, 6) && list.count == 2 &&
           list.capacity == 2);
    assert(aoe_ai_int_list_contains_ror(&list, 6) &&
           !aoe_ai_int_list_contains_ror(&list, 7));
    assert(aoe_ai_int_list_at_ror(&list, 4) && list.capacity == 5 &&
           list.count == 2 && list.data[1] == 6);
    aoe_ai_int_list_release_ror(&list);
    assert(!list.data && !list.count);

    /* Play categories. */
    assert(aoe_ai_play_category_ror("Hand") == 0);
    assert(aoe_ai_play_category_ror("Ranged") == 1);
    assert(aoe_ai_play_category_ror("Cavalry") == 2);
    assert(aoe_ai_play_category_ror("Legion") == 3);
    assert(aoe_ai_play_category_ror("Priest") == 4);
    assert(aoe_ai_play_category_ror("Siege") == 5);
    assert(aoe_ai_play_category_ror("Archer") == -1);
    assert(aoe_ai_ror_play_any_kind_ror("Any") == 5);
    assert(aoe_ai_ror_play_speed_kind_ror("Stationary") == 1);
    assert(aoe_ai_play_unit_category_ror(0x12, range, NULL) == 4);
    assert(aoe_ai_play_unit_category_ror(0x23, range, NULL) == 2 && !range_asked);
    range_value = 1.0;
    assert(aoe_ai_play_unit_category_ror(4, range, NULL) == 0 && range_asked == 1);
    range_value = 7.0;
    assert(aoe_ai_play_unit_category_ror(4, range, NULL) == 1);

    /* Resource AI. */
    {
        AoeAiResourceRor res, res2;
        AoeAiCountersRor costs;
        float have[4] = {100.9f, 50.0f, 0.0f, -3.5f};
        aoe_ai_resource_init_ror(&res, 2, 4, NULL);
        assert(res.module.id == 0x3ef && !strcmp(res.module.name, "Resource AI") &&
               res.module.player == 2 && res.resource_count == 4);
        assert(aoe_ai_resource_amount_ror(&res, have, 0) == 100);
        assert(aoe_ai_resource_amount_ror(&res, have, 3) == -3);
        assert(aoe_ai_resource_amount_ror(&res, have, 4) == -1);
        aoe_ai_counters_init_ror(&costs, 4);
        aoe_ai_counters_set_ror(&costs, 0, 100);
        aoe_ai_counters_set_ror(&costs, 1, 50);
        aoe_ai_counters_set_ror(&costs, 3, -4);
        assert(aoe_ai_resource_can_afford_ror(&res, have, &costs));
        aoe_ai_counters_set_ror(&costs, 0, 101);
        assert(!aoe_ai_resource_can_afford_ror(&res, have, &costs));
        assert(!aoe_ai_resource_can_afford_ror(&res, have, NULL));
        aoe_ai_counters_init_ror(&costs, 3);
        assert(!aoe_ai_resource_can_afford_ror(&res, have, &costs));
        assert(!strcmp(aoe_ai_resource_name_ror(2), "Stone") &&
               !strcmp(aoe_ai_resource_name_ror(9), "Unknown"));
        out.pos = 0;
        assert(aoe_ai_resource_save_ror(&res, &out) && out.pos == 4);
        in = (AoeOutcomeStream){buf, out.pos, 0, 0};
        assert(aoe_ai_resource_load_ror(&res2, 3, &in, NULL));
        assert(!strcmp(res2.module.name, "ResAI") && res2.resource_count == 4 &&
               res2.module.player == 3);
    }

    /* Defaults, get and set. */
    aoe_ai_sn_init_ror(&sn);
    assert(sn.values[0] == 34 && sn.values[2] == 66 && sn.values[89] == -25 &&
           sn.values[190] == 3000 && sn.values[225] == 1);
    aoe_ai_sn_set_ror(&sn, 226, 9);
    aoe_ai_sn_set_ror(&sn, 3, 9);
    assert(aoe_ai_sn_get_ror(&sn, 3) == 9 && aoe_ai_sn_get_ror(&sn, -1) == -1);

    /* File loading: comments and markers skip; EN ends; a line that does
       not scan sets the previous pair again. */
    file_text = "// comment\n DE ignored\nVC 1 2\n10 20\n/x\n11 21\n"
                "300 5\nEN\n12 22\n";
    aoe_ai_sn_set_ror(&sn, 10, 0);
    assert(aoe_ai_sn_load_file_ror(&sn, "strat.tmp", &files));
    assert(sn.values[10] == 20 && sn.values[11] == 21 && sn.values[12] != 22);
    assert(!strcmp(removed, "strat.tmp"));
    removed[0] = 0;
    assert(!aoe_ai_sn_load_file_ror(&sn, "NONE", &files));
    assert(!aoe_ai_sn_load_file_ror(&sn, "", &files));
    assert(!aoe_ai_sn_load_file_ror(&sn, "missing", &files));
    file_text = "x\n";
    assert(aoe_ai_sn_load_file_ror(&sn, "plain.per", &files) && !removed[0]);

    /* Category kinds and the copy into the tactical AI. */
    assert(aoe_ai_sn_category_kind_ror(1) == -2);
    assert(aoe_ai_sn_category_kind_ror(3) == 0);
    assert(aoe_ai_sn_category_kind_ror(15) == 0);
    assert(aoe_ai_sn_category_kind_ror(49) == -2);
    assert(aoe_ai_sn_category_kind_ror(50) == 0);
    assert(aoe_ai_sn_category_numbers_ror(1, &numbers) == 54 &&
           numbers[8] == 225 && numbers[53] == 204);
    assert(aoe_ai_sn_category_numbers_ror(43, &numbers) == 24);
    aoe_ai_sn_init_ror(&sn);
    tac_sets = 0;
    assert(!aoe_ai_sn_apply_category_ror(&sn, 13, 2, tac_set, NULL) && !tac_sets);
    assert(!aoe_ai_sn_apply_category_ror(&sn, 0, 3, tac_set, NULL));
    assert(aoe_ai_sn_apply_category_ror(&sn, 12, 2, tac_set, NULL) &&
           tac_sets == 11 && tac[143] == sn.values[143]);
    {
        int total = 0;
        for (i = 0; i < 50; ++i) total += aoe_ai_sn_category_numbers_ror(i, NULL);
        assert(total == 210);
    }

    /* Difficulty scaling. */
    aoe_ai_sn_init_ror(&sn);
    sn.values[36] = 7;
    sn.values[16] = 1;
    sn.values[26] = 0;
    aoe_ai_sn_scale_difficulty_ror(&sn, 2, 0);
    assert(sn.values[36] == 4);  /* 7 * 0.6f = 4.2 */
    assert(sn.values[16] == 1);  /* 0.6 truncates to 0, then 1 */
    assert(sn.values[26] == 0);  /* not positive: left */
    assert(sn.values[46] == 30 + 60 && sn.values[48] == 30 + 60);
    assert(sn.values[104] == 240 && sn.values[88] == 5 && sn.values[103] == 1);
    aoe_ai_sn_init_ror(&sn);
    aoe_ai_sn_scale_difficulty_ror(&sn, 1, 0);
    assert(!memcmp(sn.values, aoe_ai_sn_defaults_ror, 103 * 4) &&
           sn.values[103] == 1 && sn.values[104] == 0);
    aoe_ai_sn_scale_difficulty_ror(&sn, 1, 1);
    assert(sn.values[46] == 60 && sn.values[19] == 40 && sn.values[88] == 4);
    aoe_ai_sn_init_ror(&sn);
    aoe_ai_sn_scale_difficulty_ror(&sn, 4, 0);
    assert(sn.values[104] == 1260 && sn.values[103] == 0 &&
           sn.values[20] == 10);
    aoe_ai_sn_init_ror(&sn);
    aoe_ai_sn_scale_difficulty_ror(&sn, 3, 1);
    assert(sn.values[104] == 360 && sn.values[19] == 20);
    aoe_ai_sn_init_ror(&sn);
    aoe_ai_sn_scale_difficulty_ror(&sn, 5, 1);
    assert(!memcmp(sn.values, aoe_ai_sn_defaults_ror, sizeof sn.values));

    /* Strategy AI. */
    aoe_ai_strategy_ai_init_ror(&ai, 2, NULL);
    assert(ai.module.id == 0x3f0 && !strcmp(ai.module.name, "Strategy AI"));
    assert(ai.victory_kind == -1 && ai.target_type == -1 &&
           ai.target_point[1] == -1.0f && ai.numbers.values[0] == 34 &&
           !ai.sn_file[0]);
    aoe_ai_strategy_ai_set_sn_ror(&ai, 7, 70);
    assert(aoe_ai_strategy_ai_get_sn_ror(&ai, 7) == 70);
    file_text = "7 71\n";
    aoe_ai_strategy_ai_load_sn_file_ror(&ai, "a.per", &files);
    assert(!strcmp(aoe_ai_strategy_ai_sn_file_ror(&ai), "a.per") &&
           aoe_ai_strategy_ai_get_sn_ror(&ai, 7) == 71);

    tac_sets = 0;
    assert(aoe_ai_strategy_ai_update_ror(&ai, &ops) == 1);
    assert(ai.victory_kind == 3 && ai.target_id == 77 && ai.target_type == 109 &&
           ai.second_id == -1 && ai.target_point[0] == 1.5f &&
           ai.target_point[2] == 1.0f && ai.second_point[1] == 4.5f &&
           ai.field_22c == 5 && ai.update_count == 0);
    assert(ai.categories.count == 37 && ai.applied.count == 37 &&
           ai.rejected.count == 0 && tac_sets == 210 && tac[0] == 34);
    assert(ai.categories.data[0] == 1 && ai.categories.data[12] == 25 &&
           ai.categories.data[13] == 26 && ai.categories.data[36] == 49);
    tac_sets = 0;
    assert(aoe_ai_strategy_ai_update_ror(&ai, &ops) == 1 && tac_sets == 0);
    /* No condition: kind 11, everything applies again. */
    has_condition = 0;
    assert(aoe_ai_strategy_ai_update_ror(&ai, &ops) && ai.victory_kind == 11 &&
           tac_sets == 210 && ai.target_id == 77);
    /* A kind outside 0..12 records rejected categories. */
    aoe_ai_strategy_ai_set_victory_kind_ror(&ai, 13, &ops);
    assert(ai.rejected.count == 37 && ai.applied.count == 37);

    /* Save and load. */
    ai.difficulty = 2;
    ai.second_type = 44;
    out.pos = 0;
    assert(aoe_ai_strategy_ai_save_ror(&ai, &out));
    assert(out.pos == 4 * 4 + 32 + 4 * 4 + 4 + 5 + 3 * 4 + 3 * 37 * 4);
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    file_text = "36 7\n";
    assert(aoe_ai_strategy_ai_load_ror(&back, 4, 1.5f, &in, 0, &files, NULL));
    assert(in.pos == out.pos && back.module.player == 4);
    assert(back.victory_kind == 13 && back.target_id == 77 &&
           back.target_type == -1 && back.second_type == 44 &&
           back.difficulty == 2 && !strcmp(back.sn_file, "a.per"));
    assert(back.numbers.values[36] == 4 && back.numbers.values[46] == 90);
    assert(back.categories.count == 37 && back.applied.count == 37 &&
           back.rejected.count == 37 && back.rejected.data[5] == 16);
    aoe_ai_strategy_ai_release_ror(&back);
    aoe_ai_strategy_ai_release_ror(&ai);
    assert(!ai.categories.data);
    puts("ai strategy ok");
    return 0;
}
