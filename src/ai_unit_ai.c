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

void aoe_ai_unit_ai_play_message_ror(const int32_t msg[3], const AoeAiUnitAiPlayOpsRor *ops) {
    int i;
    if (!msg || !ops || !ops->play_ready || !ops->play_ready(ops->context) || msg[2] != 500)
        return;
    if (ops->set_play_value) ops->set_play_value(ops->context, msg[0]);
    for (i = 0; i < 3; ++i) {
        uint8_t kind = 0, index = 0;
        uint32_t value = 0;
        if (!ops->trigger || !ops->trigger(ops->context, i, &kind, &value, &index) || kind == 0)
            continue;
        if (kind == 5 ||
            (kind == 4 && ops->category && value == ops->category(ops->context, msg[1]))) {
            if (ops->run) ops->run(ops->context, index, 0);
        }
    }
}

int32_t aoe_ai_unit_ai_nearest_listed_ror(const AoeAiUnitAiOpsRor *ops, float *out) {
    int32_t n, i, best = -1;
    float best_v = 0.0f;
    if (!ops) return -1;
    n = ops->group_count ? ops->group_count(ops->context) : 0;
    for (i = 0; i < n; ++i) {
        int32_t id = ops->group_at ? ops->group_at(ops->context, i) : -1;
        int16_t type_id;
        float d;
        if (!ops->world_type || !ops->world_type(ops->context, id, &type_id)) continue;
        d = (float)(ops->distance_to ? ops->distance_to(ops->context, id) : 0.0);
        if (best == -1 || d < best_v) {
            best = id;
            best_v = d;
        }
    }
    if (out) *out = best_v;
    return best;
}

static int nearest_band_ok(int32_t mode, int band) {
    switch (mode) {
    case 0: return band == 1 || band == 2;
    case 1: return band == 3;
    case 2: return band == 4;
    case 3: return band == 1;
    default: return 1;
    }
}

