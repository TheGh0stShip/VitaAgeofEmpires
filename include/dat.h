#ifndef VITA_AOE_DAT_H
#define VITA_AOE_DAT_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *bytes;
    size_t size;
} DatBlob;

/* Player colour record (36 file bytes; loader 0x0041cf00, one object per
   record from the world's table 0x0051c3c0). The name (30 bytes) names
   the palette file read into the object; then two words and two bytes. */
typedef struct {
    char name[31];
    /* File +30 -> +0x26: the colour number, equal to the record's index
       (the text form sets it from the index, 0x0051b9f0); the player's
       colour 0x0045d400, the palette fill 0x004f75b0 ((n + 1) * 16). */
    uint16_t color_id;
    uint16_t field_22;      /* file +32 -> +0x22: no reader */
    /* File +34 -> +0x25: the diamond map's colour byte (0x0042d710,
       0x00431470); file +35 -> +0x24: a player may be given this colour
       (0x0051d060 / 0x0051d1a0 take the next free one with 1). */
    uint8_t minimap_color;
    uint8_t selectable;
} DatColorRecord;

typedef struct {
    uint16_t restriction_count;
    uint16_t terrain_count;
    uint16_t color_count;
    float *terrain_restrictions; /* row-major: restriction_count x terrain_count */
    DatColorRecord *colors;
    size_t next_offset;
} DatIntro;

/* Sound item (loader 0x004a2000; runtime offsets in the comments). The
   play method 0x004a2370 (and the pick 0x004a24a0) draws rand * 100 /
   32767 and takes the first variant whose shares, summed, reach it; the
   variant's object (+0x18, from 0x004a2be0 with the file name and the DRS
   resource) then plays, and the tick 0x004a2310 stops it once it has
   played longer than the item's stop time. */
typedef struct {
    char filename[14];     /* variant +0x00 (13 file bytes) */
    uint32_t resource_id;  /* variant +0x10: the DRS resource */
    uint16_t chance;       /* variant +0x14: its share of the 0..100 draw */
} DatSoundVariant;

typedef struct {
    uint16_t field_0c;     /* item +0x0c (file bytes 0..1): equals the index
                              in the installed data; no translated reader */
    uint16_t field_00;     /* item +0x00 (file bytes 2..3): only cleared */
    /* Item +0x08 (file bytes 6..9): milliseconds after which a playing
       variant is stopped (0x004a2310); 0 never. */
    uint32_t stop_time;
    uint16_t variant_count; /* item +0x02 */
    DatSoundVariant *variants; /* item +0x04, 0x24 bytes each at runtime */
} DatSound;

typedef struct {
    uint16_t count;
    size_t variant_count;
    DatSound *sounds;
    size_t next_offset;
} DatSounds;

struct DatGraphic;

typedef struct {
    int16_t graphic_id;
    const struct DatGraphic *graphic; /* resolved after the full table loads */
    uint32_t pointer_placeholder;
    int16_t offset_x;       /* +0x08, +0x0a: added to the draw point (0x004a5a50) */
    int16_t offset_y;
    /* +0x0c: the walks 0x004a5a50 / 0x004a5b50 draw the delta only for this
       angle (-1 for all); +0x0e has no reader. */
    int16_t display_angle;
    int16_t field_0e;
} DatGraphicDelta;

typedef struct {
    int16_t delay;
    int16_t sound_id;
} DatGraphicAttackSound;

