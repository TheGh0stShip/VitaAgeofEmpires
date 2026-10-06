/* The executable's stream encoder 0x005223e0 (deflate_writer.h), read
   back through the translated decoder. */
#include "deflate_writer.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inflate.h"

typedef struct {
    uint8_t *buf;
    size_t n, cap, calls, full_calls;
    int fail;
} Sink;

static int sink(void *c, const uint8_t *d, size_t n) {
    Sink *s = c;
    ++s->calls;
    if (n == 0x10000) ++s->full_calls;
    if (s->fail) return 1;
    if (s->n + n > s->cap) {
        s->cap = (s->n + n) * 2 + 1024;
        s->buf = realloc(s->buf, s->cap);
        assert(s->buf);
    }
    if (n) memcpy(s->buf + s->n, d, n);
    s->n += n;
    return 0;
}

static void reset(Sink *s) {
    free(s->buf);
    memset(s, 0, sizeof *s);
}

/* The stream decodes to exactly `in`. */
static void check(const uint8_t *in, size_t n, const Sink *s) {
    uint8_t *back;
    size_t back_n;
    assert(aoe_inflate_buffer_ror(s->buf, s->n, n + 1024, &back, &back_n));
    assert(back_n == n && (n == 0 || memcmp(in, back, n) == 0));
    free(back);
}

static void fill(uint8_t *in, size_t n, unsigned noise_every, unsigned seed) {
    static const char text[] = "VER 3.7 the tribe gathers berries ";
    size_t i;
    for (i = 0; i < n; ++i) {
        seed = seed * 1103515245u + 12345u;
        in[i] = (uint8_t)(i % noise_every == 0 ? seed >> 16 : text[i % (sizeof text - 1)]);
    }
}

int main(void) {
    static uint8_t noisy[300000], texty[200000], small[100];
    Sink s;
    AoeDeflateWriterRor *w;
    memset(&s, 0, sizeof s);
    fill(noisy, sizeof noisy, 3, 1);
    fill(texty, sizeof texty, 37, 2);
    fill(small, sizeof small, 1, 3);

    /* No input: the final empty fixed block alone (BFINAL 1, BTYPE 01, the
       seven-bit end code), padded to a byte: 03 00. Finishing returns 1
       and closes the writer (2 afterwards). */
    w = aoe_deflate_writer_new_ror(sink, &s);
    assert(w && aoe_deflate_writer_write_ror(w, NULL, 0, 1) == 1);
    assert(s.n == 2 && s.buf[0] == 0x03 && s.buf[1] == 0x00);
    assert(aoe_deflate_writer_write_ror(w, noisy, 1, 0) == 2);
    aoe_deflate_writer_free_ror(w);
    check(noisy, 0, &s);

    /* Noise: under 0x80 symbols the stored form is measured smallest:
       BFINAL 0, BTYPE 00, aligned, LEN 100, NLEN, the bytes. */
    reset(&s);
    w = aoe_deflate_writer_new_ror(sink, &s);
    assert(aoe_deflate_writer_write_ror(w, small, sizeof small, 1) == 1);
    aoe_deflate_writer_free_ror(w);
    assert((s.buf[0] & 7) == 0 && s.buf[1] == 100 && s.buf[2] == 0 && s.buf[3] == 0x9b &&
           s.buf[4] == 0xff && memcmp(s.buf + 5, small, sizeof small) == 0);
    check(small, sizeof small, &s);

    /* Mostly noise over several writes (one of a single byte): the output
       exceeds the 64 KB buffer, which is handed on each time it fills and
       once at the end. */
    reset(&s);
    w = aoe_deflate_writer_new_ror(sink, &s);
    assert(aoe_deflate_writer_write_ror(w, noisy, 100000, 0) == 0);
    assert(aoe_deflate_writer_write_ror(w, noisy + 100000, 1, 0) == 0);
    assert(aoe_deflate_writer_write_ror(w, noisy + 100001, sizeof noisy - 100001, 1) == 1);
    assert(aoe_deflate_writer_write_ror(w, noisy, 1, 1) == 2);
    aoe_deflate_writer_free_ror(w);
    assert(s.full_calls >= 1 && s.calls == s.full_calls + 1);
    check(noisy, sizeof noisy, &s);

    /* Mostly text: matches, the window wrapping, more than one
       0x3000-symbol block, dynamic codes. */
    reset(&s);
    w = aoe_deflate_writer_new_ror(sink, &s);
    assert(aoe_deflate_writer_write_ror(w, texty, sizeof texty, 1) == 1);
    aoe_deflate_writer_free_ror(w);
    assert(s.n < sizeof texty / 4);
    check(texty, sizeof texty, &s);

    /* A failing sink: 1, as a finished stream (the game's callback records
       the failure itself, 0x0046ba10). */
    reset(&s);
    s.fail = 1;
    w = aoe_deflate_writer_new_ror(sink, &s);
    assert(aoe_deflate_writer_write_ror(w, noisy, sizeof noisy, 1) == 1);
    aoe_deflate_writer_free_ror(w);

    assert(!aoe_deflate_writer_new_ror(NULL, NULL));
    reset(&s);
    puts("deflate writer ok");
    return 0;
}
