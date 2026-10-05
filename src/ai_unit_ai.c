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

int32_t aoe_ai_unit_ai_priest_d4_ror(const AoeAiUnitAiOpsRor *ops) {
    const uint8_t *e;
    int32_t n, r, t;
    if (!ops || !ops->owner_field_4 || !ops->owner_field_4(ops->context)) return 5;
    if (ops->base_d4 && ops->base_d4(ops->context, 1) == 6) return 6;
    n = ops->nearby ? ops->nearby(ops->context, 3, &e) : 0;
    r = ops->rand ? ops->rand(ops->context) : 0;
    if (n > 0 && n < r % (n + 2) && ops->call_e0 &&
        (t = ops->call_e0(ops->context, 1, 0)) != -1) {
        if (ops->call_70) ops->call_70(ops->context, t, 0);
        return 6;
    }
    t = ops->call_40_2 ? ops->call_40_2(ops->context) : -1;
    if (t != -1 && ops->object && ops->self) {
        AoeAiUnitViewRor o, me;
        if (ops->object(ops->context, t, &o)) {
            float dx, dy, d;
            double extra;
            ops->self(ops->context, &me);
            dx = o.x - me.x;
            dy = o.y - me.y;
            d = sqrtf(dx * dx + dy * dy);
            extra = (ops->range_10c ? ops->range_10c(ops->context) : 0.0) - (double)d;
            if (0.0 < extra) {
                if (ops->move)
                    ops->move(ops->context, (float)(-((double)dx / (double)d * extra) + (double)me.x),
                              (float)(-((double)dy / (double)d * extra) + (double)me.y), me.z);
                return 6;
            }
        }
    }
    return 5;
}

int32_t aoe_ai_unit_ai_priest_cc_ror(const AoeAiUnitAiOrderRor *order,
                                     const AoeAiUnitAiMessageRor *msg, int32_t arg,
                                     const AoeAiUnitAiOpsRor *ops) {
    int32_t t;
    if (!order || !msg || !ops) return 0;
    switch (msg->type) {
    case 0x1fc: {
        AoeAiUnitViewRor o;
        if (ops->object && ops->object(ops->context, order->target, &o) && ops->owner_view &&
            ops->owner_view(ops->context, aoe_ftol32(o.x), aoe_ftol32(o.y)) == 0xf)
            return 2;
        if (ops->clear_target) ops->clear_target(ops->context);
        if (ops->call_58) ops->call_58(ops->context, 1);
        if (ops->call_1c) ops->call_1c(ops->context, (void *)msg);
        return base_cc(msg, arg, ops);
    }
    case 0x1f9: case 0x1fb: case 0x202:
        break;
    default:
        return base_cc(msg, arg, ops);
    }
    if (!(ops->owner_field_4 && ops->owner_field_4(ops->context)) && order->task == 0x25c) {
        if (ops->clear_target) ops->clear_target(ops->context);
        if (ops->call_58) ops->call_58(ops->context, 1);
    } else {
        if (order->task == 0x25c) {
            if (ops->call_e4 && (t = ops->call_e4(ops->context, 0)) != -1 && ops->call_6c &&
                ops->call_6c(ops->context, t, 1) == 1)
                return 3;
        } else if (order->task == 0x25b && ops->call_e0 &&
                   (t = ops->call_e0(ops->context, 0, 0)) != -1 && ops->call_70 &&
                   ops->call_70(ops->context, t, 1) == 1) {
            return 3;
        }
        if (ops->clear_target) ops->clear_target(ops->context);
        if (ops->call_58) ops->call_58(ops->context, 0);
    }
    if (ops->call_1c) ops->call_1c(ops->context, (void *)msg);
    return 2;
}

int32_t aoe_ai_unit_ai_class14_cc_ror(const AoeAiUnitAiOrderRor *order,
                                      const AoeAiUnitAiMessageRor *msg, int32_t arg,
                                      const AoeAiUnitAiOpsRor *ops);

