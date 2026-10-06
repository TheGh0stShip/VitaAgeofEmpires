#ifndef VITA_AOE_PE_STRINGS_H
#define VITA_AOE_PE_STRINGS_H

#include <stddef.h>
#include <stdint.h>

/* String tables (RT_STRING resources) of a user-supplied Win32 language
   library (languagex.dll / language.dll), read without Windows. The
   library bytes are untrusted: every read is bounded and a malformed
   file loads as an empty table. Integration support, not an original
   function: it reproduces the LoadStringA behaviour the game relies on. */

typedef struct {
    uint8_t *data;   /* the whole file (owned) */
    size_t size;
    /* The RT_STRING type directory: file offset of the resource section
       data at RVA `rsrc_rva`, and the type's subdirectory offset inside it
       (0 when the file has no string table). */
    size_t rsrc_offset, rsrc_size;
    uint32_t rsrc_rva;
    uint32_t string_dir;
    /* The section table, for mapping data-entry RVAs to file offsets. */
    const uint8_t *sections;
    uint16_t section_count;
} AoePeStringsRor;

/* Takes ownership of `data` (malloc'd, freed by aoe_pe_strings_free);
   returns 0 when it is not a PE image with a resource section (the table
   is then empty but still freed safely). */
int aoe_pe_strings_open(AoePeStringsRor *t, uint8_t *data, size_t size);
/* Reads `path` and opens it; 0 when the file cannot be read or parsed. */
int aoe_pe_strings_load(AoePeStringsRor *t, const char *path);
void aoe_pe_strings_free(AoePeStringsRor *t);

/* LoadStringA(module, id, out, size): only the low 16 bits of `id` count
   (so 0x1d4ed names 0xd4ed); string block (id >> 4) + 1, entry id & 15,
   first language. The UTF-16 text is
   converted to Windows-1252 ('?' for anything else), truncated to
   size - 1 bytes and terminated. Returns the bytes copied; 0 for a
   missing or empty string (then `out` is left untouched, as LoadStringA
   does), or when size < 1. */
int32_t aoe_pe_load_string_ror(const AoePeStringsRor *t, int32_t id, char *out, int32_t size);

/* The game's string lookup (game vtable +0x20): Rise of Rome 1.0a
   0x004ff530 tries the expansion library and, when that yields nothing,
   0x00417490 the base library, which writes "" when it fails too; the
   last byte of `out` is always cleared. Either library may be NULL. */
void aoe_game_string_ror(const AoePeStringsRor *expansion, const AoePeStringsRor *base, int32_t id,
                         char *out, int32_t size);

#endif
