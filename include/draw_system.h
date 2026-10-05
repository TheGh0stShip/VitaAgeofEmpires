#ifndef VITA_AOE_DRAW_SYSTEM_H
#define VITA_AOE_DRAW_SYSTEM_H

#include <stdint.h>

#include "shape.h"
#include "span_region.h"

/* Patched Rise of Rome 1.0a draw system (0x00436c30..0x00438200) and
   draw surfaces (0x004382a0..0x00439960). Draw type 1 keeps surfaces in
   memory and draws through GDI; type 2 uses DirectDraw. Screen mode 1 is
   windowed, 2 full screen. Every device, palette, clipper, surface and
   window call is AoeDrawOpsRor (results 0 for success, as DirectDraw
   returns them). Semantic storage; offsets are noted. */

struct AoeDrawSurfaceRor;

typedef struct {
    void *context;
    /* Device. Cooperation flags: 8 normal, 0x11 exclusive full screen. */
    int32_t (*create)(void *context, void **device);
    void (*release)(void *context, void *object);
    int32_t (*set_level)(void *context, void *device, void *window,
                         uint32_t flags);
    int32_t (*get_mode)(void *context, void *device, int32_t *width,
                        int32_t *height, int32_t *bits);
    int32_t (*set_mode)(void *context, void *device, int32_t width,
                        int32_t height, int32_t bits);
    void (*restore_mode)(void *context, void *device);
    /* Every display mode the device offers. */
    void (*enum_modes)(void *context, void *device,
                       void (*each)(void *user, int32_t width,
                                    int32_t height, int32_t bits),
                       void *user);
    /* The device's capability words (general and colour-key). */
    int32_t (*get_caps)(void *context, void *device, uint32_t *caps,
                        uint32_t *key_caps);
    /* The desktop width without a device. */
    int32_t (*desktop_width)(void *context);
    /* Palette objects: create from 256 RGBX entries, read, write. */
    int32_t (*create_palette)(void *context, void *device,
                              const uint8_t entries[1024], void **palette);
    int32_t (*palette_get)(void *context, void *palette, int32_t start,
                           int32_t count, uint8_t *out);
    int32_t (*palette_set)(void *context, void *palette, int32_t start,
                           int32_t count, const uint8_t *in);
    /* The window's own palette: its entries, and realizing it (nonzero
       when the colours changed; the window is then invalidated). */
    int (*window_palette)(void *context, void *hpalette, uint8_t out[1024]);
    int (*realize)(void *context, void *window, void *hpalette);
    int32_t (*create_clipper)(void *context, void *device, void **clipper);
    int32_t (*clipper_window)(void *context, void *clipper, void *window);
    /* Surfaces: the primary (with its clipper and palette), an offscreen
       one (video memory unless `system`), and their description (pitch,
       capability bits; 0x800 system memory). */
    int32_t (*create_primary)(void *context, void *device, void **surface);
    int32_t (*attach_clipper)(void *context, void *surface, void *clipper);
    int32_t (*attach_palette)(void *context, void *surface, void *palette);
    int32_t (*create_surface)(void *context, void *device, int32_t width,
                              int32_t height, int system, void **surface);
    int32_t (*describe)(void *context, void *surface, int32_t *pitch,
                        uint32_t *caps);
    /* Fill a rectangle (exclusive right/bottom; NULL for all) with a
       colour; copy one (keyed: skip the source key colour). */
    int32_t (*fill)(void *context, void *surface, const int32_t *rect,
                    uint32_t color);
    int32_t (*blt)(void *context, void *dst, const int32_t dst_rect[4],
                   void *src, const int32_t src_rect[4], int keyed);
    int32_t (*is_lost)(void *context, void *surface);
    int32_t (*restore)(void *context, void *surface);
    int32_t (*lock)(void *context, void *surface, int wait, uint8_t **bits);
    void (*unlock)(void *context, void *surface, uint8_t *bits);
    int32_t (*get_dc)(void *context, void *surface, void **dc);
    void (*release_dc)(void *context, void *surface, void *dc);
    int32_t (*set_key)(void *context, void *surface, uint8_t color);
    /* The window: its client size and screen origin, a size message. */
    void (*client_size)(void *context, void *window, int32_t *width,
                        int32_t *height);
    void (*client_origin)(void *context, void *window, int32_t *x,
                          int32_t *y);
    void (*size_message)(void *context, void *window, int32_t width,
                         int32_t height);
    /* Memory surfaces (type 1): the software copy of a rectangle from
       `src` to (x, y) on `dst`, keyed or not; and the assembly
       renderer's clipped span fill (x0..x1 of row y). */
    void (*soft_blit)(void *context, struct AoeDrawSurfaceRor *dst,
                      int32_t x, int32_t y, struct AoeDrawSurfaceRor *src,
                      int32_t sx, int32_t sy, int32_t w, int32_t h, int keyed,
                      uint8_t key);
    void (*span_fill)(void *context, int32_t x0, int32_t x1, int32_t y,
                      uint8_t color);
    /* A memory surface's bitmap: put the old one back in the context,
       delete the bitmap and the context. */
    void (*free_bitmap)(void *context, void *dc, void *bitmap,
                        void *old_bitmap);
} AoeDrawOpsRor;

