#include "comm_handler.h"
#include "le_bytes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPS (c->ops)
#define CTX (c->ops->context)
#define E_FAIL_HR ((int32_t)0x80004005)
#define DPERR_INVALIDPARAMS_HR ((int32_t)0x80070057)

static int in_range(const AoeCommRor *c, uint32_t slot) { return slot != 0 && slot <= c->count; }

static void post(AoeCommRor *c, uint32_t w, uint32_t l) { OPS->post(CTX, w, l); }

static void logf_(AoeCommRor *c, const char *text) {
    if (OPS->log) OPS->log(CTX, text);
}

/* ---- slots ---- */

int aoe_comm_playing_ror(const AoeCommRor *c, uint32_t slot) {
    return in_range(c, slot) && (c->player_state[slot] == 2 || c->player_state[slot] == 5);
}

int aoe_comm_computer_ror(const AoeCommRor *c, uint32_t slot) {
    return in_range(c, slot) && c->player_state[slot] == 4;
}

void aoe_comm_set_player_state_ror(AoeCommRor *c, uint32_t slot, int32_t state) {
    if (!in_range(c, slot)) return;
    c->player_state[slot] = state;
    OPS->refresh(CTX, slot, 1);
    post(c, 0x17a6, 0);
    if (c->is_host && c->state == 3) {
        c->dirty = 1;
        aoe_comm_share_data_ror(c, 0);
    }
}

int aoe_comm_active_range_ror(AoeCommRor *c) {
    uint32_t slot;
    c->high = 0;
    c->low = (uint16_t)(c->count + 1);
    for (slot = 1; slot <= c->count; ++slot) {
        if (!aoe_comm_playing_ror(c, slot) && !aoe_comm_computer_ror(c, slot)) continue;
        if (c->high <= slot) c->high = (uint16_t)slot;
        if (slot <= c->low) c->low = (uint16_t)slot;
    }
    return 1;
}

void aoe_comm_init_player_ror(AoeCommRor *c, uint32_t slot, int32_t dpid) {
    int i;
    char text[0x80];
    if (!in_range(c, slot)) return;
    for (i = 0; i < 8; ++i) c->stat[i][slot] = 0;
    c->ready[slot] = 0;
    c->last_turn[slot] = 0;
    c->player_state[slot] = 0;
    c->dpid[slot] = dpid;
    c->rx_serial[slot] = 0;
    memset(c->caps, 0, sizeof c->caps);
    if (OPS->flush_player_queue) OPS->flush_player_queue(CTX, slot);
    snprintf(text, sizeof text, "Player %d (%ld) is INITIALIZED", (int)slot, (long)dpid);
    logf_(c, text);
}

int aoe_comm_stats_ror(const AoeCommRor *c, uint32_t slot, int32_t out[8]) {
    int i;
    for (i = 0; i < 8; ++i) out[i] = c->stat[i][slot];
    return 1;
}

int32_t aoe_comm_stat0_ror(const AoeCommRor *c, uint32_t slot) {
    return in_range(c, slot) ? c->stat[0][slot] : 0;
}

int aoe_comm_all_stat0_ror(const AoeCommRor *c) {
    uint32_t slot;
    for (slot = c->low; slot <= c->high; ++slot)
        if (aoe_comm_playing_ror(c, slot) && c->stat[0][slot] == 0) return 0;
    return 1;
}

uint32_t aoe_comm_self_ror(const AoeCommRor *c) { return c->self; }

int32_t aoe_comm_is_host_ror(const AoeCommRor *c) { return c->multiplayer ? c->is_host : 1; }

/* ---- status ---- */

const char *aoe_comm_status_text_ror(AoeCommRor *c, uint32_t slot, char out[0x20]) {
    int32_t id;
    if (aoe_comm_computer_ror(c, slot))
        id = 0x4b1;
    else if (!aoe_comm_playing_ror(c, slot))
        id = 0x4b2;
    else if (c->last_turn[slot] < 4)
        id = 0x4b3;
    else if (c->last_turn[slot] < c->turn)
        id = 0x4b4;
    else
        id = 0x4b0;
    OPS->string(CTX, id, out, 0x20);
    return out;
}

const char *aoe_comm_slot_label_ror(AoeCommRor *c, uint32_t slot, const char *name) {
    char status[0x20];
    aoe_comm_status_text_ror(c, slot, status);
    snprintf(c->text, sizeof c->text, "[%d] %s - %s", (int)slot, name, status);
    return c->text;
}

const char *aoe_comm_ready_text_ror(AoeCommRor *c) {
    uint32_t slot;
    snprintf(c->status_text, sizeof c->status_text, "Ready=                        ");
    for (slot = 1; slot <= c->count && 6 + slot < sizeof c->status_text - 1; ++slot) {
        char ch;
        if (c->last_turn[slot] < c->turn) {
            switch (c->player_state[slot]) {
            case 0: ch = '-'; break;
            case 2: ch = 'W'; break;
            case 4: ch = 'C'; break;
            case 5: ch = 'X'; break;
            case 6: ch = 'V'; break;
            default: ch = '?'; break;
            }
        } else {
            ch = 'A';
        }
        c->status_text[6 + slot] = ch;
    }
    return c->status_text;
}

int32_t aoe_comm_link_level_ror(const AoeCommRor *c) {
    if (OPS->call_4239b0 && OPS->call_4239b0(CTX) && c->turn > 5) return 0xf;
    if (c->state == 5) return c->turn > 5 ? 0xe : 0xd;
    if (c->dirty) return 0xc;
    if (OPS->self_dpid(CTX) != 0) return 9;
    if (c->is_host) return 7;
    if (memcmp(c->guid_12fc, c->guid_1318, sizeof c->guid_12fc) != 0) return 6;
    if (memcmp(c->guid_12dc, c->guid_1318, sizeof c->guid_12dc) != 0) return 5;
    if (!OPS->dp_present(CTX)) {
        if (!c->multiplayer) return 2 - (c->self != 1);
        return 3;
    }
    return 4;
}

/* ---- pause and resume ---- */

void aoe_comm_paused_ror(AoeCommRor *c, uint32_t by) {
    c->state = 4;
    post(c, 0x17a2, by);
}

void aoe_comm_running_ror(AoeCommRor *c, uint32_t by) {
    c->state = 5;
    post(c, 0x17a3, by);
}

void aoe_comm_request_ror(AoeCommRor *c, int pause) {
    if (!c->multiplayer || !c->is_host) return;
    OPS->message(CTX, pause ? 0x50 : 0x55, NULL, 0);
}

void aoe_comm_pause_ror(AoeCommRor *c) {
    if (!c->multiplayer) {
        aoe_comm_paused_ror(c, c->self);
    } else if (c->is_host) {
        aoe_comm_request_ror(c, 1);
        aoe_comm_paused_ror(c, c->self);
    } else {
        OPS->message(CTX, 0x57, NULL, 0);
    }
}

void aoe_comm_resume_ror(AoeCommRor *c) {
    if (!c->multiplayer) {
        logf_(c, "Resume requested");
        aoe_comm_running_ror(c, c->self);
    } else if (c->is_host) {
        aoe_comm_request_ror(c, 0);
        aoe_comm_running_ror(c, c->self);
    } else {
        OPS->message(CTX, 0x2b, NULL, 0);
    }
}

void aoe_comm_set_seed_ror(AoeCommRor *c, int32_t seed) {
    if (!c->is_host) return;
    if (seed == -1) {
        OPS->srand(CTX, OPS->now_ms(CTX));
        seed = OPS->rand(CTX);
    }
    c->seed = seed;
    c->dirty = 1;
    aoe_comm_share_data_ror(c, 0);
}

/* ---- names ---- */

