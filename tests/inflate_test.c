/* The executable's stream decoder 0x00525b0b (inflate.h). */
#include "inflate.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deflate_writer.h"

/* An LSB-first bit writer for hand-made blocks. */
typedef struct {
    uint8_t bytes[256];
    size_t bit;
} Bits;

static void put(Bits *b, uint32_t value, int n) {
    for (int i = 0; i < n; ++i, ++b->bit)
        if ((value >> i) & 1) b->bytes[b->bit / 8] |= (uint8_t)(1u << (b->bit % 8));
}

/* Huffman codes go most significant bit first. */
static void put_code(Bits *b, uint32_t code, int n) {
    for (int i = n - 1; i >= 0; --i) put(b, (code >> i) & 1, 1);
}

/* The fixed code (0x00525904). */
static void fixed_symbol(Bits *b, uint32_t sym) {
    if (sym < 144) put_code(b, 0x30 + sym, 8);
    else if (sym < 256) put_code(b, 0x190 + sym - 144, 9);
    else if (sym < 280) put_code(b, sym - 256, 7);
    else put_code(b, 0xc0 + sym - 280, 8);
}

static size_t bytes_of(const Bits *b) { return (b->bit + 7) / 8; }

/* Decodes all of `src` in calls of `chunk` output bytes. */
static int run(const uint8_t *src, size_t n, size_t chunk, uint8_t *out, size_t *out_n,
               int *suspensions) {
    AoeInflateRor *z = aoe_inflate_new_ror();
    size_t used = 0, made = 0;
    int r;
    *suspensions = 0;
    do {
        uint32_t in_len = (uint32_t)(n - used), out_len = (uint32_t)chunk;
        r = aoe_inflate_ror(z, src + used, &in_len, out + made, &out_len);
        if (r != -2) used += in_len;
        made += out_len;
        if (r == 0) ++*suspensions;
    } while (r == 0);
    aoe_inflate_free_ror(z);
    *out_n = made;
    return r;
}

static int sink(void *context, const uint8_t *data, size_t size) {
    uint8_t **p = context;
    memcpy(*p, data, size);
    *p += size;
    return 0;
}

int main(void) {
    uint8_t out[300000];
    size_t n;
    int s;

    /* Stored: final bit, type 0, to a byte boundary, LEN, NLEN, bytes. */
    {
        const uint8_t stored[] = {0x01, 5, 0, 0xfa, 0xff, 'h', 'e', 'l', 'l', 'o'};
        uint8_t bad[sizeof stored];
        uint32_t in_len = sizeof bad, out_len = 16;
        AoeInflateRor *z;
        assert(run(stored, sizeof stored, 64, out, &n, &s) == -1 && n == 5 && s == 0);
        assert(memcmp(out, "hello", 5) == 0);
        /* One byte of output per call: suspended after each, the same bytes. */
        assert(run(stored, sizeof stored, 1, out, &n, &s) == -1 && n == 5 && s == 5);
        assert(memcmp(out, "hello", 5) == 0);
        /* NLEN not the complement: -2, the input count left as passed. */
        memcpy(bad, stored, sizeof bad);
        bad[3] = 0xfb;
        z = aoe_inflate_new_ror();
        assert(aoe_inflate_ror(z, bad, &in_len, out, &out_len) == -2);
        assert(in_len == sizeof bad && out_len == 0);
        /* Finished: further calls return the same. */
        assert(aoe_inflate_ror(z, bad, &in_len, out, &out_len) == -2 && out_len == 0);
        aoe_inflate_free_ror(z);
    }

    /* Type 3 fails. */
    {
        const uint8_t bad[] = {0x07, 0, 0};
        assert(run(bad, sizeof bad, 64, out, &n, &s) == -2 && n == 0);
    }

    /* Fixed: 'A', a 9-bit literal (the tree path), then a match reaching
       before the window start: before the first flush it reads the zeroed
       window end (0x005257ae), then continues from the window start. */
    {
        Bits b;
        memset(&b, 0, sizeof b);
        put(&b, 1, 1);
        put(&b, 1, 2);
        fixed_symbol(&b, 'A');
        fixed_symbol(&b, 200);
        fixed_symbol(&b, 258); /* length 4 */
        put_code(&b, 2, 5);    /* distance 3 */
        fixed_symbol(&b, 256);
        assert(run(b.bytes, bytes_of(&b), 64, out, &n, &s) == -1 && n == 6);
        assert(out[0] == 'A' && out[1] == 200 && out[2] == 0 && out[3] == 'A' && out[4] == 200 &&
               out[5] == 0);
        /* Length symbol 286 fails (port: the original reads past its tables). */
        memset(&b, 0, sizeof b);
        put(&b, 1, 1);
        put(&b, 1, 2);
        fixed_symbol(&b, 286);
        assert(run(b.bytes, bytes_of(&b), 64, out, &n, &s) == -2);
    }

    /* Round trip of 200 000 bytes through the writer (fixed and dynamic
       blocks, codes over 8 bits, several window flushes), whole and in
       odd output chunks; the buffer helper agrees. */
    {
        static uint8_t in[200000], packed[300000];
        uint8_t *p = packed, *whole;
        size_t whole_n;
        unsigned seed = 7;
        AoeDeflateWriterRor *w;
        for (size_t i = 0; i < sizeof in; ++i) {
            seed = seed * 1103515245u + 12345u;
            in[i] = (uint8_t)(i % 7 == 0 ? (seed >> 16) : "the quick brown fox "[i % 20]);
        }
        w = aoe_deflate_writer_new_ror(9, sink, &p);
        assert(w && aoe_deflate_writer_write_ror(w, in, sizeof in, 1) == 0);
        aoe_deflate_writer_free_ror(w);
        assert(run(packed, (size_t)(p - packed), 0x10000, out, &n, &s) == -1);
        assert(n == sizeof in && memcmp(out, in, n) == 0);
        assert(run(packed, (size_t)(p - packed), 4093, out, &n, &s) == -1 && s > 40);
        assert(n == sizeof in && memcmp(out, in, n) == 0);
        assert(aoe_inflate_buffer_ror(packed, (size_t)(p - packed), 1 << 20, &whole, &whole_n));
        assert(whole_n == sizeof in && memcmp(whole, in, whole_n) == 0);
        free(whole);
        /* Too small a limit, and a truncated stream, fail. */
        assert(!aoe_inflate_buffer_ror(packed, (size_t)(p - packed), 1000, &whole, &whole_n));
        assert(!aoe_inflate_buffer_ror(packed, (size_t)(p - packed) / 2, 1 << 20, &whole,
                                       &whole_n));
    }
    puts("inflate ok");
    return 0;
}
