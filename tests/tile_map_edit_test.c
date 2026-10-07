#include "tile_map_edit.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "player_pair_list.h"
#include "tile_map.h"

/* Patched RoR 1.0a 0x00447e40 against its disassembly:
   0x447e61..0x447e91 swap reversed ends (16-bit cmp si,dx / cx,di);
   0x447e95..0x447ed1 clamp x0 / y0 up to 0 and x1 / y1 down to map +0x08
   / +0x0c less 1; 0x447ed3 the apply byte; 0x447edf delta word 0 picks
   the set loop 0x447f89 (cmp byte 8, jbe, mov byte 8; shl cl,5) else the
   add loop 0x447f1b (movzx ax + delta, clamp 0..0xff, shl al,5); 0x448007
   grows the rectangle; 0x44808b shape 0x4484f0(x, y) -> tile +4; 0x4480a8
   vtable +0x34(+0x38, +0x3c, 0) for each node of tile +0x10; 0x4480ba
   world +0x1d != 3 -> 0x45eac0(x, y) on the viewed player's +0x6c;
   0x448136 0x444aa0(x0, y0, x1, y1); 0x448147 0x444980(.., 0). */

enum { W = 6, H = 5 };

typedef struct TObj {
    float x, y;
    int16_t tx, ty;
    struct TObj *next;
    int placed, order;
    float px, py, pz;
} TObj;

static AoeTileMapRor g_map;
static AoePlayerPairListRor g_list;
static int g_shape_calls, g_list_calls, g_place_seq;
static int16_t g_shape_x[64], g_shape_y[64];
static uint8_t g_state;
static TObj *g_objects;

static uint8_t t_shape(void *c, int16_t x, int16_t y) {
    (void)c;
    if (g_shape_calls < 64) {
        g_shape_x[g_shape_calls] = x;
        g_shape_y[g_shape_calls] = y;
    }
    ++g_shape_calls;
    return (uint8_t)(x == 2 && y == 1 ? 2 : 0);
}
static void *t_first(void *c, int16_t x, int16_t y) {
    TObj *o;
    (void)c;
    for (o = g_objects; o; o = o->next)
        if (o->tx == x && o->ty == y) return o;
    return NULL;
}
static void *t_next(void *c, void *node) {
    TObj *o = ((TObj *)node)->next;
    (void)c;
    while (o && (o->tx != ((TObj *)node)->tx || o->ty != ((TObj *)node)->ty)) o = o->next;
    return o;
}
static void *t_object(void *c, void *node) { (void)c; return node; }
static void t_position(void *c, void *object, float *x, float *y) {
    (void)c;
    *x = ((TObj *)object)->x;
    *y = ((TObj *)object)->y;
}
static void t_place(void *c, void *object, float x, float y, float z) {
    TObj *o = object;
    (void)c;
    ++o->placed;
    o->order = ++g_place_seq;
    o->px = x;
    o->py = y;
    o->pz = z;
}
static uint8_t t_state(void *c) { (void)c; return g_state; }
static AoePlayerPairListRor *t_list(void *c) { (void)c; ++g_list_calls; return &g_list; }

static const AoeTileMapEditOpsRor ops = {NULL, t_shape, t_first, t_next, t_object,
                                         t_position, t_place, t_state, t_list};

static AoeMapTileRor *T(int x, int y) { return &g_map.tiles[y * W + x]; }
static int E(int x, int y) { return aoe_tile_elevation_ror(T(x, y)); }
static void setE(int x, int y, int e) {
    T(x, y)->terrain_elevation = (uint8_t)((T(x, y)->terrain_elevation & 0x1f) | (e << 5));
}

static void reset(void) {
    int i;
    memset(g_map.tiles, 0, sizeof(AoeMapTileRor) * W * H);
    for (i = 0; i < W * H; ++i) {
        g_map.tiles[i].terrain_elevation = 0x07; /* terrain 7, elevation 0 */
        g_map.tiles[i].draw_flag = 9;
    }
    g_shape_calls = g_list_calls = g_place_seq = 0;
    g_state = 0;
    g_objects = NULL;
    aoe_pair_list_clear_ror(&g_list);
}

static int32_t touches(const int32_t **pairs) {
    int32_t n;
    aoe_pair_list_get_ror(&g_list, pairs, &n);
    return n;
}

