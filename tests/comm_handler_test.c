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

/* The receive poll's queue: data messages, then a system message. */
static int rq_left, rq_sys, rq_runs;
static int32_t rq_count(void *c, int32_t id, uint32_t *n) { (void)c; assert(id == 100); *n = 12; return 0; }
static int32_t rq_recv(void *c, uint32_t flags, int32_t *from, int32_t *to, void *d, uint32_t *n) {
    (void)c;
    if (rq_left == 0) return (int32_t)0x887700beu;
    *from = rq_left == 1 ? 0 : 7; *to = 100; *n = 3;
    if (flags == 1) { memcpy(d, "abc", 3); rq_left--; }
    return 0;
}
static void rq_system(void *c, uint32_t n, const uint8_t *d, int32_t to) { (void)c; assert(n == 3 && d[3] == 0 && to == 100); rq_sys++; }
static void rq_run(void *c, int32_t n, const uint8_t *d, int32_t from, int32_t to, int f) {
    (void)c; assert(n == 3 && !memcmp(d, "abc", 4) && from == 7 && to == 100 && f == 0); rq_runs++;
}
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


static int32_t oc_hr; static uint32_t oc_players; static char oc_label[64]; static int chat_cleared, posted;
static int lob_ok = 1, lob_has = 1, lob_named = 1, lob_host = 1;
static int32_t d_open_create(void *c, const uint32_t g[4], uint32_t n, const char *label) {
    (void)c; (void)g; oc_players = n; snprintf(oc_label, sizeof oc_label, "%s", label); return oc_hr;
}
static void d_chat_clear(void *c) { (void)c; ++chat_cleared; }
static void d_post_error(void *c, uint32_t code) { (void)c; posted = (int)code; }
static int d_lobby_connect(void *c, int *has) { (void)c; *has = lob_has; return lob_ok; }
static int d_lobby_name(void *c, char *o, int n) { (void)c; snprintf(o, (size_t)n, "Lobby"); return lob_named; }
static const char *d_lobby_ptr(void *c) { (void)c; return "LobbyName"; }
static int d_lobby_host(void *c) { (void)c; return lob_host; }

static void session_tests(void) {
    static AoeCommRor c;
    AoeCommDpOpsRor d = dops;
    d.open_create = d_open_create; d.chat_clear = d_chat_clear; d.post_error = d_post_error;
    d.lobby_connect = d_lobby_connect; d.lobby_player_name = d_lobby_name;
    d.lobby_name_pointer = d_lobby_ptr; d.lobby_hosting = d_lobby_host;
    memset(&c, 0, sizeof c);
    c.ops = &ops;
    c.count = 4;
    snprintf(c.label, sizeof c.label, "me [x]");
    oc_hr = 0;
    assert(aoe_comm_create_session_ror(&c, &d, 123) == 1);
    assert(c.multiplayer == 1 && c.lead == 2 && c.turn == 4 && c.field_155c == 4 &&
           chat_cleared == 1 && c.is_host == 1 && oc_players == 4 &&
           strcmp(oc_label, "me [x]") == 0 && c.self == 1 && c.field_1714 == 1 &&
           c.player_state[1] == 2 && c.field_20 == 1 && c.state == 3 && c.field_14e4 == 0x7d1);
    oc_hr = (int32_t)0x887700aa;
    memset(&c, 0, sizeof c); c.ops = &ops;
    assert(aoe_comm_create_session_ror(&c, &d, 1) == 0 && posted == 0x17ae && c.state != 1);
    oc_hr = (int32_t)0x80004005;
    memset(&c, 0, sizeof c); c.ops = &ops;
    assert(aoe_comm_create_session_ror(&c, &d, 1) == 0 && c.state == 1);
    /* The lobby join: hosting makes slot 1 ours; no name fails (0). */
    memset(&c, 0, sizeof c); c.ops = &ops;
    assert(aoe_comm_join_lobby_ror(&c, &d, 5) == 1 && c.lobby_joined == 1 && c.multiplayer == 1 &&
           c.state == 3 && c.is_host == 1 && c.self == 1 && c.player_state[1] == 2 &&
           c.field_14e4 == 0x7d1 && strcmp(c.name, "LobbyName") == 0);
    lob_host = 0; lob_named = 0;
    memset(&c, 0, sizeof c); c.ops = &ops;
    assert(aoe_comm_join_lobby_ror(&c, &d, 5) == 0 && c.lobby_joined == 0 && c.is_host == 0 &&
           c.state == 3 && c.multiplayer == 1);
    lob_has = 0;
    memset(&c, 0, sizeof c); c.ops = &ops;
    assert(aoe_comm_join_lobby_ror(&c, &d, 5) == 0 && c.multiplayer == 0);
    lob_ok = 0;
    assert(aoe_comm_join_lobby_ror(&c, &d, 5) == 0);
}

