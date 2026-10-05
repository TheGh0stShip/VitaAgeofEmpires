#include "zone_map.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t *terrain_tab;
static uint32_t terrain_at(void *c, int32_t x, int32_t y) {
    (void)c;
    return 0x100u | terrain_tab[y * 6 + x]; /* the high byte is ignored */
}

/* Patched RoR 1.0a terrain-restriction zones (0x00521060 / 0x00521210 /
   0x00521330 / 0x00521620 / 0x00521680 / 0x00521770 / 0x005215c0 /
   0x00521b50). */
int main(void) {
    enum { N = 6 };
    /* Land (terrain 0) with a lake (1) splitting off the east column and
       a separate pond. Low 5 bits only: 0x20 is terrain 0 too. */
    uint8_t t[N * N] = {
        0, 0, 1, 0, 0, 0,
        0, 0, 1, 0, 0, 0,
        0x20, 0, 1, 1, 1, 1,
        0, 0, 1, 0, 0, 0,
        0, 0, 0, 0, 1, 0,
        0, 0, 0, 0, 0, 0,
    };
    float land[2] = {1.0f, 0.0f}, boat[2] = {0.0f, 1.0f},
          land2[2] = {0.5f, -1.0f};
    AoeZoneMapRor m;
    AoeZoneManagerRor z;
    memset(&z, 0, sizeof(z));
    assert(aoe_zone_map_build_ror(&m, N, N, t, land, 2));
    /* Scan order: (0,0) is zone 0 (all connected land), the water at (2,0)
       zone 1 (lake), the north-east land block zone 2, the pond zone 3. */
    assert(aoe_zone_at_ror(&m, 0, 0) == 0 && aoe_zone_at_ror(&m, 0, 2) == 0);
    assert(aoe_zone_at_ror(&m, 2, 0) == 1 && aoe_zone_at_ror(&m, 5, 2) == 1);
    assert(aoe_zone_at_ror(&m, 3, 0) == 2 && aoe_zone_at_ror(&m, 5, 1) == 2);
    assert(aoe_zone_at_ror(&m, 3, 3) == 0 && aoe_zone_at_ror(&m, 4, 4) == 3);
    /* Sizes are counted once. */
    assert(aoe_zone_size_ror(&m, 2) == 6 && m.counts[2] == 6);
    assert(aoe_zone_size_ror(&m, 3) == 1);
    /* Near: zone 2 is two tiles east of (1,1) (distance 2 at (3,1)). */
    assert(aoe_zone_near_ror(&m, 2, 1, 1, 2.0f));
    assert(!aoe_zone_near_ror(&m, 2, 1, 1, 1.9f));
    /* Diagonal (1.414) is not inside 1.0. */
    assert(!aoe_zone_near_ror(&m, 3, 3, 3, 1.0f) &&
           aoe_zone_near_ror(&m, 3, 3, 3, 1.5f));
    aoe_zone_map_free_ror(&m);

    /* The manager shares a map between tables of the same passability. */
    assert(aoe_zone_manager_index_ror(&z, N, N, t, land, 2) == 0);
    assert(aoe_zone_manager_index_ror(&z, N, N, t, boat, 2) == 1);
    assert(aoe_zone_manager_index_ror(&z, N, N, t, land2, 2) == 0);
    assert(z.count == 2);
    assert(aoe_zone_at_ror(aoe_zone_manager_map_ror(&z, 1), 2, 0) == 1);
    aoe_zone_manager_free_ror(&z);

    /* Zone numbers wrap after 255: a checkerboard of 300 one-tile zones. */
    {
        static uint8_t cb[20 * 20];
        for (int y = 0; y < 20; ++y)
            for (int x = 0; x < 20; ++x) cb[y * 20 + x] = (uint8_t)((x + y) & 1);
        assert(aoe_zone_map_build_ror(&m, 20, 20, cb, land, 2));
        /* Tile 255 (row 12, col 15) restarts at 0. */
        assert(aoe_zone_at_ror(&m, 14, 12) == 254 &&
               aoe_zone_at_ror(&m, 15, 12) == 0);
        aoe_zone_map_free_ror(&m);
    }
    {
        /* Save and load (0x00521190 / 0x00520f60) and the manager. */
        static uint8_t buf[8192];
        AoeOutcomeStream st = {buf, sizeof buf, 0, 0};
        AoeZoneMapRor a, *only = &a;
        AoeZoneManagerRor mgr = {&only, 1}, back;
        int32_t at[2];
        assert(aoe_zone_map_build_ror(&a, N, N, t, land, 2));
        a.field_40c[3] = 9;
        aoe_zone_manager_save_ror(&mgr, &st);
        assert(!st.failed && st.pos == 4 + 255 + 255 * 4 + N * N + 4 + 2 * 4);
        st.size = st.pos;
        st.pos = 0;
        assert(aoe_zone_manager_load_ror(&back, &st, N, N) && back.count == 1);
        assert(st.pos == st.size && back.maps[0]->field_40c[3] == 9);
        assert(aoe_zone_at_ror(back.maps[0], 4, 4) == 3 && back.maps[0]->table[0] == 1.0f);
        /* Near the zone of (4, 4) (the pond). */
        assert(aoe_zone_near_tile_ror(back.maps[0], 4, 4, 4, 3, 1.0f));
        assert(!aoe_zone_near_tile_ror(back.maps[0], 4, 4, 1, 1, 2.0f));
        aoe_zone_manager_free_ror(&back);
        aoe_zone_manager_init_ror(&back);
        assert(back.count == 0 && back.maps == NULL);
        /* Nearest water (1) tile to (0, 0) within 3: (2, 0). */
        terrain_tab = t;
        assert(aoe_zone_nearest_terrain_ror(&a, 0, 0, at, 1, 99, 3, terrain_at, NULL));
        assert(at[0] == 2 && at[1] == 0);
        assert(!aoe_zone_nearest_terrain_ror(&a, 0, 5, at, 1, 99, 1, terrain_at, NULL));
        aoe_zone_map_free_ror(&a);
    }
    return 0;
}
