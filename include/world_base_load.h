#ifndef VITA_AOE_WORLD_BASE_LOAD_H
#define VITA_AOE_WORLD_BASE_LOAD_H

#include <stdint.h>

#include "aoe_stream.h"
#include "scenario_object.h"

/* Patched Rise of Rome 1.0a base scenario object (vtable 0x00545e3c,
   0x1990 bytes) as its binary constructor 0x00474d40(stream, world)
   builds it: the world's scenario (+0x5c, world_base.h's scenario_read;
   the game's 0x004179b0 allocates one and passes world 0) and the base
   the RoR scenario reader 0x00506900 starts with (world_ror_scenario.h's
   base_read). The settings part (+0x0c..+0x198b) is the existing
   AoeScenarioObjectRor; the fields it does not model are kept beside it.
   `settings` comes first, so a base_read binding that hands out
   &base->settings can convert that pointer back to its
   AoeScenarioBaseRor. Semantic storage; offsets noted. */
typedef struct {
    AoeScenarioObjectRor settings; /* +0x0c..+0x198b (less +0x1888) */
    AoeScenarioEntriesRor *entries; /* +0x04, read by 0x004ebbf0 */
    void *world;                    /* +0x08 */
    float version;                  /* +0x1888, the file's f32 */
    void *picture;                  /* +0x198c, a 0x20-byte DIB picture */
} AoeScenarioBaseRor;

typedef struct {
    void *context;
    /* The string table (0x00580e38's +0x20) the reset 0x004755b0 asks
       for 0x2775 " <None> " and 0x277b "Random". Required. */
    void (*string)(void *context, int32_t id, char *out, int size);
    /* The lookups 0x004ebbf0 makes by id; required when a world is
       given (none is made without one). */
    const AoeScenarioEntriesOpsRor *entries;
    /* 0x0045af60(stream, spans) on a fresh 0x20-byte picture (spans is
       1 here; dib_picture.h's aoe_dib_picture_load_saved_ror); NULL when
       none is made. Without it no picture is read. */
    void *(*picture_read)(void *context, AoeOutcomeStream *s, int32_t spans);
    /* 0x0045b0d0 then the free, for the release below. */
    void (*picture_free)(void *context, void *picture);
} AoeScenarioBaseOpsRor;

/* 0x00474d40: the world kept (+0x08), the defaults (0x004755b0,
   aoe_scenario_reset_ror), then from the stream, every gate comparing
   the f32 version (+0x1888) with an x87 compare against an f32 (1.13,
   1.03, 1.15, 1.02) or f64 (1.13, 1.07, 1.11, 1.09, 1.10, 1.08)
   constant, little-endian:
   - f32 version;
   - above 1.13 (f32 0x00545e80): the 16 0x100-byte player names (+0x0d);
   - above 1.13 (f64 0x00545e78): per player value_194c (+0x194c),
     value_18cc (+0x18cc), value_190c (+0x190c) and the kind (+0x188c);
   - below 1.07 (f64 0x00545e70, or NaN): +0x0c is 1; else a u8;
   - the entry list (+0x04, 0x14 bytes, 0x004ebbf0 with the world);
   - texts are an i16 length then that many bytes: the description
     (+0x1010, NUL at [length]); message 0 (+0x1014, a fresh buffer, none
     for a length below 1, its last byte made the NUL);
   - at least 1.11 (f64 0x00545e68): messages 1..3 (+0x1018..+0x1020) as
     message 0 and message 4 (+0x1024) as read, without the NUL; else
     (or NaN) all four none;
   - below 1.03 (f32 0x00545e60, or NaN): three texts read and dropped;
   - the texts +0x1508, +0x15e8, +0x16c8 and, at least 1.09 (f64
     0x00544e20), +0x17a8 (NUL at [length]);
   - at least 1.10 (f64 0x00545e58): the picture (+0x198c); else none;
   - the 16 AI names (+0x12c8), 16 city-plan names (+0x1308) and, at
     least 1.08 (f64 0x00544e10), 16 personality names (+0x1348), each
     into the reset's 0xe0-byte buffer, NUL at [length];
   - per player the AI size (+0x1448), city-plan size (+0x1488) and, at
     least 1.08, personality size (+0x14c8); the three data pointers
     (+0x1388, +0x13c8, +0x1408) none and, at least 1.15 (f32
     0x00545e50), each positive size's bytes into a fresh buffer;
   - above 1.02 (f32 0x00545e4c) a dword read and dropped.
   Fields a gate skips keep the reset's values. Returns the version.
   Port guards for malformed files (the original overruns): a negative
   length leaves its text as it was (the original writes its NUL before
   the buffer); an inline 0xe0-byte text keeps 0xdf bytes; a heap text
   longer than its 0xe0 bytes grows; messages get one spare zero byte so
   message 4 always ends; a short stream reads nothing more (and data
   are not allocated for bytes it lacks). `sc` need not be initialised;
   a failed allocation leaves that pointer NULL. */
float aoe_scenario_base_read_ror(AoeScenarioBaseRor *sc, AoeOutcomeStream *s, void *world,
                                 const AoeScenarioBaseOpsRor *ops);

/* 0x00475490 for the fields beside the settings: the entry list (its
   deleting destructor 0x004ebda0) and the picture (picture_free); the
   settings through aoe_scenario_release_ror. */
void aoe_scenario_base_release_ror(AoeScenarioBaseRor *sc, const AoeScenarioBaseOpsRor *ops);

#endif
