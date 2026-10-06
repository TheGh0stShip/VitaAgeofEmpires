#include "color_table.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void synthetic(void) {
    uint8_t out[256];
    char text[2048];
    size_t pos = 0;
    for (unsigned i = 0; i < 256; ++i) {
        int n = snprintf(text + pos, sizeof(text) - pos, "%u\r\n", i);
        assert(n > 0 && (size_t)n < sizeof(text) - pos);
        pos += (size_t)n;
    }
    memset(out, 77, sizeof(out));
    assert(aoe_color_table_parse((const uint8_t *)text, pos, out));
    for (unsigned i = 0; i < 256; ++i)
        assert(out[i] == i);
    memset(out, 77, sizeof(out));
    assert(!aoe_color_table_parse((const uint8_t *)text, pos - 5, out));
    for (unsigned i = 0; i < 256; ++i)
        assert(out[i] == 77);
    text[0] = 'x';
    assert(!aoe_color_table_parse((const uint8_t *)text, pos, out));
    text[0] = '2';
    text[1] = '5';
    text[2] = '6';
    assert(!aoe_color_table_parse((const uint8_t *)text, pos, out));
}

int main(int argc, char **argv) {
    synthetic();
    {
        DatColorRecord records[2] = {
            {.color_id = 0}, {.color_id = 1}
        };
        DatIntro intro = {.color_count = 2, .colors = records};
        AoeColorTableSet set;
        memcpy(records[0].name, "missing-a.col", 14);
        memcpy(records[1].name, "missing-b.col", 14);
        assert(aoe_color_table_set_load_directory(
            &intro, "/directory/that/does/not/exist", &set));
        assert(set.count == 2 && set.present[0] && set.present[1] &&
               !set.loaded[0] && !set.loaded[1] &&
               aoe_color_table_set_resolve(&set, 0) == set.tables[0] &&
               aoe_color_table_set_resolve(&set, 2) == NULL);
    }
    {
        const char *directory = getenv("AOE_COLOR_DIR");
        if (directory) {
            DatColorRecord record = {.color_id = 0};
            DatIntro intro = {.color_count = 1, .colors = &record};
            AoeColorTableSet set;
            memcpy(record.name, "shadow.col", 11);
            assert(aoe_color_table_set_load_directory(
                &intro, directory, &set));
            assert(set.loaded[0] && set.tables[0][0] == 187 &&
                   set.tables[0][10] == 170 && set.tables[0][255] == 255);
        }
    }
    if (argc == 1) return 0;
    if (argc > 9) return 2;
    DrsArchive archives[8] = {{0}};
    DrsArchive *stack[8];
    size_t count = (size_t)argc - 1, opened = 0;
    for (; opened < count; ++opened) {
        if (!drs_open(&archives[opened], argv[opened + 1])) break;
        stack[opened] = &archives[opened];
    }
    uint8_t table[256];
    int ok = opened == count &&
             aoe_shadow_table_load(stack, count, table) &&
             table[0] == 187 && table[10] == 170 &&
             table[50] == 56 && table[128] == 119 &&
             table[255] == 255;
    const unsigned expected_changes[12] = {
        10, 0, 10, 10, 0, 10, 10, 10, 25, 238, 10, 10
    };
    for (unsigned id = 0; id < 12 && ok; ++id) {
        uint8_t current[256];
        ok = aoe_color_table_load_drs(stack, count, 50900 + id,
                                      current);
        unsigned changed = 0;
        for (unsigned i = 0; i < 256 && ok; ++i)
            changed += current[i] != i;
        ok = ok && changed == expected_changes[id];
    }
    if (ok)
        printf("original shadow table: 0->%u, 10->%u, 50->%u, 128->%u, 255->%u\n",
               table[0], table[10], table[50], table[128], table[255]);
    for (size_t i = 0; i < opened; ++i)
        drs_close(&archives[i]);
    return ok ? 0 : 1;
}
