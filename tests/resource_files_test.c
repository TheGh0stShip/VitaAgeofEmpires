/* Patched RoR 1.0a registry and resource files (0x0046ad60..0x0046b7c0). */
#include "resource_files.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "le_bytes.h"

/* Registry: three keys as tokens, one int and one string store per key. */
static int keys[3];
static int32_t ints[3];
static char strs[3][64];
static int ints_set[3];
static void *ropen(void *c, int which, const char *p) { (void)c; (void)p; return &keys[which]; }
static int closes;
static void rclose(void *c, void *k) { (void)c; (void)k; ++closes; }
static int ki(void *k) { return (int)((int *)k - keys); }
static int seti(void *c, void *k, const char *n, int32_t v) { (void)c; (void)n; ints[ki(k)] = v; ints_set[ki(k)] = 1; return 1; }
static int sets(void *c, void *k, const char *n, const char *v, int32_t len) { (void)c; (void)n; (void)len; snprintf(strs[ki(k)], 64, "%s", v); return 1; }
static int geti(void *c, void *k, const char *n, int32_t *v) { (void)c; (void)n; if (!ints_set[ki(k)]) return 0; *v = ints[ki(k)]; return 1; }
static int gets_(void *c, void *k, const char *n, char *out, int32_t len) { (void)c; (void)n; if (!strs[ki(k)][0]) return 0; snprintf(out, (size_t)len, "%s", strs[ki(k)]); return 1; }

/* A DRS image: header 0x40 + one table (type "pslb") with two entries. */
static uint8_t image[0x200];
static int32_t img_size;
static void build(void) {
    memset(image, 0, sizeof image);
    aoe_le_put_u32(image + 0x38, 1);           /* tables */
    aoe_le_put_u32(image + 0x3c, 0x40 + 12 + 24); /* header size */
    memcpy(image + 0x40, "pslb", 4);
    aoe_le_put_u32(image + 0x44, 0x40 + 12);
    aoe_le_put_u32(image + 0x48, 2);
    aoe_le_put_u32(image + 0x4c, 50300);
    aoe_le_put_u32(image + 0x50, 0x100);
    aoe_le_put_u32(image + 0x54, 4);
    aoe_le_put_u32(image + 0x58, 50301);
    aoe_le_put_u32(image + 0x5c, 0x104);
    aoe_le_put_u32(image + 0x60, 3);
    memcpy(image + 0x100, "WAVEabc", 7);
    img_size = 0x110;
}
static const uint8_t *map(void *c, const char *p, int32_t *n) { (void)c; if (strcmp(p, "data/sounds.drs") != 0) return NULL; *n = img_size; return image; }
static int unmaps;
static void unmap(void *c, const uint8_t *d) { (void)c; (void)d; ++unmaps; }
static int32_t fopen_(void *c, const char *p) { (void)c; return strcmp(p, "data/sounds.drs") == 0 ? 7 : -1; }
static int32_t read_at(void *c, int32_t h, int32_t off, void *out, int32_t n) { (void)c; assert(h == 7); memcpy(out, image + off, (size_t)n); return n; }
static int fcloses;
static void fclose_(void *c, int32_t h) { (void)c; (void)h; ++fcloses; }
static int tpath(void *c, char *out, int n) { (void)c; snprintf(out, (size_t)n, "/tmp/"); return 1; }
static int tfile(void *c, const char *d, char *out) { (void)c; sprintf(out, "%stemp1.tmp", d); return 1; }
static char wrote[8];
static int wfile(void *c, const char *p, const void *d, int32_t n) { (void)c; (void)p; memcpy(wrote, d, (size_t)n); wrote[n] = 0; return 1; }

/* Stream file. */
static int32_t sopen(void *c, const char *p, int32_t f, int32_t m) { (void)c; (void)p; (void)f; (void)m; return 3; }
static int32_t ssize(void *c, int32_t h) { (void)c; (void)h; return 100; }
static int32_t sread(void *c, int32_t h, void *o, int32_t n) { (void)c; (void)h; memset(o, 1, (size_t)n); return n; }
static int32_t write_result = 4;
static int32_t swrite(void *c, int32_t h, const void *d, int32_t n) { (void)c; (void)h; (void)d; (void)n; return write_result; }
static int32_t sclose(void *c, int32_t h) { (void)c; return h; }
static int32_t finish_result = 2;
static int32_t finish(void *c, void *z) { (void)c; (void)z; return finish_result; }
static int zfreed;
static void zfree(void *c, void *z) { (void)c; (void)z; ++zfreed; }
static int released;
static int release(void *c, void *p) { (void)c; (void)p; ++released; return 1; }

