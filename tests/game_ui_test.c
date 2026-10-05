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
static void open_game_settings(void *c, AoeUiPanelRor *p) { (void)c; ++options; options_parent = p; }
static void open_ach(void *c, const char *t) { (void)c; if (strcmp(t, "S9886") == 0) ++achievements; }
static void open_instructions(void *c) { (void)c; ++missions; }
static int quits, saves_ed, tests, news, opens;
static void *test_target;
static void ed_quit(void *c, AoeUiPanelRor *e) { (void)c; (void)e; ++quits; }
static void ed_save(void *c, AoeUiPanelRor *e, int32_t a, int32_t b) { (void)c; (void)e; assert(a == 1 && b == 0); ++saves_ed; }
static void *ed_test(void *c, AoeUiPanelRor *e) { (void)c; (void)e; return test_target; }
static void run_test(void *c, void *s) { (void)c; assert(s == test_target); ++tests; }
static void ed_save_current(void *c, AoeUiPanelRor *e) { (void)c; (void)e; ++saves_ed; }
static void new_scenario(void *c) { (void)c; ++news; }
static void open_open(void *c) { (void)c; ++opens; }
static char shown[64], sent[64];
static int sounds, commands;
static int chat_cmd(void *c, int16_t pl, const char *t) { (void)c; (void)pl; ++commands; return strcmp(t, "cheat") == 0; }
static void play(void *c, int32_t i) { (void)c; if (i == 2) ++sounds; }
static void show_chat(void *c, const char *t) { (void)c; snprintf(shown, sizeof(shown), "%s", t); }
static void send_chat(void *c, const char *t) { (void)c; snprintf(sent, sizeof(sent), "%s", t); }

