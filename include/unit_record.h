#ifndef VITA_AOE_UNIT_RECORD_H
#define VITA_AOE_UNIT_RECORD_H

#include <stdint.h>

#include "aoe_stream.h"
#include "movement.h"

/* Patched Rise of Rome 1.0a saved-game unit records: each class's load
   (its constructor from a stream) and save (vtable +0x30) append their
   own fields after their parent's. Little-endian and unaligned on disk;
   the record holds the semantic values, and the objects the IDs name
   (owner, template, sprite, attached unit) are resolved by the caller.

   Classes: 10 the root (load 0x004a6bb0, save 0x004a95e0; type 90 uses
   the same pair), 20 (0x004151c0 / 0x00415280), 30 (0x0044b360 /
   0x0044b690), 40 (0x00405be0 / 0x00405ce0), 50 (0x004261a0 /
   0x00426210), 60 (0x00448f00 / 0x00449010, from 50), 70 (0x004ae5c0 /
   0x004ae750, from 50) and 80 (0x004ac960 / 0x004ad0b0, from 70). The
   class byte (+0x4e) opens the saved record but is read by the world
   before it picks the constructor, so the loads start after it. */

typedef struct {
    int16_t a, b;
} AoeUnitRecordPairRor;

typedef struct {
    /* ---- root ---- */
    uint8_t class_code;          /* +0x4e: written first by the save */
    uint8_t owner;               /* the owner's player number (+0x4a) */
    int16_t template_id;         /* template +0x10 */
    int16_t sprite_id;           /* +0x10 graphic's ID (+0x72), -1 none */
    int32_t attached_id;         /* +0x20 unit's ID, -1 none */
    float hit_points;            /* +0x30 */
    uint8_t state;               /* +0x48 */
    uint8_t positive_hit_points; /* +0x49 */
    uint8_t type_25;             /* +0x4a: saves from 7.09 */
    uint8_t registration_pending;/* +0x4b */
    int32_t unit_id;             /* +0x04 */
    uint8_t frame;               /* +0x35 */
    float x, y, z;               /* +0x38/+0x3c/+0x40 */
    int16_t draw_left, draw_top, draw_right, draw_bottom; /* +0x28..+0x2e */
    uint8_t field_37;            /* +0x37 */
    int16_t carried_type;        /* +0x4c */
    float carried_amount;        /* +0x44 */
    uint8_t target_refs;         /* +0x4f */
    uint8_t damage_percent;      /* +0x34 */
    uint8_t byte_86;             /* +0x86 */
    int32_t dependent_source_id; /* +0x78 */
    uint32_t dependent_context_bits; /* +0x80 */
    int32_t *dependents;         /* +0x64, count +0x68 */
    int32_t dependent_count;
    int32_t *group;              /* +0x54, count +0x58 */
    int32_t group_count;
    /* ---- 20 ---- */
    float animation_rate;        /* +0x88 */
    /* ---- 30 ---- */
    uint32_t field_8c;           /* +0x8c, raw 32 bits */
    /* Velocity (+0x90..+0x98), facing (+0x9c), +0xa0, the path (+0xa4:
       count, 16-byte waypoints x, y, z, direction, then the cursor), the
       start and destination (+0x10c / +0x11c, 16 bytes each), +0x12c..
       +0x148, +0x155, +0x156, the waypoint tiles (+0x168: count,
       capacity, 3 bytes each), +0x174, +0x178 and +0x17c/+0x17d. The
       alternate path and other fields are not saved. */
    AoeMovementRor movement;
    int32_t field_158[4];        /* +0x158..+0x164: saves from 1.3, else -1 */
    /* ---- 40 ---- */
    uint8_t field_180;           /* +0x180 */
    uint8_t field_188;           /* +0x188: saves from 6.5, else 0 */
    /* ---- 50 ---- */
    int32_t field_1b4;           /* +0x1b4 */
    uint8_t field_1b8;           /* +0x1b8: saves from 2.01 */
    /* ---- 60 ---- */
    int32_t field_1bc;           /* +0x1bc: saves after 7.09 */
    /* Set by a load of version 7.09 or older: the caller fills +0x1bc from
       the template's +0x114 (max range). */
    uint8_t field_1bc_from_template;
    /* 60 +0x1c0 (saves from 7.3) / 70 +0x1bc: the unit runs on its own
       template, saved after it. */
    uint8_t private_template;
    uint8_t template_kind;       /* the loaded template's kind byte */
    /* ---- 70 ---- */
    int32_t has_ai;              /* the unit AI (+0x74) follows when 1 */
    /* ---- 80 (saves from 7.21 for the pairs onwards) ---- */
    uint8_t building_completed;  /* +0x1dd */
    int32_t field_1d4;           /* +0x1d4 */
    uint32_t completion_value;   /* +0x1d8 */
    uint8_t field_1dc;           /* +0x1dc */
    AoeUnitRecordPairRor *pairs; /* +0x1c0, count +0x1c4 */
    int16_t pair_count;
    int16_t field_1c6;           /* +0x1c6 */
    uint8_t field_1c8;           /* +0x1c8 */
} AoeUnitRecordRor;

