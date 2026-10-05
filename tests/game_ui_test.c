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
static int cds, resets, programs_closed, sp_menus, mp_screens, ed_menus, demo_ok;
static int cd_check(void *c) { (void)c; ++cds; return cds > 1; }
static void reset_input(void *c) { (void)c; ++resets; }
static void quit_program(void *c) { (void)c; ++programs_closed; }
static int play_demo(void *c) { (void)c; return demo_ok; }
static void open_sp(void *c) { (void)c; ++sp_menus; }
static void open_mp(void *c) { (void)c; ++mp_screens; }
static void open_ed(void *c) { (void)c; ++ed_menus; }
static char *load_info(void *c, const char *f, int32_t id) {
    (void)c; (void)f; (void)id;
    char *s = malloc(4);
    strcpy(s, "x");
    return s;
}
static uint32_t now;
static uint32_t clock_now(void *c) { (void)c; return now; }
static int music_from = -1, music_to = -1;
static int32_t track(void *c) { (void)c; return 0xf; }
static void play_music(void *c, int32_t a, int32_t b) { (void)c; music_from = a; music_to = b; }
static uint8_t *read_file(void *c, const char *path, uint32_t *size) {
    uint8_t *b;
    (void)c;
    if (strcmp(path, "data2\\list.cr") != 0) return NULL;
    b = malloc(5);
    memcpy(b, "A\nB\nC", 5);
    *size = 5;
    return b;
}
static char shapes[8];
static void *load_shape(void *c, const char *f, int32_t id) {
    (void)c;
    if (f && strncmp(f, "credit", 6) == 0 && id == 0xc3e9 + (f[6] - '0'))
        return &shapes[f[6] - '0'];
    return NULL;
}
static void free_shape(void *c, void *s) { (void)c; (void)s; }
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

