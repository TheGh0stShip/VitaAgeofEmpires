#ifndef VITA_AOE_CPX_H
#define VITA_AOE_CPX_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t size;
    uint32_t offset;
    char name[256];
    char filename[258];
} CpxScenario;

typedef struct {
    FILE *file;
    uint64_t file_size;
    char title[29];
    uint32_t count;
    CpxScenario *scenarios;
} CpxArchive;

/* Original Rise of Rome 1.00 campaign container. Scenario files are
   embedded verbatim at the offsets recorded in its 0x208-byte table. */
int cpx_open(CpxArchive *archive, const char *path);
void cpx_close(CpxArchive *archive);
int cpx_extract(CpxArchive *archive, uint32_t index, const char *path);

#endif
