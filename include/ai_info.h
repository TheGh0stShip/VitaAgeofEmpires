#ifndef VITA_AOE_AI_INFO_H
#define VITA_AOE_AI_INFO_H

#include <stdint.h>

#include "ai_strategy.h" /* AoeAiIntListRor */
#include "ai_unit_group.h"
#include "ai_build.h" /* AoeAiConItemRor */
#include "value_grid.h"


/* Patched Rise of Rome 1.0a computer-player information AI (vtable
   0x00548b74; the computer player holds it at +0xcec): the members in
   0x004bc230..0x004c6880 that keep its object memory, explored grid,
   area marks and statistics. Only the state those members touch is kept
   here, by original offset. The constructor, save and load live outside
   this range. */

/* A world object as the AI reads it: the ID (+4), its type's ID (+0x10)
   and class (+0x14), the owner's index (owner +0x4a) and the owner's +4,
   position (+0x38, +0x3c, +0x40), hit points (+0x30), +0x44, the state
   byte (+0x48), the type's maximum hit points (+0x26) and float +0x28,
   and the object's +0x4c. */
typedef struct {
    int32_t id;
    int16_t type_id, type_class;
    int16_t owner;
    int32_t owner_4;
    float x, y, z;
    float hp, field_44;
    uint8_t state;
    int16_t max_hp;
    float type_28;
    int16_t field_4c;
    float size_x, size_y;  /* the type's +0x30 / +0x34 (half sizes) */
    uint8_t fog_visible;   /* the type's +0x65: shown on fogged tiles */
} AoeAiWorldObjectRor;

/* One remembered object (0x24 bytes at +0x114): the ID (-1 free), type
   ID, class, tile position, the owner index when seen, hit points, +0x10,
   +0x14 and three floats (the unit's vtable +0x108 and +0x100 values and
   its range, vtable +0x10c). */
typedef struct {
    int32_t id;             /* +0x00 */
    int16_t type_id;        /* +0x04 */
    int16_t type_class;     /* +0x06 */
    uint8_t x, y, z;        /* +0x08..+0x0a */
    uint8_t owner;          /* +0x0b */
    uint16_t hp;            /* +0x0c */
    int32_t field_10;       /* +0x10 */
    uint8_t field_14;       /* +0x14 */
    float field_18, field_1c, range; /* +0x18, +0x1c, +0x20 */
} AoeAiInfoObjectRor;

/* A mark (8 bytes at +0x1f8): ID, state byte, tile x and y. */
typedef struct {
    int32_t id;
    uint8_t state, x, y, pad;
} AoeAiInfoMarkRor;

/* An area (0x18 bytes at +0x200): ID (-1 free), +4, the tile rectangle
   (x0, y0, x1, y1), two player bytes (+9, +10), +0xc, +0xe, +0x10 (the
   replacement age) and +0x14. */
typedef struct {
    int32_t id;
    uint8_t field_4, x0, y0, x1, y1, player_9, player_a, pad_b;
    uint16_t field_c;
    uint8_t field_e, pad_f;
    uint32_t age;
    int32_t field_14;
} AoeAiInfoAreaRor;

/* A target candidate (20 bytes in the four lists at +0x4d0, counts
   +0x4e0): ID, tile x and y, a pick counter (+6), +8, a wait byte (+0xc),
   a gone flag (+0xd), +0xe and +0x10. */
typedef struct {
    int32_t id;
    uint8_t x, y, picks, pad_7;
    int32_t field_8;
    uint8_t wait, gone, field_e, pad_f;
    int32_t field_10;
} AoeAiInfoTargetRor;

enum { AOE_AI_INFO_STATS = 11, AOE_AI_INFO_HISTORY = 16000 };

typedef struct {
    AoeAiIntListRor list_f0;          /* +0xf0 */
    void *owner;                      /* +0x100, the computer player */
    int32_t map_width, map_height;    /* +0x104, +0x108 (-1 unknown) */
    int32_t field_10c;                /* +0x10c */
    AoeAiInfoObjectRor *objects;      /* +0x114 */
    int32_t object_count;             /* +0x110, 500 at first */
    AoeAiIntListRor list_118, list_128, list_138; /* +0x118 .. +0x138 */
    AoeAiIntListRor list_148;         /* +0x148 (count +0x14c) */
    AoeValueGridRor explored;         /* +0x158, 254 x 254 of 0xff */
    AoeAiIntListRor list_180;         /* +0x180 */
    AoeAiIntListRor list_190;         /* +0x190 */
    int32_t last_site[2];             /* +0x1a0, +0x1a4 (-1 none) */
    int32_t prev_site[2];             /* +0x1a8, +0x1ac */
    int32_t field_1b0;                /* +0x1b0 */
    int32_t pairs_1b4[4][2];          /* +0x1b4 */
    int32_t pairs_1d4[4][2];          /* +0x1d4 */
    AoeAiInfoMarkRor *marks;          /* +0x1f8 */
    int32_t mark_count;               /* +0x1f4 */
    AoeAiInfoAreaRor *areas;          /* +0x200 */
    int32_t area_count;               /* +0x1fc */
    int32_t field_204;                /* +0x204, 1 writes the area file */
    char area_path[256];              /* +0x208 */
    uint8_t zone_flags[256];          /* +0x308 */
    int32_t cells[4][4][3];           /* +0x408 */
    int32_t **kind_lists;             /* +0x4c8, one array per kind */
    int32_t *kind_counts;             /* +0x4cc */
    AoeAiInfoTargetRor *targets[4];   /* +0x4d0 */
    int32_t target_count[4];          /* +0x4e0 */
    int32_t target_capacity[4];       /* +0x4f0 */
    int32_t age_allowed[4][4];        /* +0x500, by list and age */
    int32_t best_distance[4];         /* +0x540 */
    int32_t current_target[4];        /* +0x550 */
    int32_t field_560;                /* +0x560 */
    int32_t history[AOE_AI_INFO_HISTORY]; /* +0x564 */
    int32_t stats[AOE_AI_INFO_STATS]; /* +0xff64 */
    char learn_path[256];             /* +0xff90 */
} AoeAiInfoRor;

