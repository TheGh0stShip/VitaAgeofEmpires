#include "resource_files.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "le_bytes.h"

/* ---- registry ---- */

void aoe_registry_open_ror(AoeRegistryRor *r, const AoeRegistryKeyOpsRor *ops, const char *path) {
    r->ops = ops;
    r->machine = r->user = r->machine_read = NULL;
    r->text[0] = '\0';
    r->user = ops->open(ops->context, AOE_REG_USER, path);
    r->machine = ops->open(ops->context, AOE_REG_MACHINE, path);
    r->machine_read = ops->open(ops->context, AOE_REG_MACHINE_READ, path);
}

void aoe_registry_close_ror(AoeRegistryRor *r) {
    if (r->user) r->ops->close(r->ops->context, r->user);
    r->user = NULL;
    if (r->machine) r->ops->close(r->ops->context, r->machine);
    r->machine = NULL;
    if (r->machine_read) r->ops->close(r->ops->context, r->machine_read);
    r->machine_read = NULL;
}

int aoe_registry_set_int_ror(AoeRegistryRor *r, int user, const char *name, int32_t v) {
    return r->ops->set_int(r->ops->context, user ? r->user : r->machine, name, v);
}

int aoe_registry_set_string_ror(AoeRegistryRor *r, int user, const char *name, const char *v,
                                int32_t n) {
    return r->ops->set_string(r->ops->context, user ? r->user : r->machine, name, v, n);
}

const char *aoe_registry_get_string_ror(AoeRegistryRor *r, int user, const char *name) {
    if (!r->ops->get_string(r->ops->context, user ? r->user : r->machine_read, name, r->text,
                            0xff))
        return NULL;
    return r->text;
}

int32_t aoe_registry_get_int_ror(const AoeRegistryRor *r, int user, const char *name) {
    int32_t v;
    if (!r->ops->get_int(r->ops->context, user ? r->user : r->machine_read, name, &v)) return -1;
    return v;
}

/* ---- DRS archives ---- */

int aoe_drs_add_ror(AoeDrsFileRor **list, const AoeDrsFileOpsRor *ops, const char *name,
                    const char *password, const char *dir, int use_file_io) {
    char path[0x104];
    const uint8_t *mapped = NULL;
    int32_t handle = -1, size = 0;
    AoeDrsFileRor *f, *tail;
    snprintf(path, sizeof path, "%s%s", dir, name);
    if (!use_file_io) {
        mapped = ops->map(ops->context, path, &size);
        if (!mapped) return 0;
    } else {
        handle = ops->open(ops->context, path);
        if (handle == -1) return 0;
    }
    f = calloc(1, sizeof *f);
    if (!f) return 0;
    snprintf(f->name, sizeof f->name, "%s", name);
    f->next = NULL;
    if (!use_file_io) {
        f->handle = -1;
        f->mapped = mapped;
        f->header = mapped;
    } else {
        uint8_t head[0x40];
        int32_t n;
        f->handle = handle;
        f->mapped = NULL;
        if (ops->read_at(ops->context, handle, 0, head, 0x40) != 0x40) return 0;
        n = aoe_le_i32(head + 0x3c);
        f->owned = malloc(n > 0 ? (size_t)n : 1);
        f->header = f->owned;
        if (!f->owned || ops->read_at(ops->context, handle, 0, f->owned, n) != n) return 0;
    }
    if (!*list) {
        *list = f;
    } else {
        for (tail = *list; tail->next; tail = tail->next) {}
        tail->next = f;
    }
    /* The password and the "1.00" version are compared; the results are
       not used. */
    (void)password;
    return 1;
}

void aoe_drs_remove_ror(AoeDrsFileRor **list, const AoeDrsFileOpsRor *ops, const char *name) {
    AoeDrsFileRor *prev = NULL, *f;
    for (f = *list; f; prev = f, f = f->next) {
        if (strcmp(name, f->name) != 0) continue;
        if (!f->mapped) free(f->owned);
        if (!prev) *list = f->next;
        else prev->next = f->next;
        if (f->handle != -1) ops->close(ops->context, f->handle);
        if (f->mapped) ops->unmap(ops->context, f->mapped);
        free(f);
        return;
    }
}

