#ifndef VITA_AOE_ACTION_LIST_H
#define VITA_AOE_ACTION_LIST_H
#include <stdint.h>

typedef struct AoeActionNode {
    void *action;
    struct AoeActionNode *next;
} AoeActionNode;

typedef struct {
    void *context;
    /* Original scalar deleting destructor, vtable +0, argument 1. */
    void (*destroy)(void *context, void *action);
    /* Original action vtable +0x1c: 1 on insertion, 0 when exposed by pop. */
    void (*set_state)(void *context, void *action, uint8_t state);
} AoeActionListOps;

typedef struct {
    const uint8_t *owner_state; /* list +4 -> unit +0x48; read after callbacks */
    AoeActionNode *head;        /* original list +8 */
    AoeActionListOps ops;
} AoeActionList;

int aoe_action_list_valid(const AoeActionList *list);
/* RoR 00404410 / 00404460. In terminal owner states, destroy the offered
   action without allocating. Otherwise NULL is invalid, and success transfers
   action ownership to the list. Allocation failure leaves ownership with caller.
   Initialize an empty list with head=NULL, owner_state and both callbacks.
   Node storage is owned by these functions; do not link stack nodes. */
int aoe_action_list_push(AoeActionList *list, void *action);
int aoe_action_list_append(AoeActionList *list, void *action);
/* RoR 004044e0 / 00403e70. Unlink before destruction; then read the live
   head and owner state and activate a surviving head for owner state <3.
   Clear repeats pop, including its intermediate activations. */
int aoe_action_list_pop(AoeActionList *list);
int aoe_action_list_clear(AoeActionList *list);
typedef struct {
    void *context;
    uint8_t (*update_head)(void *, void *action); /* action virtual +0x28 */
    void (*update_waiting)(void *, void *action); /* action virtual +0x24 */
} AoeActionTickOps;
/* RoR 004043c0, vtable +0xc in both root and building-production lists.
   Empty returns original result 1. Re-read head after its update, update
   its successors, then pop the live head if the saved result is 1.
   Waiting callbacks must not destroy their current list node (same lifetime
   requirement as the original). Return 0 for invalid API dependencies. */
int aoe_action_list_update_ror(AoeActionList *list, const AoeActionTickOps *ops,
                                uint8_t *result);
#endif
