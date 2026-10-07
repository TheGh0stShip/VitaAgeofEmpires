#include "world_game_sync.h"
#include "le_bytes.h"

#include <stdio.h>
#include <string.h>

#define CTX (c->ops->context)

static void logf_(AoeCommRor *c, const char *text) {
    if (c->ops->log) c->ops->log(CTX, text);
}

/* 0x004241a0: a slot's name (ours at +0x10f8, else +0x12c8's 0x80-byte
   entry); NULL past the count. */
static const char *slot_name(const AoeCommRor *c, uint32_t slot) {
    if (c->count < slot) return NULL;
    if (slot == c->self) return c->name;
    return c->names ? c->names[slot] : NULL;
}

/* The 0x44 turn marker (0x0041ebe0, 0x41ed49..0x41edaf): our slot's turn
   becomes marked; past turn 6 our own speed sample is recorded and rides
   along at +8 / +9. Those two bytes are stale stack in the original for
   the first turns; they are 0 here. */
static void mark_turn(AoeCommRor *c, const AoeCommTurnOpsRor *o) {
    uint8_t m[0x0c];
    uint32_t marker = (uint32_t)c->lead + c->turn;
    memset(m, 0, sizeof m);
    c->last_turn[c->self] = c->turn;
    if (c->turn > 6) {
        m[8] = (uint8_t)o->latency_byte(o->context);
        m[9] = (uint8_t)o->frame_average(o->context);
        o->set_player_rate(o->context, c->self, m[9], m[8]);
    }
    *o->marker_time = c->ops->now_ms(CTX);
    m[0] = 0x44;
    aoe_le_put_u32(m + 4, marker);
    c->ops->message(CTX, 0x44, m, sizeof m);
}

/* 0x0041ebe0, 0x41ee5f..0x41f082: the other playing slots timed. */
static void time_slots(AoeCommRor *c, const AoeCommTurnOpsRor *o, uint32_t now, int32_t *others,
                       int32_t *gone, uint32_t *newest) {
    char line[0x100], fmt[100];
    uint32_t slot;
    for (slot = 1; slot <= c->count && slot < AOE_COMM_SLOTS; ++slot) {
        uint32_t elapsed;
        int timeout = 0;
        if (!aoe_comm_playing_ror(c, slot) || slot == c->self) continue;
        ++*others;
        if (*newest <= c->time_1344[slot]) *newest = c->time_1344[slot];
        elapsed = now - c->time_1344[slot];
        if (elapsed > 60000) ++*gone;
        if (elapsed > 25000 && now - (uint32_t)c->field_136c[slot] > 25000) {
            c->field_136c[slot] = (int32_t)now;
            c->ops->post(CTX, 0x17bd, slot);
        }
        if (c->turn < 8 && elapsed > 98000) {
            snprintf(line, sizeof line, "+++Timeout pregrame, turn=%d PlayerTimeout=%d",
                     (int)c->turn, (int)elapsed);
            logf_(c, line);
            timeout = 1;
        }
        if (c->turn > 7 && elapsed > 0x1da9c && c->is_host) {
            snprintf(line, sizeof line, "+++Timeout host, turn=%d PlayerTimeout=%d",
                     (int)c->turn, (int)elapsed);
            logf_(c, line);
            timeout = 1;
        }
        if (c->turn > 7 && elapsed > 0x1d6b4) {
            snprintf(line, sizeof line, "+++Timeout player, turn=%d PlayerTimeout=%d",
                     (int)c->turn, (int)elapsed);
            logf_(c, line);
        } else if (!timeout) {
            continue;
        }
        snprintf(line, sizeof line, "Player timed out with %d msec elapsed", (int)elapsed);
        logf_(c, line);
        snprintf(line, sizeof line, "Recommend player '%d' drop.", (int)slot);
        logf_(c, line);
        if (c->is_host) {
            const char *name;
            snprintf(line, sizeof line, "Player %d Turn#%d  timeout = %d ", (int)slot,
                     (int)c->turn, (int)elapsed);
            logf_(c, line);
            fmt[0] = 0;
            c->ops->string(CTX, 0x4ba, fmt, (int)sizeof fmt);
            name = slot_name(c, slot);
            snprintf(c->text, sizeof c->text, fmt, name ? name : "", (int)slot);
            if (c->turn > 6) o->chat_all(o->context, c->text);
            logf_(c, c->text);
            c->field_149c[slot] = 1;
            o->drop_player(o->context, slot);
        }
        c->time_1344[slot] = now;
        c->fast_time[slot] = now;
    }
}

