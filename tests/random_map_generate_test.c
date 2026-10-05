/* Patched RoR 1.0a random-map generation: modules 0x0046cdb0..0x00473d70
   and generate 0x0046de20 / 0x00472d80. */
#include "random_map.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

enum { W = 48, H = 48 };
static uint8_t tiles[W * H];
static int creates, created_unit[512];
static float created_x[512], created_y[512];

static void set_terrain(void *c, int32_t a, int32_t b, int32_t x0, int32_t y0,
                        int32_t x1, int32_t y1, uint8_t t, int32_t cc, int32_t d) {
    int32_t x, y;
    (void)c; (void)a; (void)b; (void)cc; (void)d;
    for (y = y0; y <= y1; ++y)
        for (x = x0; x <= x1; ++x)
            tiles[y * W + x] = (uint8_t)((tiles[y * W + x] & 0xe0) | (t & 0x1f));
}
static void *zones(void *c, const float *t, int32_t n) { (void)c; (void)t; (void)n; return tiles; }
static uint8_t zone_at(void *c, void *z, int32_t x, int32_t y) { (void)c; (void)z; return (tiles[y * W + x] & 0x1f) == 1 ? 1 : 0; }
static uint8_t alias(void *c, uint8_t t) { (void)c; return t; }

static int unit_size(void *c, int32_t p, int32_t u, float *sx, float *sy) {
    (void)c; (void)p; (void)u; *sx = 0.5f; *sy = 1.0f; return 1;
}
static int blocked(void *c, int32_t p, int32_t u, float x, float y, int32_t f) {
    (void)c; (void)p; (void)u; (void)f;
    return (tiles[(int)y * W + (int)x] & 0x1f) == 1; /* not on water */
}
static void create(void *c, int32_t p, int32_t u, float x, float y) {
    (void)c; (void)p;
    if (creates < 512) { created_unit[creates] = u; created_x[creates] = x; created_y[creates] = y; }
    ++creates;
}
static void *unit_zones(void *c, int32_t p, int32_t u) { (void)c; (void)p; (void)u; return tiles; }
static uint8_t stance(void *c, int32_t p, int32_t o) { (void)c; return p == o ? 0 : 3; }
static int32_t mode(void *c) { (void)c; return 0; }

static const char *header = "1 0";
static const char *lands =
    "1 0\n 2 3 3 3 3 2 6 1 70\n"
    /* +0x08 is the reach the growth scores within: 0 never grows. */
    "  0 0 2 2 1 2 0 0 50 1 6 3\n"
    "  1 1 2 3 9 1 24 24 30 0 -1 0\n";
static const char *terrains = "1 0 1\n 20 6 3 0 0\n";
static const char *objects =
    "1 0 2\n"
    " 109 -1 0 0 1 0 1 0 0 0 0 4\n"   /* one per player land, near it */
    " 59 0 1 0 3 1 2 2 0 -1 0 0\n";   /* clusters of 3 anywhere */

static unsigned hash_tiles(void) {
    unsigned h = 2166136261u;
    int i;
    for (i = 0; i < W * H; ++i) h = (h ^ tiles[i]) * 16777619u;
    return h;
}

static unsigned run(uint32_t seed, int ror) {
    static AoeRmControllerRor c;
    AoeRng rng;
    AoeRmMapRor map = {W, H, tiles, NULL, NULL, NULL, set_terrain, NULL, NULL,
                       alias, zones, zone_at};
    AoeRmGameOpsRor game = {NULL, stance, mode};
    AoeRmWorldOpsRor world = {NULL, unit_size, blocked, create, NULL, unit_zones,
                              zone_at, NULL, NULL};
    memset(tiles, 0, sizeof tiles);
    creates = 0;
    aoe_rng_seed(&rng, seed);
    assert(aoe_rm_controller_load_text_ror(&c, lands, terrains, objects, header,
                                           ror, &rng, &game));
    c.world = &world;
    if (ror) assert(aoe_rm_ror_setup_ror(&c, &map, 0, 0, 2, 0));
    else assert(aoe_rm_controller_setup_ror(&c, &map, 0, 0, 2, 0));
    assert(aoe_rm_generate_ror(&c));
    aoe_rm_controller_cleanup_ror(&c);
    aoe_rm_controller_release_ror(&c);
    return hash_tiles() ^ (unsigned)creates * 0x9e3779b9u ^ rng.state;
}

int main(void) {
    unsigned a = run(7, 0), b = run(7, 0), c2;
    int i, water = 0, forest = 0, tcs = 0;
    assert(a == b); /* deterministic */
    /* The fixed land 1 is water around (24, 24). */
    assert((tiles[24 * W + 24] & 0x1f) == 1);
    for (i = 0; i < W * H; ++i) {
        water += (tiles[i] & 0x1f) == 1;
        forest += (tiles[i] & 0x1f) == 6;
    }
    assert(water > 0 && forest > 0);
    /* Two units 109 near the player lands, never on water. */
    for (i = 0; i < creates && i < 512; ++i) {
        if (created_unit[i] == 109) ++tcs;
        assert((tiles[(int)created_y[i] * W + (int)created_x[i]] & 0x1f) != 1);
    }
    assert(tcs == 2 && creates > 2);
    /* The 0.5 size puts units on tile centres in x only. */
    for (i = 0; i < creates && i < 512; ++i)
        assert(created_x[i] - (float)(int)created_x[i] == 0.5f &&
               created_y[i] == (float)(int)created_y[i]);
    c2 = run(8, 0);
    assert(c2 != a);
    /* The RoR controller and modules also run deterministically. */
    assert(run(9, 1) == run(9, 1));
    for (i = 0; i < W * H; ++i) assert((tiles[i] & 0x1f) != 0x10);
    return 0;
}