int32_t aoe_ai_unit_ai_nearest_nearby_ror(const AoeAiUnitAiOpsRor *ops, int32_t type_class,
                                          int32_t mode, int32_t state, int32_t type_id,
                                          int32_t *out, int32_t *players) {
    int seen[9];
    int band, p, count = 0;
    int32_t best = -1, self_id;
    float best_d = -1.0f;
    AoeAiUnitViewRor me;
    if (!ops) return -1;
    memset(&me, 0, sizeof me);
    if (ops->self) ops->self(ops->context, &me);
    self_id = me.id;
    memset(seen, 0, sizeof seen);
    if (ops->refresh_nearby) ops->refresh_nearby(ops->context);
    for (band = 0; band < 5; ++band) {
        const uint8_t *e = NULL;
        int32_t n = ops->nearby ? ops->nearby(ops->context, band, &e) : 0, i;
        for (i = 0; i < n; ++i) {
            const uint8_t *it = e + 8 * i;
            int32_t id = near_id(it);
            int16_t cls = 0, max_hp, tid = 0;
            float hp;
            uint8_t st = 0;
            if (it[5] < 9) seen[it[5]] = 1;
            if (id == self_id || !nearest_band_ok(mode, band)) continue;
            if (!ops->target_view || !ops->target_view(ops->context, id, &cls, &hp, &max_hp, &st))
                continue;
            if (st >= 7) continue;
            if (ops->world_type) ops->world_type(ops->context, id, &tid);
            if (type_class != -1 && cls != type_class) continue;
            if (type_id != -1 && tid != type_id) continue;
            if (state != -1 && (int32_t)st != state) continue;
            if (best == -1 || (float)it[4] < best_d) {
                best_d = (float)it[4];
                best = id;
            }
        }
    }
    for (p = 1; p <= 8; ++p)
        if (seen[p]) ++count;
    if (players) *players = count;
    if (out) *out = (int32_t)best_d;
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

int aoe_ai_unit_ai_kind_for_unit_ror(int16_t class_code, int16_t id,
                                     int *kind, int32_t *arg) {
    int k;
    int32_t a = 10;
    switch ((uint16_t)class_code) {
    case 1:
        if (id == 99 || id == 10) return 0;
        k = AOE_AI_UNIT_AI_KIND_5644; a = 5; break;
    case 2: k = AOE_AI_UNIT_AI_CLASS_2; break;
    case 3: case 0x1b:
        k = (id == 0x4f || id == 199 || id == 0x45 || id == 0x116 ||
             id == 0x17f || id == 0x17c) ? AOE_AI_UNIT_AI_KIND_5724
                                         : AOE_AI_UNIT_AI_KIND_5804;
        break;
    case 4: k = AOE_AI_UNIT_AI_CLASS_4; break;
    case 5: case 7: case 8: case 0xb: case 0xe: case 0xf: case 0x10:
    case 0x1d: case 0x1e: case 0x1f: case 0x20: case 0x21:
        return 0;
    case 9: k = AOE_AI_UNIT_AI_CLASS_9; break;
    case 10:
        k = id == 0x9d ? AOE_AI_UNIT_AI_CLASS_9 :
            id == 0x30 ? AOE_AI_UNIT_AI_TYPE_30 :
            id == 0x7e ? AOE_AI_UNIT_AI_TYPE_7E : AOE_AI_UNIT_AI_CLASS_10;
        break;
    case 0x12: k = AOE_AI_UNIT_AI_PRIEST; break;
    case 0x14: k = AOE_AI_UNIT_AI_CLASS_14; break;
    case 0x15: k = AOE_AI_UNIT_AI_CLASS_15; break;
    default: k = AOE_AI_UNIT_AI_KIND_5564; break;
    }
    if (kind) *kind = k;
    if (arg) *arg = a;
    return 1;
}

/* ---- 0x00410af0 ---- */
int32_t aoe_ai_unit_ai_scan_ror(const AoeAiUnitAiRor *ai, int32_t order, int32_t target_id,
                                int *scanned_flag, const AoeAiUnitAiScanOpsRor *o) {
    int16_t owner;
    float sight;
    int has_ai;
    int32_t x, y, radius;
    if (!ai || !o || !scanned_flag || *scanned_flag == 1) return 0;
    *scanned_flag = 1;
    if (ai->class_count == 0 && ai->classes[0] == 0) return 0; /* +0x124 unset */
    if (order == 0x2bd && target_id != -1 && o->target &&
        o->target(o->context, target_id, &owner, &sight, &has_ai) && has_ai) {
        radius = (int32_t)sight;
    } else {
        radius = (int32_t)o->own_sight(o->context);
        if (o->own_type(o->context) == 0x76) radius *= 2;
        owner = o->own_owner(o->context);
    }
    o->own_tile(o->context, &x, &y);
    return o->collect(o->context, x, y, radius, owner, ai->classes, ai->class_count);
}

/* ---- 0x004118f0 ---- */
int32_t aoe_ai_unit_ai_remembered_ror(const AoeAiUnitAiMemoryOpsRor *o, uint32_t *cost) {
    int32_t excluded[100], n = 0, object = -1;
    uint8_t c = 0;
    if (!o) return -1;
    for (n = 0; n < 100; ++n) {
        uint8_t zone = o->zone(o->context, excluded, n);
        if (!o->memory(o->context, zone, excluded, n, &object, &c)) return -1;
        if (o->own_sight(o->context) + 5.0f < (float)c) return -1;
        if (o->reachable(o->context, object) == 1) break;
        excluded[n] = zone;
    }
    if (cost) *cost = c;
    return object;
}

/* ---- 0x004131c0 ---- */
int32_t aoe_ai_unit_ai_rally_ror(int32_t target_id, const AoeAiUnitAiRallyOpsRor *o) {
    float tx, ty;
    int16_t towner;
    int32_t sent = 0, b, i;
    if (!o || !o->target(o->context, target_id, &tx, &ty, &towner)) return 0;
    o->tell_unit(o->context);
    for (b = 1; b <= 2; ++b) {
        int32_t n = o->bucket_count(o->context, b);
        for (i = 0; i < n; ++i) {
            int32_t id = o->bucket_object(o->context, b, i);
            int has_ai;
            int16_t owner, cls;
            uint8_t state;
            if (id == o->own_id) continue;
            if (!o->object(o->context, id, &has_ai, &owner, &state, &cls) || !has_ai) continue;
            if (owner < 0 || state >= 3) continue;
            if (cls == 1 || cls == 3 || cls == 4 || cls == 0x15 || cls == 0x12 || cls == 0x14 ||
                cls == 0x1b)
                continue;
            if (owner != o->own_owner && !o->allied(o->context, owner, o->own_owner)) continue;
            if (o->distance(o->context, id) > 3.0f) continue;
            if (!o->ai_idle(o->context, id)) continue;
            o->order(o->context, owner, 700, target_id, o->own_owner, tx, ty,
                     o->own_range(o->context));
            ++sent;
        }
    }
    return sent;
}

/* 0x004119e0 */
int32_t aoe_ai_unit_ai_free_tile_ror(int32_t order, int filter, int32_t *out_x, int32_t *out_y,
                                     const AoeAiUnitAiFreeTileOpsRor *o) {
    if (order == 0x2c1 && o->has_point) return 0;
    int32_t cx = (int32_t)o->x, cy = (int32_t)o->y;
    int32_t x0 = (int32_t)(o->x - o->sight), x1 = (int32_t)(o->x + o->sight);
    int32_t y0 = (int32_t)(o->y - o->sight), y1 = (int32_t)(o->y + o->sight);
    if (x0 < 0) x0 = 0;
    if (x1 > o->map_w - 1) x1 = o->map_w - 1;
    if (y0 < 0) y0 = 0;
    if (y1 > o->map_h - 1) y1 = o->map_h - 1;
    int32_t slack = o->rand_(o->context) % 3, best = -1;
    if (y0 > y1) return 0;
    for (int32_t ty = y0; ty <= y1; ++ty)
        for (int32_t tx = x0; tx <= x1; ++tx) {
            if (o->blocked(o->context, tx, ty) || o->zone(o->context, tx, ty) != o->own_zone) continue;
            if (!o->accept(o->context, (float)tx, (float)ty)) continue;
            if (filter && o->filtered(o->context, tx, ty)) continue;
            int32_t d = (tx - cx) * (tx - cx) + (ty - cy) * (ty - cy);
            if (o->has_point) {
                int32_t px = (int32_t)o->px, py = (int32_t)o->py;
                d += (tx - px) * (tx - px) + (ty - py) * (ty - py);
            }
            if (best == -1 || d < slack + best) { *out_x = tx; *out_y = ty; best = d; }
        }
    if (best == -1) return 0;
    return o->move(o->context, *out_x, *out_y, o->sight - 1.0f);
}

/* 0x004128b0 */
int aoe_ai_unit_ai_move_aside_ror(const AoeAiUnitAiMoveAsideOpsRor *o, float x, float y, float z,
                                  int skip_distance, int32_t force) {
    int32_t tx, ty, tz, step;
    float best;
    if (!force && o->order != -1) return 0;
    if (x < 0.0f) x = 0.0f; else if (!(x < (float)o->map_w)) x = (float)o->map_w - 1.0f;
    if (y < 0.0f) y = 0.0f; else if (!(y < (float)o->map_h)) y = (float)o->map_h - 1.0f;
    tx = aoe_ftol32(x);
    ty = aoe_ftol32(y);
    tz = aoe_ftol32(o->unit_z);
    if (!o->can_place(o->context, tx, ty, tz)) {
        best = o->distance(o->context, x, y, o->unit_z);
        step = -1;
        for (;;) {
            ++step;
            if (step > 7) return 0;
            switch (step) {
            case 0: tx -= 2; ty -= 2; break;
            case 7: ty -= 2; break;
            case 1: case 2: tx += 2; break;
            case 3: case 4: ty += 2; break;
            default: tx -= 2; break;
            }
            if (tx < 0) tx = 0; else if (tx >= o->map_w) tx = o->map_w - 1;
            if (ty < 0) ty = 0; else if (ty >= o->map_h) ty = o->map_h - 1;
            if (!skip_distance &&
                o->distance(o->context, (float)tx + 0.5f, (float)ty + 0.5f, o->unit_z) < best) {
                ++step;
                continue;
            }
            if (o->can_place(o->context, tx, ty, tz) == 1) break;
        }
    }
    if (o->move(o->context, (float)tx + 0.5f, (float)ty + 0.5f, z, 0.5f, force) != 1) return 0;
    o->set_order(o->context, 0x262);
    o->set_target(o->context, -1, x, y, z);
    return 1;
}

/* 0x00414600 */
int32_t aoe_ai_unit_ai_check_ror(const AoeAiUnitAiCheckOpsRor *o) {
    int32_t code = o->action;
    float x, y, z, hp;
    uint8_t st;
    int16_t owner, tmpl, cls;
    int has_ai;
    (void)hp;
    if (code == -1) code = o->order == -1 ? -1 : o->order + 100;
    if (code == 700) {
        int tfound = o->target != -1 &&
                     o->resolve(o->context, o->target, &x, &y, &z, &st, &owner, &tmpl, &cls, &has_ai);
        int16_t tcls = tfound ? cls : 0;
        if (!o->debug_561458_is_1 && o->owner_kind != 0 && o->unit_7c(o->context) &&
            (!tfound || tmpl != 0x114)) {
            int32_t best = o->nearest(o->context);
            if (best != -1 && best != o->target &&
                o->resolve(o->context, best, &x, &y, &z, &st, &owner, &tmpl, &cls, &has_ai)) {
                if (cls == 3 || cls == 0x1b) {
                    if (tmpl != 0x114) return 9;
                } else if (tfound && tcls != 3 && tcls != 0x1b) {
                    return 9;
                }
                o->order_to(o->context, 700, best, owner, x, y, z,
                            o->attack_range(o->context, o->field_2c));
            }
        }
    } else if (code == 0x2bd) {
        int ffound = 0;
        if (*o->follow != -1) {
            ffound = o->resolve(o->context, *o->follow, &x, &y, &z, &st, &owner, &tmpl, &cls, &has_ai);
            if (!ffound || (st > 2 && owner != 0)) goto fail;
            if (*o->target_x == -1.0f) { *o->target_x = x; *o->target_y = y; }
        }
        if (o->order != 600 && !(o->order == 0x262 && o->unit_11c(o->context) == *o->follow)) {
            float d;
            if (*o->follow == -1) {
                d = o->distance_point(o->context, *o->target_x, *o->target_y, o->unit_z);
            } else {
                *o->target_x = x; *o->target_y = y; *o->target_z = z;
                d = o->distance_to(o->context, *o->follow);
            }
            if (o->range + 1.0f < d) {
                if (*o->follow == -1) {
                    if (o->can_place(o->context, aoe_ftol32(*o->target_x), aoe_ftol32(*o->target_y),
                                     o->range) == 1)
                        o->move_to(o->context, *o->target_x, *o->target_y, *o->target_z, o->range);
                } else if (o->can_reach(o->context, *o->follow, o->range) == 1) {
                    o->move_to_object(o->context, *o->follow, o->range, 1);
                }
                return 10;
            }
            if (ffound && has_ai) {
                int32_t t = o->member_target(o->context, *o->follow);
                if (t != -1) { o->set_5c(o->context, t); return 10; }
                if (o->query_d4(o->context, 0) == 6) return 10;
            }
        }
    } else if (code == 0x2c8) {
        float dx, dy;
        if (o->target == -1) {
            dx = dy = -10.0f;
        } else {
            if (!o->resolve(o->context, o->target, &x, &y, &z, &st, &owner, &tmpl, &cls, &has_ai) ||
                st > 2)
                goto fail;
            dy = y - *o->target_y;
            dx = x - *o->target_x;
        }
        /* The original compares against the doubles -1.0 (0x00542740) and 1.0 (0x005425f8). */
        if (dx < -1.0f || dx > 1.0f || dy < -1.0f || dy > 1.0f) {
            float d;
            if (o->target == -1) {
                d = o->distance_point(o->context, *o->target_x, *o->target_y, o->unit_z);
            } else {
                *o->target_x = x; *o->target_y = y; *o->target_z = z;
                d = o->distance_to(o->context, o->target);
            }
            if (o->range < d) {
                if (o->target != -1) o->move_to_object(o->context, o->target, o->range, 0);
                else o->move_to(o->context, *o->target_x, *o->target_y, *o->target_z, o->range);
                return 10;
            }
        }
    }
    return 9;
fail:
    *o->follow = -1;
    o->clear_target(o->context);
    o->stop(o->context, 1);
    return 9;
}

/* 0x004102f0 */
int32_t aoe_ai_play_trigger_ror(const AoeAiPlayOpsRor *o, uint8_t step, int32_t flag) {
    AoeAiPlayStateRor *m = o->members;
    const AoeAiPlayRuleRor *rule;
    float lx, ly, lz, hp;
    uint8_t st;
    int16_t maxhp, lowner;
    int has_ai, i, j;
    if (!m || !o->play_exists) return 0;
    if (step == 0xfe) { o->free_members(o->context); return 1; }
    rule = o->rule(o->context, step);
    if (!rule) return 0;
    if (!o->resolve(o->context, m->leader, &lx, &ly, &lz, &st, &hp, &maxhp, &lowner, &has_ai)) return 0;
    m->step = step;
    for (i = 0; i < 5; ++i) {
        const AoeAiPlayActionRor *a = &rule->actions[i];
        if (!a->code) continue;
        if ((a->code == 6 || a->code == 7) && flag == 0) {
            if (a->code == 6) o->sum_hp(o->context, a->category);
            else o->purge(o->context);
            continue;
        }
        for (j = 0; j < o->group_count; ++j) {
            int32_t id = o->group[j], k, target = -1, owner = -1, code;
            float x = -1.0f, y = -1.0f, z = -1.0f, mx, my, mz;
            int16_t mowner;
            uint8_t cat = 0xff;
            if (!o->resolve(o->context, id, &mx, &my, &mz, &st, &hp, &maxhp, &mowner, &has_ai) || !has_ai)
                continue;
            for (k = 0; k < AOE_AI_PLAY_MEMBERS; ++k)
                if (m->slots[k].id == id) { cat = m->slots[k].category; break; }
            if (cat != a->category) continue;
            if (flag == 1 && o->path_busy(o->context, id)) continue;
            switch (a->code) {
            case 1:
                x = (float)a->a + lx; y = (float)a->b + ly; z = (float)a->c + lz;
                if (o->gather_value(o->context, a->category, m->step) >=
                    aoe_ftol32(o->distance_point(o->context, x, y, z)))
                    continue;
                code = 0x2c6;
                break;
            case 2: code = 700; target = m->leader; owner = lowner; break;
            case 3: code = 700; target = m->target; break;
            case 4: code = 0x2c6; x = (float)m->point[0]; y = (float)m->point[1]; z = (float)m->point[2]; break;
            case 5: {
                int found = 0;
                for (k = 0; k < AOE_AI_PLAY_MEMBERS; ++k) {
                    float hx, hy, hz, hhp;
                    uint8_t hst;
                    int16_t hmax, howner;
                    int hai;
                    if (m->slots[k].category != (uint8_t)a->a) continue;
                    if (!o->resolve(o->context, m->slots[k].id, &hx, &hy, &hz, &hst, &hhp, &hmax,
                                    &howner, &hai))
                        continue;
                    if (aoe_ftol32(hhp) + 1 < hmax) { if (m->slots[k].id != -1) { found = 1; target = m->slots[k].id; } }
                    break;
                }
                if (!found) continue;
                code = 0x2bf;
                break;
            }
            case 8: code = 0x2d8; x = (float)a->a + lx; y = (float)a->b + ly; z = (float)a->c + lz; break;
            default: continue;
            }
            o->order(o->context, id, code, target, owner, x, y, z);
        }
    }
    return 1;
}

/* 0x0040fbf0 */
void aoe_ai_play_update_ror(const AoeAiPlayOpsRor *o) {
    AoeAiPlayStateRor *m = o->members;
    const AoeAiPlayRuleRor *rule;
    float lx, ly, lz, hp;
    uint8_t st;
    int16_t maxhp, owner;
    int has_ai, i, k;
    if (!m || !o->play_exists) return;
    if (!o->resolve(o->context, m->leader, &lx, &ly, &lz, &st, &hp, &maxhp, &owner, &has_ai)) {
        int32_t cand = o->pick_leader(o->context, -1);
        for (i = 0; i < o->group_count && cand == -1; ++i) {
            float x, y, z;
            if (o->group[i] == o->unit_id) continue;
            if (o->resolve(o->context, o->group[i], &x, &y, &z, &st, &hp, &maxhp, &owner, &has_ai) &&
                has_ai)
                cand = o->pick_leader(o->context, o->group[i]);
        }
        if (cand == -1) { o->free_members(o->context); return; }
        m->leader = cand;
        o->fire(o->context, m->step, 0);
        if (!o->resolve(o->context, cand, &lx, &ly, &lz, &st, &hp, &maxhp, &owner, &has_ai)) {
            lx = ly = lz = 0.0f;
        }
    }
    if (m->step == 0xfd) {
        int32_t ids[AOE_AI_PLAY_MEMBERS], n = 0, totals[5] = {0, 0, 0, 0, 0};
        int32_t classes = o->class_count(o->context);
        for (i = 0; i < o->group_count && n < AOE_AI_PLAY_MEMBERS; ++i) {
            float x, y, z;
            if (!o->resolve(o->context, o->group[i], &x, &y, &z, &st, &hp, &maxhp, &owner, &has_ai))
                continue;
            ids[n++] = o->group[i];
            for (k = 0; k < classes; ++k) o->copy_class(o->context, o->group[i], k);
        }
        m->start_time = o->world_time;
        o->fill(o->context, ids, n);
        for (i = 0; i < n; ++i) {
            float x, y, z;
            uint8_t cat = 0xff;
            for (k = 0; k < AOE_AI_PLAY_MEMBERS; ++k)
                if (m->slots[k].id == ids[i]) { cat = m->slots[k].category; break; }
            if (o->resolve(o->context, ids[i], &x, &y, &z, &st, &hp, &maxhp, &owner, &has_ai) && cat < 5)
                totals[cat] += aoe_ftol32(hp);
        }
        for (i = 0; i < 5; ++i) m->totals[i] = totals[i];
        o->fire(o->context, 0, 0);
        return;
    }
    rule = o->rule(o->context, m->step);
    if (!rule) return;
    for (i = 0; i < 3; ++i) {
        const AoeAiPlayTriggerRor *t = &rule->triggers[i];
        int fire = 0;
        if (!t->code) break;
        switch (t->code) {
        case 1: {
            uint8_t cat;
            fire = 1;
            for (cat = 0; cat < 5 && fire; ++cat) {
                const AoeAiPlayActionRor *a = NULL;
                for (k = 0; k < 5; ++k)
                    if (rule->actions[k].category == cat && rule->actions[k].code == 1) { a = &rule->actions[k]; break; }
                if (!a) continue;
                for (k = 0; k < AOE_AI_PLAY_MEMBERS && fire; ++k) {
                    float x, y, z;
                    if (m->slots[k].category != cat) continue;
                    if (!o->resolve(o->context, m->slots[k].id, &x, &y, &z, &st, &hp, &maxhp, &owner, &has_ai))
                        continue;
                    if ((float)t->a < o->distance_point(o->context, (float)a->a + lx, (float)a->b + ly,
                                                        (float)a->b + lz))
                        fire = 0;
                }
            }
            break;
        }
        case 2: {
            int32_t count = 0, dead = 0;
            for (k = 0; k < AOE_AI_PLAY_MEMBERS; ++k) {
                float x, y, z;
                if (m->slots[k].category != (uint8_t)t->a) continue;
                ++count;
                if (!o->resolve(o->context, m->slots[k].id, &x, &y, &z, &st, &hp, &maxhp, &owner, &has_ai) || st > 2)
                    ++dead;
            }
            fire = count != 0 && dead * 100 / count >= t->b;
            break;
        }
        case 3: {
            int32_t sum = 0, total = (uint8_t)t->a < 5 ? m->totals[(uint8_t)t->a] : -1;
            for (k = 0; k < AOE_AI_PLAY_MEMBERS; ++k) {
                float x, y, z;
                if (m->slots[k].category != (uint8_t)t->a) continue;
                if (o->resolve(o->context, m->slots[k].id, &x, &y, &z, &st, &hp, &maxhp, &owner, &has_ai))
                    sum += aoe_ftol32(hp);
            }
            fire = total != 0 && (total - sum) * 100 / total >= t->b;
            break;
        }
        case 8:
            fire = 1;
            for (k = 0; k < AOE_AI_PLAY_MEMBERS && fire; ++k) {
                float x, y, z;
                if (m->slots[k].category != (uint8_t)t->a) continue;
                if (o->resolve(o->context, m->slots[k].id, &x, &y, &z, &st, &hp, &maxhp, &owner, &has_ai) && st == 2)
                    fire = 0;
            }
            break;
        case 9:
            fire = 1;
            for (k = 0; k < AOE_AI_PLAY_MEMBERS && fire; ++k) {
                float x, y, z;
                if (m->slots[k].category != (uint8_t)t->a) continue;
                if (o->resolve(o->context, m->slots[k].id, &x, &y, &z, &st, &hp, &maxhp, &owner, &has_ai) &&
                    aoe_ftol32(hp) < maxhp)
                    fire = 0;
            }
            break;
        case 10: fire = 1; break;
        case 11: fire = t->a < (int32_t)(o->world_time - (uint32_t)m->start_time) / 1000; break;
        default: break;
        }
        if (fire && o->fire(o->context, t->next, 0) == 1) return;
    }
    o->fire(o->context, m->step, 1);
}

/* 0x00413890 */
static void msg_default(const AoeAiUnitAiMsgOpsRor *o, const int32_t *msg, int32_t force) {
    o->clear_target(o->context);
    o->stop(o->context, force);
    o->notify_base(o->context, msg);
}
static int msg_engage_ok(const AoeAiUnitAiMsgOpsRor *o, int32_t id) {
    return o->in_range(o->context, id) == 1 || o->can_reach(o->context, id) == 1;
}
int32_t aoe_ai_unit_ai_message_ror(const AoeAiUnitAiMsgOpsRor *o, const int32_t *msg, int32_t elapsed) {
    int32_t code = msg[2], a;
    AoeAiMsgObjectRor obj, tgt;
    switch (code) {
    case 500: case 0x20f: {
        int have_target;
        o->notify_base(o->context, msg);
        if (msg[4] <= 0) {
            o->tell_owner(o->context, 0x209, msg[0]);
            o->run_notes(o->context, elapsed);
            return 4;
        }
        if (o->group_count == 0 || o->owner_kind == 1) o->tell_owner(o->context, 0x201, msg[0]);
        if (!o->is_member(o->context, msg[0])) {
            if (o->owner_kind == 0 && *o->target != msg[0]) o->rally(o->context, msg[0]);
            o->add_member(o->context, msg[0]);
        }
        o->unit_ec(o->context, 1);
        if (*o->action != 0x2d5 && *o->action != -1 && o->owner_kind == 0) return 2;
        if (*o->action == 0x2d5) return 2;
        if (!o->resolve(o->context, msg[0], &obj)) return 2;
        if (obj.owner == o->own_owner) return 2;
        if (*o->order == 0x25b && o->self_a4(o->context, 1) == 1) return 3;
        have_target = *o->target != -1 && o->resolve(o->context, *o->target, &tgt);
        if (!have_target) {
            if (o->attackable(o->context, msg[0]) == 1 && msg_engage_ok(o, msg[0])) {
                if (o->engage(o->context, msg[0]) != 1) return 3;
                if (*o->action == 700) return 3;
                *o->action = -1;
                return 3;
            }
            if (*o->order == 0x262) return 3;
            o->self_a4(o->context, 1);
            return 3;
        }
        if ((tgt.unit_class == 3 || tgt.unit_class == 0x1b) && tgt.template_id != 0x4f &&
            tgt.template_id != 199 && tgt.template_id != 0x45 && o->owner_kind == 0) {
            if (o->attackable(o->context, msg[0]) == 1 && msg_engage_ok(o, msg[0])) {
                o->engage(o->context, msg[0]);
                return 3;
            }
            o->self_a4(o->context, 1);
            return 3;
        }
        if (o->owner_kind != 1) return 2;
        if (tgt.template_id == 0x114) return 2;
        {
            float hurt = o->has_hp(o->context, *o->target) ? o->damage_from_us(o->context, *o->target) : 0.0f;
            float d_t, d_o, s_t, s_o;
            if (o->attackable(o->context, msg[0]) == 1 && hurt == 0.0f && msg_engage_ok(o, msg[0]))
                o->engage(o->context, msg[0]);
            d_t = o->damage_to_us(o->context, *o->target);
            s_t = d_t <= 0.0f ? 25000.0f : tgt.hp / d_t;
            d_o = o->damage_to_us(o->context, msg[0]);
            s_o = d_o <= 0.0f ? 25000.0f : obj.hp / d_o;
            if (o->attackable(o->context, msg[0]) != 1) return 2;
            if (s_o <= s_t) return 2;
            if (!msg_engage_ok(o, msg[0])) return 2;
            o->engage(o->context, msg[0]);
            return 2;
        }
    }
    case 0x1f9: case 0x1fa: case 0x1fb:
        a = *o->action;
        if (a == 700 && msg[3] == 600) {
            if (o->query_d4(o->context, 0) != 5) return 2;
            if (*o->action != 0x2d5 && o->owner_kind != 0 &&
                o->move_to(o->context, *o->target_x, *o->target_y, *o->target_z, *o->range + 2.0f, 1)) {
                *o->last_action = *o->action;
                *o->action = -1;
                *o->field_2c = -1;
                return 2;
            }
            o->clear_target(o->context);
            o->stop(o->context, code == 0x1fa ? 0 : 1);
            return 2;
        }
        if ((a == -1 && *o->order == msg[3]) || (a != -1 && *o->order + 100 == a)) {
            if (code == 0x1fa) { o->clear_target(o->context); o->stop(o->context, 1); return 2; }
            msg_default(o, msg, 1);
            return 2;
        }
        if (a == 0x2d4 && msg[3] == 0x262) {
            if (code == 0x1fa) { o->clear_target(o->context); o->stop(o->context, 0); return 2; }
            msg_default(o, msg, 0);
            return 2;
        }
        if (a == 0x2bd && msg[3] == 0x262) { o->set_order(o->context, -1); return 2; }
        if (a == 0x2d5 && msg[3] == 600) { o->clear_target(o->context); o->stop(o->context, 1); return 2; }
        return 2;
    case 0x1fc:
        if (*o->target != -1 && o->resolve(o->context, *o->target, &tgt)) {
            uint8_t fog = o->fog_state(o->context, aoe_ftol32(*o->target_x), aoe_ftol32(*o->target_y));
            float x = *o->target_x, y = *o->target_y, z = *o->target_z;
            int32_t best;
            if (tgt.owner == 0) return 2;
            if (fog == 0xf) return 2;
            best = o->nearest(o->context);
            if (best != -1 && best != *o->target && o->engage(o->context, best) == 1) { *o->action = -1; return 3; }
            if (o->owner_kind == 0 && o->move_to(o->context, x, y, z, 0.0f, 1) == 1) {
                *o->last_action = *o->action;
                *o->action = -1;
                *o->field_2c = -1;
                return 3;
            }
        }
        msg_default(o, msg, 1);
        return 2;
    case 0x1fe:
        o->unit_18c(o->context);
        o->clear_target(o->context);
        o->stop(o->context, 1);
        return 2;
    case 0x202: {
        int32_t od = *o->order;
        float x, y;
        if (od != 0x25d && od != 0x26f) {
            int32_t id, k, r;
            if (od != 0x261) return 2;
            id = *o->target_type == -1 ? *o->last_target_type : *o->target_type;
            k = o->field_c0(o->context, id);
            r = k == -1 ? o->field_40(o->context, id) : o->field_44(o->context, k);
            if (r != -1 && o->field_68(o->context, r) == 1) return 3;
            msg_default(o, msg, 1);
            return 2;
        }
        if (o->find_point(o->context, &x, &y) == 1) {
            o->mark_point(o->context, x, y);
            if (!o->step_80(o->context, x, y)) msg_default(o, msg, 1);
            *o->order = od;
            return 3;
        }
        msg_default(o, msg, 1);
        return 2;
    }
    case 0x20b:
        o->unit_18c(o->context);
        o->clear_target(o->context);
        o->stop(o->context, 1);
        o->run_notes(o->context, elapsed);
        *o->field_10 = 0;
        if (o->leader_id != -1 && o->resolve(o->context, o->leader_id, &obj)) o->leader_notify(o->context);
        return 2;
    case 600:
        if (!o->is_member(o->context, msg[0])) o->add_member(o->context, msg[0]);
        o->unit_ec(o->context, 1);
        return 2;
    case 699:
        o->remove_member(o->context, msg[0]);
        return 2;
    case 0x1f6: case 0x1f7: case 0x1f8: case 0x1fd: case 0x1ff: case 0x200: case 0x203:
    case 0x259: case 0x25a: case 0x25b: case 0x25c: case 0x25d: case 0x25e: case 0x25f:
    case 0x260: case 0x261: case 0x262: case 0x265: case 0x266: case 0x267: case 0x268: case 0x269:
        return 2;
    default:
        msg_default(o, msg, 1);
        return 2;
    }
}