typedef struct AoeDrawNodeRor {
    struct AoeDrawSurfaceRor *surface;
    struct AoeDrawNodeRor *prev, *next;
} AoeDrawNodeRor;

typedef struct {
    void *instance;                   /* +0x000 */
    void *window;                     /* +0x004 */
    void *hpalette;                   /* +0x008 */
    struct AoeDrawSurfaceRor *back;   /* +0x00c */
    struct AoeDrawSurfaceRor *primary;/* +0x010 */
    AoeDrawNodeRor *surfaces;         /* +0x014 */
    void *device;                     /* +0x018 */
    void *primary_surface;            /* +0x01c */
    void *clipper;                    /* +0x020 */
    void *palette_object;             /* +0x024 */
    uint32_t flags;                   /* +0x028: bit 0 system memory */
    int32_t width, height, bits;      /* +0x02c, +0x030, +0x034 */
    int32_t error;                    /* +0x038 */
    int32_t mode_set;                 /* +0x03c */
    int32_t bank_switched;            /* +0x040 */
    int32_t key_blits;                /* +0x044 */
    int32_t restored;                 /* +0x048 */
    /* Modes on offer: 8-bit 640x480, 800x600, 1024x768, 1280x1024,
       320x200, 320x240; 16-bit 320x200, 320x240, 640x480, 800x600,
       1024x768. */
    int32_t modes[11];                /* +0x04c..+0x074 */
    uint8_t palette[1024];            /* +0x078 */
    uint8_t type;                     /* +0x478 */
    uint8_t screen;                   /* +0x479 */
    /* Globals: the mode-change flag, the fill (else software) clear
       (initially 1), the last copy's result and message id, palette
       update counts (all entries; seven entries) and marker. */
    int32_t changing_mode;
    int32_t hardware_clear;
    int32_t last_blt, last_blt_message;
    uint32_t full_palette_sets, short_palette_sets;
    int32_t palette_marker;
    const AoeDrawOpsRor *ops;
} AoeDrawSystemRor;

/* A surface (0x100 bytes). `view` carries the original's +0x08 bits,
   +0x18/+0x1c size, +0x20 pitch, +0x24 row direction, +0x28..+0x34 clip
   and +0xc8 shading table. */
typedef struct AoeDrawSurfaceRor {
    AoeDrawSystemRor *system;     /* +0x00 */
    void *window;                 /* +0x04 */
    AoeShapeSurfaceRor view;
    int32_t info_width;           /* +0x0c: bitmap header width */
    int32_t info_height;          /*        and height (negative: top-down) */
    void *bitmap, *old_bitmap;    /* +0x10, +0x14 */
    void *dc;                     /* +0x38 */
    void *surface;                /* +0x3c */
    int32_t desc_pitch;           /* +0x50 */
    uint8_t *desc_bits;           /* +0x64 */
    uint32_t desc_caps;           /* +0xa8 */
    AoeDrawNodeRor *node;         /* +0xac */
    char *name;                   /* +0xb0 */
    uint8_t *base;                /* +0xb4 */
    uint8_t **row_table;          /* +0xbc */
    uint8_t **rows;               /* +0xc0 */
    uint8_t **wrap_rows;          /* +0xc4 */
    AoeSpanRegionRor *region;     /* +0xcc */
    AoeSpanRegionRor *active;     /* +0xd0 */
    int32_t wrap_x, wrap_y;       /* +0xd4, +0xd8 */
    int32_t row_table_size;       /* +0xdc */
    int32_t wrap_size;            /* +0xe0 */
    int32_t system_memory;        /* +0xe4 */
    int32_t video;                /* +0xe8 */
    int32_t keyed;                /* +0xec */
    int32_t shares_primary;       /* +0xf0 */
    int32_t restored;             /* +0xf4 */
    int32_t extra_rows;           /* +0xf8 */
    uint8_t key;                  /* +0xfc */
} AoeDrawSurfaceRor;

