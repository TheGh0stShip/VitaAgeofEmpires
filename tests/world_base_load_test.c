/* Patched RoR 1.0a base scenario binary constructor 0x00474d40 (version
   gates at 0x00474d8c..0x00475458 of EMPIRESX.EXE; constants f32
   0x00545e80 1.13, 0x00545e60 1.03, 0x00545e50 1.15, 0x00545e4c 1.02
   and f64 0x00545e78 1.13, 0x00545e70 1.07, 0x00545e68 1.11, 0x00544e20
   1.09, 0x00545e58 1.10, 0x00544e10 1.08). */
#include "world_base_load.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void str(void *c, int32_t id, char *out, int n) {
    (void)c;
    snprintf(out, (size_t)n, "%s", id == 0x277b ? "Random" : id == 0x2775 ? " <None> " : "");
}

static int pic_obj, pic_reads, pic_frees;
static int32_t pic_spans, pic_marker;
/* Stands in for 0x45af60: consumes one marker dword. */
static void *pic_read(void *c, AoeOutcomeStream *s, int32_t spans) {
    (void)c;
    ++pic_reads;
    pic_spans = spans;
    pic_marker = aoe_stream_i32(s);
    return &pic_obj;
}
static void pic_free(void *c, void *p) {
    (void)c;
    assert(p == &pic_obj);
    ++pic_frees;
}

static int ent_obj, ent_finds;
static void *ent_find(void *c, void *w, int16_t id) {
    (void)c; (void)w;
    ++ent_finds;
    return id == 5 ? (void *)&ent_obj : NULL;
}
static int16_t ent_tag(void *c, void *o) { (void)c; (void)o; return 40; }
static const AoeScenarioEntriesOpsRor ent_ops = {NULL, ent_find, ent_tag};
static const AoeScenarioBaseOpsRor ops = {NULL, str, &ent_ops, pic_read, pic_free};

static struct {
    uint8_t b[16384];
    size_t n;
} buf;
static void w_bytes(const void *p, size_t n) {
    assert(buf.n + n <= sizeof buf.b);
    memcpy(buf.b + buf.n, p, n);
    buf.n += n;
}
static void w_u8(uint8_t v) { w_bytes(&v, 1); }
static void w_i16(int16_t v) { uint8_t b[2]; aoe_le_put_u16(b, (uint16_t)v); w_bytes(b, 2); }
static void w_i32(int32_t v) { uint8_t b[4]; aoe_le_put_u32(b, (uint32_t)v); w_bytes(b, 4); }
static void w_f32(float v) { uint8_t b[4]; aoe_le_put_f32(b, v); w_bytes(b, 4); }
static void w_text(const char *t, int16_t n) { w_i16(n); if (n > 0) w_bytes(t, (size_t)n); }
static void w_entries_empty(void) { w_i16(0); w_i16(0); w_f32(-1.0f); }

static AoeOutcomeStream stream(void) {
    AoeOutcomeStream s;
    s.data = buf.b;
    s.size = buf.n;
    s.pos = 0;
    s.failed = 0;
    return s;
}

