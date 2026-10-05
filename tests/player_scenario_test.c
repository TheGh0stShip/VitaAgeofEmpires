#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "player_scenario.h"

static int cond_saved, cond_loaded, cond_released, cond_flag;
static int cond_token;
static int locked_team;

static void cond_save(void *c, void *cond, AoeOutcomeStream *out) {
    (void)c;
    (void)cond;
    aoe_stream_put_u8(out, 0xcc);
    cond_saved++;
}
static void *cond_create(void *c, AoePlayerObjectRor *p, AoeOutcomeStream *in,
                         const int32_t *map, int32_t flag) {
    (void)c;
    (void)p;
    (void)map;
    assert(aoe_stream_u8(in) == 0xcc);
    cond_loaded++;
    cond_flag = flag;
    return &cond_token;
}
static void cond_release(void *c, void *cond) {
    (void)c;
    assert(cond == &cond_token);
    cond_released++;
}
static int32_t game_type(void *c) {
    (void)c;
    return 1;
}
static int32_t slot_player(void *c, int32_t slot) {
    (void)c;
    return slot == 3 ? 1 : 0;
}
static int32_t slot_kind(void *c, int32_t slot) {
    (void)c;
    (void)slot;
    return 4;
}
static int32_t slot_team(void *c, int32_t slot) {
    (void)c;
    assert(slot == 2);
    return locked_team;
}

static const AoePlayerObjectOps object_ops = {
    .conditions_create = cond_create, .conditions_release = cond_release};
static const AoePlayerScenarioOps ops = {
    NULL, game_type, slot_player, slot_kind, slot_team, cond_save, NULL,
    &object_ops};

static void make(AoePlayerObjectRor *p, const AoePlayerWorldRor *w,
                 uint8_t *stances, float *res) {
    memset(p, 0, sizeof(*p));
    p->world = w;
    p->player_id = 1;
    p->stances = stances;
    p->stance_count = w->player_count;
    p->resources = res;
    p->resource_count = 30;
}

static void test_round_trip(void) {
    AoePlayerWorldRor world = {3, NULL, 0, 64, 64, 4.01f, 0, 0};
    uint8_t s1[3] = {0, 1, 3}, s2[3] = {9, 9, 9};
    float r1[30], r2[30];
    AoePlayerObjectRor a, b;
    uint8_t buf[256];
    AoeOutcomeStream out = {buf, sizeof(buf), 0, 0}, in;
    int i;

    make(&a, &world, s1, r1);
    a.name = "Rome";
    a.view_x = 1.5f;
    a.view_y = 2.5f;
    a.view_tile_x = 1;
    a.view_tile_y = 2;
    a.allied_victory = 1;
    for (i = 0; i < 9; ++i) a.relations[i] = 10 + i;
    assert(aoe_player_scenario_save_ror(&a, &out, &ops));
    assert(cond_saved == 1);
    assert(out.pos == 2 + 5 + 12 + 1 + 2 + 3 + 36 + 1);

    make(&b, &world, s2, r2);
    locked_team = 0;
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    assert(aoe_player_scenario_load_ror(&b, &in, NULL, 1.09f, &ops));
    assert(in.pos == out.pos && cond_loaded == 1 && cond_flag == 1);
    assert(strcmp(b.name, "Rome") == 0 && b.view_y == 2.5f &&
           b.view_tile_y == 2 && b.allied_victory == 1);
    assert(memcmp(b.stances, s1, 3) == 0);
    /* Codes are stored for the three stance entries only. */
    for (i = 0; i < 3; ++i) assert(b.relations[i] == 10 + i);
    assert(b.relations[3] == 0);
    assert(b.conditions == &cond_token);

    /* Locked diplomacy: the bytes are consumed, nothing changes; an
       existing name is kept. The old object is released. */
    s2[0] = s2[1] = s2[2] = 9;
    b.allied_victory = 0;
    for (i = 0; i < 9; ++i) b.relations[i] = -5;
    locked_team = 2;
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    free(b.name);
    b.name = malloc(2);
    strcpy(b.name, "X");
    assert(aoe_player_scenario_load_ror(&b, &in, NULL, 1.09f, &ops));
    assert(in.pos == out.pos && cond_released == 1);
    assert(strcmp(b.name, "X") == 0 && b.allied_victory == 0);
    assert(s2[0] == 9 && b.relations[0] == -5);
    free(b.name);
}

static void test_versions(void) {
    AoePlayerWorldRor world = {3, NULL, 0, 64, 64, 4.01f, 0, 0};
    uint8_t s[3] = {9, 9, 9};
    float r[30];
    AoePlayerObjectRor p;
    uint8_t buf[256];
    AoeOutcomeStream out = {buf, sizeof(buf), 0, 0}, in;
    int32_t map[3] = {2, 1, 0};
    int i;

    /* Version 1.06 block: no name, view, allied byte, 29 floats, 3 stances,
       no relation codes, condition byte. */
    aoe_stream_put(&out, (const uint8_t[]){0, 0}, 2);
    for (i = 0; i < 2; ++i) aoe_stream_put_f32(&out, 4.0f);
    aoe_stream_put(&out, (const uint8_t[]){3, 0, 4, 0}, 4);
    aoe_stream_put_u8(&out, 1);
    for (i = 0; i < 29; ++i) aoe_stream_put_f32(&out, (float)i);
    aoe_stream_put(&out, (const uint8_t[]){3, 0}, 2);
    aoe_stream_put(&out, (const uint8_t[]){0, 1, 3}, 3);
    aoe_stream_put_u8(&out, 0xcc);

    make(&p, &world, s, r);
    for (i = 0; i < 30; ++i) r[i] = -1.0f;
    for (i = 0; i < 9; ++i) p.relations[i] = -7;
    locked_team = 0;
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    assert(aoe_player_scenario_load_ror(&p, &in, map, 1.06f, &ops));
    assert(in.pos == out.pos && cond_flag == 0 && !p.name);
    assert(r[28] == 28.0f && r[29] == -1.0f && p.allied_victory == 1);
    /* Entry i goes to map[i]: s[2] = 0, s[1] = 1, s[0] = 3. */
    assert(s[2] == 0 && s[1] == 1 && s[0] == 3);
    assert(p.relations[0] == 0 && p.relations[1] == 1 &&
           p.relations[2] == 2 && p.relations[3] == -7);

    /* The older reader: relation code 1 follows the entry's position. */
    s[0] = s[1] = s[2] = 9;
    for (i = 0; i < 9; ++i) p.relations[i] = -7;
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    assert(aoe_player_scenario_load_old_ror(&p, &in, map, 1.05f, &ops));
    assert(in.pos == out.pos && cond_flag == 0);
    assert(s[2] == 0 && s[1] == 1 && s[0] == 3);
    /* Position 1 is the player's ID (mapped to 1 as well): code 1; entry 0
       maps to player 2 with stance 0: code 2. */
    assert(p.relations[1] == 1 && p.relations[2] == 2);
    /* Above 1.05 the older reader does nothing. */
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    assert(aoe_player_scenario_load_old_ror(&p, &in, map, 1.06f, &ops));
    assert(in.pos == 0);
}

int main(void) {
    test_round_trip();
    test_versions();
    return 0;
}
