#ifndef VITA_AOE_SOUND_EFFECT_H
#define VITA_AOE_SOUND_EFFECT_H

#include <stdint.h>

#include "aoe_rng.h"
#include "aoe_stream.h"

/* Patched Rise of Rome 1.0a sound effect (0x004a1f10..0x004a2510): a
   data-file sound with weighted variants, each with its own wave object
   that is loaded on demand and unloaded again when unused for the cache
   time. Wave objects and the sound system are AoeSoundEffectOpsRor.
   Semantic storage; offsets are noted. */

typedef struct {
    char name[0x10];          /* +0x00: 13 bytes in the data, ".wav" added */
    int32_t resource;         /* +0x10 */
    int16_t probability;      /* +0x14: percent */
    void *wave;               /* +0x18 */
    uint8_t state;            /* +0x1c: 1 loaded, 2 not */
    uint32_t last_ms;         /* +0x20 */
} AoeSoundVariantRor;

typedef struct {
    int16_t field_0;          /* +0x00 */
    int16_t count;            /* +0x02 */
    AoeSoundVariantRor *items;/* +0x04 */
    uint32_t cache_ms;        /* +0x08, 300000 */
    int16_t id;               /* +0x0c */
} AoeSoundEffectRor;

typedef struct {
    void *context;
    /* A wave object for a variant (name and resource) on a sound system;
       its deletion, playing check, stop, unload, and play (once, or the
       second form for `alternate`). */
    void *(*create_wave)(void *context, void *system,
                         const AoeSoundVariantRor *variant);
    void (*free_wave)(void *context, void *wave);
    int (*playing)(void *context, void *wave);
    void (*stop)(void *context, void *wave);
    void (*unload)(void *context, void *wave);
    void (*play)(void *context, void *wave, int alternate);
    uint32_t (*now_ms)(void *context);      /* timeGetTime */
    /* Sound logging (global 0x00580d84): open and close a "{name" / "}"
       block. */
    void (*log)(void *context, const char *text);
    AoeRng *rng;
} AoeSoundEffectOpsRor;

/* 0x004a1f10: from text: "%hd" count, then per variant "%d %s %hd"
   (resource, name, probability), ".wav" appended; no waves; cache time
   300000. 0x004a2000 / 0x004a21a0: binary load (id, field 0, count,
   cache time, then 13-byte name, resource, probability per variant,
   each getting a wave on `system`) and save. 0x004a2130: free. */
int aoe_sound_effect_load_text_ror(AoeSoundEffectRor *e, const char *text,
                                   int16_t id);
int aoe_sound_effect_load_ror(AoeSoundEffectRor *e, AoeOutcomeStream *in,
                              void *system, const AoeSoundEffectOpsRor *ops);
void aoe_sound_effect_save_ror(const AoeSoundEffectRor *e,
                               AoeOutcomeStream *out);
void aoe_sound_effect_free_ror(AoeSoundEffectRor *e,
                               const AoeSoundEffectOpsRor *ops);
/* 0x004a2230: new waves on another sound system (none without one); all
   variants become unloaded. 0x004a2310: unload waves idle longer than
   the cache time. 0x004a2510: stop any that are playing. */
void aoe_sound_effect_attach_ror(AoeSoundEffectRor *e, void *system,
                                 const AoeSoundEffectOpsRor *ops);
void aoe_sound_effect_expire_ror(AoeSoundEffectRor *e, uint32_t now_ms,
                                 const AoeSoundEffectOpsRor *ops);
void aoe_sound_effect_stop_ror(AoeSoundEffectRor *e,
                               const AoeSoundEffectOpsRor *ops);
/* 0x004a2370: play a variant chosen by rand() * 100 / 0x7fff against
   the running probabilities (nothing past the last), marking it loaded
   at the current time. 0x004a24a0: the same choice without playing
   (the original returns a field inside the wave object; the port
   returns the variant, NULL without a wave). */
void aoe_sound_effect_play_ror(AoeSoundEffectRor *e, int alternate,
                               const AoeSoundEffectOpsRor *ops);
AoeSoundVariantRor *aoe_sound_effect_pick_ror(AoeSoundEffectRor *e,
                                              const AoeSoundEffectOpsRor *ops);

#endif
