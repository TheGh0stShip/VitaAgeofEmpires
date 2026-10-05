/* Patched RoR 1.0a shoreline and cliff tools 0x004ed4b0..0x004eee70. */
#include "map_shore.h"

#include <assert.h>
#include <string.h>

#define W 12
#define H 12

typedef struct {
    uint8_t t[H][W], s[H][W];
    int paints, refreshes, touches, created, placed;
    int16_t cliff_type[H][W];
    uint8_t shape_of[H][W];
    int32_t rnd;
    uint8_t shape_set;
} Map;

static uint8_t terrain(void *c, int32_t x, int32_t y) { return ((Map *)c)->t[y][x]; }
static uint8_t shape(void *c, int32_t x, int32_t y) { return ((Map *)c)->s[y][x]; }
static void paint(void *c, int32_t x, int32_t y, uint8_t v) {
    Map *m = c;
    assert(x >= 0 && x < W && y >= 0 && y < H);
    m->t[y][x] = v;
    ++m->paints;
}
static void refresh(void *c, int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    (void)x0; (void)y0; (void)x1; (void)y1;
    ++((Map *)c)->refreshes;
}
static int object_at(void *c, int32_t x, int32_t y, int32_t i,
                     AoeCliffObjectRor *out) {
    Map *m = c;
    if (i > 0 || !m->cliff_type[y][x]) return 0;
    out->object = &m->cliff_type[y][x];
    out->type = m->cliff_type[y][x];
    out->shape = m->shape_of[y][x];
    out->x = (float)x;
    out->y = (float)y;
    return 1;
}
static int32_t rnd(void *c) { return ((Map *)c)->rnd; }
static void *create(void *c, int32_t type, float x, float y) {
    Map *m = c;
    ++m->created;
    m->cliff_type[(int)y][(int)x] = (int16_t)type;
    return &m->cliff_type[(int)y][(int)x];
}
static void set_shape(void *c, void *o, uint8_t s) {
    Map *m = c;
    m->shape_set = s;
    m->shape_of[0][((int16_t *)o - &m->cliff_type[0][0])] = s;
}
static void touch(void *c, int32_t x, int32_t y) { (void)x; (void)y; ++((Map *)c)->touches; }
static void placed(void *c, int32_t t, float x, float y) {
    (void)t; (void)x; (void)y;
    ++((Map *)c)->placed;
}
static void removeo(void *c, void *o) { (void)c; *(int16_t *)o = 0; }