/* Version 1.15 (0x00475f80 writes it): every gate open. */
static void test_full_115(void) {
    AoeScenarioBaseRor *sc = calloc(1, sizeof *sc);
    AoeScenarioObjectRor *o = &sc->settings;
    AoeOutcomeStream s;
    int world = 0, i;
    float v;
    buf.n = 0;
    w_f32(1.15f);
    for (i = 0; i < 16; ++i) { /* above f32 1.13 (0x00474d9c): +0x0d names */
        uint8_t name[0x100];
        memset(name, 0, sizeof name);
        if (i == 0) memcpy(name, "Alice", 6);
        if (i == 1) memcpy(name, "Bob", 4);
        w_bytes(name, sizeof name);
    }
    for (i = 0; i < 16; ++i) { /* 0x00474dda: +0x194c, +0x18cc, +0x190c, +0x188c */
        w_i32(100 + i); w_i32(200 + i); w_i32(300 + i); w_i32(i % 6);
    }
    w_u8(0); /* +0x0c, at least 1.07 (0x00474e26) */
    /* 0x004ebbf0: one entry. */
    w_i16(1); w_i16(3); w_f32(2.5f);
    w_i32(0x11223344); w_u8(7); w_i16(-3); w_u8(9); w_i32(1); w_i32(2); w_i32(3);
    w_i16(4); w_i16(5); w_i16(40); w_i16(6); w_i16(-1);
    w_text("Hello", 5);              /* description, NUL at [5] (0x00474ea6) */
    w_text("abcd", 4);               /* message 0, [3] cut (0x00474ef9) */
    w_text("m1x", 3); w_text("m2x", 3); w_text("", 0);
    w_text("raw", 3);                /* message 4 kept whole (0x00475049) */
    w_text("T1", 2); w_text("", 0); w_text("T3", 2); w_text("T4", 2);
    w_i32(0x1234);                   /* the picture, at least 1.10 (0x00475224) */
    for (i = 0; i < 16; ++i) w_text("Rush", i == 0 ? 4 : 0);
    for (i = 0; i < 16; ++i) w_text("Wall", i == 1 ? 4 : 0);
    for (i = 0; i < 16; ++i) w_text("Def", i == 2 ? 3 : 0);
    for (i = 0; i < 16; ++i) { /* sizes then, at least f32 1.15, the bytes (0x004753cb) */
        w_i32(i == 2 ? 3 : 0); w_i32(i == 2 ? 2 : 0); w_i32(i == 3 ? 1 : 0);
        if (i == 2) { w_bytes("AIx", 3); w_bytes("CP", 2); }
        if (i == 3) w_bytes("P", 1);
    }
    w_i32(-99); /* above 1.02 (0x0047545d), dropped */
    s = stream();
    pic_reads = pic_frees = ent_finds = 0;
    v = aoe_scenario_base_read_ror(sc, &s, &world, &ops);
    assert(v == 1.15f && sc->version == 1.15f && sc->world == &world);
    assert(!s.failed && s.pos == buf.n);
    assert(strcmp(o->player_name[0], "Alice") == 0 && strcmp(o->player_name[1], "Bob") == 0);
    assert(o->player_name[2][0] == '\0');
    assert(o->value_194c[3] == 103 && o->value_18cc[3] == 203 && o->value_190c[3] == 303);
    assert(o->player_kind[3] == 3 && o->player_kind[15] == 3 && o->value_194c[15] == 115);
    assert(o->field_c == 0);
    assert(sc->entries && sc->entries->world == &world && sc->entries->count == 1);
    assert(sc->entries->v0e == 3 && sc->entries->v10 == 2.5f);
    assert(sc->entries->head && sc->entries->head->v0 == 0x11223344 && sc->entries->head->v6 == -3);
    assert(sc->entries->head->object1 == &ent_obj && sc->entries->head->object2 == NULL);
    assert(ent_finds == 1);
    assert(strcmp(o->description, "Hello") == 0);
    assert(strcmp(o->message[0], "abc") == 0 && strcmp(o->message[1], "m1") == 0);
    assert(strcmp(o->message[2], "m2") == 0 && o->message[3] == NULL);
    assert(strcmp(o->message[4], "raw") == 0);
    assert(strcmp(o->text_1508, "T1") == 0 && o->text_15e8[0] == '\0');
    assert(strcmp(o->text_16c8, "T3") == 0 && strcmp(o->text_17a8, "T4") == 0);
    assert(sc->picture == &pic_obj && pic_reads == 1 && pic_spans == 1 && pic_marker == 0x1234);
    assert(strcmp(o->ai_name[0], "Rush") == 0 && o->ai_name[1][0] == '\0');
    assert(strcmp(o->city_name[1], "Wall") == 0 && strcmp(o->per_name[2], "Def") == 0);
    assert(o->ai_size[2] == 3 && o->city_size[2] == 2 && o->per_size[3] == 1);
    assert(memcmp(o->ai_data[2], "AIx", 3) == 0 && memcmp(o->city_data[2], "CP", 2) == 0);
    assert(memcmp(o->per_data[3], "P", 1) == 0);
    assert(o->ai_data[0] == NULL && o->per_data[2] == NULL);
    aoe_scenario_base_release_ror(sc, &ops);
    assert(pic_frees == 1 && sc->entries == NULL && sc->picture == NULL);
    free(sc);
}

