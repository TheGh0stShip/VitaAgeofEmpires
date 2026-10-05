#ifndef VITA_AOE_SPAN_REGION_H
#define VITA_AOE_SPAN_REGION_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a node pool (0x004a34b0..0x004a3700 with
   0x00516280): fixed-size nodes carved from blocks, recycled through a
   free list linked through each node's first pointer. Semantic storage;
   offsets are noted. */
typedef struct {
    uint8_t **blocks;          /* +0x00 */
    int32_t *block_counts;     /* +0x04 */
    int32_t block_count;       /* +0x08 */
    int32_t capacity;          /* +0x0c */
    void *free_list;           /* +0x10 */
    int32_t cur_block;         /* +0x14, -1 */
    int32_t cur_index;         /* +0x18, -1 */
    int32_t total;             /* +0x1c */
    int32_t available;         /* +0x20 */
    int32_t per_block;         /* +0x24 */
    int32_t node_size;         /* +0x28: at least two pointers */
    int32_t stat_2c, stat_30;  /* +0x2c, +0x30 */
    int32_t blocks_added;      /* +0x34 */
    int32_t resets;            /* +0x38 */
} AoeNodePoolRor;

/* 0x004a34b0: a pool of `node_size` nodes, `initial` of them at once and
   `per_block` more whenever it runs dry; 0x004a3510 frees it. 0x004a3560
   grows the block tables (never shrinks); 0x00516280 adds a block.
   0x004a35f0 takes a node (the free list first, else the next unused
   one); 0x004a3660 returns one; 0x004a3680 returns a chain (from `first`
   through `last`, or to its end) and counts it. 0x004a36e0 makes every
   node unused again; 0x004a3700 clears the counters. */
int aoe_pool_init_ror(AoeNodePoolRor *p, int32_t initial, int32_t per_block,
                      int32_t node_size);
void aoe_pool_free_ror(AoeNodePoolRor *p);
int aoe_pool_grow_ror(AoeNodePoolRor *p, int32_t capacity);
int aoe_pool_add_block_ror(AoeNodePoolRor *p, int32_t count);
void *aoe_pool_take_ror(AoeNodePoolRor *p);
void aoe_pool_give_ror(AoeNodePoolRor *p, void *node);
int32_t aoe_pool_give_chain_ror(AoeNodePoolRor *p, void *first, void *last);
void aoe_pool_reset_ror(AoeNodePoolRor *p);
void aoe_pool_clear_stats_ror(AoeNodePoolRor *p);

/* A span: columns x0..x1 of one row, in a sorted, merged list. */
typedef struct AoeSpanRor {
    struct AoeSpanRor *next, *prev; /* +0, +4 */
    int32_t x0, x1;                 /* +8, +0xc */
} AoeSpanRor;

/* The span region (0x004a3710..0x004a4200): per row, a sorted list of
   non-overlapping column spans with the row's first and last span, its
   extent and span count; additions merge and removals split spans. Spans
   are clipped to the bounds. */
typedef struct {
    AoeNodePoolRor pool;
    AoeSpanRor **first;   /* +0x3c */
    AoeSpanRor **last;    /* +0x40 */
    int32_t *min_x;       /* +0x44 */
    int32_t *max_x;       /* +0x48 */
    int32_t *count;       /* +0x4c */
    int32_t rows, cols;   /* +0x50, +0x54: at least 16 each */
    int32_t left, right;  /* +0x58, +0x5c */
    int32_t top, bottom;  /* +0x60, +0x64 */
} AoeSpanRegionRor;

/* 0x004a3710 (w, h): the pool sized for (w + 31) / 32 spans per row,
   bounds the whole area; 0x004a37f0 frees it. 0x004a3850 bounds (left,
   top, right, bottom in either order; set only when they overlap the
   area, clamped to it); 0x004a38c0 empties every row. */
int aoe_span_region_init_ror(AoeSpanRegionRor *r, int32_t w, int32_t h);
void aoe_span_region_free_ror(AoeSpanRegionRor *r);
void aoe_span_region_set_bounds_ror(AoeSpanRegionRor *r, int32_t l, int32_t t,
                                    int32_t rr, int32_t b);
void aoe_span_region_clear_ror(AoeSpanRegionRor *r);
/* 0x004a3950 add columns x0..x1 of row y (merging touching spans);
   0x004a4000 remove them (splitting a span when needed). */
void aoe_span_region_add_ror(AoeSpanRegionRor *r, int32_t x0, int32_t x1,
                             int32_t y);
void aoe_span_region_remove_ror(AoeSpanRegionRor *r, int32_t x0, int32_t x1,
                                int32_t y);
/* 0x004a3d10: an SLP frame's outline at (x, y) (or mirrored), widened to
   whole groups of four columns. `outline` holds `height` left/right
   16-bit pairs (0x8000 marks an empty row). */
void aoe_span_region_add_outline_ror(AoeSpanRegionRor *r,
                                     const uint8_t *outline, int32_t width,
                                     int32_t height, int32_t x, int32_t y,
                                     int mirrored);
/* 0x004a3e00: a line, widened to whole groups of four columns.
   0x004a3f60 / 0x004a3fb0: add / remove a list of (row, x0, x1) byte
   triples ended by 0xff, offset by (x, y). 0x004a4200: this region
   becomes the union of two of the same size (spans widened to groups of
   four). */
void aoe_span_region_add_line_ror(AoeSpanRegionRor *r, int32_t x0, int32_t y0,
                                  int32_t x1, int32_t y1);
void aoe_span_region_add_list_ror(AoeSpanRegionRor *r, const uint8_t *list,
                                  int32_t x, int32_t y);
void aoe_span_region_remove_list_ror(AoeSpanRegionRor *r, const uint8_t *list,
                                     int32_t x, int32_t y);
void aoe_span_region_union_ror(AoeSpanRegionRor *r, const AoeSpanRegionRor *a,
                               const AoeSpanRegionRor *b);

#endif
