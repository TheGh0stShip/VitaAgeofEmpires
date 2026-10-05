#ifndef VITA_AOE_SPRITE_LIST_H
#define VITA_AOE_SPRITE_LIST_H

#include <stddef.h>
#include <stdint.h>

#include "dat.h"

typedef struct AoeSpriteNode AoeSpriteNode;

typedef struct {
    void *context;
    void *(*create)(void *context, const void *graphic,
                    int32_t arg1, int32_t arg2);
    const void *(*graphic_of)(void *context, const void *sprite);
    int (*is_animated)(void *context, const void *graphic);
    void (*destroy)(void *context, void *sprite);
    void (*invalidate)(void *context);
    int (*tick)(void *context, void *sprite, float time_delta, float speed);
} AoeSpriteListOps;

/* Portable form of the original manager's list at +0x04 and redraw target
   at +0x08. Sprite objects themselves belong to the supplied factory. */
typedef struct {
    AoeSpriteNode *head;
    AoeSpriteListOps ops;
    int has_redraw_target;
} AoeSpriteList;

/* Base 0x0044fb80/0x0044fc30; RoR 0x00415c80/0x00415d20.
   A null graphic is a no-op. Equal priorities retain insertion order.
   add returns 0 only for invalid arguments or allocation/factory failure. */
int aoe_sprite_list_add(AoeSpriteList *list, const void *graphic,
                        uint8_t priority, int32_t arg1, int32_t arg2);
int aoe_sprite_list_remove(AoeSpriteList *list, const void *graphic);
void aoe_sprite_list_clear(AoeSpriteList *list);
/* RoR 0x00415d90 / base 0x0044fcb0: update every sprite in list order. */
int aoe_sprite_list_tick(AoeSpriteList *list, float time_delta, float speed);
/* Patched RoR 1.0a load 0x004153c0 builds the list by prepending each
   loaded sprite and then reverses it; this appends in stream order with
   priority 0 and no extra references, the same result. */
int aoe_sprite_list_append_loaded(AoeSpriteList *list, void *sprite);
/* Patched RoR 1.0a list methods (vtable 0x00543000): 0x00415370
   constructs it empty with its redraw target. 0x00415760 (+0x08) and
   0x004157e0 (+0x10) hand every sprite in order to its +0x0c update /
   +0x14 draw (`fn`). 0x00415860 (+0x04), 0x00415950 (+0x28) and
   0x00415980 ask the first sprite of the graphic its +0x1c / +0x30 /
   +0x34 (0 when none: aoe_sprite_list_first_ror gives NULL); 0x004158c0
   (+0x1c) hands every sprite of the graphic to its +0x24. */
void aoe_sprite_list_init_ror(AoeSpriteList *list, const AoeSpriteListOps *ops, void *redraw_target);
void aoe_sprite_list_each_ror(const AoeSpriteList *list, void (*fn)(void *context, void *sprite),
                              void *context);
void *aoe_sprite_list_first_ror(const AoeSpriteList *list, const void *graphic);
void aoe_sprite_list_each_of_ror(const AoeSpriteList *list, const void *graphic,
                                 void (*fn)(void *context, void *sprite), void *context);
size_t aoe_sprite_list_count(const AoeSpriteList *list);
const void *aoe_sprite_list_graphic_at(const AoeSpriteList *list, size_t index);
const void *aoe_sprite_list_sprite_at(const AoeSpriteList *list, size_t index);
uint8_t aoe_sprite_list_priority_at(const AoeSpriteList *list, size_t index);

/* 0x004a63e0 (asked by 0x00415640 and 0x00415c80):
   whether the graphic, or with deltas any delta's graphic, is on layer
   0 (+0x29). */
int aoe_graphic_on_layer_zero_ror(const DatGraphic *graphic);

#endif
