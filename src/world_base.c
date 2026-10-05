#include "world_base.h"

#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

#define OPS (w->ops)
#define CTX (w->ops->context)

static uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static void put_le16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

void **aoe_world_object_table_ror;
int32_t aoe_world_counter_ror;

/* ---- colour tables ---- */

void aoe_world_color_from_text_ror(AoeWorldColorRor *c, FILE *f, int16_t palette) {
    int16_t a = palette, b = 0;
    char name[0x100];
    memset(c, 0, sizeof *c);
    c->palette = palette;
    name[0] = '\0';
    if (fscanf(f, "%hd %255s %hd %hd", &c->id, name, &a, &b) < 2) name[0] = '\0';
    c->field_25 = (uint8_t)a;
    c->field_24 = (uint8_t)b;
    /* The name joined with ".col" (0x00450e20) replaces it; the original
       copies without a bound. */
    snprintf(c->name, sizeof c->name, "%.25s.col", name);
}

void aoe_world_color_read_ror(AoeWorldColorRor *c, AoeOutcomeStream *s,
                              FILE *(*open)(void *context, const char *path), void *context) {
    const uint8_t *p = aoe_stream_take(s, 30);
    char path[300];
    FILE *f;
    int i;
    memset(c, 0, sizeof *c);
    if (p) memcpy(c->name, p, 30);
    c->name[29] = '\0';
    p = aoe_stream_take(s, 2);
    if (p) c->palette = (int16_t)le16(p);
    p = aoe_stream_take(s, 2);
    if (p) c->id = (int16_t)le16(p);
    c->field_25 = aoe_stream_u8(s);
    c->field_24 = aoe_stream_u8(s);
    snprintf(path, sizeof path, "data\\%s", c->name);
    f = open(context, path);
    if (!f) return;
    for (i = 0; i < 0x100; ++i) {
        int16_t v = 0;
        if (fscanf(f, "%hd", &v) == 1) c->map[i] = (uint8_t)v;
    }
    fclose(f);
}

void aoe_world_color_write_ror(const AoeWorldColorRor *c, AoeOutcomeStream *s) {
    uint8_t b[2];
    aoe_stream_put(s, c->name, 30);
    put_le16(b, (uint16_t)c->palette);
    aoe_stream_put(s, b, 2);
    put_le16(b, (uint16_t)c->id);
    aoe_stream_put(s, b, 2);
    aoe_stream_put_u8(s, c->field_25);
    aoe_stream_put_u8(s, c->field_24);
}

/* ---- lifetime ---- */

static void *zeroed(size_t n, size_t size) { return calloc(n ? n : 1, size); }

int aoe_world_init_ror(AoeWorldBaseRor *w, const AoeWorldOpsRor *ops) {
    int i;
    memset(w, 0, sizeof *w);
    w->ops = ops;
    w->vtbl = &aoe_world_base_vtable_ror;
    w->objects_cap = 10000;
    w->objects2_cap = 1000;
    w->field_f4 = w->field_f8 = 2;
    w->field_fc = w->field_100 = -1;
    aoe_world_counter_ror = 0;
    w->objects = zeroed((size_t)w->objects_cap, sizeof *w->objects);
    w->objects2 = zeroed((size_t)w->objects2_cap, sizeof *w->objects2);
    aoe_world_object_table_ror = w->objects;
    w->field_6c = -1;
    w->seed = 0xe;
    w->field_7c = 1;
    w->state = 2;
    for (i = 0; i < 7; ++i) w->lists[i] = ops->list_new(ops->context);
    w->state = 0;
    return w->objects && w->objects2;
}

