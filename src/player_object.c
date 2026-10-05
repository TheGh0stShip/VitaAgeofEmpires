#include "player_object.h"

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

static void stream_read(AoeOutcomeStream *s, void *dst, size_t n) {
    const uint8_t *p = aoe_stream_take(s, n);
    if (p && n) memcpy(dst, p, n);
}

void aoe_player_set_view_tile_ror(AoePlayerObjectRor *player, int16_t x,
                                  int16_t y) {
    if (!player) return;
    player->view_tile_x = x;
    player->view_tile_y = y;
}

void aoe_player_set_view_ror(AoePlayerObjectRor *player, float x, float y) {
    if (!player) return;
    player->view_x = x;
    player->view_y = y;
    /* x87 compares: C0 (below or unordered) stores 0; a value strictly
       above the int-converted limit stores the limit. */
    if (!(x >= 0.0f)) {
        player->view_x = 0.0f;
    } else if (player->world) {
        double limit = (double)(player->world->map_width - 1);
        if ((double)x > limit) player->view_x = (float)limit;
    }
    if (!(y >= 0.0f)) {
        player->view_y = 0.0f;
    } else if (player->world) {
        double limit = (double)(player->world->map_height - 1);
        if ((double)y > limit) player->view_y = (float)limit;
    }
}

void aoe_player_set_color_ror(AoePlayerObjectRor *player, uint32_t index) {
    uint32_t i = index & 0xffu;
    if (!player) return;
    player->color = NULL;
    if (player->world && player->world->colors &&
        (int32_t)i < player->world->color_count)
        player->color = player->world->colors[i];
}

static void release_templates(AoePlayerObjectRor *player,
                              const AoePlayerObjectOps *ops) {
    int16_t i;
    if (player->templates) {
        for (i = 0; i < player->template_count; ++i)
            if (player->templates[i] && ops && ops->delete_template)
                ops->delete_template(ops->context, player->templates[i]);
        free(player->templates);
    }
    player->templates = NULL;
    player->template_count = 0;
}

static void release(AoePlayerObjectRor *player) {
    free(player->stances);
    player->stances = NULL;
    player->stance_count = 0;
    free(player->name);
    player->name = NULL;
    free(player->resources);
    player->resources = NULL;
}

int aoe_player_object_load_ror(AoePlayerObjectRor *player,
                               const AoePlayerWorldRor *world,
                               uint8_t player_id, AoeOutcomeStream *in,
                               const AoePlayerObjectOps *ops) {
    int16_t name_length;
    uint8_t color;
    int i;

    if (!player || !world || !in || world->player_count < 0) return 0;
    player->field_04 = 0;
    player->field_0c = 10;
    player->player_id = player_id;
    player->field_08 = world->default_08;
    player->world = world;
    player->name = NULL;
    player->resources = NULL;
    player->stances = NULL;
    player->color = NULL;
    aoe_player_selection_init_ror(&player->selection);
    player->selection.player_id = player->player_id;
    if (ops && ops->create_members &&
        !ops->create_members(ops->context, player))
        return 0;
    player->stances =
        calloc(world->player_count ? (size_t)world->player_count : 1, 1);
    if (!player->stances) return 0;
    player->stance_count = world->player_count;
    player->stances_changed = 0;
    stream_read(in, player->stances, (size_t)world->player_count);
    for (i = 0; i < 9; ++i) player->relations[i] = aoe_stream_i32(in);
    player->field_104 =
        !(world->save_version >= 4.01f) ? 0 : aoe_stream_i32(in);
    player->allied_victory = aoe_stream_u8(in);
    name_length = stream_i16(in);
    if (name_length > 0) {
        player->name = calloc((size_t)name_length, 1);
        if (!player->name) goto fail;
        stream_read(in, player->name, (size_t)name_length);
        /* Port: keep the host string terminated. */
        player->name[name_length - 1] = '\0';
    }
    player->resource_count = stream_i16(in);
    if (player->resource_count > 0) {
        player->resources =
            calloc((size_t)player->resource_count, sizeof(float));
        if (!player->resources) goto fail;
        for (i = 0; i < player->resource_count; ++i)
            player->resources[i] = aoe_stream_f32(in);
    }
    player->view_x = aoe_stream_f32(in);
    player->view_y = aoe_stream_f32(in);
    player->view_tile_x = stream_i16(in);
    player->view_tile_y = stream_i16(in);
    player->field_54 = aoe_stream_u8(in);
    player->field_109 = aoe_stream_u8(in);
    player->state = aoe_stream_u8(in);
    player->resigned = aoe_stream_u8(in);
    color = aoe_stream_u8(in);
    aoe_player_set_color_ror(player, color);
    player->field_08 = aoe_stream_i32(in);
    player->field_0c = aoe_stream_i32(in);
    player->field_20 = 0;
    player->field_14 = 0;
    if (in->failed) goto fail;
    return 1;
fail:
    release(player);
    return 0;
}

