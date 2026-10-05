#ifndef VITA_AOE_UNIT_TEMPLATE_H
#define VITA_AOE_UNIT_TEMPLATE_H

#include <stdint.h>
#include <stdio.h>

#include "player_outcome.h" /* AoeOutcomeStream */

/* Patched Rise of Rome 1.0a unit templates (the master objects), types
   10 (vtable 0x00544534), 20 (0x0054440c), 25 (0x00544484), 30
   (0x005444fc), 40 (0x005443cc), 50 (0x00544444), 60 (0x005444c0), and
   the RoR 70 (0x00549970), 80 (0x00549930) and 90 (0x005499bc, a type
   10 with its own instances):
   each type's copy, binary load, text load and save of its own fields
   after its parent's, the attribute overrides, the clone and the
   instance factory. Sprites and sounds are the world's tables
   (resolved by index on load, written back by ID); the type 40 task
   list and the instances are ops. Semantic storage; offsets noted. */

typedef struct {
    void *sprite;       /* +0 */
    uint8_t percent;    /* +4 */
    uint8_t apply;      /* +5 */
} AoeTemplateDamageRor;

typedef struct {
    int16_t kind, amount;
} AoeTemplatePairRor;

typedef struct AoeTemplateRor {
    uint8_t type;                       /* +0x04 */
    char *name;                         /* +0x08 */
    int16_t id;                         /* +0x0c */
    int16_t field_e;                    /* +0x0e */
    int16_t field_10, field_12;         /* +0x10 (the table index), +0x12 */
    int16_t unit_class;                 /* +0x14 */
    void *sprite_18, *sprite_1c, *sprite_20; /* standing, dying, undead */
    uint8_t field_24;
    int16_t hit_points;                 /* +0x26 */
    float line_of_sight;                /* +0x28 */
    uint8_t field_2c;
    float size[3];                      /* +0x30..+0x38 */
    void *sound_3c, *sound_40, *sound_44;
    int16_t dead_unit;                  /* +0x48 */
    uint8_t field_4a, field_4b;
    int16_t field_4c;
    uint8_t field_4e;
    int16_t field_50;
    uint8_t field_52;
    int16_t field_54, field_56, field_58, field_5a;
    float field_5c, field_60;
    uint8_t field_64, field_65;
    int16_t field_66;
    uint8_t field_68;
    int16_t storage_type[3];            /* +0x6a */
    float storage_amount[3];            /* +0x70 */
    int16_t field_7c;
    float field_80, field_84;
    uint8_t storage_mode[3];            /* +0x88 */
    uint8_t field_8b[5];                /* +0x8b..+0x8f */
    uint8_t field_90, field_91, field_92;
    uint8_t damage_count;               /* +0x93 */
    AoeTemplateDamageRor *damage;       /* +0x94 */
    int32_t field_98, field_9c, field_a0;
    uint8_t recycle;                    /* +0xa4 */
    uint8_t field_a5, field_a6, field_a7, field_a8, field_a9;
    float field_ac, field_b0, field_b4;
    /* 20 */
    float speed;                        /* +0xb8 */
    /* 30 */
    void *sprite_bc, *sprite_c0;
    float rotation;                     /* +0xc4 */
    uint8_t field_c8;
    int16_t field_ca;
    uint8_t field_cc;
    float field_d0;
    uint8_t field_d4;
    /* 40 */
    void *tasks;                        /* +0xd8 */
    int16_t default_task;               /* +0xdc */
    float search_radius, work_rate;     /* +0xe0, +0xe4 */
    int16_t drop_site[2];               /* +0xe8, +0xea */
    uint8_t field_ec;
    void *sound_f0, *sound_f4;
    uint8_t field_f8;
    /* 50 */
    void *sprite_fc;
    uint8_t field_100;
    int16_t attack_count;               /* +0x102 */
    AoeTemplatePairRor *attacks;        /* +0x104 */
    int16_t armor_count;                /* +0x108 */
    AoeTemplatePairRor *armors;         /* +0x10c */
    int16_t field_110;
    float field_114, field_118;
    uint8_t field_11c;
    float field_120;
    int16_t field_124, field_126;
    uint8_t field_128;
    int16_t field_12a;
    float field_12c[3];
    float field_138;
    int16_t max_attack, max_armor;      /* +0x13c, +0x13e */
    float field_140, field_144;
    /* 60 */
    uint8_t field_148[5];
    float field_150;
    /* 70 (shares +0x148 with 60 in the original; kept apart here) */
    struct {
        int16_t type, amount;
        uint8_t flag, pad;
    } costs[3];                         /* +0x148, 6 bytes each */
    int16_t train_time;                 /* +0x15a */
    int16_t train_location;             /* +0x15c */
    uint8_t button;                     /* +0x15e */
    int16_t pierce_armor;               /* +0x160 */
    /* 80 */
    void *sound_164;                    /* +0x164 */
    void *sprite_168;                   /* +0x168 */
    uint8_t field_16c;
    int16_t field_16e;
    uint8_t field_170;
    int16_t field_172, field_174, field_176, field_178;
} AoeTemplateRor;

