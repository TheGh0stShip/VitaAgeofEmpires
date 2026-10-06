#ifndef VITA_AOE_TILEEDGE_H
#define VITA_AOE_TILEEDGE_H

#include <stddef.h>
#include <stdint.h>

enum { TILEEDGE_SHAPES = 17, TILEEDGE_PATTERNS = 47 };

typedef struct {
    const uint8_t *bytes; /* Caller owns the original file buffer. */
    size_t size;
    uint32_t offsets[TILEEDGE_SHAPES][TILEEDGE_PATTERNS][2];
} TileEdgeIndex;

/* Parses the original TILEEDGE.DAT offset tables. Empiresx.exe disc 0x00511d70
   (patched 0x0050f5c0) and Empires.exe 0x004b5e50 relocate 17 shape-table offsets, then 47 pairs
   of optional data offsets per shape. This validates and exposes those
   offsets but does not render the edge pixel streams. */
int tileedge_parse(const uint8_t *bytes, size_t size, TileEdgeIndex *index);

/* Rise of Rome's optional DATA2/BLKEDGE.DAT has the same 17 shape tables,
   but only one stream offset per each of the 47 patterns. The selected
   stream occupies side 0 of TileEdgeIndex; side 1 is always null. The base
   game's edge loader does not reference BLKEDGE.DAT. */
int blkedge_parse(const uint8_t *bytes, size_t size, TileEdgeIndex *index);

enum { TILEEDGE_SIDE_NONE = 0xff };

typedef struct {
    uint8_t tile_pattern;
    uint8_t black_pattern; /* Zero disables black-edge subtraction. */
    uint8_t first_side;    /* TILEEDGE side 0, 1, or NONE. */
    uint8_t second_side;
} TileEdgeViewPlan;

/* Empiresx.exe 0x00514c10 selects two independent clip-plane streams.
   visible_pattern and explored_pattern are the 0..46 neighbor lookups for
   the current tile. Hidden tiles select neither plane. In explored-fog mode,
   only the second plane gets TILEEDGE pattern 0 side 0. This plans stream
   selection only; it does not apply colors or draw tile art. */
int tileedge_build_view_plan(uint8_t visibility_mode, int fog_enabled,
                             int bypass_exploration_gate,
                             uint8_t visible_pattern,
                             uint8_t explored_pattern,
                             TileEdgeViewPlan *plan);

typedef void (*TileEdgeSpanVisitor)(uint8_t row, uint8_t left,
                                    uint8_t right, void *context);

/* Empiresx.exe 0x004a5690 / 0x004a56e0 consume a non-null stream as
   (row,left,right) byte triples until row 0xff. Returns zero on missing
   terminator, invalid selection, or malformed span. A null stream has zero
   spans and is valid. Visitor may be null for validation/counting only, and
   is not called at all if validation fails. */
int tileedge_visit_spans(const TileEdgeIndex *index, uint32_t shape,
                          uint32_t pattern, uint32_t side,
                          TileEdgeSpanVisitor visitor, void *context,
                          size_t *span_count);

enum { TILEEDGE_COVERAGE_ADD = 0, TILEEDGE_COVERAGE_REMOVE = 1 };

/* Applies the original inclusive scanline add/remove operations to a byte
   coverage plane (0 or 255). Origin is the screen position of the stream's
   (0,0), and spans are clipped to width x height. The destination is left
   untouched if arguments or the source stream are invalid. This does not
   select fog colors, blend terrain, or draw the game's viewport. */
int tileedge_apply_coverage(const TileEdgeIndex *index, uint32_t shape,
                            uint32_t pattern, uint32_t side,
                            int32_t origin_x, int32_t origin_y,
                            uint8_t *coverage, size_t coverage_size,
                            uint32_t width, uint32_t height, size_t stride,
                            int operation, size_t *span_count);

typedef struct {
    uint8_t *bytes;
    size_t size;
    size_t stride;
} TileEdgeCoveragePlane;

/* Executes the original add-then-optional-black-subtract sequence on two
   independent clipping planes. An inactive side needs no destination. A
   null black_edges index means the optional file is unavailable. Preflight
   validates all selected streams and active destinations before any write;
   the caller owns and initializes the planes. This is still only clip state,
   not fog palette compositing or terrain drawing. */
int tileedge_apply_view_plan(const TileEdgeIndex *tile_edges,
                             const TileEdgeIndex *black_edges,
                             uint32_t shape, const TileEdgeViewPlan *plan,
                             int32_t origin_x, int32_t origin_y,
                             TileEdgeCoveragePlane first,
                             TileEdgeCoveragePlane second,
                             uint32_t width, uint32_t height);

#endif
