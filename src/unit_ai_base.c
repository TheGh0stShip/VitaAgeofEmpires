#include "unit_ai_base.h"

#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <stddef.h>

void aoe_unit_ai_set_action_ror(AoeUnitAiRor *ai, int32_t action) {
    ai->last_action = ai->action;
    ai->action = action;
}

void aoe_unit_ai_set_order_ror(AoeUnitAiRor *ai, int32_t order) {
    ai->last_order = ai->order;
    ai->order = order;
}

void aoe_unit_ai_clear_target_ror(AoeUnitAiRor *ai) {
    int32_t type = ai->target_type;
    ai->last_target_id = ai->target_id;
    ai->target_id = -1;
    ai->target_type = -1;
    ai->last_target_type = type;
    ai->target_x = ai->target_y = ai->target_z = -1.0f;
}

void aoe_unit_ai_set_target_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t id,
                                float x, float y, float z) {
    AoeUnitAiObjectRor o;
    ai->last_target_id = ai->target_id;
    ai->last_target_type = ai->target_type;
    ai->target_id = id;
    ai->target_x = x;
    ai->target_y = y;
    ai->target_z = z;
    if (ops->object(ops->context, id, &o)) ai->target_type = o.type;
}

void aoe_unit_ai_set_target_full_ror(AoeUnitAiRor *ai, int32_t id, int32_t type, float x,
                                     float y, float z) {
    ai->last_target_id = ai->target_id;
    ai->last_target_type = ai->target_type;
    ai->target_id = id;
    ai->target_type = type;
    ai->target_x = x;
    ai->target_y = y;
    ai->target_z = z;
}

int aoe_unit_ai_add_waypoint_ror(AoeUnitAiRor *ai, int32_t x, int32_t y) {
    if (ai->waypoint_next > 7) ai->waypoint_next = 0;
    ai->waypoint_x[ai->waypoint_next] = (float)x;
    ai->waypoint_y[ai->waypoint_next] = (float)y;
    ++ai->waypoint_next;
    return 1;
}

int aoe_unit_ai_has_waypoint_ror(const AoeUnitAiRor *ai, int32_t x, int32_t y) {
    int32_t i;
    for (i = 0; i < ai->waypoint_next; ++i)
        if (ai->waypoint_x[i] == (float)x && (float)y == ai->waypoint_y[i]) return 1;
    return 0;
}

int aoe_unit_ai_stop_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int force) {
    ai->last_order = ai->order;
    if (!force || ai->order + 100 == ai->action) {
        ai->last_action = ai->action;
        ai->action = -1;
        ai->field_2c = -1;
    }
    ops->unit_stop(ops->context);
    ai->order = -1;
    return 1;
}

/* Record the object as the target: its type and position, or -1 / 0. */
static void target_object(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t id) {
    AoeUnitAiObjectRor o;
    if (!ops->object(ops->context, id, &o))
        aoe_unit_ai_set_target_full_ror(ai, id, -1, 0, 0, 0);
    else
        aoe_unit_ai_set_target_full_ror(ai, id, o.type, o.x, o.y, o.z);
}

/* The orders that run unless the unit already has them. */
static int32_t slot_order(int slot) {
    switch (slot) {
    case 0x164: return 0x265;
    case 0x16c: return 0x25c;
    case 0x88: return 0x25b;
    case 0x168: return 0x261;
    case 0x170: return 0x26a;
    case 0x174: return 0x25a;
    case 0x178: return 0x267;
    default: return -1;
    }
}

int aoe_unit_ai_order_target_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int slot,
                                 int32_t id, int force) {
    int32_t order = slot_order(slot);
    if (!force && ai->order == order) return 0;
    if (slot == 0x164) ops->self_stop(ops->context, 1);
    if (ops->unit_target(ops->context, slot, id, 0, force) != 1) return 0;
    aoe_unit_ai_set_order_ror(ai, order);
    target_object(ai, ops, id);
    return 1;
}

int aoe_unit_ai_attack_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t id,
                           int force) {
    AoeUnitAiObjectRor o;
    if (!force && ai->order != -1) return 0;
    if (!ops->object(ops->context, id, &o) || !ops->can_act_on(ops->context, &o, id)) return 0;
    if (ops->unit_target(ops->context, 0x150, id, 0, force) != 1) return 0;
    aoe_unit_ai_set_order_ror(ai, AOE_UNIT_AI_ORDER_ATTACK);
    aoe_unit_ai_set_target_ror(ai, ops, id, o.x, o.y, o.z);
    return 1;
}

