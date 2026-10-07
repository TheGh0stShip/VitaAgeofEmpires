#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

static void load_game_add(void *arg, const char *name) {
    AoeLoadGameScreenRor *l = (AoeLoadGameScreenRor *)arg;
    char text[260];
    size_t n = strlen(name);
    n = n > 4 ? n - 4 : 0;
    if (n >= sizeof text) n = sizeof text - 1;
    memcpy(text, name, n);
    text[n] = '\0';
    l->ops->list_append(l->ops->context, text);
}

/* 0x004860a0 */
void aoe_load_game_screen_fill_ror(AoeLoadGameScreenRor *l) {
    const AoeLoadGameScreenOpsRor *o = l->ops;
    char pattern[260];
    const char *dir;
    o->list_clear(o->context);
    o->list_set_sorted(o->context, 1);
    dir = o->save_dir(o->context);
    snprintf(pattern, sizeof pattern, "%s*.gam", dir ? dir : "");
    o->find_files(o->context, pattern, load_game_add, l);
    snprintf(pattern, sizeof pattern, "%s*.gmx", dir ? dir : "");
    o->find_files(o->context, pattern, load_game_add, l);
}

/* 0x00485ff0 / 0x00485fd0 */
void aoe_load_game_screen_destroy_ror(AoeLoadGameScreenRor *l, uint8_t flags) {
    const AoeLoadGameScreenOpsRor *o = l->ops;
    int i;
    for (i = 0; i < AOE_LOAD_GAME_PANELS; ++i) {
        o->release_panel(o->context, l->panels[i]);
        l->panels[i] = NULL;
    }
    o->base_destroy(o->context);
    if (flags & 1) o->free_self(o->context);
}

int32_t aoe_load_game_screen_action_ror(AoeLoadGameScreenRor *l, const AoeLoadGameActionOpsRor *o,
                                        void *from, int32_t code) {
    char path[0x200];
    if (!l || !o) return 0;
    if (from) {
        const char *dn = o->dialog_name(o->context, from);
        if (dn && strcmp(dn, "Cancel Dialog") == 0) {
            o->close_dialog(o->context, "Cancel Dialog");
            if (code == 0) {
                const char *name = o->selected_name(o->context);
                snprintf(path, sizeof path, "%s%s.gam", o->save_dir(o->context), name);
                o->delete_file(o->context, path);
                snprintf(path, sizeof path, "%s%s.gmx", o->save_dir(o->context), name);
                o->delete_file(o->context, path);
                o->refill_at_line(o->context);
                if (!o->selected_name(o->context)[0]) o->disable_buttons(o->context);
            }
            return 1;
        }
        if (from == l->panels[6] && code == 1 && o->selected_name(o->context)[0]) {
            char fmt[0x100], text[0x200];
            o->string(o->context, 0x24ca, fmt, (int)sizeof fmt);
            snprintf(text, sizeof text, fmt, o->selected_name(o->context));
            o->ask(o->context, text);
        }
        if (from == l->panels[2]) {
            if (code == 3) return aoe_load_game_screen_action_ror(l, o, l->panels[4], 1);
        } else if (from == l->panels[4]) {
            if (code == 1) {
                if (o->line_count(o->context) > 0) {
                    char name[0x104];
                    snprintf(name, sizeof name, "%s", o->selected_name(o->context));
                    snprintf(path, sizeof path, "%s%s.gmx", o->save_dir(o->context), name);
                    if (!o->file_exists(o->context, path))
                        snprintf(path, sizeof path, "%s.gam", name);
                    else
                        snprintf(path, sizeof path, "%s.gmx", name);
                    if (!o->load_game(o->context, path)) o->ok_dialog(o->context, 0x2501);
                }
                return 1;
            }
        } else if (from == l->panels[5] && code == 1) {
            if (!o->game_screen_exists(o->context)) {
                o->wait_cursor(o->context);
                o->open_single_player_menu(o->context);
                o->show_screen(o->context, "Single Player Menu");
            } else {
                if (o->paused_by_settings(o->context) && !o->game_flag_a00(o->context))
                    o->resume(o->context);
                o->show_screen(o->context, "Game Screen");
            }
            o->close_self(o->context);
            return 1;
        }
    }
    return o->base_action(o->context, from, code);
}

