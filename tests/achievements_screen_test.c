/* Patched RoR 1.0a "Achievements Screen" 0x00477340..0x004792c0. */
#include "achievements_screen.h"
#include "player_history.h"

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

static AoeOutcomeManager managers[3];
static float attrs[3][64];
static int networked_flag = 1, viewed_won = 0, iconic_flag = 0;
static int32_t state = 5;
static const char *message = "Hello world";
static int resumed, backed, restarted, quit_calls, locks, draws, shape_freed;
static int32_t draw_x[8], draw_y[8];
static char loaded[32];

static int16_t player_count(void *c) { (void)c; return 3; }
static const char *player_name(void *c, int32_t p) { (void)c; return p == 1 ? "Alpha" : "Beta"; }
static int16_t player_color(void *c, int32_t p) { (void)c; return (int16_t)(p == 1 ? 0 : 3); }
static int player_victorious(void *c, int32_t p) { (void)c; return p == 2; }
static int viewed_victorious(void *c) { (void)c; return viewed_won; }
static const AoeOutcomeManager *outcome(void *c, int32_t p) { (void)c; return &managers[p]; }
static const float *attributes(void *c, int32_t p) { (void)c; return attrs[p]; }
static uint32_t game_time_ms(void *c) { (void)c; return 3723000; } /* 01:02:03 */
static const char *scenario_message(void *c) { (void)c; return message; }
static int networked(void *c) { (void)c; return networked_flag; }
static int paused(void *c) { (void)c; return 1; }
static int flag_a00(void *c) { (void)c; return 0; }
static void resume(void *c) { (void)c; ++resumed; }
static void back_to_menu(void *c) { (void)c; ++backed; }
static void restart(void *c) { (void)c; ++restarted; }
static void quit(void *c) { (void)c; ++quit_calls; }
static int32_t game_state(void *c) { (void)c; return state; }
static int iconic(void *c) { (void)c; return iconic_flag; }
static void *shape_load(void *c, const char *name, int32_t id) {
    (void)c; snprintf(loaded, sizeof loaded, "%s/%d", name, (int)id); return (void *)loaded;
}
static void shape_free(void *c, void *s) { (void)c; (void)s; ++shape_freed; }
static int dc_lock(void *c, const char *who) { (void)c; ++locks; return strcmp(who, "scr_ach::draw") == 0; }
static void dc_unlock(void *c, const char *who) { (void)c; (void)who; }
static void shape_draw(void *c, void *s, int32_t x, int32_t y, int32_t frame) {
    (void)c; (void)s; assert(frame == 0);
    if (draws < 8) { draw_x[draws] = x; draw_y[draws] = y; }
    ++draws;
}
/* The timeline's callbacks: only what its constructor and layout touch. */
static AoePlayerHistoryRor hist[3];
static int tl_setup(void *c, int32_t x, int32_t y, int32_t w, int32_t h) { (void)c; (void)x; (void)y; (void)w; (void)h; return 1; }
static int tl_begin(void *c) { (void)c; return 1; }
static void tl_end(void *c) { (void)c; }
static int tl_has(void *c) { (void)c; return 1; }
static int tl_84(void *c) { (void)c; return 0; }
static void tl_bg(void *c, int f) { (void)c; (void)f; }
static int tl_lock(void *c, const char *w) { (void)c; (void)w; return 1; }
static void tl_unlock(void *c, const char *w) { (void)c; (void)w; }
static void tl_box(void *c, int32_t a, int32_t b, int32_t d, int32_t e, uint8_t col) { (void)c; (void)a; (void)b; (void)d; (void)e; (void)col; }
static void tl_rings(void *c, int32_t a, int32_t b, int32_t d, int32_t e, int n, const uint8_t *cols) { (void)c; (void)a; (void)b; (void)d; (void)e; (void)n; (void)cols; }
static void tl_vline(void *c, int32_t x, int32_t y, int32_t len, uint8_t col) { (void)c; (void)x; (void)y; (void)len; (void)col; }
static uint8_t tl_pal(void *c, uint32_t rgb) { (void)c; return (uint8_t)(rgb & 0xff); }
static void tl_text(void *c, int f, int32_t x, int32_t y, const char *s, int32_t n, uint32_t rgb) { (void)c; (void)f; (void)x; (void)y; (void)s; (void)n; (void)rgb; }
static int tl_string(void *c, int32_t id, char *o, int32_t n) { (void)c; snprintf(o, (size_t)n, "S%d", (int)id); return 1; }
static void tl_format(void *c, int32_t k, int32_t a, int32_t b, char *o, int32_t n) { (void)c; (void)k; (void)b; snprintf(o, (size_t)n, "civ%d", (int)a); }
static int tl_shape_obj;
static void *tl_sload(void *c, const char *n) { (void)c; (void)n; return &tl_shape_obj; }
static void tl_sfree(void *c, void *s) { (void)c; (void)s; }
static int32_t tl_sframes(void *c, void *s) { (void)c; (void)s; return 5; }
static void tl_ssize(void *c, void *s, int32_t f, int16_t *w, int16_t *h) { (void)c; (void)s; (void)f; *w = 10; *h = 8; }
static void tl_sbox(void *c, void *s, int16_t *l, int16_t *t, int16_t *r, int16_t *b) { (void)c; (void)s; *l = 0; *t = 0; *r = 15; *b = 11; }
static void tl_sdraw(void *c, void *s, int32_t x, int32_t y, int32_t f) { (void)c; (void)s; (void)x; (void)y; (void)f; }
static void tl_fclose(void *c, uintptr_t *s) { (void)c; (void)s; }
static const AoePlayerHistoryRor *tl_hist(void *c, int32_t p) { (void)c; return &hist[p]; }
static uint8_t tl_civ(void *c, int32_t p) { (void)c; return (uint8_t)p; }
static const AoeTimelineOpsRor tl_ops = {
    NULL, tl_setup, tl_begin, tl_end, tl_has, tl_84, tl_bg, tl_lock, tl_unlock, tl_lock, tl_unlock,
    tl_box, tl_box, tl_rings, tl_vline, tl_pal, tl_text, tl_string, tl_format, tl_sload, tl_sfree,
    tl_sframes, tl_ssize, tl_sbox, tl_sdraw, tl_fclose, player_count, tl_hist, player_color, tl_civ,
    player_name};

