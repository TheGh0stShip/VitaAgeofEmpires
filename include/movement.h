#ifndef VITA_AOE_MOVEMENT_H
#define VITA_AOE_MOVEMENT_H

#include <stdint.h>

#include "pathfinder_search.h"
#include "zone_map.h"

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
    /* +0x14c/+0x150: terrains passable for the current search (-1 none). */
    int32_t free_terrain_a, free_terrain_b;
    uint8_t use_alternate_path;    /* +0x154 */
    uint8_t hold_155;              /* +0x155 (0x0044eb30 sets it) */
    int32_t field_a0;              /* live +0xa0 */
    uint8_t retry;                 /* +0x156 */
    /* +0x168: intermediate waypoint tiles, 3 bytes each starting x, y
       (owned by the list functions below; storage of
       waypoint_capacity entries). */
    uint8_t *waypoint_tile;
    int32_t waypoint_count;        /* +0x16c */
    int32_t waypoint_capacity;     /* +0x170 */
    /* +0x174: set by 0x0044edd0; the next append clears the list first. */
    uint8_t waypoints_stale;
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
   0x0044bec0, 0x0044bed0, 0x0044bee0, 0x0044edd0 (marks the intermediate
   waypoints stale). */
void aoe_movement_set_destination_ror(AoeMovementRor *m, float x, float y,
                                      float z);
void aoe_movement_set_range_ror(AoeMovementRor *m, float range);
void aoe_movement_set_target_id_ror(AoeMovementRor *m, int32_t id);
void aoe_movement_set_target_size_ror(AoeMovementRor *m, float x, float y);
void aoe_movement_mark_waypoints_stale_ror(AoeMovementRor *m);

/* Intermediate waypoint list (live vtable +0x1b8..+0x1c4):
   - get 0x0044eb40 (+0x1b8): entry i or NULL;
   - append 0x0044eb70 (+0x1bc): clears a stale list first (passing `clear_arg`),
     then stores the 3-byte entry, growing storage to 5 entries and then
     doubling with zero fill; returns 0 on allocation failure;
   - clear 0x0044ed20 (+0x1c0): count 0; with arg 1 the original also calls unit
     +0x74's vtable +0x58(0), which is the caller's `on_clear` hook here;
   - remove 0x0044ed50 (+0x1c4): shifts entries from max(index, ...) down by one, so
     index 0 drops entry 0 but index k > 0 drops entry k - 1; out-of-range
     indexes do nothing. */
const uint8_t *aoe_movement_waypoint_get_ror(const AoeMovementRor *m,
                                             int32_t index);
int aoe_movement_waypoint_append_ror(AoeMovementRor *m,
                                     const uint8_t entry[3],
                                     int32_t clear_arg,
                                     void (*on_clear)(void *context),
                                     void *context);
void aoe_movement_waypoint_clear_ror(AoeMovementRor *m, int32_t arg,
                                     void (*on_clear)(void *context),
                                     void *context);
void aoe_movement_waypoint_remove_ror(AoeMovementRor *m, int32_t index);
void aoe_movement_waypoint_free_ror(AoeMovementRor *m);

/* The move action's record step (0x00405500): unless the list is stale,
   append the tile (+0x1bc(tile, 0)) and mark it stale (0x0044edd0).
   Returns 0 on allocation failure. */
int aoe_movement_record_waypoint_ror(AoeMovementRor *m,
                                     const uint8_t tile[3]);

/* Path cursor helpers: 0x00458440 reset, 0x00458450 advance (cursor +1
   while below count - 1, else clamps and returns 0), 0x004583d0 current
   waypoint or NULL; 0x0044be60 the unit's (path +0xa4) current
   waypoint byte +0x0c, 0xff without one. */
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
    float reference_speed; /* sprite +0x64 */
    int16_t facets;         /* sprite +0x60 */
    uint8_t flags;          /* sprite +0x70; bit 1 selects facet frames */
} AoeMovementSpriteRor;

