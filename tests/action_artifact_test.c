#include "action_artifact.h"

#include <assert.h>
#include <string.h>

/* Patched RoR 1.0a artifact (0x6b), discovery (0x6c) and wonder (0x78)
   actions. */

enum { W = 8, H = 8 };
static AoeLiveUnit *tiles[H][W][4];
static AoeLiveUnit art, wonder, units[6];
static DatUnit art_t, won_t, unit_t[6];
static DatUnitTask task;
static int changes, change_to, creates, create_owner[8], visions[32][4],
    vision_count, events[8], event_count, countdown[9], wins, moves;
static float tick_v = 1.0f;
static uint8_t game_over;

static float tick(void *c) { (void)c; return tick_v; }
static int16_t players(void *c) { (void)c; return 4; }
static int32_t mw(void *c) { (void)c; return W; }
static int32_t mh(void *c) { (void)c; return H; }
static AoeLiveUnit *tile(void *c, int32_t x, int32_t y, uint32_t i) {
    (void)c;
    return i < 4 ? tiles[y][x][i] : NULL;
}
static int start_move(void *c, AoeMoveActionRor *a, const void *g) {
    (void)c; (void)a; (void)g;
    ++moves;
    return 1;
}
static uint8_t sub_update(void *c, AoeMoveActionRor *a) { (void)c; (void)a; return 0; }
static void sub_clear(void *c, AoeMoveActionRor *a) { (void)c; (void)a; }
static const void *graphic(void *c, int16_t id) { (void)c; (void)id; return "g"; }
static int change(void *c, AoeLiveUnit *u, int16_t owner) {
    (void)c;
    u->runtime_owner_index = owner;
    change_to = owner;
    ++changes;
    return 1;
}
static void create(void *c, int16_t owner, int16_t id, float x, float y,
                   float z) {
    (void)c; (void)x; (void)y; (void)z;
    assert(id == 162);
    create_owner[creates++ & 7] = owner;
}
static void vision(void *c, AoeLiveUnit *u, int16_t p, int add, int flag) {
    (void)c; (void)u;
    visions[vision_count & 31][0] = p;
    visions[vision_count & 31][1] = add;
    visions[vision_count & 31][2] = flag;
    ++vision_count;
}
static void event(void *c, uint32_t e, int16_t o, int32_t x, int32_t y) {
    (void)c; (void)o; (void)x; (void)y;
    events[event_count++ & 7] = (int)e;
}
static void cd_set(void *c, int16_t o, int32_t y) { (void)c; countdown[o] = y; }
static void cd_clear(void *c, int16_t o) { (void)c; countdown[o] = -1; }
static void win(void *c, int16_t o) { (void)c; (void)o; ++wins; }

static void place(int i, int owner, int cls, int x, int y) {
    unit_t[i].unit_class = (int16_t)cls;
    units[i].core.unit_template = &unit_t[i];
    units[i].core.state = 2;
    units[i].runtime_owner_index = (int16_t)owner;
    units[i].core.position.x38 = (float)x + 0.5f;
    units[i].core.position.y3c = (float)y + 0.5f;
    for (int k = 0; k < 4; ++k)
        if (!tiles[y][x][k]) {
            tiles[y][x][k] = &units[i];
            return;
        }
}

