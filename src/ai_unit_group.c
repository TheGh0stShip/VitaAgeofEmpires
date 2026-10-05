#include "ai_unit_group.h"

#include <string.h>

#include "aoe_ftol.h"
#include "aoe_stream.h"
#include "le_bytes.h"

void aoe_ai_unit_group_set_location_ror(float out[4], float x, float y,
                                        float z) {
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

void aoe_ai_unit_group_init_ror(AoeAiUnitGroupRor *g) {
    int i;
    if (!g) return;
    memset(g, 0, sizeof(*g));
    g->id = g->task = g->field_18 = -1;
    g->commander = g->field_168 = g->field_16c = -1;
    g->field_184 = g->field_188 = -1;
    g->field_1c0 = g->field_1c4 = g->field_1c8 = -1;
    g->field_1d0 = -1;
    g->field_320 = g->field_324 = -1;
    aoe_ai_unit_group_set_location_ror(g->loc_18c, -1.0f, -1.0f, -1.0f);
    aoe_ai_unit_group_set_location_ror(g->loc_19c, -1.0f, -1.0f, -1.0f);
    aoe_ai_unit_group_set_location_ror(g->loc_1ac, -1.0f, -1.0f, -1.0f);
    aoe_ai_unit_group_set_location_ror(g->location, -1.0f, -1.0f, -1.0f);
    for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i) g->members[i] = g->values[i] = -1;
    for (i = 0; i < 20; ++i) g->list_2d0[i] = -1;
}

int aoe_ai_unit_group_add_ror(AoeAiUnitGroupRor *g, int32_t id,
                              const AoeAiUnitGroupOpsRor *ops) {
    int i;
    if (!g) return 0;
    for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i) {
        if (g->members[i] != -1) continue;
        g->members[i] = id;
        g->values[i] = -1;
        ++g->count;
        if (g->commander != -1 && ops && ops->follow)
            ops->follow(ops->context, g->commander, id, 4.0f);
        if (ops && ops->game_time) g->added_time = ops->game_time(ops->context);
        return 1;
    }
    return 0;
}

static void drop_slot(AoeAiUnitGroupRor *g, int slot,
                      const AoeAiUnitGroupOpsRor *ops) {
    int32_t id = g->members[slot];
    if (id == g->commander)
        aoe_ai_unit_group_set_commander_ror(g, -1, ops);
    else if (g->commander != -1 && ops && ops->unfollow)
        ops->unfollow(ops->context, g->commander, id);
    g->members[slot] = -1;
    g->values[slot] = -1;
    --g->count;
}

int aoe_ai_unit_group_remove_ror(AoeAiUnitGroupRor *g, int32_t id,
                                 const AoeAiUnitGroupOpsRor *ops) {
    int i;
    if (!g) return 0;
    for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i)
        if (g->members[i] == id) {
            drop_slot(g, i, ops);
            return 1;
        }
    return 0;
}

int aoe_ai_unit_group_remove_slot_ror(AoeAiUnitGroupRor *g, int32_t slot,
                                      const AoeAiUnitGroupOpsRor *ops) {
    if (!g || slot < 0 || slot >= AOE_AI_GROUP_SLOTS) return 0;
    drop_slot(g, slot, ops);
    return 1;
}

void aoe_ai_unit_group_clear_ror(AoeAiUnitGroupRor *g,
                                 const AoeAiUnitGroupOpsRor *ops) {
    int i;
    if (!g) return;
    aoe_ai_unit_group_set_commander_ror(g, -1, ops);
    g->count = 0;
    for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i) g->members[i] = g->values[i] = -1;
}

int aoe_ai_unit_group_prune_ror(AoeAiUnitGroupRor *g,
                                const AoeAiUnitGroupOpsRor *ops) {
    int i;
    if (!g) return 0;
    for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i) {
        int32_t id = g->members[i];
        if (id == -1) continue;
        if (ops && ops->present && ops->present(ops->context, id)) continue;
        if (id == g->commander)
            aoe_ai_unit_group_set_commander_ror(g, -1, ops);
        else if (g->commander != -1 && ops && ops->unfollow)
            ops->unfollow(ops->context, g->commander, id);
        if (ops && ops->dismiss) ops->dismiss(ops->context, id, 100);
        g->members[i] = -1;
        g->values[i] = -1;
        --g->count;
    }
    return 1;
}

int aoe_ai_unit_group_contains_ror(const AoeAiUnitGroupRor *g, int32_t id) {
    int i;
    if (!g) return 0;
    for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i)
        if (g->members[i] == id) return 1;
    return 0;
}

int32_t aoe_ai_unit_group_count_ror(const AoeAiUnitGroupRor *g) {
    return g ? g->count : 0;
}

static int nth_slot(const AoeAiUnitGroupRor *g, int32_t n) {
    int i, seen = -1;
    for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i) {
        if (n + 1 <= seen) return -1;
        if (g->members[i] != -1) ++seen;
        if (seen == n) return i;
    }
    return -1;
}

int32_t aoe_ai_unit_group_member_ror(const AoeAiUnitGroupRor *g, int32_t n) {
    int slot = g ? nth_slot(g, n) : -1;
    return slot < 0 ? -1 : g->members[slot];
}

