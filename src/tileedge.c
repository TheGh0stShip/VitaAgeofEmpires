#include "tileedge.h"

#include "terrain.h"

#include <string.h>

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int parse_offsets(const uint8_t *bytes, size_t size,
                         TileEdgeIndex *index, unsigned sides) {
    TileEdgeIndex parsed;
    if (!bytes || !index || size < TILEEDGE_SHAPES * 4)
        return 0;
    memset(&parsed, 0, sizeof(parsed));
    for (unsigned shape = 0; shape < TILEEDGE_SHAPES; ++shape) {
        uint32_t table = le32(bytes + shape * 4);
        if (table > size || TILEEDGE_PATTERNS * 4 * sides > size - table)
            return 0;
        for (unsigned pattern = 0; pattern < TILEEDGE_PATTERNS; ++pattern) {
            for (unsigned side = 0; side < sides; ++side) {
                uint32_t offset = le32(bytes + table +
                                       (pattern * sides + side) * 4);
                if (offset && offset >= size) return 0;
                parsed.offsets[shape][pattern][side] = offset;
            }
        }
    }
    parsed.bytes = bytes;
    parsed.size = size;
    *index = parsed;
    return 1;
}

int tileedge_parse(const uint8_t *bytes, size_t size, TileEdgeIndex *index) {
    return parse_offsets(bytes, size, index, 2);
}

int blkedge_parse(const uint8_t *bytes, size_t size, TileEdgeIndex *index) {
    return parse_offsets(bytes, size, index, 1);
}

int tileedge_build_view_plan(uint8_t visibility_mode, int fog_enabled,
                             int bypass_exploration_gate,
                             uint8_t visible_pattern,
                             uint8_t explored_pattern,
                             TileEdgeViewPlan *plan) {
    TileEdgeViewPlan selected;
    if (!plan || visible_pattern >= TILEEDGE_PATTERNS ||
        explored_pattern >= TILEEDGE_PATTERNS ||
        (visibility_mode != TERRAIN_VIEW_HIDDEN &&
         visibility_mode != TERRAIN_VIEW_VISIBLE &&
         visibility_mode != TERRAIN_VIEW_EXPLORED_FOG))
        return 0;
    selected.tile_pattern = visibility_mode == TERRAIN_VIEW_VISIBLE &&
                            fog_enabled ? visible_pattern : 0;
    selected.black_pattern = visibility_mode != TERRAIN_VIEW_HIDDEN &&
                             !bypass_exploration_gate ? explored_pattern : 0;
    selected.first_side = visibility_mode == TERRAIN_VIEW_VISIBLE ? 0 :
                          TILEEDGE_SIDE_NONE;
    selected.second_side = visibility_mode == TERRAIN_VIEW_HIDDEN ?
                           TILEEDGE_SIDE_NONE :
                           visibility_mode == TERRAIN_VIEW_EXPLORED_FOG ?
                           0 : 1;
    *plan = selected;
    return 1;
}

int tileedge_visit_spans(const TileEdgeIndex *index, uint32_t shape,
                          uint32_t pattern, uint32_t side,
                          TileEdgeSpanVisitor visitor, void *context,
                          size_t *span_count) {
    uint32_t offset;
    size_t count = 0, pos;
    if (!index || !index->bytes || !span_count ||
        shape >= TILEEDGE_SHAPES || pattern >= TILEEDGE_PATTERNS || side > 1)
        return 0;
    offset = index->offsets[shape][pattern][side];
    if (!offset) { *span_count = 0; return 1; }
    if (offset >= index->size) return 0;
    pos = offset;
    while (pos < index->size) {
        uint8_t row = index->bytes[pos];
        if (row == 0xff) {
            if (visitor) {
                for (size_t span_pos = offset; span_pos < pos; span_pos += 3)
                    visitor(index->bytes[span_pos], index->bytes[span_pos + 1],
                            index->bytes[span_pos + 2], context);
            }
            *span_count = count;
            return 1;
        }
        if (index->size - pos < 3) return 0;
        uint8_t left = index->bytes[pos + 1];
        uint8_t right = index->bytes[pos + 2];
        if (left > right) return 0;
        ++count;
        pos += 3;
    }
    return 0;
}

typedef struct {
    uint8_t *coverage;
    uint32_t width, height;
    size_t stride;
    int64_t origin_x, origin_y;
    uint8_t value;
} CoverageContext;

