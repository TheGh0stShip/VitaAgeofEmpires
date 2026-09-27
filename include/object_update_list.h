#ifndef VITA_AOE_OBJECT_UPDATE_LIST_H
#define VITA_AOE_OBJECT_UPDATE_LIST_H

#include <stdint.h>

/* Original base 0x0049ce80 / Rise of Rome 0x004521e0. The original nodes
   contain an object pointer and next pointer at offsets +0 and +4. This
   semantic representation is portable and does not claim the 32-bit ABI. */
typedef struct AoeObjectUpdateNode {
    void *object;
    struct AoeObjectUpdateNode *next;
    struct AoeObjectUpdateNode *prev;
} AoeObjectUpdateNode;

typedef struct {
    AoeObjectUpdateNode *head;
    uint16_t count;
} AoeObjectUpdateList;

/* RoR 0x00451f00/0x00451f40 and base 0x0049cbf0/0x0049cc30.
   A list owns nodes it inserts; detach removes the supplied node, or the
   first matching object when node is null. Caller must not pass a foreign
   node. Object ownership stays with the caller. */
int aoe_object_update_list_attach(AoeObjectUpdateList *list, void *object,
                                  AoeObjectUpdateNode **attached);
int aoe_object_update_list_detach(AoeObjectUpdateList *list, void *object,
                                  AoeObjectUpdateNode *node);
void aoe_object_update_list_clear(AoeObjectUpdateList *list);

typedef struct {
    void *user;
    /* Original object vtable +0x24; nonzero triggers +0x08. */
    int (*predicate)(void *user, void *object);
    void (*action)(void *user, void *object);
    /* Original global flag is re-read after each tenth non-null object. */
    int (*cooperative_pending)(void *user);
    /* Original cooperative callback: RoR 0x0044b490 / base 0x004692d0. */
    void (*cooperative_pump)(void *user);
} AoeObjectUpdateOps;

/* Traverses from the original list's head (+4). The next link is captured
   before the object methods, so they may unlink the current node. The pump
   runs after every tenth non-null object if the live flag is set then.
   Returns zero without traversing when required callbacks are absent. */
int aoe_object_update_list(AoeObjectUpdateNode *head,
                           const AoeObjectUpdateOps *ops);

#endif
