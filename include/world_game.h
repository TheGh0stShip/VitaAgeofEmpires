#ifndef VITA_AOE_WORLD_GAME_H
#define VITA_AOE_WORLD_GAME_H

#include <stdint.h>
#include <stdio.h>

#include "world_base.h"

/* Patched Rise of Rome 1.0a base world, the game side (0x0051cb90..
   0x0051e990): starting a game (new map, scenario or campaign), the
   world update, the player colours, the saved game state and file
   ("VER 8.6", version 7.24) and the scenario file ("1.11"). The game
   settings, the comm object, the players' methods, the random numbers
   and the files are ops. */

/* The new-game description (offsets in the original block). */
typedef struct {
    char names[8][0x41];       /* +0x000: player i's name at (i - 1) * 0x41 */
    int8_t type[9];            /* +0x248 + i: -1 none, 0 computer, 4 ... */
    int8_t civ[9];             /* +0x251 + i */
    int16_t color[9];          /* +0x25a + 2 i (players 1..8) */
    int16_t player_count;      /* +0x294 */
    const char *scenario;      /* +0x298: a scenario file, or NULL */
    const uint8_t *map_info;   /* +0x29c */
    int16_t field_2a0;         /* +0x2a0 */
    int16_t map_w, map_h;      /* +0x2a2, +0x2a4 */
    uint8_t campaign;          /* +0x2a8 */
} AoeWorldGameInfoRor;

/* What the scenario writer reads of an object. */
typedef struct {
    uint8_t type;              /* template +0x04 (25 is skipped) */
    float x, y, z;             /* +0x38..+0x40 */
    int32_t id;                /* +0x04 */
    int16_t template_id;       /* template +0x10 */
    uint8_t state;             /* +0x48 */
    float angle;               /* +0x9c (types 0x1f..0x59) */
    uint8_t facing;            /* +0x35 (the rest, as a float) */
} AoeWorldSceneObjectRor;

