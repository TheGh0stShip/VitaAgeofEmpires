#include "pathfinder.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"

void aoe_path_distance_table_ror(int32_t table[256]) {
    if (!table) return;
    /* fild i; fmul 1.4142f; fmul 16.0f; fadd 0.5f; __ftol. */
    for (int32_t i = 0; i < 256; ++i)
        table[i] = aoe_ftol32((double)i * (double)1.4142f * 16.0 + 0.5);
}

int32_t aoe_path_octile_ror(const int32_t table[256], int32_t x1, int32_t y1,
                            int32_t x2, int32_t y2) {
    int32_t dx, dy, shorter, extra;
    if (x2 < x1) { int32_t t = x1; x1 = x2; x2 = t; }
    if (y2 < y1) { int32_t t = y1; y1 = y2; y2 = t; }
    dx = x2 - x1;
    dy = y2 - y1;
    if (dx < dy) {
        shorter = dx;
        extra = dy - dx;
    } else {
        shorter = dy;
        extra = dx - dy;
    }
    return table[shorter & 0xff] + extra * 16;
}

void aoe_path_set_map_ror(AoePathfinderRor *path, int has_map,
                          int32_t map_width, int32_t map_height,
                          int32_t width, int32_t height) {
    if (!path) return;
    path->width = has_map ? map_width : width;
    path->height = has_map ? map_height : height;
    path->generation = 0xf0;
}

int aoe_path_fill_penalty_ror(AoePathfinderRor *path, uint8_t value) {
    if (!path || !path->penalty) return 0;
    memset(path->penalty, value, AOE_PATH_GRID_CELLS_ROR);
    return 1;
}

int aoe_path_clear_occupancy_ror(AoePathfinderRor *path) {
    if (!path || !path->occupancy) return 0;
    memset(path->occupancy, 0, AOE_PATH_OCCUPANCY_BYTES_ROR);
    return 1;
}

/* Byte index and shift of quarter cell (qx, qy); -1 outside the bitmap. */
static int32_t occupancy_cell(const AoePathfinderRor *path, int32_t qx,
                              int32_t qy, uint8_t *shift) {
    int32_t index;
    if (!path || !path->occupancy || qx < 0 ||
        qx >= AOE_PATH_OCCUPANCY_ROWS_ROR || qy < 0 ||
        (qy >> 2) >= AOE_PATH_GRID_SIDE_ROR)
        return -1;
    index = qx * AOE_PATH_GRID_SIDE_ROR + (qy >> 2);
    *shift = (uint8_t)(2 * (qy & 3)); /* table 0x00557b30 */
    return index;
}

int aoe_path_occupancy_add_ror(AoePathfinderRor *path, int32_t qx,
                               int32_t qy) {
    uint8_t shift, count;
    int32_t index = occupancy_cell(path, qx, qy, &shift);
    if (index < 0) return 0;
    count = (uint8_t)((path->occupancy[index] >> shift) & 3);
    if (count < 3)
        path->occupancy[index] = (uint8_t)(
            (path->occupancy[index] & ~(3u << shift)) | (count + 1u) << shift);
    return 1;
}

int aoe_path_occupancy_remove_ror(AoePathfinderRor *path, int32_t qx,
                                  int32_t qy) {
    uint8_t shift, count;
    int32_t index = occupancy_cell(path, qx, qy, &shift);
    if (index < 0) return 0;
    count = (uint8_t)((path->occupancy[index] >> shift) & 3);
    if (count)
        path->occupancy[index] = (uint8_t)(
            (path->occupancy[index] & ~(3u << shift)) | (count - 1u) << shift);
    return 1;
}

int aoe_path_set_penalty_ror(AoePathfinderRor *path, int32_t first,
                             int32_t second, uint8_t value) {
    int32_t index;
    if (!path || !path->penalty) return 0;
    index = first * AOE_PATH_GRID_SIDE_ROR + second;
    if (index < 0 || index >= AOE_PATH_GRID_CELLS_ROR) return 0;
    path->penalty[index] = value;
    return 1;
}

