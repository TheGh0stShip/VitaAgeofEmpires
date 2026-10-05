#ifndef VITA_AOE_DIAMOND_MAP_H
#define VITA_AOE_DIAMOND_MAP_H

#include <stdint.h>

#include "draw_system.h"
#include "ui_panel.h"

/* Patched Rise of Rome 1.0a diamond map (the minimap; base vtable
   0x005434b4, 0x0042c360..0x0042e1f0). A diamond-shaped bitmap ("map.bmp")
   gives the pixel rows the map fills: each row of 0xff pixels is one run
   (x, width). Each map diagonal d (tiles with x - y constant; d = width -
   x - 1 + y) gets a pixel row, how many pixels a tile takes and the first
   tile on it. Tiles are drawn into a buffer surface (copied to the area),
   and the view frame, the selected area and the object dots straight onto
   the area. World data is read through the ops (original offsets noted). */

/* One diagonal (0x24 bytes in the original). */
typedef struct {
    int16_t row;          /* +0x00: index into the rows */
    uint8_t valid;        /* +0x02: set from a row (others copy the last) */
    int16_t tiles;        /* +0x04 */
    float tiles_per_pixel;/* +0x08 */
    float half;           /* +0x0c: tiles_per_pixel / 2 when pixels_per_tile < 1 */
    int16_t first_x;      /* +0x10 */
    int16_t first_y;      /* +0x12 */
    int16_t pixels_ceil;  /* +0x14: pixels per tile, rounded up */
    float pixels_per_tile;/* +0x18 */
    int16_t pixels_trunc; /* +0x1c: width / pixels_per_tile, truncated */
    float width_per_tile; /* +0x20: run width / tiles */
} AoeDiamondDiagonalRor;

/* One run of map pixels (6 bytes). */
typedef struct {
    int16_t diagonal;     /* +0: the diagonal drawn on this row */
    int16_t x;            /* +2 */
    int16_t width;        /* +4 */
} AoeDiamondRowRor;

/* What a dot needs of an object. */
typedef struct {
    int has_sprite;          /* +0x1c */
    float x, y;              /* +0x38, +0x3c */
    uint8_t flags_36;        /* +0x36: bit 0 hidden from the map */
    const void *owner;       /* +0x0c */
    int16_t owner_number;    /* owner +0x4a */
    int drawn_last;          /* vtable +0xe8: drawn over the others */
    uint8_t type;            /* +0x4e: 0x19 a doppelganger */
    uint32_t doppel_mask;    /* +0xa4: players who saw it go */
    uint8_t doppel_mode;     /* +0xa8 */
    uint8_t doppel_color;    /* +0xa9 */
    uint8_t fog_visible;     /* template +0x65: shown under fog */
    uint8_t map_mode;        /* template +0x8e: 1 small, 2 large, 3 large
                                in its own colour, 4 colours its tiles */
    uint8_t map_color;       /* template +0x90 */
} AoeDiamondObjectRor;

typedef struct {
    void *context;
    /* Map +0x08 / +0x0c size; tile (x, y) +4 shape and +5 terrain (low
       five bits); map +0xb4..+0xb6 + terrain * 0x198 colour (0 dark,
       1 middle, 2 light); +0x8db8 all visible, +0x8db9 explored kept. */
    int32_t (*map_width)(void *context);
    int32_t (*map_height)(void *context);
    uint8_t (*tile_shape)(void *context, int32_t x, int32_t y);
    uint8_t (*tile_terrain)(void *context, int32_t x, int32_t y);
    uint8_t (*terrain_color)(void *context, uint8_t terrain, int which);
    int (*all_visible)(void *context);
    int (*keep_explored)(void *context);
    /* The first object on tile (x, y) whose template map mode is 4 (its
       colour in `color`), 0 for none. */
    int (*tile_feature)(void *context, int32_t x, int32_t y, uint8_t *color);
    /* The visibility bits of tile (x, y) (table 0x007d205c). */
    uint32_t (*visibility)(void *context, int32_t x, int32_t y);
    /* The viewed player: explored (+0xd4) and visible (+0xd8) masks, fog
       on (+0x38), +0xb0[number] (1: allied vision of that player), its
       map colour (+0x10c +0x25), view centre (+0x118, +0x11c) and its
       setter (0x0045d330), the selection (+0x138[25], count +0x1a0);
       the world's viewed player index (+0x7c). */
    uint32_t (*explored_mask)(void *context, const void *player);
    uint32_t (*visible_mask)(void *context, const void *player);
    int (*fog)(void *context, const void *player);
    int32_t (*shared_vision)(void *context, const void *player, int16_t number);
    uint8_t (*player_map_color)(void *context, const void *player);
    void (*view_center)(void *context, const void *player, float *x, float *y);
    void (*set_view_center)(void *context, const void *player, float x, float y);
    int32_t (*selection)(void *context, const void *player, void **out, int32_t max);
    int32_t (*viewed_index)(void *context);
    /* The world's players (+0x3c count, +0x40 list); a player's object
       list (+0x28, or +0x30 for the doppelgangers) and an object's dot. */
    int32_t (*player_count)(void *context);
    const void *(*world_player)(void *context, int32_t index);
    int32_t (*objects)(void *context, const void *player, int doppelgangers,
                       void ***out);
    int (*read_object)(void *context, void *object, AoeDiamondObjectRor *out);
    /* The player's tiles explored since the last draw (+0x6c list: x, y)
       and clearing it (0x0045eb40). */
    int32_t (*explored_tiles)(void *context, const void *player,
                              const int16_t **xy);
    void (*clear_explored)(void *context, const void *player);
    /* The main view (+0x150): its size in tiles (0x005118a0, +0x116 and
       +0x118) and its highlighted tiles (0x00510070). */
    void (*view_size)(void *context, int16_t *w, int16_t *h);
    int (*view_tiles)(void *context, int32_t *x0, int32_t *y0, int32_t *x1,
                      int32_t *y1);
    /* The game's edit mode (+0x1b4). */
    int32_t (*edit_mode)(void *context);
    /* "map.bmp": 8-bit pixels top-down (free()), NULL when it fails. */
    uint8_t *(*load_bitmap)(void *context, const char *name, int32_t id,
                            int32_t *width, int32_t *height);
} AoeDiamondMapOpsRor;

