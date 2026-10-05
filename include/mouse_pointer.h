#ifndef VITA_AOE_MOUSE_POINTER_H
#define VITA_AOE_MOUSE_POINTER_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a software mouse pointer (0x004496e0..
   0x0044ab40, the global 0x00582edc): two cursor frames from an SLP
   shape (the normal one and the one inside the game region), the save
   area under the pointer, and drawing it either into the back area (the
   frame draw, 0x0044a2a0) or straight onto the primary surface (the
   poll between frames, 0x0044a850). Semantic storage with the original
   offsets noted; DirectDraw, GDI, the shape and the draw areas are ops. */

typedef struct {
    int32_t left, top, right, bottom;
} AoeMouseRectRor;

/* A shape frame's +0x10 width, +0x14 height, +0x18 / +0x1c hotspot. */
typedef struct {
    int32_t width, height, hot_x, hot_y;
} AoeMouseFrameRor;

typedef struct {
    void *context;
    uint32_t (*time)(void *context);                    /* timeGetTime */
    int (*windowed)(void *context);                     /* system +0x479 */
    void (*cursor_pos)(void *context, int32_t *x, int32_t *y); /* GetCursorPos (+ScreenToClient
                                                                  when windowed) */
    void (*window_rect)(void *context, AoeMouseRectRor *r);
    void (*screen_size)(void *context, int32_t *w, int32_t *h); /* system +0x2c, +0x30 */
    void (*set_cursor_pos)(void *context, int32_t x, int32_t y);
    void (*set_cursor)(void *context, int arrow);       /* NULL or IDC_ARROW */
    void (*invalidate)(void *context, const AoeMouseRectRor *r);
    void (*mark_dirty)(void *context, const AoeMouseRectRor *r); /* 0x00437ac0 */
    /* IDirectDrawSurface Blt (+0x14, DDBLT_WAIT), BltFast (+0x1c, source
       colour key and wait), GetClipper (+0x3c), SetClipper (+0x70). */
    int32_t (*blt)(void *context, void *dst, const AoeMouseRectRor *dst_rect, void *src,
                   const AoeMouseRectRor *src_rect);
    int32_t (*blt_fast)(void *context, void *dst, int32_t x, int32_t y, void *src,
                        const AoeMouseRectRor *src_rect);
    int32_t (*get_clipper)(void *context, void *surface, void **clipper);
    int32_t (*set_clipper)(void *context, void *surface, void *clipper);
    void (*draw_sync)(void *context);                   /* 0x00439890(1, 0) */
    /* Draw areas: 0x004382a0 + 0x00438500 (new, sized), 0x004383d0
       (free), +0x3c (its surface), 0x00438730 / 0x004387c0 (lock and
       unlock with the caller's name), 0x00438b50(0,0) + 0x00439c90 (clear). */
    void *(*area_new)(void *context, const char *name, int32_t w, int32_t h);
    void (*area_free)(void *context, void *area);
    void *(*area_surface)(void *context, void *area);
    int (*area_lock)(void *context, void *area, const char *who);
    void (*area_unlock)(void *context, void *area, const char *who);
    void (*area_clear)(void *context, void *area, int32_t w, int32_t h);
    /* The shape: 0x0049f5f0 load ("%s.slp" and a resource), 0x0049f9e0
       plus +0x18 (usable), 0x0049fa90 (frame count), its +0x1c frames,
       0x0049fe00 (draw a frame into an area), 0x0049f840 (free). */
    void *(*shape_load)(void *context, const char *file, int32_t resource);
    int (*shape_ok)(void *context, void *shape);
    int32_t (*shape_frames)(void *context, void *shape);
    void (*shape_frame)(void *context, void *shape, int32_t i, AoeMouseFrameRor *f);
    void (*shape_draw)(void *context, void *shape, void *area, int32_t x, int32_t y, int32_t frame);
    void (*shape_free)(void *context, void *shape);
} AoeMouseOpsRor;

/* The draw area given at setup (+0x04) and what is read from it and its
   system: +0x3c surface, the system's +0x1c primary surface and +0x10
   primary area, +0x18 / +0x1c size, and the system's +0x478 (no
   pointer when 1). */
typedef struct {
    void *area;
    void *surface;
    void *primary_surface;
    void *primary_area;
    int32_t width, height;
    int gdi_only;
} AoeMouseHostRor;

