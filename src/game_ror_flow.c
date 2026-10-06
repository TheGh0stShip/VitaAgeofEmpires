#include "game_ror_flow.h"

#include <stddef.h>

typedef struct { int32_t index; int32_t id; } FlowPair;

static int32_t flow_lookup(const FlowPair *t, size_t n, int32_t index)
{
    size_t i;
    for (i = 0; i < n; ++i)
        if (t[i].index == index)
            return t[i].id;
    return -1;
}

#define FLOW_N(a) (sizeof(a) / sizeof((a)[0]))

static const FlowPair k_kind1[] = {
    {100, 0x961}, {0x65, 0x961}, {0x66, 0x961}, {0x67, 0x962},
    {0x68, 0x963}, {0x69, 0x964}, {0x6a, 0x965}};
static const FlowPair k_kind100[] = {
    {0, 0x10cd}, {0xf, 0x10cd}, {0x10, 0x10cd}, {0x11, 0x10cd},
    {1, 0x10ce}, {2, 0x10cf}, {3, 0x10d0}, {9, 0x10db}};
static const FlowPair k_kind101[] = {
    {1, 0x1069}, {2, 0x106a}, {3, 0x106b}, {4, 0x106c}};
static const FlowPair k_kind102[] = {
    {0x19, 0x1020}, {0x1a, 0x1021}, {0x1b, 0x1022}, {0x1c, 0x101c},
    {0x1d, 0x101d}, {0x1e, 0x101e}, {0x1f, 0x101f}, {0x20, 0x1025},
    {0x21, 0x1026}};
static const FlowPair k_kind103_sub[] = {
    {0, 0xbb9}, {1, 0xbba}, {2, 0xbbb}, {3, 0xbbc}, {4, 0xbbd},
    {0x20, 0xbe6}};
static const FlowPair k_kind103[] = {
    {0x65, 0xbbe}, {0x66, 0xbbf}, {0x67, 0xbc0}, {0x68, 0xbc1},
    {0x69, 0xbc3}, {0x6a, 0xbca}, {1000, 0xbc2}};
static const FlowPair k_kind104[] = {
    {1, 0x10d4}, {2, 0x10d1}, {3, 0x10d2}, {4, 0x10d3}, {5, 0x10d5},
    {6, 0x10d6}, {7, 0x10d7}, {8, 0x10d8}, {9, 0x10d9}, {10, 0x10da},
    {0xb, 0x10dc}};
static const FlowPair k_kind105[] = {
    {0xd, 0x2806}, {0xe, 0x2807}, {0xf, 0x2809}, {0x10, 0x2808}};

int32_t game_ror_flow_message_string_id(int32_t kind, int32_t index,
                                        int32_t sub, int *defer_to_base,
                                        int *terminate)
{
    int32_t id = -1;
    *defer_to_base = 0;
    *terminate = 0;
    switch (kind) {
    case 1:
        id = flow_lookup(k_kind1, FLOW_N(k_kind1), index);
        if (id < 0)
            *defer_to_base = 1;
        return id;
    case 100:
        id = flow_lookup(k_kind100, FLOW_N(k_kind100), index);
        break;
    case 0x65:
        id = flow_lookup(k_kind101, FLOW_N(k_kind101), index);
        break;
    case 0x66:
        /* 1..0x17 map to 0x1005..0x101b; 0x18 has no entry. */
        if (index >= 1 && index <= 0x17)
            id = 0x1004 + index;
        else
            id = flow_lookup(k_kind102, FLOW_N(k_kind102), index);
        break;
    case 0x67:
        if (index == 1)
            id = flow_lookup(k_kind103_sub, FLOW_N(k_kind103_sub), sub);
        else
            id = flow_lookup(k_kind103, FLOW_N(k_kind103), index);
        break;
    case 0x68:
        id = flow_lookup(k_kind104, FLOW_N(k_kind104), index);
        break;
    case 0x69:
        /* Every kind-105 entry is loaded and then force-terminated. */
        *terminate = 1;
        if (index >= 1 && index <= 0xc)
            return 0x27f6 + index;
        id = flow_lookup(k_kind105, FLOW_N(k_kind105), index);
        return id < 0 ? 0x2804 : id;
    default:
        *defer_to_base = 1;
        return -1;
    }
    if (id < 0)
        *terminate = 1;
    return id;
}

/* 0x004ff580: tribe game message-string formatter (vtable +0x28). Clears
 * buf, then maps (kind, index[, sub]) to a string resource. Kind 1 entries
 * outside the table and unknown kinds defer to the base formatter
 * 0x004174e0. Known kinds with no entry leave the empty string. Kind 105
 * always terminates buf[len-1]; other hits return the loader's result. */
char *game_ror_flow_format_message(const GameRorFlowStringOps *ops,
                                   int32_t kind, int32_t index, int32_t sub,
                                   char *buf, int32_t len)
{
    int defer, terminate;
    int32_t id;
    buf[0] = 0;
    id = game_ror_flow_message_string_id(kind, index, sub, &defer, &terminate);
    if (defer)
        return ops->base_format(ops->ctx, kind, index, sub, buf, len);
    if (id >= 0) {
        char *r = ops->load_string(ops->ctx, id, buf, len);
        if (!terminate)
            return r;
    }
    if (len > 0)
        buf[len - 1] = 0;
    return buf;
}
