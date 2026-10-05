#ifndef VITA_AOE_PLAYER_OBJECT_H
#define VITA_AOE_PLAYER_OBJECT_H

#include <stdint.h>

#include "aoe_stream.h"
#include "player_selection.h"

/* Patched Rise of Rome 1.0a base player object (vtable 0x00544d18): the
   saved header fields, the view position and the player colour. Semantic
   storage with the original offsets noted; member objects that other
   modules model (unit lists, templates, condition and memory objects) are
   created and released through callbacks. */

/* A world colour record (world +0x64 table entry); only its ID byte +0x26
   is saved. */
typedef struct {
    uint8_t id; /* +0x26 */
} AoePlayerColorRor;

typedef struct {
    int16_t player_count;                    /* world +0x3c */
    const AoePlayerColorRor *const *colors;  /* world +0x64 */
    int32_t color_count;                     /* host bound of `colors` */
    int32_t map_width, map_height;           /* world +0x28 map +0x08/+0x0c */
    float save_version;                      /* global save version */
    int32_t default_08;                      /* global copied to +0x08 */
    uint8_t game_ad9;                        /* global game +0xad9 */
} AoePlayerWorldRor;

/* The initial player data the constructor copies (its third argument). */
typedef struct {
    int16_t template_count;      /* +0x18 */
    void *const *templates;      /* +0x1c, cloned through vtable +0x1c */
    int16_t resource_count;      /* +0x20 */
    const float *resources;      /* +0x24 */
    uint8_t field_28;            /* +0x28 */
    int16_t field_2a;            /* +0x2a */
} AoePlayerSourceRor;

typedef struct {
    int32_t field_04;            /* +0x04 */
    int32_t field_08;            /* +0x08 */
    int32_t field_0c;            /* +0x0c */
    int32_t field_14;            /* +0x14 */
    uint8_t field_20;            /* +0x20 */
    char *name;                  /* +0x44, NUL-terminated */
    uint8_t type;                /* +0x48 */
    int16_t player_id;           /* +0x4a */
    int16_t template_count;      /* +0x22 */
    void **templates;            /* +0x24 */
    int16_t resource_count;      /* +0x4c */
    float *resources;            /* +0x50 */
    uint8_t field_54;            /* +0x54 */
    uint8_t state;               /* +0x80 */
    uint8_t resigned;            /* +0x81 */
    uint8_t *stances;            /* +0x84, player_count bytes */
    int16_t stance_count;        /* host bound of `stances` */
    uint8_t stances_changed;     /* +0x88 */
    int32_t relations[9];        /* +0x8c */
    int32_t field_d4[3];         /* +0xd4..+0xdc */
    int32_t field_104;           /* +0x104 */
    uint8_t allied_victory;      /* +0x108 */
    uint8_t field_109;           /* +0x109 */
    const AoePlayerColorRor *color; /* +0x10c */
    int32_t field_110;           /* +0x110 */
    float view_x, view_y;        /* +0x118, +0x11c */
    int16_t view_tile_x, view_tile_y; /* +0x120, +0x122 */
    AoePlayerSelectionRor selection;  /* +0x124..+0x220; its `units`
                                         is the +0x28 unit list */
    void *lists[3];              /* +0x28, +0x2c, +0x30 unit lists */
    void *conditions;            /* +0x34 condition object */
    void *view;                  /* +0x38 view object */
    void *table;                 /* +0x40 table object */
    void *memory;                /* +0x114 object */
    const AoePlayerWorldRor *world;   /* +0x3c */
} AoePlayerObjectRor;

