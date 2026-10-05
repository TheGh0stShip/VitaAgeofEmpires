#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define MENU(p) ((AoeMainMenuRor *)(p))
#define ERR(p) ((AoeErrorScreenRor *)(p))

/* 0x00419950: destroy and free a panel, then clear the pointer. */
static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}

#define DROP(x) drop((AoeUiPanelRor **)&(x))

static void menu_destroy(AoeUiPanelRor *p) {
    AoeMainMenuRor *m = MENU(p);
    int i;
    DROP(m->field_478);
    DROP(m->field_47c);
    DROP(m->copyright);
    DROP(m->year);
    DROP(m->warning);
    DROP(m->field_4b0);
    DROP(m->field_4b4);
    for (i = 0; i < 7; ++i) DROP(m->buttons[i]);
    DROP(m->circle);
    DROP(m->title);
    aoe_ui_ez_release_ror(&m->ez);
}

/* Move a panel right by `dx` (its vtable +0x1c with the parent-relative
   position). */
static void shift(AoeUiPanelRor *child, AoeUiPanelRor *parent, int32_t dx) {
    child->vt->set_fixed_position(child, child->x - parent->x + dx,
                                  child->y - parent->y, child->width,
                                  child->height);
}

int aoe_main_menu_init_ror(AoeMainMenuRor *m, AoeGameUiRor *ui,
                           AoeUiAreaRor *area,
                           const AoeUiPictureOpsRor *picture_ops) {
    static const int32_t texts[5] = {0x23f2, 0x23f3, 0x23f5, 0x23f6, 0x23f7};
    AoeUiEzRor *ez = &m->ez;
    AoeUiPanelRor *tab[5];
    char line[0x200];
    int32_t h, i, y;
    memset(m, 0, sizeof *m);
    m->ui = ui;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, "Main Menu");
    ez->panel.vt = &aoe_main_menu_vtable_ror.base;
    if (!aoe_ui_screen_setup_ror(ez, area, "scr1", 0xc383, 1)) {
        ez->panel.field_d8 = 1; /* setup failed */
        return 0;
    }
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    for (i = 0, y = 0xb2; y < 0x1ac; ++i, y += 0x32)
        if (!aoe_ui_ez_create_button_ror(ez, &ez->panel, &m->buttons[i], "", NULL,
                                         0xaa, y, 300, 0x28, 0, 0, 0))
            return 1;
    for (i = 0; i < 5; ++i) aoe_ui_button_set_text_id_ror(m->buttons[i], 0, texts[i]);
    aoe_ui_ez_set_reference_slot_ror(ez, ez->panel.width, ez->panel.height);
    h = ez->panel.height;
    aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &m->title, 0x2419, 5, h - 0x3c,
                                 ez->panel.width - 10, 0x12, 10, 1, 0, 0);
    aoe_ui_string_ror(ui->system, 0x2424, line, sizeof line);
    if (line[0] == '\0') {
        AoeUiRectRor r;
        aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &m->copyright, 0x241b, 5,
                                     h - 0x2a, 0x32, 0x12, 10, 0, 0, 0);
        if (m->copyright) {
            aoe_ui_text_line_rect_ror(m->copyright, 0, 0, &r, NULL);
            m->circle = calloc(1, sizeof *m->circle);
            if (m->circle) {
                aoe_ui_picture_init_ror(m->circle, ui->system, picture_ops);
                aoe_ui_picture_setup_ror(m->circle, area, &ez->panel,
                                         r.right - ez->panel.x + 3, r.top + 1,
                                         0xf, 0xe, "circlep2", 0xc4e6, 0, 1);
                aoe_ui_ez_create_text_id_ror(
                    ez, &ez->panel, &m->year, 0x241c,
                    m->circle->panel.x - ez->panel.x + m->circle->panel.width,
                    r.top, 500, 0x12, 10, 0, 0, 0);
                if (m->year) {
                    int32_t dx;
                    aoe_ui_text_line_rect_ror(m->year, 0, 0, &r, NULL);
                    dx = (ez->panel.width - r.right - 5) / 2;
                    if (dx > 0) {
                        shift(&m->copyright->panel, &ez->panel, dx);
                        shift(&m->circle->panel, &ez->panel, dx);
                        shift(&m->year->panel, &ez->panel, dx);
                    }
                }
            }
        }
    } else {
        char joined[0x600];
        snprintf(joined, sizeof joined, "%s%s %s",
                 aoe_ui_string_static_ror(ui->system, 0x241b), line,
                 aoe_ui_string_static_ror(ui->system, 0x241c));
        aoe_ui_ez_create_text_ror(ez, &ez->panel, &m->copyright, joined, 5,
                                  h - 0x2a, ez->panel.width - 10, 0x12, 10, 1, 0,
                                  0);
    }
    aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &m->warning, 0x2425, 5, h - 0x18,
                                 ez->panel.width - 10, 0x12, 10, 1, 0, 0);
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    for (i = 0; i < 5; ++i) tab[i] = &m->buttons[i]->panel;
    aoe_ui_panels_tab_cycle_ror(tab, 5);
    aoe_ui_panel_set_current_ror(&ez->panel, &m->buttons[0]->panel);
    return 1;
}