int aoe_player_object_save_ror(const AoePlayerObjectRor *player,
                               AoeOutcomeStream *out) {
    size_t name_length;
    int i;

    if (!player || !out || !player->world) return 0;
    aoe_stream_put_u8(out, player->type);
    for (i = 0; i < player->world->player_count; ++i)
        aoe_stream_put_u8(out, player->stances && i < player->stance_count
                                   ? player->stances[i]
                                   : 0);
    for (i = 0; i < 9; ++i) aoe_stream_put_i32(out, player->relations[i]);
    aoe_stream_put_i32(out, player->field_104);
    aoe_stream_put_u8(out, player->allied_victory);
    name_length = player->name ? strlen(player->name) + 1 : 0;
    stream_put_i16(out, (int16_t)name_length);
    if ((int16_t)name_length > 0)
        aoe_stream_put(out, player->name, (size_t)(int16_t)name_length);
    stream_put_i16(out, player->resource_count);
    for (i = 0; i < player->resource_count; ++i)
        aoe_stream_put_f32(out, player->resources ? player->resources[i]
                                                  : 0.0f);
    aoe_stream_put_f32(out, player->view_x);
    aoe_stream_put_f32(out, player->view_y);
    stream_put_i16(out, player->view_tile_x);
    stream_put_i16(out, player->view_tile_y);
    aoe_stream_put_u8(out, player->field_54);
    aoe_stream_put_u8(out, player->field_109);
    aoe_stream_put_u8(out, player->state);
    aoe_stream_put_u8(out, player->resigned);
    aoe_stream_put_u8(out, player->color ? player->color->id : 0);
    aoe_stream_put_i32(out, player->field_08);
    aoe_stream_put_i32(out, player->field_0c);
    return !out->failed;
}

void aoe_player_object_destroy_ror(AoePlayerObjectRor *player,
                                   const AoePlayerObjectOps *ops) {
    if (!player) return;
    if (ops && ops->destroy_lists) ops->destroy_lists(ops->context, player);
    if (player->conditions && ops && ops->conditions_release)
        ops->conditions_release(ops->context, player->conditions);
    player->conditions = NULL;
    free(player->stances);
    player->stances = NULL;
    player->stance_count = 0;
    release_templates(player, ops);
    if (player->view && ops && ops->view_release)
        ops->view_release(ops->context, player->view);
    player->view = NULL;
    if (ops && ops->destroy_members)
        ops->destroy_members(ops->context, player);
    release(player);
    player->resource_count = 0;
}

void aoe_player_conditions_new_ror(AoePlayerObjectRor *player,
                                   const AoePlayerObjectOps *ops) {
    if (!player) return;
    player->conditions = ops && ops->conditions_new
                             ? ops->conditions_new(ops->context, player)
                             : NULL;
}