static int font(void *c, int32_t index, AoeTimelineFontRor *out) {
    (void)c; out->handle = (uintptr_t)index; out->char_w = 8; out->char_h = 12; return 1;
}

static AoeAchievementsScreenRor *make(AoeGameUiRor *ui, AoeUiAreaRor *area, int32_t mode,
                                      const AoeAchievementsScreenOpsRor *ops) {
    AoeAchievementsScreenRor *s = calloc(1, sizeof *s);
    assert(aoe_achievements_screen_init_ror(s, ui, area, mode, ops));
    return s;
}

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 640, 480};
    AoeUiEzOpsRor ez_ops = {0};
    AoeGameUiOpsRor ui_ops = {0};
    static AoeGameUiRor ui;
    AoeAchievementsScreenOpsRor ops = {0};
    AoeAchievementsScreenRor *s;
    AoeUiPanelRor *p;

    pf.text_width = width;
    pf.string = string;
    sys.platform = &pf;
    ez_ops.font = fonts;
    ui.system = &sys;
    ui.ez_ops = &ez_ops;
    ui.ops = &ui_ops;
    ops.player_count = player_count;
    ops.player_name = player_name;
    ops.player_color = player_color;
    ops.player_victorious = player_victorious;
    ops.viewed_victorious = viewed_victorious;
    ops.outcome = outcome;
    ops.attributes = attributes;
    ops.game_time_ms = game_time_ms;
    ops.scenario_message = scenario_message;
    ops.networked = networked;
    ops.paused_by_settings = paused;
    ops.game_flag_a00 = flag_a00;
    ops.resume_game = resume;
    ops.back_to_menu = back_to_menu;
    ops.restart = restart;
    ops.quit = quit;
    ops.game_state = game_state;
    ops.iconic = iconic;
    ops.shape_load = shape_load;
    ops.shape_free = shape_free;
    ops.dc_lock = dc_lock;
    ops.dc_unlock = dc_unlock;
    ops.shape_draw = shape_draw;
    ops.timeline_ops = &tl_ops;
    ops.font = font;
    aoe_history_init_ror(&hist[1], NULL, 0.0f);
    aoe_history_init_ror(&hist[2], NULL, 0.0f);
    managers[1].score_sum_1c = 42;
    managers[2].score_sum_1c = 7;

    /* The colour table. */
    assert(aoe_achievements_row_color_ror(0) == 0xc40000 && aoe_achievements_row_color_ror(3) == 0x215a8c &&
           aoe_achievements_row_color_ror(7) == 0x808040 && aoe_achievements_row_color_ror(8) == 0xffffff &&
           aoe_achievements_row_color_ror(-1) == 0xffffff);

    /* A networked game with a scenario message, viewed player defeated:
       "scr8", the four-button row, the message page first. */
    s = make(&ui, &area, 1, &ops);
    p = &s->ez.panel;
    assert(strcmp(s->ez.info_name, "scr8") == 0 && s->ez.info_id == 0xc38a);
    assert(strcmp(loaded, "defcheck.slp/52002") == 0 && s->check_shape);
    assert(strcmp(s->clock->lines->text, "01:02:03") == 0);
    assert(strcmp(s->title->lines->text, "S9886") == 0);
    assert(s->message && strcmp(s->message->lines->text, "Hello world") == 0 && s->message_bar);
    assert(s->timeline && s->scenario_btn && s->timeline_btn && s->restart_btn && s->close_btn);
    assert(s->scenario_btn->panel.x == 5 && s->timeline_btn->panel.x == 0xa5 &&
           s->restart_btn->panel.x == 0x145 && s->close_btn->panel.x == 0x1e5 &&
           s->close_btn->panel.width == 0x96 && s->close_btn->panel.y == 0x1b9);
    assert(s->close_btn->hotkey == 0x1b && s->back->hotkey == 0x1b && s->back2->hotkey == 0x1b);
    assert(strcmp(s->name[0]->lines->text, "Alpha") == 0 && strcmp(s->name[1]->lines->text, "Beta") == 0 &&
           s->name[0]->panel.y == 0xab && s->name[1]->panel.y == 0xab + 0x1e &&
           s->cell[1][6]->panel.x == 0x1ee);
    assert(s->page == 5 && s->columns == 0 && s->message->panel.active &&
           !s->clock->panel.active && s->back2->panel.active && p->current == &s->back2->panel &&
           strcmp(s->page_title->lines->text, "S9906") == 0 && !s->name[0]->panel.active);

    /* Page 0: 7 columns, headings, the rows (score 42 / 7), footer and
       page buttons shown, Back hidden, focus on Close. */
    aoe_achievements_screen_set_page_ror(s, 0);
    assert(s->page == 0 && s->columns == 7 && strcmp(s->page_title->lines->text, "S9945") == 0 &&
           strcmp(s->heading[0]->lines->text, "S9900") == 0 &&
           strcmp(s->heading[6]->lines->text, "S9905") == 0 && s->heading[6]->panel.active);
    assert(strcmp(s->cell[0][6]->lines->text, "42") == 0 && strcmp(s->cell[1][6]->lines->text, "7") == 0 &&
           strcmp(s->cell[0][0]->lines->text, "0") == 0 && strcmp(s->cell[0][4]->lines->text, "") == 0);
    assert(s->name[0]->panel.active && s->name[1]->panel.active && !s->name[2]->panel.active &&
           s->cell[1][6]->panel.active); /* rows past the players are never hidden */
    assert(s->footer->panel.active && s->close_btn->panel.active && s->restart_btn->panel.active &&
           !s->back->panel.active && !s->message->panel.active && p->current == &s->close_btn->panel);

    /* Page 1 has 6 columns: the seventh heading and cells hide. */
    aoe_achievements_screen_set_page_ror(s, 1);
    assert(s->columns == 6 && !s->heading[6]->panel.active && s->heading[5]->panel.active &&
           !s->cell[0][6]->panel.active && s->back->panel.active && !s->close_btn->panel.active);

    /* Paint on a page below 5 draws the check 20 left of the winner's
       name (player 2, row 1) only. */
    draws = 0;
    p->vt->draw(p);
    assert(locks >= 1 && draws == 1 && draw_x[0] == s->name[1]->panel.x - 0x14 &&
           draw_y[0] == s->name[1]->panel.y);
    aoe_achievements_screen_set_page_ror(s, 6);
    assert(s->timeline_shown && strcmp(s->page_title->lines->text, "S9944") == 0);
    draws = 0;
    p->vt->draw(p);
    assert(draws == 0);

    /* Heading hit boxes on page 0: inside the second box -> page 2;
       inside the fourth but not the fifth -> page 4; elsewhere nothing. */
    aoe_achievements_screen_set_page_ror(s, 0);
    p->vt->mouse_down(p, 1, s->hit[1]->panel.x + 1, s->hit[1]->panel.y + 1, 0, 0);
    assert(s->page == 2);
    aoe_achievements_screen_set_page_ror(s, 0);
    p->vt->mouse_down(p, 1, s->hit[3]->panel.x + 1, s->hit[3]->panel.y + 1, 0, 0);
    assert(s->page == 4);
    aoe_achievements_screen_set_page_ror(s, 0);
    p->vt->mouse_down(p, 1, s->hit[4]->panel.x + 1, s->hit[4]->panel.y + s->hit[4]->panel.height - 1, 0, 0);
    assert(s->page == 0);
    aoe_achievements_screen_set_page_ror(s, 3);
    p->vt->mouse_down(p, 1, s->hit[1]->panel.x + 1, s->hit[1]->panel.y + 1, 0, 0);
    assert(s->page == 3);

    /* Actions: timeline -> page 6 with only Back; Back -> page 0;
       Restart; Close in a game leaves to the menus; the hidden close
       quits. */
    aoe_achievements_screen_set_page_ror(s, 0);
    p->vt->action(p, &s->timeline_btn->panel, 1, 0, 0);
    assert(s->page == 6 && s->back->panel.active && !s->close_btn->panel.active);
    p->vt->action(p, &s->back->panel, 1, 0, 0);
    assert(s->page == 0);
    p->vt->action(p, &s->scenario_btn->panel, 1, 0, 0);
    assert(s->page == 5);
    p->vt->action(p, &s->back2->panel, 1, 0, 0);
    assert(s->page == 0);
    p->vt->action(p, &s->restart_btn->panel, 1, 0, 0);
    assert(restarted == 1);
    p->vt->action(p, &s->hidden_close->panel, 1, 0, 0);
    assert(quit_calls == 1);
    /* WM_CLOSE asks once; "no" clears the request, "yes" quits. */
    assert(!aoe_ui_system_find_ror(&sys, "YesNoDialog"));
    p->vt->handle_message(p, NULL, 0x10, 0, 0);
    assert(ui.quit_requested == 1 && aoe_ui_system_find_ror(&sys, "YesNoDialog"));
    p->vt->action(p, aoe_ui_system_find_ror(&sys, "YesNoDialog"), 1, 0, 0);
    assert(ui.quit_requested == 0 && !aoe_ui_system_find_ror(&sys, "YesNoDialog"));
    state = 2; /* no dialog outside states 4..7, the request still set */
    p->vt->handle_message(p, NULL, 0x10, 0, 0);
    assert(ui.quit_requested == 1 && !aoe_ui_system_find_ror(&sys, "YesNoDialog"));
    ui.quit_requested = 0;
    state = 5;
    p->vt->handle_message(p, NULL, 0x10, 0, 0);
    p->vt->action(p, aoe_ui_system_find_ror(&sys, "YesNoDialog"), 0, 0, 0);
    assert(quit_calls == 2);
    p->vt->action(p, &s->close_btn->panel, 1, 0, 0);
    assert(backed == 1 && resumed == 0 && !aoe_ui_system_find_ror(&sys, "Achievements Screen"));
    assert(shape_freed == 1);

    /* From the menus (mode 0): "scr10", two buttons, no message, Close
       resumes a paused game and returns to the game screen. */
    message = NULL;
    viewed_won = 1;
    s = make(&ui, &area, 0, &ops);
    p = &s->ez.panel;
    assert(strcmp(s->ez.info_name, "scr10") == 0 && s->ez.info_id == 0xc38d &&
           strcmp(loaded, "viccheck.slp/52001") == 0);
    assert(!s->message && !s->scenario_btn && !s->restart_btn && s->timeline_btn->panel.x == 100 &&
           s->close_btn->panel.x == 0x154 && s->close_btn->panel.width == 200 && s->page == 0);
    p->vt->action(p, &s->close_btn->panel, 1, 0, 0);
    assert(resumed == 1 && backed == 1);

    /* Not networked but in a game with a message: still the two-button
       layout, the message page first. */
    message = "Story";
    networked_flag = 0;
    s = make(&ui, &area, 1, &ops);
    assert(strcmp(s->ez.info_name, "scr7") == 0 && !s->scenario_btn && s->message &&
           s->timeline_btn->panel.x == 100 && s->page == 5);
    aoe_ui_system_close_ror(&sys, "Achievements Screen");
    free(sys.panels);
    return 0;
}
