#ifndef VITA_AOE_UNIT_AI_BASE_H
#define VITA_AOE_UNIT_AI_BASE_H

#include "aoe_stream.h"
#include <stdint.h>

/* Patched Rise of Rome 1.0a unit AI base (vtable 0x00542d10,
   0x0040edf0..0x00414a50): the order and target bookkeeping and the
   order methods that hand a request to the unit (its vtable) and, when
   the unit takes it (returns 1), record the order code and target.
   Semantic storage; offsets noted. The unit, the world's objects and the
   random numbers are ops. */

/* Order codes (+0x30). */
enum {
    AOE_UNIT_AI_ORDER_NONE = -1,
    AOE_UNIT_AI_ORDER_ATTACK = 0x258,
    AOE_UNIT_AI_ORDER_259 = 0x259, AOE_UNIT_AI_ORDER_25A = 0x25a,
    AOE_UNIT_AI_ORDER_25B = 0x25b, AOE_UNIT_AI_ORDER_25C = 0x25c,
    AOE_UNIT_AI_ORDER_25D = 0x25d, AOE_UNIT_AI_ORDER_261 = 0x261,
    AOE_UNIT_AI_ORDER_MOVE = 0x262, AOE_UNIT_AI_ORDER_264 = 0x264,
    AOE_UNIT_AI_ORDER_265 = 0x265, AOE_UNIT_AI_ORDER_266 = 0x266,
    AOE_UNIT_AI_ORDER_267 = 0x267, AOE_UNIT_AI_ORDER_269 = 0x269,
    AOE_UNIT_AI_ORDER_26A = 0x26a, AOE_UNIT_AI_ORDER_26D = 0x26d,
    AOE_UNIT_AI_ORDER_26E = 0x26e, AOE_UNIT_AI_ORDER_26F = 0x26f
};

typedef struct {
    int32_t action, field_2c;          /* +0x28, +0x2c */
    int32_t order;                     /* +0x30 */
    int32_t target_id, target_type;    /* +0x34, +0x38 */
    float target_x, target_y, target_z;/* +0x3c..+0x44 */
    int32_t field_48, field_4c;        /* +0x48, +0x4c */
    int32_t last_action, last_order;   /* +0x50, +0x54 */
    int32_t last_target_id, last_target_type; /* +0x58, +0x5c */
    int32_t *group;                    /* +0x60 */
    int32_t group_count;               /* +0x64 */
    /* +0x70: eight waypoints of 16 bytes (x +0x70, y +0x74), next +0xf0. */
    float waypoint_x[8], waypoint_y[8];
    int32_t waypoint_next;             /* +0xf0 */
    uint32_t timer;                    /* +0xfc */
    uint32_t timer_base;               /* +0x100 */
    int32_t *classes;                  /* +0x124 */
    int32_t class_count;               /* +0x128 */
    /* +0x18 table (count +0x10) of 0x24-byte entries, +4 an object ID. */
    const int32_t *table_ids;
    int32_t table_count;
    /* The saved record's other fields (0x0040f040 / 0x0040f320). */
    int32_t field_8;                   /* +0x08 */
    int32_t field_f4, field_f8;        /* +0xf4, +0xf8 */
    int32_t field_104, field_108, field_10c; /* +0x104, +0x108 (0 before 6.93), +0x10c (1000) */
    int32_t field_110[4];              /* +0x110 */
    int32_t field_120;                 /* +0x120 */
    uint8_t byte_130;                  /* +0x130 (saves from 7.02) */
    int has_members;                   /* +0x12c: the members record (ai_tactics.h) */
} AoeUnitAiRor;

/* What the order methods read of an object (world 0x005206d0). */
typedef struct {
    int16_t type;      /* template +0x14 */
    float x, y, z;     /* +0x38..+0x40 */
} AoeUnitAiObjectRor;