int main(void) {
    AoeRegistryKeyOpsRor rops = {0, ropen, rclose, seti, sets, geti, gets_};
    AoeRegistryRor reg;
    aoe_registry_open_ror(&reg, &rops, "Software\\X");
    assert(reg.user == &keys[AOE_REG_USER] && reg.machine == &keys[AOE_REG_MACHINE]);
    aoe_registry_set_int_ror(&reg, 1, "Screen Size", 800);
    assert(aoe_registry_get_int_ror(&reg, 1, "Screen Size") == 800);
    aoe_registry_set_int_ror(&reg, 0, "X", 5); /* written to the machine key */
    assert(aoe_registry_get_int_ror(&reg, 0, "X") == -1); /* read from the read-only one */
    aoe_registry_set_string_ror(&reg, 1, "CDPath", "D:\\", 4);
    assert(strcmp(aoe_registry_get_string_ror(&reg, 1, "CDPath"), "D:\\") == 0);
    assert(aoe_registry_get_string_ror(&reg, 0, "CDPath") == NULL);
    aoe_registry_close_ror(&reg);
    assert(closes == 3 && !reg.user);

    /* DRS. */
    build();
    {
        AoeDrsFileOpsRor fops = {0, map, unmap, fopen_, read_at, fclose_, tpath, tfile, wfile};
        AoeDrsFileRor *list = NULL;
        int32_t h, off, size;
        const uint8_t *m, *d;
        int owned;
        char path[260];
        assert(aoe_drs_add_ror(&list, &fops, "sounds.drs", "pw", "data/", 0) && list && list->handle == -1);
        assert(!aoe_drs_add_ror(&list, &fops, "none.drs", "pw", "data/", 0));
        assert(aoe_drs_find_ror(list, aoe_le_u32((const uint8_t *)"pslb"), 50301, &h, &off, &m, &size));
        assert(h == -1 && off == 0x104 && m == image && size == 3);
        assert(!aoe_drs_find_ror(list, aoe_le_u32((const uint8_t *)"pslb"), 1, &h, &off, &m, &size) && h == -1);
        d = aoe_drs_data_ror(list, &fops, aoe_le_u32((const uint8_t *)"pslb"), 50300, &owned, &size);
        assert(d == image + 0x100 && owned == 0 && size == 4);
        assert(aoe_drs_to_temp_ror(list, &fops, aoe_le_u32((const uint8_t *)"pslb"), 50300, path));
        assert(strcmp(path, "/tmp/temp1.tmp") == 0 && strcmp(wrote, "WAVE") == 0);
        /* A second archive read through file I/O, searched after. */
        assert(aoe_drs_add_ror(&list, &fops, "sounds.drs", "pw", "data/", 1) && list->next && list->next->handle == 7);
        aoe_drs_remove_ror(&list, &fops, "sounds.drs"); /* the first of that name */
        assert(list && list->handle == 7 && unmaps == 1);
        d = aoe_drs_data_ror(list, &fops, aoe_le_u32((const uint8_t *)"pslb"), 50301, &owned, &size);
        assert(owned == 1 && memcmp(d, "abc", 3) == 0);
        free((void *)d);
        aoe_drs_remove_ror(&list, &fops, "sounds.drs");
        assert(!list && fcloses == 1);
    }

    /* Stream file. */
    {
        AoeStreamFileOpsRor sops = {0, sopen, ssize, sread, swrite, sclose, finish, zfree};
        AoeStreamFileRor s = {0};
        uint8_t buf[8];
        s.handle = -1;
        assert(aoe_stream_open_ror(&s, &sops, "game.gam", 0x8000, 0) == 3 && s.mode == 2 && s.remaining == 100 && s.buffer);
        aoe_stream_read_raw_ror(&s, &sops, 3, buf, 8);
        assert(s.remaining == 92 && buf[0] == 1);
        aoe_stream_read_raw_ror(&s, &sops, 4, buf, 8);
        assert(s.remaining == 92);
        write_result = 0;
        aoe_stream_write_raw_ror(&s, &sops, 3, buf, 8);
        assert(s.error == 1);
        s.error = 0;
        s.mode = 1;
        s.compressor = &s;
        assert(aoe_stream_close_ror(&s, &sops, 3) == 3 && s.error == 1 && zfreed == 1 && s.handle == -1 && !s.buffer);
        assert(aoe_stream_close_ror(&s, &sops, 9) == 9);
        assert(aoe_stream_adopt_ror(&s, 5, 40) == 5 && s.remaining == 40 && s.handle == 5);
        free(s.buffer);
    }
    assert(aoe_stream_virtual_free_ror(release, NULL, &released, 4) == 1 && released == 1);
    assert(aoe_stream_virtual_free_ror(release, NULL, &released, 0) == 0);
    puts("resource files ok");
    return 0;
}
