#include "world_base_load.h"

#include <stdlib.h>
#include <string.h>

/* The version gates' constants: f32 0x00545e80 1.13, 0x00545e60 1.03,
   0x00545e50 1.15, 0x00545e4c 1.02 (fcomp dword); f64 0x00545e78 1.13,
   0x00545e70 1.07, 0x00545e68 1.11, 0x00544e20 1.09, 0x00545e58 1.10,
   0x00544e10 1.08 (fcomp qword: the widened f32 against the double). */
#define F113 1.13f
#define F103 1.03f
#define F115 1.15f
#define F102 1.02f
#define D113 1.13
#define D107 1.07
#define D111 1.11
#define D109 1.09
#define D110 1.10
#define D108 1.08

/* The reset's (0x004755b0) heap names and description and the inline
   texts +0x1508..+0x17a8 are 0xe0 bytes. */
#define TEXT_CAP 0xe0

static int16_t read_i16(AoeOutcomeStream *s) {
    const uint8_t *p = aoe_stream_take(s, 2);
    return p ? (int16_t)aoe_le_u16(p) : 0;
}

static void read_bytes(AoeOutcomeStream *s, void *out, size_t n) {
    const uint8_t *p = aoe_stream_take(s, n);
    if (p && n) memcpy(out, p, n);
}

/* An i16 length, that many bytes into an inline 0xe0-byte text, the NUL
   at [length]. Port: at most 0xdf bytes kept; a negative length leaves
   the text; a short stream empties it. */
static void read_text_fixed(AoeOutcomeStream *s, char *buf) {
    int16_t n = read_i16(s);
    const uint8_t *p;
    size_t keep;
    if (n < 0) return;
    p = n > 0 ? aoe_stream_take(s, (size_t)n) : NULL;
    keep = p ? ((size_t)n < TEXT_CAP ? (size_t)n : TEXT_CAP - 1) : 0;
    if (keep) memcpy(buf, p, keep);
    buf[keep] = '\0';
}

/* The same into a heap text of the reset's 0xe0 bytes. Port: a longer
   text grows it (kept at 0xdf bytes when that fails); a missing one is
   made. */
static void read_text_heap(AoeOutcomeStream *s, char **buf) {
    int16_t n = read_i16(s);
    const uint8_t *p;
    size_t keep;
    if (n < 0) return;
    p = n > 0 ? aoe_stream_take(s, (size_t)n) : NULL;
    keep = p ? (size_t)n : 0;
    if (!*buf || keep >= TEXT_CAP) {
        char *g = realloc(*buf, keep >= TEXT_CAP ? keep + 1 : TEXT_CAP);
        if (g) {
            *buf = g;
        } else if (*buf) {
            keep = TEXT_CAP - 1;
        } else {
            return;
        }
    }
    if (keep) memcpy(*buf, p, keep);
    (*buf)[keep] = '\0';
}

/* An i16 length, a dropped text (into the original's 0xe0-byte local). */
static void skip_text(AoeOutcomeStream *s) {
    int16_t n = read_i16(s);
    if (n > 0) (void)aoe_stream_take(s, (size_t)n);
}

/* An i16 length; below 1 none, else a fresh buffer (0x0052607b) of the
   bytes, the last one made the NUL when `terminate`. Port: one spare
   zero byte. */
static char *read_message(AoeOutcomeStream *s, int terminate) {
    int16_t n = read_i16(s);
    const uint8_t *p;
    char *m;
    if (n < 1) return NULL;
    p = aoe_stream_take(s, (size_t)n);
    m = calloc((size_t)n + 1, 1);
    if (!m) return NULL;
    if (p) memcpy(m, p, (size_t)n);
    if (terminate) m[n - 1] = '\0';
    return m;
}

/* A positive size's bytes into a fresh buffer (0x0052607b). Port: none
   for bytes the stream lacks. */
static const void *read_data(AoeOutcomeStream *s, int32_t size) {
    const uint8_t *p;
    uint8_t *d;
    if (size <= 0) return NULL;
    p = aoe_stream_take(s, (size_t)size);
    if (!p) return NULL;
    d = malloc((size_t)size);
    if (d) memcpy(d, p, (size_t)size);
    return d;
}

