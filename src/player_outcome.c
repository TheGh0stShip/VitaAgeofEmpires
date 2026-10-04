#include "player_outcome.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "le_bytes.h"

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

AoeOutcomeCondition *aoe_outcome_condition_new_ror(AoeOutcomeManager *m,
                                                   uint8_t group,
                                                   uint8_t include_allies) {
    AoeOutcomeCondition *c;
    if (!m || !(c = calloc(1, sizeof(*c)))) return NULL;
    c->status = 0;
    c->group = group;
    c->include_allies_2d = include_allies;
    c->counter_20 = 0;
    c->next = m->conditions;
    m->conditions = c;
    ++m->condition_count;
    return c;
}

static AoeOutcomeCondition *add(AoeOutcomeManager *m, uint8_t group,
                                uint8_t allies, uint8_t kind) {
    AoeOutcomeCondition *c = aoe_outcome_condition_new_ror(m, group, allies);
    if (c) c->kind = kind;
    return c;
}

static void set_area(AoeOutcomeCondition *c, float x0, float y0, float x1,
                     float y1) {
    c->min_x = x0;
    c->min_y = y0;
    c->max_x = x1;
    c->max_y = y1;
}

int aoe_outcome_add_destroy_unit_ror(AoeOutcomeManager *m, uint8_t group,
                                     AoeOutcomeUnitRef *unit, uint8_t allies) {
    AoeOutcomeCondition *c;
    if (!unit || !(c = add(m, group, allies, 0))) return 0;
    c->unit_28 = unit;
    c->player_08 = unit->owner0c;
    return 1;
}

int aoe_outcome_add_create_ror(AoeOutcomeManager *m, uint8_t group,
                               int32_t unit_class, int32_t count,
                               uint8_t allies) {
    AoeOutcomeCondition *c = add(m, group, allies, 1);
    if (!c) return 0;
    c->unit_class_04 = unit_class;
    c->unit_target_count_1c = count;
    return 1;
}

int aoe_outcome_add_create_in_area_ror(AoeOutcomeManager *m, uint8_t group,
                                       int32_t unit_class, int32_t count,
                                       float x0, float y0, float x1, float y1,
                                       uint8_t allies) {
    AoeOutcomeCondition *c = add(m, group, allies, 8);
    if (!c) return 0;
    c->unit_class_04 = unit_class;
    c->unit_target_count_1c = count;
    set_area(c, x0, y0, x1, y1);
    return 1;
}

int aoe_outcome_add_capture_ror(AoeOutcomeManager *m, uint8_t group,
                                AoeOutcomeUnitRef *unit) {
    AoeOutcomeCondition *c;
    if (!unit || !(c = add(m, group, 0, 2))) return 0;
    c->unit_28 = unit;
    c->player_08 = unit->owner0c;
    return 1;
}

int aoe_outcome_add_destroy_count_ror(AoeOutcomeManager *m, uint8_t group,
                                      int32_t unit_class, int32_t count,
                                      AoeOutcomePlayerRef *player) {
    AoeOutcomeCondition *c = add(m, group, 0, 3);
    if (!c) return 0;
    c->unit_class_04 = unit_class;
    c->threshold_1c = count;
    c->player_08 = player;
    return 1;
}

int aoe_outcome_add_destroy_all_ror(AoeOutcomeManager *m, uint8_t group,
                                    int32_t unit_class,
                                    AoeOutcomePlayerRef *player) {
    AoeOutcomeCondition *c = add(m, group, 0, 9);
    if (!c) return 0;
    c->unit_class_04 = unit_class;
    c->player_08 = player;
    return 1;
}

int aoe_outcome_add_defeat_player_ror(AoeOutcomeManager *m, uint8_t group,
                                      AoeOutcomePlayerRef *player) {
    AoeOutcomeCondition *c = add(m, group, 0, 10);
    if (!c) return 0;
    c->player_08 = player;
    return 1;
}

