#include "world_ror_update.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "effects_io.h"

/* The text 0x004e9fa4 reads with fscanf (0x5277d7), as a cursor. */
typedef struct {
    const char *p, *end;
} TechText;

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' ||
           c == '\r';
}

/* White space before a conversion; 0 at the end of the text. */
static int skip_space(TechText *in) {
    while (in->p < in->end && is_space(*in->p)) ++in->p;
    return in->p < in->end;
}

/* %d / %hd: an optional sign then decimal digits, accumulated to 32 bits
   with wraparound. 1 matched, 0 a matching failure, -1 the end. */
static int scan_number(TechText *in, uint32_t *out) {
    uint32_t v = 0;
    int negative = 0, digits = 0;
    if (!skip_space(in)) return -1;
    if (*in->p == '+' || *in->p == '-') {
        negative = *in->p == '-';
        ++in->p;
    }
    while (in->p < in->end && *in->p >= '0' && *in->p <= '9') {
        v = v * 10u + (uint32_t)(*in->p - '0');
        ++in->p;
        ++digits;
    }
    if (!digits) return 0;
    *out = negative ? 0u - v : v;
    return 1;
}

/* %s into the 100-byte name buffer (local_84): the whole word is
   consumed, its first 99 characters kept. */
static int scan_word(TechText *in, char *out) {
    size_t n = 0;
    if (!skip_space(in)) return -1;
    while (in->p < in->end && !is_space(*in->p)) {
        if (n < 99) out[n++] = *in->p;
        ++in->p;
    }
    out[n] = '\0';
    return 1;
}

/* One fscanf: 'd' an int32_t *, 'h' an int16_t *, 's' a 100-byte buffer.
   Stops at the first failure; returns the conversions, or -1 (EOF) when
   the text ended before the first. */
static int scan(TechText *in, const char *kinds, ...) {
    va_list ap;
    int n = 0;
    va_start(ap, kinds);
    for (; *kinds; ++kinds) {
        int r;
        if (*kinds == 's') {
            r = scan_word(in, va_arg(ap, char *));
        } else {
            uint32_t v = 0;
            r = scan_number(in, &v);
            if (r == 1 && *kinds == 'd') *va_arg(ap, int32_t *) = (int32_t)v;
            else if (r == 1) *va_arg(ap, int16_t *) = (int16_t)(v & 0xffffu);
        }
        if (r != 1) {
            va_end(ap);
            return r < 0 && n == 0 ? -1 : n;
        }
        ++n;
    }
    va_end(ap);
    return n;
}

/* 0x004e9fa4 */
int aoe_techs_read_text_ror(DatTechs *techs, const char *text, size_t size) {
    TechText in;
    /* local_c (listed), local_8c (index), local_84 (name), local_18
       (button) and local_10 (paid) live across the whole loop. */
    int32_t count = 0, listed = 0, index = 0;
    int16_t button = 0, paid = 0, n, i;
    char name[100] = "";
    DatTech scratch;

    if (!techs) return 0;
    memset(techs, 0, sizeof *techs); /* *this = 0 */
    if (!text) return 0;             /* fopen 0x5263f1 failed */
    in.p = text;
    in.end = text + size;
    /* "%d %d": EOF or a non-positive int16 size leaves no list. */
    if (scan(&in, "dd", &count, &listed) == -1) return 1;
    n = (int16_t)count;
    if (n < 1) return 1;
    techs->techs = calloc((size_t)n, sizeof *techs->techs); /* 0x52607b(0x3c, n) */
    if (!techs->techs) return 0;
    techs->count = (uint16_t)n;
    memset(&scratch, 0, sizeof scratch);

    for (i = 0; i < (int16_t)listed; ++i) {
        DatTech *t;
        int16_t at, j, name_id, desc_id;
        size_t k, len;
        scan(&in, "d", &index); /* 0x0055a1c8 */
        at = (int16_t)index;
        /* Port guard: the original writes any index it reads. */
        t = (at >= 0 && at < n) ? &techs->techs[at] : &scratch;
        scan(&in, "shhhhhhh", name, &t->required_count, &t->research_time,
             &t->effect_id, &t->field_22, &t->field_24, &button,
             &t->research_location); /* 0x0055a1cc */
        t->button_id = (int8_t)button; /* +0x26 = low byte of local_18 */
        len = strlen(name);
        for (k = 0; k < len; ++k)
            if (name[k] == '_') name[k] = ' ';
        /* +0x00 = 0, then 0x00450ea0 copies the name; the port frees a
           re-listed record's old name instead of leaking it. */
        aoe_text_set_ror(&t->name, name);
        for (j = 0; j < 4; ++j)
            scan(&in, "h", &t->required_techs[j]); /* 0x0055a1ec */
        for (j = 0; j < 3; ++j) {
            scan(&in, "hhh", &t->costs[j].resource_type, &t->costs[j].amount,
                 &paid); /* 0x0055a1f0 */
            t->costs[j].paid = (int8_t)paid; /* +0x1a + j = low byte */
        }
        name_id = (int16_t)t->language_name;
        desc_id = (int16_t)t->language_description;
        scan(&in, "hhddd", &name_id, &desc_id, &t->language_help,
             &t->language_tree, &t->hotkey); /* 0x0055a1fc */
        t->language_name = (uint16_t)name_id;
        t->language_description = (uint16_t)desc_id;
    }
    free(scratch.name);
    return 1; /* fclose 0x5262f0 */
}

int aoe_techs_load_text_file_ror(DatTechs *techs, const char *path) {
    FILE *f;
    char *text = NULL;
    size_t size = 0, cap = 0;
    int ok;
    if (!techs) return 0;
    memset(techs, 0, sizeof *techs);
    if (!path) return 0;
    f = fopen(path, "r"); /* 0x005263f1(path, "r" 0x0055a1bc) */
    if (!f) return 0;
    for (;;) {
        size_t got;
        if (size == cap) {
            size_t grow = cap ? cap * 2 : 4096;
            char *more = realloc(text, grow);
            if (!more) {
                free(text);
                fclose(f);
                return 0;
            }
            text = more;
            cap = grow;
        }
        got = fread(text + size, 1, cap - size, f);
        size += got;
        if (got == 0) break;
    }
    fclose(f);
    ok = aoe_techs_read_text_ror(techs, text ? text : "", size);
    free(text);
    return ok;
}
