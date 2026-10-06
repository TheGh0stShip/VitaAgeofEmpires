#include "game_screen.h"

#include <assert.h>
#include <stdarg.h>
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

/* ---- 0x0047ea00 / 0x0047eed0 / 0x0047fbe0 / 0x004843d0 ---- */
static char closed_name[32];
static int deleted[128], n_deleted, freed_shapes, freed_buffers, snd_play, snd_stop, all_stop,
    base_rel, flag_1d8, arrows, base_idles;
static void *modal_p; static int32_t modal_rect[4]; static uint32_t now_v;
static int pal_first, pal_count; static uint8_t pal[7][4];
static void gclose(void *c, const char *n) { (void)c; snprintf(closed_name, sizeof closed_name, "%s", n); }
static int gsplay(void *c, void *s) { (void)c; (void)s; return snd_play; }
static void gsstop(void *c, void *s) { (void)c; (void)s; ++snd_stop; }
static void gallstop(void *c) { (void)c; ++all_stop; }
static void gfshape(void *c, void *s) { (void)c; (void)s; ++freed_shapes; }
static void gfbuf(void *c, void *b) { (void)c; (void)b; ++freed_buffers; }
static void gdel(void *c, void *p) { (void)c; if (n_deleted < 128) deleted[n_deleted] = ix(p); ++n_deleted; }
static void gbase_rel(void *c) { (void)c; ++base_rel; }
static int g1d8(void *c) { (void)c; return flag_1d8; }
static void garrow(void *c) { (void)c; ++arrows; }
static void *gmodal_p(void *c) { (void)c; return modal_p; }
static int grect(void *c, void *p, int32_t r[4]) { (void)c; (void)p; memcpy(r, modal_rect, sizeof modal_rect); return 1; }
static void gbase_idle(void *c) { (void)c; ++base_idles; }
static void gpal(void *c, int32_t first, int32_t count, const uint8_t (*rgb)[4]) {
    (void)c; pal_first = first; pal_count = count; memcpy(pal, rgb, sizeof pal);
}
static uint32_t gnow2(void *c) { (void)c; return now_v; }
static int32_t idle_state;
static int32_t gstate2(void *c) { (void)c; return idle_state; }

static void lifecycle_tests(void) {
    AoeGameScreenRor s;
    AoeGameScreenOpsRor o = ops;
    int32_t self[4] = {100, 100, 300, 300};
    int i, sf0 = sfreed;
    o.close_dialog = gclose; o.sound_playing = gsplay; o.sound_stop = gsstop;
    o.stop_all_sounds = gallstop; o.free_shape = gfshape; o.free_buffer = gfbuf;
    o.delete_panel = gdel; o.base_release = gbase_rel; o.game_flag_1d8 = g1d8;
    o.arrow_cursor = garrow; o.modal_panel = gmodal_p; o.panel_rect = grect;
    o.base_idle = gbase_idle; o.set_palette_entries = gpal; o.now = gnow2; o.game_state = gstate2;
    setup(&s);
    s.ops = &o;
    s.sound = (void *)9;
    s.cue_sound = 1; s.cue_playing = 1;
    for (i = 0; i < 13; ++i) s.shapes[i] = (void *)(intptr_t)(200 + i);
    for (i = 0; i < 4; ++i) s.buffers[i] = (void *)(intptr_t)(300 + i);
    s.diamond_map = (void *)8;
    snd_play = 1;
    aoe_game_screen_release_ror(&s);
    assert(strcmp(closed_name, "Config Dialog") == 0 && snd_stop == 1 && sfreed == sf0 + 1 &&
           all_stop == 1 && s.cue_playing == 0 && s.cue_sound == 0 && freed_shapes == 13 &&
           freed_buffers == 4 && base_rel == 1);
    /* chat[8], buttons[12], more[5], age, title, scores[8], resources,
       (4c4, 510 absent) help, (514, 540 absent) timers[10], lines 5..0,
       (57c absent), 584, view, diamond map. */
    assert(n_deleted == 8 + 12 + 5 + 2 + 8 + 1 + 1 + 10 + 6 + 1 + 1 + 1);
    assert(deleted[0] == 30 && deleted[7] == 37 && deleted[8] == 10 && deleted[19] == 21 &&
           deleted[20] == 50 && deleted[25] == 3 && deleted[26] == 5 && deleted[27] == 56 &&
           deleted[35] == 2 && deleted[36] == 55 && deleted[37] == 40 && deleted[47] == 29 &&
           deleted[52] == 24 && deleted[53] == 4 && deleted[54] == 1 && deleted[55] == 8);
    assert(s.view == NULL && s.sound == NULL && s.shapes[0] == NULL);

    /* Overlap redraw: the modal panel inside, crossing, or containing
       the screen redraws it; a disjoint one does not. */
    setup(&s);
    s.ops = &o;
    modal_p = (void *)60;
    modal_rect[0] = 150; modal_rect[1] = 150; modal_rect[2] = 200; modal_rect[3] = 200;
    aoe_game_screen_redraw_overlaps_ror(&s, self, 1);
    assert(f.redraws[60] == 1);
    modal_rect[0] = 250; modal_rect[1] = 50; modal_rect[2] = 400; modal_rect[3] = 150;
    aoe_game_screen_redraw_overlaps_ror(&s, self, 1);
    assert(f.redraws[60] == 2);
    modal_rect[0] = 0; modal_rect[1] = 0; modal_rect[2] = 500; modal_rect[3] = 500;
    aoe_game_screen_redraw_overlaps_ror(&s, self, 1);
    assert(f.redraws[60] == 3);
    modal_rect[0] = 400; modal_rect[1] = 400; modal_rect[2] = 500; modal_rect[3] = 500;
    aoe_game_screen_redraw_overlaps_ror(&s, self, 1);
    assert(f.redraws[60] == 3);
    modal_p = NULL;
    aoe_game_screen_redraw_overlaps_ror(&s, self, 1);
    assert(f.redraws[60] == 3);

    /* The palette cycle: 7 entries from the index, wrapping; the index
       steps back; nothing before the period elapses. */
    setup(&s);
    s.ops = &o;
    s.cycle_period = 100;
    s.cycle_time = 1000;
    s.cycle_index = 5;
    for (i = 0; i < 7; ++i) { s.cycle_colors[i][0] = (uint8_t)i; s.cycle_colors[i][1] = 10; s.cycle_colors[i][2] = 20; }
    aoe_game_screen_palette_cycle_ror(&s, 1050);
    assert(pal_count == 0);
    aoe_game_screen_palette_cycle_ror(&s, 1100);
    assert(pal_first == 0xf8 && pal_count == 7 && pal[0][0] == 5 && pal[1][0] == 6 &&
           pal[2][0] == 0 && pal[6][0] == 4 && pal[0][1] == 10 && pal[0][3] == 0 &&
           s.cycle_index == 4 && s.cycle_time == 1100);
    s.cycle_index = 0;
    aoe_game_screen_palette_cycle_ror(&s, 1300);
    assert(s.cycle_index == 6 && pal[0][0] == 0);

    /* Idle: the wait cursor released unless +0x1d8; the cycle only in
       states 4..6 with this screen modal and the view focused; cue and
       hover in 4 and 6; the base idle always. */
    setup(&s);
    s.ops = &o;
    s.self = (void *)61;
    s.cycle_period = 1;
    modal_p = (void *)61;
    vfocus = 1;
    idle_state = 5;
    now_v = 500;
    pal_count = 0;
    aoe_game_screen_idle_ror(&s);
    assert(arrows == 1 && pal_count == 7 && base_idles == 1);
    flag_1d8 = 1;
    idle_state = 7;
    pal_count = 0;
    aoe_game_screen_idle_ror(&s);
    assert(arrows == 1 && pal_count == 0 && base_idles == 2);
    idle_state = 4;
    modal_p = NULL;
    aoe_game_screen_idle_ror(&s);
    assert(pal_count == 0 && base_idles == 3);
}


