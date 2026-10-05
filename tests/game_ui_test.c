/* Patched RoR 1.0a in-game menu 0x00433c30..0x00434010. */
#include "game_ui.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int32_t width(void *c, uintptr_t f, const char *t, int32_t n) { (void)c; (void)f; (void)t; return n * 8; }
static void string(void *c, int32_t id, char *out, int size) {
    (void)c;
    snprintf(out, (size_t)size, "S%d", (int)id);
}
static int fonts(void *c, int32_t index, uintptr_t *font, int32_t *w, int32_t *h) {
    (void)c; *font = (uintptr_t)index + 1; *w = 8; *h = 12; return 1;
}
static int saves = -1, options, achievements, missions;
static AoeUiPanelRor *options_parent;
static void open_save(void *c, int32_t mode) { (void)c; saves = mode; }
static void open_options(void *c, AoeUiPanelRor *p) { (void)c; ++options; options_parent = p; }
static void open_ach(void *c, const char *t) { (void)c; if (strcmp(t, "S9886") == 0) ++achievements; }
static void open_mission(void *c) { (void)c; ++missions; }
static uint8_t state;
static uint8_t player_state(void *c, void *p) { (void)c; (void)p; return state; }

static AoeMenuDialogRor *open_menu(AoeGameUiRor *ui, AoeUiPanelRor *screen) {
    AoeMenuDialogRor *m = calloc(1, sizeof(*m));
    assert(aoe_menu_dialog_init_ror(m, ui, screen));
    return m;
}

static void press(AoeMenuDialogRor *m, int i) {
    AoeUiPanelRor *p = &m->dialog.ez.panel;
    p->vt->action(p, &m->buttons[i]->panel, 1, m->buttons[i]->ids[0], 0);
}

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 800, 600};
    AoeUiEzOpsRor ez_ops = {0};
    AoeGameUiOpsRor ops = {0};
    static AoeGameBaseRor game;
    static AoeGameSettingsRor settings;
    static AoeGameUiRor ui;
    static void *players[2];
    int dummy;
    AoeUiEzRor *screen;
    AoeMenuDialogRor *m;

    pf.text_width = width;
    pf.string = string;
    sys.platform = &pf;
    ez_ops.font = fonts;
    ops.open_save_screen = open_save;
    ops.open_options = open_options;
    ops.open_achievements = open_ach;
    ops.open_mission = open_mission;
    ops.player_state = player_state;
    players[0] = players[1] = &dummy;
    game.has_world = 1;
    game.player_count = 2;
    game.view_player = 1;
    game.players = players;
    ui.system = &sys;
    ui.ez_ops = &ez_ops;
    ui.game = &game;
    ui.settings = &settings;
    ui.ops = &ops;

    screen = calloc(1, sizeof(*screen));
    aoe_ui_ez_init_named_ror(screen, &sys, &ez_ops, "Game Screen");
    assert(aoe_ui_ez_setup_ror(screen, &area, NULL, NULL, -1, 1, 0, 0, 0, 0, 0));
    aoe_ui_system_show_ror(&sys, &screen->panel, 0);

    /* Single player: 300 + 105 high, ten buttons. */
    settings.flag_97d = 1;
    m = open_menu(&ui, &screen->panel);
    assert(m->dialog.ez.panel.height == 405 && m->count == 10);
    assert(m->buttons[0]->ids[0] == 4 && m->buttons[1]->ids[0] == 7 &&
           m->buttons[3]->ids[0] == 1 && m->buttons[5]->ids[0] == 11 &&
           m->buttons[9]->ids[0] == 6 && m->buttons[9]->hotkey == 0x1b);
    /* Rows: 15, 65, 100, 135 (save), 170, 205, 240 (options)... */
    assert(m->buttons[1]->panel.y - m->dialog.ez.panel.y == 0x41 &&
           m->buttons[3]->panel.y - m->dialog.ez.panel.y == 0x87 &&
           m->buttons[6]->panel.y - m->dialog.ez.panel.y == 0xf0 &&
           m->buttons[9]->panel.y - m->dialog.ez.panel.y == 0xf0 + 0x78);
    assert(m->buttons[0]->panel.width == 360 &&
           m->dialog.ez.panel.current == &m->buttons[0]->panel);
    /* Cancel resumes a paused single-player game and closes the menu. */
    press(m, 9);
    assert(!aoe_ui_system_find_ror(&sys, "Menu Dialog") && sys.screen == &screen->panel);

    /* Quit asks, over the game screen. */
    m = open_menu(&ui, &screen->panel);
    press(m, 0);
    assert(ui.quit_requested == 1 && !aoe_ui_system_find_ror(&sys, "Menu Dialog"));
    {
        AoeUiMessageDialogRor *q =
            (AoeUiMessageDialogRor *)aoe_ui_system_find_ror(&sys, "QuitGameDialog");
        assert(q && q->type == 2 && q->dialog.ez.panel.parent == &screen->panel &&
               strcmp(q->text->lines->text, "S3101") == 0);
        aoe_ui_system_close_ror(&sys, "QuitGameDialog");
    }
    /* Options and save go through the callbacks. */
    m = open_menu(&ui, &screen->panel);
    press(m, 6);
    assert(options == 1 && options_parent == &screen->panel);
    m = open_menu(&ui, &screen->panel);
    press(m, 3);
    assert(saves == 0 && !aoe_ui_system_find_ror(&sys, "Menu Dialog"));
    m = open_menu(&ui, &screen->panel);
    press(m, 1);
    assert(achievements == 1);
    m = open_menu(&ui, &screen->panel);
    press(m, 2);
    assert(missions == 1);

    /* Multiplayer while playing: 335 high, Resign second; no save. */
    settings.flag_97d = 0;
    settings.flag_97e = 1;
    m = open_menu(&ui, &screen->panel);
    assert(m->dialog.ez.panel.height == 335 && m->count == 8 &&
           m->buttons[1]->ids[0] == 12);
    press(m, 1);
    assert(aoe_ui_system_find_ror(&sys, "ResignDialog"));
    aoe_ui_system_close_ror(&sys, "ResignDialog");
    /* A player who has lost gets no Resign. */
    state = 2;
    m = open_menu(&ui, &screen->panel);
    assert(m->dialog.ez.panel.height == 300 && m->count == 7);
    aoe_ui_system_close_ror(&sys, "Menu Dialog");
    aoe_ui_system_close_ror(&sys, "Game Screen");
    free(sys.panels);
    return 0;
}
