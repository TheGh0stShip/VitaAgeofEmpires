#ifndef VITA_AOE_DEMO_APP_H
#define VITA_AOE_DEMO_APP_H

#include <stddef.h>
#include <stdint.h>

#include "diamond_map.h"
#include "info_panel.h"
#include "player_templates.h"
#include "research_action.h"

/* Portable tech-demo application: loads user-supplied original Rise of Rome
   data from a root directory, builds the reconstructed scenario runtime,
   runs the translated world update and draws the original-art scene into a
   32-bit framebuffer. The platform layer (Vita or host) owns the display,
   input and clock. This is a demonstration shell around the reconstruction,
   not a reconstruction of the original game's interface.

   Expected layout below the root (names are matched case-insensitively
   where the filesystem is case-sensitive):
     data/   base Age of Empires: graphics.drs, Terrain.drs, Border.drs,
             Interfac.drs, TILEEDGE.DAT and the *.col color tables
     data2/  Rise of Rome: EMPIRES.DAT, GRAPHICS.DRS, INTERFAC.DRS
     scenario/  optional *.scx / *.scn files
     campaign/  optional Rise of Rome *.cpx campaign files */

enum { AOE_DEMO_SELECTION = 25 };   /* the player's +0x138[25] */

typedef struct AoeLiveUnit AoeLiveUnit;

enum {
    AOE_DEMO_WIDTH = 960,
    AOE_DEMO_HEIGHT = 544
};

enum {
    AOE_DEMO_BUTTON_PREV = 1u << 0,      /* L: previous scenario */
    AOE_DEMO_BUTTON_NEXT = 1u << 1,      /* R: next scenario */
    AOE_DEMO_BUTTON_TRAIN = 1u << 2,     /* Cross: train a villager */
    AOE_DEMO_BUTTON_ATTACK = 1u << 3,    /* Square: army attacks */
    AOE_DEMO_BUTTON_CAMERA = 1u << 4,    /* Circle: next point of interest */
    AOE_DEMO_BUTTON_HUD = 1u << 5,       /* Triangle: toggle overlay */
    AOE_DEMO_BUTTON_PAUSE = 1u << 6,     /* Select: pause simulation */
    AOE_DEMO_BUTTON_QUIT = 1u << 7       /* Start */
};

typedef struct {
    float scroll_x, scroll_y; /* -1..1 screen-space scroll request */
    uint32_t pressed;         /* buttons newly pressed this frame */
} AoeDemoInput;

typedef struct AoeDemo AoeDemo;

/* Monotonic microsecond clock supplied by the platform. */
typedef uint64_t (*AoeDemoClock)(void);

/* Allocates the demo and loads the shared DAT/DRS data and scenario list.
   On failure returns NULL and writes a human-readable reason. */
AoeDemo *aoe_demo_open(const char *root, AoeDemoClock clock, char *error,
                       size_t error_size);
void aoe_demo_close(AoeDemo *demo);
/* As aoe_demo_open with a `width` x `height` view (aoe_demo_render_indexed
   then draws that size) and, when `scan_scenario_files` is 0, no scenario
   list: scenarios are then loaded by path (aoe_demo_load_file). The
   game screen's world view uses this. */
AoeDemo *aoe_demo_open_view(const char *root, AoeDemoClock clock,
                            uint32_t width, uint32_t height,
                            int scan_scenario_files, char *error,
                            size_t error_size);

size_t aoe_demo_scenario_count(const AoeDemo *demo);
/* Builds and starts scenario `index` (wrapping). Returns 0 on failure,
   leaving the reason in aoe_demo_status. */
int aoe_demo_load_scenario(AoeDemo *demo, size_t index);
/* Builds and starts the scenario file at `path` (`name` for messages).
   Returns 0 on failure, leaving the reason in aoe_demo_status. */
int aoe_demo_load_file(AoeDemo *demo, const char *path, const char *name);
/* The view at a player's start, as the player's start (0x004f1ae0) sets
   it for a player with units: on its type-0x6d (town center) object, else
   its first unit (in the runtime's unit order). 0 when it has none. */
int aoe_demo_view_player_start(AoeDemo *demo, int player);
/* A player's civilisation's interface style (the civilisation's +0x28,
   kept as the player's +0x54). 0 when there is no such player. */
int aoe_demo_player_icon_set(const AoeDemo *demo, int player, uint8_t *icon_set);
/* A player's attributes (player +0x4c count, +0x50 values: 0 food, 1
   wood, 2 stone, 3 gold, ...). 0 when there is no such player. */
int aoe_demo_player_attributes(const AoeDemo *demo, int player, int16_t *count,
                               const float **values);
/* The diamond map's world calls over the running scenario (the
   minimap's view of it): the map's tiles and terrain colours, the
   players (world players 0..count-1: the runtime's players) with their
   colour records' map colour, their objects (state 5 or below, the
   templates' map mode and colour), the tile features (a map-mode-4
   object's colour, found once per scenario: trees cut down later still
   colour their tile), the view's centre (the camera) and size in tiles
   (0x0050fde0: the view's width / tile width, height / tile height).
   Fog is not reconstructed: everything is visible and nobody is
   selected. Viewed player 1. load_bitmap forwards to `load_bitmap`
   with `bitmap_context`. */
