#include "tileedge.h"
#include "terrain.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void put_span(uint8_t *p, uint8_t row, uint8_t left, uint8_t right) {
    p[0] = row;
    p[1] = left;
    p[2] = right;
    p[3] = 0xff;
}

static void count_span(uint8_t row, uint8_t left, uint8_t right,
                       void *context) {
    size_t *count = context;
    assert(row == 0 && left == 2 && right == 3);
    ++*count;
}

int main(int argc, char **argv) {
    uint8_t synthetic[448] = {0};
    TileEdgeIndex index = {0};
    size_t spans = 99;
    for (unsigned shape = 0; shape < TILEEDGE_SHAPES; ++shape)
        put32(synthetic + shape * 4, 68);
    put32(synthetic + 68, 444);
    synthetic[444] = 0;
    synthetic[445] = 2;
    synthetic[446] = 3;
    synthetic[447] = 0xff;
    assert(tileedge_parse(synthetic, sizeof(synthetic), &index));
    assert(index.offsets[0][0][0] == 444);
    assert(index.offsets[16][46][1] == 0);
    assert(tileedge_visit_spans(&index, 0, 0, 0, NULL, NULL, &spans));
    assert(spans == 1);
    size_t callbacks = 0;
    assert(tileedge_visit_spans(&index, 0, 0, 0, count_span,
                                &callbacks, &spans));
    assert(callbacks == 1 && spans == 1);
    uint8_t coverage[4 * 8];
    memset(coverage, 7, sizeof(coverage));
    for (unsigned y = 0; y < 4; ++y)
        memset(coverage + y * 8, 0, 6);
    assert(tileedge_apply_coverage(&index, 0, 0, 0, -1, 1, coverage,
                                   sizeof(coverage), 6, 4, 8,
                                   TILEEDGE_COVERAGE_ADD, &spans));
    assert(spans == 1);
    for (unsigned y = 0; y < 4; ++y)
        for (unsigned x = 0; x < 8; ++x)
            assert(coverage[y * 8 + x] ==
                   (x >= 6 ? 7 : (y == 1 && (x == 1 || x == 2) ? 255 : 0)));
    assert(tileedge_apply_coverage(&index, 0, 0, 0, -1, 1, coverage,
                                   sizeof(coverage), 6, 4, 8,
                                   TILEEDGE_COVERAGE_REMOVE, &spans));
    assert(coverage[1 * 8 + 1] == 0 && coverage[1 * 8 + 2] == 0);
    assert(tileedge_apply_coverage(&index, 0, 0, 0, -3, 2, coverage,
                                   sizeof(coverage), 6, 4, 8,
                                   TILEEDGE_COVERAGE_ADD, &spans));
    assert(coverage[2 * 8] == 255 && coverage[2 * 8 + 1] == 0);
    assert(!tileedge_apply_coverage(&index, 0, 0, 0, 0, 0, coverage,
                                    sizeof(coverage) - 3, 6, 4, 8,
                                    TILEEDGE_COVERAGE_ADD, &spans));
    assert(!tileedge_apply_coverage(&index, 0, 0, 0, 0, 0, coverage,
                                    sizeof(coverage), 6, 4, 8, 2, &spans));
    synthetic[447] = 0;
    assert(!tileedge_visit_spans(&index, 0, 0, 0, count_span,
                                 &callbacks, &spans));
    assert(callbacks == 1);
    uint8_t before[sizeof(coverage)];
    memcpy(before, coverage, sizeof(before));
    assert(!tileedge_apply_coverage(&index, 0, 0, 0, 0, 0, coverage,
                                    sizeof(coverage), 6, 4, 8,
                                    TILEEDGE_COVERAGE_ADD, &spans));
    assert(memcmp(before, coverage, sizeof(before)) == 0);
    synthetic[447] = 0xff;
    assert(tileedge_visit_spans(&index, 0, 0, 1, NULL, NULL, &spans));
    assert(spans == 0);
    assert(!tileedge_visit_spans(&index, 17, 0, 0, NULL, NULL, &spans));
    assert(!tileedge_parse(synthetic, 444, &index));
    put32(synthetic, 100);
    assert(!tileedge_parse(synthetic, sizeof(synthetic), &index));
    uint8_t black_synthetic[260] = {0};
    for (unsigned shape = 0; shape < TILEEDGE_SHAPES; ++shape)
        put32(black_synthetic + shape * 4, 68);
    put32(black_synthetic + 68, 256);
    black_synthetic[256] = 0;
    black_synthetic[257] = 2;
    black_synthetic[258] = 3;
    black_synthetic[259] = 0xff;
    assert(blkedge_parse(black_synthetic, sizeof(black_synthetic), &index));
    assert(index.offsets[0][0][0] == 256);
    assert(index.offsets[0][0][1] == 0);
    assert(index.offsets[16][46][0] == 0);
    assert(tileedge_visit_spans(&index, 0, 0, 0, NULL, NULL, &spans));
    assert(spans == 1);
    assert(tileedge_visit_spans(&index, 0, 0, 1, NULL, NULL, &spans));
    assert(spans == 0);
    memset(coverage, 0, sizeof(coverage));
    assert(tileedge_apply_coverage(&index, 0, 0, 0, 0, 0, coverage,
                                   sizeof(coverage), 6, 4, 8,
                                   TILEEDGE_COVERAGE_ADD, &spans));
    assert(coverage[2] == 255 && coverage[3] == 255);
    assert(tileedge_apply_coverage(&index, 0, 0, 0, 0, 0, coverage,
                                   sizeof(coverage), 6, 4, 8,
                                   TILEEDGE_COVERAGE_REMOVE, &spans));
    assert(coverage[2] == 0 && coverage[3] == 0);
    assert(!blkedge_parse(black_synthetic, 255, &index));
    put32(black_synthetic, 260);
    assert(!blkedge_parse(black_synthetic, sizeof(black_synthetic), &index));
    TileEdgeViewPlan plan = {0};
    assert(tileedge_build_view_plan(TERRAIN_VIEW_VISIBLE, 1, 0, 7, 9,
                                    &plan));
    assert(plan.tile_pattern == 7 && plan.black_pattern == 9 &&
           plan.first_side == 0 && plan.second_side == 1);
    assert(tileedge_build_view_plan(TERRAIN_VIEW_VISIBLE, 0, 1, 7, 9,
                                    &plan));
    assert(plan.tile_pattern == 0 && plan.black_pattern == 0 &&
           plan.first_side == 0 && plan.second_side == 1);
    assert(tileedge_build_view_plan(TERRAIN_VIEW_EXPLORED_FOG, 1, 0, 7, 9,
                                    &plan));
    assert(plan.tile_pattern == 0 && plan.black_pattern == 9 &&
           plan.first_side == TILEEDGE_SIDE_NONE && plan.second_side == 0);
    assert(tileedge_build_view_plan(TERRAIN_VIEW_HIDDEN, 1, 0, 7, 9,
                                    &plan));
    assert(plan.tile_pattern == 0 && plan.black_pattern == 0 &&
           plan.first_side == TILEEDGE_SIDE_NONE &&
           plan.second_side == TILEEDGE_SIDE_NONE);
    assert(!tileedge_build_view_plan(3, 1, 0, 7, 9, &plan));
    assert(!tileedge_build_view_plan(TERRAIN_VIEW_VISIBLE, 1, 0, 47, 9,
                                     &plan));
    uint8_t dual_synthetic[460] = {0};
    for (unsigned shape = 0; shape < TILEEDGE_SHAPES; ++shape)
        put32(dual_synthetic + shape * 4, 68);
    put32(dual_synthetic + 68, 444);
    put32(dual_synthetic + 72, 448);
    put32(dual_synthetic + 76, 452);
    put32(dual_synthetic + 80, 456);
    put_span(dual_synthetic + 444, 1, 1, 2);
    put_span(dual_synthetic + 448, 1, 5, 6);
    put_span(dual_synthetic + 452, 0, 2, 4);
    put_span(dual_synthetic + 456, 0, 3, 5);
    TileEdgeIndex dual_index = {0};
    assert(tileedge_parse(dual_synthetic, sizeof(dual_synthetic),
                          &dual_index));
    put32(black_synthetic, 68);
    put32(black_synthetic + 72, 256);
    put_span(black_synthetic + 256, 0, 3, 3);
    TileEdgeIndex black_index = {0};
    assert(blkedge_parse(black_synthetic, sizeof(black_synthetic),
                         &black_index));
    uint8_t first_bytes[4 * 8] = {0}, second_bytes[4 * 8] = {0};
    TileEdgeCoveragePlane first = {first_bytes, sizeof(first_bytes), 8};
    TileEdgeCoveragePlane second = {second_bytes, sizeof(second_bytes), 8};
    assert(tileedge_build_view_plan(TERRAIN_VIEW_VISIBLE, 1, 0, 1, 1,
                                    &plan));
    assert(tileedge_apply_view_plan(&dual_index, &black_index, 0, &plan,
                                    0, 0, first, second, 8, 4));
    assert(first_bytes[2] == 255 && first_bytes[3] == 0 &&
           first_bytes[4] == 255 && first_bytes[5] == 0);
    assert(second_bytes[2] == 0 && second_bytes[3] == 0 &&
           second_bytes[4] == 255 && second_bytes[5] == 255);
    memset(first_bytes, 0, sizeof(first_bytes));
    memset(second_bytes, 0, sizeof(second_bytes));
    assert(tileedge_build_view_plan(TERRAIN_VIEW_EXPLORED_FOG, 1, 0, 1, 1,
                                    &plan));
    assert(tileedge_apply_view_plan(&dual_index, &black_index, 0, &plan,
                                    0, 0, first, second, 8, 4));
    assert(first_bytes[1 * 8 + 1] == 0 &&
           second_bytes[1 * 8 + 1] == 255 &&
           second_bytes[1 * 8 + 2] == 255 &&
           second_bytes[1 * 8 + 5] == 0);
    memset(first_bytes, 19, sizeof(first_bytes));
    memset(second_bytes, 23, sizeof(second_bytes));
    black_synthetic[259] = 0;
    assert(!tileedge_apply_view_plan(&dual_index, &black_index, 0, &plan,
                                     0, 0, first, second, 8, 4));
    for (size_t i = 0; i < sizeof(first_bytes); ++i) {
        assert(first_bytes[i] == 19);
        assert(second_bytes[i] == 23);
    }
    black_synthetic[259] = 0xff;
    assert(!tileedge_apply_view_plan(&dual_index, &black_index, 0, &plan,
                                     0, 0, first, second, 9, 4));
    assert(tileedge_build_view_plan(TERRAIN_VIEW_VISIBLE, 1, 0, 1, 1,
                                    &plan));
    dual_synthetic[459] = 0;
    assert(!tileedge_apply_view_plan(&dual_index, &black_index, 0, &plan,
                                     0, 0, first, second, 8, 4));
    for (size_t i = 0; i < sizeof(first_bytes); ++i) {
        assert(first_bytes[i] == 19);
        assert(second_bytes[i] == 23);
    }
    dual_synthetic[459] = 0xff;
    assert(tileedge_apply_view_plan(&dual_index, NULL, 0, &plan,
                                    0, 0, first, second, 8, 4));
    assert(first_bytes[3] == 255 && second_bytes[3] == 255);
    first_bytes[3] = 19;
    second_bytes[3] = 23;
    assert(tileedge_build_view_plan(TERRAIN_VIEW_HIDDEN, 1, 0, 1, 1,
                                    &plan));
    assert(tileedge_apply_view_plan(&dual_index, &black_index, 0, &plan,
                                    0, 0, first, second, 8, 4));
    assert(first_bytes[0] == 19 && second_bytes[0] == 23);
    if (argc == 1) return 0;
    if (argc != 2) return 2;
    FILE *file = fopen(argv[1], "rb");
    if (!file) return 1;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return 1; }
    long length = ftell(file);
    if (length <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 1;
    }
    uint8_t *bytes = malloc((size_t)length);
    if (!bytes) { fclose(file); return 1; }
    int ok = fread(bytes, 1, (size_t)length, file) == (size_t)length &&
             tileedge_parse(bytes, (size_t)length, &index);
    if (ok) {
        size_t nonzero = 0, total_spans = 0;
        size_t covered_pixels = 0, view_pixels = 0;
        uint8_t *plane = malloc(256 * 256);
        uint8_t *second_plane = malloc(256 * 256);
        if (!plane || !second_plane) ok = 0;
        for (unsigned shape = 0; shape < TILEEDGE_SHAPES; ++shape)
            for (unsigned pattern = 0; pattern < TILEEDGE_PATTERNS; ++pattern)
                for (unsigned side = 0; side < 2; ++side) {
                    nonzero += index.offsets[shape][pattern][side] != 0;
                    size_t count = 0;
                    if (!tileedge_visit_spans(&index, shape, pattern, side,
                                              NULL, NULL, &count)) ok = 0;
                    total_spans += count;
                    if (ok) {
                        memset(plane, 0, 256 * 256);
                        size_t applied = 0;
                        if (!tileedge_apply_coverage(&index, shape, pattern,
                                                     side, 0, 0, plane,
                                                     256 * 256, 256, 256, 256,
                                                     TILEEDGE_COVERAGE_ADD,
                                                     &applied) || applied != count)
                            ok = 0;
                        for (size_t i = 0; i < 256 * 256; ++i)
                            covered_pixels += plane[i] == 255;
                    }
                }
        if (ok) {
            TileEdgeCoveragePlane first_view = {plane, 256 * 256, 256};
            TileEdgeCoveragePlane second_view = {second_plane, 256 * 256, 256};
            for (unsigned shape = 0; shape < TILEEDGE_SHAPES; ++shape)
                for (unsigned pattern = 0; pattern < TILEEDGE_PATTERNS;
                     ++pattern) {
                    memset(plane, 0, 256 * 256);
                    memset(second_plane, 0, 256 * 256);
                    if (!tileedge_build_view_plan(TERRAIN_VIEW_VISIBLE, 1, 1,
                                                   (uint8_t)pattern, 0,
                                                   &plan) ||
                        !tileedge_apply_view_plan(&index, NULL, shape,
                                                   &plan, 0, 0,
                                                   first_view, second_view,
                                                   256, 256)) {
                        ok = 0;
                        break;
                    }
                    for (size_t i = 0; i < 256 * 256; ++i)
                        view_pixels += plane[i] == 255 ||
                                       second_plane[i] == 255;
                }
            if (!view_pixels) ok = 0;
        }
        free(second_plane);
        free(plane);
        if (nonzero && !covered_pixels) ok = 0;
        if (ok)
            printf("tileedge: %ld bytes, %zu streams, %zu spans, %zu covered pixels, %zu paired-view pixels\n",
                   length, nonzero, total_spans, covered_pixels, view_pixels);
    }
    free(bytes);
    fclose(file);
    return ok ? 0 : 1;
}
