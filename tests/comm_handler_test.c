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
static char last_log[200];
static void log_(void *c, const char *t) { (void)c; snprintf(last_log, sizeof last_log, "%s", t); }
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

static int32_t dp_created, dp_closed, released, unlink_left = 3;
static int32_t d_create(void *c, const uint32_t g[4]) { (void)c; (void)g; dp_created++; return 0; }
static int32_t d_release(void *c) { (void)c; released++; dp = 0; return 0; }
static int32_t d_close(void *c) { (void)c; dp_closed++; return 0; }
static int32_t d_caps(void *c) { (void)c; return 0; }
static int32_t d_session(void *c, uint32_t g[4], int32_t *p) { (void)c; g[0] = 9; *p = 3; return 0; }
static int32_t d_create_player(void *c, const char *n, const char *i, int32_t *id) { (void)c; (void)n; (void)i; *id = 555; return 0; }
static int32_t d_destroy(void *c, int32_t id) { (void)c; (void)id; return 0; }
static int32_t d_pcaps(void *c, int32_t id, int32_t o[3]) { (void)c; (void)id; o[0] = 1; o[1] = 2; o[2] = 3; return 0; }
static int32_t d_pdata(void *c, int32_t id) { (void)c; return id == 400 ? 1 : 0; }
static void d_self(void *c, int32_t id) { (void)c; self_id = id; }
static uint8_t first_buf = 1, turn_buf = 2;
static uint8_t *d_first(void *c) { (void)c; return &first_buf; }
static uint8_t *d_turn(void *c, uint32_t t) { (void)c; (void)t; return &turn_buf; }
static int32_t d_unlink(void *c) { (void)c; return --unlink_left; }
static const AoeCommDpOpsRor dops = {NULL, d_create, d_release, d_close, d_caps, d_session,
                                     d_create_player, d_destroy, d_pcaps, d_pdata, d_self,
                                     d_first, d_turn, d_unlink};
static int lobby_sent;
static void l_release(void *c, void *i) { (void)c; (void)i; }
static int32_t l_send(void *c, void *i, const uint8_t *d, uint32_t n) { (void)c; (void)i; lobby_sent = (int)n; assert(d[0] == 5 && d[0x2c] == 'z'); return 0; }
static void l_free(void *c, const uint8_t *s) { (void)c; (void)s; }
static const AoeLobbyOpsRor lops = {NULL, l_release, l_send, l_free};

static const AoeCommOpsRor ops = {
    NULL, dp_present, self_dpid, dp_send, set_data, message, chat, post, log_, report, drop,
    refresh, run, queue_local, NULL, NULL, NULL, string, now, srand_, rand_, block};

static int32_t en_q_hr, en_arg;
static char en_log[64];
static int32_t en_query(void *c, void *i, void **o) { (void)c; *o = i; return en_q_hr; }
static int32_t en_enable(void *c, void *d, int32_t e) { (void)c; (void)d; en_arg = e; return 7; }
static int32_t en_release(void *c, void *d) { (void)c; (void)d; return 2; }
static void en_logf(void *c, const char *l) { (void)c; snprintf(en_log, sizeof en_log, "%s", l); }

static void enable_new_players_tests(void) {
    AoeCommEnableOpsRor o = {NULL, en_query, en_enable, en_release, en_logf};
    int iface;
    assert(aoe_comm_enable_new_players_ror(NULL, 1, &o) == (int32_t)0x80070057u);
    assert(aoe_comm_enable_new_players_ror(&iface, 1, &o) == 7 && en_arg == 1 &&
           strcmp(en_log, "Released to links=2") == 0);
    en_q_hr = (int32_t)0x80004002u;
    assert(aoe_comm_enable_new_players_ror(&iface, 0, &o) == (int32_t)0x80004002u && en_arg == 1);
}

int main(void) {
    enable_new_players_tests();
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
    /* Sessions. */
    {
        int32_t players = 0;
        uint32_t guid[4] = {0, 0, 0, 0};
        dp = 0;
        assert(aoe_comm_dp_create_ror(&c, &dops) == 0 && dp_created == 1);
        dp = 1;
        assert(aoe_comm_dp_create_ror(&c, &dops) == (int32_t)0x88770005);
        assert(aoe_comm_dp_session_ror(&c, &dops, &players) == 0 && players == 3 && c.guid_12fc[0] == 9);
        assert(aoe_comm_dp_add_self_ror(&c, &dops) && self_id == 555 && c.caps[5] == 3 && c.gtd == 1);
        assert(aoe_comm_dp_close_ror(&c, &dops) == 0 && c.guid_12fc[0] == 0 && c.last_turn[1] == 0);
        drops = 0;
        aoe_comm_check_players_ror(&c, &dops);
        assert(drops == 1); /* slot 4 (dpid 400) cannot be read */
        c.multiplayer = 0;
        assert(*aoe_comm_next_command_ror(&c, &dops) == 1);
        c.multiplayer = 1;
        c.state = 5;
        assert(*aoe_comm_next_command_ror(&c, &dops) == 2);
        assert(aoe_comm_dp_release_player_ror(&c, &dops) == 0 && self_id == 0 && c.self == 0);
        aoe_comm_dp_release_ror(&c, &dops);
        assert(released == 1 && !dp);
        (void)guid;
    }
    {
        AoeLobbyRor l;
        uint32_t g[4] = {1, 2, 3, 4};
        aoe_lobby_init_ror(&l, 7);
        assert(aoe_lobby_send_ror(&l, &lops, g, "z", 1, 0, 0, 0, 0) == 0 && lobby_sent == 0);
        l.launched = 1;
        assert(aoe_lobby_send_ror(&l, &lops, g, "z", 1, 0, 0, 0, 0) == 0 && lobby_sent == 0x31);
        assert(aoe_lobby_hosting_ror(&l, 2) == 1 && aoe_lobby_hosting_ror(&l, 1) == 0);
        {
            int32_t v[4] = {9, 8, 7, 6};
            assert(aoe_lobby_send_options_ror(&l, &lops, g, "z", 1, v, &ops));
            assert(!strcmp(last_log, "Sending zone message."));
            l.launched = 0;
            assert(!aoe_lobby_send_options_ror(&l, &lops, g, "z", 1, v, &ops));
            assert(!strcmp(last_log, "NOT LOBBY LAUNCHED.  EXITING"));
            l.launched = 1;
        }
        aoe_lobby_release_ror(&l, &lops);
        assert(!aoe_lobby_launched_ror(&l));
    }
    {
        static AoeCommRor gc;
        int n = nposts;
        gc.turn = 12;
        last_log[0] = 0;
        aoe_comm_gtd_rx_log_ror(&gc, &ops, 3, 2, 14, 'A', 5);
        assert(last_log[0] == 0);
        gc.gtd = 1;
        aoe_comm_gtd_rx_log_ror(&gc, &ops, 3, 2, 14, 'A', 5);
        assert(!strcmp(last_log, "GTD: RX Ser#3 From P#2 T#12 ToExecT#14 Cmd=65 'A' Seq=5"));
        aoe_comm_post_ror(&ops, 77);
        assert(nposts == n + 1 && posts[n][0] == 77 && posts[n][1] == 0);
    }
    puts("comm_handler_test: ok");
    return 0;
}