int aoe_player_object_init_ror(AoePlayerObjectRor *player,
                               const AoePlayerWorldRor *world,
                               const AoePlayerSourceRor *source,
                               uint8_t player_id, const char *name,
                               uint8_t field_109, int full,
                               const AoePlayerObjectOps *ops) {
    int16_t i;
    int32_t half_w, half_h;

    if (!player || !world || !source || world->player_count < 0) return 0;
    player->field_04 = 0;
    player->field_0c = 10;
    player->field_08 = world->default_08;
    player->field_109 = field_109;
    player->type = 0;
    player->world = world;
    player->player_id = player_id;
    player->stances_changed = 0;
    player->name = NULL;
    player->stances = NULL;
    player->resources = NULL;
    player->templates = NULL;
    player->template_count = 0;
    aoe_player_selection_init_ror(&player->selection);
    player->selection.player_id = player->player_id;
    if (ops && ops->create_members &&
        !ops->create_members(ops->context, player))
        return 0;
    if (name) {
        size_t n = strlen(name) + 1;
        player->name = calloc(n, 1);
        if (!player->name) goto fail;
        memcpy(player->name, name, n);
    }
    player->stances =
        calloc(world->player_count ? (size_t)world->player_count : 1, 1);
    if (!player->stances) goto fail;
    player->stance_count = world->player_count;
    for (i = 0; i < 9; ++i) player->relations[i] = -1;
    for (i = 0; i < world->player_count; ++i) {
        player->stances[i] = 1;
        /* Port: the original writes past the nine codes for player IDs of
           9 and more; those writes are dropped. */
        if (i >= 9) continue;
        if (i == player->player_id)
            player->relations[i] = 1;
        else if (i == 0)
            player->relations[0] = 0;
        else
            player->relations[i] = 3;
    }
    player->field_d4[0] = player->field_d4[1] = player->field_d4[2] = 0;
    player->field_104 = 0;
    player->allied_victory = 0;
    player->template_count = source->template_count;
    if (source->template_count > 0) {
        player->templates =
            calloc((size_t)source->template_count, sizeof(void *));
        if (!player->templates) goto fail;
        for (i = 0; i < source->template_count; ++i) {
            void *src = source->templates ? source->templates[i] : NULL;
            if (src && ops && ops->clone_template)
                player->templates[i] = ops->clone_template(ops->context, src);
        }
    }
    if (!full) {
        player->conditions = NULL;
        player->view = NULL;
    } else {
        if (ops && ops->create_lists) ops->create_lists(ops->context, player);
        aoe_player_conditions_new_ror(player, ops);
        player->view = ops && ops->view_create
                           ? ops->view_create(ops->context, player)
                           : NULL;
    }
    player->resource_count = source->resource_count;
    if (source->resource_count > 0) {
        player->resources =
            calloc((size_t)source->resource_count, sizeof(float));
        if (!player->resources) goto fail;
        if (source->resources)
            memcpy(player->resources, source->resources,
                   (size_t)source->resource_count * sizeof(float));
    }
    if (player->field_109 == 15 && world->game_ad9 == 1 &&
        player->resource_count > 46)
        player->resources[46] = 0.25f;
    player->field_54 = source->field_28;
    half_w = world->map_width / 2;
    half_h = world->map_height / 2;
    aoe_player_set_view_ror(player, (float)half_w, (float)half_h);
    aoe_player_set_view_tile_ror(player, (int16_t)half_w, (int16_t)half_h);
    player->state = 0;
    player->resigned = 0;
    player->color = NULL;
    player->field_110 = source->field_2a;
    if (full && player->field_110 >= 0 && ops && ops->register_player)
        ops->register_player(ops->context, (int16_t)player->field_110,
                             player);
    player->field_20 = 0;
    player->field_14 = 0;
    return 1;
fail:
    release_templates(player, ops);
    release(player);
    return 0;
}

void aoe_player_object_delete_ror(AoePlayerObjectRor *player,
                                  const AoePlayerObjectOps *ops) {
    aoe_player_object_destroy_ror(player, ops);
}

void aoe_player_units_68_ror(AoePlayerObjectRor *player,
                             const AoePlayerObjectOps *ops) {
    size_t i;
    if (!player || !ops || !ops->unit_68 || !player->selection.units) return;
    for (i = 0; i < player->selection.unit_count; ++i)
        if (player->selection.units[i])
            ops->unit_68(ops->context, player->selection.units[i]);
}

void aoe_player_view_refresh_ror(AoePlayerObjectRor *player,
                                 const AoePlayerObjectOps *ops) {
    if (player && player->view && ops && ops->view_refresh)
        ops->view_refresh(ops->context, player->view);
}

void aoe_player_view_reset_ror(AoePlayerObjectRor *player,
                               const AoePlayerObjectOps *ops) {
    if (!player || !ops) return;
    if (player->view && ops->view_release)
        ops->view_release(ops->context, player->view);
    player->view =
        ops->view_create ? ops->view_create(ops->context, player) : NULL;
}

void aoe_player_conditions_create_ror(AoePlayerObjectRor *player,
                                      AoeOutcomeStream *in,
                                      const int32_t *player_map,
                                      int32_t flag,
                                      const AoePlayerObjectOps *ops) {
    if (!player || !ops) return;
    player->conditions =
        ops->conditions_create
            ? ops->conditions_create(ops->context, player, in, player_map,
                                     flag)
            : NULL;
}

int32_t aoe_player_default_14_ror(AoePlayerObjectRor *player, int32_t a) {
    (void)player;
    (void)a;
    return 0;
}

void aoe_player_default_38_ror(AoePlayerObjectRor *player, int32_t a) {
    (void)player;
    (void)a;
}

int32_t aoe_player_default_3c_ror(AoePlayerObjectRor *player, int32_t a) {
    (void)player;
    (void)a;
    return -1;
}

void aoe_player_default_5c_ror(AoePlayerObjectRor *player, int32_t a,
                               int32_t b, int32_t c) {
    (void)player;
    (void)a;
    (void)b;
    (void)c;
}

void aoe_player_default_2c_ror(AoePlayerObjectRor *player, int32_t a,
                               int32_t b, int32_t c, int32_t d, int32_t e) {
    (void)player;
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    (void)e;
}
