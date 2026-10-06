#include "player_selection.h"

#include <string.h>

void aoe_player_selection_init_ror(AoePlayerSelectionRor *player) {
    int i;

    if (!player) return;
    for (i = 0; i < 7; ++i) player->field_124[i] = -1;
    player->count = 0;
    player->current = NULL;
    for (i = 0; i < AOE_PLAYER_SELECTION_SLOTS; ++i) player->slots[i] = NULL;
    memset(player->group_used, 0, sizeof(player->group_used));
}

int aoe_player_selection_add_ror(AoePlayerSelectionRor *player,
                                 AoeSelectableUnitRor *unit, int play_sound,
                                 const AoePlayerSelectionOps *ops) {
    int16_t count;
    int16_t slot;

    if (!player || !unit) return 0;
    count = player->count;
    if (count >= AOE_PLAYER_SELECTION_SLOTS) return 0;
    /* Port: a negative count (never produced by these routines) cannot
       index the slots. */
    if (count < 0) return 0;
    if (player->slots[count] == NULL) {
        slot = count;
    } else {
        for (slot = 0; slot < count; ++slot)
            if (player->slots[slot] == NULL) break;
        if (slot >= count) return 0;
    }
    player->slots[slot] = unit;
    player->count++;
    player->current = unit;
    unit->selected = 1;
    if (play_sound && unit->has_selection_sound &&
        (unit->owner == player || unit->owner_id == 0) && ops &&
        ops->play_selection_sound)
        ops->play_selection_sound(ops->context, unit);
    return 1;
}

int aoe_player_selection_select_ror(AoePlayerSelectionRor *player,
                                    AoeSelectableUnitRor *unit,
                                    const AoePlayerSelectionOps *ops) {
    uint8_t group;
    size_t i;

    if (!aoe_player_selection_add_ror(player, unit, 1, ops)) return 0;
    group = unit->group;
    if (group > 10 && player->units) {
        for (i = 0; i < player->unit_count; ++i) {
            AoeSelectableUnitRor *other = player->units[i];
            if (!other || other == unit || other->group != group) continue;
            if (!aoe_player_selection_add_ror(player, other, 0, ops)) break;
        }
    }
    player->current = unit;
    return 1;
}

void aoe_player_selection_clear_ror(AoePlayerSelectionRor *player) {
    int16_t slot, seen;

    if (!player || player->count <= 0) return;
    player->current = NULL;
    seen = 0;
    for (slot = 0; seen < player->count && slot < AOE_PLAYER_SELECTION_SLOTS;
         ++slot) {
        if (player->slots[slot]) {
            player->slots[slot]->selected = 0;
            ++seen;
            player->slots[slot] = NULL;
        }
    }
    player->count = 0;
}

void aoe_player_selection_remove_ror(AoePlayerSelectionRor *player,
                                     const AoeSelectableUnitRor *unit) {
    int16_t slot, seen;

    if (!player || player->count <= 0) return;
    seen = 0;
    for (slot = 0; seen < player->count && slot < AOE_PLAYER_SELECTION_SLOTS;
         ++slot) {
        if (!player->slots[slot]) continue;
        if (player->slots[slot] == unit) {
            aoe_player_selection_remove_slot_ror(player, slot);
            return;
        }
        ++seen;
    }
}

void aoe_player_selection_remove_slot_ror(AoePlayerSelectionRor *player,
                                          int16_t slot) {
    AoeSelectableUnitRor *unit;
    int last;

    if (!player || slot < 0 || slot >= AOE_PLAYER_SELECTION_SLOTS) return;
    unit = player->slots[slot];
    if (!unit) return;
    unit->selected = 0;
    last = player->count - 1;
    if (slot < last) {
        /* Port: the original reads +0x134 + count * 4 (slot count-1)
           unchecked; a count above 25 cannot arise from these routines. */
        if (last < AOE_PLAYER_SELECTION_SLOTS) {
            player->slots[slot] = player->slots[last];
            player->slots[last] = NULL;
        } else {
            player->slots[slot] = NULL;
        }
    } else {
        player->slots[slot] = NULL;
    }
    player->count--;
    if (player->current == unit) {
        player->current = NULL;
        if (player->count > 0) {
            int16_t i;
            for (i = 0; i < AOE_PLAYER_SELECTION_SLOTS; ++i) {
                if (player->slots[i]) {
                    player->current = player->slots[i];
                    break;
                }
            }
        }
    }
}

