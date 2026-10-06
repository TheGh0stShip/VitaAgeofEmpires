#include "terrain_art.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void terrain_text_tests(void) {
    static TerrainTextRecord rec[32];
    static char text[8192];
    int32_t tables[4] = {100, 200, 300, 400};
    int16_t n = 0;
    size_t len = 0;
    int k;
    len += (size_t)snprintf(text + len, sizeof text - len, "2\n");
    len += (size_t)snprintf(text + len, sizeof text - len, "5 Deep_Water water.slp 77 9 1 2 3 2 4 6 1.5 2.5 7 8 9 10 11 12 13 2\n");
    for (k = 0; k < 19; ++k) len += (size_t)snprintf(text + len, sizeof text - len, "%d 4\n", k + 1);
    len += (size_t)snprintf(text + len, sizeof text - len, "3 33\n5 55\n");
    len += (size_t)snprintf(text + len, sizeof text - len, "40 Bad bad.slp 0 0 0 0 0 -1 0 0 0.0 0.0 0 0 0 0 0 0 0 1\n");
    for (k = 0; k < 20; ++k) len += (size_t)snprintf(text + len, sizeof text - len, "0 0\n");
    rec[5].used = 1; rec[7].used = 1;
    assert(terrain_text_load(rec, text, tables, &n) == 1 && n == 2);
    assert(rec[7].used == 0 && rec[5].used == 1 && strcmp(rec[5].name, "Deep Water") == 0 &&
           strcmp(rec[5].file_name, "water.slp") == 0 && rec[5].value_1c == 77 && rec[5].b29 == 1 &&
           rec[5].b28 == 2 && rec[5].b2a == 3 && rec[5].color_table == 300 && rec[5].b2f == 4 &&
           rec[5].w30 == 6 && rec[5].value_whole == 1 && rec[5].value == 1.5f && rec[5].value2 == 2.5f &&
           rec[5].w_b8 == 7 && rec[5].w_ba == 8 && rec[5].w_bc == 9 && rec[5].b2b == 10 && rec[5].b2c == 11 &&
           rec[5].b2e == 12 && rec[5].b2d == 13);
    assert(rec[5].shapes[0].count == 1 && rec[5].shapes[0].offset == 0 && rec[5].shapes[1].offset == 4 &&
           rec[5].shapes[2].offset == 12 && rec[5].table_be[3] == 33 && rec[5].table_be[5] == 55);
    assert(terrain_text_load(rec, NULL, tables, &n) == 0 && rec[5].used == 0);
}
int main(void) {
    static BorderTextRecord rec[16];
    static char text[16384];
    size_t n = 0;
    rec[3].used = 1;
    n += (size_t)sprintf(text + n, "2\n");
    n += (size_t)sprintf(text + n, "5 Deep_Water shw.slp 20001 0 1 2 3 -1 4 6 2.75 1.5 7 8 9\n");
    for (int i = 0; i < 19 * 12; ++i) n += (size_t)sprintf(text + n, "%d 3 ", i == 0 ? 2 : 1);
    n += (size_t)sprintf(text + n, "\n20 X x.slp 1 0 0 0 0 0 0 0 0 0 0 0 0\n");
    for (int i = 0; i < 19 * 12; ++i) n += (size_t)sprintf(text + n, "0 0 ");
    assert(border_text_load(rec, text) == 1);
    assert(!rec[3].used);
    BorderTextRecord *b = &rec[5];
    assert(b->used && strcmp(b->name, "Deep Water") == 0);
    assert(strcmp(b->file_name, "shw.slp") == 0 && b->slp_id == 20001);
    assert(b->b29 == 1 && b->b28 == 2 && b->b2a == 3 && b->color_table == -1);
    assert(b->b2b == 4 && b->w2c == 6 && b->value_whole == 2 && b->value == 2.75f);
    assert(b->value2 == 1.5f && b->b59a == 7 && b->w59c == 8 && b->w59e == 9);
    assert(b->shapes[0][0].offset == 0 && b->shapes[0][1].offset == 6);
    assert(b->shapes[1][0].offset == 6 + 11 * 3);
    assert(border_text_load(rec, NULL) == 0 && !rec[5].used);
    puts("terrain_art_test ok");
    terrain_text_tests();
    return 0;
}
