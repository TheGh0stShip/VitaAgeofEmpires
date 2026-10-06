#include "unit_template.h"

#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

/* ---- stream helpers ---- */

static int16_t g16(AoeOutcomeStream *s) {
    const uint8_t *p = aoe_stream_take(s, 2);
    return p ? (int16_t)(p[0] | (p[1] << 8)) : 0;
}
static void p16(AoeOutcomeStream *s, int16_t v) {
    uint8_t b[2] = {(uint8_t)v, (uint8_t)((uint16_t)v >> 8)};
    aoe_stream_put(s, b, 2);
}
static uint8_t g8(AoeOutcomeStream *s) { return aoe_stream_u8(s); }
static float gf(AoeOutcomeStream *s) { return aoe_stream_f32(s); }
static int32_t g32(AoeOutcomeStream *s) { return aoe_stream_i32(s); }

static void *pick(void **table, int16_t i) { return (i < 0 || !table) ? NULL : table[i]; }

static char *dup_name(const char *name) {
    char *copy;
    size_t n;
    if (!name) return NULL;
    n = strlen(name);
    copy = malloc(n + 1);
    if (copy) memcpy(copy, name, n + 1);
    return copy;
}

static int16_t sprite_id(const AoeTemplateOpsRor *ops, const void *sprite) {
    return sprite ? ops->sprite_id(ops->context, sprite) : -1;
}
static int16_t sound_id(const AoeTemplateOpsRor *ops, const void *sound) {
    return sound ? ops->sound_id(ops->context, sound) : -1;
}

/* The largest attack / armor amounts (type 50 without saved values). */
static void type50_derive(AoeTemplateRor *t) {
    int16_t i;
    t->max_attack = (int16_t)t->field_100;
    for (i = 0; i < t->attack_count; ++i)
        if (t->max_attack < t->attacks[i].amount) t->max_attack = t->attacks[i].amount;
    t->max_armor = 0;
    for (i = 0; i < t->armor_count; ++i)
        if (t->max_armor < t->armors[i].amount) t->max_armor = t->armors[i].amount;
    t->field_140 = t->field_114;
    t->field_144 = t->field_120;
}

/* Type 70 without a saved +0x160: the largest attack of kind 3, and
   +0x13c the largest other attack (or +0x100). */
static void type70_derive(AoeTemplateRor *t) {
    int16_t i;
    t->pierce_armor = 0;
    for (i = 0; i < t->attack_count; ++i)
        if (t->attacks[i].kind == 3 && t->pierce_armor < t->attacks[i].amount)
            t->pierce_armor = t->attacks[i].amount;
    t->max_attack = (int16_t)t->field_100;
    for (i = 0; i < t->attack_count; ++i)
        if (t->attacks[i].kind != 3 && t->max_attack < t->attacks[i].amount)
            t->max_attack = t->attacks[i].amount;
}

static AoeTemplatePairRor *copy_pairs(const AoeTemplatePairRor *src, int16_t n) {
    AoeTemplatePairRor *p;
    if (n < 1 || !src) return NULL;
    p = calloc((size_t)n, sizeof *p);
    if (p) memcpy(p, src, (size_t)n * sizeof *p);
    return p;
}

/* Type 40's default task: the first with its flag set. */
static void default_task(AoeTemplateRor *t, const AoeTemplateOpsRor *ops) {
    int32_t i, n = t->tasks ? ops->task_count(ops->context, t->tasks) : 0;
    t->default_task = -1;
    for (i = 0; i < n; ++i)
        if (ops->task_flag(ops->context, t->tasks, i)) {
            t->default_task = (int16_t)i;
            return;
        }
}

/* ---- copy ---- */

int aoe_template_copy_ror(AoeTemplateRor *t, const AoeTemplateRor *src,
                          const AoeTemplateOpsRor *ops) {
    uint8_t type = src->type;
    int i;
    /* 10 (and 90) */
    *t = *src;
    t->name = dup_name(src->name);
    t->field_12 = src->field_10;
    t->damage = NULL;
    t->tasks = NULL;
    t->attacks = t->armors = NULL;
    if (src->damage_count) {
        t->damage = calloc(src->damage_count, sizeof *t->damage);
        if (!t->damage) return 0;
        for (i = 0; i < src->damage_count; ++i) t->damage[i] = src->damage[i];
    }
    t->type = 10;
    if (type == 10) return 1;
    if (type == 0x5a) {
        t->type = 0x5a;
        return 1;
    }
    t->type = 20; /* speed */
    if (type == 20) return 1;
    if (type == 25) {
        t->type = 0x19;
        t->field_65 = 3;
        return 1;
    }
    t->type = 0x1e; /* the walking fields came with the struct copy */
    if (type == 0x1e) return 1;
    t->type = 0x28;
    t->tasks = src->tasks ? ops->tasks_copy(ops->context, src->tasks) : NULL;
    if (type == 0x28) return 1;
    t->type = 0x32;
    t->armors = copy_pairs(src->armors, src->armor_count);
    t->attacks = copy_pairs(src->attacks, src->attack_count);
    if (type == 0x32) return 1;
    if (type == 0x3c) {
        t->type = 0x3c;
        return 1;
    }
    t->type = 0x46;
    if (type == 0x46) return 1;
    t->type = 0x50;
    return type == 0x50;
}

