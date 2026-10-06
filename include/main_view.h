#ifndef VITA_AOE_MAIN_VIEW_H
#define VITA_AOE_MAIN_VIEW_H

#include <stddef.h>
#include <stdint.h>

/* Patched Rise of Rome 1.0a main map view panel and its game-view
   subclass. Only the small state handlers are modelled here; the
   terrain/object drawing lives in terrain_draw/sprite_draw. Semantic
   fields replace the x86 object layout; the comments give the original
   offsets. Calls the original makes through globals or vtables are
   reached through the ops table. */

/* -10000.0f, the "no hovered point" value written to +0x138/+0x13c. */
#define AOE_MAIN_VIEW_NO_POINT_ROR (-10000.0f)

typedef struct {
    void *context;
    /* Panel vtable +0x20: mark the panel for redraw (1 or 2). */
    void (*redraw)(void *context, void *panel, int32_t level);
    /* 0x0050fde0: recompute the visible tile counts and centre from the
       panel size and the tile geometry. */
    void (*recompute_layout)(void *context);
    /* 0x00444a80 on the new map (called before its geometry is read). */
    void (*map_attached)(void *context, void *map);
    /* The map's world data (map +0x28) and its tile geometry
       (+0x8d96, +0x8d98, +0x8d9c, +0x8d9a, +0x8d9e). */
    void *(*map_world)(void *context, void *map);
    void (*map_geometry)(void *context, void *world, int16_t geometry[5]);
    /* Base panel handlers: mode change 0x00452bb0, 0x00453f00. */
    void (*base_set_mode)(void *context, int32_t mode);
    void (*base_handler)(void *context, int32_t value);
    /* DAT_00582edc and 0x00449b00(panel +0x78). */
    int (*global_flag)(void *context);
    void (*notify_area)(void *context, void *area);
    /* Owned "Terrain Buffer" surface at +0x324: destroyed and freed. */
    void (*destroy_overlay)(void *context, void *overlay);
    /* 0x0050ffd0: the tile highlight rectangle. */
    void (*set_highlight)(void *context, int32_t x0, int32_t y0, int32_t x1,
                          int32_t y1);
    /* The game (DAT_00580e38) +0x1b0 state and +0x1b4 edit mode. */
    int32_t (*game_state)(void *context);
    int32_t (*game_edit_mode)(void *context);
    /* The primary surface (panel +0x20, its +0x3c back buffer): busy
       check (vtable +0x34 with 1, nonzero while a blit is pending) and the
       fast blit (vtable +0x1c, flag 0x10 wait) of `src` (left, top,
       right, bottom exclusive) to (x, y). */
    int (*surface_busy)(void *context);
    void (*surface_blit)(void *context, int32_t x, int32_t y,
                         const int32_t src[4]);
    /* The cursor (GetCursorPos), the windowed flag (+0x479 of the
       primary's owner == 1) and ScreenToClient. */
    void (*cursor_pos)(void *context, int32_t *x, int32_t *y);
    int (*windowed)(void *context);
    void (*screen_to_client)(void *context, int32_t *x, int32_t *y);
    /* The draw surface lock / unlock named "view::draw_view"; the full
       view draw (code 10 with the rectangle) and the generic view draw
       (code, then zeros). */
    int (*lock_surface)(void *context, void *surface, const char *name);
    void (*unlock_surface)(void *context, void *surface, const char *name);
    void (*draw_region)(void *context, int32_t left, int32_t top,
                        int32_t right, int32_t bottom);
    void (*draw_code)(void *context, uint8_t code);
    /* The view's hit test (the generic view draw entry in pick mode) given two (x, y) points (both start as
       the query point) and the in-out value; writes the two leading words
       and the two 16-bit fields; returns the result character ('4'
       object, '3' other hits). */
    uint8_t (*pick)(void *context, uint8_t kind, int32_t arg,
                    int32_t point_a[2], int32_t point_b[2], int32_t *value, int32_t *out0, int32_t *out1,
                    int16_t *out2, int16_t *out3);
    /* timeGetTime, the base panel draw 0x00453af0 and the timing
       statistic 0x00417370(slot, ms). */
    uint32_t (*time_ms)(void *context);
    int32_t (*base_draw)(void *context, int32_t a, int32_t b, int32_t c,
                         int32_t d, int32_t e);
    void (*record_time)(void *context, int32_t slot, uint32_t ms);
    /* 0x00419b20: the game's mouse position; the game's brush size
       (DAT_00580e38 +0x402). */
    void (*game_mouse)(void *context, int32_t *x, int32_t *y);
    int16_t (*brush_size)(void *context);
} AoeMainViewOpsRor;

/* One entry of the queued blit list (+0x2dc, 32-byte records):
   the source rectangle (inclusive right/bottom) and the destination. */
typedef struct {
    int32_t left, top, right, bottom;
    int32_t dest_x, dest_y;
    int32_t dest_right, dest_bottom; /* +0x18, +0x1c (0x00510190 fills them) */
} AoeMainViewBlitRor;

/* 0x00511430 result (five dwords, the third holding two 16-bit fields). */
typedef struct {
    int32_t word0, word1;
    int16_t half0, half1;
    int32_t object_value; /* result '4' */
    int32_t other_value;  /* result '3' */
} AoeMainViewPickRor;

typedef struct {
    const AoeMainViewOpsRor *ops;
    void *area;          /* +0x78 */
    void *map;           /* +0xfc */
    void *player;        /* +0x100 */
    void *world;         /* +0x104 */
    /* +0x108 tile width, +0x10a tile height, +0x10c half width,
       +0x10e half height, +0x110 elevation step. */
    int16_t geometry[5];
    float point_x, point_y; /* +0x138, +0x13c */
    uint8_t drag_flag;      /* +0x142 */
    void *overlay;          /* +0x324 terrain buffer */
    void *surface;          /* +0x20 primary surface */
    void *draw_target;      /* +0xf4 */
    int32_t left, top, right, bottom; /* +0x8c..+0x98 panel rectangle */
    int16_t scroll_x, scroll_y;       /* +0x126, +0x128 */
    int32_t origin_x, origin_y;       /* +0x130, +0x134 */
    /* Queued blits: +0x2d8 active, +0x2dc list, +0x2e0 count, +0x2e8
       current index, +0x2ec/+0x2f0 offset. */
    int32_t blits_active;
    const AoeMainViewBlitRor *blits;
    int32_t blit_count, blit_index, blit_off_x, blit_off_y;
    /* Row start table: +0x328 surface it was built for (never written by
       0x00513980 itself), +0x32c table, +0x330 entry count. */
    const void *rows_surface;
    uint8_t **row_starts;
    int32_t row_count;
    void *aux_panel;        /* game view +0x360 (may be NULL) */
} AoeMainViewRor;

/* 0x0050fee0: attach `map` (NULL detaches): world and tile geometry copied
   (zeroed without a map), then the layout recomputed. */
void aoe_main_view_set_map_ror(AoeMainViewRor *v, void *map);
/* 0x0050ffa0: the viewed player set, the hovered point reset to
   AOE_MAIN_VIEW_NO_POINT_ROR, full redraw (2). */
void aoe_main_view_set_player_ror(AoeMainViewRor *v, void *player);
/* 0x0050fec0: the base mode change; mode 2 clears +0x142. */
void aoe_main_view_set_mode_ror(AoeMainViewRor *v, int32_t mode);
/* 0x0050f590: the base handler, then the area notified when the global
   flag is set. */
void aoe_main_view_handle_ror(AoeMainViewRor *v, int32_t value);
/* 0x0050fba0: the +0x324 overlay destroyed and cleared. */
void aoe_main_view_free_overlay_ror(AoeMainViewRor *v);
/* 0x00518e50 (game view): 1 when the game state is 4..7, also redrawing
   (1) in edit mode 1, 7 or 8; otherwise 0. */
int aoe_game_view_check_redraw_ror(AoeMainViewRor *v);
/* 0x0051a180 / 0x0051a1d0 (game view): after a successful base call
   (the base view handlers, result `base_ok`) the +0x360 panel is
   redrawn (1); returns 1 then, else 0. */
int aoe_game_view_after_base_ror(AoeMainViewRor *v, int base_ok);
/* 0x0051afb0 (game view): highlight cleared (-1 x4), redraw (1);
   returns 0. */
int aoe_game_view_clear_highlight_ror(AoeMainViewRor *v);
/* 0x005104a0: the next queued blit (unless `force` is 0 and the
   surface is still busy); the queue ends after its last entry, or at
   once without a list or with an out-of-range index. */
void aoe_main_view_step_blits_ror(AoeMainViewRor *v, int force);
/* 0x00510ef0: draw into `target` (NULL: the primary surface). Into the
   terrain buffer the panel rectangle and scroll are first shifted to the
   buffer origin and restored afterwards. Code 10 draws the full region,
   any other the generic draw. */
void aoe_main_view_draw_ror(AoeMainViewRor *v, uint8_t code, void *target);
/* 0x00511430: hit test; the value passed is `mask` unless `kind` is
   0x28 (then 0); result '4' stores the value as object_value, '3' as
   other_value. Returns the result character. */
uint8_t aoe_main_view_pick_ror(AoeMainViewRor *v, uint8_t kind, int32_t arg,
                               int32_t x, int32_t y, int32_t mask,
                               AoeMainViewPickRor *out);
/* 0x00514500: the cursor (client coordinates in windowed mode, 16-bit
   wrapped) plus (dx, dy) in *x / *y; 1 when inside the panel rectangle
   (inclusive). */
int aoe_main_view_cursor_in_view_ror(AoeMainViewRor *v, int32_t dx,
                                     int32_t dy, int32_t *x, int32_t *y);
/* 0x00513980: per panel row (top..bottom) the surface row address plus
   the panel left, rebuilt when the surface or the height differs (the
   table reallocated when the height changed or none exists). `surface`
   NULL or an empty height leaves it untouched. Returns 0 only when an
   allocation fails. */
int aoe_main_view_update_row_starts_ror(AoeMainViewRor *v,
                                        const void *surface,
                                        uint8_t *const *surface_rows);
/* Releases the row table (not an original entry point). */
void aoe_main_view_free_row_starts_ror(AoeMainViewRor *v);
/* 0x00518e00 (game view): the base draw timed into statistic slot 13;
   returns its result. */
int32_t aoe_game_view_timed_draw_ror(AoeMainViewRor *v, int32_t a, int32_t b,
                                     int32_t c, int32_t d, int32_t e);

/* ---- Construction and destruction (vtable 0x0054a61c) ---- */

/* Owned resources released by 0x0050f240, in its order. */
typedef enum {
    AOE_MAIN_VIEW_RES_SHAPE = 0,     /* +0x164 cursor shape (0x0049f840) */
    AOE_MAIN_VIEW_RES_PEN,           /* +0x15c red pen (DeleteObject) */
    AOE_MAIN_VIEW_RES_ROW_TABLE,     /* +0x32c row starts */
    AOE_MAIN_VIEW_RES_REGION0,       /* +0x178 (0x004a37f0) */
    AOE_MAIN_VIEW_RES_REGION1,       /* +0x17c */
    AOE_MAIN_VIEW_RES_REGION2,       /* +0x180 */
    AOE_MAIN_VIEW_RES_BUF0,          /* +0x184 */
    AOE_MAIN_VIEW_RES_BUF1,          /* +0x188 */
    AOE_MAIN_VIEW_RES_BUF2,          /* +0x2c8 */
    AOE_MAIN_VIEW_RES_BLITS,         /* +0x2dc */
    AOE_MAIN_VIEW_RES_DRAW_ROWS0,    /* +0x334 (0x005161c0) */
    AOE_MAIN_VIEW_RES_DRAW_ROWS1,    /* +0x338 */
    AOE_MAIN_VIEW_RES_LIST0,         /* +0x2fc..+0x30c, five lists */
    AOE_MAIN_VIEW_RES_TIMERS = AOE_MAIN_VIEW_RES_LIST0 + 5, /* +0x354 */
    AOE_MAIN_VIEW_RES_COUNT
} AoeMainViewResourceRor;

