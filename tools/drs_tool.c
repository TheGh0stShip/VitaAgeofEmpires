#include "drs.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void show_type(uint32_t type, char out[5]) {
    for (int i = 0; i < 4; ++i) out[i] = (char)(type >> (8 * i));
    out[4] = 0;
}

int main(int argc, char **argv) {
    DrsArchive archive;
    if (argc < 3 || (strcmp(argv[1], "list") && strcmp(argv[1], "extract"))) {
        fprintf(stderr, "usage: drs_tool list ARCHIVE | extract ARCHIVE TYPE ID OUTPUT\n");
        return 2;
    }
    if (!drs_open(&archive, argv[2])) {
        fprintf(stderr, "cannot open valid DRS archive: %s\n", argv[2]);
        return 1;
    }
    if (!strcmp(argv[1], "list")) {
        printf("%zu tables, %zu entries\n", archive.table_count, archive.entry_count);
        for (size_t i = 0; i < archive.entry_count; ++i) {
            const DrsEntry *e = &archive.entries[i];
            char type[5];
            show_type(e->type, type);
            printf("%4.4s %u %u %u\n", type, e->id, e->offset, e->size);
        }
    } else {
        char *end;
        unsigned long id;
        const DrsEntry *entry;
        unsigned char *data;
        FILE *out;
        if (argc != 6 || strlen(argv[3]) != 4) {
            fprintf(stderr, "extract needs a four-byte TYPE, ID and OUTPUT\n");
            drs_close(&archive);
            return 2;
        }
        errno = 0;
        id = strtoul(argv[4], &end, 0);
        if (errno || *end || id > UINT32_MAX ||
            !(entry = drs_find(&archive, drs_type(argv[3]), (uint32_t)id))) {
            fprintf(stderr, "resource not found or invalid ID\n");
            drs_close(&archive);
            return 1;
        }
        data = malloc(entry->size ? entry->size : 1);
        if (!data || !drs_read(&archive, entry, data, entry->size)) {
            fprintf(stderr, "resource read failed\n");
            free(data);
            drs_close(&archive);
            return 1;
        }
        out = fopen(argv[5], "wb");
        if (!out || fwrite(data, 1, entry->size, out) != entry->size) {
            fprintf(stderr, "resource write failed\n");
            if (out) fclose(out);
            free(data);
            drs_close(&archive);
            return 1;
        }
        fclose(out);
        free(data);
    }
    drs_close(&archive);
    return 0;
}