static char loaded[64], deleted[64];
static int mp_setups;
static void mp_setup(void *c) { (void)c; ++mp_setups; }
static const char *idx_name(void *c, int32_t i) { (void)c; return i == 0 ? "Gaul.scn" : i == 1 ? "Egypt.scx" : NULL; }
static int idx_info(void *c, int32_t id, const char **d, int32_t *f, int32_t *pl) {
    (void)c; *d = id == 0 ? "About Gaul" : "About Egypt"; *f = id == 0; *pl = id == 0 ? 3 : 4; return 1;
}
static int deletes, open_screens, new_names, has_hints, starts, resumes, pictures;
static const char *scen_title(void *c) { (void)c; return "Rome"; }
static const char *scen_message(void *c, int32_t i) { (void)c; return i == 0 ? "Instr" : (i == 4 && has_hints) ? "Hints" : NULL; }
static void start_game(void *c) { (void)c; ++starts; }
static void resume_game(void *c) { (void)c; ++resumes; }
static void draw_pic(void *c, void *p, AoeUiAreaRor *a, int32_t x, int32_t y) { (void)c; (void)p; (void)a; (void)x; (void)y; ++pictures; }
static void new_name(void *c, AoeUiPanelRor *p) { (void)c; (void)p; ++new_names; }
static int32_t no_players(void *c) { (void)c; return 0; }
static void open_open_screen(void *c) { (void)c; ++open_screens; }
static void list_files(void *c, const char *pat, void (*found)(void *, const char *), void *u) {
    (void)c;
    if (strstr(pat, ".scn")) {
        found(u, "zed.scn");
        found(u, "alpha.scn");
    } else {
        found(u, "beta.scx");
    }
}
static int file_exists(void *c, const char *path) { (void)c; return strstr(path, "beta.scn") == NULL; }
static void delete_file(void *c, const char *path) { (void)c; ++deletes; snprintf(deleted, sizeof deleted, "%s", path); }
static const char *scen_dir(void *c) { (void)c; return "scen/"; }
static void editor_load(void *c, const char *f, int32_t flag) { (void)c; (void)flag; snprintf(loaded, sizeof loaded, "%s", f ? f : ""); }

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

    /* The main menu: five buttons, the CD check before single player,
       credits on C, the demo on L, Exit closes the program. Opening another
       screen closes (and frees) the menu, so each check makes a new one. */
    {
        AoeMainMenuRor *mm;
        AoeUiPanelRor *mp;
        ez_ops.load_info = load_info;
        ops.cd_check = cd_check;
        ops.reset_input = reset_input;
        ops.quit_program = quit_program;
        ops.play_demo = play_demo;
        ops.open_single_player_menu = open_sp;
        ops.open_mp_startup = open_mp;
        ops.open_editor_menu = open_ed;
#define NEW_MENU()                                                             \
        do {                                                                   \
            mm = calloc(1, sizeof *mm);                                        \
            assert(aoe_main_menu_init_ror(mm, &ui, &area, NULL));              \
            mp = &mm->ez.panel;                                                \
        } while (0)
        NEW_MENU();
        assert(aoe_ui_system_find_ror(&sys, "Main Menu") == mp);
        assert(mm->buttons[4] && !mm->buttons[5]);
        assert(strcmp(mm->buttons[0]->text1[0], "S9202") == 0 &&
               strcmp(mm->buttons[4]->text1[0], "S9207") == 0);
        assert(mm->title && mm->warning && mp->current == &mm->buttons[0]->panel);
        mp->vt->action(mp, &mm->buttons[0]->panel, 1, 0, 0);
        assert(cds == 1 && sp_menus == 0 && aoe_ui_system_find_ror(&sys, "OKDialog"));
        aoe_ui_system_close_ror(&sys, "OKDialog");
        mp->vt->action(mp, &mm->buttons[4]->panel, 1, 0, 0);
        assert(programs_closed == 1);
        mp->vt->key_action(mp, 'L', 0, 0, 1, 0);
        assert(aoe_ui_system_find_ror(&sys, "OKDialog"));
        aoe_ui_system_close_ror(&sys, "OKDialog");
        mp->vt->action(mp, &mm->buttons[0]->panel, 1, 0, 0);
        assert(cds == 2 && ui.cd_ok && sp_menus == 1 && resets == 1);
        assert(!aoe_ui_system_find_ror(&sys, "Main Menu"));
        NEW_MENU();
        mp->vt->action(mp, &mm->buttons[3]->panel, 1, 0, 0);
        assert(cds == 2 && ed_menus == 1);
        NEW_MENU();
        mp->vt->action(mp, &mm->buttons[1]->panel, 1, 0, 0);
        assert(mp_screens == 1);
        NEW_MENU();
        mp->vt->key_action(mp, 'C', 0, 0, 1, 0);
        assert(credits == 2 && !aoe_ui_system_find_ror(&sys, "Main Menu"));
#undef NEW_MENU
    }
    /* The error screen: OK and X close the program. */
    {
        AoeErrorScreenRor *es = calloc(1, sizeof *es);
        AoeUiPanelRor *ep = &es->ez.panel;
        assert(aoe_error_screen_init_ror(es, &ui, &area));
        aoe_error_screen_set_message_ror(es, 2007);
        assert(strcmp(es->message->lines->text, "S2007") == 0);
        ep->vt->action(ep, &es->ok->panel, 1, 0, 0);
        ep->vt->action(ep, &es->close->panel, 1, 0, 0);
        assert(programs_closed == 3);
        aoe_ui_system_close_ror(&sys, "Main Error Screen");
    }
    /* Credits: the text rises 30 px a second; pictures turn every ten
       seconds (no fade without a full-screen display); Enter, or the
       text's end, goes back to the game screen. */
    {
        AoeCreditsScreenRor *cr = calloc(1, sizeof *cr);
        AoeUiPanelRor *cp = &cr->ez.panel;
        pf.now_ms = clock_now;
        ops.read_file = read_file;
        ops.music_track = track;
        ops.play_music = play_music;
        ez_ops.load_shape = load_shape;
        ez_ops.free_shape = free_shape;
        assert(aoe_credits_screen_init_ror(cr, &ui, &area));
        aoe_ui_system_show_ror(&sys, cp, 0);
        assert(music_from == 15 && music_to == 15);
        assert(cr->text && cr->text->text.count == 3 && cr->text->speed == 30);
        assert(cr->text->text.panel.x == 400 - 250 &&
               cr->text->text.panel.y == 300 - 150 &&
               cr->text->text.panel.width == 500);
        assert(cr->pictures[1] == &shapes[2] && cr->pictures[6] == &shapes[7]);
        now = 1000;
        cr->text->text.panel.vt->handle_idle(&cr->text->text.panel);
        cp->vt->handle_idle(cp);
        assert(cr->state == 1 && cr->shown_ms == 1000);
        now = 2000;
        cr->text->text.panel.vt->handle_idle(&cr->text->text.panel);
        assert(cr->text->offset == 30);
        now = 11000;
        cp->vt->handle_idle(cp);
        assert(cr->state == 4);
        cp->vt->handle_idle(cp);
        assert(cr->switch_ms == 11000 && cr->state == 4);
        cp->vt->handle_idle(cp);
        assert(cr->state == 1 && cr->picture == 2 &&
               cr->ez.background == &shapes[2] && cr->shown_ms == 11000);
        /* The end of the text leaves on the next idle. */
        cr->text->done = 1;
        cr->text->text.panel.vt->handle_idle(&cr->text->text.panel);
        assert(cr->leave == 1);
        cp->vt->handle_idle(cp);
        assert(!aoe_ui_system_find_ror(&sys, "Credits Screen") &&
               sys.screen == &screen->panel);
        assert(music_from == 5 && music_to == 14);
        /* Enter leaves too. */
        cr = calloc(1, sizeof *cr);
        cp = &cr->ez.panel;
        assert(aoe_credits_screen_init_ror(cr, &ui, &area));
        aoe_ui_system_show_ror(&sys, cp, 0);
        assert(cp->vt->key_action(cp, 0xd, 0, 0, 1, 0) == 1);
        assert(!aoe_ui_system_find_ror(&sys, "Credits Screen"));
    }
    /* The editor's open screen. */
    {
        AoeScenarioEditorOpenRor *o = calloc(1, sizeof *o);
        AoeUiPanelRor *op = &o->ez.panel;
        ops.list_files = list_files;
        ops.file_exists = file_exists;
        ops.delete_file = delete_file;
        ops.scenario_dir = scen_dir;
        ops.editor_load = editor_load;
        assert(aoe_scenario_editor_open_init_ror(o, &ui, &area, NULL, -1));
        assert(aoe_ui_system_find_ror(&sys, "Scenario Editor Open") == op);
        assert(aoe_ui_text_count_ror(&o->list->text) == 3);
        assert(!strcmp(aoe_ui_text_line_ror(&o->list->text, 0), "alpha"));
        assert(!strcmp(aoe_ui_text_line_ror(&o->list->text, 2), "zed"));
        assert(o->cancel->hotkey == 0x1b && o->del->hotkey == 0x2e);
        assert(op->current == &o->list->text.panel);
        /* Open picks the .scx when there is no .scn. */
        aoe_ui_text_select_ror(&o->list->text, 1);
        op->vt->action(op, &o->list->text.panel, 3, 0, 0);
        assert(!strcmp(loaded, "beta.scx"));
        aoe_ui_text_select_ror(&o->list->text, 0);
        op->vt->action(op, &o->open->panel, 1, 0, 0);
        assert(!strcmp(loaded, "alpha.scn"));
        /* Delete asks, yes deletes both files and the line. */
        op->vt->action(op, &o->del->panel, 1, 0, 0);
        assert(aoe_ui_system_find_ror(&sys, "Cancel Dialog"));
        op->vt->action(op, aoe_ui_system_find_ror(&sys, "Cancel Dialog"), 0, 0, 0);
        assert(!aoe_ui_system_find_ror(&sys, "Cancel Dialog") && deletes == 2);
        assert(!strcmp(deleted, "scen/alpha.scx") && aoe_ui_text_count_ror(&o->list->text) == 2);
        /* Cancel without an editor screen: the editor menu. */
        op->vt->action(op, &o->cancel->panel, 1, 0, 0);
        assert(!aoe_ui_system_find_ror(&sys, "Scenario Editor Open"));
    }
    /* The editor menu. */
    {
        AoeScenarioEditorMenuRor *m = calloc(1, sizeof *m);
        AoeUiPanelRor *mp = &m->ez.panel;
        ops.open_editor_open_screen = open_open_screen;
        assert(aoe_scenario_editor_menu_init_ror(m, &ui, &area));
        assert(m->buttons[3] && m->buttons[3]->hotkey == 0x1b && m->close);
        assert(strcmp(m->buttons[1]->text1[0], "S9263") == 0);
        assert(m->buttons[2]->panel.y - mp->y == (0xaa + 0x64) * 600 / 480);
        loaded[0] = 'x';
        mp->vt->action(mp, &m->buttons[0]->panel, 1, 0, 0);
        assert(loaded[0] == 0);
        mp->vt->action(mp, &m->buttons[1]->panel, 1, 0, 0);
        assert(open_screens == 1 && !aoe_ui_system_find_ror(&sys, "Scenario Editor Menu"));
    }
    /* The status screen. */
    {
        AoeStatusScreenRor *st = calloc(1, sizeof *st);
        assert(aoe_status_screen_init_ror(st, &ui, &area, "Wait Screen", "Please wait", 0, "scrx",
                                          77));
        assert(aoe_ui_ez_info_id_ror(&st->ez) == 77 && st->text);
        assert(st->text->panel.width == 0x230 * 800 / 640);
        aoe_status_screen_set_text_ror(st, "Done");
        assert(!strcmp(st->text->lines->text, "Done"));
        aoe_ui_system_close_ror(&sys, "Wait Screen");
        st = calloc(1, sizeof *st);
        sys.screen = NULL;
        assert(aoe_status_screen_init_ror(st, &ui, &area, "Wait Screen", NULL, 9, NULL, -1));
        assert(aoe_ui_ez_info_id_ror(&st->ez) == 0xc384 &&
               !strcmp(aoe_ui_ez_info_name_ror(&st->ez), "scr2"));
        aoe_ui_system_close_ror(&sys, "Wait Screen");
    }
    /* The single player menu. */
    {
        AoeSinglePlayerMenuRor *m = calloc(1, sizeof *m);
        AoeUiPanelRor *mp = &m->ez.panel;
        ops.open_new_name = new_name;
        ops.campaign_players = no_players;
        assert(aoe_single_player_menu_init_ror(m, &ui, &area));
        assert(strcmp(m->buttons[1]->text1[0], "S9224") == 0 && m->buttons[5]->hotkey == 0x1b);
        assert(m->buttons[5]->panel.y - mp->y == (0x76 + 250) * 600 / 480);
        assert(aoe_settings_flag_97d_ror(&settings) == 1);
        mp->vt->action(mp, &m->buttons[1]->panel, 1, 0, 0);
        assert(new_names == 1 && aoe_ui_system_find_ror(&sys, "Single Player Menu") == mp);
        mp->vt->action(mp, &m->buttons[2]->panel, 1, 0, 0);
        assert(aoe_settings_option_ae5_ror(&settings) == 1 && aoe_settings_flag_8fc_ror(&settings) == 0);
        assert(!aoe_ui_system_find_ror(&sys, "Single Player Menu"));
    }
    /* The mission dialog. */
    {
        AoeMissionDialogRor *d = calloc(1, sizeof *d);
        AoeUiPanelRor *dp = &d->ez.panel;
        ops.scenario_title = scen_title;
        ops.scenario_message = scen_message;
        ops.start_game = start_game;
        ops.resume_game = resume_game;
        ops.draw_picture = draw_pic;
        has_hints = 1;
        assert(aoe_mission_dialog_init_ror(d, &ui, &area, "x", 0, &d));
        assert(strcmp(d->title->lines->text, "Rome") == 0);
        assert(strcmp(d->text->lines->text, "Instr") == 0);
        assert(d->buttons[0]->panel.active && d->buttons[1]->panel.active && !d->buttons[2]->panel.active);
        assert(d->ok->panel.x - dp->x == 0x25d * 800 / 800);
        dp->vt->action(dp, &d->buttons[0]->panel, 1, 0, 0);
        assert(strcmp(d->text->lines->text, "Hints") == 0);
        dp->vt->draw(dp);
        assert(pictures == 1);
        dp->vt->action(dp, &d->ok->panel, 1, 0, 0);
        assert(starts == 1 && !aoe_ui_system_find_ror(&sys, "Mission Dialog"));
        /* Only the instructions: no buttons. */
        has_hints = 0;
        d = calloc(1, sizeof *d);
        dp = &d->ez.panel;
        assert(aoe_mission_dialog_init_ror(d, &ui, &area, "x", 1, NULL));
        assert(!d->buttons[0]->panel.active && !d->buttons[1]->panel.active);
        dp->vt->action(dp, &d->ok->panel, 1, 0, 0);
        assert(resumes == 1 && starts == 1);
    }
    /* The scenario choice. */
    {
        AoeSelectScenarioRor *se = calloc(1, sizeof *se);
        AoeUiPanelRor *sp2 = &se->ez.panel;
        ops.scenario_index_name = idx_name;
        ops.scenario_index_info = idx_info;
        ops.open_mp_setup = mp_setup;
        aoe_settings_set_scenario_name_ror(&settings, "Gaul");
        assert(aoe_select_scenario_init_ror(se, &ui, &area));
        assert(se->count == 2 && aoe_ui_text_count_ror(&se->list->text) == 2);
        assert(!strcmp(aoe_ui_text_line_ror(&se->list->text, 0), "Egypt"));
        assert(!strcmp(aoe_ui_text_column_ror(&se->list->text, 0), "2 - 4"));
        assert(!strcmp(aoe_ui_text_column_ror(&se->list->text, 1), "3"));
        assert(aoe_ui_text_selection_ror(&se->list->text) == 1);
        assert(!strcmp(se->description->lines->text, "About Gaul"));
        sp2->vt->action(sp2, &se->list->text.panel, 3, 0, 0);
        assert(mp_setups == 1 && aoe_settings_flag_8fc_ror(&settings) == 1);
        assert(!strcmp(aoe_settings_scenario_name_ror(&settings), "Gaul"));
        assert(!aoe_ui_system_find_ror(&sys, "Select Scenario Screen"));
    }
    /* The game settings screen (single player, random map). */
    {
        AoeGameSettingsScreenRor *gs = calloc(1, sizeof *gs);
        AoeUiPanelRor *gp = &gs->ez.panel;
        aoe_settings_set_flag_8fc_ror(&settings, 0);
        aoe_settings_set_option_ae5_ror(&settings, 0);
        aoe_settings_set_map_size_ror(&settings, 3);
        aoe_settings_set_a8c_a90_ror(&settings, 7, 11000);
        aoe_settings_set_starting_age_ror(&settings, 2);
        assert(aoe_game_settings_screen_init_ror(gs, &ui, &area));
        assert(gs->offset == -1 && aoe_ui_text_count_ror(&gs->list->text) == 1);
        assert(aoe_ui_text_selected_id_ror(&gs->list->text) == -2 && gs->random_map);
        assert(aoe_ui_dropdown_selected_id_ror(gs->size) == 3);
        assert(aoe_ui_dropdown_selected_id_ror(gs->victory) == 7);
        assert(aoe_ui_dropdown_selected_id_ror(gs->years) == 11000);
        assert(gs->years->panel.active && !gs->score->panel.active && gs->size->panel.active);
        assert(!gs->population && !gs->checks[3]);
        /* Score victory, then OK stores it. */
        aoe_ui_dropdown_select_ror(gs->victory, (int16_t)aoe_ui_dropdown_index_of_id_ror(gs->victory, 8));
        gp->vt->action(gp, &gs->victory->panel, 0, 0, 0);
        assert(aoe_ui_dropdown_selected_id_ror(gs->score) == 900 && gs->score->panel.active);
        aoe_ui_dropdown_select_ror(gs->age, 4);
        aoe_ui_button_set_state_ror(gs->checks[0], 1);
        gp->vt->action(gp, &gs->ok->panel, 1, 0, 0);
        assert(aoe_settings_int_a8c_ror(&settings) == 8 && aoe_settings_int_a90_ror(&settings) == 900);
        assert(aoe_settings_starting_age_ror(&settings) == 4 && aoe_settings_option_ad8_ror(&settings) == 0);
        assert(aoe_settings_map_size_ror(&settings) == 3);
        aoe_ui_system_close_ror(&sys, "Game Settings Screen");
    }
    /* The editor screen's small methods. */
    {
        AoeScenarioEditorRor *edp = calloc(1, sizeof *edp);
        AoeScenarioMenuDialogRor *md = calloc(1, sizeof *md);
#define ed (*edp)
        ed.ui = &ui;
        aoe_ui_ez_init_named_ror(&ed.ez, &sys, &ez_ops, "Scenario Editor Screen");
        assert(aoe_ui_ez_setup_ror(&ed.ez, &area, NULL, NULL, -1, 1, 0, 0, 0, 0, 0));
        loaded[0] = 'x';
        aoe_scenario_editor_new_ror(&ed);
        assert(loaded[0] == 0);
        ed.modified = 1;
        assert(aoe_scenario_editor_modified_ror(&ed) == 1);
        aoe_scenario_editor_new_ror(&ed);
        assert(aoe_ui_system_find_ror(&sys, "NewSaveDialog"));
        aoe_ui_system_close_ror(&sys, "NewSaveDialog");
        assert(aoe_scenario_editor_menu_ror(&ed, md));
        assert(aoe_ui_system_find_ror(&sys, "Scenario Menu Dialog"));
        aoe_ui_system_close_ror(&sys, "Scenario Menu Dialog");
        aoe_scenario_editor_idle_ror(&ed);
        aoe_ui_system_close_ror(&sys, "Scenario Editor Screen");
#undef ed
    }
    aoe_ui_system_close_ror(&sys, "Game Screen");
    free(sys.panels);
    return 0;
}
