#include "shared_vision.h"

#include <assert.h>
#include <string.h>

/* World 0x00520c60 shared vision. */
int main(void) {
    int32_t flags[9] = {0};
    uint8_t stance[9][9];
    AoePlayerSharedVision v[9];
    memset(stance, 3, sizeof(stance)); /* everyone hostile */
    for (int i = 0; i < 9; ++i) stance[i][i] = 0;
    /* No flags: own bits only. */
    aoe_world_shared_vision_ror(4, flags, stance, v);
    assert(v[0].visible == 1u && v[0].explored == 1u << 16);
    assert(v[2].visible == 4u && v[2].explored == 1u << 18 &&
           v[2].shares[2] == 1 && v[2].shares[1] == 0 && v[2].indirect == 0);
    /* Players 1 and 2 mutually allied; only 1 has the flag. */
    stance[1][2] = stance[2][1] = 0;
    flags[1] = 1;
    aoe_world_shared_vision_ror(4, flags, stance, v);
    assert(v[1].visible == (2u | 4u) &&
           v[1].explored == ((1u << 17) | (1u << 18)) && v[1].shares[2] == 1);
    assert(v[2].visible == 4u && v[2].shares[1] == 0);
    /* One-sided alliance does not share. */
    stance[2][1] = 1;
    aoe_world_shared_vision_ror(4, flags, stance, v);
    assert(v[1].visible == 2u);
    /* Indirect: 3 shares with 1 and 2, while 1 and 2 do not share. */
    memset(stance, 3, sizeof(stance));
    for (int i = 0; i < 9; ++i) stance[i][i] = 0;
    stance[3][1] = stance[1][3] = 0;
    stance[3][2] = stance[2][3] = 0;
    memset(flags, 0, sizeof(flags));
    flags[3] = 1;
    aoe_world_shared_vision_ror(4, flags, stance, v);
    assert(v[3].shares[1] == 1 && v[3].shares[2] == 1);
    /* Player 1 does not share 2 directly; 3 shares with both 2 and 1, and 1
       does not share 3: so 1 links to 2 through 3. The binary does not
       exclude m == k, so 3 is also linked through itself. */
    assert(v[1].indirect == (4u | 8u) && v[1].via[2] == 8u &&
           v[1].via[3] == 8u);
    assert(v[2].indirect == (2u | 8u) && v[2].via[1] == 8u &&
           v[2].via[3] == 8u);
    assert(v[3].indirect == 0u);
    return 0;
}
