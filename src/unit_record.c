/* Patched RoR 1.0a saved-game unit records: the per-class load
   constructors and vtable +0x30 saves (see unit_record.h). */
#include "unit_record.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PATH_CAPACITY 10
#define TWO_PI 6.2831854820251465f /* 0x00544a30 */

static int16_t get_i16(AoeOutcomeStream *s) {
    const uint8_t *p = aoe_stream_take(s, 2);
    return p ? (int16_t)aoe_le_u16(p) : 0;
}

static void put_i16(AoeOutcomeStream *s, int16_t v) {
    uint8_t b[2];
    aoe_le_put_u16(b, (uint16_t)v);
    aoe_stream_put(s, b, 2);
}

/* The 0x004a6bb0 / 0x004a95e0 ID vectors: appended when absent, growing
   by one (the original keeps count, capacity and storage apart; the port
   grows the storage to the count). */
static int add_id(int32_t **ids, int32_t *count, int32_t id) {
    int32_t i, *grown;
    for (i = 0; i < *count; ++i)
        if ((*ids)[i] == id) return 1;
    grown = realloc(*ids, (size_t)(*count + 1) * sizeof(int32_t));
    if (!grown) return 0;
    *ids = grown;
    grown[(*count)++] = id;
    return 1;
}

static int load_ids(int32_t **ids, int32_t *count, AoeOutcomeStream *in) {
    int32_t n = aoe_stream_i32(in), i;
    for (i = 0; i < n && !in->failed; ++i)
        if (!add_id(ids, count, aoe_stream_i32(in))) return 0;
    return !in->failed;
}

static void save_ids(const int32_t *ids, int32_t count, AoeOutcomeStream *out) {
    int32_t i;
    aoe_stream_put_i32(out, count);
    for (i = 0; i < count; ++i) aoe_stream_put_i32(out, ids[i]);
}

/* ---- root 0x004a6bb0 / 0x004a95e0 ---- */

static int load_root(AoeUnitRecordRor *r, float version,
                     const AoeUnitRecordOpsRor *ops, AoeOutcomeStream *in) {
    int32_t i;
    r->class_code = 10;
    r->owner = aoe_stream_u8(in);
    r->template_id = get_i16(in);
    r->sprite_id = get_i16(in);
    r->attached_id = aoe_stream_i32(in);
    r->hit_points = aoe_stream_f32(in);
    r->state = aoe_stream_u8(in);
    r->positive_hit_points = aoe_stream_u8(in);
    if (version >= 7.09f) r->type_25 = aoe_stream_u8(in); /* 0x00544830 */
    r->registration_pending = aoe_stream_u8(in);
    r->unit_id = aoe_stream_i32(in);
    r->frame = aoe_stream_u8(in);
    r->x = aoe_stream_f32(in);
    r->y = aoe_stream_f32(in);
    r->z = aoe_stream_f32(in);
    r->draw_left = get_i16(in);
    r->draw_top = get_i16(in);
    r->draw_right = get_i16(in);
    r->draw_bottom = get_i16(in);
    r->field_37 = aoe_stream_u8(in);
    r->carried_type = get_i16(in);
    r->carried_amount = aoe_stream_f32(in);
    r->target_refs = aoe_stream_u8(in);
    r->damage_percent = aoe_stream_u8(in);
    r->byte_86 = aoe_stream_u8(in);
    r->dependent_source_id = aoe_stream_i32(in);
    r->dependent_context_bits = (uint32_t)aoe_stream_i32(in);
    if (version == 6.991f) /* 0x00547f90: six unused dwords */
        for (i = 0; i < 6; ++i) aoe_stream_i32(in);
    if (!load_ids(&r->dependents, &r->dependent_count, in)) return 0;
    if (!load_ids(&r->group, &r->group_count, in)) return 0;
    if (in->failed) return 0;
    return ops && ops->load_sprites ? ops->load_sprites(ops->context, r, in) : 1;
}

