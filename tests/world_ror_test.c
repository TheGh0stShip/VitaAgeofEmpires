#include "world_ror.h"

#include <assert.h>
#include <string.h>

#include "aoe_stream.h"

typedef struct { int num, alive, score, won, made, computer; float attr[60]; float timer[2]; } P;
static P pl[5];
static int stance_tab[5][5], events[8], nev, base_checks;

static int32_t score(void *c, void *p) { (void)c; return ((P *)p)->score; }
static int alive(void *c, void *p) { (void)c; return ((P *)p)->alive; }
static int stance(void *c, void *p, int32_t o) { (void)c; return stance_tab[((P *)p)->num][o]; }
static float attr(void *c, void *p, int i) { (void)c; return ((P *)p)->attr[i]; }
static void set_attr(void *c, void *p, int i, float v) { (void)c; ((P *)p)->attr[i] = v; }
static float timer(void *c, void *p, int w) { (void)c; return ((P *)p)->timer[w]; }
static void set_timer(void *c, void *p, int w, float v) { (void)c; ((P *)p)->timer[w] = v; }
static int16_t number(void *c, void *p) { (void)c; return (int16_t)((P *)p)->num; }
static void win(void *c, void *p) { (void)c; ((P *)p)->won = 1; }
static void event(void *c, int32_t code, int32_t n) { (void)c; events[nev++] = code * 10 + n; }
static void base_check(void *c, AoeRorWorldRor *w) { (void)c; (void)w; base_checks++; }
static void *player_new(void *c, AoeRorWorldRor *w, void *civ, int16_t i, const char *n, uint8_t id, int comp) {
    (void)c; (void)w; (void)civ; (void)n; (void)id;
    pl[i].made = 1;
    pl[i].computer = comp;
    return &pl[i];
}

static AoeRorWorldOpsRor ops;

int main(void) {
    AoeRorWorldRor w;
    void *players[5];
    uint8_t buf[64];
    AoeOutcomeStream s;
    int i;
    int8_t type[9] = {0, 0, 4, 2, -1, 0, 0, 0, 0}, civ[9] = {0};
    char names[8][0x41];
    void *civs[1] = {NULL};

    memset(&ops, 0, sizeof ops);
    ops.score = score; ops.alive = alive; ops.stance = stance; ops.attribute = attr;
    ops.set_attribute = set_attr; ops.timer = timer; ops.set_timer = set_timer; ops.number = number;
    ops.win = win; ops.event = event; ops.base_check = base_check; ops.player_new = player_new;
    memset(&w, 0, sizeof w);
    w.ror = &ops;
    for (i = 0; i < 5; ++i) {
        memset(&pl[i], 0, sizeof pl[i]);
        pl[i].num = i;
        pl[i].alive = 1;
        pl[i].timer[0] = pl[i].timer[1] = -1.0f;
        players[i] = &pl[i];
    }
    for (i = 0; i < 5; ++i) { int j; for (j = 0; j < 5; ++j) stance_tab[i][j] = 3; }
    stance_tab[1][2] = stance_tab[2][1] = 0; /* players 1 and 2 allied */
    w.base.players = players;
    w.base.player_count = 4;
    w.base.step_seconds = 0.25f;

    /* Time limit: team (1, 2) averages (10 + 30) / 2 = 20; player 3 has
       25 and wins. */
    w.victory = 2;
    w.time_limit = 1;
    w.time_left = 1.2f;
    pl[1].score = 10; pl[2].score = 30; pl[3].score = 25;
    aoe_ror_world_check_victory_ror(&w);
    assert(pl[3].won && w.base.field_1e == 0x67 && base_checks == 1);

    /* Relics: 2 needed; team (1, 2) holds 1 + 1. */
    pl[3].won = 0;
    w.victory = 0;
    w.relics = 2;
    pl[1].attr[7] = 1.0f; pl[2].attr[7] = 1.0f;
    aoe_ror_world_check_victory_ror(&w);
    assert(pl[1].timer[0] == 2000.0f && pl[2].timer[0] == 2000.0f && pl[1].attr[55] == 1.0f);
    assert(nev == 1 && events[0] == 0x72 * 10 + 1);
    w.base.step_seconds = 500.0f; /* 2000 - 1000 */
    aoe_ror_world_check_victory_ror(&w);
    assert(pl[1].timer[0] == 1000.0f && !pl[1].won);
    aoe_ror_world_check_victory_ror(&w);
    assert(pl[1].won && w.base.field_1e == 0x65);
    /* Losing a relic clears the timer. */
    pl[2].attr[7] = 0.0f;
    aoe_ror_world_check_victory_ror(&w);
    assert(pl[1].timer[0] == -1.0f && pl[2].attr[55] == 0.0f && events[nev - 1] == 0x73 * 10 + 1);
    /* Not while the game is over. */
    w.base.state = 1;
    nev = 0;
    pl[2].attr[7] = 1.0f;
    aoe_ror_world_check_victory_ror(&w);
    assert(nev == 0);

    /* Saved fields by version. */
    w.victory = 7; w.relics = 3; w.ruins = 4; w.time_limit = 1; w.time_left = 9.5f;
    w.field_11c = 5; w.field_11d = 6;
    s.data = buf; s.size = sizeof buf; s.pos = 0; s.failed = 0;
    aoe_ror_world_save_state_ror(&w, &s);
    assert(s.pos == 4 + 4 + 4 + 1 + 4 + 1 + 1);
    s.size = s.pos; s.pos = 0;
    memset(&w.victory, 0, sizeof w.victory);
    w.version = 7.24f;
    assert(aoe_ror_world_load_state_ror(&w, &s, 1) == 1);
    assert(w.victory == 7 && w.ruins == 4 && w.time_left == 9.5f && w.field_11d == 6);
    s.pos = 0;
    w.version = 5.35f; /* relics/ruins only */
    aoe_ror_world_load_state_ror(&w, &s, 1);
    assert(w.victory == 0 && w.relics == 7 && w.ruins == 3 && w.time_limit == 0 && w.field_11d == 2);
    assert(aoe_ror_world_load_state_ror(&w, &s, 0) == 0);

    /* New players: types 0 and 4 only; type 4 flagged with 0x00554730. */
    memset(names, 0, sizeof names);
    for (i = 0; i < 5; ++i) pl[i].made = 0;
    w.base.civs = civs;
    w.base.player_count = 5;
    ops.computer_flag = 1;
    aoe_ror_world_new_players_ror(&w, type, civ, names);
    assert(pl[1].made && !pl[1].computer && pl[2].made && pl[2].computer && !pl[3].made && !pl[4].made);

    /* Editor resources. */
    w.base.player_count = 3;
    aoe_ror_world_editor_resources_ror(&w);
    assert(pl[1].attr[0] == 200.0f && pl[2].attr[2] == 100.0f && pl[2].attr[3] == 0.0f);

    aoe_ror_world_vtable_init_ror();
    assert(aoe_ror_world_vtable_ror.load_text == aoe_ror_world_load_text_ror);
    assert(aoe_ror_world_vtable_ror.load_table == aoe_world_base_vtable_ror.load_table);
    puts("world_ror_test: ok");
    return 0;
}