/* ---- binary ---- */

static int load10(AoeTemplateRor *t, AoeOutcomeStream *s, void **sprites, void **sounds,
                  float version) {
    int16_t name_len, g0, g1, g2, snd40, snd3c, snd44;
    int i;
    t->type = 10;
    name_len = g16(s);
    t->field_10 = g16(s);
    t->id = g16(s);
    t->field_e = g16(s);
    t->unit_class = g16(s);
    g0 = g16(s);
    g1 = g16(s);
    g2 = g16(s);
    t->field_24 = g8(s);
    t->hit_points = g16(s);
    t->line_of_sight = gf(s);
    t->field_2c = g8(s);
    for (i = 0; i < 3; ++i) t->size[i] = gf(s);
    snd40 = g16(s);
    t->dead_unit = g16(s);
    t->field_4a = g8(s);
    t->field_4b = g8(s);
    t->field_4c = g16(s);
    t->field_4e = g8(s);
    t->field_50 = g16(s);
    t->field_52 = g8(s);
    t->field_54 = g16(s);
    t->field_56 = g16(s);
    t->field_58 = g16(s);
    t->field_5a = g16(s);
    t->field_5c = gf(s);
    t->field_60 = gf(s);
    t->field_64 = g8(s);
    t->field_65 = g8(s);
    t->field_66 = g16(s);
    t->field_68 = g8(s);
    t->field_7c = g16(s);
    t->field_80 = gf(s);
    for (i = 0; i < 5; ++i) t->field_8b[i] = g8(s);
    t->field_84 = gf(s);
    t->field_90 = g8(s);
    t->field_98 = g32(s);
    t->field_9c = g32(s);
    t->field_a0 = g32(s);
    t->recycle = g8(s);
    t->field_a5 = g8(s);
    t->field_a6 = g8(s);
    t->field_a7 = g8(s);
    if (version < 7.04f) {
        t->field_a8 = t->field_a9 = 0;
        t->field_ac = t->size[0];
        t->field_b0 = t->size[1];
        t->field_b4 = t->size[2];
    } else {
        t->field_a8 = g8(s);
        t->field_a9 = g8(s);
        t->field_ac = gf(s);
        t->field_b0 = gf(s);
        t->field_b4 = gf(s);
    }
    for (i = 0; i < 3; ++i) {
        t->storage_type[i] = g16(s);
        t->storage_amount[i] = gf(s);
        t->storage_mode[i] = g8(s);
    }
    t->damage_count = g8(s);
    t->damage = NULL;
    if (t->damage_count) {
        t->damage = calloc(t->damage_count, sizeof *t->damage);
        if (!t->damage) return 0;
        for (i = 0; i < t->damage_count; ++i) {
            int16_t id = g16(s);
            uint8_t lo = g8(s), hi = g8(s);
            (void)hi; /* the two-byte read's high byte is overwritten */
            t->damage[i].percent = lo;
            t->damage[i].apply = g8(s);
            t->damage[i].sprite = pick(sprites, id);
        }
    }
    snd3c = g16(s);
    snd44 = g16(s);
    t->field_91 = g8(s);
    t->field_92 = g8(s);
    t->name = NULL;
    if (name_len > 0) {
        const uint8_t *p = aoe_stream_take(s, (size_t)name_len);
        t->name = calloc((size_t)name_len + 1, 1);
        if (!t->name) return 0;
        if (p) memcpy(t->name, p, (size_t)name_len);
    }
    if (version >= 7.12f) t->field_12 = g16(s);
    t->sound_3c = pick(sounds, snd3c);
    t->sound_44 = pick(sounds, snd44);
    t->sound_40 = pick(sounds, snd40);
    t->sprite_18 = pick(sprites, g0);
    t->sprite_1c = pick(sprites, g1);
    t->sprite_20 = pick(sprites, g2);
    return !s->failed;
}

