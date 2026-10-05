#ifndef VITA_AOE_SPRITE_RUNTIME_H
#define VITA_AOE_SPRITE_RUNTIME_H

#include "sprite_instance.h"
#include "sprite_list.h"
#include "player_outcome.h" /* AoeOutcomeStream */

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

/* Patched RoR 1.0a sprite-list save 0x00415540 and load 0x004153c0 /
   0x00415420. Per sprite: u8 kind, then (0x00415ae0) i16 graphic id, i32
   offset x, y; animated sprites (0x00415e40) add i16 frame, f32 period,
   f32 elapsed, i16 previous frame, u8 frame changed, u8 looped, u8 run
   state. A 0 kind ends the list. The loader (0x00415a40 / 0x00415d80)
   restores the state verbatim; a negative id gives no graphic, which the
   portable loader rejects. Unknown kind bytes are skipped as in the
   original. Priorities and extra references are not saved. */
int aoe_sprite_runtime_save_ror(const AoeSpriteRuntime *runtime,
                                int16_t (*graphic_id)(void *context,
                                                      const DatGraphic *),
                                void *context, AoeOutcomeStream *out);
int aoe_sprite_runtime_load_ror(AoeSpriteRuntime *runtime,
                                const DatGraphic *(*graphic)(void *context,
                                                             int16_t id),
                                void *context, AoeOutcomeStream *in);

#endif
