#include "comm_handler_dispatch.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Patched RoR 1.0a 0x00422ce0, one unlink step. Executable evidence:
   0x00422d1d cmp eax,0xf / ja 0x0042372f and the jump table at
   0x00423778: 0,1 -> 0x00423743 (the tail), 2 -> 0x00422d2d,
   3 -> 0x00422f9b, 4 -> 0x00423207, 5 -> 0x0042325a, 6 -> 0x004232d8,
   7 -> 0x0042346e, 8 -> 0x0042372f ("UNLINK Unknown" 0x00556148),
   9 -> 0x004235f2, 10 -> 0x0042368b, 11 -> 0x00423630, 12 -> 0x0042365d,
   13 -> 0x004236bb, 14 -> 0x004236e5, 15 -> 0x00423722. The tail
   0x00423743 logs "Level is now...(%d)" (0x00556134) with +0x12d4 - 1
   and returns 0x00423870's EAX. 0x00554cf0 holds 20 00, a one-space
   string. */

static int dp_on, self_id, releases, closes, destroys, chat_clears, elapsed, flushes[2];
static int speed_new, speed_del, sync_new, sync_del, sync_fails;
static void *speed_deleted, *sync_deleted;
static int speed_obj[2], sync_obj[2];
static uint32_t clock_ms = 5000;
static char logs[96][96];
static int nlogs;

static int dp_present(void *c) { (void)c; return dp_on; }
static int32_t self_dpid(void *c) { (void)c; return self_id; }
static int32_t message(void *c, uint8_t k, const void *d, uint32_t n) { (void)c; (void)k; (void)d; (void)n; return 0; }
static void post(void *c, uint32_t w, uint32_t l) { (void)c; (void)w; (void)l; }
static void log_(void *c, const char *t) {
    (void)c;
    if (nlogs < 96) snprintf(logs[nlogs++], sizeof logs[0], "%s", t);
}
static void report(void *c, int32_t hr, const char *t) { (void)c; (void)hr; (void)t; }
static void drop(void *c, uint32_t s, int32_t d) { (void)c; (void)s; (void)d; }
static uint32_t now(void *c) { (void)c; return clock_ms; }

static const AoeCommOpsRor ops = {
    .context = NULL, .dp_present = dp_present, .self_dpid = self_dpid, .message = message,
    .post = post, .log = log_, .report = report, .drop = drop, .now_ms = now};

/* DirectPlay: 0x00425660 releases the interface (+8) and forgets it;
   0x004255e0 closes the session (+0x10); 0x004257a0 destroys our player
   (+0x24) and on success clears 0x005817bc. */
static int32_t d_release(void *c) { (void)c; releases++; dp_on = 0; return 0; }
static int32_t d_close(void *c) { (void)c; closes++; return 0; }
static int32_t d_destroy(void *c, int32_t id) { (void)c; assert(id == self_id); destroys++; return 0; }
static void d_self(void *c, int32_t id) { (void)c; self_id = id; }
static void d_chat_clear(void *c) { (void)c; chat_clears++; }

static AoeCommRor C;
static AoeCommUnlinkRor U;
static AoeCommTxRor tx[AOE_COMM_TX_SLOTS];
static AoeCommRxRor rx[AOE_COMM_RX_SLOTS];

static int32_t d_unlink(void *c);
static const AoeCommDpOpsRor dops = {
    .context = NULL, .release = d_release, .close = d_close, .destroy_player = d_destroy,
    .set_self_dpid = d_self, .unlink_step = d_unlink, .chat_clear = d_chat_clear};
/* The loop 0x004237c0 calls 0x00422ce0 through this. */
static int32_t d_unlink(void *c) { (void)c; return aoe_comm_unlink_step_ror(&C, &dops, &U); }

static void u_elapsed(void *c) { (void)c; elapsed++; }
static void u_flush(void *c, int which) { (void)c; assert(which == 0 || which == 1); flushes[which]++; }
static void *u_speed_new(void *c) { (void)c; return &speed_obj[speed_new++ & 1]; }
static void u_speed_del(void *c, void *p) { (void)c; speed_del++; speed_deleted = p; }
static void *u_sync_new(void *c) { (void)c; sync_new++; return sync_fails ? NULL : &sync_obj[sync_new & 1]; }
static void u_sync_del(void *c, void *p) { (void)c; sync_del++; sync_deleted = p; }

static int logged(const char *t) {
    int i;
    for (i = 0; i < nlogs; ++i)
        if (strcmp(logs[i], t) == 0) return 1;
    return 0;
}

