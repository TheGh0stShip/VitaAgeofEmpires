/* The Load Saved Game Screen constructor 0x00485c90 (game_ui.h). */
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
    puts("load game constructor ok");
    return 0;
}