int aoe_outcome_add_bring_to_unit_ror(AoeOutcomeManager *m, uint8_t group,
                                      AoeOutcomeUnitRef *unit,
                                      AoeOutcomeUnitRef *destination) {
    AoeOutcomeCondition *c;
    if (!destination || !(c = add(m, group, 0, 5))) return 0;
    c->unit_24 = unit;
    c->unit_28 = destination;
    c->player_08 = destination->owner0c;
    return 1;
}

int aoe_outcome_add_bring_to_area_ror(AoeOutcomeManager *m, uint8_t group,
                                      AoeOutcomeUnitRef *unit, float x0,
                                      float y0, float x1, float y1) {
    AoeOutcomeCondition *c = add(m, group, 0, 4);
    if (!c) return 0;
    c->unit_24 = unit;
    set_area(c, x0, y0, x1, y1);
    return 1;
}

int aoe_outcome_add_attribute_ror(AoeOutcomeManager *m, uint8_t group,
                                  int32_t attribute, int32_t amount,
                                  uint8_t allies) {
    AoeOutcomeCondition *c = add(m, group, allies, 6);
    if (!c) return 0;
    c->resource_index_1c = attribute;
    c->resource_target_20 = amount;
    return 1;
}

int aoe_outcome_add_explore_ror(AoeOutcomeManager *m, uint8_t group,
                                int32_t percent, uint8_t allies) {
    AoeOutcomeCondition *c = add(m, group, allies, 7);
    if (!c) return 0;
    c->progress_target_20 = percent < 1 ? 1 : percent > 100 ? 100 : percent;
    return 1;
}

int aoe_outcome_add_score_ror(AoeOutcomeManager *m, uint8_t group,
                              int32_t score, uint8_t allies) {
    AoeOutcomeCondition *c = add(m, group, allies, 12);
    if (!c) return 0;
    c->score_target_20 = score;
    return 1;
}

int aoe_outcome_add_kind100_ror(AoeOutcomeManager *m, uint8_t group,
                                int32_t value) {
    AoeOutcomeCondition *c = add(m, group, 0, 100);
    if (!c) return 0;
    c->counter_20 = (uint32_t)value;
    return 1;
}

void aoe_outcome_condition_remove_ror(AoeOutcomeManager *m,
                                      AoeOutcomeCondition *condition) {
    AoeOutcomeCondition **link;
    if (!m) return;
    for (link = &m->conditions; *link; link = &(*link)->next) {
        if (*link != condition) continue;
        *link = condition->next;
        free(condition);
        --m->condition_count;
        return;
    }
}

void aoe_outcome_conditions_clear_ror(AoeOutcomeManager *m) {
    while (m && m->conditions)
        aoe_outcome_condition_remove_ror(m, m->conditions);
}

void aoe_outcome_conditions_drop_unit_ror(AoeOutcomeManager *m,
                                          const AoeOutcomeUnitRef *unit) {
    AoeOutcomeCondition *c = m ? m->conditions : NULL;
    while (c) {
        AoeOutcomeCondition *next = c->next;
        if (c->unit_24 == unit || c->unit_28 == unit)
            aoe_outcome_condition_remove_ror(m, c);
        c = next;
    }
}

void aoe_outcome_event_remove_ror(AoeOutcomeManager *m,
                                  AoeOutcomeEvent *event) {
    AoeOutcomeEvent **link;
    if (!m) return;
    for (link = &m->events; *link; link = &(*link)->next) {
        if (*link != event) continue;
        *link = event->next;
        free(event);
        --m->event_count;
        return;
    }
}

void aoe_outcome_manager_release_ror(AoeOutcomeManager *m) {
    if (!m) return;
    aoe_outcome_conditions_clear_ror(m);
    while (m->events) aoe_outcome_event_remove_ror(m, m->events);
    m->condition_count = 0;
    m->event_count = 0;
    m->outcome = 0;
    m->owner = NULL;
}