int32_t aoe_ai_unit_group_value_ror(const AoeAiUnitGroupRor *g, int32_t n) {
    int slot = g ? nth_slot(g, n) : -1;
    return slot < 0 ? -1 : g->values[slot];
}

void aoe_ai_unit_group_set_commander_ror(AoeAiUnitGroupRor *g, int32_t id,
                                         const AoeAiUnitGroupOpsRor *ops) {
    int32_t ids[AOE_AI_GROUP_SLOTS];
    int i, n = 0;
    if (!g) return;
    if (g->commander != -1 && ops && ops->release)
        ops->release(ops->context, g->commander);
    g->commander = id;
    if (id == -1) return;
    for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i)
        if (g->members[i] != -1) ids[n++] = g->members[i];
    if (ops && ops->lead) ops->lead(ops->context, id, ids, n, 4.0f);
}

int32_t aoe_ai_unit_group_hit_points_ror(const AoeAiUnitGroupRor *g,
                                         const AoeAiUnitGroupOpsRor *ops) {
    int32_t sum = 0;
    int i;
    if (!g || !ops || !ops->hit_points) return 0;
    for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i) {
        float hp;
        if (g->members[i] != -1 && ops->hit_points(ops->context, g->members[i], &hp))
            sum += aoe_ftol32(hp);
    }
    return sum;
}

void aoe_ai_unit_group_set_all_locations_ror(AoeAiUnitGroupRor *g, float x,
                                             float y, float z) {
    if (!g) return;
    aoe_ai_unit_group_set_location_ror(g->location, x, y, z);
    aoe_ai_unit_group_set_location_ror(g->loc_19c, x, y, z);
    aoe_ai_unit_group_set_location_ror(g->loc_1ac, x, y, z);
}

int aoe_ai_unit_group_in_range_ror(AoeAiUnitGroupRor *g,
                                   const AoeAiUnitGroupOpsRor *ops) {
    int i;
    if (!g) return 0;
    for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i) {
        int32_t id = g->members[i], type, task, action, range;
        float d;
        if (id == -1) continue;
        if (!ops || !ops->unit_ai ||
            !ops->unit_ai(ops->context, id, &type, &task, &action)) {
            aoe_ai_unit_group_remove_slot_ror(g, i, ops);
            continue;
        }
        d = (float)((ops->distance ? ops->distance(ops->context, id,
                                                   g->loc_19c[0],
                                                   g->loc_19c[1],
                                                   g->loc_19c[2])
                                   : 0.0) -
                    (double)1.0f);
        if (d < 0.0f) d = 0.0f;
        range = ops->sn ? ops->sn(ops->context, g->task == 0x6d ? 23 : 41)
                        : -1;
        if ((double)d > (double)range) return 0;
    }
    return 1;
}

int aoe_ai_unit_group_settled_ror(AoeAiUnitGroupRor *g,
                                  const AoeAiUnitGroupOpsRor *ops,
                                  int32_t allow_600) {
    int i;
    if (!g) return 0;
    for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i) {
        int32_t id = g->members[i], type, task, action;
        if (id == -1) continue;
        if (!ops || !ops->unit_ai ||
            !ops->unit_ai(ops->context, id, &type, &task, &action)) {
            aoe_ai_unit_group_remove_slot_ror(g, i, ops);
            continue;
        }
        if (type == 0x7d) continue;
        if (allow_600 == 1 && task == 600 &&
            (g->task == 100 || g->task == 0x67))
            continue;
        if (task != -1 && action != 0x2bd) return 0;
    }
    return 1;
}

int32_t aoe_ai_unit_group_field_1cc_ror(const AoeAiUnitGroupRor *g) {
    return g->field_1cc;
}

void aoe_ai_unit_group_set_field_1cc_ror(AoeAiUnitGroupRor *g, int32_t v) {
    g->field_1cc = v;
}

void aoe_ai_unit_group_add_waypoint_ror(AoeAiUnitGroupRor *g, float x,
                                        float y) {
    uint8_t *entry;
    if (!g || g->flag_1d4 >= 14) return;
    entry = g->block_1d8 + (size_t)g->flag_1d4 * 16;
    aoe_le_put_f32(entry, x);
    aoe_le_put_f32(entry + 4, y);
    aoe_le_put_f32(entry + 8, 1.0f);
    ++g->flag_1d4;
}

int aoe_ai_unit_group_waypoint_ror(const AoeAiUnitGroupRor *g, int i,
                                   float out[3]) {
    const uint8_t *entry;
    if (!g || i < 0 || i >= 15) return 0;
    entry = g->block_1d8 + (size_t)i * 16;
    out[0] = aoe_le_f32(entry);
    out[1] = aoe_le_f32(entry + 4);
    out[2] = aoe_le_f32(entry + 8);
    return 1;
}

int32_t aoe_ai_unit_group_list_ror(const AoeAiUnitGroupRor *g, int32_t i) {
    if (!g || i < 0 || i >= 20) return -1;
    return g->list_2d0[i];
}

