#ifndef VITA_AOE_COMM_HANDLER_DISPATCH_H
#define VITA_AOE_COMM_HANDLER_DISPATCH_H

#include "comm_handler.h"

#include <stdint.h>

/* Patched Rise of Rome 1.0a communications handler: one unlink step
   (0x00422ce0, __fastcall on the handler; the loop 0x004237c0 calls it
   until the link level 0x00423870 reaches the wanted one). The step is a
   jump table (0x00423778) on the current level (+0x12d4), each case
   undoing what that level set up:

     0, 1  nothing.
     2, 3  the reset 0x00422a70 inlined without its first call (our
           player destroyed, 0x004257a0): multiplayer (+0x10e4) off; the
           counters +0x12b8..+0x12c4, +0x10f0, +0x10f4, +0x1748, +0x1340,
           +0x1338, +0x1334, +0x1328..+0x1330, +0x1310, +0x14e4, +0x10ec,
           +0x10e8 cleared; guaranteed delivery (+0x1514) off; the
           elapsed-time log line (0x0042c100); +0x1298, +0x12a0, +0x1288,
           +0x1518 cleared; the data time (+0x1554) now; the label
           (+0x11fa), name (+0x10f8) and info (+0x1179) set to the
           one-space string 0x00554cf0; +0x0c, +0x10, +0x12d0 cleared;
           the slot range +0x1712 1 .. +0x1710 the count (+0x12d8);
           +0x1718 0; state (+0x1558) 3; hosting (+0x1c) and +0x20 off;
           lead (+0x1590) 2; the turn (+0x10c8) and +0x155c 4; dirty
           (+0x1550) off; +0x10dc 0x32; our slot (+0x10e0) 0; +0x128c,
           +0x1294 0; "Initialize Player List" and slots 1..count
           initialised (0x00424330 with ID 0); dirty off and data time
           0; the serials (0x004219d0) and both stores (0x00421a20)
           emptied; the stop turns of slots 1..9 (+0x13c0, 9 dwords)
           cleared; +0x08 1; the ping times (0x00423970); +0x151c 0;
           each present queue (+0x14c4, +0x14c8) flushed (0x00427f30);
           +0x12a8 0; the per-slot +0x136c, +0x149c and +0x174c cleared.
           Then our slot is 1 and: level 2 empties the name and info;
           level 3 makes slot 1 a playing human (+0x16d8 = 2).
     4     "UNLINK ReleaseComm": +0x1338, +0x1334, +0x1328..+0x1330,
           +0x1310, +0x12a8 cleared, guaranteed delivery ON (+0x1514 1),
           DirectPlay released (0x00425660).
     5     "UNLINK ServiceAvailable": DirectPlay released (0x00425660);
           the player list initialised; the chat log (+0x18) dropped
           (0x0041ca10); the label emptied; then as level 11.
     6     "UNLINK SessionAvailable": both stores emptied (0x00421a20),
           the host drops everyone early (0x004239c0), the chat log
           dropped, the session closed (0x004255e0), +0x14e4 0, the
           player list initialised, the range top the count, state 3,
           lead 2, turn and +0x155c 4, our slot 0, our DirectPlay ID
           (0x005817bc) 0, the serials cleared (0x004219d0), +0x1718 and
           +0x12a8 0, the speed controller (+0x14d4) and sync checker
           (+0x14d8) recreated, and the session GUID (+0x12fc) set to the
           null GUID (+0x1318).
     7     "UNLINK SerssionHosted" (sic): hosting and +0x1714 off, the
           range top the count, state 3, lead 2, turn and +0x155c 4,
           +0x20 0, the session closed, the player list initialised, the
           chat log dropped, our DirectPlay ID 0, our slot 0, the turn
           lead + 2, +0x1718 and +0x12a8 0, the two objects recreated.
     9     "UNLINK PlayerCreated": our player destroyed (0x004257a0), our
           slot and DirectPlay ID 0, the first queue (+0x14c4) flushed
           (not tested for NULL by the original), +0x12a8 0.
     10    "UNLINK CommSettingsExcahanged" (sic): dirty off (0x00420630),
           the ping times (0x00423970), +0x12a8 0.
     11    "UNLINK PlayerSet": our slot and DirectPlay ID 0, +0x12a8 0.
     12    "UNLINK GameOptionsExchanged": the extra data freed
           (0x004258a0), dirty off, +0x12a8 0.
     13    state 3, "UNLINK GameIsLoading", +0x12a8 0.
     14    the chat log dropped, "UNLINK GameIsRunning", +0x12d0 0, the
           first queue flushed, +0x1718 and +0x12a8 0.
     15    "UNLINK GameIsPaused" only.
     8 and anything else (unsigned above 15): "UNLINK Unknown" only.

   Logged before: "Unlinking current level (%d)"; after: "Level is
   now...(%d)" with the old level less one. Returns the link level
   (0x00423870, left in EAX). */

/* The handler state AoeCommRor does not carry, and the untranslated
   callees this step needs. */
typedef struct {
    void *context;
    /* 0x0042c100 on the log (0x00580db8): the elapsed-time line. */
    void (*log_elapsed)(void *context);
    /* The command queues +0x14c4 (which 0) and +0x14c8 (which 1),
       nonzero when present, and their flush 0x00427f30. */
    int has_queue_a, has_queue_b;
    void (*flush_queue)(void *context, int which);
    /* The speed controller (+0x14d4: operator new 0x240 and the
       constructor 0x00428990(handler); the destructor 0x00428b00 and
       operator delete 0x525fc2) and the sync checker (+0x14d8: 0x7ec
       bytes, 0x00429330 / 0x004293b0). A create gives NULL when the
       allocation fails. */
    void *(*speed_create)(void *context);
    void (*speed_destroy)(void *context, void *speed);
    void *(*sync_create)(void *context);
    void (*sync_destroy)(void *context, void *sync);
    void *speed;                 /* +0x14d4 */
    void *sync;                  /* +0x14d8 */
} AoeCommUnlinkRor;

/* 0x00422ce0. The chat log drop is dp->chat_clear (0x0041ca10 on +0x18),
   our DirectPlay ID's clear dp->set_self_dpid(0). */
int32_t aoe_comm_unlink_step_ror(AoeCommRor *c, const AoeCommDpOpsRor *dp, AoeCommUnlinkRor *u);

#endif
