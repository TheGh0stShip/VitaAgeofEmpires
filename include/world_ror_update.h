#ifndef VITA_AOE_WORLD_ROR_UPDATE_H
#define VITA_AOE_WORLD_ROR_UPDATE_H

#include <stddef.h>
#include <stdint.h>

#include "dat.h"

/* Patched Rise of Rome 1.0a tech list from its text file (constructor
   0x004e9fa4, 0x004e9fa4..0x004ea348; the world's text loader 0x00509ec0
   allocates the 12-byte object with operator new and stores it at world
   +0x104). The object is the same tech list the binary reader 0x4e9c60
   (src/dat_tech.c), the save 0x4ea40d and the release 0x4ea348 use, so
   world +0x104 is the tech tree:
     +0x00 the records, +0x04 the count (the low int16 is used), +0x08 the
           world (set only by the binary reader, read by 0x4ea3bc).
   Each record is 0x3c bytes; DatTech holds it:
     +0x00 name (char *)          +0x04 required_techs[4]
     +0x0c required_count         +0x0e cost resource_type[3]
     +0x14 cost amount[3]         +0x1a cost paid byte[3]
     +0x1e research_time          +0x20 effect_id
     +0x22 field_22               +0x24 field_24
     +0x26 button_id (byte)       +0x28 research_location
     +0x2a language_name          +0x2c language_description
     +0x30 language_help          +0x34 language_tree
     +0x38 hotkey (int32 each).

   The text (fopen "r" 0x0055a1bc, fscanf 0x5277d7, every scan's result
   but the first ignored):
     "%d %d"  (0x0055a1c0) the list size and the number of listed techs;
              nothing more unless that scan returned something other
              than EOF and the low int16 of the size is positive, then
              calloc(0x3c, size);
   per listed tech:
     "%d"     (0x0055a1c8) the record index (its low int16, unchecked);
     "%s %hd %hd %hd %hd %hd %hd %hd" (0x0055a1cc) the name into a
              100-byte buffer, +0x0c, +0x1e, +0x20, +0x22, +0x24, the
              button (its low byte to +0x26), +0x28; underscores in the
              name become spaces and +0x00 is set to 0 then to a copy
              (0x00450ea0);
     "%hd" x4 (0x0055a1ec) +0x04..+0x0a;
     "%hd %hd %hd" x3 (0x0055a1f0) a cost's type, amount and paid (its
              low byte to +0x1a + i);
     "%hd %hd %d %d %d" (0x0055a1fc) +0x2a, +0x2c, +0x30, +0x34, +0x38.
   The index, name, button and paid locals keep their last values, so a
   listed tech the file no longer has rewrites the last record's name,
   button and every paid byte with the last values read.

   Port: `text` is the file's bytes (NULL = fopen failed) and the scans
   follow fscanf: white space skipped, %d/%hd an optional sign and
   decimal digits kept to 32 bits (%hd stores the low 16), %s a word.
   The locals start at 0 (the original's stack is uninitialised); a
   name longer than 99 characters keeps its first 99 (the original
   overruns its buffer); an index outside the list goes to a scratch
   record (the original writes outside its allocation); a re-listed
   index frees its old name (the original leaks it); techs->count is the
   positive list size, else 0 (the original keeps the raw int32).
   Returns 0 for a NULL text or a failed list allocation, else 1.
   Release with dat_free_techs (as 0x4ea348 frees the names, then the
   list). */
int aoe_techs_read_text_ror(DatTechs *techs, const char *text, size_t size);

/* The whole of 0x004e9fa4 for a file path: reads the file ("r") and
   parses it as aoe_techs_read_text_ror; a missing file leaves no techs
   and returns 0. */
int aoe_techs_load_text_file_ror(DatTechs *techs, const char *path);

#endif