/* The shared start of the "attacked" handlers: 4 when +0x10 is below 1,
   else the sender is noticed, warned about and listed (0). */
static int32_t attacked_head(const AoeAiUnitAiOrderRor *order, const AoeAiUnitAiMessageRor *msg,
                             int32_t eb_arg, const AoeAiUnitAiOpsRor *ops) {
    int32_t owner4;
    if (msg->field_10 < 1) {
        if (ops->owner_e8) ops->owner_e8(ops->context, 0x209, msg->sender);
        if (ops->base_414eb0) ops->base_414eb0(ops->context, eb_arg);
        return 4;
    }
    owner4 = ops->owner_field_4 ? ops->owner_field_4(ops->context) : 0;
    if ((ops->group_count ? ops->group_count(ops->context) : 0) == 0 || owner4 == 1)
        if (ops->owner_e8) ops->owner_e8(ops->context, 0x201, msg->sender);
    if (!(ops->group_has && ops->group_has(ops->context, msg->sender)) && owner4 == 0 &&
        order->target != msg->sender && ops->alert_allies)
        ops->alert_allies(ops->context, msg->sender);
    if (ops->group_add) ops->group_add(ops->context, msg->sender);
    if (ops->unit_ec) ops->unit_ec(ops->context, 1);
    return 0;
}

int32_t aoe_ai_unit_ai_attacked_ror(const AoeAiUnitAiOrderRor *order,
                                    const AoeAiUnitAiMessageRor *msg, int32_t arg,
                                    const AoeAiUnitAiOpsRor *ops) {
    int32_t r;
    if (!order || !msg || !ops) return 0;
    if (msg->type != 500 && msg->type != 0x20f) return base_cc(msg, arg, ops);
    if ((r = attacked_head(order, msg, 0, ops)) != 0) return r;
    if (!(ops->owner_field_4 && ops->owner_field_4(ops->context)) && order->action != -1)
        return 2;
    if (order->task != 0x262) {
        if (ops->call_10) ops->call_10(ops->context);
        if (ops->call_58) ops->call_58(ops->context, 1);
    }
    if (ops->call_a4) ops->call_a4(ops->context, 1);
    return 3;
}

static void stop_and_tell(const AoeAiUnitAiMessageRor *msg, const AoeAiUnitAiOpsRor *ops) {
    if (ops->clear_target) ops->clear_target(ops->context);
    if (ops->call_58) ops->call_58(ops->context, 1);
    if (ops->call_1c) ops->call_1c(ops->context, (void *)msg);
}

int32_t aoe_ai_unit_ai_class2_cc_ror(const AoeAiUnitAiOrderRor *order,
                                     const AoeAiUnitAiMessageRor *msg, int32_t arg,
                                     const AoeAiUnitAiOpsRor *ops) {
    if (!order || !msg || !ops) return 0;
    if (msg->type != 0x202) return aoe_ai_unit_ai_attacked_ror(order, msg, arg, ops);
    if (order->task == 0x267) {
        int which;
        for (which = 0; which < 2; ++which) {
            int32_t t = ops->call_40_dock ? ops->call_40_dock(ops->context, which) : -1;
            if (t != -1 && ops->call_7c && ops->call_7c(ops->context, t, 1) == 1) return 6;
        }
    } else if (order->task == 0x25d) {
        int32_t x, y;
        int ok = 0;
        if (ops->call_48 && ops->call_48(ops->context, &x, &y) == 1) {
            if (ops->add_waypoint) ops->add_waypoint(ops->context, x, y);
            ok = ops->call_80 && ops->call_80(ops->context, x, y) != 0;
        }
        if (!ok) stop_and_tell(msg, ops);
    }
    stop_and_tell(msg, ops);
    return 2;
}

