#include "sprite_list.h"

#include <stdlib.h>

struct AoeSpriteNode {
    void *sprite;
    uint8_t priority;
    uint8_t extra_references;
    struct AoeSpriteNode *next;
};

static int valid(const AoeSpriteList *list) {
    return list && list->ops.create && list->ops.graphic_of &&
           list->ops.is_animated && list->ops.destroy;
}

static void invalidate_if_needed(AoeSpriteList *list, int animated) {
    if (animated && list->has_redraw_target && list->ops.invalidate)
        list->ops.invalidate(list->ops.context);
}

int aoe_sprite_list_add(AoeSpriteList *list, const void *graphic,
                        uint8_t priority, int32_t arg1, int32_t arg2) {
    if (!valid(list))
        return 0;
    if (!graphic)
        return 1;
    /* The original does this before duplicate lookup. */
    invalidate_if_needed(list,
                         list->ops.is_animated(list->ops.context, graphic));
    for (AoeSpriteNode *node = list->head; node; node = node->next) {
        if (list->ops.graphic_of(list->ops.context, node->sprite) == graphic) {
            node->extra_references = (uint8_t)(node->extra_references + 1);
            return 1;
        }
    }
    AoeSpriteNode *node = calloc(1, sizeof(*node));
    if (!node)
        return 0;
    node->sprite = list->ops.create(list->ops.context, graphic, arg1, arg2);
    if (!node->sprite) {
        free(node);
        return 0;
    }
    node->priority = priority;
    AoeSpriteNode **slot = &list->head;
    while (*slot && (*slot)->priority <= priority)
        slot = &(*slot)->next;
    node->next = *slot;
    *slot = node;
    return 1;
}

int aoe_sprite_list_remove(AoeSpriteList *list, const void *graphic) {
    if (!valid(list))
        return 0;
    AoeSpriteNode **slot = &list->head;
    while (*slot && list->ops.graphic_of(list->ops.context,
                                         (*slot)->sprite) != graphic)
        slot = &(*slot)->next;
    if (!*slot)
        return 1;
    AoeSpriteNode *node = *slot;
    if (node->extra_references) {
        --node->extra_references;
        return 1;
    }
    *slot = node->next;
    invalidate_if_needed(list, list->ops.is_animated(
                                  list->ops.context,
                                  list->ops.graphic_of(list->ops.context,
                                                       node->sprite)));
    list->ops.destroy(list->ops.context, node->sprite);
    free(node);
    return 1;
}

void aoe_sprite_list_clear(AoeSpriteList *list) {
    if (!valid(list))
        return;
    AoeSpriteNode *node = list->head;
    while (node) {
        AoeSpriteNode *next = node->next;
        invalidate_if_needed(list, list->ops.is_animated(
                                      list->ops.context,
                                      list->ops.graphic_of(list->ops.context,
                                                           node->sprite)));
        list->ops.destroy(list->ops.context, node->sprite);
        free(node);
        node = next;
    }
    list->head = NULL;
}

int aoe_sprite_list_tick(AoeSpriteList *list, float time_delta, float speed) {
    if (!valid(list) || !list->ops.tick)
        return 0;
    for (AoeSpriteNode *node = list->head; node; node = node->next)
        if (!list->ops.tick(list->ops.context, node->sprite,
                            time_delta, speed))
            return 0;
    return 1;
}

int aoe_sprite_list_append_loaded(AoeSpriteList *list, void *sprite) {
    AoeSpriteNode **slot, *node;
    if (!list || !sprite) return 0;
    node = calloc(1, sizeof(*node));
    if (!node) return 0;
    node->sprite = sprite;
    for (slot = &list->head; *slot; slot = &(*slot)->next) {}
    *slot = node;
    return 1;
}

size_t aoe_sprite_list_count(const AoeSpriteList *list) {
    size_t count = 0;
    if (list)
        for (const AoeSpriteNode *node = list->head; node; node = node->next)
            ++count;
    return count;
}

const void *aoe_sprite_list_graphic_at(const AoeSpriteList *list, size_t index) {
    if (!valid(list))
        return NULL;
    const AoeSpriteNode *node = list->head;
    while (node && index--)
        node = node->next;
    return node ? list->ops.graphic_of(list->ops.context, node->sprite) : NULL;
}

const void *aoe_sprite_list_sprite_at(const AoeSpriteList *list, size_t index) {
    if (!list)
        return NULL;
    const AoeSpriteNode *node = list->head;
    while (node && index--)
        node = node->next;
    return node ? node->sprite : NULL;
}

uint8_t aoe_sprite_list_priority_at(const AoeSpriteList *list, size_t index) {
    if (!list)
        return 0;
    const AoeSpriteNode *node = list->head;
    while (node && index--)
        node = node->next;
    return node ? node->priority : 0;
}

void aoe_sprite_list_init_ror(AoeSpriteList *list, const AoeSpriteListOps *ops, void *redraw_target) {
    list->head = NULL;
    list->ops = *ops;
    list->has_redraw_target = redraw_target != NULL;
}

void aoe_sprite_list_each_ror(const AoeSpriteList *list, void (*fn)(void *context, void *sprite),
                              void *context) {
    const AoeSpriteNode *n;
    for (n = list->head; n; n = n->next) fn(context, n->sprite);
}

void *aoe_sprite_list_first_ror(const AoeSpriteList *list, const void *graphic) {
    const AoeSpriteNode *n;
    for (n = list->head; n; n = n->next)
        if (list->ops.graphic_of(list->ops.context, n->sprite) == graphic) return n->sprite;
    return NULL;
}

void aoe_sprite_list_each_of_ror(const AoeSpriteList *list, const void *graphic,
                                 void (*fn)(void *context, void *sprite), void *context) {
    const AoeSpriteNode *n;
    for (n = list->head; n; n = n->next)
        if (list->ops.graphic_of(list->ops.context, n->sprite) == graphic) fn(context, n->sprite);
}

int aoe_graphic_on_layer_zero_ror(const DatGraphic *graphic) {
    uint16_t i;
    if (graphic->delta_count == 0 || !graphic->deltas) return graphic->layer == 0;
    for (i = 0; i < graphic->delta_count; ++i)
        if (graphic->deltas[i].graphic && graphic->deltas[i].graphic->layer == 0) return 1;
    return 0;
}

void aoe_sprite_list_destroy_ror(AoeSpriteList *list) {
    aoe_sprite_list_clear(list);
}

AoeSpriteList *aoe_sprite_list_delete_ror(AoeSpriteList *list, uint8_t flags) {
    aoe_sprite_list_destroy_ror(list);
    if (list && (flags & 1)) free(list);
    return list;
}

int aoe_live_unit_sprite_add_ror(AoeSpriteList *unit_sprites, const void *graphic,
                                 uint8_t priority) {
    return aoe_sprite_list_add(unit_sprites, graphic, priority, 0, 0);
}

int aoe_live_unit_sprite_remove_ror(AoeSpriteList *unit_sprites, const void *graphic) {
    return aoe_sprite_list_remove(unit_sprites, graphic);
}
