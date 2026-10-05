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

/* 0x004cd2d0 helper: a live member's unit AI task and its value; a gone
   member is dropped by slot (0x004ccd90). */
static int update_live(AoeAiUnitGroupRor *g, int slot,
                       const AoeAiUnitGroupOpsRor *ops, int32_t *task) {
    int32_t type_id = 0, action = 0;
    float hp = 0.0f;
    *task = -1;
    if (!ops->unit_ai ||
        !ops->unit_ai(ops->context, g->members[slot], &type_id, task,
                      &action)) {
        aoe_ai_unit_group_remove_slot_ror(g, slot, ops);
        return 0;
    }
    if (ops->hit_points) ops->hit_points(ops->context, g->members[slot], &hp);
    g->values[slot] = aoe_ftol32(hp);
    return 1;
}

static float update_type_28(const AoeAiUnitGroupOpsRor *ops, int32_t id) {
    return ops->type_28 ? ops->type_28(ops->context, id) : 0.0f;
}

static int32_t update_sn(const AoeAiUnitGroupOpsRor *ops, int32_t i) {
    return ops->sn ? ops->sn(ops->context, i) : 0;
}

/* 0x004cd2d0: distance less 1.0, not below 0. */
static float update_gap(const AoeAiUnitGroupOpsRor *ops, int32_t id,
                        const float loc[4]) {
    float d = ops->distance ? (float)ops->distance(ops->context, id, loc[0],
                                                   loc[1], loc[2])
                            : 0.0f;
    d -= 1.0f;
    return d < 0.0f ? 0.0f : d;
}

/* 0x004cd2d0: a non-commander follows the commander (action 0x2bd at
   half its type's +0x28, value 99). */
static void update_follow(AoeAiUnitGroupRor *g, int32_t id,
                          const AoeAiUnitGroupOpsRor *ops) {
    if (ops->order_2bd)
        ops->order_2bd(ops->context, id, g->commander,
                       update_type_28(ops, id) * 0.5f, 99);
}

/* 0x004cd2d0 tasks 2, 0x15 and 0x14 for one member. `first` gates the
   area record of tasks 2 / 0x15; round-up (0x14) has none and orders the
   attack with flag 1. */
static void update_attack_member(AoeAiUnitGroupRor *g, int32_t task,
                                 int32_t id, int *first,
                                 const AoeAiUnitGroupOpsRor *ops) {
    int32_t owner;
    if (g->field_18 == 0 || g->field_18 == 4) {
        if (ops->order_2bd)
            ops->order_2bd(ops->context, id, g->field_184, 2.0f, 100);
        return;
    }
    if (!ops->present || !ops->present(ops->context, g->field_184)) return;
    owner = ops->owner ? ops->owner(ops->context, g->field_184) : 0;
    if (task == 0x14) {
        if (ops->order_attack)
            ops->order_attack(ops->context, id, g->loc_18c[0], g->loc_18c[1],
                              g->field_184, owner, g->block_1d8, g->flag_1d4,
                              g->commander, 1);
        return;
    }
    if (*first) {
        int32_t lo[2], hi[2];
        lo[0] = aoe_ftol32(g->loc_18c[0]) - 7;
        lo[1] = aoe_ftol32(g->loc_18c[1]) - 7;
        if (ops->clamp_tile) ops->clamp_tile(ops->context, lo);
        hi[0] = aoe_ftol32(g->loc_18c[0]) + 7;
        hi[1] = aoe_ftol32(g->loc_18c[1]) + 7;
        if (ops->clamp_tile) ops->clamp_tile(ops->context, hi);
        if (g->flag_1d4 >= 2) {
            /* Quirk: the original reads entry count - 2, the second-last
               waypoint, and replaces one bound of the box with it. */
            float wp[3];
            aoe_ai_unit_group_waypoint_ror(g, g->flag_1d4 - 2, wp);
            if ((float)lo[0] <= wp[0]) hi[0] = aoe_ftol32(wp[0]);
            else lo[0] = aoe_ftol32(wp[0]);
            if ((float)lo[1] <= wp[1]) hi[1] = aoe_ftol32(wp[1]);
            else lo[1] = aoe_ftol32(wp[1]);
        }
        if (ops->record_area)
            ops->record_area(ops->context, lo[0], lo[1], hi[0], hi[1], owner,
                             ops->game_time ? ops->game_time(ops->context)
                                            : 0);
        if (ops->roll_d24) ops->roll_d24(ops->context);
        *first = 0;
    }
    if (id != g->commander && ops->priest_or_siege &&
        ops->priest_or_siege(ops->context, g->commander) &&
        update_sn(ops, 0x79)) {
        if (ops->order_2bd)
            ops->order_2bd(ops->context, id, g->commander,
                           (float)update_sn(ops, 0x83), 99);
    } else if (task == 0x15) {
        if (ops->order_move)
            ops->order_move(ops->context, id, g->loc_18c[0], g->loc_18c[1],
                            100);
    } else if (ops->order_attack) {
        ops->order_attack(ops->context, id, g->loc_18c[0], g->loc_18c[1],
                          g->field_184, owner, g->block_1d8, g->flag_1d4,
                          g->commander, 0);
    }
}

