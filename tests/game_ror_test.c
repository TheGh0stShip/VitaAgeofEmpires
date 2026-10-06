#include "game_ror.h"

#include <assert.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int no_sound, base_called, pop = -1;
static void ns(void *c) { (void)c; no_sound = 1; }
static void sp(void *c, uint8_t v) { (void)c; pop = v; }
static void bc(void *c) { (void)c; base_called++; }
static float mx, my;
static void move(void *c, float x, float y) { (void)c; mx = x; my = y; }
static int32_t seq[8], seq_i;
static int32_t rnd(void *c) { (void)c; return seq[seq_i++ % 8]; }
static int removed;
static void rm(void *c, const char *n) { (void)c; (void)n; removed++; }
static uint8_t sent[0x200];
static int32_t sent_n;
static void send(void *c, const uint8_t *d, int32_t n) { (void)c; memcpy(sent, d, (size_t)n); sent_n = n; }
static const uint8_t *recv(void *c, int32_t *n) { (void)c; *n = sent_n; return sent; }
static int ended = -1, hidden, msgs[4], nmsg, base_msgs, helped;
static void cursor(void *c, int w) { (void)c; (void)w; }
static void hide(void *c) { (void)c; hidden++; }
static void endv(void *c, int skip) { (void)c; ended = skip; }
static void sendm(void *c, uint32_t w, uint32_t m) { (void)c; (void)w; msgs[nmsg++] = (int)m; }
static int32_t bproc(void *c, uint32_t m) { (void)c; (void)m; return ++base_msgs; }
static int32_t bpal(void *c) { (void)c; return 7; }
static void bact(void *c, int32_t a) { (void)c; (void)a; }
static int help(void *c) { (void)c; return ++helped; }

static int menu_ends, menu_mode, menu_track;
static char menu_file[16], menu_shown[40];
static int menu_make_ok(void *c) { (void)c; return 1; }
static int menu_make_fail(void *c) { (void)c; return 0; }
static void menu_end(void *c) { (void)c; ++menu_ends; }
static void menu_show(void *c, const char *n) { (void)c; snprintf(menu_shown, sizeof menu_shown, "%s", n); }
static void menu_set_mode(void *c, int32_t m) { (void)c; menu_mode = m; }
static void menu_play_track(void *c, int32_t t) { (void)c; menu_track = t; }
static void menu_play_file(void *c, const char *f) { (void)c; snprintf(menu_file, sizeof menu_file, "%s", f); }

static uint32_t lv_clock;
static int lv_polls, lv_resets, lv_pending = 3, lv_status, lv_unlink, lv_state = -1, lv_video = -1;
static void lv_reset(void *c) { (void)c; ++lv_resets; }
static void lv_poll(void *c) { (void)c; ++lv_polls; lv_clock += 100; if (lv_polls % 6 == 0 && lv_pending) --lv_pending; }
static int32_t lv_unacked(void *c) { (void)c; return lv_pending; }
static void lv_stat(void *c, int32_t id) { (void)c; lv_status = id; }
static uint32_t lv_now(void *c) { (void)c; return lv_clock; }
static void lv_unl(void *c, int32_t l) { (void)c; lv_unlink = l; }
static void lv_set_state(void *c, int32_t s) { (void)c; lv_state = s; }
static void lv_play(void *c, int32_t k, int w) { (void)c; (void)k; lv_video = w; }

static int w_made, w_deleted, w_ok, w_cursor, w_status;
static int w_obj;
static void *w_create(void *c) { (void)c; return w_made++ < 99 ? &w_obj : NULL; }
static int w_load(void *c, void *w) { (void)c; assert(w == &w_obj); return w_ok; }
static void w_delete(void *c, void *w) { (void)c; assert(w == &w_obj); ++w_deleted; }
static void w_cur(void *c, int wait) { (void)c; w_cursor = wait; }
static void w_stat(void *c, int32_t id) { (void)c; w_status = id; }
static void w_close(void *c) { (void)c; w_status = 0; }


static char rl_mode[4], rl_path[32], rl_out[256];
static int rl_fail, rl_closed;
static void *rl_open(void *c, const char *p, const char *m) { (void)c; snprintf(rl_path, sizeof rl_path, "%s", p); snprintf(rl_mode, sizeof rl_mode, "%s", m); return rl_fail ? NULL : (void *)rl_out; }
static void rl_write(void *c, void *f, const char *t) { (void)c; strcat((char *)f, t); }
static void rl_close(void *c, void *f) { (void)c; (void)f; ++rl_closed; }
static void rl_now(void *c, char *out, int32_t n) { (void)c; snprintf(out, (size_t)n, "Mon Oct  5 19:00:00 1998\n"); }

static void test_run_log(void) {
    AoeGameRorRunLogRor log = {NULL, 0, 0, rl_open, rl_write, rl_close, rl_now};
    aoe_game_ror_run_log_ror(&log, "x", 0);
    assert(rl_path[0] == 0 && !log.opened);
    log.enabled = 1;
    aoe_game_ror_run_log_ror(&log, "start", 1);
    assert(strcmp(rl_path, "c:\\aoerun.txt") == 0 && strcmp(rl_mode, "w") == 0);
    aoe_game_ror_run_log_ror(&log, "plain", 0);
    assert(strcmp(rl_mode, "a") == 0 && rl_closed == 2);
    assert(strcmp(rl_out, "Mon Oct  5 19:00:00 1998 - start\nplain\n") == 0);
    rl_fail = 1;
    aoe_game_ror_run_log_ror(&log, "lost", 0);
    assert(rl_closed == 2);
}