void aoe_comm_set_own_name_ror(AoeCommRor *c, const char *name) {
    snprintf(c->name, sizeof c->name, "%s", name);
    snprintf(c->info, sizeof c->info, "Optional Information");
}

void aoe_comm_set_name_ror(AoeCommRor *c, uint32_t slot, const char *name) {
    if (slot == c->self) {
        aoe_comm_set_own_name_ror(c, name);
        return;
    }
    snprintf(c->names[slot], sizeof c->names[slot], "%s", name);
}

int aoe_comm_format_label_ror(AoeCommRor *c, const char *name) {
    snprintf(c->label, sizeof c->label, "%s [%s]", name, c->name);
    return 1;
}

void aoe_comm_set_window_ror(AoeCommRor *c, uint32_t window) { c->window = window; }

void aoe_comm_set_guid_ror(AoeCommRor *c, const uint32_t guid[4]) {
    memcpy(c->guid_12dc, guid, sizeof c->guid_12dc);
}

/* ---- turns and commands ---- */

void aoe_comm_reset_turn_ror(AoeCommRor *c) {
    c->turn = 0;
    if (OPS->flush_queues) OPS->flush_queues(CTX);
}

int aoe_comm_send_command_ror(AoeCommRor *c, const void *data, uint32_t size, int32_t extra,
                              uint8_t (*seq)(void *context, uint32_t turn)) {
    if (c->multiplayer) {
        int32_t hr = OPS->message(CTX, 0x3e, data, size);
        OPS->report(CTX, hr, "NewCmnd");
        return 1;
    }
    {
        uint32_t turn = c->turn;
        uint8_t s = seq(CTX, turn);
        OPS->queue_local(CTX, (uint32_t)c->lead + turn, data, size, c->self, s, extra);
    }
    return 1;
}

uint8_t aoe_comm_command_seq_ror(AoeCommRor *c, int32_t turn) {
    if (turn != c->cmd_turn) {
        c->cmd_turn = turn;
        c->cmd_seq = 1;
        return 1;
    }
    ++c->cmd_seq;
    if (c->cmd_seq == 0xff) {
        logf_(c, "*** Too many commands in one comm turn");
        return 0;
    }
    if (c->state == 4 && c->cmd_seq > 100) c->cmd_seq = 100;
    return c->cmd_seq;
}

void aoe_comm_send_53_ror(AoeCommRor *c, int16_t a, int16_t b) {
    uint8_t buf[4];
    if (!c->is_host || !c->multiplayer) return;
    buf[0] = (uint8_t)a;
    buf[1] = (uint8_t)((uint16_t)a >> 8);
    buf[2] = (uint8_t)b;
    buf[3] = (uint8_t)((uint16_t)b >> 8);
    OPS->message(CTX, 0x53, buf, 4);
}

static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

void aoe_comm_send_turn_markers_ror(AoeCommRor *c, uint8_t a, uint8_t b) {
    uint8_t v[4], m[12];
    int k;
    if (!c->multiplayer) return;
    put32(v, (uint32_t)c->lead + c->turn);
    OPS->message(CTX, 0x51, v, 4);
    memset(m, 0, sizeof m);
    m[0] = 0x44;
    m[8] = a;
    m[9] = b;
    for (k = 1; k <= 3; ++k) {
        put32(m + 4, (uint32_t)c->lead + (uint32_t)k + c->turn);
        OPS->message(CTX, 0x44, m, 12);
    }
}

int aoe_comm_caught_up_ror(const AoeCommRor *c) {
    uint32_t slot;
    if (!c->multiplayer || c->field_12a8 || c->state == 4) return 0;
    for (slot = c->low; slot <= c->high; ++slot)
        if (aoe_comm_playing_ror(c, slot) && c->last_turn[slot] < c->turn) return 0;
    return 1;
}

int32_t aoe_comm_send_checksum_ror(AoeCommRor *c, int32_t a, int32_t b, int32_t d, int32_t e,
                                   int32_t f, int32_t g, int32_t h) {
    uint8_t m[0x24];
    int32_t hr;
    put32(m + 0x00, (uint32_t)a);
    put32(m + 0x04, c->turn);
    put32(m + 0x08, (uint32_t)b);
    put32(m + 0x0c, (uint32_t)d);
    put32(m + 0x10, (uint32_t)e);
    put32(m + 0x14, (uint32_t)f);
    put32(m + 0x18, (uint32_t)g);
    put32(m + 0x1c, (uint32_t)h);
    put32(m + 0x20, OPS->now_ms(CTX));
    hr = OPS->message(CTX, 0x4d, m, sizeof m);
    OPS->report(CTX, hr, "SendChecksum");
    return hr;
}

int aoe_comm_started_ror(const AoeCommRor *c) { return !c->multiplayer || c->turn > 6; }

int aoe_comm_started_or_tick_ror(AoeCommRor *c, void (*tick)(void *context, uint32_t v),
                                 void *context) {
    if (aoe_comm_started_ror(c)) return 1;
    tick(context, 0); /* 0x0041ebe0(0) */
    return 0;
}

/* ---- chat ---- */

void aoe_comm_chat_some_ror(AoeCommRor *c, const char *text,
                            int (*to_slot)(void *context, uint32_t slot)) {
    char mask[10];
    uint32_t slot;
    memset(mask, 'N', 8);
    mask[8] = mask[9] = 0;
    for (slot = 1; slot <= c->count && slot < 10; ++slot)
        if (to_slot(CTX, slot)) mask[slot] = 'Y';
    OPS->chat(CTX, (int32_t)c->self, mask, text);
}

void aoe_comm_chat_all_ror(AoeCommRor *c, const char *text) {
    char mask[10];
    memset(mask, 'Y', 8);
    mask[8] = mask[9] = 0;
    OPS->chat(CTX, (int32_t)c->self, mask, text);
}

void aoe_comm_chat_one_ror(AoeCommRor *c, int32_t from, int32_t slot, const char *text) {
    char mask[10], line[0x100];
    snprintf(line, sizeof line, "SendChatMsg FROM %d TO %d Text %s", (int)from, (int)slot, text);
    logf_(c, line);
    memset(mask, 'N', 8);
    mask[8] = mask[9] = 0;
    if (slot >= 0 && slot < 10) mask[slot] = 'Y';
    OPS->chat(CTX, from, mask, text);
}

/* ---- fast pings ---- */

int32_t aoe_comm_fast_send_ror(AoeCommRor *c, int32_t to, const void *data, uint32_t size) {
    int32_t self = OPS->self_dpid(CTX), hr;
    if (to == self) {
        logf_(c, "No send to self FastTX");
        return E_FAIL_HR;
    }
    if (self == 0) return E_FAIL_HR;
    hr = OPS->dp_send(CTX, to, data, size);
    if (hr == DPERR_INVALIDPARAMS_HR) {
        logf_(c, "FastTX to invalidparams ignored");
        return hr;
    }
    if (hr != 0) OPS->report(CTX, hr, "FastTX");
    return hr;
}

int aoe_comm_ping_ror(AoeCommRor *c, uint32_t slot) {
    uint32_t now = OPS->now_ms(CTX), s;
    uint8_t m[12];
    int32_t to;
    memset(m, 0, sizeof m);
    m[0] = 0x31;
    put32(m + 4, now);
    put32(m + 8, c->turn);
    if (slot == 0) {
        for (s = 1; s <= c->count; ++s) c->fast_time[s] = now;
        to = 0;
    } else {
        c->fast_time[slot] = now;
        to = c->dpid[slot];
    }
    return aoe_comm_fast_send_ror(c, to, m, sizeof m) == 0;
}

void aoe_comm_ping_quiet_ror(AoeCommRor *c, int force) {
    uint32_t now, slot;
    if (!c->multiplayer) return;
    now = OPS->now_ms(CTX);
    for (slot = 1; slot <= c->count; ++slot)
        if (aoe_comm_playing_ror(c, slot) && slot != c->self &&
            (now - c->fast_time[slot] > 8000 || force))
            aoe_comm_ping_ror(c, slot);
}