#define AOE_MAIN_VIEW_LISTS_ROR 5
/* Each list: 0x40 entries of 0x14 bytes. */
#define AOE_MAIN_VIEW_LIST_CAPACITY_ROR 0x40
#define AOE_MAIN_VIEW_LIST_ENTRY_SIZE_ROR 0x14

typedef struct {
    void *context;
    /* 0x00452750: the base panel constructor with the seven leading
       arguments; 0x004526b0 the base destructor (run twice by the
       original: once for the subobject, once for the base). */
    void (*base_construct)(void *context, const int32_t args[7]);
    void (*base_destruct)(void *context);
    /* GetStockObject(6) white pen, CreatePen(PS_SOLID, 1, RGB(255,0,0)),
       GetStockObject(5) null brush. */
    void *(*stock_object)(void *context, int32_t which);
    void *(*create_pen)(void *context, int32_t style, int32_t width,
                        uint32_t colour);
    /* operator new(0x20) + 0x0049f5f0(name, -1); NULL on failure. */
    void *(*load_shape)(void *context, const char *name);
    /* 0x50f5c0: the tile edge table set-up. */
    void (*init_edges)(void *context);
    void *(*alloc_zero)(void *context, uint32_t count, uint32_t size);
    /* Release of one owned resource (non-NULL only). */
    void (*release)(void *context, AoeMainViewResourceRor which, void *res);
    /* 0x0050fba0 (the overlay free) and 0x00515fb0 (the marks free). */
    void (*free_overlay)(void *context);
    void (*free_marks)(void *context);
    /* DAT_00582edc set: 0x0044bed0(0) then 0x00449b00(0). */
    int (*global_flag)(void *context);
    void (*clear_globals)(void *context);
    /* 0x00525fc2 operator delete of the object (deleting destructor). */
    void (*delete_self)(void *context, void *self);
} AoeMainViewLifeOpsRor;

typedef struct {
    void *link;               /* +0xf4, 0 */
    int32_t owner_arg;        /* +0xf8 */
    int16_t scroll[4];        /* +0x126..+0x12c, 0 */
    float point_x, point_y;   /* +0x138/+0x13c, -10000 */
    uint8_t flag_140;         /* +0x140, 0 */
    void *white_pen, *null_brush; /* +0x158, +0x160 (red pen: res) */
    void *res[AOE_MAIN_VIEW_RES_COUNT];
    int32_t list_count[AOE_MAIN_VIEW_LISTS_ROR]; /* +0x310.. */
} AoeMainViewLifeRor;

/* 0x0050f420: base constructor, the fields above, the cursor shape
   loaded as "<name>.shp", the edge tables, five zeroed lists. Returns 1. */
int aoe_main_view_construct_ror(AoeMainViewLifeRor *v,
                                const AoeMainViewLifeOpsRor *ops,
                                const int32_t base_args[7],
                                int32_t owner_arg, const char *shape_name);
/* 0x0050f240: releases everything in AoeMainViewResourceRor order
   (shape first, then the overlay, the pen, ...), the global clear when
   flagged, the timers, the marks, then the base destructor twice.
   0x0050f220 is the deleting form (`flags` bit 0 frees `self`);
   0x0047e9e0 is the same deleting form reached through a thunk of
   0x0050f240 (vtable slots 0x00546688 and 0x0054a840). */
void aoe_main_view_destruct_ror(AoeMainViewLifeRor *v,
                                const AoeMainViewLifeOpsRor *ops);
void *aoe_main_view_delete_ror(AoeMainViewLifeRor *v,
                               const AoeMainViewLifeOpsRor *ops,
                               uint8_t flags);

/* ---- Tile bucket grid (DAT_007d2058 singleton) ---- */

typedef struct {
    void *items;        /* 0x20 entries of 8 bytes */
    int16_t capacity;   /* 0x20 */
    int16_t count;      /* 0 */
} AoeViewGridCellRor;

#define AOE_VIEW_GRID_BUFFERS_ROR 5
#define AOE_VIEW_GRID_BUFFER_SIZE_ROR 0x800

typedef struct {
    AoeViewGridCellRor **rows;                       /* +0x00 */
    uint8_t *buffers[AOE_VIEW_GRID_BUFFERS_ROR];     /* +0x04..+0x14 */
    int32_t row_count;                               /* +0x18 */
    int32_t col_count;                               /* +0x1c (arg + 1) */
    int8_t *lookup;                                  /* +0x20 (0x516990) */
} AoeViewGridRor;

/* The globals the grid publishes: DAT_007d2038 buffer pointers,
   DAT_007d2024 their fill counts, DAT_007d2058 the live grid. */
typedef struct {
    uint8_t *buffers[AOE_VIEW_GRID_BUFFERS_ROR];
    int32_t used[AOE_VIEW_GRID_BUFFERS_ROR];
    AoeViewGridRor *current;
} AoeViewGridGlobalsRor;

/* 0x00516800: rows x (cols + 1) cells, each with a 0x20-entry array;
   five 0x800-byte buffers published to `g`; `build_lookup` is
   0x516990 (may be NULL). Allocation failures are not checked by the
   original; here a failed array leaves the cell with items NULL. */
void aoe_view_grid_init_ror(AoeViewGridRor *grid, AoeViewGridGlobalsRor *g,
                            int32_t rows, int32_t cols,
                            void (*build_lookup)(AoeViewGridRor *grid));
/* 0x005168d0: frees everything, clears the globals. */
void aoe_view_grid_free_ror(AoeViewGridRor *grid, AoeViewGridGlobalsRor *g);

/* ---- Small helpers ---- */

/* 0x005160d0 (game view): the +0x334 draw rows searched for `id` in rows
   6..0x28 with key 0 (`find` returns the node's +0x1c flags byte or
   NULL). 1 when found and either `any` is set or flag bit 0 is clear. */
int aoe_game_view_has_draw_node_ror(
    const uint8_t *(*find)(void *ctx, int32_t id, int32_t y0, int32_t y1,
                           int32_t key),
    void *ctx, int32_t id, int any);

/* 0x00517820: the radius footprint table index (clamped 0..16) and
   which of the two table sets (0x0055f7b0 when `alt`, else 0x0055f76c). */
int32_t aoe_view_footprint_index_ror(int32_t radius);
const void *aoe_view_footprint_table_ror(const void *const normal[17],
                                        const void *const alt_set[17],
                                        int32_t radius, int alt);

/* 0x00518700 (game view draw pass): 0x518720, 0x518a30, then the
   base 0x00453360. */
void aoe_game_view_draw_pass_ror(void *ctx, void (*first)(void *),
                                 void (*second)(void *),
                                 void (*base)(void *));

/* 0x00517e90: a width x height int32 grid read from a stream (width,
   height, then the cells). Reallocated only when the size changed; a
   non-positive size zeroes all three fields (the old array is not
   freed, as in the original). `read` returns nothing (0x0046b800). */
typedef struct {
    int32_t width, height; /* +0x00, +0x04 */
    int32_t *cells;        /* +0x08 */
} AoeIntGridRor;
void aoe_int_grid_read_ror(AoeIntGridRor *g,
                           void (*read)(void *ctx, void *buf, uint32_t n),
                           void *ctx);

/* 0x005116c0: the cursor action class for a hovered unit. `unit_kind`
   is the unit template's +0x8d byte, `is_target` that the view's +0x33c
   target mode is on and its +0x340 ID equals the unit's +4, `mode` the
   pointer mode. Kind 2: 5 target, 3 for modes 1/2, 2 for mode 3, else 0.
   Kind 3: 5 target, 3 for modes 1/2, else 0. Kind 4: 5 target, 4 for
   modes 1/2, 3 for mode 3, else 2. Other kinds: 1. */
uint8_t aoe_main_view_unit_cursor_ror(uint8_t unit_kind, int is_target,
                                      int32_t mode);

/* 0x005176b0: the whole visibility map filled with one value. */
typedef struct {
    int8_t **rows;              /* +0x00 per-player counts [y][x] */
    int32_t width, height;      /* +0x14, +0x18 */
    int32_t explored_count;     /* +0x1c */
    int32_t cell_total;         /* +0x20 */
    uint32_t visible_bits;      /* +0x24 */
    uint32_t clear_mask;        /* +0x28 */
    uint32_t explored_bits;     /* +0x2c */
    uint32_t unexplore_mask;    /* +0x30 */
    uint32_t **tile_masks;      /* global row table 0x007d205c */
} AoeVisibilityFillRor;
/* -1 (unexplored) and 0 (fogged) clear the visible bit in the shared
   tile masks (-1 also the explored bit); any other value sets both
   bits. The explored count becomes the cell total, or 0 for -1. */
void aoe_visibility_fill_ror(AoeVisibilityFillRor *m, int8_t value);

/* 0x0050cdf0: the leading player of a world with `count` players
   (index 0, gaia, excluded). */
typedef struct {
    void *context;
    int (*excluded)(void *context, int32_t player);  /* 0x00454150 != 0 */
    int32_t (*score)(void *context, int32_t player, int32_t category);
                                                     /* 0x0050ef70 */
    uint8_t (*colour)(void *context, int32_t player); /* player +0x54 */
    const char *(*name)(void *context, int32_t player); /* player +0x44 */
    uint8_t (*stance)(void *context, int32_t from, int32_t to); /* 0x0045da60 */
} AoeLeaderOpsRor;
typedef struct {
    int32_t score;   /* +0x00 */
    uint8_t colour;  /* +0x04 */
    char name[16];   /* +0x05 */
    uint8_t allies;  /* +0x15 */
} AoeLeaderRor;
/* The first strictly highest positive score among the non-excluded
   players wins (player 1 with score 0 otherwise); `allies` counts the
   other players with a zero stance both ways. */
void aoe_world_leader_ror(const AoeLeaderOpsRor *ops, int16_t count,
                          int32_t category, AoeLeaderRor *out);

/* 0x0051a210 (game view): one timed view render. */
typedef struct {
    void *context;
    uint32_t (*time_ms)(void *context);
    int32_t (*stat)(void *context, int32_t slot);   /* 0x00417430 */
    void (*record)(void *context, int32_t slot, uint32_t ms); /* 0x00417370 */
    void (*advance)(void *context);                 /* 0x004173c0 */
    int (*has_map)(void *context);                  /* +0xfc and +0x100 */
    /* Without a map: vtable +0x28(0), 0x00438b50(+0x24 area, +0xef
       colour), vtable +0x2c. */
    void (*begin_paint)(void *context, int32_t arg);
    void (*clear_area)(void *context);
    void (*end_paint)(void *context);
    int (*debug_log)(void *context);                /* DAT_00580d84 */
    void (*log)(void *context, const char *text);   /* 0x00416270 */
    void (*render)(void *context);                  /* 0x510580 */
} AoeGameViewRenderOpsRor;
/* Statistic 6: wall time less the slot-11 delta; 12: the slot-11
   delta. The debug log brackets the render with "<vs>" / "<vf>\n". */
void aoe_game_view_render_ror(const AoeGameViewRenderOpsRor *ops);

/* 0x00511790: the screen point of tile (x, y) of the world (width x
   height, `tiles[y][x]`), relative to the view origin (+0x130/+0x134):
   x = tile x - origin x; y = the slope's elevation offset (world +0x1c
   + slope * 6) - origin y + tile y. With `centre`, sloped tiles add half
   the elevation step (+0x110, truncated) and slopes 3, 5, 6 and 11
   subtract a full step. 16-bit arithmetic. Off the map: 0 and (0, 0). */
