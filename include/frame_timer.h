#ifndef VITA_AOE_FRAME_TIMER_H
#define VITA_AOE_FRAME_TIMER_H

#include <stddef.h>
#include <stdint.h>

/* Patched Rise of Rome 1.0a interval timer (0x0046bb00..0x0046bc70, a
   0x2b4-byte object): the last 100 intervals between marks in a ring
   (slots 1..100; slot 0 is written when the index wraps but the averages
   never read it), the shortest and longest, and a 0x100-byte report
   buffer at the start. Times come from timeGetTime through `now`. */
typedef struct {
    char report[0x100];        /* +0x000 */
    uint32_t shortest;         /* +0x100, 99999 */
    uint32_t longest;          /* +0x104 */
    uint32_t last;             /* +0x108 */
    uint32_t samples[101];     /* +0x10c */
    int32_t index;             /* +0x2a0 */
    uint32_t mark_ms;          /* +0x2a4 */
    uint32_t start_ms[2];      /* +0x2a8, +0x2ac */
    int32_t over_second;       /* +0x2b0 */
    uint32_t (*now)(void *context);
    void *context;
} AoeFrameTimerRor;

/* 0x0046bb00. */
void aoe_frame_timer_init_ror(AoeFrameTimerRor *t,
                              uint32_t (*now)(void *), void *context);
/* 0x0046bb50: the average of the last n (1..100) intervals (0 outside);
   when the running sum passes 999 ms the step is noted in over_second. */
uint32_t aoe_frame_timer_average_ror(AoeFrameTimerRor *t, int32_t n);
/* 0x0046bba0: milliseconds since the last mark. */
uint32_t aoe_frame_timer_elapsed_ror(const AoeFrameTimerRor *t);
/* 0x0046bbc0: record the interval since the last mark (0, and nothing
   recorded, when no time passed). */
uint32_t aoe_frame_timer_mark_ror(AoeFrameTimerRor *t);
/* 0x0046bc50: restart from now without recording. */
void aoe_frame_timer_restart_ror(AoeFrameTimerRor *t);
/* 0x0046bc70: "Lo=%d < Avg=%d > Hi=%d of %d" over the last n intervals,
   into the report buffer (outside 1..100 it prints n for all four). */
const char *aoe_frame_timer_report_ror(AoeFrameTimerRor *t, int32_t n);

#endif