void aoe_world_release_ror(AoeWorldBaseRor *w, FILE **ai_log) {
    int i;
    if (w->field_58) OPS->free_object(CTX, w->field_58);
    aoe_world_reset_ror(w);
    for (i = 0; i < 7; ++i) {
        if (w->lists[i]) OPS->list_free(CTX, w->lists[i]);
        w->lists[i] = NULL;
    }
    if (w->civs) {
        for (i = 0; i < w->civ_count; ++i) {
            if (w->civs[i]) OPS->civ_free(CTX, w->civs[i]);
            w->civs[i] = NULL;
        }
        free(w->civs);
        w->civs = NULL;
    }
    if (w->effects) OPS->effects_free(CTX, w->effects);
    w->effects = NULL;
    if (w->map) OPS->map_free(CTX, w->map);
    w->map = NULL;
    if (w->sprites) {
        for (i = 0; i < w->sprite_count; ++i)
            if (w->sprites[i]) OPS->sprite_free(CTX, w->sprites[i]);
        free(w->sprites);
        w->sprites = NULL;
    }
    if (w->sounds) {
        for (i = 0; i < w->sound_count; ++i)
            if (w->sounds[i]) OPS->sound_free(CTX, w->sounds[i]);
        free(w->sounds);
        w->sounds = NULL;
    }
    if (w->table) {
        for (i = 0; i < w->table_rows; ++i) free(w->table[i]);
        free(w->table);
        w->table = NULL;
    }
    if (w->colors) {
        for (i = 0; i < w->color_count; ++i) free(w->colors[i]);
        free(w->colors);
        w->colors = NULL;
    }
    if (w->objects) {
        free(w->objects);
        w->objects = NULL;
        aoe_world_object_table_ror = NULL;
    }
    free(w->objects2);
    w->objects2 = NULL;
    if (ai_log && *ai_log) {
        fclose(*ai_log);
        *ai_log = NULL;
    }
    if (w->field_a0) OPS->free_object(CTX, w->field_a0);
    w->field_a0 = NULL;
}

void aoe_world_reset_ror(AoeWorldBaseRor *w) {
    int i;
    aoe_world_counter_ror = 0;
    w->state = 2;
    w->field_1e = 0;
    for (i = 0; i < 7; ++i) OPS->list_clear(CTX, w->lists[i]);
    w->field_6c = -1;
    w->campaign = w->campaign_player = w->campaign_scenario = -1;
    w->field_100 = -1;
    w->field_68 = 0;
    for (i = 0; i < w->objects_cap; ++i) w->objects[i] = NULL;
    for (i = 0; i < w->objects2_cap; ++i) w->objects2[i] = NULL;
    w->vtbl->release_sounds(w);
    if (w->scenario) {
        OPS->scenario_free(CTX, w->scenario);
        w->scenario = NULL;
    }
    if (w->player_count > 0 && w->players) {
        for (i = 0; i < w->player_count; ++i)
            if (w->players[i]) OPS->player_units_68(CTX, w->players[i]);
        for (i = 0; i < w->player_count; ++i) {
            if (w->players[i]) OPS->player_free(CTX, w->players[i]);
            w->players[i] = NULL;
        }
        w->player_count = 0;
        free(w->players);
        w->players = NULL;
    }
    w->field_68 = 0;
    w->sound_index = 0;
    w->sprite_index = 0;
    w->update_slot = 0;
    memset(w->update_times, 0, sizeof w->update_times);
}

void aoe_world_clear_times_ror(AoeWorldBaseRor *w) {
    w->game_time = w->old_time = w->time_step = 0;
    w->step_seconds = 0.0f;
    w->field_10 = 0;
    w->last_ms = 0;
    w->field_1c = 0;
    w->field_68 = 0;
    w->update_slot = 0;
    memset(w->update_times, 0, sizeof w->update_times);
}

void aoe_world_stop_sounds_ror(AoeWorldBaseRor *w, int on) {
    int i;
    if (!on) return;
    for (i = 0; i < w->sound_count; ++i) OPS->sound_stop(CTX, w->sounds[i]);
}

void aoe_world_release_sounds_ror(AoeWorldBaseRor *w) {
    int i;
    for (i = 0; i < w->sound_count; ++i) OPS->sound_stop(CTX, w->sounds[i]);
}

void aoe_world_map_444560_ror(AoeWorldBaseRor *w, int32_t v) {
    if (w->map) OPS->map_444560(CTX, w->map, v);
}