typedef struct {
    int16_t x, y;   /* +0x00, +0x02 */
    uint8_t slope;  /* +0x04 */
} AoeMainViewTileRor;
int aoe_main_view_tile_point_ror(const AoeMainViewRor *v,
                                 const AoeMainViewTileRor *const *tiles,
                                 int32_t width, int32_t height,
                                 const int16_t *slope_offsets, int16_t x,
                                 int16_t y, int16_t *out_x, int16_t *out_y,
                                 int centre);

/* 0x005150c0: four per-player edge queries (0x514fd0 with kinds 1, 4,
   3, 2 for `flags` bits 0, 1, 2, 3) into out[0], out[3], out[2], out[1]
   (all first -1). Returns 1 only when the player's world slot (world
   +0x33ac + player * 0x5a0) is active and `flags` is nonzero. */
int aoe_main_view_edge_query_ror(
    int player_active, uint8_t flags,
    int16_t (*query)(void *ctx, int32_t kind), void *ctx, int32_t out[4]);

/* 0x00519360 (game view): mouse-up while the game state is 4..7. */
typedef struct {
    void *context;
    int32_t (*game_state)(void *context);   /* DAT_00580e38 +0x1b0 */
    int32_t (*edit_mode)(void *context);    /* +0x1b4 */
    int (*comm_waiting)(void *context);     /* 0x004199c0 */
    int (*dragging)(void *context);         /* view +0x168 byte */
    void (*release_capture)(void *context); /* 0x00453db0 */
    /* Vtable +0xf0 (finish drag), +0xec (drag box, kind 2), +0xf4
       (cancel drag), +0x20 (redraw). */
    void (*finish_drag)(void *context, int32_t a, int32_t b);
    void (*drag_box)(void *context, int32_t kind, int32_t x0, int32_t y0,
                     int32_t x1, int32_t y1);
    void (*cancel_drag)(void *context);
    void (*redraw)(void *context, int32_t level);
    /* 0x00519d50 only: 0x00453d50 capture, the viewed player's +0x134
       and its clear 0x0045dcb0, the game's vtable +0x10 mode change. */
    void (*capture)(void *context);
    int (*player_pending)(void *context);
    void (*player_clear)(void *context);
    void (*set_game_mode)(void *context, int32_t mode, int32_t arg);
} AoeGameViewMouseOpsRor;
/* Outside states 4..7: 0. While the comm waits: a drag is cancelled
   (else the capture released); 0. Otherwise a drag is finished with
   (a, b); edit mode 0 releases the capture, boxes kind 2 from the press
   point (+0xc4, +0xc8) to (c, d) and finishes with (a, b); edit
   modes 1, 7, 8 redraw (1); returns 1. */
int aoe_game_view_mouse_up_ror(const AoeGameViewMouseOpsRor *ops,
                               int32_t press_x, int32_t press_y, int32_t a,
                               int32_t b, int32_t c, int32_t d);

/* 0x00515d30: end of a mouse drag (+0x168 kind, press point +0xc4/+0xc8,
   current point +0x16c/+0x170). */
typedef struct {
    void *context;
    /* ClientToScreen + SetCursorPos. */
    void (*warp_cursor)(void *context, int32_t x, int32_t y);
    /* 0x005114d0(4, left, top, right, bottom): area selection. */
    void (*select_area)(void *context, int32_t kind, int32_t l, int32_t t,
                        int32_t r, int32_t b);
    /* Vtable +0xf8 and +0x20. */
    void (*point_action)(void *context, int32_t x0, int32_t y0, int32_t x1,
                         int32_t y1, int32_t flag);
    void (*redraw)(void *context, int32_t level);
    void (*restore_mouse)(void *context);   /* 0x00419bb0 cursor show */
    void (*release_capture)(void *context); /* 0x00453db0 */
} AoeMainViewDragOpsRor;
/* Kinds 1, 6, 7, 8 warp the cursor back to the press point; 2 selects
   the normalised press..current rectangle and redraws (1); 9 runs the
   point action at the press point and redraws (1). Kinds 1, 2, 6, 7, 8
   then restore the mouse; the capture is always released and the kind
   cleared. */
void aoe_main_view_end_drag_ror(const AoeMainViewDragOpsRor *ops,
                                uint8_t *kind, int32_t press_x,
                                int32_t press_y, int32_t cur_x,
                                int32_t cur_y);

/* 0x0051afd0 (game view, panel vtable +0xd0): the help text under
   (x, y). When the view pick (kind 0x29, 2) hits an object ('4'), its
   help string (string table 0x004542a0 of the object's +0x98) is copied
   into `text` and its help page (+0x9c) into *page; returns 1.
   Otherwise the base panel query 0x004542e0 answers. */
typedef struct {
    void *context;
    void *(*pick_object)(void *context, int32_t x, int32_t y); /* NULL: miss */
    int32_t (*help_string_id)(void *context, void *object);   /* +0x98 */
    int32_t (*help_page)(void *context, void *object);        /* +0x9c */
    const char *(*string)(void *context, int32_t id);
    int32_t (*base_help)(void *context, char *text, int32_t *page,
                         int32_t x, int32_t y);
} AoeGameViewHelpOpsRor;
/* The original copies unbounded; `text_size` bounds it here. */
int32_t aoe_game_view_help_ror(const AoeGameViewHelpOpsRor *ops, char *text,
                               size_t text_size, int32_t *page, int32_t x,
                               int32_t y);

/* 0x00511310: the brush highlight under the mouse. Cleared (-1 x4)
   first; when the mouse is inside the panel (inclusive) and the pick
   (kind 0x28) hits the map ('3'), the picked map point (the pick's
   first two words as floats) is truncated and the highlight set to the
   brush size around it. In edit mode 0x13 the point snaps to the
   centre of its 3x3 block and the size is 3. */
void aoe_main_view_brush_highlight_ror(AoeMainViewRor *v);

/* 0x00519d50 (game view): mouse press. 0 while the comm waits or
   outside states 4..7. Edit mode 0x13: a pending player state is cleared
   (and the view redrawn, 1), the game mode set to (0x13, 1) and drag
   kind 9 started with (a, b, c, d); otherwise the mouse is captured.
   Returns 1. */
int aoe_game_view_mouse_down_ror(const AoeGameViewMouseOpsRor *ops,
                                 int32_t a, int32_t b, int32_t c, int32_t d);

/* 0x00519e00 (game view): mouse move while the game state is 4..7. While
   the comm waits a drag is cancelled (else the capture released); 0. A
   drag in progress is continued with (a, b); 1. Otherwise, unless the
   viewed player's +0x1a0 word (`player_busy`) is set (0), the capture is
   released, a kind-1 box drawn from the press point to (c, d) and the
   drag continued with (a, b); 1. */
int aoe_game_view_mouse_move_ror(const AoeGameViewMouseOpsRor *ops,
                                 int player_busy, int32_t press_x,
                                 int32_t press_y, int32_t a, int32_t b,
                                 int32_t c, int32_t d);

/* 0x0051ad40 (game view): a right-click map order. Only with map orders
   on (0x0055873c) and a non-empty selection (0x0045e0c0, its list freed
   at once). The pick 0x51a650(0x2a, 1, x, y, out, 0, 1) must not hit
   the interface ('2'); the point is kept on the map (0x0051b070), the
   viewed player's vtable +0xa0 ordered with (out[3], out[0], out[1]) and
   the view redrawn (1). The game mode is then reset (vtable +0x10 with
   0, 0). Returns the order's byte result (0 when nothing was ordered). */
typedef struct {
    void *context;
    int (*orders_enabled)(void *context);
    int (*has_selection)(void *context);
    uint8_t (*pick)(void *context, int32_t x, int32_t y, int32_t out[5]);
    void (*clamp_point)(void *context, int32_t out[5]);
    uint8_t (*order)(void *context, int32_t a, int32_t x, int32_t y);
    void (*redraw)(void *context, int32_t level);
    void (*set_game_mode)(void *context, int32_t mode, int32_t arg);
} AoeGameViewOrderOpsRor;
uint8_t aoe_game_view_map_order_ror(const AoeGameViewOrderOpsRor *ops,
                                    int32_t x, int32_t y);

/* 0x00515e60: an editor brush stroke from the previous map point
   (+0x2f4/+0x2f8, `*prev_x`/`*prev_y`) to the map point under (c, d).
   The pick (kind 0x28) of (c, d) must hit the map ('3', else 0); its
   float point is truncated; (a, b) is then picked as well (unused).
   Edit mode 9 with `paint` and an elevation brush (game +0x3fe != -1):
   map vtable +0x10(prev, point, size +0x402, +0x3fe); mode 10 likewise
   +0x18 with the terrain brush +0x400; mode 0x13: +0x20(prev, point, 0,
   game +0x1b8 == 1) and the view redrawn (2). The point becomes the
   previous one; 1. */
typedef struct {
    void *context;
    uint8_t (*pick)(void *context, int32_t x, int32_t y, float *fx,
                    float *fy);
    int32_t (*edit_mode)(void *context);   /* game +0x1b4 */
    int16_t elevation, terrain, size;      /* game +0x3fe, +0x400, +0x402 */
    int32_t cliff_mode;                    /* game +0x1b8 */
    void (*stroke)(void *context, int32_t slot, int32_t x0, int32_t y0,
                   int32_t x1, int32_t y1, int32_t a, int32_t b);
    void (*redraw)(void *context, int32_t level);
} AoeMainViewBrushOpsRor;
int aoe_main_view_brush_stroke_ror(const AoeMainViewBrushOpsRor *ops,
                                   int32_t *prev_x, int32_t *prev_y,
                                   int32_t a, int32_t b, int32_t c,
                                   int32_t d, int paint);


/* ---- Mouse drag start (C3L) ---- */

/* The drag state of the view: +0x168 kind, +0xc4/+0xc8 press point,
   +0x16c/+0x170 current point, +0x2f4/+0x2f8 the kind-9 map anchor. */
typedef struct {
    uint8_t kind;
    int32_t press_x, press_y;
    int32_t cur_x, cur_y;
    int32_t anchor_x, anchor_y;
} AoeMainViewDragStateRor;

typedef struct {
    void *context;
    /* 0x00453d50 mouse capture; 0x00419c20 (game) cursor hide. */
    void (*capture)(void *context);
    void (*hide_mouse)(void *context);
    /* Vtable +0xf8 (the point action, as in 0x00515d30). */
    int (*point_action)(void *context, int32_t x0, int32_t y0, int32_t x1,
                        int32_t y1, int32_t flag);
    /* The viewed player (+0x100): 0x0045dcb0 selection clear and
       0x0045d330 the view centre set to a map point. */
    void (*player_clear)(void *context);
    void (*set_view_point)(void *context, float x, float y);
} AoeMainViewDragStartOpsRor;

/* 0x005154e0: start a drag of `kind` at (x, y). Kind 3 first needs a map
   pick ('3', else 0 returned with nothing changed) and highlights that
   single tile; kind 9 records the picked tile (truncated) as the anchor
   and needs the point action (x, y, x, y, 1) to succeed (else 0), then
   redraws (1). The kind, press and current points are stored and the
   mouse captured; kinds 1, 2, 6, 7, 8 hide the cursor; kind 2 clears the
   selection unless `keep_a` or `keep_b` and redraws (1); kinds 6 and 8
   centre the view on the picked map point plus 0.5 and redraw (1),
   returning 1 on a map hit. Everything else returns 0. */
