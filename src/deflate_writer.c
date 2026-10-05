/* Raw deflate stream writer standing in for the original's encoder
   (see deflate_writer.h). */
#include "deflate_writer.h"

#include <stdlib.h>
#include <zlib.h>

struct AoeDeflateWriterRor {
    z_stream z;
    AoeDeflateSinkRor sink;
    void *context;
    int done;
};

AoeDeflateWriterRor *aoe_deflate_writer_new_ror(int level,
                                                AoeDeflateSinkRor sink,
                                                void *context) {
    AoeDeflateWriterRor *w;
    if (!sink) return NULL;
    w = calloc(1, sizeof *w);
    if (!w) return NULL;
    if (deflateInit2(&w->z, level, Z_DEFLATED, -MAX_WBITS, 8,
                     Z_DEFAULT_STRATEGY) != Z_OK) {
        free(w);
        return NULL;
    }
    w->sink = sink;
    w->context = context;
    return w;
}

int aoe_deflate_writer_write_ror(AoeDeflateWriterRor *w, const void *data,
                                 size_t size, int finish) {
    uint8_t out[16384];
    int status;
    if (!w || w->done || (size && !data)) return 2;
    w->z.next_in = (Bytef *)data;
    w->z.avail_in = (uInt)size;
    do {
        w->z.next_out = out;
        w->z.avail_out = sizeof out;
        status = deflate(&w->z, finish ? Z_FINISH : Z_NO_FLUSH);
        if (status == Z_STREAM_ERROR) return 2;
        if (sizeof out - w->z.avail_out &&
            w->sink(w->context, out, sizeof out - w->z.avail_out))
            return 2;
    } while (w->z.avail_out == 0 || (finish && status != Z_STREAM_END));
    if (finish) w->done = 1;
    return 0;
}

void aoe_deflate_writer_free_ror(AoeDeflateWriterRor *w) {
    if (!w) return;
    deflateEnd(&w->z);
    free(w);
}
