#include "action_io.h"

#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

static int16_t get16(AoeOutcomeStream *s) {
    const uint8_t *p = aoe_stream_take(s, 2);
    return p ? (int16_t)(p[0] | (p[1] << 8)) : 0;
}

static void put16(AoeOutcomeStream *s, int16_t v) {
    uint8_t b[2] = {(uint8_t)v, (uint8_t)((uint16_t)v >> 8)};
    aoe_stream_put(s, b, 2);
}

static uint32_t get32(AoeOutcomeStream *s) { return (uint32_t)aoe_stream_i32(s); }
static void put32(AoeOutcomeStream *s, uint32_t v) { aoe_stream_put_i32(s, (int32_t)v); }

int aoe_action_load_base_ror(AoeActionRecordRor *a, AoeOutcomeStream *s, int ror,
                             int32_t player_count, float version) {
    int16_t type = a->type;
    memset(a, 0, sizeof *a);
    a->type = type;
    a->field_18 = a->field_1c = -1;
    a->state = aoe_stream_u8(s);
    a->target_id = aoe_stream_i32(s);
    a->target2_id = aoe_stream_i32(s);
    a->field_18 = aoe_stream_i32(s);
    a->field_1c = aoe_stream_i32(s);
    a->field_20 = get32(s);
    a->field_24 = get32(s);
    a->field_28 = get32(s);
    a->field_2c = get32(s);
    a->task = get16(s);
    a->field_3c = aoe_stream_u8(s);
    if (!aoe_action_list_load_ror(&a->subs, &a->sub_count, s, ror, player_count, version))
        return 0;
    a->graphic = get16(s);
    return !s->failed;
}

void aoe_action_save_base_ror(const AoeActionRecordRor *a, AoeOutcomeStream *s, int ror) {
    put16(s, a->type);
    aoe_stream_put_u8(s, a->state);
    aoe_stream_put_i32(s, a->target_id);
    aoe_stream_put_i32(s, a->target2_id);
    aoe_stream_put_i32(s, a->field_18);
    aoe_stream_put_i32(s, a->field_1c);
    put32(s, a->field_20);
    put32(s, a->field_24);
    put32(s, a->field_28);
    put32(s, a->field_2c);
    put16(s, a->task);
    aoe_stream_put_u8(s, a->field_3c);
    aoe_action_list_save_ror(a->subs, a->sub_count, s, ror);
    put16(s, a->graphic);
}

void aoe_action_release_ror(AoeActionRecordRor *a) {
    int32_t i;
    for (i = 0; i < a->sub_count; ++i) aoe_action_release_ror(&a->subs[i]);
    free(a->subs);
    a->subs = NULL;
    a->sub_count = 0;
    free(a->bytes);
    a->bytes = NULL;
    a->byte_count = 0;
}

int aoe_action_list_load_ror(AoeActionRecordRor **list, int32_t *count, AoeOutcomeStream *s,
                             int ror, int32_t player_count, float version) {
    int32_t i;
    for (i = 0; i < *count; ++i) aoe_action_release_ror(&(*list)[i]);
    free(*list);
    *list = NULL;
    *count = 0;
    for (;;) {
        int16_t type = get16(s);
        AoeActionRecordRor *grown;
        if (type == 0 || s->failed) break;
        grown = realloc(*list, (size_t)(*count + 1) * sizeof **list);
        if (!grown) return 0;
        *list = grown;
        if (!aoe_action_load_ror(&grown[*count], type, s, ror, player_count, version)) {
            aoe_action_release_ror(&grown[*count]);
            return 0;
        }
        ++*count;
    }
    return !s->failed;
}

void aoe_action_list_save_ror(const AoeActionRecordRor *list, int32_t count, AoeOutcomeStream *s,
                              int ror) {
    int32_t i;
    for (i = 0; i < count; ++i) aoe_action_save_ror(&list[i], s, ror);
    put16(s, 0);
}

/* Which types each factory makes. */
static int known(int16_t type, int ror) {
    switch (type) {
    case 1: case 4: case 5: case 8: case 9: case 10: case 0x15:
        return 1;
    case 3: case 0xc: case 0x65: case 0x66: case 0x67: case 0x68: case 0x69: case 0x6a:
    case 0x6b: case 0x6c: case 0x6e: case 0x6f: case 0x78:
        return ror;
    default:
        return 0;
    }
}

