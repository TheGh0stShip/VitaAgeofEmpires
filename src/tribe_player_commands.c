#include "tribe_player_commands.h"
#include <string.h>
#include <stdlib.h>

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

int32_t aoe_tribe_ai_message_ror(const AoeTribeAiRelayOpsRor *ops, int32_t a, int32_t b,
                                 int32_t c, int32_t d, int32_t e) {
    return ops->has_ai ? ops->ai_message(ops->context, a, b, c, d, e) : 0;
}

void aoe_tribe_build_place_ror(const AoeTribeAiRelayOpsRor *ops, int32_t a, int32_t b,
                               int32_t c, float x, float y, int32_t d, int32_t e) {
    if (ops->has_ai) ops->build_place(ops->context, a, b, c, x, y, d, e);
}

void aoe_tribe_build_named_ror(const AoeTribeAiRelayOpsRor *ops, int32_t a, int32_t b) {
    if (ops->has_ai) ops->build_named(ops->context, a, b);
}

void aoe_tribe_build_done_4_ror(const AoeTribeAiRelayOpsRor *ops, int32_t a, int32_t b,
                                int32_t c, int32_t d) {
    if (ops->has_ai) ops->build_done_4(ops->context, a, b, c, d);
}

void aoe_tribe_build_done_3_ror(const AoeTribeAiRelayOpsRor *ops, int32_t a, int32_t b,
                                int32_t c) {
    if (ops->has_ai) ops->build_done_3(ops->context, a, b, c);
}

void aoe_tribe_tactical_relay_ror(const AoeTribeAiRelayOpsRor *ops, int32_t a, int32_t b,
                                  int32_t c) {
    if (ops->has_ai) ops->tactical(ops->context, a, b, c);
}

/* 0x004eff20 */
void aoe_tribe_player_save_ror(const AoeTribePlayerSaveRor *p, AoeOutcomeStream *s,
                               const AoeTribePlayerSaveOpsRor *ops) {
    ops->base_save(ops->context, s);
    aoe_stream_put_u8(s, p->b48);
    aoe_stream_put_i32(s, p->v22c);
    aoe_stream_put_i32(s, p->v234);
    aoe_stream_put_i32(s, p->v228);
    aoe_stream_put_i32(s, p->v238);
    ops->research_save(ops->context, s);
    aoe_stream_put_i32(s, p->v230);
    ops->history_save(ops->context, s);
    aoe_stream_put_i32(s, p->v844);
    aoe_stream_put_i32(s, p->v848);
    if (p->type != 1) return;
    if (ops->has_ai && ops->is_host(ops->context) == 1) {
        aoe_stream_put_i32(s, 1);
        ops->ai_save(ops->context, s);
        return;
    }
    aoe_stream_put_i32(s, 0);
}

/* 0x004f7300 */
void aoe_tribe_player_save_timed_ror(const AoeTribePlayerSaveRor *p, AoeOutcomeStream *s,
                                     const AoeTribePlayerSaveOpsRor *ops) {
    aoe_tribe_player_save_ror(p, s, ops);
    aoe_stream_put_i32(s, p->v84c);
    aoe_stream_put_i32(s, p->v850);
    aoe_stream_put_i32(s, p->v858);
    aoe_stream_put_i32(s, p->v854);
}

/* 0x004efdc0 */
void aoe_tribe_player_destroy_ror(AoeTribePlayerMembersRor *m) {
    if (m->research) {
        m->research_free(m->context, m->research);
        m->research = NULL;
    }
    if (m->ai) {
        m->ai_delete(m->context, m->ai);
        m->ai = NULL;
    }
    if (m->history) {
        m->history_free(m->context, m->history);
        m->history = NULL;
    }
    m->base_destroy(m->context);
}

