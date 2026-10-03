#include "command_dispatch.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static AoeLiveUnit barracks;
static DatUnit barracks_template, soldier, villager;
static int16_t busy;
static char trace[128];

static void mark(const char *s) {
    strncat(trace, s, sizeof(trace) - strlen(trace) - 1);
}
static AoeLiveUnit *resolve(void *c, int32_t id) {
    (void)c;
    return id == 40 ? &barracks : NULL;
}
static int16_t active(void *c, const AoeLiveUnit *u) { (void)c; (void)u; return busy; }
static const DatUnit *tmpl(void *c, const AoeLiveUnit *b, int16_t i) {
    (void)c; (void)b;
    return i == 75 ? &soldier : i == 83 ? &villager : NULL;
}
static int16_t location(void *c, const AoeLiveUnit *b, int16_t t) {
    (void)c; (void)b;
    return t == 5 ? 12 : 99;
}
static void start_train(void *c, AoeLiveUnit *b, int16_t u, int32_t r) {
    char s[32];
    (void)c; (void)b;
    snprintf(s, sizeof(s), "T%d/%d ", u, r);
    mark(s);
}
static void start_research(void *c, AoeLiveUnit *b, int16_t t, int32_t r) {
    char s[32];
    (void)c; (void)b;
    snprintf(s, sizeof(s), "R%d/%d ", t, r);
    mark(s);
}
static int has_group(void *c, const AoeLiveUnit *u) { (void)c; (void)u; return 1; }
static void group(void *c, AoeLiveUnit *b, int32_t r) {
    char s[32];
    (void)c; (void)b;
    snprintf(s, sizeof(s), "G%d ", r);
    mark(s);
}
static void player_train(void *c, uint8_t p, int16_t u, int32_t b, int32_t r) {
    char s[32];
    (void)c;
    snprintf(s, sizeof(s), "PT%u:%d:%d:%d ", p, u, b, r);
    mark(s);
}
static void player_research(void *c, uint8_t p, int16_t t, int32_t b,
                            int32_t r) {
    char s[32];
    (void)c;
    snprintf(s, sizeof(s), "PR%u:%d:%d:%d ", p, t, b, r);
    mark(s);
}
static void expect(const char *s) {
    if (strcmp(trace, s)) {
        fprintf(stderr, "%s != %s\n", trace, s);
        assert(0);
    }
    trace[0] = 0;
}

int main(void) {
    AoeCommandProductionOps ops = {
        {0, resolve}, 0, active, tmpl, location, start_train,
        start_research, has_group, group, player_train, player_research};
    AoeCommandTribeDispatchRor tribe;
    AoeRng rng;
    uint8_t r[16];
    barracks.constructed = 1;
    barracks.core.unit_id = 40;
    barracks.core.unit_template = &barracks_template;
    barracks_template.id = 12;
    soldier.train_location_id = 12;
    villager.train_location_id = 109;
    memset(&tribe, 0, sizeof(tribe));
    aoe_rng_seed(&rng, 1);
    tribe.rng = &rng;
    aoe_command_bind_production_ror(&tribe, &ops);

    assert(aoe_command_encode_production_ror(r, sizeof(r), 100, 40, 2, 75,
                                             -1) == 16);
    assert(aoe_command_dispatch_tribe_ror(&tribe, r, 16) ==
           AOE_COMMAND_ROUTE_HANDLED);
    expect("T75/-1 G-1 ");
    /* A unit trained elsewhere does nothing at all. */
    assert(aoe_command_encode_production_ror(r, sizeof(r), 100, 40, 2, 83,
                                             7) == 16 &&
           aoe_command_execute_production_ror(r, 16, &ops) == 0);
    expect("");
    villager.train_location_id = -1; /* -1 trains anywhere */
    assert(aoe_command_execute_production_ror(r, 16, &ops) == 1);
    expect("T83/7 G7 ");
    /* A busy building (researching) defers to the player. */
    busy = 0x67;
    assert(aoe_command_execute_production_ror(r, 16, &ops) == 1);
    expect("PT2:83:40:7 ");
    busy = 0;
    /* Research: exact location match only. */
    assert(aoe_command_encode_production_ror(r, sizeof(r), 0x65, 40, 2, 5,
                                             -1) == 16 &&
           aoe_command_dispatch_tribe_ror(&tribe, r, 16) ==
               AOE_COMMAND_ROUTE_HANDLED);
    expect("R5/-1 G-1 ");
    r[10] = 6;
    assert(aoe_command_execute_production_ror(r, 16, &ops) == 0);
    expect("");
    /* Missing building: player fallback with the raw building ID. */
    r[4] = 41;
    assert(aoe_command_execute_production_ror(r, 16, &ops) == 1);
    expect("PR2:6:41:-1 ");
    assert(aoe_command_execute_production_ror(r, 15, &ops) == -1);
    return 0;
}
