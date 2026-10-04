#ifndef VITA_AOE_WORLD_MOTION_H
#define VITA_AOE_WORLD_MOTION_H

#include <stdint.h>

#include "zone_map.h"

#include "movement.h"
#include "pathfinder_search.h"
#include "scenario_runtime.h"

/* Binds the translated RoR 1.0a movement (src/movement.c), pathfinder and
   fit check (src/pathfinder_search.c) to an owned scenario runtime.

   Owned here: the main pathfinder (0x006a1cc0) storage sized to the map, its
   distance table, one player path budget per scenario player, and a side
   table of per-unit movement state (live +0xa4..+0x17d), obstruction flags
   (+0x84/+0x85) and the +0x158..+0x164 footprint record. Unit IDs resolve
   through the runtime's world registry.

   Bindings that are inferences or placeholders (each documented in
   docs/reversing.md):
   - every constructed state-0..2 unit with height > 0 restores its
     footprint at init (the original registers footprints as units enter
     the world);
   - the overlay terrain of a border index is that border's
     underlay_terrain (border records at map +0x338c, stride 0x5a0, read at
     +0x59c);
   - terrain costs and the terrain speed multiplier are the DAT terrain
     restriction row of template +0x66 (world +0x54 / 0x004aa880);
   - player path budgets are unlimited (-1), owner kind is 0, free terrains
     are -1, the zone lookup 0x004ab9e0 reports one zone, group members are
     never busy, and the unit +0x12c virtual is a no-op. */
typedef struct {
    AoeLiveUnit *unit;
    AoeMovementRor movement;
    AoeObstructionStateRor obstruction;
    int32_t footprint_low[2], footprint_high[2];
} AoeWorldMotionUnitRor;

/* Per-unit records are indexed by unit ID + OFFSET. Tables start with
   ID_SLOTS entries and grow on demand (200x200 campaign maps use IDs past
   13,000) up to ID_SLOTS_MAX. */
enum { AOE_WORLD_MOTION_ID_OFFSET = 1000, AOE_WORLD_MOTION_ID_SLOTS = 11000,
       AOE_WORLD_MOTION_ID_SLOTS_MAX = 1 << 20 };

/* Grows the zero-initialised pointer table whose T** field is at
   `table_field` so `slot` is valid. */
int aoe_world_id_table_reserve(void *table_field, size_t *slots,
                               int64_t slot);

typedef struct {
    AoeScenarioRuntimeRor *runtime;
    AoePathfinderRor path;
    int32_t distance_table[256];
    AoePathBudgetRor budgets[AOE_SCENARIO_RUNTIME_PLAYERS];
    AoeWorldMotionUnitRor **units; /* by unit ID + 1000 */
    size_t unit_slots; /* grows past the initial ID_SLOTS on demand */
    float tick;
    uint32_t searches, repaths, placements;
    /* Map +0x8dc4 terrain-restriction zones (zone_map.h), built on first
       use from the map's terrain; the zone-map index per restriction row
       stands for the unit's +0x7c cache. */
    AoeZoneManagerRor zones;
    int32_t *zone_index; /* per restriction row, -2 = not looked up */
    uint8_t *zone_terrain;
} AoeWorldMotionRor;

/* Allocates the pathfinder for the runtime map (at most 255 x 255) and
   restores the footprints of the runtime's live units. */
int aoe_world_motion_init_ror(AoeWorldMotionRor *motion,
                              AoeScenarioRuntimeRor *runtime);
void aoe_world_motion_destroy_ror(AoeWorldMotionRor *motion);
/* Frees the unit's movement record, if any (before the unit is removed). */
void aoe_world_motion_release_ror(AoeWorldMotionRor *motion,
                                  const AoeLiveUnit *unit);

/* The unit's movement record, created on first use (NULL for IDs outside
   the registry range or allocation failure). */
AoeWorldMotionUnitRor *aoe_world_motion_unit_ror(AoeWorldMotionRor *motion,
                                                 AoeLiveUnit *unit);

/* The movement setters then movement start 0x0044bf00, as the move action
   does: destination (x, y, 1.0), range, target ID and target size. Returns
   the start result (0 path ready, 2 deferred) or -1. */
int aoe_world_motion_move_to_ror(AoeWorldMotionRor *motion,
                                 AoeLiveUnit *unit, float x, float y,
                                 float range, int32_t target_id);

/* One movement update (0x0044c2d0 with 0x0044cde0) using `tick` seconds.
   Returns the update result (2 stopped, 1 moving, 0 exhausted) or -1. */
int aoe_world_motion_tick_ror(AoeWorldMotionRor *motion, AoeLiveUnit *unit,
                              float tick);

/* The fit check 0x0045a4f0 against this world. */
int aoe_world_motion_fits_ror(AoeWorldMotionRor *motion, AoeLiveUnit *unit,
                              float x, float y, int check_terrain);

/* Unit 0x004ab9e0(x, y): the zone of a tile in the unit's restriction
   zone map; -1 off the map or without a restriction row. */
int aoe_world_motion_zone_ror(AoeWorldMotionRor *motion, AoeLiveUnit *unit,
                              int32_t x, int32_t y);
/* 0x004abb60(zone, distance): a tile of the zone within `distance` of the
   unit's tile. */
int aoe_world_motion_zone_near_ror(AoeWorldMotionRor *motion,
                                   AoeLiveUnit *unit, int zone,
                                   float distance);
/* Unit +0x34 as bound for movement: place at (x, y, z) with vision and
   the footprint following. A unit whose footprint is lifted (inside a
   container) gets it restored. */
void aoe_world_motion_place_ror(AoeWorldMotionRor *motion, AoeLiveUnit *unit,
                                float x, float y, float z);
/* 0x004ab7f0(2): lift the unit's footprint (entering a container). */
void aoe_world_motion_lift_ror(AoeWorldMotionRor *motion, AoeLiveUnit *unit);

/* Bound callback sets, for callers composing other translated systems. */
void aoe_world_motion_path_ops_ror(AoeWorldMotionRor *motion,
                                   AoeLiveUnit *unit, AoePathWorldOps *ops);
void aoe_world_motion_tick_ops_ror(AoeWorldMotionRor *motion,
                                   AoeMovementTickOps *ops);

#endif
