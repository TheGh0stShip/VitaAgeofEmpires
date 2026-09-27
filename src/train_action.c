#include "train_action.h"
#include <math.h>
#include <limits.h>
#include <stdlib.h>

int16_t aoe_train_spawn_direction_ror(uint16_t random, int late_attempt) {
    int32_t value=(int32_t)((uint32_t)random<<1);
    int64_t product=(int64_t)(int32_t)UINT32_C(0x80010003)*value;
    int32_t high=(int32_t)(product>>32);
    int32_t mixed=(int32_t)((uint32_t)high+(uint32_t)value);
    int32_t quotient=mixed>>14;
    quotient+=(int32_t)((uint32_t)quotient>>31);
    return (int16_t)(quotient+(late_attempt?2:0));
}

int16_t aoe_train_spawn_direction_base(uint16_t random, int late_attempt) {
    /* Base 004a54c3/004a54d4: signed EDX:EAX / 0x7fff after `add eax,eax`. */
    int32_t quotient=(int32_t)((uint32_t)random*2u)/INT32_C(0x7fff);
    return (int16_t)(quotient+(late_attempt?2:0));
}

static float spawn_sample(AoeRng *rng, float width, float half, float center) {
    float value=(float)(int32_t)aoe_rng_next(rng);
    value=(float)(value*width);
    value=(float)(value*0x1.0002p-15f); /* exact 0054daf0 = 0x38000100 */
    value=(float)(value-half);
    return (float)(value+center);
}

static int spawn(AoeTrainAction *a, AoeRng *rng, const AoeTrainSpawnOps *ops,
                 int32_t *created_id, uint8_t *found, int base) {
    float half_x,half_y,width_x,width_y;
    if (!a || !rng || !ops || !ops->blocked || !ops->create ||
        !created_id || !found || !a->building_template || !a->trained_template)
        return 0;
    half_x=(float)(a->building_template->size[0]+a->trained_template->size[0]);
    half_x=(float)(half_x-(-0.01f));
    width_x=(float)(half_x+half_x);
    half_y=(float)(a->building_template->size[1]+a->trained_template->size[1]);
    half_y=(float)(half_y-(-0.01f));
    width_y=(float)(half_y+half_y);
    for (int ring=0;ring<3;++ring) {
        for (int attempt=0;attempt<60;++attempt) {
            int16_t direction=base ?
                aoe_train_spawn_direction_base(aoe_rng_next(rng),attempt>=40) :
                aoe_train_spawn_direction_ror(aoe_rng_next(rng),attempt>=40);
            switch (direction) {
            case 0:
                a->candidate_y=(float)(a->building_y+half_y);
                a->candidate_x=spawn_sample(rng,width_x,half_x,a->building_x);
                break;
            case 1:
                a->candidate_x=(float)(a->building_x-half_x);
                a->candidate_y=spawn_sample(rng,width_y,half_y,a->building_y);
                break;
            case 2:
                a->candidate_x=(float)(a->building_x+half_x);
                a->candidate_y=spawn_sample(rng,width_y,half_y,a->building_y);
                break;
            case 3:
                a->candidate_y=(float)(a->building_y-half_y);
                a->candidate_x=spawn_sample(rng,width_x,half_x,a->building_x);
                break;
            default:
                /* Original switch has no case 4 and tests the prior point. */
                break;
            }
            if (!ops->blocked(ops->context,a,a->candidate_x,a->candidate_y)) {
                if (!ops->create(ops->context,a,a->candidate_x,a->candidate_y,
                                 created_id)) return 0;
                *found=1;
                return 1;
            }
        }
        half_x=(float)(half_x+2.0f*a->trained_template->size[0]);
        half_y=(float)(half_y+2.0f*a->trained_template->size[1]);
        width_x=(float)(width_x+4.0f*a->trained_template->size[0]);
        width_y=(float)(width_y+4.0f*a->trained_template->size[1]);
    }
    *found=0;
    return 1;
}

int aoe_train_action_spawn_ror(AoeTrainAction *a, AoeRng *rng,
                                const AoeTrainSpawnOps *ops,
                                int32_t *created_id, uint8_t *found) {
    return spawn(a,rng,ops,created_id,found,0);
}