static int32_t oj_hr; static int oj_posted; static int32_t lb_create_hr, lb_connect_hr, lb_qi_hr, lb_read_hr; static int lb_iface = 1, lb_launched = -1, lb_enum, lb_patched_flags, lb_patched_max;
static int32_t d_open_join(void *c, const uint32_t sg[4], const uint32_t ag[4]) { (void)c; (void)sg; (void)ag; return oj_hr; }
static void d_oj_post(void *c, uint32_t code) { (void)c; oj_posted = (int)code; }
static void lb_free(void *c) { (void)c; }
static int32_t lb_create(void *c) { (void)c; return lb_create_hr; }
static int lb_interface(void *c) { (void)c; return lb_iface; }
static int32_t lb_size(void *c, uint32_t *n) { (void)c; *n = 64; return 0; }
static int32_t lb_read(void *c) { (void)c; return lb_read_hr; }
static void lb_patch(void *c, uint32_t f, uint32_t m) { (void)c; lb_patched_flags = (int)f; lb_patched_max = (int)m; }
static void lb_write(void *c) { (void)c; }
static int32_t lb_connect(void *c) { (void)c; return lb_connect_hr; }
static int32_t lb_qi(void *c, int *has) { (void)c; *has = 1; return lb_qi_hr; }
static void lb_enum_f(void *c) { (void)c; ++lb_enum; }
static void lb_set_launched(void *c, int on) { (void)c; lb_launched = on; }
static void join_lobby_tests(void) {
    static AoeCommRor c;
    AoeCommJoinOpsRor j = {NULL, d_open_join, d_oj_post};
    AoeCommLobbyOpsRor l = {NULL, lb_free, lb_create, lb_interface, lb_size, lb_read, lb_patch, lb_write, lb_connect, lb_qi, lb_enum_f, lb_set_launched};
    uint32_t guid[4] = {1, 2, 3, 4};
    int has = 0;
    memset(&c, 0, sizeof c); c.ops = &ops;
    dp = 1;
    oj_hr = 0;
    assert(aoe_comm_join_session_ror(&c, &dops, &j, guid) == 1 && c.state == 3 && c.is_host == 0 && c.guid_12fc[0] == 1);
    oj_hr = (int32_t)0x8877014a;
    assert(aoe_comm_join_session_ror(&c, &dops, &j, guid) == 0 && oj_posted == 0x17ab);
    oj_hr = (int32_t)0x80004005; c.state = 0;
    assert(aoe_comm_join_session_ror(&c, &dops, &j, guid) == 0 && c.state == 1);
    oj_hr = (int32_t)0x8877015e; c.state = 0;
    assert(aoe_comm_join_session_ror(&c, &dops, &j, guid) == 0 && c.state == 0);
    /* The lobby connect: the whole path; a connect error 0x887700f0
       quietly answers 0xff; no interface 0; a read error clears the flag. */
    assert(aoe_comm_lobby_connect_ror(&c, &l, &has) == 1 && has == 1 && lb_enum == 1 &&
           lb_launched == 1 && lb_patched_flags == 0x44 && lb_patched_max == 9);
    lb_connect_hr = (int32_t)0x887700f0;
    assert(aoe_comm_lobby_connect_ror(&c, &l, &has) == 0xff);
    lb_connect_hr = -5;
    assert(aoe_comm_lobby_connect_ror(&c, &l, &has) == 0xff);
    lb_connect_hr = 0; lb_read_hr = -1;
    assert(aoe_comm_lobby_connect_ror(&c, &l, &has) == 0 && lb_launched == 0);
    lb_read_hr = 0; lb_iface = 0;
    assert(aoe_comm_lobby_connect_ror(&c, &l, &has) == 0);
    lb_iface = 1; lb_create_hr = -1;
    assert(aoe_comm_lobby_connect_ror(&c, &l, &has) == 0);
}
/* 0x00424760: slot 3 (computer) moves into empty slot 2, slot 1 stays. */
static void compact_tests(void) {
    static AoeCommRor c;
    static AoeCommOpsRor o;
    static char names[10][0x80], infos[10][0x80];
    o.self_dpid = self_dpid; o.log = log_; o.post = post;
    c.ops = &o; c.count = 3; c.names = names; c.infos = infos;
    c.player_state[1] = 2; c.dpid[1] = 100;
    c.player_state[3] = 4; c.dpid[3] = 7; c.stat[5][3] = 55; c.last_turn[3] = 9;
    strcpy(names[3], "Cpu"); strcpy(infos[3], "x");
    nposts = 0; self_id = 100;
    aoe_comm_compact_players_ror(&c);
    assert(c.player_state[2] == 4 && c.dpid[2] == 7 && c.stat[5][2] == 55 && c.last_turn[2] == 9);
    assert(strcmp(names[2], "Cpu") == 0 && strcmp(infos[2], "x") == 0);
    assert(c.player_state[3] == 0 && c.dpid[3] == 0 && names[3][0] == 0 && c.stat[5][3] == 0);
    assert(c.self == 1 && nposts >= 1 && posts[nposts - 1][0] == 0x17b8);
}


