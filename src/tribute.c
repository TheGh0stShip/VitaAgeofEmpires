/* Patched RoR 1.0a diplomacy dialog tribute logic (0x004323b0). */
#include "tribute.h"

#include <string.h>

int aoe_tribute_exhausted_ror(const AoeTributeRor *t, int resource, float stock, int state5) {
    if (resource < 0 || resource > 3) return 1;
    return state5 || stock - t->spent[resource] <= 0.0f;
}

int aoe_tribute_click_ror(AoeTributeRor *t, int resource, int player, float stock, float fee) {
    float rate, cost;
    if (resource < 0 || resource > 3 || player < 1 || player > 8) return 0;
    if (aoe_tribute_exhausted_ror(t, resource, stock, 0)) return 0;
    if (fee < 0.0f) fee = 0.0f;
    rate = fee + 1.0f;
    cost = rate * 100.0f;
    if (cost <= stock - t->spent[resource]) {
        t->given[resource][player] += 100.0f;
        t->spent[resource] += cost;
    } else {
        float part = (stock - t->spent[resource]) / rate;
        t->given[resource][player] += part;
        t->spent[resource] += rate * part;
    }
    return 1;
}

void aoe_tribute_clear_ror(AoeTributeRor *t) { memset(t, 0, sizeof *t); }

void aoe_tribute_send_ror(const AoeTributeRor *t, int32_t players, int32_t self, float fee,
                          void (*send)(void *context, int32_t player, int resource, float amount,
                                       float fee),
                          void *context) {
    static const int order[4] = {3, 1, 0, 2};
    for (int32_t p = 1; p < players && p <= 8; ++p) {
        if (p == self) continue;
        for (int i = 0; i < 4; ++i)
            if (t->given[order[i]][p] > 0.0f) send(context, p, order[i], t->given[order[i]][p], fee);
    }
}

uint8_t aoe_diplomacy_stance_ror(int ally_on, int neutral_on) {
    if (ally_on) return 0;
    return neutral_on ? 1 : 3;
}
