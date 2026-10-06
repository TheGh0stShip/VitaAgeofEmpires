/* The translated encoder 0x005223e0 against the installed files. Each
   compressed body is decoded with the translated decoder 0x00525b0b and
   encoded again with the game's configuration (0x0046ba40); the result
   must equal the shipped bytes. Arguments: the DAT (a bare stream), then
   scenarios, whose stream follows the raw header record (version, record
   length, record; 0x00417670). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deflate_writer.h"
#include "inflate.h"

typedef struct {
    uint8_t *d;
    size_t n, cap;
} Buf;

static int sink(void *c, const uint8_t *d, size_t n) {
    Buf *b = c;
    if (b->n + n > b->cap) {
        b->cap = (b->n + n) * 2 + 1024;
        b->d = realloc(b->d, b->cap);
        if (!b->d) return 1;
    }
    if (n) memcpy(b->d + b->n, d, n);
    b->n += n;
    return 0;
}

static uint8_t *read_whole(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    uint8_t *d;
    long n;
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) {
        fclose(f);
        return NULL;
    }
    d = malloc((size_t)n + 1);
    if (!d || fread(d, 1, (size_t)n, f) != (size_t)n) {
        free(d);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *size = (size_t)n;
    return d;
}

int main(int argc, char **argv) {
    int i, failures = 0;
    for (i = 1; i < argc; ++i) {
        size_t n, off = 0, raw_n, k;
        uint8_t *d = read_whole(argv[i], &n), *raw;
        Buf out = {NULL, 0, 0};
        AoeDeflateWriterRor *w;
        if (!d) {
            printf("%s: cannot read\n", argv[i]);
            ++failures;
            continue;
        }
        if (i > 1) {
            if (n < 8) {
                printf("%s: too short\n", argv[i]);
                ++failures;
                free(d);
                continue;
            }
            off = 8 + ((size_t)d[4] | (size_t)d[5] << 8 | (size_t)d[6] << 16 | (size_t)d[7] << 24);
            if (off > n) off = n;
        }
        if (!aoe_inflate_buffer_ror(d + off, n - off, 64u << 20, &raw, &raw_n)) {
            printf("%s: does not decode\n", argv[i]);
            ++failures;
            free(d);
            continue;
        }
        w = aoe_deflate_writer_new_ror(sink, &out);
        if (!w || aoe_deflate_writer_write_ror(w, raw, raw_n, 1) != 1) {
            printf("%s: encoder failed\n", argv[i]);
            ++failures;
        } else {
            size_t shipped = n - off, common = out.n < shipped ? out.n : shipped;
            for (k = 0; k < common && out.d[k] == d[off + k]; ++k) {}
            if (out.n == shipped && k == common) {
                printf("%s: identical (%zu bytes from %zu)\n", argv[i], shipped, raw_n);
            } else {
                printf("%s: differs at byte %zu (ours %zu bytes, shipped %zu)\n", argv[i], k,
                       out.n, shipped);
                ++failures;
            }
        }
        aoe_deflate_writer_free_ror(w);
        free(out.d);
        free(raw);
        free(d);
    }
    return failures != 0;
}