void aoe_demo_minimap_ops(AoeDemo *demo, AoeDiamondMapOpsRor *ops,
                          uint8_t *(*load_bitmap)(void *context, const char *name, int32_t id,
                                                  int32_t *width, int32_t *height),
                          void *bitmap_context);
/* The unit drawn at view pixel (x, y) in the last frame: the drawn
   sprite parts (the view's sprite list in draw order, the original's
   rows) from the top down, the first non-transparent pixel winning, as
   the view's pick 0x00514d70 tests its rows. NULL for none. */
AoeLiveUnit *aoe_demo_pick(AoeDemo *demo, int32_t x, int32_t y);
/* View pixel to map tile coordinates under the last frame's camera: the
   inverse of the tile projection on flat ground, refined once by the
   found tile's elevation (the original finds ground points through its
   view's pick-mode draw). 0 off the map. */
int aoe_demo_screen_to_map(const AoeDemo *demo, int32_t sx, int32_t sy, float *mx, float *my);
/* The viewed player's selection: one unit (NULL clears), and the
   selected units. Selected units drawn get the selection box with its
   health bar (0x004a8e00). */
void aoe_demo_select(AoeDemo *demo, AoeLiveUnit *unit);
int aoe_demo_selection(const AoeDemo *demo, AoeLiveUnit **out, int max);
/* The selected units of player 1 ordered at view pixel (x, y): the unit
   drawn there as the target, else the ground point, through the order
   step 0x00405f20 (task selection: gather, attack or move). Returns the
   number ordered. */
int aoe_demo_order(AoeDemo *demo, int32_t sx, int32_t sy);
/* The view's area selection 0x005114d0 over the drawn sprites: the viewed
   player's (1) objects drawn in the view box (x0, y0)..(x1, y1)
   (0x00514b60 with rows 10..0x14, kind 4 and above) in a state below 7
   are added to the selection (0x0045dc10, up to 25), the previous one
   cleared first unless `add` (Shift); with none, the object at the box's
   centre (the original scores 0x005145a0's candidates within min(w, h)
   clamped to 50; the port takes the sprite under the centre). 1 when
   something was selected. */
int aoe_demo_select_area(AoeDemo *demo, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int add);
/* A rectangle outline in an indexed frame of this view's size. */
void aoe_demo_draw_box(const AoeDemo *demo, uint8_t *dst, size_t stride, int32_t x0, int32_t y0,
                       int32_t x1, int32_t y1, uint8_t colour);
/* What the object panel (0x004f83d0) reads of a unit (AoeInfoObjectRor:
   the template's fields by offset, the owner's colour, number and
   civilisation, the stat slots +0x230..+0x248 of the type 50 classes);
   construction, production queues, garrisons and the head action are
   not read yet. 0 for no unit. A world player as an identity (the
   object panel's and minimap's player handle), NULL for none. */
int aoe_demo_info_object(AoeDemo *demo, const AoeLiveUnit *unit, AoeInfoObjectRor *out);
const void *aoe_demo_world_player(AoeDemo *demo, int player);
/* A world player's civilisation (player +0x109), 0 for none. */
uint8_t aoe_demo_player_civ(AoeDemo *demo, const void *player);
/* The command buttons' world: a player's templates (its effects
   applied), its research owner (manager, techs and resources) and
   whether it owns an object of a template; the DAT's techs; a unit's ID;
   a tribe production command (0x64 train / 0x65 research, encoded with
   aoe_command_encode_production_ror) run in the world. */
const AoePlayerTemplates *aoe_demo_player_templates(AoeDemo *demo, int player);
int aoe_demo_research_owner(AoeDemo *demo, int player, AoeResearchOwnerRor *out);
int aoe_demo_owns_template(AoeDemo *demo, int player, int16_t template_id);
const DatTechs *aoe_demo_techs(const AoeDemo *demo);
int32_t aoe_demo_unit_id(const AoeLiveUnit *unit);
int aoe_demo_command_production(AoeDemo *demo, const uint8_t *command, size_t size);
/* Tribe command 0x66, building placement (aoe_world_combat_command_place_ror). */
int aoe_demo_command_place(AoeDemo *demo, const uint8_t *command, size_t size);
/* The template's +0x20 placement check for `player` at a map point, as
   the view's preview asks it (0x00508bb0: visible, the template's
   placement class as the shape mode, clearance, overlap mode, units):
   1 when the building may be placed there. */
int aoe_demo_can_place(AoeDemo *demo, int player, int16_t template_id, float x, float y);
/* A map point to view pixels under the last rendered camera (the tile's
   elevation lifts it as the renderer does). 0 before a render. */
int aoe_demo_map_to_screen(const AoeDemo *demo, float mx, float my, int32_t *sx, int32_t *sy);
/* Lines between the four footprint corners (screen offsets, l t r b
   pairs as 0x004433c0 gives them) about view pixel (sx, sy), drawn into an
   indexed frame of this view's size with the palette index `colour`. */