int32_t aoe_ai_unit_ai_5804_cc_ror(const AoeAiUnitAiOrderRor *order,
                                   const AoeAiUnitAiMessageRor *msg, int32_t arg,
                                   const AoeAiUnitAiOpsRor *ops) {
    int32_t r;
    if (!order || !msg || !ops) return 2;
    switch (msg->type) {
    case 500:
        if ((r = attacked_head(order, msg, arg, ops)) != 0) return r;
        return 2;
    case 600:
        if (ops->group_add) ops->group_add(ops->context, msg->sender);
        if (ops->unit_ec) ops->unit_ec(ops->context, 1);
        return 2;
    case 699:
        if (ops->group_has && ops->group_has(ops->context, msg->sender) && ops->group_remove)
            ops->group_remove(ops->context, msg->sender);
        return 2;
    default:
        return 2;
    }
}

int32_t aoe_ai_unit_ai_class14_cc_ror(const AoeAiUnitAiOrderRor *order,
                                      const AoeAiUnitAiMessageRor *msg, int32_t arg,
                                      const AoeAiUnitAiOpsRor *ops) {
    int32_t r;
    if (!order || !msg || !ops) return 0;
    if (msg->type != 500 && msg->type != 0x20f) return base_cc(msg, arg, ops);
    if ((r = attacked_head(order, msg, 0, ops)) != 0) return r;
    if (order->action != -1) return 2;
    if (ops->head_state) {
        int st = ops->head_state(ops->context);
        if (st != -1 && st != 2) return 3;
    }
    if (ops->call_a4) ops->call_a4(ops->context, 1);
    return 3;
}

static float sidestep(float at, const AoeAiUnitAiOpsRor *ops) {
    int32_t r = (ops->rand ? ops->rand(ops->context) : 0) % 8;
    float v = (float)((double)at - (double)4.0f + (double)r);
    if (v == at) v = v <= 10.0f ? v + 2.0f : v - 2.0f;
    return v;
}

int32_t aoe_ai_unit_ai_5564_cc_ror(const AoeAiUnitAiOrderRor *order,
                                   const AoeAiUnitAiMessageRor *msg, int32_t arg,
                                   const AoeAiUnitAiOpsRor *ops) {
    AoeAiUnitViewRor me, t;
    if (!order || !msg || !ops || !ops->self) return 0;
    ops->self(ops->context, &me);
    if (msg->type == 0x200) {
        double range, gap, dist;
        float dx, dy, step;
        int32_t i, n;
        if (!(ops->owner_field_4 && ops->owner_field_4(ops->context))) return base_cc(msg, arg, ops);
        range = ops->range_10c ? ops->range_10c(ops->context) : 0.0;
        if (range < 1.0 || (ops->unit_class && ops->unit_class(ops->context) == 0x1a))
            return base_cc(msg, arg, ops);
        if (order->task != 600) return 2;
        if (!ops->object || !ops->object(ops->context, order->target, &t))
            return base_cc(msg, arg, ops);
        gap = (double)(float)range - (ops->distance_to ? ops->distance_to(ops->context, order->target)
                                                       : 0.0);
        step = (float)gap;
        if (step < 1.0f) return base_cc(msg, arg, ops);
        dx = t.x - me.x;
        dy = t.y - me.y;
        if (ops->reissue) ops->reissue(ops->context, 0x2bc, 1);
        n = ops->group_count ? ops->group_count(ops->context) : 0;
        for (i = 0; i < n; ++i) {
            AoeAiUnitViewRor m;
            int32_t id = ops->group_at ? ops->group_at(ops->context, i) : -1;
            if (id == order->target || !ops->object(ops->context, id, &m)) continue;
            dx = (float)((double)(m.x - me.x) + (double)dx);
            dy = (float)((double)(m.y - me.y) + (double)dy);
        }
        dist = sqrt((double)dy * dy + (double)dx * dx);
        return ops->call_a0 &&
                       ops->call_a0(ops->context,
                                    (float)(-((double)dx / dist * (double)step) + (double)me.x),
                                    (float)(-((double)dy / dist * (double)step) + (double)me.y),
                                    me.z, 0, 1) == 1
                   ? 3
                   : 2;
    }
    if (msg->type == 0x20f) {
        int16_t type = 0;
        float x, y;
        if (!(ops->owner_field_4 && ops->owner_field_4(ops->context)) || !ops->world_type ||
            !ops->world_type(ops->context, msg->sender, &type) ||
            !(type == 0x23 || type == 0x24 || type == 0x118) || order->task == -1)
            return base_cc(msg, arg, ops);
        if (order->action != -1 && ops->reissue) ops->reissue(ops->context, order->action, 0);
        x = sidestep(me.x, ops);
        y = sidestep(me.y, ops);
        if (ops->call_a0 && ops->call_a0(ops->context, x, y, me.z, 1, 1) == 1) {
            if (ops->set_dodged) ops->set_dodged(ops->context);
            return 3;
        }
    }
    return base_cc(msg, arg, ops);
}

