/* Patched RoR 1.0a scenario object settings (0x00475800..0x00476250). */
#include "scenario_object.h"

#include <assert.h>
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
    return 0;
}