typedef struct AoeDiamondMapRor AoeDiamondMapRor;

typedef struct {
    AoeUiPanelVtblRor base;
    void (*draw_selected_area)(AoeDiamondMapRor *);         /* +0xe4 */
    void (*draw_objects)(AoeDiamondMapRor *);               /* +0xe8 */
    void (*draw_object)(AoeDiamondMapRor *, void *, uint8_t, uint8_t); /* +0xec */
    void (*draw_dot)(AoeDiamondMapRor *, int16_t, int16_t, uint8_t, int16_t,
                     void *);                               /* +0xf0 */
    void (*draw_view_frame)(AoeDiamondMapRor *);            /* +0xf4 */
} AoeDiamondMapVtblRor;

struct AoeDiamondMapRor {
    AoeUiPanelRor panel;
    const void *world;          /* +0xf4 */
    const void *player;         /* +0xf8 */
    int has_map;                /* +0xfc: the world's map (+0x28) */
    int32_t full_redraw;        /* +0x104, 1 */
    int32_t mouse_x, mouse_y;   /* +0x108, +0x10c */
    int32_t frame[4];           /* +0x114..+0x120 l, t, r, b */
    AoeDiamondRowRor *rows;     /* +0x128 */
    AoeDiamondDiagonalRor *diagonals; /* +0x12c */
    int16_t map_w, map_h;       /* +0x130, +0x132 */
    int16_t first_row;          /* +0x134 (-1 before any) */
    int16_t widest;             /* +0x136 */
    int16_t row_count;          /* +0x138 */
    int16_t diagonal_count;     /* +0x13a */
    float diagonals_per_row;    /* +0x13c */
    float pixels_per_diagonal;  /* +0x140 */
    float rows_per_diagonal;    /* +0x144 */
    int16_t row_height;         /* +0x148: rows per diagonal, rounded up */
    int16_t mean_pixels;        /* +0x14a */
    int16_t frame_x, frame_y;   /* +0x14c, +0x14e (-1) */
    AoeUiPanelRor *main_view;   /* +0x150 */
    int32_t dragging;           /* +0x154 */
    uint8_t select_color;       /* +0x158, 0xff */
    int32_t phase;              /* +0x15c: object drawing pass 1..4 */
    uint32_t player_mask;       /* +0x160: 1 << the viewed index */
    int32_t owns_buffer;        /* +0x164 */
    AoeDrawSurfaceRor *buffer;  /* +0x168 */
    const AoeDiamondMapOpsRor *ops;
};

const AoeDiamondMapVtblRor *aoe_diamond_map_vtable_ror(void);

/* 0x0042c360 constructor; 0x0042c460 destructor (thunk 0x00518030) (deleting 0x0042c440);
   0x0042c4f0 setup (area, parent, x, y, w, h, colour; the buffer, not
   owned). */
void aoe_diamond_map_init_ror(AoeDiamondMapRor *m, AoeUiSystemRor *system,
                              const AoeDiamondMapOpsRor *ops);