static void fresh(int32_t level) {
    memset(&C, 0, sizeof C);
    memset(tx, 0, sizeof tx);
    memset(rx, 0, sizeof rx);
    C.ops = &ops;
    C.tx = tx;
    C.rx = rx;
    C.count = 4;
    C.status = level;
    memset(&U, 0, sizeof U);
    U.log_elapsed = u_elapsed;
    U.flush_queue = u_flush;
    U.speed_create = u_speed_new;
    U.speed_destroy = u_speed_del;
    U.sync_create = u_sync_new;
    U.sync_destroy = u_sync_del;
    dp_on = self_id = releases = closes = destroys = chat_clears = elapsed = 0;
    flushes[0] = flushes[1] = 0;
    speed_new = speed_del = sync_new = sync_del = sync_fails = 0;
    speed_deleted = sync_deleted = NULL;
    nlogs = 0;
}

/* Level 3 (0x00422f9b): the inlined reset 0x00422a70 (no 0x004257a0), then
   +0x10e0 = 1 (0x004231f2) and +0x16d8 = 2 (0x004231f8). */
static void level3_test(void) {
    int32_t r;
    fresh(3);
    C.multiplayer = 1; /* link level 3: no DirectPlay, multiplayer */
    C.gtd = 1;
    C.turn = 9;
    C.lead = 5;
    C.state = 3;
    C.field_10dc = 7;
    C.low = 2;
    C.high = 2;
    C.stop_turn[2] = 77;
    C.field_13c0[1] = 77;
    C.dropped[3] = 1;
    C.field_149c[2] = 1;
    C.field_136c[4] = 1;
    C.player_state[2] = 5;
    C.dpid[2] = 222;
    C.rx_serial[2] = 5;
    C.ack_serial[3] = 6;
    tx[3].serial = 7;
    rx[4].size = 9;
    C.tx_pending = 3;
    C.field_12a8 = 1;
    C.rx_bytes = 100;
    C.lobby_joined = 1;
    strcpy(C.name, "Bob");
    strcpy(C.info, "x");
    strcpy(C.label, "Bob [x]");
    U.has_queue_a = U.has_queue_b = 1;
    r = aoe_comm_unlink_step_ror(&C, &dops, &U);
    assert(r == 2);
    assert(C.multiplayer == 0 && C.self == 1 && C.player_state[1] == 2);
    assert(C.player_state[2] == 0 && C.dpid[2] == 0 && C.rx_serial[2] == 0 && C.ack_serial[3] == 0);
    assert(strcmp(C.name, " ") == 0 && strcmp(C.info, " ") == 0 && strcmp(C.label, " ") == 0);
    assert(C.state == 3 && C.lead == 2 && C.turn == 4 && C.field_155c == 4);
    assert(C.field_10dc == 0x32 && C.low == 1 && C.high == 4 && C.gtd == 0);
    assert(C.stop_turn[2] == 0 && C.field_13c0[1] == 0 && C.dropped[3] == 0);
    assert(C.field_149c[2] == 0 && C.field_136c[4] == 0);
    assert(tx[3].serial == 0 && rx[4].size == 0 && C.tx_pending == 0 && C.field_12a8 == 0);
    assert(C.field_8 == 1 && C.time_1344[4] == 5000 && C.fast_time[1] == 5000);
    assert(C.data_time == 0 && C.rx_bytes == 0 && C.lobby_joined == 0);
    assert(elapsed == 1 && flushes[0] == 1 && flushes[1] == 1 && releases == 0 && destroys == 0);
    assert(strcmp(logs[0], "Unlinking current level (3)") == 0);
    assert(logged("Initialize Player List"));
    assert(strcmp(logs[nlogs - 1], "Level is now...(2)") == 0);
}

/* Level 2 (0x00422d2d): the same reset, then +0x10e0 = 1 (0x00422f84) and
   the name / info first bytes 0 (0x00422f8a / 0x00422f90); the label
   keeps the space. A missing queue (+0x14c8 NULL) is not flushed. */
static void level2_test(void) {
    fresh(2);
    C.self = 1;
    C.player_state[1] = 5;
    U.has_queue_a = 1;
    assert(aoe_comm_unlink_step_ror(&C, &dops, &U) == 2);
    assert(C.self == 1 && C.player_state[1] == 0);
    assert(C.name[0] == 0 && C.info[0] == 0 && strcmp(C.label, " ") == 0);
    assert(flushes[0] == 1 && flushes[1] == 0);
    assert(strcmp(logs[nlogs - 1], "Level is now...(1)") == 0);
}

/* Level 4 (0x00423207): guaranteed delivery ON (+0x1514 = 1 at
   0x0042321a), the counters cleared, DirectPlay released (0x00425660,
   which nulls +0x12dc from +0x1318). */