static int save_root(const AoeUnitRecordRor *r, const AoeUnitRecordOpsRor *ops,
                     AoeOutcomeStream *out) {
    aoe_stream_put_u8(out, r->class_code);
    aoe_stream_put_u8(out, r->owner);
    put_i16(out, r->template_id);
    put_i16(out, r->sprite_id);
    aoe_stream_put_i32(out, r->attached_id);
    aoe_stream_put_f32(out, r->hit_points);
    aoe_stream_put_u8(out, r->state);
    aoe_stream_put_u8(out, r->positive_hit_points);
    aoe_stream_put_u8(out, r->type_25);
    aoe_stream_put_u8(out, r->registration_pending);
    aoe_stream_put_i32(out, r->unit_id);
    aoe_stream_put_u8(out, r->frame);
    aoe_stream_put_f32(out, r->x);
    aoe_stream_put_f32(out, r->y);
    aoe_stream_put_f32(out, r->z);
    put_i16(out, r->draw_left);
    put_i16(out, r->draw_top);
    put_i16(out, r->draw_right);
    put_i16(out, r->draw_bottom);
    aoe_stream_put_u8(out, r->field_37);
    put_i16(out, r->carried_type);
    aoe_stream_put_f32(out, r->carried_amount);
    aoe_stream_put_u8(out, r->target_refs);
    aoe_stream_put_u8(out, r->damage_percent);
    aoe_stream_put_u8(out, r->byte_86);
    aoe_stream_put_i32(out, r->dependent_source_id);
    aoe_stream_put_i32(out, (int32_t)r->dependent_context_bits);
    save_ids(r->dependents, r->dependent_count, out);
    save_ids(r->group, r->group_count, out);
    if (out->failed) return 0;
    return ops && ops->save_sprites ? ops->save_sprites(ops->context, r, out) : 1;
}

/* ---- type 20: 0x004151c0 / 0x00415280 ---- */

static int load_20(AoeUnitRecordRor *r, float version,
                   const AoeUnitRecordOpsRor *ops, AoeOutcomeStream *in) {
    if (!load_root(r, version, ops, in)) return 0;
    r->class_code = 0x14;
    r->animation_rate = aoe_stream_f32(in);
    return !in->failed;
}

static int save_20(const AoeUnitRecordRor *r, const AoeUnitRecordOpsRor *ops,
                   AoeOutcomeStream *out) {
    if (!save_root(r, ops, out)) return 0;
    aoe_stream_put_f32(out, r->animation_rate);
    return !out->failed;
}

/* ---- type 30: 0x0044b360 / 0x0044b690 ---- */

static void load_point(AoeOutcomeStream *in, float *x, float *y, float *z,
                       uint8_t *direction) {
    *x = aoe_stream_f32(in);
    *y = aoe_stream_f32(in);
    *z = aoe_stream_f32(in);
    *direction = aoe_stream_u8(in);
    aoe_stream_take(in, 3);
}

static void save_point(AoeOutcomeStream *out, float x, float y, float z,
                       uint8_t direction) {
    static const uint8_t pad[3] = {0, 0, 0};
    aoe_stream_put_f32(out, x);
    aoe_stream_put_f32(out, y);
    aoe_stream_put_f32(out, z);
    aoe_stream_put_u8(out, direction);
    aoe_stream_put(out, pad, 3);
}