static int32_t menu_key(AoeUiPanelRor *p, int32_t key, int32_t a, int32_t b,
                        int32_t down, int32_t e) {
    AoeMainMenuRor *m = MENU(p);
    const AoeGameUiOpsRor *ops = m->ui->ops;
    (void)a, (void)b, (void)e;
    if (!down) return 0;
    if (key == 'C') {
        if (ops && ops->open_credits) ops->open_credits(ops->context);
        aoe_ui_system_show_named_ror(m->ui->system, "Credits Screen", 0);
        aoe_ui_system_close_ror(m->ui->system, "Main Menu");
        return 1;
    }
    if (key == 'L') {
        if (!ops || !ops->play_demo || !ops->play_demo(ops->context))
            aoe_ui_ez_ok_dialog_id_ror(&m->ez, 0x7d7, NULL, 0x1c2, 100);
        return 1;
    }
    return 0;
}

/* The CD, once; 0 after its message when it is missing. */
static int need_cd(AoeMainMenuRor *m, int32_t message) {
    const AoeGameUiOpsRor *ops = m->ui->ops;
    if (m->ui->cd_ok) return 1;
    if (!ops || !ops->cd_check || !ops->cd_check(ops->context)) {
        aoe_ui_ez_ok_dialog_id_ror(&m->ez, message, NULL, 0x1c2, 100);
        return 0;
    }
    m->ui->cd_ok = 1;
    return 1;
}

static void go(AoeMainMenuRor *m, void (*open)(void *), const char *name) {
    const AoeGameUiOpsRor *ops = m->ui->ops;
    if (ops && ops->reset_input) ops->reset_input(ops->context);
    if (ops && open) open(ops->context);
    aoe_ui_system_show_named_ror(m->ui->system, name, 0);
    aoe_ui_system_close_ror(m->ui->system, "Main Menu");
}

static int32_t menu_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code,
                           int32_t value, int32_t extra) {
    AoeMainMenuRor *m = MENU(p);
    const AoeGameUiOpsRor *ops = m->ui->ops;
    if (from && code == 1) {
        if (from == &m->buttons[0]->panel) {
            if (need_cd(m, 0x7d8))
                go(m, ops ? ops->open_single_player_menu : NULL,
                   "Single Player Menu");
            return 1;
        }
        if (from == &m->buttons[1]->panel) {
            go(m, ops ? ops->open_mp_startup : NULL, "MP Startup Screen");
            return 1;
        }
        if (from == &m->buttons[2]->panel) {
            aoe_ui_ez_help_dialog_id_ror(&m->ez, 0x1d4f7, NULL, 0x1c2, 100);
            return 1;
        }
        if (from == &m->buttons[3]->panel) {
            if (need_cd(m, 0x7da))
                go(m, ops ? ops->open_editor_menu : NULL, "Scenario Editor Menu");
            return 1;
        }
        if (from == &m->buttons[4]->panel) {
            if (ops && ops->quit_program) ops->quit_program(ops->context);
            return 1;
        }
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_main_menu_vtable_ror = {
    AOE_UI_EZ_SLOTS(menu_destroy, aoe_ui_game_idle_slot_ror,
                    aoe_ui_panel_handle_paint_ror, menu_key,
                    aoe_ui_panel_zero_ror, menu_action,
                    aoe_ui_screen_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};

/* ---- the error screen ---- */

static void error_destroy(AoeUiPanelRor *p) {
    AoeErrorScreenRor *e = ERR(p);
    DROP(e->message);
    DROP(e->ok);
    DROP(e->close);
    aoe_ui_ez_release_ror(&e->ez);
}

int aoe_error_screen_init_ror(AoeErrorScreenRor *e, AoeGameUiRor *ui,
                              AoeUiAreaRor *area) {
    AoeUiEzRor *ez = &e->ez;
    memset(e, 0, sizeof *e);
    e->ui = ui;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, "Main Error Screen");
    ez->panel.vt = &aoe_error_screen_vtable_ror.base;
    if (!aoe_ui_screen_setup_ror(ez, area, "scr1", 0xc383, 0)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    aoe_ui_ez_create_text_ror(ez, &ez->panel, &e->message, "", 0x28, 0x28,
                              0x230, 400, 4, 1, 1, 1);
    aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &e->ok, 0xfa1, 0, 200, 0x1b8,
                                   0xf0, 0x1e, 0, 0, 0);
    if (aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &e->close, 0x3ea, 0, 0,
                                       0, 0, 0, -1, -1, 0)) {
        e->close->panel.vt->set_active(&e->close->panel, 1);
        e->close->panel.vt->set_position_mode(&e->close->panel, 9, 4, 4, 4, 4,
                                              0x11, 0x11, 0x11, 0x11, NULL,
                                              NULL, NULL, NULL);
    }
    return 1;
}

void aoe_error_screen_set_message_ror(AoeErrorScreenRor *e, int32_t string_id) {
    if (e->message)
        ((const AoeUiTextVtblRor *)e->message->panel.vt)->set_text_id(e->message,
                                                                      string_id);
}

static int32_t error_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code,
                            int32_t value, int32_t extra) {
    AoeErrorScreenRor *e = ERR(p);
    const AoeGameUiOpsRor *ops = e->ui->ops;
    if (from && code == 1 &&
        ((e->ok && from == &e->ok->panel) || (e->close && from == &e->close->panel))) {
        if (ops && ops->quit_program) ops->quit_program(ops->context);
        return 1;
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_error_screen_vtable_ror = {
    AOE_UI_EZ_SLOTS(error_destroy, aoe_ui_game_idle_slot_ror,
                    aoe_ui_panel_handle_paint_ror, aoe_ui_ez_no_key_action_ror,
                    aoe_ui_panel_zero_ror, error_action,
                    aoe_ui_screen_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
