#ifndef VITA_AOE_ACTION_IO_H
#define VITA_AOE_ACTION_IO_H

#include <stdint.h>

#include "player_outcome.h" /* AoeOutcomeStream */

/* Patched Rise of Rome 1.0a action save and load: the base action's
   fields (0x00406f10 load, 0x00407160 save), the action list (0x00403d80
   load, 0x00403e10 save; a type 0 ends it), the two list factories
   (0x00403e70 base, 0x004b41a0 RoR) and each action type's own fields.
   Objects, tasks and graphics are written as IDs (-1 for none) and kept
   as IDs here; resolving them is the caller's (the original resolves the
   task through the unit's template +0xd8 and the graphic through
   0x004a95b0). */

typedef struct AoeActionRecordRor {
    int16_t type;              /* +0x04 */
    uint8_t state;             /* +0x0c */
    int32_t target_id;         /* +0x10 (as an ID; -1 none) */
    int32_t target2_id;        /* +0x14 */
    int32_t field_18, field_1c;/* +0x18, +0x1c */
    uint32_t field_20, field_24, field_28, field_2c; /* +0x20..+0x2c */
    int16_t task;              /* +0x30: the template's task index, -1 */
    uint8_t field_3c;          /* +0x3c */
    int16_t graphic;           /* +0x38: the graphic's ID (+0x72), -1 */
    struct AoeActionRecordRor *subs; /* +0x34: the sub-action list */
    int32_t sub_count;
    /* Type fields (see the per-type functions). */
    uint32_t u32[5];
    int16_t i16[2];
    uint8_t u8[3];
    int16_t graphics[3];
    uint8_t *bytes;            /* type 0x6c: one byte per world player */
    int32_t byte_count;
} AoeActionRecordRor;

/* 0x00406e40 (the load constructor: +0x18 / +0x1c -1, then 0x00406f10
   when asked) and 0x00406f10: the base fields after the type, the sub
   list and the graphic. 0x00407160: the save, starting with the type.
   0x00406ed0 (and its thunk 0x004b2bf0) releases the sub list;
   the type 0x66 destructor 0x004b4f50 is the same, as are 0x00401380
   (vtable 0x005423f8) and 0x00402620 (0x005424d4); 0x00406e80 is the
   deleting destructor (0x00401130 and 0x00402420 for the other two,
   0x004b2860 over the thunk, 0x004b4ef0 for type 0x66). */
int aoe_action_load_base_ror(AoeActionRecordRor *a, AoeOutcomeStream *s, int ror,
                             int32_t player_count, float version);
void aoe_action_save_base_ror(const AoeActionRecordRor *a, AoeOutcomeStream *s, int ror);
void aoe_action_release_ror(AoeActionRecordRor *a);

/* 0x00403d80: the list (replacing what it held): types until 0, each
   made by the factory; 0x00403e10: each action's save, then 0.
   0x00403d50 / 0x00403d70 construct and destroy the list (0x004b4180
   the RoR list, whose factory is 0x004b41a0). */
int aoe_action_list_load_ror(AoeActionRecordRor **list, int32_t *count, AoeOutcomeStream *s,
                             int ror, int32_t player_count, float version);
void aoe_action_list_save_ror(const AoeActionRecordRor *list, int32_t count, AoeOutcomeStream *s,
                              int ror);

/* Factories: 0x00403e70 (base: 1, 4, 5, 8, 9, 10, 0x15) and 0x004b41a0
   (RoR: 3, 0xc, 0x65..0x6c, 0x6e, 0x6f, 0x78, else the base factory).
   Each type's load constructor reads the base and then:
   1 0x004051c0 / 5 0x00402e80 / 0x6e 0x004b3250: one dword (+0x40), saved
     by 0x00405400;
   8 0x004048f0: five dwords (+0x40..+0x50), saved by 0x00405160;
   9 0x00401000: +0x4c, +0x50, +0x54 (2), +0x56 (2), +0x58..+0x5a (1
     each), then three graphic IDs (+0x40, +0x44, +0x48), saved by
     0x00401390;
   0x66 0x004b4e30: +0x40 (2), +0x44, +0x48, +0x4c (1, from 7.23; else
     0), saved by 0x004b4f60;
   0x67 0x004b6010: +0x40 (2), +0x44, saved by 0x004b61d0;
   0x68 0x004b1c30: +0x40 (1), +0x44 (1.0 before), saved by 0x004b1d90;
   0x6a 0x004b5550 / 0x004b56a0: +0x40 (1, from 6.5; else 0), saved by
     0x004b56f0;
   0x6c 0x004b27d0: one byte per world player, saved by 0x004b2960;
   0x6f 0x004b64a0: +0x40, +0x44, saved by 0x004b6600;
   3 0x004023f0, 4 0x00402b50, 10 0x004020b0, 0xc 0x004067a0, 0x15
   0x00404720, 0x65 0x004b1390, 0x69 0x004b2bc0, 0x6b 0x004b0dc0, 0x78
   0x004b71d0: nothing more (saved by 0x00407160). 0 for an unknown type
   (the list stops reading meaningfully, as in the original). */
int aoe_action_load_ror(AoeActionRecordRor *a, int16_t type, AoeOutcomeStream *s, int ror,
                        int32_t player_count, float version);
void aoe_action_save_ror(const AoeActionRecordRor *a, AoeOutcomeStream *s, int ror);

/* Base action slots (vtable 0x005423f8 and the action vtables that keep
   them): +0x4c 0x00401110 the target (+0x10), +0x50 0x00401120 the
   second target (+0x14); +0x20 / +0x24 0x004072f0 drops a target whose
   unit is past state 6 (through +0x54(0) / +0x58(0)) and returns 1.
   `state` reports a target's unit state (-1 when it has none). */
int32_t aoe_action_target_ror(const AoeActionRecordRor *a);
int32_t aoe_action_target2_ror(const AoeActionRecordRor *a);
int aoe_action_drop_dead_targets_ror(AoeActionRecordRor *a,
                                     int (*state)(void *context, int32_t id), void *context);

#endif