/* ---- 0x00485c90: the screen as panels (game_ui.h) ---- */

#define LG(p) ((AoeLoadGameUiRor *)(p))

static const char *const UI_NAME = "Load Saved Game Screen";

static void lg_drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) lg_drop((AoeUiPanelRor **)&(x))

/* The state's ops on this screen: the list (the thunk of 0x0046a6d0, the
   +0x15c sorted flag, 0x0046a150) and the game UI's saved-game directory
   and file search. The teardown ops stay NULL: the panels are this
   screen's own (lg_destroy). */
static void lg_list_clear(void *c) {
    AoeLoadGameUiRor *l = c;
    if (l->list) aoe_ui_text_clear_ror(&l->list->text);
}
static void lg_list_sorted(void *c, int32_t on) {
    AoeLoadGameUiRor *l = c;
    if (l->list) aoe_ui_text_set_sorted_ror(&l->list->text, on);
}
static void lg_list_append(void *c, const char *text) {
    AoeLoadGameUiRor *l = c;
    if (l->list) aoe_ui_text_append_ror(&l->list->text, text, 0);
}
static const char *lg_save_dir(void *c) {
    const AoeGameUiOpsRor *o = LG(c)->ui->ops;
    return o && o->game_dir ? o->game_dir(o->context) : "";
}
static void lg_find_files(void *c, const char *pattern, void (*each)(void *arg, const char *name),
                          void *arg) {
    const AoeGameUiOpsRor *o = LG(c)->ui->ops;
    if (o && o->list_files) o->list_files(o->context, pattern, each, arg);
}

/* 0x00485ff0: the seven panels in order, then the base. */
static void lg_destroy(AoeUiPanelRor *p) {
    AoeLoadGameUiRor *l = LG(p);
    DROP(l->title);
    DROP(l->label);
    DROP(l->list);
    DROP(l->list_bar);
    DROP(l->ok);
    DROP(l->cancel);
    DROP(l->remove);
    aoe_ui_ez_release_ror(&l->ez);
}

static const char *const LG_CONFIRM = "Cancel Dialog";

/* The screen's own panels as the senders the original compares (+0x480
   list, +0x488 OK, +0x48c Cancel, +0x490 Delete); each starts with its
   panel, and a panel that failed is NULL and matches nothing. */
#define LG_PANEL(x) ((AoeUiPanelRor *)(void *)(x))

/* The list's selected line (thunk of 0x0046a600), "" with none. */
static const char *lg_selected(const AoeLoadGameUiRor *l) {
    const char *t = l->list ? aoe_ui_text_selected_text_ror(&l->list->text) : NULL;
    return t ? t : "";
}

static void lg_delete_file(AoeLoadGameUiRor *l, const char *ext) {
    const AoeGameUiOpsRor *o = l->ui->ops;
    char path[0x200];
    snprintf(path, sizeof path, "%s%s.%s", lg_save_dir(l), lg_selected(l), ext);
    if (o && o->delete_file) o->delete_file(o->context, path); /* 0x526258 */
}

