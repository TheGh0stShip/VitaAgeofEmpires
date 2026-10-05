#ifndef VITA_AOE_GAME_SYSTEMS_H
#define VITA_AOE_GAME_SYSTEMS_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a base game subsystem steps (vtable 0x005430b4,
   0x00416240..0x0041c1b0): the start-up steps that create one subsystem
   each, their shutdown counterparts and the saved-game header checks.
   Allocation and the subsystems themselves are ops; these functions keep
   the order, the arguments, the registry values and the failure rules. */

/* 0x00418be0 (+0x9c): the common shapes "groupnum.shp" 50403,
   "waypoint.shp" 50404, "moveto.shp" 50405 into +0x5c (count +0x58 = 3,
   cleared first). Only the table allocation can fail. */
typedef struct {
    void *context;
    void *(*load_shape)(void *context, const char *name, int32_t id);
    void *(*load_sound)(void *context, const char *name, int32_t id);
    void (*preload_sound)(void *context, void *sound);   /* 0x004a2ca0(0, -1) */
} AoeGameResourceOpsRor;
int aoe_game_load_common_shapes_ror(const AoeGameResourceOpsRor *ops, void *shapes[3],
                                    int16_t *count);
/* 0x00418d10 (+0x98): "button1.wav" 50300, "button2.wav" 50301,
   "chatrcvd.wav" 50302 into +0x6c (count +0x68 = 3), then each loaded
   one preloaded. */
int aoe_game_load_common_sounds_ror(const AoeGameResourceOpsRor *ops, void *sounds[3],
                                    int16_t *count);

/* 0x00418920 (+0x94): with sound on (+0x0c +0x8a8), the sound system for
   the window and the sound directory (+0x0c +0xe21); when it cannot
   start (0x004a2800) it is dropped and the step still succeeds. The
   registry Sound Volume v sets the volume -v (0 when missing) with
   0x004a2810(volume, 1). Then +0x58. */
typedef struct {
    void *context;
    int sound_on;
    void *(*create)(void *context);              /* 0x004a2560 */
    int (*start)(void *context, void *system);   /* 0x004a2800 */
    void (*destroy)(void *context, void *system);
    void (*set_volume)(void *context, void *system, int32_t volume);
    int32_t (*get_volume)(void *context, void *system); /* 0x00454250 */
    int32_t (*registry_get)(void *context, const char *name);
    void (*registry_set)(void *context, const char *name, int32_t v);
    void (*slot_58)(void *context);
} AoeGameSoundOpsRor;
int aoe_game_start_sound_ror(const AoeGameSoundOpsRor *ops, void **system);
/* The sound system saved and destroyed: Sound Volume (negated). */
void aoe_game_stop_sound_ror(const AoeGameSoundOpsRor *ops, void **system);

/* 0x004190e0 (+0x90): sound off. The comm object's sound detached
   (0x0041ca90), 0x004522e0, every world player's sounds detached
   (0x004a2230(0)), the common sounds (+0x6c, count +0x68) freed, the
   music stopped (+0x5c), then the sound system saved and destroyed.
   0x004191d0: sound back on (when off): +0x94 and +0x9c, each player's
   sounds attached, 0x00452310, the comm object's, then +0xb8. */
typedef struct {
    void *context;
    int has_comm;                                       /* 0x00580da0 */
    void (*comm_sound)(void *context, void *system);    /* 0x0041ca90 / 0x0041caa0 */
    void (*call_4522e0)(void *context);
    void (*call_452310)(void *context);
    int32_t player_count;                               /* world +0x2c, 0 without */
    void (*player_sound)(void *context, int32_t i, void *system); /* 0x004a2230 */
    void (*free_sound)(void *context, void *sound);
    void (*stop_music)(void *context);                  /* +0x5c */
    int (*slot_94)(void *context);
    int (*slot_9c)(void *context);
    void (*slot_b8)(void *context);
} AoeGameSoundCycleOpsRor;
void aoe_game_sound_off_ror(const AoeGameSoundCycleOpsRor *c, const AoeGameSoundOpsRor *ops,
                            void **system, void ***sounds, int16_t *count);
int aoe_game_sound_on_ror(const AoeGameSoundCycleOpsRor *c, void **system);

/* 0x00419330 (+0x5c): with music, stop it (+0xb4), save Music Volume
   (-music +0x254) and destroy it. */