int aoe_drs_find_ror(const AoeDrsFileRor *list, uint32_t type, int32_t id, int32_t *handle,
                     int32_t *offset, const uint8_t **mapped, int32_t *size) {
    const AoeDrsFileRor *f;
    *handle = -1;
    *offset = 0;
    *mapped = NULL;
    *size = 0;
    for (f = list; f; f = f->next) {
        const uint8_t *h = f->header;
        int32_t tables = aoe_le_i32(h + 0x38), t;
        for (t = 0; t < tables; ++t) {
            const uint8_t *table = h + 0x40 + t * 12;
            if (aoe_le_u32(table) == type) {
                const uint8_t *e = h + aoe_le_i32(table + 4);
                int32_t count = aoe_le_i32(table + 8), i;
                for (i = 0; i < count; ++i, e += 12) {
                    if (aoe_le_i32(e) == id) {
                        *handle = f->handle;
                        *offset = aoe_le_i32(e + 4);
                        *mapped = f->mapped;
                        *size = aoe_le_i32(e + 8);
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

const uint8_t *aoe_drs_data_ror(const AoeDrsFileRor *list, const AoeDrsFileOpsRor *ops,
                                uint32_t type, int32_t id, int *owned, int32_t *size) {
    int32_t handle, offset, n;
    const uint8_t *mapped;
    uint8_t *data;
    *owned = -1;
    *size = 0;
    if (!aoe_drs_find_ror(list, type, id, &handle, &offset, &mapped, &n)) return NULL;
    if (mapped) {
        *owned = 0;
        *size = n;
        return mapped + offset;
    }
    *owned = 1;
    data = malloc(n > 0 ? (size_t)n : 1);
    *size = n;
    if (!data) return NULL;
    if (ops->read_at(ops->context, handle, offset, data, n) != n) return NULL;
    return data;
}

int aoe_drs_to_temp_ror(const AoeDrsFileRor *list, const AoeDrsFileOpsRor *ops, uint32_t type,
                        int32_t id, char *path) {
    char dir[0x104];
    int owned, ok = 0;
    int32_t size;
    const uint8_t *data = aoe_drs_data_ror(list, ops, type, id, &owned, &size);
    if (!data) return 0;
    if (!ops->temp_path(ops->context, dir, sizeof dir)) strcpy(dir, ".");
    if (ops->temp_file(ops->context, dir, path)) {
        ok = 1;
        if (size > 0) ok = ops->write_file(ops->context, path, data, size);
    }
    if (owned == 1) free((void *)data);
    return ok;
}

/* ---- stream file ---- */

int aoe_stream_virtual_free_ror(int (*release)(void *context, void *p), void *context, void *p,
                                int32_t size) {
    if (p && size > 0) return release(context, p);
    return 0;
}

static void start(AoeStreamFileRor *s, int32_t handle) {
    s->mode = 2;
    s->field_d8 = 0;
    s->buffer = calloc(0x10000, 1);
    s->field_f0 = NULL;
    s->compressor = NULL;
    s->handle = handle;
    s->cursor = s->buffer;
}

int32_t aoe_stream_adopt_ror(AoeStreamFileRor *s, int32_t handle, int32_t size) {
    if (handle > -1) {
        start(s, handle);
        s->remaining = size;
    }
    return handle;
}

int32_t aoe_stream_open_ror(AoeStreamFileRor *s, const AoeStreamFileOpsRor *ops,
                            const char *path, int32_t flags, int32_t mode) {
    int32_t h = ops->open(ops->context, path, flags, mode);
    if (h > -1) {
        start(s, h);
        s->remaining = ops->size(ops->context, h);
        s->field_e4 = 0;
    }
    return h;
}

int32_t aoe_stream_close_ror(AoeStreamFileRor *s, const AoeStreamFileOpsRor *ops,
                             int32_t handle) {
    if (handle < 0 || handle != s->handle) return handle;
    if (s->mode == 1 && s->compressor) {
        if (ops->finish(ops->context, s->compressor) == 2) s->error = 1;
        ops->free_compressor(ops->context, s->compressor);
    }
    s->handle = -1;
    free(s->buffer);
    s->buffer = NULL;
    s->compressor = NULL;
    free(s->field_f0);
    s->field_f0 = NULL;
    return ops->close(ops->context, handle);
}

void aoe_stream_read_raw_ror(AoeStreamFileRor *s, const AoeStreamFileOpsRor *ops,
                             int32_t handle, void *out, int32_t n) {
    if (handle < 0 || handle != s->handle) return;
    ops->read(ops->context, handle, out, n);
    s->remaining -= n;
}

void aoe_stream_write_raw_ror(AoeStreamFileRor *s, const AoeStreamFileOpsRor *ops,
                              int32_t handle, const void *data, int32_t n) {
    if (handle < 0 || handle != s->handle) return;
    if (ops->write(ops->context, handle, data, n) < 1) s->error = 1;
}
