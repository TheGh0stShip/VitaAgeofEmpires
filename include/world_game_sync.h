#ifndef VITA_AOE_WORLD_GAME_SYNC_H
#define VITA_AOE_WORLD_GAME_SYNC_H

#include <stdint.h>

#include "comm_handler.h"

/* Patched Rise of Rome 1.0a: the comm handler's turn step 0x0041ebe0
   (thiscall on the comm object, one argument: the world's game time),
   called once per world update (0x0051d8f0 at 0x51d921: the comm_turn
   op in world_game.h). It answers how long the next world step may be:
   -1 free running (no multiplayer and not paused), 0 nothing to do yet,
   else the turn speed controller's granularity in ms.

   Comm fields read or written (AoeCommRor, original offsets):
   +0x1c is_host, +0xfc5 text (the not-responding report), +0x10c8 turn,
   +0x10e0 self, +0x10e4 multiplayer, +0x10e8 / +0x10ec field_10e8 /
   field_10ec (an end turn: 1 and a game time reached post 0x17b0),
   +0x12c8 names, +0x10f8 name, +0x12d0 field_12d0 (the last forced ping),
   +0x12d8 count, +0x1344 time_1344 (a slot's last message time),
   +0x136c field_136c (the last 0x17bd warning), +0x1394 fast_time,
   +0x149c field_149c (a slot timed out by the host), +0x1550 dirty,
   +0x1554 data_time, +0x1558 state (3 lobby, 4 paused, 5 running),
   +0x1590 lead, +0x16d4 player_state, +0x1710 / +0x1712 high / low,
   +0x1720 last_turn, +0x174c dropped; +0x14d4 is the speed controller
   (comm_speed.h) and +0x14d8 the sync checker.

   The comm's own ops give the clock (timeGetTime), the log (0x42c330 on
   0x00580db8), the window post (0x4200e0), the game message builder
   (0x422680(0, kind, data, size, 0)) and the game's strings (0x00580e38
   vtable +0x20). Translated predicates are called directly: 0x00423da0
   (aoe_comm_playing_ror) and 0x00421770 (aoe_comm_caught_up_ror). The
   rest are the ops below. */
typedef struct {
    void *context;
    /* Globals: 0x005817c8 the last status lines' time, 0x007d3bf0 the
       last turn marker's (0x44) time. */
    uint32_t *status_time;
    uint32_t *marker_time;
    /* 0x423e20(line): one of the six status lines (logged as a format). */
    const char *(*status_line)(void *context, int32_t line);
    /* 0x0041f330: the receive pump (aoe_comm_pump_ror). */
    void (*pump)(void *context);
    /* The speed controller (comm +0x14d4): 0x00428df0 the latency byte,
       0x004290f0 the frame average (its low byte is used), 0x00428b40 a
       slot's frame and turn time, 0x00428c30 frames left, 0x00428ce0 may
       a frame run, 0x00428c40 count, 0x00428cb0 a turn begins,
       0x00429140 the host's pace adjustment (granularity, buffer, apply),
       0x00428c10 the granularity taken, 0x00428dd0 the frame timer
       restarted. */
    uint32_t (*latency_byte)(void *context);
    uint32_t (*frame_average)(void *context);
    void (*set_player_rate)(void *context, uint32_t slot, uint8_t frame_ms, uint8_t turn_ms_10);
    uint32_t (*frames_left)(void *context);
    uint32_t (*frame)(void *context, uint32_t turn);
    void (*count)(void *context);
    void (*begin_turn)(void *context);
    int (*adjust)(void *context, uint32_t *granularity, uint32_t *buffer, int apply);
    uint32_t (*take_granularity)(void *context);
    void (*reset_frame)(void *context);
    /* The sync checker (comm +0x14d8): 0x004295b0(turn). */
    void (*sync_turn)(void *context, uint32_t turn);
    /* Comm methods: 0x00420530 the stop turns checked, 0x004213e0 chat to
       everyone, 0x004256c0 a slot dropped, 0x00424330(slot, 0, " ", " ")
       a slot initialised, 0x00424ee0 every active slot refreshed,
       0x0041f5e0 the quiet slots pinged (forced). */
    void (*check_stop_turn)(void *context, uint32_t turn);
    void (*chat_all)(void *context, const char *text);
    void (*drop_player)(void *context, uint32_t slot);
    void (*init_player)(void *context, uint32_t slot);
    void (*refresh_all)(void *context);
    void (*ping_quiet)(void *context, int force);
    /* 0x0050cef0 on the game's +0x3f4: the networked game's end report. */
    void (*end_report)(void *context);
} AoeCommTurnOpsRor;

/* 0x0041ebe0. Without multiplayer: -1, or 0 while paused (state 4). Our
   slot past the count: 0. Every 5 s or more (0x005817c8) the six status
   lines are logged (6 down to 1). The receive pump runs, our +0x1344 time
   becomes now; unless in the lobby (state 3) a turn not yet marked for
   our slot is marked (last_turn) and announced: past turn 6 our own frame
   and turn times are recorded from the latency byte and the frame
   average, 0x007d3bf0 is stamped and the 12-byte message 0x44 (the kind,
   then turn + lead at +4, the latency byte at +8 and the frame average
   at +9) is sent. The sync checker sees the turn. field_10e8 == 1 with
   game_time at or past field_10ec posts 0x17b0 (field_10ec) and gives 0.
   Frames left in the turn past turn 6: the controller's frame answer.
   Every playing slot caught up: running (state 5) begins the next turn
   (the host first adjusts the pace), logs "....Starting Turn#%d, WT=%d,
   Fr=%d, Gr=%d" (the adjusted granularity, then buffer, 0 when not),
   checks the stop turns and returns the granularity; other states give 0.
   Behind, below state 5 gives 0; else the other playing slots are timed
   from their +0x1344 time: silent over 25 s (and 25 s since the last
   warning) posts 0x17bd (slot); over 60 s counts as gone; a timeout (over
   98000 ms before turn 8; past turn 7 over 121500 ms for a host or over
   120500 ms) is logged, and a host reports "Player '%s' (%d) is not
   responding..." (string 0x4ba) into +0xfc5, chats it past turn 6,
   flags +0x149c and drops the slot; a timed-out slot's +0x1344 and
   +0x1394 times become now. When gone >= (others + 1) / 2 (with exactly
   two others: both; with none at all this holds too), we dropped
   ourselves: "I Dropped myself.", 0x17b2 (our slot) posted,
   every slot's dropped flag set, the end report, every slot initialised,
   the shared data clean (+0x1550, +0x1554) and the slots refreshed.
   Otherwise, 25 s past the later of +0x12d0 and the newest slot time,
   every slot is pinged and +0x12d0 renewed; the frame timer restarts.
   Returns 0 on those paths. */
int32_t aoe_comm_turn_ror(AoeCommRor *c, const AoeCommTurnOpsRor *o, uint32_t game_time);

#endif
