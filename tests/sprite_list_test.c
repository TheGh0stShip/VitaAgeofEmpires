#include "sprite_list.h"

#include <assert.h>
#include <stdlib.h>

typedef struct { const void *graphic; } Sprite;
typedef struct {
    unsigned created, destroyed, invalidated;
    const void *last_created;
    unsigned arg1, arg2;
} Trace;

static void *create_sprite(void *context, const void *graphic,
                           int32_t arg1, int32_t arg2) {
    Trace *trace = context;
    Sprite *sprite = malloc(sizeof(*sprite));
    assert(sprite);
    sprite->graphic = graphic;
    trace->last_created = graphic;
    trace->arg1 = arg1;
    trace->arg2 = arg2;
    ++trace->created;
    return sprite;
}

static const void *graphic_of(void *context, const void *sprite) {
    (void)context;
    return ((const Sprite *)sprite)->graphic;
}

static int is_animated(void *context, const void *graphic) {
    (void)context;
    return graphic != NULL && *(const int *)graphic != 0;
}

static void destroy_sprite(void *context, void *sprite) {
    Trace *trace = context;
    ++trace->destroyed;
    free(sprite);
}

static void invalidate(void *context) {
    Trace *trace = context;
    ++trace->invalidated;
}

int main(void) {
    int still = 0, animated_a = 1, animated_b = 2, animated_c = 3;
    Trace trace = {0};
    AoeSpriteList list = {
        .ops = {&trace, create_sprite, graphic_of, is_animated,
                destroy_sprite, invalidate},
        .has_redraw_target = 1,
    };
    assert(aoe_sprite_list_add(&list, NULL, 60, 0, 0));
    assert(aoe_sprite_list_count(&list) == 0);
    assert(aoe_sprite_list_add(&list, &animated_a, 60, 7, 8));
    assert(trace.created == 1 && trace.invalidated == 1);
    assert(trace.last_created == &animated_a && trace.arg1 == 7 && trace.arg2 == 8);
    assert(aoe_sprite_list_add(&list, &still, 20, 0, 0));
    assert(aoe_sprite_list_add(&list, &animated_b, 60, 0, 0));
    assert(aoe_sprite_list_add(&list, &animated_c, 10, 0, 0));
    assert(aoe_sprite_list_count(&list) == 4);
    assert(aoe_sprite_list_graphic_at(&list, 0) == &animated_c);
    assert(aoe_sprite_list_graphic_at(&list, 1) == &still);
    assert(aoe_sprite_list_graphic_at(&list, 2) == &animated_a);
    assert(aoe_sprite_list_graphic_at(&list, 3) == &animated_b);
    assert(aoe_sprite_list_priority_at(&list, 2) == 60);
    assert(trace.invalidated == 3);

    assert(aoe_sprite_list_add(&list, &animated_a, 1, 0, 0));
    assert(aoe_sprite_list_count(&list) == 4 && trace.created == 4);
    assert(trace.invalidated == 4); /* Before duplicate check. */
    assert(aoe_sprite_list_remove(&list, &animated_a));
    assert(aoe_sprite_list_count(&list) == 4 && trace.destroyed == 0);
    assert(trace.invalidated == 4); /* Still referenced. */
    assert(aoe_sprite_list_remove(&list, &animated_a));
    assert(aoe_sprite_list_count(&list) == 3 && trace.destroyed == 1);
    assert(trace.invalidated == 5);
    assert(aoe_sprite_list_remove(&list, &animated_a));
    assert(aoe_sprite_list_count(&list) == 3 && trace.invalidated == 5);

    aoe_sprite_list_clear(&list);
    assert(aoe_sprite_list_count(&list) == 0);
    assert(trace.created == 4 && trace.destroyed == 4);
    assert(trace.invalidated == 7); /* Two animated entries in clear. */
    list.has_redraw_target = 0;
    assert(aoe_sprite_list_add(&list, &animated_a, 60, 0, 0));
    aoe_sprite_list_clear(&list);
    assert(trace.created == 5 && trace.destroyed == 5);
    assert(trace.invalidated == 7);
    aoe_sprite_list_clear(&list);
    assert(trace.destroyed == 5);
    return 0;
}
