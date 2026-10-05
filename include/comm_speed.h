#ifndef VITA_AOE_COMM_SPEED_H
#define VITA_AOE_COMM_SPEED_H

#include <stdint.h>

#include "frame_timer.h"

/* Patched Rise of Rome 1.0a multiplayer turn timing (the comm handler's
   speed controller, 0x00428990..0x00429310). A communication turn lasts
   `buffer` frames of `granularity` ms; the host measures the players'
   frame times and latencies and schedules a new pace for a later turn.
   Player slots are 1..9. Semantic storage; comments give offsets. */
typedef struct {
    void *context;
    int (*active)(void *context, int32_t player);       /* 0x00423da0 */
    int (*is_host)(void *context);                      /* 0x00423c20 */
    int (*caught_up)(void *context);                    /* 0x00421770 */
    uint32_t (*turn)(void *context);                    /* comm +0x10c8 */
    uint8_t (*turn_lead)(void *context);                /* comm +0x1590 */
    void (*send_speed)(void *context, uint32_t buffer,
                       uint32_t granularity);           /* 0x00421640 */
} AoeCommSpeedOpsRor;

typedef struct {
    char text[0x100];                   /* +0x000 report buffer */
    AoeFrameTimerRor turn_timer;        /* +0x100 */
    AoeFrameTimerRor frame_timer;       /* +0x104 */
    AoeCommSpeedOpsRor ops;             /* +0x108 comm handler */
    int32_t enabled;                    /* +0x118, 1 */
    uint32_t buffer;                    /* +0x11c, 10 */
    uint32_t pending_buffer;            /* +0x120 */
    uint32_t frames_left;               /* +0x124 */
    uint32_t field_128;                 /* 500 */
    uint32_t granularity;               /* +0x12c, 90 ms */
    uint32_t pending_granularity;       /* +0x130 */
    uint32_t slack;                     /* +0x134 */
    uint32_t field_138;
    uint32_t pending_turn;              /* +0x140, 0 none */
    uint32_t latency[10];               /* +0x144, 30 */
    uint32_t field_16c[10];
    int16_t field_1a8[10];
    uint32_t frame_ms[10];              /* +0x1bc */
    uint32_t turn_ms[10];               /* +0x1e4 (reported value * 10) */
    uint32_t max_latency;               /* +0x20c, 500 */
    uint32_t field_210;                 /* +0x210 counter */
    uint32_t optimal_buffer;            /* +0x214, 10 */
    uint32_t optimal_granularity;       /* +0x218, 90 */
    uint32_t waited_frames;             /* +0x220 */
    uint32_t checks;                    /* +0x224 */
    uint32_t elapsed;                   /* +0x228 */
    uint32_t target;                    /* +0x22c */
    uint32_t late_total, late_turn;     /* +0x230, +0x234 */
    uint32_t changes;                   /* +0x238 */
    int32_t ready;                      /* +0x23c */
} AoeCommSpeedRor;

/* 0x00428990 (0x00428b00 frees the timers). */
void aoe_comm_speed_init_ror(AoeCommSpeedRor *s, const AoeCommSpeedOpsRor *ops,
                             uint32_t (*now)(void *), void *clock_context);
/* 0x00428b40: a player's frame time and turn time (byte * 10). */
void aoe_comm_speed_set_player_rate_ror(AoeCommSpeedRor *s, uint32_t player,
                                        uint8_t frame_ms, uint8_t turn_ms_10);
/* 0x00428b80 set (clamped to 30..6000) and 0x00428e60 get a latency. */
void aoe_comm_speed_set_latency_ror(AoeCommSpeedRor *s, uint32_t player,
                                    uint32_t ms);
uint32_t aoe_comm_speed_latency_ror(const AoeCommSpeedRor *s, uint32_t player);
/* 0x00428bc0: schedule a pace for a turn unless one is pending;
   0x00428c50 applies it once that turn is reached. */
void aoe_comm_speed_schedule_ror(AoeCommSpeedRor *s, uint32_t buffer,
                                 uint32_t granularity, uint32_t turn);
void aoe_comm_speed_apply_pending_ror(AoeCommSpeedRor *s);
/* 0x00428cb0: a communication turn begins (frames_left = buffer). */
void aoe_comm_speed_begin_turn_ror(AoeCommSpeedRor *s);
/* 0x00428ce0: may the next frame run? Returns the granularity when the
   time budget allows (one frame used), else 0; waits past frame 6 count
   as late. 0x00428dd0 restarts the frame timer. */
uint32_t aoe_comm_speed_frame_ror(AoeCommSpeedRor *s, uint32_t frame);
void aoe_comm_speed_reset_frame_ror(AoeCommSpeedRor *s);
/* 0x00428df0: the highest active latency (stored) as a 10 ms byte,
   1..255. */
uint32_t aoe_comm_speed_latency_byte_ror(AoeCommSpeedRor *s);
/* 0x00429080: frames to cover the highest latency, 5..21; 0x00429100:
   the average of the last 50 frame times, 10..150 ms; 0x004290f0 that
   average unclamped. */
