/* Patched RoR 1.0a object list 0x00450ef0..0x004516c0. */
#include "object_list.h"

#include <stdlib.h>

#include "aoe_stream.h"

void aoe_object_list_init_ror(AoeObjectListRor *l) {
    l->head = NULL;
    l->count = 0;
}

static void destroy_objects(AoeObjectListRor *l, const AoeObjectListOpsRor *ops) {
    AoeObjectNodeRor *n = l->head;
    while (n) {
        AoeObjectNodeRor *next = n->next;
        if (n->object && ops && ops->destroy) ops->destroy(ops->context, n->object);
        n = next;
    }
}

void aoe_object_list_destroy_ror(AoeObjectListRor *l, const AoeObjectListOpsRor *ops) {
    destroy_objects(l, ops);
    while (l->head) {
        AoeObjectNodeRor *next = l->head->next;
        free(l->head);
        l->head = next;
    }
    l->count = 0;
}

void aoe_object_list_clear_ror(AoeObjectListRor *l, const AoeObjectListOpsRor *ops) {
    destroy_objects(l, ops);
    l->head = NULL;
    l->count = 0;
}

int aoe_object_list_append_ror(AoeObjectListRor *l, void *object) {
    AoeObjectNodeRor *n = calloc(1, sizeof *n);
    if (!n) return 0;
    n->next = l->head;
    if (l->head) l->head->prev = n;
    n->object = object;
    l->count++;
    l->head = n;
    return 1;
}

void aoe_object_list_remove_ror(AoeObjectListRor *l, void *object, AoeObjectNodeRor *node) {
    if (!l->head) return;
    if (!node) {
        for (node = l->head; node && node->object != object; node = node->next) {
        }
        if (!node) return;
    }
    if (!node->prev) l->head = node->next;
    else node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    free(node);
    l->count--;
}

void aoe_object_list_reverse_ror(AoeObjectListRor *l) {
    AoeObjectNodeRor *done = NULL, *n;
    while ((n = l->head)) {
        l->head = n->next;
        n->next = done;
        if (done) done->prev = n;
        n->prev = NULL;
        done = n;
    }
    l->head = done;
}

void *aoe_object_list_find_ror(const AoeObjectListRor *l, const AoeObjectListOpsRor *ops,
                               int32_t id) {
    for (AoeObjectNodeRor *n = l->head; n; n = n->next)
        if (n->object && ops->id(ops->context, n->object) == id) return n->object;
    return NULL;
}

static int passes(const AoeObjectListOpsRor *ops, int flagged, void *object) {
    return object && (!flagged || ops->class_flag(ops->context, object));
}

void aoe_object_list_each_ror(const AoeObjectListRor *l, const AoeObjectListOpsRor *ops,
                              int flagged, void (*fn)(void *context, void *object),
                              void *context) {
    for (AoeObjectNodeRor *n = l->head; n; n = n->next)
        if (passes(ops, flagged, n->object)) fn(context, n->object);
}

int aoe_object_list_sorted_ror(const AoeObjectListRor *l, const AoeObjectListOpsRor *ops,
                               AoeObjectNodeRor **out) {
    *out = NULL;
    for (AoeObjectNodeRor *src = l->head; src; src = src->next) {
        uint8_t key = ops->sort_key(ops->context, src->object);
        int16_t order = ops->sort_order(ops->context, src->object);
        AoeObjectNodeRor **slot = out, *at, *n;
        for (at = *out; at; at = at->next) {
            uint8_t k = ops->sort_key(ops->context, at->object);
            if (k < key || (k == key && order < ops->sort_order(ops->context, at->object)))
                break;
            slot = &at->next;
        }
        n = calloc(1, sizeof *n);
        if (!n) {
            aoe_object_list_free_nodes_ror(*out);
            *out = NULL;
            return 0;
        }
        n->next = *slot;
        if (*slot) {
            n->prev = (*slot)->prev;
            (*slot)->prev = n;
        }
        n->object = src->object;
        *slot = n;
    }
    return 1;
}

