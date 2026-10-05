#ifndef VITA_AOE_WORLD_BASE_H
#define VITA_AOE_WORLD_BASE_H

#include <stdint.h>
#include <stdio.h>

#include "player_outcome.h" /* AoeOutcomeStream */

/* Patched Rise of Rome 1.0a base world (vtable 0x0054a954,
   0x0051b140..0x0051ec00): the game data tables (sounds, sprites,
   civilisations, effects, the map, the colour tables and the terrain
   table), loaded from the text lists or from the binary data file
   ("VER 3.7"), the players, and their lifetime. The sub-objects are
   opaque handles created and driven through ops; the overridable
   methods (the RoR world replaces several, vtable 0x0054a4f8) go
   through `vtbl`. Semantic storage; offsets noted. */

/* A colour table (vtable 0x0054320c, 0x128 bytes): a name, an ID, a
   palette index, two bytes and the 256-entry map (from "data\<name>"). */
typedef struct {
    char name[30];       /* +0x04 */
    int16_t id;          /* +0x22 */
    uint8_t field_24;    /* +0x24 */
    uint8_t field_25;    /* +0x25 */
    int16_t palette;     /* +0x26 */
    uint8_t map[256];    /* +0x28 */
} AoeWorldColorRor;

/* 0x0041cfe0: from text: "%hd %s %hd %hd" (ID, name, +0x25, +0x24), the
   name given ".col"; the map zeroed and the palette from the caller.
   0x0041cf00: binary (name 30, palette 2, ID 2, +0x25, +0x24), then the
   map read as 256 "%hd" values from "data\<name>" (when it opens).
   0x0041d250: binary save (the same five fields). 0x0041d240: the
   destructor (nothing to free). */
void aoe_world_color_from_text_ror(AoeWorldColorRor *c, FILE *f, int16_t palette);
void aoe_world_color_read_ror(AoeWorldColorRor *c, AoeOutcomeStream *s,
                              FILE *(*open)(void *context, const char *path), void *context);
void aoe_world_color_write_ror(const AoeWorldColorRor *c, AoeOutcomeStream *s);

typedef struct AoeWorldBaseRor AoeWorldBaseRor;

