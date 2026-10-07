#ifndef VITA_AOE_COMM_HANDLER_SESSION_H
#define VITA_AOE_COMM_HANDLER_SESSION_H

#include <stdint.h>

#include "net_misc.h"

/* Patched Rise of Rome 1.0a 0x0041d700 (__thiscall, ret 8): the
   DirectPlay call-failure report. The communications handler keeps a
   window poster (0x0041d6a0) at its +0x14e0, and each of the 34 call
   sites hands it a failed call's HRESULT and the call's name ("NewCmnd",
   "GTD ACK", ...); comm_handler.h binds it as the `report` / `error`
   ops.

   The object (AoeWindowPosterRor): +0x00 the game window (the post's
   HWND), +0x04 the last failed code (field_4).

   Code 0 does nothing. Any other code is logged ("+++COMM______DP CALL
   FAILED_(Code %u)_____ %s____"), stored at +0x04 and matched by
   equality (the original's signed compare tree and its two byte-indexed
   jump tables, 0x41ddc8 / 0x41de00 and 0x41de98 / 0x41ded4, only
   search): each known code logs its line (strings 0x554d38..0x55536c)
   and eleven also post WM_USER through 0x0041d6c0
   (PostMessageA(+0x00, 0x400, w, 0)):
     0x17ae (COMM_NO_LINK)     0x80070005, 0x8877000a, 0x88770140, and
                               0x88770118 (posted before its line);
     0x17ac                    0x80004005, and 0x80004002 (posted first);
     0x17ab                    0x88770028, 0x887700fa, and 0x8877014a
                               (posted first);
     0x17b9 (COMM_HOST_BUSY)   0x887700f0;
     0x17a8 (COMM_CANCEL_GAME) 0x88770136.
   0x88770050 logs two lines; 0x88770410 logs nothing more; an unknown
   code logs "!!! %s: Unhandled DP ERROR/exception in call.  Result=%ld
   (%d)". The log (global 0x00580db8, its printf 0x0042c330) is empty in
   this build; here each line arrives formatted at `log` (NULL drops
   them), 0x0041d6c0's "COMM: Send msg to window ..." line included. */
typedef struct {
    void *context;
    /* 0x0041d6c0's PostMessageA(window, 0x400, w, l): the callback
       aoe_poster_post_ror takes. */
    void (*post)(void *context, uint32_t window, uint32_t msg, uint32_t w, uint32_t l);
    /* 0x0042c330 with the line formatted; may be NULL. */
    void (*log)(void *context, const char *text);
} AoeCommDpErrorOpsRor;

void aoe_comm_dp_error_ror(AoeWindowPosterRor *p, const AoeCommDpErrorOpsRor *ops,
                           int32_t code, const char *what);

/* The WM_USER w that 0x0041d700 posts for `code` (0 when none). */
uint32_t aoe_comm_dp_error_post_ror(int32_t code);

#endif
