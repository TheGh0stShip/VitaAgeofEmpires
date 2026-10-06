/* Patched RoR 1.0a "Achievements Screen" (vtable 0x00545e84,
   0x00477340..0x004792c0). */
#include "achievements_screen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define ACH(p) ((AoeAchievementsScreenRor *)(p))

static const char *const NAME = "Achievements Screen";
static const char *const YES_NO = "YesNoDialog";
static const char *const GAME_SCREEN = "Game Screen";
static const char *const DRAW_LOCK = "scr_ach::draw";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

static void show(void *panel, int32_t on) {
    AoeUiPanelRor *p = panel;
    if (p) p->vt->set_active(p, on);
}

static void focus(AoeAchievementsScreenRor *s, void *panel) {
    aoe_ui_panel_set_current_ror(&s->ez.panel, (AoeUiPanelRor *)panel);
}

/* 0x00478000 destructor (deleting 0x00477fe0): the yes/no dialog closed,
   the shape freed, every panel deleted, then the screen's 0x00468170. */
static void ach_destroy(AoeUiPanelRor *p) {
    AoeAchievementsScreenRor *s = ACH(p);
    const AoeAchievementsScreenOpsRor *o = s->ops;
    int i, j;
    aoe_ui_system_close_ror(s->ui->system, YES_NO);
    if (s->check_shape && o && o->shape_free) o->shape_free(o->context, s->check_shape);
    s->check_shape = NULL;
    DROP(s->title);
    DROP(s->page_title);
    DROP(s->clock);
    DROP(s->message);
    DROP(s->footer);
    DROP(s->restart_btn);
    DROP(s->close_btn);
    DROP(s->back);
    DROP(s->back2);
    DROP(s->scenario_btn);
    DROP(s->timeline_btn);
    if (s->timeline) {
        aoe_timeline_release_ror(s->timeline);
        free(s->timeline);
        s->timeline = NULL;
    }
    DROP(s->message_bar);
    DROP(s->extra[0]);
    DROP(s->extra[1]);
    DROP(s->hidden_close);
    for (i = 0; i < 7; ++i) DROP(s->heading[i]);
    for (i = 0; i < 5; ++i) DROP(s->hit[i]);
    for (i = 0; i < 8; ++i) {
        DROP(s->name[i]);
        for (j = 0; j < 7; ++j) DROP(s->cell[i][j]);
    }
    aoe_ui_ez_release_ror(&s->ez);
}

uint32_t aoe_achievements_row_color_ror(int16_t color_index) {
    switch (color_index) {
    case 0: return 0xc40000;
    case 1: return 0xc8;
    case 2: return 0xeaea;
    case 3: return 0x215a8c;
    case 4: return 0x80ff;
    case 5: return 0x8000;
    case 6: return 0x808080;
    case 7: return 0x808040;
    default: return 0xffffff;
    }
}

static int32_t players(const AoeAchievementsScreenRor *s) {
    const AoeAchievementsScreenOpsRor *o = s->ops;
    int32_t n = o && o->player_count ? o->player_count(o->context) - 1 : 0;
    return n > 8 ? 8 : n;
}

static void set_cell(AoeUiTextRor *t, const AoeAchievementsCellRor *c) {
    char text[16];
    if (!t) return;
    if (c->kind == AOE_ACH_CELL_YES) {
        aoe_ui_text_set_text_id_ror(t, 0xfa3);
        return;
    }
    aoe_achievements_cell_text_ror(c, "", text, (int)sizeof text);
    aoe_ui_text_set_text_ror(t, text);
}