typedef struct {
    void *context;
    /* Sounds (0x10 bytes): 0x004a1f10 from text, 0x004a2000 binary (the
       +0x80 argument), 0x004a21a0 save, 0x004a2510 stop, 0x004a2310
       idle-unload, 0x004a2130 free. */
    void *(*sound_text)(void *context, FILE *f, int16_t id);
    void *(*sound_read)(void *context, AoeOutcomeStream *s, int32_t arg);
    void (*sound_write)(void *context, void *sound, AoeOutcomeStream *s);
    void (*sound_stop)(void *context, void *sound);
    void (*sound_idle)(void *context, void *sound, uint32_t now);
    void (*sound_free)(void *context, void *sound);
    /* Sprites (0x78 bytes): 0x004a4ea0 from text (the sounds), 0x004a4bd0
       an empty one, 0x004a4c40 binary (sounds, colours), 0x004a51c0 link
       to the table, 0x004a5220 save, 0x004a6430 idle-unload, 0x004a5160
       free. */
    void *(*sprite_text)(void *context, FILE *f, int16_t id, void **sounds);
    void *(*sprite_empty)(void *context, int16_t id);
    void *(*sprite_read)(void *context, AoeOutcomeStream *s, void **sounds, void **colors);
    void (*sprite_link)(void *context, void *sprite, void **sprites);
    void (*sprite_write)(void *context, void *sprite, AoeOutcomeStream *s);
    void (*sprite_idle)(void *context, void *sprite, uint32_t now);
    void (*sprite_free)(void *context, void *sprite);
    /* Civilisations (0x2c bytes): 0x0044f1b0 from text, 0x0044f2a0
       binary then its +0x04(stream, sprites, sounds); +0x0c(value) and
       +0x10(file, id, sprites, sounds, extra) the text entries; +0x14
       save; +0x00 delete. */
    void *(*civ_text)(void *context, FILE *f);
    void *(*civ_read)(void *context, AoeOutcomeStream *s, void **sprites, void **sounds);
    void (*civ_text_value)(void *context, void *civ, int32_t v);
    void (*civ_text_entry)(void *context, void *civ, FILE *f, int16_t id, void **sprites,
                           void **sounds, int16_t extra);
    void (*civ_write)(void *context, void *civ, AoeOutcomeStream *s);
    void (*civ_free)(void *context, void *civ);
    /* Effects (0xc bytes): 0x0043b5f0 from a text file, 0x0043b480
       binary, +0x04 save, +0x00 delete. */
    void *(*effects_text)(void *context, const char *name);
    void *(*effects_read)(void *context, AoeOutcomeStream *s);
    void (*effects_write)(void *context, void *effects, AoeOutcomeStream *s);
    void (*effects_free)(void *context, void *effects);
    /* The map (0x8dd0 bytes): 0x00443d80 from the text names (terrain,
       border, overlay files, three counts, the sounds) and 0x00447230 its
       extra list; 0x00443ce0 binary (the sounds, 1); +0x08 the four more
       names; +0x2c save; 0x00444560 / 0x00444570; +0x00 delete. */
    void *(*map_text)(void *context, const char *a, const char *b, const char *c, int16_t x,
                      int16_t y, int16_t z, void **sounds);
    void (*map_text_extra)(void *context, void *map, const char *name);
    void *(*map_read)(void *context, AoeOutcomeStream *s, void **sounds, int32_t arg);
    void (*map_text_names)(void *context, void *map, const char *a, const char *b,
                           const char *c, const char *d);
    void (*map_write)(void *context, void *map, AoeOutcomeStream *s);
    void (*map_444560)(void *context, void *map, int32_t v);
    void (*map_444570)(void *context, void *map, int32_t v);
    void (*map_free)(void *context, void *map);
    /* Scenario (0x1990 bytes): 0x00474c90 new (info), 0x00474d40 binary;
       +0x00 delete. */
    void *(*scenario_new)(void *context, void *info);
    void *(*scenario_read)(void *context, AoeOutcomeStream *s, int32_t arg);
    void (*scenario_free)(void *context, void *scenario);
    /* Players (0x224 bytes): 0x0045b410 binary (world, index) then its
       +0x74(stream); 0x0045e950 new view; 0x0045d3e0 units' +0x68;
       +0x00 delete. */
    void *(*player_read)(void *context, AoeOutcomeStream *s, AoeWorldBaseRor *w, int16_t index);
    void (*player_new_view)(void *context, void *player);
    void (*player_units_68)(void *context, void *player);
    void (*player_free)(void *context, void *player);
    /* The seven lists (0x00450ef0 new, 0x00450f60 clear, 0x00450f10
       free), +0x58 and +0xa0 (delete). */
    void *(*list_new)(void *context);
    void (*list_clear)(void *context, void *list);
    void (*list_free)(void *context, void *list);
    void (*free_object)(void *context, void *object);
    /* Files: a text file (fopen "r"), the binary data file opened for
       reading (0x0046b5b0 with 0x8000) and closed (0x0046b6b0). */
    FILE *(*open_text)(void *context, const char *path);
    AoeOutcomeStream *(*open_read)(void *context, const char *path);
    void (*close_read)(void *context, AoeOutcomeStream *s);
    /* The data file opened for writing (0x0046b630, 0x8309, 0600) and
       closed. */
    AoeOutcomeStream *(*open_write)(void *context, const char *path);
    void (*close_write)(void *context, AoeOutcomeStream *s);
} AoeWorldOpsRor;

/* The overridable methods (slots noted; the base table is
   `aoe_world_base_vtable_ror`). */