void aoe_world_map_444570_ror(AoeWorldBaseRor *w, int32_t v) {
    if (w->map) OPS->map_444570(CTX, w->map, v);
}

/* ---- text lists ---- */

/* "%hd %hd": the count and the number of entries; -1 (EOF) is a failure. */
static int read_counts(FILE *f, int16_t *count, int16_t *entries) {
    return fscanf(f, "%hd %hd", count, entries) != EOF;
}

int aoe_world_load_sounds_text_ror(AoeWorldBaseRor *w, const char *name) {
    FILE *f = OPS->open_text(CTX, name);
    int16_t entries = 0, i, id;
    if (!f) return 1;
    if (!read_counts(f, &w->sound_count, &entries)) {
        w->sound_count = 0;
        w->sounds = NULL;
    } else {
        w->sounds = zeroed((size_t)(w->sound_count > 0 ? w->sound_count : 0), sizeof *w->sounds);
        for (i = 0; i < entries; ++i) {
            if (fscanf(f, "%hd", &id) == EOF) continue;
            if (id >= 0 && id < w->sound_count) w->sounds[id] = OPS->sound_text(CTX, f, id);
        }
    }
    fclose(f);
    return 1;
}

int aoe_world_load_table_text_ror(AoeWorldBaseRor *w, const char *name) {
    FILE *f = OPS->open_text(CTX, name);
    int16_t r, row, count, k, col;
    if (!f) return 1;
    if (!read_counts(f, &w->table_rows, &w->table_cols)) {
        w->table_rows = 0;
        w->table = NULL;
    } else {
        w->table = zeroed((size_t)(w->table_rows > 0 ? w->table_rows : 0), sizeof *w->table);
        for (r = 0; r < w->table_rows; ++r) {
            if (fscanf(f, "%hd %hd", &row, &count) == EOF) continue;
            if (row < 0 || row >= w->table_rows) continue;
            w->table[row] = zeroed((size_t)(w->table_cols > 0 ? w->table_cols : 0), sizeof(float));
            for (k = 0; k < count; ++k) {
                float v = 0.0f;
                if (fscanf(f, "%hd", &col) != 1) col = 0;
                if (fscanf(f, "%f", &v) == 1 && col >= 0 && col < w->table_cols)
                    w->table[row][col] = v;
            }
        }
    }
    fclose(f);
    return 1;
}

int aoe_world_load_colors_text_ror(AoeWorldBaseRor *w, const char *name) {
    FILE *f = OPS->open_text(CTX, name);
    int16_t entries = 0, i, id;
    if (!f) return 1;
    if (!read_counts(f, &w->color_count, &entries)) {
        w->color_count = 0;
        w->colors = NULL;
    } else {
        w->colors = zeroed((size_t)(w->color_count > 0 ? w->color_count : 0), sizeof *w->colors);
        for (i = 0; i < entries; ++i) {
            AoeWorldColorRor *c;
            if (fscanf(f, "%hd", &id) == EOF) continue;
            if (id < 0 || id >= w->color_count) continue;
            c = malloc(sizeof *c);
            if (c) aoe_world_color_from_text_ror(c, f, id);
            w->colors[id] = c;
        }
    }
    fclose(f);
    return 1;
}

int aoe_world_load_sprites_text_ror(AoeWorldBaseRor *w, const char *name) {
    FILE *f = OPS->open_text(CTX, name);
    int16_t entries = 0, i, id;
    if (!f) return 1;
    if (!read_counts(f, &w->sprite_count, &entries)) {
        w->sprite_count = 0;
        w->sprites = NULL;
    } else {
        w->sprites = zeroed((size_t)(w->sprite_count > 0 ? w->sprite_count : 0),
                            sizeof *w->sprites);
        for (i = 0; i < entries; ++i) {
            if (fscanf(f, "%hd", &id) == EOF) continue;
            if (id >= 0 && id < w->sprite_count)
                w->sprites[id] = OPS->sprite_text(CTX, f, id, w->sounds);
        }
        /* Every slot below the entry count without a sprite gets an empty
           one. */
        for (i = 0; i < entries && i < w->sprite_count; ++i)
            if (!w->sprites[i]) w->sprites[i] = OPS->sprite_empty(CTX, i);
    }
    fclose(f);
    return 1;
}