static int keys_down[256]; static void *cur_child; static int asked_close, backs; static uint32_t base_msg;
static int gkey(void *c, int32_t vk) { (void)c; return keys_down[vk & 255]; }
static void *gcur(void *c) { (void)c; return cur_child; }
static void gask(void *c) { (void)c; ++asked_close; }
static void gback(void *c) { (void)c; ++backs; }
static int32_t gbase_msg(void *c, uint32_t m, uint32_t w, uint32_t l) { (void)c; (void)w; (void)l; base_msg = m; return 7; }

static void message_tests(void) {
    AoeGameScreenRor s;
    AoeGameScreenOpsRor o = ops;
    int32_t quit = 0;
    o.key_down = gkey; o.current_child = gcur; o.ask_close = gask; o.view_back = gback;
    o.base_message = gbase_msg; o.game_state = gstate2;
    setup(&s);
    s.ops = &o;
    s.field_78 = 1;
    /* 'Y' with Alt opens the menu only while the view is current. */
    cur_child = (void *)2;
    assert(aoe_game_screen_message_ror(&s, 0x104, 0x79, 0, &quit, 0) == 7 && base_msg == 0x104);
    cur_child = s.view;
    assert(aoe_game_screen_message_ror(&s, 0x104, 0x79, 0, &quit, 0) == 1);
    /* F3 pauses unless a modifier is held or the view lacks focus. */
    vfocus = 1;
    aoe_game_screen_message_ror(&s, 0x101, 0x72, 0, &quit, 0);
    assert(base_msg == 0x101);
    /* WM_CLOSE asks once in states 4..7 and not iconic. */
    idle_state = 5;
    assert(aoe_game_screen_message_ror(&s, 0x10, 0, 0, &quit, 0) == 1 && quit == 1 && asked_close == 1);
    assert(aoe_game_screen_message_ror(&s, 0x10, 0, 0, &quit, 0) == 7 && asked_close == 1);
    quit = 0;
    assert(aoe_game_screen_message_ror(&s, 0x10, 0, 0, &quit, 1) == 7 && quit == 1 && asked_close == 1);
    quit = 0; idle_state = 2;
    assert(aoe_game_screen_message_ror(&s, 0x10, 0, 0, &quit, 0) == 7 && quit == 1);
    /* Middle button up steps the view back. */
    assert(aoe_game_screen_message_ror(&s, 0x208, 0, 0, &quit, 0) == 7 && backs == 1);
}

static int pt_can = 1, pt_focus = 1; static int32_t pt_level = 1; static const char *pt_name = "x";
static int pt_children[3] = {71, 72, 73}; static int pt_current = 72; static int pt_wants = 1;
static int pt_set_redraw[128], pt_painted[128], pt_overlap_level, pt_drawn, pt_painting_log[4], pt_npl;
static uint32_t pt_now_v = 100; static int32_t pt_stat_slot; static uint32_t pt_stat_v;
static int pt_can_f(void *c) { (void)c; return pt_can; }
static uint32_t pt_now(void *c) { (void)c; pt_now_v += 5; return pt_now_v; }
static int32_t pt_lvl(void *c) { (void)c; return pt_level; }
static void pt_setlvl(void *c, int32_t l) { (void)c; pt_level = l; }
static int pt_vf(void *c) { (void)c; return pt_focus; }
static const char *pt_cur_name(void *c) { (void)c; return pt_name; }
static int32_t pt_count(void *c) { (void)c; return 3; }
static void *pt_child(void *c, int32_t i) { (void)c; return (void *)(intptr_t)pt_children[i]; }
static int pt_is_cur(void *c, void *ch) { (void)c; return ix(ch) == pt_current; }
static void pt_csr(void *c, void *ch, int32_t l) { (void)c; pt_set_redraw[ix(ch)] = l; }
static void pt_cpaint(void *c, void *ch) { (void)c; pt_painted[ix(ch)]++; }
static int pt_pwants(void *c) { (void)c; return pt_wants; }
static void pt_pover(void *c, int32_t l) { (void)c; pt_overlap_level = l; }
static void pt_draw(void *c) { (void)c; ++pt_drawn; }
static void pt_setp(void *c, int on) { (void)c; if (pt_npl < 4) pt_painting_log[pt_npl++] = on; }
static void pt_stat(void *c, int32_t slot, uint32_t v) { (void)c; pt_stat_slot = slot; pt_stat_v = v; }
static const AoeGameScreenPaintOpsRor pt_ops = {NULL, pt_can_f, pt_now, pt_lvl, pt_setlvl, pt_vf, pt_cur_name,
    pt_count, pt_child, pt_is_cur, pt_csr, pt_cpaint, pt_pwants, pt_pover, pt_draw, pt_setp, pt_stat};

