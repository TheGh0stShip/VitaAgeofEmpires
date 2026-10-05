#ifndef VITA_AOE_UNIT_AI_BASE_H
#define VITA_AOE_UNIT_AI_BASE_H

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
} AoeUnitAiOpsRor;

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

/* 0x00412f90 a target class allowed (+0x124 list); 0x00413190 an ID in
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

#endif
