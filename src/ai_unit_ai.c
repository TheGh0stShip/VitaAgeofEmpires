#include "ai_unit_ai.h"

#include <math.h>
#include <string.h>

#include "aoe_ftol.h"

static const int32_t table_class9[] = {0, 4, 0x1a, 0x18, 6, 0x19, 0x11, 10,
                                       0xc, 0x23, 0x17, 0x24, 0x1c, 0x12, 0xd,
                                       0x27};
static const int32_t table_class10[] = {0x1d, 9, 0, 4, 0x1a, 0x18, 6, 0x19,
                                        0x11, 0xd, 0xc, 0x23, 0x17, 0x24,
                                        0x1c, 0x12, 0x27};
static const int32_t table_class4[] = {3, 7, 8, 9, 10, 0xf, 0x11, 0x12,
                                       0x20, 0x21, 0x1b};
static const int32_t table_priest[] = {3, 0, 4, 0x1a, 0x18, 6, 0x19, 0x11, 10,
                                       0xc, 0x23, 0x17, 0x24, 0x1c, 0x12, 0xd,
                                       0x27};
static const int32_t table_class2[] = {3, 0x16, 0x14, 2, 0x15};
static const int32_t table_class15[] = {3, 0x16, 0x14, 2, 0x15, 5, 0x1f, 0x21};

static const int32_t table_21[21] = {0, 3, 4, 0x1a, 0x18, 6, 0x23, 0x19, 0x11, 0xc, 0x17,
                                     0x24, 0x1c, 0x12, 0xd, 0x1b, 0x16, 0x14, 2, 0x15, 0x27};
static const int32_t table_19[19] = {0, 4, 0x1a, 0x18, 6, 0x23, 0x19, 0x11, 0xc, 0x17,
                                     0x24, 0x1c, 0x12, 0xd, 0x16, 0x14, 2, 0x15, 0x27};

void aoe_ai_unit_ai_init_ror(AoeAiUnitAiRor *ai, int kind,
                             const AoeAiUnitAiOpsRor *ops) {
    const int32_t *table = NULL;
    int32_t n = 0, timer = 0;
    if (!ai) return;
    memset(ai, 0, sizeof(*ai));
    ai->kind = kind;
    if (ops && ops->base_init) ops->base_init(ops->context);
    switch (kind) {
    case AOE_AI_UNIT_AI_CLASS_9:
        table = table_class9;
        n = 16;
        timer = 4000;
        break;
    case AOE_AI_UNIT_AI_CLASS_10:
        table = table_class10;
        n = 17;
        break;
    case AOE_AI_UNIT_AI_TYPE_30:
        break;
    case AOE_AI_UNIT_AI_TYPE_7E:
        table = table_class10;
        n = 17;
        timer = 6000;
        break;
    case AOE_AI_UNIT_AI_CLASS_4:
        table = table_class4;
        n = 11;
        break;
    case AOE_AI_UNIT_AI_PRIEST:
        table = table_priest;
        n = 17;
        break;
    case AOE_AI_UNIT_AI_CLASS_2:
    case AOE_AI_UNIT_AI_CLASS_14:
        table = table_class2;
        n = 5;
        break;
    case AOE_AI_UNIT_AI_CLASS_15:
        table = table_class15;
        n = 8;
        break;
    case AOE_AI_UNIT_AI_KIND_5564:
        table = table_21;
        n = 21;
        timer = 1500;
        break;
    case AOE_AI_UNIT_AI_KIND_5644:
        table = table_19;
        n = 19;
        timer = 5000;
        break;
    case AOE_AI_UNIT_AI_KIND_5724:
    case AOE_AI_UNIT_AI_KIND_5804:
        table = table_21;
        n = 21;
        break;
    default:
        return;
    }
    if (table) {
        memcpy(ai->classes, table, (size_t)n * sizeof(*table));
        ai->class_count = n;
    }
    if (timer) {
        ai->timer = timer;
        if (ops && ops->restart_timer) ops->restart_timer(ops->context, timer);
    }
}

int32_t aoe_ai_unit_ai_slot_54_ror(void) { return 1; }
int32_t aoe_ai_unit_ai_slot_d4_ror(void) { return 5; }
int32_t aoe_ai_unit_ai_slot_d8_ror(void) { return 9; }

int32_t aoe_ai_unit_ai_resource_of_ror(int32_t type_class) {
    switch (type_class) {
    case 5: case 0x1f: case 0x21: return 2;
    case 7: return 1;
    case 8: return 3;
    case 0xf: return 0;
    case 0x20: return 4;
    default: return -1;
    }
}