/* Moving unit (type 30) slots:
   - 0x0044b940 re-initialise: the type-20 part (0x004152b0), then the
     velocity, +0xa0, the blocked wait and the turn timer cleared and the
     facet refreshed (0x0044bbd0) (aoe_movement_reinit_ror: the clears;
     the other two calls are the caller's);
   - 0x0044b980 moving: the speed (+0x88) above 0;
   - 0x0044eb00 the +0x155 byte, 0 for a unit in state 6 or 7 (its
     +0x134); 0x0044eb30 sets it;
   - 0x0044f120: the path's waypoints, each counted as its y + x, summed
     and truncated (the cursor is kept). */
void aoe_movement_reinit_ror(AoeMovementRor *m);
int aoe_movement_moving_ror(const AoeMovementRor *m);
uint8_t aoe_movement_hold_ror(const AoeMovementRor *m, uint8_t unit_state);
void aoe_movement_set_hold_ror(AoeMovementRor *m, uint8_t value);
int32_t aoe_unit_path_coord_sum_ror(AoeUnitPathRor *path);

/* 0x0044bbd0: with a facet sprite, unit +0x35 becomes
   ftol((facing - pi/4) * facets / 2pi + 0.5), wrapped up by facets. */
void aoe_movement_update_facet_ror(AoeLiveUnit *unit, const AoeMovementRor *m,
                                   const AoeMovementSpriteRor *sprite);

/* 0x0044b9b0: facet offset to (x, y) (a target's position when it has
   one): atan2 heading (+2pi when negative) minus the facing, wrapped once
   into [-pi, pi], times facets / 2pi; 0 without a facet sprite (flags bit
   1, facets > 0). */
double aoe_movement_facet_offset_ror(const AoeLiveUnit *unit,
                                     const AoeMovementRor *m,
                                     const AoeMovementSpriteRor *sprite,
                                     float x, float y);

/* 0x0044ba60 (vtable +0x1f0, the attack action's "can attack"): 1 when
   the offset is within +-0.5 facets. Otherwise, once template +0xc4
   (rotation_speed) seconds have passed since `*last_turn` (live +0xa0;
   now = world +0x04 milliseconds * 0.001f), turn one facet toward it,
   wrap the facing once, refresh the facet frame, record the time and
   re-check. */
int aoe_movement_face_ror(AoeLiveUnit *unit, AoeMovementRor *m,
                          const AoeMovementSpriteRor *sprite, float x,
                          float y, uint32_t world_ms, float *last_turn);

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

typedef struct {
    void *context;
    /* Map +0x08/+0x0c (world +0x3c, map +0x28). */
    int32_t map_width, map_height;
    /* Pathfinder 0x00583bc8 when `second` is nonzero, else 0x006a1cc0:
       the search counter 0x0045a4e0 and the search 0x00458930 (its 0/1
       result). The request's unit path is the alternate one (+0x154). */
    void (*count_search)(void *context, int second);
    int32_t (*find)(void *context, int second, const AoePathRequestRor *request);
} AoeMovementPathToOps;

/* 0x0044e350 (11 arguments, the third unused): a goal tile outside the
   map (or negative) returns 0 untouched. Otherwise +0x14c/+0x150 take the
   two free terrains and +0x154 is set while the chosen pathfinder counts a
   search and runs it from the unit's position (ftol x, y) to the goal
   with `range`, `target_id`, mode 1, `budget_limited`, argument 1,
   occupancy checked, step 1, the float argument pushed as the integer 1,
   `query_filter` and `budget_base`. Afterwards +0x154 is cleared and both
   free terrains reset to -1; the search result is returned. -1 for
   missing dependencies (host-only). */
int32_t aoe_movement_path_to_ror(AoeLiveUnit *unit, AoeMovementRor *m,
                                 int32_t x2, int32_t y2, float range,
                                 int32_t target_id, int32_t budget_limited,
                                 int32_t free_terrain_a,
                                 int32_t free_terrain_b, int second,
                                 int32_t query_filter, int32_t budget_base,
                                 const AoeMovementPathToOps *ops);

