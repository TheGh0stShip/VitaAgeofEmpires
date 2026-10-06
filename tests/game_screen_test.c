#include "game_screen.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int active[64];
    char text[64][64];
    int help1[64];
    int msg_style[64], msg_color[64];
    char msg[64][64];
    int hidden[64];
    int redraws[64];
    int used[64], disabled[64], id[64], value[64], fields[64][3];
    int timer_mode[16], timer_index[16];
    int comm, last_action, last_arg, alt, place;
    int ncount;
    float age;
    int afford;
    int16_t sel_count;
    int32_t sel[25], current;
    AoeGameScreenObjectRor objs[8];
    int32_t list[6];
    int list_n;
    int alive[8], stance[8][8];
    float ptimer[8][2];
    int years[8];
    int16_t players;
    int world_mode;
} Fake;

static Fake f;
static int ix(void *p) { return (int)(intptr_t)p; }
static void set_active(void *c, void *p, int on) { (void)c; f.active[ix(p)] = on; }
static void redraw(void *c, void *p, int a) { (void)c; (void)a; f.redraws[ix(p)]++; }
static void set_text_id(void *c, void *p, int32_t id) { (void)c; snprintf(f.text[ix(p)], 64, "#%d", (int)id); }
static void set_text(void *c, void *p, const char *t) { (void)c; snprintf(f.text[ix(p)], 64, "%s", t); }
static int active(void *c, void *p) { (void)c; return f.active[ix(p)]; }
static const char *text(void *c, void *p) { (void)c; return f.text[ix(p)]; }
static void help(void *c, void *p, int32_t a, int32_t b) { (void)c; (void)b; f.help1[ix(p)] = a; }
static void message(void *c, void *p, int32_t st, const char *t, uint8_t col) {
    (void)c; f.msg_style[ix(p)] = st; f.msg_color[ix(p)] = col; snprintf(f.msg[ix(p)], 64, "%s", t);
}
static void message_hide(void *c, void *p) { (void)c; f.hidden[ix(p)] = 1; }
static int32_t b_dis(void *c, void *b) { (void)c; return f.disabled[ix(b)]; }
static void b_set_dis(void *c, void *b, int32_t d) { (void)c; f.disabled[ix(b)] = d; }
static int32_t b_id(void *c, void *b) { (void)c; return f.id[ix(b)]; }
static int32_t b_val(void *c, void *b) { (void)c; return f.value[ix(b)]; }
static void b_none(void *c, void *b) { (void)c; (void)b; }
static void b_pic(void *c, void *b, void *p, int32_t fr) { (void)c; (void)b; (void)p; (void)fr; }
static void b_state(void *c, void *b, int32_t id, int32_t v) { (void)c; f.id[ix(b)] = id; f.value[ix(b)] = v; }
static void b_lines(void *c, void *b, const char *l1, const char *l2) { (void)c; (void)l2; snprintf(f.text[ix(b)], 64, "%s", l1); }
static void b_help_line(void *c, void *b, const char *t) { (void)c; (void)b; (void)t; }
static int32_t b_used(void *c, void *b) { (void)c; return f.used[ix(b)]; }
static void b_set_used(void *c, void *b, int32_t u) { (void)c; f.used[ix(b)] = u; }
static void b_fields(void *c, void *b, int32_t a, int32_t d, int32_t k) {
    (void)c; f.fields[ix(b)][0] = a; f.fields[ix(b)][1] = d; f.fields[ix(b)][2] = k;
}
static void timer_set(void *c, void *t, int32_t m, int32_t i) { (void)c; f.timer_mode[ix(t) - 40] = m; f.timer_index[ix(t) - 40] = i; }
static int32_t comm(void *c) { (void)c; return f.comm; }
static void action(void *c, int32_t a, int32_t b) { (void)c; f.last_action = a; f.last_arg = b; }
static void format(void *c, int32_t k, int32_t a, int32_t b, char *o, int32_t n) { (void)c; snprintf(o, (size_t)n, "%x:%d:%d", (int)k, (int)a, (int)b); }
static const char *string(void *c, int32_t id) {
    (void)c;
    if (id == 0x1037) return "Build";
    if (id == 0x104b) return "Move";
    return "str";
}
static void soft(void *c, int on) { (void)c; f.ncount = on; }
static void place(void *c, int16_t id) { (void)c; f.place = id; }
static int32_t alt(void *c) { (void)c; return f.alt; }
static void set_alt(void *c, int32_t on) { (void)c; f.alt = on; }
static const char *chat_line(void *c, int32_t i) { (void)c; return i == 2 ? "hello" : "x"; }
static void wonder(void *c, int32_t p, int32_t *y) { (void)c; *y = f.years[p]; }
static int32_t wmode(void *c) { (void)c; return f.world_mode; }
static int16_t pcount(void *c) { (void)c; return f.players; }
static int palive(void *c, int32_t p) { (void)c; return f.alive[p]; }
static float ptimer(void *c, int32_t p, int w) { (void)c; return f.ptimer[p][w]; }
static int stance(void *c, int32_t p, int32_t o) { (void)c; return f.stance[p][o]; }
static float age(void *c) { (void)c; return f.age; }
static int afford(void *c, int16_t id, int16_t *fail) { (void)c; (void)id; *fail = 2; return f.afford; }
static int tflag(void *c, int16_t id) { (void)c; return id == 83; }
static int16_t sel_count(void *c) { (void)c; return f.sel_count; }
static int32_t selected(void *c, int32_t s) { (void)c; return f.sel[s]; }
static int32_t current(void *c) { (void)c; return f.current; }
static void set_current(void *c, int32_t id) { (void)c; f.current = id; }
static int object(void *c, int32_t id, AoeGameScreenObjectRor *o) {
    (void)c;
    if (id < 1 || id > 7) return 0;
    *o = f.objs[id];
    return 1;
}
static void g_int(void *c, int32_t g) { (void)c; f.last_arg = g; }
static void g_none(void *c) { (void)c; f.ncount++; }
static void vpos(void *c, float x, float y) { (void)c; (void)x; (void)y; }
static void vtile(void *c, int32_t x, int32_t y) { (void)c; f.last_action = x; f.last_arg = y; }
static int sel_ids(void *c, int32_t **ids, int16_t *n) { (void)c; *ids = NULL; *n = 0; return f.sel_count > 0; }
static void free_ids(void *c, int32_t *ids) { (void)c; (void)ids; }
static void pcall(void *c, int slot, int32_t a) { (void)c; f.last_action = slot; f.last_arg = a; }
static int busy(void *c, int32_t id) { (void)c; return id == 3; }
static int32_t lcount(void *c) { (void)c; return f.list_n; }
static int32_t lid(void *c, int32_t i) { (void)c; return f.list[i]; }
static int whas(void *c, int32_t id) { (void)c; return id >= 1 && id <= 7; }
static int ready(void *c, int32_t id) { (void)c; return id != 4; }
static void base_mouse(void *c, int32_t a, int32_t b, int32_t d, int32_t e, int32_t g) {
    (void)c; (void)b; (void)d; (void)e; (void)g; f.last_action = a;
}
static void focus(void *c, int32_t v) { (void)c; f.last_arg = v; }