int aoe_world_load_civs_text_ror(AoeWorldBaseRor *w, const char *name) {
    FILE *f = OPS->open_text(CTX, name);
    int16_t entries = 0, i, id, flag;
    if (!f) return 1;
    if (!read_counts(f, &w->civ_count, &entries)) {
        w->civ_count = 0;
        w->civs = NULL;
    } else {
        w->civs = zeroed((size_t)(w->civ_count > 0 ? w->civ_count : 0), sizeof *w->civs);
        for (i = 0; i < entries; ++i)
            if (fscanf(f, "%hd %hd", &id, &flag) != EOF) w->vtbl->civ_from_text(w, id, flag, f);
    }
    fclose(f);
    return 1;
}

void aoe_world_civ_from_text_ror(AoeWorldBaseRor *w, int16_t id, int16_t flag, FILE *f) {
    if (flag != 0 || id < 0 || id >= w->civ_count) return;
    w->civs[id] = OPS->civ_text(CTX, f);
}

int aoe_world_load_civ_entries_text_ror(AoeWorldBaseRor *w, const char *name) {
    FILE *f = OPS->open_text(CTX, name);
    int16_t count = 0, i, index, value, n, k, id, extra;
    if (!f) return 1;
    if (fscanf(f, "%hd", &count) != EOF) {
        for (i = 0; i < count; ++i) {
            void *civ;
            if (fscanf(f, "%hd", &index) == EOF) continue;
            civ = (index >= 0 && index < w->civ_count) ? w->civs[index] : NULL;
            value = 0;
            if (fscanf(f, "%hd", &value) != 1) value = 0;
            if (civ) OPS->civ_text_value(CTX, civ, value);
            n = 0;
            if (fscanf(f, "%hd", &n) != 1) n = 0;
            for (k = 0; k < n; ++k) {
                if (fscanf(f, "%hd %hd", &id, &extra) == EOF) continue;
                if (civ) OPS->civ_text_entry(CTX, civ, f, id, w->sprites, w->sounds, extra);
            }
        }
    }
    fclose(f);
    return 1;
}

void aoe_world_effects_from_text_ror(AoeWorldBaseRor *w, const char *name) {
    w->effects = OPS->effects_text(CTX, name);
}

void aoe_world_map_from_text_ror(AoeWorldBaseRor *w, const char *a, const char *b,
                                 const char *c, const char *d, int16_t x, int16_t y, int16_t z,
                                 void **sounds, const char *extra) {
    (void)b; /* passed, but the map constructor does not take it */
    w->map = OPS->map_text(CTX, a, c, d, x, y, z, sounds);
    OPS->map_text_extra(CTX, w->map, extra);
}

void aoe_world_map_text_names_ror(AoeWorldBaseRor *w, const char *a, const char *b,
                                  const char *c, const char *d) {
    OPS->map_text_names(CTX, w->map, a, b, c, d);
}

int aoe_world_load_text_ror(AoeWorldBaseRor *w, FILE *f) {
    char n[16][0x30];
    int16_t h1 = 0, h2 = 0, h3 = 0;
    int i;
    for (i = 0; i < 16; ++i) n[i][0] = '\0';
    if (fscanf(f, "%47s %47s %47s %47s %47s %47s %47s %47s %47s %47s %47s %47s %47s %47s %47s %47s "
                  "%hd %hd %hd",
               n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7], n[8], n[9], n[10], n[11], n[12],
               n[13], n[14], n[15], &h1, &h2, &h3) == EOF) {
        /* the original carries on with whatever was read */
    }
    w->vtbl->load_table_text(w, n[0]);
    w->vtbl->load_colors_text(w, n[1]);
    w->vtbl->load_sounds_text(w, n[2]);
    w->vtbl->load_sprites_text(w, n[7]);
    w->vtbl->load_civs_text(w, n[8]);
    w->vtbl->load_civ_entries_text(w, n[9]);
    w->vtbl->effects_from_text(w, n[10]);
    w->vtbl->map_from_text(w, n[3], n[4], n[5], n[6], h1, h2, h3, w->sounds, n[11]);
    w->vtbl->map_text_names(w, n[12], n[13], n[14], n[15]);
    return 1;
}