uint32_t aoe_comm_speed_optimal_buffer_ror(AoeCommSpeedRor *s);
uint32_t aoe_comm_speed_optimal_granularity_ror(AoeCommSpeedRor *s);
uint32_t aoe_comm_speed_frame_average_ror(AoeCommSpeedRor *s);
/* 0x00429140: the host's pace adjustment (see the source); 1 with the
   new buffer and granularity, scheduled 20+ turns ahead and sent when
   `apply` is set and adjusting is enabled. */
int aoe_comm_speed_adjust_ror(AoeCommSpeedRor *s, uint32_t *granularity,
                              uint32_t *buffer, int apply);
/* Small accessors: 0x00428bf0 enabled, 0x00428c00 changes, 0x00428c10
   (marks ready, returns the granularity), 0x00428c30 frames left,
   0x00428c40 counts +0x210. */
void aoe_comm_speed_set_enabled_ror(AoeCommSpeedRor *s, int32_t enabled);
uint32_t aoe_comm_speed_changes_ror(const AoeCommSpeedRor *s);
uint32_t aoe_comm_speed_take_granularity_ror(AoeCommSpeedRor *s);
uint32_t aoe_comm_speed_frames_left_ror(const AoeCommSpeedRor *s);
void aoe_comm_speed_count_ror(AoeCommSpeedRor *s);
/* Debug text: 0x00428e80 "Latency> P%d=%d ..." (into s->text),
   0x00428fd0 "PlrSpeed> P#%1d(fr%d lat%d) ...", 0x00428f70 the turn line
   (player 0..9, else NULL), 0x00428f30 the optimal pace line. */
const char *aoe_comm_speed_latency_text_ror(AoeCommSpeedRor *s);
const char *aoe_comm_speed_rate_text_ror(AoeCommSpeedRor *s, char *out,
                                         int size);
const char *aoe_comm_speed_turn_text_ror(AoeCommSpeedRor *s, int32_t player,
                                         char *out, int size);
const char *aoe_comm_speed_optimal_text_ror(AoeCommSpeedRor *s, char *out,
                                            int size);

/* Patched RoR 1.0a sync checker (0x00429330..0x00429d10): a ring of
   per-turn sync records (player, world time, turn, five checksums and a
   value) gathered from every player, compared each time a new one comes
   in. Slot 0 is written when the ring index wraps but never compared, and
   slot 54 is never written. */
enum { AOE_COMM_SYNC_RECORDS = 55 };

typedef struct {
    int32_t player, world_time, turn;
    int32_t values[5];  /* +0x0c..+0x1c */
    int32_t extra;      /* +0x20 */
} AoeCommSyncRecordRor;

typedef struct {
    int32_t interval;           /* +0x04, 1000 */
    uint32_t next_turn;         /* +0x08, 10 */
    int32_t field_0c;           /* 1 */
    int32_t field_10, field_14;
    int32_t dirty;              /* +0x18 */
    int32_t in_sync;            /* +0x1c */
    uint32_t index;             /* +0x20 */
    int32_t enabled;            /* +0x24, 1 */
    int32_t extra;              /* +0x28 (0x004296e0) */
    int32_t field_2c;
    AoeCommSyncRecordRor records[AOE_COMM_SYNC_RECORDS]; /* +0x30 */
} AoeCommSyncRor;

/* 0x00429330 constructor, 0x004293b0 clears the records, 0x004296e0 sets
   the value carried by this player's records. */
void aoe_comm_sync_init_ror(AoeCommSyncRor *s);
void aoe_comm_sync_clear_ror(AoeCommSyncRor *s);
void aoe_comm_sync_set_extra_ror(AoeCommSyncRor *s, int32_t extra);
/* 0x004293e0: store a record (0 when disabled) and check it (0x004294c0):
   out of sync when a stored record has the same turn but another world
   time, or the same world time but other values. Returns 1 in sync. The
   original then dumps C:\aoesync.txt (0x004296f0) and shows message
   0x17b0; `out_of_sync` is called instead. */
int aoe_comm_sync_record_ror(AoeCommSyncRor *s, const AoeCommSyncRecordRor *r,
                             void (*out_of_sync)(void *, const AoeCommSyncRecordRor *),
                             void *context);
int aoe_comm_sync_check_ror(AoeCommSyncRor *s, const AoeCommSyncRecordRor *r,
                            void (*out_of_sync)(void *, const AoeCommSyncRecordRor *),
                            void *context);
/* 0x00429d10: is `player` the odd one out at `turn`? Its record is
   compared with every other player's; 1 when more differ than agree and
   at least playing players - 1 reported (`playing` counts slots 1..9 in
   state 2). */
int aoe_comm_sync_vote_ror(const AoeCommSyncRor *s, int32_t player,
                           int32_t turn, int32_t playing);

#endif