static int pc_which = -1, toggles, chats, state5, is_paused, is_net;
static uint32_t pc_value, clock_ms;
static char chat_text[64];
static void pcommand(void *c, int w, uint32_t v) { (void)c; pc_which = w; pc_value = v; }
static int32_t gstate(void *c) { (void)c; return state5 ? 5 : 1; }
static int gpaused(void *c) { (void)c; return is_paused; }
static int gnet(void *c) { (void)c; return is_net; }
static void gtoggle(void *c) { (void)c; ++toggles; }
static void gchat(void *c, const char *t) { (void)c; ++chats; snprintf(chat_text, sizeof chat_text, "%s", t); }
static uint32_t gnow(void *c) { (void)c; return clock_ms; }

static int refreshes, sounds, selects, zooms = -1, orders, emode, esub, owned;
static void grefresh(void *c) { (void)c; ++refreshes; }
static int32_t gemode(void *c, int32_t *sub) { (void)c; *sub = esub; return emode; }
static int gowned(void *c, int32_t id) { (void)c; (void)id; return owned; }
static void gselect(void *c, int32_t id) { (void)c; selects = id; }
static void gsound(void *c, int32_t id) { (void)c; sounds = id; }
static int gorders(void *c) { (void)c; return orders; }
static void gzoom(void *c, int in) { (void)c; zooms = in; }
static float speed = 1.0f, sent_speed, stored_speed;
static int fast;
static float gspeed(void *c) { (void)c; return speed; }
static int gfast(void *c) { (void)c; return fast; }
static void gsend_speed(void *c, float v) { (void)c; sent_speed = v; }
static void gstore_speed(void *c, float v) { (void)c; stored_speed = v; }
static int armed, disarms, vcancels, outlines, set_modes, runs, cpauses;
static char dialog_name[32];
static int garmed(void *c) { (void)c; return armed; }
static void gdisarm(void *c) { (void)c; ++disarms; }
static void gvcancel(void *c, void *v) { (void)c; (void)v; ++vcancels; }
static void goutline(void *c) { (void)c; ++outlines; }
static void gsetmode(void *c, int32_t m, int32_t sub) { (void)c; assert(m == 0 && sub == 0); ++set_modes; }
static void grun(void *c) { (void)c; ++runs; }
static void gcpause(void *c, int32_t on, int32_t fl) { (void)c; assert(on == 1 && fl == 1); ++cpauses; }
static void gdialog(void *c, const char *n) { (void)c; snprintf(dialog_name, sizeof dialog_name, "%s", n); }
static int32_t type_asked = -1, after_asked = -1, type_found;
static int16_t trained = -1, researched = -1, instant_id = -1;
static int can_res, inst;
static void gtrain_at(void *c, int16_t id) { (void)c; trained = id; }
static void gresearch_at(void *c, int16_t id) { (void)c; researched = id; }
static int gcan_res(void *c, int16_t id, int16_t *fl) { (void)c; (void)id; *fl = 2; return can_res; }
static int ginstant(void *c) { (void)c; return inst; }
static void ginstant_train(void *c, int16_t id, int32_t a) { (void)c; (void)a; instant_id = id; }
static int sfreed, bcasts;
static void gfree_sound(void *c, void *p) { (void)c; assert(p == (void *)9); ++sfreed; }
static void gbcast(void *c) { (void)c; ++bcasts; }
static int32_t gnext_type(void *c, int32_t t, int32_t a) { (void)c; type_asked = t; after_asked = a; return type_found; }

