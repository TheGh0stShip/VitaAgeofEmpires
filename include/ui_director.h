#ifndef VITA_AOE_UI_DIRECTOR_H
#define VITA_AOE_UI_DIRECTOR_H

#include <stdint.h>

#include "game_ui.h"
#include "ui_host.h"

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
     - quitting the program sets `quit` for the platform loop. */
typedef struct {
    AoeUiHostRor *host;
    AoeGameUiRor ui;
    AoeGameUiOpsRor ops;
    AoeGameSettingsRor settings;
    int quit;
    uint32_t opened;           /* screens opened, for diagnostics */
} AoeUiDirectorRor;

/* Ties the director to an open host. */
void aoe_ui_director_init_ror(AoeUiDirectorRor *d, AoeUiHostRor *host);
/* Builds and shows the Main Menu (as the game's start does). 0 when it
   cannot be built. */
int aoe_ui_director_start_ror(AoeUiDirectorRor *d);
/* The current screen's registered name ("" when none). */
const char *aoe_ui_director_screen_name_ror(const AoeUiDirectorRor *d);

#endif