typedef struct DatGraphic {
    int present;
    uint32_t pointer_placeholder;
    char name[22];
    char filename[14];
    int32_t slp_id;
    uint8_t layer;           /* +0x29 */
    int16_t player_color;    /* +0x2a */
    /* +0x2e..+0x34 (record bytes 46..53): an explicit screen box about the
       draw point, x0 y0 x1 y1; the bounds 0x004a5d60 return it unless a pair
       is zero, and the draw 0x004a55e0 hands it to the deferred part's rect
       (0x00516390). */
    int16_t box[4];
    int16_t sound_id;
    uint16_t frame_count;
    uint16_t angle_count;
    /* Reader 0x004a4c40 places the record at runtime offsets: +0x5e frame
       count, +0x60 angle count, then +0x64 the speed the frame time is
       given for (the animation 0x00415eb0 scales the frame time by it
       over the unit's speed; 0 fixes it), +0x68 the frame time, +0x6c the
       pause after each loop, +0x70 the sequence flags (bit 0 animated,
       bit 2 random start, bit 3 stop on the last frame), +0x74 the
       mirroring limit (frame selection 0x004a5520). */
    float reference_speed;
    float frame_duration;
    float loop_delay;
    uint8_t sequence_flags;
    uint8_t mirror_limit;
    /* RoR runtime +0x14 (record byte 38): the SLP may be loaded on use. */
    uint8_t slp_loadable;
    /* RoR runtime +0x2c (record byte 43): selection test, 0 none, 1 SLP
       pixels, 2 the frame box (0x004a6150). */
    uint8_t selection_mode;
    uint16_t delta_count;
    DatGraphicDelta *deltas;
    size_t attack_sound_count; /* 3 pairs per angle when attack sounds are used. */
    DatGraphicAttackSound *attack_sounds;
} DatGraphic;

typedef struct {
    uint16_t count; /* Includes empty slots; graphic ID is the array index. */
    size_t present_count;
    DatGraphic *graphics;
    size_t next_offset;
} DatGraphics;

typedef struct {
    int16_t width;
    int16_t height;
    int16_t elevation_delta;
} DatTileSize;

typedef struct {
    int16_t frame_count;
    int16_t angle_count;
    int16_t frame_start; /* First SLP frame for this shape/pattern. */
} DatTerrainFrame;

/* Terrain record: the map object's 0x198-byte terrain entries at map
   +0x8c, read raw as part of the 0x8dd0-byte map image (0x00443ce0) and
   written field by field by the map's text loader 0x00446bb0. Offsets are
   the record's. field_XX marks an offset no translated function reads. */
typedef struct {
    uint8_t enabled;               /* +0x00 */
    uint8_t field_01;
    char name[14];                 /* +0x02 */
    char filename[14];             /* +0x0f */
    int32_t slp_id;                /* +0x1c */
    uint32_t shape_pointer_placeholder; /* +0x20 */
    int32_t sound_id;              /* +0x24 */
    uint8_t map_colors[5];         /* +0x28..+0x2c, the diamond map */
    int8_t field_2d, field_2e;
    uint8_t field_2f;
    int16_t field_30, field_32;
    float field_34, field_38;
    DatTerrainFrame elevation_frames[19]; /* +0x46, per elevation shape */
    int16_t replacement_id;        /* +0xb8 */
    int16_t frame_grid[2]; /* +0xba: rows, columns; renderer uses their low bytes. */
    int16_t borders[32];           /* +0xbe, the border between terrains */
    /* +0xfe / +0x13a / +0x176 / +0x194: the objects the terrain change
       0x004481b0 places on a converted tile: IDs, amounts, placement
       modes and their count. */
    int16_t object_ids[30];
    int16_t object_amounts[30];
    uint8_t object_modes[30];
    uint16_t object_count;
} DatTerrain;

typedef struct {
    uint32_t map_header[6];
    DatTileSize tile_sizes[19];
    uint16_t count; /* Fixed-size terrain slot table in this release. */
    uint16_t enabled_count;
    DatTerrain *terrains;
    size_t next_offset;
} DatTerrains;

/* Border record: the map object's 0x5a0-byte border entries, read raw
   with the map image (0x00443ce0). */