static void stop_pass(const AoeAiUnitAiMessageRor *msg, const AoeAiUnitAiOpsRor *ops) {
    if (ops->clear_target) ops->clear_target(ops->context);
    if (ops->call_58) ops->call_58(ops->context, 1);
    if (ops->call_1c) ops->call_1c(ops->context, (void *)msg);
}

static int32_t villager_attacked(AoeAiUnitAiOrderRor *order, const AoeAiUnitAiMessageRor *msg,
                                 int32_t arg, const AoeAiUnitAiOpsRor *ops) {
    int16_t a_owner = 0, a_cls = 0, t_owner = 0, t_cls = 0;
    int t_ok, owner4, b134;
    double f4;
    if (msg->field_10 < 1) {
        if (ops->owner_e8) ops->owner_e8(ops->context, 0x209, msg->sender);
        if (ops->base_414eb0) ops->base_414eb0(ops->context, arg);
        return 4;
    }
    if (!ops->obj_info || !ops->obj_info(ops->context, msg->sender, &a_owner, &a_cls)) return 2;
    t_ok = ops->obj_info(ops->context, order->target, &t_owner, &t_cls);
    owner4 = ops->owner_field_4 ? ops->owner_field_4(ops->context) : 0;
    if (owner4 == 1 ? !(a_owner == 0 && msg->sender == order->target)
                    : (ops->group_count ? ops->group_count(ops->context) : 0) == 0)
        if (ops->owner_e8) ops->owner_e8(ops->context, 0x201, msg->sender);
    if (!(ops->group_has && ops->group_has(ops->context, msg->sender)) && owner4 == 0 &&
        order->target != msg->sender && ops->alert_allies)
        ops->alert_allies(ops->context, msg->sender);
    if (ops->group_add) ops->group_add(ops->context, msg->sender);
    if (ops->unit_ec) ops->unit_ec(ops->context, 1);
    f4 = ops->unit_f4 ? ops->unit_f4(ops->context) : 0.0;
    b134 = ops->unit_134 ? ops->unit_134(ops->context) & 0xff : 0;
    if ((order->action == 0x2c6 && (float)f4 > 0.0f) || order->action == 700 ||
        (order->action == 0x2c9 && b134 == 0xc && t_ok && t_cls == 10 && a_owner == 0) ||
        order->action == 0x2ce)
        return 2;
    if (a_owner != 0) {
        if (order->task != 0x262 || (float)f4 == 0.0f) {
            if (ops->reissue) ops->reissue(ops->context, order->action, 0);
            if (ops->call_58) ops->call_58(ops->context, 1);
            if (ops->call_a4) ops->call_a4(ops->context, 1);
        }
        return 3;
    }
    if (order->action != -1) {
        if (ops->reissue) ops->reissue(ops->context, order->action, 0);
        order->action = 700;
        order->field_2c = 100;
        if (ops->call_5c) ops->call_5c(ops->context, msg->sender, 1);
        order->flag_130 = 1;
        return 3;
    }
    if (ops->call_5c) ops->call_5c(ops->context, msg->sender, 1);
    return 3;
}

