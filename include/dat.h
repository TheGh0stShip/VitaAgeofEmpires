#ifndef VITA_AOE_DAT_H
#define VITA_AOE_DAT_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *bytes;
    size_t size;
} DatBlob;

typedef struct {
    char name[31];
    uint16_t ordinal;
    uint16_t unknown;
    uint16_t value;
} DatColorRecord;

typedef struct {
    uint16_t restriction_count;
    uint16_t terrain_count;
    uint16_t color_count;
    float *terrain_restrictions; /* row-major: restriction_count x terrain_count */
    DatColorRecord *colors;
    size_t next_offset;
} DatIntro;

typedef struct {
    char filename[14];
    uint32_t resource_id;
    uint16_t weight;
} DatSoundVariant;

typedef struct {
    uint16_t ordinal;
    uint16_t unknown;
    uint32_t pointer_placeholder;
    uint16_t variant_count;
    DatSoundVariant *variants;
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
    int16_t offset_x;
    int16_t offset_y;
    int16_t display_angle;
    int16_t unknown;
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
    uint8_t layer;
    int16_t player_color;
    int16_t coordinates[4];
    int16_t sound_id;
    uint16_t frame_count;
    uint16_t angle_count;
    float speed_multiplier;
    float frame_duration;
    float replay_delay;
    uint8_t sequence_type;
    uint8_t mirror_mode;
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

typedef struct {
    uint8_t enabled;
    uint8_t random;
    char name[14];
    char filename[14];
    int32_t slp_id;
    uint32_t shape_pointer_placeholder;
    int32_t sound_id;
    uint8_t map_colors[5];
    int8_t passable_terrain;
    int8_t impassable_terrain;
    uint8_t animated;
    int16_t animation_frame_count;
    int16_t pause_frame_count;
    float animation_interval;
    float animation_pause;
    DatTerrainFrame elevation_frames[19];
    int16_t replacement_id;
    int16_t frame_grid[2]; /* Rows, columns; renderer uses their low bytes. */
    int16_t borders[32];
    int16_t terrain_unit_ids[30];
    int16_t terrain_unit_density[30];
    uint8_t terrain_placement_flags[30];
    uint16_t terrain_units_used_count;
} DatTerrain;

typedef struct {
    uint32_t map_header[6];
    DatTileSize tile_sizes[19];
    uint16_t count; /* Fixed-size terrain slot table in this release. */
    uint16_t enabled_count;
    DatTerrain *terrains;
    size_t next_offset;
} DatTerrains;

typedef struct {
    uint8_t enabled;
    uint8_t random;
    char name[14];
    char filename[14];
    int32_t slp_id;
    uint32_t shape_pointer_placeholder;
    int32_t sound_id;
    uint8_t map_colors[3];
    uint8_t animated;
    int16_t animation_frame_count;
    int16_t pause_frame_count;
    float animation_interval;
    float animation_pause;
    int16_t frame;
    int16_t draw_frame;
    float animation_last;
    uint8_t frame_changed;
    uint8_t drawn;
    DatTerrainFrame frames[19][12];
    int16_t draw_terrain;
    int16_t underlay_terrain;
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

typedef struct {
    int32_t map_id;
    uint32_t land_count;
    uint32_t terrain_count;
    uint32_t unit_count;
    uint32_t elevation_count;
} DatRandomMap;

typedef struct {
    uint32_t count;
    DatRandomMap *maps;
    size_t next_offset;
} DatRandomMaps;

typedef struct {
    int8_t type;
    int16_t target_unit;
    int16_t unit_class;
    int16_t attribute;
    float amount;
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

typedef struct {
    int16_t task_type, id, action_type;
    int16_t target_class, target_unit, target_terrain;
    int16_t resource_in, resource_multiplier, resource_out;
    float work_value1, work_value2, work_range, search_wait;
    int16_t moving_graphic, proceeding_graphic, working_graphic;
    int16_t carrying_graphic, gathering_sound, deposit_sound;
    uint8_t is_default, auto_search, enable_targeting;
    /* Runtime task +0x10/+0x11/+0x12 (file bytes 44..46), read by patched
       RoR 1.0a 0x004e7f90/0x004e7dc0: owner-relationship mode 1..5, "target
       must carry a resource", and "applies to a state-0 target". */
    uint8_t owner_mode, requires_carried_resource, unbuilt_target;
} DatUnitTask;

typedef struct {
    uint8_t present;
    uint8_t type;
    int16_t id;
    int16_t unit_class;
    int16_t standing_graphic;
    int16_t walking_graphic;
    int16_t dying_graphic;
    int16_t undead_graphic; /* original template +0x20 */
    uint8_t undead_mode;    /* original template +0x24 */
    int16_t hit_points;
    uint8_t hide_in_editor; /* serialized common-unit header byte */
    uint8_t enabled;        /* serialized common-unit header byte */
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
    /* Original template +0x8c/+0x8f bytes, read by the RoR task selectors
       0x004e7dc0/0x004e7f90/0x00508010. Their meaning beyond those uses is
       not yet established. */
    uint8_t field_8c, field_8f;
    float decay_rate;       /* original template +0x80 */
    int16_t selection_sound;
    int16_t dying_sound;    /* resolves to original template +0x44 */
    float speed;
    char *name;
    DatUnitStorage storage[3];
    int16_t default_task_id;
    uint8_t task_swap_group; /* original template +0xec (type >= 40) */
    float search_radius, work_rate;
    uint16_t task_count;
    DatUnitTask *tasks;
    uint8_t base_armor;
    uint16_t attack_count, armor_count;
    DatUnitAttackArmor *attacks, *armors;
    int16_t terrain_defense_bonus, projectile_unit_id, accuracy_percent;
    int16_t frame_delay, attack_graphic;
    float max_range, blast_width, reload_time, min_range;
    int16_t displayed_melee_armor, displayed_attack;
    float displayed_range, displayed_reload_time;
    DatUnitCost costs[3];
    int16_t train_time, train_location_id, displayed_pierce_armor;
    int8_t button_id;
    int16_t construction_graphic; /* type 80, original template +0x168 */
    uint8_t building_adjacent_mode; /* type 80, original template +0x16c */
    uint16_t building_angle;      /* type 80, original template +0x16e */
    uint8_t disappears_when_built; /* type 80, original template +0x170 */
    int16_t stack_unit_id;        /* type 80, original template +0x172 */
    int16_t foundation_terrain;   /* type 80, original template +0x174 */
    int16_t old_terrain;          /* type 80, original template +0x176 */
    int16_t building_research_id; /* type 80, original template +0x178 */
    int16_t completion_sound;     /* type 80, original template +0x17a */
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

typedef struct {
    int16_t required_techs[4];
    DatTechCost costs[3];
    int16_t required_count, research_location, research_time;
    int16_t effect_id, type, icon_id;
    uint16_t language_name, language_description;
    int8_t button_id;
    int32_t language_help, language_tree, hotkey;
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