static int rs_elapsed; static void rs_log_elapsed(void *c) { (void)c; ++rs_elapsed; }
static char cm_name[64], cm_text[64]; static int32_t cm_from, cm_a, cm_b; static int cm_zero, cm_53; static int32_t cm_stats[8];
static void cm_chat(void *c, const char *n, const char *t) { (void)c; snprintf(cm_name, 64, "%s", n); snprintf(cm_text, 64, "%s", t); }
static void cm_speed(void *c, int32_t from, uint8_t a, uint8_t b) { (void)c; cm_from = from; cm_a = a; cm_b = b; }
static void cm_stat(void *c, int32_t from, const int32_t st[8]) { (void)c; cm_from = from; memcpy(cm_stats, st, sizeof cm_stats); }
static void cm_speedzero(void *c) { (void)c; ++cm_zero; }
static void cm_send53(void *c, int16_t a, int16_t b, uint32_t e) { (void)c; (void)e; cm_53 = a * 1000 + b; }
static int32_t rr_reject_dpid, rr_reject_val, rr_dropped;
static void rr_reject(void *c, int32_t dpid, int32_t v) { (void)c; rr_reject_dpid = dpid; rr_reject_val = v; }
static void rr_drop(void *c, int32_t slot) { (void)c; rr_dropped = slot; }