AoeOutcomeCondition *aoe_outcome_condition_at_ror(const AoeOutcomeManager *m,
                                                  int32_t index) {
    AoeOutcomeCondition *c = m ? m->conditions : NULL;
    for (int32_t i = 0; c; c = c->next, ++i)
        if (i == index) return c;
    return NULL;
}

int aoe_outcome_won_ror(const AoeOutcomeManager *m) {
    return m && m->outcome == 2;
}

int32_t aoe_outcome_points_ror(const AoeOutcomeManager *m, int8_t category) {
    uint32_t sum = 0;
    for (const AoeOutcomeEvent *e = m ? m->events : NULL; e; e = e->next)
        if ((int8_t)e->category_03 == category || category == -1)
            sum += e->contribution_10;
    return (int32_t)sum;
}

int32_t aoe_outcome_event_points_ror(const AoeOutcomeManager *m,
                                     int8_t index) {
    int32_t value = 0;
    for (const AoeOutcomeEvent *e = m ? m->events : NULL; e; e = e->next)
        if ((int8_t)e->index_02 == index) value = (int32_t)e->contribution_10;
    return value;
}

int32_t aoe_outcome_event_sample_ror(const AoeOutcomeManager *m,
                                     int8_t index) {
    int32_t value = 0;
    for (const AoeOutcomeEvent *e = m ? m->events : NULL; e; e = e->next)
        if ((int8_t)e->index_02 == index) value = (int32_t)e->sample_14;
    return value;
}

static const char *unit_name(const AoeOutcomeUnitRef *unit) {
    return unit ? unit->type_name : NULL;
}

static const char *class_name(const AoeOutcomeCondition *c) {
    const AoeOutcomePlayerRef *p = c->player_08;
    if (!p || !p->type_names || c->unit_class_04 < 0 ||
        (size_t)c->unit_class_04 >= p->type_count) return NULL;
    return p->type_names[c->unit_class_04];
}

int aoe_outcome_condition_text_ror(const AoeOutcomeCondition *c, char *out,
                                   size_t size) {
    const char *a = NULL, *b = NULL;
    if (!c || !out || !size) return 0;
    out[0] = '\0';
    switch (c->kind) {
    case 0:
        if (!(a = unit_name(c->unit_24))) return 1;
        snprintf(out, size, "Capture %s", a);
        break;
    case 1:
        if (!(a = class_name(c))) return 1;
        snprintf(out, size, "Create %d %s", (int)c->unit_target_count_1c, a);
        break;
    case 2:
        if (!(a = unit_name(c->unit_24))) return 1;
        snprintf(out, size, "Destroy %s", a);
        break;
    case 3:
        if (!(a = class_name(c))) return 1;
        snprintf(out, size, "Destroy %d %s of player %d",
                 (int)c->threshold_1c, a, (int)c->player_08->index4a);
        break;
    case 4:
        if (!(a = unit_name(c->unit_24))) return 1;
        snprintf(out, size, "Bring %s to area (%f, %f) - (%f, %f)", a,
                 (double)c->min_x, (double)c->min_y, (double)c->max_x,
                 (double)c->max_y);
        break;
    case 5:
        if (!(a = unit_name(c->unit_24)) || !(b = unit_name(c->unit_28)))
            return 1;
        snprintf(out, size, "Bring %s to %s", a, b);
        break;
    case 6:
        snprintf(out, size, "aqquire %d of attribute %d",
                 (int)c->resource_target_20, (int)c->resource_index_1c);
        break;
    case 7:
        snprintf(out, size, "explore %d percent of the map",
                 (int)c->progress_target_20);
        break;
    case 8:
        if (!(a = class_name(c))) return 1;
        snprintf(out, size, "Create %d %s in area (%f, %f) - (%f, %f)",
                 (int)c->unit_target_count_1c, a, (double)c->min_x,
                 (double)c->min_y, (double)c->max_x, (double)c->max_y);
        break;
    case 9:
        if (!(a = class_name(c))) return 1;
        snprintf(out, size, "Destroy all %s of player %d", a,
                 (int)c->player_08->index4a);
        break;
    default:
        break;
    }
    return 1;
}

