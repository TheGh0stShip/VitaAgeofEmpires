#include "drs.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void put32(unsigned char *p, uint32_t value) {
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
}

static int write_fixture(const char *path, const unsigned char *bytes,
                         size_t size) {
    FILE *file = fopen(path, "wb");
    int ok = file && fwrite(bytes, 1, size, file) == size;
    if (file) fclose(file);
    return ok;
}

int main(void) {
    const char *path = "drs_format_fixture.bin";
    unsigned char bytes[91] = {0};
    unsigned char payload[3] = {0};
    DrsArchive archive = {0};
    const DrsEntry *entry;

    memcpy(bytes + 0x28, "1.00", 4);
    memcpy(bytes + 0x2c, "tribe", 5);
    put32(bytes + 0x38, 1);       /* one table */
    put32(bytes + 0x3c, 0x58);    /* first resource byte */

    memcpy(bytes + 0x40, " pls", 4);
    put32(bytes + 0x44, 0x4c);    /* index table */
    put32(bytes + 0x48, 1);

    put32(bytes + 0x4c, 149);
    put32(bytes + 0x50, 0x58);
    put32(bytes + 0x54, 3);
    bytes[0x58] = 0xaa;
    bytes[0x59] = 0xbb;
    bytes[0x5a] = 0xcc;

    assert(write_fixture(path, bytes, sizeof(bytes)));
    assert(drs_open(&archive, path));
    assert(archive.table_count == 1 && archive.entry_count == 1);
    entry = drs_find(&archive, drs_type(" pls"), 149);
    assert(entry && entry->offset == 0x58 && entry->size == 3);
    assert(drs_read(&archive, entry, payload, sizeof(payload)));
    assert(payload[0] == 0xaa && payload[1] == 0xbb && payload[2] == 0xcc);
    assert(!drs_find(&archive, drs_type(" vaw"), 149));
    drs_close(&archive);

    memcpy(bytes + 0x28, "1.01", 4);
    assert(write_fixture(path, bytes, sizeof(bytes)));
    assert(!drs_open(&archive, path));

    memcpy(bytes + 0x28, "1.00", 4);
    put32(bytes + 0x54, 32);      /* resource extends past EOF */
    assert(write_fixture(path, bytes, sizeof(bytes)));
    assert(!drs_open(&archive, path));

    remove(path);
    return 0;
}
