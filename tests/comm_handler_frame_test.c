#include "comm_handler_frame.h"
#include "le_bytes.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Host test for 0x0041f780 (patched RoR 1.0a), the run of one received
   message. Evidence: build/patch_1_0c/EMPIRESX.EXE 0x0041f780..0x004200c9
   (ret 0x14, five stack arguments). */

static uint32_t clock_ms = 10000;
static int32_t self_id = 100;
static uint8_t sent[16][16];
static int32_t sent_to[16], sent_n[16];
static int nsent;
static uint8_t msg_buf[16][16];
static uint8_t msg_kind[16];
static int nmsgs;
static uint32_t posts[32][2];
static int nposts, refreshes, last_refresh_flag, drops;
static char last_log[0x200], last_report[32];

static int dp_present(void *x) { (void)x; return 1; }
static int32_t self_dpid(void *x) { (void)x; return self_id; }
static int32_t dp_send(void *x, int32_t to, const void *d, uint32_t n) {
    (void)x;
    assert(nsent < 16 && n <= 16);
    sent_to[nsent] = to;
    sent_n[nsent] = (int32_t)n;
    memcpy(sent[nsent++], d, n);
    return 0;
}
static int32_t set_data(void *x, const void *d, uint32_t n) { (void)x; (void)d; (void)n; return 0; }
static int32_t message(void *x, uint8_t k, const void *d, uint32_t n) {
    (void)x;
    assert(nmsgs < 16 && n <= 16);
    msg_kind[nmsgs] = k;
    memcpy(msg_buf[nmsgs++], d, n);
    return 0;
}
static void post(void *x, uint32_t w, uint32_t l) {
    (void)x;
    assert(nposts < 32);
    posts[nposts][0] = w;
    posts[nposts++][1] = l;
}
static void log_(void *x, const char *t) { (void)x; snprintf(last_log, sizeof last_log, "%s", t); }
static void report(void *x, int32_t hr, const char *t) {
    (void)x; assert(hr == 0); snprintf(last_report, sizeof last_report, "%s", t);
}
static void drop(void *x, uint32_t s, int32_t d) { (void)x; (void)s; (void)d; drops++; }
static void refresh(void *x, uint32_t s, int f) { (void)x; (void)s; refreshes++; last_refresh_flag = f; }
static uint32_t now(void *x) { (void)x; return clock_ms; }
static void block(void *x, uint8_t out[0x1d0]) { (void)x; memset(out, 0, 0x1d0); }

static const AoeCommOpsRor ops = {
    .dp_present = dp_present, .self_dpid = self_dpid, .dp_send = dp_send,
    .dp_set_player_data = set_data, .message = message, .post = post, .log = log_,
    .report = report, .drop = drop, .refresh = refresh, .now_ms = now, .state_block = block};

/* The frame ops. */
static int32_t unlinked;
static uint32_t rate_slot, latency_slot, latency_ms, marker;
static uint8_t rate_frame, rate_turn;
static int rates;
static char chat_who[16], chat_text[0x100];
static int32_t resend_serial;
static uint32_t resend_slot;
static int handles;
static struct {
    int32_t size, exec_turn, payload_size, from, extra;
    uint32_t slot;
    uint8_t command, sequence;
    const uint8_t *payload;
} h;
static const uint8_t *g_payload;
static int32_t g_payload_size;

static int32_t f_unlink(void *x, int32_t level) { (void)x; unlinked = level; return level; }
static uint32_t f_latency_byte(void *x) { (void)x; return 7; }
static uint32_t f_frame_average(void *x) { (void)x; return 0x123; }
static void f_rate(void *x, uint32_t s, uint8_t fr, uint8_t t) {
    (void)x; rate_slot = s; rate_frame = fr; rate_turn = t; rates++;
}
static void f_latency(void *x, uint32_t s, uint32_t ms) { (void)x; latency_slot = s; latency_ms = ms; }
static const char *f_name(void *x, uint32_t s) { (void)x; return s == 2 ? "Bob" : "?"; }
static void f_chat(void *x, const char *who, const char *t) {
    (void)x;
    snprintf(chat_who, sizeof chat_who, "%s", who);
    snprintf(chat_text, sizeof chat_text, "%s", t);
}
static void f_resend(void *x, int32_t serial, uint32_t s) { (void)x; resend_serial = serial; resend_slot = s; }
static int f_handle(void *x, int32_t size, uint32_t slot, int32_t turn, uint8_t cmd, uint8_t seq,
                    const uint8_t *p, int32_t psize, int32_t from, int32_t extra) {
    (void)x;
    handles++;
    h.size = size; h.slot = slot; h.exec_turn = turn; h.command = cmd; h.sequence = seq;
    h.payload = p; h.payload_size = psize; h.from = from; h.extra = extra;
    return 1;
}