void aoe_ai_unit_ai_class4_5c_ror(int32_t id, int32_t value,
                                  const AoeAiUnitAiOpsRor *ops) {
    AoeAiUnitViewRor o;
    if (!ops || !ops->object || !ops->object(ops->context, id, &o)) return;
    if (o.type_class != 9 && o.type_class != 10 && o.type_class != 0xf) {
        if (ops->base_5c) ops->base_5c(ops->context, id, value);
        return;
    }
    if (ops->call_64) ops->call_64(ops->context, id, value);
}

int aoe_ai_unit_ai_priest_50_ror(int has_object, int32_t id,
                                 const AoeAiUnitAiOpsRor *ops) {
    if (!has_object || !ops || !ops->call_c4) return 0;
    return ops->call_c4(ops->context, id);
}

int aoe_ai_unit_ai_can_convert_ror(int32_t id, const AoeAiUnitAiOpsRor *ops) {
    AoeAiUnitViewRor me, o;
    float tech;
    if (!ops || !ops->self || !ops->object) return 0;
    ops->self(ops->context, &me);
    if (!(100.0f <= me.field_44) || !ops->object(ops->context, id, &o)) return 0;
    if (o.type_id == 0x6d || o.type_class == 0x1b || o.type_id == 0x9f ||
        o.type_id == 0x114 || o.type_id == 0x9e || o.type_id == 0xa3 ||
        o.type_class == 0x1e)
        return 0;
    if (o.type_class == 0x12)
        tech = ops->owner_resource ? ops->owner_resource(ops->context, 27) : 0.0f;
    else if (o.type_class == 3)
        tech = ops->owner_resource ? ops->owner_resource(ops->context, 28) : 0.0f;
    else
        return 1;
    return 0.0f < tech;
}

int32_t aoe_ai_unit_ai_back_off_ror(const AoeAiUnitAiRor *ai,
                                    const AoeAiUnitAiOpsRor *ops) {
    AoeAiUnitViewRor me, o;
    double r, dx, dy;
    float d, factor;
    int32_t target;
    if (!ai || !ops || !ops->target || !ops->object || !ops->self) return 5;
    factor = ai->kind == AOE_AI_UNIT_AI_CLASS_9 ? 3.0f : 7.0f;
    if (ai->kind != AOE_AI_UNIT_AI_CLASS_9 &&
        !(ops->owner_field_4 && ops->owner_field_4(ops->context)))
        return 5;
    target = ops->target(ops->context);
    if (target == -1 || !ops->object(ops->context, target, &o)) return 5;
    ops->self(ops->context, &me);
    r = (ops->speed ? ops->speed(ops->context) : 0.0) * (double)factor;
    dx = (double)o.x - (double)me.x;
    dy = (double)o.y - (double)me.y;
    d = (float)sqrt(dx * dx + dy * dy);
    if (ops->move)
        ops->move(ops->context, (float)(-(dx / (double)d * r) + (double)me.x),
                  (float)(-(dy / (double)d * r) + (double)me.y), me.z);
    return 6;
}

int32_t aoe_ai_unit_ai_7e_d4_ror(AoeAiUnitAiRor *ai,
                                 const AoeAiUnitAiOpsRor *ops) {
    AoeAiUnitViewRor me, o;
    int32_t target, level;
    float reach;
    if (!ai || !ops) return 5;
    ai->timer = 6000;
    if (ops->restart_timer) ops->restart_timer(ops->context, 6000);
    target = ops->target ? ops->target(ops->context) : -1;
    if (target == -1 || !ops->object || !ops->object(ops->context, target, &o) ||
        !ops->self)
        return 5;
    ops->self(ops->context, &me);
    level = ops->difficulty ? ops->difficulty(ops->context) : 0;
    if (ops->settings_8fc == 1 && ops->settings_97d == 1 && 2 < level) {
        reach = (level == 3 ? 0.75f : 0.5f) * me.type_28;
        if (reach < 1.0f) reach = 1.0f;
    } else {
        reach = me.type_28 + me.type_28;
    }
    if ((ops->distance_to ? ops->distance_to(ops->context, target) : 0.0) <=
        (double)reach) {
        if (ops->call_5c) ops->call_5c(ops->context, target, 0);
        return 6;
    }
    return 5;
}

