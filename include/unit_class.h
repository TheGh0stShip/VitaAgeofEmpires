#ifndef VITA_AOE_UNIT_CLASS_H
#define VITA_AOE_UNIT_CLASS_H

#include <stdint.h>

#include "dat.h"

/* Patched Rise of Rome 1.0a class tests on unit classes (a template's
   +0x14, DatUnit.unit_class) and a few other small rule helpers. Class
   values are those of the data files: 1 artifact, 2 trade boat, 3
   building, 4 villager, 7 bush, 8 stone mine, 9 prey, 10 predator, 15
   tree, 20 transport boat, 21 fishing boat, 22 war boat, 27 wall, 32 gold
   mine. Results are 0 or 1 unless noted. */

/* 0x00410bd0 and 0x004e45d0 (two copies): 7, 8, 9, 10, 15 or 32. */
int aoe_class_is_resource_ror(int32_t unit_class);
/* 0x004be2b0: a unit whose class is 2, 20, 21 or 22 (the boats). */
int aoe_class_is_boat_ror(int32_t unit_class);
/* 0x004be220: class 3 or 27 (building or wall). */
int aoe_class_is_structure_ror(int32_t unit_class);
/* 0x004e62a0: 1 unless the class is 3, 27, 4, 21, 2 or 20;
   0x004e6f10: 1 unless it is 3, 27, 4, 21 or 2. */
int aoe_class_is_combatant_ror(int32_t unit_class);
int aoe_class_is_combatant_or_transport_ror(int32_t unit_class);
/* 0x004e6280: a unit whose class is not 1 (NULL is 0). */
int aoe_unit_not_artifact_ror(const DatUnit *unit_template);
/* 0x004e6ee0: a target unit that is not an artifact while the asking
   unit's state (+0x48) is at least 2. */
int aoe_unit_target_ready_ror(const DatUnit *target_template,
                              uint8_t own_state);

/* Value sets of other codes: 0x004b9030 101, 102 or 103; 0x004be200 1 or
   30; 0x004e4660 705, 709 or 713; 0x004e4fc0 703 or 704; 0x004e5eb0 31 or
   5; 0x004e5ed0 2 for 31, 5 or 33, else -1. */
int aoe_code_in_101_103_ror(int32_t code);
int aoe_code_1_or_30_ror(int32_t code);
int aoe_code_in_705_709_713_ror(int32_t code);
int aoe_code_703_or_704_ror(int32_t code);
int aoe_code_31_or_5_ror(int32_t code);
int32_t aoe_code_31_5_33_rank_ror(int32_t code);

/* 0x004c2dc0: 2 below 2, else the square. */
int32_t aoe_square_min2_ror(int32_t value);
/* 0x0042c340: the player activation message suffix. */
const char *aoe_activation_text_ror(int32_t active);
/* 0x0045b360: a bitmap row's byte width rounded up to four bytes. */
uint32_t aoe_dib_stride_ror(int32_t row_bytes);

#endif