/* 0x00486200 (+0xb8) */
int32_t aoe_load_game_ui_action_ror(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code,
                                    int32_t value, int32_t extra) {
    AoeLoadGameUiRor *l = LG(p);
    AoeUiSystemRor *sys = l->ui->system;
    const AoeGameUiOpsRor *o = l->ui->ops;
    if (!from) return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
    /* The sender's name (0x00454170). */
    if (from->name && strcmp(from->name, LG_CONFIRM) == 0) {
        aoe_ui_system_close_ror(sys, LG_CONFIRM); /* 0x00451df0 */
        if (code == 0) {
            lg_delete_file(l, "gam");
            lg_delete_file(l, "gmx");
            /* 0x0046a550 the selected line, removed by 0x0046a480. */
            if (l->list)
                aoe_ui_text_remove_ror(&l->list->text,
                                       aoe_ui_text_selection_ror(&l->list->text));
            if (*lg_selected(l) == '\0') {
                if (l->ok) aoe_ui_button_set_disabled_ror(l->ok, 1);       /* 0x0045f3a0 */
                if (l->remove) aoe_ui_button_set_disabled_ror(l->remove, 1);
            }
        }
        return 1;
    }
    if (from == LG_PANEL(l->remove) && code == 1 && *lg_selected(l)) {
        char fmt[0x100], text[0x22c];
        aoe_ui_string_ror(sys, 0x24ca, fmt, (int)sizeof fmt); /* 0x00454270 */
        snprintf(text, sizeof text, fmt, lg_selected(l));
        aoe_ui_ez_yes_no_dialog_ror(&l->ez, text, LG_CONFIRM, 0x1c2, 100); /* 0x00457a80 */
    }
    if (from == LG_PANEL(l->list)) {
        /* A double click acts as OK, through the action slot. */
        if (code == 3) {
            p->vt->action(p, LG_PANEL(l->ok), 1, 0, 0);
            return 1;
        }
    } else if (from == LG_PANEL(l->ok)) {
        if (code == 1) {
            if (l->list && aoe_ui_text_count_ror(&l->list->text) > 0) { /* 0x0046a0b0 */
                char name[0x104], path[0x200], file[0x200];
                const AoeLoadGameActionOpsRor *a = l->action_ops;
                snprintf(name, sizeof name, "%s", lg_selected(l));
                snprintf(path, sizeof path, "%s%s.gmx", lg_save_dir(l), name);
                /* 0x52694c: the .gam when there is no .gmx. */
                if (o && o->file_exists && o->file_exists(o->context, path))
                    snprintf(file, sizeof file, "%s.gmx", name);
                else
                    snprintf(file, sizeof file, "%s.gam", name);
                /* 0x00502d60; a started game may have closed this screen. */
                if (!a || !a->load_game || !a->load_game(a->context, file))
                    aoe_ui_ez_ok_dialog_id_ror(&l->ez, 0x2501, NULL, 0x1c2, 100); /* 0x004578d0 */
            }
            return 1;
        }
    } else if (from == LG_PANEL(l->cancel) && code == 1) {
        if (aoe_ui_system_find_ror(sys, "Game Screen")) { /* 0x00451ae0 */
            /* 0x41b750 == 1 with game +0xa00 clear: 0x419a60(0, 0). */
            if (o && o->resume_game) o->resume_game(o->context);
            aoe_ui_system_show_named_ror(sys, "Game Screen", 0); /* 0x00451be0 */
        } else {
            if (o && o->wait_cursor) o->wait_cursor(o->context); /* 0x41bd60 */
            /* 0x49e040, made and registered. */
            if (o && o->open_single_player_menu) o->open_single_player_menu(o->context);
            aoe_ui_system_show_named_ror(sys, "Single Player Menu", 0);
        }
        aoe_ui_system_close_ror(sys, UI_NAME);
        return 1;
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra); /* 0x00457e50 */
}

