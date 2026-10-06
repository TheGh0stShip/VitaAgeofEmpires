/* Prints strings from user-supplied language libraries through the game's
   lookup: lang_tool EXPANSION.dll BASE.dll ID... (either path may be "-"). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pe_strings.h"

int main(int argc, char **argv) {
    AoePeStringsRor x, b;
    int i, ok_x, ok_b;
    if (argc < 4) {
        fprintf(stderr, "usage: %s EXPANSION.dll|- BASE.dll|- ID...\n", argv[0]);
        return 2;
    }
    memset(&x, 0, sizeof x);
    memset(&b, 0, sizeof b);
    ok_x = strcmp(argv[1], "-") != 0 && aoe_pe_strings_load(&x, argv[1]);
    ok_b = strcmp(argv[2], "-") != 0 && aoe_pe_strings_load(&b, argv[2]);
    if (!ok_x && !ok_b) {
        fprintf(stderr, "no string table could be read\n");
        return 1;
    }
    for (i = 3; i < argc; ++i) {
        char out[512];
        long id = strtol(argv[i], NULL, 0);
        aoe_lang_string_ror(ok_x ? &x : NULL, ok_b ? &b : NULL, (int32_t)id, out, (int32_t)sizeof out);
        printf("%ld\t%s\n", id, out);
    }
    aoe_pe_strings_free(&x);
    aoe_pe_strings_free(&b);
    return 0;
}