typedef struct {
    void *context;
    /* Constructor members: the two 400-entry rings at +0x58/+0x6c and the
       +0x40 table object (100 entries). Returns 0 on failure. */
    int (*create_members)(void *context, AoePlayerObjectRor *player);
    /* The +0x28/+0x2c/+0x30 unit lists: create (full constructor) and
       destroy (first step of the destructor). */
    void (*create_lists)(void *context, AoePlayerObjectRor *player);
    void (*destroy_lists)(void *context, AoePlayerObjectRor *player);
    /* Destructor members after the view object: the +0x40 table, the
       +0x114 object and the +0x6c and +0x58 rings. */
    void (*destroy_members)(void *context, AoePlayerObjectRor *player);
    /* A template's copy for this player (template vtable +0x1c) and a
       template's deleting destructor. */
    void *(*clone_template)(void *context, void *source);
    void (*delete_template)(void *context, void *template_object);
    /* A new empty condition object for the player (NULL when allocation
       fails) and a condition object's destruction and release. */
    void *(*conditions_new)(void *context, AoePlayerObjectRor *player);
    void (*conditions_release)(void *context, void *conditions);
    /* World +0x4c object vtable +0x08(id, player). */
    void (*register_player)(void *context, int16_t id,
                            AoePlayerObjectRor *player);
    /* Unit vtable +0x68. */
    void (*unit_68)(void *context, AoeSelectableUnitRor *unit);
    /* The view object's refresh call with argument 1, its destruction and
       release, and a new view object for the world map and the player
       (NULL when allocation fails). */
    void (*view_refresh)(void *context, void *view);
    void (*view_release)(void *context, void *view);
    void *(*view_create)(void *context, AoePlayerObjectRor *player);
    /* A new condition object loaded from the stream for the player (NULL
       when allocation fails). */
    void *(*conditions_create)(void *context, AoePlayerObjectRor *player,
                               AoeOutcomeStream *in,
                               const int32_t *player_map, int32_t flag);
} AoePlayerObjectOps;

/* 0x0045d310: the view tile (+0x120/+0x122). */
void aoe_player_set_view_tile_ror(AoePlayerObjectRor *player, int16_t x,
                                  int16_t y);

/* 0x0045d330: the view position; a negative (or NaN) coordinate becomes
   0 and one beyond the map's last tile becomes width-1 / height-1. */
void aoe_player_set_view_ror(AoePlayerObjectRor *player, float x, float y);

/* 0x0045d400: the colour is world colour table entry `index & 0xff`.
   Port: an index outside the host table leaves no colour. */
void aoe_player_set_color_ror(AoePlayerObjectRor *player, uint32_t index);

/* 0x0045b6a0: the constructor. +0x04 = 0, +0x08 = the global default,
   +0x0c = 10, +0x109 = `field_109`, type +0x48 = 0, the selection reset,
   the members created, the name copied (when given), every stance 1 with
   relation codes 1 (itself), 0 (player 0) and 3, and -1 for codes beyond
   the player count; +0xd4..+0xdc, +0x104 and +0x108 cleared; the source
   templates cloned (NULL entries stay NULL); with `full`, the unit lists,
   an empty condition object (0x0045ea50) and the view object, otherwise
   all five NULL; the resources copied (civilisation 15 with game +0xad9 = 1
   then gets 0.25 in resource 46; port: only when it exists); +0x54 from
   the source; the view centred (map size / 2, through 0x0045d330 and
   0x0045d310); state, resigned and colour cleared; +0x110 = source +0x2a,
   registered with the world when `full` and it is not negative; +0x20 and
   +0x14 cleared. Returns 0 on allocation failure (released). */
int aoe_player_object_init_ror(AoePlayerObjectRor *player,
                               const AoePlayerWorldRor *world,
                               const AoePlayerSourceRor *source,
                               uint8_t player_id, const char *name,
                               uint8_t field_109, int full,
                               const AoePlayerObjectOps *ops);

/* 0x0045ea50 (base vtable +0xf4): +0x34 = a new empty condition object. */
void aoe_player_conditions_new_ror(AoePlayerObjectRor *player,
                                   const AoePlayerObjectOps *ops);

