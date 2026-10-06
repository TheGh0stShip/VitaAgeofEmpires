#include "load_game_screen.h"

#include <stdio.h>
#include <string.h>

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
