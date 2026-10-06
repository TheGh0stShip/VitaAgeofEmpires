#include <assert.h>
#include <string.h>

#include "player_selection.h"

static int sounds;
static const AoeSelectableUnitRor *last_sound;

static void play(void *context, const AoeSelectableUnitRor *unit) {
    (void)context;
    ++sounds;
    last_sound = unit;
}

static const AoePlayerSelectionOps ops = {NULL, play};

static void make_unit(AoeSelectableUnitRor *unit,
                      const AoePlayerSelectionRor *owner, int16_t owner_id) {
    memset(unit, 0, sizeof(*unit));
    unit->owner = owner;
    unit->owner_id = owner_id;
    unit->state = 2;
    unit->type = 70;
    unit->field_8d = 5;
    unit->has_selection_sound = 1;
}

static void test_init_add_remove(void) {
    AoePlayerSelectionRor p;
    AoeSelectableUnitRor u[27];
    int i;

    memset(&p, 0x5a, sizeof(p));
    p.units = NULL;
    p.unit_count = 0;
    aoe_player_selection_init_ror(&p);
    assert(p.count == 0 && p.current == NULL);
    for (i = 0; i < 7; ++i) assert(p.field_124[i] == -1);
    for (i = 0; i < AOE_PLAYER_SELECTION_GROUPS; ++i)
        assert(p.group_used[i] == 0);
    p.player_id = 2;

    for (i = 0; i < 27; ++i) make_unit(&u[i], &p, 2);
    u[1].owner = NULL;
    u[1].owner_id = 3; /* another player's unit: no sound */
    sounds = 0;
    for (i = 0; i < 25; ++i)
        assert(aoe_player_selection_add_ror(&p, &u[i], 1, &ops) == 1);
    assert(sounds == 24 && p.count == 25 && p.current == &u[24]);
    assert(u[3].selected == 1);
    assert(aoe_player_selection_add_ror(&p, &u[25], 1, &ops) == 0);

    /* Remove slot 3: slot 24 moves into it; current (u[24]) stays. */
    aoe_player_selection_remove_slot_ror(&p, 3);
    assert(u[3].selected == 0 && p.slots[3] == &u[24] && p.slots[24] == NULL);
    assert(p.count == 24 && p.current == &u[24]);
    /* The free slot at index count is reused first. */
    assert(aoe_player_selection_add_ror(&p, &u[25], 0, &ops) == 1);
    assert(p.slots[24] == &u[25]);
    /* Removing the current unit picks the first occupied slot. */
    aoe_player_selection_remove_ror(&p, &u[25]);
    assert(p.current == &u[0] && p.count == 24);
    /* A hole below count with slot[count] taken: first free slot. */
    aoe_player_selection_remove_slot_ror(&p, 23); /* last: just cleared */
    assert(p.slots[23] == NULL && p.count == 23);
    p.slots[23] = &u[23];
    p.slots[5] = NULL;
    assert(aoe_player_selection_add_ror(&p, &u[26], 0, &ops) == 1);
    assert(p.slots[5] == &u[26]);
    /* No free slot below count fails although slots 24 is free. */
    p.count = 23;
    p.slots[23] = &u[23];
    assert(p.slots[24] == NULL);
    assert(aoe_player_selection_add_ror(&p, &u[25], 0, &ops) == 0);

    aoe_player_selection_clear_ror(&p);
    assert(p.count == 0 && p.current == NULL && u[0].selected == 0);
    /* Only `count` units are visited: the test's extra unit stays. */
    assert(p.slots[0] == NULL && p.slots[23] == &u[23]);
}

static void test_groups(void) {
    AoePlayerSelectionRor p;
    AoeSelectableUnitRor u[5];
    AoeSelectableUnitRor *list[5];
    int i;

    memset(&p, 0, sizeof(p));
    aoe_player_selection_init_ror(&p);
    p.player_id = 1;
    for (i = 0; i < 5; ++i) {
        make_unit(&u[i], &p, 1);
        list[i] = &u[i];
    }
    p.units = list;
    p.unit_count = 5;

    assert(aoe_player_selection_set_group_ror(&p, -1) == 0); /* empty */
    aoe_player_selection_add_ror(&p, &u[0], 0, &ops);
    aoe_player_selection_add_ror(&p, &u[1], 0, &ops);
    u[1].owner = NULL; /* not owned: keeps its group */
    assert(aoe_player_selection_set_group_ror(&p, -1) == 1);
    assert(u[0].group == 11 && u[1].group == 0 && p.group_used[11] == 1);
    u[1].owner = &p;

    /* Explicit number: removed from other units first, flag untouched. */
    u[4].group = 3;
    assert(aoe_player_selection_set_group_ror(&p, 0x103) == 1);
    assert(u[4].group == 0 && u[0].group == 3 && u[1].group == 3);
    assert(p.group_used[3] == 0);

    /* All of 11..126 used: recompute from the units, take from 1. */
    for (i = 11; i < 0x7f; ++i) p.group_used[i] = 1;
    p.group_used[2] = 1; /* stale: cleared by the recompute */
    u[2].group = 1;
    assert(aoe_player_selection_set_group_ror(&p, -1) == 1);
    assert(p.group_used[1] == 1 && p.group_used[2] == 1);
    assert(p.group_used[11] == 0 && u[0].group == 2);

    /* Selecting a unit of group > 10 pulls in the rest of its group. */
    aoe_player_selection_clear_ror(&p);
    for (i = 0; i < 5; ++i) u[i].group = (i % 2) ? 20 : 0;
    sounds = 0;
    assert(aoe_player_selection_select_ror(&p, &u[3], &ops) == 1);
    assert(p.count == 2 && p.slots[1] == &u[1] && p.current == &u[3]);
    assert(sounds == 1 && last_sound == &u[3]);
    /* Group 10 is not expanded. */
    aoe_player_selection_clear_ror(&p);
    u[1].group = u[3].group = 10;
    assert(aoe_player_selection_select_ror(&p, &u[3], &ops) == 1);
    assert(p.count == 1);

    aoe_player_selection_clear_ror(&p);
    sounds = 0;
    assert(aoe_player_selection_select_group_ror(&p, 10, &ops) == 1);
    assert(p.count == 2 && sounds == 1 && last_sound == &u[1]);
    assert(aoe_player_selection_select_group_ror(&p, 99, &ops) == 0);

    aoe_player_selection_clear_groups_ror(&p);
    assert(u[1].group == 0 && u[3].group == 0);
}