/* A version below every gate: the stream an unordered (NaN) version
   reads too, as the "below" gates take the jne/je C0 path
   (0x00474e24, 0x00474f17, 0x0047507c). `long_texts` adds the port's
   bounds: a 0x100-byte description grows its buffer, a 0x100-byte
   inline text keeps 0xdf bytes, a negative length leaves its text. */
static void build_old(float version, int long_texts) {
    static char big[0x100];
    int i;
    memset(big, 'x', sizeof big);
    buf.n = 0;
    w_f32(version);
    /* no names or values, no +0x0c byte */
    w_entries_empty();
    if (long_texts) { w_i16(0x100); w_bytes(big, 0x100); } else w_text("D", 1);
    w_text("", 0); /* message 0: none */
    /* messages 1..4 not read below 1.11 */
    w_text("skip1", 5); w_text("", 0); w_text("skip3", 5); /* below f32 1.03 */
    if (long_texts) {
        w_i16(0x100); w_bytes(big, 0x100);
        w_i16(-1);
    } else {
        w_text("A", 1);
        w_text("B", 1);
    }
    w_text("C", 1);
    /* no +0x17a8 below 1.09, no picture below 1.10 */
    for (i = 0; i < 16; ++i) w_text("", 0);
    for (i = 0; i < 16; ++i) w_text("", 0);
    /* no personality names below 1.08 */
    for (i = 0; i < 16; ++i) { w_i32(i == 0 ? 5 : 0); w_i32(0); }
    /* no data below 1.15, no dword up to 1.02 */
}

static void check_old(float version, int unordered) {
    AoeScenarioBaseRor *sc = calloc(1, sizeof *sc);
    AoeScenarioObjectRor *o = &sc->settings;
    AoeOutcomeStream s;
    float v;
    build_old(version, 0);
    s = stream();
    pic_reads = 0;
    v = aoe_scenario_base_read_ror(sc, &s, NULL, &ops);
    assert(unordered ? v != v : v == version);
    assert(!s.failed && s.pos == buf.n);
    assert(o->player_name[0][0] == '\0' && o->value_194c[0] == 0 && o->player_kind[0] == 0);
    assert(o->field_c == 1);
    assert(sc->entries && sc->entries->count == 0 && sc->entries->world == NULL);
    assert(strcmp(o->description, "D") == 0);
    assert(!o->message[0] && !o->message[1] && !o->message[2] && !o->message[3] && !o->message[4]);
    assert(strcmp(o->text_1508, "A") == 0 && strcmp(o->text_15e8, "B") == 0);
    assert(strcmp(o->text_16c8, "C") == 0 && strcmp(o->text_17a8, " <None> ") == 0);
    assert(sc->picture == NULL && pic_reads == 0);
    assert(o->ai_name[0][0] == '\0' && o->city_name[0][0] == '\0');
    assert(strcmp(o->per_name[0], "Random") == 0);
    assert(o->ai_size[0] == 5 && o->ai_data[0] == NULL && o->per_size[0] == 0);
    aoe_scenario_base_release_ror(sc, &ops);
    free(sc);
}

static void test_old_and_nan(void) {
    uint32_t bits = 0x7fc00000u;
    float qnan;
    memcpy(&qnan, &bits, 4);
    check_old(1.0f, 0);
    check_old(qnan, 1);
}

static void test_long_texts(void) {
    AoeScenarioBaseRor *sc = calloc(1, sizeof *sc);
    AoeScenarioObjectRor *o = &sc->settings;
    AoeOutcomeStream s;
    size_t i;
    build_old(1.0f, 1);
    s = stream();
    (void)aoe_scenario_base_read_ror(sc, &s, NULL, &ops);
    assert(!s.failed && s.pos == buf.n);
    assert(strlen(o->description) == 0x100);
    for (i = 0; i < 0x100; ++i) assert(o->description[i] == 'x');
    assert(strlen(o->text_1508) == 0xdf);
    assert(strcmp(o->text_15e8, " <None> ") == 0); /* length -1 */
    assert(strcmp(o->text_16c8, "C") == 0);
    aoe_scenario_base_release_ror(sc, &ops);
    free(sc);
}

