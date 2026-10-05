#include "comm_speed.h"

#include <stdio.h>
#include <string.h>

static int active(const AoeCommSpeedRor *s, int32_t p) {
    return s->ops.active && s->ops.active(s->ops.context, p);
}

void aoe_comm_speed_init_ror(AoeCommSpeedRor *s, const AoeCommSpeedOpsRor *ops,
                             uint32_t (*now)(void *), void *clock_context) {
    int i;
    if (!s) return;
    memset(s, 0, sizeof(*s));
    if (ops) s->ops = *ops;
    s->granularity = s->optimal_granularity = 90;
    s->buffer = s->optimal_buffer = 10;
    s->enabled = 1;
    s->field_128 = 500;
    for (i = 0; i < 10; ++i) s->latency[i] = 30;
    s->max_latency = 500;
    aoe_frame_timer_init_ror(&s->frame_timer, now, clock_context);
    aoe_frame_timer_init_ror(&s->turn_timer, now, clock_context);
}

void aoe_comm_speed_set_player_rate_ror(AoeCommSpeedRor *s, uint32_t player,
                                        uint8_t frame_ms, uint8_t turn_ms_10) {
    if (!s || player == 0 || player >= 10) return;
    s->frame_ms[player] = frame_ms;
    s->turn_ms[player] = (uint32_t)turn_ms_10 * 10;
}

void aoe_comm_speed_set_latency_ror(AoeCommSpeedRor *s, uint32_t player,
                                    uint32_t ms) {
    if (!s || player == 0 || player >= 10) return;
    s->latency[player] = ms < 30 ? 30 : ms > 6000 ? 6000 : ms;
}

uint32_t aoe_comm_speed_latency_ror(const AoeCommSpeedRor *s, uint32_t player) {
    return s && player != 0 && player < 10 ? s->latency[player] : 0;
}

void aoe_comm_speed_schedule_ror(AoeCommSpeedRor *s, uint32_t buffer,
                                 uint32_t granularity, uint32_t turn) {
    if (!s || s->pending_turn) return;
    s->pending_buffer = buffer;
    s->pending_granularity = granularity;
    s->pending_turn = turn;
}

void aoe_comm_speed_apply_pending_ror(AoeCommSpeedRor *s) {
    if (!s || !s->pending_turn) return;
    s->checks = 0;
    if (s->pending_turn <= (s->ops.turn ? s->ops.turn(s->ops.context) : 0)) {
        s->pending_turn = 0;
        ++s->changes;
        s->buffer = s->pending_buffer;
        s->granularity = s->pending_granularity;
        s->pending_buffer = 0;
        s->pending_granularity = 0;
    }
}

void aoe_comm_speed_begin_turn_ror(AoeCommSpeedRor *s) {
    if (!s) return;
    aoe_frame_timer_mark_ror(&s->turn_timer);
    s->late_turn = 0;
    aoe_comm_speed_apply_pending_ror(s);
    s->frames_left = s->buffer;
}

uint32_t aoe_comm_speed_frame_ror(AoeCommSpeedRor *s, uint32_t frame) {
    if (!s) return 0;
    if (s->ready) aoe_frame_timer_mark_ror(&s->frame_timer);
    else aoe_frame_timer_restart_ror(&s->frame_timer);
    s->ready = 0;
    if (s->frames_left == 0) return 0;
    s->elapsed = aoe_frame_timer_elapsed_ror(&s->turn_timer);
    if (s->ops.caught_up && s->ops.caught_up(s->ops.context))
        s->waited_frames += s->frames_left;
    s->target = (s->buffer - s->frames_left) * s->granularity;
    if (s->slack + s->elapsed < s->target) {
        if (frame > 6) {
            ++s->late_total;
            ++s->late_turn;
        }
        s->ready = 0;
        return 0;
    }
    s->ready = 1;
    --s->frames_left;
    return s->granularity;
}

void aoe_comm_speed_reset_frame_ror(AoeCommSpeedRor *s) {
    if (!s) return;
    aoe_frame_timer_restart_ror(&s->frame_timer);
    s->ready = 0;
}

static uint32_t highest_latency(const AoeCommSpeedRor *s) {
    uint32_t best = 0, p;
    for (p = 1; p < 10; ++p)
        if (active(s, (int32_t)p) && best <= s->latency[p]) best = s->latency[p];
    return best;
}

uint32_t aoe_comm_speed_latency_byte_ror(AoeCommSpeedRor *s) {
    uint32_t v;
    if (!s) return 1;
    s->max_latency = highest_latency(s);
    v = (s->max_latency + 5) / 10;
    if (v < 2) return 1;
    return v > 254 ? 255 : v;
}

