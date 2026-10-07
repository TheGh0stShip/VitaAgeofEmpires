#include "game_frame_step.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Patched RoR 1.0a 0x0041e1c0, the communications handler constructor.
   Evidence (objdump of build/patch_1_0c/EMPIRESX.EXE):
   - 0x41e1dc..0x41e232: +0x12b0 = +0x12b8, +0x12bc = max(+0x12b8,
     +0x12bc) (ja), +0x12b8 = 0; the same for +0x12b4 / +0x12c0 / +0x12c4.
   - 0x41e26f +0x12d4 = 1; 0x41e279..0x41e28d the four GUID dwords
     ([esp+0x54..0x60]) into +0x12ec; 0x41e2dc +0x130c the window;
     0x41e2e2 +0x18 the chat log; 0x41e2d6 movzx ax, byte: +0x12d8.
   - 0x41e2f3 / 0x41e30e two 0x42c330 lines: 0x00555420 "COMM:  RGE
     COMMUNICTIONS STARTED ---------------%s" with 0x00555454
     "D:\ROR\Com_hand.cpp"; 0x005553e8 "COMM:  %s  Version... %s" with
     0x00555454 and 0x00555404 "Wed Oct 27 18:28:14 1999".
   - 0x41e35a +0x12a4 = 0x2e2887; 0x41e376 / 0x41e3df new((count + 1)
     << 7) filled with 0x00554cf0, the bytes 20 00 (" ").
   - 0x41e457 0x427ce0(0x1f5); 0x41e4a6 / 0x41e4f3 0x1f6 elements of 0x38
     / 0x18 bytes (0x5277ed with 0x41f240 / 0x41f290); 0x41e52f
     0x41d6a0(window); 0x41e563 0x425a10(window); 0x41e596 0x428990(this);
     0x41e5c9 0x429330(this).
   - 0x41e5dc timeGetTime, 0x41e656 0x42c100, 0x41e65b +0x1554 = time;
     0x41e778 "Initialize Player List" (0x005553c4); 0x41e799 0x424330 for
     slots 1..count; 0x41e7b4 +0x1554 = 0; 0x41e7da 0x423970; 0x41e7f1 /
     0x41e800 0x427f30 on +0x14c4 / +0x14c8 when set (+0x14c8 was zeroed at
     0x41e238); 0x41e82e +0x171c = 0, 0x41e834 +0x04 = 0; eax = this. */

static char trace[64];
static void tag(char t) {
    size_t n = strlen(trace);
    assert(n + 1 < sizeof trace);
    trace[n] = t;
}

static char lines[4][160];
static int nlines;
static void log_(void *c, const char *t) {
    (void)c;
    if (!strcmp(t, "Initialize Player List")) tag('I');
    if (!strncmp(t, "COMM:", 5)) {
        tag('C');
        assert(nlines < 4);
        snprintf(lines[nlines++], sizeof lines[0], "%s", t);
    }
}
static uint32_t clock_ms;
static uint32_t now_(void *c) { (void)c; tag('T'); return clock_ms++; }
static void flush_player(void *c, uint32_t slot) { (void)c; (void)slot; tag('f'); }

static const AoeCommOpsRor comm_ops = {.log = log_, .now_ms = now_,
                                       .flush_player_queue = flush_player};

static int queue_obj, poster_obj, lobby_obj, speed_obj, sync_obj, no_queue;
static int32_t queue_capacity;
static uint32_t poster_window, lobby_window;
static AoeCommRor *speed_comm, *sync_comm;
static void *flushed[2];
static int nflushed;
static void *queue_new(void *c, int32_t n) { (void)c; tag('Q'); queue_capacity = n; return no_queue ? NULL : &queue_obj; }
static void queue_flush(void *c, void *q) { (void)c; tag('F'); flushed[nflushed++] = q; }
static void *poster_new(void *c, uint32_t w) { (void)c; tag('P'); poster_window = w; return &poster_obj; }
static void *lobby_new(void *c, uint32_t w) { (void)c; tag('L'); lobby_window = w; return &lobby_obj; }
static void *speed_new(void *c, AoeCommRor *comm) { (void)c; tag('S'); speed_comm = comm; return &speed_obj; }
static void *sync_new(void *c, AoeCommRor *comm) { (void)c; tag('Y'); sync_comm = comm; return &sync_obj; }
static void elapsed(void *c) { (void)c; tag('E'); }

static const AoeCommConstructOpsRor ops = {NULL, queue_new, queue_flush, poster_new,
                                           lobby_new, speed_new, sync_new, elapsed};

static void reset_trace(void) {
    memset(trace, 0, sizeof trace);
    nlines = 0;
    nflushed = 0;
    flushed[0] = flushed[1] = NULL;
    clock_ms = 1000;
}

static void release(AoeCommRor *c) {
    free(c->names);
    free(c->infos);
    free(c->tx);
    free(c->rx);
    free(c);
}

static AoeCommRor *fresh(void) {
    AoeCommRor *c = calloc(1, sizeof *c);
    assert(c);
    c->ops = &comm_ops;
    return c;
}