static void level4_test(void) {
    fresh(4);
    dp_on = 1;
    C.multiplayer = 1;
    C.guid_12dc[0] = 0x55;
    C.field_1328 = C.field_132c = C.field_1330 = 3;
    C.field_1338 = C.rx_messages = 4;
    C.lobby_joined = 1;
    C.field_12a8 = 1;
    assert(aoe_comm_unlink_step_ror(&C, &dops, &U) == 3);
    assert(C.gtd == 1 && releases == 1 && dp_on == 0 && C.guid_12dc[0] == 0);
    assert(C.field_1328 == 0 && C.field_132c == 0 && C.field_1330 == 0);
    assert(C.field_1338 == 0 && C.rx_messages == 0 && C.lobby_joined == 0 && C.field_12a8 == 0);
    assert(logged("UNLINK ReleaseComm") && strcmp(logs[nlogs - 1], "Level is now...(3)") == 0);
}

/* Level 5 (0x0042325a): released, the player list, the chat log (+0x18)
   dropped, the label emptied, then 0x00423646: +0x10e0, 0x005817bc and
   +0x12a8 zero. */
static void level5_test(void) {
    fresh(5);
    dp_on = 1;
    self_id = 50;
    C.self = 3;
    C.dpid[2] = 222;
    C.field_12a8 = 1;
    strcpy(C.label, "Bob [x]");
    assert(aoe_comm_unlink_step_ror(&C, &dops, &U) == 1);
    assert(releases == 1 && chat_clears == 1 && C.label[0] == 0 && C.dpid[2] == 0);
    assert(C.self == 0 && self_id == 0 && C.field_12a8 == 0);
    assert(logged("UNLINK ServiceAvailable") && logged("Initialize Player List"));
}

/* Level 6 (0x004232d8): the session closed (+0x12fc nulled by 0x004255e0),
   the stores emptied, the player list, the lobby values, our IDs 0, the
   speed controller and sync checker recreated (0x004233a2..0x0042343d),
   +0x12fc = +0x1318 (0x00423443). */
static void level6_test(void) {
    void *old_speed = &speed_obj[1], *old_sync = &sync_obj[0];
    fresh(6);
    dp_on = 1;
    C.multiplayer = 1;
    C.guid_12fc[0] = 0x66;
    C.guid_12dc[0] = 0x55;
    C.turn = 9;
    C.lead = 5;
    C.state = 4;
    C.self = 2;
    C.field_14e4 = 9;
    C.field_1718 = 1;
    C.last_turn[3] = 40;
    tx[1].serial = 4;
    U.speed = old_speed;
    U.sync = old_sync;
    assert(aoe_comm_unlink_step_ror(&C, &dops, &U) == 5);
    assert(closes == 1 && chat_clears == 1 && releases == 0 && C.guid_12fc[0] == 0);
    assert(C.guid_12dc[0] == 0x55 && C.last_turn[3] == 0 && tx[1].serial == 0);
    assert(C.turn == 4 && C.lead == 2 && C.state == 3 && C.field_155c == 4 && C.high == 4);
    assert(C.self == 0 && self_id == 0 && C.field_14e4 == 0 && C.field_1718 == 0);
    assert(speed_del == 1 && speed_deleted == old_speed && speed_new == 1);
    assert(sync_del == 1 && sync_deleted == old_sync && sync_new == 1);
    assert(U.speed == &speed_obj[0] && U.sync == &sync_obj[1]);
    assert(logged("UNLINK SessionAvailable"));
}

/* Level 7 (0x0042346e): hosting off, +0x1714 0, the session closed, the
   turn set again to lead + 2 (0x00423526..0x0042353d), the objects
   recreated; a failed operator new leaves +0x14d8 NULL (0x004235e1). An
   absent speed controller is not destroyed (0x0042354f). */
static void level7_test(void) {
    fresh(7);
    dp_on = 1;
    C.is_host = 1;
    C.field_1714 = 1;
    C.field_20 = 1;
    C.turn = 3;
    C.self = 1;
    self_id = 9;
    C.guid_12dc[0] = 0x55;
    sync_fails = 1;
    U.sync = &sync_obj[0];
    assert(aoe_comm_unlink_step_ror(&C, &dops, &U) == 5);
    assert(C.is_host == 0 && C.field_1714 == 0 && C.field_20 == 0 && closes == 1);
    assert(C.turn == 4 && C.lead == 2 && C.state == 3 && C.self == 0 && self_id == 0);
    assert(chat_clears == 1 && speed_del == 0 && speed_new == 1 && U.speed == &speed_obj[0]);
    assert(sync_del == 1 && sync_new == 1 && U.sync == NULL);
    assert(logged("UNLINK SerssionHosted"));
}

/* Level 9 (0x004235f2): our player destroyed (0x004257a0), the first
   queue flushed with no NULL test (0x0042361a). Level 10 (0x0042368b):
   dirty off, the ping times. Level 11 (0x00423630): our IDs 0. */
