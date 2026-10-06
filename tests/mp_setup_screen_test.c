#include "mp_setup_screen.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int host, slot48, seeded, rnd, refresh_arg = -1, ok_id, yes_w, yes_h;
static char yes_text[64], yes_name[32];
static int16_t chat_n, chat_bot;
static int removed, added, scrolled, minimised, released = 1, release_calls, shares, idles;
static uint32_t clock_ms;
static int32_t appended[16], n_appended, selected_index = -1;
static int color_ids[8][8], color_n[8];
static uint32_t color_rgb[8][8];
static int used_mask = 0xff, kind1_mask = 0xff;

static int f_host(void *c) { (void)c; return host; }
static void f_48(void *c) { (void)c; ++slot48; }
static void f_seed(void *c) { (void)c; ++seeded; }
static int32_t f_rand(void *c) { (void)c; return rnd++; }
static const char *f_name(void *c, int32_t p) { (void)c; return p == 3 ? "Bob" : "?"; }
static const char *f_string(void *c, int32_t id) { (void)c; return id == 0x25c8 ? "Kick %s?" : id == 0x238c ? "Host %s" : ""; }
static void f_yes(void *c, const char *t, const char *n, int32_t w, int32_t h) {
    (void)c; snprintf(yes_text, sizeof yes_text, "%s", t); snprintf(yes_name, sizeof yes_name, "%s", n);
    yes_w = w; yes_h = h;
}
static void f_ok(void *c, int32_t id, int32_t w, int32_t h) { (void)c; (void)w; (void)h; ok_id = id; }
static void f_refresh(void *c, int32_t a) { (void)c; refresh_arg = a; }
static int16_t f_ccount(void *c) { (void)c; return chat_n; }
static int16_t f_cbottom(void *c) { (void)c; return chat_bot; }
static void f_cremove(void *c) { (void)c; ++removed; --chat_n; }
static void f_cadd(void *c, const char *t) { (void)c; (void)t; ++added; ++chat_n; }
static void f_cscroll(void *c) { (void)c; ++scrolled; chat_bot = (int16_t)(chat_n - 1); }
static const char *f_cline(void *c, int32_t i) { (void)c; (void)i; return "line"; }
static int f_min(void *c) { (void)c; return minimised; }
static int f_released(void *c) { (void)c; return released; }
static void f_release(void *c) { (void)c; ++release_calls; }
static void f_share(void *c) { (void)c; ++shares; }
static uint32_t f_now(void *c) { (void)c; return clock_ms; }
static void f_idle(void *c) { (void)c; ++idles; }
static void f_kclear(void *c) { (void)c; n_appended = 0; }
static void f_kappend(void *c, const char *t, int32_t id) {
    (void)c; assert(id == 2 + n_appended); { char b[16]; snprintf(b, sizeof b, "%d", (int)id); assert(strcmp(b, t) == 0); } appended[n_appended++] = id;
}
static int32_t f_kindex(void *c, int32_t id) { (void)c; return id - 2; }
static void f_kselect(void *c, int32_t i) { (void)c; selected_index = i; }
static void f_ctext(void *c, int32_t b, int32_t s, const char *t) { (void)c; (void)b; (void)s; (void)t; }
static void f_cstate(void *c, int32_t b, int32_t s, int32_t id) { (void)c; color_ids[b][s] = id; }
static void f_crgb(void *c, int32_t b, int32_t s, uint32_t rgb) { (void)c; color_rgb[b][s] = rgb; }
static void f_ccnt(void *c, int32_t b, int32_t n) { (void)c; color_n[b] = n; }
static int f_used(void *c, int32_t s) { (void)c; return (used_mask >> s) & 1; }
static int32_t f_kind(void *c, int32_t s) { (void)c; return (kind1_mask >> s) & 1 ? 1 : 0; }

static int comm_states[10];
static int32_t f_civ(void *c, int32_t s) { (void)c; return 10 + s; }
static void f_comm(void *c, int32_t s, int32_t v) { (void)c; comm_states[s] = v; }

static int freed, released_panels;
static char closed[160];
static void f_free(void *c) { (void)c; ++freed; }
static void f_close(void *c, const char *n) { (void)c; strcat(closed, n); strcat(closed, ","); }
static void f_rel(void *c) { (void)c; ++released_panels; }

static int32_t kinds[10];
static int present_mask = 0xff;
static void *tabs[4]; static int32_t ntabs; static void *focused;
static int lobby, wait_n, drop_reason = -1, make_ok = 1, quits;
static char made[64], shown[64];
static int32_t made_arg;
static int32_t f_skind(void *c, int32_t s) { (void)c; return kinds[s]; }
static int32_t f_pidx(void *c, int32_t id) { (void)c; return (present_mask >> id) & 1 ? id : -1; }
static void f_tab(void *c, void *const *p, int32_t n) { int i; (void)c; ntabs = n; for (i = 0; i < n; ++i) tabs[i] = p[i]; }
static void f_focus(void *c, void *p) { (void)c; focused = p; }
static int f_lobby(void *c) { (void)c; return lobby; }
static void f_wait(void *c) { (void)c; ++wait_n; }
static void f_drop(void *c, int32_t r) { (void)c; drop_reason = r; }
static int f_make(void *c, const char *n, int32_t a) { (void)c; snprintf(made, sizeof made, "%s", n); made_arg = a; return make_ok; }
static void f_show(void *c, const char *n) { (void)c; snprintf(shown, sizeof shown, "%s", n); }
static void f_quit(void *c) { (void)c; ++quits; }