int aoe_template_load_ror(AoeTemplateRor *t, uint8_t type, AoeOutcomeStream *s, void **sprites,
                          void **sounds, float version, const AoeTemplateOpsRor *ops) {
    memset(t, 0, sizeof *t);
    if (!load10(t, s, sprites, sounds, version)) return 0;
    if (type == 10) return 1;
    if (type == 0x5a) {
        t->type = 0x5a;
        return !s->failed;
    }
    t->type = 0x14;
    t->speed = gf(s);
    if (type == 0x14) return !s->failed;
    if (type == 0x19) {
        t->type = 0x19;
        return !s->failed;
    }
    {
        int16_t walk, run;
        t->type = 0x1e;
        walk = g16(s);
        run = g16(s);
        t->rotation = gf(s);
        t->field_c8 = g8(s);
        t->field_ca = g16(s);
        t->field_cc = g8(s);
        t->field_d0 = gf(s);
        t->field_d4 = g8(s);
        t->sprite_bc = pick(sprites, walk);
        t->sprite_c0 = pick(sprites, run);
    }
    if (type == 0x1e) return !s->failed;
    {
        int16_t a, b;
        t->type = 0x28;
        t->default_task = g16(s);
        t->search_radius = gf(s);
        t->work_rate = gf(s);
        t->drop_site[0] = g16(s);
        t->drop_site[1] = g16(s);
        t->field_ec = g8(s);
        a = g16(s);
        b = version < 6.92f ? a : g16(s);
        t->field_f8 = g8(s);
        t->sound_f0 = pick(sounds, a);
        t->sound_f4 = pick(sounds, b);
        t->tasks = ops->tasks_read(ops->context, s, sprites, sounds);
    }
    if (type == 0x28) return !s->failed;
    {
        int16_t i, sprite;
        t->type = 0x32;
        t->field_100 = g8(s);
        t->armor_count = g16(s);
        if (t->armor_count > 0) {
            t->armors = calloc((size_t)t->armor_count, sizeof *t->armors);
            if (!t->armors) return 0;
            for (i = 0; i < t->armor_count; ++i) {
                t->armors[i].kind = g16(s);
                t->armors[i].amount = g16(s);
            }
        }
        t->attack_count = g16(s);
        if (t->attack_count > 0) {
            t->attacks = calloc((size_t)t->attack_count, sizeof *t->attacks);
            if (!t->attacks) return 0;
            for (i = 0; i < t->attack_count; ++i) {
                t->attacks[i].kind = g16(s);
                t->attacks[i].amount = g16(s);
            }
        }
        t->field_110 = g16(s);
        t->field_114 = gf(s);
        t->field_118 = gf(s);
        t->field_120 = gf(s);
        t->field_124 = g16(s);
        t->field_126 = g16(s);
        t->field_128 = g8(s);
        t->field_12a = g16(s);
        for (i = 0; i < 3; ++i) t->field_12c[i] = gf(s);
        t->field_11c = g8(s);
        t->field_138 = gf(s);
        sprite = g16(s);
        t->sprite_fc = pick(sprites, sprite);
        if (version >= 7.01f) {
            t->max_attack = g16(s);
            t->max_armor = g16(s);
            t->field_140 = gf(s);
            t->field_144 = gf(s);
        } else {
            type50_derive(t);
        }
    }
    if (type == 0x32) return !s->failed;
    if (type == 0x3c) {
        int i;
        t->type = 0x3c;
        for (i = 0; i < 5; ++i) t->field_148[i] = g8(s);
        t->field_150 = gf(s);
        return !s->failed;
    }
    {
        int i;
        t->type = 0x46;
        for (i = 0; i < 3; ++i) {
            t->costs[i].type = g16(s);
            t->costs[i].amount = g16(s);
            t->costs[i].flag = g8(s);
            t->costs[i].pad = g8(s);
        }
        t->train_time = g16(s);
        t->train_location = g16(s);
        t->button = g8(s);
        if (version >= 7.01f)
            t->pierce_armor = g16(s);
        else
            type70_derive(t);
    }
    if (type == 0x46) return !s->failed;
    {
        int16_t sprite, sound;
        t->type = 0x50;
        sprite = g16(s);
        t->field_16c = g8(s);
        t->field_16e = g16(s);
        t->field_170 = g8(s);
        t->field_172 = g16(s);
        t->field_174 = g16(s);
        t->field_176 = g16(s);
        t->field_178 = g16(s);
        sound = g16(s);
        t->sprite_168 = pick(sprites, sprite);
        t->sound_164 = pick(sounds, sound);
    }
    return type == 0x50 && !s->failed;
}