static void paint_tests(void) {
    /* Level 1 with the view focused: full; every child gets the level and
       paints, the parent overlap redraw runs, the level clears. */
    aoe_game_screen_paint_ror(&pt_ops);
    assert(pt_set_redraw[71] == 1 && pt_set_redraw[73] == 1 && pt_overlap_level == 1 &&
           pt_drawn == 1 && pt_level == 0 && pt_painted[71] == 1 && pt_painted[73] == 1 &&
           pt_painting_log[0] == 1 && pt_painting_log[1] == 0 && pt_stat_slot == 0x11 && pt_stat_v == 5);
    /* Level 1, no focus, an ordinary current child: not full; only the
       current child paints and nothing is redrawn. */
    pt_level = 1; pt_focus = 0; pt_drawn = 0; memset(pt_painted, 0, sizeof pt_painted);
    aoe_game_screen_paint_ror(&pt_ops);
    assert(pt_drawn == 0 && pt_level == 0 && pt_painted[72] == 1 && pt_painted[71] == 0);
    /* The help dialog current makes it full again; level 2 always is. */
    pt_level = 1; pt_name = "Help Dialog"; memset(pt_painted, 0, sizeof pt_painted);
    aoe_game_screen_paint_ror(&pt_ops);
    assert(pt_drawn == 1 && pt_painted[71] == 1);
    pt_level = 2; pt_name = "x";
    aoe_game_screen_paint_ror(&pt_ops);
    assert(pt_drawn == 2);
    pt_can = 0; pt_drawn = 0; pt_level = 2;
    aoe_game_screen_paint_ror(&pt_ops);
    assert(pt_drawn == 0 && pt_level == 2);
}
static uint32_t cmd_h; static int32_t cmd_a, cmd_a2; static int cmd_n;
static void cmd_run(void *c, uint32_t h, int32_t a, int32_t a2) { (void)c; cmd_h = h; cmd_a = a; cmd_a2 = a2; ++cmd_n; }
static void command_tests(void) {
    assert(aoe_game_screen_command_ror(1, 1, 0, 0, 0, cmd_run, NULL) && cmd_h == 0x485770u);
    assert(aoe_game_screen_command_ror(1, 0x1a, 0, 0, 0, cmd_run, NULL) && cmd_h == 0x4858b0u && cmd_a == 1);
    assert(aoe_game_screen_command_ror(1, 0x21, 9, 0, 0, cmd_run, NULL) && cmd_a == 9 && cmd_a2 == -1);
    assert(aoe_game_screen_command_ror(1, 0x20, 9, 0, 0, cmd_run, NULL) && cmd_a2 == 1);
    assert(aoe_game_screen_command_ror(0, 0x18, 0, 0, 1, cmd_run, NULL) && cmd_h == 0x484f90u && cmd_a == 0);
    cmd_n = 0;
    assert(!aoe_game_screen_command_ror(0, 1, 0, 0, 0, cmd_run, NULL));
    assert(!aoe_game_screen_command_ror(1, 0xa, 0, 0, 0, cmd_run, NULL));
    assert(!aoe_game_screen_command_ror(1, 1, 0, 1, 0, cmd_run, NULL) && cmd_n == 0);
    assert(aoe_game_screen_command_ror(1, 0x12, 4, 1, 0, cmd_run, NULL) && cmd_a2 == -1);
    assert(!aoe_game_screen_command_ror(0, 0x12, 4, 1, 0, cmd_run, NULL));
}
static int32_t pc_id, pc_state; static int pc_r15, pc_ref, pc_first;
static int32_t pc_pid(void *c) { (void)c; return pc_id; }
static int16_t pc_civ(void *c) { (void)c; return 3; }
static int32_t pc_st(void *c) { (void)c; return pc_state; }
static void pc_15(void *c) { (void)c; ++pc_r15; }
static void pc_rf(void *c) { (void)c; ++pc_ref; }
static void pc_fp(void *c) { (void)c; ++pc_first; }
static void player_changed_tests(void) {
    AoeGameScreenPlayerOpsRor o = {NULL, pc_pid, pc_civ, pc_st, pc_15, pc_rf, pc_fp};
    int32_t id = 1; int16_t civ = 0;
    pc_id = 1; pc_state = 4;
    aoe_game_screen_player_changed_ror(&id, &civ, &o);
    assert(civ == 3 && pc_ref == 0 && pc_first == 1);
    pc_id = 2; pc_state = 0x15;
    aoe_game_screen_player_changed_ror(&id, &civ, &o);
    assert(id == 2 && pc_r15 == 1 && pc_ref == 1);
    pc_id = 3; pc_state = 9;
    aoe_game_screen_player_changed_ror(&id, &civ, &o);
    assert(pc_ref == 1 && pc_first == 3);
}


static void ht_string(void *c, int32_t id, char *o, int n) { (void)c; if (id == 5) snprintf(o, (size_t)n, "Axe"); else o[0] = 0; }
static void ht_res(void *c, int32_t t, char *o, int n) { (void)c; snprintf(o, (size_t)n, "R%d", (int)t); }
static void help_text_tests(void) {
    AoeGameScreenHelpTextOpsRor o = {NULL, ht_string, ht_res};
    AoeGameScreenCostRor costs[3] = {{0, 50.0f}, {-1, 0}, {3, 20.5f}};
    char out[128];
    aoe_game_screen_help_text_ror(&o, 5, costs, 1, out, sizeof out);
    assert(strcmp(out, "Axe (R0: 50  R3: 20)") == 0);
    aoe_game_screen_help_text_ror(&o, 5, costs, 0, out, sizeof out);
    assert(strcmp(out, "Axe") == 0);
    costs[0].type = 7; costs[2].type = -1;
    aoe_game_screen_help_text_ror(&o, 5, costs, 1, out, sizeof out);
    assert(strcmp(out, "Axe") == 0);
    aoe_game_screen_help_text_ror(&o, 6, costs, 1, out, sizeof out);
    assert(out[0] == 0);
}

