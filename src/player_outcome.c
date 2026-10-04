#include "player_outcome.h"

#include <stdlib.h>

static int valid_events(AoeOutcomeEvent *head) {
    AoeOutcomeEvent *slow = head, *fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return 0;
    }
    return 1;
}

static int valid_conditions(AoeOutcomeCondition *head) {
    AoeOutcomeCondition *slow = head, *fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return 0;
    }
    return 1;
}

int aoe_outcome_recompute(AoeOutcomeManager *manager) {
    unsigned groups = 0, satisfied_groups = 0, cumulative_type3 = 0;
    AoeOutcomeCondition *first;
    if (!manager || !valid_conditions(manager->conditions)) return 0;
    if (!manager->conditions) return 1;
    manager->outcome = 0;
    for (AoeOutcomeCondition *item = manager->conditions;
         item; item = item->next) item->visited = 0;
    for (;;) {
        unsigned count = 0, type2 = 0;
        int has_type1 = 0;
        uint8_t group;
        first = manager->conditions;
        while (first && first->visited) first = first->next;
        if (!first) break;
        group = first->group;
        groups++;
        for (AoeOutcomeCondition *item = first; item; item = item->next) {
            if (item->group != group) continue;
            item->visited = 1;
            count++;
            if (item->status == 1) has_type1 = 1;
            else if (item->status == 2) type2++;
            else if (item->status == 3) cumulative_type3++;
        }
        if (has_type1) {
            satisfied_groups++;
            continue;
        }
        if (type2 > 0 && count == cumulative_type3 + type2) {
            manager->outcome = 2;
            break;
        }
    }
    if (groups == satisfied_groups) manager->outcome = 1;
    return 1;
}

int aoe_outcome_manager_update(AoeOutcomeManager *manager,
                               const AoeOutcomeOps *ops, uint8_t *result) {
    if (!manager || !ops || !ops->update_event ||
        !ops->update_condition || !result ||
        !valid_events(manager->events) ||
        !valid_conditions(manager->conditions)) return 0;
    for (AoeOutcomeEvent *event = manager->events; event;
         event = event->next) {
        if (event->status != 1 && event->status != 3 &&
            !ops->update_event(ops->user, event)) return 0;
    }
    for (AoeOutcomeCondition *item = manager->conditions; item;
         item = item->next) {
        if (item->status != 1 && item->status != 3 &&
            !ops->update_condition(ops->user, item)) return 0;
    }
    if (!aoe_outcome_recompute(manager)) return 0;
    *result = manager->outcome;
    return 1;
}

AoeOutcomeEvent *aoe_outcome_event_add_ror(AoeOutcomeManager *m, uint8_t kind,
                                           uint8_t category, uint8_t index,
                                           int32_t resource, int32_t field_08,
                                           int32_t weight_0c) {
    AoeOutcomeEvent *e;
    if (!m || kind > 4 || !(e = calloc(1, sizeof(*e)))) return NULL;
    e->index_02 = index;
    e->category_03 = category;
    e->next = m->events;
    m->events = e;
    ++m->event_count;
    e->kind = kind;
    e->resource_index_04 = resource;
    e->divisor_08 = field_08;
    e->weight_0c = weight_0c;
    return e;
}

void aoe_outcome_events_free_ror(AoeOutcomeManager *m) {
    if (!m) return;
    while (m->events) {
        AoeOutcomeEvent *next = m->events->next;
        free(m->events);
        m->events = next;
    }
    m->event_count = 0;
}