/* ---- text ---- */

static int text10(AoeTemplateRor *t, FILE *f, void **sprites, void **sounds, int16_t index) {
    char name[0x78];
    int16_t g0 = -1, g1 = -1, g2 = -1, b24 = 0, b2c = 0, b4a = 0, b4b = 0, b4e = 0, b52 = 0,
            b64 = 0, b65 = 0, b66 = 0, snd40 = -1, b68 = 0, b8b = 0, b8c = 0, b8d = 0, b8e = 0,
            b8f = 0, b90 = 0, ba5 = 0, ba6 = 0, ba7 = 0, ba8 = 0, ba9 = 0, count = 0;
    int16_t snd3c = -1, snd44 = -1, b91 = 0, b92 = 0;
    int i;
    t->type = 10;
    t->field_10 = index;
    name[0] = '\0';
    if (fscanf(f,
               "%119s %hd %hd %hd %hd %hd %hd %hd %hd %f %hd %f %f %f %hd %hd %hd %hd %hd %hd %hd "
               "%hd %hd %hd %hd %f %f %hd %hd %hd %hd %hd %hd %f %f %hd %hd %hd %hd %hd %hd %d %d "
               "%d %hd %hd %hd %hd %hd %f %f %f",
               name, &t->id, &t->field_e, &t->unit_class, &g0, &g1, &g2, &b24, &t->hit_points,
               &t->line_of_sight, &b2c, &t->size[0], &t->size[1], &t->size[2], &t->dead_unit,
               &b4a, &b4b, &t->field_4c, &b4e, &t->field_50, &b52, &t->field_54, &t->field_56,
               &t->field_58, &t->field_5a, &t->field_5c, &t->field_60, &b64, &b65, &b66, &snd40,
               &b68, &t->field_7c, &t->field_84, &t->field_80, &b8b, &b8c, &b8d, &b8e, &b8f,
               &b90, &t->field_98, &t->field_9c, &t->field_a0, &ba5, &ba6, &ba7, &ba8, &ba9,
               &t->field_ac, &t->field_b0, &t->field_b4) == EOF) {
        /* the original goes on with what it has */
    }
    for (i = 0; i < 3; ++i) {
        int16_t mode = 0;
        if (fscanf(f, "%hd %f %hd", &t->storage_type[i], &t->storage_amount[i], &mode) == EOF)
            mode = 0;
        t->storage_mode[i] = (uint8_t)mode;
    }
    if (fscanf(f, "%hd", &count) != 1) count = 0;
    t->damage_count = (uint8_t)count;
    t->damage = NULL;
    if (t->damage_count) {
        t->damage = calloc(t->damage_count, sizeof *t->damage);
        if (!t->damage) return 0;
        for (i = 0; i < t->damage_count; ++i) {
            int16_t id = -1, pc = 0, ap = 0;
            if (fscanf(f, "%hd %hd %hd", &id, &pc, &ap) == EOF) id = -1;
            t->damage[i].sprite = pick(sprites, id);
            t->damage[i].percent = (uint8_t)pc;
            t->damage[i].apply = (uint8_t)ap;
        }
    }
    if (fscanf(f, "%hd %hd %hd %hd", &snd3c, &snd44, &b91, &b92) == EOF) snd3c = snd44 = -1;
    t->sound_3c = pick(sounds, snd3c);
    t->sound_44 = pick(sounds, snd44);
    t->sound_40 = pick(sounds, snd40);
    t->sprite_18 = pick(sprites, g0);
    t->sprite_1c = pick(sprites, g1);
    t->sprite_20 = pick(sprites, g2);
    t->field_64 = (uint8_t)b64;
    t->field_4a = (uint8_t)b4a;
    t->field_68 = (uint8_t)b68;
    t->field_65 = (uint8_t)b65;
    t->field_8b[2] = (uint8_t)b8d;
    t->field_2c = (uint8_t)b2c;
    t->field_4b = (uint8_t)b4b;
    t->field_52 = (uint8_t)b52;
    t->field_8b[0] = (uint8_t)b8b;
    t->field_66 = (int16_t)(uint8_t)b66;
    t->field_8b[1] = (uint8_t)b8c;
    t->field_8b[3] = (uint8_t)b8e;
    t->field_91 = (uint8_t)b91;
    t->field_8b[4] = (uint8_t)b8f;
    t->field_24 = (uint8_t)b24;
    t->field_a5 = (uint8_t)ba5;
    t->field_4e = (uint8_t)b4e;
    t->field_92 = (uint8_t)b92;
    t->field_a8 = (uint8_t)ba8;
    t->field_90 = (uint8_t)b90;
    t->field_a6 = (uint8_t)ba6;
    t->field_a7 = (uint8_t)ba7;
    t->field_a9 = (uint8_t)ba9;
    if (t->field_ac == 0.0f) t->field_ac = t->size[0];
    if (t->field_b0 == 0.0f) t->field_b0 = t->size[1];
    if (t->field_b4 == 0.0f) t->field_b4 = t->size[2];
    t->recycle = 0;
    t->field_12 = t->field_10;
    if (t->unit_class == 0xb) t->recycle = 1;
    if (name[0]) t->name = dup_name(name);
    return 1;
}