static int load_30(AoeUnitRecordRor *r, float version,
                   const AoeUnitRecordOpsRor *ops, AoeOutcomeStream *in) {
    AoeMovementRor *m = &r->movement;
    int32_t n, i;
    if (!load_20(r, version, ops, in)) return 0;
    r->class_code = 0x1e;
    if (!m->path.points) aoe_unit_path_init_ror(&m->path, PATH_CAPACITY);
    if (!m->alternate_path.points)
        aoe_unit_path_init_ror(&m->alternate_path, PATH_CAPACITY);
    r->field_8c = (uint32_t)aoe_stream_i32(in);
    m->velocity_x = aoe_stream_f32(in);
    m->velocity_y = aoe_stream_f32(in);
    m->velocity_z = aoe_stream_f32(in);
    m->facing = aoe_stream_f32(in);
    m->field_a0 = aoe_stream_i32(in);
    n = aoe_stream_i32(in);
    for (i = 0; i < n && !in->failed; ++i) {
        float x, y, z;
        uint8_t d;
        load_point(in, &x, &y, &z, &d);
        if (!aoe_unit_path_append_ror(&m->path, x, y, z, d)) return 0;
    }
    n = aoe_stream_i32(in); /* the cursor: rewound, then advanced n times */
    aoe_unit_path_rewind_ror(&m->path);
    for (i = 0; i < n; ++i) aoe_unit_path_advance_ror(&m->path);
    load_point(in, &m->start_x, &m->start_y, &m->start_z, &m->start_direction);
    load_point(in, &m->dest_x, &m->dest_y, &m->dest_z, &m->dest_direction);
    m->turn_timer = aoe_stream_f32(in);
    m->blocked_wait = aoe_stream_f32(in);
    m->range = aoe_stream_f32(in);
    m->target_id = aoe_stream_i32(in);
    m->target_size_x = aoe_stream_f32(in);
    m->target_size_y = aoe_stream_f32(in);
    m->retry_baseline = aoe_stream_i32(in);
    m->status = aoe_stream_i32(in);
    m->hold_155 = aoe_stream_u8(in);
    m->retry = aoe_stream_u8(in);
    m->waypoint_count = aoe_stream_i32(in);
    m->waypoint_capacity = aoe_stream_i32(in);
    if (in->failed || m->waypoint_count < 0 ||
        m->waypoint_count > m->waypoint_capacity)
        return 0;
    free(m->waypoint_tile);
    m->waypoint_tile = NULL;
    if (m->waypoint_capacity > 0) {
        m->waypoint_tile = calloc((size_t)m->waypoint_capacity, 3);
        if (!m->waypoint_tile) return 0;
        for (i = 0; i < m->waypoint_count; ++i) {
            const uint8_t *p = aoe_stream_take(in, 3);
            if (!p) return 0;
            memcpy(m->waypoint_tile + i * 3, p, 3);
        }
    }
    m->waypoints_stale = aoe_stream_u8(in);
    if (version < 1.3f) { /* 0x00544a48 */
        for (i = 0; i < 4; ++i) r->field_158[i] = -1;
    } else {
        for (i = 0; i < 4; ++i) r->field_158[i] = aoe_stream_i32(in);
    }
    if (version < 7.22f) { /* 0x00544a44 */
        m->best = 100000.0f;
        m->facet_history[0] = 0xff;
    } else {
        m->best = aoe_stream_f32(in);
        m->facet_history[0] = aoe_stream_u8(in);
    }
    if (version < 7.23f) /* 0x00544a40 */
        m->facet_history[1] = 0xff;
    else
        m->facet_history[1] = aoe_stream_u8(in);
    return !in->failed;
}

static int save_30(const AoeUnitRecordRor *r, const AoeUnitRecordOpsRor *ops,
                   AoeOutcomeStream *out) {
    const AoeMovementRor *m = &r->movement;
    int32_t n, i;
    if (!save_20(r, ops, out)) return 0;
    aoe_stream_put_i32(out, (int32_t)r->field_8c);
    aoe_stream_put_f32(out, m->velocity_x);
    aoe_stream_put_f32(out, m->velocity_y);
    aoe_stream_put_f32(out, m->velocity_z);
    aoe_stream_put_f32(out, m->facing);
    aoe_stream_put_i32(out, m->field_a0);
    n = aoe_unit_path_count_ror(&m->path);
    aoe_stream_put_i32(out, n);
    for (i = 0; i < n; ++i) {
        const AoePathWaypointRor *w = aoe_unit_path_at_ror(&m->path, i);
        if (w) save_point(out, w->x, w->y, w->z, w->direction);
    }
    aoe_stream_put_i32(out, aoe_unit_path_cursor_ror(&m->path));
    save_point(out, m->start_x, m->start_y, m->start_z, m->start_direction);
    save_point(out, m->dest_x, m->dest_y, m->dest_z, m->dest_direction);
    aoe_stream_put_f32(out, m->turn_timer);
    aoe_stream_put_f32(out, m->blocked_wait);
    aoe_stream_put_f32(out, m->range);
    aoe_stream_put_i32(out, m->target_id);
    aoe_stream_put_f32(out, m->target_size_x);
    aoe_stream_put_f32(out, m->target_size_y);
    aoe_stream_put_i32(out, m->retry_baseline);
    aoe_stream_put_i32(out, m->status);
    aoe_stream_put_u8(out, m->hold_155);
    aoe_stream_put_u8(out, m->retry);
    aoe_stream_put_i32(out, m->waypoint_count);
    aoe_stream_put_i32(out, m->waypoint_capacity);
    for (i = 0; i < m->waypoint_count; ++i)
        aoe_stream_put(out, m->waypoint_tile + i * 3, 3);
    aoe_stream_put_u8(out, m->waypoints_stale);
    for (i = 0; i < 4; ++i) aoe_stream_put_i32(out, r->field_158[i]);
    aoe_stream_put_f32(out, m->best);
    aoe_stream_put_u8(out, m->facet_history[0]);
    aoe_stream_put_u8(out, m->facet_history[1]);
    return !out->failed;
}

