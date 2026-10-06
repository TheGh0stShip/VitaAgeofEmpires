#ifndef VITA_AOE_GAME_SCREEN_HOST_H
#define VITA_AOE_GAME_SCREEN_HOST_H

#include <stdint.h>

#include "demo_app.h"
#include "game_screen.h"
#include "game_ui.h"

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
   translated draw (0x00484110) paints behind the view. The panels on the
   bars (resources, minimap, object panel, command buttons) are not
   assembled yet. */
typedef struct {
    AoeUiEzRor ez;
    AoeGameUiRor *ui;
    AoeGameScreenRor screen;      /* the translated screen's state */
    void *shapes[AOE_GSP_SHAPES]; /* +0x478..+0x48c */
    void *page_picture;           /* +0x4ac */
    int32_t style, style_width;   /* +0x714, +0x718 */
    AoeDemo *world;               /* the director's; not owned */
    int32_t view_x, view_y, view_w, view_h;
    uint8_t *pixels;              /* view_w x view_h */
    uint32_t last_ms;
    int32_t mouse_x, mouse_y;     /* screen coordinates, -1 before any move */
} AoeGameScreenHostRor;

/* The view rectangle for a screen size (0x0047f000 without the
   overlay). */
void aoe_game_screen_host_view_rect_ror(int32_t width, int32_t height, int32_t rect[4]);
/* Constructs and registers the screen over `area`; 0 when it cannot be
   set up (the caller frees it). `world` must be open with a view of the
   rectangle's size and hold the started scenario. */
int aoe_game_screen_host_init_ror(AoeGameScreenHostRor *s, AoeGameUiRor *ui, AoeUiAreaRor *area,
                                  AoeDemo *world);

#endif
