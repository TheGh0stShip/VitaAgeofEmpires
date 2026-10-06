/* The executable's stream decoder 0x00525b0b (see inflate.h). */
#include "inflate.h"

#include <stdlib.h>
#include <string.h>

enum {
    WINDOW = 0x8000,
    LIT_COUNT = 0x120,  /* 288 */
    DIST_COUNT = 0x20,
    CL_COUNT = 0x13
};

/* Where a call resumes after a flush that filled the output (state 2). */
enum { AT_SYMBOLS = 1, AT_STORED, AT_END, AT_FAILED };

/* A code (the descriptors at work +0xa8e0 / +0xa908 / +0xa930): the
   lengths, the codes, the first-level table on 8 bits and the tree for
   longer codes. Entries are symbol x 4; tree nodes are negative. */
typedef struct {
    uint32_t count;
    uint32_t *lengths;
    uint32_t *codes;
    int32_t table[256];
    int32_t *tree;
} Code;

struct AoeInflateRor {
    uint8_t window[WINDOW + 0x200];
    uint32_t pos, previous_end;           /* +0x8200, +0x8204 (offsets) */
    uint8_t *out;                         /* +0x820c */
    uint32_t out_left;                    /* +0x8210 */
    uint32_t out_size;                    /* +0x8218 */
    const uint8_t *in;                    /* +0x8220 */
    uint32_t in_left;                     /* +0x8224 */
    uint32_t in_size;                     /* +0x822c */
    uint32_t next_code[17];               /* +0x8230 (by length) */
    uint32_t length_count[17];            /* +0x8274 */
    uint32_t lit_count, dist_count;       /* +0x82b8, +0x82bc */
    uint32_t lit_codes[LIT_COUNT];        /* +0x82c0 */
    uint32_t lit_lengths[LIT_COUNT];      /* +0x8740 */
    int32_t lit_tree[LIT_COUNT * 2];      /* +0x8fc0 */
    uint32_t dist_codes[DIST_COUNT];      /* +0x98c0 */
    uint32_t dist_lengths[DIST_COUNT];    /* +0x9940 */
    int32_t dist_tree[DIST_COUNT * 2];    /* +0x9dc0 */
    uint32_t cl_codes[CL_COUNT];          /* +0x9ec0 */
    uint32_t cl_lengths[CL_COUNT];        /* +0x9f0c */
    /* +0xa358 the code-length tree; its last entry is what a leading 16
       repeats (+0xa3ec, cleared by the build). */
    int32_t cl_tree[CL_COUNT * 2];
    uint32_t lengths[LIT_COUNT + DIST_COUNT]; /* +0xa3f0 */
    Code lit, dist, cl;
    uint32_t state;                       /* +0xa958 */
    uint32_t final_block;                 /* +0xad84 */
    /* The registers kept across a suspension: edx and ch, the flush's
       source and remaining bytes, a stored block's remaining bytes. */
    uint32_t bits;
    int ch;
    uint32_t flush_from, flush_left;
    uint32_t stored_left;
    int resume;
    int result; /* port: the result once finished (-1 / -2) */
    uint32_t padded; /* port: 0xff bytes read past the input */
};

/* 0x00525060: the code-length code order. */
static const uint8_t cl_order[CL_COUNT] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
/* 0x005250ac / 0x00525128: symbols 257..285. */
static const uint8_t length_extra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                         2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
static const uint16_t length_base[29] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,
                                         15, 17, 19, 23, 27, 31, 35, 43, 51,  59,
                                         67, 83, 99, 115, 131, 163, 195, 227, 258};
/* 0x005251a4 / 0x0052521c: distance symbols 0..29. */
static const uint8_t dist_extra[30] = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                                       6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
static const uint16_t dist_base[30] = {1,    2,    3,    4,    5,    7,     9,     13,
                                       17,   25,   33,   49,   65,   97,    129,   193,
                                       257,  385,  513,  769,  1025, 1537,  2049,  3073,
                                       4097, 6145, 8193, 12289, 16385, 24577};

/* 0x005254fc: the next input byte into dh (0xff past the end). */
static void fetch(AoeInflateRor *z) {
    uint32_t b = 0xff;
    if (z->in_left) {
        b = *z->in++;
        --z->in_left;
    } else {
        ++z->padded;
    }
    z->bits = (z->bits & ~0xff00u) | (b << 8);
}

/* 0x0052554c: drops n (at most 8) bits. */
static void drop(AoeInflateRor *z, int n) {
    int left = z->ch - n;
    if (left > 0) {
        z->ch = left;
        z->bits >>= n;
        return;
    }
    z->bits >>= z->ch;
    fetch(z);
    z->ch = left + 8;
    z->bits >>= -left;
}

