#include "terrain_art.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    return 0;
}
