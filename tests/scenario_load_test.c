#include <assert.h>
#include <string.h>

#include "scenario_load.h"

typedef struct { int present[9], views, placed, place_player, loads,
                 load_player[9], closed, ai_calls, ai_player; float scale;
                 int32_t px, id_seen; AoeScenarioLoadWorld *w; } T;
static void hdr(void *c, AoeOutcomeStream *s) { (void)c; (void)aoe_stream_i32(s); }
static void mp(void *c, AoeOutcomeStream *s) { (void)c; (void)s; }
static int pres(void *c, int p) { return ((T *)c)->present[p]; }
static void nv(void *c, int p) { (void)p; ((T *)c)->views++; }
static void place(void *c, int p, int32_t x, int32_t y, int32_t z, int16_t t,
                  uint8_t st, int32_t e) {
    T *k = c; (void)y; (void)z; (void)t; (void)st; (void)e;
    k->placed++; k->place_player = p; k->px = x;
    assert(k->w->placing == 1); k->id_seen = k->w->placing_id;
}
static void ld(void *c, int p, AoeOutcomeStream *s, const int32_t m[9], float f) {
    T *k = c; (void)s; (void)m; k->load_player[k->loads++] = p; k->scale = f;
}
static void cl(void *c, AoeOutcomeStream *s) { (void)s; ((T *)c)->closed++; }
static int32_t g(void *c, int p) { (void)c; return p; }
static void sai(void *c, int p, int32_t a, int32_t b, int32_t d, int32_t e, int32_t f) {
    T *k = c; (void)a; (void)b; (void)d; (void)e; assert(f == -1);
    k->ai_calls++; k->ai_player = p;
}

static size_t put(uint8_t *b, size_t o, int32_t v) {
    b[o] = (uint8_t)v; b[o+1] = (uint8_t)(v >> 8); b[o+2] = (uint8_t)(v >> 16);
    b[o+3] = (uint8_t)(v >> 24); return o + 4;
}

static void run(int variant) {
    uint8_t b[256]; size_t o = 0; int i;
    T t; AoeScenarioLoadWorld w; AoeOutcomeStream s;
    int32_t order[2] = {1, 0}; /* slot 1 -> player 0, slot 0 -> player 1 */
    AoeScenarioLoadOps ops = {0, hdr, mp, pres, nv, place, ld, cl, g, g, g, g, sai};
    memset(&t, 0, sizeof t); memset(&w, 0, sizeof w); memset(b, 0, sizeof b);
    t.present[0] = t.present[1] = 1; t.w = &w; ops.ctx = &t;
    w.player_count = 2; w.load_counter = 7;
    o = put(b, o, 77);           /* header value */
    o = put(b, o, 0);            /* read_header's i32 */
    o = put(b, o, 2);            /* groups */
    o += 4;                      /* 2 x (u8, u8) */
    o = put(b, o, 0);            /* group 0: no units */
    o = put(b, o, 1);            /* group 1: one unit */
    o = put(b, o, 5); o = put(b, o, 6); o = put(b, o, 7); o = put(b, o, 42);
    o += 2 + 1; o = put(b, o, 9);
    o += 13;
    memset(&s, 0, sizeof s); s.data = b; s.size = o;
    aoe_scenario_load_legacy(&w, &ops, &s, order, variant);
    assert(!s.failed && s.pos == o);
    assert(w.header_value == 77 && t.views == 2 && t.closed == 1);
    assert(t.placed == 1 && t.place_player == 0 && t.px == 5 && t.id_seen == 42);
    assert(w.placing == 0 && w.placing_id == 42);
    assert(t.loads == 1 && t.load_player[0] == 0);
    assert(t.scale == (variant ? 1.03f : 1.0f));
    assert(t.ai_calls == 1 && t.ai_player == 0);
    assert(w.load_counter == (variant ? 0 : 7));
    for (i = 0; i < 9; i++) (void)i;
}

static int ld2_calls; static float ld2_scale;
static void ld2(void *c, int p, AoeOutcomeStream *s, const int32_t m[9], float f) { (void)c; (void)p; (void)s; (void)m; ++ld2_calls; ld2_scale = f; }
static void run_late(int variant) {
    uint8_t b[256]; size_t o = 0;
    T t; AoeScenarioLoadWorld w; AoeOutcomeStream s;
    int32_t order[2] = {1, 0};
    AoeScenarioLoadOps ops = {0, hdr, mp, pres, nv, place, ld, cl, g, g, g, g, sai, ld2};
    float scale = variant == 2 || variant == 3 ? 1.04f : variant == 4 ? 1.06f : variant == 5 ? 1.07f : 1.08f;
    memset(&t, 0, sizeof t); memset(&w, 0, sizeof w); memset(b, 0, sizeof b);
    t.present[0] = t.present[1] = 1; t.w = &w; ops.ctx = &t; ld2_calls = 0;
    w.player_count = 2; w.load_counter = 7;
    o = put(b, o, 3); o += 3;    /* the skipped block */
    o = put(b, o, 77);           /* header value */
    o = put(b, o, 0);            /* read_header's i32 */
    o = put(b, o, 2);            /* groups */
    if (variant == 2) o += 4;    /* 2 x (u8, u8) */
    o = put(b, o, 0);            /* group 0: no units */
    o = put(b, o, 1);            /* group 1: one unit */
    o = put(b, o, 5); o = put(b, o, 6); o = put(b, o, 7); o = put(b, o, 42);
    o += 2 + 1; o = put(b, o, 9);
    if (variant == 2) o += 13;
    if (variant >= 4) o = put(b, o, 2); /* the count again for the +0x70 loads */
    memset(&s, 0, sizeof s); s.data = b; s.size = o;
    aoe_scenario_load_legacy(&w, &ops, &s, order, variant);
    assert(!s.failed && s.pos == o);
    assert(w.header_value == 77 && t.views == 2 && t.closed == 1 && w.load_counter == 0);
    assert(t.placed == 1 && t.place_player == 0 && t.px == 5 && t.id_seen == 42);
    assert(t.loads == 1 && t.load_player[0] == 0 && t.scale == scale);
    assert(ld2_calls == (variant >= 4 ? 1 : 0) && (variant < 4 || ld2_scale == scale));
    assert(t.ai_calls == 1 && t.ai_player == 0);
}

int main(void) {
    run(0);
    run(1);
    run_late(2); run_late(3); run_late(4); run_late(5); run_late(6);
    return 0;
}
