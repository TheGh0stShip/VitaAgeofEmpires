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
static int object(void *c, int32_t id, AoeAiWorldObjectRor *o) {
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
static int object2(void *c, int32_t id, AoeAiWorldObjectRor *o) {
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
    AoeAiWorldObjectRor o;
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
    AoeAiWorldObjectRor r, g;
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

/* World three for sightings: 500 our dock, 501 an enemy cavalry (class
   0xc), 502 a tree (class 0xf) at (4,4), 503 an enemy type 0x9f, 504 a
   class-1 object, 505 class 0x22 (unknown), 506 our granary (0x67, class
   3), 507 our type 0x32 (class 3). */
static int32_t shared_to[8], shared_n;
static uint8_t penalty[16][16];
static int32_t influence_log[16][7], influence_n;
static int object3(void *c, int32_t id, AoeAiWorldObjectRor *o) {
    (void)c;
    memset(o, 0, sizeof *o);
    o->id = id;
    o->state = 2;
    o->owner = 1;
    switch (id) {
    case 500: o->type_id = 0x2d; o->type_class = 3; return 1;
    case 501: o->type_class = 0xc; o->owner = 2; o->x = 5.5f; o->y = 5.5f; o->type_28 = 1.5f; return 1;
    case 502: o->type_class = 0xf; o->owner = 0; o->x = 4.0f; o->y = 4.0f; return 1;
    case 503: o->type_id = 0x9f; o->type_class = 0x16; o->owner = 2; return 1;
    case 504: o->type_class = 1; return 1;
    case 505: o->type_class = 0x22; return 1;
    case 506: o->type_id = 0x67; o->type_class = 3; o->x = 4.0f; o->y = 6.0f; return 1;
    case 507: o->type_id = 0x32; o->type_class = 3; o->x = 2.0f; o->y = 2.0f; return 1;
    default: return 0;
    }
}
static int32_t sn3(void *c, int32_t i) { (void)c; return i == 0xc2 || i == 0x46 || i == 0x37 ? 1 : 0; }
static int32_t field4(void *c, int32_t p) { (void)c; return p != 3; }
static int names_ally(void *c, int32_t p) { (void)c; return p == 4 || p == 3; }
static int ally3(void *c, int32_t p) { (void)c; return p == 1 || p >= 3; }
static void share(void *c, int32_t p, int32_t id) { (void)c; (void)id; shared_to[shared_n++] = p; }
static uint8_t pen_get(void *c, int32_t x, int32_t y) { (void)c; return penalty[x][y]; }
static void pen_set(void *c, int32_t x, int32_t y, uint8_t v) { (void)c; penalty[x][y] = v; }
static void inf_clear(void *c, int m) { (void)c; influence_log[influence_n][0] = 100 + m; ++influence_n; }
static void inf_origin(void *c, int m, int32_t x, int32_t y) {
    (void)c; influence_log[influence_n][0] = 200 + m; influence_log[influence_n][1] = x;
    influence_log[influence_n][2] = y; ++influence_n;
}
static void inf_add(void *c, int m, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t v) {
    int32_t *l = influence_log[influence_n++];
    (void)c; l[0] = 300 + m; l[1] = x0; l[2] = y0; l[3] = x1; l[4] = y1; l[5] = v;
}
static void inf_finish(void *c, int m) { (void)c; influence_log[influence_n][0] = 400 + m; ++influence_n; }
static double v108_3(void *c, int32_t id) { (void)c; return id == 501 ? 2.0 : 0.0; }
static double range3(void *c, int32_t id) { (void)c; return id == 501 ? 2.5 : 0.5; }

static void sighting_tests(void) {
    static AoeAiInfoRor info;
    AoeAiInfoOpsRor ops;
    AoeAiWorldObjectRor o;
    int k;

    memset(&ops, 0, sizeof ops);
    ops.self = 1;
    ops.object = object3;
    ops.world_object = object3;
    ops.zone = zone2;
    ops.tile_zone = tile_zone2;
    ops.is_enemy = is_enemy;
    ops.is_ally = ally3;
    ops.sn = sn3;
    ops.player_count = 5;
    ops.player_field_4 = field4;
    ops.names_us_ally = names_ally;
    ops.share = share;
    ops.value_108 = v108_3;
    ops.range = range3;
    ops.penalty_get = pen_get;
    ops.penalty_set = pen_set;
    ops.influence_clear = inf_clear;
    ops.influence_origin = inf_origin;
    ops.influence_add = inf_add;
    ops.influence_finish = inf_finish;
    info.map_width = info.map_height = 16;
    info.object_count = 4;
    info.objects = (AoeAiInfoObjectRor *)calloc(4, sizeof *info.objects);
    for (k = 0; k < 4; ++k) info.objects[k].id = -1;
    for (k = 0; k < 4; ++k) info.best_distance[k] = -1;

    /* An enemy cavalry: shared with player 4 only (3 has +4 clear, 2 is
       the owner and not an ally), listed as a unit and remembered. */
    object3(NULL, 501, &o);
    assert(aoe_ai_info_notice_ror(&info, &o, 0, &ops) == 1);
    assert(shared_n == 1 && shared_to[0] == 4);
    assert(info.list_118.count == 1 && info.list_128.count == 1 &&
           info.objects[0].id == 501 && info.objects[0].range == 2.5f &&
           info.objects[0].field_18 == 2.0f && info.list_148.count == 0);
    assert(aoe_ai_info_notice_ror(&info, &o, 1, &ops) == 1 && shared_n == 1 &&
           info.list_118.count == 1);
    /* Unknown class: nothing. */
    assert(!aoe_ai_info_notice_id_ror(&info, 505, &ops) && !aoe_ai_info_notice_id_ror(&info, 9, &ops));
    /* Class 1 goes on the +0x138 list, not remembered. */
    assert(aoe_ai_info_notice_id_ror(&info, 504, &ops) && info.list_138.count == 1 &&
           info.objects[1].id == -1);
    /* 0x9f with number 55 set: special, remembered. */
    assert(aoe_ai_info_notice_id_ror(&info, 503, &ops) && info.list_148.count == 1 &&
           info.list_148.data[0] == 503 && info.objects[1].id == 503);
    /* Our dock with number 70 set. */
    object3(NULL, 500, &o);
    assert(aoe_ai_info_note_special_ror(&info, &o, &ops) == 1 && info.list_148.count == 2);
    assert(aoe_ai_info_note_special_ror(&info, &o, &ops) == 1 && info.list_148.count == 2);
    o.owner = 2;
    assert(aoe_ai_info_note_special_ror(&info, &o, &ops) == 0);

    /* A tree: a new target in list 1 (capacity 100) with no dropsite yet. */
    assert(aoe_ai_info_notice_id_ror(&info, 502, &ops));
    assert(info.target_capacity[1] == 100 && info.target_count[1] == 1 &&
           info.targets[1][0].id == 502 && info.targets[1][0].x == 4 &&
           info.targets[1][0].pad_f == 1 && info.targets[1][0].field_10 == -1 &&
           info.targets[1][0].field_8 == 2010 && info.targets[1][1].id == -1 &&
           info.targets[1][1].field_8 == 0xff && info.best_distance[1] == 1000);
    /* Our granary appears: dropsite list, targets refreshed (d = 2). */
    object3(NULL, 506, &o);
    aoe_ai_info_own_object_ror(&info, &o, &ops);
    assert(info.list_180.count == 1 && info.list_190.count == 1 &&
           info.targets[1][0].field_10 == 506 && info.targets[1][0].field_8 == 14 &&
           info.best_distance[1] == 2 && info.current_target[1] == 502);
    /* Seeing the tree again keeps one entry and refreshes +8. */
    info.targets[1][0].field_8 = 0;
    object3(NULL, 502, &o);
    assert(aoe_ai_info_add_target_ror(&info, &o, &ops) && info.target_count[1] == 1 &&
           info.targets[1][0].field_8 == 14);
    /* Our type 0x32 becomes a target in list 0. */
    object3(NULL, 507, &o);
    aoe_ai_info_own_object_ror(&info, &o, &ops);
    assert(info.target_count[0] == 1 && info.targets[0][0].id == 507 &&
           info.list_190.count == 2);
    /* Losing the granary. */
    info.objects[2].id = 506;
    info.objects[2].owner = 7;
    aoe_ai_info_lose_object_ror(&info, 506, &ops);
    assert(info.list_190.count == 1 && info.list_190.data[0] == 507 &&
           info.list_180.count == 1 && info.objects[2].owner == 1);

    /* Penalties: 501 (seen as 2) at (5,5) with type +0x28 1.5 and range
       2.5: the 4..7 square twice, the 3..8 square once more. */
    aoe_ai_info_penalize_objects_ror(&info, 2, -1, &ops);
    assert(penalty[5][5] == 20 && penalty[4][7] == 20 && penalty[3][3] == 10 &&
           penalty[8][8] == 10 && penalty[9][9] == 0 && penalty[2][5] == 0);
    memset(penalty, 0xfa, sizeof penalty);
    aoe_ai_info_penalize_objects_ror(&info, 2, 501, &ops);
    assert(penalty[5][5] == 0xfa);
    aoe_ai_info_penalize_objects_ror(&info, 2, -1, &ops);
    assert(penalty[5][5] == 0xff && penalty[3][3] == 0xff);
    memset(penalty, 0, sizeof penalty);
    info.area_count = 2;
    info.areas = (AoeAiInfoAreaRor *)calloc(2, sizeof *info.areas);
    info.areas[0] = (AoeAiInfoAreaRor){1, 0, 6, 6, 7, 7, 1, 2, 0, 0, 0, 0, 0, 0};
    info.areas[1] = (AoeAiInfoAreaRor){-1, 0, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0};
    aoe_ai_info_penalize_areas_ror(&info, 2, &ops);
    assert(penalty[6][6] == 0xf && penalty[1][1] == 0xf && penalty[12][12] == 0xf &&
           penalty[13][13] == 0 && penalty[0][0] == 0);

    /* Influence maps. */
    aoe_ai_info_unit_influence_ror(&info, 2, 3, 4, &ops);
    assert(influence_n == 5 && influence_log[0][0] == 100 && influence_log[1][0] == 200 &&
           influence_log[1][1] == 3 && influence_log[2][0] == 300 &&
           influence_log[2][1] == 4 && influence_log[2][3] == 6 && influence_log[2][5] == 5 &&
           influence_log[3][0] == 300 && influence_log[3][5] == 1 && influence_log[4][0] == 400);
    influence_n = 0;
    aoe_ai_info_area_influence_ror(&info, 2, 0, 0, &ops);
    /* The area pass has no ID test: the free area counts too. */
    assert(influence_n == 5 && influence_log[2][0] == 301 && influence_log[2][1] == 6 &&
           influence_log[3][1] == 1 && influence_log[3][0] == 301 && influence_log[2][5] == 1);

    for (k = 0; k < 4; ++k) free(info.targets[k]);
    free(info.objects);
    free(info.areas);
    aoe_ai_int_list_release_ror(&info.list_118);
    aoe_ai_int_list_release_ror(&info.list_128);
    aoe_ai_int_list_release_ror(&info.list_138);
    aoe_ai_int_list_release_ror(&info.list_148);
    aoe_ai_int_list_release_ror(&info.list_180);
    aoe_ai_int_list_release_ror(&info.list_190);
}

/* World four: town centre 600 at (0,0); towers 601 (type 0x9e at 4,0),
   602 (0xa3 at 8,0), 603 (0x9f at 3,4, explored), 604 (0x9f unexplored),
   all owned by 2; a berry bush 605 target in zone 2. */
static int center_exists = 1;
static int object4(void *c, int32_t id, AoeAiWorldObjectRor *o) {
    (void)c;
    memset(o, 0, sizeof *o);
    o->id = id;
    o->state = 2;
    o->owner = 2;
    switch (id) {
    case 600: o->type_id = 0x6d; return center_exists;
    case 601: o->type_id = 0x9e; o->x = 4.0f; return 1;
    case 602: o->type_id = 0xa3; o->x = 8.0f; return 1;
    case 603: o->type_id = 0x9f; o->x = 3.2f; o->y = 4.7f; o->z = 1.0f; return 1;
    case 604: o->type_id = 0x9f; o->x = 15.0f; o->y = 15.0f; return 1;
    case 605: o->type_class = 7; o->x = 45.0f; return 1;
    case 606: o->type_class = 0xf; o->x = 3.0f; return 1;
    default: return 0;
    }
}
static int32_t first4(void *c, int32_t t) { (void)c; return t == 0x6d && center_exists ? 600 : -1; }
static uint8_t vision4(void *c, int32_t x, int32_t y) { (void)c; (void)y; return (uint8_t)(x < 10 ? 0x80 : 0); }
static double dist4(void *c, int32_t id, float x, float y, float z) {
    (void)c; (void)id; (void)z; return sqrt((double)x * x + (double)y * y);
}
static double odist4(void *c, int32_t a, int32_t b) {
    AoeAiWorldObjectRor o;
    (void)a; object4(c, b, &o); return sqrt((double)o.x * o.x + (double)o.y * o.y);
}
static float age4 = 2.9f;
static float res4(void *c, int32_t i) { (void)c; return i == 6 ? age4 : 0.0f; }
static int granary4 = 1;
static int build4(void *c, int32_t t, int32_t f) { (void)c; return t == 0x67 && f == 0 && granary4; }
static int32_t sn4(void *c, int32_t i) { (void)c; return i >= 0xa3 && i <= 0xa6 ? 10 + (i - 0xa3) : 0; }
static int32_t tile_objs(void *c, int32_t x, int32_t y, int32_t *ids, int32_t max) {
    (void)c; (void)max;
    if (x == 1 && y == 1) { ids[0] = 606; ids[1] = 601; return 2; }
    if (x == 2 && y == 0) { ids[0] = 605; return 1; }
    return 0;
}
static uint8_t terrain4(void *c, int32_t x, int32_t y) { (void)c; return (uint8_t)(x == 1 && y == 1 ? 0x2a : 0); }

static void choice_tests(void) {
    static AoeAiInfoRor info;
    AoeAiInfoOpsRor ops;
    int k;

    memset(&ops, 0, sizeof ops);
    ops.self = 1;
    ops.object = object4;
    ops.world_object = object4;
    ops.is_enemy = is_enemy;
    ops.first_of_type = first4;
    ops.vision = vision4;
    ops.distance = dist4;
    ops.object_distance = odist4;
    ops.resource = res4;
    ops.build_has = build4;
    ops.sn = sn4;
    ops.zone = zone2;
    ops.tile_zone = tile_zone2;
    ops.tile_objects = tile_objs;
    ops.tile_byte5 = terrain4;
    ops.grid_set = grid_set;
    info.object_count = 6;
    info.objects = (AoeAiInfoObjectRor *)calloc(6, sizeof *info.objects);
    info.objects[0] = (AoeAiInfoObjectRor){601, 0x9e, 0, 4, 0, 0, 2, 0, 0, 0, 0, 0, 0};
    info.objects[1] = (AoeAiInfoObjectRor){602, 0xa3, 0, 8, 0, 0, 2, 0, 0, 0, 0, 0, 0};
    info.objects[2] = (AoeAiInfoObjectRor){603, 0x9f, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0};
    info.objects[3] = (AoeAiInfoObjectRor){604, 0x9f, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0};
    info.objects[4] = (AoeAiInfoObjectRor){-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    info.objects[5] = (AoeAiInfoObjectRor){-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    /* Objects seen as 2 within 5 tiles: 601 at (4,0) counts all four (the
       0x9f entries sit at 0,0 for now), 602 at (8,0) counts two. Times
       the distance from the centre at (0,0): 16 and 16, the first wins. */
    assert(aoe_ai_info_best_9e_ror(&info, 2, &ops) == &info.objects[0]);
    center_exists = 0;
    /* Without a centre the counts decide: 602. */
    assert(aoe_ai_info_best_9e_ror(&info, 2, &ops) == &info.objects[1]);
    center_exists = 1;
    assert(!aoe_ai_info_best_9e_ror(&info, 3, &ops));
    /* 0x9f: 604 is unexplored; 603 is refreshed from its object. */
    assert(aoe_ai_info_best_9f_ror(&info, 0xffffffffu, &ops) == &info.objects[2] &&
           info.objects[2].x == 3 && info.objects[2].y == 4 && info.objects[2].z == 1 &&
           info.objects[3].x == 0);

    /* Resource needs. */
    for (k = 0; k < 4; ++k) info.best_distance[k] = -1;
    assert(aoe_ai_info_resource_need_ror(&info, 2, &ops) == 0);
    assert(aoe_ai_info_resource_need_ror(&info, 1, &ops) == -1);
    info.best_distance[0] = 16;
    assert(aoe_ai_info_resource_need_ror(&info, 1, &ops) == 2 &&
           aoe_ai_info_resource_need_ror(&info, 0, &ops) == -1);
    info.best_distance[3] = 14;
    assert(aoe_ai_info_resource_need_ror(&info, 1, &ops) == 3);
    info.best_distance[1] = 12;
    assert(aoe_ai_info_resource_need_ror(&info, 1, &ops) == 1);
    info.best_distance[1] = 5;
    granary4 = 0;
    assert(aoe_ai_info_resource_need_ror(&info, 0, &ops) == 1);
    granary4 = 1;

    /* Far targets: age 2.9 -> 1; list 0 needs age_allowed[0][1]. */
    info.best_distance[0] = 16;
    info.current_target[0] = 606;
    assert(!aoe_ai_info_target_far_ror(&info, 0, &ops));
    info.age_allowed[0][1] = 1;
    assert(aoe_ai_info_target_far_ror(&info, 0, &ops) && aoe_ai_info_food_far_ror(&info, &ops));
    info.current_target[0] = 605; /* another zone */
    assert(!aoe_ai_info_target_far_ror(&info, 0, &ops) && !aoe_ai_info_food_far_ror(&info, &ops));
    center_exists = 0;
    assert(aoe_ai_info_target_far_ror(&info, 0, &ops));
    center_exists = 1;
    info.best_distance[0] = 10;
    info.current_target[0] = 606;
    assert(!aoe_ai_info_target_far_ror(&info, 0, &ops) && !aoe_ai_info_food_far_ror(&info, &ops));
    age4 = 9.0f;
    info.best_distance[2] = 50;
    info.age_allowed[2][3] = 1;
    info.current_target[2] = 606;
    assert(aoe_ai_info_target_far_ror(&info, 2, &ops));

    /* Map scan over 4x4 tiles. */
    info.map_width = info.map_height = 4;
    memset(info.cells, 0, sizeof info.cells);
    memset(grid, 0xff, sizeof grid);
    aoe_ai_info_scan_map_ror(&info, &ops);
    assert(info.cells[0][0][0] == 1 && info.cells[3][3][0] == 1 && grid[3][3] == 0 &&
           info.field_560 == 1 && info.list_118.count == 3 && info.list_128.count == 1);
    aoe_ai_int_list_release_ror(&info.list_128);

    free(info.objects);
    for (k = 0; k < 4; ++k) free(info.targets[k]);
    aoe_ai_int_list_release_ror(&info.list_118);
    aoe_ai_int_list_release_ror(&info.list_148);
}

static char read_path[64];
static int read_file5(void *c, const char *path, void *out, int size) {
    uint8_t *b = (uint8_t *)out;
    int i;
    (void)c;
    snprintf(read_path, sizeof read_path, "%s", path);
    for (i = 0; i < size; ++i) b[i] = (uint8_t)(i % 4 == 0 ? i / 4 + 1 : 0);
    return 1;
}
static const char *name5;
static const char *learn_name5(void *c) { (void)c; return name5; }
static int reach5 = 1;
static float reach_radius;
static int32_t reach_z;
static int can_reach5(void *c, int32_t id, int32_t x, int32_t y, int32_t z, float r) {
    (void)c; (void)id; (void)x; (void)y; reach_z = z; reach_radius = r; return reach5;
}
static int object5(void *c, int32_t id, AoeAiWorldObjectRor *o) {
    (void)c;
    memset(o, 0, sizeof *o);
    o->id = id; o->z = 2.7f; o->type_28 = 3.0f;
    return id == 700;
}
static double dist5(void *c, int32_t id, float x, float y, float z) {
    (void)c; (void)id; (void)z; return (double)x + (double)y;
}
static int32_t sn5(void *c, int32_t i) { (void)c; return i == 0xca ? 3 : 0; }

static void mark_tests(void) {
    static AoeAiInfoRor info;
    AoeAiInfoOpsRor ops;
    int32_t *lists[2] = {NULL, NULL}, counts[2] = {0, 0}, out[2];

    memset(&ops, 0, sizeof ops);
    ops.self = 1;
    ops.object = object5;
    ops.zone = zone2;
    ops.tile_zone = tile_zone2;
    ops.distance = dist5;
    ops.can_reach = can_reach5;
    ops.grid_get = grid_get;
    ops.grid_set = grid_set;
    ops.is_enemy = is_enemy;
    ops.sn = sn5;
    ops.read_file = read_file5;
    ops.learn_name = learn_name5;

    info.kind_lists = lists;
    info.kind_counts = counts;
    aoe_ai_info_kind_append_ror(&info, 1, 7);
    aoe_ai_info_kind_append_ror(&info, 1, 8);
    assert(counts[1] == 2 && lists[1][1] == 8 && counts[0] == 0);
    free(lists[1]);

    assert(aoe_ai_info_site_priority_ror(0x6d) == 9 && aoe_ai_info_site_priority_ror(199) == 3 &&
           aoe_ai_info_site_priority_ror(0x114) == 6 && aoe_ai_info_site_priority_ror(1) == 5);

    aoe_ai_info_put_mark_ror(&info, 5, 1, 1, 0);
    assert(info.mark_count == 0);
    info.mark_count = 1;
    info.marks = (AoeAiInfoMarkRor *)calloc(1, sizeof *info.marks);
    info.marks[0].id = -1;
    aoe_ai_info_put_mark_ror(&info, 5, 1, 1, 0);
    assert(info.marks[0].id == 5);
    aoe_ai_info_put_mark_ror(&info, 5, 1, 1, 2);
    assert(info.marks[0].state == 2 && info.mark_count == 1);
    aoe_ai_info_put_mark_ror(&info, 6, 2, 2, 1);
    assert(info.mark_count == 2 && info.marks[1].id == 6 && info.marks[1].state == 1);
    aoe_ai_info_put_mark_ror(&info, 7, 3, 3, 4);
    assert(info.mark_count == 4 && info.marks[2].id == 7 && info.marks[3].id == -1 &&
           info.marks[3].state == 4);
    free(info.marks);

    /* Exploring: entries 1 (ours), 2 (unexplored, x+y 9), 3 (unexplored,
       x+y 5), 4 (explored), 5 (other zone). */
    info.map_width = info.map_height = 8;
    memset(grid, 0xff, sizeof grid);
    info.object_count = 5;
    info.objects = (AoeAiInfoObjectRor *)calloc(5, sizeof *info.objects);
    info.objects[0] = (AoeAiInfoObjectRor){1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0};
    info.objects[1] = (AoeAiInfoObjectRor){2, 0, 0, 4, 5, 0, 2, 0, 0, 0, 0, 0, 0};
    info.objects[2] = (AoeAiInfoObjectRor){3, 0, 0, 2, 3, 0, 2, 0, 0, 0, 0, 0, 0};
    info.objects[3] = (AoeAiInfoObjectRor){4, 0, 0, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0};
    info.objects[4] = (AoeAiInfoObjectRor){5, 0x6d, 0, 50, 0, 0, 2, 0, 0, 0, 0, 0, 0};
    grid[1][2] = 0;
    assert(aoe_ai_info_explore_toward_ror(&info, 700, out, &ops) == 1 &&
           out[0] == 2 && out[1] == 3 && grid[2][3] == 0 && reach_z == 2 &&
           reach_radius == 2.0f);
    reach5 = 0;
    grid[2][3] = 0xff;
    assert(aoe_ai_info_explore_toward_ror(&info, 700, out, &ops) == 0 && grid[2][3] == 1);
    assert(aoe_ai_info_explore_toward_ror(&info, 700, out, &ops) == 0 &&
           out[0] == 4 && grid[4][5] == 1);
    assert(!aoe_ai_info_explore_toward_ror(&info, 701, out, &ops));

    /* Enemy buildings: entry 5 (0x6d) at (50, 0), radius 3. */
    assert(aoe_ai_info_clear_of_enemy_ror(&info, 47, 0, &ops) == 0 &&
           aoe_ai_info_clear_of_enemy_ror(&info, 46, 0, &ops) == 1);
    free(info.objects);

    aoe_ai_info_load_learning_ror(&info, &ops);
    assert(!strcmp(read_path, "learn\\Default.uh") && info.stats[0] == 1 && info.stats[10] == 11);
    name5 = "Ann";
    aoe_ai_info_load_learning_ror(&info, &ops);
    assert(!strcmp(read_path, "learn\\Ann.uh"));
}

int main(void) {
    static AoeAiInfoRor info;
    AoeAiInfoOpsRor ops;
    AoeAiWorldObjectRor seen;
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
    info.list_148.count = 2;
    assert(info.field_204 == 6 && aoe_ai_info_list_148_count_ror(&info) == 2);

    free(info.objects);
    free(info.marks);
    free(info.areas);
    free(info.targets[2]);
    dropsite_tests();
    sighting_tests();
    choice_tests();
    mark_tests();
    puts("ai info ok");
    return 0;
}