typedef struct {
    uint8_t enabled;               /* +0x00 */
    uint8_t field_01;
    char name[14];
    char filename[14];
    int32_t slp_id;                /* +0x1c */
    uint32_t shape_pointer_placeholder;
    int32_t sound_id;
    uint8_t map_colors[3];
    uint8_t field_2b;
    int16_t field_2c, field_2e;
    float field_30, field_34;
    int16_t field_38, field_3a;
    float field_3c;
    uint8_t field_40, field_41;
    DatTerrainFrame frames[19][12]; /* +0x42 */
    /* +0x59a: draw the base terrain under the border (tile draw
       0x00514c10); +0x59c the terrain a border tile counts as for
       movement (0x0044ead0); +0x59e the pattern set. */
    int16_t draw_base;
    int16_t base_terrain;
    int16_t border_style;
} DatBorder;

typedef struct {
    uint16_t count; /* Fixed 16 slots in this VER 3.7 file. */
    uint16_t enabled_count;
    DatBorder *borders;
    size_t next_offset;
} DatBorders;

typedef struct {
    uint32_t map_row_pointer_placeholder;
    uint16_t terrains_used;
    uint16_t borders_used;
    int16_t max_terrain;
    int16_t tile_width;
    int16_t tile_height;
    int16_t tile_half_height;
    int16_t tile_half_width;
    int16_t elevation_height;
    size_t next_offset;
} DatMapGeometry;

/* Random-map records, decoded field by field from the DAT stream read by
   patched RoR 1.0a 0x0046da10 (base 1.0c 0x0046e5e0). Widths come from the
   same class's text loader RoR 0x0046d580; the serialized padding bytes are
   not retained. Consumers named below are RoR 1.0a; see docs/RANDOM_MAP.md.
   "Generator x/y" are the axes sized by generator +0x14/+0x18. */

/* Land record, 44 bytes. */
typedef struct {
    int32_t id;              /* +0x00, to the object setup's land table */
    uint8_t field_04;        /* +0x04, zero-extended to land work +0x40 */
    int32_t field_08;        /* +0x08, to land work +0x50 */
    int32_t field_0c;        /* +0x0c, to land work +0x4c */
    uint8_t field_10;        /* +0x10, to land work +0x54 (see per_player) */
    uint8_t placement_mode;  /* +0x11: 0 random, 1 fixed x/y, 2 spaced random */
    int32_t x, y;            /* +0x14/+0x18, used by placement mode 1 */
    uint8_t area_percent;    /* +0x1c: share of the map's land area */
    /* +0x1d: 0 places one land; otherwise one per player, and only 1 keeps
       +0x10 as the work byte (other values use the player index). */
    uint8_t per_player;
    int32_t field_20;        /* +0x20; nonnegative adds the land to the
                                terrain and elevation position lists */
    int32_t field_24;        /* +0x24, copied beside +0x20 */
    int32_t field_28;        /* +0x28, to land work +0x58; text default 8 */
} DatRandomMapLand;

/* Terrain record, 24 bytes. Setup 0x0046ead0 copies +0x04..+0x14 to its
   work entry; +0x00 is not copied directly. Text default for +0x14 is 20. */
typedef struct {
    int32_t field_00, field_04, field_08, field_0c, field_10, field_14;
} DatRandomMapTerrain;

/* Object record, 44 bytes, copied unchanged by setup 0x0046ec90. */
typedef struct {
    int32_t field_00, field_04;
    uint8_t field_08, field_09;
    int32_t field_0c, field_10, field_14, field_18;
    int32_t field_1c, field_20, field_24, field_28;
} DatRandomMapObject;

/* Elevation record, 24 bytes. Both revisions load, save and free these but
   no generator code reads them; elevation setup uses fixed layers. */
typedef struct {
    int32_t values[6];
} DatRandomMapElevation;