static int host_ok = 1, host_closed;
static uint32_t host_n;
static char ok_txt[256], ok_name[32];
static int32_t ok_h;
static void *f_hopen(void *c) { (void)c; return host_ok ? (void *)5 : NULL; }
static void f_hclose(void *c, void *h) { (void)c; (void)h; ++host_closed; }
static uint32_t f_hcount(void *c, void *h) { (void)c; (void)h; return host_n; }
static const char *f_hname(void *c, void *h) { (void)c; (void)h; return "box"; }
static const char *f_haddr(void *c, void *h, uint32_t i) { (void)c; (void)h; return i == 0 ? "1.2.3.4" : NULL; }
static const char *f_halias(void *c, void *h, uint32_t i) { (void)c; (void)h; return i == 1 ? "al" : NULL; }
static void f_oktext(void *c, const char *t, const char *n, int32_t w, int32_t h) {
    (void)c; (void)w; snprintf(ok_txt, sizeof ok_txt, "%s", t); snprintf(ok_name, sizeof ok_name, "%s", n); ok_h = h;
}

static const AoeMpSetupScreenOpsRor ops = {
    NULL, f_host, f_48, f_seed, f_rand, f_name, f_string, f_yes, f_ok, f_refresh,
    f_ccount, f_cbottom, f_cremove, f_cadd, f_cscroll, f_cline,
    f_min, f_released, f_release, f_share, f_now, f_idle,
    f_kclear, f_kappend, f_kindex, f_kselect,
    f_ctext, f_cstate, f_crgb, f_ccnt, f_used, f_kind, f_civ, f_comm, f_free, f_close, f_rel,
    f_skind, f_pidx, f_tab, f_focus, f_lobby, f_wait, f_drop, f_make, f_show, f_quit,
    f_hopen, f_hclose, f_hcount, f_hname, f_haddr, f_halias, f_oktext};

static char chat_sent[32], input_now[32] = "hi";
static int base_calls, civ_set = -1;
static const char *a_text(void *c, void *p) { (void)c; (void)p; return input_now; }
static void a_set(void *c, void *p, const char *t) { (void)c; (void)p; snprintf(input_now, sizeof input_now, "%s", t); }
static void a_chat(void *c, const char *t) { (void)c; snprintf(chat_sent, sizeof chat_sent, "%s", t); }
static int32_t a_id(void *c, void *p) { (void)c; (void)p; return 7; }
static void a_civ(void *c, int32_t pl, int32_t v) { (void)c; civ_set = pl * 100 + v; }
static int32_t a_count(void *c) { (void)c; return 4; }
static int a_host(void *c) { (void)c; return 0; }
static int a_base(void *c, void *p, int32_t ac, int32_t x, int32_t y) { (void)c; (void)p; (void)ac; (void)x; (void)y; ++base_calls; return 9; }
static const char *a_name(void *c, void *p) { (void)c; (void)p; return "Other"; }