/* The world and the owning computer player. */
typedef struct {
    void *context;
    int16_t self;  /* the owner's index (player +0x4a) */
    /* 0x0040bab0 on the player: the object, or 0. */
    int (*object)(void *context, int32_t id, AoeAiWorldObjectRor *out);
    /* 0x005206d0: any world object by ID, or 0. */
    int (*world_object)(void *context, int32_t id, AoeAiWorldObjectRor *out);
    /* The object's terrain zone (its own zone getter). */
    uint8_t (*zone)(void *context, int32_t id);
    /* The explored byte grid at +0x158 (get, and a bounded set). */
    int (*grid_get)(void *context, int32_t x, int32_t y);
    void (*grid_set)(void *context, int32_t x, int32_t y, int32_t value);
    /* The world map (player +0x3c, +0x28): width (+8), height (+0xc) and
       the tile byte +5 (row table +0x8d8c, 0x18-byte tiles). */
    int32_t (*map_width)(void *context);
    int32_t (*map_height)(void *context);
    uint8_t (*tile_byte5)(void *context, int32_t x, int32_t y);
    /* Player vtable +0x18 (stance 3 toward `player`) and +0x1c (self or
       stance 0). */
    int (*is_enemy)(void *context, int32_t player);
    int (*is_ally)(void *context, int32_t player);
    /* The tactical AI's strategic number (0x004d77d0). */
    int32_t (*sn)(void *context, int32_t index);
    /* World +0x11d (the player the statistics follow, 0xff for all) and
       the setting read by 0x0041b750. */
    uint8_t stats_player;
    uint8_t settings_97d;
    /* 0x004ab9e0 on an object: the terrain zone of a tile as that object
       moves; 0x00521620 on the map: the tile's zone. */
    uint8_t (*tile_zone)(void *context, int32_t id, int32_t x, int32_t y);
    uint8_t (*map_zone)(void *context, int32_t x, int32_t y);
    /* 0x004aaf50 on an object: its distance to a point. */
    double (*distance)(void *context, int32_t id, float x, float y, float z);
    /* Unit vtable calls: +0x104 (`unit` against `target`), +0x100 and
       +0x10c (the range). */
    double (*value_104)(void *context, int32_t unit, int32_t target);
    double (*value_100)(void *context, int32_t id);
    double (*range)(void *context, int32_t id);
    double (*value_108)(void *context, int32_t id);
    /* The pathfinder penalty grid (0x004587e0 / 0x00458800 on the global
       at 0x00583bc8). */
    uint8_t (*penalty_get)(void *context, int32_t x, int32_t y);
    void (*penalty_set)(void *context, int32_t x, int32_t y, uint8_t value);
    /* The two influence maps (map 0 the global at 0x007c0478 for units,
       map 1 at 0x007c04c8 for areas): clear to 0, set the origin
       (0x00415ac0), add a value over a rectangle (capped below the map's
       limit), finish (the maximum scan, 0x0043dfe0: the largest cell)
       and read a cell (0x0043d6a0). */
    void (*influence_clear)(void *context, int map);
    void (*influence_origin)(void *context, int map, int32_t x, int32_t y);
    void (*influence_add)(void *context, int map, int32_t x0, int32_t y0,
                          int32_t x1, int32_t y1, int32_t value);
    int32_t (*influence_finish)(void *context, int map);
    uint8_t (*influence_get)(void *context, int map, int32_t x, int32_t y);
    /* World players (+0x3c +0x3c), a player's +4 (0x00454150), whether
       that player names us an ally (its vtable +0x1c with our index),
       and passing a sighting to that player's information AI (its
       +0x23c +0xcec, 0x004bdd10 with `from_ally` 1). */
    int16_t player_count;
    int32_t (*player_field_4)(void *context, int32_t player);
    int (*names_us_ally)(void *context, int32_t player);
    void (*share)(void *context, int32_t player, int32_t id);
    /* 0x0040bad0 on the player: our first object of a type (-1 none). */
    int32_t (*first_of_type)(void *context, int32_t type_id);
    /* 0x00517670 on the player's vision map: 0 unseen, 0x80 explored,
       0xf visible. */
    uint8_t (*vision)(void *context, int32_t x, int32_t y);
    /* 0x004aaeb0: the distance between two objects. */
    double (*object_distance)(void *context, int32_t from, int32_t to);
    /* The player's resource `i` (player +0x50). */
    float (*resource)(void *context, int32_t i);
    /* The Build AI (computer player +0x104, vtable +0x48) with type and
       flag: nonzero when it has that item. */
    int (*build_has)(void *context, int32_t type_id, int32_t flag);
    /* The unit's vtable +0x198 path test toward tile (x, y) at height z
       within `radius` (the other arguments are fixed: -1, 1, -1, -1). */
    int (*can_reach)(void *context, int32_t id, int32_t x, int32_t y,
                     int32_t z, float radius);
    /* The game settings' +4 player name, or NULL when there is none, and
       reading the start of a binary file (opened in binary mode, read
       with 0x0046b800, closed): 0 when it cannot open. */
    const char *(*learn_name)(void *context);
    int (*read_file)(void *context, const char *path, void *out, int size);
    /* The objects standing on a tile (tile +0x10 list), at most `max`;
       the count. */
    int32_t (*tile_objects)(void *context, int32_t x, int32_t y,
                            int32_t *ids, int32_t max);
    /* The map's terrain at a tile (0x00447de0, low byte) and the
       tactical AI's pass over the site grid (+0x1146c, 0x004e0d80). */
    uint8_t (*terrain)(void *context, int32_t x, int32_t y);
    void (*tactical_sites)(void *context, struct AoeValueGridRor *grid);
    /* For the site finder: our first object of a class (0x0040bad0 with
       the class), whether we have a template of a type (player +0x24),
       its placement check at a point (template vtable +0x20: nonzero when
       refused), the tactical AI's age test (0x004e2190), rand(), the time
       and the world's time slice (+0xf8). */
    int32_t (*first_of_class)(void *context, int32_t type_class);
    int (*has_template)(void *context, int32_t type_id);
    int (*placement_refused)(void *context, int32_t type_id, float x, float y);
    int (*age_reached)(void *context, int32_t age);
    int (*rand)(void *context);
    uint32_t (*now_ms)(void *context);
    uint32_t time_slice;
    /* The tile count of a zone (0x00521770) in the zone map of our
       villagers' terrain (template 19's +0x66 through 0x00521b50; the
       same map `map_zone` reads). */
    int32_t (*zone_size)(void *context, int32_t zone);
    /* An object's vtable +0x114: whether it could stand at (x, y). */
    int (*free_tile)(void *context, int32_t id, float x, float y);
    /* Writing a whole binary file (created or truncated): 0 when it
       cannot open. */
    int (*write_file)(void *context, const char *path, const void *data,
                      int size);
    /* The player's explored-tile list (player +0x58, 0x0045ebc0 /
       0x0045ebe0): the (x, y) pairs and their count, and the number added
       since the last call (which is then reset). */
    int32_t (*new_tiles)(void *context, const int32_t **xy, int32_t *count);
    /* The tactical AI: how many gatherers work target `id` (0x004e33f0)
       and its +0xd68 slot `i` (0x004e2f90). */
    int32_t (*gatherers)(void *context, int32_t id);
    int32_t (*tactical_d68)(void *context, int32_t i);
    /* The tactical AI's nearest group (0x004e0630, any task, zone -1)
       with +0x180 `field_180` within `max` of tile `pt`: nonzero for one. */
    int (*group_near)(void *context, int32_t field_180, int32_t max,
                      const int32_t pt[2]);
    /* Unit `id`'s zone map holds `zone` within `range` of tile `pt` (the
       unit's own zone-map search). */
    int (*zone_reaches)(void *context, int32_t id, uint8_t zone, float range,
                        const int32_t pt[2]);
    /* A player's resource `i` (player +0x50). */
    float (*player_resource)(void *context, int32_t player, int32_t i);
    /* Unit `id`'s vtable +0x1b0 (from tile, to tile, range, a scratch
       pair, 1): nonzero when it passes. */
    int (*reach_test)(void *context, int32_t id, const int32_t from[2],
                      const int32_t to[2], float range);
    /* The head of an object's action list (+0x184): its type (vtable
       +0x18), state byte (+0xc) and +0x2c; 0 for an empty list. */
    int (*head_action)(void *context, int32_t id, int16_t *type,
                       uint8_t *state, float *field_2c);
    /* A player's name in the settings (0x0043ce80) and a whole binary
       file read into a malloc'd buffer (0 when it cannot open). */
    const char *(*player_name)(void *context, int32_t player);
    int (*read_all)(void *context, const char *path, uint8_t **data,
                    size_t *size);
    /* Unit `id`'s nearby-tile search from (x, y) with a mode, a terrain
       and 4 (its own map search): the tile goes to `out`. */
    void (*near_tile)(void *context, int32_t id, int32_t x, int32_t y,
                      int32_t out[2], int32_t mode, int32_t terrain);
} AoeAiInfoOpsRor;

