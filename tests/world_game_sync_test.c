/* Patched RoR 1.0a comm turn step 0x0041ebe0 (EMPIRESX.EXE 1.0a patch,
   build/patch_1_0c). Evidence (objdump -M intel):
   - 0x41ebec..0x41ec0d: no multiplayer gives -(state != 4) (neg / sbb);
     0x41ec10..0x41ec2f: our slot past the count gives 0.
   - 0x41ec3a..0x41ecea: now - [0x5817c8] >= 0x1388 logs 0x423e20(6..1)
     and stamps 0x5817c8.
   - 0x41ecff: [+0x1344 + self * 4] = now; 0x41ed06..0x41ed1a: edi = lead
     byte (+0x1590) + turn; 0x41ed24 state 3 skips; 0x41ed49 last_turn =
     turn; 0x41ed51 past turn 6: [esp+0x30] = 0x428df0 (al),
     [esp+0x31] = 0x4290f0 (al), 0x428b40(self, frame byte, latency byte);
     0x41ed92 0x7d3bf0 = timeGetTime; 0x41eda6 [esp+0x28] = 0x44,
     [esp+0x2c] = edi; 0x422680(0, 0x44, esp+0x28, 0xc, 0).
   - 0x41edb4: 0x4295b0(turn) on +0x14d8; 0x41edc6..0x41edfb: +0x10e8 == 1
     and game time >= +0x10ec post 0x17b0 (+0x10ec), 0.
   - 0x41edfe..0x41ee2e: 0x428c30 nonzero and turn > 6: 0x428ce0(turn).
   - 0x41f1a2..0x41f23a: caught up: state 5 runs 0x428cb0, a host's
     0x429140(&gr [esp+0x24], &buf [esp+0x20], 1), turn + 1, the log
     "....Starting Turn#%d, WT=%d, Fr=%d, Gr=%d" (turn, wt, [esp+0x24],
     [esp+0x20]), 0x420530(turn), 0x428c10.
   - 0x41ee4c: state < 5 gives 0; 0x41ee5f..0x41f082 the slot loop:
     > 0xea60 gone; > 0x61a8 twice posts 0x17bd; turn <= 7 and > 0x17ed0
     pregame; turn > 7, > 0x1da9c and host; turn > 7 and > 0x1d6b4
     player; the host report sprintf(+0xfc5, string 0x4ba, 0x4241a0(slot),
     slot), chat past turn 6, [ebx+0x158] = 1, 0x4256c0; [ebx] = [ebx+0x50]
     = now.
   - 0x41f082..0x41f0d9: inc eax; cmp eax,3: gone >= 2, else
     (others + 1) / 2 <= gone drops ourselves (0x41f0db..0x41f19f).
   - 0x41f08d..0x41f0c3: now - max(+0x12d0, newest) > 0x61a8 pings all
     (0x41f5e0(1)) and renews +0x12d0; 0x428dd0.
   String 0x4ba (a name and a slot number, formatted with sprintf) comes
   from the language DLL; the test uses a neutral stand-in format. */
#include "world_game_sync.h"
#include "le_bytes.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t clock_ms;
static int nnow;
static uint32_t now_(void *x) { (void)x; ++nnow; return clock_ms; }

static char logs[64][128];
static int nlogs;
static void log_(void *x, const char *t) {
    (void)x;
    if (nlogs < 64) snprintf(logs[nlogs++], sizeof logs[0], "%s", t);
}
static int find_log(const char *t) {
    int i;
    for (i = 0; i < nlogs; ++i)
        if (strcmp(logs[i], t) == 0) return i;
    return -1;
}

static uint32_t posts[16][2];
static int nposts;
static void post_(void *x, uint32_t w, uint32_t l) {
    (void)x;
    if (nposts < 16) { posts[nposts][0] = w; posts[nposts][1] = l; }
    ++nposts;
}

