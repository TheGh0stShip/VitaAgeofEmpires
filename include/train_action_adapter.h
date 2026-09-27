#ifndef VITA_AOE_TRAIN_ACTION_ADAPTER_H
#define VITA_AOE_TRAIN_ACTION_ADAPTER_H
#include "building_update.h"
#include "train_action.h"

/* Binds the translated training action to generic AoeActionList callbacks.
   `prepare` resolves real building/player/template/world bindings for a
   queued unit ID; it must fill a valid semantic create record. */
typedef struct {
    void *context;
    int (*prepare)(void *context, AoeBuildingUpdate *building, int16_t unit_id,
                   AoeTrainActionCreate *out);
    const AoeTrainActivationOps *activation;
    const AoeTrainActionOps *update;
    const AoeTrainWaitingOps *waiting;
    const AoeTrainTargetOps *targets;
    /* Default zero is RoR to preserve the primary-target configuration.
       Base selects its no-prepaid constructor/activation/update path. */
    enum { AOE_TRAIN_ADAPTER_ROR = 0, AOE_TRAIN_ADAPTER_BASE = 1 } edition;
    float delta;
    int fast_train;
    int failed;
} AoeTrainActionAdapter;

/* Building production + action-list compatible callbacks. The bridge records
   portable dependency failures; callers check `ok` after a list operation. */
void *aoe_train_action_adapter_create(void *context, AoeBuildingUpdate *building,
                                      int16_t unit_id);
void aoe_train_action_adapter_destroy(void *context, void *action);
void aoe_train_action_adapter_state(void *context, void *action, uint8_t state);
uint8_t aoe_train_action_adapter_head(void *context, void *action);
void aoe_train_action_adapter_waiting(void *context, void *action);
void aoe_train_action_adapter_set_timing(AoeTrainActionAdapter *adapter,
                                         float delta, int fast_train);
int aoe_train_action_adapter_ok(const AoeTrainActionAdapter *adapter);
void aoe_train_action_adapter_list_ops(AoeTrainActionAdapter *adapter,
                                       AoeActionListOps *out);
void aoe_train_action_adapter_tick_ops(AoeTrainActionAdapter *adapter,
                                       AoeActionTickOps *out);
#endif
