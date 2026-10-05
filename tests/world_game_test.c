#include "world_game.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

typedef struct { int id, color, updates, started, stance[9], da40, saved, loaded, won; } P;

static uint32_t clock_ms;
static int32_t turn, rng = 100, seeded, victories, removed;
static int stype[9], steam[9], sp, flag710, flag720, scen_which = -1;
static uint8_t buf[4096];
static AoeOutcomeStream st;
static int sessions, ok_close = 1;
static AoeWorldBaseRor world;

static uint32_t now_ms(void *c) { (void)c; return clock_ms; }
static void srand_(void *c, int32_t s) { (void)c; seeded = s; }
static int32_t rand_(void *c) { (void)c; return ++rng; }
static int32_t comm_turn(void *c, int32_t t) { (void)c; (void)t; return turn; }
static void comm_seed(void *c, int32_t s) { (void)c; (void)s; }
static int32_t comm_new_seed(void *c) { (void)c; return 77; }
static int32_t slot_type(void *c, int32_t i) { (void)c; return stype[i]; }
static void none(void *c) { (void)c; }
static void perf(void *c, int32_t id, uint32_t ms) { (void)c; (void)id; (void)ms; }
static void log_(void *c, const char *t) { (void)c; (void)t; }
static int session(void *c) { (void)c; return sessions; }
static int single(void *c) { (void)c; return sp; }
static int f710(void *c) { (void)c; return flag710; }
static int f720(void *c) { (void)c; return flag720; }
static int32_t team(void *c, int32_t i) { (void)c; return steam[i]; }
static int32_t slot_player(void *c, int32_t i) { (void)c; return i; }
static float speed(void *c) { (void)c; return 2.0f; }
static int32_t fixed_seed(void *c) { (void)c; return -1; }
static void used_seed(void *c, int32_t s) { (void)c; (void)s; }
static int32_t def100(void *c) { (void)c; return 5; }
static void camp_restore(void *c, int had, int32_t a, int32_t b, int32_t d) { (void)c; (void)had; (void)a; (void)b; (void)d; }
static void camp_read(void *c, int32_t *a, int32_t *b, int32_t *d) { (void)c; *a = 1; *b = 2; *d = 3; }
static void pupd(void *c, void *p) { (void)c; ((P *)p)->updates++; }
static void pnone(void *c, void *p) { (void)c; (void)p; }
static void pstart(void *c, void *p) { (void)c; ((P *)p)->started++; }
static void p2c(void *c, void *p, const char *t, int16_t v) { (void)c; (void)p; (void)t; (void)v; }
static void pcolor(void *c, void *p, int32_t col) { (void)c; ((P *)p)->color = col; }
static void pstance(void *c, void *p, int32_t o, int32_t s) { (void)c; ((P *)p)->stance[o] = s + 10; }
static void pda40(void *c, void *p, int32_t v) { (void)c; ((P *)p)->da40 = v; }
/* +0x84 writes the presence byte the loader reads, +0x8c the body;
   +0x88, +0x64 and +0x6c write one marker byte except +0x88. */
