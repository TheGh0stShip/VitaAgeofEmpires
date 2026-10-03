#include "player_resources.h"

#include <assert.h>

static int refreshes;
static int32_t flag_seen_at_refresh;
static int32_t field_104;

static void refresh(void *context) {
    (void)context;
    ++refreshes;
    flag_seen_at_refresh = field_104;
}

static void virtual_add_tests(void) {
    float values[60] = {0};
    AoePlayerResources resources = {60, values};
    AoePlayerResourceTargetRor base = {&resources, 0, &field_104, 0, refresh};
    AoePlayerResourceTargetRor derived = base;
    derived.derived = 1;
    /* Base vtable 0x00544d18: plain 0x0045cfe0, flag ignored. */
    assert(aoe_player_resource_add_virtual_ror(&base, 3, 5.0f, 1) &&
           values[3] == 5.0f && values[49] == 0.0f);
    assert(aoe_player_resource_add_virtual_ror(&base, 0x32, 1.0f, 0) &&
           !refreshes && !field_104 && values[0x32] == 1.0f);
    /* 0x004f6c60: flagged resource 3 also feeds slot 49. */
    assert(aoe_player_resource_add_virtual_ror(&derived, 3, 2.0f, 1) &&
           values[3] == 7.0f && values[49] == 2.0f);
    assert(aoe_player_resource_add_virtual_ror(&derived, 3, 2.0f, 0) &&
           values[3] == 9.0f && values[49] == 2.0f);
    /* Resource 0x32 sets +0x104 before the world refresh, then adds. */
    assert(aoe_player_resource_add_virtual_ror(&derived, 0x32, 4.0f, 0) &&
           refreshes == 1 && flag_seen_at_refresh == 1 &&
           values[0x32] == 5.0f);
    /* The side effects precede 0x0045cfe0's own range check. */
    resources.count = 0x32;
    field_104 = 0;
    assert(aoe_player_resource_add_virtual_ror(&derived, 0x32, 4.0f, 0) &&
           refreshes == 2 && field_104 == 1 && values[0x32] == 5.0f);
    /* Host-only guards: missing hooks or an out-of-allocation slot 49. */
    derived.refresh_world = 0;
    assert(!aoe_player_resource_add_virtual_ror(&derived, 0x32, 1.0f, 0));
    resources.count = 49;
    assert(!aoe_player_resource_add_virtual_ror(&derived, 3, 1.0f, 1) &&
           values[3] == 9.0f && values[49] == 2.0f);
    assert(!aoe_player_resource_add_virtual_ror(0, 0, 1.0f, 0));
}

int main(void) {
    virtual_add_tests();
    float values[4] = {10.0f, 20.0f, 30.0f, 40.0f};
    AoePlayerResources resources = {4, values};
    aoe_player_resource_set(&resources, 0, 50.0f);
    aoe_player_resource_add(&resources, 0, -7.5f, AOE_PLAYER_RISE_OF_ROME);
    aoe_player_resource_add(&resources, 3, 2.0f, AOE_PLAYER_BASE);
    assert(values[0] == 42.5f && values[3] == 42.0f);
    aoe_player_resource_set(&resources, -1, 99.0f);
    aoe_player_resource_add(&resources, 4, 99.0f, AOE_PLAYER_RISE_OF_ROME);
    assert(values[0] == 42.5f && values[1] == 20.0f);
    assert(values[2] == 30.0f && values[3] == 42.0f);
    resources.count = 0;
    aoe_player_resource_set(&resources, 0, 99.0f);
    assert(values[0] == 42.5f);
    resources.count = 4;
    resources.values = 0;
    aoe_player_resource_add(&resources, 0, 99.0f, AOE_PLAYER_BASE);
    aoe_player_resource_set(0, 0, 99.0f);
    assert(values[0] == 42.5f);
    return 0;
}
