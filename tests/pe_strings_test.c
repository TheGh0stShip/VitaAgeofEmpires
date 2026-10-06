/* PE RT_STRING reader: synthetic libraries. */
#include "pe_strings.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "le_bytes.h"

/* A minimal PE32 image: headers in the first 0x200 bytes, one .rsrc
   section at RVA 0x1000 / file 0x200 holding a string table with blocks
   `blocks` (each block's 16 strings from `text(block, i)`). */
static const char *text_of(uint32_t block, int i) {
    static char buf[64];
    if (block == 0x241 && i == 2) return "Single Player"; /* ID 9218 */
    if (block == 0x241 && i == 3) return "";
    if (block == 0xd4f && i == 13) return "Caf\xe9 \x80";   /* ID 0xd4ed, 1252 bytes */
    if (block == 1 && i == 0) return NULL;                   /* left empty */
    snprintf(buf, sizeof buf, "B%x.%d", (unsigned)block, i);
    return buf;
}

static void put16(uint8_t *p, uint16_t v) { aoe_le_put_u16(p, v); }
static void put32(uint8_t *p, uint32_t v) { aoe_le_put_u32(p, v); }

static uint8_t *build(const uint32_t *blocks, int nblocks, size_t *size_out, int with_rsrc) {
    size_t size = 0x4000, r = 0x200, dir, p;
    uint8_t *d = calloc(1, size);
    int b, i;
    d[0] = 'M'; d[1] = 'Z';
    put32(d + 0x3c, 0x80);
    memcpy(d + 0x80, "PE\0\0", 4);
    put16(d + 0x84, 0x14c);
    put16(d + 0x86, 1);                /* one section */
    put16(d + 0x94, 224);              /* optional header size */
    put16(d + 0x98, 0x10b);            /* PE32 */
    if (with_rsrc) {
        put32(d + 0x98 + 96 + 16, 0x1000);   /* resource directory RVA */
        put32(d + 0x98 + 96 + 20, 0x3000);
    }
    p = 0x98 + 224;                    /* section table */
    memcpy(d + p, ".rsrc", 5);
    put32(d + p + 8, 0x3000);
    put32(d + p + 12, 0x1000);
    put32(d + p + 16, 0x3000);
    put32(d + p + 20, 0x200);
    /* Root: one numbered entry, type 6 -> subdirectory at 0x18. */
    put16(d + r + 14, 1);
    put32(d + r + 16, 6);
    put32(d + r + 20, 0x80000000u | 0x18);
    /* Type directory: one entry per block -> name directory. */
    dir = 0x18;
    put16(d + r + dir + 14, (uint16_t)nblocks);
    p = dir + 16 + (size_t)nblocks * 8;   /* name directories follow */
    for (b = 0; b < nblocks; ++b) {
        size_t name_dir = p, data_entry = p + 24;
        put32(d + r + dir + 16 + b * 8, blocks[b]);
        put32(d + r + dir + 16 + b * 8 + 4, 0x80000000u | (uint32_t)name_dir);
        put16(d + r + name_dir + 14, 1);
        put32(d + r + name_dir + 16, 0x409);             /* language */
        put32(d + r + name_dir + 20, (uint32_t)data_entry); /* leaf */
        p = data_entry + 16;
        {
            size_t start = p;
            for (i = 0; i < 16; ++i) {
                const char *s = text_of(blocks[b], i);
                size_t n = s ? strlen(s) : 0, k;
                put16(d + r + p, (uint16_t)n);
                p += 2;
                for (k = 0; k < n; ++k) {
                    uint8_t c = (uint8_t)s[k];
                    put16(d + r + p, c == 0x80 ? 0x20ac : c); /* euro as UTF-16 */
                    p += 2;
                }
            }
            put32(d + r + data_entry, 0x1000 + (uint32_t)start);
            put32(d + r + data_entry + 4, (uint32_t)(p - start));
        }
        p = (p + 3) & ~(size_t)3;
    }
    *size_out = size;
    return d;
}

int main(void) {
    static const uint32_t blocks[3] = {1, 0x241, 0xd4f};
    static const uint32_t base_blocks[1] = {0x242};
    AoePeStringsRor t, b;
    char out[32];
    size_t size;
    uint8_t *img = build(blocks, 3, &size, 1);
    assert(aoe_pe_strings_open(&t, img, size) == 1);
    /* ID 9218 = block 0x241 entry 2. */
    assert(aoe_pe_load_string_ror(&t, 9218, out, sizeof out) == 13 && strcmp(out, "Single Player") == 0);
    /* Truncated to size - 1. */
    assert(aoe_pe_load_string_ror(&t, 9218, out, 7) == 6 && strcmp(out, "Single") == 0);
    /* Empty and missing strings return 0 and leave the buffer. */
    strcpy(out, "keep");
    assert(aoe_pe_load_string_ror(&t, 9219, out, sizeof out) == 0 && strcmp(out, "keep") == 0);
    assert(aoe_pe_load_string_ror(&t, 0, out, sizeof out) == 0 && strcmp(out, "keep") == 0);
    assert(aoe_pe_load_string_ror(&t, 50000, out, sizeof out) == 0);
    /* IDs above 0xffff keep their low 16 bits; text converts to 1252. */
    assert(aoe_pe_load_string_ror(&t, 0x1d4ed, out, sizeof out) == 6 && strcmp(out, "Caf\xe9 \x80") == 0);
    assert(aoe_pe_load_string_ror(&t, 0xd4ed, out, sizeof out) == 6);
    assert(aoe_pe_load_string_ror(&t, 0x2405, out, sizeof out) == 6 && strcmp(out, "B241.5") == 0);
    /* The game lookup: expansion first, the base library second, "" last. */
    {
        size_t bsize;
        uint8_t *bimg = build(base_blocks, 1, &bsize, 1);
        assert(aoe_pe_strings_open(&b, bimg, bsize) == 1);
        aoe_lang_string_ror(&t, &b, 9218, out, sizeof out);
        assert(strcmp(out, "Single Player") == 0);
        aoe_lang_string_ror(&t, &b, 9232, out, sizeof out);   /* block 0x242 entry 0 */
        assert(strcmp(out, "B242.0") == 0);
        strcpy(out, "stale");
        aoe_lang_string_ror(&t, &b, 50000, out, sizeof out);
        assert(out[0] == '\0');
        aoe_lang_string_ror(NULL, NULL, 9218, out, sizeof out);
        assert(out[0] == '\0');
        aoe_pe_strings_free(&b);
    }
    aoe_pe_strings_free(&t);
    /* Not a PE, no resources, and truncated files load as empty tables. */
    {
        uint8_t *junk = calloc(1, 64);
        assert(aoe_pe_strings_open(&t, junk, 64) == 0 && aoe_pe_load_string_ror(&t, 9218, out, sizeof out) == 0);
        aoe_pe_strings_free(&t);
        img = build(blocks, 3, &size, 0);
        assert(aoe_pe_strings_open(&t, img, size) == 0);
        aoe_pe_strings_free(&t);
        img = build(blocks, 3, &size, 1);
        assert(aoe_pe_strings_open(&t, img, 0x300) == 0 || aoe_pe_load_string_ror(&t, 9218, out, sizeof out) == 0);
        aoe_pe_strings_free(&t);
    }
    puts("pe strings ok");
    return 0;
}
