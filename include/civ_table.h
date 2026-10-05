#ifndef VITA_AOE_CIV_TABLE_H
#define VITA_AOE_CIV_TABLE_H

#include <stdint.h>
#include <stdio.h>

#include "player_outcome.h" /* AoeOutcomeStream */
#include "unit_template.h"

/* Patched Rise of Rome 1.0a civilisation (vtable 0x00544ab0, 0x2c bytes,
   0x0044f1b0..0x0044fa10): a name, the starting attribute values and the
   master templates (types 10..60 here; the RoR world's own civ handles
   the rest). Semantic storage; offsets noted. */

typedef struct {
    char name[21];               /* +0x04 (20 bytes on disk) */
    int16_t template_count;      /* +0x18 */
    AoeTemplateRor **templates;  /* +0x1c */
    int16_t value_count;         /* +0x20 */
    float *values;               /* +0x24 */
    uint8_t field_28;            /* +0x28 */
    uint8_t field_29;            /* +0x29 */
    int16_t field_2a;            /* +0x2a */
} AoeCivRor;

/* 0x0044f1b0 from text ("%s %hd %hd" name, +0x2a, value count; "%hd"
   listed, then "%hd %f" index and value each; "%hd" +0x28), no
   templates; 0x0044f2a0 binary (name 20, i16 value count, i16 +0x2a, the
   values, u8 +0x28). 0 on a failed allocation. */
int aoe_civ_from_text_ror(AoeCivRor *c, FILE *f);
int aoe_civ_read_ror(AoeCivRor *c, AoeOutcomeStream *s);
/* +0x0c 0x0044f700: the template table of `count` (empty slots).
   +0x04 0x0044f3a0: i16 count, a presence dword per slot, then per
   present slot its type byte and +0x08 0x0044f440: a template of that
   type (10, 20, 25, 30, 40, 50 or 60; others leave the slot empty) read
   from the stream. +0x10 0x0044f730: the same from text with the
   template's index. */
int aoe_civ_set_template_count_ror(AoeCivRor *c, int16_t count);
int aoe_civ_read_templates_ror(AoeCivRor *c, AoeOutcomeStream *s, void **sprites, void **sounds,
                               float version, const AoeTemplateOpsRor *ops);
int aoe_civ_read_template_ror(AoeCivRor *c, AoeOutcomeStream *s, uint8_t type, void **sprites,
                              void **sounds, int16_t slot, float version,
                              const AoeTemplateOpsRor *ops);
int aoe_civ_text_template_ror(AoeCivRor *c, FILE *f, uint8_t type, void **sprites, void **sounds,
                              int16_t slot, const AoeTemplateOpsRor *ops);
/* +0x14 0x0044fa10: u8 +0x29, name 20, i16 value count, i16 +0x2a, the
   values, u8 +0x28, i16 template count, a presence dword per slot (1 or
   0 here: the original writes its pointers) and each present
   template's save. */
void aoe_civ_save_ror(const AoeCivRor *c, AoeOutcomeStream *s, const AoeTemplateOpsRor *ops);
/* 0x0044f320 (deleting 0x0044f280): the values and every template freed. */
void aoe_civ_release_ror(AoeCivRor *c, const AoeTemplateOpsRor *ops);

#endif
