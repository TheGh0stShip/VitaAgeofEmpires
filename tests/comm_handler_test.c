#include "comm_handler.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int dp = 1, self_id = 100, refreshes, drops, runs, run_serial[8], nruns;
static uint32_t clock_ms = 10000, posts[32][2];
static int nposts, nmsgs;
static uint8_t msgs[32];
static uint8_t sent_buf[64];
static int32_t sent_to, sent_n;
static char last_mask[10];
static int dp_present(void *c) { (void)c; return dp; }
static int32_t self_dpid(void *c) { (void)c; return self_id; }
static int32_t dp_send(void *c, int32_t to, const void *d, uint32_t n) { (void)c; sent_to = to; sent_n = (int32_t)n; memcpy(sent_buf, d, n); return 0; }
static int32_t set_data(void *c, const void *d, uint32_t n) { (void)c; (void)d; sent_n = (int32_t)n; return 0; }
static int32_t message(void *c, uint8_t k, const void *d, uint32_t n) { (void)c; (void)d; (void)n; msgs[nmsgs++] = k; return 0; }
static void chat(void *c, int32_t from, const char r[10], const char *t) { (void)c; (void)from; (void)t; memcpy(last_mask, r, 10); }
static void post(void *c, uint32_t w, uint32_t l) { (void)c; posts[nposts][0] = w; posts[nposts][1] = l; nposts++; }
static void log_(void *c, const char *t) { (void)c; (void)t; }
static void report(void *c, int32_t hr, const char *t) { (void)c; (void)hr; (void)t; }
static void drop(void *c, uint32_t s, int32_t d) { (void)c; (void)s; (void)d; drops++; }
static void refresh(void *c, uint32_t s, int f) { (void)c; (void)s; (void)f; refreshes++; }
static void run(void *c, int32_t n, const uint8_t *d, int32_t from, int32_t extra, int f) {
    (void)c; (void)n; (void)from; (void)extra; (void)f; runs++; run_serial[nruns++] = d[0];
}
static uint32_t qturn, qplayer; static uint8_t qseq;
static void queue_local(void *c, uint32_t t, const void *d, uint32_t n, uint32_t p, uint8_t s, int32_t e) { (void)c; (void)d; (void)n; (void)e; qturn = t; qplayer = p; qseq = s; }
static void string(void *c, int32_t id, char *o, int n) { (void)c; snprintf(o, (size_t)n, "S%x", (unsigned)id); }
static uint32_t now(void *c) { (void)c; return clock_ms; }
static void srand_(void *c, uint32_t s) { (void)c; (void)s; }
static int32_t rand_(void *c) { (void)c; return 4242; }
static void block(void *c, uint8_t out[0x1d0]) { (void)c; memset(out, 7, 0x1d0); }
static uint8_t seq_fn(void *c, uint32_t t) { (void)c; (void)t; return 3; }
static int to_slot(void *c, uint32_t s) { (void)c; return s == 2 || s == 4; }

static const AoeCommOpsRor ops = {
    NULL, dp_present, self_dpid, dp_send, set_data, message, chat, post, log_, report, drop,
    refresh, run, queue_local, NULL, NULL, NULL, string, now, srand_, rand_, block};