static AoePathPrecheckRor early(void *unit, int32_t mode,
                                const AoePathPrecheckOps *ops,
                                AoePathPrecheckRor result) {
    if (mode == 1) ops->clear_unit_path(ops->context, unit);
    return result;
}

AoePathPrecheckRor aoe_path_precheck_ror(const AoePathfinderRor *path,
                                         const int32_t table[256],
                                         int32_t x1, int32_t y1, int32_t x2,
                                         int32_t y2, void *unit, float range,
                                         int32_t mode, float *distance,
                                         const AoePathPrecheckOps *ops) {
    float speed;
    if (!path || !table || !ops || !ops->speed || !ops->clear_unit_path)
        return AOE_PATH_PRECHECK_FAIL_ROR;
    if (x1 < 0 || y1 < 0 || x1 > path->width || y1 > path->width ||
        x2 < 0 || y2 < 0 || x2 > path->width || y2 > path->width)
        return early(unit, mode, ops, AOE_PATH_PRECHECK_FAIL_ROR);
    /* fcomp 1.0f; test ah,0x41: only a range strictly above 1 checks. */
    if (!(range > 1.0f)) return AOE_PATH_PRECHECK_SEARCH_ROR;
    /* fild distance; range * 16.0f; fcompp: in range when strictly above. */
    if ((double)range * 16.0 >
        (double)aoe_path_octile_ror(table, x1, y1, x2, y2)) {
        if (distance) *distance = 0.0f;
        return early(unit, mode, ops, AOE_PATH_PRECHECK_IN_RANGE_ROR);
    }
    speed = ops->speed(ops->context, unit);
    if (isnan(speed) || speed == 0.0f) {
        if (distance) *distance = 0.0f;
        return early(unit, mode, ops, AOE_PATH_PRECHECK_FAIL_ROR);
    }
    return AOE_PATH_PRECHECK_SEARCH_ROR;
}

uint8_t aoe_path_penalty_ror(const AoePathfinderRor *path, int32_t first,
                             int32_t second) {
    int32_t index = first * AOE_PATH_GRID_SIDE_ROR + second;
    if (!path || !path->penalty || index < 0 || index >= AOE_PATH_GRID_CELLS_ROR)
        return 0;
    return path->penalty[index];
}

void aoe_path_count_search_ror(AoePathfinderRor *path) {
    if (path) ++path->searches;
}

int aoe_path_collect_nearby_ror(const AoePathfinderRor *path,
                                AoePathIdListRor *out) {
    int64_t index;
    if (!path || !out) return 0;
    for (index = (int64_t)path->nearby_count; index >= 0; --index) {
        int32_t id, k;
        if (!path->nearby_ids || (uint64_t)index >= path->nearby_capacity)
            continue;
        id = path->nearby_ids[index];
        for (k = 0; k < out->count && k < out->capacity; ++k)
            if (out->ids[k] == id) break;
        if (k < out->count && k < out->capacity) continue;
        if (out->capacity - 1 < out->count) {
            int32_t *grown = malloc(((size_t)out->count + 1) * sizeof(*grown));
            if (!grown) return 0;
            for (k = 0; k < out->capacity && k < out->count + 1; ++k)
                grown[k] = out->ids[k];
            free(out->ids);
            out->ids = grown;
            out->capacity = out->count + 1;
        }
        out->ids[out->count++] = id;
    }
    return 1;
}

void aoe_path_release_ror(AoePathfinderRor *path) {
    if (!path) return;
    free(path->nearby_ids);
    path->nearby_ids = NULL;
    path->nearby_count = path->nearby_capacity = 0;
    free(path->list_11dc64);
    path->list_11dc64 = NULL;
    path->list_11dc64_count = path->list_11dc64_capacity = 0;
}

/* 0x004586d0 */
AoePathfinderRor *aoe_path_delete_ror(AoePathfinderRor *path, uint8_t flags) {
    aoe_path_release_ror(path);
    if (flags & 1) free(path);
    return path;
}