/* 0x004f2aa0 */
int32_t aoe_tribe_add_object_ror(const AoeTribeAddObjectOpsRor *ops, void *unit, int32_t a,
                                 int32_t b) {
    int32_t r = ops->base_add(ops->context, unit, a, b);
    int16_t unit_class;
    int32_t id;
    uint8_t state;
    if (!ops->unit_info(ops->context, unit, &unit_class, &id)) return r;
    if (unit_class == 0xb || !ops->has_ai || id < 0) return r;
    ops->ai_object_added(ops->context, unit);
    state = ops->game_state(ops->context);
    ops->build_event(ops->context, unit, (state == 2 || state == 3) ? 0 : 1);
    return r;
}

void *aoe_tribe_load_object_ror(const AoeTribeLoadObjectOpsRor *ops, uint8_t kind, void *stream,
                                void *player) {
    if (kind == 'F') return ops->load_combat(ops->context, stream, player);
    if (kind == 'P') return ops->load_building(ops->context, stream, player);
    return ops->load_base(ops->context, kind, stream, player);
}

void aoe_tribe_template_load_ror(const AoeTribeTemplateLoadOpsRor *ops, void *stream,
                                 int16_t slot, uint8_t kind, int32_t a, int32_t b) {
    switch (kind) {
    case 'F':
        ops->templates[slot] = ops->load_f(ops->context, stream, a, b);
        break;
    case 'P':
        ops->templates[slot] = ops->load_p(ops->context, stream, a, b);
        break;
    case 'Z':
        ops->templates[slot] = ops->load_z(ops->context, stream, a, b);
        break;
    default:
        ops->load_base(ops->context, stream, slot, kind, a, b);
        break;
    }
}

int aoe_tribe_player_repair_ror(const AoeTribePlayerOpsRor *ops,
                                const AoeTribeRepairTargetRor *target,
                                int (*allied)(void *context, int16_t from, int16_t to),
                                void (*ack)(void *context, const AoeTribeCommandUnitRor *unit,
                                            const AoeTribeCommandUnitRor *target)) {
    const AoeTribeCommandUnitRor *units[MAX_UNITS];
    uint8_t buf[0x200];
    int16_t owner = target->unit->owner, n;
    int ours = owner == ops->player;
    int damaged = (float)target->max_hp > target->hp;
    if (!((ours || allied(ops->context, ops->player, owner)) && target->type == 'P' &&
          damaged && target->state <= 2)) {
        if (!ours && (!allied(ops->context, ops->player, owner) ||
                      !allied(ops->context, owner, ops->player)))
            return 0;
        if (target->cls != 2 && target->cls != 20 && target->cls != 21 && target->cls != 22)
            return 0;
        if (!damaged || target->state > 2) return 0;
    }
    n = ops->collect(ops->context, 4, 4, -1, -1, units);
    if (n <= 0) return 0;
    if (ack) ack(ops->context, units[0], target->unit);
    send(ops, buf, aoe_tribe_build_repair_ror(buf, sizeof buf, units, n, target->unit));
    return 1;
}

AoeTribeTemplateKindRor aoe_tribe_template_kind_ror(uint8_t kind) {
    switch (kind) {
    case 'F': return AOE_TRIBE_TEMPLATE_F;
    case 'P': return AOE_TRIBE_TEMPLATE_P;
    case 'Z': return AOE_TRIBE_TEMPLATE_Z;
    default: return AOE_TRIBE_TEMPLATE_BASE;
    }
}