/* 1.13f widened is 1.12999999523: not above f32 1.13 nor f64 1.13, so
   no names or values; every other gate up to 1.13 open, data closed. */
static void test_113(void) {
    AoeScenarioBaseRor *sc = calloc(1, sizeof *sc);
    AoeScenarioObjectRor *o = &sc->settings;
    AoeOutcomeStream s;
    int i;
    buf.n = 0;
    w_f32(1.13f);
    w_u8(0);
    w_entries_empty();
    w_text("", 0);
    w_text("", 0);
    for (i = 0; i < 4; ++i) w_text("", 0); /* messages 1..4 */
    for (i = 0; i < 4; ++i) w_text("", 0); /* the four texts */
    w_i32(0x55);                           /* the picture */
    for (i = 0; i < 48; ++i) w_text("", 0);
    for (i = 0; i < 16; ++i) { w_i32(0); w_i32(0); w_i32(i == 5 ? 2 : 0); }
    w_i32(-99);
    s = stream();
    pic_reads = 0;
    (void)aoe_scenario_base_read_ror(sc, &s, NULL, &ops);
    assert(!s.failed && s.pos == buf.n);
    assert(o->player_name[0][0] == '\0' && o->value_18cc[0] == 0 && o->field_c == 0);
    assert(o->text_17a8[0] == '\0' && pic_reads == 1 && pic_marker == 0x55);
    assert(o->per_name[0][0] == '\0' && o->per_size[5] == 2 && o->per_data[5] == NULL);
    aoe_scenario_base_release_ror(sc, &ops);
    free(sc);
}

/* 1.09f widened is 1.09000003338 >= f64 1.09: +0x17a8 read; below 1.11
   no messages 1..4; below 1.10 no picture; personality names read. */
static void test_109(void) {
    AoeScenarioBaseRor *sc = calloc(1, sizeof *sc);
    AoeScenarioObjectRor *o = &sc->settings;
    AoeOutcomeStream s;
    int i;
    buf.n = 0;
    w_f32(1.09f);
    w_u8(1);
    w_entries_empty();
    w_text("", 0);
    w_text("hi!", 3);
    w_text("", 0); w_text("", 0); w_text("", 0); w_text("T4", 2);
    for (i = 0; i < 32; ++i) w_text("", 0);
    for (i = 0; i < 16; ++i) w_text("Per", i == 0 ? 3 : 0);
    for (i = 0; i < 16; ++i) { w_i32(0); w_i32(0); w_i32(0); }
    w_i32(-99);
    s = stream();
    pic_reads = 0;
    (void)aoe_scenario_base_read_ror(sc, &s, NULL, &ops);
    assert(!s.failed && s.pos == buf.n);
    assert(o->field_c == 1 && strcmp(o->message[0], "hi") == 0 && o->message[1] == NULL);
    assert(strcmp(o->text_17a8, "T4") == 0 && pic_reads == 0 && sc->picture == NULL);
    assert(strcmp(o->per_name[0], "Per") == 0);
    aoe_scenario_base_release_ror(sc, &ops);
    free(sc);
}

/* A short stream stops reading; the object is still whole. */
static void test_short(void) {
    AoeScenarioBaseRor *sc = calloc(1, sizeof *sc);
    AoeOutcomeStream s;
    buf.n = 0;
    w_f32(1.15f);
    w_bytes("Al", 2);
    s = stream();
    (void)aoe_scenario_base_read_ror(sc, &s, NULL, &ops);
    assert(s.failed);
    assert(sc->entries && sc->settings.description && sc->settings.message[0] == NULL);
    assert(sc->picture == &pic_obj); /* the fake made one */
    aoe_scenario_base_release_ror(sc, &ops);
    free(sc);
}

int main(void) {
    test_full_115();
    test_old_and_nan();
    test_long_texts();
    test_113();
    test_109();
    test_short();
    return 0;
}
