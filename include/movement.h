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
    float current_speed;           /* live +0x88 (0 when stopped) */
    float velocity_x, velocity_y, velocity_z; /* live +0x90/+0x94/+0x98 */
    float facing;                  /* live +0x9c, radians */
    float turn_timer;              /* +0x12c */
    float blocked_wait;            /* +0x130: >0 waiting, -1 expired */
    int32_t retry_baseline;        /* +0x144, read by the pathfinder */
    int32_t status;                /* +0x148: 1 stopped, 2 moving */
    uint8_t use_alternate_path;    /* +0x154 */
    uint8_t retry;                 /* +0x156 */
    /* +0x168: intermediate waypoint tiles, 3 bytes each starting x, y. */
    const uint8_t *waypoint_tile;
    int32_t waypoint_count;        /* +0x16c */
    uint8_t explored;              /* +0x174 (0x0044edd0 sets it) */
    float best;                    /* +0x178 */
    uint8_t facet_history[2];      /* +0x17c/+0x17d: last two facets */
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

typedef struct {
    AoeMovementOps base;
    /* Live vtable +0x1c0(0) on stopping and +0x1c4(0), which drops the
       front intermediate waypoint (updating waypoint_tile/count). */
    void (*stop)(void *context, AoeLiveUnit *unit);
    void (*pop_waypoint)(void *context, AoeLiveUnit *unit);
    /* 0x004ab9e0(x, y): the unit's terrain-restriction zone at a tile. */
    uint8_t (*zone)(void *context, AoeLiveUnit *unit, int32_t x, int32_t y);
    /* Pathfinder re-path counter +0x11dcc0 (0x0045a4d0). */
    void (*count_repath)(void *context);
    /* 0x0044cde0: one tick toward (x, y), returning 0..6. */
    int32_t (*step_to)(void *context, AoeLiveUnit *unit, float x, float y);
} AoeMovementStepOps;

/* 0x0044c2d0, one movement update. Returns 2 when stopped (in range, at the
   destination, or when the tick toward it reports 2..5), 1 while moving or
   after a deferred/failed re-path with intermediate waypoints left, and 0
   when the path is exhausted without them. Distances use the template
   footprint (+0x30/+0x34) and target size, the path waypoints and the
   intermediate tiles; re-paths follow 0x0044bf00 with two different
   centre-snap variants. Both only matter for the placement side effect:
   a one-waypoint path cannot advance, so either way the update gives up. The debug path sprites are not reproduced. Returns
   -1 for missing dependencies (host-only). */
int aoe_movement_update_ror(AoeLiveUnit *unit, AoeMovementRor *movement,
                            AoePathBudgetRor *budget,
                            const AoeMovementStepOps *ops);

/* 0x0044ef90: wrap an angle into [0, 2pi] (2pi itself is kept); with
   `facets` != -1 also snap it to a multiple of pi/4 (8 facets) or pi/8
   (any other count). The search over 0..facets multiplies each candidate
   by the best difference so far (the original reuses one stack slot),
   stopping once that difference is under 0.1; exact multiples snap to
   themselves. NaN (an endless loop in the original) is left unchanged. */
void aoe_movement_snap_angle_ror(float *angle, int32_t facets);

typedef struct {
    float speed_multiplier; /* sprite +0x64 */
    int16_t facets;         /* sprite +0x60 */
    uint8_t flags;          /* sprite +0x70; bit 1 selects facet frames */
} AoeMovementSpriteRor;

/* 0x0044bbd0: with a facet sprite, unit +0x35 becomes
   ftol((facing - pi/4) * facets / 2pi + 0.5), wrapped up by facets. */
void aoe_movement_update_facet_ror(AoeLiveUnit *unit, const AoeMovementRor *m,
                                   const AoeMovementSpriteRor *sprite);

typedef struct {
    AoeMovementStepOps step;
    /* Unit +0x10, or NULL without a sprite. */
    const AoeMovementSpriteRor *(*sprite)(void *context, AoeLiveUnit *unit);
    float (*tick_seconds)(void *context);                 /* world +0x84 */
    /* 0x005206d0 on the target ID: its position, 0 when absent. */
    int (*target_position)(void *context, int32_t id, float *x, float *y);
    /* 0x004aa880 for the terrain under unit +0x1c; 0 without a tile. */
    int (*terrain_multiplier)(void *context, AoeLiveUnit *unit, float *out);
    int32_t (*rand)(void *context);                       /* 0x0052605d */
} AoeMovementTickOps;

/* 0x0044cde0: one tick toward (x, y). Returns
   - 0 after moving, turning or starting a blocked wait;
   - 1 when a blocked re-path lacks budget;
   - 2 when the goal (or the snapped waypoint) is occupied;
   - 4 when the retry baseline runs out, the re-path fails, or a
     kind-1 owner is still blocked;
   - 5 when blocked within reach of the target.
   Turning steps one facet (2pi / facets) per rotation-speed delay;
   speed scales by 0.9 uphill, 1.03 downhill (resetting to the template
   speed on flat ground) and by the terrain multiplier. The atan2, sin and
   cos come from the host libm, whose last-bit results can differ from x87
   fpatan/fsin/fcos. Returns -1 for missing dependencies (host-only). */
int32_t aoe_movement_step_to_ror(AoeLiveUnit *unit, AoeMovementRor *movement,
                                 AoePathBudgetRor *budget,
                                 const AoeMovementTickOps *ops, float x,
                                 float y);

/* aoe_movement_update_ror with 0x0044cde0 bound to the translation above
   (ops->step.step_to is ignored). */
int aoe_movement_tick_ror(AoeLiveUnit *unit, AoeMovementRor *movement,
                          AoePathBudgetRor *budget,
                          const AoeMovementTickOps *ops);

#endif
