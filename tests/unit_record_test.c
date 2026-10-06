/* Patched RoR 1.0a unit records: the class chain 0x004a6bb0 / 0x004a95e0
   (root), 0x004151c0 / 0x00415280 (20), 0x0044b360 / 0x0044b690 (30),
   0x00405be0 / 0x00405ce0 (40), 0x004261a0 / 0x00426210 (50),
   0x00448f00 / 0x00449010 (60), 0x004ae5c0 / 0x004ae750 (70) and
   0x004ac960 / 0x004ad0b0 (80). */
#include "unit_record.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int log_n;
static char log_buf[64];
static void note(char c) { if (log_n < 63) log_buf[log_n++] = c; }

static int load_sprites(void *c, AoeUnitRecordRor *r, AoeOutcomeStream *in) {
    (void)c; (void)r; note('s'); return aoe_stream_u8(in) == 0xaa;
}
static int save_sprites(void *c, const AoeUnitRecordRor *r, AoeOutcomeStream *out) {
    (void)c; (void)r; note('S'); aoe_stream_put_u8(out, 0xaa); return 1;
}
static int load_actions(void *c, AoeUnitRecordRor *r, int which, AoeOutcomeStream *in) {
    (void)c; (void)r; note(which ? 'q' : 'a'); return aoe_stream_u8(in) == (which ? 0x51 : 0x41);
}
static int save_actions(void *c, const AoeUnitRecordRor *r, int which, AoeOutcomeStream *out) {
    (void)c; (void)r; note(which ? 'Q' : 'A'); aoe_stream_put_u8(out, which ? 0x51 : 0x41); return 1;
}
static int load_template(void *c, AoeUnitRecordRor *r, uint8_t kind, AoeOutcomeStream *in) {
    (void)c; (void)r; note(kind); return aoe_stream_u8(in) == 0x77;
}
static int save_template(void *c, const AoeUnitRecordRor *r, AoeOutcomeStream *out) {
    (void)c; note('T'); aoe_stream_put_u8(out, r->template_kind); aoe_stream_put_u8(out, 0x77); return 1;
}
static int load_ai(void *c, AoeUnitRecordRor *r, AoeOutcomeStream *in) {
    (void)c; (void)r; note('i'); return aoe_stream_u8(in) == 0x99;
}
static int save_ai(void *c, const AoeUnitRecordRor *r, AoeOutcomeStream *out) {
    (void)c; (void)r; note('I'); aoe_stream_put_u8(out, 0x99); return 1;
}

static const AoeUnitRecordOpsRor ops = {
    NULL, load_sprites, save_sprites, load_actions, save_actions,
    load_template, save_template, load_ai, save_ai,
};

static uint8_t buf[2048];

static AoeOutcomeStream writer(void) {
    AoeOutcomeStream s = {0};
    s.data = buf;
    s.size = sizeof(buf);
    return s;
}

static AoeOutcomeStream reader(size_t n) {
    AoeOutcomeStream s = {0};
    s.data = buf;
    s.size = n;
    return s;
}

static void fill_root(AoeUnitRecordRor *r) {
    r->owner = 3;
    r->template_id = 0x4d;
    r->sprite_id = 0x1234;
    r->attached_id = -1;
    r->hit_points = 25.5f;
    r->state = 2;
    r->positive_hit_points = 1;
    r->type_25 = 0;
    r->registration_pending = 1;
    r->unit_id = 77;
    r->frame = 5;
    r->x = 10.5f; r->y = 20.25f; r->z = 1.0f;
    r->draw_left = -4; r->draw_top = -8; r->draw_right = 4; r->draw_bottom = 8;
    r->field_37 = 9;
    r->carried_type = 2;
    r->carried_amount = 7.0f;
    r->target_refs = 1;
    r->damage_percent = 50;
    r->byte_86 = 3;
    r->dependent_source_id = 12;
    r->dependent_context_bits = 0xdeadbeef;
    r->dependents = malloc(2 * sizeof(int32_t));
    r->dependents[0] = 100; r->dependents[1] = 101;
    r->dependent_count = 2;
    r->group = malloc(sizeof(int32_t));
    r->group[0] = 55;
    r->group_count = 1;
}

static int lo_calls;
static void *lo_load(void *c, uint8_t k) { (void)k; ++lo_calls; return c; }

