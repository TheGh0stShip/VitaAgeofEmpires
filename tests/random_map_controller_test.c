/* Patched RoR 1.0a random-map controller 0x0046d580..0x0046ede0 and the
   RoR wrappers 0x00471cf0..0x00472c20. */
#include "random_map.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int allies[9][9];
static uint8_t stance(void *c, int32_t p, int32_t o) { (void)c; return allies[p][o] ? 0 : 3; }
static int32_t mode(void *c) { (void)c; return 0; }

static const char *header = "1 7";
static const char *lands =
    "1 0\n"
    /* count margin*4 f14 edge f1c percent */
    "2 4 4 4 4 3 6 1 30\n"
    /* id terr f08 rad player place fx fy pct per_player zr zw */
    "  10 1 0 2 9 1 20 22 25 0 -1 0\n"
    "  11 0 0 3 1 2 0 0 40 1 5 2\n";
static const char *terrains = "1 0 2\n 50 2 3 0 0\n 25 6 1 0 0\n";
static const char *objects = "1 0 1\n 109 -1 1 0 1 0 0 0 0 -1 0 0\n";

int main(void) {
    static AoeRmControllerRor c, d;
    static uint8_t tiles[64 * 64];
    AoeRng rng;
    AoeRmGameOpsRor game = {NULL, stance, mode};
    AoeRmMapRor map = {64, 64, tiles, NULL, NULL, NULL};
    static uint8_t buf[4096];
    AoeOutcomeStream out = {buf, sizeof buf, 0, 0}, in;
    int32_t i;

    aoe_rng_seed(&rng, 12345);
    assert(aoe_rm_controller_load_text_ror(&c, lands, terrains, objects, header, 0, &rng, &game));
    assert(c.map_count == 1 && c.maps[0].id == 7 && c.maps[0].land_count == 2);
    assert(c.maps[0].lands[1].per_player == 1 && c.maps[0].lands[1].f28 == 8);
    assert(c.maps[0].edge == 6 && c.maps[0].land_percent == 30);
    assert(c.maps[0].terrains[1].terrain == 6 && c.maps[0].terrains[1].f14 == 20);
    assert(c.maps[0].objects[0].unit == 109 && c.maps[0].objects[0].land == -1);

    /* DAT round trip keeps every byte. */
    assert(aoe_rm_controller_save_ror(&c, &out));
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    assert(aoe_rm_controller_load_ror(&d, &in, 1, &rng, &game) && in.pos == out.pos);
    {
        static uint8_t buf2[4096];
        AoeOutcomeStream o2 = {buf2, sizeof buf2, 0, 0};
        assert(aoe_rm_controller_save_ror(&d, &o2) && o2.pos == out.pos &&
               !memcmp(buf, buf2, out.pos));
    }
    aoe_rm_controller_release_ror(&d);
    {
        AoeRmControllerRor *n, *t;
        in = (AoeOutcomeStream){buf, out.pos, 0, 0};
        n = aoe_rm_controller_new_ror(&in, 1, &rng, &game);
        assert(n && n->map_count == 1 && n->maps[0].id == 7);
        aoe_rm_controller_release_ror(n);
        free(n);
        t = aoe_rm_controller_new_text_ror(header, lands, terrains, objects, 0, &rng, &game);
        assert(t && t->map_count == 1 && t->maps[0].land_count == 2);
        aoe_rm_controller_release_ror(t);
        free(t);
    }

    /* Setup on a 64x64 map with 4 players (base controller). */
    assert(aoe_rm_controller_setup_ror(&c, &map, 0, 0, 4, 0));
    assert(c.land_count == 5);
    /* Land 0 is fixed at (20, 22); its area is 25% of 30% of the map. */
    assert(c.lands[0].x == 20 && c.lands[0].y == 22 && c.lands[0].id == 9);
    assert(c.lands[0].area == 25 * (30 * 64 * 64 / 100) / 100);
    /* Player lands: spaced, inside the edge, outside the central box. */
    for (i = 1; i < 5; ++i) {
        AoeRmLandWorkRor *l = &c.lands[i];
        assert(l->x >= 6 && l->x < 58 && l->y >= 6 && l->y < 58);
        assert(l->id == 1 && l->area == 40 * (30 * 64 * 64 / 100) / 400);
        assert(l->bounds[0] == 4 && l->bounds[2] == 60);
    }
    /* Terrain tiles: percent of the land area; zones only for land 1+. */
    /* 50 * 1228 * 0.01f = 613.99998: the float constant loses a tile. */
    assert(c.terrain_count == 2 && c.terrains[0].tiles == 613);
    assert(c.terrains[0].flag == 1 && c.terrains[1].clumps == 1);
    assert(c.terrain_zone_count == 4 && c.terrain_zones[0].radius == 5 &&
           c.terrain_zones[0].x == c.lands[0].x); /* index quirk: slot 0 */
    /* Objects: one record; object lands follow the land records. */
    assert(c.object_count == 1 && c.objects[0].flag == 1 && c.object_land_count == 5);
    assert(c.object_lands[0].id == 10 && c.object_lands[0].player == 0);
    assert(c.object_lands[1].id == 11 && c.object_lands[1].player == 1 &&
           c.object_lands[4].player == 4);
    /* Elevation: four fixed layers. */
    assert(c.layer_count == 4 && c.layers[0].tiles == (30 * 64 * 64 / 100) * 10 / 100 &&
           c.layers[3].height == 4);
    aoe_rm_controller_cleanup_ror(&c);
    assert(!c.base.grid && c.map_index == -1);

    /* Same seed, same lands. */
    {
        AoeRng r1, r2;
        int32_t xs[5], ys[5];
        aoe_rng_seed(&r1, 99);
        c.base.rng = &r1;
        assert(aoe_rm_controller_setup_ror(&c, &map, 0, 0, 4, 0));
        for (i = 0; i < 5; ++i) { xs[i] = c.lands[i].x; ys[i] = c.lands[i].y; }
        aoe_rm_controller_cleanup_ror(&c);
        aoe_rng_seed(&r2, 99);
        c.base.rng = &r2;
        assert(aoe_rm_controller_setup_ror(&c, &map, 0, 0, 4, 0));
        for (i = 0; i < 5; ++i) assert(xs[i] == c.lands[i].x && ys[i] == c.lands[i].y);
        assert(r1.state == r2.state);
        aoe_rm_controller_cleanup_ror(&c);
    }

    /* Teams: players 1+2 and 3+4 allied; the regrouping terminates and
       keeps the same set of positions. */
    allies[1][1] = allies[1][2] = allies[2][1] = allies[2][2] = 1;
    allies[3][3] = allies[3][4] = allies[4][3] = allies[4][4] = 1;
    c.maps[0].lands[0].per_player = 1; /* only player lands */
    c.maps[0].lands[0].placement = 2;
    c.maps[0].land_count = 1;
    c.base.rng = &rng;
    assert(aoe_rm_controller_setup_ror(&c, &map, 0, 0, 4, 1));
    assert(c.land_count == 4);
    for (i = 0; i < 4; ++i)
        assert(c.lands[i].x >= 6 && c.lands[i].x < 58);
    aoe_rm_controller_cleanup_ror(&c);

    /* RoR wrappers run and stay deterministic. */
    c.ror = 1;
    c.maps[0].lands[0].per_player = 0;
    c.maps[0].land_count = 2;
    assert(aoe_rm_controller_setup_ror(&c, &map, 0, 0, 4, 0));
    /* Map index 0 adds 1..4 lake lands with objects 0x66 / 0x42. */
    assert(c.land_count > 5 && c.object_count > 1);
    assert(c.objects[1].rec.unit == 0x66 || c.objects[1].rec.unit == 0x42);
    assert(c.objects[1].rec.f28 == 0x1a && c.objects[1].flag == 12);
    assert(c.layers[0].tiles > 0);
    aoe_rm_controller_cleanup_ror(&c);

    aoe_rm_controller_release_ror(&c);
    return 0;
}
