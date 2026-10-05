#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "dib_picture.h"
#include "le_bytes.h"

static int file_exists, resource_exists, loads, released, removed,
    remapped, spans_ok = 1, spans_built, spans_released, draws, drew_spans;
static char last_name[260];
static int span_token;

/* A 5 x -3 (top-down) 8-bit block with 256 palette entries. */
static uint8_t *make_dib(size_t *size) {
    size_t n = 0x428 + 8 * 3;
    uint8_t *d = calloc(n, 1);
    aoe_le_put_u32(d, 0x28);
    aoe_le_put_u32(d + 4, 5);
    aoe_le_put_u32(d + 8, (uint32_t)-3);
    aoe_le_put_u32(d + 0x20, 256);
    d[0x428] = 0xab;
    d[n - 1] = 0xcd;
    *size = n;
    return d;
}

static uint8_t *load_dib(void *c, const char *name, size_t *size) {
    (void)c;
    loads++;
    strncpy(last_name, name, sizeof(last_name) - 1);
    if (strcmp(name, "tmp.bin") == 0 ? resource_exists : file_exists)
        return make_dib(size);
    return NULL;
}
static void release_dib(void *c, uint8_t *d) {
    (void)c;
    released++;
    free(d);
}
static int resource_file(void *c, uint32_t type, int32_t id, char path[260]) {
    (void)c;
    assert(type == 0x62696e61u && id == 50);
    strcpy(path, "tmp.bin");
    return 1;
}
static void remove_file(void *c, const char *p) {
    (void)c;
    assert(strcmp(p, "tmp.bin") == 0);
    removed++;
}
static void remap(void *c, uint8_t *d, void *t, int32_t s, int32_t e) {
    (void)c;
    (void)d;
    (void)t;
    (void)s;
    assert(e == 9);
    remapped++;
}
static void *build_spans(void *c, const uint8_t *d, const uint8_t *bits) {
    (void)c;
    assert(bits == d + 0x428 && bits[0] == 0xab);
    spans_built++;
    return spans_ok ? &span_token : NULL;
}
static void release_spans(void *c, void *s) {
    (void)c;
    assert(s == &span_token);
    spans_released++;
}
static void draw(void *c, void *surface, const AoeDibPictureRor *p,
                 int32_t x, int32_t y, int with_spans, int32_t a, int32_t b) {
    (void)c;
    (void)surface;
    (void)p;
    assert(x == 3 && y == 4 && a == 5 && b == 6);
    draws++;
    drew_spans = with_spans;
}

static AoeDibPictureOps ops = {NULL,          0,           load_dib,
                               release_dib,   resource_file, remove_file,
                               remap,         build_spans, release_spans,
                               draw};

int main(void) {
    AoeDibPictureRor p, q;
    int table;
    uint8_t buf[2048];
    AoeOutcomeStream out = {buf, sizeof(buf), 0, 0}, in;

    /* Without files first: the resource is tried before the file. */
    resource_exists = 1;
    aoe_dib_picture_init_ror(&p, "pic.bmp", 50, 1, &table, 9, &ops);
    assert(loads == 1 && strcmp(last_name, "tmp.bin") == 0 && removed == 1);
    assert(p.mode == 1 && p.width == 5 && p.height == 3 &&
           p.orientation == 1 && p.bits == p.dib + 0x428);
    assert(remapped == 1 && spans_built == 1 && p.spans == &span_token);
    aoe_dib_picture_draw_ror(&p, NULL, 3, 4, 5, 6, &ops);
    assert(draws == 1 && drew_spans == 1);

    /* Save, then the save constructor. */
    assert(aoe_dib_picture_save_ror(&p, &out));
    assert(out.pos == 4 + 4 + 4 + 2 + 0x428 + 8 * 3);
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    assert(aoe_dib_picture_load_saved_ror(&q, &in, 0, &ops));
    assert(in.pos == out.pos && q.mode == 2 && q.width == 5 &&
           q.height == 3 && q.orientation == 1 && q.bits[0] == 0xab &&
           q.bits[23] == 0xcd && q.spans == NULL);
    aoe_dib_picture_draw_ror(&q, NULL, 3, 4, 5, 6, &ops);
    assert(drew_spans == 0);
    aoe_dib_picture_destroy_ror(&q, &ops);
    assert(released == 0);
    /* A failed span table drops the saved block. */
    spans_ok = 0;
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    assert(aoe_dib_picture_load_saved_ror(&q, &in, 1, &ops));
    assert(q.dib == NULL && q.mode == 0 && q.orientation == 1);
    spans_ok = 1;

    aoe_dib_picture_destroy_ror(&p, &ops);
    assert(released == 1 && spans_released == 1);

    /* Files first: the file is loaded, the resource never asked for. */
    ops.prefer_files = 1;
    file_exists = 1;
    removed = 0;
    aoe_dib_picture_init_ror(&p, "pic.bmp", 50, 0, NULL, 0, &ops);
    assert(strcmp(last_name, "pic.bmp") == 0 && removed == 0 &&
           p.spans == NULL && remapped == 1);
    aoe_dib_picture_destroy_ror(&p, &ops);

    /* Nothing found: cleared with orientation 1. */
    file_exists = resource_exists = 0;
    aoe_dib_picture_init_ror(&p, "none", -1, 1, NULL, 0, &ops);
    assert(!p.dib && p.mode == 0 && p.orientation == 1);
    /* A failed span table releases the loaded block. */
    file_exists = 1;
    spans_ok = 0;
    released = 0;
    aoe_dib_picture_init_ror(&p, "pic.bmp", -1, 1, NULL, 0, &ops);
    assert(!p.dib && released == 1 && p.mode == 0);

    /* Empty pictures save only the fields. */
    out = (AoeOutcomeStream){buf, sizeof(buf), 0, 0};
    assert(aoe_dib_picture_save_ror(&p, &out) && out.pos == 14);
    return 0;
}
