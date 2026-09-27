#include "sprite_runtime.h"

#include <stdlib.h>
#include <string.h>

static void *create_instance(void *context, const void *graphic,
                             int32_t arg1, int32_t arg2) {
    AoeSpriteRuntime *runtime = context;
    AoeSpriteInstance *instance = malloc(sizeof(*instance));
    if (!instance)
        return NULL;
    if (!aoe_sprite_instance_init(instance, graphic, runtime->rng,
                                  arg1, arg2)) {
        free(instance);
        return NULL;
    }
    return instance;
}

static const void *graphic_of(void *context, const void *sprite) {
    (void)context;
    return ((const AoeSpriteInstance *)sprite)->graphic;
}

static int needs_redraw(void *context, const void *graphic) {
    AoeSpriteRuntime *runtime = context;
    return runtime->needs_redraw &&
        runtime->needs_redraw(runtime->redraw_context, graphic);
}

static void destroy_instance(void *context, void *sprite) {
    (void)context;
    free(sprite);
}

static void invalidate(void *context) {
    AoeSpriteRuntime *runtime = context;
    if (runtime->invalidate)
        runtime->invalidate(runtime->redraw_context);
}

static int tick_instance(void *context, void *sprite,
                         float time_delta, float speed) {
    (void)context;
    return aoe_sprite_instance_tick(sprite, time_delta, speed);
}

int aoe_sprite_runtime_init(AoeSpriteRuntime *runtime, AoeRng *rng,
                            void *redraw_context,
                            int (*redraw_predicate)(void *, const DatGraphic *),
                            void (*invalidate_callback)(void *)) {
    if (!runtime || !rng || (invalidate_callback && !redraw_predicate))
        return 0;
    memset(runtime, 0, sizeof(*runtime));
    runtime->rng = rng;
    runtime->redraw_context = redraw_context;
    runtime->needs_redraw = redraw_predicate;
    runtime->invalidate = invalidate_callback;
    runtime->list.ops = (AoeSpriteListOps){
        runtime, create_instance, graphic_of, needs_redraw,
        destroy_instance, invalidate, tick_instance,
    };
    runtime->list.has_redraw_target = invalidate_callback != NULL;
    return 1;
}

int aoe_sprite_runtime_add(AoeSpriteRuntime *runtime,
                           const DatGraphic *graphic, uint8_t priority,
                           int32_t offset_x, int32_t offset_y) {
    return runtime && aoe_sprite_list_add(&runtime->list, graphic, priority,
                                           offset_x, offset_y);
}

int aoe_sprite_runtime_remove(AoeSpriteRuntime *runtime,
                              const DatGraphic *graphic) {
    return runtime && aoe_sprite_list_remove(&runtime->list, graphic);
}

int aoe_sprite_runtime_tick(AoeSpriteRuntime *runtime,
                            float time_delta, float speed) {
    return runtime && aoe_sprite_list_tick(&runtime->list,
                                           time_delta, speed);
}

void aoe_sprite_runtime_clear(AoeSpriteRuntime *runtime) {
    if (runtime)
        aoe_sprite_list_clear(&runtime->list);
}

const AoeSpriteInstance *aoe_sprite_runtime_at(const AoeSpriteRuntime *runtime,
                                               size_t index) {
    return runtime ? aoe_sprite_list_sprite_at(&runtime->list, index) : NULL;
}