void aoe_demo_draw_footprint(const AoeDemo *demo, uint8_t *dst, size_t stride, int32_t sx,
                             int32_t sy, const int16_t corners[8], uint8_t colour);
/* 0x004433c0 with this map's tile geometry: a template's footprint corners
   about the tile holding (fx, fy). */
int aoe_demo_footprint_corners(const AoeDemo *demo, const DatUnit *unit_template, float fx,
                               float fy, int16_t out[8]);
/* The camera (map tile coordinates). */
void aoe_demo_camera(const AoeDemo *demo, float *x, float *y);
/* The DAT's terrain count (scn_load_map's `terrain_count`). */
uint8_t aoe_demo_terrain_count(const AoeDemo *demo);

/* Applies input, scrolls the camera and advances the simulation by `dt`
   seconds of real time. Returns 0 when the user asked to quit. */
int aoe_demo_update(AoeDemo *demo, const AoeDemoInput *input, float dt);

/* Draws the scene and overlay into an A8B8G8R8 (little-endian: R in the
   low byte) framebuffer of AOE_DEMO_WIDTH x AOE_DEMO_HEIGHT pixels with a
   row stride of `stride` pixels. Returns 0 on failure. */
int aoe_demo_render(AoeDemo *demo, uint32_t *framebuffer, size_t stride);

const char *aoe_demo_status(const AoeDemo *demo);

/* GPU-composited platforms: draws only the indexed scene (the view's
   size) into `dst` (row stride in bytes); the current 256-entry A8B8G8R8 palette (including the
   cycling water entries) and the overlay description are fetched
   separately. */
int aoe_demo_render_indexed(AoeDemo *demo, uint8_t *dst, size_t stride);
const uint32_t *aoe_demo_palette_abgr(const AoeDemo *demo);

enum { AOE_DEMO_OVERLAY_TEXTS = 8, AOE_DEMO_OVERLAY_RECTS = 4 };
typedef struct {
    int x, y, scale;       /* top-left; scale 1 = 7 px tall glyphs */
    uint32_t color;        /* A8B8G8R8 */
    char text[192];
} AoeDemoText;
typedef struct {
    int x, y, w, h;        /* drawn at half brightness behind the text */
} AoeDemoRect;
typedef struct {
    AoeDemoText texts[AOE_DEMO_OVERLAY_TEXTS];
    size_t text_count;
    AoeDemoRect rects[AOE_DEMO_OVERLAY_RECTS];
    size_t rect_count;
} AoeDemoOverlay;
void aoe_demo_overlay(const AoeDemo *demo, AoeDemoOverlay *overlay);

/* Optional parallel runner for per-frame work split into independent
   parts (the 32-bit colour conversion, by row bands): it must call
   task(arg, part, parts) once for every part in [0, parts) and return when
   all have finished. Without one, parts run in turn on the caller. */
typedef void (*AoeDemoTask)(void *arg, int part, int parts);
typedef void (*AoeDemoParallel)(void *context, AoeDemoTask task, void *arg,
                                int parts);
void aoe_demo_set_parallel(AoeDemo *demo, AoeDemoParallel run, void *context,
                           int parts);

/* Diagnostics: redraws the full terrain layer for the current camera and
   returns how many pixels differ from the cached layer (0 when the cache
   is exact or not yet built). */
size_t aoe_demo_verify_terrain_cache(AoeDemo *demo);

/* Moves the camera to map tile coordinates (clamped to the map). */
void aoe_demo_set_camera(AoeDemo *demo, float x, float y);

typedef struct {
    uint64_t frames, world_updates;
    uint32_t live_units, units_drawn, terrain_tiles;
    uint32_t trained, attack_orders;
    float update_ms, draw_ms;
    float camera_x, camera_y;
    float fps;                       /* smoothed from update dt */
    float terrain_ms, objects_ms, expand_ms; /* draw_ms breakdown */
    uint32_t terrain_full, terrain_shifted; /* terrain cache redraws */
} AoeDemoStats;
void aoe_demo_stats(const AoeDemo *demo, AoeDemoStats *stats);

/* Overlay text with the demo's own 5x7 font into the same framebuffer
   format; exposed for the platform's error screen. */
void aoe_demo_draw_text(uint32_t *framebuffer, size_t stride, int x, int y,
                        int scale, uint32_t color, const char *text);
void aoe_demo_fill_rect(uint32_t *framebuffer, size_t stride, int x, int y,
                        int w, int h, uint32_t color, int blend_half);
/* The same clipped to a `width` x `height` frame. */
void aoe_demo_draw_text_in(uint32_t *framebuffer, size_t stride, int width, int height, int x,
                           int y, int scale, uint32_t color, const char *text);
void aoe_demo_fill_rect_in(uint32_t *framebuffer, size_t stride, int width, int height, int x,
                           int y, int w, int h, uint32_t color, int blend_half);

#endif
