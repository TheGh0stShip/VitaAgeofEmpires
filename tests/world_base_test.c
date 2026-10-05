#include "world_base.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

/* Fake sub-objects: a small heap record with a kind and an id. */
typedef struct {
    int kind, id, value, entries, stopped, idle, linked;
} Obj;

static int frees, lists_cleared;
static char files[8][64];
static char contents[8][512];
static int nfiles;
static uint8_t outbuf[4096];
static AoeOutcomeStream out;

static Obj *obj(int kind, int id) {
    Obj *o = calloc(1, sizeof *o);
    o->kind = kind;
    o->id = id;
    return o;
}
static void *sound_text(void *c, FILE *f, int16_t id) {
    int16_t v;
    (void)c;
    if (fscanf(f, "%hd", &v) != 1) v = 0;
    return obj(1, id * 100 + v);
}
static void *sound_read(void *c, AoeOutcomeStream *s, int32_t a) { (void)c; (void)a; return obj(1, aoe_stream_i32(s)); }
static void sound_write(void *c, void *o, AoeOutcomeStream *s) { (void)c; aoe_stream_put_i32(s, ((Obj *)o)->id); }
static void sound_stop(void *c, void *o) { (void)c; ((Obj *)o)->stopped++; }
static void sound_idle(void *c, void *o, uint32_t t) { (void)c; ((Obj *)o)->idle = (int)t; }
static void ofree(void *c, void *o) { (void)c; frees++; free(o); }
static void *sprite_text(void *c, FILE *f, int16_t id, void **sounds) { (void)c; (void)f; (void)sounds; return obj(2, id); }
static void *sprite_empty(void *c, int16_t id) { (void)c; return obj(3, id); }
static void *sprite_read(void *c, AoeOutcomeStream *s, void **so, void **co) { (void)c; (void)so; (void)co; return obj(2, aoe_stream_i32(s)); }
static void sprite_link(void *c, void *o, void **all) { (void)c; (void)all; ((Obj *)o)->linked = 1; }
static void sprite_write(void *c, void *o, AoeOutcomeStream *s) { (void)c; aoe_stream_put_i32(s, ((Obj *)o)->id); }
static void *civ_text(void *c, FILE *f) { (void)c; (void)f; return obj(4, 0); }
static void *civ_read(void *c, AoeOutcomeStream *s, void **sp, void **so) { (void)c; (void)sp; (void)so; return obj(4, aoe_stream_i32(s)); }
static void civ_value(void *c, void *o, int32_t v) { (void)c; ((Obj *)o)->value = v; }
static void civ_entry(void *c, void *o, FILE *f, int16_t id, void **sp, void **so, int16_t ex) {
    (void)c; (void)f; (void)sp; (void)so; ((Obj *)o)->entries += id + ex;
}
/* A civilisation saves its own presence byte first (+0x14). */
static void civ_write(void *c, void *o, AoeOutcomeStream *s) { (void)c; aoe_stream_put_u8(s, 0); aoe_stream_put_i32(s, ((Obj *)o)->id); }
static void *effects_text(void *c, const char *n) { (void)c; return obj(5, (int)strlen(n)); }
static void *effects_read(void *c, AoeOutcomeStream *s) { (void)c; return obj(5, aoe_stream_i32(s)); }
static void effects_write(void *c, void *o, AoeOutcomeStream *s) { (void)c; aoe_stream_put_i32(s, ((Obj *)o)->id); }
static char map_names[64];
static void *map_text(void *c, const char *a, const char *b, const char *d, int16_t x, int16_t y, int16_t z, void **so) {
    (void)c; (void)so;
    snprintf(map_names, sizeof map_names, "%s,%s,%s,%d,%d,%d", a, b, d, x, y, z);
    return obj(6, 0);
}
static void map_extra(void *c, void *m, const char *n) { (void)c; ((Obj *)m)->value = (int)strlen(n); }
static void *map_read(void *c, AoeOutcomeStream *s, void **so, int32_t a) { (void)c; (void)so; (void)a; return obj(6, aoe_stream_i32(s)); }
static void map_names4(void *c, void *m, const char *a, const char *b, const char *d, const char *e) {
    (void)c; (void)a; (void)b; (void)d; ((Obj *)m)->entries = (int)strlen(e);
}
static void map_write(void *c, void *m, AoeOutcomeStream *s) { (void)c; aoe_stream_put_i32(s, ((Obj *)m)->id + 7); }
static void map_v(void *c, void *m, int32_t v) { (void)c; ((Obj *)m)->value = v; }
static void *scen_new(void *c, void *i) { (void)c; (void)i; return obj(7, 0); }
static void *scen_read(void *c, AoeOutcomeStream *s, int32_t a) { (void)c; (void)s; return obj(7, a); }
static void *player_read(void *c, AoeOutcomeStream *s, AoeWorldBaseRor *w, int16_t i) { (void)c; (void)s; (void)w; return obj(8, i); }
static void player_view(void *c, void *p) { (void)c; ((Obj *)p)->value++; }
static void player_68(void *c, void *p) { (void)c; ((Obj *)p)->entries++; }
static void *list_new(void *c) { (void)c; return obj(9, 0); }
static void list_clear(void *c, void *l) { (void)c; (void)l; lists_cleared++; }
static FILE *open_text(void *c, const char *path) {
    int i;
    (void)c;
    for (i = 0; i < nfiles; ++i) {
        if (strcmp(files[i], path) == 0) {
            FILE *f = tmpfile();
            fputs(contents[i], f);
            rewind(f);
            return f;
        }
    }
    return NULL;
}
static AoeOutcomeStream in;
static AoeOutcomeStream *open_read(void *c, const char *path) {
    (void)c;
    if (strcmp(path, "data.dat") != 0) return NULL;
    in.data = outbuf;
    in.size = out.pos;
    in.pos = 0;
    in.failed = 0;
    return &in;
}
static void close_read(void *c, AoeOutcomeStream *s) { (void)c; (void)s; }
static AoeOutcomeStream *open_write(void *c, const char *path) {
    (void)c; (void)path;
    out.data = outbuf;
    out.size = sizeof outbuf;
    out.pos = 0;
    out.failed = 0;
    return &out;
}
static void close_write(void *c, AoeOutcomeStream *s) { (void)c; (void)s; }
static int displaced;
static uint8_t ptype(void *c, void *p) { (void)c; return (uint8_t)((Obj *)p)->value; }
static void *list_items[8];
static int list_n;
static void list_add(void *c, void *l, void *o) { (void)c; ((Obj *)l)->entries++; list_items[list_n++] = o; }
static void list_remove(void *c, void *l, void *o) {
    int i;
    (void)c;
    ((Obj *)l)->entries--;
    for (i = 0; i < list_n; ++i) if (list_items[i] == o) { list_items[i] = list_items[--list_n]; break; }
}
static void *list_first(void *c, void *l) { (void)c; (void)l; return list_n ? list_items[0] : NULL; }
static void obj_displaced(void *c, void *o) { (void)c; (void)o; displaced++; }
static uint8_t map_byte(void *c, void *m, int which) { (void)c; (void)m; return (uint8_t)(which + 5); }
static int32_t def100(void *c) { (void)c; return 42; }

