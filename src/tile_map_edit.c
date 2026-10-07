#include "tile_map_edit.h"

#include <stddef.h>

static AoeMapTileRor *tile_at(const AoeTileMapRor *m, int32_t x, int32_t y) {
    return &m->tiles[(size_t)y * (size_t)m->width + (size_t)x];
}

/* 0x00447e40 */
void aoe_tilemap_edit_elevation_ror(AoeTileMapRor *m, int32_t x0_in, int32_t y0_in, int32_t x1_in,
                                    int32_t y1_in, uint8_t elevation, int16_t delta, uint8_t apply,
                                    const AoeTileMapEditOpsRor *ops) {
    int16_t x0 = (int16_t)x0_in, y0 = (int16_t)y0_in;
    int16_t x1 = (int16_t)x1_in, y1 = (int16_t)y1_in, t;
    int32_t x, y;
    void *ctx = ops ? ops->context : NULL;
    AoePlayerPairListRor *viewed;
    if (!m || !m->tiles || m->width <= 0 || m->height <= 0) return;
    /* 0x447e4d: the viewed player is fetched before anything else. */
    viewed = ops && ops->viewed_list ? ops->viewed_list(ctx) : NULL;

    /* 0x447e5d..0x447ed1: 16-bit compares throughout. */
    if (y0 > y1) {
        t = y0; y0 = y1; y1 = t;
    }
    if (x0 > x1) {
        t = x0; x0 = x1; x1 = t;
    }
    if (y0 < 0) y0 = 0;
    if ((int32_t)y1 >= m->height) y1 = (int16_t)(m->height - 1);
    if (x0 < 0) x0 = 0;
    if ((int32_t)x1 >= m->width) x1 = (int16_t)(m->width - 1);

    if (apply) {
        if (delta == 0) {
            /* 0x447f89: above 8 becomes 8; the << 5 is a byte shift. */
            uint8_t bits;
            if (elevation > 8) elevation = 8;
            bits = (uint8_t)(elevation << 5);
            for (y = y0; y <= y1; ++y)
                for (x = x0; x <= x1; ++x) {
                    AoeMapTileRor *tile = tile_at(m, x, y);
                    tile->terrain_elevation = (uint8_t)((tile->terrain_elevation & 0x1f) | bits);
                }
        } else {
            /* 0x447f1b: elevation + delta in 16 bits, clamped to 0..0xff. */
            for (y = y0; y <= y1; ++y)
                for (x = x0; x <= x1; ++x) {
                    AoeMapTileRor *tile = tile_at(m, x, y);
                    int16_t e = (int16_t)(uint16_t)((uint16_t)(tile->terrain_elevation >> 5) +
                                                    (uint16_t)delta);
                    uint8_t b;
                    if (e < 0)
                        e = 0;
                    else if (e > 0xff)
                        e = 0xff;
                    b = (uint8_t)((tile->terrain_elevation & 0x1f) | (uint8_t)(e << 5));
                    tile->terrain_elevation = b;
                    /* 0x447f5a: never true for a 3-bit field. */
                    if ((uint8_t)(b >> 5) > 8) tile->terrain_elevation = (uint8_t)(b & 0x1f);
                }
        }
    }

    /* 0x448007: one more tile on each side not at the edge. */
    if (y0 > 0) --y0;
    if ((int32_t)y1 < m->height - 1) ++y1;
    if (x0 > 0) --x0;
    if ((int32_t)x1 < m->width - 1) ++x1;

    for (y = y0; y <= y1; ++y)
        for (x = x0; x <= x1; ++x) {
            AoeMapTileRor *tile = tile_at(m, x, y);
            void *node;
            if (ops && ops->shape) tile->shape = ops->shape(ctx, (int16_t)x, (int16_t)y);
            /* 0x448093: each object placed again (vtable +0x34) at its
               +0x38 / +0x3c with z 0; the next link is read afterwards. */
            node = ops && ops->first_node ? ops->first_node(ctx, (int16_t)x, (int16_t)y) : NULL;
            while (node) {
                void *object = ops->node_object ? ops->node_object(ctx, node) : NULL;
                if (object && ops->position && ops->place) {
                    float ox = 0.0f, oy = 0.0f;
                    ops->position(ctx, object, &ox, &oy);
                    ops->place(ctx, object, ox, oy, 0.0f);
                }
                node = ops->next_node ? ops->next_node(ctx, node) : NULL;
            }
            /* 0x4480b4: world +0x1d re-read per tile. */
            if (!(ops && ops->world_state && ops->world_state(ctx) == 3) && viewed)
                aoe_pair_list_append_ror(viewed, x, y);
        }

    aoe_tilemap_positions_ror(m, x0, y0, x1, y1);
    aoe_tilemap_invalidate_ror(m, x0, y0, x1, y1, 0);
}