/* ---- 0x0048c010 / 0x0048b640 ---- */
static char in_log[4096]; static int in_n, in_exists = 1, in_load_ok = 1, in_slots[9] = {1, 1, 0, 1, 1, 0, 0, 0, 0}, in_kinds[9] = {1, 2, 0, 1, 1}; static char in_name[64] = "";
static void in_add(const char *fmt, ...) { va_list ap; va_start(ap, fmt); in_n += vsnprintf(in_log + in_n, sizeof in_log - (size_t)in_n, fmt, ap); va_end(ap); }
static void in_store(void *c, const char *n) { (void)c; snprintf(in_name, sizeof in_name, "%s", n); }
static int in_fexists(void *c, const char *p) { (void)c; in_add("exists(%s) ", p); return in_exists; }
static void in_sdel(void *c) { (void)c; in_add("del "); }
static int in_sload(void *c, const char *p) { (void)c; in_add("load(%s) ", p); return in_load_ok; }
static int in_sactive(void *c, int32_t s) { (void)c; return in_slots[s]; }
static int32_t in_skind(void *c, int32_t s) { (void)c; return in_kinds[s]; }
static int32_t in_fixed(void *c) { (void)c; return 0; }
static int32_t in_sinfo(void *c, const char *p) { (void)c; (void)p; return 77; }
static void in_pclear(void *c, int32_t j) { (void)c; if (j == 0) in_add("pclear "); }
static void in_pappend(void *c, int32_t j, const char *t, int32_t id) { (void)c; if (j == 0) in_add("p%s/%d ", t, (int)id); }
static void in_string(void *c, int32_t id, char *o, int n) { (void)c; snprintf(o, (size_t)n, id == 0x25ed || id == 0x25da || id == 0x25b6 || id == 0x25bc || id == 0x25e4 || id == 0x25e5 || id == 0x25e0 || id == 0x25e9 || id == 0x25b8 || id == 0x25ec || id == 0x25bb || id == 0x25f1 ? "F%x:%%s" : id == 0x25f0 ? "Pop:%%d" : id == 0x25be ? "V%%s=%%d" : "S%x", (unsigned)id); }
static void in_header(void *c, const char *t) { (void)c; in_add("hdr[%s] ", t); }
static void in_line(void *c, int32_t i, const char *t) { (void)c; in_add("L%d[%s] ", (int)i, t); }
static void in_hide(void *c) { (void)c; in_add("hide "); }
static void in_count(void *c, int s) { (void)c; in_add("count%d ", s); }
static void in_rows(void *c) { (void)c; in_add("rows "); }
static void info_tests(void) {
    static AoeGameSettingsRor g; static AoeMpSetupScreenRor m;
    AoeMpSetupInfoOpsRor o = {NULL, "D:/scn/", in_name, in_store, in_fexists, in_sdel, in_sload, in_sactive, in_skind, in_fixed, in_sinfo, in_pclear, in_pappend, in_string, in_header, in_line, in_hide, in_count, in_rows};
    memset(&g, 0, sizeof g); memset(&m, 0, sizeof m); m.settings = &g; m.ops = &ops;
    /* Random map (no scenario): the lists hold 1..8, players 1..8; the header with 0x25b5; map lines. */
    aoe_settings_set_map_size_ror(&g, 2); aoe_settings_set_int_a80_ror(&g, 4); aoe_settings_set_a8c_a90_ror(&g, 7, 5000); aoe_settings_set_byte_993_ror(&g, 1);
    aoe_mp_setup_screen_info_ror(&m, &o);
    assert(strstr(in_log, "pclear p1/0 p2/1 p3/2 p4/3 p5/4 p6/5 p7/6 p8/7 hide hdr[F25ed:S25b5] L1[F25da:S2975] L2[F25b6:S296e] L3[S25b7 2 - 8] L4[VS10e9=5000] L5[F25e4:S10e7] L6[F25e5:S10e7] L7[F25e0:S2bd1] L8[F25e9:Sfa3] L9[F25b8:Sfa4] L10[F25ec:Sfa4] L11[F25f1:S260e] L12[Pop:50] rows ") == in_log);
    assert(m.max_list == 8 && m.max_players == 8 && m.colors[7] == 8 && aoe_settings_byte_ac4_ror(&g, 3) == 4 && m.settings_flag == 1 && in_name[0] == '\0' && m.field_73c == 0);
    /* A scenario "hill" with 97d: slots 0, 1, 3, 4 active; kind 1 slots 0, 3, 4 become positions; the name stored. */
    in_n = 0; aoe_settings_set_flag_8fc_ror(&g, 1); aoe_settings_set_scenario_name_ror(&g, "hill"); g.flag_97d = 1; m.settings_flag = 1;
    aoe_mp_setup_screen_info_ror(&m, &o);
    assert(strstr(in_log, "exists(D:/scn/hill.scn) load(hill.scn) pclear p1/0 p4/3 p5/4 exists(D:/scn/hill.scn) hide hdr[F25ed:hill] L3[S25b7 2 - 4] L4[VS10e9=5000] ") == in_log);
    assert(m.max_list == 4 && m.max_players == 3 && m.colors[0] == 1 && m.colors[1] == 2 && m.colors[2] == 4 && m.colors[3] == 5 && strcmp(in_name, "hill") == 0 && m.field_73c == 77 && m.settings_flag == 0);
    assert(strstr(in_log, "L12[Pop:50] count1 rows "));
    /* The same name again: no reload; a missing .scn falls back to .scx and a failed load clears the scenario flag. */
    in_n = 0; aoe_mp_setup_screen_info_ror(&m, &o);
    assert(strstr(in_log, "hide hdr[F25ed:hill] ") == in_log);
    in_n = 0; aoe_settings_set_scenario_name_ror(&g, "lake"); in_exists = 0; in_load_ok = 0;
    aoe_mp_setup_screen_info_ror(&m, &o);
    assert(strstr(in_log, "del exists(D:/scn/lake.scn) load(lake.scx) ") == in_log && aoe_settings_flag_8fc_ror(&g) == 0 && m.settings_flag == 1);
}
static char rw_log[4096]; static int rw_n, rw_host = 1, rw_types[9] = {0, 2, 4, 2, 0}, rw_stats[9] = {0, 1, 1, 0, 0}, rw_unknown[8]; static int32_t rw_sel[AOE_MPS_CONTROLS][8], rw_first_id = 0;
static void rw_add(const char *fmt, ...) { va_list ap; va_start(ap, fmt); rw_n += vsnprintf(rw_log + rw_n, sizeof rw_log - (size_t)rw_n, fmt, ap); va_end(ap); }
static int rw_statsf(void *c, int32_t s) { (void)c; return rw_stats[s]; }
static int32_t rw_typef(void *c, int32_t s) { (void)c; return rw_types[s]; }
static const char *rw_cname(void *c, int32_t s) { (void)c; return s == 1 ? "Ann" : s == 3 ? "Cy" : "X"; }
static int rw_unknownf(void *c, int32_t i) { (void)c; return rw_unknown[i]; }
static int32_t rw_selid(void *c, int ctl, int32_t i) { (void)c; return rw_sel[ctl][i]; }
static int rw_open(void *c, int ctl, int32_t i) { (void)c; (void)ctl; (void)i; return 0; }
static void rw_selval(void *c, int ctl, int32_t i, int32_t v) { (void)c; rw_sel[ctl][i] = v; rw_add("sel%d.%d=%d ", ctl, (int)i, (int)v); }
static const char *rw_seltext(void *c, int ctl, int32_t i) { (void)c; static char b[32]; snprintf(b, sizeof b, "T%d.%d:%d", ctl, (int)i, (int)rw_sel[ctl][i]); return b; }
static int32_t rw_firstid(void *c, int32_t i) { (void)c; (void)i; return rw_first_id; }
static void rw_rmfirst(void *c, int32_t i) { (void)c; rw_add("rm%d ", (int)i); }
static void rw_insfirst(void *c, int32_t i, const char *t, int32_t id) { (void)c; rw_add("ins%d(%s,%d) ", (int)i, t, (int)id); }
static void rw_cstate(void *c, int32_t i, int32_t color, const char **t, uint32_t *c1, uint32_t *c2) { (void)c; (void)i; static char b[16]; snprintf(b, sizeof b, "C%d", (int)color); *t = b; *c1 = 10u + (uint32_t)color; *c2 = 20u; }
static void rw_tsel(void *c, int32_t i, int32_t idx) { (void)c; rw_add("team%d=%d ", (int)i, (int)idx); }
static const char *rw_ttext(void *c, int32_t i) { (void)c; (void)i; return "tt"; }
static void rw_active(void *c, int ctl, int32_t i, int32_t v) { (void)c; rw_add("a%d.%d=%d ", ctl, (int)i, (int)v); }
static void rw_text(void *c, int ctl, int32_t i, const char *t) { (void)c; rw_add("t%d.%d[%s] ", ctl, (int)i, t); }
static void rw_textid(void *c, int ctl, int32_t i, int32_t id) { (void)c; rw_add("id%d.%d=%x ", ctl, (int)i, (unsigned)id); }
static void rw_colors(void *c, int ctl, int32_t i, uint32_t a, uint32_t b) { (void)c; rw_add("col%d.%d=%u,%u ", ctl, (int)i, (unsigned)a, (unsigned)b); }
static void rw_ncolor(void *c, int32_t i, uint32_t rgb) { (void)c; rw_add("nc%d=%x ", (int)i, (unsigned)rgb); }
static void rw_enable(void *c, int ctl, int32_t i) { (void)c; rw_add("en%d.%d ", ctl, (int)i); }
static void rw_logf(void *c, const char *t) { (void)c; rw_add("log[%s] ", t); }
static void rw_share(void *c) { (void)c; rw_add("share "); }
static void rows_tests(void) {
    static AoeGameSettingsRor g; static AoeMpSetupScreenRor m;
    AoeMpSetupRowsOpsRor o = {NULL, 1, 1, rw_statsf, rw_typef, rw_cname, rw_unknownf, rw_selid, rw_open, rw_selval, rw_seltext, rw_firstid, rw_rmfirst, rw_insfirst, rw_cstate, rw_tsel, rw_ttext, rw_active, rw_text, rw_textid, rw_colors, rw_ncolor, rw_enable, rw_logf, rw_share};
    memset(&g, 0, sizeof g); memset(&m, 0, sizeof m); m.settings = &g; m.ops = &ops; m.field_730 = -1;
    g.flag_97e = 1; aoe_settings_set_byte_988_ror(&g, 3);
    for (int i = 0; i < 8; ++i) { aoe_settings_set_byte_ac4_ror(&g, i, (uint8_t)(i + 1)); aoe_settings_set_byte_a94_ror(&g, i, (uint8_t)(5 + i)); aoe_settings_set_player_994_ror(&g, i, (uint8_t)(i + 1)); aoe_settings_set_int_aa0_ror(&g, i, i); }
    /* Networked host, our slot 1: slot 2 is a computer, slot 3 a human without stats. */
    aoe_mp_setup_screen_rows_ror(&m, &o);
    assert(strstr(rw_log, "a3.0=0 t4.0[] a4.0=1 t5.0[C1] col5.0=11,20 a7.0=0 a5.0=0 a7.0=1 en7.0 team0=0 t6.0[tt] a8.0=0 a6.0=0 a8.0=1 en8.0 nc0=ff00 ins0(Ann,2) sel9.0=2 t0.0[T9.0:2] sel10.0=5 t1.0[T10.0:5] sel11.0=0 t2.0[T11.0:0] a9.0=0 a0.0=1 a10.0=0 a1.0=0 a10.0=1 a11.0=0 a2.0=0 ") == rw_log);
    assert(strstr(rw_log, "a3.1=0 a4.1=0 t5.1[C2] col5.1=12,20 a7.1=0 a5.1=0 team1=1 t6.1[tt] a8.1=0 a6.1=0 a8.1=1 en8.1 nc1=ff00 sel9.1=4 t0.1[T9.1:4] sel10.1=6 t1.1[T10.1:6] sel11.1=1 t2.1[T11.1:1] a9.1=1 a0.1=0 a10.1=0 a1.1=0 a10.1=1 a11.1=0 a2.1=0 "));
    assert(strstr(rw_log, "a3.2=0 t4.2[] a4.2=1 t5.2[C3] col5.2=13,20 a7.2=0 a5.2=0 a5.2=1 team2=2 t6.2[tt] a8.2=0 a6.2=0 a6.2=1 nc2=ff ins2(Cy,2) sel9.2=2 t0.2[T9.2:2] sel10.2=7 t1.2[T10.2:7] sel11.2=2 t2.2[T11.2:2] a9.2=1 a0.2=0 a10.2=0 a1.2=0 a1.2=1 a11.2=0 a2.2=0 a0.3=0 a1.3=0 "));
    assert(strstr(rw_log, "a11.7=0 ") && !strstr(rw_log, "log["));
    /* A slot sharing colour 1 with slot 0 whose kind shows "?": slot 0's civ/team + 50 copied, shared. */
    rw_n = 0; rw_unknown[0] = 1; aoe_settings_set_byte_ac4_ror(&g, 2, 1); rw_stats[3] = 1;
    aoe_mp_setup_screen_rows_ror(&m, &o);
    assert(strstr(rw_log, "log[    SETTING CIV TEAM 50 0 2] share ") == rw_log && aoe_settings_byte_a94_ror(&g, 2) == 55 && aoe_settings_player_994_ror(&g, 2) == 51);
    assert(strstr(rw_log, "team2=0 ") && strstr(rw_log, "sel10.2=5 "));
    /* Not networked: the first row is a human, the second follows its kind list (0 -> hidden colour/team texts). */
    rw_n = 0; g.flag_97e = 0; rw_sel[AOE_MPS_KIND_LIST][1] = 4; aoe_settings_set_byte_988_ror(&g, 2);
    aoe_mp_setup_screen_rows_ror(&m, &o);
    assert(strstr(rw_log, "a3.0=0 a4.0=0 t5.0[C1] col5.0=11,20 a7.0=0 a5.0=0 a5.0=1 team0=0 t6.0[tt] a8.0=0 a6.0=0 a6.0=1 t0.0[T9.0:2] ") == rw_log && strstr(rw_log, "a3.1=0 a4.1=0 t5.1[C2] col5.1=12,20 a7.1=0 a5.1=0 team1=1 t6.1[tt] a8.1=0 a6.1=0 a6.1=1 t0.1[T9.1:4] ") && strstr(rw_log, "a0.2=0 a1.2=0 a2.2=0 a3.2=0 "));
}

