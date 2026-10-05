#ifndef VITA_AOE_GAME_SETTINGS_H
#define VITA_AOE_GAME_SETTINGS_H

#include <stddef.h>
#include <stdint.h>

/* Patched Rise of Rome 1.0a game-object settings (game 0x00580e38): the
   base game's block at +0x8f8..+0xa14 (accessors 0x0041b6f0..0x0041c1a0)
   and the tribe game's block at +0xa7c..+0x1194 (0x00505260..0x005055c0).
   Fields without a name in the executable's own text are named by
   offset. Each accessor below is one original function. */
typedef struct {
    /* Base game. */
    float speed;                 /* +0x8f8 */
    uint8_t flag_8fc;            /* +0x8fc */
    char scenario_name[129];     /* +0x8fd, 128 chars, +0x97c always 0 */
    uint8_t flag_97d;            /* +0x97d, cleared when +0x97e is set */
    uint8_t flag_97e;            /* +0x97e, cleared when +0x97d is set */
    uint8_t map_width;           /* +0x97f */
    uint8_t map_height;          /* +0x980 */
    uint8_t max_players;         /* +0x981 */
    uint8_t byte_982, byte_983, byte_984, byte_985, byte_986, byte_987,
        byte_988, byte_989;      /* +0x982..+0x989 */
    /* +0x98a: per player, bit 0 a flag and bits 1..7 a game-version index
       ("1.0".."1.4c", 0x0041b840). */
    uint8_t player_98a[9];
    uint8_t byte_993;            /* +0x993 */
    uint8_t player_994[9];       /* +0x994 */
    int32_t int_9a0, int_9a4;    /* +0x9a0 / +0x9a4 */
    int32_t player_9b0[10];      /* +0x9b0 */
    /* +0x9d8: per player, the wonder countdown (-1 none, 0x0041c0b0 /
       0x0041c0d0). */
    int32_t wonder_years[10];
    float float_a0c;             /* +0xa0c */
    int32_t int_a10;             /* +0xa10 */
    uint8_t byte_a14;            /* +0xa14 */
    /* Tribe game. */
    int32_t map_size;            /* +0xa7c (0..5) */
    int32_t int_a80, int_a84, int_a88, int_a8c, int_a90;
    uint8_t byte_a94[12];        /* +0xa94 */
    int32_t int_aa0[9];          /* +0xaa0 */
    uint8_t byte_ac4[9];         /* +0xac4 */
    uint8_t byte_acd[9];         /* +0xacd */
    uint8_t option_ad6, option_ad7, option_ad8, option_ad9;
    int32_t resource_level;      /* +0xadc */
    int32_t starting_age;        /* +0xae0 */
    uint8_t option_ae4, option_ae5;
    uint8_t population_ae6;      /* +0xae6 */
    uint8_t byte_1190;           /* +0x1190 */
    int32_t int_1194;            /* +0x1194 */
} AoeGameSettingsRor;

/* ---- base game (+0x8f8 ..) ---- */
float aoe_settings_speed_ror(const AoeGameSettingsRor *g);           /* 0x0041b6f0 */
void aoe_settings_set_speed_ror(AoeGameSettingsRor *g, float v);     /* 0x0041ba10 */
uint8_t aoe_settings_flag_8fc_ror(const AoeGameSettingsRor *g);      /* 0x0041b700 */
int aoe_settings_flag_8fc_clear_ror(const AoeGameSettingsRor *g);    /* 0x0041b710 */
void aoe_settings_set_flag_8fc_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x0041ba00 */
const char *aoe_settings_scenario_name_ror(const AoeGameSettingsRor *g); /* 0x0041b740 */
/* 0x0041ba20: strncpy 128, then +0x97c = 0. */
void aoe_settings_set_scenario_name_ror(AoeGameSettingsRor *g,
                                        const char *name);
