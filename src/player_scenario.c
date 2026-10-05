#include "player_scenario.h"

#include <stdlib.h>
#include <string.h>

static int16_t stream_i16(AoeOutcomeStream *s) {
    const uint8_t *p = aoe_stream_take(s, 2);
    return p ? (int16_t)(uint16_t)(p[0] | (p[1] << 8)) : 0;
}

static void stream_put_i16(AoeOutcomeStream *s, int16_t v) {
    uint8_t b[2];
    b[0] = (uint8_t)((uint16_t)v & 0xff);
    b[1] = (uint8_t)((uint16_t)v >> 8);
    aoe_stream_put(s, b, 2);
}

int aoe_player_scenario_save_ror(const AoePlayerObjectRor *player,
                                 AoeOutcomeStream *out,
                                 const AoePlayerScenarioOps *ops) {
    size_t name_length;
    int16_t count;
    int i;

    if (!player || !out || !player->world) return 0;
    name_length = player->name ? strlen(player->name) + 1 : 0;
    stream_put_i16(out, (int16_t)name_length);
    if (name_length)
        aoe_stream_put(out, player->name, (size_t)(int16_t)name_length);
    aoe_stream_put_f32(out, player->view_x);
    aoe_stream_put_f32(out, player->view_y);
    stream_put_i16(out, player->view_tile_x);
    stream_put_i16(out, player->view_tile_y);
    aoe_stream_put_u8(out, player->allied_victory);
    count = player->world->player_count;
    stream_put_i16(out, count);
    for (i = 0; i < count; ++i)
        aoe_stream_put_u8(out, player->stances && i < player->stance_count
                                   ? player->stances[i]
                                   : 0);
    for (i = 0; i < 9; ++i) aoe_stream_put_i32(out, player->relations[i]);
    aoe_player_conditions_save_ror(player, out, ops);
    return !out->failed;
}

void aoe_player_conditions_save_ror(const AoePlayerObjectRor *player,
                                    AoeOutcomeStream *out,
                                    const AoePlayerScenarioOps *ops) {
    if (player && ops && ops->conditions_save)
        ops->conditions_save(ops->context, player->conditions, out);
}

static void read_name(AoePlayerObjectRor *player, AoeOutcomeStream *in) {
    int16_t length = stream_i16(in);
    const uint8_t *bytes;
    if (length <= 0) return;
    bytes = aoe_stream_take(in, (size_t)length);
    if (!bytes || player->name) return;
    player->name = calloc((size_t)length, 1);
    if (!player->name) return;
    memcpy(player->name, bytes, (size_t)length);
    player->name[length - 1] = '\0'; /* port: keep the host string closed */
}

static int diplomacy_locked(const AoePlayerObjectRor *player,
                            const AoePlayerScenarioOps *ops) {
    int32_t slot, kind;
    if (!ops || !ops->game_type || ops->game_type(ops->context) == 7)
        return 0;
    if (!ops->slot_player || !ops->slot_kind || !ops->slot_team) return 0;
    for (slot = 1; slot < 9; ++slot) {
        if (ops->slot_player(ops->context, slot) != player->player_id)
            continue;
        kind = ops->slot_kind(ops->context, slot);
        if (kind != 2 && ops->slot_kind(ops->context, slot) != 4) continue;
        return ops->slot_team(ops->context, slot - 1) > 1;
    }
    return 0;
}

static void read_resources(AoePlayerObjectRor *player, AoeOutcomeStream *in) {
    int i;
    for (i = 0; i < 29; ++i) {
        float value = aoe_stream_f32(in);
        if (player->resources && i < player->resource_count)
            player->resources[i] = value;
    }
}

static void set_stance(AoePlayerObjectRor *player, int32_t index,
                       int self, uint8_t stance) {
    player->stances[index] = stance;
    if (index == 0)
        player->relations[0] = 0;
    else if (index >= 9)
        return; /* port: beyond the nine codes */
    else if (self)
        player->relations[index] = 1;
    else if (stance == 0)
        player->relations[index] = 2;
    else if (stance == 1)
        player->relations[index] = 3;
    else
        player->relations[index] = 4;
}