int aoe_action_load_ror(AoeActionRecordRor *a, int16_t type, AoeOutcomeStream *s, int ror,
                        int32_t player_count, float version) {
    int i;
    memset(a, 0, sizeof *a);
    if (!known(type, ror)) return 0;
    a->type = type;
    if (!aoe_action_load_base_ror(a, s, ror, player_count, version)) return 0;
    switch (type) {
    case 1: case 5: case 0x6e:
        a->u32[0] = get32(s);
        break;
    case 8:
        for (i = 0; i < 5; ++i) a->u32[i] = get32(s);
        break;
    case 9:
        a->u32[0] = get32(s);
        a->u32[1] = get32(s);
        a->i16[0] = get16(s);
        a->i16[1] = get16(s);
        for (i = 0; i < 3; ++i) a->u8[i] = aoe_stream_u8(s);
        for (i = 0; i < 3; ++i) a->graphics[i] = get16(s);
        break;
    case 0x66:
        a->i16[0] = get16(s);
        a->u32[0] = get32(s);
        a->u32[1] = get32(s);
        a->u8[0] = version >= 7.23f ? aoe_stream_u8(s) : 0;
        break;
    case 0x67:
        a->i16[0] = get16(s);
        a->u32[0] = get32(s);
        break;
    case 0x68:
        a->u8[0] = aoe_stream_u8(s);
        a->u32[0] = get32(s); /* 1.0f before the read */
        break;
    case 0x6a:
        a->u8[0] = version >= 6.5f ? aoe_stream_u8(s) : 0;
        break;
    case 0x6c: {
        const uint8_t *p;
        a->byte_count = player_count > 0 ? player_count : 0;
        a->bytes = calloc((size_t)(a->byte_count ? a->byte_count : 1), 1);
        if (!a->bytes) return 0;
        p = aoe_stream_take(s, (size_t)a->byte_count);
        if (p) memcpy(a->bytes, p, (size_t)a->byte_count);
        break;
    }
    case 0x6f:
        a->u32[0] = get32(s);
        a->u32[1] = get32(s);
        break;
    default:
        break;
    }
    return !s->failed;
}

void aoe_action_save_ror(const AoeActionRecordRor *a, AoeOutcomeStream *s, int ror) {
    int i;
    aoe_action_save_base_ror(a, s, ror);
    switch (a->type) {
    case 1: case 5: case 0x6e:
        put32(s, a->u32[0]);
        break;
    case 8:
        for (i = 0; i < 5; ++i) put32(s, a->u32[i]);
        break;
    case 9:
        put32(s, a->u32[0]);
        put32(s, a->u32[1]);
        put16(s, a->i16[0]);
        put16(s, a->i16[1]);
        for (i = 0; i < 3; ++i) aoe_stream_put_u8(s, a->u8[i]);
        for (i = 0; i < 3; ++i) put16(s, a->graphics[i]);
        break;
    case 0x66:
        put16(s, a->i16[0]);
        put32(s, a->u32[0]);
        put32(s, a->u32[1]);
        aoe_stream_put_u8(s, a->u8[0]);
        break;
    case 0x67:
        put16(s, a->i16[0]);
        put32(s, a->u32[0]);
        break;
    case 0x68:
        aoe_stream_put_u8(s, a->u8[0]);
        put32(s, a->u32[0]);
        break;
    case 0x6a:
        aoe_stream_put_u8(s, a->u8[0]);
        break;
    case 0x6c:
        aoe_stream_put(s, a->bytes, (size_t)a->byte_count);
        break;
    case 0x6f:
        put32(s, a->u32[0]);
        put32(s, a->u32[1]);
        break;
    default:
        break;
    }
}

int32_t aoe_action_target_ror(const AoeActionRecordRor *a) { return a->target_id; }
int32_t aoe_action_target2_ror(const AoeActionRecordRor *a) { return a->target2_id; }

int aoe_action_drop_dead_targets_ror(AoeActionRecordRor *a,
                                     int (*state)(void *context, int32_t id), void *context) {
    if (a->target_id != -1 && state(context, a->target_id) > 6) a->target_id = -1;
    if (a->target2_id != -1 && state(context, a->target2_id) > 6) a->target2_id = -1;
    return 1;
}