void aoe_comm_reset_ping_times_ror(AoeCommRor *c) {
    uint32_t now = OPS->now_ms(CTX), slot;
    for (slot = 1; slot <= c->count; ++slot) {
        c->time_1344[slot] = now;
        c->fast_time[slot] = now;
    }
}

/* ---- players leaving ---- */

void aoe_comm_set_stop_turn_ror(AoeCommRor *c, uint32_t slot, int32_t turn) {
    c->stop_turn[slot] = turn;
}

void aoe_comm_check_stop_turn_ror(AoeCommRor *c, int32_t turn) {
    int any = 0, others = 0;
    uint32_t slot;
    for (slot = 1; slot <= c->count; ++slot) {
        if (c->stop_turn[slot] != 0) any = 1;
        if (c->stop_turn[slot] == turn) {
            logf_(c, "CheckPlayerStopTurn: Drop player");
            aoe_comm_init_player_ror(c, slot, 0);
            aoe_comm_set_player_state_ror(c, slot, 3);
            post(c, 0x17b4, slot);
            c->stop_turn[slot] = 0;
        }
    }
    if (c->turn <= 6 || any) return;
    for (slot = 1; slot <= c->count; ++slot)
        if ((aoe_comm_playing_ror(c, slot) || aoe_comm_computer_ror(c, slot)) && slot != c->self)
            others = 1;
    if (!others) post(c, 0x17bc, 0);
}

int aoe_comm_drop_player_ror(AoeCommRor *c, uint32_t slot) {
    if (!in_range(c, slot) || !OPS->dp_present(CTX)) return 0;
    if (!aoe_comm_playing_ror(c, slot)) return 0;
    if (c->dpid[slot] != 0) {
        uint8_t m[12];
        put32(m, slot);
        put32(m + 4, (uint32_t)c->dpid[slot]);
        put32(m + 8, c->turn);
        logf_(c, "Sent to all - drop player");
        OPS->message(CTX, 0x4b, m, sizeof m);
        if (c->stop_turn[slot] == 0) OPS->drop(CTX, slot, c->dpid[slot]);
    }
    post(c, 0x17a4, 0);
    if (c->is_host && c->turn < 7) aoe_comm_share_data_ror(c, 1);
    return 1;
}

void aoe_comm_drop_early_ror(AoeCommRor *c) {
    uint32_t slot;
    if (!c->is_host || c->turn >= 7) return;
    for (slot = 1; slot <= c->count; ++slot) aoe_comm_drop_player_ror(c, slot);
}

void aoe_comm_find_self_ror(AoeCommRor *c) {
    int32_t me = OPS->self_dpid(CTX);
    uint32_t slot;
    for (slot = 1; slot <= c->count; ++slot) {
        if (c->dpid[slot] == me) {
            logf_(c, "Match -- ID set.");
            c->self = slot;
            post(c, 0x17b8, slot);
            return;
        }
    }
    logf_(c, "SET SELF failed");
}

int32_t aoe_comm_share_data_ror(AoeCommRor *c, int force) {
    uint32_t now, size, slot;
    uint8_t *buf;
    int32_t hr;
    if (!OPS->dp_present(CTX) || !c->is_host) return 0;
    now = OPS->now_ms(CTX);
    if (!force) {
        if (now - c->data_time < 2000) return 0;
    } else {
        c->dirty = 1;
    }
    if (!c->dirty) return 0;
    if (OPS->self_dpid(CTX) == 0) return E_FAIL_HR;
    size = c->data_size + 0x1d0;
    c->data_time = now;
    buf = malloc(size);
    if (!buf) return E_FAIL_HR;
    OPS->state_block(CTX, buf);
    if (c->data_size) memcpy(buf + 0x1d0, c->data, c->data_size);
    hr = OPS->dp_set_player_data(CTX, buf, size);
    OPS->report(CTX, hr, "Set Player Data");
    free(buf);
    c->dirty = 0;
    post(c, 0x17bb, 0);
    for (slot = 1; slot <= c->count; ++slot) c->ready[slot] = 0;
    return hr;
}

void aoe_comm_clean_ror(AoeCommRor *c) { c->dirty = 0; }

void aoe_comm_free_data_ror(AoeCommRor *c) {
    if (!c->data) return;
    c->data_size = 0;
    free(c->data);
    c->data = NULL;
}

/* ---- guaranteed delivery ---- */

void aoe_comm_tx_clear_ror(AoeCommTxRor *t) {
    t->time = 0;
    free(t->data);
    t->data = NULL;
    t->serial = 0;
    t->size = 0;
    memset(t->pending, 0, sizeof t->pending);
}

void aoe_comm_rx_clear_ror(AoeCommRxRor *r) {
    free(r->data);
    r->data = NULL;
    r->serial = 0;
    r->size = 0;
    r->from = 0;
    r->extra = 0;
}

void aoe_comm_gtd_reset_ror(AoeCommRor *c) {
    int i;
    for (i = 0; i < 500; ++i) aoe_comm_rx_clear_ror(&c->rx[i]);
    for (i = 0; i < 500; ++i) aoe_comm_tx_clear_ror(&c->tx[i]);
}

void aoe_comm_clear_serials_ror(AoeCommRor *c) {
    uint32_t slot;
    logf_(c, "Cleared all serial numbers.");
    for (slot = 0; slot <= c->count && slot < AOE_COMM_SLOTS; ++slot) {
        c->rx_serial[slot] = 0;
        c->ack_serial[slot] = 0;
        c->ack_time[slot] = 0;
        c->serial_1434[slot] = 0;
        c->serial_145c[slot] = 0;
    }
}

void aoe_comm_drop_rx_from_ror(AoeCommRor *c, uint32_t slot) {
    int i;
    for (i = 0; i <= 500; ++i)
        if (c->rx[i].from == c->dpid[slot]) aoe_comm_rx_clear_ror(&c->rx[i]);
}

void aoe_comm_clear_acks_ror(AoeCommRor *c, uint32_t slot) {
    int i;
    for (i = 0; i < AOE_COMM_TX_SLOTS; ++i) c->tx[i].pending[slot] = 0;
}

void aoe_comm_forget_slot_ror(AoeCommRor *c, uint32_t slot) {
    aoe_comm_clear_acks_ror(c, slot);
    aoe_comm_drop_rx_from_ror(c, slot);
}

int32_t aoe_comm_tx_count_ror(const AoeCommRor *c) {
    int32_t n = 0;
    int i;
    for (i = 0; i < AOE_COMM_TX_SLOTS; ++i)
        if (c->tx[i].serial != 0) ++n;
    return n;
}

int aoe_comm_tx_store_ror(AoeCommRor *c, int32_t serial, const void *data, uint32_t size,
                          const int32_t *pending) {
    int i;
    if (!c->multiplayer) return 0;
    if (!c->gtd) {
        logf_(c, "*** RGEGTD Store");
        return 0;
    }
    for (i = 0; i < 0x1f5; ++i) {
        AoeCommTxRor *t = &c->tx[i];
        if (t->serial != 0) continue;
        t->size = size;
        if (size) {
            t->data = malloc(size);
            if (t->data) memcpy(t->data, data, size);
        }
        t->serial = serial;
        memcpy(t->pending, pending, ((size_t)c->count + 1) * sizeof *pending);
        t->time = OPS->now_ms(CTX);
        return 1;
    }
    return 0;
}

