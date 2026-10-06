#include "unit_graphic.h"

#include <assert.h>
#include <stddef.h>

typedef struct {
    AoeUnitGraphicState *state;
    const void *removed;
    const void *added;
    unsigned remove_count, add_count;
} Trace;

static void remove_graphic(void *context, const void *graphic) {
    Trace *trace = context;
    assert(trace->state->previous_graphic == graphic);
    assert(trace->state->current_graphic == graphic);
    trace->removed = graphic;
    ++trace->remove_count;
}

static void add_graphic(void *context, const void *graphic,
                        unsigned duration, unsigned arg1, unsigned arg2) {
    Trace *trace = context;
    assert(trace->state->current_graphic == graphic);
    assert(duration == 0x3c && arg1 == 0 && arg2 == 0);
    trace->added = graphic;
    ++trace->add_count;
}

static void test_edition(AoePlayerEdition edition) {
    static int standing, walking, fighting;
    AoeUnitGraphicTemplate unit_template = {&standing, &walking};
    AoeUnitGraphicState state = {&walking, NULL, 3, 1};
    Trace trace = {.state = &state};
    AoeUnitGraphicOps ops = {&trace, remove_graphic, add_graphic};

    assert(aoe_unit_graphic_select(&state, &unit_template, &ops, &fighting,
                                   edition));
    if (edition == AOE_PLAYER_RISE_OF_ROME) {
        assert(state.current_graphic == &walking);
        assert(state.previous_graphic == NULL && state.graphic_dirty == 1);
        assert(trace.remove_count == 0 && trace.add_count == 0);
        state.animation_state = 2;
        assert(aoe_unit_graphic_select(&state, &unit_template, &ops,
                                       &fighting, edition));
    }
    assert(state.previous_graphic == &walking);
    assert(state.current_graphic == &fighting && state.graphic_dirty == 0);
    assert(trace.removed == &walking && trace.added == &fighting);
    assert(trace.remove_count == 1 && trace.add_count == 1);

    state.graphic_dirty = 1;
    assert(aoe_unit_graphic_select(&state, &unit_template, &ops, &fighting,
                                   edition));
    assert(trace.remove_count == 1 && trace.add_count == 1);
    assert(state.graphic_dirty == 1);

    assert(aoe_unit_graphic_select(&state, &unit_template, &ops, NULL,
                                   edition));
    assert(trace.removed == &fighting && trace.added == &standing);
    assert(state.previous_graphic == &fighting);
    assert(state.current_graphic == &standing && state.graphic_dirty == 0);
    assert(trace.remove_count == 2 && trace.add_count == 2);

    AoeUnitGraphicState before = state;
    assert(!aoe_unit_graphic_select(&state, &unit_template, &ops, &walking,
                                    (AoePlayerEdition)99));
    assert(state.current_graphic == before.current_graphic);
    assert(trace.remove_count == 2 && trace.add_count == 2);
}

static void test_null_walking_graphic(void) {
    static int standing;
    AoeUnitGraphicTemplate unit_template = {&standing, NULL};
    AoeUnitGraphicState state = {NULL, NULL, 3, 1};
    Trace trace = {.state = &state};
    AoeUnitGraphicOps ops = {&trace, remove_graphic, add_graphic};
    assert(aoe_unit_graphic_select(&state, &unit_template, &ops, NULL,
                                   AOE_PLAYER_RISE_OF_ROME));
    assert(state.current_graphic == &standing);
    assert(trace.removed == NULL && trace.added == &standing);
    assert(trace.remove_count == 1 && trace.add_count == 1);
}

static const void *ov_last;
static unsigned ov_prio, ov_adds, ov_removes;
static void ov_add(void *c, const void *g, unsigned p, unsigned a1, unsigned a2) {
    (void)c; assert(a1 == 0 && a2 == 0); ov_last = g; ov_prio = p; ++ov_adds;
}
static void ov_remove(void *c, const void *g) { (void)c; ov_last = g; ++ov_removes; }

static void test_overlay(void) {
    static int g;
    AoeUnitGraphicOps ops = {NULL, ov_remove, ov_add};
    assert(aoe_unit_graphic_add_overlay(&ops, &g, 7));
    assert(ov_last == &g && ov_prio == 7 && ov_adds == 1 && ov_removes == 0);
    ov_last = NULL;
    assert(aoe_unit_graphic_remove_overlay(&ops, &g));
    assert(ov_last == &g && ov_removes == 1 && ov_adds == 1);
    assert(!aoe_unit_graphic_add_overlay(NULL, &g, 1));
}

int main(void) {
    test_overlay();
    test_edition(AOE_PLAYER_BASE);
    test_edition(AOE_PLAYER_RISE_OF_ROME);
    test_null_walking_graphic();
    return 0;
}
