#ifndef VITA_AOE_SHAPE_H
#define VITA_AOE_SHAPE_H

#include <stddef.h>
#include <stdint.h>

/* Patched Rise of Rome 1.0a shape object (0x0049f5f0..0x004a0370): a
   graphic that is either an SLP ("Fshape": header, then 32-byte frame
   records from +0x20) or an older SHP ("1.10": count, then per frame an
   offset and a palette offset; each frame a header of
   width:height, min x, min y, max x, max y and then rows of runs). Data
   comes from a loose file or from a resource. All reads are little
   endian; the port also checks every offset against the data size (the
   original trusts the files). */

typedef struct {
    const uint8_t *shp;        /* +0x00 */
    int32_t owned;             /* +0x04: 1 heap data, 0 a resource */
    uint32_t size;             /* +0x08 */
    const uint8_t *frame;      /* +0x0c: the SHP frame last looked up */
    const uint8_t *shp_header; /* +0x10 */
    const uint8_t *shp_table;  /* +0x14 */
    const uint8_t *slp;        /* +0x18 */
    const uint8_t *slp_frames; /* +0x1c */
} AoeShapeRor;

typedef struct {
    void *context;
    int loose_files;           /* 0x007c0464 */
    int32_t *load_count;       /* 0x007c0450 */
    /* A whole file read into heap memory (free()), or NULL. */
    uint8_t *(*load_file)(void *context, const char *path, uint32_t *size);
    /* Resource `type` ('slp ' / 'shp ') `id`: its data and size, and
       whether it was copied to the heap. NULL when absent. */
    uint8_t *(*load_resource)(void *context, uint32_t type, int32_t id,
                              uint32_t *size, int32_t *owned);
    /* Release mapped (not owned) resource data. */
    void (*release)(void *context, const uint8_t *data, uint32_t size);
} AoeShapeOpsRor;

/* 0x0049f5f0: load `name` (its last three characters replaced by "SLP",
   then "SHP") when loose files are on or there is no resource ID; then
   resource 'slp ' / 'shp ' `id`. An SHP resource whose version is not
   "1.10" is kept but has no tables. 0x0049f840: free. */
void aoe_shape_load_ror(AoeShapeRor *s, const char *name, int32_t id,
                        const AoeShapeOpsRor *ops);
void aoe_shape_free_ror(AoeShapeRor *s, const AoeShapeOpsRor *ops);
/* 0x0049f8b0: 1 when frame `frame` cannot be used (nothing loaded, a bad
   SHP, a frame past the count, a wrong SHP version); a negative frame
   checks only the shape. The original's message is built and dropped.
   0x0049f9e0: anything loaded. 0x0049fa90: the frame count (0 on
   error). */
int aoe_shape_bad_ror(AoeShapeRor *s, int32_t frame);
int aoe_shape_loaded_ror(const AoeShapeRor *s);
int32_t aoe_shape_count_ror(AoeShapeRor *s);
/* 0x0049fa00: a frame's width and height (SLP: the frame record's
   low 16 bits; SHP: the header's high and low halves). */
int aoe_shape_size_ror(AoeShapeRor *s, int32_t frame, int16_t *w, int16_t *h);
/* 0x0049fac0: a frame's box relative to its hotspot. */
int aoe_shape_bounds_ror(AoeShapeRor *s, int32_t frame, int32_t *min_x,
                         int32_t *min_y, int32_t *max_x, int32_t *max_y);
/* 0x0049fb90: is (x, y), relative to the hotspot, a drawn pixel? SLP
   commands follow the original's decoding, including its (byte << 4) |
   next long-run length. */
int aoe_shape_hit_ror(AoeShapeRor *s, int32_t x, int32_t y, int32_t frame);

/* An 8-bit drawing surface (the original's +0x08 bits, +0x18 / +0x1c
   size, +0x20 pitch, +0x24 row direction, +0x28..+0x34 clip). */
typedef struct {
    uint8_t *bits;
    int32_t width, height;
    int32_t pitch;
    int32_t direction;         /* 1 top-down, else bottom-up */
    int32_t clip_left, clip_top, clip_right, clip_bottom;
} AoeShapeSurfaceRor;