int aoe_main_view_begin_drag_ror(AoeMainViewRor *v,
                                 AoeMainViewDragStateRor *drag,
                                 const AoeMainViewDragStartOpsRor *ops,
                                 uint8_t kind, int32_t x, int32_t y,
                                 int32_t keep_a, int32_t keep_b);

/* ---- Pixel picking in the draw rows (C3L) ---- */

/* The shape frame header a draw node points at (+0x0c): the command
   table and outline table offsets (from the shape data base, +0x20 of
   the node), width (+0x10) and height (+0x14). */
typedef struct {
    uint32_t cmd_table_offset;   /* +0x00 */
    uint32_t outline_offset;     /* +0x04 */
    int32_t width, height;       /* +0x10, +0x14 */
} AoeShapeHitFrameRor;

/* 0x005149d0: what pixel (x, y) of the frame is. 0: outside the frame,
   its row's outline (a negative left edge, or x left of the left edge
   or right of width - right edge) or on an extended (0x?e) / end-of-row
   (0x?f) command; 2: inside a colour run (low bits 0/1 of the command
   byte = 0, 2-bit count; low nibble 2 the 12-bit count
   (byte << 4 | next) with that many bytes), a player-colour run (6), a
   fill (7), a player-colour fill (0xa) or a shadow run (0xb), the 4-bit
   counts of 6/7/0xa/0xb meaning "next byte" when 0; 1 otherwise (a skip
   run (1, low nibble 3 the 12-bit form), or the row end).
   `size` bounds the reads here (out of bounds answers 0); the original
   trusts the data. */
int aoe_view_shape_hit_ror(const uint8_t *data, size_t size,
                      const AoeShapeHitFrameRor *frame, int32_t x, int32_t y);

/* One draw-row node (64 bytes, the rows' +0x5c heads) as the pick
   routines read it. */
typedef struct {
    const void *next;            /* +0x04 */
    int32_t object_id;           /* +0x08 */
    const AoeShapeHitFrameRor *frame; /* +0x0c */
    int16_t field_14, field_18;  /* +0x14, +0x18 (low words) */
    uint32_t flags;              /* +0x1c (bit 1: mirrored) */
    const uint8_t *shape;        /* +0x20 */
    size_t shape_size;
    int32_t key;                 /* +0x2c */
    int16_t left, top, right, bottom; /* +0x30..+0x36 */
    int16_t depth;               /* +0x3a (0x005145a0 only) */
} AoeViewDrawItemRor;

typedef struct {
    void *context;
    const void *(*row_head)(void *context, int32_t row);
    void (*item)(void *context, const void *node, AoeViewDrawItemRor *out);
    /* 0x00514b60 only: the world object (map +0x88 table) of an ID: its
       owner's +0x4a player number, its +0x48 state and its template's
       +0x8d kind. 0 when there is none. */
    int (*object)(void *context, int32_t id, int16_t *player,
                  uint8_t *state, uint8_t *kind);
} AoeViewDrawRowsOpsRor;

#define AOE_VIEW_DRAW_ROWS_MAX_ROR 0x28

/* 0x00514d70: the object drawn at (x, y). Nothing (0, *id 0) unless
   (x, y) is inside the panel rectangle (inclusive) and the rows lo..hi
   (clamped to 0..0x28) are not empty. Rows are scanned from hi down;
   in each row every node with key 0 is hit-tested (mirrored nodes at
   right - x, others at x - left; y - top) and the last opaque one ('2')
   wins; the first row with a hit returns 1 with its object ID. */
int aoe_main_view_pixel_object_ror(const AoeMainViewRor *v,
                                   const AoeViewDrawRowsOpsRor *rows,
                                   int32_t x, int32_t y, int32_t lo,
                                   int32_t hi, int32_t *id);

/* The 64-entry pick result table (0x007d1cf8, 12-byte records). */
typedef struct {
    int32_t id;
    int16_t count, field_6;      /* both 1 */
    int16_t field_14, field_18;  /* the node's */
} AoeViewPickedRor;
#define AOE_VIEW_PICKED_MAX_ROR 0x40

/* 0x00514b60: the objects drawn in the panel-relative box (x0, y0) ..
   (x1, y1) (screen coordinates, the panel origin subtracted) in rows
   lo..hi (scanned from hi down; nothing unless the box meets the panel
   and the rows are not empty). A node counts when its key is 0, its ID
   non-negative, its box overlaps (inclusive), and its object is owned by
   `player` (-1: anyone), in state 2 and of kind >= `min_kind`. Each ID
   is listed once (a repeat clamps its count to 1); returns the count,
   stopping at 0x40. */
int32_t aoe_main_view_box_objects_ror(const AoeMainViewRor *v,
                                      const AoeViewDrawRowsOpsRor *rows,
                                      int32_t x0, int32_t y0, int32_t x1,
                                      int32_t y1, int32_t lo, int32_t hi,
                                      int32_t player, uint8_t min_kind,
                                      AoeViewPickedRor out[AOE_VIEW_PICKED_MAX_ROR]);

/* ---- Line/box draw nodes (C3L) ---- */

/* A draw-row node of a non-sprite kind (+0x2c 1..4): +0x1c colour,
   four 16-bit points at +0x30..+0x3e. */
typedef struct {
    int32_t kind;
    uint8_t colour;
    int16_t pt[8];   /* x0, y0, x1, y1, x2, y2, x3, y3 */
} AoeViewLineNodeRor;

typedef struct {
    void *context;
    /* Dirty region (span region) adds: 0x004a3950 columns x0..x1 of row
       y, 0x004a3e00 a line. */
    void (*add_span)(void *context, int32_t x0, int32_t x1, int32_t y);
    void (*add_line)(void *context, int32_t x0, int32_t y0, int32_t x1,
                     int32_t y1);
    /* 0x00439960 on the target surface. */
    void (*draw_line)(void *context, int32_t x0, int32_t y0, int32_t x1,
                      int32_t y1, uint8_t colour);
} AoeViewLineOpsRor;

/* 0x00513a40: the node's dirty area. Kind 1: the closed quad
   p0-p1-p2-p3 as lines. Kinds 2/3: rows y0..y1 (either order) of
   columns x0 & ~3 .. x1 | 3. Kind 4: the outline of the box p0..p1
   (either order), each edge widened to its group of four columns. */
void aoe_view_line_node_dirty_ror(const AoeViewLineNodeRor *n,
                                  const AoeViewLineOpsRor *ops);
/* 0x00513bc0: the node drawn. Kind 1: the closed quad in the node
   colour. Kind 2: for each row y0..y1, when y2 is nonzero the run
   x0..x2 in 0x4a and when y3 is nonzero the run x3..x1 in 0x97.
   Kind 4: the box p0..p1 in 0xff. Kind 3 draws nothing. */
void aoe_view_line_node_draw_ror(const AoeViewLineNodeRor *n,
                                 const AoeViewLineOpsRor *ops);

/* ---- Wall dragging (tribe main view) ---- */
typedef struct {
    void *context;
    int editing;           /* 0x0055873c */
    int32_t game_mode;     /* game (0x00580e38) +0x1b0; 7 is the editor */
    int has_template;      /* game +0x3fc names a template the player has */
    /* Template +0x20(player, x, y, 0, 1, player +0x64, 0, 1, 1, 1):
       nonzero when the tile is refused. */
    int (*blocked)(void *context, float x, float y);
    /* Player +0x94(template +0x10, x, y, 0, 1): place one. */
    void (*place)(void *context, float x, float y);
    /* 0x004f1170(template, x0, y0, x1, y1): the wall command. */
    int (*command)(void *context, int32_t x0, int32_t y0, int32_t x1, int32_t y1);
    void (*game_10)(void *context);      /* game vtable +0x10(0, 0) */
    void (*clear_marks)(void *context);  /* 0x0050ffd0(-1, -1, -1, -1) */
    void (*redraw)(void *context, int32_t level); /* vtable +0x20 */
} AoeMainViewWallOpsRor;

/* 0x00509050: one straight or diagonal run of tiles from (x0, y0) to
   (x1, y1), both ends included, each step +-1 or 0 per axis; at every
   tile centre (+0.5) not refused by the template a wall piece is
   placed. */
void aoe_main_view_wall_run_ror(const AoeMainViewWallOpsRor *ops, int32_t x0, int32_t y0,
                                int32_t x1, int32_t y1);
/* 0x00508e80(x0, y0, x1, y1, ack): 0 unless editing (0x0055873c). Out of
   the editor the wall command (0x004f1170, with a template) gives the
   result and `ack` calls game +0x10. In the editor with a template the
   drag is split into runs: a diagonal one when |dx| == |dy|, else along
   the longer axis first (to (x1, y0) or (x0, y1)) and then the rest from
   the next tile (y0 + 1 or x0 + 1 on the increasing side, y0 - 1 or
   x0 - 1 on the decreasing one); returns 1. Without a template 0. The
   marks are cleared and the view redrawn (+0x20(1)) on every edited
   path. */
int aoe_main_view_wall_drag_ror(const AoeMainViewWallOpsRor *ops, int32_t x0, int32_t y0,
                                int32_t x1, int32_t y1, int ack);

/* 0x00509880 (game view, the 0x005116c0 override): as
   aoe_main_view_unit_cursor_ror, but templates 0x45, 0x4f, 199 and
   0x116 count as kind 3, the target is the view's +0x340 unit while
   +0x33c is set, and a kind-3 unit in mode 3 answers 2 when its +0x48
   byte is at most 1 (else 0). */
uint8_t aoe_game_view_unit_cursor_ror(uint8_t unit_kind, int16_t template_id,
                                      uint8_t unit_48, int is_target,
                                      int32_t mode);

/* 0x005099b0 (game view): double click. Outside game state 7, when the
   pick (0x51a650 kind 0x29) hits an object ('4') of template kind 4
   (+0x8d) owned by the viewed player: without either modifier the
   selection is cleared (0x0045dcb0); the object is selected (0x0045dc10),
   then every unit of the player's list with the same template ID (+0x10)
   or, for class 3 (+0x8f), the same class, that is drawn on screen
   (0x005160d0(ID, 0)), and the object once more; 1. Otherwise 0. */
typedef struct {
    int16_t template_id;   /* template +0x10 */
    uint8_t kind;          /* template +0x8d */
    uint8_t unit_class;    /* template +0x8f */
    int32_t id;            /* +0x04 */
    int owned;             /* +0x0c is the viewed player */
} AoeGameViewUnitRor;
typedef struct {
    void *context;
    int32_t (*game_state)(void *context);
    /* NULL on a miss; `*info` filled on a hit. */
    void *(*pick_object)(void *context, int32_t x, int32_t y,
                         AoeGameViewUnitRor *info);
    int32_t (*unit_count)(void *context);   /* viewed player's +0x28 list */
    void *(*unit_at)(void *context, int32_t index, AoeGameViewUnitRor *info);
    int (*on_screen)(void *context, int32_t id);
    void (*clear_selection)(void *context);
    void (*select)(void *context, void *unit);
} AoeGameViewSelectOpsRor;
int aoe_game_view_select_same_ror(const AoeGameViewSelectOpsRor *ops,
                                  int32_t x, int32_t y, int shift,
                                  int ctrl);

/* 0x0051a4d0 (game view; 0x00509990 wraps it): a command at (x, y).
   Only with map orders on and a selection (as 0x0051ad40). A target
   under the point (0x51aa10 with `arg`) gets the viewed player's
   vtable +0xa8 order with its +0x38/+0x3c; on success its ID (+0x04)
   flashes (0x0050f970(ID, 1500, 2, 2)). The marks are then cleared
   (0x00515fb0), the view redrawn (1) and the order's result returned.
   With no target and `allow_move`, a map hit ('3', pick kind 0x28) is
   kept on the map and ordered (vtable +0xa0(0, x, y)); on success the
   kind-2 timers rewind (0x0051b0f0) and a mark (0x00516050: the
   player's +0x08, 0, view origin +0x130/+0x134 plus the point, 0, 0xf,
   0, 2, 0x5a) is added and the view redrawn (1); its result is
   returned. Otherwise the marks are cleared, the view redrawn; 0. */
