#include "player_body.h"

#include <stdlib.h>

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

void aoe_player_template_load_ror(AoePlayerObjectRor *player,
                                  AoeOutcomeStream *in, int16_t slot,
                                  uint8_t type, const AoePlayerBodyOps *ops) {
    void *object = NULL;
    if (!player || !player->templates || slot < 0 ||
        slot >= player->template_count)
        return;
    switch (type) {
    case 10: case 20: case 25: case 30: case 40: case 50: case 60:
        if (ops && ops->load_template)
            object = ops->load_template(ops->context, type, in);
        break;
    default:
        break;
    }
    player->templates[slot] = object;
}

static void *new_list(const AoePlayerBodyOps *ops) {
    return ops && ops->list_new ? ops->list_new(ops->context) : NULL;
}

static void load_list(void *list, AoeOutcomeStream *in,
                      const AoePlayerBodyOps *ops) {
    if (list && ops && ops->list_load) ops->list_load(ops->context, list, in);
}

int aoe_player_body_load_ror(AoePlayerObjectRor *player, AoeOutcomeStream *in,
                             const AoePlayerBodyOps *ops) {
    int16_t count, i;

    if (!player || !in || !player->world) return 0;
    count = stream_i16(in);
    player->template_count = count;
    player->templates = NULL;
    if (count > 0) {
        player->templates = calloc((size_t)count, sizeof(void *));
        if (!player->templates) return 0;
        /* The original reads the presence words straight into the table;
           here they are kept apart from the host pointers. */
        {
            uint8_t *present = calloc((size_t)count, 1);
            if (!present) return 0;
            for (i = 0; i < count; ++i) present[i] = aoe_stream_i32(in) != 0;
            for (i = 0; i < count; ++i) {
                if (!present[i]) continue;
                aoe_player_template_load_ror(player, in, i,
                                             aoe_stream_u8(in), ops);
            }
            free(present);
        }
    }
    player->view = ops && ops->view_load
                       ? ops->view_load(ops->context, player, in)
                       : NULL;
    player->memory = ops && ops->memory_load
                         ? ops->memory_load(ops->context, player, in)
                         : NULL;
    player->lists[0] = new_list(ops);
    load_list(player->lists[0], in, ops);
    player->lists[1] = new_list(ops);
    player->lists[2] = new_list(ops);
    load_list(player->lists[1], in, ops);
    if (player->world->save_version >= 7.09f)
        load_list(player->lists[2], in, ops);
    return !in->failed;
}

int aoe_player_body_save_ror(const AoePlayerObjectRor *player,
                             AoeOutcomeStream *out,
                             const AoePlayerBodyOps *ops) {
    int16_t i;
    int k;

    if (!player || !out) return 0;
    stream_put_i16(out, player->template_count);
    if (player->template_count > 0) {
        for (i = 0; i < player->template_count; ++i)
            aoe_stream_put_i32(out, player->templates && player->templates[i]
                                        ? 1
                                        : 0);
        for (i = 0; i < player->template_count; ++i)
            if (player->templates && player->templates[i] && ops &&
                ops->save_template)
                ops->save_template(ops->context, player->templates[i], out);
    }
    if (ops && ops->view_save) ops->view_save(ops->context, player->view, out);
    if (ops && ops->memory_save)
        ops->memory_save(ops->context, player->memory, out);
    for (k = 0; k < 3; ++k)
        if (ops && ops->list_save)
            ops->list_save(ops->context, player->lists[k], out);
    return !out->failed;
}

void aoe_player_body_finish_load_ror(AoePlayerObjectRor *player,
                                     AoeOutcomeStream *in,
                                     const AoePlayerBodyOps *ops) {
    int k;
    if (!player || !ops) return;
    if (ops->load_conditions)
        ops->load_conditions(ops->context, player, in, NULL, 1);
    else
        aoe_player_conditions_create_ror(player, in, NULL, 1, ops->object);
    for (k = 0; k < 3; ++k)
        if (player->lists[k] && ops->list_finish)
            ops->list_finish(ops->context, player->lists[k]);
}

void aoe_player_body_update_ror(AoePlayerObjectRor *player,
                                const AoePlayerBodyOps *ops) {
    if (!player || !ops) return;
    if (ops->global_update) ops->global_update(ops->context);
    if (ops->list_update) ops->list_update(ops->context, player->lists[2]);
    if (ops->table_update) ops->table_update(ops->context, player->table);
}

void *aoe_player_create_unit_ror(AoePlayerObjectRor *player, float x,
                                 float y, float z, int16_t index,
                                 uint8_t state, int32_t value,
                                 const AoePlayerBodyOps *ops) {
    void *template_object, *unit;
    if (!player || !ops || !player->templates || index < 0 ||
        index >= player->template_count)
        return NULL;
    template_object = player->templates[index];
    if (!template_object || !ops->create_from_template) return NULL;
    unit = ops->create_from_template(ops->context, template_object, player, x,
                                     y, z);
    if (!unit) return NULL;
    if (state >= 7) {
        if (ops->unit_68) ops->unit_68(ops->context, unit);
    } else if (state > 2) {
        if (ops->unit_6c) ops->unit_6c(ops->context, unit);
    } else if (ops->unit_b4) {
        ops->unit_b4(ops->context, unit, value);
    }
    return unit;
}

static void *pick_list(AoePlayerObjectRor *player, int in_2c, int in_30) {
    if (in_2c) return player->lists[1];
    if (in_30) return player->lists[2];
    return player->lists[0];
}

void *aoe_player_add_unit_ror(AoePlayerObjectRor *player, void *unit,
                              int in_2c, int in_30,
                              const AoePlayerBodyOps *ops) {
    void *node;
    if (!player || !ops || !ops->list_add) return NULL;
    node = ops->list_add(ops->context, pick_list(player, in_2c, in_30), unit);
    if (ops->world_unit_added) ops->world_unit_added(ops->context, unit);
    return node;
}

void aoe_player_remove_unit_ror(AoePlayerObjectRor *player, void *unit,
                                int in_2c, int in_30, void *node,
                                const AoePlayerBodyOps *ops) {
    if (!player || !ops) return;
    if (ops->list_remove)
        ops->list_remove(ops->context, pick_list(player, in_2c, in_30), unit,
                         node);
    if (ops->world_unit_removed && ops->unit_id)
        ops->world_unit_removed(ops->context, ops->unit_id(ops->context, unit));
}
