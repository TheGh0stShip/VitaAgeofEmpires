#include "unit_tasks.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int sprite_tag[8];
static int sound_tag[4];

static int16_t sprite_id(void *c, const void *p) { (void)c; return (int16_t)((const int *)p - sprite_tag); }
static int16_t sound_id(void *c, const void *p) { (void)c; return (int16_t)((const int *)p - sound_tag); }

int main(void) {
    void *sprites[8], *sounds[4];
    AoeTaskOpsRor ops = {NULL, sprite_id, sound_id};
    AoeTaskListRor l = {0}, l2 = {0}, l3 = {0};
    AoeTaskRor *t;
    uint8_t buf[512];
    AoeOutcomeStream s = {buf, sizeof buf, 0, 0};
    FILE *f;
    int i;
    for (i = 0; i < 8; ++i) sprites[i] = &sprite_tag[i];
    for (i = 0; i < 4; ++i) sounds[i] = &sound_tag[i];

    assert(aoe_task_new_ror(2, 0) == NULL);
    t = aoe_task_new_ror(1, 7);
    assert(t->type == 1 && t->id == 7 && t->field_e == -1 && t->field_1a == -1 && t->field_6 == 0);
    free(t);

    f = tmpfile();
    fputs("2\n1 0 3 5 6 7 8 9 10 11 12 1.5 2.5 3.5 1 4.5 2 3 13 14 4 5 6 0 1 -1 2 3 -1\n"
          "1 1 9 0 0 0 -1 -1 -1 -1 -1 0 0 0 0 0 0 0 0 0 0 0 0 -1 -1 -1 -1 -1 -1\n", f);
    rewind(f);
    assert(aoe_task_list_text_ror(&l, f, sprites, sounds));
    fclose(f);
    assert(l.count == 2);
    t = l.tasks[0];
    assert(t->id == 0 && t->field_4 == 3 && t->field_a == 7 && t->field_e == 8 && t->field_1a == 12);
    assert(t->field_24 == 3.5f && t->field_28 == 1 && t->field_2c == 4.5f && t->field_c == 2);
    assert(t->field_d == 3 && t->field_30 == 13 && t->field_12 == 6);
    assert(t->graphics[0] == sprites[0] && t->graphics[2] == NULL && t->sounds[0] == sounds[3]);
    assert(t->sounds[1] == NULL && l.tasks[1]->id == 1 && l.tasks[1]->field_4 == 9);

    aoe_task_list_save_ror(&l, &s, &ops);
    assert(!s.failed && s.pos == 2 + 2 * 59);
    assert(buf[0] == 2 && buf[2] == 1 && buf[4] == 0 && buf[6] == 3);
    s.size = s.pos;
    s.pos = 0;
    /* The list load reads the type as part of each entry. */
    assert(aoe_task_list_load_ror(&l2, &s, sprites, sounds));
    assert(s.pos == s.size && l2.count == 2);
    assert(memcmp(l2.tasks[0], l.tasks[0], sizeof *t) == 0);

    assert(aoe_task_list_copy_ror(&l3, &l2));
    l3.tasks[1]->field_6 = 99;
    assert(l3.tasks[1]->id == 1 && l3.tasks[0]->graphics[1] == sprites[1]);
    aoe_task_list_free_ror(&l);
    aoe_task_list_free_ror(&l2);
    aoe_task_list_free_ror(&l3);
    assert(l3.tasks == NULL && l3.count == 0);
    return 0;
}
