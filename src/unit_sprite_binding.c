#include "unit_sprite_binding.h"

int aoe_unit_graphic_select_runtime(AoeUnitGraphicState *state,
                                    const AoeUnitGraphicTemplate *unit_template,
                                    AoeSpriteRuntime *runtime,
                                    const DatGraphic *requested_graphic,
                                    AoePlayerEdition edition) {
    if (!state || !unit_template || !runtime ||
        (edition != AOE_PLAYER_BASE && edition != AOE_PLAYER_RISE_OF_ROME))
        return 0;
    if (!requested_graphic)
        requested_graphic = unit_template->standing_graphic;
    const DatGraphic *old = state->current_graphic;
    if (old == requested_graphic)
        return 1;
    if (edition == AOE_PLAYER_RISE_OF_ROME && state->animation_state > 2 &&
        old == unit_template->walking_graphic && unit_template->walking_graphic)
        return 1;

    if (requested_graphic &&
        !aoe_sprite_runtime_add(runtime, requested_graphic, 0x3c, 0, 0))
        return 0;
    if (!aoe_sprite_runtime_remove(runtime, old)) {
        if (requested_graphic)
            aoe_sprite_runtime_remove(runtime, requested_graphic);
        return 0;
    }
    state->previous_graphic = old;
    state->current_graphic = requested_graphic;
    state->graphic_dirty = 0;
    return 1;
}
