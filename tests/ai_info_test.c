/* Patched RoR 1.0a information-AI members (0x004bc230..0x004c6880). */
#include "ai_info.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t grid[8][8];
static int grid_get(void *c, int32_t x, int32_t y) { (void)c; return grid[x][y]; }
static void grid_set(void *c, int32_t x, int32_t y, int32_t v) { (void)c; grid[x][y] = (uint8_t)v; }
static int32_t width(void *c) { (void)c; return 8; }
static int32_t height(void *c) { (void)c; return 6; }
static uint8_t tile5(void *c, int32_t x, int32_t y) { (void)c; return (uint8_t)(x == 3 && y == 3 ? 0xe2 : 0x01); }
static int is_enemy(void *c, int32_t p) { (void)c; return p == 2; }
static int is_ally(void *c, int32_t p) { (void)c; return p == 1; }
static int32_t sn(void *c, int32_t i) { (void)c; return i == 0xc9 ? 1 : 0; }
static uint8_t zone(void *c, int32_t id) { (void)c; return (uint8_t)(id & 0xff); }
static int object(void *c, int32_t id, AoeAiObjectViewRor *o) {
    (void)c;
    if (id < 0 || id >= 1000) return 0;
    memset(o, 0, sizeof *o);
    o->id = id;
    o->owner = (int16_t)(id % 3);
    o->state = (uint8_t)(id == 41 ? 3 : 2);
    o->hp = id == 42 ? 0.0f : 10.0f;
    return 1;
}

/* World two: dropsites 100 (type 0x6d at 10,10), 101 (0x67 at 13,14),
   102 (0x44 at 2,2), 103 (0x2d dock at 20,20), 104 (0x67 in another zone);
   resources 200 (class 7 at 10,13), 201 (class 0xf, type 0x3b, at 3,3);
   enemies 300 (at 6,6, type range 5) and 301 (state 3). */
static int object2(void *c, int32_t id, AoeAiObjectViewRor *o) {
    (void)c;
    memset(o, 0, sizeof *o);
    o->id = id;
    o->state = 2;
    o->hp = 40.0f;
    switch (id) {
    case 100: o->type_id = 0x6d; o->x = 10.5f; o->y = 10.5f; return 1;
    case 101: o->type_id = 0x67; o->x = 13.0f; o->y = 14.0f; return 1;
    case 102: o->type_id = 0x44; o->x = 2.0f; o->y = 2.0f; return 1;
    case 103: o->type_id = 0x2d; o->x = 20.9f; o->y = 20.2f; return 1;
    case 104: o->type_id = 0x67; o->x = 50.0f; o->y = 50.0f; return 1;
    case 200: o->type_class = 7; o->x = 10.0f; o->y = 13.0f; return 1;
    case 201: o->type_class = 0xf; o->type_id = 0x3b; o->x = 3.0f; o->y = 3.0f; return 1;
    case 300: o->x = 6.0f; o->y = 6.0f; o->type_28 = 5.0f; o->owner = 2; return 1;
    case 301: o->state = 3; o->owner = 2; return 1;
    case 400: o->hp = 30.0f; return 1;
    default: return 0;
    }
}
static uint8_t zone2(void *c, int32_t id) { (void)c; (void)id; return 1; }
static uint8_t tile_zone2(void *c, int32_t id, int32_t x, int32_t y) {
    (void)c; (void)id; (void)y; return (uint8_t)(x >= 40 ? 2 : 1);
}
static uint8_t map_zone2(void *c, int32_t x, int32_t y) { (void)c; (void)y; return (uint8_t)(x == 20 ? 3 : 0); }
static double dist2(void *c, int32_t id, float x, float y, float z) {
    AoeAiObjectViewRor o;
    (void)z;
    object2(c, id, &o);
    return sqrt((double)(o.x - x) * (o.x - x) + (double)(o.y - y) * (o.y - y));
}
static double v104(void *c, int32_t u, int32_t t) { (void)c; (void)u; (void)t; return 3.0; }
static double v100(void *c, int32_t id) { (void)c; return id == 300 ? 2.0 : 4.0; }
static double range2(void *c, int32_t id) { (void)c; (void)id; return 1.0; }