uint32_t aoe_comm_speed_optimal_buffer_ror(AoeCommSpeedRor *s) {
    uint32_t v;
    if (!s || !s->granularity) return 0;
    v = highest_latency(s) / s->granularity;
    if (v >= 20) v = 20;
    if (v <= 4) v = 4;
    s->optimal_buffer = v + 1;
    return s->optimal_buffer;
}

uint32_t aoe_comm_speed_frame_average_ror(AoeCommSpeedRor *s) {
    return s ? aoe_frame_timer_average_ror(&s->frame_timer, 50) : 0;
}

uint32_t aoe_comm_speed_optimal_granularity_ror(AoeCommSpeedRor *s) {
    uint32_t v;
    if (!s) return 0;
    v = aoe_frame_timer_average_ror(&s->frame_timer, 50);
    if (v > 150) v = 150;
    if (v < 10) v = 10;
    s->optimal_granularity = v;
    return v;
}

static uint32_t distance(uint32_t a, uint32_t b) {
    int32_t d = (int32_t)(a - b);
    return (uint32_t)(d < 0 ? -d : d);
}

int aoe_comm_speed_adjust_ror(AoeCommSpeedRor *s, uint32_t *granularity,
                              uint32_t *buffer, int apply) {
    uint32_t turn, gran = 10, lat = 30, buf, current, gap, p;
    if (!s || !granularity || !buffer) return 0;
    if (s->ops.is_host && !s->ops.is_host(s->ops.context)) return 0;
    if (s->pending_turn) return 0;
    turn = (uint32_t)(s->ops.turn_lead ? s->ops.turn_lead(s->ops.context) : 0) +
           (s->ops.turn ? s->ops.turn(s->ops.context) : 0);
    if (turn < 20) return 0;
    if (++s->checks < 5) return 0;
    for (p = 1; p < 10; ++p) {
        if (!active(s, (int32_t)p)) continue;
        if (gran <= s->frame_ms[p]) gran = s->frame_ms[p];
        if (lat <= s->turn_ms[p]) lat = s->turn_ms[p];
    }
    gran = (gran + 5) / 10 * 10;
    if (gran > 150) gran = 150;
    if (distance(gran, s->granularity) < 10) gran = s->granularity;
    buf = lat / gran + 2;
    if (buf >= 20) buf = 20;
    else if (buf < 5) buf = 4;
    if (gran * buf < 250) buf = 250 / gran + 1;
    current = s->buffer * s->granularity;
    gap = distance(gran * buf, current);
    if (gap < 11) return 0;
    if (gran * buf < current && gap > 100 && s->buffer > 4) buf = s->buffer - 1;
    *granularity = gran;
    *buffer = buf;
    if (apply && s->enabled) {
        aoe_comm_speed_schedule_ror(s, buf, gran, turn);
        if (s->ops.send_speed) s->ops.send_speed(s->ops.context, buf, gran);
    }
    return 1;
}

void aoe_comm_speed_set_enabled_ror(AoeCommSpeedRor *s, int32_t enabled) {
    if (s) s->enabled = enabled;
}

uint32_t aoe_comm_speed_changes_ror(const AoeCommSpeedRor *s) {
    return s ? s->changes : 0;
}

uint32_t aoe_comm_speed_take_granularity_ror(AoeCommSpeedRor *s) {
    if (!s) return 0;
    s->ready = 1;
    return s->granularity;
}

uint32_t aoe_comm_speed_frames_left_ror(const AoeCommSpeedRor *s) {
    return s ? s->frames_left : 0;
}

void aoe_comm_speed_count_ror(AoeCommSpeedRor *s) {
    if (s) ++s->field_210;
}

static void append(char *out, size_t size, const char *piece) {
    size_t n = strlen(out), m = strlen(piece);
    if (n + m >= size) m = size - n - 1;
    memcpy(out + n, piece, m);
    out[n + m] = 0;
}

const char *aoe_comm_speed_latency_text_ror(AoeCommSpeedRor *s) {
    char piece[24];
    uint32_t p;
    if (!s) return "";
    strcpy(s->text, "Latency> ");
    for (p = 1; p < 10; ++p) {
        if (!active(s, (int32_t)p)) continue;
        snprintf(piece, sizeof piece, "P%d=%d ", (int)p, (int)s->latency[p]);
        append(s->text, sizeof s->text, piece);
    }
    return s->text;
}