int aoe_player_selection_set_group_ror(AoePlayerSelectionRor *player,
                                       int32_t group) {
    uint8_t number;
    int16_t slot, seen;
    size_t i;

    if (!player || player->count == 0) return 0;
    if (group == -1) {
        int16_t n;
        number = 0xff; /* the low byte of -1 */
        for (n = 11; n < 0x7f; ++n) {
            if (!player->group_used[n]) {
                number = (uint8_t)n;
                player->group_used[n] = 1;
                break;
            }
        }
        if (n == 0x7f) {
            memset(player->group_used + 1, 0, 126);
            for (i = 0; player->units && i < player->unit_count; ++i) {
                const AoeSelectableUnitRor *unit = player->units[i];
                /* Port: group numbers are bytes, so index 127..255 would
                   fall past the 127 flags; the original writes there. */
                if (unit && unit->group && unit->group < 0x7f)
                    player->group_used[unit->group] = 1;
            }
            for (n = 1; n < 0x7f; ++n) {
                if (!player->group_used[n]) {
                    number = (uint8_t)n;
                    player->group_used[n] = 1;
                    break;
                }
            }
            if (n == 0x7f) return 0;
        }
    } else {
        number = (uint8_t)group;
        for (i = 0; player->units && i < player->unit_count; ++i) {
            AoeSelectableUnitRor *unit = player->units[i];
            if (unit && unit->group == number) unit->group = 0;
        }
    }
    seen = 0;
    for (slot = 0; seen < player->count && slot < AOE_PLAYER_SELECTION_SLOTS;
         ++slot) {
        AoeSelectableUnitRor *unit = player->slots[slot];
        if (unit && unit->owner == player) {
            ++seen;
            unit->group = number;
        }
    }
    return 1;
}

int aoe_player_selection_select_group_ror(AoePlayerSelectionRor *player,
                                          uint8_t group,
                                          const AoePlayerSelectionOps *ops) {
    int first = 1;
    size_t i;

    if (!player || !player->units) return 0;
    for (i = 0; i < player->unit_count; ++i) {
        AoeSelectableUnitRor *unit = player->units[i];
        if (!unit || unit->group != group) continue;
        if (first) {
            if (!aoe_player_selection_add_ror(player, unit, 1, ops)) break;
            first = 0;
        } else if (!aoe_player_selection_add_ror(player, unit, 0, ops)) {
            return 1;
        }
    }
    return !first;
}

int aoe_player_selection_has_class_ror(const AoePlayerSelectionRor *player,
                                       int16_t unit_class) {
    int16_t slot, seen;

    if (!player) return 0;
    seen = 0;
    for (slot = 0; seen < player->count && slot < AOE_PLAYER_SELECTION_SLOTS;
         ++slot) {
        const AoeSelectableUnitRor *unit = player->slots[slot];
        if (!unit) continue;
        ++seen;
        if (unit->unit_class == unit_class) return 1;
    }
    return 0;
}

uint8_t aoe_player_selection_common_8f_ror(
    const AoePlayerSelectionRor *player) {
    uint8_t value = 0;
    int16_t slot, seen;

    if (!player) return 0;
    seen = 0;
    for (slot = 0; seen < player->count && slot < AOE_PLAYER_SELECTION_SLOTS;
         ++slot) {
        const AoeSelectableUnitRor *unit = player->slots[slot];
        if (!unit) continue;
        if (seen != 0 && unit->field_8f != value) return 0;
        ++seen;
        value = unit->field_8f;
    }
    return value;
}

uint8_t aoe_player_selection_min_8d_ror(const AoePlayerSelectionRor *player) {
    uint8_t low = 0xff;
    int16_t slot, seen;

    if (!player) return 0;
    seen = 0;
    for (slot = 0; seen < player->count && slot < AOE_PLAYER_SELECTION_SLOTS;
         ++slot) {
        const AoeSelectableUnitRor *unit = player->slots[slot];
        if (!unit) continue;
        if (unit->field_8d < low) low = unit->field_8d;
        ++seen;
    }
    return low == 0xff ? 0 : low;
}

