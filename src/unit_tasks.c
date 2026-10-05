#include "unit_tasks.h"

#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

static int16_t g16(AoeOutcomeStream *s) {
    const uint8_t *p = aoe_stream_take(s, 2);
    return p ? (int16_t)(p[0] | (p[1] << 8)) : 0;
}
static void p16(AoeOutcomeStream *s, int16_t v) {
    uint8_t b[2] = {(uint8_t)v, (uint8_t)((uint16_t)v >> 8)};
    aoe_stream_put(s, b, 2);
}
static void *pick(void **table, int16_t i) { return (i == -1 || !table) ? NULL : table[i]; }

AoeTaskRor *aoe_task_new_ror(int16_t type, int16_t id) {
    AoeTaskRor *t;
    if (type != 1) return NULL;
    t = calloc(1, sizeof *t);
    if (!t) return NULL;
    t->id = id;
    t->type = 1;
    t->field_e = -1;
    t->field_14 = t->field_16 = t->field_18 = t->field_1a = -1;
    return t;
}

void aoe_task_copy_ror(AoeTaskRor *t, const AoeTaskRor *src) {
    int16_t type = t->type, id = t->id;
    *t = *src;
    t->type = type;
    t->id = id;
}

void aoe_task_load_ror(AoeTaskRor *t, AoeOutcomeStream *s, void **sprites, void **sounds) {
    int i;
    t->id = g16(s);
    t->field_4 = aoe_stream_u8(s);
    t->field_6 = g16(s);
    t->field_8 = g16(s);
    t->field_a = g16(s);
    t->field_e = g16(s);
    t->field_14 = g16(s);
    t->field_16 = g16(s);
    t->field_18 = g16(s);
    t->field_1a = g16(s);
    t->field_1c = aoe_stream_f32(s);
    t->field_20 = aoe_stream_f32(s);
    t->field_24 = aoe_stream_f32(s);
    t->field_28 = aoe_stream_u8(s);
    t->field_2c = aoe_stream_f32(s);
    t->field_c = aoe_stream_u8(s);
    t->field_d = aoe_stream_u8(s);
    t->field_30 = g16(s);
    t->field_32 = g16(s);
    t->field_10 = aoe_stream_u8(s);
    t->field_11 = aoe_stream_u8(s);
    t->field_12 = aoe_stream_u8(s);
    for (i = 0; i < 4; ++i) t->graphics[i] = pick(sprites, g16(s));
    for (i = 0; i < 2; ++i) t->sounds[i] = pick(sounds, g16(s));
}

void aoe_task_text_ror(AoeTaskRor *t, FILE *f, void **sprites, void **sounds) {
    int16_t b4 = 0, b28 = 0, bc = 0, bd = 0, b10 = 0, b11 = 0, b12 = 0, g[4] = {-1, -1, -1, -1},
            snd[2] = {-1, -1};
    int i;
    if (fscanf(f,
               "%hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %f %f %f %hd %f %hd %hd %hd %hd %hd %hd "
               "%hd %hd %hd %hd %hd %hd %hd",
               &t->id, &b4, &t->field_6, &t->field_8, &t->field_a, &t->field_e, &t->field_14,
               &t->field_16, &t->field_18, &t->field_1a, &t->field_1c, &t->field_20, &t->field_24,
               &b28, &t->field_2c, &bc, &bd, &t->field_30, &t->field_32, &b10, &b11, &b12, &g[0],
               &g[1], &g[2], &g[3], &snd[0], &snd[1]) == EOF) {
    }
    t->field_4 = (uint8_t)b4;
    t->field_c = (uint8_t)bc;
    t->field_d = (uint8_t)bd;
    t->field_28 = (uint8_t)b28;
    t->field_10 = (uint8_t)b10;
    t->field_11 = (uint8_t)b11;
    t->field_12 = (uint8_t)b12;
    for (i = 0; i < 4; ++i) t->graphics[i] = pick(sprites, g[i]);
    for (i = 0; i < 2; ++i) t->sounds[i] = pick(sounds, snd[i]);
}

