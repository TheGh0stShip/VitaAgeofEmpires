#include "ai_decision.h"

#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

void aoe_ai_decision_init_ror(AoeAiDecisionRor *ai, int32_t player,
                              void *owner, AoeAiLogRor *log) {
    if (!ai) return;
    memset(ai, 0, sizeof(*ai));
    aoe_ai_module_init_ror(&ai->module, "Main Decision AI", 1000, player,
                           log);
    ai->owner = owner;
}

/* Grow to `slots` entries, copying the old ones (one at a time in the
   original). */
static int grow(AoeAiDecisionRor *ai, int32_t slots) {
    int32_t *ids = malloc((size_t)slots * sizeof(*ids));
    if (!ids) return 0;
    for (int32_t i = 0; i < ai->capacity && i < slots; ++i) ids[i] = ai->ids[i];
    free(ai->ids);
    ai->ids = ids;
    ai->capacity = slots;
    return 1;
}

static int listed(const AoeAiDecisionRor *ai, int32_t id) {
    for (int32_t i = 0; i < ai->count && i < ai->capacity; ++i)
        if (ai->ids[i] == id) return 1;
    return 0;
}

static int append(AoeAiDecisionRor *ai, int32_t id) {
    if (ai->capacity - 1 < ai->count && !grow(ai, ai->count + 1)) return 0;
    ai->ids[ai->count++] = id;
    return 1;
}

int aoe_ai_decision_load_ror(AoeAiDecisionRor *ai, int32_t player,
                             void *owner, AoeAiLogRor *log,
                             AoeOutcomeStream *in) {
    int32_t n;
    if (!ai || !in || !in->data) return 0;
    aoe_ai_decision_init_ror(ai, player, owner, log);
    n = aoe_stream_i32(in);
    for (int32_t i = 0; i < n && !in->failed; ++i) {
        int32_t id = aoe_stream_i32(in);
        if (in->failed) break;
        if (!listed(ai, id) && !append(ai, id)) return 0;
    }
    return !in->failed;
}

int aoe_ai_decision_save_ror(AoeAiDecisionRor *ai, AoeOutcomeStream *out) {
    if (!ai || !out || !out->data) return 0;
    aoe_stream_put_i32(out, ai->count);
    for (int32_t i = 0; i < ai->count; ++i) {
        if (ai->capacity - 1 < i && !grow(ai, i + 1)) return 0;
        aoe_stream_put_i32(out, ai->ids[i]);
    }
    return !out->failed;
}

void aoe_ai_decision_release_ror(AoeAiDecisionRor *ai) {
    if (!ai) return;
    free(ai->ids);
    ai->ids = NULL;
    ai->count = ai->field_fc = ai->capacity = 0;
    aoe_ai_module_release_ror(&ai->module);
}

int aoe_ai_decision_add_ror(AoeAiDecisionRor *ai, int has_object,
                            int32_t id) {
    if (!ai || !has_object) return 0;
    if (listed(ai, id)) return 1;
    return append(ai, id);
}

int aoe_ai_decision_remove_ror(AoeAiDecisionRor *ai, int32_t id) {
    int32_t i = 0;
    if (!ai) return 0;
    while (i < ai->capacity && ai->ids[i] != id) ++i;
    if (i >= ai->capacity) return 0;
    for (; i < ai->capacity - 1; ++i) ai->ids[i] = ai->ids[i + 1];
    if (--ai->count < 0) ai->count = 0;
    return 1;
}

/* The listed object at `i` (the original first grows the array to cover
   the index, which only matters after a failed growth). */
static int object_at(AoeAiDecisionRor *ai, const AoeAiDecisionOps *ops,
                     int32_t i, AoeAiObjectViewRor *view) {
    if (ai->capacity - 1 < i && !grow(ai, i + 1)) return 0;
    memset(view, 0, sizeof(*view));
    return ops->object(ops->context, ai->ids[i], view);
}

int aoe_ai_decision_object_ror(const AoeAiDecisionOps *ops, int32_t id,
                               AoeAiObjectViewRor *out) {
    AoeAiObjectViewRor view;
    if (!ops || !ops->object) return 0;
    memset(&view, 0, sizeof(view));
    if (!ops->object(ops->context, id, &view)) return 0;
    if (out) *out = view;
    return 1;
}

