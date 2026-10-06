#ifndef VITA_AOE_UI_DIRECTOR_H
#define VITA_AOE_UI_DIRECTOR_H

#include <stdint.h>

#include "game_ui.h"
#include "ui_host.h"
#include "scenario_editor_player.h"
#include "campaign_progress.h"
#include "port_registry.h"
#include "demo_app.h"
#include "scn.h"

/* The game-side callbacks the translated screens use to move between
   each other (AoeGameUiOpsRor's open_* entries): each one constructs the
   translated screen on the host's area, which registers itself; the
   calling screen then shows it and closes itself, as in the original.
   Integration support. Screens whose callbacks need game systems that
   are not integrated yet are not offered (the callback stays NULL and
   the calling screen does what the original does without it). Choices
   the port makes:
     - the CD check passes: the port reads the user's installed data in
       place of the disc;
     - quitting the program sets `quit` for the platform loop;
     - the scenario index (0x00476f10) lists the data root's scenario/
       folder (*.scn, then *.scx, each in name order as NTFS returns
       them) and reads every file's record on each refresh; the original
       keeps an entry whose stored stamp is not older than the file;
     - registry values live in <user dir>/registry.txt (port_registry.h);
     - a campaign scenario is read from the campaign file into
       <user dir>/campaign.scx (the original streams it from the file);
     - there are no videos: the scenario's intro video ends at once
       (0x005003f0 without the file);
     - the player file is game<N>.nfo in the user folder ("Game File
       Number", as the original's start-up picks it) and is saved after
       every change to it (the original saves when it is freed at exit;
       a console app may be closed at any time). */
/* One scenario of the index (0x00476f10): its file name and the RoR
   record read from the file (0x00506770). */
typedef struct {
    char name[260];
    AoeScenarioRecordExRor record;
    int has_record;
} AoeDirectorScenarioRor;

typedef struct {
    AoeUiHostRor *host;
    AoePortRegistryRor registry;
    /* The player information file (game%d.nfo, 0x0043c6b0) and the
       campaign file callbacks its loader uses. */
    AoePifRor pif;
    int pif_loaded;
    AoeCampaignOpsRor campaign_ops;
    char campaign_dir[720], scenario_dir[720];
    char user_dir[600];         /* registry.txt and game<N>.nfo */
    AoeDirectorScenarioRor *scenarios;
    int32_t scenario_count;
    AoeGameUiRor ui;
    AoeGameUiOpsRor ops;
    AoeGameSettingsRor settings;
    int quit;
    uint32_t opened;           /* screens opened, for diagnostics */
    /* The game (ui_director_game.c): the world (the scene renderer and
       world update with a view of the Game Screen's size, made when a
       game is first set up, as 0x005011e0 makes the world), the chosen
       scenario file and its settings object (players, texts), the comm
       slot states (0x00423d40) and the last failure. */
    AoeDemo *world;
    char scenario_path[720], scenario_name[260];
    ScnMap scenario;
    int scenario_loaded;
    int32_t slot_state[10];
    char status[256];
} AoeUiDirectorRor;

/* Ties the director to an open host. `user_dir` holds the files the game
   writes (registry.txt, game<N>.nfo); NULL for the data root. */
void aoe_ui_director_init_ror(AoeUiDirectorRor *d, AoeUiHostRor *host, const char *user_dir);
/* Frees the director's own data (the host stays open). */
void aoe_ui_director_release_ror(AoeUiDirectorRor *d);
/* The scenario index refresh (exposed for tests). */
void aoe_ui_director_refresh_scenarios_ror(AoeUiDirectorRor *d);
/* Builds and shows the Main Menu (as the game's start does). 0 when it
   cannot be built. */
int aoe_ui_director_start_ror(AoeUiDirectorRor *d);
/* The game callbacks (ui_director_game.c): set by init, released by
   release. */
void aoe_ui_director_game_init_ror(AoeUiDirectorRor *d);
void aoe_ui_director_game_release_ror(AoeUiDirectorRor *d);
/* The current screen's registered name ("" when none). */
const char *aoe_ui_director_screen_name_ror(const AoeUiDirectorRor *d);

#endif