typedef struct {
    int (*load_text)(AoeWorldBaseRor *w, FILE *f);                        /* +0x00 */
    int (*load_table_text)(AoeWorldBaseRor *w, const char *name);         /* +0x04 */
    int (*load_civs_text)(AoeWorldBaseRor *w, const char *name);          /* +0x08 */
    int (*load_civ_entries_text)(AoeWorldBaseRor *w, const char *name);   /* +0x0c */
    int (*load_sounds_text)(AoeWorldBaseRor *w, const char *name);        /* +0x10 */
    int (*load_colors_text)(AoeWorldBaseRor *w, const char *name);        /* +0x14 */
    int (*load_sprites_text)(AoeWorldBaseRor *w, const char *name);       /* +0x18 */
    void (*civ_from_text)(AoeWorldBaseRor *w, int16_t id, int16_t flag, FILE *f); /* +0x1c */
    void (*effects_from_text)(AoeWorldBaseRor *w, const char *name);      /* +0x20 */
    void (*map_from_text)(AoeWorldBaseRor *w, const char *a, const char *b, const char *c,
                          const char *d, int16_t x, int16_t y, int16_t z, void **sounds,
                          const char *extra);                             /* +0x24 */
    void (*map_text_names)(AoeWorldBaseRor *w, const char *a, const char *b, const char *c,
                           const char *d);                                /* +0x28 */
    int (*civ_from_stream)(AoeWorldBaseRor *w, AoeOutcomeStream *s, int16_t id, uint8_t flag); /* +0x2c */
    int (*load_civs)(AoeWorldBaseRor *w, AoeOutcomeStream *s);            /* +0x30 */
    void (*load_colors)(AoeWorldBaseRor *w, AoeOutcomeStream *s);         /* +0x38 */
    void (*load_table)(AoeWorldBaseRor *w, AoeOutcomeStream *s);          /* +0x3c */
    void (*load_sounds)(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t arg); /* +0x40 */
    void (*load_sprites)(AoeWorldBaseRor *w, AoeOutcomeStream *s);        /* +0x44 */
    void (*map_from_stream)(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t arg); /* +0x48 */
    void (*effects_from_stream)(AoeWorldBaseRor *w, AoeOutcomeStream *s); /* +0x4c */
    void (*slot_54)(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t arg); /* +0x54 */
    void (*load_data)(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t a, int32_t b); /* +0x58 */
    void (*release_sounds)(AoeWorldBaseRor *w);                           /* +0xb4 */
    void (*save_data_file)(AoeWorldBaseRor *w, const char *path);         /* +0xdc */
    void (*save_data)(const AoeWorldBaseRor *w, AoeOutcomeStream *s);     /* +0xe0 */
} AoeWorldVtblRor;

struct AoeWorldBaseRor {
    const AoeWorldVtblRor *vtbl;
    int32_t game_time;          /* +0x04 */
    int32_t old_time;           /* +0x08 */
    int32_t time_step;          /* +0x0c */
    int32_t field_10;           /* +0x10 */
    uint32_t last_ms;           /* +0x14 */
    float speed;                /* +0x18 */
    uint8_t field_1c;           /* +0x1c */
    uint8_t state;              /* +0x1d: 0 playing, 2 none, 3 new */
    uint8_t field_1e;           /* +0x1e */
    int32_t sound_index;        /* +0x20 */
    int32_t sprite_index;       /* +0x24 */
    void *map;                  /* +0x28 */
    int16_t sound_count;        /* +0x2c */
    void **sounds;              /* +0x30 */
    int16_t sprite_count;       /* +0x34 */
    void **sprites;             /* +0x38 */
    int16_t player_count;       /* +0x3c */
    void **players;             /* +0x40 */
    int16_t civ_count;          /* +0x44 */
    void **civs;                /* +0x48 */
    void *effects;              /* +0x4c */
    int16_t table_rows;         /* +0x50 */
    int16_t table_cols;         /* +0x52 */
    float **table;              /* +0x54 */
    void *field_58;             /* +0x58 */
    void *scenario;             /* +0x5c */
    int16_t color_count;        /* +0x60 */
    AoeWorldColorRor **colors;  /* +0x64 */
    int32_t field_68;           /* +0x68 */
    int32_t field_6c;           /* +0x6c, -1 */
    int32_t field_70;           /* +0x70 */
    uint8_t field_74;           /* +0x74 */
    int32_t seed;               /* +0x78, 14 */
    int16_t field_7c;           /* +0x7c, 1 */
    int32_t data_arg;           /* +0x80 */
    int32_t field_84;           /* +0x84 */
    void **objects;             /* +0x88 */
    int32_t field_8c;           /* +0x8c */
    int32_t objects_cap;        /* +0x90, 10000 */
    void **objects2;            /* +0x94 */
    int32_t field_98;           /* +0x98 */
    int32_t objects2_cap;       /* +0x9c, 1000 */
    void *field_a0;             /* +0xa0 */
    int32_t campaign;           /* +0xa4, -1 */
    int32_t campaign_player;    /* +0xa8, -1 */
    int32_t campaign_scenario;  /* +0xac, -1 */
    int32_t update_slot;        /* +0xb0 */
    int32_t update_times[9];    /* +0xb4 */
    void *lists[7];             /* +0xd8..+0xf0 */
    int32_t field_f4, field_f8; /* +0xf4, +0xf8 = 2 */
    int32_t field_fc;           /* +0xfc, -1 */
    int32_t field_100;          /* +0x100, -1 */
    const AoeWorldOpsRor *ops;
};

extern const AoeWorldVtblRor aoe_world_base_vtable_ror;

/* Globals the base world keeps: 0x007d2054 (the object table) and
   0x007d2460 (a counter cleared on load and reset). */