void aoe_object_list_free_nodes_ror(AoeObjectNodeRor *head) {
    while (head) {
        AoeObjectNodeRor *next = head->next;
        free(head);
        head = next;
    }
}

void aoe_object_list_each_sorted_ror(const AoeObjectListRor *l, const AoeObjectListOpsRor *ops,
                                     int flagged, void (*fn)(void *context, void *object),
                                     void *context) {
    AoeObjectNodeRor *sorted;
    if (!aoe_object_list_sorted_ror(l, ops, &sorted) || !sorted) return;
    for (AoeObjectNodeRor *n = sorted; n; n = n->next)
        if (passes(ops, flagged, n->object)) fn(context, n->object);
    aoe_object_list_free_nodes_ror(sorted);
}

void aoe_object_list_update_ror(AoeObjectListRor *l, const AoeObjectListOpsRor *ops) {
    int done = 0;
    AoeObjectNodeRor *n = l->head;
    while (n) {
        AoeObjectNodeRor *next = n->next;
        if (n->object) {
            if (ops->needs_update(ops->context, n->object)) ops->update(ops->context, n->object);
            if (++done > 9) {
                done = 0;
                if (ops->pump_enabled && ops->pump) ops->pump(ops->context);
            }
        }
        n = next;
    }
}

void aoe_object_list_save_ror(const AoeObjectListRor *l, const AoeObjectListOpsRor *ops,
                              AoeOutcomeStream *s) {
    for (AoeObjectNodeRor *n = l->head; n; n = n->next)
        if (n->object) ops->save(ops->context, n->object, s);
    aoe_stream_put_u8(s, 0);
}

void aoe_object_list_load_ror(AoeObjectListRor *l, const AoeObjectListOpsRor *ops,
                              AoeOutcomeStream *s, int32_t arg) {
    for (;;) {
        uint8_t type = aoe_stream_u8(s);
        if (!type || s->failed) break;
        ops->load(ops->context, l, type, s, arg);
    }
    aoe_object_list_reverse_ror(l);
}

void aoe_object_list_draw_ror(const AoeObjectListRor *l, const AoeObjectListDrawOpsRor *ops,
                              int32_t a, int32_t b, int32_t c, int fogged) {
    for (AoeObjectNodeRor *n = l->head; n; n = n->next) {
        void *o = n->object;
        int32_t colour;
        int skip = 0;
        if (!o) continue;
        colour = ops->owner_colour(ops->context, o);
        switch (ops->class_flag(ops->context, o)) {
        case 0:
            skip = fogged;
            break;
        case 2:
            skip = fogged && ops->state(ops->context, o) > 2;
            break;
        case 3:
            if (!fogged || !ops->viewer_sees_owner(ops->context, o)) skip = 1;
            else colour = ops->own_colour(ops->context, o);
            break;
        case 4:
            if (fogged && l->count > 1)
                for (AoeObjectNodeRor *m = l->head; m; m = m->next)
                    if (ops->kind(ops->context, m->object) == 0x19) skip = 1;
            break;
        default:
            break;
        }
        if (!skip) ops->draw(ops->context, o, a, b, c, colour);
    }
}

void *aoe_object_lists_find_any_ror(const AoeObjectListRor *const *lists,
                                    int16_t player_count, int16_t owner_index,
                                    const AoeObjectListOpsRor *ops,
                                    int32_t id) {
    void *found;
    int k;
    uint8_t p;
    if (!lists || id < 0 || owner_index < 0 || owner_index >= player_count)
        return NULL;
    for (k = 0; k < 3; ++k)
        if (lists[owner_index * 3 + k] &&
            (found = aoe_object_list_find_ror(lists[owner_index * 3 + k], ops, id)))
            return found;
    for (p = 0; (int16_t)p < player_count; ++p) {
        if ((int16_t)p == owner_index) continue;
        for (k = 0; k < 3; ++k)
            if (lists[p * 3 + k] &&
                (found = aoe_object_list_find_ror(lists[p * 3 + k], ops, id)))
                return found;
    }
    return NULL;
}