/* The SHP blitters for a frame entirely inside the surface at (x, y):
   0x004a0190 copy, 0x004a0270 through a colour table, 0x004a0370 remap
   the pixels under the frame through a table (a shadow). They use the
   frame last looked up (frame +0x0c). */
int aoe_shape_blit_ror(const AoeShapeRor *s, AoeShapeSurfaceRor *dst,
                       int32_t x, int32_t y);
int aoe_shape_blit_xlat_ror(const AoeShapeRor *s, AoeShapeSurfaceRor *dst,
                            int32_t x, int32_t y, const uint8_t table[256]);
int aoe_shape_blit_shadow_ror(const AoeShapeRor *s, AoeShapeSurfaceRor *dst,
                              int32_t x, int32_t y,
                              const uint8_t table[256]);

/* The clipped SHP blitters, writing only columns cl..cr of rows ct..cb
   for a frame placed with its top-left at (x, y): 0x004a0480 copy,
   0x004a0790 through a colour table, 0x004a0b20 shadow remap. */
int aoe_shape_blit_clipped_ror(const AoeShapeRor *s, AoeShapeSurfaceRor *dst,
                               int32_t x, int32_t y, int32_t cl, int32_t ct,
                               int32_t cr, int32_t cb);
int aoe_shape_blit_clipped_xlat_ror(const AoeShapeRor *s,
                                    AoeShapeSurfaceRor *dst, int32_t x,
                                    int32_t y, int32_t cl, int32_t ct,
                                    int32_t cr, int32_t cb,
                                    const uint8_t table[256]);
int aoe_shape_blit_clipped_shadow_ror(const AoeShapeRor *s,
                                      AoeShapeSurfaceRor *dst, int32_t x,
                                      int32_t y, int32_t cl, int32_t ct,
                                      int32_t cr, int32_t cb,
                                      const uint8_t table[256]);

typedef struct {
    void *context;
    /* SLP frames go to the sprite rasterizer: batched (0x007c045c set)
       or drawn into the surface at the hotspot-adjusted point with the
       colour table (the drawn count 0x007c044c rises). */
    int32_t *batching;
    int32_t *drawn;
    void (*draw_slp)(void *context, AoeShapeSurfaceRor *dst,
                     const AoeShapeRor *s, int32_t frame, int32_t x,
                     int32_t y, const uint8_t *table, int batched,
                     int mirrored);
} AoeShapeDrawOpsRor;

/* 0x0049fe00: draw frame `frame` with its hotspot at (x, y). Modes: 0
   copy, 1 through `table`, 2 shadow through `table` (no table: copy);
   others draw nothing. SHP frames are clipped to the surface's clip and
   size, using the unclipped blitter when nothing is cut. */
int aoe_shape_draw_ror(AoeShapeRor *s, AoeShapeSurfaceRor *dst, int32_t x,
                       int32_t y, int32_t frame, uint8_t mode,
                       const uint8_t *table, const AoeShapeDrawOpsRor *ops);

/* Mirrored drawing 0x004a0e90: the box is flipped about x (x - max x ..
   x - min x) and rows run leftwards from their right end; SLP frames
   pass the mirrored point to the rasterizer. Its blitters: 0x004a1230
   copy, 0x004a1300 colour table, 0x004a13f0 shadow (mode 1 / 2 with a
   table), and the clipped 0x004a14f0, 0x004a17f0, 0x004a1b40. */
int aoe_shape_draw_mirrored_ror(AoeShapeRor *s, AoeShapeSurfaceRor *dst,
                                int32_t x, int32_t y, int32_t frame,
                                uint8_t mode, const uint8_t *table,
                                const AoeShapeDrawOpsRor *ops);
int aoe_shape_blit_mirrored_ror(const AoeShapeRor *s, AoeShapeSurfaceRor *dst,
                                int32_t x, int32_t y, const uint8_t *table,
                                int32_t mode);
int aoe_shape_blit_clipped_mirrored_ror(const AoeShapeRor *s,
                                        AoeShapeSurfaceRor *dst, int32_t x,
                                        int32_t y, int32_t cl, int32_t ct,
                                        int32_t cr, int32_t cb,
                                        const uint8_t *table, int32_t mode);

#endif
