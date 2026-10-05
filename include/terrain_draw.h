#ifndef VITA_AOE_TERRAIN_DRAW_H
#define VITA_AOE_TERRAIN_DRAW_H

#include "dat.h"
#include "scn.h"
#include "terrain.h"
#include "terrain_art.h"
#include "tileedge.h"

#include <stdint.h>

enum { TERRAIN_DRAW_TERRAIN = 1, TERRAIN_DRAW_BORDER = 2 };

typedef struct {
    uint8_t kind;
    uint8_t slot;
    uint32_t frame;
} TerrainDrawLayer;

typedef struct {
    uint8_t draw_terrain_id;
    uint8_t shape;
    uint8_t border_code;
    uint8_t layer_count;
    TerrainDrawLayer layers[5]; /* Base plus at most four style-1 borders. */
} TerrainDrawPlan;

enum {
    TERRAIN_SPRITE_CLIP_DEFAULT = 0,
    TERRAIN_SPRITE_CLIP_FIRST = 1,
    TERRAIN_SPRITE_CLIP_SECOND = 2
};

typedef struct {
    uint8_t clip;
    uint8_t dither_mode; /* Original DAT_007c54c0 value 0 or 1. */
} TerrainSpritePass;

typedef struct {
    uint8_t count;
    TerrainSpritePass passes[2];
    uint32_t even_mask, even_fill, odd_mask, odd_fill;
} TerrainSpritePassPlan;

typedef struct {
    const uint8_t *bytes;
    size_t size;
    size_t stride;
} TerrainClipMask;

/* Empiresx.exe disc 0x00516b40 (patched 0x00514320) chooses palette masks and draw order after the
   terrain frame is selected. Tile flag 0x10 overrides clipping with one
   special pass; otherwise the second clip plane draws before the first.
   This records the original draw calls; the blit functions below apply
   their 8-bit palette masks. */
int terrain_build_sprite_pass_plan(uint8_t tile_flags, int first_active,
                                    int second_active,
                                    TerrainSpritePassPlan *plan);

/* Original tile-draw ordering from Empiresx.exe 0x00516710 and
   Empires.exe 0x004ba930, after their callers' one-step terrain replacement.
   Inputs are the raw tile terrain ID, shape byte, border byte, and tile X/Y.
   An empty plan is valid when the original direct art is unavailable.
   Fog, palette effects, loose/SHP resources, object layers, and pixel
   placement are outside this function. */
int terrain_build_draw_plan(const DatTerrains *terrains,
                            const DatBorders *borders,
                            const TerrainArtCache *terrain_art,
                            const BorderArtCache *border_art,
                            uint8_t raw_terrain_id, uint8_t shape,
                            uint8_t border_code, uint32_t x, uint32_t y,
                            TerrainDrawPlan *plan);

/* Draws an already selected terrain/border plan at a screen-space sprite
   anchor. Empiresx.exe 0x004a1460 subtracts each frame's SLP hotspot before
   drawing; this routine does likewise. It handles only original
   palette-color and transparent SLP pixels, not fog or palette transforms.
   All layers are validated before the destination is changed. */
int terrain_blit_draw_plan_rgba(const TerrainDrawPlan *plan,
                                const TerrainArtCache *terrain_art,
                                const BorderArtCache *border_art,
                                const Palette *palette, uint8_t *rgba,
                                size_t rgba_size, uint32_t width,
                                uint32_t height, size_t stride,
                                int32_t anchor_x, int32_t anchor_y);

/* Draws each original terrain/border layer through the ordered sprite passes.
   Clip masks are byte planes in destination coordinates; nonzero means
   drawable. screen_origin is the absolute position of destination pixel
   (0,0), preserving the original dither phase when drawing a sub-viewport.
   Dither mode applies the original 8-bit software blitter's per-row/per-byte
   palette-index masks before JASC-to-RGBA conversion.
   All frames and selected clip masks are validated before any pixel write.
   This does not implement loose/SHP art, objects, or a live viewport. */
int terrain_blit_draw_plan_passes_rgba(const TerrainDrawPlan *plan,
                                       const TerrainArtCache *terrain_art,
                                       const BorderArtCache *border_art,
                                       const Palette *palette,
                                       const TerrainSpritePassPlan *passes,
                                       TerrainClipMask first,
                                       TerrainClipMask second,
                                       uint8_t *rgba, size_t rgba_size,
                                       uint32_t width, uint32_t height,
                                       size_t stride, int32_t anchor_x,
                                       int32_t anchor_y,
                                       int32_t screen_origin_x,
                                       int32_t screen_origin_y);

/* Same original ordered passes and 8-bit dither operation, retaining palette
   indices for later player-color/shadow sprite composition. Convert the
   finished surface to RGBA only at the display boundary. */
int terrain_blit_draw_plan_passes_indexed(const TerrainDrawPlan *plan,
                                          const TerrainArtCache *terrain_art,
                                          const BorderArtCache *border_art,
                                          const TerrainSpritePassPlan *passes,
                                          TerrainClipMask first,
                                          TerrainClipMask second,
                                          uint8_t *indices,
                                          size_t indices_size,
                                          uint32_t width, uint32_t height,
                                          size_t stride,
                                          int32_t anchor_x, int32_t anchor_y,
                                          int32_t screen_origin_x,
                                          int32_t screen_origin_y);