typedef struct {
    void *context;
    int (*object)(void *context, int32_t id, AoeUnitAiObjectRor *out);
    /* The unit (+0x04): its ID, type, position and owner's visibility
       masks (+0xd4 explored, +0xd8 visible); world map size. */
    int32_t unit_id;
    int16_t unit_type;
    float unit_x, unit_y, unit_z;
    uint32_t explored_mask, visible_mask;
    uint32_t (*visibility)(void *context, int32_t x, int32_t y); /* 0x007d205c */
    int32_t map_w, map_h;
    /* Unit vtable calls by slot: a target (id, force), a point
       (x, y, z, extra, force); 0x10c range, 0x194 in range, 0x114 free
       tile, 0xfc speed, 0x18c stop. */
    int (*unit_target)(void *context, int slot, int32_t id, int32_t extra, int force);
    int (*unit_point)(void *context, int slot, float x, float y, float z, int32_t extra,
                      int force);
    float (*unit_range)(void *context);
    int (*unit_in_range)(void *context, int32_t id, float range);
    int (*unit_free_tile)(void *context, float x, float y);
    float (*unit_speed)(void *context);
    void (*unit_stop)(void *context);
    /* Our own vtable: +0x50 can act on, +0x58 stop. */
    int (*can_act_on)(void *context, const AoeUnitAiObjectRor *object, int32_t id);
    void (*self_stop)(void *context, int force);
    int (*rand)(void *context);                                 /* 0x0052605d */
    /* The unit's owner index (the original's 0x0040f6a0 returns the owner
       pointer, unit +0x0c), its group count (+0x68) and leader ID
       (+0x78); a unit AI's vtable +0xd0 with a 6-value record (ours for
       `id` -1, else that object's unit AI when it has one); the owner's
       vtable +0xe8 with the values. */
    int16_t unit_owner;
    int32_t (*unit_group_count)(void *context);
    int32_t (*unit_leader)(void *context);
    void (*ai_d0)(void *context, int32_t id, const int32_t values[6]);
    void (*owner_e8)(void *context, const int32_t values[6]);
} AoeUnitAiOpsRor;

/* 0x0040f6a0: the unit's owner (unit +0x0c). */
int16_t aoe_unit_ai_owner_ror(const AoeUnitAiOpsRor *ops);
/* Unit AI vtable +0x18 = 0x00410a20(six values): a unit in a group
   (0x004abc70: +0x68 above 0) hands the record to its group: through our
   own +0xd0 when it leads (+0x78 is its ID), else through the leader's
   unit AI (0x00414f40) when there is one. The owner hears it (+0xe8)
   either way; 1. +0x1c = 0x004109f0 unpacks a record into it. */
int aoe_unit_ai_post_ror(const AoeUnitAiOpsRor *ops, const int32_t values[6]);
int aoe_unit_ai_post_record_ror(const AoeUnitAiOpsRor *ops, const int32_t record[6]);

/* Bookkeeping: 0x0040f6b0 / 0x0040f6c0 the action (+0x28; the old one to
   +0x50), 0x0040f6d0 the order (+0x30; the old one to +0x54),
   0x0040f6e0 target ID, 0x0040f6f0 target type, 0x00412ed0 clear the
   target (ID and type kept as the last; -1, position -1), 0x00412f00 set
   it (ID, position; the type from the object when there is one),
   0x00412f50 set all of it. */
void aoe_unit_ai_set_action_ror(AoeUnitAiRor *ai, int32_t action);
void aoe_unit_ai_set_order_ror(AoeUnitAiRor *ai, int32_t order);
void aoe_unit_ai_clear_target_ror(AoeUnitAiRor *ai);
void aoe_unit_ai_set_target_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t id,
                                float x, float y, float z);
void aoe_unit_ai_set_target_full_ror(AoeUnitAiRor *ai, int32_t id, int32_t type, float x,
                                     float y, float z);

/* 0x00411c60 a waypoint (eight kept, the oldest replaced), 0x00411cb0
   is (x, y) one of them. */
int aoe_unit_ai_add_waypoint_ror(AoeUnitAiRor *ai, int32_t x, int32_t y);
int aoe_unit_ai_has_waypoint_ror(const AoeUnitAiRor *ai, int32_t x, int32_t y);

/* +0x58 0x00411d00: stop. The order goes to +0x54; unforced, or when
   the action is the order + 100, the action ends (to +0x50, -1, +0x2c
   -1); the unit stops (+0x18c); the order is -1. */
int aoe_unit_ai_stop_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int force);

