#include "comm_handler_session.h"

#include <stddef.h>
#include <stdio.h>

/* One known code of 0x0041d700: its log line (NULL: none), a second line,
   the WM_USER w posted through 0x0041d6c0 (0: none) and whether the post
   comes before the line. */
typedef struct {
    uint32_t code;
    const char *text;
    const char *text2;
    uint16_t post;
    uint8_t post_first;
} AoeDpErrorLineRor;

static const AoeDpErrorLineRor dp_error_lines[] = {
    /* COM facility HRESULTs. */
    {0x8000000au, "COM Error pending", NULL, 0, 0},
    {0x80004001u, "!!! Unsupported.", NULL, 0, 0},
    {0x80004002u, "!!! No Interface", NULL, 0x17ac, 1},
    {0x80004003u, "COM Invalid Pointer", NULL, 0, 0},
    {0x80004004u, "COM Aborted", NULL, 0, 0},
    {0x80004005u, "!!! GENERIC (Undefined) ERROR ", NULL, 0x17ac, 0},
    {0x80040110u, "!!! COMM: CLASS NO AGGREGATION.", NULL, 0, 0},
    {0x80070005u, "COM error Access Denied", NULL, 0x17ae, 0},
    {0x80070006u, "COM Invalid Handle", NULL, 0, 0},
    {0x8007000eu, "!!! Out Of Memory", NULL, 0, 0},
    {0x80070057u, "!!! Invalid Params", NULL, 0, 0},
    /* DirectPlay facility HRESULTs (0x88770000 + n). */
    {0x88770005u, "!!! Already Initialized", NULL, 0, 0},
    {0x8877000au, "!!! Access Denied - busy service or invalid use of host commands.", NULL,
     0x17ae, 0},
    {0x88770014u, "!!! Active Players - command invalid with active players.", NULL, 0, 0},
    {0x8877001eu, "!!! Buffer size failure (SMALL)", NULL, 0, 0},
    {0x88770028u, "!!! Can't Add Player", NULL, 0x17ab, 0},
    {0x88770032u, "!!! Can't Create Group", NULL, 0, 0},
    {0x8877003cu, "!!! Can't create player.", NULL, 0, 0},
    {0x88770046u, "!!! Can't create session.", NULL, 0, 0},
    {0x88770050u, "!!! Device caps are not available.", "!!! ERROR!", 0, 0},
    {0x8877005au, "!!! Exception Raised", NULL, 0, 0},
    {0x88770078u, "!!! Invalid Flags", NULL, 0, 0},
    {0x88770082u, "!!! Invalid Object", NULL, 0, 0},
    {0x88770096u, "!!! Invalid Player", NULL, 0, 0},
    {0x8877009bu, "!!! Invalid Group", NULL, 0, 0},
    {0x887700a0u, "!!! Not able to do multipayer!", NULL, 0, 0},
    {0x887700aau, "!!! No Connection", NULL, 0, 0},
    {0x887700beu, "!!! No messages", NULL, 0, 0},
    {0x887700c8u, "!!! No name server running!", NULL, 0, 0},
    {0x887700d2u, "!!! No players are connected!", NULL, 0, 0},
    {0x887700dcu, "!!! No sessions are available", NULL, 0, 0},
    {0x887700e6u, "!!! Send overflow!", NULL, 0, 0},
    {0x887700f0u, "!!! COMM TIMEOUT Rcvd from HOST connection.", NULL, 0x17b9, 0},
    {0x887700fau, "!!! Communications / Session is Unavailable", NULL, 0x17ab, 0},
    {0x8877010eu, "!!! Busy Rcvd", NULL, 0, 0},
    {0x88770118u, "!!! User Cancel", NULL, 0x17ae, 1},
    {0x88770122u, "!!! Unable to create server", NULL, 0, 0},
    {0x8877012cu, "!!! Player Lost connection.", NULL, 0, 0},
    {0x88770136u, "!!! Lost connection to session.", NULL, 0x17a8, 0},
    {0x88770140u, "!!! Unknown application specified", NULL, 0x17ae, 0},
    {0x8877014au, "!!! The session is in the process of connecting", NULL, 0x17ab, 1},
    {0x88770154u, "!!! The secure password is invalid.", NULL, 0, 0},
    {0x8877015eu, "!!! The session is in the process of connecting", NULL, 0, 0},
    {0x887703e8u, "!!! Buffer size failure (LARGE)", NULL, 0, 0},
    {0x887703f2u, "!!! Can't start the application.", NULL, 0, 0},
    {0x887703fcu, "!!! Application has not been started yet.", NULL, 0, 0},
    {0x88770410u, NULL, NULL, 0, 0},
    {0x8877041au, "!!! Unknown application specified", NULL, 0, 0},
    {0x8877042eu, "!!! Not A Lobby Session", NULL, 0, 0},
    {0x887707d0u, "!!! Security authentify failed.", NULL, 0, 0},
    {0x887707dau, "!!! Can't load security SSPI package.", NULL, 0, 0},
    {0x887707e4u, "!!! The encryption failed.", NULL, 0, 0},
    {0x887707eeu, "!!! The security sign failed.", NULL, 0, 0},
    {0x887707f8u, "!!! Can't load security package.", NULL, 0, 0},
    {0x8877080cu, "!!! Can't load CAPI", NULL, 0, 0},
    {0x88770820u, "!!! The secure logon failed.", NULL, 0, 0},
};

