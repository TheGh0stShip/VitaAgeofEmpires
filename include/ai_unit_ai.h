#ifndef VITA_AOE_AI_UNIT_AI_H
#define VITA_AOE_AI_UNIT_AI_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a per-class unit AIs (0x004e3b50..0x004e5dc0):
   the classes the unit factory gives units of a class, built
   over the shared unit AI base. This file keeps what the derived classes
   add: their target-class tables, timers and small overrides. The base
   itself is reached through the ops below. Semantic storage. */

enum {
    AOE_AI_UNIT_AI_CLASS_9,      /* 0x004e3b50, vtable 0x00548d7c */
    AOE_AI_UNIT_AI_CLASS_10,     /* 0x004e3e50, vtable 0x00548e5c */
    AOE_AI_UNIT_AI_TYPE_30,      /* 0x004e4040, vtable 0x00548f3c */
    AOE_AI_UNIT_AI_TYPE_7E,      /* 0x004e4170, vtable 0x0054901c */
    AOE_AI_UNIT_AI_CLASS_4,      /* 0x004e44d0, vtable 0x005490fc */
    AOE_AI_UNIT_AI_PRIEST,       /* 0x004e4e00 (class 0x12), 0x005491dc */
    AOE_AI_UNIT_AI_CLASS_2,      /* 0x004e56f0, vtable 0x005492c4 */
    AOE_AI_UNIT_AI_CLASS_14,     /* 0x004e5b40, vtable 0x005493a4 */
    AOE_AI_UNIT_AI_CLASS_15,     /* 0x004e5dc0, vtable 0x00549484 */
    AOE_AI_UNIT_AI_KIND_5564,    /* 0x004e60f0, vtable 0x00549564 */
    AOE_AI_UNIT_AI_KIND_5644,    /* 0x004e6900, vtable 0x00549644 */
    AOE_AI_UNIT_AI_KIND_5724,    /* 0x004e6d60, vtable 0x00549724 */
    AOE_AI_UNIT_AI_KIND_5804,    /* 0x004e6f40, vtable 0x00549804 */
    AOE_AI_UNIT_AI_KINDS
};

typedef struct {
    int kind;
    int32_t timer;            /* +0x100 */
    int32_t classes[24];      /* +0x124 (allocated), the target classes */
    int32_t class_count;      /* +0x128 */
    int32_t field_f8;         /* +0xf8, cleared by 0x004e6d20 */
} AoeAiUnitAiRor;

/* What the overrides read about a unit or target. */
typedef struct {
    int32_t id;
    int16_t type_id, type_class;
    float x, y, z;
    float field_44;     /* +0x44, the priest's faith */
    float type_28;      /* type +0x28 */
} AoeAiUnitViewRor;

typedef struct {
    void *context;
    /* The base constructor (with the unit and 10) and the
       timer restart the derived constructors call. */
    void (*base_init)(void *context);
    void (*restart_timer)(void *context, int32_t timer);
    /* World objects (0x005206d0 on the owner's world). */
    int (*object)(void *context, int32_t id, AoeAiUnitViewRor *out);
    /* The unit itself (the AI's +4). */
    void (*self)(void *context, AoeAiUnitViewRor *out);
    /* Base vtable calls: +0x40 the target search (-1 x4, 0), +0x9c move
       (x, y, z, 1), +0x5c (id, value), +0x64 (id, value), +0xc4 (id). */
    int32_t (*target)(void *context);
    void (*move)(void *context, float x, float y, float z);
    void (*call_5c)(void *context, int32_t id, int32_t value);
    void (*call_64)(void *context, int32_t id, int32_t value);
    int (*call_c4)(void *context, int32_t id);
    /* The base class's own +0x5c. */
    void (*base_5c)(void *context, int32_t id, int32_t value);
    /* The unit's vtable +0xfc (speed); the owner's +4 (0x00454150); the
       owner's resource `i` (+0x50); the game difficulty (cached); the
       settings 0x0041b700 / 0x0041b750; object distance (0x004aaeb0). */
    double (*speed)(void *context);
    int32_t (*owner_field_4)(void *context);
    float (*owner_resource)(void *context, int32_t i);
    int32_t (*difficulty)(void *context);
    int32_t settings_8fc, settings_97d;
    double (*distance_to)(void *context, int32_t id);
    /* 0x00412ed0 (clear the target), base vtable +0x58 (1), +0x1c
       (message), the base class's own +0xcc (message, arg), +0x6c (id,
       value), +0x94 (id, f) and the unit's vtable +0x10c (value). */
    void (*clear_target)(void *context);
    void (*call_58)(void *context, int32_t value);
    void (*call_1c)(void *context, void *message);
    int32_t (*base_cc)(void *context, void *message, int32_t arg);
    int32_t (*call_6c)(void *context, int32_t id, int32_t value);
    int32_t (*call_94)(void *context, int32_t id, float f);
    double (*unit_10c)(void *context, int32_t value);
    /* The base destructor 0x0040ef80 (through its thunk 0x004e5bf0) and
       the allocator's free (0x00525fc2) of the object itself. */
    void (*base_destroy)(void *context);
    void (*free_self)(void *context);
    /* Virtual +0xd4 (with 1); clear_target and call_58 above serve too. */
    int32_t (*call_d4)(void *context, int32_t arg);
} AoeAiUnitAiOpsRor;

