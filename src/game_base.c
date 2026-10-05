#include "game_base.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

#include "aoe_ftol.h"

uint32_t aoe_player_checksum_compute_ror(AoePlayerChecksumRor *p) {
    int32_t resources = 0, objects = 0, count = 0;
    float positions = 0.0f;
    uint32_t sum;
    if (!p) return 0;
    p->valid = 1;
    p->field_1c = 0;
    for (int16_t i = 0; i < p->attribute_count && p->attributes; ++i)
        resources += aoe_ftol32(p->attributes[i]);
    for (int32_t i = 0; i < p->object_count && p->objects; ++i) {
        const AoeChecksumObjectRor *o = &p->objects[i];
        objects = (int32_t)((uint32_t)objects + (uint32_t)(o->type + o->state) +
                            (uint32_t)aoe_ftol32(o->field_44) +
                            (uint32_t)o->method_1d4);
        positions = (float)((double)positions + o->x + o->y + o->z);
        ++count;
    }
    sum = (uint32_t)(p->civilization + p->player_id);
    sum = (sum << 8) + (uint32_t)count;
    sum = (sum << 8) + (uint32_t)objects;
    sum = (sum << 8) + (uint32_t)resources;
    p->checksum = sum;
    p->position_sum = aoe_ftol32(positions);
    return sum;
}

uint32_t aoe_player_checksum_ror(AoePlayerChecksumRor *p) {
    if (!p) return 0;
    if (!p->valid) aoe_player_checksum_compute_ror(p);
    return p->checksum;
}

int aoe_player_checksums_ror(AoePlayerChecksumRor *p, uint32_t *checksum,
                             int32_t *position_sum, int32_t *field_1c) {
    if (!p || !checksum || !position_sum || !field_1c) return 0;
    if (!p->valid) aoe_player_checksum_compute_ror(p);
    *checksum = p->checksum;
    *position_sum = p->position_sum;
    *field_1c = p->field_1c;
    return 1;
}

void aoe_game_default_nothing_ror(AoeGameBaseRor *game) { (void)game; }

int32_t aoe_game_default_true_ror(AoeGameBaseRor *game, int which) {
    (void)game;
    (void)which;
    return 1;
}

static int slot_ok(int32_t slot) {
    return slot >= 0 && slot < AOE_GAME_TIMING_SLOTS;
}

void aoe_game_timing_reset_ror(AoeGameBaseRor *g) {
    if (!g) return;
    memset(g->timing_504, 0, sizeof(g->timing_504));
    for (int i = 0; i < AOE_GAME_TIMING_SLOTS; ++i) {
        AoeGameTimingRor *t = &g->timing[i];
        t->total = t->field_4 = t->field_8 = 0;
        t->last = t->max = t->field_1c = 0;
    }
}

void aoe_game_timing_sample_ror(AoeGameBaseRor *g, int32_t slot,
                                uint32_t value) {
    if (!g || !slot_ok(slot)) return;
    g->timing[slot].total += value;
    g->timing[slot].last = value;
    if (g->timing[slot].max < value) g->timing[slot].max = value;
}

void aoe_game_timing_count_508_ror(AoeGameBaseRor *g) {
    if (g) ++g->timing_504[1];
}

void aoe_game_timing_count_50c_ror(AoeGameBaseRor *g) {
    if (g) ++g->timing_504[2];
}

void aoe_game_timing_set_last_ror(AoeGameBaseRor *g, int32_t slot,
                                  uint32_t value) {
    if (g && slot_ok(slot)) g->timing[slot].last = value;
}

uint32_t aoe_game_timing_field4_ror(const AoeGameBaseRor *g, int32_t slot) {
    return g && slot_ok(slot) ? g->timing[slot].field_4 : 0;
}

uint32_t aoe_game_timing_last_ror(const AoeGameBaseRor *g, int32_t slot) {
    return g && slot_ok(slot) ? g->timing[slot].last : 0;
}

