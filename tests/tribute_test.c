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

int main(void) {
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
