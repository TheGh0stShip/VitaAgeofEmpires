#include "sprite_list.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

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

static void count_sprite(void *c, void *s) {
    (void)s;
    ++*(int *)c;
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

    {
        int seen = 0;
        AoeSpriteList copy;
        aoe_sprite_list_each_ror(&list, count_sprite, &seen);
        assert(seen == 3);
        seen = 0;
        aoe_sprite_list_each_of_ror(&list, &animated_b, count_sprite, &seen);
        assert(seen == 1);
        assert(aoe_sprite_list_first_ror(&list, &animated_b) == aoe_sprite_list_sprite_at(&list, 2));
        assert(aoe_sprite_list_first_ror(&list, &animated_a) == NULL);
        aoe_sprite_list_init_ror(&copy, &list.ops, &seen);
        assert(copy.head == NULL && copy.has_redraw_target && aoe_sprite_list_count(&copy) == 0);
    }
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
    {
        DatGraphic g0, g1, parent;
        DatGraphicDelta deltas[2];
        memset(&g0, 0, sizeof g0);
        memset(&g1, 0, sizeof g1);
        memset(&parent, 0, sizeof parent);
        memset(deltas, 0, sizeof deltas);
        g1.layer = 5;
        parent.layer = 5;
        assert(aoe_graphic_on_layer_zero_ror(&g0) && !aoe_graphic_on_layer_zero_ror(&g1));
        deltas[0].graphic = &g1;
        deltas[1].graphic = &g1;
        parent.deltas = deltas;
        parent.delta_count = 2;
        assert(!aoe_graphic_on_layer_zero_ror(&parent));
        deltas[1].graphic = &g0;
        assert(aoe_graphic_on_layer_zero_ror(&parent));
    }
    /* 0x004153b0: the destructor clears the list. */
    assert(aoe_sprite_list_add(&list, &animated_a, 1, 0, 0));
    aoe_sprite_list_destroy_ror(&list);
    assert(aoe_sprite_list_count(&list) == 0);
    return 0;
}