/* ---- type 40: 0x00405be0 / 0x00405ce0 ---- */

static int load_40(AoeUnitRecordRor *r, float version,
                   const AoeUnitRecordOpsRor *ops, AoeOutcomeStream *in) {
    if (!load_30(r, version, ops, in)) return 0;
    r->class_code = 0x28;
    r->field_180 = aoe_stream_u8(in);
    r->field_188 = version < 6.5f ? 0 : aoe_stream_u8(in); /* 0x0054296c */
    if (in->failed) return 0;
    return ops && ops->load_actions ? ops->load_actions(ops->context, r, 0, in) : 1;
}

static int save_40(const AoeUnitRecordRor *r, const AoeUnitRecordOpsRor *ops,
                   AoeOutcomeStream *out) {
    if (!save_30(r, ops, out)) return 0;
    aoe_stream_put_u8(out, r->field_180);
    aoe_stream_put_u8(out, r->field_188);
    if (out->failed) return 0;
    return ops && ops->save_actions ? ops->save_actions(ops->context, r, 0, out) : 1;
}

/* ---- type 50: 0x004261a0 / 0x00426210 ---- */

static int load_50(AoeUnitRecordRor *r, float version,
                   const AoeUnitRecordOpsRor *ops, AoeOutcomeStream *in) {
    if (!load_40(r, version, ops, in)) return 0;
    r->class_code = 0x32;
    r->field_1b4 = aoe_stream_i32(in);
    if (version >= 2.01f) r->field_1b8 = aoe_stream_u8(in); /* 0x00543490 */
    return !in->failed;
}

static int save_50(const AoeUnitRecordRor *r, const AoeUnitRecordOpsRor *ops,
                   AoeOutcomeStream *out) {
    if (!save_40(r, ops, out)) return 0;
    aoe_stream_put_i32(out, r->field_1b4);
    aoe_stream_put_u8(out, r->field_1b8);
    return !out->failed;
}

static int load_template(AoeUnitRecordRor *r, const AoeUnitRecordOpsRor *ops,
                         AoeOutcomeStream *in) {
    r->template_kind = aoe_stream_u8(in);
    if (in->failed) return 0;
    return ops && ops->load_template
               ? ops->load_template(ops->context, r, r->template_kind, in)
               : 1;
}

static int save_template(const AoeUnitRecordRor *r,
                         const AoeUnitRecordOpsRor *ops, AoeOutcomeStream *out) {
    return ops && ops->save_template ? ops->save_template(ops->context, r, out) : 1;
}

/* ---- type 60: 0x00448f00 / 0x00449010 ---- */

static int load_60(AoeUnitRecordRor *r, float version,
                   const AoeUnitRecordOpsRor *ops, AoeOutcomeStream *in) {
    if (!load_50(r, version, ops, in)) return 0;
    r->class_code = 0x3c;
    if (version <= 7.09f) /* 0x00544830 */
        r->field_1bc_from_template = 1;
    else
        r->field_1bc = aoe_stream_i32(in);
    r->private_template = version < 7.3f ? 0 : aoe_stream_u8(in); /* 0x0054482c */
    if (in->failed) return 0;
    return r->private_template ? load_template(r, ops, in) : 1;
}

static int save_60(const AoeUnitRecordRor *r, const AoeUnitRecordOpsRor *ops,
                   AoeOutcomeStream *out) {
    if (!save_50(r, ops, out)) return 0;
    aoe_stream_put_i32(out, r->field_1bc);
    aoe_stream_put_u8(out, r->private_template);
    if (out->failed) return 0;
    return r->private_template ? save_template(r, ops, out) : 1;
}

/* ---- type 70: 0x004ae5c0 / 0x004ae750 ---- */

static int load_70(AoeUnitRecordRor *r, float version,
                   const AoeUnitRecordOpsRor *ops, AoeOutcomeStream *in) {
    if (!load_50(r, version, ops, in)) return 0;
    r->class_code = 0x46;
    r->private_template = aoe_stream_u8(in);
    if (r->private_template) {
        r->template_kind = aoe_stream_u8(in);
        if (in->failed) return 0;
        /* Only 'F' (0x004eca90) and 'P' (0x004ec180) make a template. */
        if ((r->template_kind == 'F' || r->template_kind == 'P') && ops &&
            ops->load_template &&
            !ops->load_template(ops->context, r, r->template_kind, in))
            return 0;
    }
    r->has_ai = aoe_stream_i32(in);
    if (in->failed) return 0;
    if (r->has_ai == 1 && ops && ops->load_ai)
        return ops->load_ai(ops->context, r, in);
    return 1;
}

