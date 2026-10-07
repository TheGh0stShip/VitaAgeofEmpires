#ifndef VITA_AOE_GAME_FRAME_STEP_H
#define VITA_AOE_GAME_FRAME_STEP_H

#include <stdint.h>

#include "comm_handler.h"

/* Patched Rise of Rome 1.0a communications handler constructor
   (0x0041e1c0, 1680 bytes, __thiscall, ret 0x1c): the game frame's
   comm_new (0x00418780 allocates the 0x1774-byte object and passes the
   window, the settings' +0x878 player byte, the chat log and the
   settings' +0x8e8..+0x8f4 GUID). Semantic storage as in comm_handler.h;
   the comm's own ops (c->ops: log 0x0042c330, now_ms timeGetTime,
   flush_player_queue 0x00427fd0) must be set before the call.

   Offsets it writes, in order:
   - +0x12b0 / +0x12b4 (parts) take +0x12b8 / +0x12c0 and the peaks
     +0x12bc / +0x12c4 the larger value; +0x12b8 and +0x12c0 cleared.
   - +0x14c8, +0x14c4 (parts), +0x14e4 0; the GUID +0x1318 cleared;
     +0x12d4 1; +0x12ec..+0x12f8 the given GUID; +0x12fc and +0x12dc
     copies of +0x1318; +0x130c the window; +0x18 the chat log (parts);
     +0x12d8 the player count (the byte widened to a short).
   - Two log lines (0x0042c330 on 0x00580db8): "COMM:  RGE COMMUNICTIONS
     STARTED ---------------%s" and "COMM:  %s  Version... %s" with the
     source file "D:\ROR\Com_hand.cpp" and "Wed Oct 27 18:28:14 1999".
     The build time "22:19:57" (0x005553dc) is then copied to a 32-byte
     stack buffer with ':' made '0' and never used; not kept here.
   - +0x12a4 0x2e2887; +0x12c8 and +0x12cc (count + 1) * 0x80-byte name
     tables, every entry " " (0x00554cf0, a single space).
   - +0x14c4 the command queue (0x00427ce0(0x1f5) in 0x118 bytes);
     +0x1520 0x1f6 sent slots (0x38 bytes, zeroed by 0x41f240) and
     +0x1524 0x1f6 received slots (0x18 bytes, zeroed by 0x41f290);
     +0x14e0 the window poster (0x0041d6a0(window), 8 bytes); +0x14cc the
     lobby (0x00425a10(window), 0x1c); +0x14d4 the speed controller
     (0x00428990(comm), 0x240); +0x14d8 the sync checker
     (0x00429330(comm), 0x7ec). A failed allocation leaves 0.
   - The reset of 0x00422a70 inlined without its 0x004257a0 (no player
     to destroy yet): counters, labels " ", 0x0042c100, "Initialize
     Player List", slots 1..count (0x00424330), 0x004219d0, 0x00421a20,
     +0x08 1, 0x00423970, the queues flushed (0x00427f30), the per-slot
     arrays +0x136c / +0x149c / +0x174c zeroed.
   - Finally +0x171c 0 and +0x04 0. Returns the object. */

/* What the constructor sets outside AoeCommRor. */
typedef struct {
    uint32_t last_rx_bytes;     /* +0x12b0 */
    uint32_t last_tx_bytes;     /* +0x12b4 */
    void *chat;                 /* +0x18: the chat log */
    void *queue_a;              /* +0x14c4: the command queue */
    void *queue_b;              /* +0x14c8: left 0 */
    void *lobby;                /* +0x14cc */
    void *speed;                /* +0x14d4 */
    void *sync;                 /* +0x14d8 */
    void *poster;               /* +0x14e0 */
} AoeCommPartsRor;

typedef struct {
    void *context;
    void *(*queue_new)(void *context, int32_t capacity);     /* 0x00427ce0 */
    void (*queue_flush)(void *context, void *queue);         /* 0x00427f30 */
    void *(*poster_new)(void *context, uint32_t window);     /* 0x0041d6a0 */
    void *(*lobby_new)(void *context, uint32_t window);      /* 0x00425a10 */
    void *(*speed_new)(void *context, AoeCommRor *comm);     /* 0x00428990 */
    void *(*sync_new)(void *context, AoeCommRor *comm);      /* 0x00429330 */
    void (*log_elapsed)(void *context);                      /* 0x0042c100 */
} AoeCommConstructOpsRor;

/* 0x0041e1c0. The name tables come from calloc and the slot stores from
   calloc (free() releases all four). */
AoeCommRor *aoe_comm_construct_ror(AoeCommRor *c, AoeCommPartsRor *parts,
                                   const AoeCommConstructOpsRor *ops, uint32_t window,
                                   uint8_t players, void *chat, const uint32_t guid[4]);

#endif