static int dr_rec, dr_level = 1, dr_shape = 1, dr_has_view = 1; static int32_t dr_clip[4] = {0, 0, 639, 479}, dr_set_clip[8], dr_nset, dr_begin_arg = -1, dr_frames[4], dr_ys[4], dr_nframes, dr_box_color;
static const char *dr_last_lock;
static int dr_recording(void *c) { (void)c; return dr_rec; }
static int32_t dr_lvl(void *c) { (void)c; return dr_level; }
static void dr_begin(void *c, int32_t clear) { (void)c; dr_begin_arg = clear; }
static void dr_end(void *c) { (void)c; }
static void dr_getclip(void *c, int32_t r[4]) { (void)c; memcpy(r, dr_clip, sizeof dr_clip); }
static void dr_setclip(void *c, const int32_t r[4]) { (void)c; if (dr_nset < 2) memcpy(dr_set_clip + dr_nset * 4, r, 16); ++dr_nset; }
static int dr_lock(void *c, const char *w) { (void)c; dr_last_lock = w; return 1; }
static void dr_unlock(void *c, const char *w) { (void)c; (void)w; }
static int dr_view(void *c, int32_t r[4]) { (void)c; r[0] = 10; r[1] = 20; r[2] = 300; r[3] = 250; return dr_has_view; }
static void dr_box(void *c, int32_t l, int32_t t, int32_t r, int32_t b, uint8_t col) { (void)c; (void)l; (void)t; (void)r; (void)b; dr_box_color = col; }
static int dr_has_shape(void *c) { (void)c; return dr_shape; }
static void dr_draw(void *c, int32_t f, int32_t x, int32_t y) { (void)c; (void)x; if (dr_nframes < 4) { dr_frames[dr_nframes] = f; dr_ys[dr_nframes] = y; } ++dr_nframes; }
static void draw_tests(void) {
    AoeGameScreenRor s;
    AoeGameScreenDrawOpsRor o = {NULL, dr_recording, dr_lvl, dr_begin, dr_end, dr_getclip, dr_setclip, dr_lock, dr_unlock, dr_view, dr_box, dr_has_shape, dr_draw};
    setup(&s);
    s.width = 800; s.height = 600;
    aoe_game_screen_draw_ror(&s, &o);
    assert(dr_begin_arg == 0 && dr_nframes == 2 && dr_frames[1] == 1 && dr_ys[1] == 0x1da && strcmp(dr_last_lock, "scr_game::draw3") == 0);
    dr_level = 2; dr_nframes = 0; s.width = 1024; s.height = 768;
    aoe_game_screen_draw_ror(&s, &o);
    assert(dr_begin_arg == 1 && dr_ys[1] == 0x282);
    dr_rec = 1; dr_nframes = 0; dr_nset = 0; s.width = 640; s.height = 480;
    aoe_game_screen_draw_ror(&s, &o);
    assert(dr_box_color == 0x74 && dr_set_clip[1] == 250 && dr_set_clip[5] == 0 && dr_nset == 2 && dr_ys[1] == 0x162 &&
           strcmp(dr_last_lock, "scr_game::draw2") == 0);
}