const char *aoe_comm_speed_rate_text_ror(AoeCommSpeedRor *s, char *out,
                                         int size) {
    char piece[24];
    uint32_t p;
    if (!s || !out || size <= 0) return "";
    snprintf(out, (size_t)size, "PlrSpeed> ");
    for (p = 1; p < 10; ++p) {
        if (!active(s, (int32_t)p)) continue;
        snprintf(piece, sizeof piece, "P#%1d(fr%d lat%d) ", (int)p,
                 (int)s->frame_ms[p], (int)s->turn_ms[p]);
        append(out, (size_t)size, piece);
    }
    return out;
}

const char *aoe_comm_speed_turn_text_ror(AoeCommSpeedRor *s, int32_t player,
                                         char *out, int size) {
    if (!s || !out || size <= 0 || player < 0 || player >= 10) return NULL;
    snprintf(out, (size_t)size, "Buf=%3d / %3d * Gran=%3dms = turn %4dms   %s",
             (int)s->frames_left, (int)s->buffer, (int)s->granularity,
             (int)(s->buffer * s->granularity),
             aoe_frame_timer_report_ror(&s->frame_timer, 50));
    return out;
}

const char *aoe_comm_speed_optimal_text_ror(AoeCommSpeedRor *s, char *out,
                                            int size) {
    uint32_t buf, gran;
    if (!s || !out || size <= 0) return "";
    buf = aoe_comm_speed_optimal_buffer_ror(s);
    gran = aoe_comm_speed_optimal_granularity_ror(s);
    snprintf(out, (size_t)size,
             "MeOptimal: Buf=%3d Gran=%3d  Target FPS=%3d  My expect turn=%4dms",
             (int)buf, (int)gran, (int)(1000 / (int64_t)gran), (int)(gran * buf));
    return out;
}

void aoe_comm_sync_clear_ror(AoeCommSyncRor *s) {
    if (s) memset(s->records, 0, sizeof(s->records));
}

void aoe_comm_sync_init_ror(AoeCommSyncRor *s) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->interval = 1000;
    s->next_turn = 10;
    s->field_0c = 1;
    s->enabled = 1;
}

void aoe_comm_sync_set_extra_ror(AoeCommSyncRor *s, int32_t extra) {
    if (s) s->extra = extra;
}

static int same_values(const AoeCommSyncRecordRor *a,
                       const AoeCommSyncRecordRor *b) {
    return !memcmp(a->values, b->values, sizeof(a->values)) &&
           a->extra == b->extra;
}

int aoe_comm_sync_check_ror(AoeCommSyncRor *s, const AoeCommSyncRecordRor *r,
                            void (*out_of_sync)(void *, const AoeCommSyncRecordRor *),
                            void *context) {
    int i;
    if (!s || !r) return 0;
    if (!s->dirty) return s->in_sync;
    s->dirty = 0;
    s->in_sync = 0;
    for (i = 1; i < AOE_COMM_SYNC_RECORDS; ++i) {
        const AoeCommSyncRecordRor *k = &s->records[i];
        int bad = (k->turn == r->turn && k->world_time != r->world_time) ||
                  (k->world_time == r->world_time && !same_values(k, r));
        if (bad) {
            if (out_of_sync) out_of_sync(context, r);
            s->in_sync = 0;
            return 0;
        }
    }
    s->in_sync = 1;
    return 1;
}

int aoe_comm_sync_record_ror(AoeCommSyncRor *s, const AoeCommSyncRecordRor *r,
                             void (*out_of_sync)(void *, const AoeCommSyncRecordRor *),
                             void *context) {
    if (!s || !r || !s->enabled) return 0;
    if (++s->index > 53) s->index = 0;
    s->records[s->index] = *r;
    s->dirty = 1;
    return aoe_comm_sync_check_ror(s, r, out_of_sync, context);
}

int aoe_comm_sync_vote_ror(const AoeCommSyncRor *s, int32_t player,
                           int32_t turn, int32_t playing) {
    const AoeCommSyncRecordRor *mine = NULL;
    int i, same = 0, differ = 0;
    if (!s) return 0;
    for (i = 1; i < AOE_COMM_SYNC_RECORDS; ++i)
        if (s->records[i].player == player && s->records[i].turn == turn) {
            mine = &s->records[i];
            break;
        }
    if (!mine) return 0;
    for (i = 1; i < AOE_COMM_SYNC_RECORDS; ++i) {
        const AoeCommSyncRecordRor *k = &s->records[i];
        if (k->player == player || k->turn != turn) continue;
        if (k->world_time == mine->world_time && same_values(k, mine)) ++same;
        else ++differ;
    }
    if (same + differ < playing - 1) return 0;
    return same < differ;
}