/* The 36-byte summary 0x004c2de0 fills: the farthest distance, two words
   it never sets (left 0 here), the hit points word (the upper half never
   set), +0x10 (0), a byte (0) and the largest of the three floats. */
typedef struct {
    int32_t max_distance;
    int32_t unset_4, unset_8;
    int16_t hp;
    int32_t field_10;
    uint8_t field_14;
    float field_18, field_1c, range;
} AoeAiInfoForceRor;

/* 0x004ba1c0: the constructor's state (the base part, 0x0040b470, is the
   diplomacy AI's): no owner, map size -1, 500 free objects (ID, type and
   class -1, the rest 0), 200 free marks, 25 free areas (ID -1, player
   bytes 0xff, +0x14 -1), the explored grid 254 x 254 of 0xff, sites and
   the +0x1b0 / +0x1b4 / +0x1d4 words -1, four empty kinds, age table
   rows 1..3 all 1 (row 0 all 0), best distances and current targets -1,
   the learn path "learn\Default.uh"; the rest 0. 0 when out of memory. */
int aoe_ai_info_init_ror(AoeAiInfoRor *info);
/* 0x004bb280 (0x004ba660 deletes too): when +0x204 is 1, the area file
   (+0x208) gets the map width and height, the used areas (count, then
   0x18 bytes each), the used remembered objects of type 0x6d (count, 0x24
   bytes each), for each target list its capacity, count and capacity x 20
   bytes (when the capacity is nonzero), and the +0x138 and +0x148 lists
   (count, values); then, when the statistics follow this player (world
   +0x11d equals ours or is 0xff), the eleven statistics go to the learn
   path. Everything is freed. The list writers first grow a list whose
   capacity is below its count (0x004cc5c0) and send that tail as is. */
void aoe_ai_info_release_ror(AoeAiInfoRor *info, const AoeAiInfoOpsRor *ops);
/* 0x004bbaa0: the owner (+0x100). */
void aoe_ai_info_set_owner_ror(AoeAiInfoRor *info, void *owner);
/* 0x004bb840: the map size from the owner's map, and the learn path
   "learn\<name>.uh" for a named player, else "learn\Default.uh". */
int aoe_ai_info_setup_ror(AoeAiInfoRor *info, const AoeAiInfoOpsRor *ops);
/* 0x004bbab0: per kind its count and values; +0x104, +0x108, +0x10c; the
   objects (count, 0x24 bytes each), marks (count, 8 each), areas (count,
   0x18 each); the +0x118, +0x128, +0x138, +0x148, +0x190 and +0x180
   lists (count, values; grown as above); the explored grid; the
   sites, +0x1b0, the four +0x1b4 / +0x1d4 pairs interleaved, +0x204; the
   area path's length and bytes; 255 zone flags; the 0xc0 cell bytes;
   the target capacities (a capacity with no array is zeroed first),
   counts and entries; the age table, best distances, current targets,
   +0x560 and the history. Statistics are not saved. Records go out as
   the original's bytes (padding 0). 0 when the stream fills up. */
int aoe_ai_info_save_ror(AoeAiInfoRor *info, AoeOutcomeStream *out);
/* 0x004ba680: the load constructor for a game of `version`: owner 0, the
   default learn path, kinds, statistics and history 0, the default age
   table and targets, then the saved fields in save order (list values
   added once each). Before 1.6 the +0x1b0 word is followed by a discarded
   word and no path is saved (the path is the global at 0x00561450, empty
   in the image); before 2.5 the cells are 0; the age table, best
   distances and current targets need 6.8, +0x560 6.91 and the history
   6.99. 0 for a short stream or out of memory (the parts read stay). */
int aoe_ai_info_load_ror(AoeAiInfoRor *info, AoeOutcomeStream *in,
                         float version);

/* 0x004bc690: 1 when the object exists and its zone has no flag;
   0x004bc6e0 sets the zone's flag. */
int aoe_ai_info_zone_open_ror(const AoeAiInfoRor *info, int32_t id,
                              const AoeAiInfoOpsRor *ops);
void aoe_ai_info_set_zone_flag_ror(AoeAiInfoRor *info, int32_t id,
                                   uint8_t value, const AoeAiInfoOpsRor *ops);
/* 0x004bc720: inside the known map and the grid is not 0xff;
   0x004bc770: clear the grid cell inside the map. */
int aoe_ai_info_explored_ror(const AoeAiInfoRor *info, int32_t x, int32_t y,
                             const AoeAiInfoOpsRor *ops);
void aoe_ai_info_forget_tile_ror(const AoeAiInfoRor *info, int32_t x,
                                 int32_t y, const AoeAiInfoOpsRor *ops);
/* 0x004bc230: in the clamped rectangle, grid cells other than 1 become 2. */
void aoe_ai_info_mark_rect_ror(const AoeAiInfoRor *info, int32_t x0,
                               int32_t y0, int32_t x1, int32_t y1,
                               const AoeAiInfoOpsRor *ops);
/* 0x004bcc10: remembered objects still alive and now owned by `owner`
   that were seen as `seen_owner` (-1 any). */
int32_t aoe_ai_info_count_owned_ror(const AoeAiInfoRor *info,
                                    int32_t seen_owner, int32_t owner,
                                    const AoeAiInfoOpsRor *ops);
/* 0x004bd250: one of the eight neighbours (from x-1, y-1 clockwise) on the
   map has tile byte +5 & 0x1f equal to `value`. */
int aoe_ai_info_neighbour_terrain_ror(int32_t x, int32_t y, uint32_t value,
                                      const AoeAiInfoOpsRor *ops);
/* 0x004bd300 / 0x004bd330: the map width / height, read once. */
int32_t aoe_ai_info_map_width_ror(AoeAiInfoRor *info,
                                  const AoeAiInfoOpsRor *ops);
int32_t aoe_ai_info_map_height_ror(AoeAiInfoRor *info,
                                   const AoeAiInfoOpsRor *ops);
/* 0x004bd710: the remembered object with this ID (NULL for -1 or none). */
AoeAiInfoObjectRor *aoe_ai_info_find_object_ror(const AoeAiInfoRor *info,
                                                int32_t id);
/* 0x004bd7e0: store an entry: overwrite the one with this ID, else take
   the first free one; with none free the table doubles (new entries
   free). A new object, when `seen` is given, number 201 is 1, the
   statistics player is ours or 0xff, its owner index is positive, its
   owner's +4 is 0 and setting 0x97d is 1, raises its statistics bucket
   (0x004c65c0). NULL when the table cannot grow. */
AoeAiInfoObjectRor *aoe_ai_info_store_object_ror(
    AoeAiInfoRor *info, const AoeAiInfoObjectRor *entry,
    const AoeAiWorldObjectRor *seen, const AoeAiInfoOpsRor *ops);
/* 0x004bd750: remember a seen object: ID, type ID and class, truncated
   x, y, z, owner index, truncated hit points, 0, 0 and the three unit
   values. 0 for no object. */
AoeAiInfoObjectRor *aoe_ai_info_remember_ror(AoeAiInfoRor *info,
                                             const AoeAiWorldObjectRor *object,
                                             float value_108,
                                             float value_100, float range,
                                             const AoeAiInfoOpsRor *ops);
/* Class sets: 0x004be140 the units and buildings the AI tracks by ID
   (0, 2, 3, 4, 6, 0xc, 0xd, 0x11, 0x12, 0x14..0x1c, 0x23, 0x24, 0x27),
   0x004be1c0 the resource classes (5, 7, 8, 9, 0xf, 0x1f, 0x20, 0x21) and
   0x004be100 any of those or the 0x004be200 set (1, 0x1e). */