static char gd_log[2048];
static void gd_rm(void *c, const char *n) { (void)c; strcat(gd_log, n); strcat(gd_log, "|"); }
static void gd_win(void *c, uint32_t w) { (void)c; assert(w == 77); strcat(gd_log, "W|"); }
static void gd_free(void *c, uint32_t l) { (void)c; assert(l == 9); strcat(gd_log, "F|"); }
static void gd_base(void *c) { (void)c; strcat(gd_log, "B|"); }
static void gd_state(void *c, int32_t s) { (void)c; assert(s == 0); strcat(gd_log, "S|"); }
static int gd_open_ok, gd_fb_ok, gd_menu_ok;
static int gd_open(void *c, const char *n) { (void)c; assert(!strcmp(n, "scn")); strcat(gd_log, "O|"); return gd_open_ok; }
static int gd_fb(void *c, const char *n, int32_t f) { (void)c; (void)n; assert(f == 0); strcat(gd_log, "FB|"); return gd_fb_ok; }
static int gd_menu(void *c) { (void)c; strcat(gd_log, "M|"); return gd_menu_ok; }
static void gd_give(void *c) { (void)c; strcat(gd_log, "G|"); }
static void gd_step(void *c, int32_t id, int32_t a, int32_t b) {
    char t[32];
    (void)c;
    snprintf(t, sizeof t, "%d:%d:%d|", (int)id, (int)a, (int)b);
    strcat(gd_log, t);
}

static void test_destroy_editor_video(void) {
    void *screen = (void *)1;
    uint32_t lib = 9;
    AoeGameRorDestroyOpsRor d = {NULL, 5, 4, 77, &screen, &lib, gd_win, gd_rm, gd_rm, gd_free, gd_base};
    AoeGameRorEditorOpsRor e;
    AoeGameRorVideoCloseOpsRor v = {NULL, 1, 1, 640, 480, NULL, NULL, gd_step};
    int32_t flag = 0;
    gd_log[0] = 0;
    aoe_game_ror_destroy_ror(&d);
    assert(d.field_1250 == 0 && d.mode == 0 && d.video_window == 0 && screen == NULL && lib == 0);
    assert(!strncmp(gd_log, "W|Object List Dialog|", 21));
    assert(strstr(gd_log, "Game Screen|Video Screen|Create Dialog|"));
    assert(strstr(gd_log, "Logo2Screen|sounds.drs|graphics.drs|Terrain.drs|Border.drs|Interfac.drs|F|B|"));
    memset(&e, 0, sizeof e);
    e.remove_screen = gd_rm; e.set_state = gd_state; e.open_editor = gd_open;
    e.fallback = gd_fb; e.main_menu = gd_menu; e.give_up = gd_give;
    gd_log[0] = 0; gd_open_ok = 1;
    assert(aoe_game_ror_open_editor_ror(&e, "scn") == 1 && !strcmp(e.scenario_name, "scn"));
    assert(!strcmp(gd_log, "Scenario Editor Screen|S|O|"));
    gd_log[0] = 0; gd_open_ok = 0; gd_fb_ok = 0; gd_menu_ok = 0;
    assert(aoe_game_ror_open_editor_ror(&e, "scn") == 0 && !strcmp(gd_log, "Scenario Editor Screen|S|O|FB|M|G|"));
    gd_log[0] = 0; gd_fb_ok = 1;
    assert(aoe_game_ror_open_editor_ror(&e, "scn") == 0 && !strcmp(gd_log, "Scenario Editor Screen|S|O|FB|"));
    gd_log[0] = 0;
    v.redraw_flag = &flag;
    aoe_game_ror_close_video_ror(&v);
    assert(!strcmp(gd_log, "1:640:480|2:0:0|3:0:0|4:0:0|5:0:0|6:0:0|2:0:0|3:0:0|7:0:0|8:0:0|9:2:0|10:0:0|11:0:0|12:0:0|"));
    assert(flag == 1 && v.video_up == 0 && v.mode_switched == 0);
    gd_log[0] = 0;
    aoe_game_ror_close_video_ror(&v);
    assert(gd_log[0] == 0);
    v.video_up = 1;
    aoe_game_ror_close_video_ror(&v);
    assert(!strncmp(gd_log, "2:0:0|3:0:0|7:0:0|", 18) && v.video_up == 0);
}

static char vo_log[512];
static int vo_avail16 = 1, vo_set16 = 0, vo_avail8 = 1, vo_set8 = 1;
static void vo_prep(void *c) { (void)c; strcat(vo_log, "prep|"); }
static void vo_hide(void *c) { (void)c; strcat(vo_log, "hide|"); }
static int vo_avail(void *c, int32_t w, int32_t h, int32_t b) { (void)c; assert(w == 640 && h == 480); return b == 16 ? vo_avail16 : vo_avail8; }
static int vo_set(void *c, int32_t w, int32_t h, int32_t b) { (void)c; (void)w; (void)h; return b == 16 ? vo_set16 : vo_set8; }
static void vo_logf(void *c, const char *t) { (void)c; strcat(vo_log, t); strcat(vo_log, "|"); }
static void vo_state(void *c, int32_t s) { (void)c; assert(s == 1); strcat(vo_log, "st|"); }
static void vo_after(void *c) { (void)c; strcat(vo_log, "ac|"); }

static void test_open_video(void) {
    AoeGameRorVideoOpenOpsRor o;
    memset(&o, 0, sizeof o);
    o.full_screen = 1; o.screen_width = 800; o.screen_height = 600;
    o.prepare = vo_prep; o.hide_cursor = vo_hide; o.mode_available = vo_avail;
    o.set_mode = vo_set; o.log = vo_logf; o.state = vo_state; o.after = vo_after;
    vo_log[0] = 0;
    assert(aoe_game_ror_open_video_ror(&o) == 1);
    assert(!strcmp(vo_log, "prep|hide|640x480 16bit|640x480 8bit|640x480 8bit success|"
                           "video_changed_res=1, double_size=1|st|ac|"));
    assert(o.video_up == 1 && o.mode_16bit == 0 && o.old_width == 800 && o.old_height == 600);
    vo_log[0] = 0;
    assert(aoe_game_ror_open_video_ror(&o) == 1 && vo_log[0] == 0);
    o.video_up = 0; vo_set16 = 1;
    vo_log[0] = 0;
    aoe_game_ror_open_video_ror(&o);
    assert(o.mode_16bit == 1 && strstr(vo_log, "16bit success|video_changed_res=1"));
    o.video_up = 0; o.eight_bit_switch = 1;
    vo_log[0] = 0;
    aoe_game_ror_open_video_ror(&o);
    assert(!strcmp(vo_log, "prep|hide|video_changed_res=0, double_size=1|st|ac|") && o.mode_16bit == 0);
    o.video_up = 0; o.full_screen = 0;
    vo_log[0] = 0;
    aoe_game_ror_open_video_ror(&o);
    assert(!strcmp(vo_log, "prep|video_changed_res=0, double_size=1|st|ac|"));
}


