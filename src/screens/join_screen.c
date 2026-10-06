#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define JS(p) ((AoeJoinScreenRor *)(p))

static const char *const NAME = "Join Screen";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

/* 0x0047b780 (deleting 0x0047b760). */
static void js_destroy(AoeUiPanelRor *p) {
    AoeJoinScreenRor *j = JS(p);
    const AoeGameUiOpsRor *ops = j->ui->ops;
    if (j->sessions_made && ops && ops->mp_release_sessions)
        ops->mp_release_sessions(ops->context);
    j->sessions_made = 0;
    DROP(j->title);
    DROP(j->label);
    DROP(j->list);
    DROP(j->list_bar);
    DROP(j->close);
    DROP(j->join);
    DROP(j->create);
    DROP(j->cancel);
    DROP(j->refresh);
    aoe_ui_ez_release_ror(&j->ez);
}

/* 0x0047b440 */
int aoe_join_screen_init_ror(AoeJoinScreenRor *j, AoeGameUiRor *ui, AoeUiAreaRor *area) {
    const AoeGameUiOpsRor *ops = ui->ops;
    AoeUiEzRor *ez = &j->ez;
    AoeUiPanelRor *tab[5];
    int32_t i;
    memset(j, 0, sizeof *j);
    j->ui = ui;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_join_screen_vtable_ror.base;
    if (ops && ops->set_slot_state) {
        ops->set_slot_state(ops->context, 1, 2);
        for (i = 2; i < 10; ++i) ops->set_slot_state(ops->context, i, 0);
    }
    if (!aoe_ui_screen_setup_ror(ez, area, "scr3", 0xc385, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_shadow_area_ror(ez, 0);
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &j->title, 0x259f, 0x14, 0x14, 600, 0x1e, 1,
                                      1, 0, 0))
        return 1;
    aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &j->label, 0x25a8, 0x14, 0x41, 600, 0x14, 4, 0, 0,
                                 0);
    if (!aoe_ui_ez_create_list_ror(ez, &ez->panel, &j->list, 0x14, 0x5a, 600, 0x10e, 0xb) ||
        !aoe_ui_ez_create_text_scrollbar_ror(ez, &j->list_bar, &j->list->text, 0x14))
        return 1;
    j->listed = 1;
    aoe_ui_text_set_text_id_ror(&j->list->text, 0x25ab);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &j->refresh, 0x25a1, 0, 0xd2, 0x172, 0xdc,
                                        0x1e, 0, 0, 0) ||
        !aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &j->join, 0x25a0, 0, 10, 0x1b8, 200, 0x1e,
                                        0, 0, 0) ||
        !aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &j->create, 0x25a2, 0, 0xdc, 0x1b8, 200,
                                        0x1e, 0, 0, 0) ||
        !aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &j->cancel, 0xfa2, 0, 0x1ae, 0x1b8, 200,
                                        0x1e, 0, 0, 0))
        return 1;
    j->cancel->hotkey = 0x1b;
    j->cancel->hotkey_shift = 0;
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &j->close, 0x3ea, 0, 0, 0, 0, 0, -1, -1,
                                        0))
        return 1;
    j->close->panel.vt->set_active(&j->close->panel, 1);
    j->close->panel.vt->set_position_mode(&j->close->panel, 9, 4, 4, 4, 4, 0x11, 0x11, 0x11, 0x11,
                                          NULL, NULL, NULL, NULL);
    aoe_ui_panel_set_current_ror(&ez->panel, &j->refresh->panel);
    tab[0] = &j->list->text.panel;
    tab[1] = &j->refresh->panel;
    tab[2] = &j->join->panel;
    tab[3] = &j->create->panel;
    tab[4] = &j->cancel->panel;
    aoe_ui_panels_tab_cycle_ror(tab, 5);
    return 1;
}

/* 0x0047ba60 */
void aoe_join_screen_refresh_ror(AoeJoinScreenRor *j) {
    const AoeGameUiOpsRor *ops = j->ui->ops;
    AoeUiTextRor *t = &j->list->text;
    int32_t n, i;
    if (ops && ops->wait_cursor) ops->wait_cursor(ops->context);
    j->listed = 0;
    j->count = 0;
    aoe_ui_text_set_text_id_ror(t, 0x25a9);
    n = ops && ops->mp_enum_sessions ? ops->mp_enum_sessions(ops->context, j->sessions_made) : -1;
    if (n < 0) {
        /* The enumerator could not be made: tried again next time. */
        aoe_ui_text_set_text_id_ror(t, 0x25ab);
        return;
    }
    j->sessions_made = 1;
    j->count = n;
    if (n < 1) {
        aoe_ui_text_set_text_id_ror(t, 0x25ab);
    } else {
        aoe_ui_text_clear_ror(t);
        for (i = 0; i < n; ++i) {
            const char *name = "";
            int32_t a = 0, b = 0;
            char line[0x80];
            if (ops->mp_session_info) ops->mp_session_info(ops->context, i, &name, &a, &b);
            snprintf(line, sizeof line, "%s ( %.1d / %.1d )", name ? name : "", (int)a, (int)b);
            aoe_ui_text_append_ror(t, line, 0);
        }
        aoe_ui_panel_set_current_ror(&j->ez.panel, &t->panel);
    }
    j->listed = 1;
}