int aoe_ai_unit_group_list_add_ror(AoeAiUnitGroupRor *g, int32_t value) {
    if (!g || g->field_2cc >= 19) return 0;
    g->list_2d0[g->field_2cc++] = value;
    return 1;
}

static void put_loc(AoeOutcomeStream *s, const float v[4]) {
    int i;
    for (i = 0; i < 4; ++i) aoe_stream_put_f32(s, v[i]);
}

static void get_loc(AoeOutcomeStream *s, float v[4]) {
    int i;
    for (i = 0; i < 4; ++i) v[i] = aoe_stream_f32(s);
}

int aoe_ai_unit_group_save_ror(const AoeAiUnitGroupRor *g,
                               AoeOutcomeStream *out) {
    int i;
    if (!g || !out) return 0;
    aoe_stream_put_i32(out, g->id);
    aoe_stream_put_i32(out, g->type);
    for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i) {
        aoe_stream_put_i32(out, g->members[i]);
        aoe_stream_put_i32(out, g->values[i]);
    }
    aoe_stream_put_i32(out, g->count);
    aoe_stream_put_i32(out, g->field_160);
    aoe_stream_put_i32(out, g->commander);
    aoe_stream_put_i32(out, g->field_168);
    aoe_stream_put_i32(out, g->field_16c);
    put_loc(out, g->location);
    aoe_stream_put_i32(out, g->field_180);
    aoe_stream_put_i32(out, g->field_184);
    aoe_stream_put_i32(out, g->field_188);
    put_loc(out, g->loc_18c);
    put_loc(out, g->loc_19c);
    put_loc(out, g->loc_1ac);
    aoe_stream_put_i32(out, g->field_1bc);
    aoe_stream_put_i32(out, g->field_1c0);
    aoe_stream_put_i32(out, g->field_1c4);
    aoe_stream_put_i32(out, g->field_1c8);
    aoe_stream_put_i32(out, g->task);
    aoe_stream_put_i32(out, g->field_18);
    aoe_stream_put_i32(out, g->field_1cc);
    aoe_stream_put_u8(out, g->flag_1d4);
    aoe_stream_put_u8(out, g->flag_1d5);
    aoe_stream_put(out, g->block_1d8, sizeof(g->block_1d8));
    aoe_stream_put_i32(out, g->field_1d0);
    aoe_stream_put_u8(out, g->flag_2c8);
    aoe_stream_put_i32(out, g->field_2cc);
    for (i = 0; i < 20; ++i) aoe_stream_put_i32(out, g->list_2d0[i]);
    aoe_stream_put_i32(out, g->field_320);
    aoe_stream_put_i32(out, g->field_324);
    aoe_stream_put_i32(out, g->field_328);
    aoe_stream_put_i32(out, g->added_time);
    return !out->failed;
}

int aoe_ai_unit_group_load_ror(AoeAiUnitGroupRor *g, float version,
                               AoeOutcomeStream *in) {
    const uint8_t *block;
    int i;
    if (!g || !in) return 0;
    g->id = aoe_stream_i32(in);
    g->type = aoe_stream_i32(in);
    for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i) {
        g->members[i] = aoe_stream_i32(in);
        g->values[i] = aoe_stream_i32(in);
    }
    g->count = aoe_stream_i32(in);
    g->field_160 = aoe_stream_i32(in);
    g->commander = aoe_stream_i32(in);
    g->field_168 = aoe_stream_i32(in);
    g->field_16c = aoe_stream_i32(in);
    get_loc(in, g->location);
    g->field_180 = aoe_stream_i32(in);
    g->field_184 = aoe_stream_i32(in);
    g->field_188 = aoe_stream_i32(in);
    get_loc(in, g->loc_18c);
    get_loc(in, g->loc_19c);
    get_loc(in, g->loc_1ac);
    g->field_1bc = aoe_stream_i32(in);
    g->field_1c0 = aoe_stream_i32(in);
    g->field_1c4 = aoe_stream_i32(in);
    g->field_1c8 = aoe_stream_i32(in);
    g->task = aoe_stream_i32(in);
    g->field_18 = aoe_stream_i32(in);
    g->field_1cc = aoe_stream_i32(in);
    g->flag_1d4 = aoe_stream_u8(in);
    g->flag_1d5 = aoe_stream_u8(in);
    block = aoe_stream_take(in, sizeof(g->block_1d8));
    if (block) memcpy(g->block_1d8, block, sizeof(g->block_1d8));
    g->field_1d0 = aoe_stream_i32(in);
    g->flag_2c8 = version < 6.94f ? 0 : aoe_stream_u8(in);
    if (version >= 7.13f) {
        g->field_2cc = aoe_stream_i32(in);
        for (i = 0; i < 20; ++i) g->list_2d0[i] = aoe_stream_i32(in);
        g->field_320 = aoe_stream_i32(in);
        g->field_324 = aoe_stream_i32(in);
    }
    if (version >= 7.14f) g->field_328 = aoe_stream_i32(in);
    if (version >= 7.2f) g->added_time = aoe_stream_i32(in);
    return !in->failed;
}
