#ifndef VITA_AOE_CAMPAIGN_PROGRESS_H
#define VITA_AOE_CAMPAIGN_PROGRESS_H

#include <stdint.h>

#include "aoe_stream.h"

/* Patched Rise of Rome 1.0a campaigns (0x0041c2d0..0x0041c810) and the
   player information file that records each player's campaign progress
   (0x0043bb40..0x0043cef0). A campaign file is a 0x108-byte header
   ("1.00", title, scenario count), a 0x208-byte entry per scenario (data
   size, data offset, name, file name) and the scenario data. Files and
   directory listings go through AoeCampaignOpsRor. Semantic storage;
   offsets are noted. */

typedef struct {
    int32_t size, offset;     /* +0x00, +0x04 */
    char name[0xff];          /* +0x08 */
    char file[0x101];         /* +0x107 */
} AoeCampaignEntryRor;

typedef struct {
    char version[4];          /* +0x000 */
    char title[0x100];        /* +0x004 */
    int32_t count;            /* +0x104 */
    AoeCampaignEntryRor *entries; /* +0x108 */
    char path[0x100];         /* +0x10c (0x20c bytes in all) */
} AoeCampaignRor;

typedef struct {
    void *context;
    const char *campaign_dir;   /* game settings +0xd1c */
    const char *scenario_dir;   /* game settings +0xc17 */
    /* A whole file (heap, free()) or NULL; writing one. */
    uint8_t *(*read_file)(void *context, const char *path, uint32_t *size);
    int (*write_file)(void *context, const char *path, const uint8_t *data,
                      uint32_t size);
    /* The scenario stream: `size` bytes at `offset` of a file (-1 when it
       cannot be opened). */
    int32_t (*open_slice)(void *context, const char *path, int32_t offset,
                          int32_t size);
    /* Directory listing: `each` for every file matching the pattern. */
    void (*find)(void *context, const char *pattern,
                 void (*each)(void *user, const char *name), void *user);
    /* The player file's compressed stream: whole contents (heap) or NULL,
       and writing it. */
    uint8_t *(*read_pif)(void *context, const char *path, uint32_t *size);
    int (*write_pif)(void *context, const char *path, const uint8_t *data,
                     uint32_t size);
    void (*log)(void *context, const char *text);
} AoeCampaignOpsRor;

/* 0x0041c2d0: load "<campaign dir><name>" (a missing file gives title
   "FILE NOT FOUND" and no scenarios). 0x0041c3d0: a new campaign of
   scenarios (file names, display names) written at once (0x0041c540:
   the header and entries, each scenario file appended and its size and
   offset filled in). 0x0041c510 frees the entries. 0x0041c700: the
   stream of scenario `i` (-1 outside). 0x0041c780 title, 0x0041c790
   count, 0x0041c7a0 a copy of the names (heap strings), 0x0041c810
   scenario `i`'s name. */
int aoe_campaign_load_ror(AoeCampaignRor *c, const char *name,
                          const AoeCampaignOpsRor *ops);
int aoe_campaign_create_ror(AoeCampaignRor *c, const char *name,
                            const char *title, int32_t count,
                            const char *const *files,
                            const char *const *names,
                            const AoeCampaignOpsRor *ops);
int aoe_campaign_write_ror(AoeCampaignRor *c, const AoeCampaignOpsRor *ops);
void aoe_campaign_free_ror(AoeCampaignRor *c);
int32_t aoe_campaign_open_ror(const AoeCampaignRor *c, int32_t i,
                              const AoeCampaignOpsRor *ops);
const char *aoe_campaign_title_ror(const AoeCampaignRor *c);
int32_t aoe_campaign_count_ror(const AoeCampaignRor *c);
int32_t aoe_campaign_names_ror(const AoeCampaignRor *c, char ***names);
const char *aoe_campaign_name_ror(const AoeCampaignRor *c, int32_t i);

/* One campaign's progress (0x114 bytes): its title, the chosen scenario,
   per-scenario states (0 locked, 1 done, 2 open) and the furthest one. */
typedef struct {
    char title[0x100];        /* +0x000 */
    int32_t current;          /* +0x100 */
    int32_t count;            /* +0x104 */
    uint8_t *states;          /* +0x108 */
    int32_t furthest;         /* +0x10c */
    AoeCampaignRor *campaign; /* +0x110 */
} AoeCampaignProgressRor;

/* 0x0043bd10 a fresh record (none: empty; else the first scenario open);
   0x0043bb40 / 0x0043be00 load / save; 0x0043be80 bind to the campaign
   of the same title (states resized to its count, the furthest and
   current clamped; 0x0043bb40 binds the same way). 0x0043c020 a title
   match; 0x0043bfe0 choose a scenario that is not locked (capped at the
   furthest); 0x0043c100 the chosen one; 0x0043c110 finish it (done, the
   next one tried and opened); 0x0043c070 the playable scenario names;
   0x0043c170 the chosen scenario's stream. */
void aoe_progress_init_ror(AoeCampaignProgressRor *p, AoeCampaignRor *c);
int aoe_progress_load_ror(AoeCampaignProgressRor *p, AoeOutcomeStream *in,
                          AoeCampaignRor **campaigns, int32_t n);
void aoe_progress_save_ror(const AoeCampaignProgressRor *p,
                           AoeOutcomeStream *out,
                           const AoeCampaignOpsRor *ops);
void aoe_progress_bind_ror(AoeCampaignProgressRor *p,
                           AoeCampaignRor **campaigns, int32_t n);
