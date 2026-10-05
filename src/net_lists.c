#include "net_lists.h"

#include <string.h>

static int provider_ok(const AoeNetProvidersRor *l, int32_t i) {
    /* The original accepts i == count; the port also keeps it in the
       table. */
    return l && i >= 0 && i <= l->count && i <= AOE_NET_PROVIDERS;
}

void aoe_net_providers_init_ror(AoeNetProvidersRor *l,
                                const AoeNetProviderOpsRor *ops) {
    int32_t i;
    if (!l) return;
    l->count = 0;
    for (i = 0; i < AOE_NET_PROVIDERS; ++i) {
        l->items[i].name[0] = '\0';
        l->items[i].guid = NULL;
        l->items[i].field_54 = l->items[i].field_58 = 0;
        l->items[i].usable = 0;
    }
    if (ops && ops->enumerate) ops->enumerate(ops->context, l);
}

int32_t aoe_net_providers_count_ror(const AoeNetProvidersRor *l) {
    return l ? l->count : 0;
}

void aoe_net_providers_grow_ror(AoeNetProvidersRor *l) {
    if (l) ++l->count;
}

AoeNetProviderRor *aoe_net_provider_ror(AoeNetProvidersRor *l, int32_t i) {
    return provider_ok(l, i) ? &l->items[i] : NULL;
}

int aoe_net_provider_set_name_ror(AoeNetProvidersRor *l, int32_t i,
                                  const char *name) {
    size_t n;
    /* Port guard: the original copies without checks. */
    if (!l || i < 0 || i > AOE_NET_PROVIDERS || !name) return 1;
    n = strlen(name);
    if (n >= sizeof(l->items[i].name)) n = sizeof(l->items[i].name) - 1;
    memcpy(l->items[i].name, name, n);
    l->items[i].name[n] = '\0';
    return 1;
}

void aoe_net_provider_set_guid_ror(AoeNetProvidersRor *l, int32_t i,
                                   const void *guid) {
    if (provider_ok(l, i)) l->items[i].guid = guid;
}

int32_t aoe_net_provider_usable_ror(const AoeNetProvidersRor *l, int32_t i) {
    return provider_ok(l, i) ? l->items[i].usable : 0;
}

void aoe_net_provider_set_usable_ror(AoeNetProvidersRor *l, int32_t i,
                                     int32_t usable) {
    if (provider_ok(l, i)) l->items[i].usable = usable;
}

int aoe_net_provider_found_ror(AoeNetProvidersRor *l, const void *guid,
                               const char *name,
                               const AoeNetProviderOpsRor *ops) {
    int32_t i = aoe_net_providers_count_ror(l);
    if (!l) return 1;
    aoe_net_provider_set_name_ror(l, i, name);
    aoe_net_provider_set_guid_ror(l, i, guid);
    aoe_net_provider_set_usable_ror(
        l, i, ops && ops->try_create && ops->try_create(ops->context, guid) ? 1 : 0);
    aoe_net_providers_grow_ror(l);
    return 1;
}

static int session_ok(const AoeNetSessionsRor *l, int32_t i) {
    return l && i >= 0 && i <= l->count && i <= AOE_NET_SESSIONS;
}

void aoe_net_sessions_clear_ror(AoeNetSessionsRor *l) {
    if (l) memset(l, 0, sizeof(*l));
}

void aoe_net_sessions_refresh_ror(AoeNetSessionsRor *l,
                                  const AoeNetSessionOpsRor *ops) {
    aoe_net_sessions_clear_ror(l);
    if (l && ops && ops->enumerate) ops->enumerate(ops->context, l);
}

void aoe_net_sessions_grow_ror(AoeNetSessionsRor *l) {
    if (l) ++l->count;
}

static void copy_text(char *dst, size_t size, const char *src) {
    size_t n = strlen(src);
    if (n >= size) n = size - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

int aoe_net_sessions_add_ror(AoeNetSessionsRor *l,
                             const AoeNetSessionDescRor *d) {
    AoeNetSessionRor *s;
    /* Port guard: the original writes the next slot unchecked. */
    if (!l || !d || l->count < 0 || l->count > AOE_NET_SESSIONS) return 1;
    s = &l->items[l->count];
    memcpy(s->application, d->application, sizeof(s->application));
    memcpy(s->instance, d->instance, sizeof(s->instance));
    s->max_players = d->max_players;
    s->players = d->players;
    s->user = (int32_t)d->user[3];
    if (d->name) copy_text(s->name, sizeof(s->name), d->name);
    if (d->password) copy_text(s->password, sizeof(s->password), d->password);
    aoe_net_sessions_grow_ror(l);
    return 1;
}

int aoe_net_session_found_ror(AoeNetSessionsRor *l,
                              const AoeNetSessionDescRor *d, uint32_t flags) {
    if (flags & 1) return 0;
    aoe_net_sessions_add_ror(l, d);
    return 1;
}

AoeNetSessionRor *aoe_net_session_ror(AoeNetSessionsRor *l, int32_t i) {
    return session_ok(l, i) ? &l->items[i] : NULL;
}

int32_t aoe_net_session_max_players_ror(const AoeNetSessionsRor *l, int32_t i) {
    return session_ok(l, i) ? l->items[i].max_players : 0;
}

int32_t aoe_net_session_players_ror(const AoeNetSessionsRor *l, int32_t i) {
    return session_ok(l, i) ? l->items[i].players : 0;
}

const uint32_t *aoe_net_session_instance_ror(const AoeNetSessionsRor *l,
                                             int32_t i) {
    return session_ok(l, i) ? l->items[i].instance : NULL;
}

int aoe_net_provider_guid_ror(const AoeNetProvidersRor *list, int32_t index, uint8_t out[16]) {
    if (!list || !out || index < 0 || index > AOE_NET_PROVIDERS || !list->items[index].guid)
        return 0;
    memcpy(out, list->items[index].guid, 16);
    return 1;
}

void aoe_net_sessions_init_ror(AoeNetSessionsRor *list, AoeNetSessionStateRor *state,
                               const void *comm, void *dplay, const uint32_t guids[8],
                               const AoeNetSessionOpsRor *ops) {
    if (state) {
        state->comm = comm;
        state->dplay = dplay;
        if (guids) {
            memcpy(state->application, guids, sizeof state->application);
            memcpy(state->guid_12ec, guids + 4, sizeof state->guid_12ec);
        }
    }
    if (!list) return;
    list->count = 0;
    aoe_net_sessions_clear_ror(list);
    aoe_net_sessions_refresh_ror(list, ops);
}