/* ---- draw system ---- */

/* 0x00436c30 clears; 0x00436cc0 shuts down (blank the full-screen
   primary, free the surfaces, release palette, clipper, mode and
   device). 0x00436d50: which modes exist (without a device, 640x480 and
   by desktop width; with one, each listed mode through 0x00436e10);
   0x00436f70 is one available. */
void aoe_draw_clear_ror(AoeDrawSystemRor *d, const AoeDrawOpsRor *ops);
void aoe_draw_shutdown_ror(AoeDrawSystemRor *d);
void aoe_draw_probe_modes_ror(AoeDrawSystemRor *d, int use_device);
void aoe_draw_note_mode_ror(AoeDrawSystemRor *d, int32_t width,
                            int32_t height, int32_t bits);
int32_t aoe_draw_mode_available_ror(const AoeDrawSystemRor *d, int32_t width,
                                    int32_t height, int32_t bits);
/* 0x00437040: open (nothing for type 1): device, level (windowed needs
   an 8-bit desktop: error 2), palette, clipper, surfaces, capabilities;
   errors 1. 0x00437370: change the full-screen mode (surfaces remade,
   size message). 0x00437490 blanks the full-screen primary. */
int aoe_draw_open_ror(AoeDrawSystemRor *d, void *instance, void *window,
                      void *hpalette, uint8_t type, uint8_t screen,
                      int32_t width, int32_t height, uint32_t flags);
int aoe_draw_change_mode_ror(AoeDrawSystemRor *d, int32_t width,
                             int32_t height, int32_t bits);
void aoe_draw_blank_ror(AoeDrawSystemRor *d);
/* 0x004374d0: after a lost primary: 1 when the display mode changed, 3
   on failure, 2 when restored (every other surface too), else 0.
   0x00437610 clears the restored flags. 0x00437630 frees the back
   buffer and primary; 0x00437690 makes them (the primary surface first).
   0x00437990 follows the window's client size. */
int aoe_draw_restore_ror(AoeDrawSystemRor *d);
void aoe_draw_clear_restored_ror(AoeDrawSystemRor *d);
void aoe_draw_free_surfaces_ror(AoeDrawSystemRor *d);
int aoe_draw_make_surfaces_ror(AoeDrawSystemRor *d);
void aoe_draw_fit_window_ror(AoeDrawSystemRor *d);
/* 0x00437a00: another window changed the palette (0x00437a30: realize
   ours when windowed, else reload it). 0x00437ac0: show the back buffer
   (all or a rectangle). */
void aoe_draw_palette_changed_ror(AoeDrawSystemRor *d, void *window);
int aoe_draw_realize_ror(AoeDrawSystemRor *d);
void aoe_draw_present_ror(AoeDrawSystemRor *d, const int32_t *rect);
/* Message ids for device results: 0x00437c50 (lost checks), 0x00437ca0
   (restores), 0x00437d50 (copies), 0x00437fa0 (surface creation). */
int32_t aoe_draw_lost_message_ror(int32_t result);
int32_t aoe_draw_restore_message_ror(int32_t result);
int32_t aoe_draw_blt_message_ror(int32_t result);
int32_t aoe_draw_create_message_ror(int32_t result);
/* 0x00438170: take a window palette (entry 0 black, 255 white);
   0x00438200: set `count` entries from `start`. */
void aoe_draw_set_palette_handle_ror(AoeDrawSystemRor *d, void *hpalette);
void aoe_draw_set_palette_ror(AoeDrawSystemRor *d, int32_t start,
                              int32_t count, const uint8_t *entries);

/* ---- surfaces ---- */