int aoe_comm_rx_store_ror(AoeCommRor *c, int32_t serial, const void *data, uint32_t size,
                          int32_t from, int32_t extra) {
    int i;
    for (i = 0; i <= 0x1f5; ++i)
        if (c->rx[i].serial == serial && c->rx[i].from == from) return 1;
    for (i = 0; c->rx[i].size != 0 || size == 0; ++i) {
        if (i >= 500) {
            logf_(c, "*** DID NOT STORE");
            return 1;
        }
    }
    c->rx[i].size = size;
    c->rx[i].data = malloc(size);
    if (c->rx[i].data) memcpy(c->rx[i].data, data, size);
    c->rx[i].serial = serial;
    c->rx[i].from = from;
    c->rx[i].extra = extra;
    return 1;
}

static int send_small(AoeCommRor *c, uint8_t kind, int32_t serial, uint32_t slot) {
    uint8_t m[8];
    memset(m, 0, sizeof m);
    m[0] = kind;
    put32(m + 4, (uint32_t)serial);
    if (OPS->self_dpid(CTX) == 0) return -1;
    return OPS->dp_send(CTX, c->dpid[slot], m, sizeof m);
}

int aoe_comm_send_ack_ror(AoeCommRor *c, int32_t serial, uint32_t slot) {
    int32_t hr;
    if (!c->gtd) {
        logf_(c, " RGEGTD Ack");
        return 0;
    }
    hr = send_small(c, 0x41, serial, slot);
    if (hr == -1) return 0;
    c->bytes_sent += 8;
    if (hr == 0) return 1;
    OPS->report(CTX, hr, "GTD ACK");
    return 0;
}

int aoe_comm_request_resend_ror(AoeCommRor *c, int32_t serial, uint32_t slot) {
    uint32_t now;
    if (!c->gtd) return 0;
    now = OPS->now_ms(CTX);
    if (serial == c->ack_serial[slot] && now - c->ack_time[slot] <= 499) return 0;
    c->ack_serial[slot] = serial;
    c->ack_time[slot] = now;
    if (send_small(c, 0x58, serial, slot) == 0) {
        c->bytes_sent += 8;
        return 1;
    }
    return 0;
}

int aoe_comm_ack_received_ror(AoeCommRor *c, int32_t serial, uint32_t slot) {
    int i;
    if (!c->gtd) {
        logf_(c, " RGEGTD RxAck");
        return 0;
    }
    c->tx_pending = 0;
    for (i = 0; i <= 500; ++i)
        if (c->tx[i].serial == serial && serial != 0) c->tx[i].pending[slot] = 0;
    return 1;
}

int aoe_comm_tx_purge_ror(AoeCommRor *c) {
    int i;
    uint32_t slot;
    for (i = 0; i <= 500; ++i) {
        int32_t waiting = 0;
        if (c->tx[i].serial == 0) continue;
        for (slot = 1; slot <= c->count; ++slot)
            if (aoe_comm_playing_ror(c, slot) && c->tx[i].pending[slot] != 0) ++waiting;
        if (waiting == 0) aoe_comm_tx_clear_ror(&c->tx[i]);
    }
    for (i = 0; i < AOE_COMM_TX_SLOTS; ++i)
        if (c->tx[i].serial != 0) ++c->tx_pending;
    return 1;
}

int aoe_comm_rx_deliver_ror(AoeCommRor *c) {
    int i;
    uint32_t slot;
    if (!c->multiplayer) return 0;
    for (i = 0; i <= 500; ++i) {
        if (c->rx[i].size == 0) continue;
        for (slot = 1; slot <= c->count; ++slot) {
            int32_t last;
            AoeCommRxRor *e = &c->rx[i];
            if (!aoe_comm_playing_ror(c, slot) || slot == c->self) continue;
            last = c->rx_serial[slot];
            if ((uint32_t)e->serial < (uint32_t)last + 1u && e->from == c->dpid[slot])
                aoe_comm_rx_clear_ror(e);
            if ((uint32_t)e->serial == (uint32_t)last + 1u && e->from == c->dpid[slot]) {
                OPS->run(CTX, (int32_t)e->size, e->data, e->from, e->extra, 1);
                aoe_comm_rx_clear_ror(e);
            }
        }
    }
    return 0;
}

/* ---- DirectPlay sessions ---- */

static const uint32_t null_guid[4] = {0, 0, 0, 0};

int32_t aoe_comm_dp_close_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp) {
    int32_t hr = 0;
    uint32_t slot;
    if (OPS->dp_present(CTX)) hr = dp->close(dp->context);
    memcpy(c->guid_12fc, c->guid_1318, sizeof c->guid_12fc);
    OPS->report(CTX, hr, "Close Session");
    for (slot = 1; slot <= c->count; ++slot) c->last_turn[slot] = 0;
    return hr;
}

int32_t aoe_comm_dp_release_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp) {
    (void)null_guid;
    if (OPS->dp_present(CTX)) {
        dp->release(dp->context);
        logf_(c, "1 Release COMM links");
        memcpy(c->guid_12dc, c->guid_1318, sizeof c->guid_12dc);
    }
    return 0;
}

int32_t aoe_comm_dp_caps_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp) {
    int32_t hr;
    if (!OPS->dp_present(CTX)) return E_FAIL_HR;
    hr = dp->caps(dp->context);
    OPS->report(CTX, hr, "Get Caps");
    hr = dp->caps(dp->context);
    OPS->report(CTX, hr, "Get guaranteed Caps");
    c->gtd = 1;
    return hr;
}

int32_t aoe_comm_dp_session_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp, int32_t *players) {
    uint32_t guid[4];
    int32_t hr;
    if (!OPS->dp_present(CTX)) return (int32_t)0x80004005;
    hr = dp->session(dp->context, guid, players);
    OPS->report(CTX, hr, "Get Session Desc");
    if (hr == 0) {
        memcpy(c->guid_12fc, guid, sizeof guid);
        logf_(c, "RX Current Players");
    }
    return hr;
}

int32_t aoe_comm_dp_release_player_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp) {
    int32_t hr, me = OPS->self_dpid(CTX);
    if (!OPS->dp_present(CTX) || me == 0) return 0;
    hr = dp->destroy_player(dp->context, me);
    OPS->report(CTX, hr, "Release Player");
    if (hr == 0) {
        dp->set_self_dpid(dp->context, 0);
        c->self = 0;
    }
    return hr;
}

int32_t aoe_comm_dp_player_caps_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp) {
    int32_t caps[3] = {0, 0, 0}, hr, me = OPS->self_dpid(CTX);
    if (!OPS->dp_present(CTX) || me == 0) return E_FAIL_HR;
    hr = dp->player_caps(dp->context, me, caps);
    OPS->report(CTX, hr, "Get Player Caps");
    if (hr != 0) return hr;
    c->caps[0] = 1;
    c->gtd = 1;
    c->caps[3] = caps[0];
    c->caps[4] = caps[1];
    c->caps[5] = caps[2];
    return 0;
}

int aoe_comm_dp_add_self_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp) {
    int32_t id = 0, hr;
    if (!OPS->dp_present(CTX)) return 0;
    hr = dp->create_player(dp->context, c->name, c->info, &id);
    OPS->report(CTX, hr, "Add SELF PLAYER");
    if (hr != 0) {
        aoe_comm_dp_close_ror(c, dp);
        aoe_comm_dp_release_ror(c, dp);
        c->state = 1;
        return 0;
    }
    dp->set_self_dpid(dp->context, id);
    logf_(c, "Rcvd DPID");
    aoe_comm_dp_player_caps_ror(c, dp);
    return 1;
}

int32_t aoe_comm_dp_create_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp) {
    int32_t hr;
    if (OPS->dp_present(CTX)) {
        logf_(c, "Already have conversation.");
        return (int32_t)0x88770005;
    }
    hr = dp->create(dp->context, c->guid_12dc);
    OPS->report(CTX, hr, "DPCreate");
    if (hr != 0) memcpy(c->guid_12dc, c->guid_1318, sizeof c->guid_12dc);
    return hr;
}