extern void **aoe_world_object_table_ror;
extern int32_t aoe_world_counter_ror;

/* 0x0051b140: the constructor: the two object tables (10000 and 1000
   entries, zeroed; the first published at 0x007d2054), the counts and
   tables empty, seed 14, +0x7c 1, state 2 then 0, the seven lists
   created. 0x0051b440 the destructor (0x0051b420 deleting): +0x58, the
   reset (0x0051c980), the lists, the civilisations, the effects, the
   map, the sprites, the sounds, the terrain table, the colour tables,
   the object tables and the AI log file (0x007d249c). */
int aoe_world_init_ror(AoeWorldBaseRor *w, const AoeWorldOpsRor *ops);
void aoe_world_release_ror(AoeWorldBaseRor *w, FILE **ai_log);
/* +0xb8 0x0051c980: back to no game: state 2, the lists cleared,
   +0x6c and the campaign -1, +0x100 -1, the object tables zeroed, the
   sounds stopped (+0xb4), the scenario deleted, each player's units
   given +0x68 then the players deleted, the clocks zeroed. */
void aoe_world_reset_ror(AoeWorldBaseRor *w);
/* 0x0051b6d0: the times, +0x84, +0x10, +0x14, +0x1c, +0x68 and the
   update slots zeroed. */
void aoe_world_clear_times_ror(AoeWorldBaseRor *w);
/* +0xec 0x0051b710: stop every sound when `on`; +0xb4 0x0051c910 stop
   them all. */
void aoe_world_stop_sounds_ror(AoeWorldBaseRor *w, int on);
void aoe_world_release_sounds_ror(AoeWorldBaseRor *w);
/* 0x0051b740 / 0x0051b760: the map's 0x00444560 / 0x00444570 (with a
   map). */
void aoe_world_map_444560_ror(AoeWorldBaseRor *w, int32_t v);
void aoe_world_map_444570_ror(AoeWorldBaseRor *w, int32_t v);

/* Text lists ("%hd %hd" count and entry count, then each entry's
   "%hd" ID; no file is not an error; 1 always):
   +0x10 0x0051b780 sounds; +0x04 0x0051b8c0 the terrain table (rows,
   columns; per row "%hd %hd" index and count, then per entry "%hd"
   column and "%f" value); +0x14 0x0051b9f0 colour tables; +0x18
   0x0051bb30 sprites (then each missing one an empty sprite); +0x08
   0x0051bd30 civilisations (per entry "%hd %hd" ID and flag to +0x1c);
   +0x0c 0x0051be00 their entries ("%hd" count; per civilisation "%hd"
   index, "%hd" value, "%hd" count, then "%hd %hd" ID and extra).
   +0x1c 0x0051bcc0: a civilisation from text unless `flag`. +0x20
   0x0051bf30 the effects. +0x24 0x0051bf90 the map (names a, c, d,
   three counts, the sounds) and its extra list. +0x28 0x0051c020 the
   map's four more names. +0x00 0x0051c040: the sixteen names and three
   counts ("%s" x 16, "%hd" x 3) read from the list file, then the
   table, colours, sounds, sprites, civilisations, entries, effects, map
   and map names; 1. */
int aoe_world_load_text_ror(AoeWorldBaseRor *w, FILE *f);
int aoe_world_load_table_text_ror(AoeWorldBaseRor *w, const char *name);
int aoe_world_load_civs_text_ror(AoeWorldBaseRor *w, const char *name);
int aoe_world_load_civ_entries_text_ror(AoeWorldBaseRor *w, const char *name);
int aoe_world_load_sounds_text_ror(AoeWorldBaseRor *w, const char *name);
int aoe_world_load_colors_text_ror(AoeWorldBaseRor *w, const char *name);
int aoe_world_load_sprites_text_ror(AoeWorldBaseRor *w, const char *name);
void aoe_world_civ_from_text_ror(AoeWorldBaseRor *w, int16_t id, int16_t flag, FILE *f);
void aoe_world_effects_from_text_ror(AoeWorldBaseRor *w, const char *name);
void aoe_world_map_from_text_ror(AoeWorldBaseRor *w, const char *a, const char *b,
                                 const char *c, const char *d, int16_t x, int16_t y, int16_t z,
                                 void **sounds, const char *extra);
void aoe_world_map_text_names_ror(AoeWorldBaseRor *w, const char *a, const char *b,
                                  const char *c, const char *d);