static int save_70(const AoeUnitRecordRor *r, const AoeUnitRecordOpsRor *ops,
                   AoeOutcomeStream *out) {
    if (!save_50(r, ops, out)) return 0;
    aoe_stream_put_u8(out, r->private_template);
    if (r->private_template && !save_template(r, ops, out)) return 0;
    aoe_stream_put_i32(out, r->has_ai ? 1 : 0);
    if (out->failed) return 0;
    if (r->has_ai && ops && ops->save_ai) return ops->save_ai(ops->context, r, out);
    return 1;
}

/* ---- type 80: 0x004ac960 / 0x004ad0b0 ---- */

static int load_80(AoeUnitRecordRor *r, float version,
                   const AoeUnitRecordOpsRor *ops, AoeOutcomeStream *in) {
    int16_t i;
    if (!load_70(r, version, ops, in)) return 0;
    r->class_code = 0x50;
    r->building_completed = aoe_stream_u8(in);
    r->field_1d4 = aoe_stream_i32(in);
    r->completion_value = (uint32_t)aoe_stream_i32(in);
    r->field_1dc = aoe_stream_u8(in);
    if (version < 7.21f) return !in->failed; /* 0x00548208 */
    r->pair_count = get_i16(in);
    if (in->failed) return 0;
    free(r->pairs);
    r->pairs = NULL;
    if (r->pair_count > 0) {
        r->pairs = calloc((size_t)r->pair_count, sizeof(*r->pairs));
        if (!r->pairs) return 0;
        for (i = 0; i < r->pair_count; ++i) {
            r->pairs[i].a = get_i16(in);
            r->pairs[i].b = get_i16(in);
        }
    }
    r->field_1c6 = get_i16(in);
    r->field_1c8 = aoe_stream_u8(in);
    if (in->failed) return 0;
    return ops && ops->load_actions ? ops->load_actions(ops->context, r, 1, in) : 1;
}

static int save_80(const AoeUnitRecordRor *r, const AoeUnitRecordOpsRor *ops,
                   AoeOutcomeStream *out) {
    int16_t i;
    if (!save_70(r, ops, out)) return 0;
    aoe_stream_put_u8(out, r->building_completed);
    aoe_stream_put_i32(out, r->field_1d4);
    aoe_stream_put_i32(out, (int32_t)r->completion_value);
    aoe_stream_put_u8(out, r->field_1dc);
    put_i16(out, r->pair_count);
    for (i = 0; i < r->pair_count; ++i) {
        put_i16(out, r->pairs[i].a);
        put_i16(out, r->pairs[i].b);
    }
    put_i16(out, r->field_1c6);
    aoe_stream_put_u8(out, r->field_1c8);
    if (out->failed) return 0;
    return ops && ops->save_actions ? ops->save_actions(ops->context, r, 1, out) : 1;
}

int aoe_unit_record_load_ror(AoeUnitRecordRor *r, uint8_t class_code,
                             float version, const AoeUnitRecordOpsRor *ops,
                             AoeOutcomeStream *in) {
    int ok;
    if (!r || !in) return 0;
    switch (class_code) {
    case 10:
    case 90: ok = load_root(r, version, ops, in); r->class_code = class_code; break;
    case 20: ok = load_20(r, version, ops, in); break;
    case 30: ok = load_30(r, version, ops, in); break;
    case 40: ok = load_40(r, version, ops, in); break;
    case 50: ok = load_50(r, version, ops, in); break;
    case 60: ok = load_60(r, version, ops, in); break;
    case 70: ok = load_70(r, version, ops, in); break;
    case 80: ok = load_80(r, version, ops, in); break;
    default: return 0;
    }
    return ok && !in->failed;
}

int aoe_unit_record_save_ror(const AoeUnitRecordRor *r,
                             const AoeUnitRecordOpsRor *ops,
                             AoeOutcomeStream *out) {
    if (!r || !out) return 0;
    switch (r->class_code) {
    case 10:
    case 90: return save_root(r, ops, out);
    case 20: return save_20(r, ops, out);
    case 30: return save_30(r, ops, out);
    case 40: return save_40(r, ops, out);
    case 50: return save_50(r, ops, out);
    case 60: return save_60(r, ops, out);
    case 70: return save_70(r, ops, out);
    case 80: return save_80(r, ops, out);
    default: return 0;
    }
}