int aoe_ai_info_unit_class_ror(int32_t type_class);
int aoe_ai_info_resource_class_ror(int32_t type_class);
int aoe_ai_info_known_class_ror(int32_t type_class);
/* 0x004be250: class 3 with type 0, 0xc, 0x2d, 0x31, 0x57, 0x65, 0x68 or
   0x6d. */
int aoe_ai_info_key_building_ror(int16_t type_class, int16_t type_id);
/* 0x004c1ab0: the count of the +0x148 list (0x004e0ba0 asks it of a
   player's AI, +0xf0 then the info at +0xcec). 0x004c3cf0: set +0x204. 0x004c6550: the current
   target of list `i` (0..3), else -1. */
int32_t aoe_ai_info_list_148_count_ror(const AoeAiInfoRor *info);
void aoe_ai_info_set_field_204_ror(AoeAiInfoRor *info, int32_t value);
int32_t aoe_ai_info_current_target_ror(const AoeAiInfoRor *info, int32_t i);
/* 0x004c2d60: clamp a tile to the map; 1 when it moved (a negative
   coordinate becomes 0 and, for y, returns 1 at once). */
int aoe_ai_info_clamp_ror(const AoeAiInfoRor *info, int32_t tile[2]);
/* 0x004c30c0: within squared distance 1 (mode 0) or 2 (mode 1); other
   modes are always near. */
int aoe_ai_info_tiles_near_ror(const int32_t a[2], const int32_t b[2],
                               int32_t mode);
/* 0x004c3110: the direction from a to b: with the same x, 5 when y does
   not shrink, else 1; with the same y, 3 when x does not shrink, else 7;
   otherwise, x shrinking gives 6 or 0 and x growing 4 or 2, for y not
   shrinking or shrinking. */
int32_t aoe_ai_info_direction_ror(const int32_t a[2], const int32_t b[2]);
/* 0x004c34c0: the mark with this ID and tile takes the state, or with
   state 3 is freed (ID -1, tile 0, state 3). 0x004c3550: the mark with
   this ID and state 0. 0x004c3590: a mark with this ID and state 1 within
   `range` of the tile on both axes. */
void aoe_ai_info_set_mark_ror(AoeAiInfoRor *info, int32_t id, uint8_t x,
                              uint8_t y, uint8_t state);
AoeAiInfoMarkRor *aoe_ai_info_open_mark_ror(const AoeAiInfoRor *info,
                                            int32_t id);
int aoe_ai_info_mark_near_ror(const AoeAiInfoRor *info, int32_t id,
                              uint8_t x, uint8_t y, int32_t range);
/* 0x004c3660: record an area: a known map counts it in the 4x4 cell of
   ((x0 + x1) * 4 / size) / 2 (the original uses the x sum for both axes;
   above 4 becomes 3) as ours (+0x40c, player_a) or against us (+0x410,
   player_9); then it fills the first free area, else the one with the
   largest age, whose ID becomes the scan's end index. Returns the area
   index or -1. The port skips the cell count past the 4x4 table (the
   original writes beyond it). 0x004c3620 records a one-tile area. */
int32_t aoe_ai_info_add_area_ror(AoeAiInfoRor *info, uint8_t field_4,
                                 uint8_t x0, uint8_t y0, uint8_t x1,
                                 uint8_t y1, uint8_t player_9,
                                 uint8_t player_a, uint16_t field_c,
                                 uint8_t field_e, uint32_t age,
                                 int32_t field_14,
                                 const AoeAiInfoOpsRor *ops);
int32_t aoe_ai_info_add_tile_area_ror(AoeAiInfoRor *info, uint8_t field_4,
                                      uint8_t x, uint8_t y,
                                      uint8_t player_9, uint8_t player_a,
                                      uint16_t field_c, uint8_t field_e,
                                      uint32_t age, int32_t field_14,
                                      const AoeAiInfoOpsRor *ops);
/* 0x004c3ef0: remembered objects seen as `owner` (or, for -1, by an
   enemy) within `radius` tiles of (x, y). */
int32_t aoe_ai_info_count_near_ror(const AoeAiInfoRor *info, int32_t owner,
                                   int32_t x, int32_t y, int32_t radius,
                                   const AoeAiInfoOpsRor *ops);
/* 0x004c3fb0: an area of kind `player_a` with +9 set has a corner within
   `radius` of the object's truncated position. */
int aoe_ai_info_area_near_ror(const AoeAiInfoRor *info, float x, float y,
                              int32_t radius, uint32_t player_a);
/* 0x004c4090: a remembered type 0x6d seen as an enemy within `radius`. */
int aoe_ai_info_enemy_center_near_ror(const AoeAiInfoRor *info, float x,
                                      float y, int32_t radius,
                                      const AoeAiInfoOpsRor *ops);
/* 0x004c4160: the nearest living (state below 3, hit points not 0)
   remembered object of one of four type IDs, seen as an enemy, within
   `radius`; its ID or -1. */
int32_t aoe_ai_info_nearest_enemy_ror(const AoeAiInfoRor *info, float x,
                                      float y, int32_t radius, int32_t t0,
                                      int32_t t1, int32_t t2, int32_t t3,
                                      const AoeAiInfoOpsRor *ops);
/* 0x004c5560: the target with this ID in list `list` gets wait byte
   `wait` (1), else 0. */
int aoe_ai_info_set_target_wait_ror(AoeAiInfoRor *info, int32_t list,
                                    int32_t id, uint8_t wait);
/* Dropsites (the +0x190 list). 0x004bd030: those of type 0x2d whose
   tile (truncated position) lies in map zone `zone`. 0x004bd130: the
   smallest nonzero truncated distance from tile (x, y) to a type-0x2d
   dropsite's truncated tile, -1 for none. */
int32_t aoe_ai_info_docks_in_zone_ror(const AoeAiInfoRor *info,
                                      uint32_t zone,
                                      const AoeAiInfoOpsRor *ops);
int32_t aoe_ai_info_nearest_dock_ror(const AoeAiInfoRor *info, int32_t x,
                                     int32_t y, const AoeAiInfoOpsRor *ops);
/* 0x004c4f30: the nearest dropsite for a resource object, in the
   object's zone: type 0x6d when `town_center` is set, 0x67 unless the
   resource's type is 0x3b or 0x32, 0x44 only for those. The distance
   (1000.0 when none; a float) and the dropsite ID in `out_id` (-1). */
float aoe_ai_info_nearest_dropsite_ror(const AoeAiInfoRor *info,
                                       const AoeAiWorldObjectRor *resource,
                                       int32_t town_center, int32_t *out_id,
                                       const AoeAiInfoOpsRor *ops);
/* 0x004c50e0: the nearest dropsite for a gatherer in its zone: type
   0x6d, 0x67 when its +0x4c is not 0x10, 0x44 when it is. -1 for none. */
int32_t aoe_ai_info_gatherer_dropsite_ror(const AoeAiInfoRor *info,
                                          const AoeAiWorldObjectRor *gatherer,
                                          const AoeAiInfoOpsRor *ops);
/* 0x004c55b0 / 0x004c5680: the dropsites of type 0x67 / 0x44. */
int32_t aoe_ai_info_count_dropsites_ror(const AoeAiInfoRor *info,
                                        int16_t type_id,
                                        const AoeAiInfoOpsRor *ops);