/* Order methods. Each one runs when forced or as noted; on success it
   sets the order and the target and returns 1, else 0.
   +0x5c 0x00411d50 attack (when idle): +0x50 must allow the object,
     unit +0x150(id, force) -> ATTACK.
   +0x60 0x00411dd0 attack a new target (unforced): +0x50; while busy
     the unit must be in range (+0x194 with +0x10c(0, 0, -1, -1));
     +0x150(id, 1) -> ATTACK.
   +0x64 0x00411e80 (unless already 0x265): we stop first (+0x58(1));
     +0x164 -> 0x265. +0x6c 0x00411f20 +0x16c -> 0x25c. +0x70 0x00411fc0
     +0x88 -> 0x25b. +0x68 0x00412060 +0x168 -> 0x261. +0x74 0x00412100
     +0x170 -> 0x26a. +0x78 0x004121a0 +0x174 -> 0x25a. +0x7c 0x00412240
     +0x178 -> 0x267 (each unless already that order); the target type
     -1 and position 0 without the object.
   The rest run when forced or idle:
   +0x80 0x004122e0 +0x17c(x, y) -> 0x25d at (x, y, 1);
   +0xb8 0x00412e60 +0x17c(x, y) -> 0x26f at (x, y, 1);
   +0xb4 0x00412df0 +0x150(id) -> 0x26e at (id, x, y, 1);
   +0x84 0x00412350 +0x180(id) -> 0x269 at (id, -1, -1, -1);
   +0x8c 0x004123b0 +0x188(x, y, z) -> 0x266 at (x, y, z);
   +0x88 0x00412420 +0x184(a, b, c, unit +0x40) -> 0x26d at the unit;
   +0x98 0x004124c0 +0x15c(id) and +0x94 0x00412550 +0x158(id, extra)
     -> MOVE at the object (position 0 without it);
   +0x90 0x004125e0 +0x154(x, y, z, extra) -> MOVE at (x, y, z);
   +0x9c 0x00412650 a move to (x, y), clamped to the map, moved up to
     five times by -3..3 in each axis (rand) until the unit finds the
     tile free (+0x114), then +0x154(x, y, z, 0) -> MOVE;
   +0xa8 0x00412cb0 (object) -> 0x264, +0x48 = extra; +0xac 0x00412d20
     (object) -> 0x259, +0x4c = id, +0x48 = extra; +0xb0 0x00412d90 (x,
     y) -> 0x259 at the unit's height, +0x4c = -1, +0x48 = extra (these
     three set the order themselves);
   +0xa4 0x00412b60 move away from the group: the sum of the members'
     offsets from us, normalised, times speed x 4, subtracted from our
     position, through our +0x9c (0x00412650, forced); unforced only when
     idle. */
int aoe_unit_ai_order_target_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int slot,
                                 int32_t id, int force);
int aoe_unit_ai_attack_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t id,
                           int force);
int aoe_unit_ai_attack_new_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t id);
int aoe_unit_ai_order_xy_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t order,
                             int32_t x, int32_t y, int force);
int aoe_unit_ai_order_id_xy_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t id,
                                int32_t x, int32_t y, int force);
int aoe_unit_ai_order_269_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t id,
                              int force);
int aoe_unit_ai_order_266_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, float x, float y,
                              float z, int force);
int aoe_unit_ai_order_26d_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t a,
                              int32_t b, int32_t c);
int aoe_unit_ai_move_to_object_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int slot,
                                   int32_t id, int32_t extra, int force);
int aoe_unit_ai_move_to_point_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, float x,
                                  float y, float z, int32_t extra, int force);
int aoe_unit_ai_move_near_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, float x, float y,
                              float z, int force);
int aoe_unit_ai_follow_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int32_t order,
                           int32_t id, int32_t extra, int force);
int aoe_unit_ai_follow_point_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, float x,
                                 float y, int32_t extra, int force);
int aoe_unit_ai_move_away_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops, int force);

/* 0x00410c00: 1 when +0x130 is 1 or the given order is ATTACK, 0x259,
   0x25b, 0x25c, 0x26a or 0x264. 0x00414f70: printf-style text (1 KB)
   handed to the unit's +0x144 when there is a unit.
   0x00412f90 a target class allowed (+0x124 list); 0x00413190 an ID in
   the +0x18 table; 0x00412fd0 the owner's view of a tile (0xf visible,
   0x80 explored, 0); 0x00413010 the timer redrawn (rand % (base / 2) +
   3 base / 4). */
int aoe_unit_ai_class_allowed_ror(const AoeUnitAiRor *ai, int32_t cls);
int aoe_unit_ai_table_has_ror(const AoeUnitAiRor *ai, int32_t id);
uint8_t aoe_unit_ai_tile_view_ror(const AoeUnitAiOpsRor *ops, int32_t x, int32_t y);
void aoe_unit_ai_reset_timer_ror(AoeUnitAiRor *ai, const AoeUnitAiOpsRor *ops);

/* 0x00414f40: a world object by ID (0x005206d0) when the unit, its owner
   and the world exist (here: when `object` is set); 0 otherwise. */
int aoe_unit_ai_object_ror(const AoeUnitAiOpsRor *ops, int32_t id,
                           AoeUnitAiObjectRor *out);

/* 0x00414fc0: an ID array (data +0x00, capacity +0x0c) reallocated to
   `size` entries, the first min(capacity, size) copied, the old freed.
   0 when the allocation fails (the array is kept); host-only, a size
   below 1 is refused. */
int aoe_unit_ai_resize_ids_ror(int32_t **data, int32_t *capacity,
                               int32_t size);

/* 0x0040ef80 the destructor (deleting 0x0040ef60; 0x004e5bf0 is a thunk
   to it): the +0x18, +0x24,
   +0x124 and +0x12c arrays freed and the group (+0x60, its count and
   capacity) freed and emptied. Here: the group and the class list. */
