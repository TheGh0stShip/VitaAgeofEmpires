#include "scenario_editor_runtime.h"

#include <assert.h>

int main(void) {
    DatGraphic children[2] = {
        {.present = 1, .layer = 7},
        {.present = 1, .layer = 0},
    };
    DatGraphicDelta deltas[2] = {
        {.graphic = &children[0]},
        {.graphic = &children[1]},
    };
    DatGraphic parent = {
        .present = 1, .layer = 9, .delta_count = 2, .deltas = deltas,
        .frame_count = 1, .angle_count = 1,
    };
    DatGraphic plain = {
        .present = 1, .layer = 4, .frame_count = 1, .angle_count = 1,
    };
    AoeRng rng = {1};
    AoeLiveUnit unit = {0};

    assert(aoe_scenario_editor_graphic_layer_ror(&plain) == 4);
    assert(aoe_scenario_editor_graphic_layer_ror(&parent) == 0);
    children[1].layer = 12;
    assert(aoe_scenario_editor_graphic_layer_ror(&parent) == 7);

    assert(aoe_sprite_runtime_init(&unit.sprites, &rng, 0, 0, 0));
    unit.constructed = 1;
    /* 0x004159d0 initializes the minimum to 20 for an empty list. */
    assert(aoe_scenario_editor_live_layer_ror(&unit) == 20);
    assert(aoe_sprite_runtime_add(&unit.sprites, &plain, 1, 0, 0));
    assert(aoe_sprite_runtime_add(&unit.sprites, &parent, 2, 0, 0));
    assert(aoe_scenario_editor_live_layer_ror(&unit) == 4);
    children[0].layer = 0;
    assert(aoe_scenario_editor_live_layer_ror(&unit) == 0);
    aoe_sprite_runtime_clear(&unit.sprites);
    return 0;
}
