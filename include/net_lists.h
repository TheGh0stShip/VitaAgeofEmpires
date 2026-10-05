#ifndef VITA_AOE_NET_LISTS_H
#define VITA_AOE_NET_LISTS_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a multiplayer enumeration lists
   (0x00428380..0x00428900): the service providers and the sessions found
   on one, filled by the platform's enumeration callbacks (DirectPlay in
   the original). Index checks accept index == count, as the original's
   do. Semantic storage; offsets are noted. */

#define AOE_NET_PROVIDERS 100
#define AOE_NET_SESSIONS 500

typedef struct {
    char name[0x50];          /* +0x00 */
    const void *guid;         /* +0x50: the provider's GUID (a pointer) */
    int32_t field_54, field_58;
    int32_t usable;           /* +0x5c: the provider could be created */
} AoeNetProviderRor;

typedef struct {
    AoeNetProviderRor items[AOE_NET_PROVIDERS + 1];
    int32_t count;            /* +0x2580 */
} AoeNetProvidersRor;

typedef struct {
    void *context;
    /* DirectPlayEnumerate: calls back once per provider. */
    void (*enumerate)(void *context, AoeNetProvidersRor *list);
    /* Create the provider and release it; nonzero when that worked. */
    int (*try_create)(void *context, const void *guid);
} AoeNetProviderOpsRor;

/* 0x00428380: empty names and fields for all entries, count 0, then the
   enumeration. 0x00428540: the enumeration callback: the next entry gets
   the name and GUID, usable when the provider can be created; the count
   grows either way. Accessors: 0x00428400 count, 0x00428410 one more,
   0x00428420 an entry, 0x00428440 name (copied whole), 0x004284b0 GUID,
   0x004284e0 / 0x00428510 usable. */
void aoe_net_providers_init_ror(AoeNetProvidersRor *list,
                                const AoeNetProviderOpsRor *ops);
int aoe_net_provider_found_ror(AoeNetProvidersRor *list, const void *guid,
                               const char *name,
                               const AoeNetProviderOpsRor *ops);
int32_t aoe_net_providers_count_ror(const AoeNetProvidersRor *list);
void aoe_net_providers_grow_ror(AoeNetProvidersRor *list);
AoeNetProviderRor *aoe_net_provider_ror(AoeNetProvidersRor *list,
                                        int32_t index);
int aoe_net_provider_set_name_ror(AoeNetProvidersRor *list, int32_t index,
                                  const char *name);
void aoe_net_provider_set_guid_ror(AoeNetProvidersRor *list, int32_t index,
                                   const void *guid);
int32_t aoe_net_provider_usable_ror(const AoeNetProvidersRor *list,
                                    int32_t index);
/* 0x00428480: entry `index`'s GUID copied out (16 bytes through its
   pointer; the original reads it unchecked, the port gives 0 for none). */
int aoe_net_provider_guid_ror(const AoeNetProvidersRor *list, int32_t index, uint8_t out[16]);
void aoe_net_provider_set_usable_ror(AoeNetProvidersRor *list, int32_t index,
                                     int32_t usable);

typedef struct {
    char name[0xff];          /* +0x00 */
    char password[0x101];     /* +0xff */
    uint32_t application[4];  /* +0x200 */
    uint32_t instance[4];     /* +0x210 */
    int32_t players;          /* +0x220 */
    int32_t max_players;      /* +0x224 */
    int32_t user;             /* +0x228: the last of the four user words */
    int32_t pad[3];
} AoeNetSessionRor;

typedef struct {
    int32_t count;            /* +0x00 */
    AoeNetSessionRor items[AOE_NET_SESSIONS + 1];
} AoeNetSessionsRor;

/* The session description passed to the enumeration callback. */
typedef struct {
    uint32_t flags_unused;
    uint32_t instance[4], application[4];
    int32_t max_players, players;
    const char *name, *password;
    uint32_t user[4];
} AoeNetSessionDescRor;

typedef struct {
    void *context;
    /* IDirectPlay2::EnumSessions (4000 ms, the game's application GUID). */
    void (*enumerate)(void *context, AoeNetSessionsRor *list);
} AoeNetSessionOpsRor;

/* 0x004285c0, the session list's constructor: the enumeration state
   (globals 0x00581800..0x00581814) takes the comm handler, its
   DirectPlay interface (0x00424ea0), and its two GUIDs (+0x12dc and
   +0x12ec); the count becomes 0; then a clear (0x00428650) and a refresh
   (0x00428890). */
typedef struct {
    const void *comm;
    void *dplay;
    uint32_t application[4];   /* comm +0x12dc */
    uint32_t guid_12ec[4];     /* comm +0x12ec */
} AoeNetSessionStateRor;
void aoe_net_sessions_init_ror(AoeNetSessionsRor *list, AoeNetSessionStateRor *state,
                               const void *comm, void *dplay, const uint32_t guids[8],
                               const AoeNetSessionOpsRor *ops);
/* 0x00428650 clear; 0x00428890 clear, then enumerate; 0x00428900 the
   callback: a timed-out call (flag 1) stops, else the session is added
   (0x00428730: GUIDs, player counts, name and password; every user word
   lands in the same slot) and the count grows (0x00428720). Accessors
   0x00428670 entry, 0x00428690 maximum players, 0x004286c0 players,
   0x004286f0 instance GUID. */
void aoe_net_sessions_clear_ror(AoeNetSessionsRor *list);
void aoe_net_sessions_refresh_ror(AoeNetSessionsRor *list,
                                  const AoeNetSessionOpsRor *ops);
int aoe_net_session_found_ror(AoeNetSessionsRor *list,
                              const AoeNetSessionDescRor *desc,
                              uint32_t flags);
int aoe_net_sessions_add_ror(AoeNetSessionsRor *list,
                             const AoeNetSessionDescRor *desc);
void aoe_net_sessions_grow_ror(AoeNetSessionsRor *list);
AoeNetSessionRor *aoe_net_session_ror(AoeNetSessionsRor *list, int32_t index);
int32_t aoe_net_session_max_players_ror(const AoeNetSessionsRor *list,
                                        int32_t index);
int32_t aoe_net_session_players_ror(const AoeNetSessionsRor *list,
                                    int32_t index);
const uint32_t *aoe_net_session_instance_ror(const AoeNetSessionsRor *list,
                                             int32_t index);

#endif