int main(void) {
    AoeObjectActionOps o;
    AoeArtifactActionRor a;
    AoeDiscoveryActionRor d;
    AoeWonderActionRor w;
    uint8_t r;
    memset(&o, 0, sizeof(o));
    o.tick_seconds = tick; o.player_count = players; o.map_width = mw;
    o.map_height = mh; o.tile_object = tile; o.start_move = start_move;
    o.sub_update = sub_update; o.sub_clear = sub_clear; o.graphic = graphic;
    o.change_owner = change; o.create = create; o.vision = vision;
    o.event = event; o.countdown_set = cd_set; o.countdown_clear = cd_clear;
    o.win = win; o.game_over = &game_over;
    art_t.unit_class = 1;
    art_t.size[0] = art_t.size[1] = 0.35f;
    art.core.unit_template = &art_t;
    art.core.position.x38 = 3.5f;
    art.core.position.y3c = 3.5f;
    art.core.state = 2;
    art.runtime_owner_index = 0;
    tiles[3][3][0] = &art;

    /* Default point (-1, -1): activation waits in 6; the scan covers tiles
       (2..4, 2..4) (3.5 -/+ 1.35). */
    assert(aoe_artifact_action_init_point_ror(&a, &art, &task, -1, -1, -1));
    assert(aoe_artifact_action_activate_ror(&a, 1, &o) && a.base.state == 6);
    /* Nobody near (only Gaia, the artifact itself): wait 3 s. */
    place(0, 0, 4, 4, 4);
    assert(aoe_artifact_action_update_ror(&a, &o, &r) &&
           a.base.state == 0x0a && a.timer == 3.0f);
    /* Player 2 next to it, but also player 3: still contested. */
    place(1, 2, 4, 2, 3);
    place(2, 3, 4, 4, 2);
    for (int i = 0; i < 3; ++i)
        assert(aoe_artifact_action_update_ror(&a, &o, &r));
    assert(a.base.state == 6);
    assert(aoe_artifact_action_update_ror(&a, &o, &r) && a.base.state == 0x0a &&
           changes == 0);
    /* Player 3's unit is a class-11 object or dying: ignored. */
    unit_t[2].unit_class = 0x0b;
    a.base.state = 6;
    assert(aoe_artifact_action_update_ror(&a, &o, &r) && changes == 1 &&
           change_to == 2);
    /* Already the owner: nothing more. */
    assert(aoe_artifact_action_update_ror(&a, &o, &r) && a.base.state == 0x0a &&
           changes == 1);
    /* Out of range (tile 6,6) does not count. */
    unit_t[2].unit_class = 4;
    tiles[2][4][0] = NULL;
    place(3, 3, 4, 6, 6);
    a.base.state = 6;
    assert(aoe_artifact_action_update_ror(&a, &o, &r) && changes == 1);
    /* State 100 returns it to Gaia. */
    a.base.state = 100;
    assert(aoe_artifact_action_update_ror(&a, &o, &r) && change_to == 0 &&
           a.base.state == 6);
    /* A target walk drops the target and waits. */
    assert(aoe_artifact_action_retarget_ror(&a, &units[0], 0, 0, 0, &o) &&
           a.base.state == 6 && !a.base.target && moves == 1 &&
           a.base.x == 4.5f);

    /* Discovery: template 162 for each other player, once. */
    {
        AoeMoveActionOps none;
        memset(&none, 0, sizeof(none));
        o.move = none;
    }
    art.runtime_owner_index = 0;
    assert(aoe_discovery_action_init_ror(&d, &art, &task, NULL, &o) &&
           d.found_count == 4);
    assert(aoe_discovery_action_activate_ror(&d, 1) && d.base.state == 6);
    assert(aoe_discovery_action_update_ror(&d, &o, &r) && creates == 1 &&
           create_owner[0] == 2 && d.base.state == 0x0a && d.timer == 3.0f);
    tiles[2][4][0] = &units[2];
    for (int i = 0; i < 4; ++i)
        assert(aoe_discovery_action_update_ror(&d, &o, &r));
    assert(creates == 2 && create_owner[1] == 3);
    aoe_discovery_action_free_ror(&d);

    /* Wonder: foundation visible to others (2), built (6) starts 2000. */
    won_t.id = 276;
    wonder.core.unit_template = &won_t;
    wonder.runtime_owner_index = 1;
    wonder.core.state = 0;
    assert(aoe_wonder_action_init_ror(&w, &wonder));
    event_count = 0;
    assert(aoe_wonder_action_activate_ror(&w, 1, &o) && w.base.state == 2 &&
           vision_count == 2 && visions[0][0] == 2 && visions[0][1] == 1 &&
           visions[0][2] == 1 && events[0] == 0x6c);
    assert(aoe_wonder_action_update_ror(&w, &o, &r) && w.base.state == 2);
    wonder.core.state = 2;
    vision_count = 0;
    assert(aoe_wonder_action_update_ror(&w, &o, &r) && w.base.state == 6 &&
           vision_count == 4 && visions[0][1] == 0 && visions[1][1] == 1 &&
           visions[1][2] == 0 && countdown[1] == 2000 && events[1] == 0x6d);
    /* Two years per second. */
    tick_v = 0.5f;
    assert(aoe_wonder_action_update_ror(&w, &o, &r) && w.timer == 1.0f &&
           countdown[1] == 1999);
    w.timer = 1999.5f;
    assert(aoe_wonder_action_update_ror(&w, &o, &r) && wins == 1 &&
           game_over == 100);
    /* Destroyed with another wonder standing: restart at 2000. */
    countdown[1] = 5;
    event_count = 0;
    assert(aoe_wonder_action_destroy_ror(&w, &o, 1) && countdown[1] == 2000 &&
           events[0] == 0x6e);
    assert(aoe_wonder_action_destroy_ror(&w, &o, 0) && countdown[1] == -1);
    /* Hidden countdown: never wins. */
    o.countdown_disabled = 1;
    w.timer = 1999.9f;
    assert(aoe_wonder_action_update_ror(&w, &o, &r) && wins == 1);
    return 0;
}
