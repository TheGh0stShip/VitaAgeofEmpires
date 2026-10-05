/* Patched RoR 1.0a small functions translated in batches: constants,
   accessors and thin wrappers, each checked against its decompilation. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "unit_vtable_defaults.h"
#include "effects_io.h"
#include <stdlib.h>

static int notices;
static void notice_fn(void *c, AoeLiveUnit *u, AoeLiveUnit *o, int32_t a, int32_t b, int32_t cc, int32_t d) {
    (void)c; (void)u; (void)o; (void)a; (void)b; (void)cc; assert(d == 0); ++notices;
}
static void root_state(void *c, AoeLiveUnit *u, uint8_t st) { (void)u; (void)st; ++((int *)c)[0]; }
static void special_state(void *c, AoeLiveUnit *u) { (void)u; ++((int *)c)[1]; }
static int pay_check(void *c, int16_t id, float f) { (void)c; return id == 12 && f == 0.25f; }


static void unit_defaults(void) {
    static AoeLiveUnit u;
    assert(aoe_unit_default_false_ror() == 0 && aoe_unit_default_true_ror() == 1);
    assert(aoe_unit_default_zero_ror() == 0 && aoe_unit_default_minus_one_ror() == -1);
    assert(aoe_unit_default_one_ror() == 1.0f && aoe_unit_default_zero_float_ror() == 0.0f &&
           aoe_unit_default_minus_one_float_ror() == -1.0f);
    aoe_unit_default_nothing_ror();
    u.animation_rate = 1.25f;
    u.core.scenario_rotation = 3.5f;
    assert(aoe_unit_animation_rate_ror(&u) == 1.25f && aoe_unit_rotation_ror(&u) == 3.5f);
    u.core.unit_template = (const DatUnit *)&u;
    {
        AoeObjectUpdateList *l = aoe_unit_new_list_ror();
        assert(l && !l->head && l->count == 0);
        free(l);
    }
    u.core.scenario_frame = 2;
    aoe_unit_turn_ror(&u, -3, 8);
    assert(u.core.scenario_frame == 7);
    aoe_unit_turn_ror(&u, 10, 8);
    assert(u.core.scenario_frame == 1);
    {
        static DatUnit t;
        int calls[2] = {0, 0};
        t.unit_class = 1;
        t.hit_points = 40;
        t.id = 12;
        u.core.unit_template = &t;
        u.core.state = 2;
        u.core.hit_points = 30.0f;
        aoe_unit_set_state_70_ror(&u, 3, 0, root_state, special_state, calls);
        assert(calls[1] == 1 && calls[0] == 0);
        aoe_unit_set_state_70_ror(&u, 3, 1, root_state, special_state, calls);
        assert(calls[0] == 1);
        aoe_unit_set_state_70_ror(&u, 2, 0, root_state, special_state, calls);
        assert(calls[0] == 2);
        assert(aoe_unit_can_pay_repair_ror(&u, pay_check, NULL));
        u.core.hit_points = 0.0f;
        assert(!aoe_unit_can_pay_repair_ror(&u, pay_check, NULL));
        u.core.unit_template = (const DatUnit *)&u;
    }
    {
        static AoeLiveUnit other;
        uint8_t n = 0;
        int32_t v = 77;
        aoe_unit_notice_from_ror(&u, &u, 1, 2, 3, &n, notice_fn, NULL);
        assert(n == 0 && notices == 0);
        aoe_unit_notice_from_ror(&u, &other, 1, 2, 3, &n, notice_fn, NULL);
        assert(n == 1 && notices == 1);
        assert(aoe_unit_field_16c_ror(&v) == 77);
    }
    aoe_unit_set_byte_86_ror(&u, 9);
    assert(aoe_unit_byte_86_ror(&u) == 9);
    assert(aoe_unit_template_ror(&u) == (const DatUnit *)&u && aoe_unit_default_ff_ror() == 0xff);
}

static void text_helpers(void) {
    char *f = NULL;
    aoe_text_set_ror(&f, "abc");
    assert(strcmp(f, "abc") == 0);
    aoe_text_set_joined_ror(&f, "dir/", "file");
    assert(strcmp(f, "dir/file") == 0);
    free(f);
}

int main(void) {
    unit_defaults();
    text_helpers();
    puts("small functions ok");
    return 0;
}
