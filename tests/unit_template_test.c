#include "unit_template.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

typedef struct { int16_t id; } Res;
static Res sprites_store[8] = {{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}};
static Res sounds_store[8] = {{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}};
static void *sprites[8], *sounds[8];
typedef struct { int n; uint8_t flags[4]; } Tasks;
static Tasks task_store = {3, {0, 0, 1, 0}};
static int frees_, recycled_calls, made;

static int16_t sid(void *c, const void *p) { (void)c; return ((const Res *)p)->id; }
static void *tcopy(void *c, const void *t) { (void)c; Tasks *n = malloc(sizeof *n); *n = *(const Tasks *)t; return n; }
static void *tread(void *c, AoeOutcomeStream *s, void **sp, void **so) {
    (void)c; (void)sp; (void)so; Tasks *n = malloc(sizeof *n); memset(n, 0, sizeof *n); n->n = aoe_stream_u8(s); return n;
}
static void *ttext(void *c, FILE *f, void **sp, void **so) { (void)c; (void)f; (void)sp; (void)so; return tcopy(c, &task_store); }
static void twrite(void *c, const void *t, AoeOutcomeStream *s) { (void)c; aoe_stream_put_u8(s, (uint8_t)((const Tasks *)t)->n); }
static void tfree(void *c, void *t) { (void)c; frees_++; free(t); }
static int32_t tcount(void *c, const void *t) { (void)c; return ((const Tasks *)t)->n; }
static int tflag(void *c, const void *t, int32_t i) { (void)c; return ((const Tasks *)t)->flags[i]; }
static int16_t task_id(void *c, const void *t, int32_t i) { (void)c; (void)t; return (int16_t)(i * 10); }
static const void *task_at(void *c, const void *t, int32_t i) { (void)c; return &((const Tasks *)t)->flags[i]; }
static int instance_dummy;
static void *rec(void *c, uint8_t k) { (void)c; recycled_calls++; return k == 0x3c ? &instance_dummy : NULL; }
static void *reinit(void *c, void *o, AoeTemplateRor *t, void *p, float x, float y, float z) { (void)c; (void)t; (void)p; (void)x; (void)y; (void)z; return o; }
static void *inew(void *c, AoeTemplateRor *t, void *p, float x, float y, float z) { (void)c; (void)t; (void)p; (void)x; (void)y; (void)z; made++; return &made; }

static const AoeTemplateOpsRor ops = {NULL, sid, sid, tcopy, tread, ttext, twrite, tfree, tcount, tflag, rec, reinit, inew};

static void fill(AoeTemplateRor *t, uint8_t type) {
    static AoeTemplateDamageRor dmg[2];
    static AoeTemplatePairRor atk[2] = {{3, 7}, {4, 12}}, arm[1] = {{3, 2}};
    memset(t, 0, sizeof *t);
    t->type = type;
    t->name = "Clubman";
    t->id = 73;
    t->field_10 = 73;
    t->field_12 = 74;
    t->unit_class = 6;
    t->sprite_18 = sprites[1];
    t->sprite_20 = sprites[3];
    t->hit_points = 40;
    t->line_of_sight = 4.0f;
    t->size[0] = 0.2f;
    t->sound_40 = sounds[2];
    t->dead_unit = -1;
    t->storage_type[1] = 11;
    t->storage_amount[1] = 1.5f;
    t->storage_mode[2] = 1;
    t->field_8b[4] = 9;
    dmg[0].sprite = sprites[5]; dmg[0].percent = 25; dmg[0].apply = 1;
    dmg[1].sprite = sprites[6]; dmg[1].percent = 75; dmg[1].apply = 0;
    t->damage = dmg;
    t->damage_count = 2;
    t->field_ac = 0.3f;
    t->speed = 1.2f;
    t->sprite_bc = sprites[4];
    t->rotation = 0.5f;
    t->field_d4 = 2;
    t->tasks = &task_store;
    t->default_task = 2;
    t->work_rate = 0.4f;
    t->sound_f0 = sounds[1];
    t->sprite_fc = sprites[7];
    t->attacks = atk; t->attack_count = 2;
    t->armors = arm; t->armor_count = 1;
    t->field_114 = 1.0f;
    t->field_120 = 2.0f;
    t->max_attack = 12; t->max_armor = 2; t->field_140 = 1.0f; t->field_144 = 2.0f;
    t->field_148[1] = 5;
    t->field_150 = 0.25f;
    t->costs[1].type = 2;
    t->costs[1].amount = 40;
    t->costs[1].flag = 1;
    t->train_time = 26;
    t->button = 3;
    t->pierce_armor = 4;
    t->sprite_168 = sprites[2];
    t->sound_164 = sounds[5];
    t->field_178 = -7;
}

static void *played;
static void play_fn(void *c, void *s) { (void)c; played = s; }


