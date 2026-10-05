#include "effects_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void aoe_text_underscores_to_spaces_ror(char *text) {
    for (; *text; ++text)
        if (*text == '_') *text = ' ';
}

static int16_t get16(AoeOutcomeStream *in) {
    const uint8_t *p = aoe_stream_take(in, 2);
    return p ? (int16_t)(p[0] | p[1] << 8) : 0;
}

static void put16(AoeOutcomeStream *out, int16_t v) {
    uint8_t b[2] = {(uint8_t)v, (uint8_t)((uint16_t)v >> 8)};
    aoe_stream_put(out, b, 2);
}

int aoe_effects_read_ror(DatEffects *effects, AoeOutcomeStream *in) {
    int32_t count = aoe_stream_i32(in), i;
    memset(effects, 0, sizeof *effects);
    if (count < 1 || in->failed) return !in->failed;
    effects->effects = calloc((size_t)count, sizeof *effects->effects);
    if (!effects->effects) return 0;
    effects->count = (uint32_t)count;
    for (i = 0; i < count && !in->failed; ++i) {
        DatEffect *e = &effects->effects[i];
        const uint8_t *name = aoe_stream_take(in, 0x1f);
        int16_t n, j;
        if (name) memcpy(e->name, name, 0x1f);
        e->name[0x1f] = '\0';
        n = get16(in);
        if (n < 1) continue;
        e->commands = calloc((size_t)n, sizeof *e->commands);
        if (!e->commands) return 0;
        e->command_count = (uint16_t)n;
        for (j = 0; j < n; ++j) {
            DatEffectCommand *c = &e->commands[j];
            c->type = (int8_t)aoe_stream_u8(in);
            c->target_unit = get16(in);
            c->unit_class = get16(in);
            c->attribute = get16(in);
            c->amount = aoe_stream_f32(in);
        }
    }
    return !in->failed;
}

int aoe_effects_tribe_read_ror(DatEffects *effects, AoeOutcomeStream *in) {
    return aoe_effects_read_ror(effects, in);
}

int aoe_effects_read_text_ror(DatEffects *effects, const char *text) {
    const char *p = text;
    int total, listed, used, i, index;
    memset(effects, 0, sizeof *effects);
    if (!text) return 0;
    if (sscanf(p, "%d %d%n", &total, &listed, &used) < 2) return 1;
    p += used;
    if (total < 1) return 1;
    effects->effects = calloc((size_t)total, sizeof *effects->effects);
    if (!effects->effects) return 0;
    effects->count = (uint32_t)total;
    for (i = 0; i < listed; ++i) {
        DatEffect *e;
        char name[256];
        short n, j;
        if (sscanf(p, "%d%n", &index, &used) < 1) break;
        p += used;
        if (sscanf(p, "%255s %hd%n", name, &n, &used) < 2) break;
        p += used;
        /* Port guard: the original writes any index it reads. */
        if (index < 0 || index >= total) break;
        e = &effects->effects[index];
        /* The original's %s fills the 31-byte name unchecked; the port
           keeps the first 31 characters. */
        name[31] = '\0';
        memcpy(e->name, name, strlen(name) + 1);
        e->command_count = 0;
        if (n > 0) {
            free(e->commands);
            e->commands = calloc((size_t)n, sizeof *e->commands);
            if (!e->commands) return 0;
            for (j = 0; j < n; ++j) {
                short type, unit, cls, attribute;
                float amount;
                if (sscanf(p, "%hd %hd %hd %hd %f%n", &type, &unit, &cls,
                           &attribute, &amount, &used) < 5)
                    break;
                p += used;
                e->commands[j].type = (int8_t)type;
                e->commands[j].target_unit = unit;
                e->commands[j].unit_class = cls;
                e->commands[j].attribute = attribute;
                e->commands[j].amount = amount;
                e->command_count = (uint16_t)(j + 1);
            }
            /* The original keeps the declared count even when a command
               fails to read; the port counts only those read. */
        }
        aoe_text_underscores_to_spaces_ror(e->name);
    }
    return 1;
}

void aoe_effects_write_ror(const DatEffects *effects, AoeOutcomeStream *out) {
    uint32_t i;
    aoe_stream_put_i32(out, (int32_t)effects->count);
    for (i = 0; i < effects->count; ++i) {
        const DatEffect *e = &effects->effects[i];
        uint16_t j;
        aoe_stream_put(out, e->name, 0x1f);
        put16(out, (int16_t)e->command_count);
        for (j = 0; j < e->command_count; ++j) {
            const DatEffectCommand *c = &e->commands[j];
            aoe_stream_put_u8(out, (uint8_t)c->type);
            put16(out, c->target_unit);
            put16(out, c->unit_class);
            put16(out, c->attribute);
            aoe_stream_put_f32(out, c->amount);
        }
    }
}

void aoe_effects_release_ror(DatEffects *effects) {
    uint32_t i;
    for (i = 0; effects->effects && i < effects->count; ++i) {
        free(effects->effects[i].commands);
        effects->effects[i].commands = NULL;
    }
    free(effects->effects);
    effects->effects = NULL;
    effects->count = 0;
}

DatEffects *aoe_effects_destroy_ror(DatEffects *effects, int heap) {
    aoe_effects_release_ror(effects);
    if (heap) {
        free(effects);
        return NULL;
    }
    return effects;
}

void aoe_effects_tribe_release_ror(DatEffects *effects) {
    aoe_effects_release_ror(effects);
}

DatEffects *aoe_effects_tribe_destroy_ror(DatEffects *effects, int heap) {
    aoe_effects_tribe_release_ror(effects);
    if (heap) {
        free(effects);
        return NULL;
    }
    return effects;
}

void aoe_text_set_ror(char **field, const char *text) {
    size_t n = strlen(text) + 1;
    free(*field);
    *field = calloc(n, 1);
    if (*field) memcpy(*field, text, n);
}

void aoe_text_set_joined_ror(char **field, const char *a, const char *b) {
    size_t na = strlen(a), nb = strlen(b);
    char *t = calloc(na + nb + 1, 1);
    if (t) {
        memcpy(t, a, na);
        memcpy(t + na, b, nb);
    }
    free(*field);
    *field = t;
}
