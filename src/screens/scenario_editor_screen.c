#include "game_ui.h"

#include "ui_dialog.h"
#include "draw_prims.h"
#include "draw_system.h"
#include "../ui_ez_slots.h"

uint8_t aoe_scenario_editor_modified_ror(const AoeScenarioEditorRor *e) { return e->modified; }

void aoe_scenario_editor_new_ror(AoeScenarioEditorRor *e) {
    const AoeGameUiOpsRor *ops = e->ui->ops;
    if (e->modified) {
        aoe_ui_ez_yes_no_cancel_dialog_id_ror(&e->ez, 0x2447, "NewSaveDialog", 0x1c2, 100);
        return;
    }
    if (ops && ops->editor_load) ops->editor_load(ops->context, NULL, 0);
}

int aoe_scenario_editor_menu_ror(AoeScenarioEditorRor *e, AoeScenarioMenuDialogRor *dialog) {
    if (!aoe_scenario_menu_dialog_init_ror(dialog, e->ui, &e->ez.panel) ||
        dialog->dialog.ez.panel.field_d8) {
        dialog->dialog.ez.panel.vt->destroy(&dialog->dialog.ez.panel);
        return 0;
    }
    return 1;
}

void aoe_scenario_editor_clear_selection_ror(AoeScenarioEditorRor *e) {
    const AoeGameUiOpsRor *ops = e->ui->ops;
    if (ops && ops->clear_selection) ops->clear_selection(ops->context);
}

void aoe_scenario_editor_set_player_ror(AoeScenarioEditorRor *e, int16_t player, int32_t on) {
    const AoeGameUiOpsRor *ops = e->ui->ops;
    if (ops && ops->set_scenario_player) ops->set_scenario_player(ops->context, player, on);
}

void aoe_scenario_editor_show_774_ror(AoeScenarioEditorRor *e, int32_t on) {
    if (e->panel_774) e->panel_774->vt->set_active(e->panel_774, on);
    if (e->bar) e->bar->vt->set_redraw(e->bar, 1);
}

int32_t aoe_scenario_editor_idle_ror(AoeScenarioEditorRor *e) {
    const AoeGameUiOpsRor *ops = e->ui->ops;
    if (ops && ops->game_state && ops->game_state(ops->context) == 7 && ops->now_ms) {
        uint32_t now = ops->now_ms(ops->context);
        /* As written: the stored time less now against the interval. */
        if (e->view_interval <= e->view_time - now && !aoe_ui_modal_panel_ror(e->ui->system)) {
            if (e->view) e->view->vt->set_redraw(e->view, 1);
            if (e->map_interval <= e->map_time - now) {
                if (e->minimap) e->minimap->vt->set_redraw(e->minimap, 1);
                e->map_time = now;
            }
        }
    }
    return aoe_ui_panel_handle_idle_ror(&e->ez.panel);
}

#include <stdio.h>
#include <string.h>

static const char *const EDITOR = "Scenario Editor Screen";

void aoe_scenario_editor_open_ror(AoeScenarioEditorRor *e) {
    const AoeGameUiOpsRor *ops = e->ui->ops;
    if (e->modified) {
        aoe_ui_ez_yes_no_cancel_dialog_id_ror(&e->ez, 0x2447, "OpenSaveDialog", 0x1c2, 100);
        return;
    }
    if (ops && ops->open_editor_open_screen) ops->open_editor_open_screen(ops->context);
    aoe_ui_system_show_named_ror(e->ui->system, "Scenario Editor Open", 0);
}

void aoe_scenario_editor_quit_ror(AoeScenarioEditorRor *e) {
    const AoeGameUiOpsRor *ops = e->ui->ops;
    if (e->modified) {
        aoe_ui_ez_yes_no_cancel_dialog_id_ror(&e->ez, 0x2447, "QuitSaveDialog", 0x1c2, 100);
        return;
    }
    if (!ops || !ops->to_main_menu || !ops->to_main_menu(ops->context)) {
        if (ops && ops->quit_program) ops->quit_program(ops->context);
        return;
    }
    aoe_ui_system_close_ror(e->ui->system, EDITOR);
}

/* The page commit and the player checks; 0 after a message. */
static int ready(AoeScenarioEditorRor *e, int close_menu) {
    const AoeGameUiOpsRor *ops = e->ui->ops;
    int32_t active = 0, human = 0;
    if (ops && ops->editor_commit) ops->editor_commit(ops->context);
    if (ops && ops->scenario_players) ops->scenario_players(ops->context, &active, &human);
    if (active < 1 || human < 1) {
        if (close_menu) aoe_ui_system_close_ror(e->ui->system, "Scenario Menu Dialog");
        aoe_ui_ez_ok_dialog_id_ror(&e->ez, active < 1 ? 0x2742 : 0x2743, NULL, 0x1c2, 100);
        return 0;
    }
    return 1;
}

static void default_name(AoeScenarioEditorRor *e, char *out, size_t n, int ext) {
    const AoeGameUiOpsRor *ops = e->ui->ops;
    int32_t k = ops && ops->game_file_number ? ops->game_file_number(ops->context) : 0;
    snprintf(out, n, ext ? "default%d.scx" : "default%d", (int)k);
}

void aoe_scenario_editor_save_as_ror(AoeScenarioEditorRor *e, int32_t after) {
    const AoeGameUiOpsRor *ops = e->ui->ops;
    if (!ready(e, 0)) return;
    if (ops && ops->open_editor_save_as) ops->open_editor_save_as(ops->context, after);
    aoe_ui_system_show_named_ror(e->ui->system, "Save Game Screen", 0);
    aoe_ui_system_close_ror(e->ui->system, "Scenario Menu Dialog");
}