typedef struct {
    int32_t map_id;
    uint32_t land_count;
    uint32_t terrain_count;
    uint32_t unit_count;      /* object records */
    uint32_t elevation_count;
    /* Header +0x04..+0x24 from the second pass, as the original overwrites. */
    /* +0x04/+0x08/+0x0c/+0x10: low-x, low-y, high-x, high-y edge margins;
       the land bounds are x in [m0, gen_x - m2] and y in [m1, gen_y - m3]. */
    int32_t edge_margin[4];
    int32_t field_14;           /* +0x14, low byte to every land work +0x6c */
    int32_t land_edge_distance; /* +0x18, placement distance from the edge */
    int32_t field_1c;           /* +0x1c, low byte to generator +0x145c */
    int32_t land_percent;       /* +0x20, percent of the map area in lands */
    int32_t field_24;           /* +0x24, not read by either generator */
    DatRandomMapLand *lands;
    DatRandomMapTerrain *terrains;
    DatRandomMapObject *objects;
    DatRandomMapElevation *elevations;
} DatRandomMap;

typedef struct {
    uint32_t count;
    DatRandomMap *maps;
    size_t next_offset;
} DatRandomMaps;

/* Effect command (11 file bytes: type, then four values). Named by the
   attribute applier 0x0045d6a0(template, class, attribute, amount), which
   types 0 / 4 / 5 call: a template ID in range applies to that template,
   otherwise to every template of the class. The other types read the
   values differently (tech_tree.c): type 1 the resource (+0x01) and the
   mode (+0x03: 0 sets, else adds; 0x0045cfc0); type 2 the template and
   its enabled flag (0x0045d010 -> 0x00443130); type 3 the source and
   target templates (0x0045d9a0); type 102 the tech in `amount`. */
typedef struct {
    int8_t type;          /* +0x00 */
    int16_t template_id;  /* +0x01 */
    int16_t class_id;     /* +0x03 */
    int16_t attribute;    /* +0x05 */
    float amount;         /* +0x07 */
} DatEffectCommand;

typedef struct {
    char name[32];
    uint16_t command_count;
    DatEffectCommand *commands;
} DatEffect;

typedef struct {
    uint32_t count;
    DatEffect *effects;
    size_t next_offset;
} DatEffects;

typedef struct {
    int16_t resource_type;
    float amount;
    int8_t paid;
} DatUnitStorage;

typedef struct {
    int16_t resource_type;
    int16_t amount;
    int16_t paid;
} DatUnitCost;

typedef struct {
    int16_t class_id;
    int16_t amount;
} DatUnitAttackArmor;

/* Damage graphic (5 file bytes; runtime template +0x94 entries of 8 bytes,
   loader 0x00441775): graphic +0, damage percent +4 (file byte 2) and
   apply mode +5 (file byte 4; byte 3 is overwritten by the loader). */
typedef struct {
    int16_t graphic_id;
    uint8_t damage_percent;
    uint8_t apply_mode;
} DatDamageGraphic;

/* Unit task (59 file bytes), named by the runtime task's offsets in
   patched RoR 1.0a: loader 0x004e74f0 reads the file record into a
   0x4c-byte task, and the task selectors 0x004e7dc0 / 0x004e7f90 and the
   actions cited below read it. field_XX marks an offset no translated
   function reads. */
