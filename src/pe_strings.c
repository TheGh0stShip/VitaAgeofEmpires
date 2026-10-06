/* RT_STRING resources of a Win32 PE library, read portably. */
#include "pe_strings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "le_bytes.h"

enum { PE_RT_STRING = 6, PE_DIR_ENTRY = 8, PE_SECTION = 40 };

static int in_range(size_t size, size_t offset, size_t length) {
    return offset <= size && length <= size - offset;
}

/* Windows-1252 0x80..0x9f; 0 marks the unassigned ones. */
static const uint16_t cp1252_high[32] = {
    0x20ac, 0, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017d, 0,
    0, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0, 0x017e, 0x0178};

static char to_cp1252(uint16_t c) {
    int i;
    if (c < 0x80 || (c >= 0xa0 && c <= 0xff)) return (char)(uint8_t)c;
    for (i = 0; i < 32; ++i)
        if (cp1252_high[i] && cp1252_high[i] == c) return (char)(uint8_t)(0x80 + i);
    return '?';
}

/* The file offset of `rva`, through the section that holds it. */
static int rva_offset(const AoePeStringsRor *t, uint32_t rva, uint32_t length, size_t *out) {
    uint16_t i;
    for (i = 0; i < t->section_count; ++i) {
        const uint8_t *s = t->sections + (size_t)i * PE_SECTION;
        uint32_t va = aoe_le_u32(s + 12), vsize = aoe_le_u32(s + 8);
        uint32_t raw = aoe_le_u32(s + 20), rsize = aoe_le_u32(s + 16);
        uint32_t span = vsize > rsize ? vsize : rsize;
        if (rva >= va && rva - va < span) {
            size_t off = (size_t)raw + (rva - va);
            if (rva - va >= rsize || !in_range(t->size, off, length)) return 0;
            *out = off;
            return 1;
        }
    }
    return 0;
}

/* A resource directory (at `dir`, relative to the section) entry with the
   numeric `id` (any entry when id < 0); its offset field, or 0. */
static uint32_t dir_find(const AoePeStringsRor *t, uint32_t dir, int32_t id) {
    const uint8_t *base = t->data + t->rsrc_offset;
    uint32_t named, numbered, i;
    if (!in_range(t->rsrc_size, dir, 16)) return 0;
    named = aoe_le_u16(base + dir + 12);
    numbered = aoe_le_u16(base + dir + 14);
    for (i = 0; i < named + numbered; ++i) {
        size_t e = (size_t)dir + 16 + (size_t)i * PE_DIR_ENTRY;
        uint32_t name;
        if (!in_range(t->rsrc_size, e, PE_DIR_ENTRY)) return 0;
        name = aoe_le_u32(base + e);
        if (i < named || (name & 0x80000000u)) continue;
        if (id < 0 || name == (uint32_t)id) return aoe_le_u32(base + e + 4);
    }
    return 0;
}

int aoe_pe_strings_open(AoePeStringsRor *t, uint8_t *data, size_t size) {
    uint32_t pe, opt_size, rsrc_rva, rsrc_size;
    uint16_t magic, sections;
    size_t opt, dirs, rsrc;
    memset(t, 0, sizeof *t);
    t->data = data;
    t->size = size;
    if (!data || size < 0x40 || data[0] != 'M' || data[1] != 'Z') return 0;
    pe = aoe_le_u32(data + 0x3c);
    if (!in_range(size, pe, 24) || memcmp(data + pe, "PE\0\0", 4) != 0) return 0;
    sections = aoe_le_u16(data + pe + 6);
    opt_size = aoe_le_u16(data + pe + 20);
    opt = (size_t)pe + 24;
    if (!in_range(size, opt, opt_size) || opt_size < 2) return 0;
    magic = aoe_le_u16(data + opt);
    /* The data directories follow the PE32 (0x10b) or PE32+ (0x20b) header. */
    dirs = opt + (magic == 0x20b ? 112 : 96);
    if (magic != 0x10b && magic != 0x20b) return 0;
    if (dirs + 3 * 8 > opt + opt_size) return 0;
    rsrc_rva = aoe_le_u32(data + dirs + 2 * 8);
    rsrc_size = aoe_le_u32(data + dirs + 2 * 8 + 4);
    if (!in_range(size, opt + opt_size, (size_t)sections * PE_SECTION)) return 0;
    t->sections = data + opt + opt_size;
    t->section_count = sections;
    if (rsrc_rva == 0 || rsrc_size < 16) return 0;
    if (!rva_offset(t, rsrc_rva, 16, &rsrc)) return 0;
    if (rsrc_size > size - rsrc) rsrc_size = (uint32_t)(size - rsrc);
    t->rsrc_offset = rsrc;
    t->rsrc_size = rsrc_size;
    t->rsrc_rva = rsrc_rva;
    {
        uint32_t entry = dir_find(t, 0, PE_RT_STRING);
        if (!(entry & 0x80000000u)) return 0;
        t->string_dir = entry & 0x7fffffffu;
    }
    return 1;
}

int aoe_pe_strings_load(AoePeStringsRor *t, const char *path) {
    FILE *f = fopen(path, "rb");
    uint8_t *data;
    long n;
    memset(t, 0, sizeof *t);
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) <= 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }
    data = malloc((size_t)n);
    if (!data || fread(data, 1, (size_t)n, f) != (size_t)n) {
        free(data);
        fclose(f);
        return 0;
    }
    fclose(f);
    return aoe_pe_strings_open(t, data, (size_t)n);
}

void aoe_pe_strings_free(AoePeStringsRor *t) {
    free(t->data);
    memset(t, 0, sizeof *t);
}

int32_t aoe_pe_load_string_ror(const AoePeStringsRor *t, int32_t id, char *out, int32_t size) {
    const uint8_t *base;
    uint32_t low = (uint32_t)id & 0xffffu, block = (low >> 4) + 1, entry, lang, rva, len;
    size_t off, end, p;
    int index = (int)(low & 15), i;
    int32_t n, count;
    if (!t || !t->string_dir || size < 1) return 0;
    base = t->data + t->rsrc_offset;
    entry = dir_find(t, t->string_dir, (int32_t)block);
    if (!(entry & 0x80000000u)) return 0;
    lang = dir_find(t, entry & 0x7fffffffu, -1);
    if (lang & 0x80000000u) return 0;
    if (!in_range(t->rsrc_size, lang, 16)) return 0;
    rva = aoe_le_u32(base + lang);
    len = aoe_le_u32(base + lang + 4);
    if (!rva_offset(t, rva, len, &off)) return 0;
    end = off + len;
    p = off;
    for (i = 0; i < index; ++i) {
        if (p + 2 > end) return 0;
        p += 2 + 2 * (size_t)aoe_le_u16(t->data + p);
    }
    if (p + 2 > end) return 0;
    count = aoe_le_u16(t->data + p);
    p += 2;
    if (count == 0 || p + 2 * (size_t)count > end) return 0;
    n = count < size - 1 ? count : size - 1;
    for (i = 0; i < n; ++i) out[i] = to_cp1252(aoe_le_u16(t->data + p + 2 * (size_t)i));
    out[n] = '\0';
    return n;
}

void aoe_lang_string_ror(const AoePeStringsRor *expansion, const AoePeStringsRor *base, int32_t id,
                         char *out, int32_t size) {
    if (size < 1) return;
    if (aoe_pe_load_string_ror(expansion, id, out, size) == 0 &&
        aoe_pe_load_string_ror(base, id, out, size) == 0)
        out[0] = '\0';
    out[size - 1] = '\0';
}