/* 0x00474d40 */
float aoe_scenario_base_read_ror(AoeScenarioBaseRor *sc, AoeOutcomeStream *s, void *world,
                                 const AoeScenarioBaseOpsRor *ops) {
    AoeScenarioObjectRor *o = &sc->settings;
    float v;
    int i;
    sc->world = world;
    sc->entries = NULL;
    sc->picture = NULL;
    (void)aoe_scenario_reset_ror(o, ops->string, ops->context); /* 0x004755b0 */
    v = aoe_stream_f32(s);
    sc->version = v;
    if (v > F113)
        for (i = 0; i < AOE_SCENARIO_PLAYERS_ROR; ++i) read_bytes(s, o->player_name[i], 0x100);
    if ((double)v > D113)
        for (i = 0; i < AOE_SCENARIO_PLAYERS_ROR; ++i) {
            o->value_194c[i] = aoe_stream_i32(s);
            o->value_18cc[i] = aoe_stream_i32(s);
            o->value_190c[i] = aoe_stream_i32(s);
            o->player_kind[i] = aoe_stream_i32(s);
        }
    /* test ah, 1: below 1.07 or unordered. */
    if ((double)v >= D107)
        o->field_c = aoe_stream_u8(s);
    else
        o->field_c = 1;
    sc->entries = calloc(1, sizeof *sc->entries); /* operator new(0x14) */
    if (sc->entries)
        aoe_scenario_entries_read_ror(sc->entries, s, world, ops->entries); /* 0x004ebbf0 */
    read_text_heap(s, &o->description);
    o->message[0] = read_message(s, 1);
    if ((double)v >= D111) {
        o->message[1] = read_message(s, 1);
        o->message[2] = read_message(s, 1);
        o->message[3] = read_message(s, 1);
        o->message[4] = read_message(s, 0);
    } else {
        o->message[1] = o->message[2] = o->message[3] = o->message[4] = NULL;
    }
    /* test ah, 1; je: below 1.03 or unordered. */
    if (!(v >= F103)) {
        skip_text(s);
        skip_text(s);
        skip_text(s);
    }
    read_text_fixed(s, o->text_1508);
    read_text_fixed(s, o->text_15e8);
    read_text_fixed(s, o->text_16c8);
    if ((double)v >= D109) read_text_fixed(s, o->text_17a8);
    if ((double)v >= D110 && ops->picture_read)
        sc->picture = ops->picture_read(ops->context, s, 1); /* 0x45af60 */
    for (i = 0; i < AOE_SCENARIO_PLAYERS_ROR; ++i) read_text_heap(s, &o->ai_name[i]);
    for (i = 0; i < AOE_SCENARIO_PLAYERS_ROR; ++i) read_text_heap(s, &o->city_name[i]);
    if ((double)v >= D108)
        for (i = 0; i < AOE_SCENARIO_PLAYERS_ROR; ++i) read_text_heap(s, &o->per_name[i]);
    for (i = 0; i < AOE_SCENARIO_PLAYERS_ROR; ++i) {
        o->ai_size[i] = aoe_stream_i32(s);
        o->city_size[i] = aoe_stream_i32(s);
        if ((double)v >= D108) o->per_size[i] = aoe_stream_i32(s);
        o->ai_data[i] = o->city_data[i] = o->per_data[i] = NULL;
        if (v >= F115) {
            o->ai_data[i] = read_data(s, o->ai_size[i]);
            o->city_data[i] = read_data(s, o->city_size[i]);
            o->per_data[i] = read_data(s, o->per_size[i]);
        }
    }
    if (v > F102) (void)aoe_stream_i32(s); /* the original's local, dropped */
    return v;
}

/* 0x00475490 */
void aoe_scenario_base_release_ror(AoeScenarioBaseRor *sc, const AoeScenarioBaseOpsRor *ops) {
    if (sc->entries) {
        aoe_scenario_entries_release_ror(sc->entries); /* deleting 0x004ebda0 */
        free(sc->entries);
        sc->entries = NULL;
    }
    aoe_scenario_release_ror(&sc->settings);
    if (sc->picture) {
        if (ops && ops->picture_free) ops->picture_free(ops->context, sc->picture);
        sc->picture = NULL;
    }
}