void aoe_unit_record_free_ror(AoeUnitRecordRor *r) {
    if (!r) return;
    free(r->dependents);
    free(r->group);
    free(r->pairs);
    r->dependents = r->group = NULL;
    r->pairs = NULL;
    r->dependent_count = r->group_count = 0;
    r->pair_count = 0;
    aoe_unit_path_free_ror(&r->movement.path);
    aoe_unit_path_free_ror(&r->movement.alternate_path);
    aoe_movement_waypoint_free_ror(&r->movement);
}

void aoe_unit_record_defaults_ror(AoeUnitRecordRor *r, uint8_t class_code) {
    AoeMovementRor *m;
    if (!r || class_code < 30) return;
    m = &r->movement;
    /* 0x0044ad30: the paths, then the fields in the constructor's order. */
    aoe_unit_path_init_ror(&m->path, PATH_CAPACITY);
    aoe_unit_path_init_ror(&m->alternate_path, PATH_CAPACITY);
    m->status = 2;
    m->free_terrain_a = m->free_terrain_b = -1;
    m->use_alternate_path = m->hold_155 = m->retry = 0;
    m->waypoint_tile = NULL;
    m->waypoint_count = m->waypoint_capacity = 0;
    m->waypoints_stale = 0;
    m->start_x = m->start_y = m->start_z = 0.0f;
    aoe_movement_set_destination_ror(m, 0.0f, 0.0f, 0.0f);
    m->turn_timer = 0.0f;
    m->facet_history[0] = m->facet_history[1] = 0xff;
    m->blocked_wait = 0.0f;
    m->range = 0.0f;
    m->target_id = -1;
    m->target_size_x = m->target_size_y = 0.0f;
    m->retry_baseline = 0;
    m->best = 100000.0f; /* 0x47c35000 */
    r->class_code = 0x1e;
    if (class_code < 40) return;
    r->field_180 = 0; /* 0x004058a0 */
    r->class_code = 0x28;
    if (class_code < 50) return;
    r->field_1b8 = 0; /* 0x00426140: +0x18c..+0x1b0 are action state */
    r->class_code = 0x32;
    if (class_code == 60) {
        r->private_template = 0; /* 0x00448ec0 +0x1c0 */
        r->class_code = 0x3c;
    } else if (class_code == 70) {
        r->class_code = 0x46;
    } else if (class_code == 80) {
        r->class_code = 0x50;
    }
}

void aoe_unit_record_init_facing_ror(AoeUnitRecordRor *r, int graphic_flag_2,
                                     int16_t angle_count, int32_t rand_value) {
    AoeMovementRor *m;
    if (!r) return;
    m = &r->movement;
    r->class_code = 0x1e;
    m->velocity_x = m->velocity_y = m->velocity_z = 0.0f;
    m->facing = 0.0f;
    m->field_a0 = 0;
    r->field_8c = 0;
    if (graphic_flag_2 && angle_count > 1) {
        /* (2 pi / n) * ((rand * n) / 0x7fff): a signed 32-bit quotient. */
        int32_t steps = (int32_t)((int64_t)rand_value * angle_count / 0x7fff);
        float f = (float)((double)(TWO_PI / (float)angle_count) * (float)steps);
        if (f < 0.0f)
            f += TWO_PI;
        else if (f > TWO_PI)
            f -= TWO_PI;
        m->facing = f;
    }
}

void aoe_unit_record_destroy_ror(AoeUnitRecordRor *r,
                                 void (*free_actions)(void *context),
                                 void *context) {
    if (!r) return;
    if (r->class_code >= 0x28 && free_actions) free_actions(context);
    aoe_unit_path_free_ror(&r->movement.path);
    aoe_unit_path_free_ror(&r->movement.alternate_path);
    aoe_movement_waypoint_free_ror(&r->movement);
}

void *aoe_unit_record_load_object_ror(uint8_t class_code,
                                      void *(*load)(void *context, uint8_t class_code),
                                      void *context) {
    switch (class_code) {
    case 10: case 0x14: case 0x19: case 0x1e: case 0x28: case 0x32: case 0x3c:
        return load ? load(context, class_code) : NULL;
    default:
        return NULL;
    }
}
