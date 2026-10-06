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