int aoe_train_action_spawn_base(AoeTrainAction *a, AoeRng *rng,
                                const AoeTrainSpawnOps *ops,
                                int32_t *created_id, uint8_t *found) {
    return spawn(a,rng,ops,created_id,found,1);
}

AoeTrainAction *aoe_train_action_create_ror(const AoeTrainActionCreate *c) {
    AoeTrainAction *a;
    AoeActionList probe;
    if (!c) return NULL;
    probe=(AoeActionList){NULL,NULL,c->owned_action_ops};
    /* owner_state is installed after allocation; validate callbacks here. */
    probe.owner_state=(const uint8_t *)&probe;
    if (!aoe_action_list_valid(&probe)) return NULL;
    a=calloc(1,sizeof(*a));
    if (!a) return NULL;
    a->primary_target_id=-1;
    a->secondary_target_id=-1;
    a->action_type=102;
    a->unit_id=c->unit_id;
    a->request_id=c->request_id;
    a->prepaid=c->prepaid;
    a->building_template=c->building_template;
    a->trained_template=c->trained_template;
    a->owner_resources=c->owner_resources;
    a->owner_id=c->owner_id;
    a->building_id=c->building_id;
    a->building_x=c->building_x;
    a->building_y=c->building_y;
    a->owned_actions=calloc(1,sizeof(*a->owned_actions));
    a->unit_registry=c->unit_registry;
    if (a->owned_actions)
        *a->owned_actions=(AoeActionList){&a->state,NULL,c->owned_action_ops};
    /* The original stores a NULL internal-list allocation and still returns
       the action, so portable allocation failure follows that behavior. */
    return a;
}

AoeTrainAction *aoe_train_action_create_base(const AoeTrainActionCreate *c) {
    AoeTrainActionCreate base;
    if (!c) return NULL;
    base=*c;
    base.prepaid=0;
    return aoe_train_action_create_ror(&base);
}

int aoe_train_action_destroy_ror(AoeTrainAction *a,
                                  const AoeTrainTargetOps *targets) {
    if (!a) return 1;
    if (a->primary_target && !aoe_train_action_set_primary_ror(a,NULL,targets))
        return 0;
    if (a->secondary_target &&
        !aoe_train_action_set_secondary_ror(a,NULL,targets)) return 0;
    if (a->owned_actions) {
        if (!aoe_action_list_valid(a->owned_actions)) return 0;
        aoe_action_list_clear(a->owned_actions);
        free(a->owned_actions);
    }
    free(a);
    return 1;
}

static int set_target(AoeTrainAction *a, void *target,
                      const AoeTrainTargetOps *ops, int secondary) {
    void **slot;
    int32_t *id_slot;
    void *registered;
    if (!a) return 0;
    slot=secondary?&a->secondary_target:&a->primary_target;
    id_slot=secondary?&a->secondary_target_id:&a->primary_target_id;
    if (*slot==target) return 1;
    if (*slot) {
        if (!a->unit_registry || !ops || !ops->detach) return 0;
        registered=aoe_live_unit_registry_lookup(a->unit_registry,*id_slot);
        if (registered) ops->detach(ops->context,registered,a);
    }
    /* Original method rereads the pointer after the detach callback. */
    if (*slot==target) return 1;
    if (target && (!ops || !ops->unit_id || !ops->attach)) return 0;
    *slot=target;
    if (!target) { *id_slot=-1;return 1; }
    *id_slot=ops->unit_id(ops->context,target);
    ops->attach(ops->context,target,a,a->action_type);
    return 1;
}

int aoe_train_action_set_primary_ror(AoeTrainAction *a, void *target,
                                      const AoeTrainTargetOps *ops) {
    return set_target(a,target,ops,0);
}
int aoe_train_action_set_secondary_ror(AoeTrainAction *a, void *target,
                                        const AoeTrainTargetOps *ops) {
    return set_target(a,target,ops,1);
}

