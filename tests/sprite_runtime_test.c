#include "sprite_runtime.h"

#include <assert.h>

typedef struct { unsigned invalidations; } Redraw;

static int needs_redraw(void *context, const DatGraphic *graphic) {
    (void)context;
    return graphic->layer == 1;
}

static void invalidate(void *context) {
    ++((Redraw *)context)->invalidations;
}

int main(void) {
    DatGraphic standing = {.present = 1, .frame_count = 1,
                           .layer = 0, .sequence_type = 0};
    DatGraphic walking = {.present = 1, .frame_count = 4,
                          .layer = 1, .sequence_type = 1,
                          .frame_duration = 0.5f};
    AoeRng rng;
    aoe_rng_seed(&rng, 1);
    Redraw redraw = {0};
    AoeSpriteRuntime runtime;
    assert(aoe_sprite_runtime_init(&runtime, &rng, &redraw,
                                    needs_redraw, invalidate));

    assert(aoe_sprite_runtime_add(&runtime, &walking, 60, 7, -3));
    assert(aoe_sprite_runtime_add(&runtime, &standing, 10, 2, 5));
    assert(aoe_sprite_list_count(&runtime.list) == 2);
    assert(redraw.invalidations == 1);
    const AoeSpriteInstance *first = aoe_sprite_runtime_at(&runtime, 0);
    const AoeSpriteInstance *second = aoe_sprite_runtime_at(&runtime, 1);
    assert(first && first->graphic == &standing && first->kind == 1);
    assert(first->offset_x == 2 && first->offset_y == 5);
    assert(second && second->graphic == &walking && second->kind == 2);
    assert(second->offset_x == 7 && second->offset_y == -3);

    assert(aoe_sprite_runtime_tick(&runtime, 0.5f, 1.0f));
    assert(second->animation.frame == 1);
    assert(aoe_sprite_runtime_add(&runtime, &walking, 1, 0, 0));
    assert(aoe_sprite_list_count(&runtime.list) == 2);
    assert(redraw.invalidations == 2);
    assert(aoe_sprite_runtime_remove(&runtime, &walking));
    assert(aoe_sprite_list_count(&runtime.list) == 2);
    assert(redraw.invalidations == 2);
    assert(aoe_sprite_runtime_remove(&runtime, &walking));
    assert(aoe_sprite_list_count(&runtime.list) == 1);
    assert(redraw.invalidations == 3);
    aoe_sprite_runtime_clear(&runtime);
    assert(aoe_sprite_list_count(&runtime.list) == 0);
    assert(redraw.invalidations == 3);
    return 0;
}
