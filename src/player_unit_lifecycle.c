#include "player_update.h"

static AoeObjectUpdateList *selected_list(AoePlayerUpdateState *player,
                                           int select_2c, int select_30) {
    if (select_2c) return &player->object_list2c;
    if (select_30) return &player->object_list30;
    return &player->object_list28;
}

int aoe_player_unit_register(AoePlayerUpdateState *player,
                             AoeOwnedVisibilityRegistry *world,
                             AoeVisibilityUnitRef *unit, int32_t unit_id,
                             int select_2c, int select_30,
                             AoeObjectUpdateNode **attached,
                             AoeVisibilityUnitRef **replaced,
                             int *needs_reassign) {
    AoeObjectUpdateList *list;
    AoeObjectUpdateNode *node;
    if (!player || !world || !unit || !attached || !replaced ||
        !needs_reassign) return 0;
    list = selected_list(player, select_2c, select_30);
    if (!aoe_object_update_list_attach(list, unit, &node)) return 0;
    if (!aoe_owned_visibility_registry_insert(world, unit_id, unit,
                                               replaced, needs_reassign)) {
        aoe_object_update_list_detach(list, unit, node);
        return 0;
    }
    *attached = node;
    return 1;
}

int aoe_player_unit_remove(AoePlayerUpdateState *player,
                           AoeOwnedVisibilityRegistry *world,
                           AoeVisibilityUnitRef *unit, int32_t unit_id,
                           int select_2c, int select_30,
                           AoeObjectUpdateNode *node,
                           int *list_removed) {
    AoeObjectUpdateList *list;
    if (!player || !world || !unit || !list_removed) return 0;
    list = selected_list(player, select_2c, select_30);
    *list_removed = aoe_object_update_list_detach(list, unit, node);
    return aoe_owned_visibility_registry_clear(world, unit_id);
}
