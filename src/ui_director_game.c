/* The director's game callbacks: the campaign scenario's players, the
   scenario game set up and started (0x502910, 0x500740 case 3,
   0x503a70) onto the port's Game Screen. Integration support. */
#include "ui_director.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game_screen_host.h"

#define DIR(c) ((AoeUiDirectorRor *)(c))

static void say(AoeUiDirectorRor *d, const char *what, const char *detail) {
    snprintf(d->status, sizeof d->status, "%.60s%s%.180s", what, detail ? ": " : "", detail ? detail : "");
}

/* 0x005011e0: the world made once and kept (it holds the DAT and art);
   its view is the Game Screen's for the host's screen size. */
static int ensure_world(AoeUiDirectorRor *d) {
    int32_t r[4];
    char error[256];
    if (d->world) return 1;
    aoe_game_screen_host_view_rect_ror(d->host->area.width, d->host->area.height, r);
    d->world = aoe_demo_open_view(d->host->root, NULL, (uint32_t)r[2], (uint32_t)r[3], 0, error,
                                  sizeof error);
    if (!d->world) say(d, "The world could not be made", error);
    return d->world != NULL;
}

/* The campaign's scenario stream (0x0041c700): the slice of the campaign
   file copied to <user dir>/campaign.scx. Handle 0, or -1. */
static int32_t open_slice(void *c, const char *path, int32_t offset, int32_t size) {
    AoeUiDirectorRor *d = DIR(c);
    FILE *in, *out;
    uint8_t *data;
    int ok;
    if (offset < 0 || size <= 0 || size > (64 << 20)) return -1;
    in = fopen(path, "rb");
    if (!in) return -1;
    data = malloc((size_t)size);
    ok = data && fseek(in, offset, SEEK_SET) == 0 && fread(data, 1, (size_t)size, in) == (size_t)size;
    fclose(in);
    snprintf(d->scenario_path, sizeof d->scenario_path, "%s/campaign.scx", d->user_dir);
    out = ok ? fopen(d->scenario_path, "wb") : NULL;
    ok = out && fwrite(data, 1, (size_t)size, out) == (size_t)size;
    if (out && fclose(out) != 0) ok = 0;
    free(data);
    return ok ? 0 : -1;
}

static void free_scenario(AoeUiDirectorRor *d) {
    if (d->scenario_loaded) scn_free_map(&d->scenario);
    memset(&d->scenario, 0, sizeof d->scenario);
    d->scenario_loaded = 0;
}

/* 0x417670(0, 1): the chosen campaign scenario's settings object read
   from its stream. */
static int load_campaign_scenario(AoeUiDirectorRor *d) {
    char **names = NULL;
    int32_t chosen = -1, n, i;
    free_scenario(d);
    if (!ensure_world(d)) return 0;
    if (aoe_pif_open_ror(&d->pif, &d->campaign_ops) == -1) {
        say(d, "The campaign scenario could not be opened", NULL);
        return 0;
    }
    if (!scn_load_map(d->scenario_path, aoe_demo_terrain_count(d->world), &d->scenario)) {
        memset(&d->scenario, 0, sizeof d->scenario);
        say(d, "The campaign scenario could not be read", d->scenario_path);
        return 0;
    }
    d->scenario_loaded = 1;
    n = aoe_pif_scenario_names_ror(&d->pif, &names, &chosen);
    snprintf(d->scenario_name, sizeof d->scenario_name, "%s",
             chosen >= 0 && chosen < n && names[chosen] ? names[chosen] : "campaign.scx");
    for (i = 0; i < n; ++i) free(names[i]);
    free(names);
    return 1;
}

/* 0x00476250 / 0x00476210 for slots 0..8: the active and kind (1 human)
   tables of the settings object, which its reader fills from each
   player's first two values (scn's `enabled` and `is_human`). */
static int campaign_scenario_slots(void *c, int32_t active[9], int32_t kind[9]) {
    AoeUiDirectorRor *d = DIR(c);
    int i;
    if (!load_campaign_scenario(d)) return 0;
    for (i = 0; i < 9; ++i) {
        active[i] = (int32_t)d->scenario.player_setup[i].enabled;
        kind[i] = (int32_t)d->scenario.player_setup[i].is_human;
    }
    return 1;
}

static void set_slot_state(void *c, int32_t slot, int32_t state) {
    if (slot >= 0 && slot < 10) DIR(c)->slot_state[slot] = state;
}

