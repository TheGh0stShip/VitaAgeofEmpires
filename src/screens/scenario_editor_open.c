#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define OPEN(p) ((AoeScenarioEditorOpenRor *)(p))

static const char *const NAME = "Scenario Editor Open";
static const char *const CONFIRM = "Cancel Dialog";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

static void open_destroy(AoeUiPanelRor *p) {
    AoeScenarioEditorOpenRor *o = OPEN(p);
    DROP(o->title);
    DROP(o->list);
    DROP(o->scrollbar);
    DROP(o->open);
    DROP(o->cancel);
    DROP(o->del);
    aoe_ui_ez_release_ror(&o->ez);
}

static const char *dir_of(const AoeScenarioEditorOpenRor *o) {
    const AoeGameUiOpsRor *ops = o->ui->ops;
    const char *d = ops && ops->scenario_dir ? ops->scenario_dir(ops->context) : NULL;
    return d ? d : "";
}

static void add_found(void *user, const char *name) {
    AoeScenarioEditorOpenRor *o = user;
    char base[260];
    size_t n = strlen(name);
    n = n >= 4 ? n - 4 : 0; /* the extension */
    if (n >= sizeof base) n = sizeof base - 1;
    memcpy(base, name, n);
    base[n] = '\0';
    aoe_ui_text_append_ror(&o->list->text, base, 0);
}

void aoe_scenario_editor_open_fill_ror(AoeScenarioEditorOpenRor *o) {
    const AoeGameUiOpsRor *ops = o->ui->ops;
    char pattern[260];
    aoe_ui_text_clear_ror(&o->list->text);
    o->list->text.sorted = 1;
    if (!ops || !ops->list_files) return;
    snprintf(pattern, sizeof pattern, "%s*.scn", dir_of(o));
    ops->list_files(ops->context, pattern, add_found, o);
    snprintf(pattern, sizeof pattern, "%s*.scx", dir_of(o));
    ops->list_files(ops->context, pattern, add_found, o);
}

static const char *selected(const AoeScenarioEditorOpenRor *o) {
    const char *s = aoe_ui_text_selected_text_ror(&o->list->text);
    return s ? s : "";
}

static void disable_when_empty(AoeScenarioEditorOpenRor *o) {
    if (*selected(o) == '\0') {
        aoe_ui_button_set_disabled_ror(o->open, 1);
        aoe_ui_button_set_disabled_ror(o->del, 1);
    }
}

int aoe_scenario_editor_open_init_ror(AoeScenarioEditorOpenRor *o, AoeGameUiRor *ui,
                                      AoeUiAreaRor *area, const char *info_name,
                                      int32_t info_id) {
    AoeUiEzRor *ez = &o->ez;
    AoeUiPanelRor *tab[4];
    memset(o, 0, sizeof *o);
    o->ui = ui;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_scenario_editor_open_vtable_ror.base;
    if (!aoe_ui_screen_setup_ror(ez, area, info_name, info_id, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_shadow_area_ror(ez, 0);
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &o->title, 0x24cd, 0x14, 0x14, 600, 0x1e,
                                      1, 1, 0, 0))
        return 1;
    if (!aoe_ui_ez_create_list_ror(ez, &ez->panel, &o->list, 0x14, 0x46, 600, 0x154, 0xb))
        return 1;
    if (!aoe_ui_ez_create_text_scrollbar_ror(ez, &o->scrollbar, &o->list->text, 0x14)) return 1;
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &o->open, 0xfa1, 0, 0x1e, 0x1b8, 0xb4,
                                        0x1e, 0, 0, 0))
        return 1;
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &o->del, 0x24c9, 0, 0xe6, 0x1b8, 0xb4,
                                        0x1e, 0, 0, 0))
        return 1;
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &o->cancel, 0xfa2, 0, 0x1ae, 0x1b8, 0xb4,
                                        0x1e, 0, 0, 0))
        return 1;
    o->cancel->hotkey = 0x1b;
    o->cancel->hotkey_shift = 0;
    o->del->hotkey_shift = 0;
    o->del->hotkey = 0x2e;
    aoe_scenario_editor_open_fill_ror(o);
    disable_when_empty(o);
    aoe_ui_panel_set_current_ror(&ez->panel, &o->list->text.panel);
    tab[0] = &o->list->text.panel;
    tab[1] = &o->open->panel;
    tab[2] = &o->del->panel;
    tab[3] = &o->cancel->panel;
    aoe_ui_panels_tab_cycle_ror(tab, 4);
    return 1;
}

static int32_t open_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                           int32_t extra) {
    AoeScenarioEditorOpenRor *o = OPEN(p);
    AoeUiSystemRor *s = o->ui->system;
    const AoeGameUiOpsRor *ops = o->ui->ops;
    char path[0x200], line[0x200];
    if (!from) return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
    if (from->name && strcmp(from->name, CONFIRM) == 0) {
        aoe_ui_system_close_ror(s, CONFIRM);
        if (code == 0 && ops && ops->delete_file) {
            snprintf(path, sizeof path, "%s%s.scn", dir_of(o), selected(o));
            ops->delete_file(ops->context, path);
            snprintf(path, sizeof path, "%s%s.scx", dir_of(o), selected(o));
            ops->delete_file(ops->context, path);
            aoe_ui_text_remove_ror(&o->list->text, aoe_ui_text_selection_ror(&o->list->text));
            disable_when_empty(o);
        }
        return 1;
    }
    if (from == &o->del->panel && code == 1 && *selected(o)) {
        char fmt[0x100];
        aoe_ui_string_ror(s, 0x24ca, fmt, sizeof fmt);
        /* The string is used as the format for the name. */
        snprintf(line, sizeof line, fmt, selected(o));
        aoe_ui_ez_yes_no_dialog_ror(&o->ez, line, CONFIRM, 0x1c2, 100);
    }
    if (from == &o->list->text.panel) {
        if (code == 3) return p->vt->action(p, &o->open->panel, 1, 0, 0);
    } else if (from == &o->open->panel) {
        if (code == 1) {
            const char *file = "%s.scn";
            if (aoe_ui_text_count_ror(&o->list->text) < 1) return 1;
            snprintf(path, sizeof path, "%s%s.scn", dir_of(o), selected(o));
            if (!ops || !ops->file_exists || !ops->file_exists(ops->context, path)) {
                snprintf(path, sizeof path, "%s%s.scx", dir_of(o), selected(o));
                if (!ops || !ops->file_exists || !ops->file_exists(ops->context, path)) {
                    aoe_ui_ez_ok_dialog_id_ror(&o->ez, 0x24cf, NULL, 0x1c2, 100);
                    return 1;
                }
                file = "%s.scx";
            }
            snprintf(line, sizeof line, file, selected(o));
            if (ops && ops->editor_load) ops->editor_load(ops->context, line, 0);
            return 1;
        }
    } else if (from == &o->cancel->panel && code == 1) {
        if (!aoe_ui_system_find_ror(s, "Scenario Editor Screen")) {
            if (ops && ops->open_editor_menu) ops->open_editor_menu(ops->context);
            aoe_ui_system_show_named_ror(s, "Scenario Editor Menu", 0);
        } else {
            aoe_ui_system_show_named_ror(s, "Scenario Editor Screen", 0);
        }
        aoe_ui_system_close_ror(s, NAME);
        return 1;
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_scenario_editor_open_vtable_ror = {
    AOE_UI_EZ_SLOTS(open_destroy, aoe_ui_panel_handle_idle_ror, aoe_ui_panel_handle_paint_ror,
                    aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror, open_action,
                    aoe_ui_ez_set_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