typedef struct {
    int16_t field_00;        /* +0x00 */
    int16_t id;              /* +0x02 */
    uint8_t field_04;        /* +0x04 */
    int16_t action_type;     /* +0x06: the action made (aoe_action_route_ror) */
    int16_t target_class;    /* +0x08 */
    int16_t target_template; /* +0x0a: the target's template ID */
    /* +0x0c: the selectors' fourth pass takes the task when any target
       qualifies through template +0x8c (0x004e7dc0). */
    uint8_t any_flagged_target;
    int16_t target_terrain;  /* +0x0e */
    /* +0x10/+0x11/+0x12, read by 0x004e7f90/0x004e7dc0: owner-relationship
       mode 1..5, "target must carry a resource", and "applies to a state-0
       target". */
    uint8_t owner_mode, requires_carried_resource, unbuilt_target;
    /* +0x14: the resource carried while working (gather 0x004b4030). */
    int16_t carry_resource;
    /* +0x16: the player attribute scaling the work rate, -1 none
       (0x00403738 / 0x004b3c5d). */
    int16_t rate_attribute;
    /* +0x18: the resource credited on deposit (0x0044b9a0). */
    int16_t deposit_resource;
    /* +0x1c: work per second against the target (gather 0x00403738, heal
       0x004b30c3, repair 0x004b5bb0, convert 0x004b2461). */
    float work_amount;
    float work_timer;        /* +0x20: the conversion timer (0x004b1cb0 action) */
    /* +0x24: the range the action works within (0x0040250e / 0x00402884,
       heal 0x004b30c3). */
    float action_range;
    uint8_t field_28;        /* +0x28 */
    float field_2c;          /* +0x2c */
    /* +0x30/+0x32 (file bytes 40..43, loader 0x004e75cb). The farm action
       0x004047d0 credits its production straight to the owner when +0x30
       is nonzero. */
    int16_t field_30, field_32;
    /* +0x34 approach, +0x38 action, +0x3c work and +0x40 carrying
       graphics (heal 0x004b2c00, gather 0x004b4030, trade 0x00403b00);
       +0x44 the action sound and +0x48 the deposit sound. */
    int16_t approach_graphic, action_graphic, work_graphic, carry_graphic;
    int16_t action_sound, deposit_sound;
} DatUnitTask;

