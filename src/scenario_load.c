#include "scenario_load.h"

static int mapped(const AoeScenarioLoadOps *o, const int32_t map[9], int i) {
    return i >= 0 && i < 9 && map[i] >= 0 && o->player_present(o->ctx, map[i]);
}

/* 0x0051ec20: legacy world load, scale 1.0; a placement is applied when
   slot i < player_count, map[i] >= 0 and players[i] (not players[map[i]],
   as in the original) exists; the final AI pass has no map check (a negative map entry would index
   players[-1]; the port skips it).
   0x0051eee0: the same with 0x007d2460 cleared first, the placement test on
   players[map[i]] (no count check), scale 1.03 and map[i] >= 0 required in
   the final pass. Both never touch slots past 8 (the original's 9-entry map
   initialisation spins forever for counts above 10). */
void aoe_scenario_load_legacy(AoeScenarioLoadWorld *w,
                              const AoeScenarioLoadOps *o,
                              AoeOutcomeStream *s, const int32_t *slot_order,
                              int variant) {
    int32_t map[9];
    int32_t n, groups, count, i, j, header;
    if (variant) w->load_counter = 0;
    header = aoe_stream_i32(s);
    o->read_header(o->ctx, s);
    o->read_map(o->ctx, s);
    n = w->player_count;
    for (i = 0; i < 9; i++) map[i] = -1;
    for (i = 0; i < n; i++)
        if (slot_order[i] >= 0 && slot_order[i] < 9) map[slot_order[i]] = i;
    for (i = 0; i < n; i++) o->player_new_view(o->ctx, i);
    groups = aoe_stream_i32(s);
    for (i = 0; i < groups && !s->failed; i++) {
        (void)aoe_stream_u8(s);
        (void)aoe_stream_u8(s);
    }
    for (i = 0; i < groups && !s->failed; i++) {
        count = aoe_stream_i32(s);
        for (j = 0; j < count && !s->failed; j++) {
            int32_t x = aoe_stream_i32(s), y = aoe_stream_i32(s);
            int32_t z = aoe_stream_i32(s), id = aoe_stream_i32(s);
            const uint8_t *t = aoe_stream_take(s, 2);
            int16_t type = t ? (int16_t)(t[0] | (t[1] << 8)) : 0;
            uint8_t st = aoe_stream_u8(s);
            int32_t extra = aoe_stream_i32(s);
            int ok;
            if (s->failed || i >= 9) continue;
            if (variant)
                ok = mapped(o, map, i);
            else
                ok = i < w->player_count && map[i] >= 0 &&
                     o->player_present(o->ctx, i);
            if (ok) {
                w->placing = 1;
                w->placing_id = id;
                o->player_place(o->ctx, map[i], x, y, z, type, st, extra);
                w->placing = 0;
            }
        }
    }
    w->header_value = header;
    (void)aoe_stream_take(s, 13);
    for (i = 1; i < groups && i < 9; i++) {
        int p = (i < w->player_count && mapped(o, map, i)) ? map[i] : 0;
        o->player_load(o->ctx, p, s, map, variant ? 1.03f : 1.0f);
    }
    o->close(o->ctx, s);
    for (i = 0; i + 1 < w->player_count && i + 1 < groups && i + 1 < 9; i++) {
        int32_t k, per, city, ai;
        if (!o->player_present(o->ctx, i + 1)) continue;
        if (map[i + 1] < 0) continue; /* variant 0 would index players[-1] */
        k = o->info_kind(o->ctx, i);
        per = o->info_personality(o->ctx, i);
        city = o->info_city(o->ctx, i);
        ai = o->info_ai(o->ctx, i);
        o->player_set_ai(o->ctx, map[i + 1], ai, city, per, k, -1);
    }
}
