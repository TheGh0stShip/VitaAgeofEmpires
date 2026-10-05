#ifndef VITA_AOE_RESOURCE_FILES_H
#define VITA_AOE_RESOURCE_FILES_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a registry and resource files
   (0x0046ad60..0x0046b7c0): the registry object, the list of open DRS
   archives (global 0x007bfac4) and the single open stream file with its
   buffers (globals 0x007bfad0..0x007bfaf4). The registry, the files and
   the temp directory are ops; the archive and stream bookkeeping is
   kept here. */

/* ---- registry (0x0046ad60..0x0046af50) ---- */

typedef struct {
    void *context;
    /* Three keys: the user's (HKEY_CURRENT_USER, +4), the machine's
       (HKEY_LOCAL_MACHINE, +0) and the machine's read-only (+8). */
    void *(*open)(void *context, int which, const char *path);
    void (*close)(void *context, void *key);
    int (*set_int)(void *context, void *key, const char *name, int32_t v);
    int (*set_string)(void *context, void *key, const char *name, const char *v, int32_t n);
    int (*get_int)(void *context, void *key, const char *name, int32_t *v);
    int (*get_string)(void *context, void *key, const char *name, char *out, int32_t n);
} AoeRegistryKeyOpsRor;

enum { AOE_REG_MACHINE, AOE_REG_USER, AOE_REG_MACHINE_READ };

typedef struct {
    void *machine;        /* +0x00 */
    void *user;           /* +0x04 */
    void *machine_read;   /* +0x08 */
    char text[0x100];     /* +0x10 */
    const AoeRegistryKeyOpsRor *ops;
} AoeRegistryRor;

/* 0x0046ad60 opens the three keys; 0x0046ade0 closes them. */
void aoe_registry_open_ror(AoeRegistryRor *r, const AoeRegistryKeyOpsRor *ops, const char *path);
void aoe_registry_close_ror(AoeRegistryRor *r);
/* 0x0046ae20 / 0x0046ae80: write an int / a string (user != 0: the user's
   key, else the machine's). 0x0046aee0: read a string into +0x10 (255
   bytes; NULL on failure); 0x0046af50: an int (-1 on failure). Reads
   use the user's key or the machine's read-only one. */
int aoe_registry_set_int_ror(AoeRegistryRor *r, int user, const char *name, int32_t v);
int aoe_registry_set_string_ror(AoeRegistryRor *r, int user, const char *name, const char *v,
                                int32_t n);
const char *aoe_registry_get_string_ror(AoeRegistryRor *r, int user, const char *name);
int32_t aoe_registry_get_int_ror(const AoeRegistryRor *r, int user, const char *name);

/* ---- DRS archives (0x0046afc0..0x0046b420) ---- */

typedef struct AoeDrsFileRor {
    const uint8_t *mapped;      /* +0x00: the whole file, or NULL */
    int32_t handle;             /* +0x04: open file, or -1 */
    const uint8_t *header;      /* +0x08: header and tables */
    struct AoeDrsFileRor *next; /* +0x0c */
    char name[0x104];           /* +0x10 */
    uint8_t *owned;             /* the header read into memory (freed) */
} AoeDrsFileRor;

typedef struct {
    void *context;
    /* The whole file mapped (NULL when it fails). */
    const uint8_t *(*map)(void *context, const char *path, int32_t *size);
    void (*unmap)(void *context, const uint8_t *data);
    /* Raw file I/O. */
    int32_t (*open)(void *context, const char *path);
    int32_t (*read_at)(void *context, int32_t handle, int32_t offset, void *out, int32_t n);
    void (*close)(void *context, int32_t handle);
    /* The temp directory and a fresh temp file name in it (GetTempPath /
       GetTempFileName), and writing the bytes there. */
    int (*temp_path)(void *context, char *out, int size);
    int (*temp_file)(void *context, const char *dir, char *out);
    int (*write_file)(void *context, const char *path, const void *data, int32_t n);
} AoeDrsFileOpsRor;

/* 0x0046afc0: add "<dir><name>" to the end of the list: mapped, or with
   `use_file_io` opened and its header (the first 0x40 bytes give the
   header size at +0x3c) read. The header's +0x2c password is compared
   with `password` (the result is not used). */
