#ifndef VITA_AOE_PLAYER_OUTCOME_DISPATCH_H
#define VITA_AOE_PLAYER_OUTCOME_DISPATCH_H

#include "player_outcome.h"

/* Original manager vtable +0/+4: RoR 0x00510260/0x005102c0;
   base 0x00433960/0x004339c0. The handlers are indexed by kind byte +0.
   Event kinds 0–4 and condition kinds 0–10, 12 use translated original
   handlers; condition kind 11 and unknown kinds are no-ops as on PC. */
typedef struct {
    AoeOutcomeManager *manager; /* Provides owner for world/player scans. */
} AoeOutcomeDispatcher;

/* These signatures can be used directly as AoeOutcomeOps callbacks with
   AoeOutcomeOps.user pointing to an AoeOutcomeDispatcher. */
int aoe_outcome_dispatch_event(void *user, AoeOutcomeEvent *event);
int aoe_outcome_dispatch_condition(void *user, AoeOutcomeCondition *condition);

/* Event kinds 0–4: RoR 0x005113b0/0x00511410/0x005114d0/
   0x005115c0/0x005116a0; base 0x00434b30/0x00434b90/0x00434c60/
   0x00434d40/0x00434e20. */
int aoe_outcome_event_kind0(AoeOutcomeManager *manager, AoeOutcomeEvent *event);
int aoe_outcome_event_kind1(AoeOutcomeManager *manager, AoeOutcomeEvent *event);
int aoe_outcome_event_kind2(AoeOutcomeManager *manager, AoeOutcomeEvent *event);
int aoe_outcome_event_kind3(AoeOutcomeManager *manager, AoeOutcomeEvent *event);
int aoe_outcome_event_kind4(AoeOutcomeManager *manager, AoeOutcomeEvent *event);

/* Shared RoR/base condition bodies: kind 0 at 0x00510380/0x00433a80,
   kind 1 at 0x00510440/0x00433b40,
   kind 8 at 0x00510550/0x00433c70,
   kind 2 at 0x005106e0/0x00433e00,
   kind 3 at 0x00510700/0x00433e30,
   kind 4 at 0x005108d0/0x00434000, kind 5 at 0x00510940/0x00434070,
   kind 6 at 0x00510a20/0x00434140,
   kind 7 at 0x00510ad0/0x00434200,
   kind 9 at 0x005107d0/0x00433f10, kind 10 at 0x005108a0/0x00433fd0,
   kind 12 at 0x00510bb0/0x004342f0. */
int aoe_outcome_condition_kind2(AoeOutcomeCondition *condition);
int aoe_outcome_condition_kind0(AoeOutcomeManager *manager,
                                AoeOutcomeCondition *condition);
int aoe_outcome_condition_kind1_or_8(AoeOutcomeManager *manager,
                                     AoeOutcomeCondition *condition,
                                     int within_region);
int aoe_outcome_condition_kind3(AoeOutcomeManager *manager,
                                AoeOutcomeCondition *condition);
int aoe_outcome_condition_kind4(AoeOutcomeCondition *condition);
int aoe_outcome_condition_kind5(AoeOutcomeCondition *condition);
int aoe_outcome_condition_kind6(AoeOutcomeManager *manager,
                                AoeOutcomeCondition *condition);
int aoe_outcome_condition_kind7(AoeOutcomeManager *manager,
                                AoeOutcomeCondition *condition);
int aoe_outcome_condition_kind9(AoeOutcomeManager *manager,
                                AoeOutcomeCondition *condition);
int aoe_outcome_condition_kind10(AoeOutcomeCondition *condition);
int aoe_outcome_condition_kind12(AoeOutcomeManager *manager,
                                 AoeOutcomeCondition *condition);

#endif