/* 0x004c53d0: refresh target list `list` (0..3): its best distance
   (+0x540) and target (+0x550) reset to -1; each target not gone whose
   object exists gets the nearest dropsite (with town centres): +8 =
   ftol(2d + 10), +0xe = ftol(d), +0x10 the dropsite; classes 7, 0xf, 8
   and 0x20 then keep the smallest ftol(d) in best distance 0, 1, 2, 3
   with that target. 0x004c5250 refreshes the four lists in turn without
   the gone test, each first resetting its own slot (so a later list can
   wipe what an earlier one recorded). The original leaves the distance on the x87 stack for other
   classes; the port does not model that stack. */
void aoe_ai_info_refresh_targets_ror(AoeAiInfoRor *info, int32_t list,
                                     const AoeAiInfoOpsRor *ops);
void aoe_ai_info_refresh_all_targets_ror(AoeAiInfoRor *info,
                                         const AoeAiInfoOpsRor *ops);
/* 0x004bc7b0: the target of list `k` for our unit `id` (-1 without one,
   or when the unit is gone); `drop` gets its +0x10 (-1 first) and `pos`
   the target object's position. Per target, in order: gone (1) ones are
   skipped; a waiting one (+0xc) counts its wait down (while not gone) and
   is skipped; one in `exclude` is skipped; one whose object is missing is
   marked gone (the list is refreshed afterwards when it was the current
   target). A boat only takes classes 5, 0x1f and 0x21, other units only
   other classes. The score is dx^2 + dy^2 (from our truncated position
   to its tile) + 10 x +8, plus picks / 150 for type 0x3b (skipped when
   +0xe is above 20; +200 with more than 2 gatherers) or picks / 50 for
   others. A score that would win then gains 25 for a class 9 or 10 object
   in state above 2 with +0x44 above 25.0, is skipped near an enemy centre
   (0x004c4090, 25 tiles), gains 500 near our kind-0xf area (0x004c3fb0);
   type 0x32 must be ours, state 2, ungathered and not in the +0xf0
   list; list 1 needs no gatherers, lists 2 and 3 fewer than 4; type 0x30
   in state 2 needs tactical slot 0 at least number 204; and the target
   tile must be in our unit's zone. The winner's picks (+6) rise. */
/* 0x004c15d0: the strategic number giving a resource's priority: type
   0x2d number 70, 0x6d 56, 0x9e and 0xa3 54, 0x9f 55; else class 7
   number 52, 8 51, 0x20 50; otherwise -1. */
int32_t aoe_ai_info_resource_priority_ror(int32_t type_id, int32_t type_class,
                                          const AoeAiInfoOpsRor *ops);
/* 0x004c1310: for our unit `id`, the remembered +0x148-listed object with
   +0x10 below 1 and no group of +0x180 4 within number 92 of its tile
   whose priority is lowest (the first on ties); NULL without one. */
AoeAiInfoObjectRor *aoe_ai_info_free_resource_ror(const AoeAiInfoRor *info,
                                                  int32_t id,
                                                  const AoeAiInfoOpsRor *ops);
/* 0x004c1450: for our unit `id`, the remembered +0x148-listed object with
   +0x10 at most 0 that still exists, is in the unit's zone or within its
   range (vtable +0x10c) of a tile of that zone, and whose priority is
   below that of (`type_id`, `type_class`) and the best so far. */
AoeAiInfoObjectRor *aoe_ai_info_better_resource_ror(
    const AoeAiInfoRor *info, int32_t id, int32_t type_id, int32_t type_class,
    const AoeAiInfoOpsRor *ops);
/* 0x004c1ac0: for our unit `id`, the nearest (squared tile distance from
   its truncated position) remembered dock (type 0x2d) seen as another
   player's that still exists and whose owner has resource 9 above 0.0;
   NULL without one. */
AoeAiInfoObjectRor *aoe_ai_info_nearest_trade_dock_ror(
    const AoeAiInfoRor *info, int32_t id, const AoeAiInfoOpsRor *ops);
/* 0x004c1bf0: a site for a building of footprint side `size` (squared
   when above 1, else 2: 0x004c2dc0) on the ring between `inner` and
   `outer` (inner < outer, else 0) around `centre`: the site grid is
   prepared in mode 0 for type -1 over the outer box (centre +- outer +
   inner, clamped); each tile outside the inner box (centre +- outer -
   inner) whose footprint box (+- side / 2) needs no clamping, on which
   our unit could stand (vtable +0x114) over [lo, hi) of that box and
   whose grid cells over the closed box are all below 0xff scores 0x80
   (0x40 when its reach test toward the centre within `outer` fails) plus
   those cells; the first best goes to `out`. 1 with a site. */
int aoe_ai_info_ring_site_ror(AoeAiInfoRor *info, struct AoeValueGridRor *grid,
                              const int32_t centre[2], int32_t size,
                              int32_t outer, int32_t inner, int32_t unit,
                              int32_t out[2], const AoeAiInfoOpsRor *ops);
/* 0x004c2de0: for our unit `id`, a summary of the remembered objects
   seen as `player`'s that still exist, are in the unit's zone (or within
   its range of a tile of that zone) and are within 5 tiles of where they
   were seen or on a visible tile: `mobile` counts those not of class 3 or
   0x1b, `land` those not of class 2, 0x15, 0x16 or 0x14 nor type 0x2d;
   the summary keeps the farthest ftol(unit distance to the seen tile),
   the largest hit points (then +0x10 when positive, then +0x14 when
   nonzero, replacing it) and the largest floats. */
void aoe_ai_info_force_ror(const AoeAiInfoRor *info, AoeAiInfoForceRor *out,
                           uint32_t player, int32_t id, int32_t *mobile,
                           int32_t *land, const AoeAiInfoOpsRor *ops);
/* 0x004c5980: a remembered wonder (type 0x114) of `player` (-1: of any
   player we are the enemy of) that still exists below state 3 and whose
   first action is of type 0x78: the first found, then one in action state
   6 over one in state 2, among state 2 the most hit points, among state 6
   the smallest action +0x2c. NULL for none. */
AoeAiInfoObjectRor *aoe_ai_info_wonder_ror(const AoeAiInfoRor *info,
                                           int32_t player,
                                           const AoeAiInfoOpsRor *ops);
/* 0x004c3820: the area file (+0x208) "learn\\default.<our index>"
   without `name`, else "learn\\<name><our player name>.<our index>",
   read as the destructor writes it: the map size, each area recorded
   (0x004c3660, its ID not used), each object stored (0x004bd7e0) and its
   ID added once to +0x128, the four target lists (capacity, count,
   entries; the old arrays are dropped) and the +0x138 and +0x148 lists
   (values added once). A missing file changes only the path. 0 when the
   file is short. */
int aoe_ai_info_load_area_file_ror(AoeAiInfoRor *info, const char *name,
                                   const AoeAiInfoOpsRor *ops);
/* 0x004c42a0: a tile near remembered object `target` for unit `unit` to
   attack from or land at. The unit's nearby-tile search from (x, y)
   (mode 1 and terrain 0x16 for a boat when `landing` is 0, else mode 2,
   terrain 0) gives the zone to keep. The two influence maps are built
   for the target's owner around its tile; over its tile +- `radius`
   (clamped) a tile qualifies by terrain (tile byte 5 & 0x1f): with
   `landing` 1, terrain 2 and every non-water (1, 0x16) tile within 2 of
   it standable for the unit; else with `land` 1 any non-water tile; with
   `land` 0 water only; any other `land` every tile. In the unit's zone it
   scores ftol(its distance to the target's tile) + rand() % (3 x radius)
   + radius when the unit's reach test toward the target's clamped tile
   within 1.0 passes - map 0's cell x radius / map 0's maximum - map 1's
   cell x radius / map 1's maximum (each only for a positive maximum);
   the first best goes to `out`. 1 with a tile. */
