#include "comm_handler_frame.h"
#include "le_bytes.h"

#include <stdio.h>
#include <string.h>

#define OPS (c->ops)
#define CTX (c->ops->context)

/* Bounded reads of the received message (a port guard: see the header). */
static uint8_t at8(const uint8_t *d, uint32_t n, uint32_t off) { return off < n ? d[off] : 0; }

static uint32_t at32(const uint8_t *d, uint32_t n, uint32_t off) {
    return off <= n && n - off >= 4 ? aoe_le_u32(d + off) : 0;
}

/* The zero-terminated text at `off`, at most `cap - 1` bytes. */
static const char *text_at(const uint8_t *d, uint32_t n, uint32_t off, char *out, size_t cap) {
    size_t i = 0;
    while (off + i < n && i + 1 < cap && d[off + i] != 0) {
        out[i] = (char)d[off + i];
        ++i;
    }
    out[i] = 0;
    return out;
}

static void logf_(AoeCommRor *c, const char *text) {
    if (OPS->log) OPS->log(CTX, text);
}

/* The slot (low..high) holding a DirectPlay ID; 0 for ID 0 or none (the
   two inlined searches of 0x0041f780). */
static uint32_t slot_of(const AoeCommRor *c, int32_t dpid) {
    uint32_t slot;
    if (dpid == 0) return 0;
    for (slot = c->low; slot <= c->high && slot < AOE_COMM_SLOTS; ++slot)
        if (c->dpid[slot] == dpid) return slot;
    return 0;
}

/* The turn marker inlined in 0x0041f780's '1' and '2' cases (0x0041fb0d..
   0x0041fbbf, 0x0041fda9..0x0041fe58). Bytes 10..11 of the message are
   stack garbage in the original (8..9 too up to turn 6); zero here. */
static void turn_marker(AoeCommRor *c, const AoeCommFrameOpsRor *f) {
    uint32_t turn = c->turn, self = c->self;
    uint8_t m[12];
    if (!c->multiplayer || c->state == 3 || self >= AOE_COMM_SLOTS ||
        c->last_turn[self] == turn)
        return;
    c->last_turn[self] = turn;
    memset(m, 0, sizeof m);
    if (c->turn > 6) {
        m[8] = (uint8_t)(f->latency_byte ? f->latency_byte(f->context) : 0);
        m[9] = (uint8_t)(f->frame_average ? f->frame_average(f->context) : 0);
        if (f->player_rate) f->player_rate(f->context, self, m[9], m[8]);
    }
    if (f->marker_time) *f->marker_time = OPS->now_ms(CTX);
    m[0] = 0x44;
    aoe_le_put_u32(m + 4, (uint32_t)c->lead + turn);
    OPS->message(CTX, 0x44, m, sizeof m);
}

/* The 12-byte ping replies ('2' / '4'): the ping's time and our turn,
   unguaranteed to the slot (0x0041f6e0) and reported (0x0041d700). Bytes
   1..3 are stack garbage in the original; zero here. */
static void ping_reply(AoeCommRor *c, uint8_t kind, uint32_t ping_time, uint32_t slot,
                       const char *what) {
    uint8_t m[12];
    int32_t hr;
    memset(m, 0, sizeof m);
    m[0] = kind;
    aoe_le_put_u32(m + 4, ping_time);
    aoe_le_put_u32(m + 8, c->turn);
    hr = aoe_comm_fast_send_ror(c, c->dpid[slot], m, sizeof m);
    if (OPS->report) OPS->report(CTX, hr, what);
}

static void debug_line(AoeCommRor *c, const AoeCommFrameOpsRor *f, const char *text) {
    logf_(c, text);
    if (f->chat_line) f->chat_line(f->context, "DEBUG", text);
}

static const char *name_of(const AoeCommFrameOpsRor *f, uint32_t slot) {
    const char *name = f->slot_name ? f->slot_name(f->context, slot) : NULL;
    return name ? name : "";
}