typedef struct {
    uint8_t *rgba;
    size_t rgba_size;
    uint32_t width, height;
    size_t stride;
    uint32_t clip_left, clip_top, clip_width, clip_height;
} TerrainViewSurface;

typedef struct {
    uint8_t *indices;
    size_t indices_size;
    uint32_t width, height;
    size_t stride;
    uint32_t clip_left, clip_top, clip_width, clip_height;
} TerrainViewIndexedSurface;

typedef struct {
    const uint32_t *tile_masks; /* Runtime row-major visibility grid. */
    size_t tile_masks_count;
    const uint8_t *tile_flags;  /* Runtime tile byte +10, row-major. */
    size_t tile_flags_count;
    uint32_t visible_mask, explored_mask;
    int fog_enabled, bypass_exploration_gate;
    const TileEdgeIndex *tile_edges;
    const TileEdgeIndex *black_edges; /* Optional in Rise of Rome. */
    TileEdgeCoveragePlane first, second; /* Caller-owned frame clip planes. */
    /* Nonzero walks the same tiles and calls the tile callback for each
       non-hidden projected tile without drawing terrain (an objects-only
       pass over a terrain layer drawn separately). */
    int skip_terrain;
} TerrainViewVisibility;

/* Object/overlay seam in the original visibility-aware tile traversal.
   The callback runs after the terrain passes for each non-hidden projected
   tile, using the same tile order and the absolute screen-space terrain
   anchor. It also runs when that tile has no direct terrain art, since an
   absent terrain layer does not suppress objects owned by the tile. */
typedef int (*TerrainViewTileDraw)(void *context, uint32_t tile_x,
                                   uint32_t tile_y, int32_t anchor_x,
                                   int32_t anchor_y,
                                   uint8_t visibility_mode);

/* Draws only the original direct terrain/border layer in viewport tile order
   and clips pixels to the supplied viewport rectangle. The camera state must
   come from terrain_compute_view_camera for the same rectangle. Missing
   non-direct art is left blank. All tiles are assumed visible; runtime
   visibility/fog, objects, overlays, and UI are not drawn.
   Failure may leave earlier tiles written to the destination. */
int terrain_draw_view_terrain_rgba(const ScnTile *tiles, uint32_t map_width,
                                   uint32_t map_height,
                                   const DatMapGeometry *geometry,
                                   const DatTerrains *terrains,
                                   const DatBorders *borders,
                                   const TerrainArtCache *terrain_art,
                                   const BorderArtCache *border_art,
                                   const Palette *palette,
                                   const TerrainViewCamera *camera,
                                   const TerrainViewSurface *surface,
                                   size_t *drawn_tiles);

/* Original 0x00514c10 terrain-layer path using caller-supplied runtime
   visibility and tile flags. Clears both clip planes at frame start, walks
   the original viewport order, applies TILEEDGE/optional BLKEDGE spans,
   then draws direct terrain/borders through the original ordered passes.
   Caller initializes the RGBA destination (hidden tiles leave it unchanged)
   and supplies actual camera, visibility, and original assets; objects,
   loose/SHP art, UI, dynamic updates, and exact screen comparison remain
   outside this function. Failure can leave earlier pixels written. */
int terrain_draw_view_visibility_rgba(const ScnTile *tiles,
                                      uint32_t map_width,
                                      uint32_t map_height,
                                      const DatMapGeometry *geometry,
                                      const DatTerrains *terrains,
                                      const DatBorders *borders,
                                      const TerrainArtCache *terrain_art,
                                      const BorderArtCache *border_art,
                                      const Palette *palette,
                                      const TerrainViewCamera *camera,
                                      const TerrainViewSurface *surface,
                                      const TerrainViewVisibility *visibility,
                                      size_t *drawn_tiles);

/* The same original visibility-aware tile walk and clip planes, but writes
   original 8-bit palette indices so later object shadows can remap the
   actual terrain destination. Caller owns/initializes the surface and
   visibility arrays; hidden pixels remain unchanged. */
int terrain_draw_view_visibility_indexed(
    const ScnTile *tiles, uint32_t map_width, uint32_t map_height,
    const DatMapGeometry *geometry, const DatTerrains *terrains,
    const DatBorders *borders, const TerrainArtCache *terrain_art,
    const BorderArtCache *border_art, const TerrainViewCamera *camera,
    const TerrainViewIndexedSurface *surface,
    const TerrainViewVisibility *visibility, size_t *drawn_tiles);

/* Indexed terrain draw with an authentic per-tile composition seam for
   object sprites. A callback failure aborts the walk. object_tiles counts
   successful callback invocations; drawn_tiles retains the existing meaning
   of tiles whose direct terrain/border art was actually blitted. */
int terrain_draw_view_visibility_indexed_with_tiles(
    const ScnTile *tiles, uint32_t map_width, uint32_t map_height,
    const DatMapGeometry *geometry, const DatTerrains *terrains,
    const DatBorders *borders, const TerrainArtCache *terrain_art,
    const BorderArtCache *border_art, const TerrainViewCamera *camera,
    const TerrainViewIndexedSurface *surface,
    const TerrainViewVisibility *visibility, TerrainViewTileDraw draw_tile,
    void *draw_tile_context, size_t *drawn_tiles, size_t *object_tiles);

#endif
