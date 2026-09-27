#include "player_group.h"

static int capacity(AoePlayerGroupEdition edition) {
    if (edition == AOE_GROUP_BASE) return 40;
    if (edition == AOE_GROUP_RISE_OF_ROME) return 25;
    return 0;
}

static int valid(const AoePlayerGroup *group, int limit) {
    return group && limit && group->count >= 0 && group->count <= limit;
}

int aoe_player_group_remove(AoePlayerGroup *group,
                            AoePlayerGroupEdition edition, int16_t slot) {
    int limit = capacity(edition);
    AoePlayerGroupMember *removed;
    if (!valid(group, limit) || slot < 0 || slot >= limit) return 0;
    removed = group->slots[slot];
    if (!removed) return 1;
    if (group->count == 0) return 0;
    removed->flag36 = 0;
    if (slot < group->count - 1) {
        group->slots[slot] = group->slots[group->count - 1];
        group->slots[group->count - 1] = 0;
    } else {
        group->slots[slot] = 0;
    }
    group->count--;
    if (group->leader == removed) {
        group->leader = 0;
        if (group->count > 0) {
            for (int i = 0; i < limit; ++i) {
                if (group->slots[i]) {
                    group->leader = group->slots[i];
                    break;
                }
            }
        }
    }
    return 1;
}

int aoe_player_group_prune(AoePlayerGroup *group,
                           AoePlayerGroupEdition edition) {
    int limit = capacity(edition);
    int kept = 0;
    if (!valid(group, limit)) return 0;
    for (int slot = 0; slot < limit; ++slot) {
        AoePlayerGroupMember *member;
        if (group->count <= kept) return 1;
        member = group->slots[slot];
        if (!member) continue;
        if (member->code48 < 7 && member->field20 == 0) {
            kept++;
        } else if (!aoe_player_group_remove(group, edition, (int16_t)slot)) {
            return 0;
        }
        /* The original advances slot even after a last-entry swap. */
    }
    return 1;
}
