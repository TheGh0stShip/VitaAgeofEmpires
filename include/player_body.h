#ifndef VITA_AOE_PLAYER_BODY_H
#define VITA_AOE_PLAYER_BODY_H

#include <stdint.h>

#include "aoe_stream.h"
#include "player_object.h"

/* Patched Rise of Rome 1.0a base player members that live in other
   modules' objects: the template table (+0x22/+0x24), the unit lists
   (+0x28/+0x2c/+0x30), the view (+0x38), the table (+0x40) and the
   +0x114 object. The member objects are opaque; their own routines are
   callbacks. */

typedef struct {
    void *context;
    /* A template of DAT type 10, 20, 25, 30, 40, 50 or 60 loaded from the
       stream with the world's two template tables (world +0x38/+0x30);
       NULL when allocation fails. */
    void *(*load_template)(void *context, uint8_t type,
                           AoeOutcomeStream *in);
    void (*save_template)(void *context, void *template_object,
                          AoeOutcomeStream *out);      /* vtable +0x14 */
    void *(*view_load)(void *context, AoePlayerObjectRor *player,
                       AoeOutcomeStream *in);
    void (*view_save)(void *context, void *view, AoeOutcomeStream *out);
    void *(*memory_load)(void *context, AoePlayerObjectRor *player,
                         AoeOutcomeStream *in);
    void (*memory_save)(void *context, void *memory, AoeOutcomeStream *out);
    void *(*list_new)(void *context);
    void (*list_load)(void *context, void *list, AoeOutcomeStream *in);
    void (*list_save)(void *context, void *list, AoeOutcomeStream *out);
    /* The lists' after-load pass. */
    void (*list_finish)(void *context, void *list);
    /* Per-update work: a global object's step (when it exists), the +0x30
       list's step and the table's step. */
    void (*global_update)(void *context);
    void (*list_update)(void *context, void *list);
    void (*table_update)(void *context, void *table);
    /* Unit list insertion (returns the node) and removal, and the world's
       vtable +0xf8(unit) / +0xfc(unit ID) notices. */
    void *(*list_add)(void *context, void *list, void *unit);
    void (*list_remove)(void *context, void *list, void *unit, void *node);
    void (*world_unit_added)(void *context, void *unit);
    void (*world_unit_removed)(void *context, int32_t unit_id);
    int32_t (*unit_id)(void *context, void *unit);           /* +0x04 */
    /* Template vtable +0x18(player, x, y, z): a new unit. */
    void *(*create_from_template)(void *context, void *template_object,
                                  AoePlayerObjectRor *player, float x,
                                  float y, float z);
    void (*unit_68)(void *context, void *unit);
    void (*unit_6c)(void *context, void *unit);
    void (*unit_b4)(void *context, void *unit, int32_t value);
    /* Player vtable +0xf0(stream, player map, flag); NULL uses the base
       0x0045e9e0 through `object`. */
    void (*load_conditions)(void *context, AoePlayerObjectRor *player,
                            AoeOutcomeStream *in, const int32_t *player_map,
                            int32_t flag);
    const AoePlayerObjectOps *object;
} AoePlayerBodyOps;

/* 0x0045cb70: load template `slot` of type `type` (10, 20, 25, 30, 40,
   50, 60). Port: other types store NULL; the original leaves the slot
   holding the raw presence word it read from the stream. */
void aoe_player_template_load_ror(AoePlayerObjectRor *player,
                                  AoeOutcomeStream *in, int16_t slot,
                                  uint8_t type, const AoePlayerBodyOps *ops);

/* 0x0045c990: the template count (int16) and a 32-bit presence word per
   slot, then for each present slot its type byte and template; the view
   object, the +0x114 object, the +0x28 list (loaded), the +0x2c and +0x30
   lists, the +0x2c list loaded, and the +0x30 list loaded only for saves
   of version 7.09 and later. Returns 0 for a short stream or allocation
   failure. */
int aoe_player_body_load_ror(AoePlayerObjectRor *player, AoeOutcomeStream *in,
                             const AoePlayerBodyOps *ops);

/* 0x0045d250 (vtable +0x88): the template count, then (for a positive
   count) the presence words (1 or 0) and each present template's own
   save, then the view, the +0x114 object and the three lists. */
int aoe_player_body_save_ror(const AoePlayerObjectRor *player,
                             AoeOutcomeStream *out,
                             const AoePlayerBodyOps *ops);

/* 0x0045ce30: load the condition object (+0xf0(stream, NULL, 1)) and run
   each list's after-load pass. */
void aoe_player_body_finish_load_ror(AoePlayerObjectRor *player,
                                     AoeOutcomeStream *in,
                                     const AoePlayerBodyOps *ops);

/* 0x0045d0a0 (vtable +0x80): the per-update steps. */
void aoe_player_body_update_ror(AoePlayerObjectRor *player,
                                const AoePlayerBodyOps *ops);

/* 0x0045c2d0 (vtable +0x60): create a unit from template `index` at
   x/y/z; a state of 7 or more then runs unit +0x68, 3..6 unit +0x6c, and
   0..2 unit +0xb4(value). Returns NULL without that template. Ports: an
   index outside the table and a failed creation return NULL. */
void *aoe_player_create_unit_ror(AoePlayerObjectRor *player, float x,
                                 float y, float z, int16_t index,
                                 uint8_t state, int32_t value,
                                 const AoePlayerBodyOps *ops);

/* 0x0045e880 (base vtable +0xdc) and 0x0045e8d0 (base +0xe0): add a unit
   to / remove it from the +0x2c list when `in_2c`, else the +0x30 list
   when `in_30`, else +0x28, then notify the world. Add returns the list
   node. */
void *aoe_player_add_unit_ror(AoePlayerObjectRor *player, void *unit,
                              int in_2c, int in_30,
                              const AoePlayerBodyOps *ops);
void aoe_player_remove_unit_ror(AoePlayerObjectRor *player, void *unit,
                                int in_2c, int in_30, void *node,
                                const AoePlayerBodyOps *ops);

#endif
