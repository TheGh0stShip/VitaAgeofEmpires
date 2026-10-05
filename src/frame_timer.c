#include "frame_timer.h"

#include <stdio.h>
#include <string.h>

static uint32_t now(const AoeFrameTimerRor *t) {
    return t->now ? t->now(t->context) : 0;
}

void aoe_frame_timer_init_ror(AoeFrameTimerRor *t,
                              uint32_t (*clock)(void *), void *context) {
    uint32_t ms;
    if (!t) return;
    memset(t, 0, sizeof(*t));
    t->now = clock;
    t->context = context;
    t->shortest = 99999;
    ms = now(t);
    t->start_ms[0] = t->start_ms[1] = t->mark_ms = ms;
}

uint32_t aoe_frame_timer_average_ror(AoeFrameTimerRor *t, int32_t n) {
    uint32_t sum = 0;
    int32_t i, at;
    if (!t || n <= 0 || n >= 101) return 0;
    at = t->index;
    for (i = 0; i < n; ++i) {
        if (--at < 1) at = 100;
        sum += t->samples[at];
        if (sum > 999) t->over_second = i;
    }
    return sum / (uint32_t)n;
}

uint32_t aoe_frame_timer_elapsed_ror(const AoeFrameTimerRor *t) {
    return t ? now(t) - t->mark_ms : 0;
}

uint32_t aoe_frame_timer_mark_ror(AoeFrameTimerRor *t) {
    uint32_t ms, delta;
    if (!t) return 0;
    ms = now(t);
    if (ms == t->mark_ms) return 0;
    delta = ms - t->mark_ms;
    t->mark_ms = ms;
    t->last = delta;
    if (++t->index > 100) t->index = 0;
    t->samples[t->index] = delta;
    if (delta < t->shortest) t->shortest = delta;
    if (delta > t->longest) t->longest = delta;
    return delta;
}

void aoe_frame_timer_restart_ror(AoeFrameTimerRor *t) {
    if (t) t->mark_ms = now(t);
}

const char *aoe_frame_timer_report_ror(AoeFrameTimerRor *t, int32_t n) {
    uint32_t lo, avg, hi, sum = 0;
    if (!t) return "";
    lo = avg = hi = (uint32_t)n;
    if (n > 0 && n < 101) {
        int32_t i, at = t->index;
        lo = 999999;
        hi = 0;
        for (i = 0; i < n; ++i) {
            uint32_t v;
            if (--at < 1) at = 100;
            v = t->samples[at];
            sum += v;
            if (v < lo) lo = v;
            if (v > hi) hi = v;
        }
        avg = sum / (uint32_t)n;
    }
    snprintf(t->report, sizeof t->report, "Lo=%d < Avg=%d > Hi=%d of %d",
             (int)lo, (int)avg, (int)hi, (int)n);
    return t->report;
}