static void dropsite_tests(void) {
    static AoeAiInfoRor info;
    AoeAiInfoOpsRor ops;
    AoeAiObjectViewRor r, g;
    int32_t dst, ids[2] = {400, 999};
    float d;
    static AoeAiUnitGroupRor group;

    memset(&ops, 0, sizeof ops);
    ops.object = object2;
    ops.world_object = object2;
    ops.zone = zone2;
    ops.tile_zone = tile_zone2;
    ops.map_zone = map_zone2;
    ops.is_enemy = is_enemy;
    ops.distance = dist2;
    ops.value_104 = v104;
    ops.value_100 = v100;
    ops.range = range2;
    aoe_ai_int_list_add_unique_ror(&info.list_190, 104);
    aoe_ai_int_list_add_unique_ror(&info.list_190, 100);
    aoe_ai_int_list_add_unique_ror(&info.list_190, 101);
    aoe_ai_int_list_add_unique_ror(&info.list_190, 102);
    aoe_ai_int_list_add_unique_ror(&info.list_190, 103);
    aoe_ai_int_list_add_unique_ror(&info.list_190, 77);

    assert(aoe_ai_info_docks_in_zone_ror(&info, 3, &ops) == 1 &&
           aoe_ai_info_docks_in_zone_ror(&info, 0, &ops) == 0);
    assert(aoe_ai_info_nearest_dock_ror(&info, 20, 20, &ops) == -1);
    assert(aoe_ai_info_nearest_dock_ror(&info, 17, 16, &ops) == 5);
    assert(aoe_ai_info_count_dropsites_ror(&info, 0x67, &ops) == 2 &&
           aoe_ai_info_count_dropsites_ror(&info, 0x44, &ops) == 1);

    object2(NULL, 200, &r);
    d = aoe_ai_info_nearest_dropsite_ror(&info, &r, 1, &dst, &ops);
    assert(dst == 100 && d == (float)sqrt(0.25 + 6.25));
    d = aoe_ai_info_nearest_dropsite_ror(&info, &r, 0, &dst, &ops);
    assert(dst == 101 && d == (float)sqrt(9.0 + 1.0));
    object2(NULL, 201, &r);
    d = aoe_ai_info_nearest_dropsite_ror(&info, &r, 0, &dst, &ops);
    assert(dst == 102 && d == (float)sqrt(2.0));
    r.id = 999; r.type_id = 1;
    d = aoe_ai_info_nearest_dropsite_ror(&info, &r, 0, &dst, &ops);
    assert(dst == 101);

    memset(&g, 0, sizeof g);
    g.x = 3.0f; g.y = 3.0f; g.field_4c = 0x10;
    assert(aoe_ai_info_gatherer_dropsite_ror(&info, &g, &ops) == 102);
    g.field_4c = 0;
    assert(aoe_ai_info_gatherer_dropsite_ror(&info, &g, &ops) == 100);
    g.x = 13.0f; g.y = 15.0f;
    assert(aoe_ai_info_gatherer_dropsite_ror(&info, &g, &ops) == 101);

    /* Target lists. */
    info.target_count[0] = 2;
    info.targets[0] = (AoeAiInfoTargetRor *)calloc(2, sizeof(AoeAiInfoTargetRor));
    info.targets[0][0].id = 200;
    info.targets[0][1].id = 201;
    info.target_count[1] = 0;
    info.best_distance[1] = info.best_distance[2] = info.best_distance[3] = -1;
    aoe_ai_info_refresh_targets_ror(&info, 0, &ops);
    /* 200: d = sqrt(6.5) = 2.55: +8 = ftol(15.1) = 15, +0xe = 2. */
    assert(info.targets[0][0].field_8 == 15 && info.targets[0][0].field_e == 2 &&
           info.targets[0][0].field_10 == 100 && info.best_distance[0] == 2 &&
           info.current_target[0] == 200);
    /* 201 (class 0xf) lands in slot 1: d = sqrt(2) with 102 (0x44). */
    assert(info.targets[0][1].field_10 == 102 && info.best_distance[1] == 1 &&
           info.current_target[1] == 201);
    info.targets[0][1].gone = 1;
    info.targets[0][1].field_8 = 0;
    info.best_distance[1] = 9;
    aoe_ai_info_refresh_targets_ror(&info, 0, &ops);
    assert(info.targets[0][1].field_8 == 0 && info.best_distance[1] == 9);
    aoe_ai_info_refresh_all_targets_ror(&info, &ops);
    /* Each list resets its own slot first: list 1 wipes what list 0 put in
       slot 1. */
    assert(info.targets[0][1].field_8 == 12 && info.best_distance[1] == -1 &&
           info.best_distance[0] == 2 &&
           info.best_distance[2] == -1);

    /* Threat and reach: enemy 300 at (6,6) reaches 5 tiles. */
    info.object_count = 3;
    info.objects = (AoeAiInfoObjectRor *)calloc(3, sizeof *info.objects);
    info.objects[0].id = 300; info.objects[0].owner = 2;
    info.objects[1].id = 301; info.objects[1].owner = 2;
    info.objects[2].id = 100; info.objects[2].owner = 1;
    assert(aoe_ai_info_threat_ror(&info, 400, 3, 3, &ops) == 1.5f);
    assert(aoe_ai_info_threat_ror(&info, 400, 0, 0, &ops) == 0.0f);
    assert(aoe_ai_info_threat_ror(&info, 999, 3, 3, &ops) == 0.0f);
    assert(aoe_ai_info_in_reach_ror(&info, -1, 3, 3, 0, &ops) &&
           !aoe_ai_info_in_reach_ror(&info, -1, 3, 3, 1, &ops) &&
           aoe_ai_info_in_reach_ror(&info, 2, 6, 5, 1, &ops) &&
           !aoe_ai_info_in_reach_ror(&info, 1, 3, 3, 0, &ops));

    /* Group sums: 3 / 4 for 400, nothing for 999; endurance 30 / threat. */
    assert(aoe_ai_info_ids_damage_ror(&info, ids, 2, 7, &ops) == 0.75f);
    aoe_ai_unit_group_init_ror(&group);
    aoe_ai_unit_group_add_ror(&group, 999, NULL);
    aoe_ai_unit_group_add_ror(&group, 400, NULL);
    assert(aoe_ai_info_group_damage_ror(&info, &group, 7, &ops) == 0.75f);
    g.x = 3.5f; g.y = 3.9f;
    assert(aoe_ai_info_ids_endurance_ror(&info, ids, 2, &g, &ops) == 20.0f);
    assert(aoe_ai_info_group_endurance_ror(&info, &group, &g, &ops) == 20.0f);
    g.x = 0.0f; g.y = 0.0f;
    assert(aoe_ai_info_ids_endurance_ror(&info, ids, 2, &g, &ops) == 600.0f);

    free(info.targets[0]);
    free(info.objects);
    aoe_ai_int_list_release_ror(&info.list_190);
}

