#ifndef VITA_AOE_COMM_HANDLER_FRAME_H
#define VITA_AOE_COMM_HANDLER_FRAME_H

#include <stdint.h>

#include "comm_handler.h"

/* Patched Rise of Rome 1.0a: 0x0041f780, the communications handler's
   run of one received DirectPlay message (thiscall, ret 0x14: size,
   data, the sender's DirectPlay ID, extra, flag). It is what the receive
   poll (0x0041f3e0, flag 0, extra = the receiver's ID) and the ordered
   delivery of stored messages (0x00421890, flag 1) call as ops->run.

   The message's header (little-endian): byte 0 the command, byte 1 the
   sequence, +4 the turn to execute (or a ping's time / a serial for the
   control kinds), and with guaranteed delivery (+0x1514) a serial at +8
   and the payload at +0xc (without it the payload starts at +8).

   Handler fields used (AoeCommRor): +0x1c is_host, +0xfc5 text, +0x10c8
   turn, +0x10e0 self, +0x10e4 multiplayer, +0x12a4 field_12a4, +0x12a8
   field_12a8, +0x12c0 bytes_sent, +0x1330 field_1330 (out-of-order
   receives), +0x1344 time_1344[], +0x1514 gtd, +0x1528 rx_serial[],
   +0x1550 dirty, +0x1558 state, +0x1564 dpid[], +0x1590 lead, +0x1594..
   +0x16ac stat[8][], +0x16fc ready[], +0x1710 / +0x1712 high / low,
   +0x1720 last_turn[]. The speed controller (+0x14d4), the chat log
   (+0x18), the dispatcher by kind (0x00420120), the unlink (0x004237c0)
   and the resend reply (0x00422330, which takes DirectPlay ops) are ops
   below; the globals 0x007d3bf0 (the last 0x44 turn marker time) and
   0x005817cc / 0x005817d0 (the current payload and its size) are
   optional outputs.

   Kinds, in the original's order:
   'I' unlink to level 5, post 0x17a4 and 0x17aa; 1.
   A sender outside the active slot range (or ID 0): an 8-byte 'A' (the
     serial at +8) straight to that ID when we have one, +8 bytes sent; 0.
   Otherwise the slot's +0x1344 time is renewed, then:
   'K' in the lobby (state 3): "KILL Message ..." logged, the serial
     (+8) taken when next, acknowledged (0x00421d90), the killed slot's
     (+0xc) received serial cleared; our ID at +0x10 posts 0x17aa; 1.
   'C' first: the text bytes 3,2,1 discard it (0); a pregame chat outside
     the lobby is logged "DISCARD: ..." (0); else "CHAT: ..." and on.
   'A' an ack (0x00421e40) and the purge (0x00421ec0); 1.
   'R' "PLAYER READY MSG"; the host stores the eight stats (byte 1, then
     dwords +4..+0x1c), clears our ready flag, makes the slot human (2)
     unless it plays, compares the versions (+0x20 against +0x12a4:
     0x17b7 and the warning text when they differ), marks the data dirty,
     shares it (0x00420640), refreshes the slot (0x00424f30) and posts
     0x17a4; 1.
   '1' a ping: the turn marker below, then the reply '2' (the ping's
     time, our turn) unguaranteed (0x0041f6e0), "TX Ping" reported; 1.
   '3' a debug ping: the reply '4' ("TX Debug Ping"), "Received debug
     ping from %s (#%d)" logged and put in the chat log as DEBUG; 1.
   '?' a missing-player report (host only): the reported ID's slot;
     outside the lobby "Rcvd MAX TX report ..." and that slot dropped
     (0x004256c0); in the lobby both report lines, its stat 0 cleared,
     its ready flag set, the data dirty; 1.
   '2' a ping reply: the turn marker, then the slot's latency is the
     round trip (0x00428b80); 1.
   '4' a debug ping reply: "%s #%d Debug ping - elapsed=%d" logged and in
     the chat log; 1.
   'Y' in the lobby: the slot's received serial is +4 and its stored
     messages are dropped (0x00421ab0); 1.
   Else before state 4 only a pregame 'C' goes on (0 otherwise). With
   guaranteed delivery: 'X' is a resend request (0x00422330); the first
   contact sets the slot's serial to one before this one; an old serial is
   acknowledged again; a later one is stored (0x00421b80, +0x1330
   counted), acknowledged and the missing one requested (0x00422550); the
   next one is acknowledged (unless flag) and taken. Without it the GTD
   receive log (0x0041f2e0, silent then) is called. The payload is
   published and, unless +0x12a8, dispatched by kind (0x00420120); 1.

   The turn marker (inlined twice): multiplayer, outside the lobby and
   our last turn (+0x1720[self]) not yet the current turn: it becomes the
   turn; past turn 6 our own rate is recorded (0x00428b40 with the
   0x004290f0 average and the 0x00428df0 latency byte); 0x007d3bf0 is
   the time and the 12-byte 0x44 message (+4 turn + lead, bytes 8 / 9
   the two rate bytes) goes to everyone (0x00422680). */
typedef struct {
    void *context;
    int32_t (*unlink)(void *context, int32_t level);                    /* 0x004237c0 */
    uint32_t (*latency_byte)(void *context);                            /* 0x00428df0 */
    uint32_t (*frame_average)(void *context);                           /* 0x004290f0 */
    void (*player_rate)(void *context, uint32_t slot, uint8_t frame_ms,
                        uint8_t turn_ms_10);                            /* 0x00428b40 */
    void (*set_latency)(void *context, uint32_t slot, uint32_t ms);     /* 0x00428b80 */
    const char *(*slot_name)(void *context, uint32_t slot);             /* 0x004241a0 */
    void (*chat_line)(void *context, const char *who, const char *text); /* 0x0041c900 */
    void (*resend)(void *context, int32_t serial, uint32_t slot);       /* 0x00422330 */
    /* 0x00420120(size, slot, turn, command, sequence, payload, payload
       size, from, extra). */
    int (*handle)(void *context, int32_t size, uint32_t slot, int32_t exec_turn,
                  uint8_t command, uint8_t sequence, const uint8_t *payload,
                  int32_t payload_size, int32_t from, int32_t extra);
    uint32_t *marker_time;          /* 0x007d3bf0 (optional) */
    const uint8_t **payload;        /* 0x005817cc (optional) */
    int32_t *payload_size;          /* 0x005817d0 (optional) */
} AoeCommFrameOpsRor;

/* Port guard: every read of the message is bounded by `size` (bytes past
   it read as 0; the original reads whatever follows in its buffer), and
   slot indices from the message are bounded by AOE_COMM_SLOTS. */
int aoe_comm_run_frame_ror(AoeCommRor *c, const AoeCommFrameOpsRor *f, int32_t size,
                           const uint8_t *data, int32_t from, int32_t extra, int flag);

#endif
