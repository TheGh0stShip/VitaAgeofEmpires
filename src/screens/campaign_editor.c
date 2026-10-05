#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define CE(p) ((AoeCampaignEditorRor *)(p))

static const char *const NAME = "Campaign Editor Screen";
static const char *const CANCEL_DIALOG = "Cancel Dialog";
static const char *const LOAD_DIALOG = "Load Dialog";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

static const AoeCampaignOpsRor *files_of(AoeCampaignEditorRor *c) {
    const AoeGameUiOpsRor *ops = c->ui->ops;
    return ops && ops->campaign_file_ops ? ops->campaign_file_ops(ops->context) : NULL;
}

static void free_campaign(AoeCampaignEditorRor *c) {
    if (c->campaign) {
        aoe_campaign_free_ror(c->campaign);
        free(c->campaign);
        c->campaign = NULL;
    }
}

static void ce_destroy(AoeUiPanelRor *p) {
    AoeCampaignEditorRor *c = CE(p);
    AoeUiSystemRor *sys = c->ui->system;
    aoe_ui_system_close_ror(sys, CANCEL_DIALOG);
    aoe_ui_system_close_ror(sys, LOAD_DIALOG);
    DROP(c->title);
    DROP(c->available_label);
    DROP(c->chosen_label);
    DROP(c->available);
    DROP(c->available_bar);
    DROP(c->chosen);
    DROP(c->chosen_bar);
    DROP(c->file_label);
    DROP(c->name);
    DROP(c->files);
    DROP(c->cancel);
    DROP(c->save);
    DROP(c->done);
    DROP(c->add);
    DROP(c->remove);
    DROP(c->up);
    DROP(c->down);
    free_campaign(c);
    aoe_ui_ez_release_ror(&c->ez);
}

typedef struct {
    void (*add)(void *target, const char *text);
    void *target;
} Lister;

/* A found file's name without its four-character extension. */
static void found(void *user, const char *name) {
    Lister *l = user;
    char base[260];
    size_t n = strlen(name);
    n = n >= 4 ? n - 4 : 0;
    if (n >= sizeof base) n = sizeof base - 1;
    memcpy(base, name, n);
    base[n] = '\0';
    l->add(l->target, base);
}

static void add_to_dropdown(void *d, const char *text) {
    aoe_ui_dropdown_append_ror(d, text, 0);
}
static void add_to_text(void *t, const char *text) { aoe_ui_text_append_ror(t, text, 0); }

static void list_two(const AoeCampaignOpsRor *f, const char *dir, const char *a, const char *b,
                     Lister *l) {
    char pattern[0x200];
    if (!f || !f->find) return;
    snprintf(pattern, sizeof pattern, "%s*.%s", dir ? dir : "", a);
    f->find(f->context, pattern, found, l);
    snprintf(pattern, sizeof pattern, "%s*.%s", dir ? dir : "", b);
    f->find(f->context, pattern, found, l);
}

void aoe_campaign_editor_fill_names_ror(AoeCampaignEditorRor *c) {
    const AoeCampaignOpsRor *f = files_of(c);
    Lister l = {add_to_dropdown, c->files};
    aoe_ui_dropdown_set_sorted_ror(c->files, 1);
    aoe_ui_dropdown_clear_ror(c->files);
    list_two(f, f ? f->campaign_dir : NULL, "cpn", "cpx", &l);
}

void aoe_campaign_editor_fill_available_ror(AoeCampaignEditorRor *c) {
    const AoeCampaignOpsRor *f = files_of(c);
    Lister l = {add_to_text, &c->available->text};
    c->available->text.sorted = 1;
    aoe_ui_text_clear_ror(&c->available->text);
    list_two(f, f ? f->scenario_dir : NULL, "scn", "scx", &l);
}

static void fill_chosen(AoeCampaignEditorRor *c) {
    char **names = NULL;
    int32_t n, i;
    if (!c->campaign) return;
    aoe_ui_text_clear_ror(&c->chosen->text);
    n = aoe_campaign_names_ror(c->campaign, &names);
    if (!names) return;
    for (i = 0; i < n; ++i) {
        aoe_ui_text_append_ror(&c->chosen->text, names[i], 0);
        free(names[i]);
    }
    free(names);
}

static int exists(AoeCampaignEditorRor *c, const char *path) {
    const AoeGameUiOpsRor *ops = c->ui->ops;
    return ops && ops->file_exists && ops->file_exists(ops->context, path);
}

