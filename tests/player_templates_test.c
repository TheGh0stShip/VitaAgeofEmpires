#include "player_templates.h"

#include <assert.h>
#include <string.h>

int main(void) {
    char name0[] = "Tree";
    char name2[] = "Town Center";
    DatUnitTask tasks[] = {{.task_type = 11, .work_value1 = 2.5f}};
    DatUnitAttackArmor attacks[] = {{.class_id = 4, .amount = 12}};
    DatUnitAttackArmor armors[] = {{.class_id = 3, .amount = 5}};
    DatUnit units[3] = {
        {.present = 1, .type = 90, .id = 0, .name = name0,
         .hit_points = 40},
        {0},
        {.present = 1, .type = 80, .id = 2, .name = name2,
         .task_count = 1, .tasks = tasks,
         .attack_count = 1, .attacks = attacks,
         .armor_count = 1, .armors = armors,
         .hit_points = 400}
    };
    DatCiv civ = {.player_type = 1, .unit_count = 3, .units = units};
    AoePlayerTemplates player = {0};
    DatUnit *tree, *building;
    assert(aoe_player_templates_clone_civ(&civ, &player));
    assert(player.count == 3 && player.slots);
    tree = aoe_player_templates_get(&player, 0);
    building = aoe_player_templates_get(&player, 2);
    assert(tree && building && !aoe_player_templates_get(&player, 1));
    assert(!aoe_player_templates_get(&player, 3));
    assert(tree != &units[0] && tree->name != name0);
    assert(building != &units[2] && building->name != name2);
    assert(building->tasks != tasks && building->attacks != attacks &&
           building->armors != armors);
    assert(!strcmp(tree->name, "Tree") && !strcmp(building->name, "Town Center"));
    name0[0] = 'F';
    name2[0] = 'B';
    tasks[0].task_type = 99;
    attacks[0].amount = 99;
    armors[0].amount = 99;
    units[2].hit_points = 1;
    assert(!strcmp(tree->name, "Tree"));
    assert(!strcmp(building->name, "Town Center"));
    assert(building->tasks[0].task_type == 11 &&
           building->attacks[0].amount == 12 &&
           building->armors[0].amount == 5 && building->hit_points == 400);
    assert(!aoe_player_templates_clone_civ(&civ, &player));
    aoe_player_templates_free(&player);
    assert(player.count == 0 && player.slots == NULL);
    units[2].type = 15; /* fail after an earlier slot was cloned */
    assert(!aoe_player_templates_clone_civ(&civ, &player));
    assert(player.count == 0 && player.slots == NULL);
    assert(!aoe_player_templates_get(&player, 0));
    units[2].type = 80;
    units[2].tasks = NULL;
    assert(!aoe_player_templates_clone_civ(&civ, &player));
    assert(player.count == 0 && player.slots == NULL);
    return 0;
}