static const AoeCommFrameOpsRor fops = {
    NULL, f_unlink, f_latency_byte, f_frame_average, f_rate, f_latency, f_name, f_chat,
    f_resend, f_handle, &marker, &g_payload, &g_payload_size};

static AoeCommRor c;

static void setup(int32_t state, uint8_t gtd) {
    static AoeCommTxRor tx[AOE_COMM_TX_SLOTS];
    static AoeCommRxRor rx[AOE_COMM_RX_SLOTS];
    int i;
    for (i = 0; i < AOE_COMM_RX_SLOTS; ++i) free(rx[i].data);
    for (i = 0; i < AOE_COMM_TX_SLOTS; ++i) free(tx[i].data);
    memset(tx, 0, sizeof tx);
    memset(rx, 0, sizeof rx);
    memset(&c, 0, sizeof c);
    c.ops = &ops;
    c.tx = tx;
    c.rx = rx;
    c.count = 4;
    c.low = 1;
    c.high = 4;
    c.dpid[1] = 100; /* ours */
    c.dpid[2] = 200;
    c.dpid[3] = 300;
    c.player_state[1] = c.player_state[2] = c.player_state[3] = 2;
    c.self = 1;
    c.multiplayer = 1;
    c.state = state;
    c.turn = 10;
    c.lead = 2;
    c.gtd = gtd;
    nsent = nmsgs = nposts = refreshes = drops = handles = rates = 0;
    self_id = 100;
    clock_ms = 10000;
}

static void put(uint8_t *m, uint32_t off, uint32_t v) { aoe_le_put_u32(m + off, v); }

static int run(const uint8_t *m, int32_t size, int32_t from, int flag) {
    return aoe_comm_run_frame_ror(&c, &fops, size, m, from, 100, flag);
}

/* 0x0041f7c6: 'I' unlinks to level 5 (0x004237c0) and posts 0x17a4 then
   0x17aa (0x004200d0), even from an unknown sender. */
static void test_unlink(void) {
    uint8_t m[8] = {'I'};
    setup(5, 1);
    assert(run(m, 8, 999, 0) == 1 && unlinked == 5);
    assert(nposts == 2 && posts[0][0] == 0x17a4 && posts[1][0] == 0x17aa && posts[1][1] == 0);
}

/* 0x0041f7fe..0x0041f858: a sender outside the active range gets an
   8-byte 'A' with the serial at +8 through DirectPlay +0x68, +0x12c0
   counts 8; 0. Without our ID nothing is sent. */
static void test_unknown_sender(void) {
    uint8_t m[16] = {0x3e};
    setup(5, 1);
    put(m, 8, 77);
    assert(run(m, 16, 999, 0) == 0);
    assert(nsent == 1 && sent_to[0] == 999 && sent_n[0] == 8 && sent[0][0] == 0x41 &&
           aoe_le_u32(sent[0] + 4) == 77 && c.bytes_sent == 8);
    self_id = 0;
    assert(run(m, 16, 999, 0) == 0 && nsent == 1 && c.bytes_sent == 8);
    /* Slot 4 has ID 0: a sender ID of 0 is never a slot. */
    self_id = 100;
    assert(run(m, 16, 0, 0) == 0 && nsent == 2 && sent_to[1] == 0);
}

/* 0x0041fb05..0x0041fc11: a ping. Our turn marker first (0x00428df0 byte
   8, 0x004290f0 byte 9, 0x00428b40(self, b9, b8), 0x007d3bf0 the time,
   0x44 with turn + lead at +4 through 0x00422680), then the '2' reply
   (the time, our turn) to the sender's ID (0x0041f6e0, "TX Ping"). */