/* 0x0041ebe0, 0x41f0db..0x41f19f: the rest stopped answering. */
static void dropped_myself(AoeCommRor *c, const AoeCommTurnOpsRor *o) {
    uint32_t slot;
    logf_(c, "I Dropped myself.");
    c->ops->post(CTX, 0x17b2, c->self);
    for (slot = 1; slot <= c->count && slot < AOE_COMM_SLOTS; ++slot) c->dropped[slot] = 1;
    o->end_report(o->context);
    logf_(c, "Initialize Player List");
    for (slot = 1; slot <= c->count; ++slot) o->init_player(o->context, slot);
    c->dirty = 0;
    c->data_time = 0;
    o->refresh_all(o->context);
}

/* 0x0041ebe0 */
int32_t aoe_comm_turn_ror(AoeCommRor *c, const AoeCommTurnOpsRor *o, uint32_t game_time) {
    char line[0x80];
    uint32_t now, newest = 0, last;
    int32_t others = 0, gone = 0, line_no;
    if (!c->multiplayer) return c->state != 4 ? -1 : 0;
    /* Port guard: the per-slot arrays hold AOE_COMM_SLOTS entries. */
    if (c->count < c->self || c->self >= AOE_COMM_SLOTS) return 0;
    now = c->ops->now_ms(CTX);
    if (now - *o->status_time >= 5000) {
        for (line_no = 6; line_no >= 1; --line_no) {
            const char *text = o->status_line(o->context, line_no);
            if (text) logf_(c, text);
        }
        *o->status_time = now;
    }
    o->pump(o->context);
    c->time_1344[c->self] = now;
    if (c->multiplayer && c->state != 3 && c->last_turn[c->self] != c->turn) mark_turn(c, o);
    o->sync_turn(o->context, c->turn);
    if (c->field_10e8 == 1 && c->field_10ec <= game_time) {
        c->ops->post(CTX, 0x17b0, c->field_10ec);
        return 0;
    }
    if (o->frames_left(o->context) != 0 && c->turn > 6)
        return (int32_t)o->frame(o->context, c->turn);
    if (aoe_comm_caught_up_ror(c)) {
        uint32_t granularity = 0, buffer = 0;
        if (c->state != 5) return 0;
        o->begin_turn(o->context);
        if (c->is_host) o->adjust(o->context, &granularity, &buffer, 1);
        ++c->turn;
        snprintf(line, sizeof line, "....Starting Turn#%d, WT=%d, Fr=%d, Gr=%d", (int)c->turn,
                 (int)game_time, (int)granularity, (int)buffer);
        logf_(c, line);
        o->check_stop_turn(o->context, c->turn);
        return (int32_t)o->take_granularity(o->context);
    }
    if (c->state < 5) return 0;
    o->count(o->context);
    now = c->ops->now_ms(CTX);
    time_slots(c, o, now, &others, &gone, &newest);
    if (others + 1 == 3 ? gone > 1 : (others + 1) / 2 <= gone) {
        dropped_myself(c, o);
        return 0;
    }
    last = c->field_12d0;
    if (last < newest) last = newest;
    if (now - last > 25000) {
        o->ping_quiet(o->context, 1);
        c->field_12d0 = c->ops->now_ms(CTX);
    }
    o->reset_frame(o->context);
    return 0;
}