static void psave(void *c, void *p, int slot, AoeOutcomeStream *s) {
    (void)c; (void)p;
    if (slot == 0x84) aoe_stream_put_u8(s, 0);
    else if (slot != 0x88) aoe_stream_put_u8(s, (uint8_t)slot);
}
static void pbody(void *c, void *p, AoeOutcomeStream *s) { (void)c; ((P *)p)->loaded = aoe_stream_u8(s); }
static void plog(void *c, void *p, FILE *f, int first) { (void)c; (void)p; (void)f; (void)first; }
static int pwon(void *c, void *p) { (void)c; return ((P *)p)->won; }
static int32_t pobjs(void *c, void *p, int list) { (void)c; (void)p; return list == 0 ? 2 : 1; }
static void pobj(void *c, void *p, int list, int32_t i, AoeWorldSceneObjectRor *o) {
    (void)c; (void)p;
    memset(o, 0, sizeof *o);
    o->type = (list == 0 && i == 1) ? 0x19 : 0x46;
    o->id = list * 10 + i;
    o->angle = 1.5f;
}
static void map_new(void *c, int16_t w, int16_t h) { (void)c; (void)w; (void)h; }
static void map_info(void *c, uint8_t f, int32_t v) { (void)c; (void)f; (void)v; }
static void map_io(void *c, AoeOutcomeStream *s) { (void)c; aoe_stream_put_u8(s, 0xaa); }
static void map_load(void *c, AoeOutcomeStream *s) { (void)c; assert(aoe_stream_u8(s) == 0xaa); }
static void on518(void *c, int on) { (void)c; (void)on; }
static void scen_save(void *c, void *sc, AoeOutcomeStream *s) { (void)c; (void)sc; aoe_stream_put_u8(s, 0x5c); }
static void info_header(void *c, AoeOutcomeStream *s) { (void)c; aoe_stream_put_u8(s, 0x17); }
static AoeOutcomeStream *camp_stream(void *c) {
    (void)c;
    st.data = buf; st.size = 8; st.pos = 0; st.failed = 0;
    memcpy(buf, "1.03", 4);
    buf[4] = 2; buf[5] = buf[6] = buf[7] = 0;
    st.size = 10;
    return &st;
}
static AoeOutcomeStream *open_read(void *c, const char *path) {
    (void)c;
    if (strstr(path, "missing")) return NULL;
    st.pos = 0; st.failed = 0;
    return &st;
}
static void close_read(void *c, AoeOutcomeStream *s) { (void)c; (void)s; }
static AoeOutcomeStream *open_write(void *c, const char *path) {
    (void)c; (void)path;
    st.data = buf; st.size = sizeof buf; st.pos = 0; st.failed = 0;
    return &st;
}
static int close_write(void *c, AoeOutcomeStream *s) { (void)c; st.size = s->pos; return ok_close ? 0 : 1; }
static void remove_file(void *c, const char *p) { (void)c; (void)p; removed++; }
static const char *ai_name(void *c) { (void)c; return "x"; }
static FILE *open_append(void *c, const char *p) { (void)c; (void)p; return NULL; }
static void string(void *c, int32_t id, char *o, int32_t n) { (void)c; (void)id; snprintf(o, (size_t)n, "s"); }

static AoeWorldGameOpsRor ops = {
    NULL, now_ms, srand_, rand_, comm_turn, comm_seed, comm_new_seed, slot_type, none, none, perf,
    log_, session, single, f710, f720, team, slot_player, speed, fixed_seed, used_seed, def100,
    camp_restore, camp_read, 0, 0, NULL,
    pupd, pnone, pnone, pnone, pstart, p2c, pcolor, pstance, pda40, psave, pbody, plog, pwon,
    pobjs, pobj,
    map_new, map_info, map_load, map_io, map_io, none, none, on518, scen_save, info_header,
    camp_stream, none,
    "save\\", "scen\\", open_read, close_read, open_write, close_write, remove_file, ai_name,
    open_append, string};

