#ifndef VITA_AOE_WORLD_ROR_H
#define VITA_AOE_WORLD_ROR_H

#include <stdint.h>
#include <stdio.h>

#include "world_base.h"

/* Patched Rise of Rome 1.0a RoR world (vtable 0x0054a4f8,
   0x00509ba0..0x0050c550): the base world plus the random-map data
   (+0x104), the victory settings (+0x108 type, +0x10c relics, +0x110
   ruins, +0x114 / +0x118 the time limit) and two bytes. It replaces the
   civilisation, map, effects, player and scenario factories with the
   RoR classes, adds its fields to the saved game (gated by the saved
   version) and checks the relic, ruin and time-limit victories. */

typedef struct {
    AoeWorldBaseRor base;
    void *random_map;       /* +0x104 */
    int32_t victory;        /* +0x108: 0 standard, 2 time limit, ... */
    int32_t relics;         /* +0x10c: relics to hold (0 off) */
    int32_t ruins;          /* +0x110: ruins to hold (0 off) */
    uint8_t time_limit;     /* +0x114 */
    float time_left;        /* +0x118 */
    uint8_t field_11c;      /* +0x11c */
    uint8_t field_11d;      /* +0x11d */
    float version;          /* the loaded game's version (0x0055f804) */
    const struct AoeRorWorldOpsRor *ror;
} AoeRorWorldRor;

typedef struct AoeRorWorldOpsRor {
    void *context;
    /* Random-map data: 0x004e9fa4 from its text file, 0x004e9c60 binary
       (stream, world), 0x004ea40d save, 0x004ea348 free. */
    void *(*rm_text)(void *context, const char *name);
    void *(*rm_read)(void *context, AoeOutcomeStream *s, AoeRorWorldRor *w);
    void (*rm_write)(void *context, void *rm, AoeOutcomeStream *s);
    void (*rm_free)(void *context, void *rm);
    /* RoR classes: civilisation 0x004ef4b0 text / 0x004ef4f0 binary then
       +0x04(stream, sprites); map 0x004ed800 text (four names, three
       counts, the sounds) / 0x004ed4b0 binary; effects 0x004ebac0; +0x58
       0x004e8390(world, arg); players 0x004ef8c0 (kinds 1 and 3) and
       0x004f71c0 (kind 2) binary then +0x74, 0x004efb00 new (world, civ,
       index, name, civ ID, computer, 1, 0, 0, 0), gaia 0x004f7290;
       scenario 0x00506830 new / 0x00506900 binary (0x514c bytes; the
       tribe game's 0x00500270 allocates and reads one). */
    void *(*civ_text)(void *context, FILE *f);
    void *(*civ_read)(void *context, AoeOutcomeStream *s, void **sprites);
    void *(*map_text)(void *context, const char *a, const char *b, const char *c,
                      const char *d, int16_t x, int16_t y, int16_t z, void **sounds);
    void *(*map_read)(void *context, AoeOutcomeStream *s, void **sounds, int32_t arg);
    void *(*effects_read)(void *context, AoeOutcomeStream *s);
    void *(*field58_new)(void *context, AoeRorWorldRor *w, int32_t arg);
    void *(*player_read)(void *context, AoeOutcomeStream *s, AoeRorWorldRor *w, int16_t index,
                         int gaia);
    void *(*player_new)(void *context, AoeRorWorldRor *w, void *civ, int16_t index,
                        const char *name, uint8_t civ_id, int computer);
    void *(*gaia_new)(void *context, AoeRorWorldRor *w, void *civ);
    void *(*scenario_new)(void *context, const void *info);
    void *(*scenario_read)(void *context, AoeOutcomeStream *s, int32_t arg);
    /* Global 0x00554730 (computer players flagged as such). */
    int computer_flag;
    /* The AI log header: the time text (asctime), the scenario name
       (+0x1010, may be empty), the strings for the map size (0x2973 +
       0x00505260), map type (0x296a + 0x00505310) and victory (0x28a6 +
       0x00505370). */
    const char *(*time_text)(void *context);
    const char *(*scenario_name)(void *context);
    int32_t (*map_size)(void *context);
    int32_t (*map_type)(void *context);
    int32_t (*victory_kind)(void *context);
    void (*string)(void *context, int32_t id, char *out, int32_t n);
    /* Victory: a player's score (+0x34's 0x00454170), alive (+0x108),
       stance (0x0045da60, 0 allied), attributes (+0x50), the two
       timers (+0x848 relics, +0x844 ruins), number (+0x4a), the win
       (0x0045d460), the game event (+0x40(code, number, 0, 0, 0)), the
       base check 0x0051dd50, +0x58's 0x004e99d0. */
    int32_t (*score)(void *context, void *player);
    int (*alive)(void *context, void *player);
    int (*stance)(void *context, void *player, int32_t other);
    float (*attribute)(void *context, void *player, int index);
    void (*set_attribute)(void *context, void *player, int index, float v);
    float (*timer)(void *context, void *player, int which);
    void (*set_timer)(void *context, void *player, int which, float v);
    int16_t (*number)(void *context, void *player);
    void (*win)(void *context, void *player);
    void (*event)(void *context, int32_t code, int32_t number);
    void (*base_check)(void *context, AoeRorWorldRor *w);
    void (*call_4e99d0)(void *context, void *field58, int16_t v7c, int32_t arg);
} AoeRorWorldOpsRor;