static uint8_t msg[16], msg_kind;
static uint32_t msg_size;
static int nmsgs;
static int32_t message_(void *x, uint8_t k, const void *d, uint32_t n) {
    (void)x;
    msg_kind = k;
    msg_size = n;
    memcpy(msg, d, n <= sizeof msg ? n : sizeof msg);
    ++nmsgs;
    return 0;
}

static int nstrings;
static void string_(void *x, int32_t id, char *out, int n) {
    (void)x;
    assert(id == 0x4ba && n == 100);
    ++nstrings;
    snprintf(out, (size_t)n, "%s", "S1210 '%s' (%d)");
}

static AoeCommOpsRor cops;

/* The turn step's ops. */
static uint32_t status_time, marker_time;
static int32_t status_lines[8];
static int nstatus;
static char status_buf[32];
static const char *status_line(void *x, int32_t l) {
    (void)x;
    if (nstatus < 8) status_lines[nstatus] = l;
    ++nstatus;
    snprintf(status_buf, sizeof status_buf, "status %d", (int)l);
    return status_buf;
}
static int npump, nlat, navg, nrate, ncount, nbegin, nadjust, ntake, nreset, nsync, nstop;
static int nchat, ndrop, ninit, nrefresh, nping, nend, nframe;
static uint32_t rate_slot, sync_turn_no, stop_turn_no, frame_turn, frames, drop_slot;
static uint32_t init_slots[10];
static uint8_t rate_frame, rate_turn;
static int adjust_apply, ping_force;
static char chat_text[128];
static void pump(void *x) { (void)x; ++npump; }
static uint32_t latency_byte(void *x) { (void)x; ++nlat; return 0x2e; }
static uint32_t frame_average(void *x) { (void)x; ++navg; return 0x123; }
static void set_rate(void *x, uint32_t s, uint8_t f, uint8_t t) {
    (void)x; ++nrate; rate_slot = s; rate_frame = f; rate_turn = t;
}
static uint32_t frames_left(void *x) { (void)x; return frames; }
static uint32_t frame(void *x, uint32_t t) { (void)x; ++nframe; frame_turn = t; return 90; }
static void count(void *x) { (void)x; ++ncount; }
static void begin_turn(void *x) { (void)x; ++nbegin; }
static int adjust(void *x, uint32_t *g, uint32_t *b, int apply) {
    (void)x; ++nadjust; adjust_apply = apply; *g = 60; *b = 7; return 1;
}
static uint32_t take_granularity(void *x) { (void)x; ++ntake; return 120; }
static void reset_frame(void *x) { (void)x; ++nreset; }
static void sync_turn(void *x, uint32_t t) { (void)x; ++nsync; sync_turn_no = t; }
static void check_stop(void *x, uint32_t t) { (void)x; ++nstop; stop_turn_no = t; }
static void chat_all(void *x, const char *t) {
    (void)x; ++nchat; snprintf(chat_text, sizeof chat_text, "%s", t);
}
static void drop_player(void *x, uint32_t s) { (void)x; ++ndrop; drop_slot = s; }
static void init_player(void *x, uint32_t s) { (void)x; if (ninit < 10) init_slots[ninit] = s; ++ninit; }
static void refresh_all(void *x) { (void)x; ++nrefresh; }
static void ping_quiet(void *x, int f) { (void)x; ++nping; ping_force = f; }
static void end_report(void *x) { (void)x; ++nend; }

static AoeCommTurnOpsRor tops;
static AoeCommRor c;
static char names[AOE_COMM_SLOTS][0x80];

/* Three slots, all playing; we are slot 1 of a running game at turn 10,
   everyone caught up; the status lines just logged. */