int aoe_unit_ai_attack_new_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t id) {
    AoeUnitAiObjectRor o;
    if (ai->target_id == id) return 0;
    if (!ops->object(ops->context, id, &o) || !ops->can_act_on(ops->context, &o, id)) return 0;
    if (ai->order != -1 && !ops->unit_in_range(ops->context, id, ops->unit_range(ops->context)))
        return 0;
    if (ops->unit_target(ops->context, 0x150, id, 0, 1) != 1) return 0;
    aoe_unit_ai_set_order_ror(ai, AOE_UNIT_AI_ORDER_ATTACK);
    aoe_unit_ai_set_target_ror(ai, ops, id, o.x, o.y, o.z);
    return 1;
}

int aoe_unit_ai_order_xy_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t order,
                             int32_t x, int32_t y, int force) {
    if (!force && ai->order != -1) return 0;
    if (ops->unit_point(ops->context, 0x17c, (float)x, (float)y, 0, 0, force) != 1) return 0;
    aoe_unit_ai_set_order_ror(ai, order);
    aoe_unit_ai_set_target_ror(ai, ops, -1, (float)x, (float)y, 1.0f);
    return 1;
}

int aoe_unit_ai_order_id_xy_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t id,
                                int32_t x, int32_t y, int force) {
    if (!force && ai->order != -1) return 0;
    if (ops->unit_target(ops->context, 0x150, id, 0, force) != 1) return 0;
    aoe_unit_ai_set_order_ror(ai, AOE_UNIT_AI_ORDER_26E);
    aoe_unit_ai_set_target_ror(ai, ops, id, (float)x, (float)y, 1.0f);
    return 1;
}

int aoe_unit_ai_order_269_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t id,
                              int force) {
    if (!force && ai->order != -1) return 0;
    if (ops->unit_target(ops->context, 0x180, id, 0, force) != 1) return 0;
    aoe_unit_ai_set_order_ror(ai, AOE_UNIT_AI_ORDER_269);
    aoe_unit_ai_set_target_ror(ai, ops, id, -1.0f, -1.0f, -1.0f);
    return 1;
}

int aoe_unit_ai_order_266_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, float x, float y,
                              float z, int force) {
    if (!force && ai->order != -1) return 0;
    if (ops->unit_point(ops->context, 0x188, x, y, z, 0, force) != 1) return 0;
    aoe_unit_ai_set_order_ror(ai, AOE_UNIT_AI_ORDER_266);
    aoe_unit_ai_set_target_ror(ai, ops, -1, x, y, z);
    return 1;
}

int aoe_unit_ai_order_26d_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t a,
                              int32_t b, int32_t c) {
    (void)b, (void)c;
    if (a == 0 && ai->order != -1) return 0;
    if (ops->unit_target(ops->context, 0x184, a, b, c) != 1) return 0;
    aoe_unit_ai_set_order_ror(ai, AOE_UNIT_AI_ORDER_26D);
    aoe_unit_ai_set_target_full_ror(ai, ops->unit_id, ops->unit_type, ops->unit_x, ops->unit_y,
                                    ops->unit_z);
    return 1;
}

int aoe_unit_ai_move_to_object_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int slot,
                                   int32_t id, int32_t extra, int force) {
    AoeUnitAiObjectRor o;
    if (!force && ai->order != -1) return 0;
    if (ops->unit_target(ops->context, slot, id, extra, force) != 1) return 0;
    aoe_unit_ai_set_order_ror(ai, AOE_UNIT_AI_ORDER_MOVE);
    if (!ops->object(ops->context, id, &o))
        aoe_unit_ai_set_target_ror(ai, ops, id, 0, 0, 0);
    else
        aoe_unit_ai_set_target_ror(ai, ops, id, o.x, o.y, o.z);
    return 1;
}

int aoe_unit_ai_move_to_point_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, float x,
                                  float y, float z, int32_t extra, int force) {
    if (!force && ai->order != -1) return 0;
    if (ops->unit_point(ops->context, 0x154, x, y, z, extra, force) != 1) return 0;
    aoe_unit_ai_set_order_ror(ai, AOE_UNIT_AI_ORDER_MOVE);
    aoe_unit_ai_set_target_ror(ai, ops, -1, x, y, z);
    return 1;
}

static float clamp_axis(float v, int32_t size) {
    if (v < 0.0f) return 0.0f;
    if ((float)size <= v) return (float)size - 1.0f;
    return v;
}

static float jitter(const AoeUnitAiOpsRor *ops, float v) {
    if (ops->rand(ops->context) % 2 == 0)
        return v - (float)(ops->rand(ops->context) % 4);
    return (float)(ops->rand(ops->context) % 4) + v;
}

