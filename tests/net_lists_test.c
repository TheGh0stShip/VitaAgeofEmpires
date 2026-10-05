/* Patched RoR 1.0a multiplayer enumeration lists 0x00428380..0x00428900. */
#include "net_lists.h"

#include <assert.h>
#include <string.h>

static int guid_a, guid_b;
static int try_create(void *c, const void *g) { (void)c; return g == &guid_a; }
static void enumerate(void *c, AoeNetProvidersRor *l) {
    const AoeNetProviderOpsRor *ops = c;
    aoe_net_provider_found_ror(l, &guid_a, "TCP/IP", ops);
    aoe_net_provider_found_ror(l, &guid_b, "Modem", ops);
}

static void sessions(void *c, AoeNetSessionsRor *l) {
    AoeNetSessionDescRor d;
    (void)c;
    memset(&d, 0, sizeof(d));
    d.instance[0] = 7;
    d.max_players = 8;
    d.players = 3;
    d.name = "Game";
    d.user[0] = 1;
    d.user[3] = 4;
    assert(aoe_net_session_found_ror(l, &d, 0) == 1);
    assert(aoe_net_session_found_ror(l, &d, 1) == 0);
}

int main(void) {
    static AoeNetProvidersRor p;
    static AoeNetSessionsRor s;
    AoeNetProviderOpsRor ops = {NULL, enumerate, try_create};
    AoeNetSessionOpsRor sops = {NULL, sessions};
    ops.context = &ops;

    aoe_net_providers_init_ror(&p, &ops);
    assert(aoe_net_providers_count_ror(&p) == 2);
    assert(strcmp(aoe_net_provider_ror(&p, 0)->name, "TCP/IP") == 0 &&
           aoe_net_provider_ror(&p, 1)->guid == &guid_b);
    assert(aoe_net_provider_usable_ror(&p, 0) == 1 &&
           aoe_net_provider_usable_ror(&p, 1) == 0);
    /* Index == count is accepted; beyond it is not. */
    assert(aoe_net_provider_ror(&p, 2) && !aoe_net_provider_ror(&p, 3) &&
           !aoe_net_provider_ror(&p, -1));

    aoe_net_sessions_refresh_ror(&s, &sops);
    assert(s.count == 1 && strcmp(aoe_net_session_ror(&s, 0)->name, "Game") == 0);
    assert(aoe_net_session_max_players_ror(&s, 0) == 8 &&
           aoe_net_session_players_ror(&s, 0) == 3 &&
           aoe_net_session_instance_ror(&s, 0)[0] == 7 &&
           aoe_net_session_ror(&s, 0)->user == 4);
    assert(aoe_net_session_ror(&s, 1) && !aoe_net_session_ror(&s, 2));
    aoe_net_sessions_clear_ror(&s);
    assert(s.count == 0 && s.items[0].name[0] == '\0');
    {
        /* 0x00428480 */
        static AoeNetProvidersRor pl;
        static const uint8_t g[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        uint8_t out[16];
        pl.items[2].guid = g;
        assert(aoe_net_provider_guid_ror(&pl, 2, out) == 1 && memcmp(out, g, 16) == 0);
        assert(aoe_net_provider_guid_ror(&pl, 3, out) == 0);
    }
    {
        /* 0x004285c0 */
        static AoeNetSessionsRor sl;
        AoeNetSessionStateRor st;
        uint32_t g[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        int comm;
        sl.count = 9;
        aoe_net_sessions_init_ror(&sl, &st, &comm, NULL, g, NULL);
        assert(sl.count == 0 && st.comm == &comm && st.application[3] == 4 &&
               st.guid_12ec[0] == 5);
    }
    return 0;
}
