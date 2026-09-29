#include "dat.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

static int raw_deflate(const uint8_t *input, size_t input_size,
                       uint8_t *output, size_t output_capacity,
                       size_t *output_size) {
    z_stream stream = {0};
    int status;
    if (input_size > UINT_MAX || output_capacity > UINT_MAX) return 0;
    if (deflateInit2(&stream, Z_BEST_SPEED, Z_DEFLATED, -MAX_WBITS, 8,
                     Z_DEFAULT_STRATEGY) != Z_OK)
        return 0;
    stream.next_in = (Bytef *)input;
    stream.avail_in = (uInt)input_size;
    stream.next_out = output;
    stream.avail_out = (uInt)output_capacity;
    status = deflate(&stream, Z_FINISH);
    if (status == Z_STREAM_END)
        *output_size = output_capacity - stream.avail_out;
    deflateEnd(&stream);
    return status == Z_STREAM_END;
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
    uint8_t compressed[128];
    size_t compressed_size = 0;
    DatBlob blob = {0};

    assert(raw_deflate(raw, sizeof(raw), compressed, sizeof(compressed),
                       &compressed_size));
    assert(write_file(path, compressed, compressed_size));
    assert(dat_load(path, &blob));
    assert(blob.size == sizeof(raw) && memcmp(blob.bytes, raw, sizeof(raw)) == 0);
    dat_free(&blob);

    raw[6] = '8';
    assert(raw_deflate(raw, sizeof(raw), compressed, sizeof(compressed),
                       &compressed_size));
    assert(write_file(path, compressed, compressed_size));
    assert(!dat_load(path, &blob));

    raw[6] = '7';
    assert(raw_deflate(raw, sizeof(raw), compressed, sizeof(compressed),
                       &compressed_size));
    assert(compressed_size > 1);
    assert(write_file(path, compressed, compressed_size - 1));
    assert(!dat_load(path, &blob));

    remove(path);
    return 0;
}