int aoe_unit_ai_move_near_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, float x, float y,
                              float z, int force) {
    int i;
    if (!force && ai->order != -1) return 0;
    x = clamp_axis(x, ops->map_w);
    y = clamp_axis(y, ops->map_h);
    for (i = 0; i < 5; ++i) {
        if (ops->unit_free_tile(ops->context, x, y) == 1) break;
        x = jitter(ops, x);
        y = jitter(ops, y);
        x = clamp_axis(x, ops->map_w);
        y = clamp_axis(y, ops->map_h);
    }
    if (ops->unit_point(ops->context, 0x154, x, y, z, 0, force) != 1) return 0;
    aoe_unit_ai_set_order_ror(ai, AOE_UNIT_AI_ORDER_MOVE);
    aoe_unit_ai_set_target_ror(ai, ops, -1, x, y, z);
    return 1;
}

int aoe_unit_ai_follow_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t order,
                           int32_t id, int32_t extra, int force) {
    AoeUnitAiObjectRor o;
    if (!force && ai->order != -1) return 0;
    if (!ops->object(ops->context, id, &o)) return 0;
    aoe_unit_ai_set_order_ror(ai, order);
    aoe_unit_ai_set_target_ror(ai, ops, id, o.x, o.y, o.z);
    if (order == AOE_UNIT_AI_ORDER_259) ai->field_4c = id;
    ai->field_48 = extra;
    return 1;
}

int aoe_unit_ai_follow_point_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, float x,
                                 float y, int32_t extra, int force) {
    if (!force && ai->order != -1) return 0;
    aoe_unit_ai_set_order_ror(ai, AOE_UNIT_AI_ORDER_259);
    aoe_unit_ai_set_target_ror(ai, ops, -1, x, y, ops->unit_z);
    ai->field_4c = -1;
    ai->field_48 = extra;
    return 1;
}

int aoe_unit_ai_move_away_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int force) {
    float dx = 0.0f, dy = 0.0f, speed, len;
    int32_t i;
    if (!force && ai->order != -1) return 0;
    for (i = 0; i < ai->group_count; ++i) {
        AoeUnitAiObjectRor o;
        if (ops->object(ops->context, ai->group[i], &o)) {
            dx = (o.x - ops->unit_x) + dx;
            dy = (o.y - ops->unit_y) + dy;
        }
    }
    speed = ops->unit_speed(ops->context);
    len = (float)sqrt((double)dx * dx + (double)dy * dy);
    return aoe_unit_ai_move_near_ror(ai, ops, -((dx / len) * speed * 4.0f) + ops->unit_x,
                                     -((dy / len) * speed * 4.0f) + ops->unit_y, ops->unit_z, 1);
}

int aoe_unit_ai_class_allowed_ror(const AoeUnitAiRor *ai, int32_t cls) {
    int32_t i;
    for (i = 0; i < ai->class_count; ++i)
        if (ai->classes[i] == cls) return 1;
    return 0;
}

int aoe_unit_ai_table_has_ror(const AoeUnitAiRor *ai, int32_t id) {
    int32_t i;
    for (i = 0; i < ai->table_count; ++i)
        if (ai->table_ids[i] == id) return 1;
    return 0;
}

uint8_t aoe_unit_ai_tile_view_ror(const AoeUnitAiOpsRor *ops, int32_t x, int32_t y) {
    uint32_t bits = ops->visibility(ops->context, x, y);
    if (ops->visible_mask & bits) return 0xf;
    return (ops->explored_mask & bits) ? 0x80 : 0;
}

void aoe_unit_ai_reset_timer_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops) {
    uint32_t base = ai->timer_base;
    ai->timer = (uint32_t)ops->rand(ops->context) % (base >> 1) + (base * 3 >> 2);
}

int aoe_unit_ai_object_ror(const AoeUnitAiOpsRor *ops, int32_t id,
                           AoeUnitAiObjectRor *out) {
    if (!ops || !ops->object) return 0;
    return ops->object(ops->context, id, out);
}

int aoe_unit_ai_resize_ids_ror(int32_t **data, int32_t *capacity,
                               int32_t size) {
    int32_t *fresh;
    if (!data || !capacity || size <= 0) return 0;
    fresh = malloc((size_t)size * sizeof *fresh);
    if (!fresh) return 0;
    for (int32_t i = 0; i < *capacity && i < size; i++) fresh[i] = (*data)[i];
    free(*data);
    *data = fresh;
    *capacity = size;
    return 1;
}

void aoe_unit_ai_release_ror(AoeUnitAiRor *ai) {
    free(ai->group);
    ai->group = NULL;
    ai->group_count = 0;
    free(ai->classes);
    ai->classes = NULL;
    ai->class_count = 0;
}

