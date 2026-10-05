#include "sound_effect.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int32_t roll(const AoeSoundEffectOpsRor *ops) {
    uint16_t r = ops && ops->rng ? aoe_rng_next(ops->rng) : 0;
    return (int32_t)r * 100 / 0x7fff;
}

static void clear_items(AoeSoundEffectRor *e) {
    e->items = NULL;
    e->count = 0;
}

int aoe_sound_effect_load_text_ror(AoeSoundEffectRor *e, const char *text,
                                   int16_t id) {
    int n = 0, i;
    short count = 0;
    if (!e) return 0;
    e->id = id;
    e->field_0 = 0;
    clear_items(e);
    e->cache_ms = 300000;
    if (!text || sscanf(text, "%hd%n", &count, &n) < 1) return 1;
    text += n;
    e->count = count;
    if (count < 1) return 1;
    e->items = calloc((size_t)count, sizeof(*e->items));
    if (!e->items) {
        e->count = 0;
        return 0;
    }
    for (i = 0; i < count; ++i) {
        AoeSoundVariantRor *v = &e->items[i];
        char name[0x100];
        int resource = 0;
        short prob = 0;
        name[0] = '\0';
        n = 0;
        if (sscanf(text, "%d %255s %hd%n", &resource, name, &prob, &n) >= 3)
            text += n;
        v->resource = resource;
        v->probability = prob;
        /* Port guard: the original appends ".wav" past 13 characters. */
        snprintf(v->name, sizeof(v->name), "%.11s.wav", name);
        v->wave = NULL;
    }
    return 1;
}

int aoe_sound_effect_load_ror(AoeSoundEffectRor *e, AoeOutcomeStream *in,
                              void *system, const AoeSoundEffectOpsRor *ops) {
    const uint8_t *p;
    int i;
    if (!e || !in) return 0;
    p = aoe_stream_take(in, 2);
    e->id = p ? (int16_t)(p[0] | p[1] << 8) : 0;
    p = aoe_stream_take(in, 2);
    e->field_0 = p ? (int16_t)(p[0] | p[1] << 8) : 0;
    p = aoe_stream_take(in, 2);
    e->count = p ? (int16_t)(p[0] | p[1] << 8) : 0;
    e->cache_ms = (uint32_t)aoe_stream_i32(in);
    if (e->count < 1) {
        e->items = NULL;
        return !in->failed;
    }
    e->items = calloc((size_t)e->count, sizeof(*e->items));
    if (!e->items) {
        e->count = 0;
        return 0;
    }
    for (i = 0; i < e->count; ++i) {
        AoeSoundVariantRor *v = &e->items[i];
        p = aoe_stream_take(in, 0xd);
        if (p) memcpy(v->name, p, 0xd);
        v->name[0xd] = '\0';
        v->resource = aoe_stream_i32(in);
        p = aoe_stream_take(in, 2);
        v->probability = p ? (int16_t)(p[0] | p[1] << 8) : 0;
        v->wave = ops && ops->create_wave
                      ? ops->create_wave(ops->context, system, v)
                      : NULL;
        v->state = 2;
    }
    return !in->failed;
}

static void put16(AoeOutcomeStream *out, int16_t v) {
    uint8_t b[2] = {(uint8_t)v, (uint8_t)((uint16_t)v >> 8)};
    aoe_stream_put(out, b, 2);
}

void aoe_sound_effect_save_ror(const AoeSoundEffectRor *e,
                               AoeOutcomeStream *out) {
    int i;
    if (!e || !out) return;
    put16(out, e->id);
    put16(out, e->field_0);
    put16(out, e->count);
    aoe_stream_put_i32(out, (int32_t)e->cache_ms);
    for (i = 0; i < e->count && e->items; ++i) {
        aoe_stream_put(out, e->items[i].name, 0xd);
        aoe_stream_put_i32(out, e->items[i].resource);
        put16(out, e->items[i].probability);
    }
}

static void drop_wave(AoeSoundVariantRor *v, const AoeSoundEffectOpsRor *ops) {
    if (v->wave && ops && ops->free_wave) ops->free_wave(ops->context, v->wave);
    v->wave = NULL;
}

void aoe_sound_effect_free_ror(AoeSoundEffectRor *e,
                               const AoeSoundEffectOpsRor *ops) {
    int i;
    if (!e) return;
    e->field_0 = 0;
    if (!e->items) return;
    for (i = 0; i < e->count; ++i) {
        e->items[i].name[0] = '\0';
        drop_wave(&e->items[i], ops);
    }
    free(e->items);
    clear_items(e);
}

void aoe_sound_effect_attach_ror(AoeSoundEffectRor *e, void *system,
                                 const AoeSoundEffectOpsRor *ops) {
    int i;
    if (!e || !e->items) return;
    for (i = 0; i < e->count; ++i) {
        AoeSoundVariantRor *v = &e->items[i];
        drop_wave(v, ops);
        if (system && ops && ops->create_wave)
            v->wave = ops->create_wave(ops->context, system, v);
        v->state = 2;
    }
}

void aoe_sound_effect_expire_ror(AoeSoundEffectRor *e, uint32_t now_ms,
                                 const AoeSoundEffectOpsRor *ops) {
    int i;
    if (!e || e->count < 1 || !e->cache_ms || !e->items) return;
    for (i = 0; i < e->count; ++i) {
        AoeSoundVariantRor *v = &e->items[i];
        if (v->state == 1 && e->cache_ms < now_ms - v->last_ms) {
            if (ops && ops->unload) ops->unload(ops->context, v->wave);
            v->state = 2;
        }
    }
}

void aoe_sound_effect_stop_ror(AoeSoundEffectRor *e,
                               const AoeSoundEffectOpsRor *ops) {
    int i;
    if (!e || e->count < 1 || !e->items || !ops) return;
    for (i = 0; i < e->count; ++i) {
        void *w = e->items[i].wave;
        if (w && ops->playing && ops->playing(ops->context, w) && ops->stop)
            ops->stop(ops->context, w);
    }
}

static int choose(AoeSoundEffectRor *e, const AoeSoundEffectOpsRor *ops) {
    int32_t r;
    int i;
    if (!e || e->count < 1 || !e->items) return -1;
    r = roll(ops);
    for (i = 0; i < e->count; ++i) {
        r -= e->items[i].probability;
        if (r < 1) return i;
    }
    return -1;
}

void aoe_sound_effect_play_ror(AoeSoundEffectRor *e, int alternate,
                               const AoeSoundEffectOpsRor *ops) {
    int i = choose(e, ops);
    AoeSoundVariantRor *v;
    if (i < 0) return;
    v = &e->items[i];
    if (!v->wave) return;
    if (ops->log) {
        char line[0x100];
        snprintf(line, sizeof(line), "{%s", v->name);
        ops->log(ops->context, line);
    }
    if (ops->play) ops->play(ops->context, v->wave, alternate);
    if (ops->log) ops->log(ops->context, "}");
    v->state = 1;
    v->last_ms = ops->now_ms ? ops->now_ms(ops->context) : 0;
}

AoeSoundVariantRor *aoe_sound_effect_pick_ror(AoeSoundEffectRor *e,
                                              const AoeSoundEffectOpsRor *ops) {
    int i = choose(e, ops);
    if (i < 0 || !e->items[i].wave) return NULL;
    return &e->items[i];
}