typedef struct {
    void *context;
    int (*orders_enabled)(void *context);
    int (*has_selection)(void *context);
    void *(*find_target)(void *context, int32_t x, int32_t y, int32_t arg);
    uint8_t (*order_target)(void *context, void *target); /* vtable +0xa8 */
    void (*flash)(void *context, void *target);
    void (*clear_marks)(void *context);
    void (*redraw)(void *context, int32_t level);
    uint8_t (*pick_map)(void *context, int32_t x, int32_t y, float point[2]);
    void (*clamp_point)(void *context, float point[2]);
    uint8_t (*order_move)(void *context, float x, float y); /* vtable +0xa0 */
    void (*rewind_timers)(void *context, int32_t kind);
    void (*add_mark)(void *context, int32_t x, int32_t y);
    /* 0x0051ae10 only: the game's vtable +0x10. */
    void (*set_game_mode)(void *context, int32_t mode, int32_t arg);
} AoeGameViewCommandOpsRor;
uint8_t aoe_game_view_command_ror(const AoeGameViewCommandOpsRor *ops,
                                  int32_t origin_x, int32_t origin_y,
                                  int32_t x, int32_t y, int allow_move,
                                  int32_t arg);
/* 0x0051ae10 (game view): the alternate command. As 0x0051a4d0 with
   `arg` -1 and a move always allowed, but both orders go through the
   viewed player's vtable +0xa4 (the caller's order_target/order_move),
   a map hit redraws whether or not the order succeeded, and every path
   past the selection check ends with the game mode reset (0, 0). */
uint8_t aoe_game_view_alt_command_ror(const AoeGameViewCommandOpsRor *ops,
                                      int32_t origin_x, int32_t origin_y,
                                      int32_t x, int32_t y);

/* ---- Tile object pick (C3L) ---- */

typedef struct {
    void *context;
    /* Object +0x48 state, template +0x65 (selectable) and +0x8d kind,
       +0x0c owner, +0x28/+0x2a screen offset. */
    uint8_t (*state)(void *context, void *obj);
    int (*selectable)(void *context, void *obj);
    uint8_t (*kind)(void *context, void *obj);
    void *(*owner)(void *context, void *obj);
    void (*offset)(void *context, void *obj, int16_t *x, int16_t *y);
    /* The live-unit box test (x, y, a.x, a.y, b.x, b.y) and 0x004aafe0 (x, y, b.x,
       b.y, 5): the object's sprite hit tests (nonzero: hit). */
    int (*hit_box)(void *context, void *obj, int32_t x, int32_t y,
                   const int16_t a[3], const int16_t b[3]);
    int (*hit)(void *context, void *obj, int32_t x, int32_t y,
               const int16_t b[3], int32_t slack);
    /* 0x0045dc10(obj, 0) on the viewed player: 0 when the selection is
       full. 0x004aa8e0: the sprite offset (0: none). */
    int (*select_add)(void *context, void *obj);
    int (*sprite_offset)(void *context, void *obj, int32_t *dx,
                         int32_t *dy);
} AoeTilePickOpsRor;

/* 0x005152f0: the object of a tile under (x, y). `objects` is the tile's
   list in draw order (0x004512c0, built and freed here by the original;
   `tile_present` 0 is the NULL tile). Objects in state >= 7, not
   selectable while `selectable_only`, or of kind < `min_kind` (view
   +0x141) are skipped. Pick modes '+' and ',' (view +0x140) take only
   the viewed player's objects; ',' adds every box hit to the selection
   (stopping when it is full) and returns the last added. Otherwise the
   best hit (slack 5) wins: `preferred` returns at once when something
   was already found, an object loses to an earlier pick of the viewed
   player's when it is not the player's, and to a higher kind. The
   winner's screen point (offset + sprite offset + (x, y)) goes to
   *out_x / *out_y. */
void *aoe_main_view_tile_object_ror(const AoeTilePickOpsRor *ops,
                                    int tile_present, void *const *objects,
                                    int32_t count, void *view_player,
                                    uint8_t pick_mode, uint8_t min_kind,
                                    int32_t x, int32_t y, const int16_t a[3],
                                    const int16_t b[3], int16_t *out_x,
                                    int16_t *out_y, void *preferred,
                                    int selectable_only);

/* 0x005151d0: does the terrain tile drawn at (left, top) cover `point`?
   `tile` NULL counts as shape 0, terrain 0; else shape = byte +4,
   terrain = byte +5 & 0x1f. Outside the shape's box (the terrain set
   +0x104's +0x18 / +0x1a words per shape, stride 6): 0. Inside: 1 when
   the terrain is out of range (+0x8d90 count), not loaded (+0x8c of its
   0x198-byte record), has no art (+0xac) or no frame (0x00514ee0(terrain,
   shape, tx, ty) gives -1); otherwise the art's pixel test 0x0049fb90 at
   the point relative to (left, top). */
typedef struct {
    void *context;
    int16_t (*shape_width)(void *context, int32_t shape);   /* +0x18 */
    int16_t (*shape_height)(void *context, int32_t shape);  /* +0x1a */
    int16_t terrain_count;                                   /* +0x8d90 */
    int (*loaded)(void *context, int32_t terrain);
    void *(*art)(void *context, int32_t terrain);
    int16_t (*frame)(void *context, int32_t terrain, int32_t shape,
                     int32_t tx, int32_t ty);
    int (*pixel)(void *context, void *art, int32_t x, int32_t y,
                 int32_t frame);
} AoeMainViewTerrainHitOpsRor;
int aoe_main_view_terrain_hit_ror(const AoeMainViewTerrainHitOpsRor *ops,
                                  const uint8_t *tile, int16_t left,
                                  int16_t top, int32_t tx, int32_t ty,
                                  const int32_t point[2]);

/* Vtable slot 0x00546788 0x00508d50(x, y, ack): a click placing a
   building. 0 unless editing (0x0055873c); out of the editor (game +0x1b4
   not 7) the base handler 0x0051a2e0(x, y, 1) answers. Otherwise the
   picked point (0x51a650(0x28, 0, x, y, &point, 0, 1) must hit a tile,
   code '3') is snapped (0x0051b070); with a template selected (game
   +0x3fc) that the player (+0x100) has, the template adjusts the point
   (+0x24, view +0xfc) and, unless it refuses it (+0x20), `ack` or
   template 0x6d calls game +0x10(0, 0) and the build command 0x004f10e0
   (id, x, y) gives the result. */
typedef struct {
    void *context;
    int editing;           /* 0x0055873c */
    int32_t mode_1b4;      /* game +0x1b4 */
    int16_t template_id;   /* game +0x3fc, -1 none */
    int has_template;      /* the player's +0x24[id] */
    int (*base_click)(void *context, int32_t x, int32_t y, int32_t flag);
    uint8_t (*pick)(void *context, int32_t x, int32_t y, float point[2]);
    void (*snap)(void *context, float point[2]);
    void (*adjust)(void *context, float point[2]);
    int (*blocked)(void *context, float x, float y);
    void (*game_10)(void *context);
    int (*build)(void *context, int16_t id, float x, float y);
} AoeMainViewPlaceOpsRor;
int aoe_main_view_place_click_ror(const AoeMainViewPlaceOpsRor *ops, int32_t x, int32_t y,
                                  int32_t ack);


/* 0x00516c10: the objects of a player's class lists near a tile. The
   lists (per player, per class: a node list and its count) hold 8-byte
   nodes: the object (+0), a byte (+4), the kind (+5) and the tile x / y
   (+6 / +7). For each class in `classes` every node within `radius`
   (clamped to 15) of (cx, cy) by the 16 x 16 distance table is appended
   to the bucket `kind_bucket[kind]` (five buckets of 256 at 0x007d2024 /
   0x007d2038) with its byte +4 replaced by the distance; the buckets
   are emptied first. Returns the number appended; 0 for an empty class
   list or a player past the table. */
typedef struct {
    int32_t object;
    uint8_t byte_4, kind, x, y;
} AoeViewClassNodeRor;

typedef struct {
    const AoeViewClassNodeRor *nodes;
    uint16_t count;
} AoeViewClassListRor;

typedef struct {
    void *context;
    int32_t player_count;                     /* +0x18 */
    const uint8_t (*distance)[16];            /* +0x20: [dy][dx] */
    /* The class list of a player (NULL when the class has none). */
    const AoeViewClassListRor *(*list)(void *context, int32_t player, int32_t class_id);
} AoeViewClassListsRor;

#define AOE_VIEW_NEARBY_BUCKETS 5
#define AOE_VIEW_NEARBY_MAX 0x100
typedef struct {
    int32_t count[AOE_VIEW_NEARBY_BUCKETS];
    AoeViewClassNodeRor entry[AOE_VIEW_NEARBY_BUCKETS][AOE_VIEW_NEARBY_MAX];
} AoeViewNearbyRor;

int32_t aoe_view_collect_nearby_ror(const AoeViewClassListsRor *lists, int32_t cx, int32_t cy,
                                    int32_t radius, int32_t player, const int32_t *classes,
                                    int32_t class_count, const uint8_t kind_bucket[256],
                                    AoeViewNearbyRor *out);


/* 0x0050fbd0: the terrain overlay rebuilt. True at once for a zero-size
   view. Without an overlay a "Terrain Buffer" surface (0x004382a0) is
   made and initialised (0x00438500) from the primary surface at the
   view's size; with one and a positive height: cleared (0x00438890),
   its blit origin (+0x2d0 / +0x2d4) and surface origin (0x00438f40)
   zeroed, both draw-row tables (+0x334 / +0x338) replaced (0x005161c0
   freed; 0x00516110 for 2 * (height + 450) rows, height + 450, 0x40,
   -200, height + 249, 40) and marked dirty over the whole view
   (0x00516360). Returns whether an overlay exists. */
typedef struct {
    void *context;
    void *(*surface_create)(void *context, const char *name);
    int (*surface_init)(void *context, void *surface, void *primary, int32_t w, int32_t h);
    void (*surface_clear)(void *context, void *surface, int32_t w, int32_t h);
    void (*surface_set_origin)(void *context, void *surface, int32_t x, int32_t y);
    void (*draw_rows_free)(void *context, void *rows);
    void *(*draw_rows_new)(void *context, int32_t rows2, int32_t rows, int32_t a, int32_t b,
                           int32_t c, int32_t d);
    void (*dirty)(void *context, void *rows, int32_t x0, int32_t y0, int32_t x1, int32_t y1);
    /* 0x0050fa10 setup: the panel's 0x00452920, the layout 0x0050fde0,
       the three span regions (0x004a37f0 freed, 0x004a3710(w, h) new),
       the game region 0x00449b40 and 0x0044bed0(1) when 0x00582edc is
       set, the redraw (+0x20(2)). */
    void (*base_setup)(void *context, int32_t x, int32_t y, int32_t w, int32_t h);
    void (*layout)(void *context);
    void (*region_free)(void *context, void *region);
    void *(*region_new)(void *context, int32_t w, int32_t h);
    int (*game_region_enabled)(void *context);
    void (*game_region)(void *context, int32_t x0, int32_t y0, int32_t x1, int32_t y1);
    void (*pointer_switch)(void *context, int32_t on);
    void (*redraw)(void *context, int32_t level);
} AoeMainViewSetupOpsRor;