int aoe_comm_dp_present_ror(const AoeCommRor *c) { return OPS->dp_present(CTX); }

void aoe_comm_refresh_all_ror(AoeCommRor *c) {
    uint32_t slot;
    for (slot = c->low; slot <= c->high; ++slot) OPS->refresh(CTX, slot, 0);
    post(c, 0x17a4, 0);
}

void aoe_comm_check_players_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp) {
    uint32_t slot;
    for (slot = 1; slot <= c->count; ++slot) {
        uint8_t m[12];
        if (!aoe_comm_playing_ror(c, slot)) continue;
        if (dp->player_data(dp->context, c->dpid[slot]) == 0) continue;
        put32(m, slot);
        put32(m + 4, (uint32_t)c->dpid[slot]);
        put32(m + 8, c->turn);
        OPS->message(CTX, 0x4b, m, sizeof m);
        if (c->stop_turn[slot] == 0) OPS->drop(CTX, slot, c->dpid[slot]);
    }
}

uint8_t *aoe_comm_next_command_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp) {
    if (!c->multiplayer || c->state == 3) return dp->take_first(dp->context);
    return dp->take_turn(dp->context, c->turn);
}

int32_t aoe_comm_unlink_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp, int32_t level) {
    int32_t start, now;
    c->status = aoe_comm_link_level_ror(c);
    if (level < 1) level = 1;
    start = now = c->status;
    while (now > level) {
        now = dp->unlink_step(dp->context);
        c->status = now;
        if (now == start) {
            snprintf(c->text, sizeof c->text, "Unlink from level %d Failed", (int)start);
            logf_(c, c->text);
            return aoe_comm_link_level_ror(c);
        }
    }
    logf_(c, "AFTER UNLINKING");
    return aoe_comm_link_level_ror(c);
}

/* ---- the lobby ---- */

void aoe_lobby_init_ror(AoeLobbyRor *l, uint32_t window) {
    l->window = window;
    l->has_poster = 1;
    l->field_c = 0;
    l->settings = NULL;
    l->iface = NULL;
    l->launched = 0;
}

void aoe_lobby_release_ror(AoeLobbyRor *l, const AoeLobbyOpsRor *ops) {
    if (l->settings) {
        ops->free_settings(ops->context, l->settings);
        l->settings = NULL;
    }
    if (l->iface) {
        ops->release_iface(ops->context, l->iface);
        l->iface = NULL;
    }
    l->launched = 0;
    l->field_c = 0;
}

void aoe_lobby_destroy_ror(AoeLobbyRor *l, const AoeLobbyOpsRor *ops) {
    l->has_poster = 0;
    aoe_lobby_release_ror(l, ops);
}

uint8_t aoe_lobby_launched_ror(const AoeLobbyRor *l) { return l->launched; }

uint32_t aoe_lobby_hosting_ror(const AoeLobbyRor *l, uint32_t flags) {
    return l->launched ? (flags >> 1) & 1u : 0;
}

int aoe_lobby_name_ror(const AoeLobbyRor *l, const char *name, const char **out) {
    if (!l->launched) return 0;
    *out = name;
    return 1;
}

int aoe_lobby_session_ror(const AoeLobbyRor *l, const uint8_t session[80], uint8_t out[80]) {
    if (!l->launched) return 0;
    memcpy(out, session, 80);
    return 1;
}

int32_t aoe_lobby_send_ror(AoeLobbyRor *l, const AoeLobbyOpsRor *ops, const uint32_t guid[4],
                           const void *data, uint32_t size, int32_t a, int32_t b, int32_t c,
                           int32_t d) {
    uint8_t *m;
    uint32_t total = size + 0x30;
    int32_t hr;
    int i;
    if (!l->launched) return 0;
    m = calloc(total, 1);
    if (!m) return (int32_t)0x8007000e;
    put32(m + 0x00, 5);
    put32(m + 0x04, 0);
    for (i = 0; i < 4; ++i) put32(m + 8 + 4 * i, guid[i]);
    put32(m + 0x18, (uint32_t)a);
    put32(m + 0x1c, (uint32_t)b);
    put32(m + 0x20, (uint32_t)c);
    put32(m + 0x24, (uint32_t)d);
    put32(m + 0x28, total);
    if (size) memcpy(m + 0x2c, data, size);
    hr = ops->send(ops->context, l->iface, m, total);
    free(m);
    return hr;
}

int aoe_lobby_enum_player_ror(int32_t dpid, const char *name, uint8_t flags) {
    (void)dpid;
    (void)name;
    (void)flags; /* 0x10 remote, 8 local: logged only */
    return 1;
}

int aoe_lobby_send_options_ror(AoeLobbyRor *l, const AoeLobbyOpsRor *ops, const uint32_t guid[4],
                               const void *data, uint32_t size, const int32_t values[4],
                               const AoeCommOpsRor *comm) {
    int32_t hr;
    if (!l->launched) {
        comm->log(comm->context, "NOT LOBBY LAUNCHED.  EXITING");
        return 0;
    }
    comm->log(comm->context, "Sending zone message.");
    hr = aoe_lobby_send_ror(l, ops, guid, data, size, values[0], values[1], values[2], values[3]);
    if (hr == 0) return 1;
    comm->report(comm->context, hr, "SendGameOptions");
    return 0;
}

void aoe_comm_gtd_rx_log_ror(const AoeCommRor *c, const AoeCommOpsRor *ops, int32_t serial,
                             int32_t from, int32_t exec_turn, uint8_t command, uint8_t sequence) {
    char line[160];
    if (!c->gtd) return;
    snprintf(line, sizeof line, "GTD: RX Ser#%d From P#%d T#%ld ToExecT#%d Cmd=%d '%c' Seq=%d",
             (int)serial, (int)from, (long)c->turn, (int)exec_turn, (int)command, (char)command,
             (int)sequence);
    ops->log(ops->context, line);
}

void aoe_comm_post_ror(const AoeCommOpsRor *ops, uint32_t w) { ops->post(ops->context, w, 0); }

int aoe_comm_set_ready_ror(AoeCommRor *c, const int32_t stats[8]) {
    char line[160];
    uint8_t msg[0x28];
    uint32_t self = c->self;
    int i;
    if (self >= AOE_COMM_SLOTS) return 0;
    snprintf(line, sizeof line,
             "SET-MY-READINESS: P#%d RDY=%d O1=%d O2=%d O3=%d O4=%d O5=%d O6=%d O7=%d ",
             (int)self, (int)c->stat[0][self], (int)c->stat[1][self], (int)c->stat[2][self],
             (int)c->stat[3][self], (int)c->stat[4][self], (int)c->stat[5][self],
             (int)c->stat[6][self], (int)c->stat[7][self]);
    logf_(c, line);
    for (i = 0; i < 8; ++i) c->stat[i][self] = stats[i];
    memset(msg, 0, sizeof msg);
    msg[0] = 0x52;
    msg[1] = (uint8_t)stats[0];
    for (i = 1; i < 8; ++i) put32(msg + 4 * i, (uint32_t)stats[i]);
    put32(msg + 0x20, (uint32_t)c->field_12a4);
    msg[0x24] = c->gtd;
    if (!c->is_host) (void)aoe_comm_fast_send_ror(c, 0, msg, sizeof msg);
    aoe_comm_post_ror(c->ops, 0x17a6);
    return 1;
}