void aoe_world_nothing_ror(AoeWorldBaseRor *w) { (void)w; }

int aoe_world_convert_text_ror(AoeWorldBaseRor *w, const char *data_path, const char *list_path) {
    FILE *f = OPS->open_text(CTX, list_path);
    if (!f) return 0;
    w->vtbl->load_text(w, f);
    fclose(f);
    w->vtbl->save_data_file(w, data_path);
    return 1;
}

void aoe_world_save_data_file_ror(AoeWorldBaseRor *w, const char *path) {
    AoeOutcomeStream *s = OPS->open_write(CTX, path);
    if (!s) return;
    w->vtbl->save_data(w, s);
    OPS->close_write(CTX, s);
}

/* ---- binary data ---- */

static int16_t read_i16(AoeOutcomeStream *s) {
    const uint8_t *p = aoe_stream_take(s, 2);
    return p ? (int16_t)le16(p) : 0;
}

static void write_i16(AoeOutcomeStream *s, int16_t v) {
    uint8_t b[2];
    put_le16(b, (uint16_t)v);
    aoe_stream_put(s, b, 2);
}

int aoe_world_civ_from_stream_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, int16_t id,
                                  uint8_t flag) {
    if (flag == 0 && id >= 0 && id < w->civ_count)
        w->civs[id] = OPS->civ_read(CTX, s, w->sprites, w->sounds);
    return 1;
}

int aoe_world_load_civs_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s) {
    int16_t i;
    w->civ_count = read_i16(s);
    if (w->civ_count > 0) {
        w->civs = zeroed((size_t)w->civ_count, sizeof *w->civs);
        for (i = 0; i < w->civ_count; ++i) {
            uint8_t flag = aoe_stream_u8(s);
            w->vtbl->civ_from_stream(w, s, i, flag);
        }
    }
    return 1;
}

int aoe_world_load_civs_slot50_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s) {
    return w->vtbl->load_civs(w, s);
}

void aoe_world_load_table_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s) {
    int16_t i, k;
    char *present;
    w->table_rows = read_i16(s);
    w->table_cols = read_i16(s);
    if (w->table_rows <= 0 || w->table_cols <= 0) return;
    w->table = zeroed((size_t)w->table_rows, sizeof *w->table);
    present = zeroed((size_t)w->table_rows, 1);
    for (i = 0; i < w->table_rows; ++i) present[i] = aoe_stream_i32(s) != 0;
    for (i = 0; i < w->table_rows; ++i) {
        if (!present[i]) continue;
        w->table[i] = zeroed((size_t)w->table_cols, sizeof(float));
        for (k = 0; k < w->table_cols; ++k) {
            float v = aoe_stream_f32(s);
            if (w->table[i]) w->table[i][k] = v;
        }
    }
    free(present);
}

void aoe_world_load_colors_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s) {
    int16_t i;
    w->color_count = read_i16(s);
    if (w->color_count <= 0) return;
    w->colors = zeroed((size_t)w->color_count, sizeof *w->colors);
    for (i = 0; i < w->color_count; ++i) {
        AoeWorldColorRor *c = malloc(sizeof *c);
        if (c) aoe_world_color_read_ror(c, s, OPS->open_text, CTX);
        w->colors[i] = c;
    }
}

void aoe_world_load_sounds_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t arg) {
    int16_t i;
    (void)arg;
    w->sound_count = read_i16(s);
    w->sounds = zeroed((size_t)(w->sound_count > 0 ? w->sound_count : 0), sizeof *w->sounds);
    for (i = 0; i < w->sound_count; ++i) w->sounds[i] = OPS->sound_read(CTX, s, w->data_arg);
}

