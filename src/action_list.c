#include "action_list.h"
#include <stdlib.h>

int aoe_action_list_valid(const AoeActionList *list) {
    return list && list->owner_state && list->ops.destroy && list->ops.set_state;
}

static int insert(AoeActionList *list, void *action, int append) {
    AoeActionNode *node;
    if (!aoe_action_list_valid(list)) return 0;
    if (*list->owner_state > 2) {
        if (action) list->ops.destroy(list->ops.context, action);
        return 1;
    }
    if (!action) return 0;
    node = calloc(1, sizeof(*node));
    if (!node) return 0;
    node->action = action;
    if (append) {
        AoeActionNode **slot = &list->head;
        while (*slot) slot = &(*slot)->next;
        *slot = node;
    } else {
        node->next = list->head;
        list->head = node;
    }
    list->ops.set_state(list->ops.context, action, 1);
    return 1;
}

int aoe_action_list_push(AoeActionList *list, void *action) {
    return insert(list, action, 0);
}
int aoe_action_list_append(AoeActionList *list, void *action) {
    return insert(list, action, 1);
}
int aoe_action_list_pop(AoeActionList *list) {
    AoeActionNode *node;
    if (!aoe_action_list_valid(list)) return 0;
    node = list->head;
    if (!node) return 1;
    list->head = node->next;
    if (node->action) list->ops.destroy(list->ops.context, node->action);
    free(node);
    if (list->head && *list->owner_state < 3)
        list->ops.set_state(list->ops.context, list->head->action, 0);
    return 1;
}
int aoe_action_list_clear(AoeActionList *list) {
    if (!aoe_action_list_valid(list)) return 0;
    while (list->head) aoe_action_list_pop(list);
    return 1;
}

int aoe_action_list_update_ror(AoeActionList *list, const AoeActionTickOps *ops,
                                uint8_t *result) {
    uint8_t saved;
    if (!aoe_action_list_valid(list) || !ops || !ops->update_head ||
        !ops->update_waiting || !result) return 0;
    if (!list->head) { *result=1; return 1; }
    saved=ops->update_head(ops->context,list->head->action);
    if (list->head) {
        AoeActionNode *node=list->head->next;
        while (node) {
            ops->update_waiting(ops->context,node->action);
            node=node->next;
        }
        if (saved==1) aoe_action_list_pop(list);
    }
    *result=saved;
    return 1;
}

void *aoe_action_list_head_ror(const AoeActionList *list) {
    return list->head ? list->head->action : NULL;
}

void aoe_action_list_each_ror(const AoeActionList *list, void (*fn)(void *context, void *action),
                              void *context) {
    const AoeActionNode *n;
    for (n = list->head; n; n = n->next) fn(context, n->action);
}

int aoe_unit_push_action_ror(AoeActionList *list, void *action) {
    return action ? aoe_action_list_push(list, action) : 1;
}

int aoe_unit_append_action_ror(AoeActionList *list, void *action) {
    return action ? aoe_action_list_append(list, action) : 1;
}

void aoe_unit_cleanup_actions_ror(AoeActionList *list, void (*cleanup)(void *context),
                                  void *context) {
    if (list) aoe_action_list_clear(list);
    cleanup(context);
}

AoeActionList *aoe_action_list_new_ror(const uint8_t *owner_state, const AoeActionListOps *ops) {
    AoeActionList *l = calloc(1, sizeof *l);
    if (!l) return NULL;
    l->owner_state = owner_state;
    l->ops = *ops;
    return l;
}

int32_t aoe_unit_action_target_id_ror(const AoeActionList *list,
                                      int32_t (*target_id)(void *context, void *action),
                                      void *context) {
    void *head = aoe_action_list_head_ror(list);
    if (!head || !target_id) return -1;
    return target_id(context, head);
}