int aoe_train_action_waiting_ror(AoeTrainAction *a,
                                  const AoeTrainWaitingOps *ops,
                                  uint8_t *result) {
    uint8_t state;
    if (!a || !result) return 0;
    if (a->primary_target) {
        if (!ops || !ops->target_state) return 0;
        state=ops->target_state(ops->context,a->primary_target);
        if (state>=7) {
            if (!ops->targets ||
                !aoe_train_action_set_primary_ror(a,NULL,ops->targets)) return 0;
        }
    }
    if (a->secondary_target) {
        if (!ops || !ops->target_state) return 0;
        state=ops->target_state(ops->context,a->secondary_target);
        if (state>=7) {
            if (!ops->targets ||
                !aoe_train_action_set_secondary_ror(a,NULL,ops->targets)) return 0;
        }
    }
    *result=1;
    return 1;
}

int aoe_train_action_refund_ror(AoeTrainAction *a) {
    if (!a || !a->trained_template || !a->owner_resources) return 0;
    if (a->trained_template->type < 70) return 1;
    for (int i=0;i<3;++i) {
        const DatUnitCost *cost=&a->trained_template->costs[i];
        if (cost->resource_type >= 0 && (uint8_t)cost->paid != 0) {
            if (!a->owner_resources->values ||
                cost->resource_type >= a->owner_resources->count) return 0;
        }
    }
    for (int i=0;i<3;++i) {
        const DatUnitCost *cost=&a->trained_template->costs[i];
        if (cost->resource_type >= 0 && (uint8_t)cost->paid != 0)
            aoe_player_resource_add(a->owner_resources,cost->resource_type,
                                    cost->amount,AOE_PLAYER_RISE_OF_ROME);
    }
    return 1;
}

int aoe_train_action_cancel_ror(AoeTrainAction *a,
                                const AoeTrainCancelOps *ops) {
    if (!a || !ops || !ops->notify_cancel || !ops->signal_cancel) return 0;
    if (a->state != 1) {
        a->state = 1; /* 0054db00 +0x5c = inherited 004073c0. */
        if (!a->prepaid && !aoe_train_action_refund_ror(a)) return 0;
    }
    ops->notify_cancel(ops->context, a);
    ops->signal_cancel(ops->context, a);
    return 1;
}
int aoe_train_action_cancel_base(AoeTrainAction *a,
                                 const AoeTrainCancelOps *ops) {
    if (!a || !ops || !ops->notify_cancel || !ops->signal_cancel) return 0;
    if (a->state != 1) {
        a->state = 1;
        if (!aoe_train_action_refund_ror(a)) return 0;
    }
    ops->notify_cancel(ops->context,a);
    ops->signal_cancel(ops->context,a);
    return 1;
}

int aoe_train_action_progress_ror(const AoeTrainAction *a,
                                  int16_t *unit_id, int16_t *percent) {
    long double value;
    int64_t whole;
    if (!a || !unit_id || !percent) return 0;
    *unit_id=a->unit_id;
    if (a->state==1) { *percent=100; return 1; }
    if (!a->trained_template || !a->trained_template->train_time ||
        !isfinite(a->progress)) return 0;
    value=(long double)a->progress*100.0L/(long double)a->trained_template->train_time;
    if (!isfinite(value) || value<(long double)INT64_MIN || value>(long double)INT64_MAX)
        return 0;
    /* 005283e0 changes the x87 control word to round toward zero, then FISTP. */
    whole=(int64_t)value;
    *percent=(int16_t)(uint16_t)whole;
    return 1;
}
int aoe_train_action_progress_base(const AoeTrainAction *a,
                                   int16_t *unit_id, int16_t *percent) {
    return aoe_train_action_progress_ror(a,unit_id,percent);
}

int aoe_train_action_serialize_ror(const AoeTrainAction *a,
                                   const AoeTrainActionSerializeOps *ops) {
    if (!a || !ops || !ops->serialize_base || !ops->write) return 0;
    return ops->serialize_base(ops->context,a) &&
           ops->write(ops->context,&a->unit_id,sizeof(a->unit_id)) &&
           ops->write(ops->context,&a->progress,sizeof(a->progress)) &&
           ops->write(ops->context,&a->request_id,sizeof(a->request_id)) &&
           ops->write(ops->context,&a->prepaid,sizeof(a->prepaid));
}
int aoe_train_action_serialize_base(const AoeTrainAction *a,
                                    const AoeTrainActionSerializeOps *ops) {
    if (!a || !ops || !ops->serialize_base || !ops->write) return 0;
    return ops->serialize_base(ops->context,a) &&
           ops->write(ops->context,&a->unit_id,sizeof(a->unit_id)) &&
           ops->write(ops->context,&a->progress,sizeof(a->progress)) &&
           ops->write(ops->context,&a->request_id,sizeof(a->request_id));
}

