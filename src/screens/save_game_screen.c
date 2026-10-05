#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define SV(p) ((AoeSaveGameScreenRor *)(p))

static const char *const NAME = "Save Game Screen";
static const char *const CONFIRM = "Cancel Dialog";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

static void sv_destroy(AoeUiPanelRor *p) {
    AoeSaveGameScreenRor *s = SV(p);
    DROP(s->title);
    DROP(s->name);
    DROP(s->list);
    DROP(s->list_bar);
    DROP(s->save);
    DROP(s->cancel);
    DROP(s->del);
    aoe_ui_ez_release_ror(&s->ez);
}

static const char *dir_of(const AoeSaveGameScreenRor *s) {
    const AoeGameUiOpsRor *ops = s->ui->ops;
    const char *d = NULL;
    if (ops) d = s->mode == 0 ? (ops->game_dir ? ops->game_dir(ops->context) : NULL)
                              : (ops->scenario_dir ? ops->scenario_dir(ops->context) : NULL);
    return d ? d : "";
}

static const char *ext(const AoeSaveGameScreenRor *s, int x) {
    if (s->mode == 0) return x ? "gmx" : "gam";
    return x ? "scx" : "scn";
}

static void add_found(void *user, const char *name) {
    AoeSaveGameScreenRor *s = user;
    char base[260];
    size_t n = strlen(name);
    n = n >= 4 ? n - 4 : 0;
    if (n >= sizeof base) n = sizeof base - 1;
    memcpy(base, name, n);
    base[n] = '\0';
    aoe_ui_text_append_ror(&s->list->text, base, 0);
}

void aoe_save_game_screen_fill_ror(AoeSaveGameScreenRor *s) {
    const AoeGameUiOpsRor *ops = s->ui->ops;
    char pattern[0x200];
    int x;
    aoe_ui_text_clear_ror(&s->list->text);
    s->list->text.sorted = 1;
    if (!ops || !ops->list_files) return;
    for (x = 0; x < 2; ++x) {
        snprintf(pattern, sizeof pattern, "%s*.%s", dir_of(s), ext(s, x));
        ops->list_files(ops->context, pattern, add_found, s);
    }
}

static const char *selected(const AoeSaveGameScreenRor *s) {
    const char *t = aoe_ui_text_selected_text_ror(&s->list->text);
    return t ? t : "";
}

int aoe_save_game_screen_init_ror(AoeSaveGameScreenRor *s, AoeGameUiRor *ui, AoeUiAreaRor *area,
                                  int32_t mode, const char *name, int32_t after,
                                  const char *popup_name, int32_t popup_id) {
    AoeUiEzRor *ez = &s->ez;
    const AoeGameUiOpsRor *ops = ui->ops;
    AoeUiPanelRor *tab[5];
    int32_t max;
    size_t dir_len;
    memset(s, 0, sizeof *s);
    s->ui = ui;
    s->mode = mode;
    s->after = after;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_save_game_screen_vtable_ror.base;
    if (!name && mode == 0 && ops && ops->default_save_name)
        name = ops->default_save_name(ops->context);
    if (!aoe_ui_screen_setup_ror(ez, area, popup_name, popup_id, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_shadow_area_ror(ez, 0);
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &s->title, 0x24c3 + (mode != 0), 0x14,
                                      0x14, 600, 0x1e, 1, 1, 0, 0))
        return 1;
    /* The field holds what fits in a 122-character path, at most 80
       (the length of the game directory, game +0x1e8, as the original). */
    dir_len = ops && ops->game_dir && ops->game_dir(ops->context)
                  ? strlen(ops->game_dir(ops->context))
                  : 0;
    max = 0x7a - (int32_t)dir_len < 0x51 ? 0x7a - (int32_t)dir_len : 0x50;
    if (!aoe_ui_ez_create_edit_ror(ez, &ez->panel, &s->name, name ? name : "", (int16_t)max, 5,
                                   0x14, 0x46, 600, 0x17, 0xb, 1, 1))
        return 1;
    aoe_ui_panel_set_help_ror(&s->name->panel, 0x7670, -1);
    if (!aoe_ui_ez_create_list_ror(ez, &ez->panel, &s->list, 0x14, 0x6e, 600, 300, 0xb) ||
        !s->list)
        return 1;
    if (!aoe_ui_ez_create_text_scrollbar_ror(ez, &s->list_bar, &s->list->text, 0x14)) return 1;
    aoe_ui_panel_set_help_ror(&s->list_bar->panel, 0x7671, -1);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &s->save, 0xfa1, 0, 0x1e, 0x1b8, 0xb4,
                                        0x1e, 0, 0, 0) ||
        !aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &s->del, 0x24c9, 0, 0xe6, 0x1b8, 0xb4,
                                        0x1e, 0, 0, 0) ||
        !aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &s->cancel, 0xfa2, 0, 0x1ae, 0x1b8, 0xb4,
                                        0x1e, 0, 0, 0))
        return 1;
    aoe_ui_panel_set_help_ror(&s->save->panel, 0x7531, -1);
    aoe_ui_panel_set_help_ror(&s->cancel->panel, 0x7532, -1);
    s->cancel->hotkey = 0x1b;
    s->cancel->hotkey_shift = 0;
    s->del->hotkey_shift = 0;
    s->del->hotkey = 0x2e;
    aoe_save_game_screen_fill_ror(s);
    if (*selected(s) == '\0') aoe_ui_button_set_disabled_ror(s->del, 1);
    aoe_ui_panel_set_current_ror(&ez->panel, &s->name->panel);
    tab[0] = &s->name->panel;
    tab[1] = &s->list->text.panel;
    tab[2] = &s->save->panel;
    tab[3] = &s->del->panel;
    tab[4] = &s->cancel->panel;
    aoe_ui_panels_tab_cycle_ror(tab, 5);
    return 1;
}

