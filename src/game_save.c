#include "game_save.h"

#include <string.h>

/* 0x00501250 */
int32_t aoe_game_save_ror(const AoeGameSaveOpsRor *o, const char *name) {
    char base[0x108], *dot;
    size_t n;
    uint8_t ok;
    if (o->mode != 4 && o->mode != 6 && o->mode != 7 && o->mode != 5) return 0;
    o->cursor(o->context, 1);
    o->status(o->context, 0x453);
    ok = o->world_save(o->context, name);
    *o->global_558094 = 1;
    if (ok == 0) {
        o->status_close(o->context);
        o->cursor(o->context, 0);
        return 0;
    }
    n = strlen(name);
    if (n > sizeof base - 1) n = sizeof base - 1;
    memcpy(base, name, n);
    base[n] = '\0';
    dot = strchr(base, '.');
    if (dot) *dot = '\0';
    o->set_saved_name(o->context, base);
    if (o->single_player(o->context) == 1) o->unpause(o->context);
    o->show_screen(o->context, "Game Screen");
    o->remove_screen(o->context, "Save Game Screen");
    o->remove_screen(o->context, "Status Screen");
    o->cursor(o->context, 0);
    return 1;
}