int aoe_unit_ai_notify_filter_ror(const AoeUnitAiRor *ai, uint8_t unit_state, int32_t sender,
                                  int32_t code, int32_t value) {
    if (unit_state != 2) return 0;
    if (code == 699) {
        for (int32_t i = 0; i < ai->group_count; ++i)
            if (ai->group[i] == sender) return 2;
        return 0;
    }
    if (code == 0x1fb) return value == ai->order ? 2 : 0;
    if (code == 0x1fa && value == 0x262 && ai->order != 0x262 && ai->order != 0x268) return 1;
    return 2;
}

int aoe_unit_ai_note_ror(AoeUnitAiNotesRor *q, int32_t sender, int32_t code, int32_t extra,
                         const int32_t values[6], int front) {
    int32_t *r;
    if (q->count == q->capacity) {
        int32_t (*grown)[9];
        if (!q->capacity) return 0;
        grown = malloc((size_t)q->capacity * 2 * sizeof *grown);
        if (!grown) return 0;
        memcpy(grown, q->records, (size_t)q->count * sizeof *grown);
        free(q->records);
        q->records = grown;
        q->capacity *= 2;
    }
    if (front && q->count) {
        for (int32_t i = 0; i < q->count; ++i) memcpy(q->records[i + 1], q->records[i], sizeof q->records[0]);
        r = q->records[0];
    } else {
        r = q->records[q->count];
    }
    r[0] = sender;
    r[1] = code;
    r[2] = extra;
    for (int i = 0; i < 6; ++i) r[3 + i] = values[i];
    q->count++;
    return 1;
}

int aoe_unit_ai_send_ror(AoeUnitAiOutRor *q, const int32_t values[6]) {
    if (q->count == q->capacity) {
        int32_t (*grown)[6];
        if (!q->capacity) return 0;
        grown = malloc((size_t)q->capacity * 2 * sizeof *grown);
        if (!grown) return 0;
        memcpy(grown, q->records, (size_t)q->count * sizeof *grown);
        free(q->records);
        q->records = grown;
        q->capacity *= 2;
    }
    memcpy(q->records[q->count], values, sizeof q->records[0]);
    q->count++;
    return 1;
}

int16_t aoe_unit_ai_owner_ror(const AoeUnitAiOpsRor *ops) {
    return ops ? ops->unit_owner : -1;
}

int aoe_unit_ai_post_ror(const AoeUnitAiOpsRor *ops, const int32_t values[6]) {
    if (!ops || !values) return 0;
    if (ops->unit_group_count && ops->unit_group_count(ops->context) > 0 && ops->ai_d0) {
        int32_t leader = ops->unit_leader ? ops->unit_leader(ops->context) : -1;
        if (ops->unit_id == leader) {
            ops->ai_d0(ops->context, -1, values);
        } else {
            AoeUnitAiObjectRor o;
            if (ops->object && ops->object(ops->context, leader, &o))
                ops->ai_d0(ops->context, leader, values);
        }
    }
    if (ops->owner_e8) ops->owner_e8(ops->context, values);
    return 1;
}

int aoe_unit_ai_post_record_ror(const AoeUnitAiOpsRor *ops, const int32_t record[6]) {
    return aoe_unit_ai_post_ror(ops, record);
}

int aoe_unit_ai_order_ror(AoeUnitAiRor *ai, AoeUnitAiNotesRor *q, uint8_t unit_state,
                          int32_t unit_id, int32_t sender, int32_t code, const int32_t values[6],
                          int32_t now, int32_t requeue, int32_t extra,
                          const AoeUnitAiOrderOpsRor *ops) {
    if (!ai || !q || !values || unit_state >= 3) return 1;
    if (!now) {
        aoe_unit_ai_note_ror(q, sender, code, extra, values, requeue);
        return 1;
    }
    if (requeue == 1) {
        int32_t cur[6];
        cur[0] = ai->target_id;
        cur[1] = -1;
        memcpy(&cur[2], &ai->target_x, 4);
        memcpy(&cur[3], &ai->target_y, 4);
        memcpy(&cur[4], &ai->target_z, 4);
        cur[5] = ai->field_48;
        aoe_unit_ai_note_ror(q, unit_id, ai->action, ai->field_2c, cur, 1);
    }
    aoe_unit_ai_note_ror(q, sender, code, extra, values, 1);
    if (code != 0x2d9) {
        aoe_unit_ai_clear_target_ror(ai);
        if (ops && ops->self_stop) ops->self_stop(ops->context, 1);
    }
    if (ops && ops->run_notes) ops->run_notes(ops->context, 0);
    if (ops && ops->handle_c8 && q->count) ops->handle_c8(ops->context, q->records[0], 0);
    return 1;
}