static void reset(void) {
    uint32_t slot;
    memset(&c, 0, sizeof c);
    c.ops = &cops;
    c.multiplayer = 1;
    c.state = 5;
    c.self = 1;
    c.count = 3;
    c.low = 1;
    c.high = 3;
    c.lead = 2;
    c.turn = 10;
    c.names = names;
    snprintf(c.name, sizeof c.name, "Me");
    for (slot = 1; slot <= 3; ++slot) {
        c.player_state[slot] = 2;
        c.last_turn[slot] = 10;
    }
    clock_ms = 500000;
    status_time = clock_ms;
    marker_time = 0;
    nnow = nlogs = nposts = nmsgs = nstrings = nstatus = 0;
    npump = nlat = navg = nrate = ncount = nbegin = nadjust = ntake = nreset = nsync = 0;
    nstop = nchat = ndrop = ninit = nrefresh = nping = nend = nframe = 0;
    frames = 0;
    memset(msg, 0xcc, sizeof msg);
    chat_text[0] = 0;
}

static void no_multiplayer_tests(void) {
    reset();
    c.multiplayer = 0;
    assert(aoe_comm_turn_ror(&c, &tops, 0) == -1);
    c.state = 4;
    assert(aoe_comm_turn_ror(&c, &tops, 0) == 0);
    assert(nnow == 0 && npump == 0);
    /* Our slot past the count. */
    reset();
    c.self = 4;
    assert(aoe_comm_turn_ror(&c, &tops, 0) == 0 && nnow == 0 && npump == 0);
}

static void marker_tests(void) {
    int i;
    /* Status lines 6..1 at 5000 ms; the marker with our speed sample; a
       turn with frames left runs the controller's frame answer. */
    reset();
    clock_ms = 100000;
    status_time = 95000;
    c.last_turn[1] = 9;
    frames = 2;
    assert(aoe_comm_turn_ror(&c, &tops, 777) == 90);
    assert(nstatus == 6 && status_time == 100000);
    for (i = 0; i < 6; ++i) assert(status_lines[i] == 6 - i);
    assert(strcmp(logs[0], "status 6") == 0 && strcmp(logs[5], "status 1") == 0);
    assert(npump == 1 && c.time_1344[1] == 100000 && c.last_turn[1] == 10);
    assert(nlat == 1 && navg == 1 && nrate == 1 && rate_slot == 1 && rate_frame == 0x23 &&
           rate_turn == 0x2e);
    assert(nmsgs == 1 && msg_kind == 0x44 && msg_size == 12 && msg[0] == 0x44 &&
           aoe_le_u32(msg + 4) == 12 && msg[8] == 0x2e && msg[9] == 0x23);
    assert(marker_time == 100000 && nsync == 1 && sync_turn_no == 10);
    assert(nframe == 1 && frame_turn == 10 && nbegin == 0);
    /* 4999 ms later: no status lines; the turn already marked. */
    clock_ms = 104999;
    assert(aoe_comm_turn_ror(&c, &tops, 778) == 90);
    assert(nstatus == 6 && nmsgs == 1 && nsync == 2);

    /* In the lobby (state 3) nothing is marked. */
    reset();
    c.state = 3;
    c.turn = 4;
    c.last_turn[1] = 0;
    assert(aoe_comm_turn_ror(&c, &tops, 0) == 0);
    assert(nmsgs == 0 && c.last_turn[1] == 0 && nsync == 1 && sync_turn_no == 4);
    /* Paused at turn 4: marked, no speed sample before turn 7. */
    c.state = 4;
    assert(aoe_comm_turn_ror(&c, &tops, 0) == 0);
    assert(nmsgs == 1 && c.last_turn[1] == 4 && aoe_le_u32(msg + 4) == 6 && msg[8] == 0 &&
           msg[9] == 0 && nlat == 0 && nrate == 0 && nbegin == 0 && ncount == 0);
}

static void end_turn_tests(void) {
    reset();
    c.field_10e8 = 1;
    c.field_10ec = 500;
    assert(aoe_comm_turn_ror(&c, &tops, 500) == 0);
    assert(nposts == 1 && posts[0][0] == 0x17b0 && posts[0][1] == 500 && nbegin == 0 &&
           c.turn == 10);
    c.field_10e8 = 2;
    assert(aoe_comm_turn_ror(&c, &tops, 500) == 120 && nposts == 1 && c.turn == 11);
}