/* The RoR vtable: the base table with the RoR overrides. */
extern AoeWorldVtblRor aoe_ror_world_vtable_ror;
void aoe_ror_world_vtable_init_ror(void);

/* 0x00509bc0 the destructor (0x00509ba0 deleting): the random-map data,
   then the base. */
void aoe_ror_world_release_ror(AoeRorWorldRor *w, FILE **ai_log);

/* +0x00 0x00509ec0: the base text lists, then "%s " the random-map
   file; the victory fields cleared. +0x58 0x0050a210: the base data,
   then the random-map data and the same clearing. +0xe0 0x0050a2d0:
   the base data saved, then the random-map data. +0xb0 0x0050a2b0 =
   the base data file. */
int aoe_ror_world_load_text_ror(AoeWorldBaseRor *w, FILE *f);
void aoe_ror_world_load_data_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t a,
                                 int32_t b);
void aoe_ror_world_save_data_ror(const AoeWorldBaseRor *w, AoeOutcomeStream *s);
int aoe_ror_world_load_data_file_ror(AoeWorldBaseRor *w, const char *path, int32_t a,
                                     int32_t b);
/* +0x1c 0x00509d90 / +0x2c 0x00509f60: flag 1 makes a RoR
   civilisation, else the base. +0x24 0x00509e20 / +0x48 0x0050a020 the
   RoR map; +0x4c 0x0050a1b0 the RoR effects; +0x54 0x0050a090 +0x58 =
   0x004e8390(world, b). */
void aoe_ror_world_civ_from_text_ror(AoeWorldBaseRor *w, int16_t id, int16_t flag, FILE *f);
int aoe_ror_world_civ_from_stream_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, int16_t id,
                                      uint8_t flag);
void aoe_ror_world_map_from_text_ror(AoeWorldBaseRor *w, const char *a, const char *b,
                                     const char *c, const char *d, int16_t x, int16_t y,
                                     int16_t z, void **sounds, const char *extra);
void aoe_ror_world_map_from_stream_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t arg);
void aoe_ror_world_effects_from_stream_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s);
void aoe_ror_world_slot_54_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t arg);
/* +0x34 0x0050a380: kinds 1 and 3 a RoR player, 2 the RoR gaia, else
   the base; then its +0x74 (`load_74`). */
void aoe_ror_world_player_from_stream_ror(AoeRorWorldRor *w, AoeOutcomeStream *s, uint8_t kind,
                                          int16_t index,
                                          void (*load_74)(void *context, void *player,
                                                          AoeOutcomeStream *s));
/* +0x5c 0x0050a470: the RoR gaia. +0x60 0x0050a4f0: players of type 0
   (and type 4, flagged as computers when 0x00554730 is set). +0x6c
   0x0050a830: a player by kind (4 computer). */
void aoe_ror_world_new_gaia_ror(AoeRorWorldRor *w);
void aoe_ror_world_new_players_ror(AoeRorWorldRor *w, const int8_t type[9], const int8_t civ[9],
                                   const char names[8][0x41]);
void aoe_ror_world_new_player_ror(AoeRorWorldRor *w, int16_t index, uint8_t civ, uint8_t kind,
                                  const char *name);
/* +0xf0 0x0050a150 / +0xf4 0x0050a0f0: the RoR scenario. */
void aoe_ror_world_scenario_new_ror(AoeRorWorldRor *w, const void *info);
void aoe_ror_world_scenario_from_stream_ror(AoeRorWorldRor *w, AoeOutcomeStream *s,
                                            int32_t arg);
/* +0x68 0x0050a2f0 / +0xc8 0x0050a960: the victory fields after the base
   state (`save_base` / `load_base`): +0x108 from 5.6, +0x10c and +0x110
   from 5.3, +0x114 and +0x118 from 5.4, +0x11c from 6.6, +0x11d from
   6.97 (else 2); older saves zero them. */
void aoe_ror_world_save_state_ror(AoeRorWorldRor *w, AoeOutcomeStream *s);
int aoe_ror_world_load_state_ror(AoeRorWorldRor *w, AoeOutcomeStream *s, int loaded_base);
/* +0x9c 0x00509c30: the AI log header (on the first status only). */
void aoe_ror_world_ai_log_header_ror(AoeRorWorldRor *w, FILE *f, int first);
/* +0xd4 0x0050be80: the editor game (`base_ok` from 0x0051d640), then
   every player starts with food 200, wood 200, stone 100, gold 0. */
void aoe_ror_world_editor_resources_ror(AoeRorWorldRor *w);
/* +0xc4 0x0050bf00: with the game running: the time limit (type 2,
   counting down by twice the step) gives the win to the best team
   average score (0x67); held relics (attribute 7) and ruins
   (attribute 14) by a mutually allied team start a 2000 timer (events
   0x72 / 0x74, the team's attribute 55 / 54 set) that counts down to a
   win (0x65 / 0x66); losing them clears it (0x73 / 0x75). Then the base
   check. */
void aoe_ror_world_check_victory_ror(AoeRorWorldRor *w);
/* 0x0050c550: +0x58's 0x004e99d0(+0x7c, v). */
void aoe_ror_world_call_4e99d0_ror(AoeRorWorldRor *w, int32_t v);

#endif