int aoe_ai_info_approach_tile_ror(AoeAiInfoRor *info, int32_t x, int32_t y,
                                  int32_t out[2], int32_t landing,
                                  int32_t land, int32_t target,
                                  int32_t radius, int32_t unit,
                                  const AoeAiInfoOpsRor *ops);
int32_t aoe_ai_info_pick_target_ror(AoeAiInfoRor *info, int32_t k,
                                    int32_t id, float pos[2], int32_t *drop,
                                    const int32_t *exclude,
                                    int32_t exclude_count,
                                    const AoeAiInfoOpsRor *ops);
/* 0x004c5750: the threat at tile (x, y) on unit `id`: for each live
   remembered object seen as an enemy whose distance to the point is at
   most its type's +0x28, value_104(id, it) / value_100(it) when both are
   nonzero, summed as a float. 0.0 when `id` does not exist. */
float aoe_ai_info_threat_ror(const AoeAiInfoRor *info, int32_t id,
                             int32_t x, int32_t y, const AoeAiInfoOpsRor *ops);
/* 0x004c5880: a live remembered object seen as `owner` (-1: an enemy)
   reaches tile (x, y): its distance (as a float) is within its range, or,
   unless `range_only`, within its type's +0x28 when that is larger. */
int aoe_ai_info_in_reach_ror(const AoeAiInfoRor *info, int32_t owner,
                             int32_t x, int32_t y, int32_t range_only,
                             const AoeAiInfoOpsRor *ops);
/* 0x004c62f0 (a group's members, 0x004ccf20 / 0x004ccf30) and
   0x004c6380 (an ID array): the sum of value_104(attacker, it) /
   value_100(it) over existing objects, as a float. 0x004c6400 / 0x004c64b0:
   the sum of hit points / threat at the unit's truncated tile (0x004c5750
   for each), 600 when the threat is 0. */
float aoe_ai_info_group_damage_ror(const AoeAiInfoRor *info,
                                   const AoeAiUnitGroupRor *group,
                                   int32_t attacker,
                                   const AoeAiInfoOpsRor *ops);
float aoe_ai_info_ids_damage_ror(const AoeAiInfoRor *info,
                                 const int32_t *ids, int32_t count,
                                 int32_t attacker,
                                 const AoeAiInfoOpsRor *ops);
float aoe_ai_info_group_endurance_ror(const AoeAiInfoRor *info,
                                      const AoeAiUnitGroupRor *group,
                                      const AoeAiWorldObjectRor *unit,
                                      const AoeAiInfoOpsRor *ops);
float aoe_ai_info_ids_endurance_ror(const AoeAiInfoRor *info,
                                    const int32_t *ids, int32_t count,
                                    const AoeAiWorldObjectRor *unit,
                                    const AoeAiInfoOpsRor *ops);
/* 0x004c1730: special objects go on the +0x148 list (once): a dock
   (type 0x2d) or town centre (0x6d) of ours while number 70 / 56 is
   positive, types 0x9e and 0xa3 while 54 is, 0x9f while 55 is, and the
   classes 7, 8 and 0x20 while 52, 51 and 50 are. 1 when the object is of
   such a kind with its number positive (already listed or not). */
int aoe_ai_info_note_special_ror(AoeAiInfoRor *info,
                                 const AoeAiWorldObjectRor *object,
                                 const AoeAiInfoOpsRor *ops);
/* 0x004c49c0: add a resource object to target list 1 (class 0xf), 2
   (class 8), 3 (class 0x20) or 0 (others). A full list grows to twice
   its capacity (100 at first; new entries free). A listed object gets
   +8 = ftol(2d + 10) and its dropsite; a new one its tile, list index
   (+0xf), +0xe = ftol(d), dropsite and +8. Classes 0xf, 0x20, 8 and 7
   then keep the smallest ftol(d) as before. 0 when the list cannot grow,
   else 1. */
int aoe_ai_info_add_target_ror(AoeAiInfoRor *info,
                               const AoeAiWorldObjectRor *object,
                               const AoeAiInfoOpsRor *ops);
/* 0x004bf250: one of our objects appeared: its remembered owner is
   updated, it joins the +0x180 list and, for building classes 3 and 0x1b,
   the dropsite list; town centres, 0x67 and 0x44 refresh all target
   lists (0x004c5250) and type 0x32 becomes a target (0x004c49c0). */
void aoe_ai_info_own_object_ror(AoeAiInfoRor *info,
                                const AoeAiWorldObjectRor *object,
                                const AoeAiInfoOpsRor *ops);
/* 0x004bf440: one of our objects is gone from the dropsite and +0x180
   lists (searched up to their capacity, as in the original); if it still
   exists its remembered owner is updated. */
void aoe_ai_info_lose_object_ror(AoeAiInfoRor *info, int32_t id,
                                 const AoeAiInfoOpsRor *ops);
/* 0x004bdd10: an object was seen. Known classes only: a sighting of
   our own (not `from_ally`) of an enemy-owned object, in a game of more
   than three players with number 194 at 1, is passed to every computer
   ally (+4 set) that names us an ally. Then the special list (0x004c1730),
   the +0x118 list, and the unit (+0x128), resource (0x004c49c0) or
   +0x138 list; units and special objects are remembered (0x004bd750).
   0x004be0d0 looks the object up by ID first. */
int aoe_ai_info_notice_ror(AoeAiInfoRor *info,
                           const AoeAiWorldObjectRor *object, int32_t from_ally,
                           const AoeAiInfoOpsRor *ops);
int aoe_ai_info_notice_id_ror(AoeAiInfoRor *info, int32_t id,
                              const AoeAiInfoOpsRor *ops);
/* 0x004bd360: raise the pathfinder penalty by 10 (to 255) over the
   clamped square of the type's +0x28 around each live remembered object
   seen as `owner` (other than `skip`), and again over its range square
   when the range is above 1.0. */
void aoe_ai_info_penalize_objects_ror(const AoeAiInfoRor *info,
                                      uint32_t owner, int32_t skip,
                                      const AoeAiInfoOpsRor *ops);
/* 0x004bd590: for each of our areas of kind `player_a`, set the penalty
   to 0x4b over its clamped rectangle, then to 0xf over that rectangle
   grown by 5 (which also covers the first). */
void aoe_ai_info_penalize_areas_ror(const AoeAiInfoRor *info,
                                    uint32_t player_a,
                                    const AoeAiInfoOpsRor *ops);
/* 0x004c4770: influence map 0 from the origin: each live remembered
   object seen as `owner` adds 5 (1 when its vtable +0x108 value is not
   above 0) over the square of its truncated type +0x28 around its
   truncated tile. 0x004c4880: map 1 gains 1 over each of our areas of
   kind `player_a`. Both return the map's largest cell (the original
   passes on the scan's result); 0 without the map ops. */
int32_t aoe_ai_info_unit_influence_ror(const AoeAiInfoRor *info, uint32_t owner,
                                    int32_t x, int32_t y,
                                    const AoeAiInfoOpsRor *ops);
int32_t aoe_ai_info_area_influence_ror(const AoeAiInfoRor *info,
                                    uint32_t player_a, int32_t x, int32_t y,
                                    const AoeAiInfoOpsRor *ops);
/* 0x004c5b80: among live remembered objects of type 0x9e or 0xa3 seen
   as `owner` (-1: an enemy), the one with the fewest such objects within
   5 tiles (0x004c3ef0), that count multiplied by the truncated distance
   from our first town centre to its tile when we have one; the first
   wins ties. 0x004c5d10 does the same for type 0x9f, but only for
   objects on explored tiles, whose remembered tile is refreshed first and
   whose distance is object to object. NULL for none. */