static void dropped(AoeJoinScreenRor *j) {
    const AoeGameUiOpsRor *ops = j->ui->ops;
    if (ops && ops->mp_drop) ops->mp_drop(ops->context, 5);
    j->joining = 0;
    if (ops && ops->arrow_cursor) ops->arrow_cursor(ops->context);
    aoe_ui_ez_ok_dialog_id_ror(&j->ez, 0x25a3, NULL, 0x1c2, 100);
}

/* 0x0047b870 (+0x4c) */
static int32_t js_idle(AoeUiPanelRor *p) {
    AoeJoinScreenRor *j = JS(p);
    const AoeGameUiOpsRor *ops = j->ui->ops;
    if (!j->joining) {
        if (j->listed && ops && ops->arrow_cursor) ops->arrow_cursor(ops->context);
    } else if (ops && ops->now_ms && ops->now_ms(ops->context) - j->join_ms > 10000) {
        dropped(j);
        return 1;
    }
    return aoe_ui_panel_handle_idle_ror(p);
}

/* 0x0047b900 (+0x64) */
static int32_t js_user(AoeUiPanelRor *p, uint32_t a, uint32_t b) {
    AoeJoinScreenRor *j = JS(p);
    const AoeGameUiOpsRor *ops = j->ui->ops;
    switch (a) {
    case 0x17a8:
    case 0x17a9:
    case 0x17aa:
    case 0x17b2:
        if (j->joining) {
            dropped(j);
            return 1;
        }
        break;
    case 0x17b8:
        if (j->joining) {
            if (ops && ops->open_mp_setup) ops->open_mp_setup(ops->context);
            aoe_ui_system_show_named_ror(j->ui->system, "MP Setup Screen", 0);
            aoe_ui_system_close_ror(j->ui->system, NAME);
            return 1;
        }
        break;
    }
    return aoe_ui_panel_ignore_ror(p, a, b);
}

/* 0x0047bc20 (+0xb8) */
static int32_t js_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                         int32_t extra) {
    AoeJoinScreenRor *j = JS(p);
    const AoeGameUiOpsRor *ops = j->ui->ops;
    AoeUiSystemRor *sys = j->ui->system;
    if (!from) return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
    if (from == &j->list->text.panel) {
        if (code == 3 && j->count > 0) p->vt->action(p, &j->join->panel, 1, 0, 0);
        return 1;
    }
    if (from == &j->join->panel) {
        if (code != 1) return 1;
        if (j->count > 0) {
            AoeUiTextRor *t = &j->list->text;
            if (!(ops && ops->mp_join_session &&
                  ops->mp_join_session(ops->context, aoe_ui_text_selection_ror(t)))) {
                char fmt[0x100], line[0x100];
                const char *sel = aoe_ui_text_selected_text_ror(t);
                aoe_ui_string_ror(sys, 0x25a3, fmt, sizeof fmt);
                /* The string is the format for the session line. */
                snprintf(line, sizeof line, fmt, sel ? sel : "");
                aoe_ui_ez_ok_dialog_ror(&j->ez, line, NULL, 0x1c2, 100);
                return 1;
            }
            if (ops->wait_cursor) ops->wait_cursor(ops->context);
            j->joining = 1;
            j->join_ms = ops->now_ms ? ops->now_ms(ops->context) : 0;
            return 1;
        }
        aoe_ui_ez_ok_dialog_id_ror(&j->ez, 0x25a4, NULL, 0x1c2, 100);
        return 1;
    }
    if (from == &j->cancel->panel) {
        if (code != 1) return 1;
        if (ops && ops->wait_cursor) ops->wait_cursor(ops->context);
        if (ops && ops->mp_drop) ops->mp_drop(ops->context, 2);
        if (ops && ops->open_mp_startup) ops->open_mp_startup(ops->context);
        aoe_ui_system_show_named_ror(sys, "MP Startup Screen", 0);
        aoe_ui_system_close_ror(sys, NAME);
        return 1;
    }
    if (from == &j->create->panel && code == 1) {
        AoeCreateDialogRor *d = calloc(1, sizeof *d);
        /* The dialog registers itself; one that cannot be set up is
           closed through the system again. */
        if (d && !aoe_create_dialog_init_ror(d, j->ui, p)) {
            if (aoe_ui_system_find_ror(sys, "Create Dialog"))
                aoe_ui_system_close_ror(sys, "Create Dialog");
            else
                free(d);
        }
        return 1;
    }
    if (from == &j->refresh->panel) {
        if (code == 1) aoe_join_screen_refresh_ror(j);
        return 1;
    }
    if (from == &j->close->panel && code == 1) {
        if (ops && ops->quit_program) ops->quit_program(ops->context);
        return 1;
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_join_screen_vtable_ror = {
    AOE_UI_EZ_SLOTS_MDU(js_destroy, aoe_ui_ez_draw_slot_ror, js_idle,
                        aoe_ui_panel_handle_paint_ror, aoe_ui_ez_no_key_action_ror,
                        aoe_ui_panel_zero_ror, js_action, aoe_ui_screen_focus_slot_ror,
                        aoe_ui_ez_mouse_down_slot_ror, js_user),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
