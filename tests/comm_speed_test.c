/* Patched RoR 1.0a interval timer 0x0046bb00..0x0046bc70, turn timing
   0x00428990..0x00429140 and sync checker 0x00429330..0x00429d10. */
#include "comm_speed.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t clock_ms;
static uint32_t now(void *c) { (void)c; return clock_ms; }

static int playing[10];
static uint32_t turn_no;
static int caught;
static uint32_t sent_buf, sent_gran;
static int act(void *c, int32_t p) { (void)c; return p > 0 && p < 10 && playing[p]; }
static int host(void *c) { (void)c; return 1; }
static int up(void *c) { (void)c; return caught; }
static uint32_t turn(void *c) { (void)c; return turn_no; }
static uint8_t lead(void *c) { (void)c; return 2; }
static void send(void *c, uint32_t b, uint32_t g) { (void)c; sent_buf = b; sent_gran = g; }

static int bad_calls;
static void bad(void *c, const AoeCommSyncRecordRor *r) { (void)c; (void)r; ++bad_calls; }

/* Slots 1 (us, player 1), 2 (player 2, a computer), 3 (player 3, +0xa15
   set), 4 (player 4). */
static int32_t st_slot_player(void *c, int32_t slot) { (void)c; return slot <= 4 ? slot : 0; }
static int32_t st_kind(void *c, int32_t p) { (void)c; return p == 2; }
static uint8_t st_8db8(void *c) { (void)c; return 1; }
static uint8_t st_8db9(void *c) { (void)c; return 0; }
static uint8_t st_60(void *c) { (void)c; return 0; }
static uint8_t st_64(void *c) { (void)c; return 1; }
static uint8_t st_a15(void *c, int32_t slot) { (void)c; return slot == 3; }
static void st_sums(void *c, uint32_t *a, int32_t *b, int32_t *d) { (void)c; *a = 11; *b = 12; *d = 13; }
static AoeCommSyncRecordRor st_sent;
static void st_send(void *c, const AoeCommSyncRecordRor *r) { (void)c; st_sent = *r; }

static void sync_turn_tests(void) {
    static AoeCommSyncRor s;
    AoeCommSyncTurnOpsRor o;
    memset(&o, 0, sizeof o);
    o.self_slot = 1;
    o.slot_player = st_slot_player;
    o.player_kind = st_kind;
    o.map_8db8 = st_8db8;
    o.map_8db9 = st_8db9;
    o.game_60 = st_60;
    o.game_64 = st_64;
    o.slot_a15 = st_a15;
    o.world_7c = 20;
    o.has_world = 1;
    o.world_time = 5000;
    o.checksums = st_sums;
    o.settings_982 = 1;
    o.global_sum = 100;
    o.send = st_send;
    /* Bits: slot 1 = 2, slot 3 = 2 << 4, slot 4 = 1 << 6. */
    assert(aoe_comm_sync_slot_bits_ror(&o) == (2u | 2u << 4 | 1u << 6));
    /* Sum: 20 (us) + 3 + 4. */
    assert(aoe_comm_sync_slot_sum_ror(&o) == 27);
    aoe_comm_sync_init_ror(&s);
    assert(aoe_comm_sync_turn_ror(&s, 9, &o, NULL, NULL) == 0);
    assert(aoe_comm_sync_turn_ror(&s, 10, &o, NULL, NULL) == 1 && s.next_turn == 50);
    assert(st_sent.player == 1 && st_sent.turn == 10 && st_sent.world_time == 5000 &&
           st_sent.values[2] == 11 && st_sent.values[4] == 13 &&
           (uint32_t)st_sent.extra == ((2u | 2u << 4 | 1u << 6) | (1u << 15 | 27u) << 16) + 100u);
}

