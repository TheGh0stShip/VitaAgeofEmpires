#include "tribe_player_commands.h"

#include "command_production.h"

#include <stdarg.h>
#include <stdio.h>

#include "aoe_stream.h"

#define MAX_UNITS 25

static int send(const AoeTribePlayerOpsRor *ops, const uint8_t *buf, size_t n) {
    return n ? ops->queue(ops->context, buf, n) : 0;
}

int aoe_tribe_player_build_ror(const AoeTribePlayerOpsRor *ops, int16_t id, float x, float y) {
    const AoeTribeCommandUnitRor *units[MAX_UNITS];
    uint8_t buf[0x200];
    int16_t n;
    int state = ops->template_state(ops->context, id);
    if (state == 0 || (state != 2 && id != 0x6d)) return 0;
    n = ops->collect(ops->context, 4, 4, -1, -1, units);
    if (n <= 0) return 0;
    ops->ack_builder(ops->context);
    send(ops, buf, aoe_tribe_build_66_ror(buf, sizeof buf, units, n, id, x, y));
    return 1;
}

int aoe_tribe_player_build_69_ror(const AoeTribePlayerOpsRor *ops, int16_t id, int32_t a,
                                  int32_t b, int32_t c, int32_t d) {
    const AoeTribeCommandUnitRor *units[MAX_UNITS];
    uint8_t buf[0x200];
    int16_t n;
    if (ops->template_state(ops->context, id) != 2) return 0;
    n = ops->collect(ops->context, 4, 4, -1, -1, units);
    if (n <= 0) return 0;
    ops->ack_builder(ops->context);
    send(ops, buf, aoe_tribe_build_69_ror(buf, sizeof buf, units, n, id, a, b, c, d));
    return 1;
}

static int produce(const AoeTribePlayerOpsRor *ops, uint8_t opcode, int16_t item) {
    const AoeTribeCommandUnitRor *b = ops->selected(ops->context);
    uint8_t buf[AOE_COMMAND_PRODUCTION_SIZE_ROR];
    if (!b || ops->object_type(ops->context, b) != 'P') return 0;
    send(ops, buf, aoe_command_encode_production_ror(buf, sizeof buf, opcode, b->id,
                                                     (uint8_t)b->owner, item, -1));
    return 1;
}

int aoe_tribe_player_train_ror(const AoeTribePlayerOpsRor *ops, int16_t item) {
    return produce(ops, AOE_COMMAND_TRAIN_ROR, item);
}

int aoe_tribe_player_research_ror(const AoeTribePlayerOpsRor *ops, int16_t item) {
    return produce(ops, AOE_COMMAND_RESEARCH_ROR, item);
}

int aoe_tribe_player_delete_ror(const AoeTribePlayerOpsRor *ops,
                                const AoeTribeCommandUnitRor *o, int ours) {
    uint8_t buf[8], t;
    if (!o || !ours) return 0;
    t = ops->object_type(ops->context, o);
    if (t != 'P' && t != 'F') return 0;
    send(ops, buf, aoe_tribe_build_6a_ror(buf, sizeof buf, o));
    return 1;
}

int aoe_tribe_player_delete_selected_ror(const AoeTribePlayerOpsRor *ops) {
    const AoeTribeCommandUnitRor *o = ops->selected(ops->context);
    if (!o || o->owner != ops->player) return 0;
    if (ops->object_class(ops->context, o) == 1) return 0;
    return aoe_tribe_player_delete_ror(ops, o, 1);
}

int aoe_tribe_player_6d_ror(const AoeTribePlayerOpsRor *ops, uint32_t value) {
    const AoeTribeCommandUnitRor *units[MAX_UNITS];
    uint8_t buf[0x200];
    int16_t n = ops->collect(ops->context, 4, -1, -1, 5, units);
    if (n <= 0) return 0;
    ops->ack_unit(ops->context, units[0]);
    send(ops, buf, aoe_tribe_build_6d_ror(buf, sizeof buf, units, n, value));
    return 1;
}

int aoe_tribe_player_6d_one_ror(const AoeTribePlayerOpsRor *ops, int32_t id, uint32_t value) {
    uint8_t buf[0x20];
    send(ops, buf, aoe_tribe_build_6d_one_ror(buf, sizeof buf, id, value));
    return 1;
}

int aoe_tribe_player_6b_ror(const AoeTribePlayerOpsRor *ops, uint32_t a, uint32_t b) {
    const AoeTribeCommandUnitRor *units[MAX_UNITS];
    uint8_t buf[0x200];
    int16_t n = ops->collect(ops->context, 4, -1, -1, 4, units);
    if (n <= 0) return 0;
    ops->ack_unit(ops->context, units[0]);
    send(ops, buf, aoe_tribe_build_6b_ror(buf, sizeof buf, units, n, a, b));
    return 1;
}

int aoe_tribe_player_6f_ror(const AoeTribePlayerOpsRor *ops, float x, float y) {
    const AoeTribeCommandUnitRor *units[MAX_UNITS];
    uint8_t buf[0x200];
    int16_t n = ops->collect(ops->context, 4, -1, -1, 7, units);
    if (n <= 0) return 0;
    ops->ack_unit(ops->context, units[0]);
    send(ops, buf, aoe_tribe_build_6f_ror(buf, sizeof buf, units, n, x, y));
    return 1;
}