static const char *scenario_title(void *c) {
    AoeUiDirectorRor *d = DIR(c);
    return d->scenario_loaded ? d->scenario.texts[SCN_TEXT_DESCRIPTION] : NULL;
}

static const char *scenario_message(void *c, int32_t i) {
    AoeUiDirectorRor *d = DIR(c);
    if (!d->scenario_loaded || i < 0 || i > 4) return NULL;
    return d->scenario.texts[SCN_TEXT_MESSAGE + i];
}

/* 0x503a70: the Game Screen made; then it is shown, the Status Screen
   and the menu screens removed and (outside mode 3) the game started
   (0x005046e0: its log and music, not integrated). */
static void start_game(void *c) {
    static const char *const menus[] = {
        "Single Player Menu", "Game Setup Screen", "Select Scenario Screen",
        "Game Settings Screen", "Load Saved Game Screen", "MP Setup Screen", "Join Screen",
        "MP Startup Screen", "Main Menu", "Campaign Selection Screen"};
    AoeUiDirectorRor *d = DIR(c);
    AoeUiSystemRor *system = &d->host->system;
    AoeGameScreenHostRor *s;
    size_t i;
    if (!d->world) return;
    aoe_ui_system_close_ror(system, "Game Screen");
    s = calloc(1, sizeof *s);
    if (!s) return;
    if (!aoe_game_screen_host_init_ror(s, &d->ui, &d->host->area, d->world)) {
        s->ez.panel.vt->destroy(&s->ez.panel);
        free(s);
        say(d, "The Game Screen could not be made", NULL);
        return;
    }
    d->opened++;
    aoe_ui_system_show_named_ror(system, "Game Screen", 0);
    aoe_ui_system_close_ror(system, "Status Screen");
    for (i = 0; i < sizeof menus / sizeof *menus; ++i) aoe_ui_system_close_ror(system, menus[i]);
}

/* 0x500740 case 3, after the scenario's video: with instructions
   (message 0, 0x00475b40), not networked, and a scenario or campaign
   game (settings +0x8fc clear, or +0x9a0), the Mission Dialog
   (0x0049ec00 with the briefing picture; the port has none yet) is shown;
   else the game starts. */
static void after_video(AoeUiDirectorRor *d) {
    const char *text = scenario_message(d, 0);
    if (text && *text && !d->ops.networked(d) &&
        (aoe_settings_flag_8fc_clear_ror(&d->settings) || aoe_settings_int_9a0_ror(&d->settings))) {
        AoeMissionDialogRor *m = calloc(1, sizeof *m);
        if (m && aoe_mission_dialog_init_ror(m, &d->ui, &d->host->area, text, 0, NULL)) {
            d->opened++;
            aoe_ui_system_show_named_ror(&d->host->system, "Mission Dialog", 0);
            return;
        }
        free(m);
    }
    start_game(d);
}

/* 0x502910(0): the world ensured (status 0x44d), the game set up
   (status 0x451, 0x00503100: the scenario built and started in the
   world), the restart names (0x004ff4b0 / 0x004ff4f0, not integrated),
   then the scenario's video (0x005003f0(3, +0x1508)), which the port does
   not have, so its end (0x500740) follows at once. */
static int start_scenario_game(void *c) {
    AoeUiDirectorRor *d = DIR(c);
    if (!ensure_world(d)) return 0;
    if (!d->scenario_loaded && !load_campaign_scenario(d)) return 0;
    if (!aoe_demo_load_file(d->world, d->scenario_path, d->scenario_name)) {
        say(d, "The scenario could not be started", aoe_demo_status(d->world));
        return 0;
    }
    /* The player's start (0x004f1ae0, with units): the view on its town
       center, else its first unit. The port plays as player 1. */
    aoe_demo_view_player_start(d->world, 1);
    after_video(d);
    return 1;
}

void aoe_ui_director_game_init_ror(AoeUiDirectorRor *d) {
    d->campaign_ops.open_slice = open_slice;
    d->ops.campaign_scenario_slots = campaign_scenario_slots;
    d->ops.set_slot_state = set_slot_state;
    d->ops.start_scenario_game = start_scenario_game;
    d->ops.scenario_title = scenario_title;
    d->ops.scenario_message = scenario_message;
    d->ops.start_game = start_game;
}

void aoe_ui_director_game_release_ror(AoeUiDirectorRor *d) {
    free_scenario(d);
    aoe_demo_close(d->world);
    d->world = NULL;
}
