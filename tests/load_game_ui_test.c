/* The Load Saved Game Screen constructor 0x00485c90 and its action
   handler 0x00486200 (game_ui.h). */
#include "game_ui.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui_text.h"

static int32_t width(void *c, uintptr_t f, const char *t, int32_t n) { (void)c; (void)f; (void)t; return n * 8; }
static void string(void *c, int32_t id, char *out, int size) {
    (void)c;
    snprintf(out, (size_t)size, "S%d", (int)id);
}
static int fonts(void *c, int32_t index, uintptr_t *font, int32_t *w, int32_t *h) {
    (void)c; *font = (uintptr_t)index + 1; *w = 8; *h = 12; return 1;
}

/* The saved-game directory and its files: two .gam and one .gmx when
   `have_files`; the first saved-game name. */
static int have_files;
static const char *save_name;
static char patterns[128];
static const char *game_dir(void *c) { (void)c; return "save/"; }
static void list_files(void *c, const char *pattern, void (*found)(void *user, const char *name),
                       void *user) {
    (void)c;
    strncat(patterns, pattern, sizeof patterns - strlen(patterns) - 1);
    strncat(patterns, ",", sizeof patterns - strlen(patterns) - 1);
    if (!have_files) return;
    if (strstr(pattern, ".gam")) {
        found(user, "one.gam");
        found(user, "two.gam");
    } else {
        found(user, "three.gmx");
    }
}
static const char *default_save_name(void *c) { (void)c; return save_name; }

/* The action handler's files, game and screens: a .gmx exists only for
   "three"; deletes and loads recorded; the load succeeds when `load_ok`;
   the Single Player Menu made and registered as the original's
   0x49e040. */