int32_t aoe_ai_unit_ai_class10_cc_ror(int32_t target, int32_t message_type,
                                      void *message, int32_t arg,
                                      const AoeAiUnitAiOpsRor *ops) {
    AoeAiUnitViewRor me, o;
    if (!ops) return 0;
    if (message_type == 0x1fc) {
        if (ops->object && ops->object(ops->context, target, &o) && ops->self) {
            double d = ops->distance_to ? ops->distance_to(ops->context, target) : 0.0;
            ops->self(ops->context, &me);
            /* fcomp; test ah, 0x41: only "greater" skips, NaN passes. */
            if (!(d > (double)me.type_28)) {
                if (ops->call_5c) ops->call_5c(ops->context, target, 1);
                return 3;
            }
        }
        if (ops->clear_target) ops->clear_target(ops->context);
        if (ops->call_58) ops->call_58(ops->context, 1);
        if (ops->call_1c) ops->call_1c(ops->context, message);
    }
    return ops->base_cc ? ops->base_cc(ops->context, message, arg) : 0;
}

int32_t aoe_ai_unit_ai_priest_5c_ror(int32_t field_28, int32_t id,
                                     int32_t value,
                                     const AoeAiUnitAiOpsRor *ops) {
    if (!ops) return 0;
    if (ops->call_c4 && ops->call_c4(ops->context, id))
        return ops->call_6c ? ops->call_6c(ops->context, id, value) : 0;
    if (field_28 == -1 && ops->owner_field_4 && ops->owner_field_4(ops->context)) {
        float f = (float)(ops->unit_10c ? ops->unit_10c(ops->context, value) : 0.0);
        return ops->call_94 ? ops->call_94(ops->context, id, f) : 0;
    }
    return 0;
}

AoeAiUnitAiRor *aoe_ai_unit_ai_delete_ror(AoeAiUnitAiRor *ai, uint8_t flags,
                                          const AoeAiUnitAiOpsRor *ops) {
    if (!ai) return ai;
    if (ops && ops->base_destroy) ops->base_destroy(ops->context);
    ai->class_count = 0;
    if ((flags & 1) && ops && ops->free_self) ops->free_self(ops->context);
    return ai;
}

int32_t aoe_ai_unit_ai_5644_d8_ror(AoeAiUnitAiRor *ai,
                                   const AoeAiUnitAiOpsRor *ops) {
    if (!ai || !ops || !ops->call_d4 || ops->call_d4(ops->context, 1) != 6)
        return 9;
    if (ops->clear_target) ops->clear_target(ops->context);
    if (ops->call_58) ops->call_58(ops->context, 1);
    ai->field_f8 = 0;
    return 10;
}

int32_t aoe_ai_unit_ai_priest_heal_target_ror(int32_t self, int32_t far, float *dist,
                                             const AoeAiUnitAiOpsRor *ops) {
    int band, bands, best = -1;
    float best_v = 0.0f;
    int16_t owner;
    if (!ops) return -1;
    if (ops->refresh_nearby) ops->refresh_nearby(ops->context);
    owner = ops->owner_index ? ops->owner_index(ops->context) : 0;
    bands = owner == 0 ? 4 : 2;
    for (band = 0; band < bands; ++band) {
        const uint8_t *e = NULL;
        int32_t n = ops->nearby ? ops->nearby(ops->context, band, &e) : 0, i;
        for (i = 0; i < n; ++i) {
            const uint8_t *it = e + 8 * i;
            int32_t id = (int32_t)((uint32_t)it[0] | (uint32_t)it[1] << 8 |
                                   (uint32_t)it[2] << 16 | (uint32_t)it[3] << 24);
            int16_t cls, max_hp;
            float hp, d, v;
            uint8_t state;
            if (id == self || !ops->target_view ||
                !ops->target_view(ops->context, id, &cls, &hp, &max_hp, &state))
                continue;
            if (cls == 3 || !(hp < (float)max_hp) || state >= 3) continue;
            if (!far && it[4] >= 5) continue;
            if (owner == 0 && it[4] >= 3) continue;
            if (!ops->unit_194 || !ops->unit_194(ops->context, id)) continue;
            d = (float)it[4];
            v = (float)max_hp - hp / d;
            if (best == -1 || v < best_v) {
                best = id;
                best_v = v;
                if (dist) *dist = d;
            }
        }
        if (best != -1) return best;
    }
    return -1;
}