/* 0x00525564: drops 8 bits (ch unchanged). */
static void drop8(AoeInflateRor *z) {
    z->bits >>= z->ch;
    fetch(z);
    z->bits >>= 8 - z->ch;
}

/* 0x00525574: reads n (at most 16) bits. */
static uint32_t take(AoeInflateRor *z, int n) {
    uint32_t value;
    if (n <= 8) {
        value = z->bits & ((1u << n) - 1);
        drop(z, n);
        return value;
    }
    value = z->bits & 0xff;
    drop8(z);
    value |= (z->bits & ((1u << (n - 8)) - 1)) << 8;
    drop(z, n - 8);
    return value;
}

/* 0x005252b0: builds a code from its lengths; 0 when it is neither
   complete nor of at most one length. */
static int build(AoeInflateRor *z, Code *c) {
    uint32_t i, code = 0, used = 0;
    int32_t node = -1;
    memset(z->length_count, 0, sizeof z->length_count);
    for (i = 0; i < c->count; ++i) ++z->length_count[c->lengths[i]];
    z->next_code[1] = 0;
    for (i = 1; i < 16; ++i) {
        code = (code + z->length_count[i]) * 2;
        z->next_code[i + 1] = code;
    }
    if (code != 0x10000) {
        for (i = 1; i <= 16; ++i) used += z->length_count[i];
        if (used > 1) return 0;
    }
    for (i = 0; i < c->count; ++i)
        c->codes[i] = c->lengths[i] ? z->next_code[c->lengths[i]]++ : 0;
    memset(c->table, 0, sizeof c->table);
    memset(c->tree, 0, c->count * 2 * sizeof *c->tree);
    for (i = 0; i < c->count; ++i) {
        uint32_t length = c->lengths[i], reversed = 0, k;
        int32_t entry = (int32_t)(i * 4);
        if (!length) continue;
        for (k = 0; k < length; ++k) reversed = (reversed << 1) | ((c->codes[i] >> k) & 1);
        if (length <= 8) {
            for (k = reversed; k < 256; k += 1u << length) c->table[k] = entry;
        } else {
            /* 0x005253cb: the first 8 bits pick the table slot, each further
               bit a child; missing nodes are allocated as -1, -3, ... */
            int32_t *slot = &c->table[reversed & 0xff];
            uint32_t bit = 8;
            if (!*slot) {
                *slot = node;
                node -= 2;
            }
            for (; bit + 1 < length; ++bit) {
                int32_t *child = &c->tree[~*slot + ((reversed >> bit) & 1)];
                if (!*child) {
                    *child = node;
                    node -= 2;
                }
                slot = child;
            }
            c->tree[~*slot + ((reversed >> bit) & 1)] = entry;
        }
    }
    return 1;
}

/* 0x005255d8 / 0x00525624: the next symbol of a code. */
static uint32_t decode(AoeInflateRor *z, const Code *c) {
    int32_t entry = c->table[z->bits & 0xff];
    if (entry >= 0) {
        drop(z, (int)(c->lengths[entry >> 2] & 0xff));
        return (uint32_t)entry >> 2;
    }
    drop8(z);
    do {
        uint32_t index = (uint32_t)~entry;
        if (--z->ch < 0) {
            fetch(z);
            z->ch = 7;
        }
        index += z->bits & 1;
        z->bits >>= 1;
        entry = c->tree[index];
    } while (entry < 0);
    return (uint32_t)entry >> 2;
}

/* 0x0052566d: the window to the output; 0 when the output filled first
   (state 2). A new flush copies before it checks (so it suspends on no
   output space even with nothing to copy); a resumed one (0x005256b7)
   first checks what is left. */
static int flush(AoeInflateRor *z, int resumed) {
    if (!resumed || z->flush_left) {
        do {
            uint32_t n = z->flush_left < z->out_left ? z->flush_left : z->out_left;
            memcpy(z->out, z->window + z->flush_from, n);
            z->out += n;
            z->out_left -= n;
            z->flush_from += n;
            z->flush_left -= n;
            if (!z->out_left) {
                z->state = 2;
                return 0;
            }
        } while (z->flush_left);
    }
    z->pos = 0;
    return 1;
}

static int start_flush(AoeInflateRor *z, int resume) {
    z->previous_end = z->pos;
    z->flush_from = 0;
    z->flush_left = z->pos;
    z->resume = resume;
    return flush(z, 0);
}

/* 0x00525969..0x005259a9 tail: a fresh two-byte look-ahead. */
static void reload(AoeInflateRor *z) {
    z->bits = 0;
    fetch(z);
    z->bits = (z->bits >> 8) & 0xff;
    fetch(z);
    z->ch = 8;
}