uint32_t aoe_game_timing_total_ror(const AoeGameBaseRor *g, int32_t slot) {
    return g && slot_ok(slot) ? g->timing[slot].total : 0;
}

int32_t aoe_game_timing_514_ror(const AoeGameBaseRor *g) {
    return g ? g->timing_504[4] : 0;
}

int32_t aoe_game_field_40_ror(const AoeGameBaseRor *g) {
    return g ? g->field_40 : 0;
}

char *aoe_game_load_string_ror(const AoeGameBaseOps *ops, uint32_t id,
                               char *out, int size) {
    if (!out || size <= 0) return out;
    if (!ops || !ops->load_string ||
        !ops->load_string(ops->context, id, out, size)) out[0] = '\0';
    out[size - 1] = '\0';
    return out;
}

const char *aoe_game_string_ror(const AoeGameBaseOps *ops, uint32_t id) {
    static char buffer[0x200];
    return aoe_game_load_string_ror(ops, id, buffer, sizeof(buffer));
}

void aoe_game_set_mode_ror(AoeGameBaseRor *g, int32_t mode) {
    if (g) g->mode = mode;
}

void aoe_game_set_state_ror(AoeGameBaseRor *g, int32_t state,
                            int32_t substate) {
    if (!g) return;
    g->state = state;
    g->substate = substate;
}

void aoe_game_view_player_ror(AoeGameBaseRor *g, int16_t player,
                              const AoeGameBaseOps *ops) {
    if (!g || !g->has_world || player >= g->player_count) return;
    g->view_player = player;
    if (ops && ops->refresh_view) ops->refresh_view(ops->context);
}

int32_t aoe_game_comm_value_ror(const AoeGameBaseRor *g,
                                const AoeGameBaseOps *ops) {
    if (!g || g->mode == 7 || !ops || !ops->comm_value) return 0;
    return ops->comm_value(ops->context);
}

void aoe_game_comm_mark_ror(AoeGameBaseRor *g, const AoeGameBaseOps *ops) {
    if (ops && ops->comm_mark) ops->comm_mark(ops->context);
    if (g && g->has_world) g->world_dirty = 1;
}

void aoe_game_comm_update_ror(AoeGameBaseRor *g, int32_t value,
                              int32_t extra, const AoeGameBaseOps *ops) {
    int32_t current;
    if (!g) return;
    current = aoe_game_comm_value_ror(g, ops);
    g->comm_state = current;
    if (value == current) return;
    g->comm_value = value ? extra : 0;
    aoe_game_comm_mark_ror(g, ops);
}

const uint8_t *aoe_game_record_1e4_ror(const AoeGameBaseRor *g, int32_t i) {
    return g && g->records_1e4 ? g->records_1e4 + 12 * i : NULL;
}

void *aoe_game_entry_5c_ror(const AoeGameBaseRor *g, int32_t i) {
    return g && g->array_5c ? g->array_5c[i] : NULL;
}

void *aoe_game_entry_6c_ror(const AoeGameBaseRor *g, int32_t i) {
    return g && g->array_6c ? g->array_6c[i] : NULL;
}

void *aoe_game_view_player_object_ror(const AoeGameBaseRor *g) {
    if (!g || !g->has_world || !g->player_count || !g->players ||
        g->player_count <= g->view_player || g->view_player < 0)
        return NULL;
    return g->players[g->view_player];
}

uint32_t aoe_game_player_checksum_ror(AoeGameBaseRor *g, int32_t p) {
    if (!g || !g->has_world || !g->checksums) return 0;
    if (p < 0) p = g->view_player;
    else if (p >= g->player_count) return 0;
    return aoe_player_checksum_ror(g->checksums[p]);
}

