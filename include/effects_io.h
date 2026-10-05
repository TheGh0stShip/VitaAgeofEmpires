#ifndef VITA_AOE_EFFECTS_IO_H
#define VITA_AOE_EFFECTS_IO_H

#include <stdint.h>

#include "aoe_stream.h"
#include "dat.h"

/* Patched Rise of Rome 1.0a effects object (base 0x0043b480..0x0043b840,
   tribe subclass 0x004ebac0..0x004ebb00): the effect list kept in a
   DatEffects. Each effect is a 31-byte name and commands of type (1 byte),
   unit, class, attribute (int16 each) and amount (float). */

/* 0x0043b480: from a stream (int32 count; per effect the name, an int16
   command count and the commands). 0x004ebac0 is the tribe constructor
   around it. Returns 0 on a short stream or allocation failure. */
int aoe_effects_read_ror(DatEffects *effects, AoeOutcomeStream *in);
int aoe_effects_tribe_read_ror(DatEffects *effects, AoeOutcomeStream *in);
/* 0x0043b5f0: from text ("%d %d" total and listed; per listed effect "%d"
   index, "%s %hd" name and command count, then "%hd %hd %hd %hd %f" per
   command; underscores in names become spaces). A missing file leaves no
   effects; parsing stops at the first failure. */
int aoe_effects_read_text_ror(DatEffects *effects, const char *text);
/* 0x0043b840: write as 0x0043b480 reads. */
void aoe_effects_write_ror(const DatEffects *effects, AoeOutcomeStream *out);
/* 0x0043b7d0 frees the commands and list; 0x0043b5d0 and the tribe's
   0x004ebae0 also free the object when asked (0x004ebb00 is the tribe's
   thunk to the base). */
void aoe_effects_release_ror(DatEffects *effects);
DatEffects *aoe_effects_destroy_ror(DatEffects *effects, int heap);
DatEffects *aoe_effects_tribe_destroy_ror(DatEffects *effects, int heap);
void aoe_effects_tribe_release_ror(DatEffects *effects);

/* 0x00450e00: underscores become spaces, in place. */
void aoe_text_underscores_to_spaces_ror(char *text);
/* 0x00450ea0: *field becomes a new copy of `text` (the old one freed);
   0x00450e20: a new copy of a followed by b. */
void aoe_text_set_ror(char **field, const char *text);
void aoe_text_set_joined_ror(char **field, const char *a, const char *b);

#endif
