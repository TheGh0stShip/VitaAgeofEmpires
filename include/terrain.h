#ifndef VITA_AOE_TERRAIN_H
#define VITA_AOE_TERRAIN_H

#include "dat.h"
#include "scn.h"
#include <stddef.h>

/* Empires.exe 0x00428970 / Empiresx.exe 0x004493a0 elevation shape byte.
   The caller still needs the original DAT terrain/frame and border rules
   before selecting artwork. Out-of-map neighbors use the current elevation. */
uint8_t terrain_elevation_shape(const ScnTile *tiles, uint32_t width,
                                uint32_t height, uint32_t x, uint32_t y);

/* RoR live-unit placement vtable +0x34 (0x004ab160), base counterpart:
   the 17-case fractional surface-height adjustment for the tile's elevation
   shape. `fraction_x/y` are the position within one tile, in [0,1).
   The output excludes the tile's integer elevation. */
int terrain_shape_surface_offset(uint8_t shape, float fraction_x,
                                 float fraction_y, float *offset);

/* Samples that same surface using scenario tile elevations and the original
   neighbor-derived shape. Coordinates are clamped to the map (upper edge is
   width/height minus 0.001, as in the original). This computes ground height
   only; the unit template's elevation policy and map-link updates are separate
   parts of the original placement method. */
int terrain_surface_height_at(const ScnTile *tiles, uint32_t width,
                              uint32_t height, float x, float y,
                              float *surface_height);

/* Final Z decision in RoR 0x004ab160 / base 0x00415bf0. Template +0x68
   mode 0 snaps to ground; mode 1 only raises Z to ground; other modes retain
   the requested Z. This excludes the remaining tile-link bookkeeping. */
int terrain_unit_surface_z(const DatUnit *unit_template, float requested_z,
                           float surface_height, float *unit_z);

/* Terrain-only minimap palette index from Empires.exe 0x00456d03 and
   Empiresx.exe 0x0042d610. Other minimap layers are not represented here.
   Invalid shape or terrain returns palette index zero. */
uint8_t terrain_minimap_palette_index(const DatTerrain *terrain, uint8_t shape);

/* Reads the original DAT span for a tile's elevation shape. This identifies
   possible SLP frames, not the original choice among multiple flat frames.
   The caller must check the span against the loaded SLP's frame count.
   Returns false for absent/invalid DAT spans or shapes outside 0..16. */
int terrain_shape_frame_span(const DatTerrain *terrain, uint8_t shape,
                             uint16_t *first_frame, uint16_t *frame_count);

/* Empiresx.exe disc 0x00517790 (patched 0x00514ee0) / Empires.exe 0x004bbbd0. Selects the SLP frame
   from an elevation shape and nonnegative tile coordinates. For multi-frame
   shapes, the DAT frame_grid low bytes tile variants by row/column; an
   out-of-range variant falls back to the first frame. This only selects a
   frame number: the caller must still resolve the terrain's actual art. */
int terrain_select_frame(const DatTerrain *terrain, uint8_t shape,
                         uint32_t x, uint32_t y, uint32_t *frame_index);

/* Empiresx.exe 0x00517880 / Empires.exe 0x004bbcc0. Selects a border SLP
   frame for a nonzero map border pattern. The original lookup flattens the
   DAT's 19x12 frame table, so patterns 13..15 cross into the next row.
   A zero pattern is rejected here because it points into non-frame header
   data. This does not decide whether/when a border is drawn. */
int terrain_select_border_frame(const DatBorder *border, uint8_t shape,
                                uint8_t pattern, uint32_t x, uint32_t y,
                                uint32_t *frame_index);

/* Original region-paint admission gate: Empires.exe 0x00428700 and
   Empiresx.exe 0x00449110. A disabled terrain may be painted only when
   its immediate replacement exists and is enabled. */
int terrain_paint_target_available(const DatTerrains *terrains, uint8_t id);

/* Empiresx.exe 0x00514c10 / Empires.exe 0x004b8e80 tile-draw caller:
   substitute the raw map terrain's immediate DAT replacement, if present.
   This is a single step; it does not resolve a chain. The original draw
   routine separately checks whether the chosen runtime art is available. */
int terrain_resolve_draw_id(const DatTerrains *terrains, uint8_t raw_id,
                            uint8_t *draw_id);

/* Empires.exe 0x00428ce0 / Empiresx.exe 0x004496b0 border byte:
   low nibble is DAT border slot (0 means none),
   high nibble is one of its neighbor patterns. Invalid input returns 0. */
uint8_t terrain_border_code(const ScnTile *tiles, uint32_t width,
                            uint32_t height, uint32_t x, uint32_t y,
                            const DatTerrains *terrains,
                            const DatBorders *borders);

/* Empires.exe 0x00424a60 / Empiresx.exe 0x00445880 projected tile origin,
   before camera offset. */
