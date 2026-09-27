#include "object_update_list.h"

#include <limits.h>
#include <stdlib.h>

int aoe_object_update_list_attach(AoeObjectUpdateList *list, void *object,
                                  AoeObjectUpdateNode **attached) {
    AoeObjectUpdateNode *node;
    if (!list || !attached || list->count == UINT16_MAX) return 0;
    node = calloc(1, sizeof(*node));
    if (!node) return 0;
    node->object = object;
    node->next = list->head;
    if (list->head) list->head->prev = node;
    list->head = node;
    list->count++;
    *attached = node;
    return 1;
}

int aoe_object_update_list_detach(AoeObjectUpdateList *list, void *object,
                                  AoeObjectUpdateNode *node) {
    AoeObjectUpdateNode *cursor;
    if (!list) return 0;
    for (cursor = list->head; cursor; cursor = cursor->next)
        if (node ? cursor == node : cursor->object == object) break;
    if (!cursor) return 0;
    if (cursor->prev) cursor->prev->next = cursor->next;
    else list->head = cursor->next;
    if (cursor->next) cursor->next->prev = cursor->prev;
    free(cursor);
    list->count--;
    return 1;
}

void aoe_object_update_list_clear(AoeObjectUpdateList *list) {
    AoeObjectUpdateNode *node;
    if (!list) return;
    node = list->head;
    while (node) {
        AoeObjectUpdateNode *next = node->next;
        free(node);
        node = next;
    }
    list->head = 0;
    list->count = 0;
}

int aoe_object_update_list(AoeObjectUpdateNode *head,
                           const AoeObjectUpdateOps *ops) {
    unsigned nonnull_since_pump = 0;
    if (!ops || !ops->predicate || !ops->action ||
        !ops->cooperative_pending || !ops->cooperative_pump) return 0;
    for (AoeObjectUpdateNode *node = head; node;) {
        AoeObjectUpdateNode *next = node->next;
        if (node->object) {
            if (ops->predicate(ops->user, node->object))
                ops->action(ops->user, node->object);
            nonnull_since_pump++;
            if (nonnull_since_pump == 10) {
                nonnull_since_pump = 0;
                if (ops->cooperative_pending(ops->user))
                    ops->cooperative_pump(ops->user);
            }
        }
        node = next;
    }
    return 1;
}