static P players[9];
static void v_gaia(AoeWorldBaseRor *w) { w->players[0] = &players[0]; }
static void v_players(AoeWorldBaseRor *w, const AoeWorldGameInfoRor *info) {
    int i;
    (void)info;
    for (i = 1; i < w->player_count; ++i) w->players[i] = &players[i];
}
static int v_random(AoeWorldBaseRor *w, const AoeWorldGameInfoRor *i) { (void)w; (void)i; return 1; }
static AoeWorldGameRor *gp;
static void v_save(AoeWorldBaseRor *w, AoeOutcomeStream *s) { (void)w; aoe_world_save_state_ror(gp, s); }
static int v_campaign(AoeWorldBaseRor *w, const AoeWorldGameInfoRor *i) { (void)w; return aoe_world_campaign_game_ror(gp, i); }
static int v_scenario(AoeWorldBaseRor *w, const char *n, const AoeWorldGameInfoRor *i) { (void)w; return aoe_world_scenario_game_ror(gp, n, i); }
static void v_body(AoeWorldBaseRor *w, int which, AoeOutcomeStream *s, const AoeWorldGameInfoRor *i) { (void)w; (void)s; (void)i; scen_which = which; }
static void v_header(AoeWorldBaseRor *w, FILE *f, int first) { (void)w; (void)f; (void)first; }
static void v_colors(AoeWorldBaseRor *w, const AoeWorldGameInfoRor *i) { (void)w; (void)i; }
static void v_pfs(AoeWorldBaseRor *w, AoeOutcomeStream *s, uint8_t flag, int16_t index) { (void)s; (void)flag; w->players[index] = &players[index]; }
static void v_victory(AoeWorldBaseRor *w) { (void)w; victories++; }
static int v_load(AoeWorldBaseRor *w, AoeOutcomeStream *s) { (void)w; return aoe_world_load_state_ror(gp, s); }
static void v_sfs(AoeWorldBaseRor *w, AoeOutcomeStream *s) { (void)w; assert(aoe_stream_u8(s) == 0x5c); }
static void v_snew(AoeWorldBaseRor *w, const AoeWorldGameInfoRor *i) { (void)w; (void)i; }
static void v_gen(AoeWorldBaseRor *w, const AoeWorldGameInfoRor *i, int32_t last) { (void)w; (void)i; (void)last; }
static const AoeWorldGameVtblRor vt = {v_gaia, v_players, v_random, v_save, v_campaign, v_scenario,
                                        v_body, v_header, v_colors, v_pfs, v_victory, v_load,
                                        v_sfs, v_snew, v_gen};

/* World ops: only the lists and the sound/sprite idles are reached. */
static void *list_new(void *c) { (void)c; return NULL; }
static void list_x(void *c, void *l) { (void)c; (void)l; }
static void idle(void *c, void *o, uint32_t t) { (void)c; (void)o; (void)t; }
static AoeWorldOpsRor wops;