int32_t aoe_ai_decision_count_order_ror(AoeAiDecisionRor *ai,
                                        const AoeAiDecisionOps *ops,
                                        int32_t order) {
    int32_t n = 0;
    AoeAiObjectViewRor view;
    if (!ai || !ops || !ops->object) return 0;
    for (int32_t i = 0; i < ai->count; ++i)
        if (object_at(ai, ops, i, &view) && view.has_ai &&
            view.ai_order == order) ++n;
    return n;
}

static int base_match(const AoeAiObjectViewRor *v, int32_t id, int32_t type,
                      int32_t unit_class) {
    return v->has_ai && (id == -1 || v->id == id) &&
           (type == -1 || v->type == type) &&
           (unit_class == -1 || v->unit_class == unit_class);
}

int aoe_ai_decision_find_ror(AoeAiDecisionRor *ai,
                             const AoeAiDecisionOps *ops, int32_t id,
                             int32_t type, int32_t unit_class, int32_t order,
                             int32_t accept, int32_t ai_34, int32_t ai_38,
                             int32_t state, int32_t max_44,
                             AoeAiObjectViewRor *out) {
    AoeAiObjectViewRor v;
    if (!ai || !ops || !ops->object) return 0;
    for (int32_t i = 0; i < ai->count; ++i) {
        if (!object_at(ai, ops, i, &v) || !base_match(&v, id, type, unit_class))
            continue;
        if (order != -1 && v.ai_order != order) continue;
        if (accept != -1 &&
            (!ops->accept || !ops->accept(ops->context, ai->ids[i], accept)))
            continue;
        if (ai_34 != -1 && v.ai_34 != ai_34) continue;
        if (ai_38 != -1 && v.ai_38 != ai_38) continue;
        if (state != -1 && v.state != (uint32_t)state) continue;
        if (max_44 != -1 && !(v.field_44 <= (float)max_44)) continue;
        if (out) *out = v;
        return 1;
    }
    return 0;
}

int aoe_ai_decision_find_other_ror(AoeAiDecisionRor *ai,
                                   const AoeAiDecisionOps *ops, int32_t id,
                                   int32_t type, int32_t unit_class,
                                   int32_t not_order, int32_t not_order2,
                                   int32_t state, AoeAiObjectViewRor *out) {
    AoeAiObjectViewRor v;
    if (!ai || !ops || !ops->object) return 0;
    for (int32_t i = 0; i < ai->count; ++i) {
        if (!object_at(ai, ops, i, &v) || !base_match(&v, id, type, unit_class))
            continue;
        if (v.ai_order == not_order || v.ai_order == not_order2) continue;
        if (state != -1 && v.state != (uint32_t)state) continue;
        if (out) *out = v;
        return 1;
    }
    return 0;
}

int aoe_ai_decision_healthiest_damaged_ror(AoeAiDecisionRor *ai,
                                           const AoeAiDecisionOps *ops,
                                           int32_t unit_class, int32_t type,
                                           AoeAiObjectViewRor *out) {
    AoeAiObjectViewRor v;
    int32_t best = 0;
    int found = 0;
    if (!ai || !ops || !ops->object) return 0;
    for (int32_t i = 0; i < ai->count; ++i) {
        int32_t percent;
        if (!object_at(ai, ops, i, &v) || !v.has_ai || !v.state ||
            v.unit_class != unit_class || !(v.hp < (float)v.max_hp) ||
            v.method_90 != 1 || (type != -1 && type != v.type) || !v.max_hp)
            continue;
        percent = ((int32_t)v.hp * 100) / v.max_hp;
        if (best < percent) {
            best = percent;
            found = 1;
            if (out) *out = v;
        }
    }
    return found;
}

int32_t aoe_ai_unknown_action_ror(const AoeAiModuleRor *module,
                                  int32_t action) {
    aoe_ai_log_ror(module, "ERROR!  %d is an unrecognized action id.",
                   (int)action);
    return -1;
}