AoeAiInfoObjectRor *aoe_ai_info_best_9e_ror(AoeAiInfoRor *info,
                                            uint32_t owner,
                                            const AoeAiInfoOpsRor *ops);
AoeAiInfoObjectRor *aoe_ai_info_best_9f_ror(AoeAiInfoRor *info,
                                            uint32_t owner,
                                            const AoeAiInfoOpsRor *ops);
/* 0x004c5ec0: which resource to gather next for `mode`: mode 2 none
   (0); with no granary on the build list and a wood target, 1; in mode
   1, wood (1) when its distance passes number 164, gold (3) past 166,
   food (2) past 15 tiles, stone (2) past 165; else -1. */
int32_t aoe_ai_info_resource_need_ror(const AoeAiInfoRor *info, int32_t mode,
                                      const AoeAiInfoOpsRor *ops);
/* 0x004c5fc0: list `k`'s best distance is above number 163 + k, when the
   age table (age = ftol(resource 6) - 1, 0..3) allows it and, with a town
   centre, the target lies in the centre's zone. 0x004c60b0: the same for
   list 0 without the age table. */
int aoe_ai_info_target_far_ror(const AoeAiInfoRor *info, int32_t k,
                               const AoeAiInfoOpsRor *ops);
int aoe_ai_info_food_far_ror(const AoeAiInfoRor *info,
                             const AoeAiInfoOpsRor *ops);
/* 0x004c6170: scan the whole known map: each tile counts in its 4x4 cell
   (+0x408), is cleared in the explored grid, and every object of a known
   class on it is noticed (0x004bdd10); a tile of terrain 10, 0x13, 0xd or
   0x14 holding a class-0xf object raises +0x560. */
void aoe_ai_info_scan_map_ror(AoeAiInfoRor *info,
                              const AoeAiInfoOpsRor *ops);
/* 0x004bb8d0: the same for the tiles explored since the last call (the
   tail of the explored-tile list), except that on a tile the vision map
   holds as explored but unseen (0x80) only objects whose type is shown
   in fog (+0x65) count. Always 1. */
int aoe_ai_info_scan_new_tiles_ror(AoeAiInfoRor *info,
                                   const AoeAiInfoOpsRor *ops);
/* 0x004c4930: append a value to the kind's array (a new array one longer
   each time). */
void aoe_ai_info_kind_append_ror(AoeAiInfoRor *info, int32_t kind,
                                 int32_t value);
/* 0x004c3180: a building type's site priority: 0x67 1, 0x44 2, 0x45 /
   0x4f / 199 3, 0x48 / 0x75 / 0x9b 4, 0x46 / 0x52 / 0x54 / 0x6e / 0x114 6,
   0x2d 7, 0x32 8, 0x6d 9, others 5. */
int32_t aoe_ai_info_site_priority_ror(int32_t type_id);
/* 0x004c3300: set the state of the mark with this ID and tile, else fill
   the first free mark, else double the table (new marks free, with the
   given state) and append. The original writes into an empty (zero-byte)
   table when there are no marks yet; the port then stores nothing. */
void aoe_ai_info_put_mark_ror(AoeAiInfoRor *info, int32_t id, uint8_t x,
                              uint8_t y, uint8_t state);
/* 0x004c3d00: for unit `id`, the nearest remembered object of another
   player (not ours, not 0) on an unexplored tile (0x004bc720) of the
   unit's zone, by truncated distance; its tile goes to `out`, and the
   unit's path test there (radius type +0x28 - 1, height its truncated z)
   marks the grid cell 0 (1 returned) or 1 (0 returned). 0 without one. */
int aoe_ai_info_explore_toward_ror(AoeAiInfoRor *info, int32_t id,
                                   int32_t out[2],
                                   const AoeAiInfoOpsRor *ops);
/* 0x004bc2d0: for unit `id` at (x, y) (truncated), a square spiral that
   starts in direction rand() % 4 (0 y+1, 1 x+1, 2 y-1, 3 x-1; legs of 1,
   1, 2, 2, 3, 3, ... tiles, each leg turning to the next direction)
   looks for a tile not visible now (vision not 0xf) whose explored-grid
   value is 0xff (also 2 unless the unit is of class 4) where the unit
   could stand (its vtable +0x114, the last argument 1) in its own zone.
   Its path test there (as above) then marks the cell 0 and gives that
   tile; when it fails the cell becomes 1 and the target is a random
   corner (rand() % 4: 0 low x/low y, 1 low x/high y, 2 high/high, 3 high
   x/low y; high = low + size / 4 - 1) of the 4x4 map cell with the
   smallest cell count + 25 x its second word + rand() % 100 (first
   smallest wins), its low corner size x index / 4. 1 then, with `out`
   set. Once the spiral has run off all four map edges the unit's zone
   is flagged (0x004bc6e0) and 0 returned, as for a missing unit. */
int aoe_ai_info_explore_step_ror(AoeAiInfoRor *info, int32_t id, float x,
                                 float y, float out[2],
                                 const AoeAiInfoOpsRor *ops);
/* 0x004c6780: no remembered enemy town centre, granary, storage pit or
   type 0x4f, 199 or 0x45 lies within number 202 tiles of (x, y). */
int aoe_ai_info_clear_of_enemy_ror(const AoeAiInfoRor *info, int32_t x,
                                   int32_t y, const AoeAiInfoOpsRor *ops);
/* 0x004c6670: load the eleven statistics (44 bytes) from "learn\<name>.uh"
   for a named player, else "learn\Default.uh". */
void aoe_ai_info_load_learning_ror(AoeAiInfoRor *info,
                                   const AoeAiInfoOpsRor *ops);
/* 0x004c65c0: the statistics bucket of a unit: class 0x12 5, 0x1c 3,
   0xd 4, 0xc 2, 0x23 7; type 0x49 0, 0x4b 1, 4/5/0x27 6, 0x29 10,
   0x2e/0x19 9; class 0x16 8; else -1. */
int32_t aoe_ai_info_stat_bucket_ror(int16_t type_class, int16_t type_id);
/* 0x004c6570: 100 - ftol(statistic 0 / the sum of the history * 100). */
int32_t aoe_ai_info_stat_share_ror(const AoeAiInfoRor *info);
/* 0x004c6710: the largest and second-largest statistics buckets (ties
   keep the first; -1 when none). */
void aoe_ai_info_top_stats_ror(const AoeAiInfoRor *info, int32_t *first,
                               int32_t *second);

/* 0x004c1ef0(mode, type, centre, lo, hi): the site grid (the 254 x 254
   global 0x007c04a0) prepared for a building of `type` with placement
   mode `mode` (site priority, 0..9):
   - filled with 20, cap 255 (two build-list counts are taken and unused);
   - modes 0, 3, 4: map tiles within 5 of the map's edge lowered by 10;
   - modes 1, 2, 9: each known target (not gone) blocked: its centre tile
     (modes 1, 2) or its box (x - sx - 1 .. x + sy + 1, y - sy - 1 .. y +
     sy + 1; the x bound uses the y size, as the original does) set to
     255; mode 1 then, for class 7, lowers the box grown by 5 by 0x80, and
     for the class matching the resource wanted (0x004c5ec0: none -1, 3
     class 0x20, 2 class 8, 1 class 0xf, 0 any other class) raises it
     grown by 1, 4 and 9 by 8, 4 and 2; mode 2 lowers a class 7 box grown
     by 3 by 0x80;
   - modes 0..6 and 8: each dropsite (+0x190 list) blocked by its footprint
     (by its type's half size 2.5, 1.5 (2.5 for type 0x46), 1.0 or 0.5 and
     its site priority; other sizes are logged and skipped), then: modes
     0 and 3 lower (mode 0 with priority 3: raise) a box grown by its
     +0x28 (halved in mode 3) by 10; modes 1 and 2 lower one grown by +0x28
     by 10; mode 5 lowers one grown by twice +0x28 by 20; mode 8 raises
     boxes grown by +0x28, half and a third of it by 20 each. Priority
     comparisons against 0x6d, 0x44 and 0x67 never match (priorities run
     1..9) and are kept;
   - modes 0, 3, 4: over lo..hi, water (terrain 1) tiles are set to 255
     with the single cell at (x - 2, y - 2) lowered by 10, and every tile
     raised by 5 x its elevation (tile byte 5 >> 5);
   - modes 0, 3..6: our areas (+0x200, player byte +0xa) raise their box
     grown by 4 by 16;
   - mode 0: the tactical AI's pass;
   - the open-neighbour counts (threshold 255, off-edge 1, off-corner
     unless mode 4, diagonals) over the map;
   - other modes: marks for `type` in state 2 set to 255. */