int aoe_tribe_player_66_one_ror(const AoeTribePlayerOpsRor *ops, int32_t id, int16_t value,
                                float x, float y, int32_t field_10) {
    uint8_t buf[0x40];
    send(ops, buf, aoe_tribe_build_66_ids_ror(buf, sizeof buf, (uint8_t)ops->player, &id, 1,
                                              value, x, y, field_10));
    return 1;
}

int aoe_tribe_player_67_ror(const AoeTribePlayerOpsRor *ops, int16_t b, int16_t value) {
    uint8_t buf[0x10];
    send(ops, buf, aoe_tribe_build_67_value_ror(buf, sizeof buf, ops->player, b, value));
    return 1;
}

int aoe_tribe_player_template_usable_ror(const AoeTribePlayerOpsRor *ops, int16_t id) {
    return ops->template_state(ops->context, id) == 2 || id == 0x6d;
}

int aoe_tribe_tech_state_is_one_ror(const uint8_t *entries, int32_t i) {
    return (int16_t)(entries[i * 8 + 4] | entries[i * 8 + 5] << 8) == 1;
}

int aoe_tribe_research_open_ror(int16_t state) {
    return state != 3 && state != -1;
}

void aoe_tribe_ai_kick_ror(const AoeTribeForwardOpsRor *ops, int32_t kind) {
    if (ops->has_ai) ops->ai_kick(ops->context, kind);
}

int32_t aoe_tribe_ai_strategic_ror(const AoeTribeForwardOpsRor *ops, int32_t i) {
    return ops->has_ai ? ops->ai_strategic(ops->context, i) : -1;
}

void aoe_tribe_ai_log_ror(const AoeTribeForwardOpsRor *ops, const char *format, ...) {
    char line[1024];
    va_list args;
    if (!ops->has_ai) return;
    va_start(args, format);
    vsnprintf(line, sizeof line, format, args);
    va_end(args);
    ops->ai_log(ops->context, line);
}

void aoe_tribe_add_unit_ror(const AoeTribeForwardOpsRor *ops, void *unit, int32_t a, int32_t b,
                            int32_t c) {
    int16_t unit_class;
    int32_t id;
    ops->add_unit(ops->context, unit, a, b, c);
    if (!ops->has_ai || !ops->unit_info(ops->context, unit, &unit_class, &id)) return;
    if (unit_class != 0xb && id >= 0) ops->ai_unit_added(ops->context, id);
}

void aoe_tribe_queue_train_ror(const AoeTribeForwardOpsRor *ops, int32_t a, int32_t b, int32_t c) {
    ops->queue_train(ops->context, ops->player_id, b, a, c);
}

void aoe_tribe_queue_research_ror(const AoeTribeForwardOpsRor *ops, int32_t a, int32_t b, int32_t c) {
    ops->queue_research(ops->context, ops->player_id, b, a, c);
}

void aoe_tribe_order_one_ror(const AoeTribeForwardOpsRor *ops, int32_t id, int32_t a, int32_t b,
                             int32_t c, int32_t d) {
    int32_t ids[1];
    ids[0] = id;
    ops->order_ids(ops->context, ops->player_id, ids, 1, a, b, c, d);
}

int aoe_tribe_tribute_ror(const AoeTribeForwardOpsRor *ops, int32_t a, int32_t resource,
                          float amount, int32_t b) {
    if (resource == -1 || amount == 0.0f) return 0;
    ops->tribute(ops->context, ops->player_id, a, resource, amount, b);
    return 1;
}

void aoe_tribe_sample_score_ror(const AoeTribeForwardOpsRor *ops, float score) {
    ops->sample(ops->context, 0, score < 0.0f ? 0 : (int32_t)score);
}

void aoe_tribe_save_record_ror(const int32_t record[4], AoeOutcomeStream *s) {
    aoe_stream_put_i32(s, record[0]);
    aoe_stream_put_i32(s, record[1]);
    aoe_stream_put_i32(s, record[3]);
    aoe_stream_put_i32(s, record[2]);
}

int aoe_tribe_player_delete_id_ror(const AoeTribePlayerOpsRor *ops,
                                   const AoeTribeCommandUnitRor *(*lookup)(void *context,
                                                                         int32_t id),
                                   int32_t id) {
    const AoeTribeCommandUnitRor *o = lookup(ops->context, id);
    return o && aoe_tribe_player_delete_ror(ops, o, 1) == 1; /* the player's own list */
}

void *aoe_tribe_next_of_template_ror(const AoeTribeListUnitRor *units, int32_t count, int16_t id,
                                     const void *after) {
    int32_t i;
    if (after) {
        int seen = 0;
        for (i = 0; i < count; ++i) {
            if (!units[i].unit) continue;
            if (seen) {
                if (units[i].template_id == id && units[i].state < 3) return units[i].unit;
            } else if (units[i].unit == after) {
                seen = 1;
            }
        }
    }
    for (i = 0; i < count; ++i)
        if (units[i].unit && units[i].template_id == id && units[i].state == 2) return units[i].unit;
    return NULL;
}

void aoe_tribe_cycle_tick_ror(float *seconds, int32_t *counter, float tick) {
    *seconds += tick;
    while (1.0f < *seconds) {
        *seconds -= 1.0f;
        if (++*counter == 0x1e) *counter = 0;
    }
}

int aoe_tribe_shared_vision_check_ror(float attribute_50, int32_t *flag_104,
                                      void (*refresh)(void *context), void *context) {
    if (!(0.0f < attribute_50)) return 0;
    *flag_104 = 1;
    refresh(context);
    return 1;
}