static void test_ping(void) {
    uint8_t m[12] = {'1'};
    setup(5, 1);
    c.last_turn[1] = 9;
    put(m, 4, 5555);
    clock_ms = 12345;
    assert(run(m, 12, 200, 0) == 1 && c.time_1344[2] == 12345);
    assert(nmsgs == 1 && msg_kind[0] == 0x44 && msg_buf[0][0] == 0x44 &&
           aoe_le_u32(msg_buf[0] + 4) == 12 && msg_buf[0][8] == 7 && msg_buf[0][9] == 0x23);
    assert(rates == 1 && rate_slot == 1 && rate_frame == 0x23 && rate_turn == 7);
    assert(marker == 12345 && c.last_turn[1] == 10);
    assert(nsent == 1 && sent_to[0] == 200 && sent_n[0] == 12 && sent[0][0] == 0x32 &&
           aoe_le_u32(sent[0] + 4) == 5555 && aoe_le_u32(sent[0] + 8) == 10 &&
           strcmp(last_report, "TX Ping") == 0);
    /* The same turn: no second marker. */
    assert(run(m, 12, 200, 0) == 1 && nmsgs == 1 && nsent == 2);
    /* Up to turn 6 the rate bytes are not taken (0x0041fb5d jbe). */
    setup(5, 1);
    c.turn = 5;
    assert(run(m, 12, 200, 0) == 1 && nmsgs == 1 && rates == 0 && msg_buf[0][8] == 0 &&
           aoe_le_u32(msg_buf[0] + 4) == 7);
    /* In the lobby (state 3) no marker. */
    setup(3, 1);
    assert(run(m, 12, 200, 0) == 1 && nmsgs == 0 && nsent == 1);
}

/* 0x0041fd9a..0x0041fe7d: a ping reply sets the round trip as the
   slot's latency (0x00428b80); 0x0041fc14 / 0x0041fe80 the debug pings. */
static void test_ping_reply_and_debug(void) {
    uint8_t m[12] = {'2'};
    setup(5, 1);
    c.last_turn[1] = 10;
    put(m, 4, 9960);
    assert(run(m, 12, 200, 0) == 1 && latency_slot == 2 && latency_ms == 40 && nmsgs == 0);
    m[0] = '3';
    put(m, 4, 777);
    assert(run(m, 12, 200, 0) == 1 && nsent == 1 && sent[0][0] == 0x34 &&
           aoe_le_u32(sent[0] + 4) == 777 && strcmp(last_report, "TX Debug Ping") == 0);
    assert(strcmp(chat_who, "DEBUG") == 0 &&
           strcmp(chat_text, "Received debug ping from Bob (#2)") == 0 &&
           strcmp(last_log, chat_text) == 0);
    m[0] = '4';
    put(m, 4, 9975);
    assert(run(m, 12, 200, 0) == 1 && strcmp(chat_text, "Bob #2 Debug ping - elapsed=25") == 0);
}

/* 0x0041f9c1..0x0041fb02: the readiness message on the host. */
static void test_ready(void) {
    uint8_t m[0x28] = {'R', 1};
    int i;
    setup(3, 1);
    c.is_host = 1;
    c.player_state[2] = 0;
    c.ready[1] = 1;
    for (i = 1; i < 8; ++i) put(m, 4u * (uint32_t)i, 10u + (uint32_t)i);
    put(m, 0x20, 99);
    c.field_12a4 = 99;
    assert(run(m, 0x28, 200, 0) == 1);
    assert(c.stat[0][2] == 1 && c.stat[1][2] == 11 && c.stat[7][2] == 17 && c.ready[1] == 0);
    assert(c.player_state[2] == 2 && last_refresh_flag == 0 && posts[nposts - 1][0] == 0x17a4);
    /* A different version posts 0x17b7 with the slot and the warning. */
    setup(3, 1);
    c.is_host = 1;
    c.field_12a4 = 98;
    assert(run(m, 0x28, 200, 0) == 1 && posts[0][0] == 0x17b7 && posts[0][1] == 2 &&
           strncmp(c.text, "Player 2 has a different Communications Version", 47) == 0);
    /* A guest only logs it. */
    setup(3, 1);
    assert(run(m, 0x28, 200, 0) == 1 && c.stat[0][2] == 0 &&
           strcmp(last_log, "PLAYER READY MSG") == 0);
}

