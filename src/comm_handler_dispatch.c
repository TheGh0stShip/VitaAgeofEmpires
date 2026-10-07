#include "comm_handler_dispatch.h"

#include <stdio.h>
#include <string.h>

#define OPS (c->ops)
#define CTX (c->ops->context)

/* 0x00554cf0: the one-space string the original copies into the label,
   the name and the info (and passes to 0x00424330 for the log line). */
static const char unlink_blank[] = " ";

static void unlink_log(AoeCommRor *c, const char *text) {
    if (OPS->log) OPS->log(CTX, text);
}

static void unlink_flush(AoeCommUnlinkRor *u, int which) {
    if (u && u->flush_queue) u->flush_queue(u->context, which);
}

/* The "Initialize Player List" block the cases share (e.g. 0x00422ec6..
   0x00422f0c): slots 1..count initialised by 0x00424330 with ID 0, then
   dirty (+0x1550) off and the data time (+0x1554) 0. */
static void unlink_init_players(AoeCommRor *c) {
    uint32_t slot;
    unlink_log(c, "Initialize Player List");
    for (slot = 1; slot <= c->count && slot < AOE_COMM_SLOTS; ++slot)
        aoe_comm_init_player_ror(c, slot, 0);
    c->dirty = 0;
    c->data_time = 0;
}

/* Levels 6 and 7 (0x004233a2 / 0x0042352c): the speed controller
   (+0x14d4) and the sync checker (+0x14d8) destroyed when present, then
   both created again (NULL when operator new 0x52601d fails). */
static void unlink_renew_objects(AoeCommUnlinkRor *u) {
    if (!u) return;
    if (u->speed && u->speed_destroy) u->speed_destroy(u->context, u->speed);
    if (u->sync && u->sync_destroy) u->sync_destroy(u->context, u->sync);
    u->speed = u->speed_create ? u->speed_create(u->context) : NULL;
    u->sync = u->sync_create ? u->sync_create(u->context) : NULL;
}

/* Levels 2 and 3 (0x00422d2d / 0x00422f9b): the body of the reset
   0x00422a70 inlined, without its first call (0x004257a0) and with
   multiplayer cleared first. */
static void unlink_reset(AoeCommRor *c, AoeCommUnlinkRor *u) {
    uint32_t now, slot;
    c->multiplayer = 0;
    now = OPS->now_ms(CTX);
    c->rx_bytes = 0;
    c->field_12bc = 0;
    c->bytes_sent = 0;
    c->field_12c4 = 0;
    c->field_10f0 = c->field_10f4 = c->field_1748 = 0;
    c->field_1340 = c->field_1338 = c->rx_messages = 0;
    c->gtd = 0;
    c->field_1328 = c->field_132c = c->field_1330 = 0;
    c->lobby_joined = 0;
    c->field_14e4 = 0;
    c->field_10ec = c->field_10e8 = 0;
    if (u && u->log_elapsed) u->log_elapsed(u->context);
    c->field_1298 = c->field_12a0 = c->field_1288 = c->field_1518 = 0;
    c->data_time = now;
    memcpy(c->label, unlink_blank, sizeof unlink_blank);
    memcpy(c->name, unlink_blank, sizeof unlink_blank);
    memcpy(c->info, unlink_blank, sizeof unlink_blank);
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
    unlink_init_players(c);
    aoe_comm_clear_serials_ror(c);
    aoe_comm_gtd_reset_ror(c);
    /* rep stos of 9 dwords at +0x13c0: the stop turns (+0x13bc) of slots
       1..9; field_13c0 is the same memory as the reset 0x00422a70 names it. */
    for (slot = 1; slot < AOE_COMM_SLOTS; ++slot) c->stop_turn[slot] = 0;
    memset(c->field_13c0, 0, sizeof c->field_13c0);
    c->field_8 = 1;
    aoe_comm_reset_ping_times_ror(c);
    c->tx_pending = 0;
    if (u && u->has_queue_a) unlink_flush(u, 0);
    if (u && u->has_queue_b) unlink_flush(u, 1);
    c->field_12a8 = 0;
    memset(c->field_136c, 0, sizeof c->field_136c);
    memset(c->field_149c, 0, sizeof c->field_149c);
    memset(c->dropped, 0, sizeof c->dropped);
}

/* Level 11's tail, also level 5's (the shared block at 0x00423646). */
static void unlink_forget_self(AoeCommRor *c, const AoeCommDpOpsRor *dp) {
    c->self = 0;
    dp->set_self_dpid(dp->context, 0);
    c->field_12a8 = 0;
}