int aoe_template_text_ror(AoeTemplateRor *t, uint8_t type, FILE *f, void **sprites, void **sounds,
                          int16_t index, const AoeTemplateOpsRor *ops) {
    memset(t, 0, sizeof *t);
    if (!text10(t, f, sprites, sounds, index)) return 0;
    if (type == 10) return 1;
    if (type == 0x5a) {
        t->type = 0x5a;
        return 1;
    }
    t->type = 0x14;
    if (fscanf(f, "%f", &t->speed) != 1) t->speed = 0.0f;
    if (type == 0x14) return 1;
    if (type == 0x19) {
        t->type = 0x19;
        t->recycle = 1;
        return 1;
    }
    {
        int16_t walk = -1, run = -1, c8 = 0, cc = 0, d4 = 0;
        t->type = 0x1e;
        if (fscanf(f, "%hd %hd %f %hd %hd %hd %f %hd", &walk, &run, &t->rotation, &c8,
                   &t->field_ca, &cc, &t->field_d0, &d4) == EOF)
            walk = run = -1;
        t->field_cc = (uint8_t)cc;
        t->field_c8 = (uint8_t)c8;
        t->field_d4 = (uint8_t)d4;
        t->sprite_bc = pick(sprites, walk);
        t->sprite_c0 = pick(sprites, run);
    }
    if (type == 0x1e) return 1;
    {
        int16_t ec = 0, a = -1, b = -1, f8 = 0;
        t->type = 0x28;
        if (fscanf(f, "%f %f %hd %hd %hd %hd %hd %hd", &t->search_radius, &t->work_rate,
                   &t->drop_site[0], &t->drop_site[1], &ec, &a, &b, &f8) == EOF)
            a = b = -1;
        t->field_ec = (uint8_t)ec;
        t->field_f8 = (uint8_t)f8;
        t->sound_f0 = pick(sounds, a);
        t->sound_f4 = pick(sounds, b);
        t->tasks = ops->tasks_text(ops->context, f, sprites, sounds);
        default_task(t, ops);
    }
    if (type == 0x28) return 1;
    {
        int16_t sprite = -1, b100 = 0, b128 = 0, b12a = 0, b11c = 0, i;
        t->type = 0x32;
        if (fscanf(f, "%hd %hd %hd %hd %hd %f %f %f %hd %hd %hd %hd %f %f %f %hd %f", &sprite,
                   &b100, &t->attack_count, &t->armor_count, &t->field_110, &t->field_114,
                   &t->field_118, &t->field_120, &t->field_124, &t->field_126, &b128, &b12a,
                   &t->field_12c[0], &t->field_12c[1], &t->field_12c[2], &b11c,
                   &t->field_138) == EOF)
            sprite = -1;
        if (t->attack_count > 0) {
            t->attacks = calloc((size_t)t->attack_count, sizeof *t->attacks);
            if (!t->attacks) return 0;
            for (i = 0; i < t->attack_count; ++i)
                if (fscanf(f, "%hd %hd", &t->attacks[i].kind, &t->attacks[i].amount) == EOF) break;
        }
        if (t->armor_count > 0) {
            t->armors = calloc((size_t)t->armor_count, sizeof *t->armors);
            if (!t->armors) return 0;
            for (i = 0; i < t->armor_count; ++i)
                if (fscanf(f, "%hd %hd", &t->armors[i].kind, &t->armors[i].amount) == EOF) break;
        }
        t->sprite_fc = pick(sprites, sprite);
        t->field_128 = (uint8_t)b128;
        t->field_11c = (uint8_t)b11c;
        t->field_100 = (uint8_t)b100;
        t->field_12a = (int16_t)(uint8_t)b12a;
        type50_derive(t);
    }
    if (type == 0x32) return 1;
    if (type == 0x3c) {
        int16_t v[5] = {0, 0, 0, 0, 0};
        int i;
        t->type = 0x3c;
        t->recycle = 1;
        if (fscanf(f, "%hd %hd %hd %hd %hd %f", &v[0], &v[1], &v[2], &v[3], &v[4],
                   &t->field_150) == EOF) {
        }
        for (i = 0; i < 5; ++i) t->field_148[i] = (uint8_t)v[i];
        return 1;
    }
    {
        int i;
        int16_t button = 0;
        t->type = 0x46;
        for (i = 0; i < 3; ++i) {
            int16_t flag = 0;
            if (fscanf(f, "%hd %hd %hd", &t->costs[i].type, &t->costs[i].amount, &flag) == EOF)
                flag = 0;
            t->costs[i].flag = (uint8_t)flag;
        }
        if (fscanf(f, "%hd %hd %hd", &t->train_time, &t->train_location, &button) == EOF)
            button = 0;
        t->button = (uint8_t)button;
        type70_derive(t);
    }
    if (type == 0x46) return 1;
    {
        int16_t sprite = -1, b16c = 0, b170 = 0, sound = -1;
        t->type = 0x50;
        if (fscanf(f, "%hd %hd %hd %hd %hd %hd %hd %hd %hd", &sprite, &b16c, &t->field_16e, &b170,
                   &t->field_172, &t->field_174, &t->field_176, &t->field_178, &sound) == EOF)
            sprite = sound = -1;
        t->field_16c = (uint8_t)b16c;
        t->field_170 = (uint8_t)b170;
        t->sprite_168 = pick(sprites, sprite);
        t->sound_164 = pick(sounds, sound);
    }
    return type == 0x50;
}

