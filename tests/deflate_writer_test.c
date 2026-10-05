/* The raw deflate writer replacing the original encoder 0x005223e0. */
#include "deflate_writer.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

typedef struct { uint8_t buf[1 << 16]; size_t n; } Sink;

static int sink(void *c, const uint8_t *d, size_t n) {
    Sink *s = c;
    if (s->n + n > sizeof s->buf) return 1;
    memcpy(s->buf + s->n, d, n);
    s->n += n;
    return 0;
}

int main(void) {
    static Sink s;
    static uint8_t in[100000], back[100000];
    z_stream z = {0};
    AoeDeflateWriterRor *w = aoe_deflate_writer_new_ror(6, sink, &s);
    for (size_t i = 0; i < sizeof in; i++) in[i] = (uint8_t)(i * 7 % 251 / 3);
    assert(w);
    assert(aoe_deflate_writer_write_ror(w, in, 40000, 0) == 0);
    assert(aoe_deflate_writer_write_ror(w, in + 40000, sizeof in - 40000, 1) == 0);
    assert(aoe_deflate_writer_write_ror(w, in, 1, 0) == 2);
    aoe_deflate_writer_free_ror(w);
    assert(inflateInit2(&z, -MAX_WBITS) == Z_OK);
    z.next_in = s.buf;
    z.avail_in = (uInt)s.n;
    z.next_out = back;
    z.avail_out = sizeof back;
    assert(inflate(&z, Z_FINISH) == Z_STREAM_END);
    assert(z.total_out == sizeof in && !memcmp(in, back, sizeof in));
    inflateEnd(&z);
    assert(!aoe_deflate_writer_new_ror(6, NULL, NULL));
    return 0;
}