int32_t aoe_tribe_player_load_templates_ror(AoeOutcomeStream *s, float version,
                                            const AoeTribePlayerLoadOpsRor *o) {
    int16_t count, i;
    const uint8_t *flags;
    if (!s || !o) return -1;
    flags = aoe_stream_take(s, 2);
    if (!flags) return -1;
    count = (int16_t)aoe_le_u16(flags);
    if (count > 0) {
        flags = aoe_stream_take(s, (size_t)count * 4);
        if (!flags) return -1;
        for (i = 0; i < count; ++i) {
            if (aoe_le_u32(flags + i * 4) == 0) continue;
            {
                uint8_t kind = aoe_stream_u8(s);
                if (s->failed) return -1;
                o->load_template(o->context, i, kind, aoe_tribe_template_kind_ror(kind), s);
            }
        }
    }
    o->load_tile_map(o->context, s);
    o->load_object_memory(o->context, s);
    for (i = 0; i < 3; ++i) o->new_list(o->context, i);
    o->load_list(o->context, 0, s);
    o->load_list(o->context, 1, s);
    if (version >= 7.09f) o->load_list(o->context, 2, s);
    return count;
}

/* 0x004f34c0 */
void aoe_tribe_player_message_ror(const AoeTribeMessageOpsRor *ops, int32_t id, int32_t from,
                                  int32_t code, int32_t a, int32_t b, int32_t c)
{
    float x = 0.0f, y = 0.0f;
    int exists = ops->object_exists(ops->context, id, &x, &y);
    if (code != 0x20a && code != 0x72 && code != 0x74) {
        if (!exists || !ops->game_live(ops->context))
            return;
    }
    switch (code) {
    case 0x1f9:
        if (a == 0x25d && ops->is_ours(ops->context, id) && ops->mode(ops->context) == 0x6b)
            ops->mode_6b_action(ops->context);
        return;
    case 0x201: case 0x72: case 0x74:
        ops->forward_ai(ops->context, id, from, code, a, b, c);
        return;
    case 0x202:
        if (a == 0x25d || a == 0x26f) {
            int32_t px = b, py = a;
            if (ops->can_retarget(ops->context, id) &&
                ops->find_point(ops->context, id, x, y, &px, &py) == 1)
                ops->goto_point(ops->context, id, px, py);
            else
                ops->fallback(ops->context, id);
        }
        return;
    case 0x209:
        if (ops->object_exists(ops->context, a, NULL, NULL))
            ops->flash(ops->context, ops->object_template_id(ops->context, a), 0, 10);
        return;
    case 0x20a: ops->on_20a(ops->context, id, a, b); return;
    case 0x20d: ops->on_20d(ops->context, a); return;
    default: return;
    }
}

/* 0x004f0030 */
void aoe_tribe_player_tick_ror(AoeTribeTickStateRor *p, float dt, const AoeTribeTickOpsRor *ops)
{
    AoeTribeTickRecordRor *r = p->rec;
    p->accum += dt;
    while (p->accum > 1.0f) {
        p->accum -= 1.0f;
        if (++p->counter > 14) {
            p->counter -= 15;
            ops->sample(ops->context, 0, r->score >= 0.0f ? (int32_t)r->score : 0);
        }
        r->explored = ops->full_explore(ops->context) ? 100.0f
                                                      : ops->explored_fraction(ops->context) * 100.0f;
        r->margin = r->r50 - (r->r4c - r->score);
        if (r->margin < 0.0f)
            r->margin = 0.0f;
        r->r24 += r->r28;
        if (r->r24 > 100.0f)
            r->r24 = 100.0f;
        if (p->down_22c > 0)
            --p->down_22c;
        if (p->down_234 > 0)
            --p->down_234;
        if (p->player_id > 0) {
            int32_t i;
            for (i = 0; i < p->unit_count; ++i) {
                int16_t c = p->units[i].cls;
                if (c != 11 && c != 30 && c != 27 && c != 1 && c != 21 && c != 20 && c != 2 &&
                    c != 37 && p->units[i].state == 2)
                    break;
            }
            if (i == p->unit_count) {
                ops->notify(ops->context, 2);
                r->rb4 = 0;
            }
        }
    }
    ops->base_update(ops->context);
    r->r00 -= r->r84 * r->score * dt * (1.0f / 60.0f);
    if (r->r00 < 0.0f)
        r->r00 = 0.0f;
    if (p->has_ai && ops->ai_ready(ops->context) == 1) {
        if (ops->world_fc(ops->context) == -1)
            ops->world_signal(ops->context, 3);
        ops->ai_update(ops->context, 0);
    }
    if (ops->global_flag(ops->context))
        ops->global_hook(ops->context);
}