static void delete_files(AoeSaveGameScreenRor *s, const char *name, int both) {
    const AoeGameUiOpsRor *ops = s->ui->ops;
    char path[0x200];
    int x;
    if (!ops || !ops->delete_file) return;
    for (x = 0; x < (both ? 2 : 1); ++x) {
        snprintf(path, sizeof path, "%s%s.%s", dir_of(s), name, ext(s, x));
        ops->delete_file(ops->context, path);
    }
}

/* Save under s->file (the .gmx / .scx name); messages on failure. */
static void write_file(AoeSaveGameScreenRor *s) {
    const AoeGameUiOpsRor *ops = s->ui->ops;
    if (s->mode == 0) {
        if (!ops || !ops->save_game || !ops->save_game(ops->context, s->file))
            aoe_ui_ez_ok_dialog_id_ror(&s->ez, 0x962, NULL, 0x1c2, 100);
    } else if (s->mode == 1) {
        if (!ops || !ops->save_scenario || !ops->save_scenario(ops->context, s->file))
            aoe_ui_ez_ok_dialog_id_ror(&s->ez, 0x963, NULL, 0x1c2, 100);
    } else if (s->mode == 2) {
        aoe_save_game_screen_editor_save_ror(s);
    }
}

void aoe_save_game_screen_editor_save_ror(AoeSaveGameScreenRor *s) {
    const AoeGameUiOpsRor *ops = s->ui->ops;
    AoeUiSystemRor *sys = s->ui->system;
    if (ops && ops->show_status) ops->show_status(ops->context, 0x450);
    if (!ops || !ops->editor_save_file || !ops->editor_save_file(ops->context, s->file)) {
        aoe_ui_ez_ok_dialog_id_ror(&s->ez, 0x963, NULL, 0x1c2, 100);
        return;
    }
    if (ops->editor_saved) ops->editor_saved(ops->context);
    switch (s->after) {
    case 1:
        if (!ops->to_main_menu || !ops->to_main_menu(ops->context)) {
            if (ops->quit_program) ops->quit_program(ops->context);
            return;
        }
        aoe_ui_system_close_ror(sys, "Scenario Editor Screen");
        break;
    case 2:
        if (ops->open_editor_open_screen) ops->open_editor_open_screen(ops->context);
        aoe_ui_system_show_named_ror(sys, "Scenario Editor Open", 0);
        break;
    case 3:
        if (ops->editor_load) ops->editor_load(ops->context, NULL, 0);
        break;
    default:
        aoe_ui_system_show_named_ror(sys, "Scenario Editor Screen", 0);
        break;
    }
    aoe_ui_system_close_ror(sys, "Status Screen");
    aoe_ui_system_close_ror(sys, NAME);
}