int32_t aoe_comm_enable_new_players_ror(void *iface, int32_t enable,
                                        const AoeCommEnableOpsRor *ops) {
    void *dplay = NULL;
    int32_t hr, refs;
    char line[64];
    if (!iface) return (int32_t)0x80070057u;
    if (!ops || !ops->query_dplay) return (int32_t)0x80004005u;
    hr = ops->query_dplay(ops->context, iface, &dplay);
    if (hr < 0) return hr;
    hr = ops->enable_new_players ? ops->enable_new_players(ops->context, dplay, enable) : 0;
    refs = ops->release ? ops->release(ops->context, dplay) : 0;
    snprintf(line, sizeof line, "Released to links=%d", (int)refs);
    if (ops->log) ops->log(ops->context, line);
    return hr;
}

void aoe_comm_receive_ror(AoeCommRor *c) {
    const AoeCommOpsRor *ops = c->ops;
    char line[96];
    uint32_t start, t0, t1, count = 0;
    if (!ops->dp_present || !ops->dp_present(ops->context) || !ops->dp_receive) return;
    start = ops->now_ms(ops->context);
    if (ops->dp_message_count &&
        ops->dp_message_count(ops->context, ops->self_dpid(ops->context), &count) == 0 &&
        count > 10) {
        snprintf(line, sizeof(line), "!!! DPMsgCount = %u", (unsigned)count);
        logf_(c, line);
    }
    for (;;) {
        int32_t from = 0, to = 0, hr;
        uint32_t size = 0;
        t0 = ops->now_ms(ops->context);
        hr = ops->dp_receive(ops->context, 8, &from, &to, NULL, &size);
        t1 = ops->now_ms(ops->context);
        if (t1 - t0 > 50) {
            snprintf(line, sizeof(line), "!!! Receive Peek slow - %u msec", (unsigned)(t1 - t0));
            logf_(c, line);
        }
        if (hr == 0) {
            if (size >= 4000) {
                snprintf(line, sizeof(line), "True overflow %d requested.  Max = %d", (int)size,
                         4000);
                logf_(c, line);
                return;
            }
        } else {
            size = 4000;
        }
        t0 = ops->now_ms(ops->context);
        hr = ops->dp_receive(ops->context, 1, &from, &to, c->rx_buf, &size);
        t1 = ops->now_ms(ops->context);
        if (t1 - t0 > 50) {
            snprintf(line, sizeof(line), "!!! Receive slow - %u msec", (unsigned)(t1 - t0));
            logf_(c, line);
        }
        t1 = ops->now_ms(ops->context);
        if (t1 - start > 500) {
            snprintf(line, sizeof(line), "!!! In Receive Loop for %u ms", (unsigned)(t1 - start));
            logf_(c, line);
        }
        if ((uint32_t)hr == 0x887700beu || size == 0) return;
        if (hr != 0) {
            if (ops->report) ops->report(ops->context, hr, "Rcv MSG");
            return;
        }
        if (size > 4000) size = 4000;
        c->rx_bytes += size;
        c->rx_messages++;
        c->rx_buf[size] = 0;
        if (from == 0) {
            if (ops->system_message) ops->system_message(ops->context, size, c->rx_buf, to);
            return;
        }
        if (ops->run) ops->run(ops->context, (int32_t)size, c->rx_buf, from, to, 0);
    }
}

void aoe_comm_pump_ror(AoeCommRor *c) {
    const AoeCommOpsRor *ops = c->ops;
    int32_t self;
    if (!c->multiplayer) return;
    if (c->gtd) aoe_comm_rx_deliver_ror(c);
    self = ops->self_dpid ? ops->self_dpid(ops->context) : 0;
    if (self != 0) {
        uint32_t count = 0;
        int32_t hr = ops->dp_message_count ? ops->dp_message_count(ops->context, self, &count) : 0;
        if (hr != 0) {
            if (ops->report) ops->report(ops->context, hr, "MsgWait");
            goto store;
        }
        if (count == 0) goto store;
    }
    aoe_comm_receive_ror(c);
    if (!c->gtd) goto ping;
    aoe_comm_rx_deliver_ror(c);
store:
    if (c->gtd && ops->gtd_send_store) ops->gtd_send_store(ops->context);
ping:
    aoe_comm_ping_quiet_ror(c, 0);
}

void aoe_comm_pump_flagged_ror(AoeCommRor *c) {
    c->field_12a8 = 1;
    aoe_comm_pump_ror(c);
}

/* ---- 0x004245e0 / 0x004241f0 ---- */

static void became_host(AoeCommRor *c, int32_t game_seed) {
    c->self = 1;
    c->host_dpid = OPS->self_dpid ? OPS->self_dpid(CTX) : 0;
    c->player_state[c->self] = 2;
    c->field_20 = 1;
    c->state = 3;
    aoe_comm_set_seed_ror(c, game_seed);
    c->field_14e4 = 0x7d1;
}

int aoe_comm_create_session_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp, int32_t game_seed) {
    int32_t hr, players = 0;
    if (!c || !dp) return 0;
    c->multiplayer = 1;
    c->field_20 = 0;
    c->lead = 2;
    c->turn = 4;
    c->field_155c = 4;
    if (dp->chat_clear) dp->chat_clear(dp->context);
    hr = aoe_comm_dp_create_ror(c, dp);
    OPS->report(CTX, hr, "DPCreateConv");
    c->is_host = 1;
    hr = dp->open_create ? dp->open_create(dp->context, c->session_guid, c->count, c->label)
                         : (int32_t)0x80004005;
    OPS->report(CTX, hr, "DP_Open_Create");
    if (hr != 0) {
        if ((uint32_t)hr == 0x887700aau) {
            if (dp->post_error) dp->post_error(dp->context, 0x17ae);
            return 0;
        }
        c->state = 1;
        return 0;
    }
    aoe_comm_set_seed_ror(c, game_seed);
    aoe_comm_dp_add_self_ror(c, dp);
    aoe_comm_dp_caps_ror(c, dp);
    aoe_comm_dp_session_ror(c, dp, &players);
    c->self = 1;
    c->field_1714 = 1;
    c->host_dpid = OPS->self_dpid ? OPS->self_dpid(CTX) : 0;
    c->player_state[c->self] = 2;
    c->field_20 = 1;
    c->state = 3;
    c->field_14e4 = 0x7d1;
    aoe_comm_refresh_all_ror(c);
    return 1;
}

int aoe_comm_join_lobby_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp, int32_t game_seed) {
    int has = 0, named = 0;
    int32_t players = 0;
    char name[0x50];
    if (!c || !dp) return 0;
    if (dp->lobby_connect && dp->lobby_connect(dp->context, &has)) {
        if (OPS->dp_present(CTX)) {
            char line[64];
            int32_t left = dp->release ? dp->release(dp->context) : 0;
            snprintf(line, sizeof line, "Release count = %d", (int)left);
            OPS->log(CTX, line);
        }
        if (!has) {
            c->lobby_joined = 0;
            return 0;
        }
        name[0] = '\0';
        named = dp->lobby_player_name ? dp->lobby_player_name(dp->context, name, (int)sizeof name) : 0;
        aoe_comm_dp_caps_ror(c, dp);
        c->multiplayer = 1;
        aoe_comm_dp_session_ror(c, dp, &players);
        if (dp->lobby_name_pointer) aoe_comm_set_own_name_ror(c, dp->lobby_name_pointer(dp->context));
        c->state = 3;
        aoe_comm_dp_add_self_ror(c, dp);
        c->is_host = dp->lobby_hosting ? (dp->lobby_hosting(dp->context) != 0) : 0;
        if (c->is_host) became_host(c, game_seed);
        aoe_comm_refresh_all_ror(c);
    }
    c->lobby_joined = named ? 1 : 0;
    return named ? 1 : 0;
}