int main(void) {
    static AoeCommRor c;
    static AoeCommTxRor tx[AOE_COMM_TX_SLOTS];
    static AoeCommRxRor rx[AOE_COMM_RX_SLOTS];
    static char names[AOE_COMM_SLOTS][0x80];
    char text[0x20];
    int32_t pending[AOE_COMM_SLOTS] = {0};
    uint8_t m = 9;
    memset(&c, 0, sizeof c);
    c.ops = &ops;
    c.tx = tx;
    c.rx = rx;
    c.names = names;
    c.count = 4;
    c.multiplayer = 1;
    c.is_host = 1;
    c.self = 1;
    c.state = 5;

    /* Slots. */
    aoe_comm_init_player_ror(&c, 2, 200);
    aoe_comm_set_player_state_ror(&c, 1, 2);
    aoe_comm_set_player_state_ror(&c, 2, 5);
    aoe_comm_set_player_state_ror(&c, 3, 4);
    assert(refreshes == 3 && posts[0][0] == 0x17a6);
    assert(aoe_comm_playing_ror(&c, 1) && aoe_comm_playing_ror(&c, 2) && !aoe_comm_playing_ror(&c, 3));
    assert(aoe_comm_computer_ror(&c, 3) && !aoe_comm_computer_ror(&c, 5));
    aoe_comm_active_range_ror(&c);
    assert(c.low == 1 && c.high == 3);
    c.turn = 10;
    c.last_turn[1] = 10;
    c.last_turn[2] = 3;
    assert(strcmp(aoe_comm_status_text_ror(&c, 2, text), "S4b3") == 0);
    c.last_turn[2] = 8;
    assert(strcmp(aoe_comm_status_text_ror(&c, 2, text), "S4b4") == 0);
    assert(strcmp(aoe_comm_status_text_ror(&c, 3, text), "S4b1") == 0);
    assert(strcmp(aoe_comm_status_text_ror(&c, 4, text), "S4b2") == 0);
    assert(strcmp(aoe_comm_slot_label_ror(&c, 1, "Ann"), "[1] Ann - S4b0") == 0);
    assert(strncmp(aoe_comm_ready_text_ror(&c), "Ready= AXC-", 11) == 0);
    assert(!aoe_comm_caught_up_ror(&c));
    c.last_turn[2] = 10;
    assert(aoe_comm_caught_up_ror(&c));
    c.stat[0][1] = 1;
    assert(!aoe_comm_all_stat0_ror(&c));
    c.stat[0][2] = 1;
    assert(aoe_comm_all_stat0_ror(&c));
    assert(aoe_comm_link_level_ror(&c) == 0xe);

    /* Pause / resume. */
    nmsgs = 0;
    aoe_comm_pause_ror(&c);
    assert(c.state == 4 && msgs[0] == 0x50);
    aoe_comm_resume_ror(&c);
    assert(c.state == 5 && msgs[1] == 0x55);
    c.is_host = 0;
    aoe_comm_pause_ror(&c);
    assert(msgs[2] == 0x57 && c.state == 5);
    c.is_host = 1;

    /* Commands. */
    assert(aoe_comm_command_seq_ror(&c, 5) == 1 && aoe_comm_command_seq_ror(&c, 5) == 2);
    c.multiplayer = 0;
    c.lead = 2;
    aoe_comm_send_command_ror(&c, "x", 1, 0, seq_fn);
    assert(qturn == 12 && qplayer == 1 && qseq == 3);
    c.multiplayer = 1;
    nmsgs = 0;
    aoe_comm_send_turn_markers_ror(&c, 1, 2);
    assert(nmsgs == 4 && msgs[0] == 0x51 && msgs[3] == 0x44);

    /* Chat masks. */
    aoe_comm_chat_some_ror(&c, "hi", to_slot);
    assert(memcmp(last_mask, "NNYNYNNN", 8) == 0);
    aoe_comm_chat_one_ror(&c, 1, 3, "x");
    assert(memcmp(last_mask, "NNNYNNNN", 8) == 0);

    /* Pings: only the quiet ones. */
    c.dpid[2] = 200;
    c.fast_time[2] = clock_ms - 9000;
    aoe_comm_ping_quiet_ror(&c, 0);
    assert(sent_to == 200 && sent_buf[0] == 0x31 && c.fast_time[2] == clock_ms);
    assert(aoe_comm_fast_send_ror(&c, self_id, &m, 1) == (int32_t)0x80004005);

    /* Sharing the player data. */
    c.data_time = clock_ms - 100;
    c.dirty = 1;
    assert(aoe_comm_share_data_ror(&c, 0) == 0 && c.dirty == 1);
    clock_ms += 3000;
    assert(aoe_comm_share_data_ror(&c, 0) == 0 && c.dirty == 0 && sent_n == 0x1d0);

    /* Guaranteed delivery. */
    c.gtd = 1;
    pending[2] = 1;
    pending[3] = 1;
    assert(aoe_comm_tx_store_ror(&c, 11, "abc", 3, pending) && aoe_comm_tx_count_ror(&c) == 1);
    aoe_comm_tx_purge_ror(&c);
    assert(aoe_comm_tx_count_ror(&c) == 1); /* slot 2 still owes an ack */
    aoe_comm_ack_received_ror(&c, 11, 2);
    aoe_comm_tx_purge_ror(&c); /* slot 3 is a computer: not waited for */
    assert(aoe_comm_tx_count_ror(&c) == 0);
    /* Received out of order from slot 2 (dpid 200): 2 then 1. */
    {
        uint8_t one = 1, two = 2;
        aoe_comm_rx_store_ror(&c, 2, &two, 1, 200, 0);
        aoe_comm_rx_store_ror(&c, 2, &two, 1, 200, 0); /* once */
        aoe_comm_rx_store_ror(&c, 1, &one, 1, 200, 0);
        c.rx_serial[2] = 0;
        aoe_comm_rx_deliver_ror(&c);
        /* The scan finds serial 2 first (not next), then 1 (runs). */
        assert(runs == 1 && run_serial[0] == 1);
        c.rx_serial[2] = 1;
        aoe_comm_rx_deliver_ror(&c);
        assert(runs == 2 && run_serial[1] == 2);
    }
    assert(aoe_comm_send_ack_ror(&c, 11, 2) && sent_buf[0] == 'A' && c.bytes_sent == 8);
    assert(aoe_comm_request_resend_ror(&c, 12, 2) && sent_buf[0] == 'X');
    assert(!aoe_comm_request_resend_ror(&c, 12, 2)); /* within 0.5 s */
    aoe_comm_gtd_reset_ror(&c);

    /* Stops and drops. */
    nposts = 0;
    aoe_comm_set_stop_turn_ror(&c, 2, 30);
    aoe_comm_check_stop_turn_ror(&c, 30);
    assert(c.player_state[2] == 3 && c.stop_turn[2] == 0);
    c.dpid[1] = 100;
    self_id = 100;
    c.self = 0;
    aoe_comm_find_self_ror(&c);
    assert(c.self == 1);
    c.turn = 3;
    c.player_state[4] = 2;
    c.dpid[4] = 400;
    drops = 0;
    assert(aoe_comm_drop_player_ror(&c, 4) && drops == 1);
    puts("comm_handler_test: ok");
    return 0;
}