static int fg = 1, vfocus = 1, modal_on, keeps, mx, my, overmap, hit_btn = -1;
static int gfg(void *c) { (void)c; return fg; }
static int gvfocus(void *c) { (void)c; return vfocus; }
static int gmodal(void *c) { (void)c; return modal_on; }
static void gmouse(void *c, int32_t *x, int32_t *y) { (void)c; *x = mx; *y = my; }
static int gkeeps(void *c) { (void)c; return keeps; }
static int govermap(void *c, int32_t x, int32_t y) { (void)c; (void)x; (void)y; return overmap; }
static int ghit(void *c, void *b, int32_t x, int32_t y) { (void)c; (void)x; (void)y; return ix(b) == hit_btn; }
static const char *ghelp_text(void *c, void *b) { (void)c; return ix(b) == 52 ? "tip52" : "tip"; }
static int sready = 1, mplaying, mstops;
static int16_t ncues;
static uint8_t cue_on[4], cue_pri[4];
static char played[32];
static int16_t gcue_count(void *c) { (void)c; return ncues; }
static void gcue(void *c, int32_t i, uint8_t *on, uint8_t *p, void **e) {
    (void)c; *on = cue_on[i]; *p = cue_pri[i]; *e = (void *)(intptr_t)(100 + i);
}
static const char *gchoose(void *c, void *e) { (void)c; return ix(e) == 102 ? "cue2" : "cue"; }
static void gmstop(void *c) { (void)c; ++mstops; }
static void gmplay(void *c, const char *n) { (void)c; snprintf(played, sizeof played, "%s", n); }
static int gmplaying(void *c) { (void)c; return mplaying; }
static int sready_f(void *c) { (void)c; return sready; }
static int map_flag = -1, help_y;
static int ngame = 3, kinds[9], lost_slot = -1;
static int32_t scores_v[9];
static uint32_t lat[9], pings[9];
static AoeGameScoreLineRor lines_out[64];
static void gmapflag(void *c, uint8_t on) { (void)c; map_flag = on; }
static void ghelp_place(void *c, void *p, int32_t y) { (void)c; assert(ix(p) == 55); help_y = y; }
static int32_t ggame_players(void *c) { (void)c; return ngame; }
static int32_t gwslot(void *c, int32_t p) { (void)c; return p; }
static int32_t gscore(void *c, int32_t s) { (void)c; return scores_v[s]; }
static int16_t gcolour(void *c, int32_t s) { (void)c; return (int16_t)s; }
static int glost(void *c, int32_t s) { (void)c; return s == lost_slot; }
static int32_t gkind(void *c, int32_t p) { (void)c; return kinds[p]; }
static uint32_t glat(void *c, int32_t p) { (void)c; return lat[p]; }
static uint32_t gping(void *c, int32_t p) { (void)c; return pings[p]; }
static const char *gpname(void *c, int32_t p) { (void)c; static const char *n[] = {"", "A", "B", "C", "D"}; return n[p]; }
static void gscore_line(void *c, void *p, const AoeGameScoreLineRor *l) { (void)c; lines_out[ix(p)] = *l; }