int aoe_drs_add_ror(AoeDrsFileRor **list, const AoeDrsFileOpsRor *ops, const char *name,
                    const char *password, const char *dir, int use_file_io);
/* 0x0046b1d0: remove and close an archive by name. */
void aoe_drs_remove_ror(AoeDrsFileRor **list, const AoeDrsFileOpsRor *ops, const char *name);
/* 0x0046b280: the first archive with (type, id): its handle, offset,
   mapping and size (handle -1 and zeros when absent, 0 returned). */
int aoe_drs_find_ror(const AoeDrsFileRor *list, uint32_t type, int32_t id, int32_t *handle,
                     int32_t *offset, const uint8_t **mapped, int32_t *size);
/* 0x0046b360: the resource's bytes: inside the mapping (`owned` 0), or
   read into a new buffer (`owned` 1, freed by the caller); NULL when
   absent or short. */
const uint8_t *aoe_drs_data_ror(const AoeDrsFileRor *list, const AoeDrsFileOpsRor *ops,
                                uint32_t type, int32_t id, int *owned, int32_t *size);
/* 0x0046b420: the resource written to a fresh temp file (prefix from the
   executable, "." without a temp path); 1 with the file. */
int aoe_drs_to_temp_ror(const AoeDrsFileRor *list, const AoeDrsFileOpsRor *ops, uint32_t type,
                        int32_t id, char *path);

/* ---- the stream file (0x0046b520..0x0046b7c0) ---- */

typedef struct {
    int32_t handle;          /* 0x007bfad0, -1 none */
    uint8_t mode;            /* 0x007bfad4: 1 writing, 2 reading */
    int32_t field_d8;        /* 0x007bfad8 */
    void *compressor;        /* 0x007bfadc */
    uint8_t *buffer;         /* 0x007bfae0, 64 KB */
    int32_t field_e4;        /* 0x007bfae4 */
    uint8_t *cursor;         /* 0x007bfae8 */
    int32_t remaining;       /* 0x007bfaec */
    void *field_f0;          /* 0x007bfaf0 */
    int32_t error;           /* 0x007bfaf4 */
} AoeStreamFileRor;

typedef struct {
    void *context;
    int32_t (*open)(void *context, const char *path, int32_t flags, int32_t mode);
    int32_t (*size)(void *context, int32_t handle);
    int32_t (*read)(void *context, int32_t handle, void *out, int32_t n);
    int32_t (*write)(void *context, int32_t handle, const void *data, int32_t n);
    int32_t (*close)(void *context, int32_t handle);
    /* The compressor's finish (0x005223e0(.., 0, 0, 1)): 2 is an error. */
    int32_t (*finish)(void *context, void *compressor);
    void (*free_compressor)(void *context, void *compressor);
} AoeStreamFileOpsRor;

/* 0x0046b520: VirtualFree (MEM_RELEASE) of a positive size. */
int aoe_stream_virtual_free_ror(int (*release)(void *context, void *p), void *context, void *p,
                                int32_t size);
/* 0x0046b550: an already open handle as the stream (reading, 64 KB
   buffer, the given size remaining). 0x0046b5b0 / 0x0046b630: open a file
   (with a share mode) and adopt it, its size remaining. */
int32_t aoe_stream_adopt_ror(AoeStreamFileRor *s, int32_t handle, int32_t size);
int32_t aoe_stream_open_ror(AoeStreamFileRor *s, const AoeStreamFileOpsRor *ops,
                            const char *path, int32_t flags, int32_t mode);
/* 0x0046b6b0: close; a writing stream's compressor is finished first (an
   error sets the error flag). Another handle is returned untouched. */
int32_t aoe_stream_close_ror(AoeStreamFileRor *s, const AoeStreamFileOpsRor *ops,
                             int32_t handle);
/* 0x0046b780 / 0x0046b7c0: raw read (the remaining count drops) and write
   (a short write sets the error flag), only on the stream's handle. */
void aoe_stream_read_raw_ror(AoeStreamFileRor *s, const AoeStreamFileOpsRor *ops,
                             int32_t handle, void *out, int32_t n);
void aoe_stream_write_raw_ror(AoeStreamFileRor *s, const AoeStreamFileOpsRor *ops,
                              int32_t handle, const void *data, int32_t n);

#endif
