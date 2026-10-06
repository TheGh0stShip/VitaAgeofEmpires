#include "unit_sprite_binding.h"

#include <assert.h>

static void run_edition(AoePlayerEdition edition) {
    DatGraphic standing = {.present = 1, .frame_count = 1};
    DatGraphic walking = {.present = 1, .frame_count = 4,
                          .sequence_flags = 1, .frame_duration = 0.5f};
    DatGraphic invalid = {.present = 0};
    AoeRng rng;
    aoe_rng_seed(&rng, 1);
    AoeSpriteRuntime runtime;
    assert(aoe_sprite_runtime_init(&runtime, &rng, NULL, NULL, NULL));
    assert(aoe_sprite_runtime_add(&runtime, &standing, 0x3c, 0, 0));
    AoeUnitGraphicTemplate unit_template = {&standing, &walking};
    AoeUnitGraphicState state = {&standing, NULL, 3, 1};

    assert(!aoe_unit_graphic_select_runtime(&state, &unit_template,
                                            &runtime, &invalid, edition));
    assert(state.current_graphic == &standing && state.graphic_dirty == 1);
    assert(aoe_sprite_runtime_at(&runtime, 0)->graphic == &standing);

    assert(aoe_unit_graphic_select_runtime(&state, &unit_template,
                                           &runtime, &walking, edition));
    assert(state.previous_graphic == &standing);
    assert(state.current_graphic == &walking && state.graphic_dirty == 0);
    assert(aoe_sprite_list_count(&runtime.list) == 1);
    assert(aoe_sprite_runtime_at(&runtime, 0)->graphic == &walking);
    assert(aoe_sprite_runtime_at(&runtime, 0)->kind == 2);
    assert(aoe_sprite_runtime_tick(&runtime, 0.5f, 1.0f));
    assert(aoe_sprite_runtime_at(&runtime, 0)->animation.frame == 1);

    state.graphic_dirty = 1;
    assert(aoe_unit_graphic_select_runtime(&state, &unit_template,
                                           &runtime, NULL, edition));
    if (edition == AOE_PLAYER_RISE_OF_ROME) {
        assert(state.current_graphic == &walking && state.graphic_dirty == 1);
        assert(aoe_sprite_runtime_at(&runtime, 0)->graphic == &walking);
        state.animation_state = 2;
        assert(aoe_unit_graphic_select_runtime(&state, &unit_template,
                                               &runtime, NULL, edition));
    }
    assert(state.current_graphic == &standing && state.graphic_dirty == 0);
    assert(state.previous_graphic == &walking);
    assert(aoe_sprite_runtime_at(&runtime, 0)->graphic == &standing);
    assert(aoe_sprite_list_count(&runtime.list) == 1);
    aoe_sprite_runtime_clear(&runtime);
}

int main(void) {
    run_edition(AOE_PLAYER_BASE);
    run_edition(AOE_PLAYER_RISE_OF_ROME);
    return 0;
}