int aoe_game_checksums_ror(AoeGameBaseRor *g, uint32_t *checksum,
                           int32_t *position_sum, int32_t *field_1c) {
    if (!checksum || !position_sum || !field_1c) return 0;
    *checksum = 0;
    *position_sum = *field_1c = 0;
    if (!g || !g->has_world) return 0;
    for (int16_t i = 0; i < g->player_count && g->checksums; ++i) {
        uint32_t c = 0;
        int32_t a = 0, b = 0;
        aoe_player_checksums_ror(g->checksums[i], &c, &a, &b);
        *checksum += c;
        *position_sum += a;
        *field_1c += b;
    }
    return 1;
}

int aoe_game_has_option_ror(const AoeGameBaseRor *g, const char *option) {
    char line[256];
    size_t n = 0;
    if (!g || !g->command_line || !option) return 0;
    while (n < 255 && g->command_line[n]) {
        line[n] = (char)toupper((unsigned char)g->command_line[n]);
        ++n;
    }
    line[n] = '\0';
    return strstr(line, option) != NULL;
}

void aoe_game_mark_3f8_ror(AoeGameBaseRor *g, const AoeGameBaseOps *ops) {
    if (g) g->field_3f8 = 1;
    if (ops && ops->world_reset) ops->world_reset(ops->context);
}

void aoe_game_decode_mouse_ror(uint32_t wparam, uint32_t lparam,
                               int32_t xy[2], uint32_t *left,
                               uint32_t *right, uint32_t *shift,
                               uint32_t *control) {
    uint32_t x = lparam & 0xffff, y = lparam >> 16;
    if (!xy || !left || !right || !shift || !control) return;
    xy[0] = (int32_t)x;
    xy[1] = (int32_t)y;
    if (x > 60000) xy[0] = (int32_t)x - 0x10000;
    if (y > 60000) xy[1] = (int32_t)y - 0x10000;
    *left = wparam & 1;
    *right = wparam & 2;
    *shift = wparam & 8;
    *control = wparam & 4;
}

static void system_cursor(const AoeGameBaseOps *ops, uintptr_t cursor) {
    if (ops && ops->set_system_cursor)
        ops->set_system_cursor(ops->context, cursor);
}

static void draw_on(const AoeGameBaseOps *ops) {
    if (ops && ops->cursor_draw_on) ops->cursor_draw_on(ops->context);
}

static void draw_off(const AoeGameBaseOps *ops) {
    if (ops && ops->cursor_draw_off) ops->cursor_draw_off(ops->context);
}

void aoe_game_cursor_show_ror(AoeGameBaseRor *g, const AoeGameBaseOps *ops) {
    if (!g || g->cursor_shown) return;
    if (g->cursor_capable && !g->software_cursor) draw_on(ops);
    else system_cursor(ops, g->cursor);
    g->cursor_shown = 1;
}

void aoe_game_cursor_hide_ror(AoeGameBaseRor *g, const AoeGameBaseOps *ops) {
    if (!g || !g->cursor_shown) return;
    if (g->cursor_capable && !g->software_cursor) draw_off(ops);
    else system_cursor(ops, 0);
    g->cursor_shown = 0;
}

void aoe_game_cursor_software_ror(AoeGameBaseRor *g, int32_t on,
                                  const AoeGameBaseOps *ops) {
    if (!g) return;
    if (!g->cursor_capable) {
        g->software_cursor = 1;
    } else {
        g->software_cursor = on;
        if (!on) {
            system_cursor(ops, 0);
            if (!g->cursor_shown) draw_off(ops);
            else draw_on(ops);
            return;
        }
    }
    draw_off(ops);
    system_cursor(ops, g->cursor_shown ? g->cursor : 0);
}

void aoe_game_cursor_set_ror(AoeGameBaseRor *g, uintptr_t cursor,
                             const AoeGameBaseOps *ops) {
    if (!g) return;
    g->cursor = cursor;
    if (cursor && ops && ops->last_cursor) *ops->last_cursor = cursor;
    if (g->cursor_shown && (!g->cursor_capable || g->software_cursor))
        system_cursor(ops, g->cursor);
}