void aoe_campaign_editor_open_ror(AoeCampaignEditorRor *c) {
    const AoeCampaignOpsRor *f = files_of(c);
    const char *dir = f && f->campaign_dir ? f->campaign_dir : "";
    const char *text = aoe_ui_edit_text_ror(c->name);
    char file[0x200], path[0x400];
    int taken = 0, found_file;
    snprintf(file, sizeof file, "%s.cpn", text ? text : "");
    snprintf(path, sizeof path, "%s%s", dir, file);
    found_file = exists(c, path);
    if (!found_file) {
        snprintf(file, sizeof file, "%s.cpx", text ? text : "");
        snprintf(path, sizeof path, "%s%s", dir, file);
        found_file = exists(c, path);
    }
    if (found_file && f) {
        c->campaign = calloc(1, sizeof *c->campaign);
        /* A file that cannot be read still gives an empty campaign. */
        if (c->campaign) aoe_campaign_load_ror(c->campaign, file, f);
        aoe_campaign_editor_fill_available_ror(c);
        fill_chosen(c);
        taken = aoe_ui_text_take_chosen_ror(&c->available->text, &c->chosen->text);
        free_campaign(c);
    }
    if (!taken) aoe_ui_text_clear_ror(&c->chosen->text);
    c->changed = 0;
}

void aoe_campaign_editor_save_ror(AoeCampaignEditorRor *c) {
    const AoeGameUiOpsRor *ops = c->ui->ops;
    const AoeCampaignOpsRor *f = files_of(c);
    AoeUiTextRor *t = &c->chosen->text;
    int32_t count = aoe_ui_text_count_ror(t), i;
    const char *name = aoe_ui_edit_text_ror(c->name);
    char file[0x200], path[0x400];
    if (!name) {
        c->changed = 0;
        return;
    }
    if (!*name || *name == ' ') {
        aoe_ui_ez_ok_dialog_id_ror(&c->ez, 0x2bcc, NULL, 0x1c2, 100);
        return;
    }
    snprintf(file, sizeof file, "%s.cpx", name);
    if (count < 1) {
        aoe_ui_ez_ok_dialog_id_ror(&c->ez, 0x2bcb, NULL, 0x1c2, 100);
    } else if (f) {
        snprintf(path, sizeof path, "%s%s.cpn", f->campaign_dir ? f->campaign_dir : "", name);
        if (exists(c, path)) {
            if (ops->file_writable && !ops->file_writable(ops->context, path)) goto done;
            if (ops->delete_file) ops->delete_file(ops->context, path);
        }
        {
            char **names = calloc((size_t)count, sizeof *names);
            char **files = calloc((size_t)count, sizeof *files);
            if (names && files) {
                int ok = 1;
                for (i = 0; i < count && ok; ++i) {
                    const char *line = aoe_ui_text_line_ror(t, i);
                    size_t n = strlen(line ? line : "") + 5;
                    names[i] = malloc(n);
                    files[i] = malloc(n);
                    if (!names[i] || !files[i]) {
                        ok = 0;
                        break;
                    }
                    snprintf(names[i], n, "%s", line ? line : "");
                    snprintf(path, sizeof path, "%s%s.scn", f->scenario_dir ? f->scenario_dir : "",
                             names[i]);
                    snprintf(files[i], n, "%s%s", names[i], exists(c, path) ? ".scn" : ".scx");
                }
                if (ok) {
                    c->campaign = calloc(1, sizeof *c->campaign);
                    if (c->campaign)
                        aoe_campaign_create_ror(c->campaign, file, name, count,
                                                (const char *const *)files,
                                                (const char *const *)names, f);
                    free_campaign(c);
                }
            }
            for (i = 0; names && files && i < count; ++i) {
                free(names[i]);
                free(files[i]);
            }
            free(names);
            free(files);
        }
    }
done:
    aoe_campaign_editor_fill_names_ror(c);
    if (ops && ops->refresh_campaigns) ops->refresh_campaigns(ops->context);
    c->changed = 0;
}

static void reveal_list(void *context, AoeUiTextRor *t, int32_t index) {
    AoeCampaignEditorRor *c = context;
    AoeUiListRor *l = t == &c->available->text ? c->available : c->chosen;
    aoe_ui_list_move_ror(l, 1, (int16_t)index, 1);
}