void aoe_tribe_player_train_list_ror(const AoeTribeTrainListOpsRor *o, int16_t building,
                                     AoeTribeTrainEntryRor **list, int16_t *count) {
    int16_t i, n = 0, k = 0, extra = 0;
    uint8_t kind, enabled;
    int16_t at, f4c, types[3], amounts[3];
    const char *name;
    if (!o || !list || !count) return;
    for (i = 0; i < o->template_count; ++i)
        if (o->template_info(o->context, i, &kind, &enabled, &at, &f4c, &name) && kind > 'E' &&
            enabled && at == building)
            ++n;
    if (building == 0x76 && o->attribute_48(o->context) == 0.0f &&
        o->template_info(o->context, 0x6d, &kind, &enabled, &at, &f4c, &name) && !enabled &&
        !o->owns_template(o->context, 0x6d)) {
        extra = 1;
        ++n;
    }
    free(*list);
    *list = NULL;
    *count = n;
    if (n == 0) return;
    *list = calloc((size_t)n, sizeof **list);
    if (!*list) { *count = 0; return; }
    for (i = 0; i < o->template_count && k < n - extra; ++i) {
        AoeTribeTrainEntryRor *e;
        if (!o->template_info(o->context, i, &kind, &enabled, &at, &f4c, &name) || kind <= 'E' ||
            !enabled || at != building)
            continue;
        e = &(*list)[k++];
        e->name = name; e->id = i; e->field_4c = f4c;
        o->costs(o->context, i, types, amounts);
        e->cost_type[0] = types[0]; e->cost_type[1] = types[1]; e->cost_type[2] = types[2];
        e->cost_amount[0] = (float)amounts[0]; e->cost_amount[1] = (float)amounts[1];
        e->cost_amount[2] = (float)amounts[2];
        e->help = o->help(o->context, i);
        e->tree_text = o->tree_text(o->context, i);
    }
    if (extra) {
        AoeTribeTrainEntryRor *e = &(*list)[k];
        o->template_info(o->context, 0x6d, &kind, &enabled, &at, &f4c, &name);
        e->name = name; e->id = 0x6d; e->field_4c = f4c;
        o->costs(o->context, 0x6d, types, amounts);
        e->cost_type[0] = types[0]; e->cost_type[1] = types[1]; e->cost_type[2] = types[2];
        e->cost_amount[0] = (float)amounts[0]; e->cost_amount[1] = (float)amounts[1];
        e->cost_amount[2] = (float)amounts[2];
        e->help = o->help(o->context, 0x6d);
        e->tree_text = o->tree_text(o->context, 0x6d);
    }
}