/* ---- save ---- */

void aoe_template_save_ror(const AoeTemplateRor *t, AoeOutcomeStream *s,
                           const AoeTemplateOpsRor *ops) {
    int16_t name_len = 0;
    int i;
    /* 10 */
    if (t->name && t->name[0]) name_len = (int16_t)(strlen(t->name) + 1);
    aoe_stream_put_u8(s, t->type);
    p16(s, name_len);
    p16(s, t->field_10);
    p16(s, t->id);
    p16(s, t->field_e);
    p16(s, t->unit_class);
    p16(s, sprite_id(ops, t->sprite_18));
    p16(s, sprite_id(ops, t->sprite_1c));
    p16(s, sprite_id(ops, t->sprite_20));
    aoe_stream_put_u8(s, t->field_24);
    p16(s, t->hit_points);
    aoe_stream_put_f32(s, t->line_of_sight);
    aoe_stream_put_u8(s, t->field_2c);
    for (i = 0; i < 3; ++i) aoe_stream_put_f32(s, t->size[i]);
    p16(s, sound_id(ops, t->sound_40));
    p16(s, t->dead_unit);
    aoe_stream_put_u8(s, t->field_4a);
    aoe_stream_put_u8(s, t->field_4b);
    p16(s, t->field_4c);
    aoe_stream_put_u8(s, t->field_4e);
    p16(s, t->field_50);
    aoe_stream_put_u8(s, t->field_52);
    p16(s, t->field_54);
    p16(s, t->field_56);
    p16(s, t->field_58);
    p16(s, t->field_5a);
    aoe_stream_put_f32(s, t->field_5c);
    aoe_stream_put_f32(s, t->field_60);
    aoe_stream_put_u8(s, t->field_64);
    aoe_stream_put_u8(s, t->field_65);
    p16(s, t->field_66);
    aoe_stream_put_u8(s, t->field_68);
    p16(s, t->field_7c);
    aoe_stream_put_f32(s, t->field_80);
    for (i = 0; i < 5; ++i) aoe_stream_put_u8(s, t->field_8b[i]);
    aoe_stream_put_f32(s, t->field_84);
    aoe_stream_put_u8(s, t->field_90);
    aoe_stream_put_i32(s, t->field_98);
    aoe_stream_put_i32(s, t->field_9c);
    aoe_stream_put_i32(s, t->field_a0);
    aoe_stream_put_u8(s, t->recycle);
    aoe_stream_put_u8(s, t->field_a5);
    aoe_stream_put_u8(s, t->field_a6);
    aoe_stream_put_u8(s, t->field_a7);
    aoe_stream_put_u8(s, t->field_a8);
    aoe_stream_put_u8(s, t->field_a9);
    aoe_stream_put_f32(s, t->field_ac);
    aoe_stream_put_f32(s, t->field_b0);
    aoe_stream_put_f32(s, t->field_b4);
    for (i = 0; i < 3; ++i) {
        p16(s, t->storage_type[i]);
        aoe_stream_put_f32(s, t->storage_amount[i]);
        aoe_stream_put_u8(s, t->storage_mode[i]);
    }
    aoe_stream_put_u8(s, t->damage_count);
    for (i = 0; i < t->damage_count; ++i) {
        p16(s, sprite_id(ops, t->damage[i].sprite));
        aoe_stream_put_u8(s, t->damage[i].percent);
        aoe_stream_put_u8(s, t->damage[i].apply);
        aoe_stream_put_u8(s, t->damage[i].apply);
    }
    p16(s, sound_id(ops, t->sound_3c));
    p16(s, sound_id(ops, t->sound_44));
    aoe_stream_put_u8(s, t->field_91);
    aoe_stream_put_u8(s, t->field_92);
    if (name_len) aoe_stream_put(s, t->name, (size_t)name_len);
    p16(s, t->field_12);
    if (t->type == 10 || t->type == 0x5a) return;
    /* 20 (and 25) */
    aoe_stream_put_f32(s, t->speed);
    if (t->type == 0x14 || t->type == 0x19) return;
    /* 30 */
    p16(s, sprite_id(ops, t->sprite_bc));
    p16(s, sprite_id(ops, t->sprite_c0));
    aoe_stream_put_f32(s, t->rotation);
    aoe_stream_put_u8(s, t->field_c8);
    p16(s, t->field_ca);
    aoe_stream_put_u8(s, t->field_cc);
    aoe_stream_put_f32(s, t->field_d0);
    aoe_stream_put_u8(s, t->field_d4);
    if (t->type == 0x1e) return;
    /* 40 */
    p16(s, t->default_task);
    aoe_stream_put_f32(s, t->search_radius);
    aoe_stream_put_f32(s, t->work_rate);
    p16(s, t->drop_site[0]);
    p16(s, t->drop_site[1]);
    aoe_stream_put_u8(s, t->field_ec);
    p16(s, sound_id(ops, t->sound_f0));
    p16(s, sound_id(ops, t->sound_f4));
    aoe_stream_put_u8(s, t->field_f8);
    ops->tasks_write(ops->context, t->tasks, s);
    if (t->type == 0x28) return;
    /* 50 */
    aoe_stream_put_u8(s, t->field_100);
    p16(s, t->armor_count);
    for (i = 0; t->armors && i < t->armor_count; ++i) {
        p16(s, t->armors[i].kind);
        p16(s, t->armors[i].amount);
    }
    p16(s, t->attack_count);
    for (i = 0; t->attacks && i < t->attack_count; ++i) {
        p16(s, t->attacks[i].kind);
        p16(s, t->attacks[i].amount);
    }
    p16(s, t->field_110);
    aoe_stream_put_f32(s, t->field_114);
    aoe_stream_put_f32(s, t->field_118);
    aoe_stream_put_f32(s, t->field_120);
    p16(s, t->field_124);
    p16(s, t->field_126);
    aoe_stream_put_u8(s, t->field_128);
    p16(s, t->field_12a);
    for (i = 0; i < 3; ++i) aoe_stream_put_f32(s, t->field_12c[i]);
    aoe_stream_put_u8(s, t->field_11c);
    aoe_stream_put_f32(s, t->field_138);
    p16(s, sprite_id(ops, t->sprite_fc));
    p16(s, t->max_attack);
    p16(s, t->max_armor);
    aoe_stream_put_f32(s, t->field_140);
    aoe_stream_put_f32(s, t->field_144);
    if (t->type == 0x32) return;
    if (t->type == 0x3c) {
        /* 60 */
        for (i = 0; i < 5; ++i) aoe_stream_put_u8(s, t->field_148[i]);
        aoe_stream_put_f32(s, t->field_150);
        return;
    }
    /* 70 */
    for (i = 0; i < 3; ++i) {
        p16(s, t->costs[i].type);
        p16(s, t->costs[i].amount);
        aoe_stream_put_u8(s, t->costs[i].flag);
        aoe_stream_put_u8(s, t->costs[i].pad);
    }
    p16(s, t->train_time);
    p16(s, t->train_location);
    aoe_stream_put_u8(s, t->button);
    p16(s, t->pierce_armor);
    if (t->type == 0x46) return;
    /* 80 */
    p16(s, sprite_id(ops, t->sprite_168));
    aoe_stream_put_u8(s, t->field_16c);
    p16(s, t->field_16e);
    aoe_stream_put_u8(s, t->field_170);
    p16(s, t->field_172);
    p16(s, t->field_174);
    p16(s, t->field_176);
    p16(s, t->field_178);
    p16(s, sound_id(ops, t->sound_164));
}