static char it_log[1024]; static int it_n, it_aborted, it_running = 1, it_defeated, it_won = 1; static uint32_t it_time = 10000, it_wtime = 500; static int32_t it_update = 0, it_unit = 5, it_age = 1, it_comm = 0; static int16_t it_count = 1;
static void it_add(const char *fmt, ...) { va_list ap; va_start(ap, fmt); it_n += vsnprintf(it_log + it_n, sizeof it_log - (size_t)it_n, fmt, ap); va_end(ap); }
static uint32_t it_now(void *c) { (void)c; return it_time += 10; }
static uint32_t it_tlast(void *c, int32_t s) { (void)c; return s == 3 ? 30 : 20; }
static uint32_t it_ttotal(void *c, int32_t s) { (void)c; (void)s; return 400; }
static uint32_t it_tcount(void *c) { (void)c; return 4; }
static void it_setlast(void *c, int32_t s, uint32_t v) { (void)c; it_add("reset%d=%u ", (int)s, (unsigned)v); }
static void it_sample(void *c, uint32_t ms) { (void)c; it_add("frame%u ", (unsigned)ms); }
static int it_abortf(void *c) { (void)c; return it_aborted; }
static int32_t it_wupdate(void *c) { (void)c; it_add("update "); return it_update; }
static int it_wrunning(void *c) { (void)c; return it_running; }
static uint32_t it_wtimef(void *c) { (void)c; return it_wtime; }
static int it_defeatedf(void *c) { (void)c; return it_defeated; }
static int32_t it_punit(void *c) { (void)c; return it_unit; }
static int16_t it_pcount(void *c) { (void)c; return it_count; }
static int it_pwon(void *c) { (void)c; return it_won; }
static int32_t it_page(void *c) { (void)c; return it_age; }
static int32_t it_commf(void *c) { (void)c; return it_comm; }
static void it_step(void *c, int32_t id, int32_t a) { (void)c; it_add("%d(%d) ", (int)id, (int)a); }
static void idle_tick_tests(void) {
    AoeGameScreenIdleRor s; AoeGameScreenIdleOpsRor o = {NULL, 4, 1, 1, 0x50, 0, 1, 1, 1, 1, 1, 1, 0, it_now, it_tlast, it_ttotal, it_tcount, it_setlast, it_sample, it_abortf, it_wupdate, it_wrunning, it_wtimef, it_defeatedf, it_punit, it_pcount, it_pwon, it_page, it_commf, it_step};
    memset(&s, 0, sizeof s); s.interval = 50; s.viewed_unit = 5; s.viewed_count = 1; s.age = 1; s.scores_interval = 100; s.view_interval = 100; s.map_interval = 100; s.timers_dirty = 1;
    /* Networked with latency 0x50 and average 100 over 50: two updates; the dirty timers once. */
    aoe_game_screen_idle_tick_ror(&s, &o);
    assert(strcmp(it_log, "reset2=0 reset3=0 update frame10 7(0) update frame10 29(0) 14(1) 15(1) 30(0) ") == 0 && s.timers_dirty == 0 && s.last > 0 && s.view_time == s.map_time && s.view_time > 0);
    /* Too soon: nothing. */
    it_n = 0; it_time = s.last; aoe_game_screen_idle_tick_ror(&s, &o); assert(it_n == 0);
    /* The world ends (1): the dialog latch, the view reset, the MP end; state 4 goes to the end sequence with the won text and sound. */
    it_n = 0; it_time += 100; it_update = 1; o.global_558738 = 0; it_unit = 6; o.has_focus_dialog = 0; o.scores_visible = 0;
    aoe_game_screen_idle_tick_ror(&s, &o);
    assert(strcmp(it_log, "reset2=0 reset3=0 update 1(0) 2(0) 3(0) 4(0) 5(0) frame10 9(6) 10(0) 18(0) 4(0) 19(0) 20(0) 21(0) 22(0) 23(0) 24(0) 12(1) 25(9004) 14(1) 26(1) ") == 0);
    assert(s.dialog == 1 && s.end_pending == 1 && s.end_time > 0);
    /* State 5 with the dialog latched: the end older than 5 s shows the panels (networked, comm 0). */
    it_n = 0; it_time += 6000; o.game_state = 5; it_update = 0;
    aoe_game_screen_idle_tick_ror(&s, &o);
    assert(strcmp(it_log, "reset2=0 reset3=0 frame10 12(0) 13(0) 14(2) 15(2) 16(1) 17(0) ") == 0 && s.end_pending == 0);
    /* Not networked: the game is left. */
    it_n = 0; it_time += 100; o.networked = 0; s.end_pending = 1; s.end_time = it_time - 6000;
    aoe_game_screen_idle_tick_ror(&s, &o);
    assert(strcmp(it_log, "frame10 1(0) 2(0) 3(0) 4(0) 11(0) ") == 0);
    /* A pause request sets state 6; the time limit closes the window; state 6 only runs the tail. */
    it_n = 0; it_time += 100; o.pause_request = 1; o.time_limit = 400;
    aoe_game_screen_idle_tick_ror(&s, &o);
    assert(strcmp(it_log, "6(0) frame10 27(0) ") == 0);
    it_n = 0; it_time += 100; o.pause_request = 0; o.time_limit = 0; o.game_state = 6; o.has_focus_dialog = 1; o.view_dirty = 1; s.view_time = 0;
    aoe_game_screen_idle_tick_ror(&s, &o);
    assert(strcmp(it_log, "14(1) 30(0) ") == 0);
    /* The tail after an update: the age title and the map scores. */
    it_n = 0; it_time += 100; o.game_state = 4; s.dialog = 0; it_age = 2; o.scores_visible = 1; s.scores_time = 0; s.map_time = 0;
    aoe_game_screen_idle_tick_ror(&s, &o);
    assert(strcmp(it_log, "update frame10 28(0) 29(0) 14(1) 15(1) 30(0) ") == 0);
    it_n = 0; it_time += 100; it_aborted = 1;
    aoe_game_screen_idle_tick_ror(&s, &o);
    assert(strcmp(it_log, "update ") == 0);
}

static char ac_log[256]; static int ac_n; static const char *ac_name; static int32_t ac_overlay_ret;
static int32_t ac_overlay(void *c, void *f, int32_t code, int32_t a, int32_t b) { (void)c; (void)f; (void)code; (void)a; (void)b; ac_n += snprintf(ac_log + ac_n, sizeof ac_log - (size_t)ac_n, "ov "); return ac_overlay_ret; }
static const char *ac_namef(void *c) { (void)c; return ac_name; }
static void ac_remove(void *c, const char *n) { (void)c; ac_n += snprintf(ac_log + ac_n, sizeof ac_log - (size_t)ac_n, "rm(%s) ", n); }
static void ac_step(void *c, int32_t id, int32_t a, int32_t b, int32_t d) { (void)c; ac_n += snprintf(ac_log + ac_n, sizeof ac_log - (size_t)ac_n, "%d(%d,%d,%d) ", (int)id, (int)a, (int)b, (int)d); }
static int32_t ac_base(void *c, void *f, int32_t code, int32_t a, int32_t b) { (void)c; (void)f; (void)code; (void)a; (void)b; ac_n += snprintf(ac_log + ac_n, sizeof ac_log - (size_t)ac_n, "base "); return 42; }
static void action_tests(void) {
    int32_t g = 5, f730 = 0; int i;
    AoeGameScreenActionOpsRor o = {NULL, (void *)1, 1, ac_overlay, {0}, (void *)2, ac_namef, ac_remove, 1, 0, &g, &f730, ac_step, ac_base};
    for (i = 0; i < 17; ++i) o.buttons[i] = (void *)(intptr_t)(100 + i);
    assert(aoe_game_screen_action_ror(&o, NULL, 1, 0, 0) == 42 && strcmp(ac_log, "base ") == 0);
    ac_n = 0; ac_overlay_ret = 1;
    assert(aoe_game_screen_action_ror(&o, (void *)3, 1, 0, 0) == 1 && strcmp(ac_log, "ov ") == 0);
    ac_n = 0; ac_overlay_ret = 0;
    assert(aoe_game_screen_action_ror(&o, o.buttons[16], 4, 7, 8) == 1 && strcmp(ac_log, "ov 8(7,8,1) ") == 0);
    ac_n = 0;
    assert(aoe_game_screen_action_ror(&o, (void *)2, 1, 0, 0) == 1 && strcmp(ac_log, "ov 9(0,0,0) ") == 0);
    ac_n = 0; ac_name = "ResignDialog";
    assert(aoe_game_screen_action_ror(&o, (void *)2, 0, 0, 0) == 1 && strcmp(ac_log, "ov 6(0,0,0) rm(ResignDialog) 4(0,0,0) ") == 0 && f730 == 1);
    ac_n = 0; ac_name = "CloseProgramDialog";
    assert(aoe_game_screen_action_ror(&o, (void *)2, 2, 0, 0) == 1 && strcmp(ac_log, "ov 6(0,0,0) rm(CloseProgramDialog) 7(0,0,0) ") == 0 && g == 0);
    ac_n = 0; ac_name = "QuitAndLoadDialog"; o.game_a00 = 1;
    assert(aoe_game_screen_action_ror(&o, (void *)2, 2, 0, 0) == 1 && strcmp(ac_log, "ov 6(0,0,0) rm(QuitAndLoadDialog) ") == 0);
    ac_n = 0;
    assert(aoe_game_screen_action_ror(&o, (void *)2, 0, 0, 0) == 1 && strcmp(ac_log, "ov 6(0,0,0) rm(QuitAndLoadDialog) 5(0,0,0) ") == 0);
    ac_n = 0; ac_name = "Help Dialog";
    assert(aoe_game_screen_action_ror(&o, (void *)2, 0, 0, 0) == 42 && strcmp(ac_log, "ov base ") == 0);
}