static AoeUiSystemRor *g_sys;
static const AoeUiEzOpsRor *g_ez_ops;
static AoeUiAreaRor *g_area;
static char checked[64], deleted[4][64], loaded[64];
static int deletes, loads, load_ok, resumes, waits, sp_opens;
static AoeUiEzRor *sp_menu;
static int file_exists(void *c, const char *path) {
    (void)c;
    snprintf(checked, sizeof checked, "%s", path);
    return strcmp(path, "save/three.gmx") == 0;
}
static void delete_file(void *c, const char *path) {
    (void)c;
    if (deletes < 4) snprintf(deleted[deletes], sizeof deleted[0], "%s", path);
    ++deletes;
}
static int load_game(void *c, const char *path) {
    (void)c;
    snprintf(loaded, sizeof loaded, "%s", path);
    ++loads;
    return load_ok;
}
static void resume_game(void *c) { (void)c; ++resumes; }
static void wait_cursor(void *c) { (void)c; ++waits; }
static void open_sp_menu(void *c) {
    (void)c;
    ++sp_opens;
    sp_menu = calloc(1, sizeof *sp_menu);
    aoe_ui_ez_init_named_ror(sp_menu, g_sys, g_ez_ops, "Single Player Menu");
    assert(aoe_ui_ez_setup_ror(sp_menu, g_area, NULL, NULL, -1, 1, 0, 0, 0, 0, 0));
}
static void select_line(AoeLoadGameUiRor *l, const char *text) {
    int32_t i = aoe_ui_text_index_of_ror(&l->list->text, text);
    assert(i >= 0);
    aoe_ui_text_select_ror(&l->list->text, (int16_t)i);
}

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 800, 600};
    AoeUiEzOpsRor ez_ops = {0};
    AoeGameUiOpsRor ops = {0};
    static AoeGameSettingsRor settings;
    static AoeGameUiRor ui;
    AoeLoadGameUiRor *l;
    int32_t two;

    pf.text_width = width;
    pf.string = string;
    sys.platform = &pf;
    ez_ops.font = fonts;
    ops.game_dir = game_dir;
    ops.list_files = list_files;
    ops.default_save_name = default_save_name;
    ui.system = &sys;
    ui.ez_ops = &ez_ops;
    ui.settings = &settings;
    ui.ops = &ops;

    /* No saved games: the panels, the search for "<dir>*.gam" then
       "<dir>*.gmx", nothing selected, OK and Delete disabled, the list
       focused, Escape on Cancel and Delete on Delete. */
    l = calloc(1, sizeof *l);
    assert(aoe_load_game_ui_init_ror(l, &ui, &area) == 1);
    assert(strcmp(aoe_ui_text_line_ror(l->title, 0), "S9471") == 0);
    assert(strcmp(aoe_ui_text_line_ror(l->label, 0), "S9474") == 0);
    assert(l->list && l->list_bar && l->ok && l->cancel && l->remove);
    assert(strcmp(patterns, "save/*.gam,save/*.gmx,") == 0);
    assert(aoe_ui_text_count_ror(&l->list->text) == 0);
    assert(l->ok->disabled == 1 && l->remove->disabled == 1);
    assert(l->cancel->hotkey == 0x1b && l->cancel->hotkey_shift == 0);
    assert(l->remove->hotkey == 0x2e && l->remove->hotkey_shift == 0);
    assert(l->ez.panel.current == &l->list->text.panel);
    assert(l->state.panels[2] == (void *)l->list && l->state.panels[4] == (void *)l->ok &&
           l->state.panels[5] == (void *)l->cancel && l->state.panels[6] == (void *)l->remove);
    assert(l->state.ops == &l->state_ops && l->action_ops == NULL);
    l->ez.panel.vt->destroy(&l->ez.panel);
    free(l);

    /* Three saved games (listed without their extensions) and a first
       saved-game name among them: it is selected (0x004650c0(1, i, 1)). */
    patterns[0] = '\0';
    have_files = 1;
    save_name = "two";
    l = calloc(1, sizeof *l);
    assert(aoe_load_game_ui_init_ror(l, &ui, &area) == 1);
    assert(aoe_ui_text_count_ror(&l->list->text) == 3);
    two = aoe_ui_text_index_of_ror(&l->list->text, "two");
    assert(two >= 0 && aoe_ui_text_index_of_ror(&l->list->text, "three") >= 0 &&
           aoe_ui_text_index_of_ror(&l->list->text, "one.gam") == -1);
    assert(aoe_ui_text_selected_ror(&l->list->text) == two);
    assert(strcmp(aoe_ui_text_selected_text_ror(&l->list->text), "two") == 0);
    l->ez.panel.vt->destroy(&l->ez.panel);
    free(l);

    /* A name that is not listed selects nothing. */
    save_name = "nothing";
    l = calloc(1, sizeof *l);
    assert(aoe_load_game_ui_init_ror(l, &ui, &area) == 1);
    assert(aoe_ui_text_index_of_ror(&l->list->text, "nothing") == -1);
    l->ez.panel.vt->destroy(&l->ez.panel);
    free(l);

    /* The action handler 0x00486200 in the vtable (+0xb8). */
    {
        static AoeLoadGameActionOpsRor aops;
        AoeUiPanelRor *p, *d;
        AoeUiEzRor *game;
        g_sys = &sys;
        g_ez_ops = &ez_ops;
        g_area = &area;
        ops.file_exists = file_exists;
        ops.delete_file = delete_file;
        ops.resume_game = resume_game;
        ops.wait_cursor = wait_cursor;
        ops.open_single_player_menu = open_sp_menu;
        aops.load_game = load_game;
        save_name = "two";
        l = calloc(1, sizeof *l);
        assert(aoe_load_game_ui_init_ror(l, &ui, &area) == 1);
        p = &l->ez.panel;
        assert(p->vt->action == aoe_load_game_ui_action_ror);
        assert(aoe_ui_system_find_ror(&sys, "Load Saved Game Screen") == p);
        assert(l->ok->disabled == 0 && l->remove->disabled == 0);

        /* OK without a load op: the load fails, the OK dialog (0x2501)
           shows, and its answer is the base slot's (it closes). */
        assert(p->vt->action(p, &l->ok->panel, 1, 0, 0) == 1);
        d = aoe_ui_system_find_ror(&sys, "OKDialog");
        assert(d && loads == 0);
        assert(p->vt->action(p, d, 0, 0, 0) == 1);
        assert(!aoe_ui_system_find_ror(&sys, "OKDialog"));

        /* OK on "two" (no .gmx): "save/two.gmx" checked, "two.gam" loaded. */
        l->action_ops = &aops;
        load_ok = 1;
        assert(p->vt->action(p, &l->ok->panel, 1, 0, 0) == 1);
        assert(loads == 1 && strcmp(checked, "save/two.gmx") == 0 &&
               strcmp(loaded, "two.gam") == 0 && !aoe_ui_system_find_ror(&sys, "OKDialog"));
        /* A double click on the list (code 3) acts as OK: "three.gmx". */
        select_line(l, "three");
        assert(p->vt->action(p, &l->list->text.panel, 3, 0, 0) == 1);
        assert(loads == 2 && strcmp(checked, "save/three.gmx") == 0 &&
               strcmp(loaded, "three.gmx") == 0);
        /* A failed load shows the OK dialog. */
        load_ok = 0;
        assert(p->vt->action(p, &l->ok->panel, 1, 0, 0) == 1);
        assert(loads == 3 && aoe_ui_system_find_ror(&sys, "OKDialog"));
        aoe_ui_system_close_ror(&sys, "OKDialog");
        /* OK's other codes and a selection change (list code 1) are the
           base slot's: nothing loads and the buttons stay enabled. */
        assert(p->vt->action(p, &l->ok->panel, 2, 0, 0) == 0);
        select_line(l, "one");
        assert(p->vt->action(p, &l->list->text.panel, 1, 0, 0) == 0);
        assert(loads == 3 && l->ok->disabled == 0 && l->remove->disabled == 0);

        /* Delete asks about "one" (the "Cancel Dialog"); No keeps it. */
        assert(p->vt->action(p, &l->remove->panel, 1, 0, 0) == 0);
        d = aoe_ui_system_find_ror(&sys, "Cancel Dialog");
        assert(d);
        assert(p->vt->action(p, d, 1, 0, 0) == 1);
        assert(!aoe_ui_system_find_ror(&sys, "Cancel Dialog") && deletes == 0 &&
               aoe_ui_text_count_ror(&l->list->text) == 3);
        /* Yes deletes its .gam and .gmx and the line; "three" is selected
           next and OK and Delete stay enabled. */
        p->vt->action(p, &l->remove->panel, 1, 0, 0);
        assert(p->vt->action(p, aoe_ui_system_find_ror(&sys, "Cancel Dialog"), 0, 0, 0) == 1);
        assert(!aoe_ui_system_find_ror(&sys, "Cancel Dialog") && deletes == 2 &&
               strcmp(deleted[0], "save/one.gam") == 0 && strcmp(deleted[1], "save/one.gmx") == 0);
        assert(aoe_ui_text_count_ror(&l->list->text) == 2 &&
               aoe_ui_text_index_of_ror(&l->list->text, "one") == -1 &&
               strcmp(aoe_ui_text_selected_text_ror(&l->list->text), "three") == 0);
        assert(l->ok->disabled == 0 && l->remove->disabled == 0);
        /* The last two go: with nothing selected OK and Delete are disabled. */
        p->vt->action(p, &l->remove->panel, 1, 0, 0);
        p->vt->action(p, aoe_ui_system_find_ror(&sys, "Cancel Dialog"), 0, 0, 0);
        assert(l->ok->disabled == 0 && l->remove->disabled == 0);
        p->vt->action(p, &l->remove->panel, 1, 0, 0);
        p->vt->action(p, aoe_ui_system_find_ror(&sys, "Cancel Dialog"), 0, 0, 0);
        assert(deletes == 6 && strcmp(deleted[2], "save/three.gam") == 0 &&
               strcmp(deleted[3], "save/three.gmx") == 0);
        assert(aoe_ui_text_count_ror(&l->list->text) == 0);
        assert(l->ok->disabled == 1 && l->remove->disabled == 1);
        /* Delete with nothing selected asks nothing; OK with no lines
           loads nothing. */
        p->vt->action(p, &l->remove->panel, 1, 0, 0);
        assert(!aoe_ui_system_find_ror(&sys, "Cancel Dialog"));
        assert(p->vt->action(p, &l->ok->panel, 1, 0, 0) == 1);
        assert(loads == 3 && !aoe_ui_system_find_ror(&sys, "OKDialog"));

        /* Cancel without a "Game Screen": the wait cursor, a new "Single
           Player Menu" shown, and this screen closed (freed). */
        assert(p->vt->action(p, &l->cancel->panel, 1, 0, 0) == 1);
        assert(waits == 1 && sp_opens == 1 && resumes == 0);
        assert(sp_menu && sys.screen == &sp_menu->panel);
        assert(!aoe_ui_system_find_ror(&sys, "Load Saved Game Screen"));
        aoe_ui_system_close_ror(&sys, "Single Player Menu");
        sp_menu = NULL;

        /* Cancel with a "Game Screen": the game resumed and shown. */
        game = calloc(1, sizeof *game);
        aoe_ui_ez_init_named_ror(game, &sys, &ez_ops, "Game Screen");
        assert(aoe_ui_ez_setup_ror(game, &area, NULL, NULL, -1, 1, 0, 0, 0, 0, 0));
        have_files = 0;
        l = calloc(1, sizeof *l);
        assert(aoe_load_game_ui_init_ror(l, &ui, &area) == 1);
        p = &l->ez.panel;
        assert(p->vt->action(p, &l->cancel->panel, 1, 0, 0) == 1);
        assert(resumes == 1 && waits == 1 && sp_opens == 1 && sys.screen == &game->panel);
        assert(!aoe_ui_system_find_ror(&sys, "Load Saved Game Screen"));
        aoe_ui_system_close_ror(&sys, "Game Screen");
    }
    puts("load game constructor ok");
    return 0;
}