void aoe_game_timing_setup_ror(AoeGameBaseRor *g) {
    g->timing[1].counted = 1;
    g->timing[2].counted = 1;
    g->timing[3].counted = 1;
    g->timing[10].counted = 1;
    g->timing[13].counted = 1;
    g->timing[0].held = 1;
    g->timing[4].held = 1;
}

static uint32_t per_second(uint32_t elapsed, uint32_t count, uint32_t none) {
    uint32_t per;
    if (elapsed == 0 || count == 0) return none;
    per = elapsed / count;
    return per == 0 ? none : 1000u / per;
}

void aoe_game_timing_rollover_ror(AoeGameBaseRor *g, uint32_t now,
                                  const AoeGameRolloverOpsRor *ops) {
    AoeGameTimingRor *t = g->timing;
    uint32_t busy = 0, elapsed, n;
    int i;
    for (i = 0; i < AOE_GAME_TIMING_SLOTS; ++i) {
        if (t[i].held) continue;
        t[i].field_4 = t[i].total;
        t[i].field_1c = t[i].max;
        t[i].total = 0;
        t[i].max = 0;
        t[i].field_8 = now;
        if (t[i].counted) busy += t[i].field_4;
    }
    if (t[0].field_8 == 0)
        t[0].field_4 = busy;
    else
        t[0].field_4 = now - t[0].field_8;
    elapsed = t[0].field_4;
    t[0].field_8 = now;
    t[4].field_4 = busy < elapsed ? elapsed - busy : 0;
    if (t[6].field_4 + t[7].field_4 < t[2].field_4)
        t[9].field_4 = t[2].field_4 - t[6].field_4 - t[7].field_4;
    else
        t[9].field_4 = 0;
    g->timing_504[6] = (int32_t)per_second(elapsed, (uint32_t)g->timing_504[0], 1000);
    g->timing_504[8] = (int32_t)per_second(elapsed, (uint32_t)g->timing_504[2], 0);
    g->timing_504[7] = (int32_t)per_second(elapsed, (uint32_t)g->timing_504[1], 0);
    if (ops->hook) ops->hook(ops->context);
    if (ops->log && g->mode == 4) {
        n = (uint32_t)g->timing_504[1];
#define PER(v) (n ? (unsigned long)((v) / n) : 0ul)
        fprintf(ops->log,
                "%lu, %ld, %ld, %ld, %ld, %d, %d, %d, %d, %lu, %lu, %lu, %lu, %lu, %lu, %lu, "
                "%lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %d, %d, %d \n",
                (unsigned long)ops->world_id, (long)ops->globals[0], (long)ops->globals[1],
                (long)ops->globals[2], (long)ops->globals[3], ops->gaia_counts[0],
                ops->gaia_counts[1], (int)ops->other_counts[0], (int)ops->other_counts[1],
                (unsigned long)n, (unsigned long)(uint32_t)g->timing_504[2],
                (unsigned long)(uint32_t)g->timing_504[7],
                (unsigned long)(uint32_t)g->timing_504[8], PER(t[6].field_4),
                (unsigned long)t[6].field_1c, PER(t[7].field_4), (unsigned long)t[7].field_1c,
                PER(t[3].field_4), (unsigned long)t[3].field_1c, PER(t[1].field_4),
                (unsigned long)t[1].field_1c, PER(t[12].field_4 + t[10].field_4),
                (unsigned long)(t[12].field_1c + t[10].field_1c), PER(t[13].field_4),
                (unsigned long)t[13].field_1c, PER(t[4].field_4), (unsigned long)t[4].field_1c,
                0, -1, -1);
#undef PER
        fflush(ops->log);
    }
    g->timing_504[3] = g->timing_504[0];
    g->timing_504[4] = g->timing_504[1];
    g->timing_504[5] = g->timing_504[2];
    g->timing_504[0] = g->timing_504[1] = g->timing_504[2] = 0;
}