static const AoeGameScreenOpsRor ops = {
    NULL, set_active, redraw, set_text_id, set_text, active, text, help, message, message_hide,
    b_dis, b_set_dis, b_id, b_val, b_none, b_none, b_pic, b_state, b_lines, b_help_line,
    b_used, b_set_used, b_fields, timer_set, comm, action, format, string, soft, place, alt,
    set_alt, chat_line, wonder, wmode, pcount, palive, ptimer, stance, age, afford, tflag,
    sel_count, selected, current, set_current, object, g_int, g_int, g_none, g_none, g_none,
    vpos, vtile, sel_ids, free_ids, pcall, busy, lcount, lid, whas, ready, base_mouse,
    pcommand, gstate, gpaused, gnet, gtoggle, gchat, gnow, grefresh, gemode, gowned, gselect,
    gsound, gorders, gzoom, gspeed, gfast, gsend_speed, gstore_speed,
    garmed, gdisarm, gvcancel, goutline, gsetmode, grun, gcpause, gdialog, gnext_type,
    gtrain_at, gresearch_at, gcan_res, ginstant, ginstant_train, gfree_sound, gbcast,
    gfg, gvfocus, gmodal, gmouse, gkeeps, govermap, ghit, ghelp_text,
    sready_f, gcue_count, gcue, gchoose, gmstop, gmplay, gmplaying,
    gmapflag, ghelp_place, ggame_players, gwslot, gscore, gcolour, glost, gkind, glat, gping,
    gpname, gscore_line};

static void setup(AoeGameScreenRor *s) {
    int i;
    memset(&f, 0, sizeof f);
    memset(s, 0, sizeof *s);
    s->ops = &ops;
    s->view = (void *)1;
    s->resources = (void *)2;
    s->age_text = (void *)3;
    s->panel_584 = (void *)4;
    s->title = (void *)5;
    for (i = 0; i < 12; ++i) s->buttons[i] = (void *)(intptr_t)(10 + i);
    for (i = 0; i < 6; ++i) s->lines[i] = (void *)(intptr_t)(24 + i);
    for (i = 0; i < 8; ++i) s->chat[i] = (void *)(intptr_t)(30 + i);
    for (i = 0; i < 10; ++i) s->timers[i] = (void *)(intptr_t)(40 + i);
    for (i = 0; i < 5; ++i) s->more_buttons[i] = (void *)(intptr_t)(50 + i);
    s->help_panel = (void *)55;
    for (i = 0; i < 8; ++i) s->scores[i] = (void *)(intptr_t)(56 + i);
    s->ping_picture = (void *)7;
    s->width = 640;
    s->height = 480;
}