static void start_turn_tests(void) {
    /* A host: the pace adjusted (granularity, then buffer, logged as Fr
       and Gr), the turn advanced, the stop turns checked. */
    reset();
    c.is_host = 1;
    assert(aoe_comm_turn_ror(&c, &tops, 1234) == 120);
    assert(nbegin == 1 && nadjust == 1 && adjust_apply == 1 && c.turn == 11 && ntake == 1);
    assert(find_log("....Starting Turn#11, WT=1234, Fr=60, Gr=7") >= 0);
    assert(nstop == 1 && stop_turn_no == 11 && ncount == 0 && nreset == 0);
    /* A guest: no adjustment; frames left before turn 7 are ignored. */
    reset();
    c.turn = 6;
    c.last_turn[1] = c.last_turn[2] = c.last_turn[3] = 6;
    frames = 3;
    assert(aoe_comm_turn_ror(&c, &tops, 9) == 120);
    assert(nframe == 0 && nadjust == 0 && c.turn == 7 &&
           find_log("....Starting Turn#7, WT=9, Fr=0, Gr=0") >= 0);
    /* Caught up in the lobby: nothing. */
    reset();
    c.state = 3;
    assert(aoe_comm_turn_ror(&c, &tops, 0) == 0 && nbegin == 0 && c.turn == 10 && ncount == 0);
}

static void timeout_tests(void) {
    int a, b;
    /* A host past turn 7: slot 2 silent 121501 ms times out (host and
       player lines), is reported, chatted, flagged and dropped; slot 3
       (behind, 30 s) is the newest, so every slot is pinged. */
    reset();
    c.is_host = 1;
    c.last_turn[3] = 9;
    c.time_1344[2] = 500000 - 121501;
    c.time_1344[3] = 500000 - 30000;
    c.field_136c[3] = 500000 - 1000;
    assert(aoe_comm_turn_ror(&c, &tops, 0) == 0);
    assert(ncount == 1 && nbegin == 0);
    assert(nposts == 1 && posts[0][0] == 0x17bd && posts[0][1] == 2 &&
           c.field_136c[2] == 500000);
    a = find_log("+++Timeout host, turn=10 PlayerTimeout=121501");
    b = find_log("+++Timeout player, turn=10 PlayerTimeout=121501");
    assert(a >= 0 && b == a + 1);
    assert(find_log("Player timed out with 121501 msec elapsed") == b + 1);
    assert(find_log("Recommend player '2' drop.") == b + 2);
    assert(find_log("Player 2 Turn#10  timeout = 121501 ") == b + 3);
    assert(strcmp(c.text, "S1210 'Bob' (2)") == 0);
    assert(find_log("S1210 'Bob' (2)") == b + 4);
    assert(nstrings == 1 && nchat == 1 && strcmp(chat_text, c.text) == 0);
    assert(c.field_149c[2] == 1 && ndrop == 1 && drop_slot == 2);
    assert(c.time_1344[2] == 500000 && c.fast_time[2] == 500000);
    assert(c.time_1344[3] == 470000 && c.fast_time[3] == 0 && c.field_149c[3] == 0);
    assert(nend == 0 && nping == 1 && ping_force == 1 && c.field_12d0 == 500000 &&
           nnow == 3 && nreset == 1);

    /* A guest before turn 8: 98001 ms is a pregame timeout, logged and
       its times renewed but not reported; 98000 ms is not. */
    reset();
    c.turn = 5;
    c.last_turn[1] = c.last_turn[2] = 5;
    c.last_turn[3] = 4;
    c.time_1344[2] = 500000 - 98001;
    c.time_1344[3] = 500000 - 98000;
    c.field_136c[2] = c.field_136c[3] = 500000;
    c.field_12d0 = 500000;
    assert(aoe_comm_turn_ror(&c, &tops, 0) == 0);
    assert(find_log("+++Timeout pregrame, turn=5 PlayerTimeout=98001") >= 0 &&
           find_log("+++Timeout pregrame, turn=5 PlayerTimeout=98000") < 0);
    assert(find_log("Recommend player '2' drop.") >= 0 &&
           find_log("Recommend player '3' drop.") < 0);
    assert(nstrings == 0 && nchat == 0 && ndrop == 0 && nposts == 1 && posts[0][0] == 0x17b2);
    assert(c.time_1344[2] == 500000 && c.fast_time[2] == 500000 &&
           c.time_1344[3] == 500000 - 98000);
    /* Both others over 60 s gone: two of two drop ourselves. */
    assert(nend == 1 && nping == 0 && nreset == 0);
}

