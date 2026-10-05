#include <stddef.h>
/* Patched RoR 1.0a diplomacy tribute logic 0x004323b0 / 0x00432ff0. */
#include "tribute.h"

#include <assert.h>

static int sends;
static float last_amount;
static int last_resource;
static void send(void *c, int32_t p, int r, float a, float f) {
    (void)c; (void)p; (void)f;
    ++sends;
    last_amount = a;
    last_resource = r;
}

static int td_base, td_begin, td_end, td_icons, td_last_y;
static uint8_t td_fill;
static const uint8_t td_table[1];
static void tdb(void *c) { (void)c; ++td_base; }
static void tdbeg(void *c, int32_t a) { (void)c; assert(a == 0); ++td_begin; }
static void tdend(void *c) { (void)c; ++td_end; }
static const uint8_t *tdtab(void *c, int32_t p, int16_t *col) { (void)c; *col = 2; return p == 2 ? NULL : td_table; }
static void tdfill(void *c, uint8_t f) { (void)c; td_fill = f; }
static void tdicon(void *c, int32_t x, int32_t y, const uint8_t *t) { (void)c; assert(x == 15 && t == td_table); td_last_y = y; ++td_icons; }

static void draw_tests(void) {
    AoeTributeDrawOpsRor o = {NULL, tdb, 1, tdbeg, tdend, 4, tdtab, tdfill, tdicon, 5, 7};
    aoe_tribute_draw_ror(&o);
    /* Players 1 and 3 (2 has no table): the third row at y 7 + 0x50 + 60. */
    assert(td_base == 1 && td_begin == 1 && td_end == 1 && td_icons == 2 && td_fill == 48 &&
           td_last_y == 7 + 0x50 + 60);
    o.has_icons = 0;
    aoe_tribute_draw_ror(&o);
    assert(td_base == 2 && td_begin == 1);
}

int main(void) {
    draw_tests();
    AoeTributeRor t;
    aoe_tribute_clear_ror(&t);
    assert(aoe_tribute_exhausted_ror(&t, 3, 0.0f, 0));
    assert(aoe_tribute_exhausted_ror(&t, 3, 500.0f, 1));
    /* A 25% fee: 125 per hundred. */
    assert(aoe_tribute_click_ror(&t, 3, 2, 300.0f, 0.25f));
    assert(t.given[3][2] == 100.0f && t.spent[3] == 125.0f);
    assert(aoe_tribute_click_ror(&t, 3, 2, 300.0f, 0.25f));
    /* 50 left pays for 40. */
    assert(aoe_tribute_click_ror(&t, 3, 4, 300.0f, 0.25f));
    assert(t.given[3][4] == 40.0f && t.spent[3] == 300.0f);
    assert(!aoe_tribute_click_ror(&t, 3, 4, 300.0f, 0.25f));
    aoe_tribute_click_ror(&t, 1, 2, 1000.0f, 0.0f);
    aoe_tribute_send_ror(&t, 5, 4, 0.25f, send, 0);
    assert(sends == 2 && last_resource == 1 && last_amount == 100.0f);
    assert(aoe_diplomacy_stance_ror(1, 1) == 0 && aoe_diplomacy_stance_ror(0, 1) == 1 &&
           aoe_diplomacy_stance_ror(0, 0) == 3);
    return 0;
}
