#include "world_ror_scenario.h"

#include <string.h>

/* The version gates' constants: 0x00545e4c 1.02, 0x00545e60 1.03,
   0x00545e80 1.13, 0x0054a3d8 1.05, 0x0054a3dc 1.12, 0x0054a3e0 1.04
   (f32), and the exact 1.01 pattern 0x3f8147ae. */
#define V102 1.02f
#define V103 1.03f
#define V104 1.04f
#define V105 1.05f
#define V112 1.12f
#define V113 1.13f
#define V101_BITS 0x3f8147aeu

/* fld [+0x1888]; fcomp c; test ah, 0x41; je/jne: true only when the
   version is ordered and greater (NaN counts as not above). */
static int above(float v, float c) { return v > c; }

static void read_i32s(AoeOutcomeStream *s, int32_t *out, int n) {
    int i;
    for (i = 0; i < n; ++i) out[i] = aoe_stream_i32(s);
}

static void read_marker(AoeOutcomeStream *s) {
    (void)aoe_stream_i32(s); /* 0x00506900's local, discarded */
}

/* 0x00506900 */
float aoe_ror_scenario_read_ror(AoeScenarioInfoRor *info, AoeScenarioObjectRor *obj,
                                AoeOutcomeStream *s, void *world,
                                const AoeRorScenarioReadOpsRor *ops) {
    float v;
    uint32_t bits;
    int p, c, i;
    if (!info || !obj || !s || !ops || !ops->base_read) return 0.0f;
    v = ops->base_read(ops->context, s, world, obj); /* 0x474d40 */
    memcpy(&bits, &v, 4);
    aoe_scenario_reset_goals_ror(info);     /* 0x00506f40 */
    aoe_scenario_reset_diplomacy_ror(info); /* 0x00506fb0 */
    aoe_scenario_enable_all_techs_ror(info); /* 0x00506e40 */
    info->victory_type = 0;
    info->score = 900;
    info->time_limit = 9000;
    info->all_custom_required = 0;
    info->selected = 0;
    if (!above(v, V113))
        for (p = 0; p < AOE_SCENARIO_PLAYERS_ROR; ++p) {
            const uint8_t *name = aoe_stream_take(s, 0x100);
            if (name) memcpy(obj->player_name[p], name, 0x100);
        }
    if (above(v, V113)) {
        for (p = 0; p < AOE_SCENARIO_PLAYERS; ++p)
            read_i32s(s, info->start_resources[p], 4);
    } else {
        for (p = 0; p < AOE_SCENARIO_PLAYERS; ++p) {
            obj->value_194c[p] = aoe_stream_i32(s);
            read_i32s(s, info->start_resources[p], 4);
            obj->value_18cc[p] = aoe_stream_i32(s);
            obj->value_190c[p] = aoe_stream_i32(s);
            obj->player_kind[p] = aoe_stream_i32(s);
        }
    }
    if (above(v, V102)) read_marker(s);
    info->custom_conquest = aoe_stream_i32(s);
    info->ruins = aoe_stream_i32(s);
    info->artifacts = aoe_stream_i32(s);
    info->discoveries = aoe_stream_i32(s);
    info->explore = aoe_stream_i32(s);
    info->gold = aoe_stream_i32(s);
    info->all_custom_required = aoe_stream_i32(s);
    /* test ah, 1: below 1.13 or unordered takes the defaults. */
    if (v >= V113) {
        info->victory_type = aoe_stream_i32(s);
        info->score = aoe_stream_i32(s);
        info->time_limit = aoe_stream_i32(s);
    } else {
        info->victory_type = 4;
        info->score = 900;
        info->time_limit = 9000;
    }
    for (p = 0; p < AOE_SCENARIO_PLAYERS; ++p)
        read_i32s(s, info->diplomacy[p], AOE_SCENARIO_PLAYERS);
    for (p = 0; p < AOE_SCENARIO_PLAYERS; ++p)
        for (c = 0; c < AOE_SCENARIO_CONDITIONS; ++c) {
            AoeScenarioConditionRor *r = &info->conditions[p][c];
            r->object_class = aoe_stream_i32(s);
            r->all_flag = aoe_stream_i32(s);
            r->player = aoe_stream_i32(s);
            r->field_0c = aoe_stream_i32(s);
            for (i = 0; i < 4; ++i) r->area[i] = aoe_stream_f32(s);
            r->type = aoe_stream_i32(s);
            r->amount = aoe_stream_i32(s);
            r->subtype = aoe_stream_i32(s);
            r->unit_ref = aoe_stream_i32(s);
            r->target_ref = aoe_stream_i32(s);
            read_i32s(s, r->field_34, 2);
        }
    if (above(v, V102)) read_marker(s);
    if (bits == V101_BITS)
        for (i = 0; i < 16; ++i) read_i32s(s, info->allied_victory, AOE_SCENARIO_PLAYERS);
    if (v >= V102) read_i32s(s, info->allied_victory, AOE_SCENARIO_PLAYERS);
    if (above(v, V103))
        for (p = 0; p < AOE_SCENARIO_PLAYERS; ++p) read_i32s(s, info->tech_toggles[p], 20);
    if (above(v, V104)) {
        if (v >= V112) {
            read_i32s(s, info->options, 3);
        } else {
            info->options[0] = aoe_stream_i32(s);
            info->options[1] = 0;
            info->options[2] = 0;
        }
    }
    if (above(v, V105)) read_i32s(s, info->starting_age, AOE_SCENARIO_PLAYERS);
    if (above(v, V102)) read_marker(s);
    info->custom_conquest = obj->field_c;
    info->conquest = obj->field_c;
    return v;
}