int16_t aoe_player_selection_collect_ror(
    const AoePlayerSelectionRor *player,
    AoeSelectableUnitRor *out[AOE_PLAYER_SELECTION_SLOTS], int16_t min_8d,
    int16_t unit_class, int16_t template_id, int16_t field_8f) {
    int16_t found = 0;
    int seen = 0;
    int slot;

    if (!player || !out || player->count == 0) return 0;
    if (min_8d == -1) min_8d = 4;
    for (slot = 0; slot < AOE_PLAYER_SELECTION_SLOTS; ++slot) {
        AoeSelectableUnitRor *unit;
        if (seen >= player->count) break;
        unit = player->slots[slot];
        if (!unit) continue;
        ++seen;
        if (unit->state != 2 || unit->owner != player) continue;
        if ((int16_t)unit->field_8d < min_8d && player->player_id != 0)
            continue;
        if (unit_class != -1 &&
            unit->unit_class != (int16_t)(uint8_t)unit_class)
            continue;
        if (template_id != -1 &&
            unit->template_id != (int16_t)(uint8_t)template_id)
            continue;
        if (field_8f != -1 && unit->field_8f != (uint8_t)field_8f) continue;
        if (unit->type < 40 || unit->type == 90) continue;
        out[found++] = unit;
    }
    return found;
}

void aoe_player_selection_prune_ror(AoePlayerSelectionRor *player) {
    int16_t slot, kept;

    if (!player) return;
    kept = 0;
    for (slot = 0; kept < player->count && slot < AOE_PLAYER_SELECTION_SLOTS;
         ++slot) {
        AoeSelectableUnitRor *unit = player->slots[slot];
        if (!unit) continue;
        if (unit->state < 7 && !unit->has_field_20)
            ++kept;
        else
            aoe_player_selection_remove_slot_ror(player, slot);
    }
}

void aoe_player_selection_reset_124_ror(AoePlayerSelectionRor *player) {
    if (!player) return;
    player->field_124[0] = -1;
    player->field_124[1] = -1;
    player->field_124[2] = -1;
    player->field_124[3] = -1;
}

void aoe_player_selection_clear_groups_ror(AoePlayerSelectionRor *player) {
    int slot, seen;

    if (!player) return;
    seen = 0;
    for (slot = 0; seen < player->count && slot < AOE_PLAYER_SELECTION_SLOTS;
         ++slot) {
        if (player->slots[slot]) {
            ++seen;
            player->slots[slot]->group = 0;
        }
    }
}

/* 0x0055bdc8: per-class selection flags (40 entries). */
static const uint32_t class_flags[40] = {
    0x7, 0x0, 0x4, 0x0, 0x67, 0x0, 0x7, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7, 0x7, 0x0, 0x0, 0x0, 0x10,
    0x96, 0x0, 0x0, 0x0, 0x1, 0x7, 0x7, 0x7, 0x7, 0x0, 0x7, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7,
    0x7, 0x0, 0x0, 0x7};

uint32_t aoe_player_selection_class_flags_ror(const AoePlayerSelectionRor *player,
                                              int32_t *priests, int32_t *last_priest) {
    uint32_t flags = 0;
    int32_t n = 0, slot, seen = 0;
    if (priests) *priests = 0;
    if (!player || player->count == 0) return 0;
    for (slot = 0; slot < AOE_PLAYER_SELECTION_SLOTS && seen < player->count; ++slot) {
        const AoeSelectableUnitRor *u = player->slots[slot];
        if (!u) continue;
        ++seen;
        if (u->owner_id != player->player_id) continue;
        if (u->unit_class >= 0 && u->unit_class < 40) {
            if (u->unit_class == 0x12) {
                flags |= 0x16;
                ++n;
                if (last_priest) *last_priest = u->id;
                if (u->carried_44 >= 100.0f) flags |= 0x80;
            } else {
                flags |= class_flags[u->unit_class];
            }
        }
        if (u->template_id == 0x4f || u->template_id == 199 || u->template_id == 0x45 ||
            u->template_id == 0x116)
            flags |= 1;
    }
    if (priests) *priests = n;
    return flags;
}