int aoe_train_action_activate_ror(AoeTrainAction *a, int active,
                                   const AoeTrainActivationOps *ops) {
    if (!a) return 0;
    if (!active) return 1;
    if (!ops || !ops->failed) return 0;
    if (a->unit_id < 0 || !a->trained_template) {
        ops->failed(ops->context,a);
        return 1;
    }
    if (!a->prepaid) {
        if (!ops->charge || !ops->signal_failure) return 0;
        if (!ops->charge(ops->context,a)) {
            ops->failed(ops->context,a);
            ops->signal_failure(ops->context,a);
            a->state=1;
            return 1;
        }
    }
    a->state=6;
    return 1;
}

int aoe_train_action_activate_base(AoeTrainAction *a, int active,
                                   const AoeTrainActivationOps *ops) {
    if (!a) return 0;
    if (!active) return 1;
    if (!ops || !ops->failed) return 0;
    if (a->unit_id < 0 || !a->trained_template) {
        ops->failed(ops->context,a);
        return 1;
    }
    if (!ops->charge || !ops->signal_failure) return 0;
    if (!ops->charge(ops->context,a)) {
        ops->failed(ops->context,a);
        ops->signal_failure(ops->context,a);
        a->state=1;
        return 1;
    }
    a->state=6;
    return 1;
}

int aoe_train_action_update_ror(AoeTrainAction *a, float delta,
                                 int fast, const AoeTrainActionOps *ops,
                                 uint8_t *result) {
    float next;
    int32_t created_id=0;
    uint8_t spawned=0;
    if (!a || !result) return 0;
    if (a->state==1) { *result=1;return 1; }
    if (a->state!=6) { *result=0;return 1; }
    if (!a->building_template || !a->trained_template || !ops || !ops->rng ||
        !ops->spawn ||
        !ops->notify || !ops->failed || !ops->completed ||
        !ops->signal) return 0;
    if (fast) next=1000.0f;
    else {
        if (!isfinite(delta) || !isfinite(a->progress) ||
            !isfinite(a->building_template->work_rate)) return 0;
        next=(float)((long double)a->building_template->work_rate * delta +
                     a->progress);
        if (!isfinite(next)) return 0;
    }
    /* Coordinates are converted by the x87 int64 helper, then low EAX
       reaches notification. This API supports finite signed-32 coordinates. */
    if (!isfinite(a->building_x) || !isfinite(a->building_y) ||
        (double)a->building_x < INT32_MIN || (double)a->building_x > INT32_MAX ||
        (double)a->building_y < INT32_MIN || (double)a->building_y > INT32_MAX)
        return 0;
    a->progress=next;
    if (next < a->trained_template->train_time) { *result=0;return 1; }
    /* Vtable +0x5c = 004073c0, a direct byte store before spawning. */
    a->state=1;
    if (!aoe_train_action_spawn_ror(a,ops->rng,ops->spawn,&created_id,&spawned))
        return 0;
    if (!spawned) {
        if (!aoe_train_action_refund_ror(a)) return 0;
        ops->notify(ops->context,104,a->owner_id,a->unit_id,0,0);
        ops->failed(ops->context,a);
        ops->signal(ops->context,a,505);
    } else {
        ops->notify(ops->context,102,a->owner_id,a->unit_id,
                    (int32_t)a->building_x,(int32_t)a->building_y);
        ops->completed(ops->context,a,created_id);
        ops->signal(ops->context,a,506);
    }
    *result=1;
    return 1;
}

int aoe_train_action_update_base(AoeTrainAction *a, float delta,
                                 int fast, const AoeTrainActionOps *ops,
                                 uint8_t *result) {
    return aoe_train_action_update_ror(a,delta,fast,ops,result);
}