typedef struct {
    void *context;
    /* Clock and random numbers (0x00526053 srand, 0x0052605d rand). */
    uint32_t (*now_ms)(void *context);
    void (*srand)(void *context, int32_t seed);
    int32_t (*rand)(void *context);
    /* The comm object (+0x58 +0x14): 0x0041ebe0 the turn's elapsed time
       (0 nothing to do, -1 free running), 0x0041ebc0 the seed recorded,
       0x00423be0 its seed, 0x00423d10 a slot's player type; +0x58's
       0x0042a140; the scenario's 0x00476270; the performance counter
       0x00417370(id, ms); the rand log 0x0042c330. */
    int32_t (*comm_turn)(void *context, int32_t game_time);
    void (*comm_seed)(void *context, int32_t seed);
    int32_t (*comm_new_seed)(void *context);
    int32_t (*slot_type)(void *context, int32_t slot);
    void (*comm_42a140)(void *context);
    void (*scenario_476270)(void *context);
    void (*perf)(void *context, int32_t id, uint32_t ms);
    void (*log)(void *context, const char *text);
    /* Game settings: 0x0041b760 session, 0x0041b750 single player,
       0x0041b710 / 0x0041b720 flags, 0x0041b9b0 a slot's team,
       0x0041c080 a slot's player, 0x0041c170 the speed, game +0x28 the
       fixed seed (-1 none), game +0x30 the used seed, 0x0041b990 +0x100's
       default, 0x0041ba50 / 0x00417a60 the campaign restored, 0x00417a20
       the campaign read, global 0x00580de0 (the computer-team rule),
       0x00580de8 (AI logging), 0x00582edc (0x0044a850 when set). */
    int (*session)(void *context);
    int (*single_player)(void *context);
    int (*flag_710)(void *context);
    int (*flag_720)(void *context);
    int32_t (*team)(void *context, int32_t slot);
    int32_t (*slot_player)(void *context, int32_t slot);
    float (*speed)(void *context);
    int32_t (*fixed_seed)(void *context);
    void (*set_used_seed)(void *context, int32_t seed);
    int32_t (*default_100)(void *context);
    void (*campaign_restore)(void *context, int had, int32_t campaign, int32_t player,
                             int32_t scenario);
    void (*campaign_read)(void *context, int32_t *campaign, int32_t *player, int32_t *scenario);
    int32_t computer_team_rule;
    int ai_logging;
    int (*hook_582edc)(void *context);
    /* Players: +0x7c update, +0x58's 0x0045eb40, +0x30's 0x00451270,
       +0x40's 0x00436a20, +0x90 start, +0x2c(name, name, name, 0, v),
       0x0045d400 colour, 0x0045da70 stance and 0x0045da40, +0x84 / +0x88
       / +0x8c save, 0x0045ce30 load, +0x64 / +0x6c scenario save, +0xec
       the AI log, +0x80 won, its objects (lists +0x28 and +0x2c), a
       colour table's +0x24. */
    void (*player_update)(void *context, void *player);
    void (*player_eb40)(void *context, void *player);
    void (*player_451270)(void *context, void *player);
    void (*player_436a20)(void *context, void *player);
    void (*player_start)(void *context, void *player);
    void (*player_2c)(void *context, void *player, const char *text, int16_t v);
    void (*player_color)(void *context, void *player, int32_t color);
    void (*player_stance)(void *context, void *player, int32_t other, int32_t stance);
    void (*player_da40)(void *context, void *player, int32_t v);
    void (*player_save)(void *context, void *player, int slot, AoeOutcomeStream *s);
    void (*player_load_body)(void *context, void *player, AoeOutcomeStream *s);
    void (*player_ai_log)(void *context, void *player, FILE *f, int first);
    int (*player_won)(void *context, void *player);
    int32_t (*player_objects)(void *context, void *player, int list);
    void (*player_object)(void *context, void *player, int list, int32_t i,
                          AoeWorldSceneObjectRor *out);
    /* The map: 0x004445e0 new (w, h), 0x004448f0 from the map info, 0x00446640
       / 0x004468a0 game load / save, 0x00444bd0 scenario save; 0x005209c0;
       0x00520c60 and 0x00518000(on) around the bodies; the scenario's +0x0c
       save; 0x00417890 the scenario info header; 0x0041bc60 the campaign's
       scenario stream; the pathfinders' reset 0x00458850. */
    void (*map_new)(void *context, int16_t w, int16_t h);
    void (*map_from_info)(void *context, uint8_t first, int32_t v);
    void (*map_load)(void *context, AoeOutcomeStream *s);
    void (*map_save)(void *context, AoeOutcomeStream *s);
    void (*map_scenario_save)(void *context, AoeOutcomeStream *s);
    void (*call_5209c0)(void *context);
    void (*call_520c60)(void *context);
    void (*call_518000)(void *context, int on);
    void (*scenario_save)(void *context, void *scenario, AoeOutcomeStream *s);
    void (*scenario_info_header)(void *context, AoeOutcomeStream *s);
    AoeOutcomeStream *(*campaign_stream)(void *context);
    void (*reset_paths)(void *context);
    /* Files: the saved-game and scenario directories (game +0xc +0xb12 /
       +0xc17), open/close, remove, the AI log name (0x0043ce80) and a
       string (game +0x20). */
    const char *save_dir;
    const char *scenario_dir;
    AoeOutcomeStream *(*open_read)(void *context, const char *path);
    void (*close_read)(void *context, AoeOutcomeStream *s);
    AoeOutcomeStream *(*open_write)(void *context, const char *path);
    int (*close_write)(void *context, AoeOutcomeStream *s); /* 1 on a write error */
    void (*remove_file)(void *context, const char *path);
    const char *(*ai_log_name)(void *context);
    FILE *(*open_append)(void *context, const char *path);
    void (*string)(void *context, int32_t id, char *out, int32_t n);
} AoeWorldGameOpsRor;

/* The game-side overridable methods (slots noted). */
typedef struct {
    void (*new_gaia)(AoeWorldBaseRor *w);                                     /* +0x5c */
    void (*new_players)(AoeWorldBaseRor *w, const AoeWorldGameInfoRor *info); /* +0x60 */
    int (*random_map_game)(AoeWorldBaseRor *w, const AoeWorldGameInfoRor *info); /* +0x64 */
    void (*save_state)(AoeWorldBaseRor *w, AoeOutcomeStream *s);              /* +0x68 */
    int (*campaign_game)(AoeWorldBaseRor *w, const AoeWorldGameInfoRor *info);   /* +0x74 */
    int (*scenario_game)(AoeWorldBaseRor *w, const char *name,
                         const AoeWorldGameInfoRor *info);                     /* +0x78 */
    /* +0x7c..+0x98: the scenario body for a version (0 "1.01", 1 "1.02"
       and "1.03", 2 "1.04" .. 6 "1.08", 7 "1.09".."1.12"). */
    void (*scenario_body)(AoeWorldBaseRor *w, int which, AoeOutcomeStream *s,
                          const AoeWorldGameInfoRor *info);
    void (*ai_log_header)(AoeWorldBaseRor *w, FILE *f, int first);           /* +0x9c */
    void (*assign_colors)(AoeWorldBaseRor *w, const AoeWorldGameInfoRor *info); /* +0xa8 */
    void (*player_from_stream)(AoeWorldBaseRor *w, AoeOutcomeStream *s, uint8_t flag,
                               int16_t index);                                /* +0x34 */
    void (*check_victory)(AoeWorldBaseRor *w);                                /* +0xc4 */
    int (*load_state)(AoeWorldBaseRor *w, AoeOutcomeStream *s);               /* +0xc8 */
    void (*scenario_from_stream)(AoeWorldBaseRor *w, AoeOutcomeStream *s);    /* +0xf4 */
    void (*scenario_new)(AoeWorldBaseRor *w, const AoeWorldGameInfoRor *info);   /* +0xf0 */
    void (*map_generate)(AoeWorldBaseRor *w, const AoeWorldGameInfoRor *info,
                         int32_t last_player);                                /* map +0x28 */
} AoeWorldGameVtblRor;