/* 0x004cd2d0 tasks 9 and 0xe: gather at +0x19c. */
static int update_gather(AoeAiUnitGroupRor *g, int32_t task, int32_t force,
                         const AoeAiUnitGroupOpsRor *ops) {
    int32_t zone;
    int i;
    float pos[3];
    if (g->flag_2c8 != 0xff) ++g->flag_2c8;
    if (!ops->present || !ops->present(ops->context, g->commander)) return 0;
    zone = ops->zone ? ops->zone(ops->context, g->commander) & 0xff : 0;
    /* A gather point outside the commander's zone moves to the commander
       (0x004cd210). */
    if (ops->zone_at &&
        (ops->zone_at(ops->context, g->commander, aoe_ftol32(g->loc_19c[0]),
                      aoe_ftol32(g->loc_19c[1])) & 0xff) != zone &&
        ops->position && ops->position(ops->context, g->commander, pos))
        aoe_ai_unit_group_set_location_ror(g->loc_19c, pos[0], pos[1],
                                           pos[2]);
    for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i) {
        int32_t id = g->members[i], type_id = 0, t = -1, action = -1;
        float limit, gap;
        if (id == -1) continue;
        /* No value is stored for these tasks; a member in another zone is
           dropped like a gone one. */
        if (!ops->unit_ai ||
            !ops->unit_ai(ops->context, id, &type_id, &t, &action) ||
            !ops->position || !ops->position(ops->context, id, pos) ||
            !ops->zone_at ||
            (ops->zone_at(ops->context, g->commander, aoe_ftol32(pos[0]),
                          aoe_ftol32(pos[1])) & 0xff) != zone) {
            aoe_ai_unit_group_remove_slot_ror(g, i, ops);
            continue;
        }
        if (!force && t != -1 && t != 0x266) continue;
        limit = task == 0xe ? 0.0f : (float)update_sn(ops, 0x29);
        gap = update_gap(ops, id, g->loc_19c);
        if (gap < limit) {
            if (action != -1 && ops->dismiss)
                ops->dismiss(ops->context, id, 100);
        } else if (ops->order_2d4) {
            ops->order_2d4(ops->context, id, g->commander, g->loc_19c[0],
                           g->loc_19c[1], g->loc_19c[2], limit,
                           ops->effort ? ops->effort(ops->context, gap) : 0);
        }
    }
    return 1;
}