/* ---- 0x00424a90 ---- */
int aoe_comm_join_session_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp,
                              const AoeCommJoinOpsRor *join, const uint32_t guid[4]) {
    int32_t hr;
    char line[96];
    if (!c || !dp || !join) return 0;
    if (!OPS->dp_present(CTX)) {
        OPS->log(CTX, "JMPG: Create w/no valid glpIDC");
        return 0;
    }
    memcpy(c->guid_12fc, guid, sizeof c->guid_12fc);
    c->is_host = 0;
    snprintf(line, sizeof line, "Open session for JOIN. Session DPID = %lx", (unsigned long)guid[0]);
    OPS->log(CTX, line);
    hr = join->open_join(join->context, guid, c->session_guid);
    if ((uint32_t)hr == 0x8877015eu) {
        OPS->log(CTX, "DPERR_CONNECTING in JoinMultiplayerGame");
        OPS->report(CTX, hr, "Join MP Game");
        return 0;
    }
    if ((uint32_t)hr == 0x88770005u) {
        OPS->log(CTX, "DPERR_ALREADYINITIALIZED in JoinMultiplayerGame");
        OPS->report(CTX, hr, "Join MP Game");
        return 0;
    }
    if ((uint32_t)hr == 0x8877014au) {
        OPS->log(CTX, "DPERR_NONEWPLAYERS in JoinMultiplayerGame");
        OPS->report(CTX, hr, "Join MP Game");
        if (join->post_error) join->post_error(join->context, 0x17ab);
        return 0;
    }
    if (hr != 0) {
        OPS->log(CTX, "hr != DP_OK in JoinMultiplayerGame");
        OPS->report(CTX, hr, "Join MP Game");
        c->state = 1;
        return 0;
    }
    if (aoe_comm_dp_add_self_ror(c, dp)) {
        c->state = 3;
        return 1;
    }
    OPS->log(CTX, "AddSelfPlayer failed in JoinMultiplayerGame");
    return 0;
}

/* ---- 0x00425bb0 ---- */
int32_t aoe_comm_lobby_connect_ror(AoeCommRor *c, const AoeCommLobbyOpsRor *l,
                                   int *has_interface) {
    int32_t hr;
    uint32_t size = 0;
    if (!c || !l || !has_interface) return 0;
    *has_interface = 0;
    l->free_settings(l->context);
    hr = l->lobby_create(l->context);
    OPS->report(CTX, hr, "Lobby Create");
    if (hr < 0) return 0;
    if (!l->lobby_interface(l->context)) return 0;
    l->settings_size(l->context, &size);
    hr = l->settings_read(l->context);
    if (hr < 0) {
        l->set_launched(l->context, 0);
        return 0;
    }
    OPS->log(CTX, "LOBBY LAUNCHED GAME!");
    l->settings_patch(l->context, 0x44, 9);
    l->settings_write(l->context);
    OPS->log(CTX, "Pre connect");
    hr = l->connect(l->context);
    OPS->log(CTX, "Post Connect");
    if (hr < 0) {
        if ((uint32_t)hr == 0x887700f0u) return 0xff;
        OPS->log(CTX, "Failed to connect!");
        OPS->report(CTX, hr, "Connect");
        return 0xff;
    }
    OPS->log(CTX, "Pre QI");
    hr = l->query_interface(l->context, has_interface);
    OPS->report(CTX, hr, "Lobby QI");
    if (hr < 0) {
        OPS->log(CTX, "Queryinterface failed!");
        return 0xff;
    }
    OPS->log(CTX, "Calling enumeration of players.");
    l->enum_players(l->context);
    OPS->log(CTX, "Post ENUMP");
    l->set_launched(l->context, 1);
    return 1;
}

static int compact_live(const AoeCommRor *c, uint32_t slot) {
    return aoe_comm_playing_ror(c, slot) || aoe_comm_computer_ror(c, slot);
}

static void compact_locked(AoeCommRor *c, uint32_t slot) {
    char text[0x100];
    snprintf(text, sizeof text, "Position %d *LOCKED*.   Plr=%s", (int)slot,
             c->names ? c->names[slot] : "");
    if (c->ops && c->ops->log) c->ops->log(c->ops->context, text);
}

void aoe_comm_compact_players_ror(AoeCommRor *c) {
    uint32_t i, j, k;
    char text[0x40];
    for (i = 1; i <= c->count; ++i) {
        if (compact_live(c, i)) {
            compact_locked(c, i);
            continue;
        }
        for (j = i; j <= c->count && !compact_live(c, j); ++j) {}
        if (j > c->count) continue;
        if (c->names) memcpy(c->names[i], c->names[j], sizeof c->names[i]);
        if (c->infos) memcpy(c->infos[i], c->infos[j], sizeof c->infos[i]);
        c->dpid[i] = c->dpid[j];
        for (k = 0; k < 8; ++k) c->stat[k][i] = c->stat[k][j];
        c->player_state[i] = c->player_state[j];
        c->last_turn[i] = c->last_turn[j];
        c->time_1344[i] = c->time_1344[j];
        snprintf(text, sizeof text, "Moved %d->%d", (int)j, (int)i);
        if (c->ops && c->ops->log) c->ops->log(c->ops->context, text);
        compact_locked(c, i);
        aoe_comm_init_player_ror(c, j, 0);
        if (c->names) c->names[j][0] = '\0';
        if (c->infos) c->infos[j][0] = '\0';
    }
    aoe_comm_find_self_ror(c);
}

/* ---- 0x00422a70 ---- */
void aoe_comm_reset_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp, const AoeCommResetOpsRor *r) {
    uint32_t i;
    if (!c) return;
    aoe_comm_dp_release_player_ror(c, dp);
    c->data_time = OPS->now_ms(CTX);
    c->rx_bytes = c->field_12bc = c->bytes_sent = c->field_12c4 = 0;
    c->field_10f0 = c->field_10f4 = c->field_1748 = 0;
    c->field_1340 = c->field_1338 = c->rx_messages = 0;
    c->gtd = 0;
    c->field_1328 = c->field_132c = c->field_1330 = 0;
    c->lobby_joined = c->field_14e4 = c->field_10ec = c->field_10e8 = 0;
    if (r && r->log_elapsed) r->log_elapsed(r->context);
    c->field_1298 = c->field_12a0 = c->field_1288 = c->field_1518 = 0;
    c->label[0] = c->name[0] = c->info[0] = 0;
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
    OPS->log(CTX, "Initialize Player List");
    for (i = 1; i <= c->count && i < AOE_COMM_SLOTS; ++i) aoe_comm_init_player_ror(c, i, 0);
    c->dirty = 0;
    c->data_time = 0;
    aoe_comm_clear_serials_ror(c);
    aoe_comm_gtd_reset_ror(c);
    memset(c->field_13c0, 0, sizeof c->field_13c0);
    c->field_8 = 1;
    aoe_comm_reset_ping_times_ror(c);
    c->tx_pending = 0;
    if (r && (r->has_queue_a || r->has_queue_b) && OPS->flush_queues) OPS->flush_queues(CTX);
    c->field_12a8 = 0;
    memset(c->field_136c, 0, sizeof c->field_136c);
    memset(c->field_149c, 0, sizeof c->field_149c);
    memset(c->dropped, 0, sizeof c->dropped);
}

