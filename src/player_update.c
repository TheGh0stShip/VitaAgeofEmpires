#include "player_update.h"
#include "player_outcome_dispatch.h"

int aoe_player_state_set(AoeOutcomePlayerRef *player, uint8_t state,
                         const AoePlayerUpdateOps *ops) {
    if (!player || !ops) return 0;
    if (state == player->state80) return 1;
    if (player->state_lock81 && state != 2) return 1;
    if (state == 2 && !player->state_lock81 && !ops->emit_state_event) return 0;
    player->state80 = state;
    if (state == 2 && !player->state_lock81)
        ops->emit_state_event(ops->user, 6, player->index4a);
    return 1;
}

int aoe_player_update_step(AoePlayerUpdateState *player,
                           AoePlayerGroupEdition edition,
                           const AoePlayerUpdateOps *ops) {
    int limit;
    uint8_t outcome;
    AoeOutcomeDispatcher dispatcher;
    AoeOutcomeOps outcome_ops;
    if (!player || !ops || !player->outcome_manager ||
        !player->outcome_manager->owner || !ops->emit_state_event ||
        !ops->map_update || !ops->object_ops.predicate ||
        !ops->object_ops.action || !ops->object_ops.cooperative_pending ||
        !ops->object_ops.cooperative_pump) return 0;
    if (edition == AOE_GROUP_BASE) limit = 40;
    else if (edition == AOE_GROUP_RISE_OF_ROME) limit = 25;
    else return 0;
    if (player->group.count < 0 || player->group.count > limit) return 0;

    if (ops->object_ops.cooperative_pending(ops->object_ops.user))
        ops->object_ops.cooperative_pump(ops->object_ops.user);
    player->field_10 = 0;
    if (!aoe_object_update_list(player->object_list28.head,
                                &ops->object_ops)) return 0;
    if (!aoe_player_group_prune(&player->group, edition)) return 0;
    dispatcher.manager = player->outcome_manager;
    outcome_ops.user = &dispatcher;
    outcome_ops.update_event = aoe_outcome_dispatch_event;
    outcome_ops.update_condition = aoe_outcome_dispatch_condition;
    if (!aoe_outcome_manager_update(player->outcome_manager,
                                    &outcome_ops, &outcome)) return 0;
    if (outcome == 1 &&
        !aoe_player_state_set(player->outcome_manager->owner, 2, ops)) return 0;
    player->flag_20 = 0;
    if (player->map_update_flag)
        ops->map_update(ops->user, player);
    return 1;
}