int32_t aoe_ai_unit_ai_priest_convert_target_ror(float *dist,
                                                const AoeAiUnitAiOpsRor *ops) {
    const uint8_t *e = NULL;
    int32_t n, i, best = -1;
    float best_v = 0.0f;
    if (!ops) return -1;
    if (ops->refresh_nearby) ops->refresh_nearby(ops->context);
    n = ops->nearby ? ops->nearby(ops->context, 3, &e) : 0;
    for (i = 0; i < n; ++i) {
        const uint8_t *it = e + 8 * i;
        int32_t id = (int32_t)((uint32_t)it[0] | (uint32_t)it[1] << 8 |
                               (uint32_t)it[2] << 16 | (uint32_t)it[3] << 24);
        int16_t cls, max_hp;
        float hp, d, v;
        uint8_t state;
        if (!ops->owner_enemy || !ops->owner_enemy(ops->context, it[5])) continue;
        if (!ops->target_view || !ops->target_view(ops->context, id, &cls, &hp, &max_hp, &state) ||
            state >= 3)
            continue;
        if (!ops->call_c4 || !ops->call_c4(ops->context, id)) continue;
        d = (float)it[4];
        if (d < 1.0f) d = 1.0f;
        v = (float)max_hp / d;
        if (best == -1 || best_v < v) {
            best = id;
            best_v = v;
            if (dist) *dist = d;
        }
    }
    return best;
}

int32_t aoe_ai_unit_ai_priest_d8_ror(const AoeAiUnitAiOrderRor *order,
                                     const AoeAiUnitAiOpsRor *ops) {
    AoeAiUnitViewRor me;
    int32_t t;
    if (!order || !ops || !ops->owner_field_4 || ops->owner_field_4(ops->context) != 1)
        return 9;
    if (!ops->self) return 9;
    ops->self(ops->context, &me);
    if (!(100.0f <= me.field_44)) return 9;
    if (ops->head_state && ops->head_state(ops->context) == 7) return 9;
    t = ops->call_30 ? ops->call_30(ops->context) : -1;
    if (t == -1 || order->target == t) return 9;
    if (order->action != -1) {
        if (ops->call_10) ops->call_10(ops->context);
        if (ops->call_58) ops->call_58(ops->context, 1);
    }
    return ops->call_5c_r && ops->call_5c_r(ops->context, t, 1) == 1 ? 10 : 9;
}

int32_t aoe_ai_unit_ai_5564_d4_ror(const AoeAiUnitAiOrderRor *order, int32_t first,
                                   const AoeAiUnitAiOpsRor *ops) {
    int16_t type;
    int32_t t, owner4;
    if (!order || !ops) return 5;
    if (first == 1 && ops->call_dc && ops->call_dc(ops->context) == 8) return 6;
    type = ops->unit_type ? ops->unit_type(ops->context) : 0;
    owner4 = ops->owner_field_4 ? ops->owner_field_4(ops->context) : 0;
    if ((ops->owner_index ? ops->owner_index(ops->context) : 0) == 0) return 5;
    if (owner4 == 0 && type == 299) return 5;
    if (order->action == 0x2d5 &&
        (type == 0x23 || type == 0x24 || type == 0x118 || type == 0xfa || type == 0x115))
        return 5;
    t = ops->call_30 ? ops->call_30(ops->context) : -1;
    if (t != -1 && ops->call_5c_r && ops->call_5c_r(ops->context, t, 1) == 1) return 6;
    if (owner4 == 0 || (ops->unit_check && ops->unit_check(ops->context) == 1)) return 5;
    if (ops->settings_8fc == 1 && ops->settings_97d == 1 &&
        !(ops->owner_3c_8f && ops->owner_3c_8f(ops->context)))
        return 5;
    if (ops->settings_97d == 1 && !(ops->owner_3c_8f && ops->owner_3c_8f(ops->context)))
        return 5;
    if (order->action != 0x2bd && order->action != 0x2d5) {
        AoeAiUnitViewRor me, o;
        int32_t target = ops->target ? ops->target(ops->context) : -1;
        if (target != -1 && ops->object && ops->object(ops->context, target, &o) && ops->self) {
            double r, dx, dy;
            float d;
            ops->self(ops->context, &me);
            r = (ops->speed ? ops->speed(ops->context) : 0.0) * (double)3.0f;
            dx = (double)o.x - (double)me.x;
            dy = (double)o.y - (double)me.y;
            d = (float)sqrt(dx * dx + dy * dy);
            if (ops->move)
                ops->move(ops->context, (float)(-(dx / (double)d * r) + (double)me.x),
                          (float)(-(dy / (double)d * r) + (double)me.y), me.z);
            return 6;
        }
    }
    return 5;
}