/* 0x0045b410: the load constructor. +0x04 = 0, +0x08 = the global default,
   +0x0c = 10, the selection is reset (0x0045db40), the members are
   created, then: stances (player_count bytes; the changed byte cleared),
   the nine relation codes, +0x104 (saves of version 4.01 and later, else
   0), +0x108, the name (int16 length including the NUL, absent when not
   positive), the resource count and floats, the view position and tile,
   +0x54, +0x109, the state, the resigned byte, the colour ID (through
   0x0045d400), +0x08 and +0x0c; finally +0x20 = 0 and +0x14 = 0. The type
   byte +0x48 is read by the caller. Returns 0 on a short stream or
   allocation failure (anything allocated is released). */
int aoe_player_object_load_ror(AoePlayerObjectRor *player,
                               const AoePlayerWorldRor *world,
                               uint8_t player_id, AoeOutcomeStream *in,
                               const AoePlayerObjectOps *ops);

/* 0x0045d0d0 (vtable +0x84): the type byte, then the load fields in load
   order (+0x104 always written; the colour ID is 0 without a colour).
   Port: a missing name is written as length 0 (the original takes the
   string length of a NULL name). Returns 0 when the stream fills up. */
int aoe_player_object_save_ror(const AoePlayerObjectRor *player,
                               AoeOutcomeStream *out);

/* 0x0045ba50: destroy the unit lists, release the condition object,
   the stances, the templates (each through its deleting destructor), the
   view object, the remaining members, the name and the resources.
   0x0045b680 (vtable +0x00) is the deleting form; the caller owns the
   object storage, so it is the same here. */
void aoe_player_object_destroy_ror(AoePlayerObjectRor *player,
                                   const AoePlayerObjectOps *ops);
void aoe_player_object_delete_ror(AoePlayerObjectRor *player,
                                  const AoePlayerObjectOps *ops);

/* 0x0045d3e0: every unit of the +0x28 list receives vtable +0x68. */
void aoe_player_units_68_ror(AoePlayerObjectRor *player,
                             const AoePlayerObjectOps *ops);

/* 0x0045e940: refresh the +0x38 view object. Port: a missing view
   object is skipped (the original calls through it unchecked). */
void aoe_player_view_refresh_ror(AoePlayerObjectRor *player,
                                 const AoePlayerObjectOps *ops);

/* 0x0045e950: release the +0x38 view object (when present) and create a
   new one. */
void aoe_player_view_reset_ror(AoePlayerObjectRor *player,
                               const AoePlayerObjectOps *ops);

/* 0x0045e9e0 (base vtable +0xf0, called as +0xf0(stream, player map,
   flag)): +0x34 = a new condition object loaded from the stream. */
void aoe_player_conditions_create_ror(AoePlayerObjectRor *player,
                                      AoeOutcomeStream *in,
                                      const int32_t *player_map,
                                      int32_t flag,
                                      const AoePlayerObjectOps *ops);

/* Base player vtable defaults that ignore their arguments (the derived
   player vtables override each):
   0x0045bd40 (+0x14, one argument) returns 0;
   0x0045bcd0 (+0x38, one argument) does nothing;
   0x0045bce0 (+0x3c and +0x58, one argument) returns -1;
   0x0045bcf0 (+0x5c, three arguments) does nothing;
   0x0045be50 (+0x2c, five arguments) does nothing. */
int32_t aoe_player_default_14_ror(AoePlayerObjectRor *player, int32_t a);
void aoe_player_default_38_ror(AoePlayerObjectRor *player, int32_t a);
int32_t aoe_player_default_3c_ror(AoePlayerObjectRor *player, int32_t a);
void aoe_player_default_5c_ror(AoePlayerObjectRor *player, int32_t a,
                               int32_t b, int32_t c);
void aoe_player_default_2c_ror(AoePlayerObjectRor *player, int32_t a,
                               int32_t b, int32_t c, int32_t d, int32_t e);

#endif