static void stats_text_tests(void) {
    static AoeGameBaseRor g;
    char title[200], view[100];
    memset(&g, 0, sizeof g);
    aoe_game_ror_stats_text_ror(&g, 0, title, sizeof title, view, sizeof view);
    assert(strcmp(title, "f?") == 0 && strcmp(view, "avg view time=???") == 0);
    g.timing_504[1] = 2; g.timing_504[2] = 4; g.timing_504[7] = 30; g.timing_504[8] = 25;
    g.timing[6].field_4 = 20; g.timing[7].field_4 = 4; g.timing[17].field_4 = 6; g.timing[3].field_4 = 8;
    g.timing[1].field_4 = 10; g.timing[14].field_4 = 12; g.timing[12].field_4 = 14; g.timing[10].field_4 = 2;
    g.timing[13].field_4 = 16; g.timing[4].field_4 = 18;
    g.timing[6].field_1c = 99; g.timing[4].field_1c = 7;
    aoe_game_ror_stats_text_ror(&g, 0, title, sizeof title, view, sizeof view);
    assert(strcmp(title, "t30,f25,avg(r(v10,m2,o3),s4,u5,c6,l8,p8,o9)") == 0);
    assert(strcmp(view, "avg view time=5, count=4") == 0);
    aoe_game_ror_stats_text_ror(&g, 1, title, sizeof title, view, sizeof view);
    assert(strncmp(title, "t30,f25,max(r(v99,", 18) == 0 && strstr(title, ",o7)") != NULL);
}
static const char *al_text(void *c, int32_t s) { (void)c; return s == 1 ? "ok" : (s == 3 ? "three" : (s == 0 ? "zero" : NULL)); }
static const char *al_shown[6];
static void al_show(void *c, const char *const l[6]) { int i; (void)c; for (i = 0; i < 6; i++) al_shown[i] = l[i]; }
static void ai_lines_tests(void) {
    char l[6][256];
    AoeGameRorAiLinesOpsRor o = {0};
    o.has_world = 1; o.player = 2; o.debug_mode = 1; o.panel_up = 1;
    o.ai_text = al_text; o.show = al_show;
    aoe_game_ror_ai_lines_ror(&o, l);
    assert(!strcmp(l[0], "Player #2: ok") && !strcmp(l[2], "three") && !l[1][0]);
    assert(al_shown[0] == l[5] && al_shown[5] == l[0]);
    o.debug_mode = 0;
    aoe_game_ror_ai_lines_ror(&o, l);
    assert(!strcmp(l[0], "zero") && !strcmp(l[1], "Player #2 is not a computer player."));
    assert(al_shown[0] == l[0] && !l[2][0]);
}