/* A fake native edit window for the chat line. */
static char edit_text[128];
static int edit_window;
static void *ed_create(void *c, AoeUiAreaRor *a, int32_t x, int32_t y, int32_t w, int32_t h, int m) {
    (void)c; (void)a; (void)x; (void)y; (void)w; (void)h; (void)m; edit_text[0] = 0; return &edit_window;
}
static void ed_destroy(void *c, void *w) { (void)c; (void)w; }
static void ed_set(void *c, void *w, const char *t) { (void)c; (void)w; snprintf(edit_text, sizeof(edit_text), "%s", t); }
static void ed_get(void *c, void *w, char *o, int32_t n) { (void)c; (void)w; snprintf(o, (size_t)n, "%s", edit_text); }
static int credits;
static const char *pid(void *c) { (void)c; return NULL; }
static void open_credits(void *c) { (void)c; ++credits; }
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
    ops.open_game_settings = open_game_settings;
    ops.open_achievements = open_ach;
    ops.open_instructions = open_instructions;
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

    /* The scenario editor's menu. */
    {
        AoeUiEzRor *editor = calloc(1, sizeof(*editor));
        AoeScenarioMenuDialogRor *sm;
        AoeUiPanelRor *sp;
        ops.editor_quit = ed_quit;
        ops.editor_save = ed_save;
        ops.editor_test_scenario = ed_test;
        ops.test_scenario = run_test;
        ops.editor_save_current = ed_save_current;
        ops.new_scenario = new_scenario;
        ops.open_editor_open = open_open;
        aoe_ui_ez_init_named_ror(editor, &sys, &ez_ops, "Scenario Editor Screen");
        assert(aoe_ui_ez_setup_ror(editor, &area, NULL, NULL, -1, 1, 0, 0, 0, 0, 0));
        sm = calloc(1, sizeof(*sm));
        assert(aoe_scenario_menu_dialog_init_ror(sm, &ui, &editor->panel));
        sp = &sm->dialog.ez.panel;
        assert(sp->width == 460 && sp->height == 400 &&
               sm->buttons[6]->panel.y - sp->y == 0x154 &&
               sm->buttons[0]->panel.width == 400 && sm->buttons[6]->hotkey == 0x1b);
        assert(strcmp(sm->buttons[1]->text1[0], "S9281") == 0);
        sp->vt->action(sp, &sm->buttons[1]->panel, 1, 0, 0);
        assert(saves_ed == 1 && !aoe_ui_system_find_ror(&sys, "Scenario Menu Dialog") &&
               sys.screen == &editor->panel);
        /* Test without a scenario only closes. */
        sm = calloc(1, sizeof(*sm));
        aoe_scenario_menu_dialog_init_ror(sm, &ui, &editor->panel);
        sp = &sm->dialog.ez.panel;
        sp->vt->action(sp, &sm->buttons[5]->panel, 1, 0, 0);
        assert(tests == 0 && !aoe_ui_system_find_ror(&sys, "Scenario Menu Dialog"));
        test_target = &dummy;
        sm = calloc(1, sizeof(*sm));
        aoe_scenario_menu_dialog_init_ror(sm, &ui, &editor->panel);
        sp = &sm->dialog.ez.panel;
        sp->vt->action(sp, &sm->buttons[5]->panel, 1, 0, 0);
        assert(tests == 1);
        /* "New Dialog" answers: Yes saves, then a new scenario. */
        sm = calloc(1, sizeof(*sm));
        aoe_scenario_menu_dialog_init_ror(sm, &ui, &editor->panel);
        sp = &sm->dialog.ez.panel;
        aoe_ui_ez_yes_no_cancel_dialog_ror(&sm->dialog.ez, "new?", "New Dialog", 300, 200);
        {
            AoeUiPanelRor *q = aoe_ui_system_find_ror(&sys, "New Dialog");
            assert(sp->vt->action(sp, q, 0, 0, 0) == 1);
        }
        assert(saves_ed == 2 && news == 1 && !aoe_ui_system_find_ror(&sys, "New Dialog") &&
               !aoe_ui_system_find_ror(&sys, "Scenario Menu Dialog"));
        /* "Open Dialog" No: no save, the open screen. */
        sm = calloc(1, sizeof(*sm));
        aoe_scenario_menu_dialog_init_ror(sm, &ui, &editor->panel);
        sp = &sm->dialog.ez.panel;
        aoe_ui_ez_yes_no_cancel_dialog_ror(&sm->dialog.ez, "open?", "Open Dialog", 300, 200);
        sp->vt->action(sp, aoe_ui_system_find_ror(&sys, "Open Dialog"), 1, 0, 0);
        assert(saves_ed == 2 && opens == 1);
        aoe_ui_system_close_ror(&sys, "Scenario Editor Screen");
    }
    /* The quick chat line: Enter sends; single player shows it locally. */
    {
        AoeUiEditOpsRor eops = {0};
        AoeQuickChatDialogRor *q = calloc(1, sizeof(*q));
        AoeUiPanelRor *qp;
        eops.create = ed_create; eops.destroy = ed_destroy;
        eops.set_text = ed_set; eops.get_text = ed_get;
        ez_ops.edit_ops = &eops;
        ops.chat_command = chat_cmd; ops.play_sound = play;
        ops.show_chat = show_chat; ops.send_chat = send_chat;
        settings.flag_97e = 0;
        assert(aoe_quick_chat_dialog_init_ror(q, &ui, &screen->panel, 0));
        qp = &q->dialog.ez.panel;
        assert(qp->width == 540 && qp->height == 35 &&
               strcmp(q->label->lines->text, "S9832") == 0 &&
               q->edit->max == 0x42 && q->edit->panel.width == 0x195 &&
               qp->current == &q->edit->panel);
        snprintf(edit_text, sizeof(edit_text), "hello");
        qp->vt->action(qp, &q->edit->panel, 0, 0, 0);
        assert(strcmp(shown, "hello") == 0 && sounds == 1 && !sent[0] &&
               !aoe_ui_system_find_ror(&sys, "Send Quick Message Dialog"));
        /* Multiplayer sends; a game command does neither; mode 5 holds. */
        settings.flag_97e = 1;
        aoe_game_ui_chat_ror(&ui, "hi all");
        assert(strcmp(sent, "hi all") == 0);
        aoe_game_ui_chat_ror(&ui, "cheat");
        assert(strcmp(sent, "hi all") == 0 && commands == 3);
        game.mode = 5;
        aoe_game_ui_chat_ror(&ui, "late");
        assert(strcmp(sent, "hi all") == 0);
        /* An empty Enter, or a press outside, just closes. */
        q = calloc(1, sizeof(*q));
        assert(aoe_quick_chat_dialog_init_ror(q, &ui, &screen->panel, 1));
        assert(strcmp(q->label->lines->text, "S9837") == 0);
        qp = &q->dialog.ez.panel;
        qp->vt->mouse_down(qp, 1, 0, 0, 0, 0);
        assert(!aoe_ui_system_find_ror(&sys, "Send Quick Message Dialog"));
    }
    /* About: the version line gets "a"; one copyright line when 9252 has
       text; the default product ID; Close goes back to the game. */
    {
        AoeAboutDialogRor *a = calloc(1, sizeof(*a));
        AoeUiPanelRor *ap;
        ops.product_id = pid;
        ops.open_credits = open_credits;
        settings.flag_97d = 1;
        assert(aoe_about_dialog_init_ror(a, &ui, &screen->panel));
        ap = &a->dialog.ez.panel;
        assert(ap->width == 520 && ap->height == 472);
        assert(strcmp(a->version->lines->text, "S9242a") == 0);
        assert(strcmp(a->copyright->lines->text, "S9243S9252 S9244") == 0 &&
               !a->logo && !a->copyright2);
        assert(strcmp(a->pid->lines->text, "00000-000-0000000-00000") == 0);
        assert(a->close->hotkey == 0x1b && ap->current == &a->credits->panel);
        ap->vt->action(ap, &a->close->panel, 1, 0, 0);
        assert(!aoe_ui_system_find_ror(&sys, "About Dialog") && sys.screen == &screen->panel);
        a = calloc(1, sizeof(*a));
        aoe_about_dialog_init_ror(a, &ui, &screen->panel);
        ap = &a->dialog.ez.panel;
        ap->vt->action(ap, &a->credits->panel, 1, 0, 0);
        assert(credits == 1 && !aoe_ui_system_find_ror(&sys, "About Dialog"));
    }
    aoe_ui_system_close_ror(&sys, "Game Screen");
    free(sys.panels);
    return 0;
}