void aoe_diamond_map_release_ror(AoeDiamondMapRor *m);
int aoe_diamond_map_setup_ror(AoeDiamondMapRor *m, AoeUiAreaRor *area,
                              AoeUiPanelRor *parent, int32_t x, int32_t y,
                              int32_t w, int32_t h, int32_t color,
                              AoeDrawSurfaceRor *buffer);
/* 0x0042c540: the rows from the bitmap (runs of 0xff; rows without one
   are skipped), then the diagonals when there is a world. 0 when the
   bitmap does not load. */
int aoe_diamond_map_load_ror(AoeDiamondMapRor *m, const char *name, int32_t id);
/* 0x0042c780: the world (and its map); builds the diagonals and redraws
   everything. 0x0042cbb0: the viewed player. */
void aoe_diamond_map_set_world_ror(AoeDiamondMapRor *m, const void *world);
void aoe_diamond_map_set_player_ror(AoeDiamondMapRor *m, const void *player);
/* 0x0042cc00 drop an owned buffer; 0x0042cc50 make one ("Diamond map
   (restored)", the panel's size) or resize the owned one. */
void aoe_diamond_map_free_buffer_ror(AoeDiamondMapRor *m);
int aoe_diamond_map_make_buffer_ror(AoeDiamondMapRor *m);
/* 0x0042cf30 clear the map's pixels in the buffer; 0x0042d080 copy the
   buffer to the panel; 0x0042d0c0 draw every tile; 0x0042d130 the newly
   explored tiles; 0x0042d1c0 one tile (mode 0 into the buffer in its
   terrain colour, or `color` when given, or a mode-4 neighbour's; mode 1
   onto the area in the select colour). */
void aoe_diamond_map_clear_image_ror(AoeDiamondMapRor *m);
void aoe_diamond_map_blit_ror(AoeDiamondMapRor *m);
void aoe_diamond_map_draw_tiles_ror(AoeDiamondMapRor *m);
void aoe_diamond_map_draw_explored_ror(AoeDiamondMapRor *m);
void aoe_diamond_map_draw_tile_ror(AoeDiamondMapRor *m, int16_t x, int16_t y,
                                   int32_t mode, uint8_t color);
/* 0x0042df00: the tile under a screen point (and nonzero), else 0. */
int aoe_diamond_map_tile_at_ror(const AoeDiamondMapRor *m, int32_t x, int32_t y,
                                int16_t *tx, int16_t *ty);
/* 0x0042e040 start dragging the view (capture, centre on the tile,
   redraw both); 0x0042e0f0 drag; 0x0042e190 stop. */
void aoe_diamond_map_press_ror(AoeDiamondMapRor *m, int32_t x, int32_t y);
void aoe_diamond_map_drag_ror(AoeDiamondMapRor *m, int32_t x, int32_t y);
void aoe_diamond_map_release_drag_ror(AoeDiamondMapRor *m);

/* Slots (exported for the subclasses): +0x20 0x0042cbd0 (2 asks for a
   full redraw), +0x30 0x0042cd60 draw, +0x50 0x0042e000 size (the frame
   recomputed, the buffer made), +0xc0 0x0042e1a0 inside (on the map),
   +0xe4 0x0042d670 selected area, +0xe8 0x0042d710 objects (gaia, each
   player's doppelgangers and objects but the viewer's last-drawn ones,
   the viewer's selection in white, then those), +0xec 0x0042d910 an
   object (fog rules, doppelganger memory), +0xf0 0x0042daa0 a dot (1 a
   3x3 box, 2 a plus, 3 a 7-wide blob, 4 a 5x5 box), +0xf4 0x0042dc10 the
   view frame. */
void aoe_diamond_map_set_redraw_ror(AoeUiPanelRor *p, int32_t redraw);
void aoe_diamond_map_draw_ror(AoeUiPanelRor *p);
int32_t aoe_diamond_map_size_ror(AoeUiPanelRor *p, int32_t w, int32_t h);
int32_t aoe_diamond_map_inside_ror(AoeUiPanelRor *p, int32_t x, int32_t y);
void aoe_diamond_map_draw_selected_area_ror(AoeDiamondMapRor *m);
void aoe_diamond_map_draw_objects_ror(AoeDiamondMapRor *m);
void aoe_diamond_map_draw_object_ror(AoeDiamondMapRor *m, void *object,
                                     uint8_t color, uint8_t small_color);
void aoe_diamond_map_draw_dot_ror(AoeDiamondMapRor *m, int16_t x, int16_t y,
                                  uint8_t color, int16_t shape, void *object);
void aoe_diamond_map_draw_view_frame_ror(AoeDiamondMapRor *m);

#endif