/* The parts saved by other objects. Each returns nonzero on success. */
typedef struct {
    void *context;
    /* The sprite list (0x004153c0 / 0x00415540), after the root fields. */
    int (*load_sprites)(void *context, AoeUnitRecordRor *r,
                        AoeOutcomeStream *in);
    int (*save_sprites)(void *context, const AoeUnitRecordRor *r,
                        AoeOutcomeStream *out);
    /* An action list (0x00403d80 / 0x00403e10): `which` 0 the unit's
       (+0x184, type 40 on), 1 the building's queue (+0x1cc, type 80). */
    int (*load_actions)(void *context, AoeUnitRecordRor *r, int which,
                        AoeOutcomeStream *in);
    int (*save_actions)(void *context, const AoeUnitRecordRor *r, int which,
                        AoeOutcomeStream *out);
    /* The private template: type 60 builds a type-60 template
       (0x004403d0) whatever its kind byte; type 70 builds one only for
       'F' (0x004eca90) or 'P' (0x004ec180). The save is the template's
       vtable +0x14 (its kind byte first). */
    int (*load_template)(void *context, AoeUnitRecordRor *r, uint8_t kind,
                         AoeOutcomeStream *in);
    int (*save_template)(void *context, const AoeUnitRecordRor *r,
                         AoeOutcomeStream *out);
    /* The unit AI: made through unit vtable +0x254 then loaded (its +0x08)
       / saved (its +0x04). */
    int (*load_ai)(void *context, AoeUnitRecordRor *r, AoeOutcomeStream *in);
    int (*save_ai)(void *context, const AoeUnitRecordRor *r,
                   AoeOutcomeStream *out);
} AoeUnitRecordOpsRor;

/* Loads the record of class `class_code` (10, 20, 30, 40, 50, 60, 70, 80
   or 90) saved at `version` (0x0055f804). Fields an older save lacks keep
   the values the caller set, except the defaults noted above. The
   dependent and group IDs skip duplicates as they are added. Returns 0
   for an unknown class, a short stream or a failed part. */
int aoe_unit_record_load_ror(AoeUnitRecordRor *r, uint8_t class_code,
                             float version, const AoeUnitRecordOpsRor *ops,
                             AoeOutcomeStream *in);
/* Saves the record (class byte first) at the current version. */
int aoe_unit_record_save_ror(const AoeUnitRecordRor *r,
                             const AoeUnitRecordOpsRor *ops,
                             AoeOutcomeStream *out);
/* Frees the record's arrays and movement storage. */
void aoe_unit_record_free_ror(AoeUnitRecordRor *r);

/* The constructors' field defaults for a class and its parents: type 30
   (0x0044ad30 / 0x0044aea0: both paths with room for 10 waypoints,
   status 2, +0x14c / +0x150 / +0x138 -1, +0x178 100000.0, facets 0xff,
   the rest 0), type 40 (0x004058a0 / 0x00405a60: +0x180 0; 0x00405b90 /
   0x00405be0 class 0x28), type 50 (0x00426140 / 0x004261a0: +0x18c..
   +0x1b0 and +0x1b8 0; +0x1b4 is the template's +0x120, left to the
   caller), type 60 (0x00448d30 / 0x00448de0 / 0x00448ec0: +0x1c0 0) and
   the class byte each sets (0x1e, 0x28, 0x32, 0x3c, 0x46, 0x50). */
void aoe_unit_record_defaults_ror(AoeUnitRecordRor *r, uint8_t class_code);

/* 0x004516e0(class, stream, world): the base game's loader of a saved
   object by class byte: 10 (root, 0x004a6590), 0x14 (0x004150c0), 0x19
   doppelganger (0x004361f0), 0x1e (0x0044aea0), 0x28 (0x00405a60), 0x32
   (0x00425fe0) and 0x3c (0x00448de0), each its load constructor run with
   the stream, the world and 1; any other class (and a failed allocation)
   gives NULL. `load` stands for the allocation and the constructor. */
void *aoe_unit_record_load_object_ror(uint8_t class_code,
                                      void *(*load)(void *context, uint8_t class_code),
                                      void *context);

/* The type-30 initialiser 0x0044b240 (0x0044b070 does the same after
   the defaults): class 0x1e, the velocity and facing cleared, then with
   a graphic whose sequence flag 2 is set and angle count n above 1 the
   facing is (2 pi / n) * ((rand() * n) / 0x7fff), wrapped into [0, 2 pi]
   (rand() is the C runtime's 0..0x7fff). The facet refresh (0x0044bbd0)
   is the caller's. */
void aoe_unit_record_init_facing_ror(AoeUnitRecordRor *r, int graphic_flag_2,
                                     int16_t angle_count, int32_t rand_value);

/* The destructors (0x0044afe0 type 30: both paths and the waypoint tiles
   freed; 0x00405ad0 type 40: the action list destroyed and freed (the
   caller's `free_actions`), then type 30's; 0x0044ae80 / 0x00405a40 the
   deleting forms) free what this record owns. */
void aoe_unit_record_destroy_ror(AoeUnitRecordRor *r,
                                 void (*free_actions)(void *context),
                                 void *context);

#endif