int main(void) {
    sync_turn_tests();
    static AoeFrameTimerRor t;
    static AoeCommSpeedRor s;
    static AoeCommSyncRor y;
    AoeCommSpeedOpsRor ops = {NULL, act, host, up, turn, lead, send};
    uint32_t g, b;
    char line[160];
    int i;

    /* Timer ring: marks record intervals; averages walk back from the
       index, skipping slot 0. */
    clock_ms = 1000;
    aoe_frame_timer_init_ror(&t, now, NULL);
    assert(t.shortest == 99999 && t.mark_ms == 1000);
    assert(aoe_frame_timer_mark_ror(&t) == 0 && t.index == 0);
    clock_ms = 1040;
    assert(aoe_frame_timer_mark_ror(&t) == 40 && t.index == 1);
    clock_ms = 1100;
    assert(aoe_frame_timer_mark_ror(&t) == 60);
    /* The newest interval (at the index) is not part of the average. */
    assert(aoe_frame_timer_average_ror(&t, 1) == 40 &&
           aoe_frame_timer_average_ror(&t, 2) == 20 &&
           t.shortest == 40 && t.longest == 60);
    assert(aoe_frame_timer_average_ror(&t, 0) == 0 && aoe_frame_timer_average_ror(&t, 101) == 0);
    clock_ms = 1125;
    assert(aoe_frame_timer_elapsed_ror(&t) == 25);
    assert(!strcmp(aoe_frame_timer_report_ror(&t, 2), "Lo=0 < Avg=20 > Hi=40 of 2"));
    assert(!strcmp(aoe_frame_timer_report_ror(&t, 0), "Lo=0 < Avg=0 > Hi=0 of 0"));
    for (i = 0; i < 99; ++i) { clock_ms += 10; aoe_frame_timer_mark_ror(&t); }
    assert(t.index == 0 && t.samples[0] == 10); /* wrapped into slot 0 */
    aoe_frame_timer_average_ror(&t, 100);
    assert(t.over_second == 99);

    /* Turn timing. */
    clock_ms = 0;
    aoe_comm_speed_init_ror(&s, &ops, now, NULL);
    assert(s.buffer == 10 && s.granularity == 90 && s.latency[3] == 30);
    aoe_comm_speed_set_latency_ror(&s, 1, 10);
    aoe_comm_speed_set_latency_ror(&s, 2, 9000);
    aoe_comm_speed_set_latency_ror(&s, 0, 400);
    assert(s.latency[1] == 30 && aoe_comm_speed_latency_ror(&s, 2) == 6000 &&
           aoe_comm_speed_latency_ror(&s, 0) == 0);
    playing[1] = playing[2] = 1;
    assert(aoe_comm_speed_latency_byte_ror(&s) == 255 && s.max_latency == 6000);
    aoe_comm_speed_set_latency_ror(&s, 2, 455);
    assert(aoe_comm_speed_latency_byte_ror(&s) == 46);
    assert(aoe_comm_speed_optimal_buffer_ror(&s) == 6);   /* 455/90 = 5 */
    aoe_comm_speed_set_latency_ror(&s, 2, 60);
    assert(aoe_comm_speed_optimal_buffer_ror(&s) == 5);   /* 0 -> 4, + 1 */
    assert(!strcmp(aoe_comm_speed_latency_text_ror(&s), "Latency> P1=30 P2=60 "));

    /* A turn: 10 frames of 90 ms; a frame may run once its share of the
       turn has elapsed. */
    aoe_comm_speed_begin_turn_ror(&s);
    assert(s.frames_left == 10);
    assert(aoe_comm_speed_frame_ror(&s, 1) == 90 && s.frames_left == 9);
    assert(aoe_comm_speed_frame_ror(&s, 7) == 0 && s.late_total == 1); /* 90 ms due */
    clock_ms = 95;
    assert(aoe_comm_speed_frame_ror(&s, 2) == 90 && s.frames_left == 8);
    caught = 1;
    clock_ms = 180;
    assert(aoe_comm_speed_frame_ror(&s, 3) == 90 && s.waited_frames == 8);

    /* Pace adjustment: frame times 46 and 23 ms -> 50 ms; turn times 400
       ms -> 10 frames; 500 ms is far enough below 10 x 90 that the
       buffer only drops by one. */
    aoe_comm_speed_set_player_rate_ror(&s, 1, 46, 40);
    aoe_comm_speed_set_player_rate_ror(&s, 2, 23, 10);
    turn_no = 30;
    for (i = 0; i < 4; ++i) assert(!aoe_comm_speed_adjust_ror(&s, &g, &b, 1));
    assert(aoe_comm_speed_adjust_ror(&s, &g, &b, 1));
    assert(g == 50 && b == 9 && s.pending_turn == 32 && sent_buf == 9 && sent_gran == 50);
    assert(!aoe_comm_speed_adjust_ror(&s, &g, &b, 1)); /* one pending */
    aoe_comm_speed_apply_pending_ror(&s);
    assert(s.granularity == 90 && s.checks == 0);
    turn_no = 32;
    aoe_comm_speed_begin_turn_ror(&s);
    assert(s.granularity == 50 && s.buffer == 9 && s.changes == 1 && s.frames_left == 9);
    /* Short turns are padded to 250 ms (26 frames of 10 ms), but a much
       shorter turn again only steps the buffer down by one. */
    s.checks = 4;
    aoe_comm_speed_set_player_rate_ror(&s, 1, 10, 1);
    aoe_comm_speed_set_player_rate_ror(&s, 2, 10, 1);
    assert(aoe_comm_speed_adjust_ror(&s, &g, &b, 0) && g == 10 && b == 8);
    assert(!s.pending_turn);
    {
        const char *want = "Buf=  9 /   9 * Gran= 50ms = turn  450ms   Lo=";
        assert(!strncmp(aoe_comm_speed_turn_text_ror(&s, 1, line, sizeof line),
                        want, strlen(want)));
    }
    assert(!aoe_comm_speed_turn_text_ror(&s, 10, line, sizeof line));
    assert(!strcmp(aoe_comm_speed_rate_text_ror(&s, line, sizeof line),
                   "PlrSpeed> P#1(fr10 lat10) P#2(fr10 lat10) "));

    /* Sync records. */
    aoe_comm_sync_init_ror(&y);
    assert(y.interval == 1000 && y.next_turn == 10 && y.enabled);
    {
        AoeCommSyncRecordRor a = {1, 500, 40, {1, 2, 3, 4, 5}, 0};
        AoeCommSyncRecordRor b2 = a, c = a;
        b2.player = 2;
        assert(aoe_comm_sync_record_ror(&y, &a, bad, NULL) && y.index == 1);
        assert(aoe_comm_sync_record_ror(&y, &b2, bad, NULL));
        c.player = 3;
        c.values[2] = 99;
        assert(!aoe_comm_sync_record_ror(&y, &c, bad, NULL) && bad_calls == 1);
        /* Player 3 differs from two agreeing players. */
        assert(aoe_comm_sync_vote_ror(&y, 3, 40, 3) == 1);
        assert(aoe_comm_sync_vote_ror(&y, 1, 40, 3) == 0);
        assert(aoe_comm_sync_vote_ror(&y, 1, 40, 5) == 0); /* too few reports */
        assert(aoe_comm_sync_vote_ror(&y, 7, 40, 3) == 0);
        c.world_time = 501; /* same turn, other time */
        c.values[2] = 3;
        assert(!aoe_comm_sync_record_ror(&y, &c, bad, NULL) && bad_calls == 2);
        y.enabled = 0;
        assert(!aoe_comm_sync_record_ror(&y, &a, bad, NULL) && y.index == 4);
        aoe_comm_sync_clear_ror(&y);
        assert(y.records[1].turn == 0);
    }
    return 0;
}