/* 0x004382a0 a named surface; 0x004383d0 frees it (and unlinks it).
   0x00438500 joins a system (listed; type 1: 0) and allocates; 0x00438890
   (re)allocates at a size (`extra` adds ten rows), rebuilding the clip
   and the whole-surface region. */
int aoe_surface_init_ror(AoeDrawSurfaceRor *s, const char *name,
                         int32_t system_memory);
void aoe_surface_free_ror(AoeDrawSurfaceRor *s);
int aoe_surface_attach_ror(AoeDrawSurfaceRor *s, AoeDrawSystemRor *d,
                           int32_t width, int32_t height, int32_t extra,
                           int32_t shares_primary);
void aoe_surface_allocate_ror(AoeDrawSurfaceRor *s, int32_t width,
                              int32_t height, int32_t extra);
/* 0x004385d0: like the system restore for one surface (reallocated when
   out of memory or the mode is wrong). */
int aoe_surface_restore_ror(AoeDrawSurfaceRor *s);
/* 0x00438730 lock (the bits; rows rebuilt when they moved), 0x004387c0
   unlock, 0x00438800 device context, 0x00438850 release it. */
uint8_t *aoe_surface_lock_ror(AoeDrawSurfaceRor *s, int wait);
void aoe_surface_unlock_ror(AoeDrawSurfaceRor *s);
void *aoe_surface_dc_ror(AoeDrawSurfaceRor *s);
void aoe_surface_release_dc_ror(AoeDrawSurfaceRor *s);
/* 0x00438b50 clear all or a rectangle (inclusive) with a colour (the
   software 0x00438d20 when the fill flag is off). */
void aoe_surface_fill_ror(AoeDrawSurfaceRor *s, const int32_t *rect,
                          uint8_t color);
void aoe_surface_soft_fill_ror(AoeDrawSurfaceRor *s, const int32_t *rect,
                               uint8_t color);
/* 0x00438e60 row pointers (and the wrapped ones, 0x00438f40, which start
   at row y and column x); 0x00438ff0 pitch and memory kind from the
   description. */
void aoe_surface_build_rows_ror(AoeDrawSurfaceRor *s);
void aoe_surface_wrap_rows_ror(AoeDrawSurfaceRor *s, int32_t x, int32_t y);
void aoe_surface_read_desc_ror(AoeDrawSurfaceRor *s);
/* 0x00439020 clip to a rectangle (NULL: everything), 0x00439100 set it
   and the clip region. */
void aoe_surface_clip_ror(AoeDrawSurfaceRor *s, const int32_t *rect);
void aoe_surface_set_clip_ror(AoeDrawSurfaceRor *s, int32_t left,
                              int32_t top, int32_t right, int32_t bottom);
/* 0x00439240: copy `rect` (all when NULL) of this surface to (x, y) on
   `dst`, clipped to both (keyed when the surface is and `use_key`).
   0x004395b0: copy the spans of `spans` (per row lists of x0..x1, from
   rect's left) from `src` at (sx, sy) into `rect`. 0x00439700: move a
   rectangle within the surface by (dx, dy) in 4-byte words. */
void aoe_surface_blit_ror(AoeDrawSurfaceRor *s, AoeDrawSurfaceRor *dst,
                          int32_t x, int32_t y, const int32_t *rect,
                          int use_key);
void aoe_surface_span_copy_ror(AoeDrawSurfaceRor *s, AoeDrawSurfaceRor *src,
                               int32_t sx, int32_t sy, const int32_t rect[4],
                               AoeSpanRor *const *spans);
void aoe_surface_scroll_ror(AoeDrawSurfaceRor *s, const int32_t from[4],
                            const int32_t to[4], int32_t dx, int32_t dy,
                            int32_t ox, int32_t oy);
/* 0x00439890 key colour (or none); 0x00439900 plot a clipped pixel;
   0x00439960 a line of span fills. */
void aoe_surface_set_key_ror(AoeDrawSurfaceRor *s, int32_t keyed,
                             uint8_t color);
void aoe_surface_plot_ror(AoeDrawSurfaceRor *s, int32_t x, int32_t y,
                          uint8_t color);
void aoe_draw_line_ror(const AoeDrawOpsRor *ops, int32_t x0, int32_t y0,
                       int32_t x1, int32_t y1, uint8_t color);

#endif
