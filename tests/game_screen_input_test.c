#include "game_screen_input.h"

#include <assert.h>
#include <string.h>

typedef struct {
    int state, mode, sub, issue, busy, consumed_chat;
    int last, arg1, arg2, calls;
    char sent[64];
} Ctx;

#define REC(name, code) static void name(void *p) { Ctx *x = p; x->last = code; x->calls++; }
REC(r_help, 1) REC(r_a, 2) REC(r_b, 3) REC(r_c, 4) REC(r_d, 5) REC(r_enter, 6) REC(r_space, 7)
REC(r_home, 8) REC(r_del, 9) REC(r_t, 10) REC(r_refresh, 11) REC(r_be, 12) REC(r_clear, 13)
static void r_n(void *p, int32_t n) { Ctx *x = p; x->last = 20; x->arg1 = n; x->calls++; }
static void r_grp(void *p, int32_t n, int m) { Ctx *x = p; x->last = 21; x->arg1 = n; x->arg2 = m; x->calls++; }
static void r_cyc(void *p, int w) { Ctx *x = p; x->last = 22; x->arg1 = w; x->calls++; }
static void r_gk(void *p, int32_t n) { Ctx *x = p; x->last = 23; x->arg1 = n; x->calls++; }
static void r_s(void *p, int on) { Ctx *x = p; x->last = 24; x->arg1 = on; x->calls++; }
static void r_zoom(void *p, int z) { Ctx *x = p; x->arg2 = z; }
static void r_press(void *p, int32_t id) { Ctx *x = p; x->last = 25; x->arg1 = id; x->calls++; }
static void r_fkey(void *p, int32_t n) { Ctx *x = p; x->last = 26; x->arg1 = n; x->calls++; }
static int zero(void *p) { (void)p; return 0; }
static int one(void *p) { (void)p; return 1; }
static int busy(void *p) { return ((Ctx *)p)->busy; }
static int ckey(void *p, int32_t k, int32_t n, int a, int c, int s) { (void)p; (void)k; (void)n; (void)a; (void)c; (void)s; return 0; }
static int32_t gstate(void *p) { return ((Ctx *)p)->state; }
static int32_t gmode(void *p) { return ((Ctx *)p)->mode; }
static int32_t gsub(void *p) { return ((Ctx *)p)->sub; }
static int issue(void *p) { return ((Ctx *)p)->issue; }
static int chat(void *p, const char *t) { Ctx *x = p; strcpy(x->sent, t); return x->consumed_chat; }
static int32_t bcount(void *p) { (void)p; return 2; }
static int32_t bid(void *p, int32_t i) { (void)p; return 10 + i; }
static char bhot(void *p, int32_t id) { (void)p; return id == 11 ? 'B' : 'A'; }

int main(void)
{
    Ctx x = {0};
    AoeGameScreenInputOpsRor o = {
        &x, busy, zero, ckey, gstate, gmode, gsub, issue, chat, zero, bcount, bid, bhot,
        r_clear, r_press, r_n, r_cyc, r_grp, r_help, r_fkey, r_gk, r_a, r_b, r_c, r_d,
        r_enter, r_space, r_home, r_del, one, r_s, r_t, zero, r_zoom, r_refresh, r_be};
    AoeGameScreenInputRor s;
    memset(&s, 0, sizeof s);
    x.issue = 1;

    /* Chat: lower case upper-cased, punctuation dropped. */
    assert(aoe_game_screen_key_down_ror(&s, &o, 'h', 1, 1, 1, 1) == 1);
    assert(aoe_game_screen_key_down_ror(&s, &o, '!', 1, 1, 1, 1) == 1);
    assert(s.chat_len == 1 && strcmp(s.chat, "H") == 0 && strcmp(x.sent, "H") == 0);
    /* Full buffer scrolls left. */
    s.chat_len = AOE_GS_INPUT_CHAT_LEN;
    memset(s.chat, 'X', AOE_GS_INPUT_CHAT_LEN); s.chat[0] = 'Y'; s.chat[AOE_GS_INPUT_CHAT_LEN] = 0;
    s.chat[AOE_GS_INPUT_CHAT_LEN + 1] = 0;
    aoe_game_screen_key_down_ror(&s, &o, 'Z', 1, 1, 1, 1);
    assert(s.chat[0] == 'X' && s.chat[AOE_GS_INPUT_CHAT_LEN - 1] == 'Z' && s.chat_len == AOE_GS_INPUT_CHAT_LEN);
    x.consumed_chat = 1;
    aoe_game_screen_key_down_ror(&s, &o, '1', 1, 1, 1, 1);
    assert(s.chat_len == 0 && s.chat[0] == 0);

    /* Ctrl+digit selects group. */
    assert(aoe_game_screen_key_down_ror(&s, &o, '3', 1, 0, 1, 0) == 1 && x.last == 20 && x.arg1 == 3);
    /* Shift F-keys gated on can_issue. */
    assert(aoe_game_screen_key_down_ror(&s, &o, 0x73, 1, 0, 0, 1) == 1 && x.last == 26 && x.arg1 == 2);
    /* Alt+digit: group mode 1 then space. */
    assert(aoe_game_screen_key_down_ror(&s, &o, '4', 1, 1, 0, 0) == 1 && x.last == 7);
    /* Plain keys. */
    assert(aoe_game_screen_key_down_ror(&s, &o, 9, 1, 0, 0, 0) == 1 && x.last == 22 && x.arg1 == 1);
    assert(aoe_game_screen_key_down_ror(&s, &o, '5', 1, 0, 0, 0) == 0 && x.last == 21 && x.arg2 == 1);
    assert(aoe_game_screen_key_down_ror(&s, &o, 0x73, 1, 0, 0, 0) == 0 && x.last == 24 && x.arg1 == 0);
    assert(aoe_game_screen_key_down_ror(&s, &o, 0x7a, 1, 0, 0, 0) == 0 && x.last == 11 && x.arg2 == 1);
    /* Rebindable key a, blocked when cannot issue. */
    s.key_a = 'Q'; x.issue = 0; x.calls = 0;
    assert(aoe_game_screen_key_down_ror(&s, &o, 'Q', 1, 0, 0, 0) == 0 && x.calls == 0);
    x.issue = 1;
    aoe_game_screen_key_down_ror(&s, &o, 'Q', 1, 0, 0, 0);
    assert(x.last == 2);
    /* Ctrl group keys. */
    s.group_keys[5] = 'G';
    assert(aoe_game_screen_key_down_ror(&s, &o, 'G', 1, 0, 1, 0) == 1 && x.last == 23 && x.arg1 == 5);
    /* Command button hotkeys in mode 6/1. */
    x.mode = 6; x.sub = 1;
    assert(aoe_game_screen_key_down_ror(&s, &o, 'B', 1, 0, 0, 0) == 1 && x.last == 25 && x.arg1 == 11);
    /* Busy swallows nothing. */
    x.busy = 1; x.calls = 0;
    assert(aoe_game_screen_key_down_ror(&s, &o, 9, 1, 0, 0, 0) == 0 && x.calls == 0);
    return 0;
}