/* The constructors: the base, then the target-class table (and, for
   class 9 and type 0x7e, a 4000 / 6000 ms timer that is restarted).
   0x004e60f0, 0x004e6d60 and 0x004e6f40 take the 21 classes 0, 3, 4,
   0x1a, 0x18, 6, 0x23, 0x19, 0x11, 0xc, 0x17, 0x24, 0x1c, 0x12, 0xd,
   0x1b, 0x16, 0x14, 2, 0x15, 0x27 (the first with a 1500 ms timer);
   0x004e6900 the 19 without 3 and 0x1b and a 5000 ms timer. */
void aoe_ai_unit_ai_init_ror(AoeAiUnitAiRor *ai, int kind,
                             const AoeAiUnitAiOpsRor *ops);

/* Fixed overrides: 0x004e3fa0 (vtable +0x54) 1, 0x004e4160 (+0xd4) 5,
   0x004e4df0 (+0xd8) 9. */
int32_t aoe_ai_unit_ai_slot_54_ror(void);
int32_t aoe_ai_unit_ai_slot_d4_ror(void);
int32_t aoe_ai_unit_ai_slot_d8_ror(void);
/* 0x004e4690 (class 4, vtable +0xc0): the resource a target class gives:
   0xf wood 0, 7 food 1, 5 / 0x1f / 0x21 2, 8 stone 3, 0x20 gold 4, else
   -1. */
int32_t aoe_ai_unit_ai_resource_of_ror(int32_t type_class);
/* 0x004e4600 (class 4, +0x5c): targets of class 9, 10 or 0xf go to
   +0x64, others to the base's +0x5c; nothing for a missing object. */
void aoe_ai_unit_ai_class4_5c_ror(int32_t id, int32_t value,
                                  const AoeAiUnitAiOpsRor *ops);
/* 0x004e5090 (priest, +0x50): +0xc4 on the object's ID, 0 for none. */
int aoe_ai_unit_ai_priest_50_ror(int has_object, int32_t id,
                                 const AoeAiUnitAiOpsRor *ops);
/* 0x004e4fe0 (priest, +0xc4): with faith at least 100 and an object
   that is not type 0x6d, 0x9f, 0x114, 0x9e or 0xa3 nor class 0x1b or
   0x1e: priests need owner resource 27, class 3 resource 28 above 0;
   others yes. */
int aoe_ai_unit_ai_can_convert_ror(int32_t id, const AoeAiUnitAiOpsRor *ops);
/* 0x004e3da0 (class 9, +0xd4) / 0x004e5a80 (classes 2, 0x14, 0x15; only
   for a computer owner): step back from the target by speed * 3 (or 7)
   along the line from it, keeping z; 6 when moved, else 5. */
int32_t aoe_ai_unit_ai_back_off_ror(const AoeAiUnitAiRor *ai,
                                    const AoeAiUnitAiOpsRor *ops);
/* 0x004e43e0 (type 0x7e, +0xd4): restart a 6000 ms timer; with a target
   within the unit's reach (its type +0x28 x 2, or with both settings 1 x
   0.75 / 0.5
   for difficulty 3 / above, not below 1.0) call +0x5c(target, 0): 6;
   else 5. */
int32_t aoe_ai_unit_ai_7e_d4_ror(AoeAiUnitAiRor *ai,
                                 const AoeAiUnitAiOpsRor *ops);

/* 0x004e3fb0 (class 10, vtable +0xcc): for message 0x1fc (+8) with the
   target (+0x34) present and within the unit type's +0x28 reach (an
   unordered, NaN distance counts as within): +0x5c(target, 1), 3.
   Otherwise for 0x1fc: clear the target, +0x58(1), +0x1c(message). Then
   the base's +0xcc(message, arg). */
int32_t aoe_ai_unit_ai_class10_cc_ror(int32_t target, int32_t message_type,
                                      void *message, int32_t arg,
                                      const AoeAiUnitAiOpsRor *ops);
/* 0x004e4f50 (priest, +0x5c): convertible (+0xc4) targets go to
   +0x6c(id, value); else with no current action (+0x28 == -1) and an
   owner with +4 set, +0x94(id, (float)unit +0x10c(value)); else 0. The
   +0x6c and +0x94 results are returned. */
int32_t aoe_ai_unit_ai_priest_5c_ror(int32_t field_28, int32_t id,
                                     int32_t value,
                                     const AoeAiUnitAiOpsRor *ops);
/* 0x004e5e90, vtable +0x00 of all four 0x00549564 / 0x00549644 /
   0x00549724 / 0x00549804 classes: the scalar deleting destructor. Runs
   the base destructor (here also emptying the class list), then frees
   the object only when bit 0 of `flags` is set; other bits are ignored.
   Returns the object, as the original returns `this`. */
AoeAiUnitAiRor *aoe_ai_unit_ai_delete_ror(AoeAiUnitAiRor *ai, uint8_t flags,
                                          const AoeAiUnitAiOpsRor *ops);
/* 0x004e6d20 (0x00549644 only, vtable +0xd8): virtual +0xd4(1); on 6
   clear the target, call +0x58(1), zero +0xf8 and return 10; any other
   result returns 9 with nothing changed. The +0xd4 slot is called with
   an argument here, whatever override the vtable holds. */
int32_t aoe_ai_unit_ai_5644_d8_ror(AoeAiUnitAiRor *ai,
                                   const AoeAiUnitAiOpsRor *ops);

#endif