void aoe_ai_info_prepare_sites_ror(AoeAiInfoRor *info, struct AoeValueGridRor *grid,
                                   int32_t mode, int32_t type, const int32_t lo[2],
                                   const int32_t hi[2], const AoeAiInfoOpsRor *ops);

/* The site finder's saved progress (0x4c dwords; +0x114 the state). */
typedef struct {
    int32_t type;                 /* [0] */
    float size;                   /* [1] */
    char name[256];               /* [2] */
    int32_t builder;              /* [0x42] */
    int32_t min, max;             /* [0x43], [0x44] (saved, not reused) */
    int32_t state;                /* [0x45]: 1 resuming */
    int32_t best_x, best_y;       /* [0x46], [0x47] */
    int32_t best;                 /* [0x48] */
    int32_t cursor_x;             /* [0x49] */
    int32_t jitter;               /* [0x4a] */
    int has_item;                 /* [0x4b] */
} AoeAiSiteSearchRor;

/* 0x004be2e0(item, builder, type, size, name, min, max, x, y, search,
   start): a site for a building, searched over the site grid in time
   slices. Without an object of class 4 or a template of the type it
   fails (state 0, NULL); so does a type without a site priority. The
   centre: our town centre (type 0x6d), else the builder, else that
   class-4 object; a request without an item uses the given point and
   distances. Otherwise by site priority: 1 (houses) 7 to number 0x56 x
   the age (1, or 2..4 by the tactical AI's age test); 2 4 to number
   0x57; 3 and 4 number 0x49 to 0x4a; others 4 to 0x4a (a maximum not
   above the minimum fails). An item request first takes an open mark
   of its type (made state 1; priority 4 also moves the last-site pair
   +0x1a0 / +0x1a8). Then the grid is prepared (0x004c1ef0) and every
   tile of the outer box (priorities 3 and 4 skip the inner box) is
   scored: its footprint (by size 5, 3, 2 or 1 and priority; another
   size fails) must lie inside the map and pass the placement check;
   the score is the grid's sum over the footprint (a 255 cell rules the
   tile out); priorities 4 and 6 multiply it by the open-neighbour score
   less the grid's last count (priority 4 drops tiles above 6), priority
   4 halves it away from the last site and triples it along the last
   direction (and its repeat, and along the axis the last site leans
   to), priorities 1 and 2 divide it by 10 near a placed mark of the
   type (others except 6 within the size) and take one per five tiles
   past ten from the centre while above 20 x size x size. The best tile
   wins (a random jitter of number 0xa8 favours later ties, halved each
   time a tile loses). Whenever the time slice is used up the search is
   saved and returns NULL (state 1). Failure for no tile, or for
   priorities 1 and 2 at most 20 x size x size. Priority 4 records the
   site. A plain request marks the tile (state 0) and returns 1 through
   `found`; an item request marks it (state 1) and returns a new item at
   the tile (+0.5 unless the size is 2). */
AoeAiConItemRor *aoe_ai_info_find_site_ror(AoeAiInfoRor *info, struct AoeValueGridRor *grid,
                                           const AoeAiPlanItemRor *item, int32_t builder,
                                           int32_t type, float size, const char *name,
                                           int32_t min, int32_t max, int32_t x, int32_t y,
                                           AoeAiSiteSearchRor *search, uint32_t start,
                                           int *found, const AoeAiInfoOpsRor *ops);

/* 0x004bcca0(item, x, y, terrain, value): a shore-side site near
   (x, y) for `item` (its RoR plan's 0x004b9f80 asks for terrain 1 and
   neighbour terrain 2): every interior tile (1 .. size - 3) that the
   player has seen, whose terrain (tile byte 5 & 0x1f) matches unless -1,
   whose zone has at least number 0x70 tiles, with a neighbour of terrain
   `value` (0x004bd250) and no placed plan item (0x0040a6e0 on the
   construction plan) is scored by its squared distance, plus 1000 per
   dock in its zone (counted once per zone) and 100000 / the distance to
   the nearest dock; the lowest wins, a later tile replacing the best
   only when it beats it by more than a jitter of rand() % 50. The new
   item stands at the tile + 0.5 at height 0 with the item's sizes, type
   and name; NULL when no tile qualifies. */
AoeAiConItemRor *aoe_ai_info_shore_site_ror(AoeAiInfoRor *info, AoeAiConPlanRor *plan,
                                            const AoeAiPlanItemRor *item, float x, float y,
                                            int32_t terrain, uint32_t value,
                                            const AoeAiInfoOpsRor *ops);

/* 0x004b9f80 (the RoR plan, its owner's information AI at +0x314 +
   0xcec): a shore site for `item` near the plan's origin (terrain 1,
   neighbour terrain 2) appended to the placed list (+0x178, count
   +0x174), +0x310 0; none sets +0x310 to 5 and gives NULL. */
AoeAiConItemRor *aoe_ai_ror_plan_add_shore_ror(AoeAiConPlanRor *plan, AoeAiInfoRor *info,
                                               const AoeAiPlanItemRor *item,
                                               const AoeAiInfoOpsRor *ops);

/* 0x004b98e0(item, builder, search, start), the RoR plan's site for an
   item: without an origin inside the map the information AI's finder
   decides (0x004be2e0 with the item, the builder and the search); docks
   (0x2d) take a shore site (0x004b9f80) and return a copy; types 0x67,
   0x44, 0x4f, 199, 0x45, 0x48, 0x75 and 0x9b, free placement or an
   empty plan go to the finder. Otherwise, with an object of class 4 (else
   NULL), each unused planned entry of the item's type (or none) and its
   size whose footprint (from x - size_x, y - size_y, size_x by size_y
   tiles) lies inside the map, in the builder's zone and free for that
   object is a candidate; with no candidate used fewer than twice the
   finder is asked first. Without candidates +0x310 becomes 5 (NULL).
   The chosen candidate is the first, replaced by any used more than five
   fewer times (type 0x48), else the least used (0x004ba0a0); the plan
   entry there (0x0040a640) is marked used (+0x74 1), its count raised and
   its unique ID set from the item; the result is a copy of that entry
   with the item's type, +0x310 0. */
AoeAiConItemRor *aoe_ai_ror_plan_choose_ror(AoeAiConPlanRor *plan, AoeAiInfoRor *info,
                                            struct AoeValueGridRor *grid,
                                            const AoeAiPlanItemRor *item, int32_t builder,
                                            int free_placement, AoeAiSiteSearchRor *search,
                                            uint32_t start, const AoeAiInfoOpsRor *ops);

#endif