/* 0x0041fcb0..0x0041fd97: the missing-player report on the host. */
static void test_missing_report(void) {
    uint8_t m[8] = {'?'};
    setup(3, 1);
    c.is_host = 1;
    put(m, 4, 300);
    assert(run(m, 8, 200, 0) == 1 && c.stat[0][3] == 0 && c.ready[3] == 1 && c.dirty == 1);
    assert(strcmp(c.text, " P#2 reported P#3 (dco=300) was invalid ") == 0 &&
           strcmp(last_log, "Rcvd Missing Player report for player #3, ID=300 from player 2") ==
               0);
    /* In the game the reported slot is dropped (0x004256c0: 0x4b). */
    setup(5, 1);
    c.is_host = 1;
    assert(run(m, 8, 200, 0) == 1 && nmsgs == 1 && msg_kind[0] == 0x4b && drops == 1);
}

/* 0x0041fedf: 'Y' in the lobby takes the serial and drops the sender's
   stored messages (0x00421ab0); 0x0041f996: 'A' acks (0x00421e40) and
   purges (0x00421ec0). */
static void test_reset_and_ack(void) {
    uint8_t m[12] = {'Y'};
    setup(3, 1);
    c.rx[4].from = 200;
    c.rx[4].size = 1;
    c.rx[4].serial = 9;
    put(m, 4, 41);
    assert(run(m, 8, 200, 0) == 1 && c.rx_serial[2] == 41 && c.rx[4].size == 0);
    setup(5, 1);
    assert(run(m, 8, 200, 0) == 1 && c.rx_serial[2] == 0);
    m[0] = 'A';
    put(m, 4, 9);
    c.tx[0].serial = 9;
    c.tx[0].pending[2] = 1;
    assert(run(m, 8, 200, 0) == 1 && c.tx[0].serial == 0);
}

static void gtd_message(uint8_t *m, uint8_t cmd, uint32_t serial) {
    memset(m, 0, 0x10);
    m[0] = cmd;
    m[1] = 4;
    put(m, 4, 12);
    put(m, 8, serial);
    m[0xc] = 0xab;
}

/* 0x0041ff47..0x0042005a: guaranteed-delivery ordering, then 0x00420084
   publishes the payload and 0x004200b5 dispatches it (0x00420120). */
static void test_gtd_order(void) {
    uint8_t m[0x10];
    setup(5, 1);
    gtd_message(m, 0x3e, 50);
    assert(run(m, 0x10, 200, 0) == 1 && c.rx_serial[2] == 50);
    assert(nsent == 1 && sent_to[0] == 200 && sent[0][0] == 0x41 &&
           aoe_le_u32(sent[0] + 4) == 50);
    assert(handles == 1 && h.size == 0x10 && h.slot == 2 && h.exec_turn == 12 &&
           h.command == 0x3e && h.sequence == 4 && h.payload == m + 0xc && h.payload[0] == 0xab &&
           h.payload_size == 4 && h.from == 200 && h.extra == 100);
    assert(g_payload == m + 0xc && g_payload_size == 4);
    /* An old serial is acknowledged again only. */
    assert(run(m, 0x10, 200, 0) == 1 && handles == 1 && nsent == 2);
    /* A gap: stored, acknowledged, the missing serial 51 requested ('X'). */
    gtd_message(m, 0x3e, 53);
    assert(run(m, 0x10, 200, 0) == 1 && handles == 1 && c.field_1330 == 1);
    assert(c.rx[0].serial == 53 && c.rx[0].from == 200 && c.rx[0].size == 0x10 &&
           c.rx[0].extra == 100);
    assert(nsent == 4 && sent[2][0] == 0x41 && aoe_le_u32(sent[2] + 4) == 53 &&
           sent[3][0] == 0x58 && aoe_le_u32(sent[3] + 4) == 51);
    /* From the store (flag 1) the next one is not acknowledged again. */
    gtd_message(m, 0x3e, 51);
    assert(run(m, 0x10, 200, 1) == 1 && nsent == 4 && handles == 2 && c.rx_serial[2] == 51);
    /* With +0x12a8 set the payload is published but not dispatched. */
    c.field_12a8 = 1;
    gtd_message(m, 0x3e, 52);
    g_payload = NULL;
    assert(run(m, 0x10, 200, 0) == 1 && handles == 2 && g_payload == m + 0xc);
    /* 'X' is a resend request (0x00422330) for the serial at +4. */
    gtd_message(m, 'X', 0);
    put(m, 4, 33);
    assert(run(m, 0x10, 200, 0) == 1 && resend_serial == 33 && resend_slot == 2);
}