void aoe_template_release_ror(AoeTemplateRor *t, const AoeTemplateOpsRor *ops) {
    free(t->armors);
    free(t->attacks);
    t->armors = t->attacks = NULL;
    if (t->tasks) ops->tasks_free(ops->context, t->tasks);
    t->tasks = NULL;
    free(t->name);
    t->name = NULL;
    free(t->damage);
    t->damage = NULL;
}

AoeTemplateRor *aoe_template_clone_ror(const AoeTemplateRor *t, const AoeTemplateOpsRor *ops) {
    AoeTemplateRor *c = calloc(1, sizeof *c);
    if (!c) return NULL;
    if (!aoe_template_copy_ror(c, t, ops)) {
        aoe_template_release_ror(c, ops);
        free(c);
        return NULL;
    }
    return c;
}

void *aoe_template_create_ror(AoeTemplateRor *t, void *player, float x, float y, float z,
                              const AoeTemplateOpsRor *ops) {
    if (t->recycle) {
        void *old = ops->recycled(ops->context, t->type);
        if (old) return ops->reinit(ops->context, old, t, player, x, y, z);
    }
    return ops->instance_new(ops->context, t, player, x, y, z);
}

int aoe_template_set_attribute_ror(AoeTemplateRor *t, uint8_t attribute, float v, int op) {
    float *field = NULL;
    /* Type 60's add and multiply set +0x149 for attribute 19, then the
       parent's run. */
    if (t->type == 0x3c && attribute == 0x13 && op != 0) {
        t->field_148[1] = (uint8_t)(int32_t)v;
        return 0;
    }
    /* Type 90 is a type 10. */
    if (t->type == 0x5a) return 0;
    if (op != 0 && attribute == 0xd && t->type >= 0x28)
        field = &t->work_rate;
    else if (op != 0 && attribute == 6 && t->type >= 0x1e)
        field = &t->rotation;
    else if (attribute == 5 && t->type >= 0x14)
        field = &t->speed;
    if (!field) return 0;
    if (op == 0)
        *field = v;
    else if (op == 1)
        *field += v;
    else
        *field *= v;
    return 1;
}