int aoe_progress_is_ror(const AoeCampaignProgressRor *p, const char *title);
int aoe_progress_select_ror(AoeCampaignProgressRor *p, int32_t i);
int32_t aoe_progress_current_ror(const AoeCampaignProgressRor *p);
void aoe_progress_complete_ror(AoeCampaignProgressRor *p);
int32_t aoe_progress_scenarios_ror(const AoeCampaignProgressRor *p,
                                   char ***names, int32_t *current);
int32_t aoe_progress_open_ror(const AoeCampaignProgressRor *p,
                              const AoeCampaignOpsRor *ops);
void aoe_progress_free_ror(AoeCampaignProgressRor *p);

/* A player (0x114 bytes): name, chosen campaign record and records. */
typedef struct {
    char name[0x100];                 /* +0x000 */
    int32_t current;                  /* +0x100 */
    int32_t count;                    /* +0x104 */
    AoeCampaignProgressRor **records; /* +0x108 */
    int32_t campaign_count;           /* +0x10c */
    AoeCampaignRor **campaigns;       /* +0x110 */
} AoePifPlayerRor;

/* 0x0043c280 new, 0x0043c190 / 0x0043c350 load / save, 0x0043c2e0 free,
   0x0043c3c0 rebind, 0x0043c410 the name (the record itself), 0x0043c420
   choose campaign `i` (its record, made when missing), 0x0043c570 the
   chosen record's campaign index, 0x0043c5d0 its chosen scenario (the
   original tests the campaign list at the record's index), 0x0043c600
   choose a scenario, 0x0043c640 the scenario names, 0x0043c670 finish,
   0x0043c690 the stream. */
void aoe_pif_player_init_ror(AoePifPlayerRor *p, const char *name,
                             AoeCampaignRor **campaigns, int32_t n);
int aoe_pif_player_load_ror(AoePifPlayerRor *p, AoeOutcomeStream *in,
                            AoeCampaignRor **campaigns, int32_t n);
void aoe_pif_player_save_ror(const AoePifPlayerRor *p, AoeOutcomeStream *out,
                             const AoeCampaignOpsRor *ops);
void aoe_pif_player_free_ror(AoePifPlayerRor *p);
void aoe_pif_player_rebind_ror(AoePifPlayerRor *p, AoeCampaignRor **campaigns,
                               int32_t n);
const char *aoe_pif_player_name_ror(const AoePifPlayerRor *p);
int aoe_pif_player_select_campaign_ror(AoePifPlayerRor *p, int32_t i);
int32_t aoe_pif_player_campaign_ror(const AoePifPlayerRor *p);
int32_t aoe_pif_player_scenario_ror(const AoePifPlayerRor *p);
int aoe_pif_player_select_scenario_ror(AoePifPlayerRor *p, int32_t i);
int32_t aoe_pif_player_scenarios_ror(const AoePifPlayerRor *p, char ***names,
                                     int32_t *current);
void aoe_pif_player_complete_ror(AoePifPlayerRor *p);
int32_t aoe_pif_player_open_ror(const AoePifPlayerRor *p,
                                const AoeCampaignOpsRor *ops);

/* The player information file. */
typedef struct {
    int32_t current;              /* +0x00 */
    int32_t count;                /* +0x04 */
    AoePifPlayerRor **players;    /* +0x08 */
    int32_t campaign_count;       /* +0x0c */
    AoeCampaignRor **campaigns;   /* +0x10 */
    char path[0x104];             /* +0x14 */
} AoePifRor;

/* 0x0043c6b0: the campaigns, then the file ("1.00", chosen player,
   players; none when missing or of another version). 0x0043c810: save
   (when a path is set) and free. 0x0043c8c0: save to `path`. 0x0043c970:
   load every "*.cpn" then "*.cpx" campaign and rebind the players.
   0x0043cc00 add a player (chosen), 0x0043cc80 choose one, 0x0043cef0
   remove one. Passed to the chosen player: 0x0043cca0 campaign,
   0x0043ccd0 scenario, 0x0043ce40 / 0x0043ce60 current scenario /
   campaign, 0x0043ced0 stream, 0x0043cea0 finish (then save).
   0x0043cd00 player names, 0x0043cd70 campaign titles (and the chosen
   one), 0x0043cdf0 scenario names, 0x0043ce80 the chosen player's name. */
int aoe_pif_load_ror(AoePifRor *f, const char *path,
                     const AoeCampaignOpsRor *ops);
void aoe_pif_free_ror(AoePifRor *f, const AoeCampaignOpsRor *ops);
int aoe_pif_save_ror(AoePifRor *f, const char *path,
                     const AoeCampaignOpsRor *ops);
void aoe_pif_find_campaigns_ror(AoePifRor *f, const AoeCampaignOpsRor *ops);
int aoe_pif_add_player_ror(AoePifRor *f, const char *name);
int aoe_pif_select_player_ror(AoePifRor *f, int32_t i);
void aoe_pif_remove_player_ror(AoePifRor *f, int32_t i);
int aoe_pif_select_campaign_ror(AoePifRor *f, int32_t i);
int aoe_pif_select_scenario_ror(AoePifRor *f, int32_t i);
int32_t aoe_pif_player_names_ror(const AoePifRor *f, char ***names,
                                 int32_t *current);
int32_t aoe_pif_campaign_names_ror(const AoePifRor *f, char ***names,
                                   int32_t *current);
int32_t aoe_pif_scenario_names_ror(const AoePifRor *f, char ***names,
                                   int32_t *current);
int32_t aoe_pif_scenario_ror(const AoePifRor *f);
int32_t aoe_pif_campaign_ror(const AoePifRor *f);
const char *aoe_pif_player_ror(const AoePifRor *f);
void aoe_pif_complete_ror(AoePifRor *f, const AoeCampaignOpsRor *ops);
int32_t aoe_pif_open_ror(const AoePifRor *f, const AoeCampaignOpsRor *ops);

#endif
