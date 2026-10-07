#include "world_base_map.h"

#include <stddef.h>

/* 0x00443d80 */
AoeBaseMapRor *aoe_base_map_init_ror(AoeBaseMapRor *m, const char *border_name,
                                     const char *terrain_name, const char *unused_name,
                                     int16_t tile_w, int16_t tile_h, int16_t elevation_h,
                                     void **sounds, const AoeBaseMapOpsRor *ops) {
    /* The 19 shapes after shape 0: the height (0 the tile, 1 the
       elevation step, 2 both, added in 16 bits), the offset and whether
       the width is the half width. */
    static const struct {
        uint8_t height;
        int8_t offset; /* -1 for -100, 1 for 100 */
        uint8_t half;
    } k[19] = {{0, 0, 0},  {1, -1, 0}, {2, 1, 0},  {0, -1, 0}, {0, -1, 0},
               {1, -1, 0}, {2, 1, 0},  {1, -1, 0}, {2, 1, 0},  {1, -1, 0},
               {2, 1, 0},  {0, -1, 0}, {0, -1, 0}, {1, -1, 0}, {2, 1, 0},
               {0, 1, 0},  {0, 1, 0},  {0, 0, 1},  {0, 0, 1}};
    AoeTileMapRor *t = &m->tile;
    int i;
    (void)unused_name; /* the third argument is never read */

    m->zones = NULL;      /* +0x8dc4 */
    m->random_map = NULL; /* +0x8dbc */
    m->world = ops->game_world(ops->context);

    t->tile_w = tile_w;
    t->half_w = (int16_t)(tile_w / 2);
    t->tile_h = tile_h;
    t->half_h = (int16_t)(tile_h / 2);
    t->elevation_h = elevation_h;
    t->width = 0;
    t->height = 0;
    m->pixel_w = 0;
    m->pixel_h = 0;

    /* Shape 0 takes the arguments, the rest the stored geometry. */
    aoe_tilemap_set_shape_ror(t, 0, tile_w, tile_h, 0);
    for (i = 1; i < 19; ++i) {
        int16_t h = k[i].height == 0 ? t->tile_h
                    : k[i].height == 1 ? t->elevation_h
                                       : (int16_t)(t->elevation_h + t->tile_h);
        aoe_tilemap_set_shape_ror(t, (uint8_t)i, k[i].half ? t->half_w : t->tile_w, h,
                                  (int16_t)(k[i].offset * 100));
    }

    m->field_8da0 = 0;
    m->field_8da2 = 0;
    m->terrain_count = 0;
    m->field_8d94 = 0;
    m->field_8da4 = 0;
    m->field_8da6 = 0;
    m->field_8da8 = 0;
    m->field_8daa = 0;
    m->field_8dac = 0;
    m->reveal_all = 0;
    m->fog_edges = 1;

    for (i = 0; i < 32; ++i) {
        AoeBaseMapTerrainRor *r = &m->terrains[i];
        int j;
        r->object_count = 0;
        r->field_ba = 0;
        r->field_bc = 0;
        for (j = 0; j < 30; ++j) {
            r->object_ids[j] = 0;
            r->object_density[j] = 0;
        }
        for (j = 0; j < 32; ++j) r->table_be[j] = 0;
        r->shape = NULL;
        r->sound = NULL;
    }
    for (i = 15; i >= 0; --i) {
        m->borders[i].shape = NULL;
        m->borders[i].sound = NULL;
    }

    ops->terrain_text(ops->context, m, terrain_name, sounds);
    ops->border_text(ops->context, m, border_name, sounds);

    t->tiles = NULL; /* +0x04 and +0x8d8c */
    m->raw8db0 = NULL;
    m->raw8db4 = NULL;
    m->masks = NULL;
    m->v8dcc = NULL;
    return m;
}
