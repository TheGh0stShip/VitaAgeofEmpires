#include "game_frame_step.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPS (c->ops)
#define CTX (c->ops->context)

/* 0x00554cf0: the blank the labels and name tables start with (one
   space, not an empty string). */
static const char k_blank[] = " ";

static void log_line(AoeCommRor *c, const char *text) {
    /* 0x0042c330 on the log object 0x00580db8 (empty in this build). */
    if (OPS->log) OPS->log(CTX, text);
}

/* The (count + 1) * 0x80-byte name table, each entry the blank (the
   original strcpy writes only its two bytes; calloc zeroes the rest). */
static char (*name_table(uint32_t entries))[0x80] {
    char (*t)[0x80] = calloc(entries, sizeof *t);
    uint32_t i;
    if (!t) return NULL;
    for (i = 0; i < entries; ++i) memcpy(t[i], k_blank, sizeof k_blank);
    return t;
}

/* ---- 0x0041e1c0 ---- */
AoeCommRor *aoe_comm_construct_ror(AoeCommRor *c, AoeCommPartsRor *parts,
                                   const AoeCommConstructOpsRor *ops, uint32_t window,
                                   uint8_t players, void *chat, const uint32_t guid[4]) {
    char line[160];
    uint32_t now, i;
    /* The comm's ops are required (the shared slot helpers use them). */
    if (!c || !c->ops || !parts) return c;

    /* The traffic counters roll over: last = current, peak = the larger. */
    parts->last_rx_bytes = c->rx_bytes;
    if (c->rx_bytes > c->field_12bc) c->field_12bc = c->rx_bytes;
    c->rx_bytes = 0;
    parts->last_tx_bytes = c->bytes_sent;
    if (c->bytes_sent > c->field_12c4) c->field_12c4 = c->bytes_sent;
    c->bytes_sent = 0;
    parts->queue_b = NULL;
    parts->queue_a = NULL;
    c->field_14e4 = 0;
    memset(c->guid_1318, 0, sizeof c->guid_1318);
    c->status = 1;
    for (i = 0; i < 4; ++i) c->session_guid[i] = guid ? guid[i] : 0;
    memcpy(c->guid_12fc, c->guid_1318, sizeof c->guid_12fc);
    memcpy(c->guid_12dc, c->guid_1318, sizeof c->guid_12dc);
    c->window = window;
    parts->chat = chat;
    c->count = (uint16_t)players;

    /* 0x00555420 with 0x00555454; 0x005553e8 with 0x00555454 and
       0x00555404 (the source file and its build date). The build time
       0x005553dc ("22:19:57") then has its ':' made '0' in a stack
       buffer that nothing reads. */
    snprintf(line, sizeof line, "COMM:  RGE COMMUNICTIONS STARTED ---------------%s",
             "D:\\ROR\\Com_hand.cpp");
    log_line(c, line);
    snprintf(line, sizeof line, "COMM:  %s  Version... %s", "D:\\ROR\\Com_hand.cpp",
             "Wed Oct 27 18:28:14 1999");
    log_line(c, line);

    c->field_12a4 = 0x2e2887;
    c->names = name_table((uint32_t)players + 1);
    c->infos = name_table((uint32_t)players + 1);
    parts->queue_a = ops && ops->queue_new ? ops->queue_new(ops->context, 0x1f5) : NULL;
    /* Array new with the element constructors 0x41f240 / 0x41f290, which
       zero every field. */
    c->tx = calloc(AOE_COMM_TX_SLOTS, sizeof *c->tx);
    c->rx = calloc(AOE_COMM_RX_SLOTS, sizeof *c->rx);
    parts->poster = ops && ops->poster_new ? ops->poster_new(ops->context, window) : NULL;
    parts->lobby = ops && ops->lobby_new ? ops->lobby_new(ops->context, window) : NULL;
    parts->speed = ops && ops->speed_new ? ops->speed_new(ops->context, c) : NULL;
    parts->sync = ops && ops->sync_new ? ops->sync_new(ops->context, c) : NULL;

    /* The reset 0x00422a70 inlined, without its 0x004257a0 call. */
    now = OPS->now_ms ? OPS->now_ms(CTX) : 0;
    c->rx_bytes = c->field_12bc = c->bytes_sent = c->field_12c4 = 0;
    c->field_10f0 = c->field_10f4 = c->field_1748 = 0;
    c->field_1340 = c->field_1338 = c->rx_messages = 0;
    c->gtd = 0;
    c->field_1328 = c->field_132c = c->field_1330 = 0;
    c->lobby_joined = 0;
    c->field_14e4 = 0;
    c->field_10ec = c->field_10e8 = 0;
    if (ops && ops->log_elapsed) ops->log_elapsed(ops->context);
    c->data_time = now;
    c->field_1298 = c->field_12a0 = c->field_1288 = c->field_1518 = 0;
    memcpy(c->label, k_blank, sizeof k_blank);
    memcpy(c->name, k_blank, sizeof k_blank);
    memcpy(c->info, k_blank, sizeof k_blank);
    c->field_c = c->field_10 = c->field_12d0 = 0;
    c->low = 1;
    c->high = c->count;
    c->field_1718 = 0;
    c->state = 3;
    c->is_host = 0;
    c->field_20 = 0;
    c->lead = 2;
    c->turn = 4;
    c->field_155c = 4;
    c->dirty = 0;
    c->field_10dc = 0x32;
    c->self = 0;
    c->multiplayer = 0;
    c->field_128c = c->field_1294 = 0;
    log_line(c, "Initialize Player List");
    /* 0x00424330(slot, 0, " ", " "); the slot arrays hold ten entries. */
    for (i = 1; i <= c->count && i < AOE_COMM_SLOTS; ++i) aoe_comm_init_player_ror(c, i, 0);
    c->dirty = 0;
    c->data_time = 0;
    aoe_comm_clear_serials_ror(c);
    if (c->tx && c->rx) aoe_comm_gtd_reset_ror(c);
    memset(c->field_13c0, 0, sizeof c->field_13c0);
    c->field_8 = 1;
    if (OPS->now_ms) aoe_comm_reset_ping_times_ror(c);
    c->tx_pending = 0;
    if (parts->queue_a && ops && ops->queue_flush) ops->queue_flush(ops->context, parts->queue_a);
    if (parts->queue_b && ops && ops->queue_flush) ops->queue_flush(ops->context, parts->queue_b);
    c->field_12a8 = 0;
    memset(c->field_136c, 0, sizeof c->field_136c);
    memset(c->field_149c, 0, sizeof c->field_149c);
    memset(c->dropped, 0, sizeof c->dropped);
    c->data_size = 0;
    c->data = NULL;
    return c;
}