void aoe_achievements_screen_set_page_ror(AoeAchievementsScreenRor *s, int32_t page) {
    const AoeAchievementsScreenOpsRor *o = s->ops;
    const AoeGameUiOpsRor *ui_ops = s->ui->ops;
    AoeAchievementsPageRor data;
    int32_t n = players(s), i, j;
    if (ui_ops && ui_ops->wait_cursor) ui_ops->wait_cursor(ui_ops->context);
    show(s->message, 0);
    s->page = page;
    s->timeline_shown = 0;
    show(s->footer, 0);
    show(s->clock, 1);
    show(s->restart_btn, 0);
    show(s->close_btn, 0);
    show(s->scenario_btn, 0);
    show(s->timeline_btn, 0);
    show(s->back, 1);
    show(s->back2, 0);
    focus(s, s->back);
    s->columns = 0;
    if (page >= 0 && page <= 4 && aoe_achievements_page_ror(page, &data)) {
        s->columns = data.columns;
        aoe_ui_text_set_text_id_ror(s->page_title, data.title_id);
        for (j = 0; j < 7; ++j)
            if (data.heading[j]) aoe_ui_text_set_text_id_ror(s->heading[j], data.heading[j]);
        if (page == 0) {
            focus(s, s->close_btn);
            show(s->restart_btn, 1);
            show(s->close_btn, 1);
            show(s->scenario_btn, 1);
            show(s->timeline_btn, 1);
            show(s->back, 0);
            show(s->footer, 1);
        }
        for (i = 0; i < n; ++i) {
            AoeAchievementsCellRor cells[7];
            const AoeOutcomeManager *m = o && o->outcome ? o->outcome(o->context, i + 1) : NULL;
            const float *attr = o && o->attributes ? o->attributes(o->context, i + 1) : NULL;
            if (!aoe_achievements_row_ror(page, m, attr, cells)) continue;
            for (j = 0; j < 7; ++j) set_cell(s->cell[i][j], &cells[j]);
        }
    } else if (page == 5) {
        aoe_ui_text_set_text_id_ror(s->page_title, 0x26b2);
        show(s->message, 1);
        show(s->clock, 0);
        show(s->back, 1);
        show(s->back2, 1);
        focus(s, s->back2);
    } else if (page == 6) {
        s->timeline_shown = 1;
        aoe_ui_text_set_text_id_ror(s->page_title, 0x26d8);
        show(s->clock, 1);
    }
    for (i = 0; i < 8; ++i) show(s->name[i], s->columns != 0 && i < n);
    for (j = 0; j < 7; ++j) {
        show(s->heading[j], j < s->columns);
        for (i = 0; i < n; ++i) show(s->cell[i][j], j < s->columns);
    }
}

/* 0x00478e90 draw (+0x30): the screen's 0x00454f80, then on pages 0..4
   frame 0 of the check shape 20 left of each victorious player's name. */
static void ach_draw(AoeUiPanelRor *p) {
    AoeAchievementsScreenRor *s = ACH(p);
    const AoeAchievementsScreenOpsRor *o = s->ops;
    int32_t n = players(s), i;
    aoe_ui_ez_draw_slot_ror(p);
    if (s->page >= 5) return;
    p->vt->begin_draw(p, 0);
    if (s->check_shape && o && o->dc_lock && o->dc_lock(o->context, DRAW_LOCK)) {
        for (i = 0; i < n; ++i) {
            AoeUiTextRor *t = s->name[i];
            if (t && o->player_victorious && o->player_victorious(o->context, i + 1) &&
                o->shape_draw)
                o->shape_draw(o->context, s->check_shape, t->panel.x - 0x14, t->panel.y, 0);
        }
        if (o->dc_unlock) o->dc_unlock(o->context, DRAW_LOCK);
    }
    p->vt->end_draw(p);
}

/* 0x00478f60 mouse down: the base, then on page 0 a press inside one of
   the heading hit boxes picks page 1, 2, 3, or 4 when inside the fourth
   box but not the fifth. */
static int32_t ach_mouse_down(AoeUiPanelRor *p, uint8_t button, int32_t x, int32_t y,
                              int32_t ctrl, int32_t shift) {
    AoeAchievementsScreenRor *s = ACH(p);
    int32_t r = aoe_ui_ez_mouse_down_slot_ror(p, button, x, y, ctrl, shift);
    int i;
    if (s->page != 0) return r;
    for (i = 0; i < 3; ++i) {
        AoeUiPanelRor *h = s->hit[i] ? &s->hit[i]->panel : NULL;
        if (h && h->vt->is_inside(h, x, y)) {
            aoe_achievements_screen_set_page_ror(s, i + 1);
            return r;
        }
    }
    if (s->hit[3] && s->hit[4] && s->hit[3]->panel.vt->is_inside(&s->hit[3]->panel, x, y) &&
        !s->hit[4]->panel.vt->is_inside(&s->hit[4]->panel, x, y))
        aoe_achievements_screen_set_page_ror(s, 4);
    return r;
}

static void show_page_buttons(AoeAchievementsScreenRor *s, int32_t page) {
    aoe_achievements_screen_set_page_ror(s, page);
    show(s->restart_btn, 0);
    show(s->close_btn, 0);
    show(s->scenario_btn, 0);
    show(s->timeline_btn, 0);
    show(s->back, 1);
}