static char vp_log[512]; static int vp_n, vp_exists = 1, vp_up, vp_open_ok = 1, vp_create_ok = 1; static const char *vp_cd, *vp_mode = "playing";
static void vp_add(const char *fmt, const char *a) { vp_n += snprintf(vp_log + vp_n, sizeof vp_log - (size_t)vp_n, fmt, a); }
static void vp_wmode(void *c, void *w, char *o, int n) { (void)c; (void)w; snprintf(o, (size_t)n, "%s", vp_mode); }
static void vp_stop(void *c, void *w) { (void)c; (void)w; vp_add("stop ", NULL); }
static void vp_wclose(void *c, void *w) { (void)c; (void)w; vp_add("close ", NULL); }
static int vp_fexists(void *c, const char *p) { (void)c; vp_add("exists(%s) ", p); return vp_exists; }
static const char *vp_cdpath(void *c) { (void)c; return vp_cd; }
static int vp_open(void *c) { (void)c; vp_add("open ", NULL); return vp_open_ok; }
static void *vp_create(void *c, const char *p) { (void)c; vp_add("create(%s) ", p); return vp_create_ok ? (void *)1 : NULL; }
static void vp_pal(void *c, void *w) { (void)c; (void)w; vp_add("palette ", NULL); }
static void vp_size(void *c, void *w, int32_t *x, int32_t *y) { (void)c; (void)w; *x = 320; *y = 240; }
static void vp_move(void *c, void *w, int32_t x, int32_t y, int32_t cw, int32_t ch) { (void)c; (void)w; char b[64]; snprintf(b, 64, "move(%d,%d,%d,%d) ", (int)x, (int)y, (int)cw, (int)ch); vp_add("%s", b); }
static void vp_before(void *c) { (void)c; vp_add("before ", NULL); }
static uint32_t vp_time(void *c) { (void)c; return 777; }
static void vp_play(void *c, void *w) { (void)c; (void)w; vp_add("play ", NULL); }
static void vp_after(void *c) { (void)c; vp_add("after ", NULL); }
static void vp_closev(void *c) { (void)c; vp_add("closevideo ", NULL); }
static void play_video_tests(void) {
    AoeGameRorVideoStateRor s = {NULL, 0, 5, 0};
    AoeGameRorVideoPlayOpsRor o = {NULL, 0, "D:\\avi\\", 1, &vp_up, 0, 1, 640, 480, vp_wmode, vp_stop, vp_wclose, vp_fexists, vp_cdpath, vp_open, vp_create, vp_pal, vp_size, vp_move, vp_before, vp_time, vp_play, vp_after, vp_closev};
    assert(aoe_game_ror_play_video_ror(&s, &o, 3, "intro") == 1 && s.window == (void *)1 && s.after == 3 && s.field_bf8 == 0 && s.start_time == 777);
    assert(strcmp(vp_log, "exists(D:\\avi\\intro.avi) open create(D:\\avi\\intro.avi) palette move(0,0,640,480) before play after ") == 0);
    /* A playing window is stopped and closed; the CD path is the fallback; 16-bit skips the palette. */
    vp_n = 0; vp_exists = 0; vp_cd = "E:\\"; o.mode_16bit = 1; o.double_size = 0; vp_up = 1;
    assert(aoe_game_ror_play_video_ror(&s, &o, 4, "win") == 0 && s.window == NULL && s.after == 4);
    assert(strcmp(vp_log, "stop close exists(D:\\avi\\win.avi) exists(E:\\game\\avi\\win.avi) closevideo ") == 0);
    vp_n = 0; vp_exists = 1; vp_mode = "stopped"; s.window = (void *)2;
    assert(aoe_game_ror_play_video_ror(&s, &o, 4, "win") == 1);
    assert(strcmp(vp_log, "close exists(D:\\avi\\win.avi) create(D:\\avi\\win.avi) move(160,120,320,240) before play after ") == 0);
    vp_n = 0; vp_create_ok = 0;
    assert(aoe_game_ror_play_video_ror(&s, &o, 4, "win") == 0 && strstr(vp_log, "create(D:\\avi\\win.avi) closevideo ") != NULL);
    vp_n = 0; o.no_videos = 1; s.window = NULL;
    assert(aoe_game_ror_play_video_ror(&s, &o, 1, "x") == 0 && strcmp(vp_log, "closevideo ") == 0);
    vp_n = 0; o.no_videos = 0; vp_cd = NULL; vp_exists = 0;
    assert(aoe_game_ror_play_video_ror(&s, &o, 1, "x") == 0 && strcmp(vp_log, "exists(D:\\avi\\x.avi) closevideo ") == 0);
    vp_n = 0; vp_exists = 1; vp_up = 0; vp_open_ok = 0;
    assert(aoe_game_ror_play_video_ror(&s, &o, 1, "x") == 0 && strcmp(vp_log, "exists(D:\\avi\\x.avi) open closevideo ") == 0);
}

