#ifndef VITA_AOE_GAME_SCREEN_HOST_H
#define VITA_AOE_GAME_SCREEN_HOST_H

#include <stdint.h>

#include "demo_app.h"
#include "game_screen.h"
#include "game_ui.h"
#include "command_button.h"
#include "game_minimap.h"
#include "info_panel.h"
#include "resource_panel.h"

/* The port's "Game Screen": the panel the game start (0x503a70)
   registers, assembled from what is integrated so far. Integration
   support, not a translation of the constructor 0x0047d440: it is an
   easy screen set up as the original's (0x00468180 with info 50056,
   which gives palette 50500 and pointer 51000, no background), with the
   world view at the layout's rectangle for the screen size (0x0047f000:
   (0, 0x14, 640, 0x14e) at 640x480) drawn by the reconstructed scene
   renderer, the world advanced on idle, and the view scrolled by the
   arrow keys or the pointer at a screen edge. The viewed player's
   interface (0x004816d0) loads its style's bar shape, which the
   translated draw (0x00484110) paints behind the view. The resource bar
   (0x004f7c00), the minimap (0x005085a0) and the object panel
   (0x004f83d0) are the translated panels over the world, with the
   seventeen command buttons (0x004f7420) rebuilt for the selection
   (0x004822a0) and run through the dispatcher (0x00481410). The screen
   is laid out by the translated layout (0x0047f000). Keys: Escape (0x00484900) clears the selection, F10
   (0x004850b0) opens the Menu Dialog; the dialogs' answers go through
   the screen's action (0x00480fe0). Leaving a game returns to the Main
   Menu (the original plays the end video and shows the Achievements
   Screen first, 0x500740 case 4). */
typedef struct {
    AoeUiEzRor ez;
    AoeGameUiRor *ui;
    AoeGameScreenRor screen;      /* the translated screen's state */
    void *shapes[AOE_GSP_SHAPES]; /* +0x478..+0x48c */
    void *page_picture;           /* +0x4ac */
    int32_t style, style_width;   /* +0x714, +0x718 */
    int32_t game_state;           /* game +0x1b0: 4 once started (0x005046e0) */
    AoeGameScreenOpsRor screen_ops;   /* the translated methods' calls */
    AoeUiPanelRor *action_from;       /* the sender being answered */
    int32_t field_730;                /* +0x730: resigned */
    AoeResourcePanelRor *resources;   /* +0x4c0 */
    AoeGameMinimapRor *map;           /* +0x4bc */
    AoeInfoPanelRor *object_panel;    /* +0x4c4 */
    AoeInfoPanelOpsRor object_ops;
    void *unit_icons;                 /* AOE_GSC_SHAPE 0: "btnunit.shp" (+0x494) */
    void *tech_icons;                 /* AOE_GSC_SHAPE 1: "btntech.shp" (+0x490) */
    AoeCommandButtonRor *buttons[17]; /* +0x4c8.. */
    AoeCommandButtonOpsRor button_ops;
    int16_t button_count, button_page, button_shown; /* +0x5e0, +0x5e2, +0x5e4 */
    int16_t frame_4b0, frame_4b2, frame_4b4;
    void *train_list;                 /* +0x5d4 (AoeTribeTrainEntryRor) */
    int16_t train_count;
    void *research_list;              /* +0x5d0 (AoeResearchListEntryRor) */
    int16_t research_count;
    void *building_icons[5];          /* AOE_GSC_SHAPE 3 + i: "btnbldg%d.shp" */
    uint32_t map_time;                /* +0x5c4 */
    AoeDiamondMapOpsRor map_ops;
    AoeGameMinimapOpsRor map_game_ops;
    /* A resource's bytes (malloc'd) by type and ID, or NULL. */
    uint8_t *(*resource)(void *context, uint32_t type, int32_t id, uint32_t *size);
    void *resource_context;
    AoeResourcePlayerRor resource_player;
    AoeDemo *world;               /* the director's; not owned */
    int32_t view_x, view_y, view_w, view_h;
    uint8_t *pixels;              /* view_w x view_h */
    int rendered;                 /* `pixels` hold a frame */
    uint32_t last_ms;
    int32_t mouse_x, mouse_y;     /* screen coordinates, -1 before any move */
    int16_t place_unit;           /* game +0x3fc: the unit to place, -1 none */
    int32_t edit_mode, edit_sub;  /* game +0x1b4 / +0x1b8: 0x15 places a building */
} AoeGameScreenHostRor;

/* The view rectangle for a screen size (0x0047f000 without the
   overlay). */
void aoe_game_screen_host_view_rect_ror(int32_t width, int32_t height, int32_t rect[4]);
/* Constructs and registers the screen over `area`; 0 when it cannot be
   set up (the caller frees it). `world` must be open with a view of the
   rectangle's size and hold the started scenario. */
int aoe_game_screen_host_init_ror(AoeGameScreenHostRor *s, AoeGameUiRor *ui, AoeUiAreaRor *area,
                                  AoeDemo *world,
                                  uint8_t *(*resource)(void *context, uint32_t type, int32_t id,
                                                       uint32_t *size),
                                  void *resource_context);
/* An 8-bit BMP (file header, BITMAPINFOHEADER, palette, rows bottom-up
   padded to four bytes) as top-down pixels (malloc'd), or NULL. */
uint8_t *aoe_game_screen_host_bmp_ror(const uint8_t *data, uint32_t size, int32_t *width,
                                      int32_t *height);

#endif