static const AoeWorldOpsRor ops = {
    NULL, sound_text, sound_read, sound_write, sound_stop, sound_idle, ofree,
    sprite_text, sprite_empty, sprite_read, sprite_link, sprite_write, sound_idle, ofree,
    civ_text, civ_read, civ_value, civ_entry, civ_write, ofree,
    effects_text, effects_read, effects_write, ofree,
    map_text, map_extra, map_read, map_names4, map_write, map_v, map_v, ofree,
    scen_new, scen_read, ofree,
    player_read, player_view, player_68, ofree,
    list_new, list_clear, ofree, ofree,
    open_text, open_read, close_read, open_write, close_write,
    ptype, list_add, list_remove, list_first, obj_displaced, map_byte, def100};

static void add_file(const char *name, const char *text) {
    snprintf(files[nfiles], sizeof files[0], "%s", name);
    snprintf(contents[nfiles], sizeof contents[0], "%s", text);
    ++nfiles;
}

static void *player_new(void *c, AoeWorldBaseRor *w, void *civ, int16_t i, const char *name, uint8_t id, int flag) {
    (void)c; (void)w; (void)civ; (void)id; (void)flag;
    return obj(8, i * 10 + (int)strlen(name));
}

int main(void) {
    AoeWorldBaseRor w, w2;
    FILE *list;
    int i;

    add_file("table.txt", "3 2\n0 1\n1 1.5\n2 2\n0 1\n1 0.25\n");
    add_file("colors.txt", "2 2\n1 7 player1 4 5\n0 3 player0 1 2\n");
    add_file("sounds.txt", "3 2\n2 9\n0 4\n");
    add_file("sprites.txt", "4 3\n1\n");
    add_file("civs.txt", "2 2\n0 0\n1 1\n");
    add_file("entries.txt", "1\n0 5 2\n3 4\n10 20\n");
    add_file("list.txt", "table.txt colors.txt sounds.txt a b c d sprites.txt civs.txt entries.txt eff.txt x m1 m2 m3 mlast 1 2 3\n");

    assert(aoe_world_init_ror(&w, &ops));
    assert(w.objects_cap == 10000 && w.seed == 14 && w.state == 0 && w.field_100 == -1);
    assert(aoe_world_object_table_ror == w.objects);

    list = open_text(NULL, "list.txt");
    assert(aoe_world_load_text_ror(&w, list) == 1);
    fclose(list);
    assert(w.table_rows == 3 && w.table_cols == 2 && w.table[0][1] == 1.5f && w.table[0][0] == 0.0f);
    assert(w.table[2][0] == 1.0f && w.table[2][1] == 0.25f && w.table[1] == NULL);
    assert(w.color_count == 2 && w.colors[1]->id == 7 && strcmp(w.colors[1]->name, "player1.col") == 0);
    assert(w.colors[1]->palette == 1 && w.colors[1]->field_25 == 4 && w.colors[1]->field_24 == 5);
    assert(w.sound_count == 3 && ((Obj *)w.sounds[2])->id == 209 && ((Obj *)w.sounds[0])->id == 4);
    assert(w.sounds[1] == NULL);
    /* Sprites: entry 1 from text; slots below the entry count (3) filled
       with empty ones; slot 3 stays empty. */
    assert(w.sprite_count == 4 && ((Obj *)w.sprites[1])->kind == 2 && ((Obj *)w.sprites[0])->kind == 3);
    assert(((Obj *)w.sprites[2])->kind == 3 && w.sprites[3] == NULL);
    assert(w.civ_count == 2 && w.civs[0] && w.civs[1] == NULL);
    assert(((Obj *)w.civs[0])->value == 5 && ((Obj *)w.civs[0])->entries == 37);
    assert(((Obj *)w.effects)->id == 7);
    assert(strcmp(map_names, "a,c,d,1,2,3") == 0 && ((Obj *)w.map)->value == 1 && ((Obj *)w.map)->entries == 5);

    /* Convert: save the data file, then load it into a new world. */
    w.sound_count = 3;
    w.sounds[1] = obj(1, 55);
    w.civs[1] = obj(4, 9);
    list = open_text(NULL, "list.txt");
    fclose(list);
    aoe_world_save_data_file_ror(&w, "data.dat");
    assert(!out.failed && memcmp(outbuf, "VER 3.7", 8) == 0);

    assert(aoe_world_init_ror(&w2, &ops));
    assert(aoe_world_load_data_file_ror(&w2, "missing", 3, 4) == 0);
    assert(aoe_world_load_data_file_ror(&w2, "data.dat", 3, 4) == 1);
    assert(!in.failed && in.pos == in.size);
    assert(w2.data_arg == 3 && w2.table_rows == 3 && w2.table[1] == NULL && w2.table[2][1] == 0.25f);
    assert(w2.color_count == 2 && w2.colors[1]->id == 7 && w2.colors[1]->palette == 1);
    assert(strcmp(w2.colors[0]->name, "player0.col") == 0);
    assert(w2.sound_count == 3 && ((Obj *)w2.sounds[1])->id == 55);
    assert(w2.sprite_count == 4 && w2.sprites[3] == NULL && ((Obj *)w2.sprites[1])->linked);
    assert(((Obj *)w2.map)->id == 7 && ((Obj *)w2.effects)->id == 7);
    assert(w2.civ_count == 2 && ((Obj *)w2.civs[1])->id == 9);

    outbuf[0] = 'X';
    assert(aoe_world_load_data_file_ror(&w2, "data.dat", 0, 0) == 0);

    /* Round robin idle unloads. */
    w2.sound_index = 7;
    aoe_world_idle_sound_ror(&w2, 99);
    assert(((Obj *)w2.sounds[0])->idle == 99 && w2.sound_index == 1);
    w2.sprite_index = 3;
    aoe_world_idle_sprite_ror(&w2, 5);
    assert(w2.sprite_index == 4);

    /* Players and reset. */
    w2.player_count = 3;
    w2.players = calloc(3, sizeof *w2.players);
    aoe_world_new_gaia_ror(&w2, player_new);
    assert(((Obj *)w2.players[0])->id == 4);
    aoe_world_new_player_ror(&w2, 2, 0, 1, "abc", player_new);
    aoe_world_new_player_ror(&w2, 1, 1, 1, "abc", player_new);
    assert(((Obj *)w2.players[2])->id == 23 && w2.players[1] == NULL);
    aoe_world_new_views_ror(&w2);
    assert(((Obj *)w2.players[2])->value == 1);
    w2.scenario = obj(7, 0);
    lists_cleared = 0;
    frees = 0;
    aoe_world_reset_ror(&w2);
    assert(w2.state == 2 && w2.player_count == 0 && w2.players == NULL && w2.scenario == NULL);
    assert(lists_cleared == 7 && frees == 3 && w2.campaign == -1);
    assert(((Obj *)w2.sounds[0])->stopped == 1);

    for (i = 0; i < 9; ++i) w2.update_times[i] = i;
    w2.update_slot = 4;
    assert(aoe_world_update_slot_is_ror(&w2, 4) && aoe_world_update_time_ror(&w2, 3) == 3);
    aoe_world_clear_times_ror(&w2);
    assert(w2.update_slot == 0);

    /* Object IDs and registration. */
    w2.objects[0] = obj(0, 0);
    w2.objects[1] = obj(0, 1);
    w2.field_68 = 0;
    assert(aoe_world_next_id_ror(&w2) == 2 && w2.field_68 == 3);
    w2.field_74 = 1;
    w2.field_70 = 77;
    assert(aoe_world_next_id_ror(&w2) == 77);
    w2.field_6c = 5;
    aoe_world_next_negative_id_ror(&w2);
    assert(w2.field_6c == 5); /* below +0x70 already */
    w2.field_6c = 90;
    aoe_world_next_negative_id_ror(&w2);
    assert(w2.field_6c == 76);
    /* A displaced object while loading ends the load. */
    {
        void *old = w2.objects[1];
        assert(aoe_world_register_ror(&w2, obj(0, 1), 1) && displaced == 1 && w2.field_74 == 0);
        free(old);
    }
    free(w2.objects[0]);
    w2.objects[0] = NULL;
    /* Growth: 10000 doubles to 20000; 25000 needs x 1.1. */
    assert(aoe_world_register_ror(&w2, w2.objects[1], 12000) && w2.objects_cap == 20000);
    assert(aoe_world_object_table_ror == w2.objects);
    assert(aoe_world_register_ror(&w2, w2.objects[1], 45000) && w2.objects_cap == 49500);
    assert(aoe_world_register_ror(&w2, w2.objects[1], -1500) && w2.objects2_cap == 2000);
    assert(w2.objects2[1500] == w2.objects[1]);
    assert(aoe_world_unregister_ror(&w2, -1500) && w2.objects2[1500] == NULL);
    assert(aoe_world_unregister_ror(&w2, 45000) && !aoe_world_unregister_ror(&w2, 50000));
    w2.objects[12000] = NULL;
    free(w2.objects[1]);
    w2.objects[1] = NULL;
    /* Kind lists. */
    {
        Obj a, b2;
        assert(aoe_world_list_add_ror(&w2, 25, &a) && ((Obj *)w2.lists[6])->entries == 1);
        assert(!aoe_world_list_add_ror(&w2, 26, &b2));
        assert(aoe_world_list_take_ror(&w2, 25) == &a && ((Obj *)w2.lists[6])->entries == 0);
        assert(aoe_world_list_take_ror(&w2, 25) == NULL && aoe_world_list_take_ror(&w2, 1) == NULL);
    }
    /* Next player of a type (player +0x48 kept in `value`). */
    {
        Obj p[4];
        void *pp[4];
        for (i = 0; i < 4; ++i) { memset(&p[i], 0, sizeof p[i]); pp[i] = &p[i]; }
        p[1].value = 3; p[3].value = 3;
        w2.players = pp;
        w2.player_count = 4;
        w2.field_fc = -1;
        aoe_world_next_player_of_type_ror(&w2, 3);
        assert(w2.field_fc == 1);
        aoe_world_next_player_of_type_ror(&w2, 3);
        assert(w2.field_fc == 3);
        aoe_world_next_player_of_type_ror(&w2, 3);
        assert(w2.field_fc == 1);
        w2.players = NULL;
        w2.player_count = 0;
    }
    w2.field_f8 = 5;
    aoe_world_f8_less_ror(&w2, 9);
    assert(w2.field_f8 == 0);
    w2.field_100 = -1;
    assert(aoe_world_field_100_ror(&w2) == 42 && w2.field_100 == 42);
    assert(aoe_world_map_8db8_ror(&w2) == 5 && aoe_world_map_8db9_ror(&w2) == 6);

    aoe_world_release_ror(&w2, NULL);
    aoe_world_release_ror(&w, NULL);
    puts("world_base_test: ok");
    return 0;
}