typedef struct {
    uint8_t present;
    uint8_t type;
    int16_t id;
    int16_t unit_class;
    int16_t standing_graphic;
    int16_t walking_graphic;   /* template +0xbc (type >= 30) */
    int16_t walking_graphic2;  /* template +0xc0 */
    int16_t dying_graphic;
    /* Template +0x20 / +0x24: an emptied corpse (state 3/5) with +0x24 set
       enters state 6 showing +0x20; otherwise it goes to state 7 and is
       cleaned up (live_unit_factory.c). */
    int16_t state6_graphic;
    uint8_t state6_mode;
    /* Original template +0x48: the corpse spawned by terminal cleanup
       0x0044b910 / 0x004a9450 (-1 for none). */
    int16_t dead_unit_id;
    int16_t hit_points;
    uint16_t language_name_id; /* original template +0x0c (loader 0x004413b0) */
    uint16_t language_creation_id; /* +0x0e: the "Create ..." text */
    /* +0x98 / +0x9c / +0xa0: the help string, the tech-tree text and the
       hotkey string IDs (0x0040c1a0 / 0x00407ae0 read the first two). */
    int32_t language_help_id, language_tree_id, hotkey_id;
    uint8_t garrison_capacity;  /* original template +0x2c */
    int16_t icon;           /* template +0x4c: the object panel's picture */
    uint8_t hide_in_editor; /* template +0x4e */
    uint8_t enabled;        /* template +0x52 (effect type 2, 0x00443130) */
    uint8_t placement_mode; /* original template +0x4a */
    uint8_t placement_overlap_mode; /* original template +0x4b */
    int16_t placement_side_terrain[2]; /* original +0x54/+0x56 */
    int16_t placement_terrain[2]; /* original +0x58/+0x5a */
    float placement_clearance[2]; /* original +0x5c/+0x60 */
    uint8_t negative_id_mode; /* original template +0xa4 */
    uint8_t state_notify_mode; /* original template +0xa5 */
    uint8_t state_effect_mode; /* original template +0xa6 */
    uint8_t state_notify_value; /* original template +0xa7 */
    float line_of_sight;
    float size[3];
    uint8_t placement_class; /* template +0x64, editor check 0x0051a392 */
    uint8_t draw_class; /* template +0x65, RoR tile draw 0x00451fd0 */
    int16_t terrain_restriction; /* original template +0x66 */
    uint8_t elevation_mode; /* original template +0x68 */
    /* Original template +0x7c: carried-resource capacity (0x004aace0
       clamps to it; gathering scales it by the task's attribute). */
    int16_t resource_capacity;
    /* Original template +0x8c/+0x8f bytes, read by the RoR task selectors
       0x004e7dc0/0x004e7f90/0x00508010. Their meaning beyond those uses is
       not yet established. */
    uint8_t field_8c, field_8f;
    /* Template +0x8e / +0x90 (loader 0x004413b0): the diamond map's mode
       (1 small, 2 large, 3 large in its own colour, 4 colours its tiles)
       and its colour. */
    uint8_t map_mode, map_color;
    /* Template +0xa8 / +0xa9 (selection box: bit 0 its own colour, bit 1
       no hit-point bar; the colour) and +0xac / +0xb0 / +0xb4, the
       selection outline's half sizes and height (0x004413b0). */
    uint8_t selection_flags, selection_color;
    float outline[3];
    /* Template +0x8b: blasts of a lower level (+0x11c) skip this unit. */
    uint8_t blast_defense_level;
    uint8_t blast_level;    /* type >= 50, template +0x11c */
    float decay_rate;       /* original template +0x80 */
    uint8_t damage_graphic_count;     /* template +0x93 */
    DatDamageGraphic *damage_graphics; /* template +0x94 */
    int16_t selection_sound;
    int16_t dying_sound;    /* resolves to original template +0x44 */
    float speed;
    /* Type >= 30: per-facet turn delay, original template +0xc4 (after
       the two walking-graphic slots); 0x0044cde0 adds it to live +0x12c. */
    float rotation_speed;
    char *name;
    DatUnitStorage storage[3];
    int16_t default_task_id;
    uint8_t task_swap_group; /* original template +0xec (type >= 40) */
    float field_e0, work_rate; /* template +0xe0, +0xe4 */
    int16_t drop_sites[2];          /* type >= 40, template +0xe8/+0xea */
    uint16_t task_count;
    DatUnitTask *tasks;
    uint8_t base_armor;
    uint16_t attack_count, armor_count;
    DatUnitAttackArmor *attacks, *armors;
    /* Type 50 (loader 0x0043f0c0): +0x110 the row of the world's terrain
       multiplier table (damage 0x00426910); +0x124 the projectile; +0x126
       the hit percentage; +0x12a the frame that fires; +0xfc the attack
       graphic. */
    int16_t terrain_table, projectile_unit_id, accuracy_percent;
    int16_t fire_frame, attack_graphic;
    /* Original template +0x114, +0x118, +0x120 and +0x138. */
    float max_range, blast_width, reload_time, min_range;
    float projectile_offset[3]; /* +0x12c/+0x130/+0x134 */
    /* Type 60 (0x00404ab0/0x00404db0): flight mode +0x148 (0 directed,
       1/2 stationary, 3 instant), target leading +0x149, hit mode +0x14a
       and arc +0x150. */
    uint8_t projectile_flight_mode, projectile_lead, projectile_hit_mode;
    float projectile_arc;
    /* +0x13c / +0x13e / +0x140 / +0x144: before 7.04 the loader derives
       them as the largest armor, the largest attack, +0x114 and +0x120
       (0x0043f0c0); the panels show them. */
    int16_t displayed_melee_armor, displayed_attack;
    float displayed_range, displayed_reload_time;
    DatUnitCost costs[3];
    int16_t train_time, train_location_id, displayed_pierce_armor;
    int8_t button_id;
    int16_t construction_graphic; /* type 80, original template +0x168 */
    uint8_t building_adjacent_mode; /* type 80, original template +0x16c */
    /* Type 80 (loader 0x004ec310), template +0x16e: the building's facet
       when placed and the base of its age picture (attribute 17). */
    uint16_t building_frame;
    uint8_t disappears_when_built; /* type 80, original template +0x170 */
    int16_t stack_unit_id;        /* type 80, original template +0x172 */
    int16_t foundation_terrain;   /* type 80, original template +0x174 */
    int16_t field_176;            /* type 80, original template +0x176 */
    int16_t building_research_id; /* type 80, original template +0x178 */
    int16_t sound_164;            /* type 80, template +0x164 (a sound) */
} DatUnit;

