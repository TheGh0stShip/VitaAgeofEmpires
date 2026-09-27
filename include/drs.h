#ifndef VITA_AOE_DRS_H
#define VITA_AOE_DRS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

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
/* Empiresx.exe 0x0046c450: search registered archives in append order and
   return the first matching type/ID. Pass expansion archives before base. */
int drs_find_first(DrsArchive *const *archives, size_t count,
                   uint32_t type, uint32_t id, DrsMatch *match);
int drs_read(DrsArchive *archive, const DrsEntry *entry, void *buffer, size_t capacity);

#endif