uint8_t aoe_settings_flag_97d_ror(const AoeGameSettingsRor *g);      /* 0x0041b750 */
uint8_t aoe_settings_flag_97e_ror(const AoeGameSettingsRor *g);      /* 0x0041b760 */
void aoe_settings_set_flag_97d_ror(AoeGameSettingsRor *g, int v);    /* 0x0041ba70 */
void aoe_settings_set_flag_97e_ror(AoeGameSettingsRor *g, int v);    /* 0x0041ba90 */
uint8_t aoe_settings_map_width_ror(const AoeGameSettingsRor *g);     /* 0x0041b770 */
uint8_t aoe_settings_map_height_ror(const AoeGameSettingsRor *g);    /* 0x0041b780 */
uint8_t aoe_settings_max_players_ror(const AoeGameSettingsRor *g);   /* 0x0041b790 */
void aoe_settings_set_map_ror(AoeGameSettingsRor *g, uint8_t w,
                              uint8_t h, uint8_t players);           /* 0x0041bab0 */
uint8_t aoe_settings_byte_982_ror(const AoeGameSettingsRor *g);      /* 0x0041b7a0 */
uint8_t aoe_settings_byte_983_ror(const AoeGameSettingsRor *g);      /* 0x0041b9d0 */
uint8_t aoe_settings_byte_984_ror(const AoeGameSettingsRor *g);      /* 0x0041b7b0 */
uint8_t aoe_settings_byte_985_ror(const AoeGameSettingsRor *g);      /* 0x0041b7c0 */
uint8_t aoe_settings_byte_986_ror(const AoeGameSettingsRor *g);      /* 0x0041b7d0 */
uint8_t aoe_settings_byte_987_ror(const AoeGameSettingsRor *g);      /* 0x0041b7e0 */
uint8_t aoe_settings_byte_988_ror(const AoeGameSettingsRor *g);      /* 0x0041b9a0 */
uint8_t aoe_settings_byte_989_ror(const AoeGameSettingsRor *g);      /* 0x0041b7f0 */
void aoe_settings_set_byte_982_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x0041bae0 */
void aoe_settings_set_byte_983_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x0041bc00 */
void aoe_settings_set_byte_984_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x0041baf0 */
void aoe_settings_set_byte_985_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x0041bb00 */
void aoe_settings_set_byte_986_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x0041bb10 */
void aoe_settings_set_byte_987_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x0041bb20 */
void aoe_settings_set_byte_988_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x0041bb30 */
void aoe_settings_set_byte_989_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x0041bb40 */
uint8_t aoe_settings_player_98a_ror(const AoeGameSettingsRor *g, int i); /* 0x0041b980 */
void aoe_settings_set_player_98a_ror(AoeGameSettingsRor *g, int i,
                                     uint8_t v);                     /* 0x0041bba0 */
uint8_t aoe_settings_player_flag_ror(const AoeGameSettingsRor *g, int i); /* 0x0041b800 */
uint8_t aoe_settings_player_version_ror(const AoeGameSettingsRor *g,
                                        int i);                      /* 0x0041b820 */
/* 0x0041b840: the version text of a player's index ("" for 0, "1.0" ..
   "1.4c" for 1..22, "?" otherwise). */
const char *aoe_settings_player_version_text_ror(const AoeGameSettingsRor *g,
                                                 int i);
void aoe_settings_set_player_flag_ror(AoeGameSettingsRor *g, int i,
                                      uint8_t bit);                  /* 0x0041bb50 */
void aoe_settings_set_player_version_ror(AoeGameSettingsRor *g, int i,
                                         uint8_t version);           /* 0x0041bb70 */
uint8_t aoe_settings_byte_993_ror(const AoeGameSettingsRor *g);      /* 0x0041b990 */
void aoe_settings_set_byte_993_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x0041bbc0 */
uint8_t aoe_settings_player_994_ror(const AoeGameSettingsRor *g, int i); /* 0x0041b9b0 */
void aoe_settings_set_player_994_ror(AoeGameSettingsRor *g, int i,
                                     uint8_t v);                     /* 0x0041bbd0 */