/* 0x0041ff47 je 0x0042005c: without guaranteed delivery the payload
   starts at +8 and nothing is acknowledged. */
static void test_no_gtd(void) {
    uint8_t m[0x0c] = {0x3e, 3};
    setup(5, 0);
    put(m, 4, 12);
    m[8] = 0xcd;
    assert(run(m, 0xc, 200, 0) == 1 && nsent == 0 && handles == 1 && h.payload == m + 8 &&
           h.payload_size == 4 && h.payload[0] == 0xcd && h.sequence == 3);
    /* 'X' is not special then. */
    m[0] = 'X';
    assert(run(m, 0xc, 200, 0) == 1 && handles == 2 && h.command == 'X');
}

/* 0x0041f8f9..0x0041f993 and 0x0041ff14..0x0041ff44: chat filtering and
   the lobby gate. */
static void test_chat_and_lobby(void) {
    uint8_t m[0x40];
    setup(5, 0);
    memset(m, 0, sizeof m);
    m[0] = 'C';
    m[8 + 0x11] = 3; m[8 + 0x12] = 2; m[8 + 0x13] = 1;
    assert(run(m, sizeof m, 200, 0) == 0 && handles == 0);
    memcpy(m + 8 + 0x11, "hi", 3);
    m[8 + 0x10] = 1;
    assert(run(m, sizeof m, 200, 0) == 0 &&
           strcmp(last_log, "DISCARD: ChtPregame=1 PgmState=5  Msg=hi") == 0);
    /* In the lobby a pregame chat goes on to the dispatcher. */
    c.state = 3;
    assert(run(m, sizeof m, 200, 0) == 1 && handles == 1 && h.command == 'C' &&
           strcmp(last_log, "CHAT: ChtPregame=1 PgmState=3  Msg=hi") == 0);
    /* Other chats and other kinds stop there before state 4. */
    m[8 + 0x10] = 0;
    assert(run(m, sizeof m, 200, 0) == 0 && handles == 1);
    m[0] = 0x3e;
    assert(run(m, sizeof m, 200, 0) == 0 && handles == 1);
    /* In the game an ordinary chat is dispatched. */
    m[0] = 'C';
    c.state = 5;
    assert(run(m, sizeof m, 200, 0) == 1 && handles == 2);
}

/* 0x0041f86a..0x0041f8f6: a kill in the lobby. */
static void test_kill_in_lobby(void) {
    uint8_t m[0x18];
    setup(3, 1);
    memset(m, 0, sizeof m);
    m[0] = 'K';
    put(m, 8, 61);
    put(m, 0xc, 3);
    put(m, 0x10, 300);
    c.rx_serial[2] = 60;
    c.rx_serial[3] = 5;
    assert(run(m, sizeof m, 200, 0) == 1 && c.rx_serial[2] == 61 && c.rx_serial[3] == 0);
    assert(nsent == 1 && sent[0][0] == 0x41 && aoe_le_u32(sent[0] + 4) == 61 && nposts == 0 &&
           handles == 0 &&
           strcmp(last_log, "KILL Message recieved from 200 to kill dcoID #300 ") == 0);
    /* Our own ID posts 0x17aa. */
    put(m, 0x10, 100);
    assert(run(m, sizeof m, 200, 0) == 1 && nposts == 1 && posts[0][0] == 0x17aa);
    /* Outside the lobby a kill is an ordinary guaranteed message. */
    setup(5, 1);
    put(m, 0x10, 300);
    assert(run(m, sizeof m, 200, 0) == 1 && handles == 1 && h.command == 'K');
}

int main(void) {
    test_unlink();
    test_unknown_sender();
    test_ping();
    test_ping_reply_and_debug();
    test_ready();
    test_missing_report();
    test_reset_and_ack();
    test_gtd_order();
    test_no_gtd();
    test_chat_and_lobby();
    test_kill_in_lobby();
    puts("comm_handler_frame_test: ok");
    return 0;
}