int32_t aoe_ai_unit_ai_5644_cc_ror(const AoeAiUnitAiOrderRor *order,
                                   const AoeAiUnitAiMessageRor *msg,
                                   const AoeAiUnitAiOpsRor *ops) {
    if (!order || !msg || !ops) return 2;
    switch (msg->type) {
    case 500: case 0x1f6: case 0x1f7: case 0x1f8: case 0x1fd: case 0x1fe: case 0x1ff:
    case 0x200: case 0x202: case 0x203: case 0x20b: case 0x20f: case 0x259: case 0x25a:
    case 0x25b: case 0x25c: case 0x25d: case 0x25e: case 0x25f: case 0x260: case 0x261:
    case 0x262: case 0x265: case 0x266: case 0x267: case 0x268: case 0x269: case 699:
        return 2;
    case 0x1f9: case 0x1fa: case 0x1fb:
        if (order->action != -1) {
            if (order->task + 100 != order->action) return 2;
        } else if (order->task != msg->value) {
            return 2;
        }
        break;
    case 0x1fc: {
        AoeAiUnitViewRor t;
        if (ops->object && ops->object(ops->context, order->target, &t) && ops->vision &&
            ops->vision(ops->context, aoe_ftol32(t.x), aoe_ftol32(t.y)) == 0xf)
            return 2;
        break;
    }
    case 600:
        if (ops->group_add) ops->group_add(ops->context, msg->sender);
        if (ops->unit_ec) ops->unit_ec(ops->context, 1);
        return 2;
    default:
        break;
    }
    if (ops->clear_target) ops->clear_target(ops->context);
    if (ops->call_58) ops->call_58(ops->context, 1);
    if (ops->call_1c) ops->call_1c(ops->context, (void *)msg);
    return 2;
}

static int32_t base_cc(const AoeAiUnitAiMessageRor *msg, int32_t arg,
                       const AoeAiUnitAiOpsRor *ops) {
    return ops->base_cc ? ops->base_cc(ops->context, (void *)msg, arg) : 0;
}

static int32_t join_500(const AoeAiUnitAiMessageRor *msg, int32_t arg, int type30,
                        const AoeAiUnitAiOpsRor *ops) {
    if (!msg || !ops) return 0;
    if (msg->type != 500) return msg->type == 0x20f ? 2 : base_cc(msg, arg, ops);
    if (msg->field_10 < 1) {
        if (ops->base_414eb0) ops->base_414eb0(ops->context, arg);
        return 4;
    }
    if (ops->group_add) ops->group_add(ops->context, msg->sender);
    if (ops->unit_ec) ops->unit_ec(ops->context, 1);
    if (type30) {
        int32_t t = ops->call_3c ? ops->call_3c(ops->context, 0) : -1;
        if (ops->call_5c) ops->call_5c(ops->context, t, 0);
    } else if (ops->call_a4) {
        ops->call_a4(ops->context, 1);
    }
    return 3;
}

int32_t aoe_ai_unit_ai_class9_cc_ror(const AoeAiUnitAiMessageRor *msg, int32_t arg,
                                     const AoeAiUnitAiOpsRor *ops) {
    return join_500(msg, arg, 0, ops);
}

int32_t aoe_ai_unit_ai_type30_cc_ror(const AoeAiUnitAiMessageRor *msg, int32_t arg,
                                     const AoeAiUnitAiOpsRor *ops) {
    return join_500(msg, arg, 1, ops);
}

int32_t aoe_ai_unit_ai_7e_cc_ror(AoeAiUnitAiRor *ai, const AoeAiUnitAiOrderRor *order,
                                 const AoeAiUnitAiMessageRor *msg, int32_t arg,
                                 const AoeAiUnitAiOpsRor *ops) {
    if (!ai || !order || !msg || !ops) return 0;
    if (msg->type == 0x1fc) {
        AoeAiUnitViewRor t, me;
        if (ops->object && ops->object(ops->context, order->target, &t) && ops->self) {
            double d = ops->distance_to ? ops->distance_to(ops->context, order->target) : 0.0;
            ops->self(ops->context, &me);
            if (d <= (double)me.type_28) {
                if (ops->call_5c) ops->call_5c(ops->context, order->target, 1);
                return 3;
            }
        }
        if (ops->clear_target) ops->clear_target(ops->context);
        if (ops->call_58) ops->call_58(ops->context, 1);
        if (ops->call_1c) ops->call_1c(ops->context, (void *)msg);
        return base_cc(msg, arg, ops);
    }
    if (msg->type == 0x1fa && msg->value == 600) {
        if (ops->settings_8fc == 1 && ops->settings_97d == 1)
            ai->timer = ((ops->difficulty ? ops->difficulty(ops->context) : 0) + 3) * 5000;
        else
            ai->timer = 25000;
        if (ops->restart_timer) ops->restart_timer(ops->context, ai->timer);
        if (ops->clear_target) ops->clear_target(ops->context);
        if (ops->call_58) ops->call_58(ops->context, 1);
    }
    return base_cc(msg, arg, ops);
}
