#include "world_base_map.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

/* Executable evidence: patched RoR 1.0a 0x00443d80 (EMPIRESX.EXE).
   - 0x443d8e / 0x443d94: +0x8dc4 and +0x8dbc cleared; 0x443d9a..0x443dab:
     +0x8dc0 = [0x580e38] +0x3f4.
   - 0x443db5..0x443dc4: movsx, cdq, sub, sar 1 (half width toward zero)
     into +0x8d9c; 0x443dcc..0x443de3 the half height into +0x8d9a.
   - 0x443e10 push 0xffffff9c, 0x443e32 push 0x64: shape offsets -100 /
     100 (0x00444240 uses -8 / 8); 0x443e2b add dx, [+0x8d98]: tile plus
     elevation in 16 bits; 0x443fdc / 0x443ff6: shapes 17 and 18 take the
     half width (+0x8d9c), offset ebx (0).
   - 0x444047 byte +0x8db8 = 0, 0x44404d byte +0x8db9 = 1.
   - 0x4440ca call 0x00446bb0 with [esp+0x18] (second argument), then
     0x4440d7 call 0x00446f80 with [esp+0x14] (first); both with
     [esp+0x2c] (seventh). The third argument ([esp+0x1c]) is never read.
   - 0x4440dc..0x4440f7: +0x04, +0x8db0, +0x8d8c, +0x8db4, +0x8dc8 and
     +0x8dcc cleared after both loaders. */

static int world_obj, dummy, sound_a, sound_b;
static int calls;
static char order[4];
static const char *seen_terrain, *seen_border;
static void **seen_sounds;

static void *t_world(void *c) {
    (void)c;
    return &world_obj;
}

static void t_terrain(void *c, AoeBaseMapRor *m, const char *name, void **sounds) {
    int i, j;
    (void)c;
    order[calls++] = 'T';
    seen_terrain = name;
    seen_sounds = sounds;
    /* The state the loader sees: cleared counters and records, the
       geometry set, the tile block not yet cleared. */
    assert(m->terrain_count == 0 && m->reveal_all == 0 && m->fog_edges == 1);
    assert(m->world == &world_obj && m->zones == NULL && m->random_map == NULL);
    assert(m->tile.shapes[2].height == 48 && m->tile.half_w == 32);
    assert(m->tile.tiles == (AoeMapTileRor *)&dummy && m->masks == &dummy);
    for (i = 0; i < 32; ++i) {
        assert(m->terrains[i].shape == NULL && m->terrains[i].sound == NULL);
        assert(m->terrains[i].object_count == 0 && m->terrains[i].field_ba == 0);
        assert(m->terrains[i].field_bc == 0);
        for (j = 0; j < 30; ++j)
            assert(m->terrains[i].object_ids[j] == 0 && m->terrains[i].object_density[j] == 0);
        for (j = 0; j < 32; ++j) assert(m->terrains[i].table_be[j] == 0);
    }
    for (i = 0; i < 16; ++i) assert(m->borders[i].shape == NULL && m->borders[i].sound == NULL);
    /* What 0x00446bb0 fills: the count and a terrain's sound. */
    m->terrain_count = 3;
    m->terrains[2].sound = sounds[1];
    m->terrains[2].object_count = 4;
}

static void t_border(void *c, AoeBaseMapRor *m, const char *name, void **sounds) {
    (void)c;
    order[calls++] = 'B';
    seen_border = name;
    assert(sounds == seen_sounds);
    assert(m->terrain_count == 3 && m->terrains[2].sound == &sound_b);
    m->borders[4].sound = sounds[0];
    /* Set here to show the constructor clears these after the loaders. */
    m->raw8db0 = &dummy;
    m->v8dcc = &dummy;
}

static int quiet_calls;
static void q_loader(void *c, AoeBaseMapRor *m, const char *name, void **sounds) {
    (void)c; (void)m; (void)name; (void)sounds;
    ++quiet_calls;
}

