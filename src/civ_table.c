#include "civ_table.h"

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

int aoe_civ_from_text_ror(AoeCivRor *c, FILE *f, uint8_t ror) {
    int16_t listed = 0, i, index = 0, b = 0;
    float v = 0.0f;
    memset(c, 0, sizeof *c);
    c->ror = ror;
    if (fscanf(f, "%20s %hd %hd", c->name, &c->field_2a, &c->value_count) != 3) c->value_count = 0;
    if (c->value_count > 0) {
        c->values = calloc((size_t)c->value_count, sizeof *c->values);
        if (!c->values) return 0;
    }
    if (fscanf(f, "%hd", &listed) != 1) listed = 0;
    for (i = 0; i < listed; ++i) {
        if (fscanf(f, "%hd %f", &index, &v) != 2) break;
        if (index >= 0 && index < c->value_count) c->values[index] = v; /* host guard */
    }
    if (fscanf(f, "%hd", &b) == 1) c->field_28 = (uint8_t)b;
    return 1;
}

int aoe_civ_read_ror(AoeCivRor *c, AoeOutcomeStream *s, uint8_t ror) {
    const uint8_t *name;
    int16_t i;
    memset(c, 0, sizeof *c);
    c->ror = ror;
    name = aoe_stream_take(s, 20);
    if (name) memcpy(c->name, name, 20);
    c->value_count = g16(s);
    c->field_2a = g16(s);
    if (c->value_count > 0) {
        c->values = calloc((size_t)c->value_count, sizeof *c->values);
        if (!c->values) return 0;
        for (i = 0; i < c->value_count; ++i) c->values[i] = aoe_stream_f32(s);
    }
    c->field_28 = aoe_stream_u8(s);
    return !s->failed;
}

int aoe_civ_set_template_count_ror(AoeCivRor *c, int16_t count) {
    c->template_count = count;
    if (count > 0) {
        c->templates = calloc((size_t)count, sizeof *c->templates);
        if (!c->templates) return 0;
    }
    return 1;
}

static int known_type(const AoeCivRor *c, uint8_t type) {
    if (c->ror && (type == 70 || type == 80 || type == 90)) return 1;
    return type == 10 || type == 20 || type == 25 || type == 30 || type == 40 || type == 50 ||
           type == 60;
}

int aoe_civ_read_template_ror(AoeCivRor *c, AoeOutcomeStream *s, uint8_t type, void **sprites,
                              void **sounds, int16_t slot, float version,
                              const AoeTemplateOpsRor *ops) {
    AoeTemplateRor *t;
    if (!known_type(c, type)) return 1;
    t = calloc(1, sizeof *t);
    if (t && !aoe_template_load_ror(t, type, s, sprites, sounds, version, ops)) {
        aoe_template_release_ror(t, ops);
        free(t);
        t = NULL;
    }
    c->templates[slot] = t;
    return t != NULL;
}

int aoe_civ_read_templates_ror(AoeCivRor *c, AoeOutcomeStream *s, void **sprites, void **sounds,
                               float version, const AoeTemplateOpsRor *ops) {
    int16_t i, n = g16(s);
    uint8_t *present;
    int ok = 1;
    c->template_count = n;
    c->templates = NULL;
    if (n < 1) return !s->failed;
    c->templates = calloc((size_t)n, sizeof *c->templates);
    present = calloc((size_t)n, 1);
    if (!c->templates || !present) {
        free(present);
        return 0;
    }
    for (i = 0; i < n; ++i) present[i] = aoe_stream_i32(s) != 0;
    for (i = 0; i < n && !s->failed; ++i)
        if (present[i]) {
            uint8_t type = aoe_stream_u8(s);
            if (!aoe_civ_read_template_ror(c, s, type, sprites, sounds, i, version, ops) &&
                known_type(c, type))
                ok = 0;
        }
    free(present);
    return ok && !s->failed;
}

int aoe_civ_text_template_ror(AoeCivRor *c, FILE *f, uint8_t type, void **sprites, void **sounds,
                              int16_t slot, const AoeTemplateOpsRor *ops) {
    AoeTemplateRor *t;
    if (!known_type(c, type)) return 1;
    t = calloc(1, sizeof *t);
    if (t && !aoe_template_text_ror(t, type, f, sprites, sounds, slot, ops)) {
        aoe_template_release_ror(t, ops);
        free(t);
        t = NULL;
    }
    c->templates[slot] = t;
    return t != NULL;
}

void aoe_civ_save_ror(const AoeCivRor *c, AoeOutcomeStream *s, const AoeTemplateOpsRor *ops) {
    char name[20];
    int16_t i;
    aoe_stream_put_u8(s, c->ror);
    memset(name, 0, sizeof name);
    memcpy(name, c->name, strnlen(c->name, sizeof name));
    aoe_stream_put(s, name, sizeof name);
    p16(s, c->value_count);
    p16(s, c->field_2a);
    for (i = 0; i < c->value_count; ++i) aoe_stream_put_f32(s, c->values[i]);
    aoe_stream_put_u8(s, c->field_28);
    p16(s, c->template_count);
    if (c->template_count <= 0) return;
    for (i = 0; i < c->template_count; ++i) aoe_stream_put_i32(s, c->templates[i] ? 1 : 0);
    for (i = 0; i < c->template_count; ++i)
        if (c->templates[i]) aoe_template_save_ror(c->templates[i], s, ops);
}

void aoe_civ_release_ror(AoeCivRor *c, const AoeTemplateOpsRor *ops) {
    int16_t i;
    if (c->value_count > 0 && c->values) {
        free(c->values);
        c->values = NULL;
        c->value_count = 0;
    }
    if (c->template_count > 0 && c->templates) {
        for (i = 0; i < c->template_count; ++i)
            if (c->templates[i]) {
                aoe_template_release_ror(c->templates[i], ops);
                free(c->templates[i]);
                c->templates[i] = NULL;
            }
        free(c->templates);
        c->templates = NULL;
        c->template_count = 0;
    }
}