int main(void) {
    AoeWorldGameRor g;
    AoeWorldGameInfoRor info;
    AoeWorldColorRor colors[10];
    AoeWorldColorRor *cp[10];
    int i;

    wops.list_new = list_new;
    wops.list_clear = list_x;
    wops.list_free = list_x;
    wops.sound_idle = idle;
    wops.sprite_idle = idle;
    wops.player_units_68 = list_x;
    wops.player_free = list_x;
    wops.sound_stop = list_x;
    memset(&g, 0, sizeof g);
    gp = &g;
    g.w = &world;
    g.ops = &ops;
    g.vtbl = &vt;
    assert(aoe_world_init_ror(&world, &wops));

    /* Update timing. */
    world.player_count = 3;
    world.players = calloc(3, sizeof *world.players);
    for (i = 0; i < 3; ++i) world.players[i] = &players[i];
    world.speed = 1.5f;
    turn = 0;
    assert(aoe_world_update_ror(&g) == 0 && world.time_step == 0 && players[1].updates == 0);
    turn = -1;
    clock_ms = 1000;
    aoe_world_update_ror(&g); /* first free-running turn: catch up with 0 */
    assert(world.game_time == 0 && players[1].updates == 1 && victories == 1);
    assert(world.update_slot == 1 && world.last_ms == 1000);
    clock_ms = 1500; /* capped at 100 ms, x 1.5 */
    aoe_world_update_ror(&g);
    assert(world.game_time == 150 && world.time_step == 150 && world.step_seconds > 0.149f);
    assert(world.update_times[1] == 0 && world.update_times[2] == 150 && world.update_slot == 2);
    turn = 33; /* 49.5 -> 49 */
    world.field_1c = 1;
    aoe_world_update_ror(&g);
    assert(world.game_time == 199 && world.field_1c == 0 && world.update_slot == 0);
    turn = -1;
    world.field_1c = 1;
    clock_ms = 1550;
    aoe_world_update_ror(&g); /* a set +0x1c skips a free-running step */
    assert(world.game_time == 199 && world.time_step == 0 && world.field_1c == 0);

    /* Colours: player 1 wants 2; player 2 wants 2 too and gets the next
       free table with +0x24 = 1. */
    for (i = 0; i < 10; ++i) {
        memset(&colors[i], 0, sizeof colors[i]);
        colors[i].field_24 = (uint8_t)(i != 0);
        cp[i] = &colors[i];
    }
    world.colors = cp;
    world.color_count = 10;
    memset(&info, 0, sizeof info);
    info.player_count = 2;
    info.color[1] = 2;
    info.color[2] = 2;
    aoe_world_assign_colors_ror(&g, &info);
    assert(players[0].color == 7 && players[1].color == 2 && players[2].color == 1);
    aoe_world_assign_default_colors_ror(&g);
    assert(players[1].color == 1 && players[2].color == 2);

    /* Save and load the game. */
    world.game_time = 1234;
    world.campaign = 4;
    flag720 = 1;
    assert(aoe_world_save_game_ror(&g, "a.gam") == 1);
    assert(world.campaign == 4 && memcmp(buf, "VER 8.6", 8) == 0);
    world.game_time = 0;
    free(world.players);
    world.players = NULL;
    world.player_count = 0;
    st.pos = 0;
    assert(aoe_world_load_game_ror(&g, "a.gam") == 1);
    assert(g.version == 7.24f && world.game_time == 1234 && world.campaign == 4);
    assert(world.player_count == 3 && world.players[2] == &players[2] && world.field_100 == 5);
    ok_close = 0;
    removed = 0;
    assert(aoe_world_save_game_ror(&g, "b.gam") == 0 && removed == 1);
    ok_close = 1;
    buf[0] = 'X';
    assert(aoe_world_load_game_ror(&g, "a.gam") == 0);

    /* Scenario dispatch: 1.03 skips its sized block. */
    assert(aoe_world_campaign_game_ror(&g, &info) == 1 && scen_which == 1 && st.pos == 10);
    assert(world.campaign_scenario == 3);
    st.data = buf;
    memcpy(buf, "1.11", 4);
    st.size = 4;
    assert(aoe_world_scenario_game_ror(&g, "x.scn", &info) == 1 && scen_which == 7);
    memcpy(buf, "2.00", 4);
    assert(aoe_world_scenario_game_ror(&g, "x.scn", &info) == 0);
    assert(aoe_world_scenario_game_ror(&g, "missing", &info) == 0);

    /* Scenario save: 3 players, each with two listed objects (one of
       type 25 skipped). */
    world.player_count = 3; /* the failed load above reset the world */
    world.players = calloc(3, sizeof *world.players);
    for (i = 0; i < 3; ++i) world.players[i] = &players[i];
    assert(aoe_world_save_scenario_ror(&g, "my.scx") == 1);
    assert(memcmp(buf, "1.11", 4) == 0 && buf[4] == 0x17);
    /* "1.11", header, +0x68, scenario, map, count, 2 player saves, then
       3 x (count + 2 objects x 23 bytes), count, 2 player saves. */
    assert(st.size == 4 + 1 + 4 + 1 + 1 + 4 + 2 + 3 * (4 + 2 * 23) + 4 + 2);

    /* New game with teams: slots 1 and 2 are human (type 2) on team 2. */
    aoe_world_reset_ror(&world);
    memset(players, 0, sizeof players);
    stype[1] = stype[2] = 2;
    steam[0] = steam[1] = 2;
    memset(&info, 0, sizeof info);
    info.player_count = 2;
    assert(aoe_world_new_game_ror(&g, &info, 5) == 1);
    assert(world.state == 0 && world.field_7c == 5 && world.player_count == 3);
    assert(players[1].stance[2] == 10 && players[2].stance[1] == 10 && players[1].da40 == 1);
    assert(players[1].started == 1 && world.speed == 1.0f);
    world.colors = NULL; /* the test's tables are on the stack */
    world.color_count = 0;
    aoe_world_release_ror(&world, NULL);
    puts("world_game_test: ok");
    return 0;
}
