/* dat_load through the translated decoder (inflate.h); the fixtures are
   made with the translated encoder (deflate_writer.h). */
#include "dat.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "deflate_writer.h"

typedef struct {
    uint8_t bytes[1024];
    size_t size;
} Out;

static int sink(void *context, const uint8_t *data, size_t size) {
    Out *out = context;
    if (out->size + size > sizeof out->bytes) return 1;
    if (size) memcpy(out->bytes + out->size, data, size);
    out->size += size;
    return 0;
}

static int pack(const uint8_t *input, size_t input_size, Out *out) {
    AoeDeflateWriterRor *w = aoe_deflate_writer_new_ror(sink, out);
    int r;
    out->size = 0;
    if (!w) return 0;
    r = aoe_deflate_writer_write_ror(w, input, input_size, 1);
    aoe_deflate_writer_free_ror(w);
    return r == 1;
}

static int write_file(const char *path, const uint8_t *bytes, size_t size) {
    FILE *file = fopen(path, "wb");
    int ok = file && fwrite(bytes, 1, size, file) == size;
    if (file) fclose(file);
    return ok;
}

int main(void) {
    const char *path = "dat_load_fixture.bin";
    uint8_t raw[16] = {'V','E','R',' ','3','.','7','\0', 1,0, 2,0, 0,0,0,0};
    Out out;
    DatBlob blob = {0};

    assert(pack(raw, sizeof(raw), &out));
    assert(write_file(path, out.bytes, out.size));
    assert(dat_load(path, &blob));
    assert(blob.size == sizeof(raw) && memcmp(blob.bytes, raw, sizeof(raw)) == 0);
    dat_free(&blob);

    /* The wrong version string is refused. */
    raw[6] = '8';
    assert(pack(raw, sizeof(raw), &out));
    assert(write_file(path, out.bytes, out.size));
    assert(!dat_load(path, &blob));

    /* A truncated stream is refused. */
    raw[6] = '7';
    assert(pack(raw, sizeof(raw), &out));
    assert(out.size > 1);
    assert(write_file(path, out.bytes, out.size - 1));
    assert(!dat_load(path, &blob));

    remove(path);
    return 0;
}
