#include "unit_graphic.h"

int aoe_unit_graphic_select(AoeUnitGraphicState *state,
                            const AoeUnitGraphicTemplate *unit_template,
                            const AoeUnitGraphicOps *ops,
                            const void *requested_graphic,
                            AoePlayerEdition edition) {
    if (!state || !unit_template || !ops || !ops->remove || !ops->add ||
        (edition != AOE_PLAYER_BASE && edition != AOE_PLAYER_RISE_OF_ROME))
        return 0;

    if (!requested_graphic)
        requested_graphic = unit_template->standing_graphic;
    const void *old = state->current_graphic;
    if (old == requested_graphic)
        return 1;
    if (edition == AOE_PLAYER_RISE_OF_ROME && state->animation_state > 2 &&
        old == unit_template->walking_graphic && unit_template->walking_graphic)
        return 1;

    state->previous_graphic = old;
    ops->remove(ops->context, old);
    state->current_graphic = requested_graphic;
    ops->add(ops->context, requested_graphic, 0x3c, 0, 0);
    state->graphic_dirty = 0;
    return 1;
}