int aoe_ai_unit_group_update_ror(AoeAiUnitGroupRor *g,
                                 const AoeAiUnitGroupOpsRor *ops,
                                 int32_t task, int32_t record,
                                 int32_t force) {
    int32_t sum = 0, r = 0, t;
    int i, first = 1, ret = 1;
    if (!g || !ops || g->commander == -1) return 0;
    if (task != 9 && task != 0xe) g->flag_2c8 = 0;
    switch (task) {
    case 9:
    case 0xe:
        return update_gather(g, task, force, ops);
    case 3:
        /* Retreat: no values, no type, no record. */
        for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i) {
            int32_t id = g->members[i], type_id, action;
            if (id == -1) continue;
            if (!ops->unit_ai ||
                !ops->unit_ai(ops->context, id, &type_id, &t, &action)) {
                aoe_ai_unit_group_remove_slot_ror(g, i, ops);
                continue;
            }
            if (update_type_28(ops, id) * (1.0f / 3.0f) <=
                    update_gap(ops, id, g->loc_1ac) &&
                ops->order_move)
                ops->order_move(ops->context, id, g->loc_1ac[0],
                                g->loc_1ac[1], 100);
        }
        return 1;
    case 0x14: {
        int32_t now = ops->game_time ? ops->game_time(ops->context) : 0;
        if ((now - g->field_328) / 1000 < 10) return 0;
        g->field_328 = now;
        if (g->field_1d0 != -1) return 0;
        break;
    }
    case 2:
    case 0x15:
        if (ops->game_time) g->field_328 = ops->game_time(ops->context);
        break;
    case 7:
        /* One roll for the whole group. */
        r = ops->rand ? ops->rand(ops->context) : 0;
        break;
    case 0: case 1: case 4: case 5: case 6: case 8: case 10: case 0xb:
    case 0xc: case 0x13:
        break;
    default:
        return 0;
    }
    for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i) {
        int32_t id = g->members[i];
        if (id == -1) continue;
        if (!update_live(g, i, ops, &t)) continue;
        sum += g->values[i];
        switch (task) {
        case 0:
        case 1:
            if (ops->dismiss) ops->dismiss(ops->context, id, 100);
            break;
        case 2:
        case 0x15:
            if (g->field_1d0 != -1) break;
            /* Only task 600 with unit AI +0x38 other than 0x1b is busy. */
            if (!force && t == 600 && ops->unit_ai_38 &&
                ops->unit_ai_38(ops->context, id) != 0x1b)
                break;
            update_attack_member(g, task, id, &first, ops);
            break;
        case 0x14:
            if (!force && t != -1) break;
            update_attack_member(g, task, id, &first, ops);
            break;
        case 4:
            if (!force && t != -1) break;
            {
                /* No 1.0 is taken off this distance. */
                float d = ops->distance
                              ? (float)ops->distance(ops->context, id,
                                                     g->loc_18c[0],
                                                     g->loc_18c[1],
                                                     g->loc_18c[2])
                              : 0.0f;
                int32_t e = ops->effort ? ops->effort(ops->context, d) : 0;
                float n = (float)update_sn(
                    ops, g->field_188 == 0x6d ? 0x16 : 0x39);
                if (ops->order_2bd)
                    ops->order_2bd(ops->context, id, g->field_184, n, e);
            }
            break;
        case 7:
            if (!force && t != -1) break;
            if (id != g->commander && r % 100 < update_sn(ops, 200))
                update_follow(g, id, ops);
            else if (ops->order_zero)
                ops->order_zero(ops->context, id, 0);
            break;
        case 8:
            if (!force && t != -1) break;
            if (ops->order_zero) ops->order_zero(ops->context, id, 0);
            break;
        case 10:
            if (!force && t != -1) break;
            if (id != g->commander) update_follow(g, id, ops);
            else if (ops->order_10)
                ops->order_10(ops->context, id, g->field_184, g->loc_18c[0],
                              g->loc_18c[1]);
            break;
        case 0xb:
            if (!force && t != -1) break;
            if (id != g->commander) update_follow(g, id, ops);
            else if (ops->order_11)
                ops->order_11(ops->context, id, g->field_184);
            break;
        default:
            break;
        }
    }
    if ((task == 2 || task == 0x15) && g->field_1d0 != -1) {
        /* The "play" attack of the occupied members in slot order; the
           type is left alone and the result is 0 (as when the original's
           list allocation fails). */
        int32_t ids[AOE_AI_GROUP_SLOTS];
        int32_t n = 0;
        for (i = 0; i < AOE_AI_GROUP_SLOTS; ++i)
            if (g->members[i] != -1) ids[n++] = g->members[i];
        if (ops->order_play)
            ops->order_play(ops->context, g->commander, ids, g->count,
                            g->field_184, g->field_1d0, g->block_1d8,
                            g->flag_1d4);
        return 0;
    }
    g->type = (task == 0 || task == 1) ? 0 : 1;
    /* Tasks 5, 6, 0xc and 0x13 return 0 even after recording. */
    if (task == 5 || task == 6 || task == 0xc || task == 0x13) ret = 0;
    if (record == 1) {
        g->field_168 = sum;
        g->field_16c = g->count;
        if (task == 2 || task == 0x15 || task == 0x14) g->flag_1d4 = 0;
    }
    return ret;
}