typedef struct {
    void *context;
    /* IDs written for the world's sprites (+0x72) and sounds (+0x0c). */
    int16_t (*sprite_id)(void *context, const void *sprite);
    int16_t (*sound_id)(void *context, const void *sound);
    /* Type 40's task list: 0x004e7ae0 copy, 0x004e7b60 binary,
       0x004e7c00 text, 0x004e7cb0 save, 0x004e7ad0 free; its count and
       whether task i is a default (+0x04). */
    void *(*tasks_copy)(void *context, const void *tasks);
    void *(*tasks_read)(void *context, AoeOutcomeStream *s, void **sprites, void **sounds);
    void *(*tasks_text)(void *context, FILE *f, void **sprites, void **sounds);
    void (*tasks_write)(void *context, const void *tasks, AoeOutcomeStream *s);
    void (*tasks_free)(void *context, void *tasks);
    int32_t (*task_count)(void *context, const void *tasks);
    int (*task_flag)(void *context, const void *tasks, int32_t i);
    /* The instance factory (vtable +0x18): a recycled instance of the
       kind from the world (0x00520b10, re-initialised with +0x04) when
       +0xa4 is set, else a new one of the type's class. */
    void *(*recycled)(void *context, uint8_t kind);
    void *(*reinit)(void *context, void *instance, AoeTemplateRor *t, void *player, float x,
                    float y, float z);
    void *(*instance_new)(void *context, AoeTemplateRor *t, void *player, float x, float y,
                          float z);
} AoeTemplateOpsRor;

/* Each type's own fields after its parent's:
   copy  10 0x004410b0 (ctor 0x00440ff0), 20 0x0043eaa0 (0x0043e930),
         25 0x00440140 (0x0043ffd0), 30 0x00440a80 (0x00440910), 40
         0x0043e180 (0x0043e010), 50 0x0043ef00 (0x0043ed90), 60
         0x004404c0 (0x00440350);
   load  10 0x004413b0 (ctor 0x00441040), 20 0x0043ead0 (0x0043e9b0), 25
         0x00440160 (0x00440050), 30 0x00440b00 (0x00440990), 40
         0x0043e230 (0x0043e090), 50 0x0043f0c0 (0x0043ee10), 60
         0x00440530 (0x004403d0);
   text  10 0x00441930 (ctor 0x00441070), 20 0x0043eb10 (0x0043ea20), 25
         0x00440190 (0x004400c0), 30 0x00440bf0 (0x00440a00), 40
         0x0043e350 (0x0043e100), 50 0x0043f370 (0x0043ee80), 60
         0x004405c0 (0x00440440);
   save  10 0x00442500, 20 0x0043ed60 (25 0x00440340), 30 0x00440f20, 40
         0x0043e810, 50 0x0043fd60, 60 0x004408a0;
   free  10 0x00441dd0 (thunk 0x004ed2b0; deleting 0x00441020), 20 0x0043eb60 (0x0043e990),
         25 0x004401d0 (0x00440030), 30 0x00440cc0 (0x00440970), 40
         0x0043e460 (0x0043e070), 50 0x0043f5a0 (0x0043edf0), 60
         0x00440660 (0x004403b0);
   and for RoR: 70 copy 0x004ecb80 (ctor 0x004eca10), load 0x004ecc10
   (0x004eca90), text 0x004ecd30 (0x004ecb00), save 0x004ed0b0, free
   0x004ece40 (0x004eca70); 80 copy 0x004ec270 (0x004ec100), load
   0x004ec310 (0x004ec180), text 0x004ec410 (0x004ec1f0), save
   0x004ec600, free 0x004ec4e0 (0x004ec160); 90 copy 0x004ed220, load
   0x004ed2c0, text 0x004ed340 (the type 10 fields as type 0x5a), free
   0x004ed290. 70 from 7.01 reads +0x160; before, +0x160 is the largest
   attack of kind 3 and +0x13c the largest other (or +0x100).
   Version gates on load: 10 +0xa8..+0xb4 from 7.04 (else the size) and
   +0x12 from 7.12; 40 the second sound from 7.04 (else the first); 50
   +0x13c..+0x144 from 7.04 (else the largest attack / armor amounts,
   +0x114 and +0x120). */
