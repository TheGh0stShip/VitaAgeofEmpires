#include "building_update.h"
#include "research_action.h"
#include "train_action.h"
#include <math.h>
#include <limits.h>

int aoe_building_production_ops_valid(const AoeBuildingProductionOps *ops) {
    return ops && ops->refund_one && ops->cancel_action && ops->create_action &&
           ops->action_tick && ops->action_tick->update_head &&
           ops->action_tick->update_waiting && ops->notify;
}

int aoe_building_active_action_query_ror(const AoeBuildingUpdate *b,
                                          int16_t *action_type,
                                          int16_t *item_id,
                                          int16_t *progress) {
    const void *action;
    int16_t type;
    if (!b || !b->live_actions || !action_type || !item_id || !progress)
        return 0;
    action = b->live_actions->head ? b->live_actions->head->action : NULL;
    type = action ? *(const int16_t *)action : 0;
    *action_type = type;
    if (type == 102)
        return aoe_train_action_progress_ror(action,item_id,progress);
    if (type == 103)
        return aoe_research_action_progress_ror(action,item_id,progress);
    *item_id = -1;
    *progress = 0;
    return 0;
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
int aoe_building_queue_cancel_ror(AoeBuildingUpdate *b, int16_t unit_id,
                                   int16_t amount, uint32_t *count,
                                   const AoeBuildingQueueCancelOps *ops) {
    int index = -1;
    int16_t quantity;
    int16_t new_quantity;
    int refund_count;
    if (!b || !count || !ops || !ops->refund_one || !ops->notify ||
        amount < 0 || !entries_valid(b) ||
        !aoe_action_list_valid(b->queue_actions)) return 0;
    if (!b->queue_count) return 1;
    for (int i=b->queue_count-1; i>=0; --i) {
        if (b->queue_entries[i].unit_id == unit_id) { index=i; break; }
    }
    if (index >= 0) {
        quantity=b->queue_entries[index].quantity;
        new_quantity=(int16_t)((uint16_t)quantity-(uint16_t)amount);
        if (!index && new_quantity <= 0 && b->queue_actions->head &&
            !ops->cancel_action) return 0;
        refund_count=quantity <= amount ? quantity : amount;
        for (int i=0; i<refund_count; ++i)
            ops->refund_one(ops->context,unit_id);
        /* Preserve x86 word subtraction for malformed/extreme queue data. */
        b->queue_entries[index].quantity=new_quantity;
        b->queue_revision=(uint8_t)(b->queue_revision+1);
        if (b->queue_entries[index].quantity <= 0) {
            --b->queue_count;
            for (int i=index; i<b->queue_count; ++i)
                b->queue_entries[i]=b->queue_entries[i+1];
            if (!index) {
                void *action=b->queue_actions->head ?
                    b->queue_actions->head->action : NULL;
                if (b->queue_status != -1) {
                    if (b->queue_status == 4) --*count;
                    b->queue_status=-1;
                }
                if (action) ops->cancel_action(ops->context,action);
                if (!aoe_action_list_clear(b->queue_actions)) return 0;
            }
        }
    }
    ops->notify(ops->context,124,b->owner_id,0);
    return 1;
}
static int fill_entry(AoeBuildingUpdate *b, int16_t index, int16_t unit_id,
                      int16_t count, const AoeBuildingQueueAddOps *ops) {
    int added = 0;
    for (int i = 0; i < count; ++i) {
        int16_t failing = unit_id; /* the original reuses the argument slot */
        if (!ops->can_afford(ops->context, unit_id, &failing)) {
            ops->notify(ops->context, 0x7d, b->owner_id,
                        (uint32_t)(int32_t)failing);
            break;
        }
        ops->pay(ops->context, unit_id);
        b->queue_entries[index].quantity =
            (int16_t)(uint16_t)((uint16_t)b->queue_entries[index].quantity + 1u);
        added = 1;
    }
    return added;
}

int aoe_building_queue_add_ror(AoeBuildingUpdate *b, int16_t unit_id,
                               int16_t count,
                               const AoeBuildingQueueAddOps *ops) {
    int16_t failing;
    if (!b || !ops || !ops->can_afford || !ops->pay || !ops->notify ||
        !ops->allocate_zeroed || !ops->deallocate || !entries_valid(b) ||
        b->queue_aux_count < 0 ||
        (b->queue_aux_count && !b->queue_entries) ||
        (size_t)b->queue_aux_count > b->queue_capacity ||
        b->queue_count > b->queue_aux_count)
        return 0;
    if (b->queue_count > 0 &&
        b->queue_entries[b->queue_count - 1].unit_id == unit_id) {
        if (count <= 0) return 1;
        if (!fill_entry(b, (int16_t)(b->queue_count - 1), unit_id, count,
                        ops))
            return 1;
        b->queue_revision = (uint8_t)(b->queue_revision + 1);
        ops->notify(ops->context, 0x7c, b->owner_id, 0);
        return 1;
    }
    while (b->queue_count > 0) {
        int16_t before_count = b->queue_count;
        AoeBuildingQueueEntry before = b->queue_entries[0];
        if (!ops->owner_status_four_count ||
            !aoe_building_queue_cancel_ror(b, before.unit_id, 1,
                                           ops->owner_status_four_count,
                                           &ops->cancel))
            return 0;
        if (b->queue_count == before_count &&
            b->queue_entries[0].unit_id == before.unit_id &&
            b->queue_entries[0].quantity == before.quantity)
            return 0;
    }
    failing = unit_id;
    if (!ops->can_afford(ops->context, unit_id, &failing)) {
        ops->notify(ops->context, 0x7d, b->owner_id,
                    (uint32_t)(int32_t)failing);
        return 1;
    }
    if (b->queue_count == b->queue_aux_count) {
        int32_t capacity = (int32_t)b->queue_aux_count + 10;
        AoeBuildingQueueEntry *grown = ops->allocate_zeroed(
            ops->context, (size_t)capacity, sizeof(*grown));
        if (!grown) return 1;
        if (b->queue_entries) {
            for (int16_t i = 0; i < b->queue_aux_count; ++i)
                grown[i] = b->queue_entries[i];
            ops->deallocate(ops->context, b->queue_entries);
        }
        b->queue_entries = grown;
        b->queue_aux_count = (int16_t)capacity;
        b->queue_capacity = (size_t)capacity;
    }
    b->queue_entries[b->queue_count].quantity = 0;
    (void)fill_entry(b, b->queue_count, unit_id, count, ops);
    b->queue_entries[b->queue_count].unit_id = unit_id;
    ++b->queue_count;
    b->queue_revision = (uint8_t)(b->queue_revision + 1);
    ops->notify(ops->context, 0x7c, b->owner_id, 0);
    return 1;
}

int aoe_building_state_queue_transition_ror(
    AoeBuildingUpdate *b, uint8_t new_state, uint32_t *count,
    const AoeBuildingQueueCancelOps *ops) {
    int16_t amount;
    if (!b || !count || !ops) return 0;
    if (new_state <= 2 || b->state <= 1 || b->queue_count <= 0) return 1;
    if (!entries_valid(b)) return 0;
    if (b->queue_status == 4) {
        b->queue_status=-1;
        --*count;
        amount=b->queue_entries[0].quantity;
    } else {
        amount=(int16_t)((uint16_t)b->queue_entries[0].quantity-1u);
    }
    if (amount < 0) return 0;
    return aoe_building_queue_cancel_ror(
        b,b->queue_entries[0].unit_id,amount,count,ops);
}
int aoe_building_production_ror(AoeBuildingUpdate *b, uint32_t *count,
                                const AoeBuildingProductionOps *ops) {
    int16_t code=0, item_id, progress;
    uint8_t action_result;
    if (!b || !count || !aoe_building_production_ops_valid(ops) ||
        !aoe_action_list_valid(b->queue_actions) || !entries_valid(b)) return 0;
    if (!b->queue_enabled || !b->queue_count) {
        if (b->queue_status==4) { b->queue_status=-1; --*count; }
        return 1;
    }
    if (!b->live_actions) return 0;
    if (aoe_building_active_action_query_ror(b,&code,&item_id,&progress) &&
        (code==102 || code==103)) {
        const AoeBuildingQueueCancelOps cancel_ops={
            ops->context,ops->refund_one,ops->cancel_action,ops->notify
        };
        while (b->queue_count>0) {
            int16_t previous_count=b->queue_count;
            int16_t previous_id=b->queue_entries[0].unit_id;
            int16_t previous_quantity=b->queue_entries[0].quantity;
            if (!entries_valid(b)) return 0;
            if (!aoe_building_queue_cancel_ror(
                    b,b->queue_entries[0].unit_id,1,count,&cancel_ops)) return 0;
            if (b->queue_count==previous_count &&
                b->queue_entries[0].unit_id==previous_id &&
                b->queue_entries[0].quantity==previous_quantity) return 0;
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