void aoe_tribe_player_dump_ror(const AoeTribeDumpOpsRor *o, int full) {
    char line[512], civ[256];
    if (!o) return;
#define L(...) do { snprintf(line, sizeof line, __VA_ARGS__); o->line(o->context, line); } while (0)
    L("Player %d:", (int)o->player);
    if (full) {
        civ[0] = 0;
        o->civ_name(o->context, civ, (int)sizeof civ);
        if (!o->is_computer) {
            L("Human Player: %s", civ);
        } else {
            L("Computer Player: %s", civ);
            L("Build List: %s", o->build_list(o->context));
            L("City Plan: %s", o->city_plan(o->context));
            L("VC File: %s", o->vc_file(o->context));
        }
    }
    L("Points:");
    L("Combat %d", (int)o->points(o->context, 0));
    L("Economy %d", (int)o->points(o->context, 1));
    L("Religion %d", (int)o->points(o->context, 2));
    L("Science %d", (int)o->points(o->context, 3));
    L("Survival %d", (int)o->event_points(o->context, 0x16));
    L("Wonder %d", (int)o->event_points(o->context, 0x17));
    L("Total: %d", (int)o->score(o->context));
    L("Population: %d", (int)o->attribute(o->context, 11));
    L("Kills: %d", (int)o->samples(o->context, 0));
    L("Kill Ratio: %d", (int)o->samples(o->context, 2));
    L("Razings: %d", (int)o->samples(o->context, 1));
    L("Food: %d", (int)o->attribute(o->context, 0));
    L("Gold: %d", (int)o->attribute(o->context, 3));
    L("Stone: %d", (int)o->attribute(o->context, 2));
    L("Wood: %d", (int)o->attribute(o->context, 1));
    L("Technology: %d", (int)o->samples(o->context, 0x11));
    L("Exploration: %d", (int)o->samples(o->context, 7));
    if (o->is_computer) {
        char buf[256];
        buf[0] = 0; o->build_ai(o->context, buf, (int)sizeof buf); o->line(o->context, buf);
        buf[0] = 0; o->civilians(o->context, buf, (int)sizeof buf); o->line(o->context, buf);
        L("Number Soldiers %d", (int)o->soldiers(o->context));
    }
#undef L
}

/* 0x004f6de0 */
int aoe_tribe_player_cursor_command_ror(const AoeTribeCursorOpsRor *o, int32_t id,
                                        int32_t *command, int32_t *cursor) {
    AoeTribeCursorTargetRor t;
    int32_t offset = 0, cls, stance, action, cur, hp;
    int flagged;
    *command = 0;
    *cursor = 0;
    if (!o->target(o->context, id, &t)) return 0;
    if (t.owner < 0 || t.owner >= o->player_count) return 0;
    o->refresh_flags(o->context);
    if (t.template_id == 0x9e || t.template_id == 0xa3) offset = 0x32;
    cls = t.unit_class;
    for (;;) {
        const AoeTribeCursorRowRor *rows, *row;
        int32_t n, i;
        flagged = o->selection_count != 0 && *o->class_flags != 0;
        rows = flagged ? o->flagged : o->plain;
        n = flagged ? o->flagged_count : o->plain_count;
        if (cls >= 0 && cls < n && rows[cls].v[0] == cls) {
            row = &rows[cls];
        } else {
            for (i = 0; i < n && rows[i].v[0] != cls; ++i) {}
            if (i >= n) return 0;
            row = &rows[i];
        }
        stance = o->stance(o->context, t.owner);
        switch (stance) {
        case 0: action = row->v[3]; cur = row->v[4]; break;
        case 1: action = row->v[1]; cur = row->v[2]; break;
        case 2: action = row->v[5]; cur = row->v[6]; break;
        case 3: action = row->v[7]; cur = row->v[8]; break;
        case 4: action = row->v[9]; cur = row->v[10]; break;
        default: return 0;
        }
        if (action != -1) {
            uint32_t f = *o->class_flags;
            for (;;) {
                int retry = 0;
                switch (action) {
                case 0: *command = 0; *cursor = cur + offset; break;
                case 1: case 8:
                    if ((f & 1) && t.state < 3) { *command = 4; *cursor = cur + offset; }
                    else if (action == 8) { action = 7; retry = 1; }
                    break;
                case 2: if (f & 2) { *command = 5; *cursor = cur + offset; } break;
                case 3: if (f & 4) { *command = 5; *cursor = cur + offset; } break;
                case 4:
                    if ((f & 0x10) && !(o->priests == 1 && id == o->last_priest)) {
                        hp = (int32_t)t.hp;
                        if (hp > 0 && hp < t.max_hp) { *command = 5; *cursor = cur + offset; }
                    }
                    break;
                case 5:
                    if (f & 0x20) {
                        if (t.state == 0) offset = 0x32;
                        hp = (int32_t)t.hp;
                        if (hp > 0 && hp < t.max_hp) { *command = 3; *cursor = cur + offset; }
                    }
                    break;
                case 6: case 9:
                    if (f & 0x40) { *command = 3; *cursor = cur + offset; }
                    else if (action == 9) { action = 1; retry = 1; }
                    break;
                case 7:
                    if (f & 0x80) {
                        if (cls == 0x12) {
                            if (o->convert_units) { *command = 5; *cursor = cur + offset; }
                        } else if (cls != 3) {
                            *command = 5; *cursor = cur + offset;
                        } else if (o->convert_buildings && t.template_id != 0x6d &&
                                   t.template_id != 0x114) {
                            *command = 5; *cursor = cur + offset;
                        }
                    }
                    break;
                default: *command = 0; *cursor = 0; break;
                }
                if (!retry) break;
                offset = 0x32;
            }
        } else {
            return 0;
        }
        if (!flagged || *command != 0 || *cursor != 0) return 1;
        *o->class_flags = 0;
    }
}