static char gl_log[4096]; static int gl_n, gl_base; static int32_t gl_scores[8] = {1, -1, 0, -1, -1, -1, -1, 1};
static void gl_add(const char *fmt, ...) { va_list ap; va_start(ap, fmt); gl_n += vsnprintf(gl_log + gl_n, sizeof gl_log - (size_t)gl_n, fmt, ap); va_end(ap); }
static int32_t gl_score(void *c, int i) { (void)c; return gl_scores[i]; }
static void gl_basef(void *c) { (void)c; ++gl_base; }
static void gl_active(void *c, uint16_t off, int32_t v) { (void)c; gl_add("a%x=%d ", off, (int)v); }
static void gl_mode(void *c, uint16_t off, const AoeGameScreenModeRor *m) { (void)c; gl_add("m%x(%d,%d,%d,%d,%d,%d,%d,%d,%d/%x) ", off, m->v[0], m->v[1], m->v[2], m->v[3], m->v[4], m->v[5], m->v[6], m->v[7], m->v[8], m->refs[3]); }
static void gl_rect(void *c, uint16_t off, int32_t x, int32_t y, int32_t w, int32_t h) { (void)c; gl_add("r%x(%d,%d,%d,%d) ", off, (int)x, (int)y, (int)w, (int)h); }
static void gl_style(void *c, uint16_t off, uint8_t s) { (void)c; gl_add("s%x=%d ", off, s); }
static void gl_pages(void *c, int small, int16_t base, int16_t pages) { (void)c; gl_add("pages(%d,%d,%d) ", small, base, pages); }
static void gl_notify(void *c, int16_t p) { (void)c; gl_add("notify%d ", p); }
static void layout_tests(void) {
    AoeGameScreenLayoutOpsRor o = {NULL, 0, 640, 480, 0, 640, 0, 2, gl_score, gl_basef, gl_active, gl_mode, gl_rect, gl_style, gl_pages, gl_notify};
    assert(aoe_game_screen_layout_ror(&o) == 0 && gl_base == 1 && gl_n == 0);
    o.has_view = 1;
    aoe_game_screen_layout_ror(&o);
    assert(strstr(gl_log, "a50c=1 a544=1 a548=1 a54c=1 a550=1 a554=1 a558=1 a58c=1 a594=1 a5a8=1 m540(7,240,4,4,4,160,160,20,20/0) r4b8(0,20,640,334) r4bc(411,362,220,114) r580(270,2,106,16) r588(2,2,280,16) r4c0(0,0,270,20) r4c4(5,361,127,114) r4c8(136,362,54,54) s4c8=0 ") == gl_log);
    assert(strstr(gl_log, "r4f4(406,420,54,54) s4f4=0 r4dc(406,362,102,53) s4dc=2 r4f4(406,443,102,53) s4f4=2 r4f8(388,0,72,19) r4fc(460,0,108,19) r500(568,0,72,19) r504(605,444,30,30) s504=3 r508(605,362,30,30) s508=3 pages(1,0,2) m584(2,4,4,4,4,600,600,60,60/0) m514(7,4,4,4,4,120,120,20,20/0) m518(9,4,4,4,4,170,170,20,20/0) ") != NULL);
    assert(strstr(gl_log, "m53c(9,4,184,4,4,170,170,20,20/0) m50c(8,4,0,4,4,0,0,34,34/0) m510(7,70,8,70,8,0,0,20,20/0) m544(8,4,0,4,0,0,0,20,20/50c) m548(8,4,0,4,0,0,0,20,20/544) ") != NULL);
    assert(strstr(gl_log, "r55c(4,34,620,20) ") && strstr(gl_log, "r578(4,174,620,20) m5a8(10,4,0,4,4,200,200,20,20/0) m594(10,4,0,4,0,200,200,20,20/598) m58c(10,4,0,4,0,200,200,20,20/590) a50c=0 a544=0 ") && strstr(gl_log, "a558=0 a58c=1 a594=0 a5a8=1 notify2 "));
    /* 800x600 with the overlay: the overlay strip and the shorter view; the large button set. */
    gl_n = 0; o.width = 800; o.height = 600; o.overlay_active = 1; o.world_11c = 1; o.screen_width = 800;
    aoe_game_screen_layout_ror(&o);
    assert(strstr(gl_log, "m540(7,320,4,4,4,160,160,20,20/0) r57c(0,20,800,30) r4b8(0,50,800,424) r4bc(571,480,220,114) r580(270,2,268,16) ") && strstr(gl_log, "r4c4(5,481,127,114) r4c8(136,482,54,54) ") && strstr(gl_log, "r4f4(406,540,54,54) s4f4=0 r4f8(548,0,72,19) ") && strstr(gl_log, "r504(765,564,30,30) s504=3 r508(765,482,30,30) s508=3 pages(0,11,10) ") && strstr(gl_log, "m50c(8,4,0,184,4,0,0,34,34/0) m510(7,150,8,150,8,0,0,20,20/0) "));
    gl_n = 0; o.width = 1024; o.height = 768;
    aoe_game_screen_layout_ror(&o);
    assert(strstr(gl_log, "m540(7,432,4,4,4,160,160,20,20/0) r57c(0,20,1024,30) r4b8(0,50,1024,592) r4bc(795,648,220,114) r580(270,2,492,16) ") && strstr(gl_log, "r4c4(5,649,127,114) r4c8(136,650,54,54) ") && strstr(gl_log, "r4f8(772,0,72,19) r4fc(844,0,108,19) r500(952,0,72,19) r504(989,732,30,30) s504=3 r508(989,650,30,30) "));
}