static void reload_conditions(AoePlayerObjectRor *player,
                              AoeOutcomeStream *in,
                              const int32_t *player_map, int32_t flag,
                              const AoePlayerScenarioOps *ops) {
    const AoePlayerObjectOps *object = ops ? ops->object : NULL;
    if (player->conditions && object && object->conditions_release)
        object->conditions_release(object->context, player->conditions);
    player->conditions = NULL;
    if (ops && ops->load_conditions)
        ops->load_conditions(ops->context, player, in, player_map, flag);
    else
        aoe_player_conditions_create_ror(player, in, player_map, flag,
                                         object);
}

static int ready(const AoePlayerObjectRor *player, const AoeOutcomeStream *in) {
    return player && in && player->world;
}

int aoe_player_scenario_load_ror(AoePlayerObjectRor *player,
                                 AoeOutcomeStream *in,
                                 const int32_t *player_map, float version,
                                 const AoePlayerScenarioOps *ops) {
    int locked;
    int16_t n, i;
    int32_t codes[9];

    if (!ready(player, in)) return 0;
    read_name(player, in);
    player->view_x = aoe_stream_f32(in);
    player->view_y = aoe_stream_f32(in);
    player->view_tile_x = stream_i16(in);
    player->view_tile_y = stream_i16(in);
    locked = diplomacy_locked(player, ops);
    if ((double)version > 1.0) {
        uint8_t allied = aoe_stream_u8(in);
        if (!locked) player->allied_victory = allied;
    }
    player->field_104 = 0;
    if (!((double)version > 1.06) && player->resource_count > 0)
        read_resources(player, in);
    n = stream_i16(in);
    for (i = 0; i < n; ++i) {
        uint8_t stance = aoe_stream_u8(in);
        int16_t index;
        if (locked) continue;
        index = player_map ? (int16_t)player_map[i] : i;
        if (index >= player->world->player_count || index < 0) continue;
        if (!player->stances || index >= player->stance_count) continue;
        set_stance(player, index, index == player->player_id, stance);
    }
    if ((double)version >= 1.08) {
        for (i = 0; i < 9; ++i) codes[i] = aoe_stream_i32(in);
        if (!locked) {
            for (i = 0; i < n; ++i) {
                int16_t index = i;
                if (player_map && player_map[i] >= 0)
                    index = (int16_t)player_map[i];
                /* Port: the original reads past its nine codes and writes
                   past the player's nine slots. */
                if (i >= 9 || index < 0 || index >= 9) continue;
                player->relations[index] = codes[i];
            }
        }
    }
    reload_conditions(player, in, player_map,
                      (double)version >= 1.09 ? 1 : 0, ops);
    return !in->failed;
}

int aoe_player_scenario_load_old_ror(AoePlayerObjectRor *player,
                                     AoeOutcomeStream *in,
                                     const int32_t *player_map,
                                     float version,
                                     const AoePlayerScenarioOps *ops) {
    int locked;
    int16_t n, i;

    if (!ready(player, in)) return 0;
    if ((double)version > 1.05) return 1;
    read_name(player, in);
    player->view_x = aoe_stream_f32(in);
    player->view_y = aoe_stream_f32(in);
    player->view_tile_x = stream_i16(in);
    player->view_tile_y = stream_i16(in);
    locked = diplomacy_locked(player, ops);
    if ((double)version > 1.0) {
        uint8_t allied = aoe_stream_u8(in);
        if (!locked) player->allied_victory = allied;
    }
    player->field_104 = 0;
    if (player->resource_count > 0) read_resources(player, in);
    n = stream_i16(in);
    for (i = 0; i < n; ++i) {
        uint8_t stance = aoe_stream_u8(in);
        int32_t index;
        if (locked) continue;
        index = player_map ? player_map[i] : i;
        if (index >= player->world->player_count || index < 0) continue;
        if (!player->stances || index >= player->stance_count) continue;
        set_stance(player, index, i == player->player_id, stance);
    }
    if ((double)version > 1.08) {
        for (i = 0; i < 9; ++i) player->relations[i] = aoe_stream_i32(in);
    }
    reload_conditions(player, in, player_map, 0, ops);
    return !in->failed;
}