const void *aoe_template_find_task_ror(const AoeTemplateRor *t, int32_t id,
                                       int16_t (*task_id)(void *context, const void *tasks,
                                                          int32_t i),
                                       const void *(*task_at)(void *context, const void *tasks,
                                                              int32_t i),
                                       const AoeTemplateOpsRor *ops) {
    int32_t i, n;
    if (id == -1 || !t->tasks) return NULL;
    n = ops->task_count(ops->context, t->tasks);
    for (i = 0; i < n; ++i)
        if (task_id(ops->context, t->tasks, i) == id) return task_at(ops->context, t->tasks, i);
    return NULL;
}

void aoe_template_play_sound_ror(const AoeTemplateRor *t, int second,
                                 void (*play)(void *context, void *sound), void *context) {
    void *sound = second ? t->sound_f4 : t->sound_f0;
    if (sound) play(context, sound);
}

void aoe_template_outline_draw_ror(const AoeTemplateOutlineOpsRor *o, int has_graphic, int tribe,
                                   int frame_16c_clear, int16_t frame_16e, int32_t frame,
                                   int32_t sx, int32_t sy, int32_t arg5, int32_t arg2,
                                   int outline, float size_x, float size_y, uint8_t color) {
    int16_t c[8];
    if (!o || !has_graphic) return;
    if (tribe) {
        int32_t f = frame_16c_clear ? frame_16e : (frame == 0 ? 2 : frame - 1);
        o->plan(o->context, f, sx, sy, arg5, arg2, 0);
    } else {
        o->plan(o->context, 0, sx, sy, arg5, arg2, 10);
    }
    if (!outline) return;
    o->project(o->context, size_x, -size_y, sx, sy, &c[0], &c[1]);
    o->project(o->context, size_x, size_y, sx, sy, &c[2], &c[3]);
    o->project(o->context, -size_x, size_y, sx, sy, &c[4], &c[5]);
    o->project(o->context, -size_x, -size_y, sx, sy, &c[6], &c[7]);
    o->line_node(o->context, c, color);
}
