#ifndef VITA_AOE_SPRITE_RUNTIME_H
#define VITA_AOE_SPRITE_RUNTIME_H

#include "sprite_instance.h"
#include "sprite_list.h"

typedef struct {
    AoeSpriteList list;
    AoeRng *rng; /* Shared original game RNG, not owned. */
    void *redraw_context;
    int (*needs_redraw)(void *context, const DatGraphic *graphic);
    void (*invalidate)(void *context);
} AoeSpriteRuntime;

/* Binds the original sprite-list operations to DAT-backed static/animated
   instances. The renderer supplies the original graphic visibility/redraw
   decision; pass null callbacks if no redraw target is attached yet. */
int aoe_sprite_runtime_init(AoeSpriteRuntime *runtime, AoeRng *rng,
                            void *redraw_context,
                            int (*needs_redraw)(void *, const DatGraphic *),
                            void (*invalidate)(void *));
int aoe_sprite_runtime_add(AoeSpriteRuntime *runtime,
                           const DatGraphic *graphic, uint8_t priority,
                           int32_t offset_x, int32_t offset_y);
int aoe_sprite_runtime_remove(AoeSpriteRuntime *runtime,
                              const DatGraphic *graphic);
int aoe_sprite_runtime_tick(AoeSpriteRuntime *runtime,
                            float time_delta, float speed);
void aoe_sprite_runtime_clear(AoeSpriteRuntime *runtime);
const AoeSpriteInstance *aoe_sprite_runtime_at(const AoeSpriteRuntime *runtime,
                                               size_t index);

#endif
