#include "cpx.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { CPX_TABLE_OFFSET = 0x108, CPX_RECORD_SIZE = 0x208,
       CPX_MAX_SCENARIOS = 64 };

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int read_at(FILE *file, uint64_t offset, void *data, size_t size) {
    if (offset > LONG_MAX || fseek(file, (long)offset, SEEK_SET)) return 0;
    return fread(data, 1, size, file) == size;
}

void cpx_close(CpxArchive *archive) {
    if (!archive) return;
    if (archive->file) fclose(archive->file);
    free(archive->scenarios);
    memset(archive, 0, sizeof(*archive));
}

int cpx_open(CpxArchive *archive, const char *path) {
    uint8_t header[CPX_TABLE_OFFSET], record[CPX_RECORD_SIZE], version[4];
    uint64_t table_end;
    if (!archive || !path) return 0;
    memset(archive, 0, sizeof(*archive));
    archive->file = fopen(path, "rb");
    if (!archive->file) return 0;
    if (fseek(archive->file, 0, SEEK_END)) goto fail;
    long end = ftell(archive->file);
    if (end < 0) goto fail;
    archive->file_size = (uint64_t)end;
    if (archive->file_size < sizeof(header) ||
        !read_at(archive->file, 0, header, sizeof(header)) ||
        memcmp(header, "1.00", 4)) goto fail;
    memcpy(archive->title, header + 4, 28);
    archive->count = le32(header + 0x104);
    if (!archive->count || archive->count > CPX_MAX_SCENARIOS) goto fail;
    table_end = CPX_TABLE_OFFSET + (uint64_t)archive->count * CPX_RECORD_SIZE;
    if (table_end > archive->file_size) goto fail;
    archive->scenarios = calloc(archive->count, sizeof(CpxScenario));
    if (!archive->scenarios) goto fail;
    for (uint32_t i = 0; i < archive->count; ++i) {
        CpxScenario *scenario = &archive->scenarios[i];
        if (!read_at(archive->file, CPX_TABLE_OFFSET + (uint64_t)i * CPX_RECORD_SIZE,
                     record, sizeof(record))) goto fail;
        scenario->size = le32(record);
        scenario->offset = le32(record + 4);
        memcpy(scenario->name, record + 8, 255);
        memcpy(scenario->filename, record + 0x107, 257);
        if (scenario->size < 8 || scenario->offset < table_end ||
            scenario->offset > archive->file_size ||
            scenario->size > archive->file_size - scenario->offset ||
            !memchr(scenario->name, 0, 255) ||
            !memchr(scenario->filename, 0, 257) ||
            !read_at(archive->file, scenario->offset, version, 4) ||
            (memcmp(version, "1.10", 4) && memcmp(version, "1.11", 4))) goto fail;
    }
    return 1;
fail:
    cpx_close(archive);
    return 0;
}

int cpx_extract(CpxArchive *archive, uint32_t index, const char *path) {
    uint8_t buffer[65536];
    FILE *output;
    uint32_t remaining;
    if (!archive || !archive->file || !path || index >= archive->count) return 0;
    if (fseek(archive->file, (long)archive->scenarios[index].offset, SEEK_SET))
        return 0;
    output = fopen(path, "wb");
    if (!output) return 0;
    remaining = archive->scenarios[index].size;
    while (remaining) {
        size_t chunk = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        if (fread(buffer, 1, chunk, archive->file) != chunk ||
            fwrite(buffer, 1, chunk, output) != chunk) break;
        remaining -= (uint32_t)chunk;
    }
    if (fclose(output)) return 0;
    return remaining == 0;
}