int terrain_project_tile(const ScnTile *tiles, uint32_t width, uint32_t height,
                         uint32_t x, uint32_t y, const DatMapGeometry *geometry,
                         int32_t *screen_x, int32_t *screen_y);

/* Empiresx.exe 0x00514c10 / Empires.exe 0x004b8e80 take the projected
   tile-origin shorts from the runtime map and subtract viewport camera
   pixel offsets at renderer fields +0x130/+0x134 before sprite drawing.
   This reconstructs the visible anchor from scenario tiles and DAT geometry.
   The camera arguments are pixel offsets, not scenario camera tile floats. */
int terrain_project_tile_view(const ScnTile *tiles, uint32_t width,
                              uint32_t height, uint32_t x, uint32_t y,
                              const DatMapGeometry *geometry,
                              int32_t camera_x, int32_t camera_y,
                              int32_t *screen_x, int32_t *screen_y);

/* Empiresx.exe 0x00514c10 / Empires.exe 0x004b8e80 visit map cells in
   viewport draw order. The caller supplies the renderer's signed starting
   tile coordinates and its horizontal/diagonal sweep counts (the originals
   derive these from fields +0x12a, +0x12c, +0x116, and +0x118).
   Out-of-map cells are skipped. This is order/bounds only: pixel clipping,
   visibility, fog, and art decisions remain the caller's responsibility. */
typedef void (*TerrainTileVisitor)(uint32_t x, uint32_t y, void *context);
int terrain_walk_view_tiles(uint32_t width, uint32_t height,
                            int32_t start_x, int32_t start_y,
                            uint32_t diagonal_count, uint32_t diagonal_length,
                            TerrainTileVisitor visit, void *context,
                            size_t *visited_count);

typedef struct {
    int32_t camera_pixel_x, camera_pixel_y;
    int32_t start_x, start_y;
    uint32_t diagonal_count, diagonal_length;
    int32_t view_left, view_top, view_width, view_height;
} TerrainViewCamera;

/* Empiresx.exe 0x0045e290 clamps requested camera tile floats to the map's
   inclusive [0,width-1] and [0,height-1] bounds before the viewport reads
   them. NaNs follow its negative/unordered branch to zero. */
int terrain_clamp_camera_tiles(uint32_t width, uint32_t height,
                               float requested_x, float requested_y,
                               float *camera_x, float *camera_y);

enum {
    TERRAIN_VIEW_HIDDEN = 0x00,
    TERRAIN_VIEW_VISIBLE = 0x0f,
    TERRAIN_VIEW_EXPLORED_FOG = 0x80
};

/* Empiresx.exe 0x00514c10 / 0x0051a0d0 (base 0x004b8e80 /
   0x00409f30) classify one runtime visibility-mask tile. The viewport uses
   0x0f for visible, 0x80 for explored under fog, and zero for unexplored.
   Bypassing the exploration gate admits otherwise-unexplored tiles, but
   still returns fog mode when fog is enabled. This only returns the mode;
   rendering fog/black and objects is separate. */
uint8_t terrain_view_visibility_code(uint32_t tile_mask,
                                      uint32_t visible_mask,
                                      uint32_t explored_mask,
                                      int bypass_exploration_gate,
                                      int fog_enabled);

/* Empiresx.exe disc 0x00511d70 (patched 0x0050f5c0) / Empires.exe 0x004b5e50 generate a 256-entry
   lookup that canonicalizes the eight-neighbor visibility edge mask to one
   of 47 edge patterns. The renderer stores it at +0x1a0 (base +0x19c). */
int terrain_build_visibility_edge_table(uint8_t table[256]);

/* Empiresx.exe disc 0x00516590 (patched 0x00513d30) / Empires.exe 0x004ba7a0: inspect the eight
   in-map neighbors of (x,y), set bits where owner_mask is absent, and map
   the resulting mask through the generated edge table. tile_masks is a
   row-major runtime visibility grid; out-of-map neighbors contribute zero. */
int terrain_visibility_edge_pattern(const uint32_t *tile_masks,
                                     uint32_t width, uint32_t height,
                                     uint32_t x, uint32_t y,
                                     uint32_t owner_mask,
                                     const uint8_t table[256],
                                     uint8_t *pattern);

/* Recomputes the viewport camera and sweep fields from original camera tile
   floats and viewport rectangle. Expansion 0x005125a0 (base 0x004b6670)
   derives tile counts by dividing viewport width/height by DAT tile width/
   height; expansion disc 0x00513440 (patched 0x00510c70; base 0x004b76b0) uses those counts. The
   caller must supply actual runtime camera floats; scenario defaults alone
   are not a live game camera. Returns false for out-of-map or numerically
   unsafe inputs. */
int terrain_compute_view_camera(const ScnTile *tiles, uint32_t width,
                                uint32_t height, const DatMapGeometry *geometry,
                                float camera_tile_x, float camera_tile_y,
                                int32_t view_left, int32_t view_top,
                                int32_t view_width, int32_t view_height,
                                TerrainViewCamera *camera);

#endif
