/* Patched RoR 1.0a base action vtable entries 0x00407290..0x00407690. */
#include "action_move.h"

#include <assert.h>
#include <string.h>

typedef struct {
    uint8_t target_state, second_state;
    int32_t sub;
    int cleared, cleared_second;
} Env;

static uint8_t unit_state(void *c, AoeLiveUnit *u) {
    Env *e = c;
    return (uintptr_t)u == 1 ? e->target_state : e->second_state;
}
static AoeLiveUnit *second(void *c, AoeMoveActionRor *a) {
    (void)c; (void)a;
    return (AoeLiveUnit *)(uintptr_t)2;
}
static void clear_target(void *c, AoeMoveActionRor *a) {
    ((Env *)c)->cleared = 1;
    a->target = NULL;
}
static void clear_second(void *c, AoeMoveActionRor *a) {
    (void)a;
    ((Env *)c)->cleared_second = 1;
}
static int32_t sub_count(void *c, AoeMoveActionRor *a) {
    (void)a;
    return ((Env *)c)->sub;
}
static uint32_t sub_done(void *c, AoeMoveActionRor *a) {
    (void)c; (void)a;
    return 7;
}

int main(void) {
    AoeMoveActionRor action = {0};
    Env env = {0};
    AoeActionDoneOps ops = {&env, unit_state, second, clear_target,
                            clear_second, sub_count, sub_done};
    char text[16];

    action.type = 9;
    action.z = 2.5f;
    assert(aoe_action_type_ror(&action) == 9 && aoe_action_z_ror(&action) == 2.5f);
    action.state = 15;
    aoe_action_state_name_ror(&action, text, sizeof(text));
    assert(!strcmp(text, "MoveNoSearch"));
    assert(!strcmp(aoe_action_state_text_ror(0), "None") &&
           !strcmp(aoe_action_state_text_ror(9), "Turn") &&
           !strcmp(aoe_action_state_text_ror(16), "Unknown"));
    assert(!aoe_action_default_activate_ror(&action) &&
           !aoe_action_default_update_ror(&action) &&
           !aoe_action_default_2c_ror(&action));
    aoe_action_default_retarget_ror(&action);

    action.target = (AoeLiveUnit *)(uintptr_t)1;
    env.target_state = 7;
    env.second_state = 6;
    action.state = 1;
    assert(aoe_action_base_done_ror(&action, &ops) == 1);
    assert(env.cleared && !action.target && !env.cleared_second);
    env.second_state = 8;
    env.sub = 2;
    assert(aoe_action_base_done_ror(&action, &ops) == 7 && env.cleared_second);
    env.sub = 0;
    action.state = 4;
    assert(aoe_action_base_done_ror(&action, &ops) == 0);
    return 0;
}