typedef struct {
    void *context;
    /* 0x00521b50 on the map's zone manager (+0x8dc4) with the world's
       passability table for the unit's restriction (+0x54[template
       +0x66], count +0x52); and 0x00521c60, the map of an index. */
    int32_t (*zone_index)(void *context, AoeLiveUnit *unit);
    const AoeZoneMapRor *(*zone_map)(void *context, int32_t index);
    /* The second pathfinder 0x00583bc8: search 0x00458930 (its 0/1 result
       is discarded) and penalty fill 0x00458820(0). */
    int32_t (*find)(void *context, const AoePathRequestRor *request);
    void (*clear_penalty)(void *context);
} AoeMovementReachOps;

/* 0x0044ede0: the unit's zone map index (unit +0x7c, `zone_index`) is
   looked up once while -1. When the zone at the unit's tile (ftol x, y)
   differs from the goal tile's and 0x00521640(unit tile, goal, range) finds
   no tile of the unit's zone within `range` of the goal, NULL. Otherwise, with +0x154 set, the
   second pathfinder searches from the unit's tile to the goal with
   `range`, `target_id`, mode 1, no budget limit, argument 0, occupancy
   checked, step 1, the float argument pushed as the integer 1, filter -1
   and budget base -1; +0x154 is cleared, the penalties filled with 0 and
   the alternate path (+0xd8) returned. NULL for missing dependencies or
   tiles outside the zone map (the original reads unchecked; host-only). */
AoeUnitPathRor *aoe_movement_reach_path_ror(AoeLiveUnit *unit,
                                            AoeMovementRor *m,
                                            int32_t *zone_index,
                                            const int32_t goal[2],
                                            float range, int32_t target_id,
                                            const AoeMovementReachOps *ops);

/* Unit vtable +0x114(x, y, arg): nonzero when the unit may stand at the
   point. */
typedef int32_t (*AoeMovementPassableFn)(void *context, AoeLiveUnit *unit,
                                         float x, float y, int32_t arg);

/* 0x0044e7a0 (moving-unit slot: x1, y1, x2, y2, range, arg): 0 for
   equal ends. Otherwise steps = 2 * max(|dx|, |dy|) of the float increments
   dx / steps, dy / steps (rounded to float); from (x1, y1) each step's
   float point is checked with `passable` whenever its ftol tile differs
   from the last checked one (initially -999, -999): blocked returns 0,
   and a point within `range` of the end (x87 sqrt, compared <=) ends the
   walk with 1. 1 after the last step. 0x0044e600 is the same over
   from/to points, also storing the blocked tile in `blocked` (when
   non-NULL here). -1 without `passable` (host-only). */
int32_t aoe_movement_line_passable_ror(AoeLiveUnit *unit, int32_t x1,
                                       int32_t y1, int32_t x2, int32_t y2,
                                       float range, int32_t arg,
                                       AoeMovementPassableFn passable,
                                       void *context, int32_t blocked[2]);
/* 0x0044e600: from/to as tile points; see above. */
int32_t aoe_movement_line_first_block_ror(AoeLiveUnit *unit,
                                          const int32_t from[2],
                                          const int32_t to[2], float range,
                                          int32_t blocked[2], int32_t arg,
                                          AoeMovementPassableFn passable,
                                          void *context);

typedef struct {
    AoeMovementPathToOps path;   /* map size, pathfinder count and search */
    /* As in AoeMovementReachOps (0x00521b50 / 0x00521c60). */
    int32_t (*zone_index)(void *context, AoeLiveUnit *unit);
    const AoeZoneMapRor *(*zone_map)(void *context, int32_t index);
} AoeMovementZonePathOps;

/* 0x0044da90 (9 arguments, the third unused): a goal outside the map
   returns 0. The zone map index (unit +0x7c) is looked up once while -1;
   without a map, 0. When the goal's zone differs from the unit's tile's
   and 0x00521640(unit tile, goal, range) finds no tile of the unit's zone
   within `range` of the goal, 0. Otherwise the chosen pathfinder counts a
   search and runs it from the unit's tile with `range`, `target_id`, mode
   0, `budget_limited`, argument 1, occupancy checked, step 1, the float
   argument pushed as the integer 1, `query_filter` and `budget_base`,
   returning its result (the normal path, +0x154 untouched). -1 for
   missing dependencies (host-only). */
