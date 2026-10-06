#ifndef VITA_AOE_ACTION_BASE_H
#define VITA_AOE_ACTION_BASE_H

#include <stdint.h>

#include "action_repair.h"

/* Patched Rise of Rome 1.0a base-list actions not yet translated
   elsewhere: enter (3, vtable 0x005424d4), explore (4, 0x00542538), fly
   (10, 0x00542474) and transport/unload (12, 0x00542970). */
typedef struct {
    AoeMoveActionRor base;
    float timer;             /* +0x2c */
    const DatUnitTask *task; /* +0x30, may be NULL for transport */
    uint8_t unload_flag;     /* transport +0x1c (written over the base
                                secondary ID slot) */
} AoeBaseActionRor;

typedef struct {
    AoeMoveActionOps move; /* resolve, target hooks, unit_notice,
                              select_graphic, clear_secondary */
    const void *(*graphic)(void *context, int16_t id);
    float (*tick_seconds)(void *context);
    uint16_t (*rand)(void *context);                          /* 0x0052605d */
    int32_t (*map_width)(void *context);
    int32_t (*map_height)(void *context);
    /* Point walk 0x00405250(unit, x, y, z, range, graphic) on the
       sub-action list; quiet before (1) or after (0) the push, or never
       (-1). */
    int (*start_move)(void *context, AoeMoveActionRor *action, float range,
                      const void *graphic, int quiet);
    uint8_t (*sub_update)(void *context, AoeMoveActionRor *action);
    void (*sub_clear)(void *context, AoeMoveActionRor *action);
    /* Unit +0x34(x, y, z): place (fly raises the unit to 6). */
    void (*place)(void *context, AoeLiveUnit *unit, float x, float y,
                  float z);
    /* Enter. */
    double (*distance)(void *context, AoeLiveUnit *unit,
                       AoeLiveUnit *target);                  /* 0x004aaeb0 */
    double (*distance_to_point)(void *context, AoeLiveUnit *unit, float x,
                                float y, float z);            /* 0x004aaf50 */
    int (*allied)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target);
    int32_t (*owner_kind)(void *context, AoeLiveUnit *unit);  /* 0x00454150 */
    uint8_t (*head_state)(void *context, AoeLiveUnit *unit);  /* +0x134 */
    /* The head action's +0x1c (secondary ID) and destination 0x00407670 /
       0x00407680. */
    int32_t (*head_secondary)(void *context, AoeLiveUnit *unit);
    void (*head_destination)(void *context, AoeLiveUnit *unit, float *x,
                             float *y);
    int (*move_to)(void *context, AoeLiveUnit *unit, float x, float y,
                   float z, float range, int force);          /* +0x154 */
    int (*enter)(void *context, AoeLiveUnit *unit,
                 AoeLiveUnit *container);                     /* +0xd0 */
    AoeLineSearchOps line;                                    /* +0x1b4 */
    /* Transport. */
    int (*zone_at)(void *context, AoeLiveUnit *unit, int32_t x, int32_t y);
    /* 0x004abb60(zone, 2.0): a tile of the zone within 2 of the unit. */
    int (*zone_near)(void *context, AoeLiveUnit *unit, int zone,
                     float distance);
    int (*terrain_at)(void *context, int32_t x, int32_t y);  /* 0x00447de0 */
    /* 0x004ab280: unload every passenger; the count unloaded. */
    uint8_t (*unload)(void *context, AoeLiveUnit *unit, int zone);
    uint16_t (*contents)(void *context, AoeLiveUnit *unit);  /* +0x24 +8 */
    /* Owner +0xe8(id, id, code, 0x266, 0, 0). */
    void (*owner_notice)(void *context, AoeLiveUnit *unit, int32_t code);
} AoeBaseActionOps;

/* Fly (10): constructor 0x004020e0 (point). +0x1c = 0x00402120 (`arg`
   lifts the unit to z 6), +0x5c = 0x00402160 (4/6 set a 5..15 s timer
   and show the task's moving / proceeding graphic, or standing; other
   states clear the walk), +0x28 = 0x00402210 (3 flies to a random map
   point with the current graphic; 4/6 alternate when the timer runs out;
   an arrived walk returns to 3). */
int aoe_fly_action_init_ror(AoeBaseActionRor *action, AoeLiveUnit *unit,
                            const DatUnitTask *task, float x, float y,
                            float z);
int aoe_fly_action_set_state_ror(AoeBaseActionRor *action, uint8_t state,
                                 const AoeBaseActionOps *ops);
int aoe_fly_action_activate_ror(AoeBaseActionRor *action, int arg,
                                const AoeBaseActionOps *ops);
int aoe_fly_action_update_ror(AoeBaseActionRor *action,
                              const AoeBaseActionOps *ops, uint8_t *result);

/* Enter (3): constructor 0x00402440 (a target: its position; an idle
   boat-like target (head state 2) with room and owner kind 0, or owner
   kind 1, is called to a beach tile on the line (+0x1b4) or to the unit;
   a target walking (head state 11) without a head secondary gives its
   destination). +0x1c = 0x00402630 (4), +0x5c = 0x00402640 (2 waits 1 s;
   4 walks to the point, quiet after), +0x28 = 0x00402760 (within task
   range + 1.0 the unit enters (+0xd0), notice 0x1fa unless quiet, state 1,
   result 2; a lost, dying or unallied target: state 1, notice 0x1fb,
   result 4). Notices carry 0x269. */
