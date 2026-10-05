#include "sprite_runtime.h"

#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

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

static void put_i16(AoeOutcomeStream *s, int16_t v) {
    uint8_t b[2] = {(uint8_t)((uint16_t)v & 0xff), (uint8_t)((uint16_t)v >> 8)};
    aoe_stream_put(s, b, 2);
}

static int16_t get_i16(AoeOutcomeStream *s) {
    const uint8_t *p = aoe_stream_take(s, 2);
    return p ? (int16_t)(uint16_t)(p[0] | (p[1] << 8)) : 0;
}

int aoe_sprite_runtime_save_ror(const AoeSpriteRuntime *runtime,
                                int16_t (*graphic_id)(void *context,
                                                      const DatGraphic *),
                                void *context, AoeOutcomeStream *out) {
    size_t i, n;
    if (!runtime || !graphic_id || !out) return 0;
    n = aoe_sprite_list_count(&runtime->list);
    for (i = 0; i < n; ++i) {
        const AoeSpriteInstance *sp = aoe_sprite_list_sprite_at(&runtime->list, i);
        aoe_stream_put_u8(out, sp->kind);
        put_i16(out, graphic_id(context, sp->graphic));
        aoe_stream_put_i32(out, sp->offset_x);
        aoe_stream_put_i32(out, sp->offset_y);
        if (sp->kind == 2) {
            const AoeSpriteAnimationState *a = &sp->animation;
            put_i16(out, a->frame);
            aoe_stream_put_f32(out, a->period);
            aoe_stream_put_f32(out, a->elapsed);
            put_i16(out, a->previous_frame);
            aoe_stream_put_u8(out, a->frame_changed);
            aoe_stream_put_u8(out, a->looped);
            aoe_stream_put_u8(out, a->run_state);
        }
    }
    aoe_stream_put_u8(out, 0);
    return !out->failed;
}

int aoe_sprite_runtime_load_ror(AoeSpriteRuntime *runtime,
                                const DatGraphic *(*graphic)(void *context,
                                                             int16_t id),
                                void *context, AoeOutcomeStream *in) {
    if (!runtime || !graphic || !in) return 0;
    for (;;) {
        uint8_t kind = aoe_stream_u8(in);
        AoeSpriteInstance *sp;
        int16_t id;
        if (in->failed) return 0;
        if (kind == 0) return 1;
        if (kind != 1 && kind != 2) continue;
        sp = calloc(1, sizeof(*sp));
        if (!sp) return 0;
        sp->kind = kind;
        id = get_i16(in);
        sp->graphic = id < 0 ? NULL : graphic(context, id);
        sp->offset_x = aoe_stream_i32(in);
        sp->offset_y = aoe_stream_i32(in);
        if (kind == 2) {
            AoeSpriteAnimationState *a = &sp->animation;
            a->frame = get_i16(in);
            a->period = aoe_stream_f32(in);
            a->elapsed = aoe_stream_f32(in);
            a->previous_frame = get_i16(in);
            a->frame_changed = aoe_stream_u8(in);
            a->looped = aoe_stream_u8(in);
            a->run_state = aoe_stream_u8(in);
        }
        if (in->failed || !sp->graphic ||
            !aoe_sprite_list_append_loaded(&runtime->list, sp)) {
            free(sp);
            return 0;
        }
    }
}