static void levels_9_to_11_test(void) {
    fresh(9);
    dp_on = 1;
    self_id = 321;
    C.self = 2;
    C.field_12a8 = 1;
    assert(aoe_comm_unlink_step_ror(&C, &dops, &U) == 4);
    assert(destroys == 1 && self_id == 0 && C.self == 0 && flushes[0] == 1 && flushes[1] == 0);
    assert(C.field_12a8 == 0 && logged("UNLINK PlayerCreated"));

    fresh(10);
    C.dirty = 1;
    C.field_12a8 = 1;
    assert(aoe_comm_unlink_step_ror(&C, &dops, &U) == 1);
    assert(C.dirty == 0 && C.time_1344[3] == 5000 && C.fast_time[4] == 5000 && C.field_12a8 == 0);
    assert(logged("UNLINK CommSettingsExcahanged"));

    fresh(11);
    self_id = 8;
    C.self = 1;
    C.field_12a8 = 1;
    assert(aoe_comm_unlink_step_ror(&C, &dops, &U) == 1);
    assert(self_id == 0 && C.self == 0 && C.field_12a8 == 0 && logged("UNLINK PlayerSet"));
}

/* Level 12 (0x0042365d): the extra data freed (0x004258a0) and dirty off.
   13 (0x004236bb): state 3. 14 (0x004236e5): chat dropped, +0x12d0 and
   +0x1718 0, the first queue flushed. 15 and unknown levels only log
   (+0x12a8 stays). */
static void late_levels_test(void) {
    static const int32_t only_log[] = {15, 8, 16, -1};
    static const char *const only_text[] = {"UNLINK GameIsPaused", "UNLINK Unknown",
                                             "UNLINK Unknown", "UNLINK Unknown"};
    size_t i;
    fresh(12);
    C.dirty = 1;
    C.data = malloc(8);
    C.data_size = 8;
    assert(aoe_comm_unlink_step_ror(&C, &dops, &U) == 1);
    assert(C.dirty == 0 && C.data == NULL && C.data_size == 0 && logged("UNLINK GameOptionsExchanged"));

    fresh(13);
    C.state = 5;
    C.field_12a8 = 1;
    assert(aoe_comm_unlink_step_ror(&C, &dops, &U) == 1);
    assert(C.state == 3 && C.field_12a8 == 0 && logged("UNLINK GameIsLoading"));

    fresh(14);
    C.field_12d0 = 1;
    C.field_1718 = 1;
    assert(aoe_comm_unlink_step_ror(&C, &dops, &U) == 1);
    assert(chat_clears == 1 && C.field_12d0 == 0 && C.field_1718 == 0 && flushes[0] == 1);
    assert(logged("UNLINK GameIsRunning"));

    for (i = 0; i < sizeof only_log / sizeof only_log[0]; ++i) {
        char want[32];
        fresh(only_log[i]);
        C.field_12a8 = 1;
        C.state = 4;
        assert(aoe_comm_unlink_step_ror(&C, &dops, &U) == 1);
        assert(C.field_12a8 == 1 && C.state == 4 && nlogs == 3);
        assert(strcmp(logs[1], only_text[i]) == 0);
        snprintf(want, sizeof want, "Level is now...(%d)", (int)(only_log[i] - 1));
        assert(strcmp(logs[2], want) == 0);
    }

    fresh(0);
    C.field_12a8 = 1;
    assert(aoe_comm_unlink_step_ror(&C, &dops, &U) == 1 && nlogs == 2 && C.field_12a8 == 1);
}

/* The loop 0x004237c0 (aoe_comm_unlink_ror) to level 2, as the callers at
   0x0047bd9d / 0x00487b38 ask: 9 (our player) -> 4 (DirectPlay) -> 3
   (multiplayer) -> 2. */
static void unlink_loop_test(void) {
    fresh(0);
    dp_on = 1;
    self_id = 77;
    C.multiplayer = 1;
    C.state = 3;
    U.has_queue_a = 1;
    assert(aoe_comm_unlink_ror(&C, &dops, 2) == 2);
    assert(destroys == 1 && releases == 1 && C.self == 1 && C.multiplayer == 0);
    assert(logged("UNLINK PlayerCreated") && logged("UNLINK ReleaseComm"));
    assert(logged("Unlinking current level (3)") && !logged("Unlinking current level (2)"));
    assert(strcmp(logs[nlogs - 1], "AFTER UNLINKING") == 0);
}

int main(void) {
    level3_test();
    level2_test();
    level4_test();
    level5_test();
    level6_test();
    level7_test();
    levels_9_to_11_test();
    late_levels_test();
    unlink_loop_test();
    puts("comm_handler_dispatch_test: ok");
    return 0;
}