void aoe_task_save_ror(const AoeTaskRor *t, AoeOutcomeStream *s, const AoeTaskOpsRor *ops) {
    int i;
    p16(s, t->type);
    p16(s, t->id);
    aoe_stream_put_u8(s, t->field_4);
    p16(s, t->field_6);
    p16(s, t->field_8);
    p16(s, t->field_a);
    p16(s, t->field_e);
    p16(s, t->field_14);
    p16(s, t->field_16);
    p16(s, t->field_18);
    p16(s, t->field_1a);
    aoe_stream_put_f32(s, t->field_1c);
    aoe_stream_put_f32(s, t->field_20);
    aoe_stream_put_f32(s, t->field_24);
    aoe_stream_put_u8(s, t->field_28);
    aoe_stream_put_f32(s, t->field_2c);
    aoe_stream_put_u8(s, t->field_c);
    aoe_stream_put_u8(s, t->field_d);
    p16(s, t->field_30);
    p16(s, t->field_32);
    aoe_stream_put_u8(s, t->field_10);
    aoe_stream_put_u8(s, t->field_11);
    aoe_stream_put_u8(s, t->field_12);
    for (i = 0; i < 4; ++i)
        p16(s, t->graphics[i] ? ops->sprite_id(ops->context, t->graphics[i]) : -1);
    for (i = 0; i < 2; ++i) p16(s, t->sounds[i] ? ops->sound_id(ops->context, t->sounds[i]) : -1);
}

void aoe_task_list_free_ror(AoeTaskListRor *l) {
    int16_t i;
    if (l->tasks) {
        for (i = 0; i < l->count; ++i) {
            free(l->tasks[i]);
            l->tasks[i] = NULL;
        }
        free(l->tasks);
        l->tasks = NULL;
    }
    l->count = 0;
}

static int make(AoeTaskListRor *l, int16_t count) {
    l->count = count;
    if (count <= 0) return 1;
    l->tasks = calloc((size_t)count, sizeof *l->tasks);
    return l->tasks != NULL;
}

int aoe_task_list_copy_ror(AoeTaskListRor *l, const AoeTaskListRor *src) {
    int16_t i;
    if (l->tasks) aoe_task_list_free_ror(l);
    if (!make(l, src->count)) return 0;
    for (i = 0; i < l->count; ++i) {
        l->tasks[i] = aoe_task_new_ror(src->tasks[i]->type, i);
        if (l->tasks[i]) aoe_task_copy_ror(l->tasks[i], src->tasks[i]);
    }
    return 1;
}

int aoe_task_list_load_ror(AoeTaskListRor *l, AoeOutcomeStream *s, void **sprites, void **sounds) {
    int16_t i;
    if (l->tasks) aoe_task_list_free_ror(l);
    if (!make(l, g16(s))) return 0;
    for (i = 0; i < l->count; ++i) {
        int16_t type = g16(s);
        l->tasks[i] = aoe_task_new_ror(type, i);
        if (!l->tasks[i]) return 0; /* the original dereferences NULL */
        aoe_task_load_ror(l->tasks[i], s, sprites, sounds);
    }
    return !s->failed;
}

int aoe_task_list_text_ror(AoeTaskListRor *l, FILE *f, void **sprites, void **sounds) {
    int16_t i, count = 0;
    if (l->tasks) aoe_task_list_free_ror(l);
    if (fscanf(f, "%hd", &count) != 1) count = 0;
    if (!make(l, count)) return 0;
    for (i = 0; i < l->count; ++i) {
        int16_t type = 0;
        if (fscanf(f, "%hd", &type) != 1) type = 0;
        l->tasks[i] = aoe_task_new_ror(type, i);
        if (!l->tasks[i]) return 0;
        aoe_task_text_ror(l->tasks[i], f, sprites, sounds);
    }
    return 1;
}

void aoe_task_list_save_ror(const AoeTaskListRor *l, AoeOutcomeStream *s, const AoeTaskOpsRor *ops) {
    int16_t i;
    p16(s, l->count);
    for (i = 0; i < l->count; ++i) aoe_task_save_ror(l->tasks[i], s, ops);
}

void aoe_task_list_init_ror(AoeTaskListRor *l) {
    l->count = 0;
    l->tasks = NULL;
}

int aoe_task_is_kind_7_ror(const AoeTaskRor *t) { return t->field_6 == 7; }