static void drop_self_tests(void) {
    /* One other slot, silent 60001 ms: half of it is gone. */
    reset();
    c.count = 2;
    c.high = 2;
    c.player_state[3] = 0;
    c.last_turn[2] = 9;
    c.time_1344[2] = 500000 - 60001;
    c.dirty = 1;
    c.data_time = 99;
    assert(aoe_comm_turn_ror(&c, &tops, 0) == 0);
    assert(nposts == 2 && posts[0][0] == 0x17bd && posts[0][1] == 2 && posts[1][0] == 0x17b2 &&
           posts[1][1] == 1);
    assert(find_log("I Dropped myself.") >= 0 &&
           find_log("Initialize Player List") > find_log("I Dropped myself."));
    assert(c.dropped[1] == 1 && c.dropped[2] == 1 && c.dropped[3] == 0 && nend == 1);
    assert(ninit == 2 && init_slots[0] == 1 && init_slots[1] == 2);
    assert(c.dirty == 0 && c.data_time == 0 && nrefresh == 1 && nreset == 0 && nping == 0);
    /* 60000 ms is not gone. */
    reset();
    c.count = 2;
    c.high = 2;
    c.player_state[3] = 0;
    c.last_turn[2] = 9;
    c.time_1344[2] = 500000 - 60000;
    c.field_12d0 = 500000;
    assert(aoe_comm_turn_ror(&c, &tops, 0) == 0 && nend == 0 && nreset == 1);
    /* Two others with one gone stay (two needed). */
    reset();
    c.last_turn[2] = 9;
    c.time_1344[2] = 500000 - 70000;
    c.time_1344[3] = 500000;
    assert(aoe_comm_turn_ror(&c, &tops, 0) == 0 && nend == 0 && nreset == 1);
    /* Alone and held back (+0x12a8): (0 + 1) / 2 <= 0 drops us too. */
    reset();
    c.count = 1;
    c.high = 1;
    c.field_12a8 = 1;
    assert(aoe_comm_turn_ror(&c, &tops, 0) == 0 && nend == 1 && ninit == 1 &&
           posts[0][0] == 0x17b2);
}

int main(void) {
    snprintf(names[2], sizeof names[2], "Bob");
    snprintf(names[3], sizeof names[3], "Cy");
    cops.now_ms = now_;
    cops.log = log_;
    cops.post = post_;
    cops.message = message_;
    cops.string = string_;
    tops.status_time = &status_time;
    tops.marker_time = &marker_time;
    tops.status_line = status_line;
    tops.pump = pump;
    tops.latency_byte = latency_byte;
    tops.frame_average = frame_average;
    tops.set_player_rate = set_rate;
    tops.frames_left = frames_left;
    tops.frame = frame;
    tops.count = count;
    tops.begin_turn = begin_turn;
    tops.adjust = adjust;
    tops.take_granularity = take_granularity;
    tops.reset_frame = reset_frame;
    tops.sync_turn = sync_turn;
    tops.check_stop_turn = check_stop;
    tops.chat_all = chat_all;
    tops.drop_player = drop_player;
    tops.init_player = init_player;
    tops.refresh_all = refresh_all;
    tops.ping_quiet = ping_quiet;
    tops.end_report = end_report;
    no_multiplayer_tests();
    marker_tests();
    end_turn_tests();
    start_turn_tests();
    timeout_tests();
    drop_self_tests();
    puts("world_game_sync_test: ok");
    return 0;
}
