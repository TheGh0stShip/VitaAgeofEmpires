/* The game-side screen switching for the translated screens. */
#include "ui_director.h"

#include <stdlib.h>
#include <string.h>

#define DIR(c) ((AoeUiDirectorRor *)(c))

/* Each opener constructs its screen with calloc (the screens are heap
   objects the panel system frees on close) and keeps it registered. */
#define OPENER(fn, Type, init, ...)                                          \
    static void fn(void *c) {                                                \
        AoeUiDirectorRor *d = DIR(c);                                        \
        Type *s = calloc(1, sizeof *s);                                      \
        if (!s) return;                                                      \
        if (!init(s, &d->ui, &d->host->area, ##__VA_ARGS__)) {               \
            free(s);                                                         \
            return;                                                          \
        }                                                                    \
        d->opened++;                                                         \
    }

static int build_main_menu(AoeMainMenuRor *m, AoeGameUiRor *ui, AoeUiAreaRor *area,
                           const AoeUiPictureOpsRor *pictures) {
    return aoe_main_menu_init_ror(m, ui, area, pictures);
}

OPENER(open_main_menu_impl, AoeMainMenuRor, build_main_menu, &DIR(c)->host->picture_ops)
OPENER(open_single_player_menu, AoeSinglePlayerMenuRor, aoe_single_player_menu_init_ror)
OPENER(open_editor_menu, AoeScenarioEditorMenuRor, aoe_scenario_editor_menu_init_ror)
OPENER(open_mp_startup, AoeMpStartupScreenRor, aoe_mp_startup_screen_init_ror)
OPENER(open_credits, AoeCreditsScreenRor, aoe_credits_screen_init_ror)

static void open_main_menu(void *c) { open_main_menu_impl(c); }

static int cd_check(void *c) { (void)c; return 1; }
static void reset_input(void *c) { (void)c; }
static void wait_cursor(void *c) { (void)c; }
static void quit_program(void *c) { DIR(c)->quit = 1; }
static uint32_t now_ms(void *c) {
    AoeUiHostRor *h = DIR(c)->host;
    return h->now_ms ? h->now_ms(h->now_context) : 0;
}

void aoe_ui_director_init_ror(AoeUiDirectorRor *d, AoeUiHostRor *host) {
    memset(d, 0, sizeof *d);
    d->host = host;
    d->ops.context = d;
    d->ops.cd_check = cd_check;
    d->ops.reset_input = reset_input;
    d->ops.wait_cursor = wait_cursor;
    d->ops.quit_program = quit_program;
    d->ops.now_ms = now_ms;
    d->ops.open_main_menu = open_main_menu;
    d->ops.open_single_player_menu = open_single_player_menu;
    d->ops.open_editor_menu = open_editor_menu;
    d->ops.open_mp_startup = open_mp_startup;
    d->ops.open_credits = open_credits;
    d->ui.system = &host->system;
    d->ui.ez_ops = &host->ez_ops;
    d->ui.settings = &d->settings;
    d->ui.ops = &d->ops;
    d->ui.picture_ops = &host->picture_ops;
}

int aoe_ui_director_start_ror(AoeUiDirectorRor *d) {
    open_main_menu(d);
    return aoe_ui_system_show_named_ror(&d->host->system, "Main Menu", 0);
}

const char *aoe_ui_director_screen_name_ror(const AoeUiDirectorRor *d) {
    const AoeUiPanelRor *s = d->host->system.screen;
    return s && s->name ? s->name : "";
}
