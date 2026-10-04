#ifndef VITA_AOE_MOVEMENT_H
#define VITA_AOE_MOVEMENT_H

#include <stdint.h>

#include "pathfinder_search.h"

/* Patched Rise of Rome 1.0a moving-unit movement state (semantic
   storage for live fields +0xa4..+0x178). */
typedef struct {
    AoeUnitPathRor path;           /* +0xa4 */
    AoeUnitPathRor alternate_path; /* +0xd8, used when +0x154 is set */
    float start_x, start_y, start_z; /* +0x10c */
    uint8_t start_direction;         /* +0x118, copied with the waypoint */
    float dest_x, dest_y, dest_z;    /* +0x11c */
    uint8_t dest_direction;          /* +0x128 */
    float range;                   /* +0x134 */
    int32_t target_id;             /* +0x138 */
    float target_size_x, target_size_y; /* +0x13c/+0x140 */
    int32_t retry_baseline;        /* +0x144, read by the pathfinder */
    uint8_t use_alternate_path;    /* +0x154 */
    uint8_t retry;                 /* +0x156 */
    const uint8_t *waypoint_tile;  /* +0x168: x, y bytes */
    int32_t waypoint_count;        /* +0x16c */
    uint8_t explored;              /* +0x174 (0x0044edd0 sets it) */
    float best;                    /* +0x178 */
} AoeMovementRor;

/* Owner player path budget (+0x08 max per tick, +0x0c retry override,
   +0x10 used). */
typedef struct {
    int32_t max_per_tick, retry_override, used;
} AoePathBudgetRor;

/* 0x0045c180 / 0x0045c1c0. */
int aoe_path_budget_allows_ror(const AoePathBudgetRor *budget, int32_t retry);
void aoe_path_budget_consume_ror(AoePathBudgetRor *budget);

/* Setters 0x0044be90 (z is passed as 1.0f by the move action),
   0x0044bec0, 0x0044bed0, 0x0044bee0, 0x0044edd0. */
void aoe_movement_set_destination_ror(AoeMovementRor *m, float x, float y,
                                      float z);
void aoe_movement_set_range_ror(AoeMovementRor *m, float range);
void aoe_movement_set_target_id_ror(AoeMovementRor *m, int32_t id);
void aoe_movement_set_target_size_ror(AoeMovementRor *m, float x, float y);
void aoe_movement_mark_explored_ror(AoeMovementRor *m);

/* Path cursor helpers: 0x00458440 reset, 0x00458450 advance (cursor +1
   while below count - 1, else clamps and returns 0), 0x004583d0 current
   waypoint or NULL. */
void aoe_unit_path_rewind_ror(AoeUnitPathRor *path);
int aoe_unit_path_advance_ror(AoeUnitPathRor *path);
const AoePathWaypointRor *aoe_unit_path_current_ror(const AoeUnitPathRor *path);

typedef struct {
    void *context;
    void (*order_flag)(void *context, AoeLiveUnit *unit, int32_t value);
    /* Owner player 0x00454150; 0 selects the larger retry baseline. */
    int32_t (*owner_kind)(void *context, AoeLiveUnit *unit);
    /* The global pathfinder 0x006a1cc0: search 0x00458930 (with the
       distance pointer NULL) returning its 0/1 result, and the search
       counter +0x11dcbc (0x0045a4c0). */
    int32_t (*find)(void *context, const AoePathRequestRor *request);
    void (*count_search)(void *context);
    /* 0x0045a4f0(unit, x, y, 1): nonzero when the unit fits at (x, y). */
    int (*fits)(void *context, AoeLiveUnit *unit, float x, float y);
    /* Unit vtable +0x34: place the unit at (x, y, z). */
    void (*place)(void *context, AoeLiveUnit *unit, float x, float y,
                  float z);
} AoeMovementOps;

/* 0x0044bf00. A unit whose template +0x38 (DAT size[2], the height that
   0x00404a60 halves for a centre point; not the speed at +0xb8) is <= 0
   or NaN gets the path start -> destination directly and returns 0. Otherwise:
   - without owner path budget, +0x156 counts the retry and it returns 2;
   - with budget, the retry resets, +0x144 becomes waypoint_count + 25
     (owner kind 0) or + 15, and the search runs from the unit's tile to
     the intermediate tile (when waypoint_count > 0) or the destination tile
     with range +0x134, target +0x138, mode 1, budget-limited, occupancy
     checked, step 1 and unit queries off;
   - a failed search that left a one-waypoint path for an off-centre unit
     whose tile centre is free snaps the unit to the centre and returns 2;
   - otherwise the path cursor is rewound and advanced, returning 0.
   The moving branch is guarded by a constant (code address 0x0044f1a0
   tested against zero) whose dead else returns 1. Map pointer stores
   (0x00451cc0) and the debug path sprites (global 0x00582ee4) are not
   reproduced. Returns -1 for missing dependencies (host-only). */
int aoe_movement_start_ror(AoeLiveUnit *unit, AoeMovementRor *movement,
                           AoePathBudgetRor *budget,
                           const AoeMovementOps *ops);

#endif
