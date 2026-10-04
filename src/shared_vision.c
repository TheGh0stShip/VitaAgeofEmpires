#include "shared_vision.h"

#include <string.h>

void aoe_world_shared_vision_ror(int count, const int32_t shares_flag[],
                                 const uint8_t stance[][AOE_SHARED_VISION_PLAYERS],
                                 AoePlayerSharedVision out[]) {
    if (count > AOE_SHARED_VISION_PLAYERS) count = AOE_SHARED_VISION_PLAYERS;
    for (int i = 0; i < count; ++i) {
        AoePlayerSharedVision *p = &out[i];
        memset(p, 0, sizeof(*p));
        p->shares[i] = 1;
        p->visible = 1u << i;
        p->explored = 1u << (i + 16);
    }
    for (int i = 1; i < count; ++i) {
        if (shares_flag[i] != 1) continue;
        for (int j = 1; j < count; ++j) {
            if (j == i || stance[i][j] != 0 || stance[j][i] != 0) continue;
            out[i].shares[j] = 1;
            out[i].visible |= 1u << j;
            out[i].explored |= 1u << (j + 16);
        }
    }
    for (int i = 0; i < count; ++i)
        for (int k = 1; k < count; ++k) {
            if (k == i || out[i].shares[k] != 0) continue;
            for (int m = 1; m < count; ++m) {
                if (out[i].shares[m] != 0 || out[m].shares[k] != 1 ||
                    out[m].shares[i] != 1)
                    continue;
                out[i].indirect |= 1u << k;
                out[i].via[k] |= 1u << m;
            }
        }
}