typedef struct {
    AoeWorldBaseRor *w;
    const AoeWorldGameOpsRor *ops;
    const AoeWorldGameVtblRor *vtbl;
    float version;          /* 0x0055f804: the saved game's version */
    int32_t path_flags[13]; /* 0x007d2464 */
    FILE *ai_log;           /* 0x007d249c */
    int32_t ai_log_time;    /* 0x007d24a0 */
    int16_t field_7d2498;   /* 0x007d2498 */
} AoeWorldGameRor;

/* +0xbc 0x0051d8f0: one update. The comm turn decides the elapsed time
   (-1: the real time since the last update, at most 100 ms, nothing on
   the first); game time += elapsed x speed (truncated; a set +0x1c skips
   a free-running turn once). On a change: +0x84 = step / 1000, each
   player's update time and +0x7c and +0x58, the hook, 0x00451270 and
   0x00436a20 per player under a saved random state, the next update
   slot cleared and +0xc4. Then a new seed is recorded and, with AI
   logging, a status every 30 s ("AILog\<name>.log") and the result at
   game over. Returns the state; nothing is done (step 0) without a
   turn. */
uint8_t aoe_world_update_ror(AoeWorldGameRor *g);

/* +0xa8 0x0051d060: the colours: gaia 7; players 1.. (up to the
   description's count) their chosen colour when free; the rest the next
   free colour table with +0x24 = 1. +0xa4 0x0051d1a0 the same without
   choices. */
void aoe_world_assign_colors_ror(AoeWorldGameRor *g, const AoeWorldGameInfoRor *info);
void aoe_world_assign_default_colors_ror(AoeWorldGameRor *g);
/* +0x60 0x0051cfa0: players 1.. of the description that are present
   (type != -1): name and civilisation (0x0045b6a0 through `player_new`). */
void aoe_world_new_players_ror(AoeWorldGameRor *g, const AoeWorldGameInfoRor *info,
                               void *(*player_new)(void *context, AoeWorldBaseRor *w, void *civ,
                                                   int16_t index, const char *name,
                                                   uint8_t civ_id, int flag));

/* +0x68 0x0051dee0 save / +0xc8 0x0051cb90 load the game state (the
   version 7.24 set on save; read fields gated by the loaded version:
   the campaign >= 7.24, +0x100 >= 7.16). */
void aoe_world_save_state_ror(AoeWorldGameRor *g, AoeOutcomeStream *s);
int aoe_world_load_state_ror(AoeWorldGameRor *g, AoeOutcomeStream *s);
/* +0xd8 0x0051e0c0 / +0xcc 0x0051ce30: the saved game file
   "<save dir><name>": "VER 8.6", the version, the state. A failed write
   removes the file (0); a load resets first and needs the header. */
int aoe_world_save_game_ror(AoeWorldGameRor *g, const char *name);
int aoe_world_load_game_ror(AoeWorldGameRor *g, const char *name);

/* +0xd0 0x0051d210: a new game (state 3 while it starts, 0 when it
   did); +0xd4 0x0051d640: a scenario-editor game; +0x64 0x0051d760: a
   random map game (the map generated, three player updates). */
int aoe_world_new_game_ror(AoeWorldGameRor *g, const AoeWorldGameInfoRor *info, int16_t v7c);
int aoe_world_editor_game_ror(AoeWorldGameRor *g, const AoeWorldGameInfoRor *info,
                              int32_t seed);
int aoe_world_random_map_game_ror(AoeWorldGameRor *g, const AoeWorldGameInfoRor *info);
/* +0x78 0x0051e6d0: a scenario file "<scenario dir><name>" by its
   version tag; +0x74 0x0051e990: the campaign's scenario (then the
   campaign position read). Unknown versions close and fail. */
int aoe_world_scenario_game_ror(AoeWorldGameRor *g, const char *name,
                                const AoeWorldGameInfoRor *info);
int aoe_world_campaign_game_ror(AoeWorldGameRor *g, const AoeWorldGameInfoRor *info);
/* +0xe4 0x0051e330: save the scenario "<scenario dir><name>" (the old
   "<base>.scn" removed first): "1.11", the info header, +0x68, the
   scenario, the map, the player count, players' +0x64, each player's
   objects (type 25 skipped: x, y, z, ID, template, state, angle or
   facing), the count again and players' +0x6c. */
int aoe_world_save_scenario_ror(AoeWorldGameRor *g, const char *name);

#endif