int32_t aoe_settings_int_9a0_ror(const AoeGameSettingsRor *g);       /* 0x0041b720 */
int32_t aoe_settings_int_9a4_ror(const AoeGameSettingsRor *g);       /* 0x0041b730 */
void aoe_settings_set_int_9a0_ror(AoeGameSettingsRor *g, int32_t v); /* 0x0041ba50 */
void aoe_settings_set_int_9a4_ror(AoeGameSettingsRor *g, int32_t v); /* 0x0041ba60 */
void aoe_settings_set_player_9b0_ror(AoeGameSettingsRor *g, int i,
                                     int32_t v);                     /* 0x0041c090 */
void aoe_settings_clear_wonder_ror(AoeGameSettingsRor *g, int player); /* 0x0041c0b0 */
/* 0x0041c0d0: keeps the lower countdown; an unset (negative) slot takes
   the new value. */
void aoe_settings_set_wonder_ror(AoeGameSettingsRor *g, int player,
                                 int32_t years);
void aoe_settings_wonder_ror(const AoeGameSettingsRor *g, int player,
                             int32_t *years);                        /* 0x0041c100 */
float aoe_settings_float_a0c_ror(const AoeGameSettingsRor *g);       /* 0x0041c170 */
void aoe_settings_set_float_a0c_ror(AoeGameSettingsRor *g, float v); /* 0x0041c180 */
int32_t aoe_settings_int_a10_ror(const AoeGameSettingsRor *g);       /* 0x0041c190 */
void aoe_settings_set_int_a10_ror(AoeGameSettingsRor *g, int32_t v); /* 0x0041c1a0 */
uint8_t aoe_settings_byte_a14_ror(const AoeGameSettingsRor *g);      /* 0x0041b9c0 */
void aoe_settings_set_byte_a14_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x0041bbf0 */
/* 0x0041b9e0 / 0x005055d0: a static empty string (its byte zeroed). */
const char *aoe_settings_empty_text_ror(void);

/* The 0xa8-byte settings record 0x0041b5c0 builds and 0x0041b490 applies
   (little-endian): +0x00 speed, +0x04 +0x8fc, +0x05 name[128], +0x85
   +0x97d, +0x86 +0x97e, +0x87..+0x89 map width/height/players, +0x8a
   +0x982, +0x8b +0x983, +0x8c..+0x91 +0x984..+0x989, +0x92[9] +0x98a,
   +0x9b +0x993, +0x9c[9] +0x994; the rest unused. Applying it uses the
   setters, so +0x97e (applied second) decides the +0x97d/+0x97e pair. */
enum { AOE_SETTINGS_RECORD_SIZE = 0xa8 };
void aoe_settings_pack_ror(const AoeGameSettingsRor *g,
                           uint8_t out[AOE_SETTINGS_RECORD_SIZE]);
void aoe_settings_apply_ror(AoeGameSettingsRor *g,
                            const uint8_t in[AOE_SETTINGS_RECORD_SIZE]);

/* ---- tribe game (+0xa7c ..) ---- */
int32_t aoe_settings_map_size_ror(const AoeGameSettingsRor *g);      /* 0x00505260 */
/* 0x00505270: stores the size and, for 0..5, the map 72/96/120/144/200/
   250 tiles square with 8 players. */
void aoe_settings_set_map_size_ror(AoeGameSettingsRor *g, int32_t size);
int32_t aoe_settings_int_a80_ror(const AoeGameSettingsRor *g);       /* 0x00505310 */
void aoe_settings_set_int_a80_ror(AoeGameSettingsRor *g, int32_t v); /* 0x00505320 */
int32_t aoe_settings_int_a84_ror(const AoeGameSettingsRor *g);       /* 0x00505330 */
void aoe_settings_set_int_a84_ror(AoeGameSettingsRor *g, int32_t v); /* 0x00505340 */
int32_t aoe_settings_int_a88_ror(const AoeGameSettingsRor *g);       /* 0x00505350 */
void aoe_settings_set_int_a88_ror(AoeGameSettingsRor *g, int32_t v); /* 0x00505360 */
int32_t aoe_settings_int_a8c_ror(const AoeGameSettingsRor *g);       /* 0x00505370 */
int32_t aoe_settings_int_a90_ror(const AoeGameSettingsRor *g);       /* 0x00505380 */
void aoe_settings_set_a8c_a90_ror(AoeGameSettingsRor *g, int32_t a,
                                  int32_t b);                        /* 0x00505390 */