static char id_log[1024]; static int id_n, id_started, id_tick = 1, id_up = 1; static uint32_t id_now = 5000; static const char *id_mode = "playing";
static void id_add(const char *s) { id_n += snprintf(id_log + id_n, sizeof id_log - (size_t)id_n, "%s", s); }
static int id_base(void *c) { (void)c; id_add("tick "); return id_tick; }
static uint32_t id_nowf(void *c) { (void)c; return id_now; }
static void id_wmode(void *c, void *w, char *o, int n) { (void)c; (void)w; snprintf(o, (size_t)n, "%s", id_mode); }
static void id_end(void *c) { (void)c; id_add("end "); }
static void id_855(void *c, void *w) { (void)c; (void)w; id_add("855 "); }
static int id_comm(void *c) { (void)c; return id_started; }
static void id_str(void *c, int32_t id, char *o, int n) { (void)c; snprintf(o, (size_t)n, "S%d", (int)id); }
static int32_t id_own(void *c) { (void)c; return 2; }
static int id_playing(void *c, int32_t s) { (void)c; return s == 1 || s == 2 || s == 5; }
static const char *id_name(void *c, int32_t s) { (void)c; return s == 1 ? "Ann" : "Ed"; }
static int id_upf(void *c) { (void)c; return id_up; }
static void id_text(void *c, const char *t) { (void)c; id_add("text["); id_add(t); id_add("] "); }
static void id_start(void *c) { (void)c; id_add("start "); }
static void id_sidle(void *c) { (void)c; id_add("idle "); }
static void idle_tests(void) {
    uint32_t vtime = 0, wtime = 0; int32_t busy = 0;
    AoeGameRorIdleOpsRor o = {NULL, 3, NULL, 0, &vtime, &wtime, &busy, 1, 0, id_base, id_nowf, id_wmode, id_end, id_855, id_comm, id_str, id_own, id_playing, id_name, id_upf, id_text, id_start, id_sidle};
    assert(aoe_game_ror_idle_ror(&o) == 1 && strcmp(id_log, "tick text[S1108\n\nAnn\nEd] ") == 0 && wtime == 5000 && busy == 0);
    id_n = 0; id_now = 5400;
    assert(aoe_game_ror_idle_ror(&o) == 1 && strcmp(id_log, "tick ") == 0);
    id_n = 0; id_started = 1;
    assert(aoe_game_ror_idle_ror(&o) == 1 && strcmp(id_log, "tick start ") == 0);
    id_n = 0; o.mode = 1; o.video_window = (void *)1;
    assert(aoe_game_ror_idle_ror(&o) == 1 && strcmp(id_log, "tick ") == 0 && vtime == 5400);
    id_n = 0; id_now = 6400; id_mode = "paused";
    assert(aoe_game_ror_idle_ror(&o) == 1 && strcmp(id_log, "tick 855 ") == 0 && vtime == 6400);
    id_n = 0; id_now = 7400; id_mode = "stopped";
    assert(aoe_game_ror_idle_ror(&o) == 1 && strcmp(id_log, "tick end ") == 0);
    id_n = 0; o.mode = 5;
    assert(aoe_game_ror_idle_ror(&o) == 1 && strcmp(id_log, "tick idle ") == 0);
    id_n = 0; o.global_580e28 = 1;
    assert(aoe_game_ror_idle_ror(&o) == 1 && strcmp(id_log, "tick ") == 0);
    id_n = 0; busy = 1;
    assert(aoe_game_ror_idle_ror(&o) == 0 && busy == 1);
    id_n = 0; busy = 0; id_tick = 0;
    assert(aoe_game_ror_idle_ror(&o) == 0);
}
static char sf_log[1024]; static int sf_n, sf_world_ok = 1, sf_load_ok = 1; static const char *sf_back;
static void sf_add(const char *fmt, ...) { va_list ap; va_start(ap, fmt); sf_n += vsnprintf(sf_log + sf_n, sizeof sf_log - (size_t)sf_n, fmt, ap); va_end(ap); }
static int sf_ensure(void *c) { (void)c; sf_add("ensure "); return sf_world_ok; }
static const char *sf_return(void *c) { (void)c; return sf_back; }
static void sf_cursor(void *c, int w) { (void)c; sf_add("cur%d ", w); }
static void sf_stop(void *c) { (void)c; sf_add("stop "); }
static void sf_remove(void *c, const char *n) { (void)c; sf_add("rm(%s) ", n); }
static void sf_show(void *c, const char *n) { (void)c; sf_add("show(%s) ", n); }
static void sf_status(void *c, int32_t id) { (void)c; sf_add("st%x ", (unsigned)id); }
static void sf_sclose(void *c) { (void)c; sf_add("stclose "); }
static void sf_hold(void *c, int h) { (void)c; sf_add("hold%d ", h); }
static int sf_load(void *c, const char *n) { (void)c; sf_add("load(%s) ", n); return sf_load_ok; }
static void sf_wonder(void *c, int32_t p) { (void)c; sf_add("w%d ", (int)p); }
static void sf_bytes(void *c, uint8_t a, uint8_t b, uint8_t d, int32_t e, uint8_t f) { (void)c; sf_add("set(%d,%d,%d,%d,%d) ", a, b, d, (int)e, f); }
static void sf_splayer(void *c, int32_t s, int32_t p) { (void)c; sf_add("sp(%d,%d) ", (int)s, (int)p); }
static void sf_sname(void *c, int32_t s) { (void)c; sf_add("sn%d ", (int)s); }
static void sf_sstate(void *c, int32_t s, int32_t st) { (void)c; sf_add("ss(%d,%d) ", (int)s, (int)st); }
static void sf_names(void *c, const char *n) { (void)c; sf_add("names(%s) ", n); }
static void sf_del(void *c) { (void)c; sf_add("del "); }
static int32_t sf_start(void *c) { (void)c; sf_add("start "); return 7; }
static void start_file_tests(void) {
    int32_t mode = 5, busy = 1, g1 = 0, g2 = 0; void *gs = (void *)1;
    AoeGameRorStartFileOpsRor o = {NULL, &mode, &busy, &gs, 1, &g1, &g2, 3, 1, 1, 2, sf_ensure, sf_return, sf_cursor, sf_stop, sf_remove, sf_show, sf_status, sf_sclose, sf_hold, sf_load, sf_wonder, sf_bytes, sf_splayer, sf_sname, sf_sstate, sf_names, sf_del, sf_start};
    assert(aoe_game_ror_start_file_ror(&o, "hill.gmx") == 7 && mode == 0 && busy == 0 && gs == NULL && g1 == 1 && g2 == 7500);
    assert(strcmp(sf_log, "cur1 stop rm(Game Screen) st44d ensure st452 hold1 load(hill.gmx) w0 w1 w2 hold0 set(1,1,1,1,2) sp(0,0) sp(1,1) sn1 ss(1,2) sp(2,2) sn2 ss(2,4) sp(3,0) ss(3,0) sp(4,0) ss(4,0) sp(5,0) ss(5,0) sp(6,0) ss(6,0) sp(7,0) ss(7,0) sp(8,0) ss(8,0) names(hill) del start ") == 0);
    sf_n = 0; sf_load_ok = 0; o.path_finding = 1; o.has_sound = 0;
    assert(aoe_game_ror_start_file_ror(&o, "x.scn") == 0 && strcmp(sf_log, "cur1 st44d ensure st452 load(x.scn) w0 w1 w2 stclose cur0 ") == 0);
    sf_n = 0; sf_world_ok = 0; sf_back = "Single Player Menu";
    assert(aoe_game_ror_start_file_ror(&o, "x.scn") == 0 && strcmp(sf_log, "cur1 st44d ensure show(Single Player Menu) rm(Status Screen) cur0 ") == 0);
}