typedef struct {
    int32_t width, height;     /* +0x14, +0x18 */
    int32_t blit_origin_x, blit_origin_y; /* +0x2d0, +0x2d4 */
    void *draw_rows[2];        /* +0x334, +0x338 */
    void *regions[3];          /* +0x178, +0x17c, +0x180 */
} AoeMainViewBuffersRor;

int aoe_main_view_rebuild_overlay_ror(AoeMainViewRor *v, AoeMainViewBuffersRor *b,
                                      const AoeMainViewSetupOpsRor *o);
void aoe_main_view_setup_ror(AoeMainViewRor *v, AoeMainViewBuffersRor *b,
                             const AoeMainViewSetupOpsRor *o, int32_t x, int32_t y, int32_t w,
                             int32_t h);

/* 0x00508bb0 (game view): the placement preview at a click. With a tile
   under (x, y) (0x00511790): the point is the tile plus 0.5; the
   template's footprint corners (0x004433c0 with the map) are taken; the
   placement check (template +0x20 with the player, the point, 0, 1, the
   template's +0x64 byte, 0, 1, 1, 1) failing sets the cursor code
   (0x007c0458) to 5, else to ((v < 4 ? v + 0x24 : 0x2c - v) << 4) | 9 for
   v = bits 7..9 of the template's +0x04; with the template's +0x18 the
   preview is drawn (+0x34: the area, the corner midpoint, the player's
   colour) and 0x007c0448 takes the midpoint's y. The code is cleared
   afterwards (the original leaves 0 in it); returns 1. */
typedef struct {
    void *context;
    int (*tile_at)(void *context, int32_t x, int32_t y, int32_t *tx, int32_t *ty);
    void (*corners)(void *context, float fx, float fy, int16_t out[8]); /* l, t, r, b pairs */
    int (*can_place)(void *context, float fx, float fy);
    uint32_t (*template_flags)(void *context);   /* template +0x04 */
    int (*template_has_graphic)(void *context);   /* template +0x18 */
    void (*draw_preview)(void *context, int32_t x, int32_t y);
} AoeMainViewPreviewOpsRor;
int aoe_main_view_place_preview_ror(const AoeMainViewPreviewOpsRor *o, int32_t x, int32_t y,
                                    int32_t *cursor_code, int32_t *preview_y);


/* 0x00511080 (editor/game view): the placement preview under the game
   mouse. Needs a selected template (game +0x3fc != -1 with a non-null
   template in the player's table) and the mouse inside the view rect
   (+0x8c..+0x98); the view pick must hit terrain
   ('3'). The pick point goes through the template's +0x24 snap, then the
   tile check and footprint corners; the
   placement check (+0x20) sets the cursor code as in the RoR sibling.
   With the template's +0x18 the preview is drawn at the corner midpoint
   with facing bit (+0xa8 & 1) and shape 0x97 when placeable else +0xa9;
   in game mode 8 / sub-mode 1 with a relocation object at +0x174 the
   relocated object's colour and its +0x35 byte are used. The cursor code
   is cleared afterwards. Returns 1 when the preview path ran. */
typedef struct {
    void *context;
    int (*has_template)(void *context);
    void (*mouse)(void *context, int32_t *x, int32_t *y);
    int (*pick_terrain)(void *context, int32_t vx, int32_t vy); /* nonzero for '3' */
    int (*tile_at)(void *context, int32_t vx, int32_t vy, int32_t *tx, int32_t *ty);
    void (*corners)(void *context, float fx, float fy, int16_t out[8]);
    int (*can_place)(void *context, float fx, float fy);
    uint32_t (*template_flags)(void *context);
    int (*template_has_graphic)(void *context);
    uint8_t (*template_facing)(void *context);   /* +0xa8 */
    uint8_t (*template_shape)(void *context);    /* +0xa9 */
    int (*relocating)(void *context);            /* mode 8/1 and +0x174 set */
    void (*draw_preview)(void *context, int32_t x, int32_t y, int relocated,
                         uint32_t facing, uint32_t shape);
} AoeMainViewEditorPreviewOps;
int aoe_main_view_editor_preview(const AoeMainViewEditorPreviewOps *o, int32_t left,
                                 int32_t top, int32_t right, int32_t bottom,
                                 int32_t *cursor_code, int32_t *preview_y);

/* 0x00519ed0 (game view): the right-click handler. Only in game modes
   4..7. A pending drag (+0x168) is cancelled (+0xf4) -> 1. Otherwise
   runs; in sub-mode 0x14 a pick hitting an
   object ('4') tells it +0x78(-1) -> 1. Else a busy check
   or a selection kind (+0xec byte) other than 1/2 returns 0. For an
   observer-less game (player +0x8dc == 2): outside modes 5/7, sub-mode 0
   either issues the default order (+0x104 with x, y, 1, -1) when no
   unit is given, or with a found unit and the rally feature
   on, a terrain pick ('3') sets the rally point (,
   view +0xd8 with 1.0f, (2)), flashes and redraws
   (+0x20(1)); sub-mode 0x15 clears the box and redraws;
   any other sub-mode resets the mode (+0x10(0,0)). Otherwise outside
   mode 7, sub-mode 0 cancels the selection and redraws,
   anything else resets the mode. */
typedef struct {
    void *context;
    int32_t mode, sub_mode;
    int drag_pending;
    uint8_t selection_kind;
    int full_player; /* player +0x8dc == 2 */
    int rally_enabled;
    void (*cancel_drag)(void *context);
    void (*prepare)(void *context);
    int (*pick_object)(void *context, int32_t x, int32_t y); /* nonzero for '4' */
    void (*object_notify)(void *context);
    int (*busy)(void *context);
    void (*default_order)(void *context, int32_t x, int32_t y);
    int (*find_unit)(void *context, int32_t unit);
    int (*pick_terrain)(void *context, int32_t x, int32_t y); /* nonzero for '3' */
    void (*set_rally)(void *context);
    void (*clear_box)(void *context);
    void (*redraw)(void *context);
    void (*reset_mode)(void *context);
    void (*cancel_selection)(void *context);
} AoeMainViewRightClickOps;
int aoe_main_view_right_click(const AoeMainViewRightClickOps *o, int32_t x, int32_t y,
                              int32_t unit);


/* 0x00510190: the scroll blit queue for a shift of (dx, dy) of the view
   rectangle `src` within the clip rectangle `clip` (nothing for 0, 0).
   Disjoint rectangles queue one blit of the whole view to the clip.
   Otherwise the strips: the axis with fewer steps ((extent + |d|) / |d|)
   is chosen; each record covers a strip of the view (its first row /
   column shifted for a negative d) whose destination is the strip less
   (dx, dy), stepping by (dx, dy) and clamping to the view. The list
   (+0x2dc, capacity +0x2e4) grows when short; the queue becomes active
   (+0x2d8) with the count, index -1 and the offset (+0x8c, +0x90). */
typedef struct {
    AoeMainViewBlitRor *list;  /* +0x2dc */
    int32_t capacity;          /* +0x2e4 */
} AoeMainViewBlitQueueRor;
int aoe_main_view_queue_scroll_ror(AoeMainViewRor *v, AoeMainViewBlitQueueRor *q,
                                   const int32_t src[4], const int32_t clip[4], int32_t dx,
                                   int32_t dy);


/* ---- 0x0050eff0: the argument-less constructor (vtable 0x0054a61c) ---- */

/* The fields +0xf4..+0x35c as the constructor leaves them. */
typedef struct {
    void *link;                 /* +0xf4 */
    int32_t fields_f8[4];       /* +0xf8..+0x104 */
    int16_t shorts_108[20];     /* +0x108..+0x12e */
    int32_t field_130, field_134;
    float point_x, point_y;     /* +0x138 / +0x13c: -10000 */
    uint8_t flag_140, flag_142;
    int16_t field_154;          /* -1 */
    void *white_pen, *red_pen, *null_brush, *shape; /* +0x158..+0x164 */
    uint8_t byte_168;
    int32_t field_16c, field_170; /* -1 */
    int32_t field_174;
    void *regions[3];           /* +0x178..+0x180 */
    void *buffers[2];           /* +0x184, +0x188 */
    void *buffer2;              /* +0x2c8 */
    int32_t fields_2cc[10];     /* +0x2cc..+0x2f0 */
    void *lists[AOE_MAIN_VIEW_LISTS_ROR];        /* +0x2fc.. */
    int32_t list_count[AOE_MAIN_VIEW_LISTS_ROR]; /* +0x310.. */
    void *overlay;              /* +0x324 */
    int32_t fields_328[3];      /* +0x328..+0x330 */
    void *draw_rows[2];         /* +0x334, +0x338 */
    int32_t field_33c;
    int32_t field_340;          /* INT32_MIN */
    int32_t fields_344[4];      /* +0x344..+0x350 */
    void *timers;               /* +0x354: 8 entries of 0x14 */
    int32_t timer_count, timer_capacity; /* +0x358, +0x35c: 0, 8 */
} AoeGameViewDefaultsRor;

typedef struct {
    void *context;
    void (*base_construct)(void *context);   /* 0x00452580 */
    void (*set_highlight)(void *context, int32_t x0, int32_t y0, int32_t x1, int32_t y1); /* 0x0050ffd0 */
    void *(*alloc_zero)(void *context, uint32_t count, uint32_t size); /* 0x0052607b */
    int32_t *globals[2];                     /* 0x007d201c, 0x007d2020: cleared */
} AoeGameViewDefaultOpsRor;

/* 0x0050eff0: the base panel constructor, the vtable, every field above
   zeroed except the no-point values (-10000), +0x154 / +0x16c / +0x170
   (-1), +0x340 (0x80000000) and the timer list (8 x 0x14, capacity 8);
   the highlight cleared with -1s; the two globals cleared. */
void aoe_game_view_construct_default_ror(AoeGameViewDefaultsRor *v,
                                         const AoeGameViewDefaultOpsRor *ops);


/* 0x005094d0 (game view, +0x?: x, y, a, b): 0 outside game modes 4..7;
   with a comm value (0x004199c0) the base handler (0x00519ed0). Else by
   the game's edit mode (+0x1b4), each first releasing the capture
   (0x00453db0) and giving 1 when the view's +0xec byte is not 1 or 2:
   100 / 0x69: the map point under (x, y) (0x51a650(0x28, 0, x, y, &pt,
   0, 1) == '3'), kept on the map (0x0051b070), given to the command
   0x004f1350 / 0x004f1580; success rewinds the kind-2 timers
   (0x0051b0f0(2)), marks the point (0x00516050 at +0x130 + x, +0x134 + y
   with 0, 0xf, 0, 2, 0x7d) and redraws (+0x20(1)); 0x65 / 0x66 / 0x67:
   the object command +0x104(x, y, 0, 0x68 / 0x69 / 7); 0x68: the object
   under the point (0x51aa10(x, y, 0, -1)) repaired (0x004f1450) flashes
   (0x0050f970(id, 1500, 2, 2)), the marks are freed (0x00515fb0) and the
   view redraws. A failure plays the refusal (0x0041c120(3)). Then the
   game's +0x10(0, 0); 1. Any other mode goes to the base handler. */