int main(void) {
    static AoeAiInfoRor info;
    AoeAiInfoOpsRor ops;
    AoeAiObjectViewRor seen;
    AoeAiInfoObjectRor *e;
    int32_t t[2], a[2], b[2], first, second, i;

    memset(&ops, 0, sizeof ops);
    ops.self = 1;
    ops.object = object;
    ops.world_object = object;
    ops.zone = zone;
    ops.grid_get = grid_get;
    ops.grid_set = grid_set;
    ops.map_width = width;
    ops.map_height = height;
    ops.tile_byte5 = tile5;
    ops.is_enemy = is_enemy;
    ops.is_ally = is_ally;
    ops.sn = sn;
    ops.stats_player = 0xff;
    ops.settings_97d = 1;

    info.map_width = info.map_height = -1;
    assert(aoe_ai_info_map_width_ror(&info, &ops) == 8 &&
           aoe_ai_info_map_height_ror(&info, &ops) == 6);

    /* Zones. */
    assert(aoe_ai_info_zone_open_ror(&info, 5, &ops));
    aoe_ai_info_set_zone_flag_ror(&info, 5, 1, &ops);
    assert(!aoe_ai_info_zone_open_ror(&info, 5, &ops) && info.zone_flags[5] == 1);
    assert(!aoe_ai_info_zone_open_ror(&info, 2000, &ops));

    /* Explored grid. */
    memset(grid, 0xff, sizeof grid);
    grid[1][1] = 1;
    assert(!aoe_ai_info_explored_ror(&info, 0, 0, &ops));
    aoe_ai_info_mark_rect_ror(&info, -3, 0, 2, 9, &ops);
    assert(grid[0][0] == 2 && grid[1][1] == 1 && grid[2][5] == 2 &&
           grid[3][0] == 0xff && grid[2][6] == 0xff);
    assert(aoe_ai_info_explored_ror(&info, 2, 5, &ops) &&
           !aoe_ai_info_explored_ror(&info, 2, 6, &ops));
    aoe_ai_info_forget_tile_ror(&info, 2, 5, &ops);
    aoe_ai_info_forget_tile_ror(&info, 8, 0, &ops);
    assert(grid[2][5] == 0);

    /* Clamping, nearness and directions. */
    t[0] = -1; t[1] = 3;
    assert(aoe_ai_info_clamp_ror(&info, t) && t[0] == 0 && t[1] == 3);
    t[0] = 4; t[1] = 9;
    assert(aoe_ai_info_clamp_ror(&info, t) && t[1] == 5);
    t[0] = 9; t[1] = -2;
    assert(aoe_ai_info_clamp_ror(&info, t) && t[0] == 7 && t[1] == 0);
    t[0] = 3; t[1] = 3;
    assert(!aoe_ai_info_clamp_ror(&info, t));
    a[0] = 2; a[1] = 2; b[0] = 3; b[1] = 3;
    assert(!aoe_ai_info_tiles_near_ror(a, b, 0) && aoe_ai_info_tiles_near_ror(a, b, 1) &&
           aoe_ai_info_tiles_near_ror(a, b, 7));
    b[0] = 2; b[1] = 5;
    assert(aoe_ai_info_direction_ror(a, b) == 5);
    b[1] = 0;
    assert(aoe_ai_info_direction_ror(a, b) == 1);
    b[0] = 4; b[1] = 2;
    assert(aoe_ai_info_direction_ror(a, b) == 3);
    b[0] = 0;
    assert(aoe_ai_info_direction_ror(a, b) == 7);
    b[1] = 3;
    assert(aoe_ai_info_direction_ror(a, b) == 6);
    b[1] = 1;
    assert(aoe_ai_info_direction_ror(a, b) == 0);
    b[0] = 3;
    assert(aoe_ai_info_direction_ror(a, b) == 2);
    b[1] = 9;
    assert(aoe_ai_info_direction_ror(a, b) == 4);

    /* Neighbour terrain: (3,3) has 0xe2 & 0x1f = 2. */
    assert(aoe_ai_info_neighbour_terrain_ror(2, 2, 2, &ops) &&
           aoe_ai_info_neighbour_terrain_ror(4, 3, 2, &ops) &&
           !aoe_ai_info_neighbour_terrain_ror(3, 3, 2, &ops) &&
           !aoe_ai_info_neighbour_terrain_ror(0, 0, 2, &ops));

    /* Object memory: two slots, then the table doubles. */
    info.object_count = 2;
    info.objects = (AoeAiInfoObjectRor *)calloc(2, sizeof *info.objects);
    info.objects[0].id = info.objects[1].id = -1;
    memset(&seen, 0, sizeof seen);
    seen.id = 10; seen.type_id = 0x49; seen.type_class = 6; seen.owner = 2;
    seen.x = 3.9f; seen.y = 1.2f; seen.z = 0.5f; seen.hp = 25.7f;
    e = aoe_ai_info_remember_ror(&info, &seen, 1.0f, 2.0f, 7.0f, &ops);
    assert(e == &info.objects[0] && e->x == 3 && e->y == 1 && e->owner == 2 &&
           e->hp == 25 && e->range == 7.0f && info.stats[0] == 1);
    seen.x = 5.0f;
    assert(aoe_ai_info_remember_ror(&info, &seen, 1, 2, 3, &ops) == &info.objects[0] &&
           info.objects[0].x == 5 && info.stats[0] == 1);
    seen.id = 11; seen.owner_4 = 1; /* a computer owner: no statistics */
    assert(aoe_ai_info_remember_ror(&info, &seen, 1, 2, 3, &ops) == &info.objects[1] &&
           info.stats[0] == 1);
    seen.id = 12; seen.owner_4 = 0; seen.type_class = 0x12; seen.x = 3.0f;
    e = aoe_ai_info_remember_ror(&info, &seen, 1, 2, 3, &ops);
    assert(info.object_count == 4 && e == &info.objects[2] &&
           info.objects[3].id == -1 && info.stats[5] == 1 &&
           info.objects[0].id == 10);
    assert(aoe_ai_info_find_object_ror(&info, 12) == &info.objects[2] &&
           !aoe_ai_info_find_object_ror(&info, -1) &&
           !aoe_ai_info_find_object_ror(&info, 99));
    /* 10, 11, 12 are owned by 1, 2, 0 now; all were seen as owner 2. */
    assert(aoe_ai_info_count_owned_ror(&info, -1, 2, &ops) == 1 &&
           aoe_ai_info_count_owned_ror(&info, 2, 1, &ops) == 1 &&
           aoe_ai_info_count_owned_ror(&info, 3, 1, &ops) == 0);
    /* Within radius 2 of (5, 1): entries at x 5 (two) and 3 (12). */
    assert(aoe_ai_info_count_near_ror(&info, 2, 5, 1, 2, &ops) == 3 &&
           aoe_ai_info_count_near_ror(&info, 2, 5, 1, 1, &ops) == 2 &&
           aoe_ai_info_count_near_ror(&info, -1, 5, 1, 1, &ops) == 2 &&
           aoe_ai_info_count_near_ror(&info, 1, 5, 1, 9, &ops) == 0);
    info.objects[1].type_id = 0x6d;
    assert(aoe_ai_info_enemy_center_near_ror(&info, 5.9f, 1.0f, 0, &ops) &&
           !aoe_ai_info_enemy_center_near_ror(&info, 7.0f, 1.0f, 1, &ops));
    assert(aoe_ai_info_nearest_enemy_ror(&info, 4.0f, 1.0f, 3, 0x49, 0x6d, -5, -5,
                                         &ops) == 10);
    info.objects[0].x = 9;
    assert(aoe_ai_info_nearest_enemy_ror(&info, 4.0f, 1.0f, 3, 0x49, 0x6d, -5, -5,
                                         &ops) == 11);
    info.objects[1].id = 41; /* state 3: not alive */
    assert(aoe_ai_info_nearest_enemy_ror(&info, 4.0f, 1.0f, 3, 0x49, 0x6d, -5, -5,
                                         &ops) == 12);
    info.objects[2].id = 42; /* no hit points */
    assert(aoe_ai_info_nearest_enemy_ror(&info, 4.0f, 1.0f, 3, 0x49, 0x6d, -5, -5,
                                         &ops) == -1);

    /* Class sets. */
    assert(aoe_ai_info_unit_class_ror(0x27) && !aoe_ai_info_unit_class_ror(1));
    assert(aoe_ai_info_resource_class_ror(0x21) && !aoe_ai_info_resource_class_ror(6));
    assert(aoe_ai_info_known_class_ror(0x1e) && aoe_ai_info_known_class_ror(9) &&
           !aoe_ai_info_known_class_ror(0x22));
    assert(aoe_ai_info_key_building_ror(3, 0x68) && !aoe_ai_info_key_building_ror(3, 0x69) &&
           !aoe_ai_info_key_building_ror(4, 0x68));

    /* Marks. */
    info.mark_count = 3;
    info.marks = (AoeAiInfoMarkRor *)calloc(3, sizeof *info.marks);
    info.marks[0] = (AoeAiInfoMarkRor){7, 1, 10, 10, 0};
    info.marks[1] = (AoeAiInfoMarkRor){7, 0, 20, 20, 0};
    info.marks[2] = (AoeAiInfoMarkRor){8, 1, 30, 30, 0};
    assert(aoe_ai_info_open_mark_ror(&info, 7) == &info.marks[1]);
    assert(aoe_ai_info_mark_near_ror(&info, 7, 12, 8, 2) &&
           !aoe_ai_info_mark_near_ror(&info, 7, 13, 10, 2) &&
           !aoe_ai_info_mark_near_ror(&info, 7, 20, 20, 0));
    aoe_ai_info_set_mark_ror(&info, 7, 20, 20, 1);
    assert(info.marks[1].state == 1 && !aoe_ai_info_open_mark_ror(&info, 7));
    aoe_ai_info_set_mark_ror(&info, 8, 30, 30, 3);
    assert(info.marks[2].id == -1 && info.marks[2].state == 3 && info.marks[2].x == 0);

    /* Areas and the 4x4 cells. */
    info.area_count = 3;
    info.areas = (AoeAiInfoAreaRor *)calloc(3, sizeof *info.areas);
    for (i = 0; i < 3; ++i) info.areas[i].id = -1;
    info.areas[1].id = 5;
    info.areas[1].age = 9;
    assert(aoe_ai_info_add_area_ror(&info, 1, 2, 1, 3, 1, 0, 1, 7, 8, 4, 5, &ops) == 0);
    /* (2 + 3) * 4 = 20: 20 / 8 / 2 = 1, 20 / 6 / 2 = 1; ours counts at +0x40c. */
    assert(info.cells[1][1][1] == 1 && info.areas[0].id == 0 && info.areas[0].x1 == 3 &&
           info.areas[0].age == 4 && info.areas[0].field_14 == 5);
    /* (5 + 5) * 4 = 40: column 40 / 8 / 2 = 2, row 40 / 6 / 2 = 3. */
    assert(aoe_ai_info_add_tile_area_ror(&info, 1, 5, 5, 1, 2, 0, 0, 2, 0, &ops) == 2);
    assert(info.cells[2][3][2] == 1 && info.areas[2].x1 == 5 && info.areas[2].y1 == 5 &&
           info.areas[2].id == 2);

    /* Full: the oldest (largest age) is replaced, its ID the end index. */
    assert(aoe_ai_info_add_area_ror(&info, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, &ops) == 1 &&
           info.areas[1].id == 3 && info.areas[1].age == 1);
    info.areas[0].player_9 = 3;
    assert(aoe_ai_info_area_near_ror(&info, 3.5f, 1.5f, 0, 1) &&
           !aoe_ai_info_area_near_ror(&info, 3.5f, 1.5f, 0, 2) &&
           !aoe_ai_info_area_near_ror(&info, 5.0f, 5.0f, 2, 1));
    {
        int c, r, k, total = 0;
        for (c = 0; c < 4; ++c)
            for (r = 0; r < 4; ++r)
                for (k = 0; k < 3; ++k) total += info.cells[c][r][k];
        assert(total == 2);
        /* Row 4 would land past the table: the port does not count it. */
        aoe_ai_info_add_tile_area_ror(&info, 1, 7, 5, 1, 2, 0, 0, 0, 0, &ops);
        for (c = 0, total = 0; c < 4; ++c)
            for (r = 0; r < 4; ++r)
                for (k = 0; k < 3; ++k) total += info.cells[c][r][k];
        assert(total == 2);
    }

    /* Targets. */
    info.target_count[2] = 2;
    info.targets[2] = (AoeAiInfoTargetRor *)calloc(2, sizeof(AoeAiInfoTargetRor));
    info.targets[2][1].id = 33;
    info.current_target[2] = 33;
    assert(aoe_ai_info_set_target_wait_ror(&info, 2, 33, 4) && info.targets[2][1].wait == 4 &&
           !aoe_ai_info_set_target_wait_ror(&info, 2, 34, 4));
    assert(aoe_ai_info_current_target_ror(&info, 2) == 33 &&
           aoe_ai_info_current_target_ror(&info, 4) == -1);

    /* Statistics. */
    assert(aoe_ai_info_stat_bucket_ror(0x23, 0) == 7 &&
           aoe_ai_info_stat_bucket_ror(1, 0x27) == 6 &&
           aoe_ai_info_stat_bucket_ror(1, 0x19) == 9 &&
           aoe_ai_info_stat_bucket_ror(0x16, 1) == 8 &&
           aoe_ai_info_stat_bucket_ror(1, 1) == -1);
    memset(info.stats, 0, sizeof info.stats);
    info.stats[4] = 7;
    info.stats[9] = 7;
    info.stats[2] = 3;
    aoe_ai_info_top_stats_ror(&info, &first, &second);
    assert(first == 4 && second == 9);
    assert(aoe_ai_info_stat_share_ror(&info) == 100);
    info.stats[0] = 1;
    info.history[0] = 3;
    info.history[15999] = 1;
    assert(aoe_ai_info_stat_share_ror(&info) == 75);
    aoe_ai_info_set_field_204_ror(&info, 6);
    info.field_14c = 2;
    assert(info.field_204 == 6 && aoe_ai_info_field_14c_ror(&info) == 2);

    free(info.objects);
    free(info.marks);
    free(info.areas);
    free(info.targets[2]);
    dropsite_tests();
    puts("ai info ok");
    return 0;
}