int aoe_enter_action_init_ror(AoeBaseActionRor *action, AoeLiveUnit *unit,
                              const DatUnitTask *task, AoeLiveUnit *target,
                              const AoeBaseActionOps *ops);
int aoe_enter_action_set_state_ror(AoeBaseActionRor *action, uint8_t state,
                                   const AoeBaseActionOps *ops);
int aoe_enter_action_activate_ror(AoeBaseActionRor *action,
                                  const AoeBaseActionOps *ops);
int aoe_enter_action_update_ror(AoeBaseActionRor *action,
                                const AoeBaseActionOps *ops,
                                uint8_t *result);

/* Explore (4): constructor 0x00402b80; +0x1c = 0x00402bc0 (no point: 3,
   else walk 11), +0x5c = 0x00402be0, +0x28 = 0x00402d70, +0x34 =
   0x00402e20 (a nonnegative point walks, else idle). Notices 0x25d. */
int aoe_explore_action_init_ror(AoeBaseActionRor *action, AoeLiveUnit *unit,
                                const DatUnitTask *task, float x, float y,
                                float z);
int aoe_explore_action_set_state_ror(AoeBaseActionRor *action,
                                     uint8_t state,
                                     const AoeBaseActionOps *ops);
int aoe_explore_action_activate_ror(AoeBaseActionRor *action,
                                    const AoeBaseActionOps *ops);
int aoe_explore_action_retarget_ror(AoeBaseActionRor *action, float x,
                                    float y, float z,
                                    const AoeBaseActionOps *ops);
int aoe_explore_action_update_ror(AoeBaseActionRor *action,
                                  const AoeBaseActionOps *ops,
                                  uint8_t *result);

/* Transport (12): constructor 0x004067d0 (point); +0x1c = 0x00406810
   (`arg` and a nonnegative point: 11, else 2), +0x5c = 0x00406860 (4/11
   sail to the point, range 0.3, quiet after), +0x28 = 0x004069a0 (an
   arrived sail (4) unloads (6); 11 idles (2) on arrival; 6 unloads:
   outside the point's zone near the boat, notice 0x207; with passengers
   and a usable shore (+0x60 = 0x00406c90) 0x004ab280, notice 0x206 once
   empty or 0x207 when none left; otherwise 0x207 unless the point is
   water (terrain 1 or 22)), +0x34 = 0x00406b80 (a point within 2 in a
   zone near the boat unloads here (6); a nonnegative point sails (4 for
   the transport itself as target, else 11)), +0x30 = 0x00406b50 (sail
   to the point, 11). Owner notices carry 0x266. */
/* Two adjacent unit slots (table words 0x005433d0 / 0x005433d4 and
   0x0054476c / 0x00544770): 0x00427820(x, y, z) retargets a head transport action (+0x18
   type 0xc) to the unit itself at (x, y, z); without one a new transport
   action (0x004067d0) at the point replaces the list (0x00403e50) and is
   adopted (+0x208) first. 0x00427910(unused, x, y, z) does the same, but a
   head transport action is retargeted to the unit's own position, and a
   new one starts there before its retarget to (x, y, z). 0 when the
   action cannot be made, else 1. */
typedef struct {
    void *context;
    void *(*head_transport)(void *context, AoeLiveUnit *unit); /* NULL: none */
    void *(*new_transport)(void *context, AoeLiveUnit *unit, float x, float y, float z);
    void (*clear_list)(void *context, AoeLiveUnit *unit);
    void (*adopt)(void *context, AoeLiveUnit *unit, void *action);
    void (*retarget)(void *context, void *action, AoeLiveUnit *target, float x, float y,
                     float z);
} AoeTransportUnitOpsRor;
int aoe_transport_unit_go_ror(AoeLiveUnit *unit, float x, float y, float z,
                              const AoeTransportUnitOpsRor *ops);
int aoe_transport_unit_go_from_here_ror(AoeLiveUnit *unit, float x, float y, float z,
                                        const AoeTransportUnitOpsRor *ops);
int aoe_transport_action_init_ror(AoeBaseActionRor *action,
                                  AoeLiveUnit *unit, const DatUnitTask *task,
                                  float x, float y, float z);
int aoe_transport_action_set_state_ror(AoeBaseActionRor *action,
                                       uint8_t state,
                                       const AoeBaseActionOps *ops);
int aoe_transport_action_activate_ror(AoeBaseActionRor *action, int arg,
                                      const AoeBaseActionOps *ops);
int aoe_transport_action_retarget_ror(AoeBaseActionRor *action,
                                      AoeLiveUnit *target, float x, float y,
                                      float z, const AoeBaseActionOps *ops);
int aoe_transport_action_walk_ror(AoeBaseActionRor *action, float x,
                                  float y, float z,
                                  const AoeBaseActionOps *ops);
/* 0x00406c90: a task with target terrain -1, or a beach/shallows tile
   (terrain 2 or 4) in the point's zone within 1 + size of the boat. */
int aoe_transport_shore_ror(const AoeBaseActionRor *action,
                            const AoeBaseActionOps *ops);
int aoe_transport_action_update_ror(AoeBaseActionRor *action,
                                    const AoeBaseActionOps *ops,
                                    uint8_t *result);

#endif
