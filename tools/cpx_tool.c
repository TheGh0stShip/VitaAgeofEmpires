#include "cpx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    CpxArchive archive;
    int ok = 1;
    if (!((argc == 3 && !strcmp(argv[1], "list")) ||
          (argc == 5 && !strcmp(argv[1], "extract")))) {
        fprintf(stderr, "usage: cpx_tool list CAMPAIGN.CPX | "
                        "extract CAMPAIGN.CPX INDEX OUTPUT.scx\n");
        return 2;
    }
    if (!cpx_open(&archive, argv[2])) {
        fprintf(stderr, "cannot read 1.00 campaign container\n");
        return 1;
    }
    if (argc == 3) {
        printf("%s: %u scenarios\n", archive.title, archive.count);
        for (uint32_t i = 0; i < archive.count; ++i)
            printf("%u: %s (%s), %u bytes at 0x%x\n", i,
                   archive.scenarios[i].name, archive.scenarios[i].filename,
                   archive.scenarios[i].size, archive.scenarios[i].offset);
    } else {
        char *end;
        unsigned long index = strtoul(argv[3], &end, 0);
        if (*end || index >= archive.count ||
            !cpx_extract(&archive, (uint32_t)index, argv[4])) {
            fprintf(stderr, "cannot extract scenario\n");
            ok = 0;
        }
    }
    cpx_close(&archive);
    return ok ? 0 : 1;
}
