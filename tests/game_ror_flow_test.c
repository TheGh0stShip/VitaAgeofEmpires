#include "game_ror_flow.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int32_t g_last_id;
static int g_base_calls;
static char g_ret_sentinel[4];

static char *load(void *ctx, int32_t id, char *buf, int32_t len)
{
    (void)ctx;
    g_last_id = id;
    snprintf(buf, (size_t)len, "s%d", (int)id);
    return g_ret_sentinel;
}

static char *base(void *ctx, int32_t kind, int32_t index, int32_t sub,
                  char *buf, int32_t len)
{
    (void)ctx; (void)kind; (void)index; (void)sub; (void)len;
    ++g_base_calls;
    return buf;
}

int main(void)
{
    GameRorFlowStringOps ops = {NULL, load, base};
    char buf[16];
    char *r;

    r = game_ror_flow_format_message(&ops, 1, 0x67, 0, buf, 16);
    assert(r == g_ret_sentinel && g_last_id == 0x962);
    r = game_ror_flow_format_message(&ops, 1, 5, 0, buf, 16);
    assert(g_base_calls == 1 && r == buf);
    game_ror_flow_format_message(&ops, 7, 1, 0, buf, 16);
    assert(g_base_calls == 2);

    game_ror_flow_format_message(&ops, 100, 0x11, 0, buf, 16);
    assert(g_last_id == 0x10cd);
    game_ror_flow_format_message(&ops, 0x66, 0x17, 0, buf, 16);
    assert(g_last_id == 0x101b);
    game_ror_flow_format_message(&ops, 0x66, 0x1c, 0, buf, 16);
    assert(g_last_id == 0x101c);
    game_ror_flow_format_message(&ops, 0x66, 0x19, 0, buf, 16);
    assert(g_last_id == 0x1020);
    game_ror_flow_format_message(&ops, 0x67, 1, 0x20, buf, 16);
    assert(g_last_id == 0xbe6);
    game_ror_flow_format_message(&ops, 0x67, 1000, 0, buf, 16);
    assert(g_last_id == 0xbc2);
    game_ror_flow_format_message(&ops, 0x68, 2, 0, buf, 16);
    assert(g_last_id == 0x10d1);

    /* Known kind, unknown index: empty, terminated buffer. */
    g_last_id = 0;
    r = game_ror_flow_format_message(&ops, 0x66, 0x18, 0, buf, 16);
    assert(r == buf && buf[0] == 0 && g_last_id == 0);
    r = game_ror_flow_format_message(&ops, 0x67, 1, 5, buf, 16);
    assert(r == buf && buf[0] == 0 && g_last_id == 0);

    /* Kind 105 returns buf, forcibly terminated; unknown -> 0x2804. */
    memset(buf, 0, sizeof buf);
    r = game_ror_flow_format_message(&ops, 0x69, 0xc, 0, buf, 16);
    assert(r == buf && g_last_id == 0x2802 && buf[15] == 0);
    game_ror_flow_format_message(&ops, 0x69, 0x10, 0, buf, 16);
    assert(g_last_id == 0x2808);
    game_ror_flow_format_message(&ops, 0x69, 99, 0, buf, 16);
    assert(g_last_id == 0x2804);
    game_ror_flow_format_message(&ops, 0x69, 0, 0, buf, 16);
    assert(g_last_id == 0x2804);
    puts("game_ror_flow_test: ok");
    return 0;
}