uint8_t aoe_settings_byte_a94_ror(const AoeGameSettingsRor *g, int i); /* 0x005053b0 */
void aoe_settings_set_byte_a94_ror(AoeGameSettingsRor *g, int i,
                                   uint8_t v);                       /* 0x005053c0 */
int32_t aoe_settings_int_aa0_ror(const AoeGameSettingsRor *g, int i); /* 0x005053e0 */
void aoe_settings_set_int_aa0_ror(AoeGameSettingsRor *g, int i,
                                  int32_t v);                        /* 0x005053f0 */
uint8_t aoe_settings_byte_ac4_ror(const AoeGameSettingsRor *g, int i); /* 0x00505410 */
void aoe_settings_set_byte_ac4_ror(AoeGameSettingsRor *g, int i,
                                   uint8_t v);                       /* 0x00505420 */
uint8_t aoe_settings_byte_acd_ror(const AoeGameSettingsRor *g, int i); /* 0x00505440 */
void aoe_settings_set_byte_acd_ror(AoeGameSettingsRor *g, int i,
                                   uint8_t v);                       /* 0x00505450 */
uint8_t aoe_settings_option_ad6_ror(const AoeGameSettingsRor *g);    /* 0x00505470 */
uint8_t aoe_settings_option_ad7_ror(const AoeGameSettingsRor *g);    /* 0x00505480 */
uint8_t aoe_settings_option_ad8_ror(const AoeGameSettingsRor *g);    /* 0x00505490 */
uint8_t aoe_settings_option_ad9_ror(const AoeGameSettingsRor *g);    /* 0x005054a0 */
int32_t aoe_settings_resource_level_ror(const AoeGameSettingsRor *g); /* 0x005054b0 */
int32_t aoe_settings_starting_age_ror(const AoeGameSettingsRor *g);  /* 0x005054c0 */
uint8_t aoe_settings_option_ae4_ror(const AoeGameSettingsRor *g);    /* 0x005054d0 */
uint8_t aoe_settings_option_ae5_ror(const AoeGameSettingsRor *g);    /* 0x005054e0 */
/* 0x005054f0: +0xae6 when +0x97e is set, else 50. */
uint8_t aoe_settings_population_limit_ror(const AoeGameSettingsRor *g);
uint8_t aoe_settings_byte_1190_ror(const AoeGameSettingsRor *g);     /* 0x00505510 */
void aoe_settings_set_option_ad6_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x00505520 */
void aoe_settings_set_option_ad7_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x00505530 */
void aoe_settings_set_option_ad8_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x00505540 */
void aoe_settings_set_option_ad9_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x00505550 */
void aoe_settings_set_resource_level_ror(AoeGameSettingsRor *g,
                                         int32_t v);                 /* 0x00505560 */
void aoe_settings_set_starting_age_ror(AoeGameSettingsRor *g,
                                       int32_t v);                   /* 0x00505570 */
void aoe_settings_set_option_ae4_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x00505580 */
void aoe_settings_set_option_ae5_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x00505590 */
void aoe_settings_set_population_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x005055a0 */
void aoe_settings_set_byte_1190_ror(AoeGameSettingsRor *g, uint8_t v); /* 0x005055b0 */
void aoe_settings_set_int_1194_ror(AoeGameSettingsRor *g, int32_t v); /* 0x005055c0 */

#endif