/* The next resource for a gatherer (task 0x265 / 0x261): -2 when there
   is no known kind (stop now), -1 for none found. */
static int32_t next_resource(const AoeAiUnitAiOrderRor *order, const AoeAiUnitAiMessageRor *msg,
                             int gatherer_261, const AoeAiUnitAiOpsRor *ops) {
    int32_t from = order->field_38, k;
    if (from == -1 && (from = order->field_5c) == -1) return -2;
    k = ops->call_c0 ? ops->call_c0(ops->context, from) : -1;
    if (k != -1) return ops->call_44 ? ops->call_44(ops->context, k) : -1;
    if (gatherer_261 && msg->field_10 == 0x32)
        return ops->call_40_args ? ops->call_40_args(ops->context, from, -1, 2, 0x32) : -1;
    return ops->call_40_args ? ops->call_40_args(ops->context, from, -1, -1, -1) : -1;
}

int32_t aoe_ai_unit_ai_villager_cc_ror(AoeAiUnitAiOrderRor *order,
                                       const AoeAiUnitAiMessageRor *msg, int32_t arg,
                                       const AoeAiUnitAiOpsRor *ops) {
    if (!order || !msg || !ops) return 0;
    switch (msg->type) {
    case 0x20f:
        if ((ops->difficulty ? ops->difficulty(ops->context) : 0) >= 3) return 2;
        /* fall through */
    case 500:
        return villager_attacked(order, msg, arg, ops);
    case 0x1f9: case 0x1fa:
        if (order->task != msg->value) return 2;
        stop_pass(msg, ops);
        return 2;
    case 0x1fb: case 0x202:
        break;
    default:
        return base_cc(msg, arg, ops);
    }
    if (order->task == 0x265 || order->task == 0x261) {
        int g261 = order->task == 0x261;
        int32_t t = next_resource(order, msg, g261, ops);
        if (t == -2) {
            stop_pass(msg, ops);
            return 2;
        }
        if (t != -1) {
            int32_t r = g261 ? (ops->call_68 ? ops->call_68(ops->context, t, 1) : 0)
                             : (ops->call_64_r ? ops->call_64_r(ops->context, t, 1) : 0);
            if (r == 1) return 3;
            stop_pass(msg, ops);
        } else if (ops->owner_field_4 && ops->owner_field_4(ops->context) == 1) {
            stop_pass(msg, ops);
        } else if (g261 && msg->field_10 == 0x32 && ops->self && ops->move) {
            AoeAiUnitViewRor me;
            ops->self(ops->context, &me);
            ops->move(ops->context, me.x <= 5.0f ? me.x + 2.5f : me.x - 2.5f,
                      me.y <= 5.0f ? me.y + 2.5f : me.y - 2.5f, me.z);
        }
    } else if (order->task == 0x25a) {
        int32_t t = ops->call_40_args ? ops->call_40_args(ops->context, order->field_38, 3, 0, -1)
                                      : -1;
        if (t == -1 && ops->call_40_args)
            t = ops->call_40_args(ops->context, order->field_38 == 0x1b ? 3 : 0x1b, 3, 0, -1);
        if (t != -1 && ops->unit_194 && ops->unit_194(ops->context, t) == 1) {
            if (ops->call_78 && ops->call_78(ops->context, t, 1) == 1) return 3;
        } else {
            AoeAiUnitViewRor tv;
            if (!ops->object || !ops->object(ops->context, order->target, &tv) ||
                (ops->distance_to ? ops->distance_to(ops->context, order->target) : 0.0) > 1.0)
                if (ops->call_90) ops->call_90(ops->context, order->x, order->y, order->z);
        }
    } else if (order->task == 0x25d) {
        int32_t x, y;
        if (ops->call_48 && ops->call_48(ops->context, &x, &y) == 1) {
            if (ops->add_waypoint) ops->add_waypoint(ops->context, x, y);
            if (ops->call_80 && ops->call_80(ops->context, x, y) != 0) return 3;
        }
        stop_pass(msg, ops);
    }
    if (msg->type == 0x202) return 2;
    stop_pass(msg, ops);
    return 2;
}

