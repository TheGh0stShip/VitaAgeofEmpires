#include "zone_map.h"

#include <assert.h>
#include <string.h>

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
    return 0;
}