/* 0x00479050 action (code 1 presses): Close leaves to the game screen
   (resuming a game the settings paused) when in a game, else to the
   menus (0x00500d80), and closes this screen; Restart 0x00500fb0; the
   timeline / scenario buttons show page 6 / 5 with only Back; Back and
   Back2 return to page 0; the hidden close button closes the window. A
   yes/no dialog answer closes it: yes (code 0) closes the window, no
   clears the quit request. Others go to the easy panel's 0x00457e50. */
static int32_t ach_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                          int32_t extra) {
    AoeAchievementsScreenRor *s = ACH(p);
    const AoeAchievementsScreenOpsRor *o = s->ops;
    AoeUiSystemRor *sys = s->ui->system;
#define IS(btn) ((btn) && from == &(btn)->panel)
    if (IS(s->close_btn) && code == 1) {
        if (s->mode == 0) {
            if (o && o->paused_by_settings && o->paused_by_settings(o->context) &&
                o->game_flag_a00 && !o->game_flag_a00(o->context) && o->resume_game)
                o->resume_game(o->context);
            aoe_ui_system_show_named_ror(sys, GAME_SCREEN, 0);
        } else if (o && o->back_to_menu) {
            o->back_to_menu(o->context);
        }
        aoe_ui_system_close_ror(sys, NAME);
        return 1;
    }
    if (IS(s->restart_btn) && code == 1) {
        if (o && o->restart) o->restart(o->context);
        return 1;
    }
    if (IS(s->timeline_btn) && code == 1) {
        show_page_buttons(s, 6);
        return 1;
    }
    if (IS(s->scenario_btn) && code == 1) {
        show_page_buttons(s, 5);
        return 1;
    }
    if ((IS(s->back) || IS(s->back2)) && code == 1) {
        aoe_achievements_screen_set_page_ror(s, 0);
        return 1;
    }
    if (IS(s->hidden_close) && code == 1) {
        if (o && o->quit) o->quit(o->context);
        return 1;
    }
#undef IS
    if (from && from->name && strcmp(from->name, YES_NO) == 0) {
        aoe_ui_system_close_ror(sys, YES_NO);
        if (code == 0) {
            if (o && o->quit) o->quit(o->context);
        } else {
            s->ui->quit_requested = 0;
        }
        return 1;
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

/* 0x004792c0 window messages (+0x48): WM_CLOSE (0x10) once while no
   quit is pending sets the request and, in game states 4..7 with the
   window not iconic, asks with string 3101 (450 x 100); everything else
   goes to the panel's 0x00452fa0. */
static int32_t ach_message(AoeUiPanelRor *p, void *window, uint32_t msg, uint32_t wparam,
                           uint32_t lparam) {
    AoeAchievementsScreenRor *s = ACH(p);
    const AoeAchievementsScreenOpsRor *o = s->ops;
    if (msg == 0x10 && !s->ui->quit_requested) {
        int32_t state = o && o->game_state ? o->game_state(o->context) : 0;
        s->ui->quit_requested = 1;
        if ((state >= 4 && state <= 7) && !(o && o->iconic && o->iconic(o->context))) {
            aoe_ui_ez_yes_no_dialog_id_ror(&s->ez, 0xc1d, YES_NO, 0x1c2, 100);
            return 1;
        }
    }
    return aoe_ui_panel_handle_message_ror(p, window, msg, wparam, lparam);
}

static int text(AoeAchievementsScreenRor *s, AoeUiTextRor **out, int32_t id, const char *str,
                int32_t x, int32_t y, int32_t w, int32_t h, int32_t font, int32_t centred,
                int32_t middle, int32_t wrap) {
    AoeUiEzRor *ez = &s->ez;
    if (str)
        return aoe_ui_ez_create_text_ror(ez, &ez->panel, out, str, x, y, w, h, font, centred,
                                         middle, wrap);
    return aoe_ui_ez_create_text_id_ror(ez, &ez->panel, out, id, x, y, w, h, font, centred,
                                        middle, wrap);
}

static int button(AoeAchievementsScreenRor *s, AoeUiButtonRor **out, int32_t id, int32_t x,
                  int32_t w) {
    AoeUiEzRor *ez = &s->ez;
    return aoe_ui_ez_create_button_id_ror(ez, &ez->panel, out, id, 0, x, 0x1b9, w, 0x1e, 0, 0,
                                          0);
}

static void escape(AoeUiButtonRor *b) {
    if (!b) return;
    b->hotkey = 0x1b;
    b->hotkey_shift = 0;
}

int aoe_achievements_screen_init_ror(AoeAchievementsScreenRor *s, AoeGameUiRor *ui,
                                     AoeUiAreaRor *area, int32_t mode,
                                     const AoeAchievementsScreenOpsRor *ops) {
    static const int32_t head_x[7] = {0x62, 0x98, 0xd4, 0x104, 0x16c, 0x172, 0x1a8};
    static const int32_t head_y[7] = {0x97, 0x83, 0x6f, 0x5b, 0x6f, 0x83, 0x97};
    static const int32_t head_w[7] = {200, 200, 0x91, 200, 0x91, 200, 200};
    static const int32_t hit_x[5] = {100, 0x9a, 0xd6, 0x11a, 0x184};
    static const int32_t hit_y[5] = {0x97, 0x83, 0x6f, 0x5b, 0x6f};
    static const int32_t hit_w[5] = {0x82, 0x80, 0x79, 0x9c, 0x79};
    static const int32_t hit_h[5] = {0xfa, 0x112, 0x121, 0x135, 0x121};
    static const int32_t cell_x[7] = {0xb2, 0xe7, 0x11b, 0x150, 0x185, 0x1ba, 0x1ee};
    AoeUiEzRor *ez = &s->ez;
    const char *info, *message = NULL;
    int32_t info_id, n, i, j, k = 0;
    int victorious, networked;
    char clock[100];
    uint32_t t;
    AoeUiPanelRor *tab[4];
    AoeTimelineFontRor font_a = {0}, font_b = {0};

    memset(s, 0, sizeof *s);
    s->ui = ui;
    s->ops = ops;
    s->mode = mode;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_achievements_screen_vtable_ror()->base;
    victorious = ops && ops->viewed_victorious && ops->viewed_victorious(ops->context);
    if (mode == 0) {
        info = "scr10";
        info_id = 0xc38d;
    } else if (victorious) {
        info = "scr7";
        info_id = 0xc389;
    } else {
        info = "scr8";
        info_id = 0xc38a;
    }
    if (!aoe_ui_screen_setup_ror(ez, area, info, info_id, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);

    /* "%02ld:%02ld:%02ld" of the game time in seconds. */
    t = ops && ops->game_time_ms ? ops->game_time_ms(ops->context) / 1000 : 0;
    snprintf(clock, sizeof clock, "%02ld:%02ld:%02ld", (long)(t / 3600), (long)(t / 60 % 60),
             (long)(t % 60));
    if (!text(s, &s->clock, 0, clock, 0x1e0, 10, 0x6e, 0x14, 0xb, 0, 0, 0)) return 1;
    aoe_ui_text_set_alignment_ror(s->clock, 3, 2);
    if (!text(s, &s->title, 0x269e, NULL, 0, 5, 0x280, 0x1e, 1, 1, 0, 0)) return 1;
    if (!text(s, &s->page_title, 0, NULL, 10, 0x23, 0x26c, 0x1e, 1, 1, 0, 0)) return 1;
    if (!text(s, &s->footer, 0x26db, NULL, 10, 0x19a, 0x26c, 0x14, 0xb, 1, 0, 0)) return 1;
    if (mode != 0) {
        message = ops && ops->scenario_message ? ops->scenario_message(ops->context) : NULL;
        if (message && *message) {
            if (!text(s, &s->message, 0, "", 10, 0x55, 0x26c, 0x14a, 0xb, 0, 0, 1)) return 1;
            aoe_ui_ez_create_text_scrollbar_ror(ez, &s->message_bar, s->message, 0x14);
            aoe_ui_text_set_bevel_ror(s->message, 4, ez->bevel);
            aoe_ui_text_set_text_ror(s->message, message);
        } else {
            message = NULL;
        }
    }
    if (ops && ops->font) {
        ops->font(ops->context, 0xb, &font_a);
        ops->font(ops->context, 9, &font_b);
    }
    s->timeline = aoe_timeline_new_ror(ops ? ops->timeline_ops : NULL, ez->panel.width,
                                       ez->panel.height, 10, 0x55, 0x26c, 0x14a, &font_a,
                                       &font_b, 0x280, 0x1e0);
    if (!s->timeline) {
        ez->panel.field_d8 = 1;
        return 1;
    }
    aoe_timeline_set_border_ror(s->timeline, 3, ez->bevel[0], ez->bevel[1], ez->bevel[2],
                                ez->bevel[3], ez->bevel[4], ez->bevel[5]);
    aoe_timeline_set_icons_ror(s->timeline, "timeline.slp");
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &s->hidden_close, 0x3ea, 0, 0, 0, 0, 0,
                                        -1, -1, 0))
        return 1;
    show(s->hidden_close, 1);
    s->hidden_close->panel.vt->set_position_mode(&s->hidden_close->panel, 9, 4, 4, 4, 4, 0x11,
                                                 0x11, 0x11, 0x11, NULL, NULL, NULL, NULL);
    for (j = 0; j < 7; ++j)
        if (!text(s, &s->heading[j], 0, NULL, head_x[j], head_y[j], head_w[j], 0x1e, 0xb,
                  j == 3, 0, 0))
            return 1;
    for (j = 0; j < 3; ++j) aoe_ui_text_set_alignment_ror(s->heading[j], 3, 2);
    for (j = 0; j < 5; ++j)
        if (!text(s, &s->hit[j], 0, NULL, hit_x[j], hit_y[j], hit_w[j], hit_h[j], 0xb, 0, 0, 0))
            return 1;
    n = players(s);
    for (i = 0; i < 8; ++i) {
        int32_t y = 0xab + 0x1e * i;
        const char *name = i < n && ops && ops->player_name ? ops->player_name(ops->context, i + 1)
                                                            : NULL;
        if (!text(s, &s->name[i], 0, name ? name : "", 0x16, y, 0x96, 0x1d, 0xb, 0, 1, 0))
            return 1;
        for (j = 0; j < 7; ++j)
            if (!text(s, &s->cell[i][j], 0, NULL, cell_x[j], y, 0x33, 0x1d, 0xb, 1, 1, 0))
                return 1;
        if (i < n) {
            uint32_t rgb = aoe_achievements_row_color_ror(
                ops && ops->player_color ? ops->player_color(ops->context, i + 1) : -1);
            aoe_ui_text_set_colors_ror(s->name[i], rgb, 0);
            for (j = 0; j < 7; ++j) aoe_ui_text_set_colors_ror(s->cell[i][j], rgb, 0);
        }
    }
    networked = ops && ops->networked && ops->networked(ops->context);
    if (mode == 0 || !networked) {
        if (!button(s, &s->timeline_btn, 0x26d8, 100, 200)) return 1;
        if (!button(s, &s->close_btn, 0x269c, 0x154, 200)) return 1;
    } else if (!message) {
        if (!button(s, &s->timeline_btn, 0x26d8, 0x14, 0xb4)) return 1;
        if (!button(s, &s->restart_btn, 0x269f, 0xe6, 0xb4)) return 1;
        if (!button(s, &s->close_btn, 0x269c, 0x1b8, 0xb4)) return 1;
    } else {
        if (!button(s, &s->scenario_btn, 0x26b2, 5, 0x96)) return 1;
        if (!button(s, &s->timeline_btn, 0x26d8, 0xa5, 0x96)) return 1;
        if (!button(s, &s->restart_btn, 0x269f, 0x145, 0x96)) return 1;
        if (!button(s, &s->close_btn, 0x269c, 0x1e5, 0x96)) return 1;
    }
    if (!button(s, &s->back, 0x26da, 200, 0xf0)) return 1;
    show(s->back, 0);
    escape(s->back);
    if (!button(s, &s->back2, 0x269c, 200, 0xf0)) return 1;
    show(s->back2, 0);
    escape(s->back2);
    escape(s->close_btn);
    if (ops && ops->shape_load)
        s->check_shape = victorious ? ops->shape_load(ops->context, "viccheck.slp", 0xcb21)
                                    : ops->shape_load(ops->context, "defcheck.slp", 0xcb22);
    if (s->scenario_btn) tab[k++] = &s->scenario_btn->panel;
    if (s->timeline_btn) tab[k++] = &s->timeline_btn->panel;
    if (s->restart_btn) tab[k++] = &s->restart_btn->panel;
    tab[k++] = &s->close_btn->panel;
    aoe_ui_panels_tab_cycle_ror(tab, (int16_t)k);
    focus(s, s->close_btn);
    aoe_achievements_screen_set_page_ror(s, message ? 5 : 0);
    return 1;
}

/* The screen's vtable with the five overridden slots (+0x00, +0x30,
   +0x48, +0x70, +0xb8). */
static AoeUiEzVtblRor vtable;

const AoeUiEzVtblRor *aoe_achievements_screen_vtable_ror(void) {
    if (!vtable.base.destroy) {
        vtable = aoe_ui_screen_vtable_ror;
        vtable.base.destroy = ach_destroy;
        vtable.base.draw = ach_draw;
        vtable.base.handle_message = ach_message;
        vtable.base.mouse_down = ach_mouse_down;
        vtable.base.action = ach_action;
    }
    return &vtable;
}
