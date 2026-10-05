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
/* Patched RoR 1.0a 00404400 / 00404450 (disc 00404410 / 00404460). In terminal owner states, destroy the offered
   action without allocating. Otherwise NULL is invalid, and success transfers
   action ownership to the list. Allocation failure leaves ownership with caller.
   Initialize an empty list with head=NULL, owner_state and both callbacks.
   Node storage is owned by these functions; do not link stack nodes. */
int aoe_action_list_push(AoeActionList *list, void *action);
int aoe_action_list_append(AoeActionList *list, void *action);
/* Patched RoR 1.0a 004044d0 / 00403e50 (disc 004044e0 / 00403e70; patched
   00403e70 is an action factory). Unlink before destruction; then read the live
   head and owner state and activate a surviving head for owner state <3.
   Clear repeats pop, including its intermediate activations. */
int aoe_action_list_pop(AoeActionList *list);
int aoe_action_list_clear(AoeActionList *list);
typedef struct {
    void *context;
    uint8_t (*update_head)(void *, void *action); /* action virtual +0x28 */
    void (*update_waiting)(void *, void *action); /* action virtual +0x24 */
} AoeActionTickOps;
/* Patched RoR 1.0a 004043b0 (disc 004043c0), vtable +0xc in both root and building-production lists.
   Empty returns original result 1. Re-read head after its update, update
   its successors, then pop the live head if the saved result is 1.
   Waiting callbacks must not destroy their current list node (same lifetime
   requirement as the original). Return 0 for invalid API dependencies. */
int aoe_action_list_update_ror(AoeActionList *list, const AoeActionTickOps *ops,
                                uint8_t *result);
/* 0x00404360 (list vtable +0x08, the update while attached) is the same
   walk with the head's +0x20 in place of +0x28: pass that slot as
   update_head. 0x004045b0: the head action (NULL when empty); 0x00404580
   asks it +0x34 (0 when empty). 0x00403df0 (+0x10) and 0x00404600
   (+0x44 with three values) hand every action, in order, to `fn`. */
/* The action unit's slots on its list (+0x184; vtable 0x00542748):
   +0x204 0x00406470 pushes and +0x20c 0x004064c0 appends an action,
   ignoring NULL (1); +0x08 0x00405b40 clears the list (when it has one)
   before the common cleanup 0x004a6920 (`cleanup`). */
int aoe_unit_push_action_ror(AoeActionList *list, void *action);
int aoe_unit_append_action_ror(AoeActionList *list, void *action);
void aoe_unit_cleanup_actions_ror(AoeActionList *list, void (*cleanup)(void *context),
                                  void *context);
/* A new empty list (0x00403d50) for a unit: the action unit's +0x21c
   0x00405c60 makes one for itself, an action's +0x0c 0x004070a0 one for
   its unit (+0x08). NULL when out of memory; free with free() once
   cleared. */
AoeActionList *aoe_action_list_new_ror(const uint8_t *owner_state, const AoeActionListOps *ops);
void *aoe_action_list_head_ror(const AoeActionList *list);
void aoe_action_list_each_ror(const AoeActionList *list, void (*fn)(void *context, void *action),
                              void *context);
#endif