/* +0x9c 0x0051c1c0: nothing. +0xac 0x0051c1d0: convert: open the text
   list, run +0x00 on it, close it, then save the data file (+0xdc); 0
   when the list does not open. +0xdc 0x0051e2f0: open `path` for writing
   and save the data (+0xe0). */
void aoe_world_nothing_ror(AoeWorldBaseRor *w);
int aoe_world_convert_text_ror(AoeWorldBaseRor *w, const char *data_path, const char *list_path);
void aoe_world_save_data_file_ror(AoeWorldBaseRor *w, const char *path);

/* Binary data: +0x2c 0x0051c220 a civilisation unless `flag` (1);
   +0x30 0x0051c2b0 the civilisations (i16 count; per one a flag byte);
   +0x3c 0x0051c320 the terrain table (i16 rows, columns; the row
   row pointers as presence words, then each present row of floats); +0x38 0x0051c3c0
   the colour tables; +0x40 0x0051c470 the sounds; +0x44 0x0051c510 the
   sprites (presence words, then each present sprite, then linked);
   +0x48 0x0051c6b0 the map; +0x4c 0x0051c720 the effects; +0x50
   0x0051c780 = +0x30; +0x58 0x0051c7f0 all of them in that order (the
   table, colours, sounds, sprites, map, effects, civilisations, +0x54);
   +0xf4 0x0051c5f0 / +0xf0 0x0051c650 the scenario from a stream / for
   a game info. +0x34 0x0051cb00 a player from the stream unless `flag`
   (its +0x74 then). +0xb0 0x0051c850: open the data file (+0x80 =
   `a`, +0x68 = 0), and with "VER 3.7" load it (+0x58). */
int aoe_world_civ_from_stream_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, int16_t id,
                                  uint8_t flag);
int aoe_world_load_civs_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s);
void aoe_world_load_table_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s);
void aoe_world_load_colors_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s);
void aoe_world_load_sounds_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t arg);
void aoe_world_load_sprites_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s);
void aoe_world_map_from_stream_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t arg);
void aoe_world_effects_from_stream_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s);
int aoe_world_load_civs_slot50_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s);
void aoe_world_load_data_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t a, int32_t b);
void aoe_world_scenario_from_stream_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t arg);
void aoe_world_scenario_new_ror(AoeWorldBaseRor *w, void *info);
void aoe_world_player_from_stream_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, uint8_t flag,
                                      int16_t index, void (*load_74)(void *context, void *player,
                                                                    AoeOutcomeStream *s));
int aoe_world_load_data_file_ror(AoeWorldBaseRor *w, const char *path, int32_t a, int32_t b);
/* +0xe0 0x0051e190: save the data ("VER 3.7", the table, the colour
   tables, the sounds, the sprites with presence words, the map, the
   effects, the civilisations). The original writes its row and sprite
   pointers as the presence words; the port writes 1 for present (only
   nonzero is ever tested). */
void aoe_world_save_data_ror(const AoeWorldBaseRor *w, AoeOutcomeStream *s);

/* +0xb4 helpers: 0x0051d870 / 0x0051d8b0 the next sound / sprite (round
   robin) unloads what has idled. */
void aoe_world_idle_sound_ror(AoeWorldBaseRor *w, uint32_t now);
void aoe_world_idle_sprite_ror(AoeWorldBaseRor *w, uint32_t now);
/* 0x0051dd10 the update slot is `v`; 0x0051dd30 update time `i`;
   +0xc0 0x0051dd40 the state. */
int aoe_world_update_slot_is_ror(const AoeWorldBaseRor *w, int32_t v);
int32_t aoe_world_update_time_ror(const AoeWorldBaseRor *w, int32_t i);
uint8_t aoe_world_state_ror(const AoeWorldBaseRor *w);
/* +0x5c 0x0051cf20: player 0 "GAIA" (civilisation 0); +0x6c
   0x0051eb70: player `index` (civilisation `civ`, name) unless `flag`.
   0x0051c950 every player's new view. */
void aoe_world_new_gaia_ror(AoeWorldBaseRor *w,
                            void *(*player_new)(void *context, AoeWorldBaseRor *w, void *civ,
                                                int16_t index, const char *name, uint8_t civ_id,
                                                int flag));
void aoe_world_new_player_ror(AoeWorldBaseRor *w, int16_t index, uint8_t flag, uint8_t civ,
                              const char *name,
                              void *(*player_new)(void *context, AoeWorldBaseRor *w, void *civ,
                                                  int16_t index, const char *name,
                                                  uint8_t civ_id, int flag));
void aoe_world_new_views_ror(AoeWorldBaseRor *w);

#endif