/* ---- 0x00420120 ---- */
int aoe_comm_handle_message_ror(AoeCommRor *c, const AoeCommMessageOpsRor *m, int32_t from,
                                uint32_t extra, uint8_t kind, const uint8_t *data, uint32_t size) {
    char line[128];
    if (!c || !c->multiplayer) return 0;
    switch (kind) {
    case 0x2b:
        if (c->is_host) {
            aoe_comm_request_ror(c, 0);
            OPS->log(CTX, "Non-host requested resume");
            aoe_comm_running_ror(c, (uint32_t)from);
        }
        return 1;
    case 0x3e:
        if (OPS->queue_local) OPS->queue_local(CTX, extra, data, size, (uint32_t)from, 0, 0);
        return 1;
    case 0x43:
        if ((c->self + 1 < size && data[c->self + 1] == 'Y') || c->state == 3) {
            if (m && m->chat_name && c->names) m->chat_name(m->context, c->names[data[0]],
                                                            (const char *)data + 0x11);
        }
        return 1;
    case 0x44: {
        uint32_t turn = aoe_le_u32(data + 4), t;
        if (m && m->speed_sample) m->speed_sample(m->context, from, data[9], data[8]);
        if ((c->state < 5 || c->turn < 6) && turn < c->turn) {
            snprintf(line, sizeof line, "*** REGRESSION WARNING: Current=%lu, received=%lu",
                     (unsigned long)c->turn, (unsigned long)turn);
            OPS->log(CTX, line);
            return 0;
        }
        t = turn - c->lead;
        if (t > c->last_turn[from]) c->last_turn[from] = t;
        return 1;
    }
    case 0x4b: {
        int32_t dpid = (int32_t)aoe_le_u32(data + 4);
        uint32_t slot;
        snprintf(line, sizeof line, "KILL Message recieved from %d to %ld", (int)from,
                 (long)(int32_t)aoe_le_u32(data));
        OPS->log(CTX, line);
        if (dpid == 0) return 1;
        for (slot = c->low; slot <= c->high && slot < AOE_COMM_SLOTS; ++slot) {
            if (c->dpid[slot] != dpid) continue;
            if (slot == 0) return 1;
            c->dropped[slot] = 1;
            if (c->stop_turn[slot] == 0 && OPS->drop) OPS->drop(CTX, slot, dpid);
            if (slot == c->self) post(c, 0x17aa, 0);
            else if (c->stop_turn[slot] == 0) post(c, 0x17b3, slot);
            return 1;
        }
        return 1;
    }
    case 0x4d: {
        int32_t st[8];
        int i;
        for (i = 0; i < 8; ++i) st[i] = (int32_t)aoe_le_u32(data + 4 * i);
        if (m && m->stats) m->stats(m->context, from, st);
        return 1;
    }
    case 0x4e:
        return 1;
    case 0x50:
        OPS->log(CTX, "P %d Requested pause");
        aoe_comm_paused_ror(c, (uint32_t)from);
        if (m && m->speed_zero) m->speed_zero(m->context);
        return 1;
    case 0x51:
        aoe_comm_set_stop_turn_ror(c, (uint32_t)from, (int32_t)aoe_le_u32(data));
        return 1;
    case 0x53:
        if (m && m->send_53) m->send_53(m->context, (int16_t)aoe_le_u16(data),
                                        (int16_t)aoe_le_u16(data + 2), extra);
        return 1;
    case 0x55:
        OPS->log(CTX, "Resume msg rcvd");
        aoe_comm_running_ror(c, (uint32_t)from);
        return 1;
    case 0x57:
        if (c->is_host) {
            aoe_comm_request_ror(c, 1);
            aoe_comm_paused_ror(c, (uint32_t)from);
        }
        return 1;
    default:
        snprintf(line, sizeof line, "Unknown message... %d (%c)", kind, kind >= 0x20 ? kind : '?');
        OPS->log(CTX, line);
        return 1;
    }
}

/* ---- 0x00422330 ---- */
int aoe_comm_resend_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp, const AoeCommResendOpsRor *r,
                        int32_t serial, int32_t slot) {
    uint32_t now, i;
    int32_t sent = 0, failed = 0, self_dpid;
    char line[128];
    (void)dp;
    if (!c || !c->gtd) return 0;
    if (slot < 0 || slot >= AOE_COMM_SLOTS || c->dpid[slot] == 0) {
        OPS->log(CTX, "Invalid player req!");
        return 0;
    }
    self_dpid = OPS->self_dpid ? OPS->self_dpid(CTX) : 0;
    if (self_dpid == 0) return 0;
    now = OPS->now_ms(CTX);
    if (serial == c->serial_1434[slot] && now - (uint32_t)c->serial_145c[slot] < 500) {
        snprintf(line, sizeof line, "Res reply too soon!! %u %u", (unsigned)serial,
                 (unsigned)(now - (uint32_t)c->serial_145c[slot]));
        OPS->log(CTX, line);
        return 0;
    }
    c->serial_1434[slot] = serial;
    c->serial_145c[slot] = (int32_t)now;
    for (i = 0; c->tx && i < AOE_COMM_TX_SLOTS; ++i) {
        AoeCommTxRor *t = &c->tx[i];
        int32_t hr;
        if (t->serial != serial) continue;
        t->pending[slot]--;
        if (self_dpid == 0) return 0;
        hr = OPS->dp_send(CTX, c->dpid[slot], t->data, t->size);
        if (hr == 0) {
            ++sent;
            c->bytes_sent += t->size;
        } else {
            OPS->report(CTX, hr, "Resend TX");
            ++failed;
        }
    }
    if (sent == 0 && failed == 0 && c->state == 3) {
        snprintf(line, sizeof line, "*** Did not have requested Ser#%d for player %d", (int)serial,
                 (int)slot);
        OPS->log(CTX, line);
        if (r && r->send_reject) r->send_reject(r->context, c->dpid[slot], c->field_14e4 - 1);
        if (r && r->drop_received) r->drop_received(r->context, slot);
    }
    aoe_comm_tx_purge_ror(c);
    return 1;
}

/* 0x00421fb0 */
int32_t aoe_comm_resend_store_ror(AoeCommRor *c, const AoeCommStoreOpsRor *o, uint32_t now) {
    uint32_t i, oldest = 0, oldest_age = 0, active = 0, need;
    AoeCommTxRor *e;
    if (!c->gtd) { o->log(o->context, "!RGEGTD SendStore"); return 0; }
    if (!(*o->flags & 1)) { *o->flags |= 1; *o->window_start = now; }
    if ((float)*o->window_bytes / (float)(int32_t)(now - *o->window_start) > 2.0f) return 0;
    *o->window_bytes = 0;
    *o->window_start = now;
    aoe_comm_tx_purge_ror(c);
    for (i = 0; i < AOE_COMM_TX_SLOTS; ++i) {
        if (!c->tx[i].serial) continue;
        ++active;
        if (now - c->tx[i].time > oldest_age) { oldest_age = now - c->tx[i].time; oldest = i; }
    }
    if (!active) return 0;
    if (c->turn < 7) need = 2000;
    else if (c->field_12a8 == 0) {
        need = o->base_c4 + (uint32_t)o->info_20c * 2;
        if (need < o->base_c0 + 500) need = o->base_c0 + 500;
    } else need = 250;
    if (oldest_age < need) return 0;
    e = &c->tx[oldest];
    e->time = now;
    for (i = 1; i <= c->count && i < AOE_COMM_SLOTS; ++i) {
        if (!e->pending[i] || !o->slot_playing(o->context, i) || !c->dpid[i] || i == c->self) continue;
        if (e->pending[i]) --e->pending[i];
        if (!o->self_dpid) return 0;
        {
            int32_t r = o->send(o->context, c->dpid[i], e->data, e->size);
            if (r == 0) { ++c->field_132c; c->bytes_sent += e->size; *o->window_bytes += e->size; }
            else o->error(o->context, r, "RESEND Transmit");
        }
        if (e->pending[i] == 0 && o->slot_playing(o->context, i) && c->dpid[i] && i != c->self) {
            char text[80];
            if (!c->is_host) {
                snprintf(text, sizeof text, "TX Missing Player report for player=%d ", (int)c->dpid[i]);
                o->log(o->context, text);
                o->send_missing(o->context);
            } else {
                snprintf(text, sizeof text, "HOST Rcvd MAX TX for player #%d", (int)i);
                o->log(o->context, text);
                c->field_149c[i] = 1;
                o->kick(o->context, i);
            }
        }
    }
    return (int32_t)c->field_132c;
}