#include "aoe_ftol.h"
/* 0x004f1ae0 */
void aoe_tribe_player_place_start_ror(const AoeTribeStartOpsRor *o) {
    int shared = 0;
    int32_t i, j;
    if (!o->has_units) {
        float x, y;
        int32_t tries = 1;
        y = (float)(o->rand_(o->context) * (o->map_h - 30) / 0x7fff + 15) + 0.5f;
        x = (float)(o->rand_(o->context) * (o->map_w - 30) / 0x7fff + 15) + 0.5f;
        for (;;) {
            if (!o->main_blocked(o->context, x + 0.5f, y + 0.5f)) {
                if (tries < 5000) {
                    int k;
                    o->create_main(o->context, x, y);
                    o->set_view(o->context, aoe_ftol32(x), aoe_ftol32(y), x, y);
                    o->add_work(o->context, 10000.0f);
                    for (k = 0; k < 3; ++k) {
                        float ex, ey;
                        for (;;) {
                            int32_t ry = o->rand_(o->context), rx = o->rand_(o->context);
                            ey = (float)o->rand_(o->context) * 3.0518509e-05f +
                                 (float)(ry * 6 / 0x7fff - 3) + y;
                            ex = (float)o->rand_(o->context) * 3.0518509e-05f +
                                 (float)(rx * 6 / 0x7fff - 3) + x;
                            if (ey < 0.0f || ey >= (float)o->map_h || ex < 0.0f ||
                                ex >= (float)o->map_w)
                                continue;
                            if (!o->extra_blocked(o->context, ex, ey)) break;
                        }
                        o->create_extra(o->context, ex, ey);
                    }
                }
                break;
            }
            if (++tries >= 5000) break;
            y = (float)(o->rand_(o->context) * (o->map_h - 30) / 0x7fff + 15) + 0.5f;
            x = (float)(o->rand_(o->context) * (o->map_w - 30) / 0x7fff + 15) + 0.5f;
        }
    } else {
        float x = 0.0f, y = 0.0f;
        int16_t t;
        if (o->anchor(o->context, &x, &y, &t))
            o->set_view(o->context, aoe_ftol32(x), aoe_ftol32(y), x, y);
    }
    if (o->share) {
        for (i = 1; i < o->player_count; ++i) {
            float x, y;
            int16_t t = 0;
            if (!o->stance(o->context, i) || i == o->own_id) continue;
            if (o->anchor(o->context, &x, &y, &t)) o->reveal(o->context, i);
            if (t == 0x6d) o->wonder_notice(o->context, i);
            shared = 1;
        }
    }
    if (o->viewed_player == o->own_id && shared) {
        for (i = 1; i < o->player_count; ++i) {
            if (i == o->own_id) continue;
            for (j = 1; j < 9; ++j)
                if (o->slot_player(o->context, j) == i)
                    o->chat_recipient(o->context, j, o->stance(o->context, i));
        }
    }
}