static void apply_span(uint8_t row, uint8_t left, uint8_t right,
                       void *context) {
    CoverageContext *dst = context;
    int64_t y = dst->origin_y + row;
    int64_t x0 = dst->origin_x + left;
    int64_t x1 = dst->origin_x + right;
    if (y < 0 || y >= dst->height || x1 < 0 || x0 >= dst->width)
        return;
    if (x0 < 0) x0 = 0;
    if (x1 >= dst->width) x1 = dst->width - 1;
    memset(dst->coverage + (size_t)y * dst->stride + (size_t)x0,
           dst->value, (size_t)(x1 - x0 + 1));
}

int tileedge_apply_coverage(const TileEdgeIndex *index, uint32_t shape,
                            uint32_t pattern, uint32_t side,
                            int32_t origin_x, int32_t origin_y,
                            uint8_t *coverage, size_t coverage_size,
                            uint32_t width, uint32_t height, size_t stride,
                            int operation, size_t *span_count) {
    size_t count;
    CoverageContext dst;
    if (!coverage || !span_count || !width || !height || width > stride ||
        width > coverage_size ||
        (size_t)(height - 1) > (coverage_size - width) / stride ||
        (operation != TILEEDGE_COVERAGE_ADD &&
         operation != TILEEDGE_COVERAGE_REMOVE))
        return 0;
    /* Validate before the first write, so a malformed stream is atomic. */
    if (!tileedge_visit_spans(index, shape, pattern, side,
                              NULL, NULL, &count))
        return 0;
    dst.coverage = coverage;
    dst.width = width;
    dst.height = height;
    dst.stride = stride;
    dst.origin_x = origin_x;
    dst.origin_y = origin_y;
    dst.value = operation == TILEEDGE_COVERAGE_ADD ? 255 : 0;
    if (!tileedge_visit_spans(index, shape, pattern, side,
                              apply_span, &dst, &count))
        return 0;
    *span_count = count;
    return 1;
}

static int plane_valid(TileEdgeCoveragePlane plane, uint32_t width,
                       uint32_t height) {
    return plane.bytes && width && height && width <= plane.stride &&
           width <= plane.size &&
           (size_t)(height - 1) <= (plane.size - width) / plane.stride;
}

int tileedge_apply_view_plan(const TileEdgeIndex *tile_edges,
                             const TileEdgeIndex *black_edges,
                             uint32_t shape, const TileEdgeViewPlan *plan,
                             int32_t origin_x, int32_t origin_y,
                             TileEdgeCoveragePlane first,
                             TileEdgeCoveragePlane second,
                             uint32_t width, uint32_t height) {
    const TileEdgeCoveragePlane planes[2] = {first, second};
    const uint8_t sides[2] = {plan ? plan->first_side : 0,
                              plan ? plan->second_side : 0};
    size_t count;
    if (!plan || shape >= TILEEDGE_SHAPES ||
        plan->tile_pattern >= TILEEDGE_PATTERNS ||
        plan->black_pattern >= TILEEDGE_PATTERNS ||
        (sides[0] != TILEEDGE_SIDE_NONE && sides[0] > 1) ||
        (sides[1] != TILEEDGE_SIDE_NONE && sides[1] > 1) ||
        (sides[0] != TILEEDGE_SIDE_NONE && sides[1] != TILEEDGE_SIDE_NONE &&
         first.bytes == second.bytes))
        return 0;
    for (unsigned i = 0; i < 2; ++i) {
        if (sides[i] == TILEEDGE_SIDE_NONE) continue;
        if (!plane_valid(planes[i], width, height) ||
            !tileedge_visit_spans(tile_edges, shape, plan->tile_pattern,
                                  sides[i], NULL, NULL, &count))
            return 0;
        if (black_edges && plan->black_pattern &&
            !tileedge_visit_spans(black_edges, shape, plan->black_pattern,
                                  0, NULL, NULL, &count))
            return 0;
    }
    for (unsigned i = 0; i < 2; ++i) {
        if (sides[i] == TILEEDGE_SIDE_NONE) continue;
        if (!tileedge_apply_coverage(tile_edges, shape, plan->tile_pattern,
                                     sides[i], origin_x, origin_y,
                                     planes[i].bytes, planes[i].size, width,
                                     height, planes[i].stride,
                                     TILEEDGE_COVERAGE_ADD, &count))
            return 0;
        if (black_edges && plan->black_pattern &&
            !tileedge_apply_coverage(black_edges, shape,
                                     plan->black_pattern, 0,
                                     origin_x, origin_y, planes[i].bytes,
                                     planes[i].size, width, height,
                                     planes[i].stride,
                                     TILEEDGE_COVERAGE_REMOVE, &count))
            return 0;
    }
    return 1;
}