static int ol_frame, ol_flag, ol_lines; static int16_t ol_c[8];
static void ol_plan(void *c, int32_t f, int32_t sx, int32_t sy, int32_t a5, int32_t a2, int32_t flag) { (void)c; (void)sx; (void)sy; (void)a5; (void)a2; ol_frame = f; ol_flag = flag; }
static void ol_project(void *c, float dx, float dy, int32_t sx, int32_t sy, int16_t *ox, int16_t *oy) { (void)c; *ox = (int16_t)(sx + (int)dx); *oy = (int16_t)(sy + (int)dy); }
static void ol_node(void *c, const int16_t k[8], uint8_t color) { (void)c; memcpy(ol_c, k, sizeof ol_c); ++ol_lines; assert(color == 7); }
static void outline_tests(void) {
    AoeTemplateOutlineOpsRor o = {NULL, ol_plan, ol_project, ol_node};
    aoe_template_outline_draw_ror(&o, 1, 0, 0, 0, 0, 100, 50, 0, 0, 1, 2.0f, 3.0f, 7);
    assert(ol_flag == 10 && ol_frame == 0 && ol_lines == 1 && ol_c[0] == 102 && ol_c[1] == 47 &&
           ol_c[2] == 102 && ol_c[3] == 53 && ol_c[4] == 98 && ol_c[5] == 53 && ol_c[6] == 98 && ol_c[7] == 47);
    aoe_template_outline_draw_ror(&o, 1, 1, 1, 9, 5, 100, 50, 0, 0, 0, 2.0f, 3.0f, 7);
    assert(ol_flag == 0 && ol_frame == 9 && ol_lines == 1);
    aoe_template_outline_draw_ror(&o, 1, 1, 0, 9, 0, 100, 50, 0, 0, 0, 2.0f, 3.0f, 7);
    assert(ol_frame == 2);
    aoe_template_outline_draw_ror(&o, 1, 1, 0, 9, 5, 100, 50, 0, 0, 0, 2.0f, 3.0f, 7);
    assert(ol_frame == 4);
    aoe_template_outline_draw_ror(&o, 0, 0, 0, 0, 0, 100, 50, 0, 0, 1, 2.0f, 3.0f, 7);
    assert(ol_lines == 1);
}
int main(void) {
    static uint8_t buf[4096];
    static const uint8_t types[] = {10, 0x14, 0x19, 0x1e, 0x28, 0x32, 0x3c, 0x46, 0x50, 0x5a};
    AoeTemplateRor t, back, *clone;
    size_t k;
    int i;
    for (i = 0; i < 8; ++i) { sprites[i] = &sprites_store[i]; sounds[i] = &sounds_store[i]; }

    for (k = 0; k < sizeof types; ++k) {
        AoeOutcomeStream s = {buf, sizeof buf, 0, 0};
        uint8_t type;
        fill(&t, types[k]);
        aoe_template_save_ror(&t, &s, &ops);
        assert(!s.failed);
        s.size = s.pos;
        s.pos = 0;
        type = aoe_stream_u8(&s);
        assert(type == types[k]);
        assert(aoe_template_load_ror(&back, type, &s, sprites, sounds, 7.24f, &ops));
        assert(s.pos == s.size);
        assert(back.type == types[k] && strcmp(back.name, "Clubman") == 0 && back.id == 73);
        assert(back.field_12 == 74 && back.sprite_18 == sprites[1] && back.sprite_1c == NULL);
        if (types[k] == 0x5a) assert(back.speed == 0.0f && back.tasks == NULL);
        assert(back.sound_40 == sounds[2] && back.damage_count == 2 && back.damage[1].percent == 75);
        assert(back.damage[0].sprite == sprites[5] && back.damage[0].apply == 1);
        assert(back.storage_amount[1] == 1.5f && back.field_8b[4] == 9 && back.field_ac == 0.3f);
        if (types[k] >= 0x14 && types[k] != 0x5a) assert(back.speed == 1.2f);
        if (types[k] >= 0x1e && types[k] != 0x5a) assert(back.sprite_bc == sprites[4] && back.field_d4 == 2);
        if (types[k] >= 0x28 && types[k] != 0x5a) assert(back.default_task == 2 && back.sound_f0 == sounds[1] && ((Tasks *)back.tasks)->n == 3);
        if (types[k] >= 0x32 && types[k] != 0x5a) assert(back.attack_count == 2 && back.attacks[1].amount == 12 && back.max_attack == 12 && back.sprite_fc == sprites[7]);
        if (types[k] == 0x3c) assert(back.field_148[1] == 5 && back.field_150 == 0.25f);
        if (types[k] == 0x46 || types[k] == 0x50) assert(back.costs[1].amount == 40 && back.costs[1].flag == 1 && back.train_time == 26 && back.button == 3 && back.pierce_armor == 4);
        if (types[k] == 0x50) assert(back.sprite_168 == sprites[2] && back.sound_164 == sounds[5] && back.field_178 == -7);
        aoe_template_release_ror(&back, &ops);
    }

    /* Copy and clone. */
    fill(&t, 0x32);
    clone = aoe_template_clone_ror(&t, &ops);
    assert(clone && clone->type == 0x32 && clone->name != t.name && strcmp(clone->name, "Clubman") == 0);
    assert(clone->attacks != t.attacks && clone->attacks[0].amount == 7 && clone->field_12 == 73);
    assert(clone->tasks != t.tasks && ((Tasks *)clone->tasks)->n == 3);
    aoe_template_release_ror(clone, &ops);
    free(clone);
    fill(&t, 0x19);
    clone = aoe_template_clone_ror(&t, &ops);
    assert(clone->type == 0x19 && clone->field_65 == 3 && clone->tasks == NULL);
    aoe_template_release_ror(clone, &ops);
    free(clone);

    /* Text: a type 50 with its default task found. */
    {
        FILE *f = tmpfile();
        fputs("Archer 4 5 0 1 2 -1 0 35 6.5 0 0.2 0.2 0.3 -1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0 0 "
              "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
              "4 25 1 -1 0 0 -1 0 0\n"
              "1 6 75 0\n"
              "3 1 0 0\n"
              "1.4\n"
              "4 -1 0.5 0 0 0 0.0 0\n"
              "7 0.8 -1 -1 0 1 2 0\n"
              "7 4 1 1 0 1.5 0 2 0 0 0 0 0 0 0 0 0\n"
              "3 4\n"
              "3 1\n", f);
        rewind(f);
        assert(aoe_template_text_ror(&back, 0x32, f, sprites, sounds, 99, &ops));
        fclose(f);
        assert(strcmp(back.name, "Archer") == 0 && back.field_10 == 99 && back.field_12 == 99);
        assert(back.hit_points == 35 && back.sprite_18 == sprites[1] && back.sprite_20 == NULL);
        assert(back.field_ac == 0.2f && back.storage_type[0] == 4 && back.storage_amount[0] == 25.0f);
        assert(back.damage_count == 1 && back.damage[0].sprite == sprites[6]);
        assert(back.sound_3c == sounds[3] && back.sound_44 == sounds[1]);
        assert(back.speed == 1.4f && back.sprite_bc == sprites[4] && back.rotation == 0.5f);
        assert(back.search_radius == 7.0f && back.sound_f0 == sounds[1] && back.default_task == 2);
        assert(back.sprite_fc == sprites[7] && back.field_100 == 4 && back.attacks[0].amount == 4);
        assert(back.max_attack == 4 && back.max_armor == 1 && back.field_140 == 1.5f);
        assert(back.recycle == 0);
        aoe_template_release_ror(&back, &ops);
    }

    /* Attributes. */
    fill(&t, 0x3c);
    assert(aoe_template_set_attribute_ror(&t, 5, 2.0f, 2) && t.speed == 2.4f);
    assert(aoe_template_set_attribute_ror(&t, 0xd, 0.1f, 1) && t.work_rate > 0.49f);
    assert(!aoe_template_set_attribute_ror(&t, 6, 1.0f, 0));
    assert(!aoe_template_set_attribute_ror(&t, 0x13, 9.7f, 1) && t.field_148[1] == 9);
    fill(&t, 0x14);
    assert(!aoe_template_set_attribute_ror(&t, 6, 1.0f, 1));
    fill(&t, 0x5a);
    assert(!aoe_template_set_attribute_ror(&t, 5, 1.0f, 0));
    /* Type 70's derived pierce armor and +0x13c. */
    {
        FILE *f = tmpfile();
        fputs("Hut 1 0 3 -1 -1 -1 0 1 0 0 1 1 0 -1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -1 0 0 0 0 "
              "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0\n0\n-1 -1 0 0\n0\n"
              "-1 -1 0 0 0 0 0 0\n0 0 -1 -1 0 -1 -1 0\n-1 1 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
              "3 6 4 2\n"
              "0 0 0 0 0 0 0 0 0\n9 8 1\n", f);
        rewind(f);
        assert(aoe_template_text_ror(&back, 0x46, f, sprites, sounds, 3, &ops));
        fclose(f);
        assert(back.pierce_armor == 6 && back.max_attack == 2 && back.train_time == 9 && back.button == 1);
        aoe_template_release_ror(&back, &ops);
    }

    /* Instances: recycled when +0xa4. */
    fill(&t, 0x3c);
    t.recycle = 1;
    assert(aoe_template_create_ror(&t, NULL, 0, 0, 0, &ops) == &instance_dummy);
    t.recycle = 0;
    assert(aoe_template_create_ror(&t, NULL, 0, 0, 0, &ops) == &made && made == 1);

    fill(&t, 0x28);
    assert(aoe_template_find_task_ror(&t, 20, task_id, task_at, &ops) == &task_store.flags[2]);
    assert(aoe_template_find_task_ror(&t, -1, task_id, task_at, &ops) == NULL);
    puts("unit_template_test: ok");
    {
        static AoeTemplateRor st;
        int snd = 0;
        played = NULL;
        aoe_template_play_sound_ror(&st, 0, play_fn, NULL);
        assert(played == NULL);
        st.sound_f4 = &snd;
        aoe_template_play_sound_ror(&st, 1, play_fn, NULL);
        assert(played == &snd);
    }
    outline_tests();
    return 0;
}