typedef struct {
    int8_t player_type;
    char name[21];
    int16_t tech_tree_id;
    uint8_t icon_set;
    uint16_t resource_count;
    float *resources;
    uint16_t unit_count;
    DatUnit *units;
} DatCiv;

typedef struct {
    uint16_t count;
    DatCiv *civs;
    size_t next_offset;
} DatCivilizations;

typedef struct {
    int16_t resource_type, amount;
    int8_t paid;
} DatTechCost;

/* Tech record (reader 0x004e9c60; runtime offsets in the comments). The
   research check 0x0045d9a0 reads the prerequisites, the required count
   and the research time; 0x004ea3bc applies the effect; the help text
   0x004eba87 shows the name and description strings. */
typedef struct {
    int16_t required_techs[4];       /* +0x00 */
    DatTechCost costs[3];            /* +0x08, 5 file bytes each */
    int16_t required_count;          /* +0x1c */
    int16_t research_location;       /* +0x28: the building that researches */
    int16_t research_time;           /* +0x1e */
    int16_t effect_id;               /* +0x20 */
    int16_t field_22;                /* +0x22: no translated function reads it */
    int16_t field_24;                /* +0x24: no translated function reads it */
    uint16_t language_name, language_description; /* +0x2a, +0x2c */
    int8_t button_id;                /* +0x26: the research button slot */
    int32_t language_help, language_tree, hotkey; /* +0x30, +0x34, +0x38 */
    char *name;
} DatTech;

typedef struct {
    uint16_t count;
    DatTech *techs;
    size_t next_offset;
} DatTechs;

/* Loads the original raw-DEFLATE Empires.dat and checks its VER 3.7 header. */
int dat_load(const char *path, DatBlob *blob);
void dat_free(DatBlob *blob);
int dat_parse_intro(const DatBlob *blob, DatIntro *intro);
void dat_free_intro(DatIntro *intro);
int dat_parse_sounds(const DatBlob *blob, const DatIntro *intro, DatSounds *sounds);
void dat_free_sounds(DatSounds *sounds);
int dat_parse_graphics(const DatBlob *blob, const DatSounds *sounds,
                       DatGraphics *graphics);
void dat_free_graphics(DatGraphics *graphics);
int dat_parse_terrains(const DatBlob *blob, const DatIntro *intro,
                       const DatGraphics *graphics, DatTerrains *terrains);
void dat_free_terrains(DatTerrains *terrains);
int dat_parse_borders(const DatBlob *blob, const DatTerrains *terrains,
                      DatBorders *borders);
void dat_free_borders(DatBorders *borders);
int dat_parse_map_geometry(const DatBlob *blob, const DatTerrains *terrains,
                           const DatBorders *borders, DatMapGeometry *geometry);
int dat_parse_random_maps(const DatBlob *blob, const DatMapGeometry *geometry,
                          DatRandomMaps *maps);
void dat_free_random_maps(DatRandomMaps *maps);
int dat_parse_effects(const DatBlob *blob, const DatRandomMaps *maps,
                      DatEffects *effects);
void dat_free_effects(DatEffects *effects);
int dat_parse_civilizations(const DatBlob *blob, const DatEffects *effects,
                            DatCivilizations *civilizations);
void dat_free_civilizations(DatCivilizations *civilizations);
const DatUnit *dat_find_unit(const DatCivilizations *civilizations,
                             uint16_t civilization_id, uint16_t unit_id);
int dat_parse_techs(const DatBlob *blob, const DatCivilizations *civilizations,
                    DatTechs *techs);
void dat_free_techs(DatTechs *techs);

#endif