static char sg_log[2048]; static int sg_n;
static void sg_add(const char *fmt, ...) { va_list ap; va_start(ap, fmt); sg_n += vsnprintf(sg_log + sg_n, sizeof sg_log - (size_t)sg_n, fmt, ap); va_end(ap); }
static uint8_t sg_ac4(void *c, int i) { (void)c; return (uint8_t)(10 + i); }
static uint8_t sg_994(void *c, int i) { (void)c; return (uint8_t)(2 + i); }
static uint8_t sg_a94(void *c, int i) { (void)c; return (uint8_t)(5 + i); }
static void sg_show(void *c, const char *n) { (void)c; sg_add("show(%s) ", n); }
static void sg_remove(void *c, const char *n) { (void)c; sg_add("rm(%s) ", n); }
static void sg_logf(void *c, const char *t, int s) { (void)c; sg_add("log%d[%s] ", s, t); }
static void sg_string(void *c, int32_t id, char *o, int n) { (void)c; snprintf(o, (size_t)n, "S%x", (unsigned)id); }
static void sg_tracks(void *c, int32_t a, int32_t b) { (void)c; sg_add("music(%d,%d) ", (int)a, (int)b); }
static void sg_stop(void *c) { (void)c; sg_add("stop "); }
static void sg_timing(void *c) { (void)c; sg_add("timing "); }
static void sg_state(void *c, int32_t s) { (void)c; sg_add("state%d ", (int)s); }
static void sg_resume(void *c) { (void)c; sg_add("resume "); }
static void sg_drop(void *c) { (void)c; sg_add("drop "); }
static void sg_recip(void *c, int32_t p) { (void)c; sg_add("recip%d ", (int)p); }
static void sg_centre(void *c) { (void)c; sg_add("centre "); }
static void sg_release(void *c) { (void)c; sg_add("release "); }
static void start_game_tests(void) {
    int32_t held = 1, f9fc = 3;
    AoeGameRorStartOpsRor o = {NULL, 1, 0, 2, 4, 7, 1500, 0, 2, 3, 1, 1, 0, 3, 2, sg_ac4, sg_994, sg_a94, 2, 9, &held, &f9fc, 1, 1,
                               sg_show, sg_remove, sg_logf, sg_string, sg_tracks, sg_stop, sg_timing, sg_state, sg_resume, sg_drop, sg_recip, sg_centre, sg_release};
    aoe_game_ror_start_game_ror(&o);
    assert(strcmp(sg_log, "show(Game Screen) rm(Status Screen) rm(Multiplayer) log1[game started] "
                          "log0[    randomgame=1, randommap=0, mapsize=S2975, maptype=S296e] "
                          "log0[    resources=Medium, age=Tool, victory=Time(1500), difficulty=S2bd1] "
                          "log0[    fulltech=1, revealmap=0] "
                          "log0[    numplayers=2, players=1011, teams=12, civs=5 6 ] "
                          "music(1,9) timing state4 resume drop recip2 centre release ") == 0);
    assert(held == 0 && f9fc == 0);
    sg_n = 0; held = 1; o.map_size = 9; o.map_type = -1; o.victory = 8; o.death_match = 1; o.starting_age = 7; o.music_kind = 3; o.settings_paused = 0; o.can_centre_cursor = 0; o.player_count = 1;
    aoe_game_ror_start_game_ror(&o);
    assert(strstr(sg_log, "mapsize=S2804, maptype=S2804] ") && strstr(sg_log, "resources=DeathMatch, age=Unknown, victory=Score(1500),") &&
           strstr(sg_log, "numplayers=0, players=, teams=, civs=] stop timing state4 drop recip2 release ") && held == 0);
    sg_n = 0; held = 1; o.music_kind = 0; o.victory = 9; o.death_match = 0; o.resource_level = 7;
    aoe_game_ror_start_game_ror(&o);
    assert(strstr(sg_log, "resources=Unknown, age=Unknown, victory=Standard,") && strstr(sg_log, "civs=] timing ") && held == 1);
}

static int snd_pre; static char snd_log[512]; static int snd_n;
static void *snd_alloc(void *c, uint32_t n, uint32_t s) { (void)c; return calloc(n, s); }
static void *snd_load(void *c, const char *name, int32_t id) { (void)c; snd_n += snprintf(snd_log + snd_n, sizeof snd_log - (size_t)snd_n, "%s=%x ", name, (unsigned)id); return id == 0xc47f ? NULL : (void *)(intptr_t)id; }
static void snd_preload(void *c, void *s) { (void)c; (void)s; ++snd_pre; }
static void sounds_tests(void) {
    AoeGameRorSoundOpsRor o = {NULL, snd_alloc, snd_load, snd_preload}; void **s = NULL; int16_t n = 0;
    assert(aoe_game_ror_load_sounds_ror(&o, &s, &n) == 1 && n == 17 && s && s[0] == (void *)(intptr_t)0xc47c && s[3] == NULL && s[16] == (void *)(intptr_t)0xc48c && snd_pre == 16);
    assert(strncmp(snd_log, "button1.wav=c47c button2.wav=c47d chatrcvd.wav=c47e cantdo.wav=c47f ", 66) == 0 && strstr(snd_log, "farmdie.wav=c48c "));
    free(s);
}