static int exists(AoeSaveGameScreenRor *s, const char *file) {
    const AoeGameUiOpsRor *ops = s->ui->ops;
    char path[0x200];
    snprintf(path, sizeof path, "%s%s", dir_of(s), file);
    return ops && ops->file_exists && ops->file_exists(ops->context, path);
}

static int32_t sv_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                         int32_t extra) {
    AoeSaveGameScreenRor *s = SV(p);
    AoeUiSystemRor *sys = s->ui->system;
    const AoeGameUiOpsRor *ops = s->ui->ops;
    char line[0x300];
    if (!from) return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
    if (from->name && strcmp(from->name, CONFIRM) == 0) {
        aoe_ui_system_close_ror(sys, CONFIRM);
        if (code == 0) {
            delete_files(s, selected(s), 1);
            aoe_ui_text_remove_ror(&s->list->text, aoe_ui_text_selection_ror(&s->list->text));
            aoe_ui_edit_set_text_ror(s->name, selected(s));
            if (*selected(s) == '\0') aoe_ui_button_set_disabled_ror(s->del, 1);
        }
        return 1;
    }
    if (from == &s->del->panel && code == 1 && *selected(s)) {
        char fmt[0x100];
        aoe_ui_string_ror(sys, 0x24ca, fmt, sizeof fmt);
        snprintf(line, sizeof line, fmt, selected(s));
        aoe_ui_ez_yes_no_dialog_ror(&s->ez, line, CONFIRM, 0x1c2, 100);
    }
    if (from == &s->list->text.panel && code == 1) {
        aoe_ui_edit_set_text_ror(s->name, selected(s));
        return 1;
    }
    if ((from == &s->save->panel && code == 1) || (from == &s->name->panel && code == 0) ||
        (from == &s->list->text.panel && code == 3)) {
        int found;
        aoe_ui_edit_trimmed_ror(s->name, s->typed, sizeof s->typed);
        if (s->typed[0] == '\0') {
            aoe_ui_ez_ok_dialog_id_ror(&s->ez, s->mode != 0 ? 0x24c7 : 0x24c6, NULL, 0x1c2, 100);
            return 1;
        }
        snprintf(s->file, sizeof s->file, "%.250s.%s", s->typed, ext(s, 0));
        found = exists(s, s->file);
        snprintf(s->file, sizeof s->file, "%.250s.%s", s->typed, ext(s, 1));
        if (!found) found = exists(s, s->file);
        if (found) {
            aoe_ui_ez_yes_no_dialog_id_ror(&s->ez, 0x24c8, NULL, 0x1c2, 100);
            return 1;
        }
        write_file(s);
        return 1;
    }
    if ((from == &s->cancel->panel && code == 1) || (from == &s->name->panel && code == 1)) {
        if (ops && ops->resume_game) ops->resume_game(ops->context);
        aoe_ui_system_show_named_ror(sys, s->mode == 2 ? "Scenario Editor Screen" : "Game Screen",
                                     0);
        aoe_ui_system_close_ror(sys, NAME);
        return 1;
    }
    if (from->name && strcmp(from->name, "OKDialog") == 0) {
        aoe_ui_system_close_ror(sys, "OKDialog");
        aoe_ui_panel_set_current_ror(p, &s->name->panel);
        return 1;
    }
    if (from->name && strcmp(from->name, "YesNoDialog") == 0) {
        aoe_ui_system_close_ror(sys, "YesNoDialog");
        if (code == 0) {
            aoe_ui_edit_trimmed_ror(s->name, s->typed, sizeof s->typed);
            delete_files(s, s->typed, 0);
            write_file(s);
        } else {
            aoe_ui_panel_set_current_ror(p, &s->name->panel);
        }
        return 1;
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_save_game_screen_vtable_ror = {
    AOE_UI_EZ_SLOTS(sv_destroy, aoe_ui_panel_handle_idle_ror, aoe_ui_panel_handle_paint_ror,
                    aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror, sv_action,
                    aoe_ui_screen_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
