/* Patched RoR 1.0a panel system: named panels, screens and modal panels
   0x00451ae0..0x00451f20. */
#include "ui_panel.h"

#include <assert.h>
#include <stdlib.h>

static int updates;
static void update(void *c) { (void)c; ++updates; }

static AoeUiPanelRor *named(AoeUiSystemRor *s, const char *name) {
    AoeUiPanelRor *p = calloc(1, sizeof(*p));
    assert(aoe_ui_panel_init_named_ror(p, s, name));
    return p;
}

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiPanelRor *menu, *game, *dlg, *parent, *other;

    pf.update_window = update;
    sys.platform = &pf;
    menu = named(&sys, "Main Menu");
    game = named(&sys, "Game Screen");
    assert(aoe_ui_system_find_ror(&sys, "Main Menu") == menu &&
           !aoe_ui_system_find_ror(&sys, "main menu") &&
           !aoe_ui_system_find_ror(&sys, NULL));
    assert(aoe_ui_system_has_ror(&sys, game) && !aoe_ui_system_has_ror(&sys, NULL));
    /* Registering again keeps one entry. */
    assert(aoe_ui_system_add_ror(&sys, menu, 1, 1) && sys.panel_count == 2);
    assert(sys.screen == menu && menu->has_focus && !menu->previous &&
           sys.modal == menu && updates == 1);

    /* A new screen remembers the old one, which loses focus. */
    aoe_ui_system_show_ror(&sys, game, 0);
    assert(sys.screen == game && game->previous == menu && !menu->has_focus &&
           game->has_focus && sys.modal == menu);
    aoe_ui_system_restore_modal_ror(&sys);
    assert(sys.modal == game && game->chained_modal == menu);

    /* Back without closing: the screens swap their previous links. */
    assert(aoe_ui_system_back_ror(&sys, 0));
    assert(sys.screen == menu && menu->previous == game && menu->has_focus);
    /* Back with closing: the current screen is destroyed. */
    sys.modal = menu;
    assert(aoe_ui_system_back_ror(&sys, 1));
    assert(sys.screen == game && !aoe_ui_system_find_ror(&sys, "Main Menu") &&
           !sys.modal && game->previous == NULL);

    /* Modal panels: the parent's focus moves to the dialog and back. */
    parent = calloc(1, sizeof(*parent));
    aoe_ui_panel_init_ror(parent, &sys);
    other = calloc(1, sizeof(*other));
    aoe_ui_panel_init_ror(other, &sys);
    dlg = named(&sys, "OKDialog");
    other->parent = parent;
    dlg->parent = parent;
    parent->current = other;
    sys.modal = game;
    aoe_ui_system_set_modal_ror(&sys, dlg);
    assert(sys.modal == dlg && dlg->chained_modal == game &&
           sys.saved_focus == other && parent->current == dlg);
    /* Popping returns to the chained modal panel. */
    assert(aoe_ui_system_pop_modal_ror(&sys) && sys.modal == game &&
           sys.keyboard == game && sys.capture == game && game->has_focus);
    /* Closing it by name clears the references and frees it. */
    sys.modal = dlg;
    aoe_ui_system_set_modal_ror(&sys, NULL);
    assert(!sys.modal && parent->current == other && !sys.saved_focus);
    assert(aoe_ui_system_close_ror(&sys, "OKDialog") && !aoe_ui_system_close_ror(&sys, "OKDialog"));
    assert(!aoe_ui_system_pop_modal_ror(&sys));

    assert(aoe_ui_system_remove_ror(&sys, "Game Screen") &&
           !aoe_ui_system_find_ror(&sys, "Game Screen") && sys.panel_count == 0);
    aoe_ui_panel_set_previous_ror(game, parent);
    assert(game->previous == parent && aoe_ui_panel_field4_ror(game) == parent);
    aoe_ui_panel_release_ror(game);
    free(game);
    aoe_ui_panel_release_ror(other);
    aoe_ui_panel_release_ror(parent);
    free(other);
    free(parent);
    free(sys.panels);
    return 0;
}