void aoe_world_load_sprites_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s) {
    int16_t i;
    char *present;
    w->sprite_count = read_i16(s);
    w->sprites = zeroed((size_t)(w->sprite_count > 0 ? w->sprite_count : 0), sizeof *w->sprites);
    present = zeroed((size_t)(w->sprite_count > 0 ? w->sprite_count : 0), 1);
    for (i = 0; i < w->sprite_count; ++i) present[i] = aoe_stream_i32(s) != 0;
    for (i = 0; i < w->sprite_count; ++i)
        if (present[i]) w->sprites[i] = OPS->sprite_read(CTX, s, w->sounds, (void **)w->colors);
    for (i = 0; i < w->sprite_count; ++i)
        if (w->sprites[i]) OPS->sprite_link(CTX, w->sprites[i], w->sprites);
    free(present);
}

void aoe_world_map_from_stream_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t arg) {
    (void)arg;
    w->map = OPS->map_read(CTX, s, w->sounds, 1);
}

void aoe_world_effects_from_stream_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s) {
    w->effects = OPS->effects_read(CTX, s);
}

static void slot_54_none(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t arg) {
    (void)w;
    (void)s;
    (void)arg;
}

void aoe_world_load_data_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t a, int32_t b) {
    w->vtbl->load_table(w, s);
    w->vtbl->load_colors(w, s);
    w->vtbl->load_sounds(w, s, a);
    w->vtbl->load_sprites(w, s);
    w->vtbl->map_from_stream(w, s, a);
    w->vtbl->effects_from_stream(w, s);
    aoe_world_load_civs_slot50_ror(w, s);
    w->vtbl->slot_54(w, s, b);
}

void aoe_world_scenario_from_stream_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, int32_t arg) {
    w->scenario = OPS->scenario_read(CTX, s, arg);
}

void aoe_world_scenario_new_ror(AoeWorldBaseRor *w, void *info) {
    w->scenario = OPS->scenario_new(CTX, info);
}

void aoe_world_player_from_stream_ror(AoeWorldBaseRor *w, AoeOutcomeStream *s, uint8_t flag,
                                      int16_t index, void (*load_74)(void *context, void *player,
                                                                    AoeOutcomeStream *s)) {
    if (flag != 0 || index < 0 || index >= w->player_count) return;
    w->players[index] = OPS->player_read(CTX, s, w, index);
    if (w->players[index] && load_74) load_74(CTX, w->players[index], s);
}

int aoe_world_load_data_file_ror(AoeWorldBaseRor *w, const char *path, int32_t a, int32_t b) {
    AoeOutcomeStream *s;
    const uint8_t *tag;
    w->data_arg = a;
    w->field_68 = 0;
    s = OPS->open_read(CTX, path);
    if (!s) return 0;
    tag = aoe_stream_take(s, 8);
    if (tag && memcmp(tag, "VER 3.7", 8) == 0) {
        w->vtbl->load_data(w, s, a, b);
        OPS->close_read(CTX, s);
        return 1;
    }
    OPS->close_read(CTX, s);
    return 0;
}

void aoe_world_save_data_ror(const AoeWorldBaseRor *w, AoeOutcomeStream *s) {
    int16_t i, k;
    aoe_stream_put(s, "VER 3.7", 8);
    write_i16(s, w->table_rows);
    write_i16(s, w->table_cols);
    if (w->table_rows > 0 && w->table_cols > 0) {
        for (i = 0; i < w->table_rows; ++i) aoe_stream_put_i32(s, w->table[i] ? 1 : 0);
        for (i = 0; i < w->table_rows; ++i)
            if (w->table[i])
                for (k = 0; k < w->table_cols; ++k) aoe_stream_put_f32(s, w->table[i][k]);
    }
    write_i16(s, w->color_count);
    for (i = 0; i < w->color_count; ++i) aoe_world_color_write_ror(w->colors[i], s);
    write_i16(s, w->sound_count);
    for (i = 0; i < w->sound_count; ++i) OPS->sound_write(CTX, w->sounds[i], s);
    write_i16(s, w->sprite_count);
    for (i = 0; i < w->sprite_count; ++i) aoe_stream_put_i32(s, w->sprites[i] ? 1 : 0);
    for (i = 0; i < w->sprite_count; ++i)
        if (w->sprites[i]) OPS->sprite_write(CTX, w->sprites[i], s);
    OPS->map_write(CTX, w->map, s);
    OPS->effects_write(CTX, w->effects, s);
    write_i16(s, w->civ_count);
    for (i = 0; i < w->civ_count; ++i) OPS->civ_write(CTX, w->civs[i], s);
}

