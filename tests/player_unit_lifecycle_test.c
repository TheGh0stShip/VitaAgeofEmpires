#include "player_update.h"

#include <assert.h>

int main(void) {
    AoePlayerUpdateState player = {0};
    AoeOwnedVisibilityRegistry world;
    AoeVisibilityUnitRef units[4] = {{0}};
    AoeVisibilityUnitRef *replaced = 0;
    AoeObjectUpdateNode *first = 0;
    AoeObjectUpdateNode *second = 0;
    AoeObjectUpdateNode *third = 0;
    AoeObjectUpdateNode *fourth = 0;
    int needs_reassign = 0;
    int list_removed = 0;
    assert(aoe_owned_visibility_registry_init(&world));

    assert(aoe_player_unit_register(&player, &world, &units[0], 0,
                                     0, 0, &first, &replaced,
                                     &needs_reassign));
    assert(first == player.object_list28.head &&
           player.object_list28.count == 1);
    assert(aoe_visibility_registry_resolve(&world.view, 0) == &units[0]);
    assert(!needs_reassign && replaced == 0);

    assert(aoe_player_unit_register(&player, &world, &units[1], 1,
                                     1, 1, &second, &replaced,
                                     &needs_reassign));
    assert(second == player.object_list2c.head &&
           player.object_list2c.count == 1);
    assert(player.object_list30.count == 0); /* +0x2c has priority. */
    assert(aoe_player_unit_register(&player, &world, &units[2], -1,
                                     0, 1, &third, &replaced,
                                     &needs_reassign));
    assert(third == player.object_list30.head &&
           player.object_list30.count == 1);
    assert(aoe_visibility_registry_resolve(&world.view, -1) == &units[2]);

    assert(aoe_player_unit_register(&player, &world, &units[3], 2,
                                     0, 0, &fourth, &replaced,
                                     &needs_reassign));
    assert(player.object_list28.head == fourth && fourth->next == first);
    assert(first->prev == fourth && player.object_list28.count == 2);

    assert(aoe_player_unit_remove(&player, &world, &units[0], 0,
                                   0, 0, first, &list_removed));
    assert(list_removed && player.object_list28.head == fourth);
    assert(fourth->next == 0 && player.object_list28.count == 1);
    assert(aoe_visibility_registry_resolve(&world.view, 0) == 0);
    assert(aoe_player_unit_remove(&player, &world, &units[1], 1,
                                   1, 1, 0, &list_removed));
    assert(list_removed && player.object_list2c.count == 0);
    assert(aoe_visibility_registry_resolve(&world.view, 1) == 0);
    assert(aoe_player_unit_remove(&player, &world, &units[2], -1,
                                   0, 1, third, &list_removed));
    assert(list_removed && player.object_list30.count == 0);
    assert(aoe_visibility_registry_resolve(&world.view, -1) == 0);
    assert(aoe_player_unit_remove(&player, &world, &units[3], 2,
                                   0, 0, fourth, &list_removed));
    assert(list_removed && player.object_list28.count == 0);
    assert(aoe_visibility_registry_resolve(&world.view, 2) == 0);

    aoe_object_update_list_clear(&player.object_list28);
    aoe_object_update_list_clear(&player.object_list2c);
    aoe_object_update_list_clear(&player.object_list30);
    aoe_owned_visibility_registry_destroy(&world);
    return 0;
}