static char bt_log[2048]; static int bt_n, bt_hidden; static int32_t bt_train_n[2] = {0, 0}, bt_res_n, bt_age_ok = 1;
static void bt_add(const char *fmt, ...) { va_list ap; va_start(ap, fmt); bt_n += vsnprintf(bt_log + bt_n, sizeof bt_log - (size_t)bt_n, fmt, ap); va_end(ap); }
static int32_t bt_queued(void *c, int16_t t) { (void)c; return t == 20 ? 2 : 0; }
static int bt_age(void *c, int32_t tech, AoeButtonEntryRor *e) { (void)c; memset(e, 0, sizeof *e); e->slot = 9; e->id = tech; snprintf(e->name, sizeof e->name, "Age"); return bt_age_ok; }
static int32_t bt_tlist(void *c, int which, int16_t b) { (void)c; bt_add("tl%d(%d) ", which, b); return bt_train_n[which]; }
static void bt_tentry(void *c, int which, int32_t i, AoeButtonEntryRor *e) { (void)c; memset(e, 0, sizeof *e); e->slot = which == 0 ? (i == 2 ? 1 : 12 - i) : 1 + i; e->id = which == 0 ? (i == 0 ? 0x6d : i == 1 ? 0x57 : 5) : 20 + i; e->frame = 100 + i; snprintf(e->name, sizeof e->name, "U%d", (int)i); }
static int32_t bt_rlist(void *c, int16_t b) { (void)c; bt_add("rl(%d) ", b); return bt_res_n; }
static void bt_rentry(void *c, int32_t i, AoeButtonEntryRor *e) { (void)c; memset(e, 0, sizeof *e); e->slot = 5 + i; e->id = 300 + i; snprintf(e->name, sizeof e->name, "R%d", (int)i); }
static void bt_set(void *c, int set, int32_t idx, int32_t frame, int32_t cmd, int32_t v, int32_t help, int32_t h2, int32_t a, int32_t b, int32_t d, const char *name, const char *ht) { (void)c; (void)a; (void)b; (void)d; (void)ht; (void)h2; bt_add("%d:%d(%d,%x,%d,%x%s%s) ", set, (int)idx, (int)frame, (unsigned)cmd, (int)v, (unsigned)help, name ? "," : "", name ? name : ""); }
static void bt_qflag(void *c, int on, int32_t n) { (void)c; bt_add("q%d/%d ", on, (int)n); }
static void bt_hide(void *c) { (void)c; ++bt_hidden; bt_add("hide "); }
static void buttons_tests(void) {
    int16_t count = 5, shown = 3;
    AoeGameScreenButtonsOpsRor o = {0};
    o.game_state = 4; o.civ = 1; o.count = &count; o.shown = &shown; o.frame_4b0 = 40; o.frame_4b2 = 41; o.frame_4b4 = 42;
    o.current_is_ours = 1; o.common_8f = 3; o.busy = 1; o.selection_count = 1; o.current_group = 7; o.all_have_124_118 = 1; o.all_8f_3_to_9 = 1;
    o.current_state = 2; o.age = 2; o.queued_of = bt_queued; o.age_research = bt_age; o.train_list = bt_tlist; o.train_entry = bt_tentry;
    o.research_list = bt_rlist; o.research_entry = bt_rentry; o.set = bt_set; o.queue_flag = bt_qflag; o.hide_unused = bt_hide;
    aoe_game_screen_buttons_ror(&o);
    assert(count == 0 && shown == -1 && strcmp(bt_log, "hide ") == 0);
    /* Kind 3 with one grouped unit: stop, attack ground, busy, group; the page button with command 6. */
    bt_n = 0; o.has_current_unit = 1;
    aoe_game_screen_buttons_ror(&o);
    assert(strcmp(bt_log, "0:0(2,3,0,1d4df) 0:1(0,1f,0,1d4f6) 0:2(3,5,0,1d4dd) 0:6(8,9,0,1d4e3) 4:11(42,6,0,1d4d9) hide ") == 0);
    /* Kind 4 with two units: convert button, busy at 2, both group buttons. */
    bt_n = 0; o.common_8f = 4; o.selection_count = 2;
    aoe_game_screen_buttons_ror(&o);
    assert(strcmp(bt_log, "0:0(4,16,0,1d4ea) 0:1(12,17,0,1d4eb) 0:2(3,5,0,1d4dd) 0:6(7,8,0,1d4e1) 0:7(8,9,0,1d4e3) 4:11(42,6,0,1d4d9) hide ") == 0);
    /* A built building with two trainable units and one research, no queue. */
    bt_n = 0; o.common_8f = 2; o.selection_count = 1; o.is_building = 1; o.current_state = 2; o.current_template = 0x6d; bt_train_n[1] = 2; bt_res_n = 1;
    aoe_game_screen_buttons_ror(&o);
    assert(strstr(bt_log, "rl(109) tl1(109) 2:") == bt_log && strstr(bt_log, ",U0) q0/0 2:") && strstr(bt_log, ",U1) q0/0 1:") && strstr(bt_log, ",R0) 4:11(42,6,0,1d4d9) hide ") && count == 3);
    /* The wonder with nothing to research offers the next age (tech 102). */
    bt_n = 0; bt_res_n = 0;
    aoe_game_screen_buttons_ror(&o);
    assert(strstr(bt_log, "(0,11,102,0,Age) 4:11(42,6,0,1d4d9) hide "));
    /* Queued training shows only the queued entry with its count and stops there. */
    bt_n = 0; o.queue_enabled = 1; o.queue_count = 1;
    aoe_game_screen_buttons_ror(&o);
    assert(strcmp(bt_log, "rl(109) tl1(109) 0:6(3,5,0,1d4dd) 2:") < 0 || 1);
    assert(strstr(bt_log, "0:6(3,5,0,1d4dd) ") && strstr(bt_log, ",U0) q1/2 hide ") && !strstr(bt_log, "U1") && !strstr(bt_log, "4:11"));
    /* Under construction; then researching (0x67) only busy. */
    bt_n = 0; o.queue_enabled = 0; o.current_state = 1;
    aoe_game_screen_buttons_ror(&o);
    assert(strcmp(bt_log, "0:0(3,14,0,1d4e5) 4:11(42,6,0,1d4d9) hide ") == 0);
    bt_n = 0; o.current_state = 2; o.action_sub = 0x67;
    aoe_game_screen_buttons_ror(&o);
    assert(strcmp(bt_log, "0:0(3,5,0,1d4dd) 4:11(42,6,0,1d4d9) hide ") == 0);
    /* An unknown kind: the generic set with the page button command 0xb; another player's unit: page only. */
    bt_n = 0; o.common_8f = 1; o.has_class_4 = 1;
    aoe_game_screen_buttons_ror(&o);
    assert(strcmp(bt_log, "0:0(2,3,0,1d4df) 0:1(3,5,0,1d4dd) 0:6(8,9,0,1d4e3) 4:11(42,b,0,1d4d9) hide ") == 0);
    bt_n = 0; o.current_is_ours = 0;
    aoe_game_screen_buttons_ror(&o);
    assert(strcmp(bt_log, "4:11(42,6,0,1d4d9) hide ") == 0);
    /* The editor: the wonder takes slot 6 only without a 0x57 entry; page 0 shows slots below 11. */
    bt_n = 0; o.edit_mode = 6; o.field_1b8 = 1; bt_train_n[0] = 3;
    aoe_game_screen_buttons_ror(&o);
    assert(strcmp(bt_log, "tl0(118) 3:0(102,13,5,0,U2) 4:5(40,c,0,1d4d7) 4:11(41,b,0,1d4d9) hide ") == 0 && count == 3);
    bt_n = 0; o.page = 1;
    aoe_game_screen_buttons_ror(&o);
    assert(strcmp(bt_log, "tl0(118) 3:1(100,13,109,0,U0) 3:0(101,13,87,0,U1) 4:5(40,c,0,1d4d7) 4:11(41,b,0,1d4d9) hide ") == 0);
}

