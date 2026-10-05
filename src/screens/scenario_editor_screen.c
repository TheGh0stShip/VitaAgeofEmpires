#include "game_ui.h"

#include "ui_dialog.h"

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