static void reset_message_resend_tests(void) {
    static AoeCommRor c;
    static char names[AOE_COMM_SLOTS][0x80], infos[AOE_COMM_SLOTS][0x80];
    static AoeCommTxRor tx[AOE_COMM_TX_SLOTS];
    static AoeCommRxRor rx[AOE_COMM_RX_SLOTS];
    AoeCommResetOpsRor r = {NULL, rs_log_elapsed, 1, 0};
    AoeCommMessageOpsRor m = {NULL, cm_chat, cm_speed, cm_stat, cm_speedzero, cm_send53};
    AoeCommResendOpsRor rr = {NULL, rr_reject, rr_drop};
    uint8_t d[64];
    memset(&c, 0, sizeof c); c.ops = &ops; c.names = names; c.infos = infos; c.tx = tx; c.rx = rx;
    c.count = 3; c.state = 5; c.is_host = 1; c.gtd = 1; c.multiplayer = 1; c.self = 2; c.field_12a8 = 9;
    snprintf(c.label, sizeof c.label, "x");
    aoe_comm_reset_ror(&c, &dops, &r);
    assert(rs_elapsed == 1 && c.state == 3 && !c.is_host && c.lead == 2 && c.turn == 4 && c.field_155c == 4 &&
           c.field_10dc == 0x32 && c.self == 0 && !c.multiplayer && c.low == 1 && c.high == 3 &&
           c.field_8 == 1 && c.label[0] == 0 && c.field_12a8 == 0 && !c.gtd);
    /* Messages. */
    c.multiplayer = 1; c.is_host = 1; c.state = 5; c.turn = 10; c.lead = 2; c.self = 1;
    memset(d, 0, sizeof d);
    snprintf(names[2], 0x80, "Bob"); d[0] = 2; snprintf((char *)d + 0x11, 20, "hello");
    c.state = 3;
    assert(aoe_comm_handle_message_ror(&c, &m, 2, 0, 0x43, d, 40) == 1 && strcmp(cm_name, "Bob") == 0 && strcmp(cm_text, "hello") == 0);
    c.state = 5;
    d[4] = 12; d[8] = 7; d[9] = 8; /* turn 12 */
    assert(aoe_comm_handle_message_ror(&c, &m, 2, 0, 0x44, d, 16) == 1 && cm_a == 8 && cm_b == 7 && c.last_turn[2] == 10);
    c.turn = 20; d[4] = 12; c.state = 4;
    assert(aoe_comm_handle_message_ror(&c, &m, 2, 0, 0x44, d, 16) == 0); /* regression */
    c.dpid[3] = 77; c.low = 1; c.high = 3; memset(d, 0, 16); d[4] = 77;
    assert(aoe_comm_handle_message_ror(&c, &m, 2, 0, 0x4b, d, 16) == 1 && c.dropped[3] == 1 && drops >= 1);
    for (int i = 0; i < 8; ++i) d[i * 4] = (uint8_t)(i + 1);
    assert(aoe_comm_handle_message_ror(&c, &m, 3, 0, 0x4d, d, 32) == 1 && cm_from == 3 && cm_stats[7] == 8);
    assert(aoe_comm_handle_message_ror(&c, &m, 3, 0, 0x50, d, 8) == 1 && cm_zero == 1);
    d[0] = 2; d[1] = 0; d[2] = 5; d[3] = 0;
    assert(aoe_comm_handle_message_ror(&c, &m, 3, 0, 0x53, d, 8) == 1 && cm_53 == 2005);
    assert(aoe_comm_handle_message_ror(&c, &m, 3, 0, 0x7f, d, 8) == 1);
    c.multiplayer = 0;
    assert(aoe_comm_handle_message_ror(&c, &m, 3, 0, 0x55, d, 8) == 0);
    /* Resend: slot 2's serial 9 is stored twice; one send; too soon refused. */
    c.multiplayer = 1; c.gtd = 1; c.state = 3; dp = 1; c.dpid[2] = 55; self_id = 42;
    memset(tx, 0, sizeof tx);
    tx[0].serial = 9; tx[0].data = calloc(8, 1); tx[0].size = 8; tx[0].pending[2] = 1;
    tx[5].serial = 9; tx[5].data = calloc(4, 1); tx[5].size = 4; tx[5].pending[2] = 1;
    c.bytes_sent = 0;
    assert(aoe_comm_resend_ror(&c, &dops, &rr, 9, 2) == 1 && c.bytes_sent == 12 && tx[0].pending[2] == 0);
    assert(aoe_comm_resend_ror(&c, &dops, &rr, 9, 2) == 0); /* too soon */
    c.field_14e4 = 2001;
    assert(aoe_comm_resend_ror(&c, &dops, &rr, 10, 2) == 1 && rr_reject_dpid == 55 && rr_reject_val == 2000 && rr_dropped == 2);
    assert(aoe_comm_resend_ror(&c, &dops, &rr, 10, 4) == 0); /* no such player */
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

    /* Readiness: stored; sent only by a non-host. */
    {
        int32_t st[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        int np = nposts;
        sent_to = -5;
        assert(aoe_comm_set_ready_ror(&c, st) == 1);
        assert(c.stat[0][1] == 1 && c.stat[7][1] == 8 && sent_to == -5 && nposts == np + 1 &&
               posts[np][0] == 0x17a6);
        c.is_host = 0;
        c.field_12a4 = 0x11;
        c.gtd = 1;
        assert(aoe_comm_set_ready_ror(&c, st) == 1);
        assert(sent_to == 0 && sent_n == 0x28 && sent_buf[0] == 'R' && sent_buf[1] == 1 &&
               sent_buf[4] == 2 && sent_buf[0x1c] == 8 && sent_buf[0x20] == 0x11 &&
               sent_buf[0x24] == 1);
        c.is_host = 1;
        c.gtd = 0;
        c.field_12a4 = 0;
        memset(st, 0, sizeof st);
        for (int i = 0; i < 8; ++i) c.stat[i][1] = 0;
    }

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
    /* 0x0041f3e0: two data messages run, the system message ends it. */
    {
        AoeCommOpsRor rops = *c.ops;
        rops.dp_message_count = rq_count; rops.dp_receive = rq_recv;
        rops.system_message = rq_system; rops.run = rq_run;
        c.ops = &rops;
        dp = 1; rq_left = 3; self_id = 100;
        aoe_comm_receive_ror(&c);
        assert(rq_runs == 2 && rq_sys == 1 && c.rx_messages == 3 && c.rx_bytes == 9 &&
               strcmp(last_log, "!!! DPMsgCount = 12") == 0);
        aoe_comm_receive_ror(&c);
        assert(rq_runs == 2 && c.rx_messages == 3);
        c.ops = &ops;
    }
    /* 0x0041f330 / 0x0041f3d0: the pump polls only with messages waiting. */
    {
        AoeCommOpsRor rops = *c.ops;
        int32_t mp = c.multiplayer;
        uint8_t gtd = c.gtd;
        rops.dp_message_count = rq_count; rops.dp_receive = rq_recv;
        rops.system_message = rq_system; rops.run = rq_run;
        rops.gtd_send_store = NULL;
        c.ops = &rops;
        c.multiplayer = 0; rq_left = 2; rq_runs = 0; rq_sys = 0;
        aoe_comm_pump_ror(&c);
        assert(rq_left == 2);
        c.multiplayer = 1; c.gtd = 0; c.field_12a8 = 0;
        aoe_comm_pump_flagged_ror(&c);
        assert(c.field_12a8 == 1 && rq_left == 0 && rq_runs == 1 && rq_sys == 1);
        c.multiplayer = mp; c.gtd = gtd;
        c.ops = &ops;
    }
    puts("comm_handler_test: ok");
    session_tests();
    join_lobby_tests();
    compact_tests();
    reset_message_resend_tests();
    return 0;
}