int aoe_campaign_editor_init_ror(AoeCampaignEditorRor *c, AoeGameUiRor *ui, AoeUiAreaRor *area) {
    AoeUiEzRor *ez = &c->ez;
    AoeUiPanelRor *tab[8];
    char text[0x100];
    memset(c, 0, sizeof *c);
    c->ui = ui;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_campaign_editor_vtable_ror.base;
    if (!aoe_ui_screen_setup_ror(ez, area, "scr4", 0xc386, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_shadow_area_ror(ez, 0);
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &c->title, 0x2bc1, 10, 10, 0x26c, 0x32, 1, 3, 0, 0);
    aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &c->file_label, 0x2bc2, 0x14, 0x28, 0x104, 0x14, 4,
                                 0, 0, 0);
    aoe_ui_ez_create_dropdown_ror(ez, &ez->panel, &c->files, 0x17c, 200, 0x14, 0x3c, 0x17c,
                                  ez->panel.height ? ez->ref_h * 0x1e / ez->panel.height : 0x1e,
                                  0xb);
    if (c->files) {
        aoe_ui_string_ror(ui->system, 0x2bca, text, sizeof text);
        aoe_ui_ez_create_edit_ror(ez, &c->files->panel, &c->name, text, 100, 5, 0, 0, 0x168, 0x1e,
                                  0xb, 1, 1);
    }
    aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &c->save, 0x2bc4, 0, 0x1a4, 0x3c, 200, 0x1e, 0,
                                   0, 0);
    aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &c->add, 0x2bc5, 0, 0x118, 0x104, 0xa0, 0x1e, 0,
                                   0, 0);
    aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &c->remove, 0x2bc6, 0, 0x1cc, 0x104, 0xa0, 0x1e,
                                   0, 0, 0);
    aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &c->available_label, 0x2bc7, 0x14, 100, 200, 0x14,
                                 4, 0, 0, 0);
    aoe_ui_ez_create_list_ror(ez, &ez->panel, &c->available, 0x14, 0x78, 600, 0x82, 0xb);
    if (c->available)
        aoe_ui_ez_create_text_scrollbar_ror(ez, &c->available_bar, &c->available->text, 0x14);
    aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &c->chosen_label, 0x2bc8, 0x14, 0x118, 200, 0x14,
                                 4, 0, 0, 0);
    aoe_ui_ez_create_list_ror(ez, &ez->panel, &c->chosen, 0x14, 300, 500, 0x82, 0xb);
    if (c->chosen)
        aoe_ui_ez_create_text_scrollbar_ror(ez, &c->chosen_bar, &c->chosen->text, 0x14);
    aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &c->done, 0xfa1, 0, 0x14, 0x1bd, 0x118, 0x1e, 0,
                                   0, 0);
    aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &c->cancel, 0xfa2, 0, 0x154, 0x1bd, 0x118, 0x1e,
                                   0, 0, 0);
    if (c->cancel) {
        c->cancel->hotkey = 0x1b;
        c->cancel->hotkey_shift = 0;
    }
    aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &c->up, 0x2bd5, 0, 0x212, 300, 0x5a, 0x1e, 0, 0,
                                   0);
    aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &c->down, 0x2bd6, 0, 0x212, 0x154, 0x5a, 0x1e, 0,
                                   0, 0);
    /* The original goes on unchecked; the host stops at a missing part. */
    if (!c->title || !c->file_label || !c->files || !c->name || !c->save || !c->add ||
        !c->remove || !c->available_label || !c->available || !c->available_bar ||
        !c->chosen_label || !c->chosen || !c->chosen_bar || !c->done || !c->cancel || !c->up ||
        !c->down)
        return 1;
    tab[0] = &c->files->panel;
    tab[1] = &c->save->panel;
    tab[2] = &c->available->text.panel;
    tab[3] = &c->add->panel;
    tab[4] = &c->remove->panel;
    tab[5] = &c->chosen->text.panel;
    tab[6] = &c->done->panel;
    tab[7] = &c->cancel->panel;
    aoe_ui_panels_tab_cycle_ror(tab, 8);
    aoe_ui_panel_set_current_ror(&ez->panel, &c->name->panel);
    aoe_ui_panel_set_current_ror(&ez->panel, &c->files->panel);
    aoe_campaign_editor_fill_available_ror(c);
    aoe_campaign_editor_fill_names_ror(c);
    aoe_ui_text_clear_ror(&c->chosen->text);
    aoe_campaign_editor_open_ror(c);
    c->changed = 0;
    return 1;
}

