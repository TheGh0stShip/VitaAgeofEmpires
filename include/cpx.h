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

/* Original Rise of Rome 1.00 campaign container, as the campaign loader
   reads it (0x0041c2d0, translated in campaign_progress.c): a 0x108-byte
   header ("1.00", the title, the scenario count at +0x104), then a
   0x208-byte entry per scenario (data size, data offset, name, file name
   at +0x107); the scenario's stream is the slice at that offset
   (0x0041c700). Scenario files are embedded verbatim. */
int cpx_open(CpxArchive *archive, const char *path);
void cpx_close(CpxArchive *archive);
int cpx_extract(CpxArchive *archive, uint32_t index, const char *path);

#endif
