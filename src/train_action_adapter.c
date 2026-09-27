#include "train_action_adapter.h"

void *aoe_train_action_adapter_create(void *p, AoeBuildingUpdate *building,
                                      int16_t unit_id) {
    AoeTrainActionAdapter *a=p;
    AoeTrainActionCreate create;
    if (!a || !a->prepare || !building) return NULL;
    if (!a->prepare(a->context,building,unit_id,&create)) { a->failed=1;return NULL; }
    /* RoR 004af900 calls 004b6820(building, unit ID, -1, 1); base
       004a50c0 has no fourth prepaid field. */
    create.unit_id=unit_id;
    create.request_id=-1;
    create.prepaid=a->edition==AOE_TRAIN_ADAPTER_BASE?0:1;
    AoeTrainAction *result=a->edition==AOE_TRAIN_ADAPTER_BASE?
        aoe_train_action_create_base(&create):aoe_train_action_create_ror(&create);
    if (!result) a->failed=1;
    return result;
}
void aoe_train_action_adapter_destroy(void *p, void *action) {
    AoeTrainActionAdapter *a=p;
    if (!a || !aoe_train_action_destroy_ror(action,a->targets))
        if (a) a->failed=1;
}
void aoe_train_action_adapter_state(void *p, void *action, uint8_t state) {
    AoeTrainActionAdapter *a=p;
    if (!a || !(a->edition==AOE_TRAIN_ADAPTER_BASE?
                aoe_train_action_activate_base(action,state,a->activation):
                aoe_train_action_activate_ror(action,state,a->activation)))
        if (a) a->failed=1;
}
uint8_t aoe_train_action_adapter_head(void *p, void *action) {
    AoeTrainActionAdapter *a=p;
    uint8_t result=0;
    if (!a || !(a->edition==AOE_TRAIN_ADAPTER_BASE?
                aoe_train_action_update_base(action,a->delta,a->fast_train,
                                             a->update,&result):
                aoe_train_action_update_ror(action,a->delta,a->fast_train,
                                            a->update,&result))) {
        if (a) a->failed=1;
        return 0;
    }
    return result;
}
void aoe_train_action_adapter_waiting(void *p, void *action) {
    AoeTrainActionAdapter *a=p;
    uint8_t result;
    if (!a || !aoe_train_action_waiting_ror(action,a->waiting,&result))
        if (a) a->failed=1;
}
void aoe_train_action_adapter_set_timing(AoeTrainActionAdapter *a,
                                         float delta, int fast_train) {
    if (a) { a->delta=delta;a->fast_train=fast_train; }
}
int aoe_train_action_adapter_ok(const AoeTrainActionAdapter *a) {
    return a && !a->failed;
}
void aoe_train_action_adapter_list_ops(AoeTrainActionAdapter *a,
                                       AoeActionListOps *out) {
    if (out) *out=(AoeActionListOps){a,aoe_train_action_adapter_destroy,
                                     aoe_train_action_adapter_state};
}
void aoe_train_action_adapter_tick_ops(AoeTrainActionAdapter *a,
                                       AoeActionTickOps *out) {
    if (out) *out=(AoeActionTickOps){a,aoe_train_action_adapter_head,
                                     aoe_train_action_adapter_waiting};
}