int main(void) {
    {
        int tag;
        assert(aoe_unit_record_load_object_ror(0x19, lo_load, &tag) == &tag && lo_calls == 1);
        assert(aoe_unit_record_load_object_ror(0x3c, lo_load, &tag) == &tag && lo_calls == 2);
        assert(aoe_unit_record_load_object_ror(0x46, lo_load, &tag) == NULL && lo_calls == 2);
        assert(aoe_unit_record_load_object_ror(0x0b, lo_load, &tag) == NULL);
    }
    AoeUnitRecordRor w, r;
    AoeOutcomeStream out, in;

    /* Root (type 10) round trip; the saved class byte opens the record;
       the sprite list follows the ID vectors. */
    memset(&w, 0, sizeof w);
    w.class_code = 10;
    fill_root(&w);
    out = writer();
    log_n = 0;
    assert(aoe_unit_record_save_ror(&w, &ops, &out));
    assert(buf[0] == 10 && buf[1] == 3 && buf[2] == 0x4d && buf[3] == 0 &&
           buf[4] == 0x34 && buf[5] == 0x12);
    assert(log_n == 1 && log_buf[0] == 'S');
    /* class, owner, template, sprite, attached, hp, 4 bytes, id, frame,
       xyz, 4 i16, b, i16, f, 3 b, 2 i32, counts + ids, sprite byte */
    assert(out.pos == 1 + 1 + 2 + 2 + 4 + 4 + 4 + 4 + 1 + 12 + 8 + 1 + 2 + 4 + 3 + 8 +
                          (4 + 8) + (4 + 4) + 1);
    memset(&r, 0, sizeof r);
    in = reader(out.pos);
    aoe_stream_u8(&in); /* the world reads the class byte first */
    log_n = 0;
    assert(aoe_unit_record_load_ror(&r, 10, 7.23f, &ops, &in));
    assert(in.pos == out.pos && log_buf[0] == 's');
    assert(r.class_code == 10 && r.owner == 3 && r.template_id == 0x4d &&
           r.sprite_id == 0x1234 && r.attached_id == -1 && r.hit_points == 25.5f &&
           r.state == 2 && r.positive_hit_points == 1 && r.registration_pending == 1 &&
           r.unit_id == 77 && r.frame == 5 && r.x == 10.5f && r.y == 20.25f &&
           r.z == 1.0f && r.draw_left == -4 && r.draw_bottom == 8 && r.field_37 == 9 &&
           r.carried_type == 2 && r.carried_amount == 7.0f && r.target_refs == 1 &&
           r.damage_percent == 50 && r.byte_86 == 3 && r.dependent_source_id == 12 &&
           r.dependent_context_bits == 0xdeadbeef && r.dependent_count == 2 &&
           r.dependents[1] == 101 && r.group_count == 1 && r.group[0] == 55);
    aoe_unit_record_free_ror(&r);
    aoe_unit_record_free_ror(&w);

    /* An old save (7.0): no +0x4a byte; duplicate IDs collapse. */
    {
        AoeOutcomeStream o = writer();
        aoe_stream_put_u8(&o, 1);          /* owner */
        aoe_stream_put(&o, "\x05\x00", 2); /* template */
        aoe_stream_put(&o, "\xff\xff", 2); /* sprite -1 */
        aoe_stream_put_i32(&o, 9);         /* attached */
        aoe_stream_put_f32(&o, 1.0f);
        aoe_stream_put_u8(&o, 2);
        aoe_stream_put_u8(&o, 1);
        aoe_stream_put_u8(&o, 0); /* +0x4b (no +0x4a before 7.09) */
        aoe_stream_put_i32(&o, 5);
        aoe_stream_put_u8(&o, 0);
        aoe_stream_put_f32(&o, 0); aoe_stream_put_f32(&o, 0); aoe_stream_put_f32(&o, 0);
        aoe_stream_put(&o, "\0\0\0\0\0\0\0\0", 8);
        aoe_stream_put_u8(&o, 0);
        aoe_stream_put(&o, "\0\0", 2);
        aoe_stream_put_f32(&o, 0);
        aoe_stream_put(&o, "\0\0\0", 3);
        aoe_stream_put_i32(&o, -1); aoe_stream_put_i32(&o, 0);
        aoe_stream_put_i32(&o, 3);
        aoe_stream_put_i32(&o, 8); aoe_stream_put_i32(&o, 8); aoe_stream_put_i32(&o, 9);
        aoe_stream_put_i32(&o, 0);
        aoe_stream_put_u8(&o, 0xaa);
        memset(&r, 0, sizeof r);
        r.type_25 = 0x5c;
        in = reader(o.pos);
        assert(aoe_unit_record_load_ror(&r, 90, 7.0f, &ops, &in) && in.pos == o.pos);
        assert(r.class_code == 90 && r.type_25 == 0x5c && r.attached_id == 9 &&
               r.sprite_id == -1 && r.dependent_count == 2 && r.dependents[0] == 8 &&
               r.dependents[1] == 9 && r.group_count == 0);
        aoe_unit_record_free_ror(&r);
        /* A short stream fails. */
        memset(&r, 0, sizeof r);
        in = reader(10);
        assert(!aoe_unit_record_load_ror(&r, 10, 7.23f, &ops, &in));
        aoe_unit_record_free_ror(&r);
        assert(!aoe_unit_record_load_ror(&r, 15, 7.23f, &ops, &in));
    }

    /* Type 80: the whole chain. Defaults first (type 30's constructor). */
    memset(&w, 0, sizeof w);
    aoe_unit_record_defaults_ror(&w, 80);
    assert(w.class_code == 0x50 && w.movement.status == 2 && w.movement.free_terrain_a == -1 &&
           w.movement.target_id == -1 && w.movement.best == 100000.0f &&
           w.movement.facet_history[0] == 0xff && w.movement.path.capacity == 10 &&
           w.movement.alternate_path.capacity == 10);
    fill_root(&w);
    w.animation_rate = 1.5f;
    w.field_8c = 0x11223344;
    w.movement.velocity_x = 0.5f;
    w.movement.facing = 3.0f;
    w.movement.field_a0 = 6;
    assert(aoe_unit_path_append_ror(&w.movement.path, 1, 2, 3, 4));
    assert(aoe_unit_path_append_ror(&w.movement.path, 5, 6, 7, 8));
    assert(aoe_unit_path_append_ror(&w.movement.path, 9, 10, 11, 12));
    aoe_unit_path_rewind_ror(&w.movement.path);
    aoe_unit_path_advance_ror(&w.movement.path);
    w.movement.start_x = 1; w.movement.start_direction = 3;
    w.movement.dest_x = 7; w.movement.dest_y = 8; w.movement.dest_direction = 2;
    w.movement.turn_timer = 0.25f;
    w.movement.blocked_wait = -1.0f;
    w.movement.range = 2.0f;
    w.movement.target_id = 42;
    w.movement.target_size_x = 1.0f;
    w.movement.retry_baseline = 3;
    w.movement.status = 1;
    w.movement.hold_155 = 1;
    w.movement.retry = 2;
    w.movement.waypoint_tile = calloc(5, 3);
    memcpy(w.movement.waypoint_tile, "\x01\x02\x03\x04\x05\x06", 6);
    w.movement.waypoint_count = 2;
    w.movement.waypoint_capacity = 5;
    w.movement.waypoints_stale = 1;
    w.field_158[0] = 10; w.field_158[3] = 13;
    w.movement.best = 12.5f;
    w.movement.facet_history[0] = 4; w.movement.facet_history[1] = 5;
    w.field_180 = 0x80; w.field_188 = 0x88;
    w.field_1b4 = 0x1b4; w.field_1b8 = 0xb8;
    w.private_template = 1; w.template_kind = 'F';
    w.has_ai = 1;
    w.building_completed = 1; w.field_1d4 = -5; w.completion_value = 0xfeed; w.field_1dc = 7;
    w.pairs = malloc(2 * sizeof(*w.pairs));
    w.pairs[0].a = 1; w.pairs[0].b = -2; w.pairs[1].a = 300; w.pairs[1].b = 400;
    w.pair_count = 2; w.field_1c6 = -9; w.field_1c8 = 0xc8;
    out = writer();
    log_n = 0;
    assert(aoe_unit_record_save_ror(&w, &ops, &out));
    assert(!strncmp(log_buf, "SATIQ", 5) && log_n == 5);
    memset(&r, 0, sizeof r);
    in = reader(out.pos);
    aoe_stream_u8(&in);
    log_n = 0;
    assert(aoe_unit_record_load_ror(&r, 80, 7.23f, &ops, &in));
    assert(in.pos == out.pos && !strncmp(log_buf, "saFiq", 5));
    assert(r.class_code == 0x50 && r.animation_rate == 1.5f && r.field_8c == 0x11223344 &&
           r.movement.velocity_x == 0.5f && r.movement.facing == 3.0f && r.movement.field_a0 == 6 &&
           aoe_unit_path_count_ror(&r.movement.path) == 3 &&
           aoe_unit_path_cursor_ror(&r.movement.path) == 1 &&
           aoe_unit_path_at_ror(&r.movement.path, 2)->z == 11 &&
           aoe_unit_path_at_ror(&r.movement.path, 1)->direction == 8 &&
           r.movement.start_x == 1 && r.movement.start_direction == 3 &&
           r.movement.dest_y == 8 && r.movement.dest_direction == 2 &&
           r.movement.turn_timer == 0.25f && r.movement.blocked_wait == -1.0f &&
           r.movement.range == 2.0f && r.movement.target_id == 42 &&
           r.movement.target_size_x == 1.0f && r.movement.retry_baseline == 3 &&
           r.movement.status == 1 && r.movement.hold_155 == 1 && r.movement.retry == 2 &&
           r.movement.waypoint_count == 2 && r.movement.waypoint_capacity == 5 &&
           !memcmp(r.movement.waypoint_tile, "\x01\x02\x03\x04\x05\x06", 6) &&
           r.movement.waypoints_stale == 1 && r.field_158[0] == 10 && r.field_158[3] == 13 &&
           r.movement.best == 12.5f && r.movement.facet_history[0] == 4 &&
           r.movement.facet_history[1] == 5 && r.field_180 == 0x80 && r.field_188 == 0x88 &&
           r.field_1b4 == 0x1b4 && r.field_1b8 == 0xb8 && r.private_template == 1 &&
           r.template_kind == 'F' && r.has_ai == 1 && r.building_completed == 1 &&
           r.field_1d4 == -5 && r.completion_value == 0xfeed && r.field_1dc == 7 &&
           r.pair_count == 2 && r.pairs[0].b == -2 && r.pairs[1].a == 300 &&
           r.field_1c6 == -9 && r.field_1c8 == 0xc8);
    aoe_unit_record_free_ror(&r);
    aoe_unit_record_free_ror(&w);

    /* Type 30 at an old version (1.2): +0x158.. are -1, +0x178 100000,
       both facets 0xff, nothing read for them; 40 at 6.4 keeps +0x188 0;
       80 before 7.21 stops after +0x1dc. */
    {
        AoeUnitRecordRor t;
        memset(&t, 0, sizeof t);
        t.class_code = 30;
        out = writer();
        assert(aoe_unit_record_save_ror(&t, &ops, &out));
        memset(&r, 0, sizeof r);
        /* Before 7.09 the root has no +0x4a byte (offset 16 here). */
        memmove(buf + 16, buf + 17, out.pos - 17);
        in = reader(out.pos - 1 - (16 + 4 + 2)); /* drop +0x158.. +0x17d */
        aoe_stream_u8(&in);
        assert(aoe_unit_record_load_ror(&r, 30, 1.2f, &ops, &in) && in.pos == in.size);
        memmove(buf + 17, buf + 16, out.pos - 17); /* restore for 7.22 */
        buf[16] = 0;
        assert(r.field_158[2] == -1 && r.movement.best == 100000.0f &&
               r.movement.facet_history[0] == 0xff && r.movement.facet_history[1] == 0xff);
        aoe_unit_record_free_ror(&r);
        memset(&r, 0, sizeof r);
        in = reader(out.pos); /* 7.22: +0x17d missing -> 0xff, rest read */
        aoe_stream_u8(&in);
        assert(aoe_unit_record_load_ror(&r, 30, 7.22f, &ops, &in) && in.pos == out.pos - 1);
        assert(r.movement.facet_history[1] == 0xff && r.movement.best == 0.0f);
        aoe_unit_record_free_ror(&r);
    }

    /* Type 60: field +0x1bc comes from the template at 7.09 and older;
       the private template flag is unread before 7.3; 70 ignores an
       unknown template kind and no AI. */
    memset(&w, 0, sizeof w);
    w.class_code = 60; w.field_1bc = 0x1bc; w.private_template = 1; w.template_kind = 'X';
    out = writer();
    log_n = 0;
    assert(aoe_unit_record_save_ror(&w, &ops, &out) && !strncmp(log_buf, "SAT", 3));
    memset(&r, 0, sizeof r);
    in = reader(out.pos);
    aoe_stream_u8(&in);
    log_n = 0;
    assert(aoe_unit_record_load_ror(&r, 60, 7.3f, &ops, &in) && in.pos == out.pos &&
           r.field_1bc == 0x1bc && r.private_template == 1 && r.template_kind == 'X' &&
           !r.field_1bc_from_template && log_buf[2] == 'X');
    aoe_unit_record_free_ror(&r);
    memset(&r, 0, sizeof r);
    /* At 7.09 the type-30 part also lacks +0x178 / +0x17c / +0x17d: the 6
       bytes before the type-40 (3) and type-50 (5) parts and the 7 of
       +0x1bc, the flag and the template go. */
    memmove(buf + out.pos - 21, buf + out.pos - 15, 8);
    in = reader(out.pos - 13);
    aoe_stream_u8(&in);
    assert(aoe_unit_record_load_ror(&r, 60, 7.09f, &ops, &in) && in.pos == in.size &&
           r.field_1bc_from_template && !r.private_template &&
           r.movement.best == 100000.0f);
    aoe_unit_record_free_ror(&r);
    memset(&w, 0, sizeof w);
    w.class_code = 70; w.private_template = 1; w.template_kind = 'Z';
    out = writer();
    assert(aoe_unit_record_save_ror(&w, &ops, &out));
    /* The save wrote kind 'Z' and 0x77; the load reads only the kind and
       then the AI flag, so it sees 0x77 as part of the flag: feed a record
       without the template payload instead. */
    out = writer();
    aoe_stream_put_u8(&out, 70);
    {
        AoeUnitRecordRor t; memset(&t, 0, sizeof t); t.class_code = 50;
        AoeOutcomeStream o = writer();
        assert(aoe_unit_record_save_ror(&t, &ops, &o));
        out.pos = o.pos; /* the type-50 prefix, class byte rewritten below */
    }
    buf[0] = 70;
    aoe_stream_put_u8(&out, 1);   /* private template */
    aoe_stream_put_u8(&out, 'Z'); /* unknown kind: no template made */
    aoe_stream_put_i32(&out, 0);  /* no AI */
    memset(&r, 0, sizeof r);
    in = reader(out.pos);
    aoe_stream_u8(&in);
    log_n = 0;
    assert(aoe_unit_record_load_ror(&r, 70, 7.23f, &ops, &in) && in.pos == out.pos &&
           r.class_code == 0x46 && r.template_kind == 'Z' && !r.has_ai && log_n == 2);
    aoe_unit_record_free_ror(&r);

    /* The random facing of 0x0044b240: 8 angles, rand 0x4000 -> step 4
       -> pi; no angles or flag -> 0. */
    memset(&w, 0, sizeof w);
    aoe_unit_record_init_facing_ror(&w, 1, 8, 0x4000);
    assert(fabs((double)w.movement.facing - 3.1415927) < 1e-5 && w.class_code == 0x1e);
    aoe_unit_record_init_facing_ror(&w, 0, 8, 0x4000);
    assert(w.movement.facing == 0.0f);
    aoe_unit_record_init_facing_ror(&w, 1, 1, 0x7fff);
    assert(w.movement.facing == 0.0f);

    /* Destroy frees the movement storage and calls the action-list
       release only from type 40 on. */
    memset(&w, 0, sizeof w);
    aoe_unit_record_defaults_ror(&w, 40);
    log_n = 0;
    aoe_unit_record_destroy_ror(&w, (void (*)(void *))note, (void *)(intptr_t)'d');
    assert(w.movement.path.points == NULL);
    memset(&w, 0, sizeof w);
    aoe_unit_record_defaults_ror(&w, 30);
    aoe_unit_record_destroy_ror(&w, (void (*)(void *))note, (void *)(intptr_t)'d');
    assert(w.movement.path.points == NULL);
    return 0;
}
