#include "player_tile_map.h"

#include <stdlib.h>

#include "aoe_stream.h"

void aoe_player_tile_map_free_ror(AoePlayerTileMapRor *m) {
    free(m->data);
    m->data = NULL;
    free(m->rows);
    m->rows = NULL;
    m->player = NULL;
    m->field_c = 0;
}

int aoe_player_tile_map_save_ror(const AoePlayerTileMapRor *m, int16_t player_id, float version,
                                 AoeOutcomeStream *s) {
    aoe_stream_put_i32(s, m->width);
    aoe_stream_put_i32(s, m->height);
    if (version >= 6.7f) aoe_stream_put_i32(s, m->explored);
    aoe_stream_put_u8(s, (uint8_t)player_id);
    aoe_stream_put_u8(s, (uint8_t)((uint16_t)player_id >> 8));
    if (m->width > 0 && m->height > 0) aoe_stream_put(s, m->data, (size_t)m->width * (size_t)m->height);
    return !s->failed;
}

float aoe_player_tile_map_explored_ror(const AoePlayerTileMapRor *m) {
    return (float)m->explored / (float)m->total;
}

void aoe_player_tile_map_reveal_ror(AoePlayerTileMapRor *m, AoeVisibilityMaskMap *masks,
                                    void (*explored)(void *context, int32_t x, int32_t y),
                                    void (*done)(void *context), void *context) {
    int32_t x, y;
    for (y = 0; y < m->height; ++y)
        for (x = 0; x < m->width; ++x) {
            uint8_t *t = &m->rows[y][x];
            if (*t != 0xff) continue;
            *t = 0;
            ++m->explored;
            masks->masks[(size_t)y * masks->width + (size_t)x] |= m->mask;
            explored(context, x, y);
        }
    done(context);
}

int aoe_player_object_memory_save_ror(const AoePlayerObjectMemory *m, AoeOutcomeStream *s) {
    size_t i, j;
    aoe_stream_put_i32(s, (int32_t)m->category_count);
    for (i = 0; i < m->category_count; ++i) {
        aoe_stream_put_i32(s, (int32_t)m->categories[i].capacity);
        aoe_stream_put_i32(s, (int32_t)m->categories[i].count);
    }
    for (i = 0; i < m->category_count; ++i)
        for (j = 0; j < m->categories[i].count; ++j) {
            const AoePlayerMemoryEntry *e = &m->categories[i].entries[j];
            aoe_stream_put_i32(s, e->unit_id);
            aoe_stream_put_u8(s, e->field_04);
            aoe_stream_put_u8(s, e->field_05);
            aoe_stream_put_u8(s, e->x);
            aoe_stream_put_u8(s, e->y);
        }
    return !s->failed;
}

int aoe_visibility_masks_save_ror(const AoeVisibilityMaskMap *m, AoeOutcomeStream *s) {
    size_t i, n;
    aoe_stream_put_i32(s, (int32_t)m->width);
    aoe_stream_put_i32(s, (int32_t)m->height);
    if ((int32_t)m->height > 0 && (int32_t)m->width > 0) {
        n = (size_t)m->width * m->height;
        for (i = 0; i < n; ++i) aoe_stream_put_i32(s, (int32_t)m->masks[i]);
    }
    return !s->failed;
}

void aoe_visibility_masks_bind_rows_ror(const AoeVisibilityMaskMap *m, uint32_t *rows[256]) {
    uint32_t y;
    for (y = 0; y < m->height && y < 256; ++y) rows[y] = m->masks + (size_t)y * m->width;
    for (; y < 256; ++y) rows[y] = NULL;
}