enum {
    AOE_VIEW_CLICK_RELEASE = 1, /* 0x00453db0 */
    AOE_VIEW_CLICK_REWIND,      /* 0x0051b0f0(2) */
    AOE_VIEW_CLICK_MARK,        /* 0x00516050(..., a, b, ...) */
    AOE_VIEW_CLICK_REDRAW,      /* +0x20(1) */
    AOE_VIEW_CLICK_REFUSE,      /* 0x0041c120(3) */
    AOE_VIEW_CLICK_FLASH,       /* 0x0050f970(a, 1500, 2, 2) */
    AOE_VIEW_CLICK_FREE_MARKS,  /* 0x00515fb0 */
    AOE_VIEW_CLICK_GAME_10      /* game +0x10(0, 0) */
};
typedef struct {
    void *context;
    int32_t game_state, edit_mode;  /* game +0x1b0, +0x1b4 */
    int32_t comm_value;             /* 0x004199c0 */
    uint8_t mode_ec;                /* view +0xec */
    int32_t origin_x, origin_y;     /* +0x130, +0x134 */
    int32_t (*base_click)(void *context, int32_t x, int32_t y, int32_t a, int32_t b);
    int (*point_at)(void *context, int32_t x, int32_t y, int32_t *px, int32_t *py); /* 0x51a650 == '3' */
    void (*clamp)(void *context, int32_t *px, int32_t *py);                        /* 0x0051b070 */
    int (*command_6d)(void *context, int32_t px, int32_t py);                      /* 0x004f1350 */
    int (*command_6f)(void *context, int32_t px, int32_t py);                      /* 0x004f1580 */
    int (*object_command)(void *context, int32_t x, int32_t y, int32_t code);      /* +0x104 */
    int32_t (*object_at)(void *context, int32_t x, int32_t y);                     /* 0x51aa10; -1 none */
    int (*repair)(void *context, int32_t id);                                      /* 0x004f1450 */
    void (*step)(void *context, int32_t id, int32_t a, int32_t b);
} AoeViewEditClickOpsRor;
int32_t aoe_view_edit_click_ror(const AoeViewEditClickOpsRor *o, int32_t x, int32_t y, int32_t a,
                                int32_t b);


/* 0x005156a0 (+0x?: x, y): the mouse moved over the view. Nothing (0)
   at the last point (+0x16c / +0x170). By the drag kind (+0x168): 2
   stores the point pulled into the panel (0x00453e60) and redraws (0).
   6: over the view (+0xc0) with a map point under the cursor (0x00511430
   == '3') the view goes there (0x0045d330 with +0.5); the point is
   stored. 7: a vertical step under 100 adds ftol(dy / 2) to the player's
   +0x122 and takes it from +0x120, a horizontal one adds ftol(dx / 2) to
   both; within 8 of the press point (+0xc4 / +0xc8) the point is stored
   and the view redrawn (1), else the cursor is warped back to the press
   point, which becomes the point; redraw, 1. 8: like 7 but scrolling the
   view by ftol(d / 2) (y: -s, +s; x: +s, +s). Other kinds: for 3 / 4 / 9
   only near the panel's edges (64 wide, 32 high) and moving outward,
   steps under 200 scroll the view by d / 16 (y: -, +; x: +, +); the point
   is stored; near: more than 8 from the press point warps the cursor
   back; not near: the press point follows. Then kind 3 highlights the
   tile from +0x144 / +0x148 to the one under the cursor (0x0050ffd0);
   kind 4 moves the current unit (+0x134) to the point (+0x34), its
   +0x18 asking 0x004159d0, else its +0x10 asking 0x004a6030, a 0 answer
   redrawing with 2, otherwise 1; kind 9 reports the drag rectangle
   (+0xf8: old point, new point, 2) and redraws. Finally an unchanged
   view position gives 0, else a redraw and 1. */
enum {
    AOE_VIEW_MOVE_PULL = 1,    /* 0x00453e60 on the point */
    AOE_VIEW_MOVE_HIGHLIGHT,   /* 0x0050ffd0(+0x144, +0x148, a, b) */
    AOE_VIEW_MOVE_UNIT,        /* current unit +0x34(point) then 0x004159d0 / 0x004a6030 -> result in arg */
    AOE_VIEW_MOVE_DRAG_RECT,   /* +0xf8(old, new, 2) */
    AOE_VIEW_MOVE_WARP,        /* ClientToScreen + SetCursorPos to the press point */
    AOE_VIEW_MOVE_REDRAW       /* +0x20(a) */
};
typedef struct {
    void *context;
    uint8_t kind;                    /* +0x168 */
    int32_t *last_x, *last_y;        /* +0x16c, +0x170 */
    int32_t *press_x, *press_y;      /* +0xc4, +0xc8 */
    int32_t left, top, right, bottom; /* +0x24..+0x30 */
    float *view_x, *view_y;          /* player +0x118, +0x11c */
    int16_t *player_120, *player_122;
    int has_current_unit;            /* player +0x134 */
    int unit_18, unit_10;            /* that unit's +0x18, +0x10 */
    int (*over_view)(void *context, int32_t x, int32_t y);  /* +0xc0 */
    int (*map_point)(void *context, int32_t x, int32_t y, float *px, float *py); /* 0x00511430 == '3' */
    void (*set_view)(void *context, float x, float y);      /* 0x0045d330 */
    int32_t (*step)(void *context, int32_t id, int32_t a, int32_t b, int32_t c, int32_t d);
} AoeViewMoveOpsRor;
int32_t aoe_view_mouse_move_ror(const AoeViewMoveOpsRor *o, int32_t x, int32_t y);


/* ---- 0x005145a0: the objects under a point, by hit class ---- */
/* The view's hit lists +0x2fc + class * 4 (0x40 entries of 0x14 bytes each,
   counts +0x310 + class * 4), classes 1..4 used. */
typedef struct AoeViewHitEntryRor {
    int32_t id;                              /* +0x00 */
    int16_t hit_class, depth;                /* +0x04, +0x06 */
    int16_t field_14, field_18;              /* +0x08, +0x0a */
    struct AoeViewHitEntryRor *next, *prev;  /* +0x0c, +0x10 */
} AoeViewHitEntryRor;
typedef struct {
    AoeViewHitEntryRor *lists[AOE_MAIN_VIEW_LISTS_ROR];
    int32_t counts[AOE_MAIN_VIEW_LISTS_ROR];
} AoeViewHitListsRor;
/* 0x005145a0(x, y, lo, hi, slack, max_class, positive_only): nothing (0)
   unless slack >= 0, lo <= hi, (x, y) is inside the panel rectangle
   (inclusive), lo <= 0x28 and hi >= 0; the rows are then clamped to
   0..0x28 and the point made panel-relative. The lists' counts are reset.
   Rows hi down to lo of `rows` (the view's +0x334): a node with key 0
   (and, with `positive_only`, a non-negative ID) whose box widened by
   `slack` holds the point is class 4; inside the box itself class 3,
   and by the pixel (0x005149d0, mirrored at right - x) class 2 for a
   skip run and 1 for a colour run. A class at most `max_class` is
   listed: an entry already listed for the ID in a better (lower) class,
   or the same class with a greater depth (+0x3a), wins; otherwise the
   old entry is unlinked and the new one (from the class's 0x40-entry
   store, nothing when full) is inserted by descending depth. The
   entries of classes 1..4 then fill `out` (ID, class, depth, +0x14,
   +0x18); returns their number (at most 0x40). */
int32_t aoe_main_view_point_objects_ror(const AoeMainViewRor *v, const AoeViewDrawRowsOpsRor *rows,
                                        AoeViewHitListsRor *lists, int32_t x, int32_t y, int32_t lo,
                                        int32_t hi, int32_t slack, int32_t max_class,
                                        int positive_only,
                                        AoeViewPickedRor out[AOE_VIEW_PICKED_MAX_ROR]);


/* 0x00516990: the grid's 16 x 16 lookup (+0x20, 256 bytes, -1 empty):
   for footprint tables 0..16 (0x0055f76c..; triples row, first column,
   last column ended by a row of 999), rows 0..15 with the columns
   clamped to 0..15 take the first table that covers them. The old
   lookup is freed. */
void aoe_view_grid_build_lookup_ror(AoeViewGridRor *grid, const int32_t *const tables[17]);


/* ---- 0x00518720 / 0x00518a30: the game view's edge and key scrolling ---- */
typedef struct {
    int32_t edge_active;   /* +0x368 */
    uint32_t edge_time;    /* +0x364 */
    int32_t key_active;    /* +0x378 */
    uint32_t key_time;     /* +0x374 */
} AoeGameViewScrollRor;
typedef struct {
    void *context;
    int32_t (*game_state)(void *context);     /* game +0x1b0 */
    int32_t (*edit_mode)(void *context);      /* game +0x1b4 */
    int (*comm_waiting)(void *context);       /* 0x004199c0 */
    /* The game's +0x0c object: +0x8c0 edge scrolling on, +0x8c4 the edge margin, +0x8d0 the
       speed period (ms), +0x8cc / +0x8d4 the edge / key speed caps, +0x8d8 the held object's step. */
    int edge_enabled;
    int32_t margin, period;
    float edge_max, key_max, held_step;
    int (*foreground)(void *context);         /* GetForegroundWindow() == the area's window (+0x78 +0x04) */
    int (*modal)(void *context);              /* 0x00451ad0 != 0 */
    int (*focus_elsewhere)(void *context);    /* 0x00451ac0 set and not this view */
    void (*window_rect)(void *context, int32_t r[4]);               /* GetWindowRect */
    void (*cursor)(void *context, int32_t *x, int32_t *y);          /* GetCursorPos */
    int (*buttons_down)(void *context);       /* GetAsyncKeyState(1) or (2) negative */
    void (*keys)(void *context, int *left, int *up, int *right, int *down); /* GetKeyboardState 0x25..0x28 bit 7 */
    uint32_t (*time_ms)(void *context);
    int has_player, has_area, has_held;       /* +0x100, +0x78, player +0x134 */
    void (*view_point)(void *context, float *x, float *y);           /* player +0x118 / +0x11c */
    void (*set_view_point)(void *context, float x, float y);         /* 0x0045d330 */
    void (*held_move)(void *context, int sx, int sy);                /* held object's vtable +0x34(x + sx step, y + sy step, 0) */
    void (*redraw)(void *context);            /* this view's vtable +0x20(1) and the +0x360 panel's */
} AoeGameViewScrollOpsRor;
/* The speed of a continued scroll: 1.0 on the first tick (the active
   flag set), else the elapsed time over the period, truncated plus 0.5
   when the fraction reached 0.5; capped at `max`. */
float aoe_game_view_scroll_speed_ror(int32_t *active, uint32_t now, uint32_t last, int32_t period,
                                     float max);
/* 0x00518720: in game states 4..7, not while the comm waits, with edge
   scrolling on, the window in front, no modal screen, the focus here
   (or nowhere) and no key scroll active: the cursor against the window
   edges (within the margin) picks a direction unless a mouse button is
   down. A direction moves the view: x -1 / +1 by (-s, -s) / (+s, +s),
   then y -1 / +1 by (+2s, -2s) / (-2s, +2s) from the moved point;
   redraws and stamps the time. Returns the active flag (cleared when
   nothing scrolls). */
int32_t aoe_game_view_edge_scroll_ror(AoeGameViewScrollRor *st, const AoeGameViewScrollOpsRor *o);
/* 0x00518a30: not while the comm waits, in states 4..7, the window in
   front and with an area. In edit mode 0x12 with a held object the
   arrows move it by the step (up (+, -), down (-, +), left (-, -),
   right (+, +)); the active flag is cleared. Otherwise, with a player,
   the arrows scroll as above with the key cap and time. Returns 1 when
   anything moved (then redrawing). */
int aoe_game_view_key_scroll_ror(AoeGameViewScrollRor *st, const AoeGameViewScrollOpsRor *o);

