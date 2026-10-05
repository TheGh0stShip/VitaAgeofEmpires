#ifndef VITA_AOE_SCENARIO_OBJECT_H
#define VITA_AOE_SCENARIO_OBJECT_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a scenario object's settings (vtable 0x00545e3c,
   0x00475800..0x00476250): the player names, the scenario texts, the
   per-player AI, city-plan and personality files (by name or embedded
   data) and three per-player values. Semantic storage; offsets noted. */

#define AOE_SCENARIO_PLAYERS_ROR 16

typedef struct {
    char player_name[AOE_SCENARIO_PLAYERS_ROR][256];   /* +0x0d */
    char *description;                                 /* +0x1010 */
    char *field_1014;                                  /* +0x1014 */
    char ai_path[0xe0], city_path[0xe0], per_path[0xe0]; /* +0x1028, +0x1108, +0x11e8 */
    char *ai_name[AOE_SCENARIO_PLAYERS_ROR];           /* +0x12c8 */
    char *city_name[AOE_SCENARIO_PLAYERS_ROR];         /* +0x1308 */
    char *per_name[AOE_SCENARIO_PLAYERS_ROR];          /* +0x1348 */
    /* Embedded files and their sizes. */
    const void *ai_data[AOE_SCENARIO_PLAYERS_ROR];     /* +0x1388 */
    const void *city_data[AOE_SCENARIO_PLAYERS_ROR];   /* +0x13c8 */
    const void *per_data[AOE_SCENARIO_PLAYERS_ROR];    /* +0x1408 */
    int32_t ai_size[AOE_SCENARIO_PLAYERS_ROR];         /* +0x1448 */
    int32_t city_size[AOE_SCENARIO_PLAYERS_ROR];       /* +0x1488 */
    int32_t per_size[AOE_SCENARIO_PLAYERS_ROR];        /* +0x14c8 */
    char text_1508[0xe0], text_15e8[0xe0], text_16c8[0xe0], text_17a8[0xe0];
    int32_t player_kind[9];                            /* +0x188c */
    int32_t value_18cc[AOE_SCENARIO_PLAYERS_ROR];      /* +0x18cc */
    int32_t value_190c[AOE_SCENARIO_PLAYERS_ROR];      /* +0x190c */
    int32_t value_194c[AOE_SCENARIO_PLAYERS_ROR];      /* +0x194c */
} AoeScenarioObjectRor;

/* 0x00475800 description (NULL empties it), 0x00475840 / 0x00475880 /
   0x004758c0 / 0x00475900 the four texts (strcpy, unbounded as the
   original), 0x00475af0 a player's name. 0x00475940 / 0x004759b0 /
   0x00475a20 a player's AI / city-plan / personality name; with `strip`
   1 its last four characters are cut (a ".cty" / ".per" extension; one
   more than ".ai"). */
void aoe_scenario_set_description_ror(AoeScenarioObjectRor *s, const char *text);
void aoe_scenario_set_text_ror(AoeScenarioObjectRor *s, int which, const char *text);
void aoe_scenario_set_player_name_ror(AoeScenarioObjectRor *s, int32_t player,
                                      const char *name);
void aoe_scenario_set_file_name_ror(char *dest, const char *name, int strip);
/* 0x00475a90 / 0x00475ab0 / 0x00475ad0 setters and 0x00476210 /
   0x00476230 / 0x00476250 getters of the three per-player values (0..15;
   others ignored / 0). */
void aoe_scenario_set_value_ror(int32_t table[AOE_SCENARIO_PLAYERS_ROR], int32_t player,
                                int32_t value);
int32_t aoe_scenario_value_ror(const int32_t table[AOE_SCENARIO_PLAYERS_ROR],
                               int32_t player);
/* 0x00475b30 / 0x00475b40 the two string pointers; 0x00475b50 /
   0x00475b60 / 0x00475b70 / 0x00475b80 the texts; 0x00476200 a player's
   name. */
const char *aoe_scenario_text_ror(const AoeScenarioObjectRor *s, int which);
const char *aoe_scenario_player_name_ror(const AoeScenarioObjectRor *s, int32_t player);
/* 0x00476190: player i's kind 1..5 (others and out of 0..8: 0). */
int32_t aoe_scenario_player_kind_ror(const AoeScenarioObjectRor *s, int32_t player);
/* 0x00475f80: the scenario version written, 1.15. */
float aoe_scenario_version_ror(void);

/* 0x00475b90 / 0x00475d80 / 0x00475f90: the AI / city-plan / personality
   file for a player. In the scenario editor (state 7) the stored name.
   Otherwise, with `use_files` 1: embedded data is written to
   "<temp>temp_buildlist%d.tmp" / "temp_CityPlan%d.tmp" /
   "temp_AiRules%d.tmp" (temp path, "." when none) and that path
   returned; else a name other than "Random" (10107) and " <None> "
   (10101) becomes "<ai dir>%s.ai" / ".cty" / ".per" (the AI dir is the
   start-up config's +0x102b). In both other cases the plain name. As in
   the original, all three test the AI name against the two strings. */
typedef struct {
    void *context;
    int32_t game_state;                                   /* +0x1b0 */
    void (*string)(void *context, int32_t id, char *out, int size);
    int (*temp_path)(void *context, char *out, int size);
    int (*write_file)(void *context, const char *path, const void *data, int32_t size);
    const char *ai_dir;
} AoeScenarioFileOpsRor;
enum { AOE_SCENARIO_FILE_AI, AOE_SCENARIO_FILE_CITY, AOE_SCENARIO_FILE_PER };
const char *aoe_scenario_player_file_ror(AoeScenarioObjectRor *s, int kind, int32_t player,
                                         int use_files, const AoeScenarioFileOpsRor *ops);

#endif
