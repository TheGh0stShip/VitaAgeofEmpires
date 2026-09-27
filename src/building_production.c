#include "building_update.h"
#include <math.h>
#include <limits.h>

int aoe_building_production_ops_valid(const AoeBuildingProductionOps *ops) {
    return ops && ops->query && ops->cancel && ops->create_action &&
           ops->action_tick && ops->action_tick->update_head &&
           ops->action_tick->update_waiting && ops->notify;
}
static int entries_valid(const AoeBuildingUpdate *b) {
    return b->queue_count >= 0 && (!b->queue_count ||
        (b->queue_entries && (size_t)b->queue_count <= b->queue_capacity));
}
int aoe_building_queue_consume_ror(AoeBuildingUpdate *b,
                                    const AoeBuildingProductionOps *ops) {
    int16_t quantity;
    if (!b || !ops || !ops->notify || !entries_valid(b)) return 0;
    if (!b->queue_count) return 1;
    quantity = b->queue_entries[0].quantity;
    /* Preserve 16-bit DEC, avoiding signed conversion overflow. */
    b->queue_entries[0].quantity = quantity == INT16_MIN ? INT16_MAX : quantity-1;
    b->queue_revision = (uint8_t)(b->queue_revision + 1);
    if (b->queue_entries[0].quantity <= 0) {
        --b->queue_count;
        for (int i=0; i<b->queue_count; ++i)
            b->queue_entries[i] = b->queue_entries[i+1];
    }
    ops->notify(ops->context,124,b->owner_id,0);
    return 1;
}
int aoe_building_production_ror(AoeBuildingUpdate *b, uint32_t *count,
                                const AoeBuildingProductionOps *ops) {
    int16_t code=0;
    uint8_t action_result;
    if (!b || !count || !aoe_building_production_ops_valid(ops) ||
        !aoe_action_list_valid(b->queue_actions) || !entries_valid(b)) return 0;
    if (!b->queue_enabled || !b->queue_count) {
        if (b->queue_status==4) { b->queue_status=-1; --*count; }
        return 1;
    }
    if (ops->query(ops->context,b,&code) && (code==102 || code==103)) {
        while (b->queue_count>0) {
            int16_t previous=b->queue_count;
            if (!entries_valid(b)) return 0;
            ops->cancel(ops->context,b,b->queue_entries[0].unit_id,-1);
            /* A failed/misbound callee must not hang the portable runtime. */
            if (b->queue_count>=previous) return 0;
        }
        return 1;
    }
    if (!b->queue_actions->head) {
        const AoePlayerResources *r=&b->owner_resources;
        float population, limit, room;
        if (!r->values || r->count<=32) return 0;
        population=r->values[11]; limit=r->values[32]; room=r->values[4];
        if (!isfinite(population) || !isfinite(limit) || !isfinite(room)) return 0;
        if (population>=limit || room<=0) {
            if (b->queue_status!=4) {
                b->queue_status=4;
                ops->notify(ops->context,125,b->owner_id,population>=limit?32:4);
                ++*count;
            }
            return 1;
        }
        if (b->queue_status==4) --*count;
        b->queue_status=-1;
        if (!entries_valid(b) || !b->queue_count) return 0;
        void *action=ops->create_action(ops->context,b,b->queue_entries[0].unit_id);
        if (action && !aoe_action_list_push(b->queue_actions,action)) {
            /* Portable node allocation failure: release the unlinked action. */
            b->queue_actions->ops.destroy(b->queue_actions->ops.context,action);
            return 0;
        }
    }
    if (!aoe_action_list_update_ror(b->queue_actions,ops->action_tick,&action_result))
        return 0;
    if (action_result>=1 && action_result<=3) {
        if (!aoe_action_list_pop(b->queue_actions)) return 0;
        return aoe_building_queue_consume_ror(b,ops);
    }
    return 1;
}