int main(void) {
    static Map m;
    AoeShoreMapRor map = {W, H, &m, terrain, shape, paint, refresh};
    AoeShoreMapRor small = {8, 8, &m, terrain, shape, paint, refresh};
    static AoeCliffTableRor table;
    AoeCliffOpsRor ops = {&m, object_at, rnd, create, set_shape, touch, placed,
                          removeo, NULL, NULL};
    AoeCliffObjectRor found;
    int32_t key[4];

    assert(aoe_shore_is_water_ror(1) && aoe_shore_is_water_ror(22) &&
           aoe_shore_is_water_ror(4) && !aoe_shore_is_water_ror(2));

    /* Flatness looks at the eight neighbours only. */
    m.s[3][3] = 1;
    assert(aoe_shore_flat_ror(&map, 3, 3) && !aoe_shore_flat_ror(&map, 4, 4) &&
           aoe_shore_flat_ror(&map, 5, 5));

    /* Water only on flat tiles. */
    aoe_shore_paint_area_ror(&map, 4, 4, 1, 1);
    assert(m.t[4][4] == 0 && m.t[5][5] == 1 && m.t[3][5] == 1);
    m.t[2][2] = 1; /* water beside the slope */
    aoe_shore_dry_slopes_ror(&small, 0, 0, 7, 7);
    assert(m.t[2][2] == 0 && m.t[5][5] == 1);

    /* Beaches: land next to water becomes 2, a lone beach reverts. */
    memset(m.t, 0, sizeof(m.t));
    memset(m.s, 0, sizeof(m.s));
    m.t[4][4] = 1;
    m.t[0][7] = 2;
    aoe_shore_fix_beaches_ror(&small, 0, 0, 7, 7, 5);
    assert(m.t[3][3] == 2 && m.t[5][4] == 2 && m.t[0][7] == 5 &&
           m.t[2][2] == 0 && m.t[4][4] == 1);

    /* Smoothing: land pinched between water (W and E) gets flooded when
       painting water. */
    memset(m.t, 0, sizeof(m.t));
    m.t[3][2] = 1;
    m.t[3][4] = 1;
    m.refreshes = 0;
    aoe_shore_smooth_ror(&map, 3, 3, 3, 3, 1);
    assert(m.t[3][3] == 1 && m.refreshes == 1);
    /* Painting land instead dries the water neighbours. */
    memset(m.t, 0, sizeof(m.t));
    m.t[3][2] = 1;
    m.t[3][4] = 1;
    aoe_shore_smooth_ror(&map, 3, 3, 3, 3, 0);
    assert(m.t[3][2] != 1 && m.t[3][4] != 1);

    /* Cliff table. */
    aoe_cliff_table_init_ror(&table);
    assert(table.last_x == -1 && aoe_cliff_rule_exists_ror(&table, 1, 0, 0, 0) &&
           !aoe_cliff_rule_exists_ror(&table, 1, 1, 1, 1) &&
           !aoe_cliff_rule_ror(&table, 3, 0, 0, 0));
    assert(aoe_cliff_rule_ror(&table, -1, 1, 0, 0)->type == 0x10e &&
           aoe_cliff_rule_ror(&table, -1, 1, 0, 0)->offset_y == 2.0f);
    aoe_cliff_key_of_ror(&table, 1, 0x109, 7, key);
    assert(key[0] == 0 && key[1] == -1 && key[2] == 0 && key[3] == -1);
    aoe_cliff_key_of_ror(&table, 0, 0x109, 7, key);
    assert(!key[1]);
    assert(aoe_cliff_opposite_ror(0) == 2 && aoe_cliff_opposite_ror(3) == 1);

    /* Place with the alternative piece, then find it. */
    m.rnd = 0x5000;
    assert(aoe_cliff_place_ror(&table, &ops, 1, 1, 1, 0, 1, 0));
    assert(m.cliff_type[4][4] == 0x108 && m.shape_set == 5 &&
           m.touches == 9 && m.placed == 1);
    m.rnd = 0;
    aoe_cliff_place_ror(&table, &ops, 1, 1, 1, 0, 1, 0);
    assert(m.shape_set == 4);
    assert(!aoe_cliff_place_ror(&table, &ops, 0, 0, 1, 1, 1, 1));
    assert(aoe_cliff_at_ror(&map, &ops, 1, 1, &found) && found.type == 0x108);
    assert(!aoe_cliff_at_ror(&map, &ops, 0, 0, &found));
    assert(!aoe_cliff_at_ror(&map, &ops, 4, 0, &found)); /* off the map */

    /* Drag a cliff from cell (1,1) to (2,1): the first press starts the
       line, the move joins both cells; a move inside the row snaps. */
    memset(m.cliff_type, 0, sizeof(m.cliff_type));
    aoe_cliff_table_init_ror(&table);
    assert(aoe_cliff_drag_ror(&table, &map, &ops, 4, 4, 0, 0) == 1);
    assert(table.last_x == 1 && table.last_y == 1 && table.last_side == -1);
    assert(aoe_cliff_drag_ror(&table, &map, &ops, 4, 5, 0, 0) == 0);
    assert(aoe_cliff_drag_ror(&table, &map, &ops, 7, 5, 0, 0) == 1);
    assert(table.last_x == 2 && table.last_y == 1 && table.last_side == 2);
    assert(aoe_cliff_at_ror(&map, &ops, 1, 1, &found) && found.type == 0x108);
    assert(aoe_cliff_at_ror(&map, &ops, 2, 1, &found) && found.type == 0x108);
    aoe_cliff_press_ror(&table, &ops, 0, 0, 0, 0, 0, 0);
    assert(table.last_x == -1 && table.cell_x == -1);
    return 0;
}