/* ---- small methods ---- */

void aoe_world_idle_sound_ror(AoeWorldBaseRor *w, uint32_t now) {
    if (w->sound_index >= w->sound_count) w->sound_index = 0;
    if (w->sound_count > 0) OPS->sound_idle(CTX, w->sounds[w->sound_index], now);
    ++w->sound_index;
}

void aoe_world_idle_sprite_ror(AoeWorldBaseRor *w, uint32_t now) {
    if (w->sprite_index >= w->sprite_count) w->sprite_index = 0;
    if (w->sprite_count > 0 && w->sprites[w->sprite_index])
        OPS->sprite_idle(CTX, w->sprites[w->sprite_index], now);
    ++w->sprite_index;
}

int aoe_world_update_slot_is_ror(const AoeWorldBaseRor *w, int32_t v) {
    return w->update_slot == v;
}

int32_t aoe_world_update_time_ror(const AoeWorldBaseRor *w, int32_t i) {
    return w->update_times[i];
}

uint8_t aoe_world_state_ror(const AoeWorldBaseRor *w) { return w->state; }

void aoe_world_new_gaia_ror(AoeWorldBaseRor *w,
                            void *(*player_new)(void *context, AoeWorldBaseRor *w, void *civ,
                                                int16_t index, const char *name, uint8_t civ_id,
                                                int flag)) {
    w->players[0] = player_new(CTX, w, w->civs ? w->civs[0] : NULL, 0, "GAIA", 0, 1);
}

void aoe_world_new_player_ror(AoeWorldBaseRor *w, int16_t index, uint8_t flag, uint8_t civ,
                              const char *name,
                              void *(*player_new)(void *context, AoeWorldBaseRor *w, void *civ,
                                                  int16_t index, const char *name,
                                                  uint8_t civ_id, int flag)) {
    if (flag != 0) return;
    w->players[index] = player_new(CTX, w, w->civs[civ], index, name, civ, 1);
}

void aoe_world_new_views_ror(AoeWorldBaseRor *w) {
    int i;
    for (i = 0; i < w->player_count; ++i)
        if (w->players[i]) OPS->player_new_view(CTX, w->players[i]);
}

const AoeWorldVtblRor aoe_world_base_vtable_ror = {
    aoe_world_load_text_ror,
    aoe_world_load_table_text_ror,
    aoe_world_load_civs_text_ror,
    aoe_world_load_civ_entries_text_ror,
    aoe_world_load_sounds_text_ror,
    aoe_world_load_colors_text_ror,
    aoe_world_load_sprites_text_ror,
    aoe_world_civ_from_text_ror,
    aoe_world_effects_from_text_ror,
    aoe_world_map_from_text_ror,
    aoe_world_map_text_names_ror,
    aoe_world_civ_from_stream_ror,
    aoe_world_load_civs_ror,
    aoe_world_load_colors_ror,
    aoe_world_load_table_ror,
    aoe_world_load_sounds_ror,
    aoe_world_load_sprites_ror,
    aoe_world_map_from_stream_ror,
    aoe_world_effects_from_stream_ror,
    slot_54_none,
    aoe_world_load_data_ror,
    aoe_world_release_sounds_ror,
    aoe_world_save_data_file_ror,
    aoe_world_save_data_ror,
};