static void put(AoeOutcomeStream *s, const uint8_t *bytes, size_t n) {
    if (s->failed || s->size - s->pos < n || s->pos > s->size) {
        s->failed = 1;
        return;
    }
    memcpy(s->data + s->pos, bytes, n);
    s->pos += n;
}

static void put_u8(AoeOutcomeStream *s, uint8_t v) { put(s, &v, 1); }

static void put_u32(AoeOutcomeStream *s, uint32_t v) {
    uint8_t b[4];
    aoe_le_put_u32(b, v);
    put(s, b, 4);
}

static void put_f32(AoeOutcomeStream *s, float v) {
    uint8_t b[4];
    aoe_le_put_f32(b, v);
    put(s, b, 4);
}

static const uint8_t *take(AoeOutcomeStream *s, size_t n) {
    const uint8_t *p;
    if (s->failed || s->pos > s->size || s->size - s->pos < n) {
        s->failed = 1;
        return NULL;
    }
    p = s->data + s->pos;
    s->pos += n;
    return p;
}

static uint8_t get_u8(AoeOutcomeStream *s) {
    const uint8_t *p = take(s, 1);
    return p ? p[0] : 0;
}

static int32_t get_i32(AoeOutcomeStream *s) {
    const uint8_t *p = take(s, 4);
    return p ? aoe_le_i32(p) : 0;
}

static float get_f32(AoeOutcomeStream *s) {
    const uint8_t *p = take(s, 4);
    return p ? aoe_le_f32(p) : 0.0f;
}

int aoe_outcome_manager_save_ror(const AoeOutcomeManager *m,
                                 AoeOutcomeStream *out) {
    if (!m || !out || !out->data) return 0;
    put_f32(out, 1.0f);
    put_u32(out, m->condition_count);
    put_u8(out, m->outcome);
    for (const AoeOutcomeCondition *c = m->conditions; c; c = c->next) {
        int32_t player = c->player_08 ? c->player_08->index4a : -1;
        int32_t id24 = c->unit_24 ? c->unit_24->id04 : -1;
        int32_t id28 = -1;
        if (c->unit_28) {
            id28 = c->unit_28->id04;
            player = c->unit_28->owner0c ? c->unit_28->owner0c->index4a : -1;
        }
        put_u8(out, c->kind);
        put_u32(out, (uint32_t)c->unit_class_04);
        put_u32(out, (uint32_t)player);
        put_f32(out, c->min_x);
        put_f32(out, c->min_y);
        put_f32(out, c->max_x);
        put_f32(out, c->max_y);
        put_u32(out, (uint32_t)c->threshold_1c);
        put_u32(out, c->counter_20);
        put_u32(out, (uint32_t)id24);
        put_u32(out, (uint32_t)id28);
        put_u8(out, c->group);
        put_u8(out, c->include_allies_2d);
        put_u8(out, c->status);
    }
    put_u32(out, m->score_sum_1c);
    put_u32(out, m->event_count);
    for (const AoeOutcomeEvent *e = m->events; e; e = e->next) {
        put_u8(out, e->kind);
        put_u8(out, e->status);
        put_u32(out, (uint32_t)e->resource_index_04);
        put_u32(out, (uint32_t)e->divisor_08);
        put_u32(out, (uint32_t)e->weight_0c);
        put_u32(out, e->contribution_10);
        put_u8(out, e->index_02);
        put_u8(out, e->category_03);
        put_f32(out, e->sample_14);
    }
    return !out->failed;
}