int32_t aoe_movement_zone_path_ror(AoeLiveUnit *unit, int32_t *zone_index,
                                   int32_t x2, int32_t y2, float range,
                                   int32_t target_id, int32_t budget_limited,
                                   int second, int32_t query_filter,
                                   int32_t budget_base,
                                   const AoeMovementZonePathOps *ops);

/* 0x0044dc40 (target_id, range, budget_limited, second, query_filter,
   budget_base): 0x0044da90 toward an object. The caller resolves the ID
   (0x005206d0; NULL gives 0). There is no map-bounds check; the goal is
   the object's tile (ftol x, y). The zone test and the search (mode 0,
   the object's ID as target) are those of 0x0044da90, whose result is
   returned (0 when the zone test fails or there is no zone map). */
int32_t aoe_movement_object_path_ror(AoeLiveUnit *unit, int32_t *zone_index,
                                     const AoeLiveUnit *target, int32_t target_id, float range,
                                     int32_t budget_limited, int second, int32_t query_filter,
                                     int32_t budget_base, const AoeMovementZonePathOps *ops);

/* 0x0044e130 (target_id, range, budget_limited, second, query_filter,
   budget_base, met): as 0x0044dc40 but mode 1 with +0x154 set around the
   search (the alternate path); with `budget_base` not -1 the chosen
   pathfinder's met IDs are then collected into `met` (0x0045ade0,
   `collect`). +0x154 is cleared; the search result is returned. */
typedef void (*AoeMovementCollectFn)(void *context, int second, void *met);
int32_t aoe_movement_object_path_alt_ror(AoeLiveUnit *unit, AoeMovementRor *m,
                                         int32_t *zone_index, const AoeLiveUnit *target,
                                         int32_t target_id, float range,
                                         int32_t budget_limited, int second,
                                         int32_t query_filter, int32_t budget_base, void *met,
                                         AoeMovementCollectFn collect,
                                         const AoeMovementZonePathOps *ops);


typedef struct {
    void *context;
    AoeLiveUnit *(*resolve)(void *context, int32_t id);          /* 0x005206d0 */
    int32_t (*zone_index)(void *context, AoeLiveUnit *unit);     /* 0x00521b50 */
    const AoeZoneMapRor *(*zone_map)(void *context, int32_t index); /* 0x00521c60 */
    /* Pathfinder 0x00583bc8 (`second`) or 0x006a1cc0: the search counter
       0x0045a4e0 and the search 0x00458930 (its 0/1 result). */
    void (*count_search)(void *context, int second);
    int32_t (*find)(void *context, int second, const AoePathRequestRor *request);
} AoeMovementReachViaOps;

/* 0x0044de30 (via_id, goal_id, range, budget_limited, second, query_filter,
   budget_base): a path through object `via_id` to object `goal_id`. Both
   must resolve. The unit's zone map index (+0x7c) is looked up while -1;
   no map gives 0. When the via object's tile lies in another zone than
   the unit's and 0x00521640(unit tile, via tile, range) finds none of
   the unit's zone within range, 0. Otherwise with +0x154 set the chosen
   pathfinder counts a search and runs unit tile -> via tile (`range`,
   `via_id`, mode 1, `budget_limited`, argument 1, occupancy, step 1, the
   float argument the integer 1, `query_filter`, `budget_base`); a failure
   clears +0x154 and gives 0; else a second search runs from the alternate
   path's last waypoint (0x00458420, ftol) to the goal's tile with
   `goal_id` and argument 0; +0x154 is cleared and that result returned.
   -1 for missing dependencies (host-only). */
int32_t aoe_movement_reach_via_ror(AoeLiveUnit *unit, AoeMovementRor *m, int32_t *zone_index,
                                   int32_t via_id, int32_t goal_id, float range,
                                   int32_t budget_limited, int second, int32_t query_filter,
                                   int32_t budget_base, const AoeMovementReachViaOps *ops);

#endif