/* ---- 0x0051a650: the game view's pick ---- */
typedef struct {
    void *context;
    int32_t left, top;         /* +0x8c, +0x90 */
    int reveal_all;            /* map +0x8db8 */
    /* 0x00511430 for kinds other than ')' / '*'. */
    uint8_t (*base_pick)(void *context, uint8_t kind, uint8_t min_kind, int32_t x, int32_t y,
                         AoeMainViewPickRor *out, void *current);
    /* 0x005145a0(x, y, lo, hi, 0xf, 4, 1) into `out`; the count. */
    int32_t (*point_objects)(void *context, int32_t x, int32_t y, int32_t lo, int32_t hi,
                             AoeViewPickedRor out[AOE_VIEW_PICKED_MAX_ROR]);
    /* The world object of an ID (world +0x88 table; NULL none), its +0x48 state, template +0x8d
       kind and +0x38 / +0x3c position. */
    void *(*object)(void *context, int32_t id, uint8_t *state, uint8_t *kind, float *x, float *y);
    int (*visible)(void *context, void *object);              /* this view's vtable +0xe4 */
    uint8_t (*fog)(void *context, int32_t x, int32_t y);      /* 0x00517670 */
    int32_t (*priority)(void *context, void *object, int32_t hit_class); /* vtable +0xe8 */
} AoeGameViewPickOpsRor;
/* 0x0051a650(kind, min_kind, x, y, out, current, ignore_fog). ')': the
   objects under the point (rows 0..0x28, or 10..0x14 with a min kind)
   in state below 7 and of at least the min kind, each in a seen tile
   (0x0f) unless `ignore_fog`, the map is revealed or the view sees it;
   without `current` the highest +0xe8 priority wins ('4' with the
   object, its screen point from the record plus the panel origin, and
   its position); with `current`, among those of positive priority, the
   first one after it, else the first one before it, else itself (the
   original reads the record past the last there; this port uses the
   current's own). '*' tries ')' with the fog ignored and falls back to
   the base pick of '('. Other kinds go to the base pick. '2' when
   nothing was hit. */
uint8_t aoe_game_view_pick_ror(const AoeGameViewPickOpsRor *o, uint8_t kind, uint8_t min_kind,
                               int32_t x, int32_t y, AoeMainViewPickRor *out, void *current,
                               int ignore_fog);

/* ---- 0x0051aa10: the selection's target under a point ---- */
typedef struct {
    void *context;
    int32_t (*point_objects)(void *context, int32_t x, int32_t y, int32_t lo, int32_t hi,
                             AoeViewPickedRor out[AOE_VIEW_PICKED_MAX_ROR]); /* (10, 0x14, 0xf, 4, 1) */
    int32_t (*selection)(void *context);                      /* 0x0045e0c0: the count (0 none) */
    void *(*selected)(void *context, int32_t i);
    void *(*object)(void *context, int32_t id, uint8_t *state);
    /* The unit's template's ability (+0xd8 vtable +4(unit, object, 0, 0, 0)): the action record
       (NULL none) and its +6 kind; +8 on it: the flag. */
    void *(*ability)(void *context, void *unit, void *object, int16_t *kind);
    int (*ability_flag)(void *context, void *action);
    /* The unit's template +0xec; the owner's templates (+0x40 +0x24, count +0x22): one that
       matches the unit's (+0x14, +0x04, +0xec equal) and its ability for the pair. */
    int (*template_ec)(void *context, void *unit);
    int32_t (*template_count)(void *context);
    int (*template_matches)(void *context, int32_t index, void *unit);
    void *(*template_ability)(void *context, int32_t index, void *unit, void *object, int16_t *kind);
    int32_t (*priority)(void *context, void *object, int32_t hit_class); /* vtable +0xe8 */
} AoeGameViewTargetOpsRor;
/* 0x0051aa10(x, y, flag, arg): NULL (flag 0) without objects under the
   point or a selection. Every selected unit scores every object in
   state below 7: its ability for the object (of kind `arg`, or any
   when -1) adds the object's priority (the ability flag noted); failing
   that a unit with template +0xec set tries the owner's matching
   templates in order, the first with such an ability counting. The
   object with the highest positive total wins (its flag written). */
void *aoe_game_view_selection_target_ror(const AoeGameViewTargetOpsRor *o, int32_t x, int32_t y,
                                         int32_t *flag, int16_t arg);


/* ---- 0x00510580: the view's render (scroll copy and redraw) ---- */
typedef struct {
    int has_map;                  /* +0xfc */
    int16_t start[4];             /* +0x126, +0x128, +0x12a, +0x12c */
    int32_t origin_x, origin_y;   /* +0x130, +0x134 */
    int32_t width, height;        /* +0x14, +0x18 */
    uint8_t incremental;          /* +0x142 */
    int32_t highlight_on, highlight2_on; /* +0x2a0, +0x2a4 */
    int32_t rect1[4], rect2[4];   /* +0x2a8.., +0x2b8.. */
    int32_t buf_x, buf_y;         /* +0x2d0, +0x2d4: the terrain buffer's wrapped origin */
    int32_t buf_w, buf_h;         /* the terrain buffer's +0x18, +0x1c */
    int32_t blits_active;         /* +0x2d8 */
} AoeViewRenderRor;
enum { /* AoeViewRenderOpsRor.step ids */
    AOE_VR_BEGIN,          /* vtable +0x28(0) */
    AOE_VR_CLEAR,          /* 0x00438b50(+0x24 area, +0xef colour) (no map) */
    AOE_VR_END,            /* vtable +0x2c */
    AOE_VR_REGIONS_CLEAR,  /* 0x004a38c0 on +0x178, +0x17c, +0x180 */
    AOE_VR_MOVE_PIXELS,    /* 0x00439700(dst, src, dx, dy, clip left, top): windowed (+0x478 == 1) or no back buffer (+0xe8) */
    AOE_VR_QUEUE_BLITS,    /* 0x00510190(dst, src, dx, dy) */
    AOE_VR_MOUSE,          /* 0x0044a850 (with the global 0x00582edc) */
    AOE_VR_STEP_BLITS,     /* 0x005104a0(0) (while +0x2d8) */
    AOE_VR_SCROLL_ROWS,    /* 0x00516660(dx, dy) */
    AOE_VR_WRAP_FORWARD,   /* "view::draw" lock: the first (new buf_x) bytes of row 0 copied past the last row, unlock */
    AOE_VR_WRAP_BACK,      /* "view::draw2" lock: (new buf_x) bytes of row 0 copied past the last row, unlock "view::draw3" */
    AOE_VR_SET_ORIGIN,     /* 0x00438f40(buf_x, buf_y) */
    AOE_VR_DIRTY,          /* 0x00438d20(rect a: 1 rect1, 2 rect2, 0 the buffer's +0x28 rectangle, 0) */
    AOE_VR_BRUSH,          /* 0x00511310 (edit modes 9, 10, 0x13) */
    AOE_VR_DRAW,           /* 0x00510ef0(10, +0x324) */
    AOE_VR_COUNT           /* 0x007d2010 += 1 */
};
typedef struct {
    void *context;
    void (*step)(void *context, int id, int32_t a, int32_t b);
    /* 0x00510c70: the start fields and origin recomputed into `v`. */
    void (*layout)(void *context, AoeViewRenderRor *v);
    /* The copy rectangles (left, top, right, bottom) handed to the move / queue step. */
    void (*copy_rects)(void *context, const int32_t dst[4], const int32_t src[4]);
    int direct_move;              /* the area's owner +0x478 == 1 or its +0xe8 == 0 */
    int (*mouse_global)(void *context);
    int32_t (*edit_mode)(void *context);
} AoeViewRenderOpsRor;
/* 0x00510580: without a map: begin, clear, end. Otherwise the regions
   are emptied, the highlights cleared and the layout recomputed; in the
   incremental mode with changed start fields the origin's move (dx, dy,
   16-bit) copies the overlap: a move past the view falls back to a full
   draw (+0x142 cleared); each delta is rounded towards zero to a
   multiple of 8 (the origin adjusted); the overlap (dst, src) is moved
   directly or queued as blits, the draw rows scrolled, the buffer origin
   advanced and wrapped (copying the wrapped row part under a lock), set
   (0x00438f40), and the exposed strips dirtied: the new columns (rect
   1) and the new rows (rect 2, or rect 1 when no columns were exposed).
   Then the mouse, a blit step, the brush highlight in edit modes 9, 10,
   0x13; a non-incremental view resets the buffer origin and dirties the
   whole buffer; the draw (code 10), +0x142 set, end, the frame count. */
void aoe_main_view_render_ror(AoeViewRenderRor *v, const AoeViewRenderOpsRor *o);

/* ---- 0x0050cef0: the networked game's end report ---- */
#define AOE_MP_END_REPORT_SIZE 0x21c
typedef struct {
    void *context;
    int32_t *sent;                /* world +0x120 */
    int (*launched)(void *context);                /* 0x004241e0 */
    int16_t player_count;         /* world +0x3c */
    int (*player_active)(void *context, int32_t player);   /* 0x00454150 != 0 */
    const char *(*scenario_title)(void *context);  /* 0x00475b30 */
    uint32_t game_time_ms;        /* world +0x04 */
    void (*leader)(void *context, int32_t category, AoeLeaderRor *out); /* 0x0050cdf0 */
    int32_t (*slot_player)(void *context, int32_t slot);   /* 0x0041c080 */
    const char *(*slot_name)(void *context, int32_t slot); /* 0x004241a0 on the comm's +0x14 */
    int32_t (*points)(void *context, int32_t player);      /* 0x00454170 on the player's +0x34 */
    uint8_t (*player_80)(void *context, int32_t player);
    uint8_t (*player_88)(void *context, int32_t player);
    uint8_t (*slot_a94)(void *context, int32_t i);         /* 0x005053b0 */
    uint8_t (*slot_ac4)(void *context, int32_t i);         /* 0x00505410 */
    uint8_t (*slot_994)(void *context, int32_t i);         /* 0x0041b9b0 */
    uint8_t (*game_a58)(void *context, int32_t slot);      /* game +0xa58 + 4 (slot - 1) */
    int (*lobby_slot)(void *context, int32_t slot);        /* comm +0x14's +0x1750 + 4 (slot - 1) */
    uint8_t (*game_a15)(void *context, int32_t slot);      /* game +0xa15 + slot */
    /* The player's +0x840 list: the value of the node of kind 0 / 1 / 2 (-1 none). */
    int32_t (*player_value)(void *context, int32_t player, int kind);
    uint8_t cheats;               /* 0x0041b7a0 */
    uint8_t option_985;           /* 0x0041b7c0 */
    uint8_t map_size_code;        /* world +0x1e */
    const uint8_t *settings;      /* the 0x6c-byte settings snapshot (0x00505170, packed) */
    void (*send)(void *context, const uint8_t *record, int32_t size); /* 0x00425930 */
} AoeMpEndReportOpsRor;
/* 0x0050cef0: once (world +0x120), when launched: the 0x21c-byte record
   (little-endian): +0 slots with players, +1 active players of 1..n-1,
   +2 the scenario title (34), +0x24 the game time in seconds, +0x28 /
   +0x40 / +0x58 / +0x70 / +0x88 the leaders of categories 0xff, 2, 1,
   0, 3 (score, colour, name 16, allies, 2 pad), +0xa0 eight 0x2c-byte
   slot entries (name 16, points, +0x80 == 1, 0x005053b0, game +0xa58,
   0x00505410, 0x0041b9b0, status 3 lobby / 2 dropped / 0 (+0x80 == 1)
   / 1, -1 pad, the three list values, +0x88), +0x200 the settings' map
   size, map type, population (16-bit), victory, starting age, resource
   level, +0x5d, +0x5c, 0x0041b7c0, +0x69, 0x0041b7a0, the map size code
   (+0x1e: 0, 1, 6, 2..5 for 100..0x67, 7 for 0x68, else 8) and three
   zero dwords; sent with 0x00425930. Unwritten bytes are zero here
   (the original sends its stack). Returns 1 when sent. */
int aoe_world_mp_end_report_ror(const AoeMpEndReportOpsRor *o, uint8_t out[AOE_MP_END_REPORT_SIZE]);

#endif