static char cm_log[2048]; static int cm_n, cm_host = 1, cm_types[9] = {0, 2, 2, 0, 4, 0, 0, 0, 0}; static int32_t cm_stats7[7] = {3, 2, 2, 5, 2, 9, 1}, cm_stat = 11;
static void cm_add(const char *fmt, ...) { va_list ap; va_start(ap, fmt); cm_n += vsnprintf(cm_log + cm_n, sizeof cm_log - (size_t)cm_n, fmt, ap); va_end(ap); }
static int32_t cm_type(void *c, int32_t s) { (void)c; return cm_types[s]; }
static void cm_statsf(void *c, int32_t s, int32_t out[7]) { (void)c; (void)s; memcpy(out, cm_stats7, sizeof cm_stats7); }
static int32_t cm_statf(void *c, int32_t s) { (void)c; (void)s; return cm_stat; }
static int32_t cm_rand(void *c) { (void)c; return 37; }
static const char *cm_cname(void *c, int32_t s) { (void)c; return s == 2 ? "Bob" : "X"; }
static void cm_string(void *c, int32_t id, char *o, int n) { (void)c; snprintf(o, (size_t)n, "S%x:%%s", (unsigned)id); }
static void cm_ok(void *c, const char *t) { (void)c; cm_add("ok[%s] ", t); }
static void cm_ready(void *c, int32_t st, int32_t cd, int32_t info) { (void)c; cm_add("ready(%d,%d,%d) ", (int)st, (int)cd, (int)info); }
static void cm_logf(void *c, const char *t) { (void)c; cm_add("log[%s] ", t); }
static void cm_step(void *c, int32_t id, int32_t a) { (void)c; cm_add("%d(%d) ", (int)id, (int)a); }
static void command_tests(void) {
    static AoeGameSettingsRor g; static AoeMpSetupScreenRor m;
    AoeMpSetupCommandOpsRor o = {NULL, 1, 1, 1, 0, 0, cm_type, cm_statsf, cm_statf, cm_rand, cm_cname, cm_string, cm_ok, cm_ready, cm_logf, cm_step};
    memset(&g, 0, sizeof g); memset(&m, 0, sizeof m); m.settings = &g; m.ops = &ops; m.stored_name = "hill";
    aoe_settings_set_byte_988_ror(&g, 3); aoe_settings_set_scenario_name_ror(&g, "hill");
    for (int i = 0; i < 8; ++i) m.colors[i] = (uint8_t)(i + 1);
    /* UPDATE_PLAYERS as host: our slot flagged (CD ok), slot 2's stats applied, slot 3 (empty) randomised, slot 4 (computer) kept. */
    assert(aoe_mp_setup_screen_command_ror(&m, &o, 0x17a4, 0) == 1 && m.field_854 == 1 && aoe_settings_player_flag_ror(&g, 0) == 1);
    assert(aoe_settings_byte_a94_ror(&g, 1) == 3 && aoe_settings_int_aa0_ror(&g, 1) == 1 && aoe_settings_player_flag_ror(&g, 1) == 1 && aoe_settings_byte_ac4_ror(&g, 1) == 5 && aoe_settings_player_994_ror(&g, 1) == 2 && m.field_868[1] == 9 && aoe_settings_player_version_ror(&g, 1) == 1);
    assert(aoe_settings_byte_a94_ror(&g, 2) == 6 && aoe_settings_int_aa0_ror(&g, 2) == 2 && aoe_settings_byte_ac4_ror(&g, 2) == 3 && aoe_settings_player_994_ror(&g, 2) == 1 && aoe_settings_byte_a94_ror(&g, 3) == 0);
    assert(strcmp(cm_log, "log[$$$ USER_COMMAND: COMM_UPDATE_PLAYERS (6052,0)] 1(0) 4(0) 3(1) ") == 0);
    /* UPDATE_PARAMS as a client: civ 55 reduced (ours), the name differs -> +0x730 -1, readiness sent once. */
    cm_n = 0; o.host = 0; aoe_settings_set_byte_a94_ror(&g, 0, 55); m.field_730 = 2; aoe_settings_set_scenario_name_ror(&g, "lake"); m.field_73c = 7; m.field_85c = 11;
    assert(aoe_mp_setup_screen_command_ror(&m, &o, 0x17a6, 0) == 1 && aoe_settings_byte_a94_ror(&g, 0) == 5 && m.field_72c == 0 && m.field_730 == -1 && m.field_844 == 7);
    assert(strcmp(cm_log, "log[$$$ USER_COMMAND: COMM_UPDATE_PARAMS (6054,0)] 2(0) log[$$$ CIV2>50] log[$$$ RESETTING MYCIV2] 5(0) ready(11,2,7) ") == 0);
    cm_n = 0; assert(aoe_mp_setup_screen_command_ror(&m, &o, 0x17a6, 0) == 1 && strcmp(cm_log, "log[$$$ USER_COMMAND: COMM_UPDATE_PARAMS (6054,0)] 2(0) 5(0) ") == 0);
    cm_n = 0; assert(aoe_mp_setup_screen_command_ror(&m, &o, 0x17b6, 2) == 1 && strcmp(cm_log, "log[$$$ USER_COMMAND: COMM_BAD_VERSION (6070,2)] ok[S25dd:Bob] ") == 0);
    cm_n = 0; assert(aoe_mp_setup_screen_command_ror(&m, &o, 0x17b9, 0) == 1 && strstr(cm_log, "] 7(0) "));
    cm_n = 0; o.launched = 1; assert(aoe_mp_setup_screen_command_ror(&m, &o, 0x17b9, 0) == 1 && strstr(cm_log, "] 8(1) "));
    cm_n = 0; o.iconic = 1; assert(aoe_mp_setup_screen_command_ror(&m, &o, 0x17d5, 4) == 1 && m.chat_from == 4 && m.chat_to == 4 && !strstr(cm_log, "9("));
    cm_n = 0; o.iconic = 0; assert(aoe_mp_setup_screen_command_ror(&m, &o, 0x17d5, 5) == 1 && strstr(cm_log, "] 9(5) "));
    cm_n = 0; o.host = 1; aoe_settings_set_player_994_ror(&g, 1, 60);
    assert(aoe_mp_setup_screen_command_ror(&m, &o, 0x17bb, 0) == 1 && aoe_settings_player_994_ror(&g, 1) == 10 && m.field_88c == 1 && strstr(cm_log, "log[$$$ HOST RESET TEAM+50] "));
    cm_n = 0; assert(aoe_mp_setup_screen_command_ror(&m, &o, 0x1234, 1) == 1 && strcmp(cm_log, "log[$$$ USER_COMMAND: UNKNOWN (4660, 1)] ") == 0);
    g.flag_97d = 1; cm_n = 0; assert(aoe_mp_setup_screen_command_ror(&m, &o, 0x17a7, 0) == 1 && cm_n == 0);
}
int main(void) {
    static AoeGameSettingsRor g;
    static AoeMpSetupScreenRor m;
    int i;

    /* Map type pick. */
    assert(aoe_mp_setup_random_map_type_ror(0) == 4 && aoe_mp_setup_random_map_type_ror(11) == 4);
    assert(aoe_mp_setup_random_map_type_ror(2) == 3 && aoe_mp_setup_random_map_type_ror(3) == 2);
    assert(aoe_mp_setup_random_map_type_ror(4) == 1 && aoe_mp_setup_random_map_type_ror(7) == 7);
    assert(aoe_mp_setup_random_map_type_ror(8) == 8 && aoe_mp_setup_random_map_type_ror(19) == 0);
    assert(aoe_mp_setup_screen_slot_68_ror() == 1);

    /* Single player reset. */
    memset(&g, 0, sizeof g);
    m.settings = &g;
    m.ops = &ops;
    m.field_730 = 7;
    g.int_a10 = 5;
    rnd = 13;
    aoe_mp_setup_screen_reset_ror(&m);
    assert(m.field_730 == -1 && m.max_players == 8 && m.max_list == 8);
    assert(g.map_size == 3 && g.int_a80 == 2 && aoe_settings_byte_988_ror(&g) == 4);
    assert(g.option_ad6 == 1 && aoe_settings_byte_982_ror(&g) == 1 && seeded == 0);

    /* Multiplayer reset with a host. */
    memset(&g, 0, sizeof g);
    aoe_settings_set_flag_97e_ror(&g, 1);
    aoe_settings_set_flag_8fc_ror(&g, 1);
    host = 1;
    rnd = 30;
    aoe_mp_setup_screen_reset_ror(&m);
    assert(seeded == 1 && slot48 == 1 && !aoe_settings_flag_8fc_ror(&g));
    assert(g.int_a80 == 3 && aoe_settings_byte_988_ror(&g) == 8 && aoe_settings_byte_982_ror(&g) == 0);
    for (i = 0; i < 8; ++i) {
        assert(m.colors[i] == i + 1 && g.byte_ac4[i] == i + 1);
        assert(g.byte_a94[i] == (30 + i) % 16 + 1 && g.int_aa0[i] == i);
    }

    /* Kick and ready messages. */
    aoe_mp_setup_screen_kick_ror(&m, 3);
    assert(m.kick_player == 3 && !strcmp(yes_text, "Kick Bob?") && !strcmp(yes_name, "Kick Dialog"));
    assert(yes_w == 450 && yes_h == 100);
    aoe_mp_setup_screen_ready_message_ror(&m, 1);
    assert(refresh_arg == 0 && ok_id == 0x25c9);
    aoe_mp_setup_screen_ready_message_ror(&m, 0);
    assert(ok_id == 0x25ca);

    /* Chat: following at the bottom; a full list drops its first line. */
    chat_n = 100; chat_bot = 99;
    aoe_mp_setup_screen_chat_ror(&m, 1);
    assert(removed == 1 && added == 1 && scrolled == 1 && chat_n == 100);
    chat_bot = 10;
    aoe_mp_setup_screen_chat_ror(&m, 1);
    assert(scrolled == 1 && added == 2);

    /* Idle: pending lines, the capture, the host data share. */
    m.chat_from = 2; m.chat_to = 4; released = 0;
    clock_ms = 50;
    aoe_mp_setup_screen_idle_ror(&m);
    assert(added == 5 && m.chat_from == 0 && m.chat_to == 0 && release_calls == 1);
    assert(m.share_time == 50 && shares == 0 && idles == 1);
    aoe_mp_setup_screen_idle_ror(&m);
    assert(shares == 0);
    clock_ms = 60;
    aoe_mp_setup_screen_idle_ror(&m);
    assert(shares == 1 && m.share_time == 60);
    minimised = 1; m.chat_from = 1; m.chat_to = 1;
    aoe_mp_setup_screen_idle_ror(&m);
    assert(added == 5 && m.chat_from == 1);

    /* Player counts 2..max, the current one selected. */
    m.max_list = 6;
    aoe_settings_set_byte_988_ror(&g, 4);
    aoe_mp_setup_screen_fill_counts_ror(&m);
    assert(n_appended == 5 && appended[4] == 6 && selected_index == 2);

    /* Colours: all eight; with a scenario, only used slots (kind 1 when
       +0x97d). */
    aoe_mp_setup_screen_fill_colors_ror(&m);
    assert(color_n[0] == 8 && color_n[7] == 8 && color_ids[3][7] == 8);
    assert(color_rgb[0][0] == 0xff0000 && color_rgb[5][7] == 0x93bf2b);
    aoe_settings_set_flag_8fc_ror(&g, 1);
    m.scenario = 1;
    used_mask = 0x0f;
    aoe_mp_setup_screen_fill_colors_ror(&m);
    assert(color_n[2] == 4 && color_ids[2][3] == 4);
    aoe_settings_set_flag_97d_ror(&g, 1);
    kind1_mask = 0x05;
    aoe_mp_setup_screen_fill_colors_ror(&m);
    assert(color_n[1] == 2 && color_ids[1][1] == 3 && color_rgb[1][1] == 0xffff);

    /* Players from a scenario: slots 0..3 used, the first kind 1 is 2. */
    memset(&g, 0, sizeof g);
    aoe_settings_set_flag_8fc_ror(&g, 1);
    m.scenario = 1; m.max_list = 4;
    used_mask = 0x0f; kind1_mask = 0x0c;
    aoe_mp_setup_screen_players_ror(&m);
    assert(g.int_aa0[0] == 2 && g.byte_a94[0] == 12 && g.byte_ac4[0] == 3);
    assert(g.int_aa0[2] == 0 && g.byte_a94[2] == 10 && g.byte_ac4[2] == 1);
    assert(g.int_aa0[1] == 1 && g.int_aa0[3] == 3 && aoe_settings_byte_988_ror(&g) == 4);
    assert(comm_states[1] == 2 && comm_states[8] == 4 && comm_states[9] == 0);
    assert(aoe_settings_player_994_ror(&g, 7) == 1 && n_appended == 3);
    /* Without a scenario: random civilisations and colours i + 1. */
    memset(&g, 0, sizeof g);
    rnd = 5;
    aoe_mp_setup_screen_players_ror(&m);
    assert(g.byte_a94[0] == 6 && g.byte_a94[7] == 13 && g.byte_ac4[7] == 8);

    m.scenario = 1;
    aoe_mp_setup_screen_destroy_ror(&m);
    assert(freed == 1 && m.scenario == 0 && released_panels == 1);
    assert(!strcmp(closed, "Game Settings Screen,Select Tribe Screen,Kick Dialog,OKDialog,YesNoDialog,"));
    aoe_mp_setup_screen_destroy_ror(&m);
    assert(freed == 1 && released_panels == 2);
    /* Random positions (0x0048a610): humans in slots 1 and 3 of 4. */
    memset(&g, 0, sizeof g);
    aoe_settings_set_byte_988_ror(&g, 4);
    for (i = 0; i < 8; ++i) g.int_aa0[i] = 77;
    kinds[1] = 2; kinds[2] = 1; kinds[3] = 4; kinds[4] = 0;
    m.max_list = 4; present_mask = 0x0f; rnd = 0;
    aoe_mp_setup_screen_random_positions_ror(&m);
    assert(g.int_aa0[0] == 0 && g.int_aa0[1] == 77 && g.int_aa0[2] == 1 && g.int_aa0[3] == 77);
    /* Fewer free positions than humans: nothing. */
    present_mask = 0x01; g.int_aa0[0] = 77;
    aoe_mp_setup_screen_random_positions_ror(&m);
    assert(g.int_aa0[0] == 77);

    /* Tab order (0x0048ced0). */
    m.panel_610 = (void *)1; m.panel_620 = (void *)2; m.panel_700 = (void *)3;
    m.panel_70c = (void *)4; m.panel_714 = (void *)5;
    aoe_settings_set_flag_97d_ror(&g, 1);
    aoe_mp_setup_screen_tab_order_ror(&m);
    assert(ntabs == 3 && tabs[0] == (void *)3 && tabs[1] == (void *)4 && tabs[2] == (void *)2 && focused == (void *)3);
    aoe_settings_set_flag_97d_ror(&g, 0);
    aoe_mp_setup_screen_tab_order_ror(&m);
    assert(ntabs == 4 && tabs[0] == (void *)1 && tabs[3] == (void *)2 && focused == (void *)1);
    m.panel_620 = NULL;
    aoe_mp_setup_screen_tab_order_ror(&m);
    assert(ntabs == 3 && tabs[1] == (void *)5 && tabs[2] == (void *)4);

    /* Leaving (0x0048aa10). */
    closed[0] = 0;
    aoe_mp_setup_screen_leave_ror(&m, 0);
    assert(drop_reason == 5 && wait_n == 1 && !strcmp(shown, "Join Screen") && !strcmp(closed, "MP Setup Screen,"));
    lobby = 1; closed[0] = 0; shown[0] = 0;
    aoe_mp_setup_screen_leave_ror(&m, 0);
    assert(quits == 1 && shown[0] == 0 && closed[0] == 0);
    aoe_mp_setup_screen_leave_ror(&m, 1);
    assert(made_arg == 0x96a && !strcmp(shown, "Main Error Screen"));
    make_ok = 0; shown[0] = 0;
    aoe_mp_setup_screen_leave_ror(&m, 1);
    assert(quits == 2 && shown[0] == 0);
    aoe_settings_set_flag_97d_ror(&g, 1);
    aoe_mp_setup_screen_leave_ror(&m, 0);
    assert(!strcmp(shown, "Single Player Menu"));
    aoe_settings_set_flag_8fc_ror(&g, 1);
    aoe_mp_setup_screen_leave_ror(&m, 0);
    assert(!strcmp(shown, "Select Scenario Screen"));

    /* Addresses (0x0048a780). */
    ok_id = 0; host_n = 0;
    aoe_mp_setup_screen_addresses_ror(&m);
    assert(ok_id == 0x238d && ok_txt[0] == 0 && host_closed == 1);
    host_n = 2;
    aoe_mp_setup_screen_addresses_ror(&m);
    assert(!strcmp(ok_txt, "\n\nHost box\n1.2.3.4 \nal\n") && !strcmp(ok_name, "OKDialog"));
    assert(ok_h == 136 && host_closed == 2);
    host_ok = 0; ok_txt[0] = 0;
    aoe_mp_setup_screen_addresses_ror(&m);
    assert(ok_txt[0] == 0 && host_closed == 2);

    /* 0x004895b0: chat input, a civilisation list, the base fallback. */
    {
        static AoeMpSetupScreenOpsRor o;
        static AoeMpSetupScreenRor a;
        static AoeGameSettingsRor ag;
        static int chat_panel, civ_panel, other;
        o.input_text = a_text; o.input_set = a_set; o.send_chat = a_chat; o.list_id = a_id;
        o.set_civ = a_civ; o.player_count = a_count; o.host = a_host; o.base_action = a_base;
        o.panel_name = a_name;
        memset(&ag, 0, sizeof ag);
        aoe_settings_set_flag_97e_ror(&ag, 1);
        a.settings = &ag; a.ops = &o;
        a.panel_610 = &chat_panel; a.civ_lists[2] = &civ_panel;
        assert(aoe_mp_setup_screen_action_ror(&a, &chat_panel, 0, 0, 0) == 1);
        assert(strcmp(chat_sent, "hi") == 0 && input_now[0] == 0);
        assert(aoe_mp_setup_screen_action_ror(&a, &civ_panel, 0, 0, 0) == 1);
        assert(civ_set == 207 && a.field_72c == 7);
        assert(aoe_mp_setup_screen_action_ror(&a, &other, 0, 0, 0) == 9 && base_calls == 1);
        assert(aoe_mp_setup_screen_action_ror(&a, NULL, 1, 0, 0) == 9 && base_calls == 2);
    }

    puts("mp_setup_screen_test: ok");
    info_tests();
    rows_tests();
    command_tests();
    return 0;
}