/* 0x0041f780 */
int aoe_comm_run_frame_ror(AoeCommRor *c, const AoeCommFrameOpsRor *f, int32_t size,
                           const uint8_t *data, int32_t from, int32_t extra, int flag) {
    uint32_t n = size > 0 ? (uint32_t)size : 0;
    uint32_t slot, base, serial, expected, payload_size;
    uint8_t cmd = at8(data, n, 0);
    const uint8_t *payload;
    char line[0x180], text[0x100]; /* the line holds a whole text plus its prefix */

    slot = slot_of(c, from);
    if (cmd == 'I') {
        if (f->unlink) f->unlink(f->context, 5);
        aoe_comm_post_ror(OPS, 0x17a4);
        aoe_comm_post_ror(OPS, 0x17aa);
        return 1;
    }
    if (slot == 0) {
        /* An ack to an unknown sender, straight through DirectPlay +0x68. */
        uint8_t ack[8];
        if (OPS->self_dpid(CTX) == 0) return 0;
        memset(ack, 0, sizeof ack);
        ack[0] = 0x41;
        aoe_le_put_u32(ack + 4, at32(data, n, 8));
        (void)OPS->dp_send(CTX, from, ack, sizeof ack);
        c->bytes_sent += 8;
        return 0;
    }
    c->time_1344[slot] = OPS->now_ms(CTX);

    if (cmd == 'K' && c->state == 3) {
        uint32_t killed = at32(data, n, 0xc);
        int32_t dpid = (int32_t)at32(data, n, 0x10);
        snprintf(line, sizeof line, "KILL Message recieved from %d to kill dcoID #%d ",
                 (int)from, (int)dpid);
        logf_(c, line);
        serial = at32(data, n, 8);
        if (serial == (uint32_t)c->rx_serial[slot] + 1u) c->rx_serial[slot] = (int32_t)serial;
        aoe_comm_send_ack_ror(c, (int32_t)serial, slot);
        if (killed < AOE_COMM_SLOTS) c->rx_serial[killed] = 0;
        if (dpid == OPS->self_dpid(CTX)) aoe_comm_post_ror(OPS, 0x17aa);
        return 1;
    }
    if (cmd == 'C') {
        uint8_t pregame;
        base = c->gtd ? 0xc : 8;
        if (at8(data, n, base + 0x11) == 3 && at8(data, n, base + 0x12) == 2 &&
            at8(data, n, base + 0x13) == 1)
            return 0;
        pregame = at8(data, n, base + 0x10);
        text_at(data, n, base + 0x11, text, sizeof text);
        if (pregame != 0 && c->state != 3) {
            snprintf(line, sizeof line, "DISCARD: ChtPregame=%d PgmState=%d  Msg=%s", (int)pregame,
                     (int)c->state, text);
            logf_(c, line);
            return 0;
        }
        snprintf(line, sizeof line, "CHAT: ChtPregame=%d PgmState=%d  Msg=%s", (int)pregame,
                 (int)c->state, text);
        logf_(c, line);
    }

    switch (cmd) {
    case 'A':
        aoe_comm_ack_received_ror(c, (int32_t)at32(data, n, 4), slot);
        aoe_comm_tx_purge_ror(c);
        return 1;
    case 'R': {
        int i;
        logf_(c, "PLAYER READY MSG");
        if (!c->is_host) return 1;
        c->stat[0][slot] = at8(data, n, 1);
        snprintf(line, sizeof line, "Rcvd Ready=%d from player %d", (int)at8(data, n, 1),
                 (int)slot);
        logf_(c, line);
        if (c->self < AOE_COMM_SLOTS) c->ready[c->self] = 0;
        for (i = 1; i < 8; ++i) c->stat[i][slot] = (int32_t)at32(data, n, 4u * (uint32_t)i);
        if (!aoe_comm_playing_ror(c, slot)) aoe_comm_set_player_state_ror(c, slot, 2);
        if (c->field_12a4 == (int32_t)at32(data, n, 0x20)) {
            snprintf(line, sizeof line,
                     "Communications versions for HOST and player %d match (%d)", (int)slot,
                     (int)c->field_12a4);
            logf_(c, line);
        } else {
            OPS->post(CTX, 0x17b7, slot);
            snprintf(c->text, sizeof c->text,
                     "Player %d has a different Communications Version than you.\n This game "
                     "will fail synchronization or lock up if started.\n Please notify that "
                     "player also. ",
                     (int)slot);
            logf_(c, c->text);
        }
        c->dirty = 1;
        aoe_comm_share_data_ror(c, 0);
        OPS->refresh(CTX, slot, 0);
        aoe_comm_post_ror(OPS, 0x17a4);
        return 1;
    }
    case '1': {
        uint32_t ping_time = at32(data, n, 4);
        turn_marker(c, f);
        ping_reply(c, 0x32, ping_time, slot, "TX Ping");
        return 1;
    }
    case '3':
        ping_reply(c, 0x34, at32(data, n, 4), slot, "TX Debug Ping");
        snprintf(line, sizeof line, "Received debug ping from %s (#%d)", name_of(f, slot),
                 (int)slot);
        debug_line(c, f, line);
        return 1;
    case '?': {
        int32_t dco;
        uint32_t reported;
        if (!c->is_host) return 1;
        dco = (int32_t)at32(data, n, 4);
        reported = slot_of(c, dco);
        if (c->state != 3) {
            snprintf(line, sizeof line,
                     "Rcvd MAX TX report for player #%d, ID=%d from player %d", (int)reported,
                     (int)dco, (int)slot);
            logf_(c, line);
            aoe_comm_drop_player_ror(c, reported);
            return 1;
        }
        snprintf(c->text, sizeof c->text, " P#%d reported P#%d (dco=%d) was invalid ", (int)slot,
                 (int)reported, (int)dco);
        logf_(c, c->text);
        snprintf(line, sizeof line,
                 "Rcvd Missing Player report for player #%d, ID=%d from player %d",
                 (int)reported, (int)dco, (int)slot);
        logf_(c, line);
        c->stat[0][reported] = 0;
        c->ready[reported] = 1;
        c->dirty = 1;
        return 1;
    }
    case '2': {
        uint32_t rtt = OPS->now_ms(CTX) - at32(data, n, 4);
        turn_marker(c, f);
        if (f->set_latency) f->set_latency(f->context, slot, rtt);
        return 1;
    }
    case '4': {
        uint32_t elapsed = OPS->now_ms(CTX) - at32(data, n, 4);
        snprintf(line, sizeof line, "%s #%d Debug ping - elapsed=%d", name_of(f, slot),
                 (int)slot, (int)elapsed);
        debug_line(c, f, line);
        return 1;
    }
    case 'Y':
        if (c->state == 3) {
            c->rx_serial[slot] = (int32_t)at32(data, n, 4);
            aoe_comm_drop_rx_from_ror(c, slot);
        }
        return 1;
    default:
        break;
    }

    if (c->state < 4) {
        /* Before the game only a pregame chat goes on. */
        if (cmd != 'C') return 0;
        if (at8(data, n, (c->gtd ? 0xc : 8) + 0x10) == 0) return 0;
    }
    serial = at32(data, n, 8);
    if (!c->gtd) {
        aoe_comm_gtd_rx_log_ror(c, OPS, (int32_t)serial, (int32_t)slot,
                                (int32_t)at32(data, n, 4), cmd, at8(data, n, 1));
        base = 8;
    } else {
        if (cmd == 'X') {
            if (f->resend) f->resend(f->context, (int32_t)at32(data, n, 4), slot);
            return 1;
        }
        if (c->rx_serial[slot] == 0) {
            snprintf(line, sizeof line, "+++First contact with P#%d Next Serial Init to %d",
                     (int)slot, (int)serial);
            logf_(c, line);
            c->rx_serial[slot] = (int32_t)(serial - 1u);
        }
        expected = (uint32_t)c->rx_serial[slot] + 1u;
        if (serial < expected) {
            aoe_comm_send_ack_ror(c, (int32_t)serial, slot);
            return 1;
        }
        if (serial > expected) {
            c->field_1330++;
            aoe_comm_rx_store_ror(c, (int32_t)serial, data, n, from, extra);
            aoe_comm_send_ack_ror(c, (int32_t)serial, slot);
            aoe_comm_request_resend_ror(c, (int32_t)expected, slot);
            return 1;
        }
        if (flag == 0) aoe_comm_send_ack_ror(c, (int32_t)serial, slot);
        c->rx_serial[slot] = (int32_t)serial;
        base = 0xc;
    }
    payload = n >= base ? data + base : data + n;
    payload_size = (uint32_t)size - base;
    if (f->payload) *f->payload = payload;
    if (f->payload_size) *f->payload_size = (int32_t)payload_size;
    if (c->field_12a8 == 0 && f->handle)
        f->handle(f->context, size, slot, (int32_t)at32(data, n, 4), cmd, at8(data, n, 1), payload,
                  (int32_t)payload_size, from, extra);
    return 1;
}