int main(void) {
    AoeGameScreenRor s;
    AoeGameScreenButtonRor b;
    int i;
    int32_t ra[5] = {0, 1, 5, 5, 9}, rb[5] = {0, 2, 5, 5, 9};

    setup(&s);
    s.chat_next = 7;
    aoe_game_screen_chat_line_ror(&s, 2);
    assert(strcmp(f.msg[37], "hello") == 0 && f.msg_style[37] == 3 && f.msg_color[37] == 0xff);
    assert(s.chat_next == 0);

    aoe_game_screen_show_584_ror(&s);
    assert(f.active[4] == 1 && strcmp(f.text[4], "#9001") == 0 && f.redraws[1] == 1);
    aoe_game_screen_hide_584_ror(&s);
    assert(f.active[4] == 0 && f.redraws[1] == 2);
    aoe_game_screen_hide_584_ror(&s);
    assert(f.redraws[1] == 2);

    f.age = 3.0f;
    aoe_game_screen_age_ror(&s);
    assert(s.age == 3 && f.help1[3] == 0x106b && strcmp(f.text[3], "65:3:0") == 0);

    aoe_game_screen_title_ror(&s, "abc");
    assert(strcmp(f.text[5], "abc") == 0);

    aoe_game_screen_mouse_ror(&s, 7, 0, 0, 0, 0);
    assert(f.hidden[24] && f.hidden[29] && f.last_action == 7);

    assert(aoe_game_screen_button_slot_ror(1) == 0 && aoe_game_screen_button_slot_ror(6) == 6);
    assert(aoe_game_screen_button_slot_ror(11) == 0 && aoe_game_screen_button_slot_ror(20) == 10);
    assert(aoe_game_screen_button_slot_ror(0) == 0 && aoe_game_screen_button_slot_ror(21) == 0);

    memset(&b, 0, sizeof b);
    b.index = 2;
    b.command = 3;
    b.value = 9;
    aoe_game_screen_set_button_ror(&s, &b);
    assert(f.used[12] == 1 && f.id[12] == 3 && f.fields[12][2] == 'B' && f.active[12] == 1);
    f.active[12] = 0;
    aoe_game_screen_set_button_ror(&s, &b);
    assert(f.active[12] == 0); /* unchanged: no re-show */
    b.command = 6;
    aoe_game_screen_set_button_ror(&s, &b);
    assert(f.fields[12][2] == 0x1b);
    b.command = 0x11;
    b.hotkey_id = 0;
    aoe_game_screen_set_button_ror(&s, &b);
    assert(f.fields[12][1] == 2 && f.fields[12][2] == 0);
    aoe_game_screen_hide_unused_ror(&s);
    assert(f.active[12] == 1 && f.active[10] == 0 && f.value[10] == -1 && f.help1[10] == -1);

    /* Timers: player 1 wonder; players 1 and 2 allied, so 2's timers are
       not shown; player 3 timer 1. */
    f.world_mode = 0;
    f.players = 4;
    f.years[1] = 100;
    for (i = 0; i < 4; ++i) { f.ptimer[i][0] = -1.0f; f.ptimer[i][1] = -1.0f; f.alive[i] = 1; }
    for (i = 0; i < 4; ++i) { int j; for (j = 0; j < 4; ++j) f.stance[i][j] = 3; }
    f.stance[1][2] = f.stance[2][1] = 0;
    f.ptimer[2][0] = 5.0f;
    f.ptimer[3][1] = 2.0f;
    aoe_game_screen_timers_ror(&s);
    assert(f.timer_mode[0] == 4 && f.timer_index[0] == 1);
    assert(f.timer_mode[1] == 6 && f.timer_index[1] == 3);
    assert(f.timer_mode[2] == 0 && f.timer_mode[9] == 0);
    f.world_mode = 2;
    aoe_game_screen_timers_ror(&s);
    assert(f.timer_mode[0] == 3);

    assert(aoe_game_screen_score_cmp_ror(ra, rb) == 1);
    assert(aoe_game_screen_score_cmp_ror(rb, ra) == -1);
    assert(aoe_game_screen_score_cmp_ror(ra, ra) == 0);

    setup(&s);
    aoe_game_screen_announce_ror(&s, 0xbe2, 0x67);
    assert(f.last_action == 0x67 && f.msg_color[24] == 0x32);
    f.comm = 1;
    f.last_action = 0;
    aoe_game_screen_announce_ror(&s, 0xbe2, 0x67);
    assert(f.last_action == 0);
    f.comm = 0;

    f.afford = 1;
    aoe_game_screen_train_ror(&s, 83);
    assert(f.place == 83 && f.last_action == 0x15);
    aoe_game_screen_train_ror(&s, 84);
    assert(f.last_action == 7);
    f.afford = 0;
    aoe_game_screen_train_ror(&s, 84);
    assert(strcmp(f.msg[24], "67:1:2") == 0 && f.msg_color[24] == 0x56);

    aoe_game_screen_alt_bar_ror(&s);
    assert(f.alt == 1 && f.active[5] == 1 && f.active[2] == 0);
    aoe_game_screen_alt_bar_ror(&s);
    assert(f.alt == 1);
    aoe_game_screen_alt_bar_ror(&s);
    assert(f.alt == 0 && f.active[2] == 1);

    aoe_game_screen_group_assign_ror(&s, 0);
    assert(f.last_arg == 10);

    /* Cycling the selection: 2, 5, 6 in slots 0, 3, 7. */
    f.sel_count = 3;
    f.sel[0] = 2; f.sel[3] = 5; f.sel[7] = 6;
    f.current = 5;
    aoe_game_screen_cycle_ror(&s, 1);
    assert(f.current == 6);
    aoe_game_screen_cycle_ror(&s, 1);
    assert(f.current == 2);
    aoe_game_screen_cycle_ror(&s, 0);
    assert(f.current == 6);
    aoe_game_screen_cycle_ror(&s, 0);
    assert(f.current == 5);

    f.objs[3].type = 0x30;
    f.objs[5].type = 0x30;
    assert(!aoe_game_screen_selection_busy_ror(&s));
    f.sel[3] = 3;
    assert(aoe_game_screen_selection_busy_ror(&s));

    f.objs[2].x = 3.7f; f.objs[2].y = 9.2f;
    f.current = 2;
    aoe_game_screen_center_ror(&s);
    assert(f.last_action == 3 && f.last_arg == 9);

    /* Next idle: list 1..5; 4 not ready, 5 has category 4. */
    f.list_n = 5;
    for (i = 0; i < 5; ++i) { f.list[i] = i + 1; f.objs[i + 1].category = 0; }
    f.objs[4].category = 4;
    f.objs[5].category = 4;
    f.objs[1].category = 2;
    assert(aoe_game_screen_next_idle_ror(&s, 3) == 5 && s.idle_id == 5);
    assert(aoe_game_screen_next_idle_ror(&s, 5) == 1);
    assert(aoe_game_screen_next_idle_ror(&s, 99) == 5);

    s.field_78 = 1;
    aoe_game_screen_focus_ror(&s, focus, 4);
    assert(f.last_arg == 4 && f.ncount == 0);
    puts("game_screen_test: ok");
    /* Delete, opcode 0x6d and the pause key. */
    f.comm = 1;
    aoe_game_screen_delete_ror(&s);
    assert(pc_which == -1);
    f.comm = 0;
    aoe_game_screen_delete_ror(&s);
    assert(pc_which == 0);
    aoe_game_screen_command_6d_ror(&s, 9);
    assert(pc_which == 1 && pc_value == 9);
    s.pause_time = 0;
    clock_ms = 5000;
    is_net = 1;
    aoe_game_screen_pause_ror(&s);
    assert(toggles == 1 && chats == 1 && !strcmp(chat_text, "str") && s.pause_time == 5000);
    clock_ms = 5500;
    aoe_game_screen_pause_ror(&s);
    assert(toggles == 1);
    is_paused = 1;
    aoe_game_screen_pause_ror(&s);
    assert(toggles == 2 && chats == 2);
    state5 = 1;
    clock_ms = 9000;
    aoe_game_screen_pause_ror(&s);
    assert(toggles == 2 && s.pause_time == 9000);

    /* Button pages. */
    s.button_pages = 3; s.shown_page = 1; s.button_page = 1;
    aoe_game_screen_next_page_ror(&s);
    assert(s.button_page == 2 && refreshes == 1);
    s.shown_page = 2;
    aoe_game_screen_next_page_ror(&s);
    assert(s.button_page == 0 && refreshes == 2);
    emode = 6; esub = 1;
    aoe_game_screen_next_page_ror(&s);
    assert(s.button_page == 10);
    aoe_game_screen_next_page_ror(&s);
    assert(s.button_page == 0 && refreshes == 4);
    s.button_page = 5;
    f.comm = 1;
    aoe_game_screen_cancel_page_ror(&s);
    assert(s.button_page == 5 && refreshes == 4);
    f.comm = 0;
    aoe_game_screen_cancel_page_ror(&s);
    assert(s.button_page == 0 && refreshes == 5 && f.last_action == 6 && f.last_arg == 1);
    s.button_page = 4;
    aoe_game_screen_first_page_ror(&s);
    assert(s.button_page == 0 && refreshes == 6);

    /* Go to idle: list 1..5 as above; the current owned idle unit 1
       starts the search, so 5 is next after... 1 -> 2,3 not idle. */
    f.current = 1; owned = 1; s.idle_id = 5;
    aoe_game_screen_goto_idle_ror(&s);
    assert(selects == 5 && f.last_action == 0 && s.view && f.redraws[1] > 0);
    owned = 0; s.idle_id = 5;
    aoe_game_screen_goto_idle_ror(&s);
    assert(selects == 1);
    f.list_n = 0;
    aoe_game_screen_goto_idle_ror(&s);
    assert(sounds == 3);

    /* Zoom keys. */
    assert(aoe_game_screen_zoom_key_ror(&s, '+') == 0 && zooms == -1);
    orders = 1;
    aoe_game_screen_zoom_key_ror(&s, '=');
    assert(zooms == 1);
    aoe_game_screen_zoom_key_ror(&s, '-');
    assert(zooms == 0);
    zooms = -1;
    aoe_game_screen_zoom_key_ror(&s, 'a');
    assert(zooms == -1);
    /* Game speed step (0x00484c60). */
    state5 = 0; is_paused = 0; is_net = 0; chats = 0; s.speed_time = 0; clock_ms = 20000;
    speed = 1.0f;
    aoe_game_screen_speed_step_ror(&s, 1);
    assert(sent_speed == 1.5f && stored_speed == 0.0f && s.speed_time == 20000);
    sent_speed = 0; clock_ms = 20500; speed = 1.5f;
    aoe_game_screen_speed_step_ror(&s, 1);
    assert(sent_speed == 0.0f);                 /* rate limited */
    clock_ms = 22000; speed = 2.0f;
    aoe_game_screen_speed_step_ror(&s, 1);
    assert(sent_speed == 0.0f);                 /* no fast speeds */
    is_paused = 1; fast = 1; is_net = 1; clock_ms = 23000;
    aoe_game_screen_speed_step_ror(&s, 1);
    assert(sent_speed == 4.0f && stored_speed == 4.0f && chats == 1);
    speed = 16.0f;
    aoe_game_screen_speed_step_ror(&s, 0);
    assert(sent_speed == 8.0f);
    speed = 1.0f; sent_speed = 0;
    aoe_game_screen_speed_step_ror(&s, 0);
    assert(sent_speed == 0.0f);
    state5 = 1; speed = 1.5f;
    aoe_game_screen_speed_step_ror(&s, 0);
    assert(sent_speed == 0.0f);
    state5 = 0;
    aoe_game_screen_speed_step_ror(&s, 0);
    assert(sent_speed == 1.0f);
    /* Escape (0x00484900). */
    armed = 1;
    aoe_game_screen_escape_ror(&s);
    assert(disarms == 1 && set_modes == 0);
    armed = 0; emode = 0x15; f.redraws[1] = 0;
    aoe_game_screen_escape_ror(&s);
    assert(vcancels == 1 && outlines == 1 && set_modes == 1 && f.redraws[1] == 0);
    emode = 3;
    aoe_game_screen_escape_ror(&s);
    assert(vcancels == 1 && set_modes == 2);
    emode = 0;
    aoe_game_screen_escape_ror(&s);
    assert(set_modes == 2 && f.redraws[1] == 1);
    f.comm = 1;
    aoe_game_screen_escape_ror(&s);
    assert(f.redraws[1] == 1);
    /* Diplomacy (0x00484b60) and menu (0x004850b0). */
    aoe_game_screen_diplomacy_ror(&s);
    assert(dialog_name[0] == 0);
    f.comm = 0; is_paused = 1;
    aoe_game_screen_diplomacy_ror(&s);
    assert(!strcmp(dialog_name, "Diplomacy Dialog") && cpauses == 1);
    is_paused = 0; state5 = 0;
    aoe_game_screen_menu_ror(&s);
    assert(!strcmp(dialog_name, "Menu Dialog") && cpauses == 1 && runs == 0);
    /* Building keys (0x00485390). */
    f.current = 2; f.objs[2].template_id = 0x0c; f.objs[3].x = 7.5f; f.objs[3].y = 3.0f;
    type_found = 0;
    aoe_game_screen_building_key_ror(&s, 1);
    assert(type_asked == 0x0c && after_asked == 2);
    aoe_game_screen_building_key_ror(&s, 0);
    assert(type_asked == 0x6d && after_asked == 0);
    type_asked = -1;
    aoe_game_screen_building_key_ror(&s, 8);
    assert(type_asked == -1);
    type_found = 3; f.redraws[1] = 0;
    aoe_game_screen_building_key_ror(&s, 6);
    assert(type_asked == 0x68 && selects == 3 && f.redraws[1] == 1 && f.last_action == 7 && f.last_arg == 3);
    /* Train (0x00485520) and research (0x004852f0) buttons. */
    f.comm = 0; f.afford = 1; inst = 0;
    aoe_game_screen_train_button_ror(&s, 83, 0);
    assert(trained == 83 && f.place == 83);
    f.afford = 0; f.msg_color[24] = 0;
    aoe_game_screen_train_button_ror(&s, 84, 0);
    assert(trained == 83 && f.msg_color[24] == 0x56 && !strcmp(f.msg[24], "67:1:2"));
    inst = 1;
    aoe_game_screen_train_button_ror(&s, 85, 1);
    assert(instant_id == 85 && trained == 83);
    can_res = 1;
    aoe_game_screen_research_button_ror(&s, 40);
    assert(researched == 40);
    can_res = 0; f.msg_color[24] = 0;
    aoe_game_screen_research_button_ror(&s, 41);
    assert(researched == 40 && f.msg_color[24] == 0x56);
    /* Chat (0x00484a70 / 0x004849b0). */
    dialog_name[0] = 0; state5 = 1; s.dialog = 0; f.msg_color[24] = 0;
    aoe_game_screen_quick_chat_ror(&s);
    assert(dialog_name[0] == 0 && f.msg_color[24] == 0x32);
    state5 = 0;
    aoe_game_screen_quick_chat_ror(&s);
    assert(!strcmp(dialog_name, "Send Quick Message Dialog"));
    is_paused = 0;
    aoe_game_screen_chat_ror(&s);
    assert(!strcmp(dialog_name, "Send Message Dialog"));
    is_paused = 1;
    aoe_game_screen_chat_ror(&s);
    assert(!strcmp(dialog_name, "Send Quick Message Dialog"));
    state5 = 1; s.dialog = 1; dialog_name[0] = 0; is_paused = 0;
    aoe_game_screen_chat_ror(&s);
    assert(!strcmp(dialog_name, "Send Message Dialog"));
    /* Broadcast (0x00485a50). */
    s.sound = (void *)9;
    aoe_game_screen_broadcast_ror(&s);
    aoe_game_screen_broadcast_ror(&s);
    assert(sfreed == 1 && bcasts == 2 && s.sound == NULL);
    /* Event cue (0x004844a0). */
    setup(&s);
    s.cue_interval = 500; clock_ms = 1000; ncues = 3;
    cue_on[0] = 1; cue_pri[0] = 3; cue_on[1] = 0; cue_pri[1] = 9; cue_on[2] = 1; cue_pri[2] = 5;
    aoe_game_screen_cue_ror(&s);
    assert(s.cue_playing == 1 && !strcmp(played, "cue2") && mstops == 1 && s.cue_start == 1000);
    clock_ms = 1999; aoe_game_screen_cue_ror(&s);
    assert(s.cue_time == 1000 && s.cue_playing == 1);
    mplaying = 1; clock_ms = 2000; aoe_game_screen_cue_ror(&s);
    assert(s.cue_playing == 1 && s.cue_time == 2000);
    clock_ms = 7000; aoe_game_screen_cue_ror(&s);
    assert(s.cue_playing == 0 && mstops == 2);
    clock_ms = 7400; played[0] = 0; aoe_game_screen_cue_ror(&s);
    assert(!s.cue_playing && played[0] == 0);
    sready = 0; clock_ms = 9000; aoe_game_screen_cue_ror(&s);
    assert(!s.cue_playing && s.cue_time == 7000);
    sready = 1; s.cue_playing = 1; s.cue_time = 0; mplaying = 0; aoe_game_screen_cue_ror(&s);
    assert(!s.cue_playing && mstops == 2);
    /* Hover help (0x00484620). */
    setup(&s);
    mx = 100; my = 480; hit_btn = 52; f.active[52] = 1; f.active[10] = 1;
    aoe_game_screen_hover_ror(&s);
    assert(s.mouse_x == 100 && s.mouse_y == 480 && f.hidden[55] && !strcmp(f.msg[55], "tip52"));
    f.msg[55][0] = 0; aoe_game_screen_hover_ror(&s);
    assert(f.msg[55][0] == 0);
    my = 481; aoe_game_screen_hover_ror(&s);
    assert(f.msg[55][0] == 0 && s.mouse_y == 481);
    my = 10; overmap = 1; aoe_game_screen_hover_ror(&s);
    assert(f.msg[55][0] == 0);
    overmap = 0; my = 11; f.id[52] = -1; aoe_game_screen_hover_ror(&s);
    assert(f.msg[55][0] == 0);
    modal_on = 1; my = 12; aoe_game_screen_hover_ror(&s);
    assert(s.mouse_y == 11);
    modal_on = 0;
    /* Map scores (0x00484f90 / 0x00483c40): players 1 and 3 allied. */
    setup(&s);
    f.players = 4; ngame = 3;
    for (i = 1; i <= 3; ++i) f.alive[i] = 1;
    f.stance[1][2] = f.stance[2][1] = 1; f.stance[2][3] = f.stance[3][2] = 1;
    scores_v[1] = 100; scores_v[2] = 50; scores_v[3] = 300;
    is_net = 1; kinds[1] = 2; kinds[2] = 2; kinds[3] = 0;
    lat[1] = 200; lat[2] = 20; pings[1] = 1200; pings[2] = 400; lost_slot = 2;
    aoe_game_screen_map_scores_ror(&s, 1);
    assert(map_flag == 1 && help_y == 0xb8 && f.active[56] == 1 && f.redraws[1] == 1);
    assert(!strcmp(lines_out[56].text, "C: 300/200") && lines_out[56].font == 8);
    assert(!strcmp(lines_out[57].text, "A: 100/200") && lines_out[57].font == 0xb);
    assert(lines_out[57].colour == 0x2139c6 && lines_out[57].icon2 == (void *)7 && lines_out[57].frame2 == 0x11);
    assert(lines_out[57].icon1 == (void *)7 && lines_out[57].frame1 == 0x14);
    assert(!strcmp(lines_out[58].text, "B: 50/50") && lines_out[58].font == 8);
    assert(lines_out[58].icon2 == NULL && lines_out[58].frame1 == 0x13);
    assert(f.active[59] == 0);
    aoe_game_screen_map_scores_ror(&s, 0);
    assert(map_flag == 0 && help_y == 4 && f.active[56] == 0);
    is_net = 0;
    return 0;
}
