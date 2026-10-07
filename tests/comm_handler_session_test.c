#include "comm_handler_session.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Patched RoR 1.0a 0x0041d700, the DirectPlay call-failure report. The
   evidence is the executable's code: the store `mov [edi+0x4],esi` at
   0x41d730, the posts through 0x0041d6c0 with ecx = edi (this) at
   0x41d78d (0x17ac before "!!! No Interface"), 0x41d82b (0x17ac after
   the GENERIC line), 0x41d86d (0x17ae), 0x41d982 (0x17ab), 0x41dbbe
   (0x17a8), 0x41dbe6 (0x17b9) and 0x41dc14 (0x17ae before "!!! User
   Cancel"), the switch tables 0x41ddc8 / 0x41de00 and 0x41de98 /
   0x41ded4, the bare return for 0x88770410 at 0x41dc81, and the default
   log at 0x41dd8f (push esi, esi, ebx: the name, then the code twice). */

#define MAX_EVENTS 16

static char events[MAX_EVENTS][160];
static int event_count;
static uint32_t post_window, post_msg, post_w, post_l;
static int posts;

static void add_event(const char *text) {
    assert(event_count < MAX_EVENTS);
    snprintf(events[event_count++], sizeof events[0], "%s", text);
}

static void cap_post(void *context, uint32_t window, uint32_t msg, uint32_t w, uint32_t l) {
    char line[64];
    assert(context == (void *)&posts);
    post_window = window;
    post_msg = msg;
    post_w = w;
    post_l = l;
    ++posts;
    snprintf(line, sizeof line, "POST %lx", (unsigned long)w);
    add_event(line);
}

static void cap_log(void *context, const char *text) {
    assert(context == (void *)&posts);
    add_event(text);
}

static void reset(void) {
    event_count = 0;
    posts = 0;
    post_window = post_msg = post_w = post_l = 0;
}

static AoeCommDpErrorOpsRor ops(void) {
    AoeCommDpErrorOpsRor o;
    memset(&o, 0, sizeof o);
    o.context = &posts;
    o.post = cap_post;
    o.log = cap_log;
    return o;
}

static void zero_code_does_nothing(void) {
    AoeCommDpErrorOpsRor o = ops();
    AoeWindowPosterRor p;
    aoe_poster_init_ror(&p, 0x1234);
    p.field_4 = 77;
    reset();
    /* 0x41d707: test esi,esi; je 0x41ddc0 (the epilogue). */
    aoe_comm_dp_error_ror(&p, &o, 0, "NewCmnd");
    assert(event_count == 0 && posts == 0 && p.field_4 == 77);
    assert(aoe_comm_dp_error_post_ror(0) == 0);
}

static void timeout_posts_host_busy(void) {
    AoeCommDpErrorOpsRor o = ops();
    AoeWindowPosterRor p;
    aoe_poster_init_ror(&p, 0x1234);
    reset();
    /* 0x41dbd2: "!!! COMM TIMEOUT Rcvd from HOST connection." (0x554f3c),
       then 0x0041d6c0(0x17b9, 0). */
    aoe_comm_dp_error_ror(&p, &o, (int32_t)0x887700f0u, "GTD ACK");
    assert(p.field_4 == (int32_t)0x887700f0u && p.window == 0x1234);
    assert(posts == 1 && post_window == 0x1234 && post_msg == 0x400 && post_w == 0x17b9 &&
           post_l == 0);
    assert(event_count == 4);
    assert(strcmp(events[0], "+++COMM______DP CALL FAILED_(Code 2289500400)_____ GTD ACK____") ==
           0);
    assert(strcmp(events[1], "!!! COMM TIMEOUT Rcvd from HOST connection.") == 0);
    assert(strcmp(events[2], "POST 17b9") == 0);
    assert(strcmp(events[3], "COMM: Send msg to window WM_USER  Msg=6073  Param=0 ") == 0);
}

static void post_order(void) {
    AoeCommDpErrorOpsRor o = ops();
    AoeWindowPosterRor p;
    aoe_poster_init_ror(&p, 9);
    /* 0x41dc14: the post of 0x17ae precedes "!!! User Cancel". */
    reset();
    aoe_comm_dp_error_ror(&p, &o, (int32_t)0x88770118u, "x");
    assert(posts == 1 && post_w == 0x17ae && event_count == 4);
    assert(strcmp(events[1], "POST 17ae") == 0 && strcmp(events[3], "!!! User Cancel") == 0);
    /* 0x41d78d: 0x17ac before "!!! No Interface" (0x80004002). */
    reset();
    aoe_comm_dp_error_ror(&p, &o, (int32_t)0x80004002u, "x");
    assert(posts == 1 && post_w == 0x17ac && strcmp(events[1], "POST 17ac") == 0 &&
           strcmp(events[3], "!!! No Interface") == 0);
    /* 0x41d818: the GENERIC line (E_FAIL), then 0x17ac. */
    reset();
    aoe_comm_dp_error_ror(&p, &o, (int32_t)0x80004005u, "x");
    assert(posts == 1 && post_w == 0x17ac && strcmp(events[1], "!!! GENERIC (Undefined) ERROR ") == 0 &&
           strcmp(events[2], "POST 17ac") == 0);
    /* 0x41db3c: 0x17ab, then the "connecting" line (0x55501c); 0x41db19:
       the same line for 0x8877015e without a post. */
    reset();
    aoe_comm_dp_error_ror(&p, &o, (int32_t)0x8877014au, "x");
    assert(posts == 1 && post_w == 0x17ab &&
           strcmp(events[3], "!!! The session is in the process of connecting") == 0);
    reset();
    aoe_comm_dp_error_ror(&p, &o, (int32_t)0x8877015eu, "x");
    assert(posts == 0 && event_count == 2 &&
           strcmp(events[1], "!!! The session is in the process of connecting") == 0);
}

static void posted_codes(void) {
    static const struct { uint32_t code, w; } posted[] = {
        {0x80070005u, 0x17ae}, {0x8877000au, 0x17ae}, {0x88770140u, 0x17ae},
        {0x88770118u, 0x17ae}, {0x80004005u, 0x17ac}, {0x80004002u, 0x17ac},
        {0x88770028u, 0x17ab}, {0x887700fau, 0x17ab}, {0x8877014au, 0x17ab},
        {0x887700f0u, 0x17b9}, {0x88770136u, 0x17a8},
    };
    /* Log only (each a `ret 8` after its line). */
    static const uint32_t quiet[] = {
        0x8000000au, 0x80004001u, 0x80004003u, 0x80004004u, 0x80040110u, 0x80070006u,
        0x8007000eu, 0x80070057u, 0x88770005u, 0x88770014u, 0x8877001eu, 0x88770032u,
        0x8877003cu, 0x88770046u, 0x88770050u, 0x8877005au, 0x88770078u, 0x88770082u,
        0x88770096u, 0x8877009bu, 0x887700a0u, 0x887700aau, 0x887700beu, 0x887700c8u,
        0x887700d2u, 0x887700dcu, 0x887700e6u, 0x8877010eu, 0x88770122u, 0x8877012cu,
        0x88770154u, 0x8877015eu, 0x887703e8u, 0x887703f2u, 0x887703fcu, 0x88770410u,
        0x8877041au, 0x8877042eu, 0x887707d0u, 0x887707dau, 0x887707e4u, 0x887707eeu,
        0x887707f8u, 0x8877080cu, 0x88770820u,
    };
    AoeCommDpErrorOpsRor o = ops();
    AoeWindowPosterRor p;
    size_t i;
    aoe_poster_init_ror(&p, 3);
    for (i = 0; i < sizeof posted / sizeof posted[0]; ++i) {
        reset();
        aoe_comm_dp_error_ror(&p, &o, (int32_t)posted[i].code, "Open");
        assert(posts == 1 && post_w == posted[i].w && post_window == 3);
        assert(p.field_4 == (int32_t)posted[i].code);
        assert(aoe_comm_dp_error_post_ror((int32_t)posted[i].code) == posted[i].w);
    }
    for (i = 0; i < sizeof quiet / sizeof quiet[0]; ++i) {
        reset();
        aoe_comm_dp_error_ror(&p, &o, (int32_t)quiet[i], "Open");
        assert(posts == 0 && p.field_4 == (int32_t)quiet[i]);
        assert(aoe_comm_dp_error_post_ror((int32_t)quiet[i]) == 0);
        /* None reaches the default line. */
        assert(event_count >= 1 && strstr(events[event_count - 1], "Unhandled") == NULL);
    }
}

static void special_lines(void) {
    AoeCommDpErrorOpsRor o = ops();
    AoeWindowPosterRor p;
    aoe_poster_init_ror(&p, 3);
    /* 0x41d9d3: two lines, 0x55514c then 0x555140. */
    reset();
    aoe_comm_dp_error_ror(&p, &o, (int32_t)0x88770050u, "Get Caps");
    assert(event_count == 3 && strcmp(events[1], "!!! Device caps are not available.") == 0 &&
           strcmp(events[2], "!!! ERROR!") == 0);
    /* 0x41dc72: 0x88770410 returns after the header line and the store. */
    reset();
    p.field_4 = 0;
    aoe_comm_dp_error_ror(&p, &o, (int32_t)0x88770410u, "Open");
    assert(event_count == 1 && posts == 0 && p.field_4 == (int32_t)0x88770410u);
    /* 0x41dd8f: an unknown failure and a positive code take the default. */
    reset();
    aoe_comm_dp_error_ror(&p, &o, (int32_t)0x88770001u, "Receive");
    assert(event_count == 2 && posts == 0 && p.field_4 == (int32_t)0x88770001u);
    assert(strcmp(events[0], "+++COMM______DP CALL FAILED_(Code 2289500161)_____ Receive____") ==
           0);
    assert(strcmp(events[1], "!!! Receive: Unhandled DP ERROR/exception in call.  "
                             "Result=-2005467135  (-2005467135)") == 0);
    reset();
    aoe_comm_dp_error_ror(&p, &o, 1, "Send");
    assert(event_count == 2 && p.field_4 == 1 &&
           strcmp(events[1], "!!! Send: Unhandled DP ERROR/exception in call.  Result=1  (1)") ==
               0);
}

static void without_log(void) {
    AoeCommDpErrorOpsRor o = ops();
    AoeWindowPosterRor p;
    o.log = NULL;
    aoe_poster_init_ror(&p, 5);
    reset();
    /* The log is empty in this build: only the store and the post remain. */
    aoe_comm_dp_error_ror(&p, &o, (int32_t)0x88770136u, NULL);
    assert(posts == 1 && post_w == 0x17a8 && post_window == 5 && event_count == 1);
    assert(p.field_4 == (int32_t)0x88770136u);
}

int main(void) {
    zero_code_does_nothing();
    timeout_posts_host_busy();
    post_order();
    posted_codes();
    special_lines();
    without_log();
    puts("comm_handler_session_test: ok");
    return 0;
}
