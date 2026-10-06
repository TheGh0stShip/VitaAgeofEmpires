#include "load_game_screen.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static char lines[256], patterns[256];
static int sorted, cleared, released[16], nrel, base, freed;

static void f_clear(void *c) { (void)c; ++cleared; lines[0] = 0; }
static void f_sorted(void *c, int32_t on) { (void)c; sorted = on; }
static void f_append(void *c, const char *t) { (void)c; strcat(lines, t); strcat(lines, ","); }
static const char *f_dir(void *c) { (void)c; return "save/"; }
static void f_find(void *c, const char *p, void (*each)(void *, const char *), void *arg) {
    (void)c;
    strcat(patterns, p); strcat(patterns, ",");
    if (strstr(p, ".gam")) { each(arg, "one.gam"); each(arg, "two.gam"); }
    else each(arg, "three.gmx");
}
static void f_rel(void *c, void *p) { (void)c; released[nrel++] = (int)(intptr_t)p; }
static void f_base(void *c) { (void)c; ++base; }
static void f_free(void *c) { (void)c; ++freed; }

static const AoeLoadGameScreenOpsRor ops = {NULL, f_clear, f_sorted, f_append, f_dir, f_find,
                                            f_rel, f_base, f_free};


static const char *lg_dname; static char lg_closed[32], lg_deleted[2][64], lg_asked[128], lg_shown[32], lg_loaded[64]; static int lg_ndel, lg_refills, lg_disabled, lg_exists = 1, lg_load_ok = 1, lg_ok_id, lg_gs = 1, lg_paused = 1, lg_resumed, lg_waits, lg_spm, lg_closed_self, lg_base; static const char *lg_sel = "save1"; static int32_t lg_lines = 1;
static const char *lg_dialog_name(void *c, void *p) { (void)c; (void)p; return lg_dname; }
static void lg_close(void *c, const char *n) { (void)c; snprintf(lg_closed, 32, "%s", n); }
static const char *lg_selected(void *c) { (void)c; return lg_sel; }
static int32_t lg_count(void *c) { (void)c; return lg_lines; }
static const char *lg_dir(void *c) { (void)c; return "D:/"; }
static void lg_del(void *c, const char *p) { (void)c; if (lg_ndel < 2) snprintf(lg_deleted[lg_ndel], 64, "%s", p); ++lg_ndel; }
static void lg_refill(void *c) { (void)c; ++lg_refills; lg_sel = ""; }
static void lg_disable(void *c) { (void)c; ++lg_disabled; }
static void lg_ask(void *c, const char *t) { (void)c; snprintf(lg_asked, 128, "%s", t); }
static void lg_string(void *c, int32_t id, char *o, int n) { (void)c; snprintf(o, (size_t)n, "Delete %%s? (%d)", (int)id); }
static int lg_fexists(void *c, const char *p) { (void)c; (void)p; return lg_exists; }
static int lg_load(void *c, const char *p) { (void)c; snprintf(lg_loaded, 64, "%s", p); return lg_load_ok; }
static void lg_okd(void *c, int32_t id) { (void)c; lg_ok_id = id; }
static int lg_gs_exists(void *c) { (void)c; return lg_gs; }
static int lg_paused_f(void *c) { (void)c; return lg_paused; }
static int lg_a00(void *c) { (void)c; return 0; }
static void lg_resume(void *c) { (void)c; ++lg_resumed; }
static void lg_wait(void *c) { (void)c; ++lg_waits; }
static void lg_open_spm(void *c) { (void)c; ++lg_spm; }
static void lg_show(void *c, const char *n) { (void)c; snprintf(lg_shown, 32, "%s", n); }
static void lg_close_self(void *c) { (void)c; ++lg_closed_self; }
static int32_t lg_base_f(void *c, void *f, int32_t code) { (void)c; (void)f; (void)code; ++lg_base; return 42; }
static void action_tests(void) {
    AoeLoadGameScreenRor l; int i;
    AoeLoadGameActionOpsRor o = {NULL, lg_dialog_name, lg_close, lg_selected, lg_count, lg_dir, lg_del, lg_refill, lg_disable, lg_ask, lg_string, lg_fexists, lg_load, lg_okd, lg_gs_exists, lg_paused_f, lg_a00, lg_resume, lg_wait, lg_open_spm, lg_show, lg_close_self, lg_base_f};
    memset(&l, 0, sizeof l);
    for (i = 0; i < AOE_LOAD_GAME_PANELS; ++i) l.panels[i] = (void *)(intptr_t)(100 + i);
    /* Delete asks; a yes deletes both files and refreshes; no selection left disables. */
    assert(aoe_load_game_screen_action_ror(&l, &o, l.panels[6], 1) == 42 && strcmp(lg_asked, "Delete save1? (9418)") == 0);
    lg_dname = "Cancel Dialog";
    assert(aoe_load_game_screen_action_ror(&l, &o, (void *)7, 0) == 1 && strcmp(lg_closed, "Cancel Dialog") == 0 &&
           strcmp(lg_deleted[0], "D:/save1.gam") == 0 && strcmp(lg_deleted[1], "D:/save1.gmx") == 0 && lg_refills == 1 && lg_disabled == 1);
    lg_dname = NULL; lg_sel = "save2";
    /* OK loads the .gmx when it exists, the .gam otherwise; a failure shows 0x2501. */
    assert(aoe_load_game_screen_action_ror(&l, &o, l.panels[4], 1) == 1 && strcmp(lg_loaded, "save2.gmx") == 0);
    lg_exists = 0; lg_load_ok = 0;
    assert(aoe_load_game_screen_action_ror(&l, &o, l.panels[2], 3) == 1 && strcmp(lg_loaded, "save2.gam") == 0 && lg_ok_id == 0x2501);
    /* Cancel: back to the game (resumed) or to a new single player menu. */
    assert(aoe_load_game_screen_action_ror(&l, &o, l.panels[5], 1) == 1 && lg_resumed == 1 && strcmp(lg_shown, "Game Screen") == 0 && lg_closed_self == 1);
    lg_gs = 0;
    assert(aoe_load_game_screen_action_ror(&l, &o, l.panels[5], 1) == 1 && lg_waits == 1 && lg_spm == 1 && strcmp(lg_shown, "Single Player Menu") == 0);
    assert(aoe_load_game_screen_action_ror(&l, &o, l.panels[0], 1) == 42);
}
int main(void) {
    AoeLoadGameScreenRor l;
    int i;
    memset(&l, 0, sizeof l);
    l.ops = &ops;
    aoe_load_game_screen_fill_ror(&l);
    assert(cleared == 1 && sorted == 1);
    assert(!strcmp(patterns, "save/*.gam,save/*.gmx,"));
    assert(!strcmp(lines, "one,two,three,"));
    for (i = 0; i < 7; ++i) l.panels[i] = (void *)(intptr_t)(i + 1);
    aoe_load_game_screen_destroy_ror(&l, 0);
    assert(nrel == 7 && released[0] == 1 && released[6] == 7 && base == 1 && freed == 0);
    aoe_load_game_screen_destroy_ror(&l, 1);
    assert(base == 2 && freed == 1 && l.panels[0] == NULL);
    puts("load_game_screen_test: ok");
    action_tests();
    return 0;
}