int aoe_template_copy_ror(AoeTemplateRor *t, const AoeTemplateRor *src, const AoeTemplateOpsRor *ops);
int aoe_template_load_ror(AoeTemplateRor *t, uint8_t type, AoeOutcomeStream *s, void **sprites,
                          void **sounds, float version, const AoeTemplateOpsRor *ops);
int aoe_template_text_ror(AoeTemplateRor *t, uint8_t type, FILE *f, void **sprites, void **sounds,
                          int16_t index, const AoeTemplateOpsRor *ops);
void aoe_template_save_ror(const AoeTemplateRor *t, AoeOutcomeStream *s, const AoeTemplateOpsRor *ops);
void aoe_template_release_ror(AoeTemplateRor *t, const AoeTemplateOpsRor *ops);

/* Clones (+0x1c): 10 0x00441ee0, 20 0x0043ec40, 25 0x004402c0, 30
   0x00440da0, 40 0x0043e600, 50 0x0043f6c0, 60 0x00440740, 70 0x004ed1b0,
   80 0x004ec7d0, 90 0x004ed440. */
AoeTemplateRor *aoe_template_clone_ror(const AoeTemplateRor *t, const AoeTemplateOpsRor *ops);
/* Instances (+0x18): 10 0x00441e10, 20 0x0043eb70, 25 0x004401e0, 30
   0x00440cd0, 40 0x0043e530, 50 0x0043f5f0, 60 0x00440670, 70 0x004ed130,
   80 0x004ec750 (and +0x3c 0x004ec6d0 with one more argument), 90
   0x004ed3c0. */
void *aoe_template_create_ror(AoeTemplateRor *t, void *player, float x, float y, float z,
                              const AoeTemplateOpsRor *ops);

/* Attributes: set (+0x08), add (+0x0c), multiply (+0x10) for the
   type's own attribute, else the parent's (base 0x004422d0 / 0x00442370
   / 0x00442420, given as `base`): 20 attribute 5 the speed (0x0043ecd0,
   0x0043ed00, 0x0043ed30); 30 attribute 6 the rotation (add 0x00440ec0,
   multiply 0x00440ef0); 40 attribute 13 the work rate (add 0x0043e7b0,
   multiply 0x0043e7e0); 60 attribute 19 sets +0x149 (0x00440840 add,
   0x00440870 multiply; both then the parent's). `op` 0 set, 1 add, 2
   multiply. Returns 1 when handled here. */
int aoe_template_set_attribute_ror(AoeTemplateRor *t, uint8_t attribute, float v, int op);
/* 0x0043e6a0 (type 40): the task with ID `id` (+0x06) in the list; NULL
   for -1 or none. */
const void *aoe_template_find_task_ror(const AoeTemplateRor *t, int32_t id,
                                       int16_t (*task_id)(void *context, const void *tasks,
                                                          int32_t i),
                                       const void *(*task_at)(void *context, const void *tasks,
                                                              int32_t i),
                                       const AoeTemplateOpsRor *ops);

#endif
