#include "drs.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static uint32_t le32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int read_at(FILE *file, uint64_t offset, void *buffer, size_t size) {
    if (offset > LONG_MAX || fseek(file, (long)offset, SEEK_SET) != 0)
        return 0;
    return fread(buffer, 1, size, file) == size;
}

uint32_t drs_type(const char text[4]) {
    return le32((const unsigned char *)text);
}

void drs_close(DrsArchive *archive) {
    if (!archive) return;
    if (archive->file) fclose(archive->file);
    free(archive->entries);
    memset(archive, 0, sizeof(*archive));
}

int drs_open(DrsArchive *archive, const char *path) {
    unsigned char header[0x40], table[12], item[12];
    uint32_t table_count, first_data;
    size_t total = 0;

    if (!archive || !path) return 0;
    memset(archive, 0, sizeof(*archive));
    archive->file = fopen(path, "rb");
    if (!archive->file) return 0;
    if (fseek(archive->file, 0, SEEK_END) != 0) goto fail;
    long end = ftell(archive->file);
    if (end < 0) goto fail;
    archive->file_size = (uint64_t)end;
    if (archive->file_size < sizeof(header) ||
        !read_at(archive->file, 0, header, sizeof(header))) goto fail;
    /* The game checks the 1.00 version and tribe signature at 0x28/0x2c. */
    if (memcmp(header + 0x28, "1.00", 4) != 0 ||
        memcmp(header + 0x2c, "tribe", 5) != 0) goto fail;
    table_count = le32(header + 0x38);
    first_data = le32(header + 0x3c);
    if (table_count > (archive->file_size - 0x40) / 12 ||
        first_data > archive->file_size) goto fail;
    archive->table_count = table_count;

    for (uint32_t i = 0; i < table_count; ++i) {
        uint32_t count, index_offset;
        if (!read_at(archive->file, 0x40u + (uint64_t)i * 12, table, 12)) goto fail;
        index_offset = le32(table + 4);
        count = le32(table + 8);
        if (index_offset > first_data || count > (first_data - index_offset) / 12 ||
            count > SIZE_MAX - total) goto fail;
        total += count;
    }
    if (total > SIZE_MAX / sizeof(DrsEntry)) goto fail;
    archive->entries = calloc(total ? total : 1, sizeof(DrsEntry));
    if (!archive->entries) goto fail;

    for (uint32_t i = 0; i < table_count; ++i) {
        uint32_t type, count, index_offset;
        if (!read_at(archive->file, 0x40u + (uint64_t)i * 12, table, 12)) goto fail;
        type = le32(table);
        index_offset = le32(table + 4);
        count = le32(table + 8);
        for (uint32_t j = 0; j < count; ++j) {
            DrsEntry *entry = &archive->entries[archive->entry_count];
            if (!read_at(archive->file, (uint64_t)index_offset + (uint64_t)j * 12,
                         item, 12)) goto fail;
            entry->type = type;
            entry->id = le32(item);
            entry->offset = le32(item + 4);
            entry->size = le32(item + 8);
            if (entry->offset < first_data || entry->offset > archive->file_size ||
                entry->size > archive->file_size - entry->offset) goto fail;
            archive->entry_count++;
        }
    }
    return 1;
fail:
    drs_close(archive);
    return 0;
}

const DrsEntry *drs_find(const DrsArchive *archive, uint32_t type, uint32_t id) {
    if (!archive) return NULL;
    for (size_t i = 0; i < archive->entry_count; ++i)
        if (archive->entries[i].type == type && archive->entries[i].id == id)
            return &archive->entries[i];
    return NULL;
}

int drs_find_first(DrsArchive *const *archives, size_t count,
                   uint32_t type, uint32_t id, DrsMatch *match) {
    if (!match) return 0;
    match->archive = NULL;
    match->entry = NULL;
    if (!archives) return 0;
    for (size_t i = 0; i < count; ++i) {
        const DrsEntry *entry = drs_find(archives[i], type, id);
        if (entry) {
            match->archive = archives[i];
            match->entry = entry;
            return 1;
        }
    }
    return 0;
}

int drs_read(DrsArchive *archive, const DrsEntry *entry, void *buffer, size_t capacity) {
    if (!archive || !archive->file || !entry || !buffer ||
        capacity < entry->size || entry->offset > archive->file_size ||
        entry->size > archive->file_size - entry->offset) return 0;
    return read_at(archive->file, entry->offset, buffer, entry->size);
}