typedef struct {
    int enabled;                      /* +0x000 */
    AoeMouseHostRor *host;            /* +0x004 */
    void *save_area;                  /* +0x008 */
    void *primary_area;               /* +0x00c */
    int32_t frame_count;              /* +0x010 */
    int32_t max_cursors;              /* +0x014 */
    void *cursor_areas[16];           /* +0x018 */
    void *shape;                      /* +0x058 */
    int32_t mode;                     /* +0x05c: 0 blit, 1 draw */
    int32_t max_w, max_h;             /* +0x060 */
    int32_t normal_w, normal_h;       /* +0x068 */
    int32_t game_w, game_h;           /* +0x070 */
    int32_t x, y;                     /* +0x078 */
    int32_t last_x, last_y;           /* +0x080 (-32767) */
    int32_t normal_frame, game_frame; /* +0x088 */
    int32_t frame, drawn_frame;       /* +0x090 */
    int32_t normal_hot_y, normal_hot_x, normal_below, normal_right; /* +0x098 */
    int32_t game_hot_y, game_hot_x, game_below, game_right;         /* +0x0a8 */
    int saved;                        /* +0x0b8 */
    AoeMouseRectRor save_src;         /* +0x0bc */
    AoeMouseRectRor dest;             /* +0x0cc */
    AoeMouseRectRor clip;             /* +0x0dc */
    AoeMouseRectRor src;              /* +0x0ec */
    AoeMouseRectRor save_dest;        /* +0x0fc */
    AoeMouseRectRor region;           /* +0x10c */
    void *saved_surface;              /* +0x11c */
    void *draw_surface;               /* +0x120 */
    void *primary_surface;            /* +0x124 */
    int visible;                      /* +0x128 */
    int drawn;                        /* +0x12c */
    int detached;                     /* +0x130 */
    int ready;                        /* +0x134 */
    int region_set, region_on;        /* +0x138, +0x13c */
    int on_screen;                    /* +0x140 */
    uint32_t last_time;               /* +0x144 */
    int32_t error;                    /* +0x148 */
    char file[260];                   /* +0x14c */
    int32_t resource;                 /* +0x250 */
} AoeMousePtrRor;

/* 0x004496e0: everything zero, the last position -32767, the resource
   -1, the time 36 ms back. */
void aoe_mouse_init_ror(AoeMousePtrRor *m, int32_t mode, const AoeMouseOpsRor *ops);
/* 0x004499a0: stores the pointer globally, then (when enabled and the
   system allows it) hides the system cursor, loads "<name>.slp", keeps
   up to `max_cursors` (1..16) frames and makes the areas (0x00449b70);
   frames 0 for both. 0x00449880 frees the shape and areas and clears
   the global. */
int aoe_mouse_setup_ror(AoeMousePtrRor *m, int enabled, AoeMouseHostRor *host, const char *name,
                        int32_t resource, int32_t max_cursors, const AoeMouseOpsRor *ops);
void aoe_mouse_destroy_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops);
/* 0x00449b70: (re)load the shape (keeping the name when `reload`),
   find its largest frame (then fixed at 64x48), make the areas
   (0x00449e30: "Mouse Save area" and "Mouse Cursor %d"), reselect the
   frames, and in blit mode draw each frame into its cursor area (0 when
   a frame is larger than 64x48 or the frames outnumber the areas). */
int aoe_mouse_load_ror(AoeMousePtrRor *m, const char *name, int32_t resource, int reload,
                       const AoeMouseOpsRor *ops);
int aoe_mouse_make_areas_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops);
/* 0x00449dc0: free the areas. */
void aoe_mouse_free_areas_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops);
/* 0x004498c0 detaches from the draw area (when ready), 0x00449920
   reattaches to a new one and reloads. */
int aoe_mouse_detach_ror(AoeMousePtrRor *m);
int aoe_mouse_attach_ror(AoeMousePtrRor *m, AoeMouseHostRor *host, const AoeMouseOpsRor *ops);
/* 0x0044a660 (the game's 0x00419da0) / 0x0044a6c0: the normal / game frame (ignored when out of
   range) and its size and hotspot. */
void aoe_mouse_set_frame_ror(AoeMousePtrRor *m, int game, int32_t frame, const AoeMouseOpsRor *ops);
/* 0x00449b40 the game region; 0x00449b00 / 0x00449b10 its switch;
   0x00449b20 whether both are set. */
void aoe_mouse_set_region_ror(AoeMousePtrRor *m, int32_t l, int32_t t, int32_t r, int32_t b);
void aoe_mouse_set_region_on_ror(AoeMousePtrRor *m, int on);
int aoe_mouse_region_on_ror(const AoeMousePtrRor *m);
int aoe_mouse_in_game_ror(const AoeMousePtrRor *m);
/* 0x00449fa0: read the position (windowed coordinates above 0x8000
   wrap negative), pick the frame, and work out the pointer's rectangle
   clipped to the area; 0 when nothing changed. */
int aoe_mouse_update_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops);
/* 0x0044a2a0: the frame draw (into the back area; at most every 34 ms
   in the game region unless forced); 0x0044a850 the poll (onto the
   primary surface). 0x0044a5a0 restores what was under it; 0x0044a720
   hides it (and the system cursor), 0x0044a780 shows it (the arrow
   without a software pointer); 0x0044a7c0 forgets the saved area;
   0x0044a7d0 centres the system cursor. */
void aoe_mouse_draw_ror(AoeMousePtrRor *m, int force, const AoeMouseOpsRor *ops);
void aoe_mouse_poll_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops);
void aoe_mouse_restore_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops);
void aoe_mouse_hide_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops);
void aoe_mouse_show_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops);
void aoe_mouse_forget_ror(AoeMousePtrRor *m);
void aoe_mouse_center_ror(const AoeMouseOpsRor *ops);
/* 0x0044ab40: a DirectDraw HRESULT as the game's error index (-1 for
   one it does not know). */
int32_t aoe_mouse_error_ror(int32_t hr);

extern AoeMousePtrRor *aoe_mouse_global_ror; /* 0x00582edc */

#endif