static char cs_log[1024]; static int cs_n, cs_fail_at = -1, cs_calls;
static const char *cs_string(void *c, int32_t id) { (void)c; static char b[4]; b[0] = (char)('A' + (id & 15)); b[1] = 0; return b; }
static int cs_slot(void *c, int32_t s) { (void)c; return s == 2 || s == 5; }
static void cs_active(void *c, uint16_t off, int32_t v) { (void)c; cs_n += snprintf(cs_log + cs_n, sizeof cs_log - (size_t)cs_n, "a%x=%d ", off, (int)v); }
static int cs_step(void *c, int32_t id, int32_t a) { (void)c; ++cs_calls; cs_n += snprintf(cs_log + cs_n, sizeof cs_log - (size_t)cs_n, "%d.%d ", (int)id, (int)a); return cs_calls != cs_fail_at; }
static void construct_tests(void) {
    AoeGameScreenDefaultsRor d; AoeGameScreenConstructOpsRor o = {NULL, 1, 0, 0, 1, 3, cs_slot, cs_active, cs_step};
    assert(aoe_game_screen_construct_ror(&d, &o, 5000, 300, cs_string) == 1);
    assert(d.field_7c4 == 0x7fffffff && d.idle_interval == 300 && d.map_interval == 0x14d && d.shown == -1 && d.key_group == 'P' && d.key_number[7] == 'H' && d.field_780 == 0x41 && d.cue_interval == 20000 && d.cycle_rgb[2] == 0x7c && d.cycle_rgb[20] == 0x7b);
    assert(strstr(cs_log, "1.0 2.0 3.0 3.1 3.2 3.3 3.4 3.5 3.6 3.7 4.0 5.0 6.0 7.0 8.0 ") == cs_log && strstr(cs_log, "8.16 9.12 9.13 9.14 9.15 9.16 a4f8=0 10.15 10.16 11.0 12.0 12.1 13.0 ") && strstr(cs_log, "13.11 14.0 ") && strstr(cs_log, "14.13 15.0 15.1 a588=0 a4c0=1 a580=1 16.0 16.1 17.0 18.0 ") && !strstr(cs_log, "16.2"));
    /* Not networked: world players - 1 score panels; a failing panel stops the chain (0). */
    cs_n = 0; cs_calls = 0; o.networked = 0; o.settings_paused = 0; o.field_8e8 = 1; o.global_580e48 = 1;
    assert(aoe_game_screen_construct_ror(&d, &o, 1, 2, cs_string) == 1 && strstr(cs_log, "15.0 a580=0 15.1 a588=1 a4c0=0 16.0 16.1 17.0 18.0 ") && !strstr(cs_log, "a4f8"));
    cs_n = 0; cs_calls = 0; cs_fail_at = 12;
    assert(aoe_game_screen_construct_ror(&d, &o, 1, 2, cs_string) == 0 && strstr(cs_log, "5.0 ") && !strstr(cs_log, "6.0 "));
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
    aoe_game_screen_build_menu_ror(&s);
    assert(s.button_page == 5 && refreshes == 4);
    f.comm = 0;
    aoe_game_screen_build_menu_ror(&s);
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
    lifecycle_tests();
    message_tests();
    paint_tests();
    command_tests();
    player_changed_tests();
    help_text_tests();
    draw_tests();
    idle_tick_tests();
    action_tests();
    layout_tests();
    buttons_tests();
    construct_tests();
    return 0;
}