static void test_queries(void) {
    AoePlayerSelectionRor p;
    AoeSelectableUnitRor u[4];
    AoeSelectableUnitRor *out[AOE_PLAYER_SELECTION_SLOTS];
    int i;

    memset(&p, 0, sizeof(p));
    aoe_player_selection_init_ror(&p);
    p.player_id = 1;
    assert(aoe_player_selection_common_8f_ror(&p) == 0);
    assert(aoe_player_selection_min_8d_ror(&p) == 0);
    assert(aoe_player_selection_collect_ror(&p, out, -1, -1, -1, -1) == 0);
    for (i = 0; i < 4; ++i) {
        make_unit(&u[i], &p, 1);
        u[i].field_8f = 7;
        u[i].unit_class = (int16_t)(i + 1);
        u[i].template_id = (int16_t)(100 + i);
    }
    p.slots[2] = &u[0];
    p.slots[7] = &u[1];
    p.slots[9] = &u[2];
    p.count = 3;
    assert(aoe_player_selection_common_8f_ror(&p) == 7);
    u[2].field_8f = 8;
    assert(aoe_player_selection_common_8f_ror(&p) == 0);
    u[1].field_8d = 3;
    assert(aoe_player_selection_min_8d_ror(&p) == 3);
    for (i = 0; i < 3; ++i) u[i].field_8d = 0xff;
    assert(aoe_player_selection_min_8d_ror(&p) == 0);
    assert(aoe_player_selection_has_class_ror(&p, 3) == 1);
    assert(aoe_player_selection_has_class_ror(&p, 4) == 0);

    for (i = 0; i < 3; ++i) u[i].field_8d = 5;
    u[1].field_8d = 3; /* below the default level 4 */
    assert(aoe_player_selection_collect_ror(&p, out, -1, -1, -1, -1) == 2);
    assert(out[0] == &u[0] && out[1] == &u[2]);
    assert(aoe_player_selection_collect_ror(&p, out, 3, -1, -1, -1) == 3);
    /* Class and template ID compare the parameter's low byte. */
    assert(aoe_player_selection_collect_ror(&p, out, 3, 0x102, -1, -1) == 1);
    assert(out[0] == &u[1]);
    assert(aoe_player_selection_collect_ror(&p, out, 3, -1, 102, -1) == 1);
    assert(aoe_player_selection_collect_ror(&p, out, 3, -1, -1, 8) == 1);
    assert(out[0] == &u[2]);
    u[0].type = 90;
    u[2].type = 30;
    u[1].state = 3;
    assert(aoe_player_selection_collect_ror(&p, out, 0, -1, -1, -1) == 0);
    u[1].state = 2;
    u[1].field_8d = 0;
    p.player_id = 0; /* gaia ignores the level */
    assert(aoe_player_selection_collect_ror(&p, out, 9, -1, -1, -1) == 1);

    /* Prune state >= 7 and units with +0x20 set. */
    u[0].state = 7;
    u[2].has_field_20 = 1;
    p.current = &u[0];
    aoe_player_selection_prune_ror(&p);
    assert(p.count == 1 && p.slots[7] == &u[1]);

    p.field_124[4] = 9;
    for (i = 0; i < 4; ++i) p.field_124[i] = 1;
    aoe_player_selection_reset_124_ror(&p);
    for (i = 0; i < 4; ++i) assert(p.field_124[i] == -1);
    assert(p.field_124[4] == 9);
}


static void test_class_flags(void) {
    AoePlayerSelectionRor p;
    AoeSelectableUnitRor u[4];
    int32_t priests = -1, last = -1;
    int i;
    aoe_player_selection_init_ror(&p);
    p.player_id = 2;
    for (i = 0; i < 4; ++i) { make_unit(&u[i], &p, 2); u[i].id = 100 + i; }
    u[0].unit_class = 6;                       /* soldier: 7 */
    u[1].unit_class = 0x12; u[1].carried_44 = 100.0f; /* priest at full faith */
    u[2].unit_class = 4; u[2].template_id = 0x4f;     /* civilian + id 79: 0x67? no: class 4 -> 0x67 */
    u[3].unit_class = 1; u[3].owner_id = 3;    /* not ours */
    for (i = 0; i < 4; ++i) p.slots[i] = &u[i];
    p.count = 4;
    assert(aoe_player_selection_class_flags_ror(&p, &priests, &last) ==
           (0x7u | 0x16u | 0x80u | 0x67u | 0x1u));
    assert(priests == 1 && last == 101);
    u[1].carried_44 = 99.0f;
    assert((aoe_player_selection_class_flags_ror(&p, NULL, NULL) & 0x80u) == 0);
    p.count = 0;
    assert(aoe_player_selection_class_flags_ror(&p, &priests, NULL) == 0 && priests == 0);
}
int main(void) {
    test_init_add_remove();
    test_groups();
    test_queries();
    test_class_flags();
    return 0;
}