/* 0x0052571c: the symbols of a fixed or dynamic block. 1 at its end,
   0 suspended, -1 failure. */
static int symbols(AoeInflateRor *z) {
    for (;;) {
        uint32_t sym = decode(z, &z->lit), length, distance;
        if (sym < 256) {
            z->window[z->pos++] = (uint8_t)sym;
        } else if (sym == 256) {
            return 1;
        } else {
            if (sym > 285) return -1; /* port: see inflate.h */
            length = length_base[sym - 257] + take(z, length_extra[sym - 257]);
            if (length > 0x102) return -1;
            sym = decode(z, &z->dist);
            if (sym > 29) return -1; /* port: see inflate.h */
            distance = dist_base[sym] + take(z, dist_extra[sym]);
            if (distance <= z->pos) {
                uint32_t from = z->pos - distance;
                while (length--) z->window[z->pos++] = z->window[from++];
            } else {
                /* 0x005257ae: before the start, from the previous fill's end. */
                uint32_t before = distance - z->pos, from = z->previous_end - before;
                uint32_t first = before < length ? before : length;
                length -= first;
                while (first--) z->window[z->pos++] = z->window[from++];
                from = 0;
                while (length--) z->window[z->pos++] = z->window[from++];
            }
        }
        if (z->pos >= WINDOW && !start_flush(z, AT_SYMBOLS)) return 0;
    }
}

/* 0x00525953 copy loop; 1 at the end, 0 suspended. */
static int stored_bytes(AoeInflateRor *z) {
    while (z->stored_left) {
        fetch(z);
        z->window[z->pos++] = (uint8_t)(z->bits >> 8);
        --z->stored_left;
        if (z->pos >= WINDOW && !start_flush(z, AT_STORED)) return 0;
    }
    reload(z);
    return 1;
}

/* 0x00525953: a stored block; 1, 0 suspended, -1 failure. */
static int stored(AoeInflateRor *z) {
    uint32_t length, check;
    if (z->ch != 8) drop(z, z->ch);
    length = z->bits & 0xffff;
    fetch(z);
    check = (z->bits >> 8) & 0xff;
    fetch(z);
    check |= z->bits & 0xff00;
    if ((~check & 0xffff) != length) return -1;
    z->stored_left = length;
    return stored_bytes(z);
}

/* 0x00525904: the fixed lengths. */
static void fixed(AoeInflateRor *z) {
    uint32_t i;
    for (i = 0; i < 0x90; ++i) z->lit_lengths[i] = 8;
    for (; i < 0x100; ++i) z->lit_lengths[i] = 9;
    for (; i < 0x118; ++i) z->lit_lengths[i] = 7;
    for (; i < LIT_COUNT; ++i) z->lit_lengths[i] = 8;
    for (i = 0; i < DIST_COUNT; ++i) z->dist_lengths[i] = 5;
}

/* 0x005257c9: a dynamic block's lengths; 0 on failure. */
static int dynamic(AoeInflateRor *z) {
    uint32_t i, total, *out = z->lengths;
    memset(z->lit_lengths, 0, sizeof z->lit_lengths);
    memset(z->dist_lengths, 0, sizeof z->dist_lengths);
    memset(z->cl_lengths, 0, sizeof z->cl_lengths);
    z->lit_count = take(z, 5) + 0x101;
    if (z->lit_count > 0x11e) return 0;
    z->dist_count = take(z, 5) + 1;
    if (z->dist_count > 0x1e) return 0;
    total = take(z, 4) + 4;
    for (i = 0; i < total; ++i) z->cl_lengths[cl_order[i]] = take(z, 3);
    if (!build(z, &z->cl)) return 0;
    total = z->lit_count + z->dist_count;
    while (total) {
        /* Code-length codes are at most 7 bits: the table always holds them. */
        uint32_t entry = (uint32_t)z->cl.table[z->bits & 0xff], sym = entry >> 2, repeat, value;
        drop(z, (int)(z->cl_lengths[sym] & 0xff));
        if (sym <= 15) {
            *out++ = sym;
            --total;
            continue;
        }
        if (sym == 16) {
            repeat = take(z, 2) + 3;
            value = out == z->lengths ? (uint32_t)z->cl_tree[CL_COUNT * 2 - 1] : out[-1];
        } else {
            repeat = sym == 17 ? take(z, 3) + 3 : take(z, 7) + 11;
            value = 0;
        }
        if (repeat > total) return 0;
        total -= repeat;
        while (repeat--) *out++ = value;
    }
    memcpy(z->lit_lengths, z->lengths, z->lit_count * sizeof *z->lengths);
    memcpy(z->dist_lengths, z->lengths + z->lit_count, z->dist_count * sizeof *z->lengths);
    return 1;
}

