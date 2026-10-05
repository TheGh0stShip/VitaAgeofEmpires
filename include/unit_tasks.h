#ifndef VITA_AOE_UNIT_TASKS_H
#define VITA_AOE_UNIT_TASKS_H

#include <stdint.h>
#include <stdio.h>

#include "player_outcome.h" /* AoeOutcomeStream */

/* Patched Rise of Rome 1.0a unit tasks (0x004e73b0..0x004e7d50; the type
   40 template's +0xd8 list, vtable 0x005498e4): each task's fields, its
   four graphics (sprites) and two sounds, copied, loaded from the data
   file or a text list, and saved. Semantic storage; offsets noted. */

typedef struct {
    int16_t type;            /* +0x00: 1 */
    int16_t id;              /* +0x02 */
    uint8_t field_4;
    int16_t field_6, field_8, field_a;
    uint8_t field_c, field_d;
    int16_t field_e;
    uint8_t field_10, field_11, field_12;
    int16_t field_14, field_16, field_18, field_1a;
    float field_1c, field_20, field_24;
    uint8_t field_28;
    float field_2c;
    int16_t field_30, field_32;
    void *graphics[4];       /* +0x34..+0x40 */
    void *sounds[2];         /* +0x44, +0x48 */
} AoeTaskRor;

typedef struct {
    AoeTaskRor **tasks;      /* +0x04 */
    int16_t count;           /* +0x08 */
} AoeTaskListRor;

typedef struct {
    void *context;
    int16_t (*sprite_id)(void *context, const void *sprite);  /* +0x72 */
    int16_t (*sound_id)(void *context, const void *sound);    /* +0x0c */
} AoeTaskOpsRor;

/* 0x004e7d50 (the list's factory): a new task of type 1 (0x004e73b0:
   the given ID, -1 in +0x0e and +0x14..+0x1a, the rest zero); NULL for
   other types. 0x004e7430 copies one task's fields (not its type or ID);
   0x004e74f0 loads them (the graphics and sounds by index, -1 none),
   0x004e7710 reads them from text, 0x004e78a0 saves them (type and ID
   first). */
AoeTaskRor *aoe_task_new_ror(int16_t type, int16_t id);
void aoe_task_copy_ror(AoeTaskRor *t, const AoeTaskRor *src);
void aoe_task_load_ror(AoeTaskRor *t, AoeOutcomeStream *s, void **sprites, void **sounds);
void aoe_task_text_ror(AoeTaskRor *t, FILE *f, void **sprites, void **sounds);
void aoe_task_save_ror(const AoeTaskRor *t, AoeOutcomeStream *s, const AoeTaskOpsRor *ops);

/* The list: 0x004e7cf0 emptied (0x004e7ad0 the destructor); 0x004e7ae0
   a copy (each task made by the factory and copied); 0x004e7b60 loaded
   (a count, then each task's type, then its fields); 0x004e7c00 read
   from text (the same with "%hd"); 0x004e7cb0 saved. */
void aoe_task_list_free_ror(AoeTaskListRor *l);
int aoe_task_list_copy_ror(AoeTaskListRor *l, const AoeTaskListRor *src);
int aoe_task_list_load_ror(AoeTaskListRor *l, AoeOutcomeStream *s, void **sprites, void **sounds);
int aoe_task_list_text_ror(AoeTaskListRor *l, FILE *f, void **sprites, void **sounds);
void aoe_task_list_save_ror(const AoeTaskListRor *l, AoeOutcomeStream *s, const AoeTaskOpsRor *ops);

#endif
