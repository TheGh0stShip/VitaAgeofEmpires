#ifndef VITA_AOE_DEMO_APP_H
#define VITA_AOE_DEMO_APP_H

#include <stddef.h>
#include <stdint.h>

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

#endif
