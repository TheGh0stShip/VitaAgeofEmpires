#include "player_group.h"

#include <assert.h>
#include <string.h>

int main(void) {
    AoePlayerGroupMember a = {0, 0, 1};
    AoePlayerGroupMember b = {7, 0, 1};
    AoePlayerGroupMember c = {8, 0, 1};
    AoePlayerGroup group = {0};
    group.slots[0] = &a;
    group.slots[1] = &b;
    group.slots[2] = &c;
    group.count = 3;
    group.leader = &b;
    assert(aoe_player_group_prune(&group, AOE_GROUP_RISE_OF_ROME));
    assert(group.count == 2 && group.slots[1] == &c);
    assert(group.slots[2] == 0 && group.leader == &a);
    assert(b.flag36 == 0 && c.flag36 == 1);
    /* A swapped-in stale member is not rechecked until the next sweep. */
    assert(aoe_player_group_prune(&group, AOE_GROUP_RISE_OF_ROME));
    assert(group.count == 1 && group.slots[0] == &a);
    assert(group.slots[1] == 0 && c.flag36 == 0);

    memset(&group, 0, sizeof(group));
    AoePlayerGroupMember members[40];
    for (int i = 0; i < 40; ++i) {
        members[i].code48 = 0;
        members[i].field20 = 0;
        members[i].flag36 = 1;
        group.slots[i] = &members[i];
    }
    group.count = 40;
    group.leader = &members[39];
    members[39].field20 = 1;
    assert(aoe_player_group_prune(&group, AOE_GROUP_BASE));
    assert(group.count == 39 && group.slots[39] == 0);
    assert(group.leader == &members[0] && members[39].flag36 == 0);
    assert(!aoe_player_group_prune(&group, (AoePlayerGroupEdition)0));
    return 0;
}