static const AoeDpErrorLineRor *dp_error_find(int32_t code) {
    size_t i;
    for (i = 0; i < sizeof dp_error_lines / sizeof dp_error_lines[0]; ++i)
        if (dp_error_lines[i].code == (uint32_t)code) return &dp_error_lines[i];
    return NULL;
}

static void dp_log(const AoeCommDpErrorOpsRor *ops, const char *text) {
    if (ops->log) ops->log(ops->context, text);
}

/* 0x0041d6c0: PostMessageA(+0x00, WM_USER, w, l), then its log line. */
static void dp_post(const AoeWindowPosterRor *p, const AoeCommDpErrorOpsRor *ops, uint32_t w,
                    uint32_t l) {
    char line[96];
    if (ops->post) aoe_poster_post_ror(p, ops->post, ops->context, w, l);
    if (!ops->log) return;
    snprintf(line, sizeof line, "COMM: Send msg to window WM_USER  Msg=%d  Param=%d ", (int)w,
             (int)l);
    ops->log(ops->context, line);
}

uint32_t aoe_comm_dp_error_post_ror(int32_t code) {
    const AoeDpErrorLineRor *e = code != 0 ? dp_error_find(code) : NULL;
    return e ? e->post : 0;
}

/* 0x0041d700: a failed DirectPlay call reported; the code kept at +0x04. */
void aoe_comm_dp_error_ror(AoeWindowPosterRor *p, const AoeCommDpErrorOpsRor *ops,
                           int32_t code, const char *what) {
    const AoeDpErrorLineRor *e;
    char line[320];
    if (code == 0) return;
    if (what == NULL) what = "";
    if (ops->log) {
        snprintf(line, sizeof line, "+++COMM______DP CALL FAILED_(Code %lu)_____ %s____",
                 (unsigned long)(uint32_t)code, what);
        ops->log(ops->context, line);
    }
    p->field_4 = code;
    e = dp_error_find(code);
    if (e == NULL) {
        if (ops->log) {
            snprintf(line, sizeof line,
                     "!!! %s: Unhandled DP ERROR/exception in call.  Result=%ld  (%d)", what,
                     (long)code, (int)code);
            ops->log(ops->context, line);
        }
        return;
    }
    if (e->post != 0 && e->post_first) dp_post(p, ops, e->post, 0);
    if (e->text) dp_log(ops, e->text);
    if (e->text2) dp_log(ops, e->text2);
    if (e->post != 0 && !e->post_first) dp_post(p, ops, e->post, 0);
}