int aoe_load_game_ui_init_ror(AoeLoadGameUiRor *l, AoeGameUiRor *ui, AoeUiAreaRor *area) {
    AoeUiEzRor *ez = &l->ez;
    const AoeGameUiOpsRor *o = ui->ops;
    AoeUiPanelRor *tab[4];
    const char *name;
    int32_t index;
    memset(l, 0, sizeof *l);
    l->ui = ui;
    l->state_ops.context = l;
    l->state_ops.list_clear = lg_list_clear;
    l->state_ops.list_set_sorted = lg_list_sorted;
    l->state_ops.list_append = lg_list_append;
    l->state_ops.save_dir = lg_save_dir;
    l->state_ops.find_files = lg_find_files;
    l->state.ops = &l->state_ops;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, UI_NAME);
    ez->panel.vt = &aoe_load_game_ui_vtable_ror.base;
    /* 0x00451ab0 reads the system's current screen; the value is unused.
       0x00468180 with the screen's own info name (+0xfc) and ID (+0x200). */
    if (!aoe_ui_screen_setup_ror(ez, area, ez->info_name, ez->info_id, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_shadow_area_ror(ez, 0);
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &l->title, 0x24ff, 0x14, 10, 600, 0x1e, 1, 1,
                                      0, 0) ||
        !aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &l->label, 0x2502, 0x14, 0x2d, 600, 0x14, 4,
                                      0, 0, 0) ||
        !aoe_ui_ez_create_list_ror(ez, &ez->panel, &l->list, 0x14, 0x41, 600, 0x160, 0xb) ||
        !aoe_ui_ez_create_text_scrollbar_ror(ez, &l->list_bar, &l->list->text, 0x14))
        return 1;
    aoe_ui_panel_set_help_ror(&l->list_bar->panel, 0x7670, -1);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &l->ok, 0xfa1, 0, 0x1e, 0x1b8, 0xb4, 0x1e,
                                        0, 0, 0) ||
        !aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &l->remove, 0x24c9, 0, 0xe6, 0x1b8, 0xb4,
                                        0x1e, 0, 0, 0) ||
        !aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &l->cancel, 0xfa2, 0, 0x1ae, 0x1b8, 0xb4,
                                        0x1e, 0, 0, 0))
        return 1;
    aoe_ui_panel_set_help_ror(&l->ok->panel, 0x7531, -1);
    aoe_ui_panel_set_help_ror(&l->cancel->panel, 0x7532, -1);
    l->cancel->hotkey = 0x1b;
    l->cancel->hotkey_shift = 0;
    l->remove->hotkey = 0x2e;
    l->remove->hotkey_shift = 0;
    l->state.panels[0] = l->title;
    l->state.panels[1] = l->label;
    l->state.panels[2] = l->list;
    l->state.panels[3] = l->list_bar;
    l->state.panels[4] = l->ok;
    l->state.panels[5] = l->cancel;
    l->state.panels[6] = l->remove;
    aoe_load_game_screen_fill_ror(&l->state); /* 0x004860a0 */
    if (aoe_ui_text_selected_text_ror(&l->list->text)[0] == '\0') {
        aoe_ui_button_set_disabled_ror(l->ok, 1);
        aoe_ui_button_set_disabled_ror(l->remove, 1);
    }
    /* The saved-game name (0x005001a0) selected and scrolled to when listed. */
    name = o && o->default_save_name ? o->default_save_name(o->context) : NULL;
    index = name ? aoe_ui_text_index_of_ror(&l->list->text, name) : -1;
    if (index != -1) aoe_ui_list_move_ror(l->list, 1, (int16_t)index, 1);
    aoe_ui_panel_set_current_ror(&ez->panel, &l->list->text.panel);
    tab[0] = &l->list->text.panel;
    tab[1] = &l->ok->panel;
    tab[2] = &l->remove->panel;
    tab[3] = &l->cancel->panel;
    aoe_ui_panels_tab_cycle_ror(tab, 4);
    return 1;
}

const AoeUiEzVtblRor aoe_load_game_ui_vtable_ror = {
    AOE_UI_EZ_SLOTS_MDU(lg_destroy, aoe_ui_ez_draw_slot_ror, aoe_ui_game_idle_slot_ror,
                        aoe_ui_panel_handle_paint_ror, aoe_ui_ez_no_key_action_ror,
                        aoe_ui_panel_zero_ror, aoe_load_game_ui_action_ror,
                        aoe_ui_screen_focus_slot_ror,
                        aoe_ui_ez_mouse_down_slot_ror, aoe_ui_panel_ignore_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