static int32_t ch_code = -2, ch_gaia = -1, ch_p54 = -1;
static void ch_cheat(void *c, int32_t code) { (void)c; ch_code = code; }
static void ch_gaiaf(void *c, int g) { (void)c; ch_gaia = g; }
static void ch_p54f(void *c, int32_t p) { (void)c; ch_p54 = p; }
static void cheat_tests(void) {
    AoeGameRorCheatOpsRor o = {NULL, 4, 1, 0, 6, 1, ch_cheat, ch_gaiaf, ch_p54f};
    assert(aoe_game_ror_cheat_ror(&o, 1, "photon man") == 1 && ch_code == 0);
    assert(aoe_game_ror_cheat_ror(&o, 1, "Photon MQN") == 1 && ch_code == 0);  /* A and Q share a code */
    assert(aoe_game_ror_cheat_ror(&o, 1, "I say big momma now") == 1 && ch_code == 0x10);
    assert(aoe_game_ror_cheat_ror(&o, 1, "HOME RUN") == 0 && ch_code == 0x10);
    assert(aoe_game_ror_cheat_ror(&o, 1, "KILL3") == 0);
    assert(aoe_game_ror_cheat_ror(&o, 1, "gaia") == 1 && ch_code == -1 && ch_gaia == -1);
    o.flag_41b750 = 1;
    assert(aoe_game_ror_cheat_ror(&o, 1, "HOME RUN") == 1 && ch_code == 0x66);
    assert(aoe_game_ror_cheat_ror(&o, 1, "KILL3") == 1 && ch_code == 0xcb);
    assert(aoe_game_ror_cheat_ror(&o, 1, "kill9") == 1 && ch_code == 0xd0 && aoe_game_ror_cheat_ror(&o, 1, "kill8") == 0);
    assert(aoe_game_ror_cheat_ror(&o, 1, "gaia") == 1 && ch_gaia == 1);
    assert(aoe_game_ror_cheat_ror(&o, 1, "no fog") == 1 && ch_code == 0xe7);
    assert(aoe_game_ror_cheat_ror(&o, 1, "e=mc2 trooper") == 1 && ch_code == 1);
    assert(aoe_game_ror_cheat_ror(&o, 2, "zeus") == 1 && ch_p54 == 2);
    assert(aoe_game_ror_cheat_ror(&o, 2, "zeus please") == 0);
    assert(aoe_game_ror_cheat_ror(&o, 4, "zeus") == 0 && aoe_game_ror_cheat_ror(&o, 1, NULL) == 0);
    o.cheats_on = 0;
    assert(aoe_game_ror_cheat_ror(&o, 1, "zeus") == 0);
}
int main(void) {
    char coded[64];
    AoeGameRorCommandOpsRor cops = {NULL, ns, sp, bc};
    AoeViewHistoryRor h;
    AoeComputerNamesRor names;
    AoeTrigTablesRor *t;
    AoeTribeSettingsRor a, b;
    uint8_t packed[0x6c];
    AoeGameSettingsRor *g = calloc(1, sizeof *g), *g2 = calloc(1, sizeof *g2);
    AoeGameRorCommOpsRor comm = {NULL, 1, send, recv};
    AoeGameRorInputOpsRor in;
    char name[0x104];
    int i;

    /* "LIMIT=" is +!#L%c#L ^= in the code; digits pass through. */
    aoe_game_ror_encode_ror("LIMIT=", coded, 60);
    assert(strcmp(coded, "+!#L%c#L ^=") == 0);
    aoe_game_ror_command_line_ror("game nosound limit=150 x", &cops);
    assert(pop == 150 && !no_sound && base_called == 1);
    aoe_game_ror_command_line_ror("NoTerrainSound LIMIT=5", &cops);
    assert(pop == 25 && no_sound);
    aoe_game_ror_command_line_ror("LIMIT=999", &cops);
    assert(pop == 200);
    pop = -1;
    aoe_game_ror_command_line_ror("LIMITS", &cops);
    assert(pop == -1);

    /* View history. */
    memset(&h, 0, sizeof h);
    for (i = 0; i < 5; ++i) h.x[i] = h.y[i] = -1;
    h.head = h.cursor = -1;
    aoe_game_ror_view_push_ror(&h, 10, 20);
    aoe_game_ror_view_push_ror(&h, 10, 20);
    assert(h.head == 0);
    aoe_game_ror_view_push_ror(&h, 30, 40);
    aoe_game_ror_view_back_ror(&h, move, NULL);
    assert(mx == 30.0f && my == 40.0f && h.cursor == 0);
    aoe_game_ror_view_back_ror(&h, move, NULL);
    /* slot 4 is empty: back to the head */
    assert(mx == 10.0f && h.cursor == 1);
    for (i = 0; i < 6; ++i) aoe_game_ror_view_push_ror(&h, i, i);
    assert(h.head == 2 && h.x[2] == 5);

    /* Names: random picks retry while used. */
    aoe_game_ror_clear_names_ror(&names);
    seq[0] = 3; seq[1] = 3; seq[2] = 13; seq[3] = 4;
    assert(aoe_game_ror_pick_name_ror(&names, 2, 10, rnd, NULL) == 20 + 3 - 9);
    assert(aoe_game_ror_pick_name_ror(&names, 2, 10, rnd, NULL) == 20 + 4 - 9 && names.used[2][3]);
    for (i = 0; i < 10; ++i) names.used[1][i] = (uint8_t)(i != 6);
    for (i = 0; i < 8; ++i) seq[i] = 0;
    assert(aoe_game_ror_pick_name_ror(&names, 1, 10, rnd, NULL) == 10 + 6 - 9);

    /* Trig tables. */
    t = malloc(sizeof *t);
    aoe_game_ror_trig_ror(t);
    assert(t->sin4096[1024] == 1.0f && t->tan4096[1024] == 0.0f && t->atan4096[2048] == 0.0f);
    assert(fabsf(t->cos360[180] + 1.0f) < 1e-6f && t->tan360[90] == 0.0f);
    assert(fabsf(t->atan360[45] - (float)atan(45 * 0.01745329252)) < 1e-6f);
    free(t);

    /* Settings snapshot round trip and exchange. */
    aoe_settings_set_map_size_ror(g, 2);
    aoe_settings_set_byte_a94_ror(g, 3, 7);
    aoe_settings_set_int_aa0_ror(g, 8, -5);
    aoe_settings_set_starting_age_ror(g, 2);
    aoe_settings_set_option_ae5_ror(g, 1);
    aoe_game_ror_settings_get_ror(g, &a);
    aoe_game_ror_settings_pack_ror(&a, packed);
    aoe_game_ror_settings_unpack_ror(packed, &b);
    assert(memcmp(&a, &b, sizeof a) == 0 && packed[0x18 + 3] == 7 && packed[0x69] == 1);
    aoe_game_ror_send_settings_ror(g, &comm);
    assert(sent_n == 0x114);
    aoe_game_ror_receive_settings_ror(g2, &comm);
    aoe_game_ror_settings_get_ror(g2, &b);
    assert(b.map_size == 2 && b.byte_a94[3] == 7 && b.int_aa0[8] == -5 && b.starting_age == 2);
    sent_n = 3;
    aoe_settings_set_starting_age_ror(g2, 0);
    aoe_game_ror_receive_settings_ror(g2, &comm);
    assert(aoe_settings_starting_age_ror(g2) == 0);
    /* The base record alone. */
    aoe_settings_set_speed_ror(g, 2.5f);
    aoe_game_send_settings_ror(g, &comm);
    assert(sent_n == 0xa8);
    aoe_settings_set_speed_ror(g2, 1.0f);
    aoe_game_receive_settings_ror(g2, &comm);
    assert(aoe_settings_speed_ror(g2) == 2.5f);
    sent_n = 0x114;
    aoe_settings_set_speed_ror(g2, 1.0f);
    aoe_game_receive_settings_ror(g2, &comm);
    assert(aoe_settings_speed_ror(g2) == 1.0f);

    /* Input. */
    memset(&in, 0, sizeof in);
    in.cursor = cursor; in.hide_cursor = hide; in.end_video = endv; in.send = sendm;
    in.base_proc = bproc; in.base_palette = bpal; in.base_activate = bact; in.help = help;
    in.mode = 1;
    in.video = 5;
    assert(aoe_game_ror_window_proc_ror(&in, 0x201) == 0 && ended == 1);
    assert(aoe_game_ror_window_proc_ror(&in, 0x200) == 1 && hidden == 1);
    assert(aoe_game_ror_key_ror(&in, 0x1b) == 0 && ended == 0);
    assert(aoe_game_ror_key_ror(&in, 'a') == 1);
    aoe_game_ror_activate_ror(&in, 0);
    aoe_game_ror_activate_ror(&in, 0);
    aoe_game_ror_activate_ror(&in, 1);
    assert(nmsg == 2 && msgs[0] == 0x809 && msgs[1] == 0x855 && in.paused == 0);
    assert(aoe_game_ror_palette_ror(&in) == 1 && msgs[2] == 0x476);
    in.mode = 4;
    in.session = 1;
    assert(aoe_game_ror_palette_ror(&in) == 7);
    assert(aoe_game_ror_key_ror(&in, 0x70) == 0 && helped == 0);
    in.mode = 2;
    assert(aoe_game_ror_key_ror(&in, 0x70) == 0 && helped == 1);

    aoe_game_ror_set_name_ror(name, "save1");
    assert(strcmp(name, "save1") == 0);
    aoe_game_ror_set_name_ror(name, NULL);
    assert(name[0] == 0);
    {
        void *gs = (void *)1;
        aoe_game_ror_delete_screens_ror(rm, NULL, 1, &gs);
        assert(removed == 22 && gs == NULL);
    }
    assert(aoe_game_ror_end_text_id_ror(1) == 0x47e && aoe_game_ror_end_text_id_ror(2) == 0x47f);
    free(g);
    free(g2);
    puts("game_ror_test: ok");
    {
        /* 0x00500c60 / 0x00500b40 to a menu screen. */
        AoeGameRorMenuOpsRor m = {0, 1, menu_end, menu_make_fail, menu_show, menu_set_mode,
                                  2, 0, menu_play_track, menu_play_file};
        assert(!aoe_game_ror_to_menu_ror("Main Menu", &m) && menu_ends == 1 && !menu_mode);
        m.make_screen = menu_make_ok;
        m.video_playing = 0;
        assert(aoe_game_ror_to_menu_ror("Main Menu", &m));
        assert(menu_mode == 2 && !strcmp(menu_shown, "Main Menu") && !strcmp(menu_file, "xopen.mid"));
        m.music_source = 1;
        assert(aoe_game_ror_to_menu_ror("Campaign Selection Screen", &m) && menu_track == 2);
        m.music_source = 3;
        m.music_held = 1;
        menu_file[0] = 0;
        assert(aoe_game_ror_to_menu_ror("Main Menu", &m) && !menu_file[0]);
    }
    {
        /* 0x005063e0 / 0x00505f10 / 0x00503060 leaving a session. */
        AoeGameRorEndOpsRor e;
        memset(&e, 0, sizeof e);
        e.leave.reset_comm = lv_reset;
        e.leave.poll = lv_poll;
        e.leave.unacknowledged = lv_unacked;
        e.leave.status = lv_stat;
        e.leave.now_ms = lv_now;
        e.leave.unlink = lv_unl;
        e.set_state = lv_set_state;
        e.play_video = lv_play;
        aoe_game_ror_leave_ror(&e.leave);
        assert(lv_resets == 1 && !lv_polls);
        e.leave.networked = 1;
        e.leave.state = 4;
        aoe_game_ror_leave_ror(&e.leave);
        assert(lv_status == 0x4c0 && lv_pending == 0 && lv_unlink == 5 && lv_resets == 2);
        /* Commands that are never acknowledged stop after 15 s. */
        lv_pending = 1000;
        lv_polls = 0;
        lv_clock = 0;
        aoe_game_ror_leave_ror(&e.leave);
        assert(lv_clock >= 15000 && lv_clock < 16000);
        e.has_comm = 0;
        assert(aoe_game_ror_quit_session_ror(&e) && lv_state == -1);
        e.has_comm = 1;
        lv_pending = 0;
        assert(aoe_game_ror_quit_session_ror(&e) && lv_state == 0);
        lv_state = -1;
        e.leave.state = 5;
        e.has_world = 1;
        e.won = 1;
        aoe_game_ror_end_session_ror(&e);
        assert(lv_state == 0 && lv_video == 1);
    }
    {
        AoeGameRorWorldOpsRor w = {NULL, NULL, w_create, w_load, w_load, w_delete, w_cur,
                                   w_stat, w_close};
        w_ok = 0;
        assert(!aoe_game_ror_ensure_world_ror(&w) && w.world == NULL && w_deleted == 1);
        w_ok = 1;
        assert(aoe_game_ror_ensure_world_ror(&w) && w.world == &w_obj && w_made == 2);
        assert(aoe_game_ror_ensure_world_ror(&w) && w_made == 2);
        assert(aoe_game_ror_check_world_ror(&w) && w.world == NULL && w_deleted == 3);
        assert(w_made == 3 && w_cursor == 0 && w_status == 0);
        w_ok = 0;
        assert(!aoe_game_ror_check_world_ror(&w) && w_deleted == 4);
        w_made = 100;
        assert(!aoe_game_ror_check_world_ror(&w) && w_deleted == 4);
    }
    test_run_log();
    test_destroy_editor_video();
    test_open_video();
    stats_text_tests();
    ai_lines_tests();
    play_video_tests();
    idle_tests();
    start_file_tests();
    start_game_tests();
    sounds_tests();
    cheat_tests();
    return 0;
}
