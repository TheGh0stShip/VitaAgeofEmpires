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
    return 0;
}