void aoe_unit_ai_release_ror(AoeUnitAiRor *ai);

/* 0x00410930 (a notification for this unit; the unit's state +0x48):
   0 dropped (state not 2; code 699 from a sender outside the group; code
   0x1fb whose value is not the current order), 1 taken without passing
   it on (code 0x1fa with value 0x262 while the order is neither 0x262 nor
   0x268), 2 passed on. */
int aoe_unit_ai_notify_filter_ror(const AoeUnitAiRor *ai, uint8_t unit_state, int32_t sender,
                                  int32_t code, int32_t value);

/* The unit AI's notification queue (+0x10 count, +0x14 capacity, +0x18
   records of 0x24 bytes: sender, code, extra, then six values) and its
   outgoing queue (+0x1c, +0x20, +0x24: records of six dwords). Both
   double when full (0 when the allocation fails, the queue unchanged; host-only, a queue of capacity 0 is refused: the
   original would double 0 and write past its empty array). 0x00414b00
   appends; with `front` and a non-empty queue it inserts first, copying
   each record onto the next from the front (as the original does), so
   the older records all become copies of the old first one. 0x00414ac0
   unpacks a packed record into it. 0x00414d90 appends to the outgoing
   queue; 0x00414d60 unpacks a packed one. */
typedef struct {
    int32_t (*records)[9];
    int32_t count, capacity;
} AoeUnitAiNotesRor;
typedef struct {
    int32_t (*records)[6];
    int32_t count, capacity;
} AoeUnitAiOutRor;
int aoe_unit_ai_note_ror(AoeUnitAiNotesRor *q, int32_t sender, int32_t code, int32_t extra,
                         const int32_t values[6], int front);
/* Base vtable +0x10 = 0x00410830(sender, code, six values, now, requeue,
   extra): only for a unit below state 3 (else nothing). Not `now`: the
   order is queued (first with `requeue`). Now: with `requeue` 1 the
   current order (our unit, action +0x28, extra +0x2c, values target
   +0x34, -1, the target point's raw +0x3c..+0x44 and +0x48) goes first,
   then the new one before it; an order other than 0x2d9 clears the target
   (0x00412ed0) and stops (+0x58 with 1); the queue is then run
   (0x00414eb0 with 0) and the first record handed to +0xc8 with 0.
   Always 1. */
typedef struct {
    void *context;
    void (*self_stop)(void *context, int force);                    /* +0x58 */
    void (*run_notes)(void *context, int32_t arg);                  /* 0x00414eb0 */
    void (*handle_c8)(void *context, const int32_t record[9], int32_t arg);
} AoeUnitAiOrderOpsRor;
int aoe_unit_ai_order_ror(AoeUnitAiRor *ai, AoeUnitAiNotesRor *q, uint8_t unit_state,
                          int32_t unit_id, int32_t sender, int32_t code, const int32_t values[6],
                          int32_t now, int32_t requeue, int32_t extra,
                          const AoeUnitAiOrderOpsRor *ops);
int aoe_unit_ai_send_ror(AoeUnitAiOutRor *q, const int32_t values[6]);


/* The unit AI record, save 0x0040f040 and load 0x0040f320 (version
   0x0055f804): +0x08, then +0x28..+0x48, +0x54, +0x50, +0x58, +0x5c,
   +0xf4, +0xf8, +0x100, +0xfc, +0x104, +0x108, +0x10c (the last two read
   only from 6.93, else 0 / 1000), +0x4c, +0x120, the 16 bytes at +0x110,
   the notification queue (count, 0x24-byte records, each unpacked on
   load by 0x00414ac0), the outgoing queue (count, 0x18-byte records,
   0x00414d60), the group IDs (count then each; duplicates skipped on
   load), a 1 and the members record (0x0040d2f0 / 0x0040d4f0 /
   0x0040d5b0, ai_tactics.h) or 0, and the +0x130 byte (from 7.02). The
   members record goes through `members_save` / `members_load`. */
typedef struct {
    void *context;
    int (*members_save)(void *context, AoeOutcomeStream *out);
    int (*members_load)(void *context, AoeOutcomeStream *in);
} AoeUnitAiRecordOpsRor;
int aoe_unit_ai_save_ror(const AoeUnitAiRor *ai, const AoeUnitAiNotesRor *notes,
                         const AoeUnitAiOutRor *out_queue, const AoeUnitAiRecordOpsRor *o,
                         AoeOutcomeStream *out);
int aoe_unit_ai_load_ror(AoeUnitAiRor *ai, AoeUnitAiNotesRor *notes, AoeUnitAiOutRor *out_queue,
                         float version, const AoeUnitAiRecordOpsRor *o, AoeOutcomeStream *in);

#endif