static void test_eight_players(void) {
    static const uint32_t guid[4] = {0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u};
    static uint8_t extra[4];
    AoeCommRor *c = fresh();
    AoeCommPartsRor parts;
    int chat;
    uint32_t i;
    memset(&parts, 0xcd, sizeof parts);
    reset_trace();
    /* Stale values the constructor must roll over or clear. */
    c->rx_bytes = 100; c->field_12bc = 50;
    c->bytes_sent = 7; c->field_12c4 = 9;
    c->guid_1318[2] = 5; c->guid_12fc[0] = 6; c->guid_12dc[3] = 7;
    c->field_136c[1] = 1; c->field_149c[3] = 5; c->dropped[2] = 1; c->field_13c0[8] = 1;
    c->is_host = 1; c->multiplayer = 1; c->self = 3; c->state = 5; c->field_12a8 = 1;
    c->data = extra; c->data_size = 4; c->tx_pending = 2; c->lobby_joined = 1;

    assert(aoe_comm_construct_ror(c, &parts, &ops, 0x1234u, 8, &chat, guid) == c);

    /* Order: the two COMM lines, the queue, (the stores), the poster, the
       lobby, the speed controller, the sync checker, timeGetTime,
       0x42c100, "Initialize Player List", 0x427fd0 per slot (inside
       0x424330), 0x423970's timeGetTime, 0x427f30 on the queue. */
    assert(!strcmp(trace, "CCQPLSYTEIffffffffTF"));
    assert(!strcmp(lines[0], "COMM:  RGE COMMUNICTIONS STARTED ---------------D:\\ROR\\Com_hand.cpp"));
    assert(!strcmp(lines[1], "COMM:  D:\\ROR\\Com_hand.cpp  Version... Wed Oct 27 18:28:14 1999"));

    /* +0x12b0 / +0x12b4 keep the old counters; the tail clears the rest. */
    assert(parts.last_rx_bytes == 100 && parts.last_tx_bytes == 7);
    assert(c->rx_bytes == 0 && c->field_12bc == 0 && c->bytes_sent == 0 && c->field_12c4 == 0);
    for (i = 0; i < 4; ++i) {
        assert(c->session_guid[i] == guid[i]);
        assert(c->guid_1318[i] == 0 && c->guid_12fc[i] == 0 && c->guid_12dc[i] == 0);
    }
    assert(c->status == 1 && c->window == 0x1234u && parts.chat == &chat && c->count == 8);
    assert(c->field_12a4 == 0x2e2887);

    assert(c->names && c->infos);
    for (i = 0; i <= 8; ++i) assert(!strcmp(c->names[i], " ") && !strcmp(c->infos[i], " "));
    assert(queue_capacity == 0x1f5 && parts.queue_a == &queue_obj && parts.queue_b == NULL);
    assert(nflushed == 1 && flushed[0] == &queue_obj);
    assert(c->tx && c->rx);
    for (i = 0; i < AOE_COMM_TX_SLOTS; ++i) {
        assert(c->tx[i].serial == 0 && c->tx[i].data == NULL && c->tx[i].size == 0);
        assert(c->rx[i].serial == 0 && c->rx[i].data == NULL && c->rx[i].size == 0);
    }
    assert(parts.poster == &poster_obj && poster_window == 0x1234u);
    assert(parts.lobby == &lobby_obj && lobby_window == 0x1234u);
    assert(parts.speed == &speed_obj && speed_comm == c);
    assert(parts.sync == &sync_obj && sync_comm == c);

    /* The inlined 0x422a70 tail. */
    assert(!strcmp(c->label, " ") && !strcmp(c->name, " ") && !strcmp(c->info, " "));
    assert(c->low == 1 && c->high == 8);
    assert(c->state == 3 && c->is_host == 0 && c->field_20 == 0 && c->lead == 2);
    assert(c->turn == 4 && c->field_155c == 4 && c->field_10dc == 0x32);
    assert(c->self == 0 && c->multiplayer == 0 && c->dirty == 0 && c->data_time == 0);
    assert(c->field_8 == 1 && c->tx_pending == 0 && c->field_12a8 == 0 && c->lobby_joined == 0);
    for (i = 1; i <= 8; ++i) assert(c->time_1344[i] == 1001 && c->fast_time[i] == 1001);
    for (i = 0; i < AOE_COMM_SLOTS; ++i)
        assert(c->field_136c[i] == 0 && c->field_149c[i] == 0 && c->dropped[i] == 0);
    for (i = 0; i < 9; ++i) assert(c->field_13c0[i] == 0);
    assert(c->data == NULL && c->data_size == 0);
    release(c);
}

static void test_no_players_no_queue(void) {
    AoeCommRor *c = fresh();
    AoeCommPartsRor parts;
    memset(&parts, 0, sizeof parts);
    reset_trace();
    no_queue = 1;
    /* No GUID given: the session GUID stays zero; no slot is initialised
       (0x41e780 cmp word +0x12d8, 1; jb); 0x427f30 is skipped without a
       queue (0x41e7ef je). The tables still hold count + 1 = 1 entry. */
    assert(aoe_comm_construct_ror(c, &parts, &ops, 7u, 0, NULL, NULL) == c);
    no_queue = 0;
    assert(!strcmp(trace, "CCQPLSYTEIT"));
    assert(parts.queue_a == NULL && nflushed == 0);
    assert(c->count == 0 && c->high == 0 && c->low == 1);
    assert(!strcmp(c->names[0], " ") && !strcmp(c->infos[0], " "));
    assert(c->session_guid[0] == 0 && c->session_guid[3] == 0);
    release(c);
}

int main(void) {
    test_eight_players();
    test_no_players_no_queue();
    puts("game_frame_step_test ok");
    return 0;
}