int main(void) {
    const int32_t *p;
    int x, y;
    memset(&g_map, 0, sizeof g_map);
    g_map.width = W;
    g_map.height = H;
    g_map.tiles = calloc(W * H, sizeof *g_map.tiles);
    assert(g_map.tiles);
    aoe_tilemap_default_shapes_ror(&g_map); /* half 32 x 16, elevation 16 */
    assert(aoe_pair_list_init_ror(&g_list, 400));

    /* The old map load 0x444dc1: (0, 0, width - 1, height - 1, 0, 0, 0)
       leaves elevations, refreshes every tile row by row. */
    reset();
    setE(1, 1, 2);
    aoe_tilemap_edit_elevation_ror(&g_map, 0, 0, W - 1, H - 1, 0, 0, 0, &ops);
    assert(E(1, 1) == 2 && aoe_tile_terrain_ror(T(1, 1)) == 7 && E(0, 0) == 0);
    assert(g_shape_calls == W * H && g_list_calls == 1);
    assert(g_shape_x[0] == 0 && g_shape_y[0] == 0 && g_shape_x[1] == 1 && g_shape_y[1] == 0);
    assert(g_shape_x[W] == 0 && g_shape_y[W] == 1);
    assert(T(2, 1)->shape == 2 && T(0, 0)->shape == 0);
    assert(touches(&p) == W * H && p[0] == 0 && p[1] == 0 && p[2] == 1 && p[3] == 0);
    assert(p[2 * W] == 0 && p[2 * W + 1] == 1);
    for (y = 0; y < H; ++y)
        for (x = 0; x < W; ++x) assert(T(x, y)->dirty == 0xcc && T(x, y)->draw_flag == 0);
    /* 0x444aa0: x (x + y) * 32, y (y - x) * 16 less elevation * 16 (and
       one more step for shape 2). */
    assert(T(5, 4)->screen_x == 9 * 32 && T(5, 4)->screen_y == -16);
    assert(T(1, 1)->screen_x == 64 && T(1, 1)->screen_y == -32);
    assert(T(2, 1)->screen_y == -16 - 16);

    /* Set (delta 0) over a reversed rectangle: x 3..2, y 2..2 becomes
       x 2..3, y 2; the refresh covers x 1..4, y 1..3 only. */
    reset();
    aoe_tilemap_edit_elevation_ror(&g_map, 3, 2, 2, 2, 3, 0, 1, &ops);
    for (y = 0; y < H; ++y)
        for (x = 0; x < W; ++x) {
            int in = y == 2 && (x == 2 || x == 3);
            int grown = x >= 1 && x <= 4 && y >= 1 && y <= 3;
            assert(E(x, y) == (in ? 3 : 0) && aoe_tile_terrain_ror(T(x, y)) == 7);
            assert(T(x, y)->dirty == (grown ? 0xcc : 0));
            assert(T(x, y)->draw_flag == (grown ? 0 : 9));
        }
    assert(g_shape_calls == 12 && touches(&p) == 12);
    assert(p[0] == 1 && p[1] == 1 && p[22] == 4 && p[23] == 3);

    /* 0x447f89: an elevation above 8 is taken as 8, whose byte shift
       leaves the field 0; 8 itself also gives 0; 7 stays. */
    reset();
    setE(0, 0, 5); setE(1, 0, 5); setE(2, 0, 5);
    aoe_tilemap_edit_elevation_ror(&g_map, 0, 0, 0, 0, 9, 0, 1, &ops);
    aoe_tilemap_edit_elevation_ror(&g_map, 1, 0, 1, 0, 8, 0, 1, &ops);
    aoe_tilemap_edit_elevation_ror(&g_map, 2, 0, 2, 0, 7, 0, 1, &ops);
    assert(E(0, 0) == 0 && E(1, 0) == 0 && E(2, 0) == 7);
    assert(aoe_tile_terrain_ror(T(0, 0)) == 7);

    /* 0x447f1b: elevation + delta clamped to 0..0xff, low 3 bits kept. */
    reset();
    setE(0, 0, 7); setE(1, 0, 0); setE(2, 0, 3); setE(3, 0, 2);
    aoe_tilemap_edit_elevation_ror(&g_map, 0, 0, 3, 0, 0, 1, 1, &ops);
    assert(E(0, 0) == 0 && E(1, 0) == 1 && E(2, 0) == 4 && E(3, 0) == 3); /* 7 + 1 wraps */
    aoe_tilemap_edit_elevation_ror(&g_map, 0, 0, 3, 0, 0, -3, 1, &ops);
    assert(E(0, 0) == 0 && E(1, 0) == 0 && E(2, 0) == 1 && E(3, 0) == 0);
    aoe_tilemap_edit_elevation_ror(&g_map, 0, 0, 3, 0, 0, 300, 1, &ops);
    assert(E(0, 0) == 7 && E(1, 0) == 7 && E(2, 0) == 7 && E(3, 0) == 7); /* 0xff */
    aoe_tilemap_edit_elevation_ror(&g_map, 0, 0, 3, 0, 0, INT16_MIN, 1, &ops);
    assert(E(0, 0) == 0 && E(3, 0) == 0); /* 16-bit sum negative */
    assert(E(0, 1) == 0 && aoe_tile_terrain_ror(T(0, 0)) == 7);

    /* Apply 0: no edit, the refresh still runs. */
    reset();
    aoe_tilemap_edit_elevation_ror(&g_map, 0, 0, 1, 1, 5, 1, 0, &ops);
    assert(E(0, 0) == 0 && E(1, 1) == 0 && g_shape_calls == 9);

    /* Only the low 16 bits count; the low ends clamp to 0 and the high
       ends to the last column / row. */
    reset();
    aoe_tilemap_edit_elevation_ror(&g_map, -5, -5, 100, 0x10001, 2, 0, 1, &ops);
    for (x = 0; x < W; ++x) assert(E(x, 0) == 2 && E(x, 1) == 2 && E(x, 2) == 0);
    assert(g_shape_calls == W * 3);

    /* Objects of the grown rectangle placed again at their own position
       with z 0, in list order; world state 3 skips the touches but the
       viewed player is still fetched. */
    reset();
    {
        TObj c = {0.5f, 0.5f, 0, 0, NULL, 0, 0, 0, 0, 0};
        TObj b = {2.75f, 2.125f, 2, 2, &c, 0, 0, 0, 0, 0};
        TObj a = {2.25f, 2.5f, 2, 2, &b, 0, 0, 0, 0, 0};
        g_objects = &a;
        g_state = 3;
        aoe_tilemap_edit_elevation_ror(&g_map, 2, 2, 2, 2, 0, 0, 0, &ops);
        assert(a.placed == 1 && b.placed == 1 && c.placed == 0);
        assert(a.order == 1 && b.order == 2);
        assert(a.px == 2.25f && a.py == 2.5f && a.pz == 0.0f);
        assert(b.px == 2.75f && b.py == 2.125f && b.pz == 0.0f);
        assert(touches(&p) == 0 && g_list_calls == 1 && g_shape_calls == 9);
    }

    aoe_pair_list_destroy_ror(&g_list);
    free(g_map.tiles);
    return 0;
}