static void to_menu(AoeCampaignEditorRor *c) {
    const AoeGameUiOpsRor *ops = c->ui->ops;
    if (ops && ops->open_editor_menu) ops->open_editor_menu(ops->context);
    aoe_ui_system_show_named_ror(c->ui->system, "Scenario Editor Menu", 0);
    aoe_ui_system_close_ror(c->ui->system, NAME);
}

static void load_picked(AoeCampaignEditorRor *c) {
    aoe_ui_edit_set_text_ror(c->name, aoe_ui_dropdown_selected_text_ror(c->files));
    aoe_campaign_editor_open_ror(c);
}

static int unsaved(AoeCampaignEditorRor *c) {
    return c->changed && aoe_ui_text_count_ror(&c->chosen->text) >= 1;
}

static void ask(AoeCampaignEditorRor *c, const char *dialog) {
    char fmt[0x100], line[0x200];
    const char *name = aoe_ui_edit_text_ror(c->name);
    aoe_ui_string_ror(c->ui->system, 0x2bc9, fmt, sizeof fmt);
    /* The string is used as the format for the name. */
    snprintf(line, sizeof line, fmt, name ? name : "");
    aoe_ui_ez_yes_no_cancel_dialog_ror(&c->ez, line, dialog, 0x1c2, 100);
}

static int32_t ce_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                         int32_t extra) {
    AoeCampaignEditorRor *c = CE(p);
    AoeUiSystemRor *sys = c->ui->system;
    if (!from) return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
    if (from->name && strcmp(from->name, CANCEL_DIALOG) == 0) {
        aoe_ui_system_close_ror(sys, CANCEL_DIALOG);
        if (code == 0) aoe_campaign_editor_save_ror(c);
        else if (code != 1) return 1;
        to_menu(c);
        return 1;
    }
    if (from->name && strcmp(from->name, LOAD_DIALOG) == 0) {
        aoe_ui_system_close_ror(sys, LOAD_DIALOG);
        if (code == 0) aoe_campaign_editor_save_ror(c);
        else if (code != 1) return 1;
        load_picked(c);
        return 1;
    }
    if (c->files && from == &c->files->panel) {
        if (code == 2) {
            aoe_ui_panel_set_current_ror(p, &c->name->panel);
            aoe_ui_panel_set_current_ror(p, &c->files->panel);
        } else if (code == 0) {
            if (!unsaved(c)) load_picked(c);
            else ask(c, LOAD_DIALOG);
            return 1;
        }
    }
    if (code == 3) {
        if (from == &c->available->text.panel)
            aoe_ui_text_move_selected_ror(&c->available->text, &c->chosen->text, reveal_list, c,
                                          &c->changed);
        else if (from == &c->chosen->text.panel)
            aoe_ui_text_move_selected_ror(&c->chosen->text, &c->available->text, reveal_list, c,
                                          &c->changed);
    } else if (code == 1) {
        if (from == &c->done->panel) {
            if (!unsaved(c)) to_menu(c);
            else ask(c, CANCEL_DIALOG);
            return 1;
        }
        if (from == &c->save->panel) {
            aoe_campaign_editor_save_ror(c);
        } else if (from == &c->cancel->panel) {
            to_menu(c);
            return 1;
        } else if (from == &c->add->panel) {
            aoe_ui_text_move_selected_ror(&c->available->text, &c->chosen->text, reveal_list, c,
                                          &c->changed);
        } else if (from == &c->remove->panel) {
            aoe_ui_text_move_selected_ror(&c->chosen->text, &c->available->text, reveal_list, c,
                                          &c->changed);
        } else if (from == &c->up->panel) {
            aoe_ui_text_move_up_ror(&c->chosen->text, reveal_list, c, &c->changed);
        } else if (from == &c->down->panel) {
            aoe_ui_text_move_down_ror(&c->chosen->text, reveal_list, c, &c->changed);
        }
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_campaign_editor_vtable_ror = {
    AOE_UI_EZ_SLOTS(ce_destroy, aoe_ui_panel_handle_idle_ror, aoe_ui_panel_handle_paint_ror,
                    aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror, ce_action,
                    aoe_ui_screen_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