static void fill(AoeBaseMapRor *m) {
    memset(m, 0xa5, sizeof *m); /* operator new memory is not cleared */
    m->tile.tiles = (AoeMapTileRor *)&dummy;
    m->raw8db0 = m->raw8db4 = m->masks = m->v8dcc = &dummy;
    m->zones = m->random_map = m->world = &dummy;
}

int main(void) {
    static AoeBaseMapRor m;
    void *sounds[2] = {&sound_a, &sound_b};
    AoeBaseMapOpsRor ops = {NULL, t_world, t_terrain, t_border};
    const char *border = "tileedge", *terrain = "terrain", *unused = "overlay";
    int i;

    fill(&m);
    assert(aoe_base_map_init_ror(&m, border, terrain, unused, 64, 32, 16, sounds, &ops) == &m);
    /* Terrain list (second argument) first, then the border list (first). */
    assert(calls == 2 && order[0] == 'T' && order[1] == 'B');
    assert(seen_terrain == terrain && seen_border == border && seen_sounds == sounds);

    /* Geometry. */
    assert(m.tile.tile_w == 64 && m.tile.tile_h == 32 && m.tile.elevation_h == 16);
    assert(m.tile.half_w == 32 && m.tile.half_h == 16);
    assert(m.tile.width == 0 && m.tile.height == 0 && m.pixel_w == 0 && m.pixel_h == 0);

    /* The shapes: offsets -100 / 100, not 0x444240's -8 / 8. */
    {
        static const int16_t want[19][3] = {
            {64, 32, 0},    {64, 16, -100}, {64, 48, 100},  {64, 32, -100}, {64, 32, -100},
            {64, 16, -100}, {64, 48, 100},  {64, 16, -100}, {64, 48, 100},  {64, 16, -100},
            {64, 48, 100},  {64, 32, -100}, {64, 32, -100}, {64, 16, -100}, {64, 48, 100},
            {64, 32, 100},  {64, 32, 100},  {32, 32, 0},    {32, 32, 0}};
        for (i = 0; i < 19; ++i) {
            assert(m.tile.shapes[i].width == want[i][0]);
            assert(m.tile.shapes[i].height == want[i][1]);
            assert(m.tile.shapes[i].offset == want[i][2]);
        }
    }

    /* Counters and flags. */
    assert(m.field_8da0 == 0 && m.field_8da2 == 0 && m.field_8d94 == 0);
    assert(m.field_8da4 == 0 && m.field_8da6 == 0 && m.field_8da8 == 0 && m.field_8daa == 0);
    assert(m.field_8dac == 0 && m.reveal_all == 0 && m.fog_edges == 1);

    /* What the loaders filled survives; the late fields are cleared. */
    assert(m.terrain_count == 3 && m.terrains[2].sound == &sound_b);
    assert(m.terrains[2].object_count == 4 && m.borders[4].sound == &sound_a);
    assert(m.terrains[3].sound == NULL && m.borders[3].sound == NULL);
    assert(m.tile.tiles == NULL && m.raw8db0 == NULL && m.raw8db4 == NULL);
    assert(m.masks == NULL && m.v8dcc == NULL);
    assert(m.world == &world_obj && m.zones == NULL && m.random_map == NULL);

    /* Odd and negative sizes: halves truncate toward zero (cdq; sub; sar),
       shape 2 is tile plus elevation, 17 and 18 the half width. */
    fill(&m);
    ops.terrain_text = q_loader;
    ops.border_text = q_loader;
    quiet_calls = 0;
    assert(aoe_base_map_init_ror(&m, border, terrain, unused, -7, 33, 20, NULL, &ops) == &m);
    assert(quiet_calls == 2);
    assert(m.tile.half_w == -3 && m.tile.half_h == 16);
    assert(m.tile.shapes[0].width == -7 && m.tile.shapes[0].height == 33);
    assert(m.tile.shapes[1].height == 20 && m.tile.shapes[2].height == 53);
    assert(m.tile.shapes[3].height == 33 && m.tile.shapes[15].offset == 100);
    assert(m.tile.shapes[17].width == -3 && m.tile.shapes[18].height == 33);
    assert(m.tile.shapes[18].offset == 0 && m.terrain_count == 0);
    return 0;
}