void aoe_scenario_editor_save_ror(AoeScenarioEditorRor *e, int close_menu, int32_t after) {
    const AoeGameUiOpsRor *ops = e->ui->ops;
    AoeUiSystemRor *sys = e->ui->system;
    const char *file;
    char name[0x104], def[0x104];
    int ok;
    if (!ready(e, 1)) return;
    file = ops && ops->scenario_file ? ops->scenario_file(ops->context) : NULL;
    if (!file || !*file) {
        aoe_scenario_editor_save_as_ror(e, after);
        return;
    }
    snprintf(name, sizeof name, "%s", file);
    if (ops->show_status) ops->show_status(ops->context, 0x450);
    if (close_menu) aoe_ui_system_close_ror(sys, "Scenario Menu Dialog");
    ok = ops->editor_save_file && ops->editor_save_file(ops->context, name);
    if (ops->close_status) ops->close_status(ops->context);
    if (!ok) {
        aoe_ui_ez_ok_dialog_id_ror(&e->ez, 0x963, NULL, 0x1c2, 100);
        return;
    }
    default_name(e, def, sizeof def, 0);
    if (strcmp(name, def) != 0) e->modified = 0;
    if (after == 1) {
        if (!ops->to_main_menu || !ops->to_main_menu(ops->context)) {
            if (ops->quit_program) ops->quit_program(ops->context);
        } else {
            aoe_ui_system_close_ror(sys, EDITOR);
        }
    } else if (after == 2) {
        if (ops->open_editor_open_screen) ops->open_editor_open_screen(ops->context);
        aoe_ui_system_show_named_ror(sys, "Scenario Editor Open", 0);
    } else if (after == 3) {
        if (ops->editor_load) ops->editor_load(ops->context, NULL, 0);
    }
}

const char *aoe_scenario_editor_file_ror(AoeScenarioEditorRor *e) {
    const AoeGameUiOpsRor *ops = e->ui->ops;
    const char *file = ops && ops->scenario_file ? ops->scenario_file(ops->context) : NULL;
    if (!file || !*file) {
        char def[0x104];
        default_name(e, def, sizeof def, 1);
        if (ops && ops->set_scenario_file) ops->set_scenario_file(ops->context, def);
    }
    return ops && ops->scenario_file ? ops->scenario_file(ops->context) : NULL;
}

void aoe_scenario_editor_save_default_ror(AoeScenarioEditorRor *e) {
    aoe_scenario_editor_file_ror(e);
    aoe_scenario_editor_save_ror(e, 0, 0);
}

const char *aoe_scenario_editor_test_save_ror(AoeScenarioEditorRor *e) {
    const AoeGameUiOpsRor *ops = e->ui->ops;
    if (!ready(e, 0)) return NULL;
    if (ops->show_status) ops->show_status(ops->context, 0x450);
    default_name(e, e->test_file, sizeof e->test_file, 1);
    if (!ops->editor_save_file || !ops->editor_save_file(ops->context, e->test_file)) {
        if (ops->close_status) ops->close_status(ops->context);
        aoe_ui_ez_ok_dialog_id_ror(&e->ez, 0x963, NULL, 0x1c2, 100);
        return NULL;
    }
    default_name(e, e->test_file, sizeof e->test_file, 0);
    return e->test_file;
}

void aoe_scenario_editor_store_techs_ror(AoeScenarioEditorRor *e) {
    const AoeGameUiOpsRor *ops = e->ui->ops;
    int32_t i;
    if (e->current_player == 0) e->current_player = 1;
    for (i = 0; i < 16; ++i)
        if (e->techs[i] && ops && ops->set_scenario_tech)
            ops->set_scenario_tech(ops->context, e->current_player - 1, i,
                                   aoe_ui_button_state_ror(e->techs[i]) == 0);
    if (e->players)
        e->current_player = (int16_t)(aoe_ui_dropdown_selection_ror(e->players) + 1);
}

void aoe_scenario_editor_show_techs_ror(AoeScenarioEditorRor *e) {
    const AoeGameUiOpsRor *ops = e->ui->ops;
    int32_t i;
    for (i = 0; i < 16; ++i)
        if (e->techs[i] && ops && ops->scenario_tech)
            aoe_ui_button_set_state_ror(
                e->techs[i], ops->scenario_tech(ops->context, e->current_player - 1, i) == 0);
}

void aoe_scenario_editor_draw_ror(AoeScenarioEditorRor *e) {
    AoeUiPanelRor *p = &e->ez.panel;
    const AoeUiRectRor *r = &p->rect, *c = &p->clip;
    struct AoeDrawSurfaceRor *s;
    aoe_ui_ez_draw_slot_ror(p);
    if (!e->ez.bevel_on || !e->view || r->left != c->left ||
        r->top != c->top || r->right != c->right || r->bottom != c->bottom)
        return;
    p->vt->begin_draw(p, 0);
    s = p->area ? p->area->surface : NULL;
    if (s && aoe_surface_lock_ror(s, 1)) {
        const AoeUiRectRor *v = &e->view->rect;
        aoe_draw_bevel3_ror(&s->view, 0, 0, p->width - 1, v->top - 1,
                            e->ez.bevel);
        aoe_draw_bevel3_ror(&s->view, 0, v->bottom + 1, p->width - 1,
                            p->height - 1, e->ez.bevel);
        aoe_surface_unlock_ror(s);
    }
    p->vt->end_draw(p);
}
