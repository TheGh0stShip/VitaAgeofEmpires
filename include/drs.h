#ifndef VITA_AOE_DRS_H
#define VITA_AOE_DRS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* A resource archive as the executable registers it (patched RoR 1.0a
   0x0046afc0): a 0x40-byte header whose +0x2c holds the group name
   ("tribe") it compares, the table count at +0x38 and the first data
   offset at +0x3c, then 12-byte table records (type, index offset,
   count) and 12-byte entries (ID, offset, size); lookups search the
   registered archives in order (0x0046b280). */

typedef struct {
    uint32_t type;
    uint32_t id;
    uint32_t offset;
    uint32_t size;
} DrsEntry;

typedef struct {
    FILE *file;
    uint64_t file_size;
    DrsEntry *entries;
    size_t entry_count;
    size_t table_count;
} DrsArchive;

typedef struct {
    DrsArchive *archive;
    const DrsEntry *entry;
} DrsMatch;

/* DRS type bytes are stored in the order used by the Windows executable. */
uint32_t drs_type(const char text[4]);
int drs_open(DrsArchive *archive, const char *path);
void drs_close(DrsArchive *archive);
const DrsEntry *drs_find(const DrsArchive *archive, uint32_t type, uint32_t id);
/* Patched RoR 1.0a 0x0046b280 (disc 0x0046c450): search registered archives in append order and
   return the first matching type/ID. Pass expansion archives before base. */
int drs_find_first(DrsArchive *const *archives, size_t count,
                   uint32_t type, uint32_t id, DrsMatch *match);
int drs_read(DrsArchive *archive, const DrsEntry *entry, void *buffer, size_t capacity);

#endif