/* 0x005259c0 and the loop of 0x00525ab4: blocks until the final one.
   1 done, 0 suspended, -1 failure. */
static int blocks(AoeInflateRor *z) {
    for (;;) {
        int r;
        uint32_t type;
        z->final_block = take(z, 1);
        type = take(z, 2);
        if (type == 0) {
            r = stored(z);
        } else if (type == 3) {
            return -1;
        } else {
            if (type == 1)
                fixed(z);
            else if (!dynamic(z))
                return -1;
            /* 0x005256e6 */
            if (!build(z, &z->lit) || !build(z, &z->dist)) return -1;
            r = symbols(z);
        }
        if (r <= 0) return r;
        if (z->final_block == 1) return 1;
    }
}

/* After a resumed flush: back where it was. */
static int resume_blocks(AoeInflateRor *z) {
    int r;
    if (z->resume == AT_SYMBOLS)
        r = symbols(z);
    else
        r = stored_bytes(z);
    if (r <= 0) return r;
    if (z->final_block == 1) return 1;
    return blocks(z);
}

AoeInflateRor *aoe_inflate_new_ror(void) {
    AoeInflateRor *z = calloc(1, sizeof *z);
    if (!z) return NULL;
    /* 0x005259de: the descriptors. */
    z->lit = (Code){LIT_COUNT, z->lit_lengths, z->lit_codes, {0}, z->lit_tree};
    z->dist = (Code){DIST_COUNT, z->dist_lengths, z->dist_codes, {0}, z->dist_tree};
    z->cl = (Code){CL_COUNT, z->cl_lengths, z->cl_codes, {0}, z->cl_tree};
    return z;
}

void aoe_inflate_free_ror(AoeInflateRor *z) { free(z); }

int aoe_inflate_ror(AoeInflateRor *z, const uint8_t *src, uint32_t *src_len, uint8_t *dst,
                    uint32_t *dst_len) {
    int r;
    if (z->result) {
        *src_len = 0;
        *dst_len = 0;
        return z->result;
    }
    z->in = src;
    z->in_size = z->in_left = *src_len;
    z->out = dst;
    z->out_size = z->out_left = *dst_len;
    if (z->state == 0) {
        z->previous_end = WINDOW;
        z->pos = 0;
        reload(z);
        r = blocks(z);
    } else {
        if (!flush(z, 1)) {
            r = 0;
        } else if (z->resume == AT_END) {
            r = 2;
        } else if (z->resume == AT_FAILED) {
            r = 3;
        } else {
            r = resume_blocks(z);
        }
    }
    /* 0x00525ab4..0x00525b00: the final flush, or the flush before failing. */
    if (r == 1) r = start_flush(z, AT_END) ? 2 : 0;
    if (r == -1) r = start_flush(z, AT_FAILED) ? 3 : 0;
    *dst_len = z->out_size - z->out_left;
    if (r == 0) {
        *src_len = z->in_size - z->in_left;
        return 0;
    }
    if (r == 2) {
        *src_len = z->in_size - z->in_left;
        return z->result = -1;
    }
    *src_len = z->in_size;
    return z->result = -2;
}

int aoe_inflate_buffer_ror(const uint8_t *src, size_t size, size_t max_size, uint8_t **out,
                           size_t *out_size) {
    AoeInflateRor *z;
    uint8_t *buffer = NULL;
    size_t capacity = 0, produced = 0, consumed = 0;
    int r = 0;
    *out = NULL;
    *out_size = 0;
    if (size > UINT32_MAX || !(z = aoe_inflate_new_ror())) return 0;
    do {
        uint32_t in_len = (uint32_t)(size - consumed), out_len;
        if (produced == capacity) {
            size_t next = capacity ? capacity * 2 : 0x10000;
            uint8_t *grown;
            if (capacity >= max_size) break;
            if (next > max_size) next = max_size;
            if (!(grown = realloc(buffer, next))) break;
            buffer = grown;
            capacity = next;
        }
        out_len = capacity - produced > UINT32_MAX ? UINT32_MAX : (uint32_t)(capacity - produced);
        r = aoe_inflate_ror(z, src + consumed, &in_len, buffer + produced, &out_len);
        consumed += in_len;
        produced += out_len;
        if (z->padded > 2) r = -2;
    } while (r == 0);
    aoe_inflate_free_ror(z);
    if (r != -1) {
        free(buffer);
        return 0;
    }
    *out = buffer;
    *out_size = produced;
    return 1;
}
