#include "game_ui.h"

#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define SMENU(p) ((AoeScenarioMenuDialogRor *)(p))

static const char *const NAME = "Scenario Menu Dialog";
static const char *const EDITOR = "Scenario Editor Screen";

static void free_button(AoeUiButtonRor **b) {
    if (*b) {
        (*b)->panel.vt->destroy(&(*b)->panel);
        free(*b);
        *b = NULL;
    }
}

static void smenu_destroy(AoeUiPanelRor *p) {
    AoeScenarioMenuDialogRor *m = SMENU(p);
    AoeUiSystemRor *s = p->system;
    int i;
    aoe_ui_system_close_ror(s, "Open Dialog");
    aoe_ui_system_close_ror(s, "New Dialog");
    m->field_494 = NULL;
    for (i = 0; i < 7; ++i) free_button(&m->buttons[i]);
    aoe_ui_dialog_release_ror(&m->dialog);
}

int aoe_scenario_menu_dialog_init_ror(AoeScenarioMenuDialogRor *m,
                                      AoeGameUiRor *ui, AoeUiPanelRor *editor) {
    static const int32_t texts[7] = {0x2443, 0x2441, 0x2442, 0x2445,
                                     0x242e, 0x2446, 0x2444};
    static const int32_t ys[7] = {0x14, 0x46, 0x78, 0xaa, 0xdc, 0x10e, 0x154};
    AoeUiEzRor *ez, *eez = (AoeUiEzRor *)editor;
    int i;
    if (!m || !ui) return 0;
    aoe_ui_dialog_init_ror(&m->dialog, ui->system, ui->ez_ops, NAME);
    m->dialog.ez.panel.vt = &aoe_scenario_menu_dialog_vtable_ror.base;
    m->editor = editor;
    m->field_494 = NULL;
    for (i = 0; i < 7; ++i) m->buttons[i] = NULL;
    m->ui = ui;
    ez = &m->dialog.ez;
    if (!editor ||
        !aoe_ui_dialog_setup_ror(&m->dialog, editor->area, editor, 0x1cc, 400,
                                 aoe_ui_ez_popup_name_ror(eez),
                                 aoe_ui_ez_popup_id_ror(eez), 1))
        return 0;
    for (i = 0; i < 7; ++i)
        if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &m->buttons[i],
                                            texts[i], 0, 0x1e, ys[i], 400,
                                            0x28, 0, 0, 0))
            return 0;
    m->buttons[6]->hotkey = 0x1b;
    m->buttons[6]->hotkey_shift = 0;
    return 1;
}

static int answer(AoeScenarioMenuDialogRor *m, const char *dialog,
                  int32_t code, int open) {
    AoeGameUiRor *ui = m->ui;
    AoeUiSystemRor *s = ui->system;
    const AoeGameUiOpsRor *ops = ui->ops;
    AoeUiPanelRor *editor = m->editor;
    aoe_ui_system_show_named_ror(s, EDITOR, 0);
    aoe_ui_system_close_ror(s, dialog);
    aoe_ui_system_close_ror(s, NAME);
    if (code == 0 || code == 1) {
        if (code == 0 && ops && ops->editor_save_current)
            ops->editor_save_current(ops->context, editor);
        if (!open) {
            aoe_ui_system_close_ror(s, NAME);
            if (ops && ops->new_scenario) ops->new_scenario(ops->context);
        } else {
            if (ops && ops->open_editor_open) ops->open_editor_open(ops->context);
            aoe_ui_system_show_named_ror(s, "Scenario Editor Open", 0);
            aoe_ui_system_close_ror(s, NAME);
        }
    }
    if (code == 2) aoe_ui_system_close_ror(s, NAME);
    return 1;
}

static int32_t smenu_action(AoeUiPanelRor *p, AoeUiPanelRor *from,
                            int32_t code, int32_t value, int32_t extra) {
    AoeScenarioMenuDialogRor *m = SMENU(p);
    AoeGameUiRor *ui = m->ui;
    AoeUiSystemRor *s = ui->system;
    const AoeGameUiOpsRor *ops = ui->ops;
    AoeUiPanelRor *editor = m->editor;
    int i, which = -1;
    (void)value;
    (void)extra;
    if (code == 1) {
        for (i = 0; i < 7; ++i)
            if (m->buttons[i] && from == &m->buttons[i]->panel) which = i;
    }
    if (which >= 0) {
        aoe_ui_system_show_named_ror(s, EDITOR, 0);
        if (which == 6) {
            aoe_ui_system_close_ror(s, NAME);
            return 1;
        }
        aoe_ui_system_close_ror(s, NAME);
        if (!ops) return 1;
        switch (which) {
        case 0: if (ops->editor_quit) ops->editor_quit(ops->context, editor); break;
        case 1: if (ops->editor_save) ops->editor_save(ops->context, editor, 1, 0); break;
        case 2: if (ops->editor_save_as) ops->editor_save_as(ops->context, editor, 0); break;
        case 3: if (ops->editor_edit) ops->editor_edit(ops->context, editor); break;
        case 4: if (ops->editor_create) ops->editor_create(ops->context, editor); break;
        case 5: {
            void *scenario = ops->editor_test_scenario
                                 ? ops->editor_test_scenario(ops->context, editor)
                                 : NULL;
            if (scenario && ops->test_scenario) ops->test_scenario(ops->context, scenario);
            break;
        }
        }
        return 1;
    }
    if (from && from->name) {
        if (strcmp(from->name, "New Dialog") == 0)
            return answer(m, "New Dialog", code, 0);
        if (strcmp(from->name, "Open Dialog") == 0)
            return answer(m, "Open Dialog", code, 1);
    }
    return 0;
}

const AoeUiEzVtblRor aoe_scenario_menu_dialog_vtable_ror = {
    AOE_UI_EZ_SLOTS(smenu_destroy, aoe_ui_panel_handle_idle_ror,
                    aoe_ui_panel_handle_paint_ror, aoe_ui_ez_no_key_action_ror,
                    aoe_ui_panel_zero_ror, smenu_action,
                    aoe_ui_ez_set_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
