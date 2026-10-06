#ifndef VITA_AOE_OBJECT_LIST_H
#define VITA_AOE_OBJECT_LIST_H

#include <stdint.h>

#include "player_outcome.h" /* AoeOutcomeStream */

/* Patched Rise of Rome 1.0a object list (vtable 0x00544ad4,
   0x00450ef0..0x004516c0): the world's and the players' unit lists. A
   doubly linked list of 0x10-byte nodes (object +0x00, next +0x04,
   previous +0x08) headed at +0x04 with a 16-bit count at +0x08; new
   objects go to the front. Objects are opaque; their virtual calls are
   ops. Semantic storage. */
typedef struct AoeObjectNodeRor {
    void *object;
    struct AoeObjectNodeRor *next, *prev;
} AoeObjectNodeRor;

typedef struct {
    AoeObjectNodeRor *head;   /* +0x04 */
    int16_t count;            /* +0x08 */
} AoeObjectListRor;

typedef struct {
    void *context;
    /* Object +0x00(1): its deleting destructor. The original objects
       take themselves off their list (0x00450fe0) as they go. */
    void (*destroy)(void *context, void *object);
    int32_t (*id)(void *context, const void *object);            /* +0x04 */
    uint8_t (*class_flag)(void *context, const void *object);    /* template +0x65 */
    uint8_t (*sort_key)(void *context, const void *object);      /* template +0x4a */
    int16_t (*sort_order)(void *context, const void *object);    /* +0x2a */
    int (*needs_update)(void *context, void *object);            /* +0x24 */
    void (*update)(void *context, void *object);                 /* +0x08 */
    /* 0x0044a850 every ten updates while 0x00582edc is set. */
    void (*pump)(void *context);
    int pump_enabled;
    void (*save)(void *context, void *object, AoeOutcomeStream *s); /* +0x30 */
    /* The list's +0x00 factory: a new object of `type` read from the
       stream and added to the list (through append). */
    void (*load)(void *context, AoeObjectListRor *list, uint8_t type,
                 AoeOutcomeStream *s, int32_t arg);
} AoeObjectListOpsRor;

/* 0x00450ef0: empty. 0x00450f10 the destructor: every object destroyed,
   then the nodes still listed freed. 0x00450f60: every object destroyed
   (nodes left to the objects), then empty. */
void aoe_object_list_init_ror(AoeObjectListRor *l);
void aoe_object_list_destroy_ror(AoeObjectListRor *l, const AoeObjectListOpsRor *ops);
void aoe_object_list_clear_ror(AoeObjectListRor *l, const AoeObjectListOpsRor *ops);

/* 0x00450fa0: a new front node (0 when it cannot be allocated, a
   host-only outcome; the original does not check). 0x00450fe0(object,
   node): the node (found by object when NULL) unlinked and freed; absent
   objects are ignored. 0x00451040 reverses the list. */
int aoe_object_list_append_ror(AoeObjectListRor *l, void *object);
void aoe_object_list_remove_ror(AoeObjectListRor *l, void *object, AoeObjectNodeRor *node);
void aoe_object_list_reverse_ror(AoeObjectListRor *l);

/* 0x00451340: the first object with this ID (+0x04), else NULL. */
void *aoe_object_list_find_ror(const AoeObjectListRor *l, const AoeObjectListOpsRor *ops,
                               int32_t id);

/* Each object (non-NULL) handed to `fn`: 0x00451160 (+0x10 with four
   arguments; with `flagged` only objects whose class flag is nonzero),
   0x004516c0 (+0x2c). */
void aoe_object_list_each_ror(const AoeObjectListRor *l, const AoeObjectListOpsRor *ops,
                              int flagged, void (*fn)(void *context, void *object),
                              void *context);

/* 0x004512c0: a new list (NULL when empty) of the same objects ordered
   by sort key descending, then sort order ascending, otherwise in list
   order; 0x004511e0 hands it (filtered like 0x00451160) to +0x14 and
   frees it. Returns 0 if a node cannot be allocated (host-only). */
int aoe_object_list_sorted_ror(const AoeObjectListRor *l, const AoeObjectListOpsRor *ops,
                               AoeObjectNodeRor **out);
void aoe_object_list_free_nodes_ror(AoeObjectNodeRor *head);
void aoe_object_list_each_sorted_ror(const AoeObjectListRor *l, const AoeObjectListOpsRor *ops,
                                     int flagged, void (*fn)(void *context, void *object),
                                     void *context);

/* 0x00451270: every object that needs it (+0x24) updates (+0x08),
   pumping messages after every ten objects. The next node is taken
   before the object's calls. */
void aoe_object_list_update_ror(AoeObjectListRor *l, const AoeObjectListOpsRor *ops);

/* 0x00451640: each object's save, then a 0 byte. 0x00451680: type bytes
   until 0, each loaded by the factory; then the list reversed (so saved
   order is kept). */
void aoe_object_list_save_ror(const AoeObjectListRor *l, const AoeObjectListOpsRor *ops,
                              AoeOutcomeStream *s);
void aoe_object_list_load_ror(AoeObjectListRor *l, const AoeObjectListOpsRor *ops,
                              AoeOutcomeStream *s, int32_t arg);

/* 0x00451080(a, b, c, fogged): each object's +0x0c draw(a, b, c, colour)
   with the owner's colour (+0x0c player +0x10c), by template class flag
   (+0x65) when `fogged`: 0 skipped; 2 skipped above state 2 (+0x48);
   3 skipped unless the viewed player has the owner flagged (its +0xb0
   table) and then drawn with the object's own colour +0x90; 4 skipped
   when the list holds more than one object and any has +0x4e 0x19. */
typedef struct {
    void *context;
    uint8_t (*class_flag)(void *context, const void *object);
    uint8_t (*state)(void *context, const void *object);          /* +0x48 */
    uint8_t (*kind)(void *context, const void *object);           /* +0x4e */
    int32_t (*owner_colour)(void *context, const void *object);
    int32_t (*own_colour)(void *context, const void *object);     /* +0x90 */
    int (*viewer_sees_owner)(void *context, const void *object);
    void (*draw)(void *context, void *object, int32_t a, int32_t b, int32_t c,
                 int32_t colour);
} AoeObjectListDrawOpsRor;
void aoe_object_list_draw_ror(const AoeObjectListRor *l, const AoeObjectListDrawOpsRor *ops,
                              int32_t a, int32_t b, int32_t c, int fogged);

/* 0x004a94d0 (a live unit's lookup of a unit by ID): a negative ID finds
   nothing; otherwise the owner's three lists (player +0x28, +0x2c, +0x30)
   are searched, then those of every other world player in index order
   (world +0x40[0..+0x3c), skipping the owner's index +0x4a). `lists` holds
   three lists per player, by player index. The first match wins. */
void *aoe_object_lists_find_any_ror(const AoeObjectListRor *const *lists,
                                    int16_t player_count, int16_t owner_index,
                                    const AoeObjectListOpsRor *ops,
                                    int32_t id);

#endif