/* ---- 0x00422ce0 ---- */
int32_t aoe_comm_unlink_step_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp, AoeCommUnlinkRor *u) {
    char line[64];
    snprintf(line, sizeof line, "Unlinking current level (%d)", (int)c->status);
    unlink_log(c, line);
    /* cmp eax,0xf; ja: negative levels are "Unknown" too. */
    switch ((uint32_t)c->status) {
    case 0:
    case 1:
        break;
    case 2:
        unlink_reset(c, u);
        c->self = 1;
        c->name[0] = '\0';
        c->info[0] = '\0';
        break;
    case 3:
        unlink_reset(c, u);
        c->self = 1;
        c->player_state[1] = 2; /* +0x16d8 */
        break;
    case 4:
        c->field_1338 = 0;
        c->rx_messages = 0;
        c->gtd = 1;
        c->field_1328 = c->field_132c = c->field_1330 = 0;
        c->lobby_joined = 0;
        c->field_12a8 = 0;
        unlink_log(c, "UNLINK ReleaseComm");
        aoe_comm_dp_release_ror(c, dp);
        break;
    case 5:
        unlink_log(c, "UNLINK ServiceAvailable");
        aoe_comm_dp_release_ror(c, dp);
        unlink_init_players(c);
        if (dp->chat_clear) dp->chat_clear(dp->context);
        c->label[0] = '\0';
        unlink_forget_self(c, dp);
        break;
    case 6:
        aoe_comm_gtd_reset_ror(c);
        aoe_comm_drop_early_ror(c);
        unlink_log(c, "UNLINK SessionAvailable");
        if (dp->chat_clear) dp->chat_clear(dp->context);
        aoe_comm_dp_close_ror(c, dp);
        c->field_14e4 = 0;
        unlink_init_players(c);
        c->high = c->count;
        c->state = 3;
        c->lead = 2;
        c->turn = 4;
        c->field_155c = 4;
        c->self = 0;
        dp->set_self_dpid(dp->context, 0);
        aoe_comm_clear_serials_ror(c);
        c->field_1718 = 0;
        c->field_12a8 = 0;
        unlink_renew_objects(u);
        memcpy(c->guid_12fc, c->guid_1318, sizeof c->guid_12fc);
        break;
    case 7:
        unlink_log(c, "UNLINK SerssionHosted");
        c->is_host = 0;
        c->field_1714 = 0;
        c->high = c->count;
        c->state = 3;
        c->lead = 2;
        c->turn = 4;
        c->field_155c = 4;
        c->field_20 = 0;
        aoe_comm_dp_close_ror(c, dp);
        unlink_init_players(c);
        if (dp->chat_clear) dp->chat_clear(dp->context);
        dp->set_self_dpid(dp->context, 0);
        c->self = 0;
        c->turn = (uint32_t)c->lead + 2;
        c->field_1718 = 0;
        c->field_12a8 = 0;
        unlink_renew_objects(u);
        break;
    case 9:
        unlink_log(c, "UNLINK PlayerCreated");
        aoe_comm_dp_release_player_ror(c, dp);
        c->self = 0;
        dp->set_self_dpid(dp->context, 0);
        unlink_flush(u, 0); /* +0x14c4 without a NULL test (0x0042361a) */
        c->field_12a8 = 0;
        break;
    case 10:
        unlink_log(c, "UNLINK CommSettingsExcahanged");
        aoe_comm_clean_ror(c);
        aoe_comm_reset_ping_times_ror(c);
        c->field_12a8 = 0;
        break;
    case 11:
        unlink_log(c, "UNLINK PlayerSet");
        unlink_forget_self(c, dp);
        break;
    case 12:
        unlink_log(c, "UNLINK GameOptionsExchanged");
        aoe_comm_free_data_ror(c);
        c->dirty = 0;
        c->field_12a8 = 0;
        break;
    case 13:
        c->state = 3;
        unlink_log(c, "UNLINK GameIsLoading");
        c->field_12a8 = 0;
        break;
    case 14:
        if (dp->chat_clear) dp->chat_clear(dp->context);
        unlink_log(c, "UNLINK GameIsRunning");
        c->field_12d0 = 0;
        unlink_flush(u, 0); /* +0x14c4 without a NULL test (0x004236fe) */
        c->field_1718 = 0;
        c->field_12a8 = 0;
        break;
    case 15:
        unlink_log(c, "UNLINK GameIsPaused");
        break;
    default: /* 8 and above 15 (0x0042372f) */
        unlink_log(c, "UNLINK Unknown");
        break;
    }
    snprintf(line, sizeof line, "Level is now...(%d)", (int)(c->status - 1));
    unlink_log(c, line);
    return aoe_comm_link_level_ror(c);
}