static int32_t near_id(const uint8_t *it) {
    return (int32_t)((uint32_t)it[0] | (uint32_t)it[1] << 8 | (uint32_t)it[2] << 16 |
                     (uint32_t)it[3] << 24);
}

int32_t aoe_ai_unit_ai_most_damaged_ror(const AoeAiUnitAiOpsRor *ops, float *out) {
    const uint8_t *e = NULL;
    int32_t n, i, best = -1;
    float best_v = 0.0f;
    if (!ops) return -1;
    if (ops->refresh_nearby) ops->refresh_nearby(ops->context);
    n = ops->nearby ? ops->nearby(ops->context, 3, &e) : 0;
    for (i = 0; i < n; ++i) {
        int32_t id = near_id(e + 8 * i);
        int16_t cls, max_hp;
        float hp, v;
        uint8_t state;
        if (!ops->target_view || !ops->target_view(ops->context, id, &cls, &hp, &max_hp, &state))
            continue;
        v = (float)(ops->damage_to ? ops->damage_to(ops->context, id) : 0.0);
        if (best == -1 || best_v < v) {
            best_v = v;
            best = id;
        }
    }
    if (out) *out = best_v;
    return best;
}

int32_t aoe_ai_unit_ai_weakest_ror(const AoeAiUnitAiOpsRor *ops, float *out) {
    const uint8_t *e = NULL;
    int32_t n, i, best = -1;
    float best_v = 0.0f;
    if (!ops) return -1;
    if (ops->refresh_nearby) ops->refresh_nearby(ops->context);
    n = ops->nearby ? ops->nearby(ops->context, 3, &e) : 0;
    for (i = 0; i < n; ++i) {
        int32_t id = near_id(e + 8 * i);
        int16_t cls, max_hp;
        float hp;
        uint8_t state;
        if (!ops->target_view || !ops->target_view(ops->context, id, &cls, &hp, &max_hp, &state))
            continue;
        if (best == -1 || hp < best_v) {
            best_v = hp;
            best = id;
        }
    }
    if (out) *out = best_v;
    return best;
}

int32_t aoe_ai_unit_ai_report_nearby_ror(AoeAiUnitAiOrderRor *ai, const AoeAiUnitAiOpsRor *ops) {
    int band;
    int32_t r;
    if (!ai || !ops) return 0;
    if (ops->refresh_nearby) ops->refresh_nearby(ops->context);
    for (band = 3; band >= 2; --band) {
        const uint8_t *e = NULL;
        int32_t n = ops->nearby ? ops->nearby(ops->context, band, &e) : 0, i;
        for (i = 0; i < n; ++i) {
            int32_t id = near_id(e + 8 * i);
            int16_t cls, max_hp;
            float hp;
            uint8_t state;
            if (ops->target_view && ops->target_view(ops->context, id, &cls, &hp, &max_hp, &state) &&
                state < 3 && ops->owner_e8)
                ops->owner_e8(ops->context, 0x20d, id);
        }
    }
    ai->field_108 = 0;
    r = ops->rand ? ops->rand(ops->context) : 0;
    ai->field_10c = r % 1000 + 4000;
    return r / 1000;
}

int32_t aoe_ai_unit_ai_base_d4_ror(const AoeAiUnitAiOpsRor *ops, int32_t first) {
    int32_t t;
    if (!ops) return 5;
    if (first == 1 && ops->call_dc && ops->call_dc(ops->context) == 8) return 6;
    t = ops->call_30_args ? ops->call_30_args(ops->context, 1, 0, 0) : -1;
    if (t != -1 && ops->call_5c_r && ops->call_5c_r(ops->context, t, 1) == 1) return 6;
    return 5;
}