static AoeOutcomePlayerRef *world_player(const AoeOutcomePlayerRef *owner,
                                         int32_t index) {
    const AoeOutcomeWorldRef *w = owner ? owner->world3c : NULL;
    if (!w || !w->players || index < 0 || index >= w->player_count ||
        (size_t)index >= w->player_capacity) return NULL;
    return w->players[index];
}

int aoe_outcome_manager_load_ror(AoeOutcomeManager *m,
                                 AoeOutcomePlayerRef *owner,
                                 AoeOutcomeStream *in, int has_version,
                                 const int32_t *player_map,
                                 size_t player_map_count,
                                 AoeOutcomeFindUnitFn find_unit,
                                 void *context) {
    AoeOutcomeCondition **tail;
    float version = 0.0f;
    int32_t count, events;
    if (!m || !in || !in->data || !find_unit) return 0;
    memset(m, 0, sizeof(*m));
    if (has_version) version = get_f32(in);
    count = get_i32(in);
    m->condition_count = (uint32_t)count;
    m->outcome = get_u8(in);
    m->owner = owner;
    tail = &m->conditions;
    for (int32_t i = 0; i < count && !in->failed; ++i) {
        AoeOutcomeCondition *c = calloc(1, sizeof(*c));
        int32_t player, id24, id28;
        if (!c) goto fail;
        c->kind = get_u8(in);
        c->unit_class_04 = get_i32(in);
        player = get_i32(in);
        c->min_x = get_f32(in);
        c->min_y = get_f32(in);
        c->max_x = get_f32(in);
        c->max_y = get_f32(in);
        c->threshold_1c = get_i32(in);
        c->counter_20 = (uint32_t)get_i32(in);
        id24 = get_i32(in);
        id28 = get_i32(in);
        c->group = get_u8(in);
        c->include_allies_2d = get_u8(in);
        c->status = get_u8(in);
        *tail = c;
        tail = &c->next;
        if (id24 >= 0) {
            const AoeOutcomeWorldRef *w = owner ? owner->world3c : NULL;
            for (int32_t p = 0; w && p < w->player_count; ++p) {
                AoeOutcomePlayerRef *each = world_player(owner, p);
                if ((c->unit_24 = find_unit(context, each, id24))) break;
            }
            if (!c->unit_24) c->status = 3;
        }
        if (id28 >= 0) c->unit_28 = find_unit(context, owner, id28);
        if (player < 0) {
            c->player_08 = NULL;
        } else {
            if (player_map)
                player = (size_t)player < player_map_count
                             ? player_map[player] : -1;
            if (player < 0) {
                c->status = 3;
            } else if ((c->player_08 = world_player(owner, player)) &&
                       !c->unit_28) {
                if (id28 < 0) continue;
                c->unit_28 = find_unit(context, c->player_08, id28);
            }
        }
        if (id28 >= 0 && !c->unit_28) c->status = 3;
    }
    if (in->failed) goto fail;
    if (!(version < 0.0f)) {
        m->score_sum_1c = (uint32_t)get_i32(in);
        events = get_i32(in);
        for (int32_t i = 0; i < events && !in->failed; ++i) {
            AoeOutcomeEvent *e = calloc(1, sizeof(*e));
            if (!e) goto fail;
            e->resource_index_04 = -1;
            e->next = m->events;
            m->events = e;
            ++m->event_count;
            e->kind = get_u8(in);
            e->status = get_u8(in);
            e->resource_index_04 = get_i32(in);
            e->divisor_08 = get_i32(in);
            e->weight_0c = get_i32(in);
            e->contribution_10 = (uint32_t)get_i32(in);
            e->index_02 = get_u8(in);
            e->category_03 = get_u8(in);
            e->sample_14 = get_f32(in);
            if ((int32_t)m->score_sum_1c < 0) e->weight_0c = 0;
        }
        if (in->failed) goto fail;
        if ((int32_t)m->score_sum_1c < 0) m->score_sum_1c = 0;
    }
    return 1;
fail:
    aoe_outcome_manager_release_ror(m);
    return 0;
}
