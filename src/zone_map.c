#include "zone_map.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"

static int passable(const AoeZoneMapRor *m, const uint8_t *terrain,
                    int32_t x, int32_t y) {
    int idx = terrain[(size_t)y * m->width + x] & 0x1f;
    /* fld table[t]; fcomp 0.0; test ah,0x41: passable when > 0 (an entry
       beyond the table counts as impassable) */
    return idx < m->table_count && m->table[idx] > 0.0f;
}

/* 0x00521330: fill the region of (x, y) with `zone` through edge
   neighbours of the same passability, using an explicit stack
   (0x00520f20 push / 0x00520ed0 pop). */
static int fill(AoeZoneMapRor *m, const uint8_t *terrain, int32_t x,
                int32_t y, int pass, uint8_t zone) {
    size_t cap = 256, n = 0;
    int32_t *stack = malloc(cap * 2 * sizeof(*stack));
    if (!stack) return 0;
    for (;;) {
        static const int32_t d[4][2] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};
        for (int k = 0; k < 4; ++k) {
            int32_t nx = x + d[k][0], ny = y + d[k][1];
            uint8_t *z;
            if (nx < 0 || ny < 0 || nx >= m->width || ny >= m->height)
                continue;
            z = &m->zones[(size_t)ny * m->width + nx];
            if (*z != 0xff || passable(m, terrain, nx, ny) != pass) continue;
            *z = zone;
            if (n == cap) {
                int32_t *grown = realloc(stack, cap * 4 * sizeof(*stack));
                if (!grown) {
                    free(stack);
                    return 0;
                }
                stack = grown;
                cap *= 2;
            }
            stack[2 * n] = nx;
            stack[2 * n + 1] = ny;
            ++n;
        }
        if (!n) break;
        --n;
        x = stack[2 * n];
        y = stack[2 * n + 1];
    }
    free(stack);
    return 1;
}

int aoe_zone_map_build_ror(AoeZoneMapRor *m, int32_t width, int32_t height,
                           const uint8_t *terrain, const float *table,
                           int32_t table_count) {
    int32_t next = 0;
    if (!m || width <= 0 || height <= 0 || !terrain || table_count < 0 ||
        (table_count && !table))
        return 0;
    memset(m, 0, sizeof(*m));
    m->width = width;
    m->height = height;
    m->zones = malloc((size_t)width * height);
    m->table = table_count ? malloc((size_t)table_count * sizeof(float))
                           : NULL;
    if (!m->zones || (table_count && !m->table)) {
        aoe_zone_map_free_ror(m);
        return 0;
    }
    if (table_count) memcpy(m->table, table, (size_t)table_count * sizeof(float));
    m->table_count = table_count;
    memset(m->zones, 0xff, (size_t)width * height);
    for (int32_t y = 0; y < height; ++y)
        for (int32_t x = 0; x < width; ++x) {
            uint8_t *z = &m->zones[(size_t)y * width + x];
            if (*z != 0xff) continue;
            if (next >= 0xff) next = 0;
            *z = (uint8_t)next;
            if (!fill(m, terrain, x, y, passable(m, terrain, x, y),
                      (uint8_t)next)) {
                aoe_zone_map_free_ror(m);
                return 0;
            }
            ++next;
        }
    return 1;
}

void aoe_zone_map_free_ror(AoeZoneMapRor *m) {
    if (!m) return;
    free(m->zones);
    free(m->table);
    m->zones = NULL;
    m->table = NULL;
}

uint8_t aoe_zone_at_ror(const AoeZoneMapRor *m, int32_t x, int32_t y) {
    return m->zones[(size_t)y * m->width + x];
}

int aoe_zone_near_ror(const AoeZoneMapRor *m, uint8_t zone, int32_t x,
                      int32_t y, float distance) {
    int32_t r = aoe_ftol32(distance);
    int32_t x0 = x - r, y0 = y - r, x1 = x + r, y1 = y + r;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > m->width - 1) x1 = m->width - 1;
    if (y1 > m->height - 1) y1 = m->height - 1;
    for (int32_t i = x0; i <= x1; ++i)
        for (int32_t j = y0; j <= y1; ++j) {
            int32_t dx = x - i, dy = y - j;
            if (sqrt((double)(dx * dx + dy * dy)) <= (double)distance &&
                m->zones[(size_t)j * m->width + i] == zone)
                return 1;
        }
    return 0;
}

int32_t aoe_zone_size_ror(AoeZoneMapRor *m, uint8_t zone) {
    int32_t n = 0;
    if (zone == 0xff) return 0;
    if (m->counts[zone] > 0) return m->counts[zone];
    for (int32_t i = 0; i < m->width; ++i)
        for (int32_t j = 0; j < m->height; ++j)
            if (m->zones[(size_t)j * m->width + i] == zone) ++n;
    m->counts[zone] = n;
    return n;
}

int aoe_zone_table_matches_ror(const AoeZoneMapRor *m, const float *table,
                               int32_t count) {
    if (count != m->table_count) return 0;
    for (int32_t i = 0; i < count; ++i)
        if ((table[i] > 0.0f) != (m->table[i] > 0.0f)) return 0;
    return 1;
}

int32_t aoe_zone_manager_index_ror(AoeZoneManagerRor *z, int32_t width,
                                   int32_t height, const uint8_t *terrain,
                                   const float *table, int32_t count) {
    AoeZoneMapRor **grown, *m;
    if (!z) return -1;
    for (int32_t i = 0; i < z->count; ++i)
        if (aoe_zone_table_matches_ror(z->maps[i], table, count)) return i;
    grown = realloc(z->maps, (size_t)(z->count + 1) * sizeof(*grown));
    if (!grown) return -1;
    z->maps = grown;
    if (!(m = calloc(1, sizeof(*m)))) return -1;
    if (!aoe_zone_map_build_ror(m, width, height, terrain, table, count)) {
        free(m);
        return -1;
    }
    z->maps[z->count] = m;
    return z->count++;
}

AoeZoneMapRor *aoe_zone_manager_map_ror(AoeZoneManagerRor *z, int32_t i) {
    return z && i >= 0 && i < z->count ? z->maps[i] : NULL;
}

void aoe_zone_manager_free_ror(AoeZoneManagerRor *z) {
    if (!z) return;
    for (int32_t i = 0; i < z->count; ++i) {
        aoe_zone_map_free_ror(z->maps[i]);
        free(z->maps[i]);
    }
    free(z->maps);
    z->maps = NULL;
    z->count = 0;
}