typedef struct {
    void *context;
    void (*stop)(void *context);                 /* +0xb4 */
    int32_t (*volume)(void *context, void *music);
    void (*destroy)(void *context, void *music);
    void (*registry_set)(void *context, const char *name, int32_t v);
} AoeGameMusicOpsRor;
void aoe_game_stop_music_ror(const AoeGameMusicOpsRor *ops, void **music);

/* 0x004184b0 (+0x88's pointer step): the Custom Mouse registry value
   (written 0 when missing; 1 custom pointer; 2 the game's custom mouse
   flag +0x54 off), then the pointer object (0x004496e0(custom)) set up
   with the flag, the main surface, the pointer name (+0x0c +0x773),
   51000 and 10 (0x004499a0); then 0x00419c20. */
typedef struct {
    void *context;
    int32_t (*registry_get)(void *context, const char *name);
    void (*registry_set)(void *context, const char *name, int32_t v);
    void *(*create)(void *context, int custom);
    int (*setup)(void *context, void *pointer, int32_t custom_mouse, int32_t id,
                 int32_t frames);
    void (*after)(void *context);                /* 0x00419c20 */
} AoeGamePointerOpsRor;
int aoe_game_setup_pointer_ror(const AoeGamePointerOpsRor *ops, int32_t *custom_mouse,
                               void **pointer);

/* 0x00418620: the registry objects: the game's key (+0x0c +0x418) into
   +0x1ac and the global "Software\Microsoft\Microsoft Games\Age of
   Empires Expansion\1.0" (0x00580dc0). Succeeds with the game's key. */
typedef struct {
    void *context;
    void *(*open_key)(void *context, const char *path);
} AoeGameRegistryOpsRor;
int aoe_game_open_registry_ror(const AoeGameRegistryOpsRor *ops, const char *game_key,
                               void **key, void **global_key);
extern const char aoe_game_global_key_ror[];

/* 0x004186d0: the debug log object (0x0042bf80) with the log flag
   (+0x1a8), the global level (0x00580db4) and 1. */
typedef struct {
    void *context;
    void *(*create)(void *context);
    void (*set_flag)(void *context, void *log, int32_t v);    /* 0x0042c010 */
    void (*set_level)(void *context, void *log, int32_t v);   /* 0x0042c050 */
    void (*set_on)(void *context, void *log, int32_t v);      /* 0x0042c090 */
} AoeGameLogOpsRor;
int aoe_game_open_log_ror(const AoeGameLogOpsRor *ops, int32_t flag, int32_t level,
                          void **log);
/* 0x00416240: a line ("%s") to the draw log (0x00580b70) and a flush;
   0x00416270: the same, then the file reopened ("a+") so it survives a
   crash. */
typedef struct {
    void *context;
    void *file;
    void (*write)(void *context, void *file, const char *text);
    void (*flush)(void *context, void *file);
    void (*close)(void *context, void *file);
    void *(*reopen)(void *context);
} AoeGameDrawLogOpsRor;
void aoe_game_draw_log_ror(AoeGameDrawLogOpsRor *ops, const char *text);
void aoe_game_draw_log_reopen_ror(AoeGameDrawLogOpsRor *ops, const char *text);

/* 0x00418fe0 (+0xa8): the "Blank Screen" (full area, no info), shown,
   then 0x0041c290 and WM_PAINT. */
typedef struct {
    void *context;
    void *(*create_screen)(void *context, const char *name);
    int (*setup)(void *context, void *screen);
    void (*show)(void *context, const char *name);
    void (*after)(void *context);
    void (*paint)(void *context);
} AoeGameBlankOpsRor;
int aoe_game_blank_screen_ror(const AoeGameBlankOpsRor *ops);

/* 0x00417780: a saved game's first word is one of the version tags
   "1.03" .. "1.12" (0x00554854..0x0055489c); then +0x38 reads it.
   0x0041c040: the same through +0x34's info (its +0x0c), 0 without. */
int aoe_game_saved_version_ok_ror(const char tag[4]);

/* 0x0041c1b0: the 280 x 140 "Diamond Map Save Area" surface (+0xa20). */
typedef struct {
    void *context;
    void *(*create)(void *context, const char *name);
    int (*attach)(void *context, void *surface, int32_t w, int32_t h);
    void (*destroy)(void *context, void *surface);
} AoeGameSurfaceOpsRor;
int aoe_game_map_save_area_ror(const AoeGameSurfaceOpsRor *ops, void **area);

/* 0x0041b3f0: the caption "No AI Information for Player #%d." for the
   viewed player. */
void aoe_game_no_ai_caption_ror(char *out, int size, int16_t player);

#endif
