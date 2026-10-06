/* The executable's stream encoder 0x005223e0 (see deflate_writer.h). */
#include "deflate_writer.h"

#include <stdlib.h>
#include <string.h>

enum {
    WINDOW = 0x8000,
    MIRROR = 0x1102,
    STEP = 0x1000,
    LIT = 0x120,
    DIST = 0x20,
    CL = 0x13,
    MAX_SYMBOLS = 0x3000,
    OUT_SIZE = 0x10000,
    CHAIN_LIMIT = 75, /* 0x0046ba40's argument */
    OPEN = 0x12345678,
    DONE = 0xabcd1234
};

/* The scratch the code builders keep in globals (0x007d24b0..0x007d3370). */
typedef struct {
    int32_t count[33];          /* 0x007d32e0, by length */
    int32_t offset[33];         /* 0x007d24b0, by length */
    int32_t sorted[LIT + 1];    /* 0x007d29bc, 1-based */
    int32_t lengths[LIT];       /* 0x007d2540 */
    int32_t used;               /* 0x007d2e40 */
    int32_t group[LIT];         /* 0x007d2e48: next symbol of a group */
    int32_t heap[LIT + 1];      /* 0x007d3368, 1-based */
} Huff;

struct AoeDeflateWriterRor {
    uint8_t window[WINDOW + MIRROR];      /* +0x0000 */
    uint16_t head[0x2000];                /* +0x9102 */
    uint16_t next[WINDOW];                /* +0xd102 */
    uint16_t back[WINDOW];                /* +0x1d102 */
    uint8_t symbols[MAX_SYMBOLS * 3];     /* +0x2d102 */
    uint32_t symbol_at;                   /* +0x36104 */
    int32_t symbol_count;                 /* +0x36108 */
    uint32_t flags[MAX_SYMBOLS / 32];     /* +0x3610c */
    uint32_t flag_word;                   /* +0x3670c (index) */
    int32_t flag_bits;                    /* +0x36710 */
    uint32_t stored_from, block_start;    /* +0x36714, +0x36718 */
    int32_t block_bytes;                  /* +0x3671c */
    int32_t lit_freq[LIT], dist_freq[DIST], cl_freq[CL]; /* +0x36720 */
    int32_t lit_len[LIT], dist_len[DIST], cl_len[CL];    /* +0x36c6c */
    uint32_t lit_code[LIT], dist_code[DIST], cl_code[CL]; /* +0x371b8 */
    int32_t all_len[LIT + DIST];          /* +0x37704 */
    int32_t runs[LIT + DIST];             /* +0x37c04 */
    uint32_t runs_end;                    /* +0x38104 (index) */
    int32_t lit_count, dist_count;        /* +0x38108, +0x3810c */
    uint32_t bits;                        /* +0x38110 */
    int32_t bit_count;                    /* +0x38114 */
    int32_t measuring;                    /* +0x38118 */
    uint32_t measured;                    /* +0x3811c */
    uint32_t cur;                         /* +0x38120 */
    int32_t lookahead;                    /* +0x38124 */
    uint32_t limit;                       /* +0x38128 */
    int32_t best;                         /* +0x3812c */
    uint32_t match;                       /* +0x38130 */
    int32_t finish;                       /* +0x38140 */
    int32_t wrapped;                      /* +0x38144 */
    uint32_t step;                        /* +0x38148 */
    uint32_t fill;                        /* +0x3814c */
    int32_t room;                         /* +0x38150 */
    const uint8_t *in;                    /* +0x38154 */
    int32_t in_left;                      /* +0x38158 */
    uint8_t out[OUT_SIZE];                /* +0x38164 (the caller's buffer) */
    uint32_t out_at;                      /* +0x38170 */
    int32_t out_left;                     /* +0x38174 */
    uint32_t magic;                       /* +0x38178 */
    AoeDeflateSinkRor sink;               /* +0x3816c */
    void *context;
    Huff h;
};

/* The code-length code order (0x0055fa20). */
static const uint8_t cl_order[CL] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
static const uint16_t length_base[29] = {3,  4,  5,  6,  7,  8,  9,  10,  11,  13,
                                         15, 17, 19, 23, 27, 31, 35, 43,  51,  59,
                                         67, 83, 99, 115, 131, 163, 195, 227, 258};
static const uint8_t length_extra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                         2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
static const uint16_t dist_base[30] = {1,    2,    3,    4,    5,    7,     9,     13,
                                       17,   25,   33,   49,   65,   97,    129,   193,
                                       257,  385,  513,  769,  1025, 1537,  2049,  3073,
                                       4097, 6145, 8193, 12289, 16385, 24577};
static const uint8_t dist_extra[30] = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                                       6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

/* The length and distance tables at 0x0055fa38..0x00560738 hold, for a
   length - 3 (0..255) and a distance - 1, the symbol, its extra bits and
   their mask (applied to length - 3 / distance - 1); they are the
   deflate bases above. */
static int length_symbol(uint32_t length_3) {
    int i = 28;
    if (length_3 + 3 < 258)
        while (length_base[i] > length_3 + 3) --i;
    return i;
}

static int dist_symbol(uint32_t distance) {
    int i = 29;
    while (dist_base[i] > distance) --i;
    return i;
}

/* 0x00522730: the output buffer to the sink. */
static int hand_on(AoeDeflateWriterRor *w) {
    if (w->sink(w->context, w->out, (size_t)(OUT_SIZE - w->out_left))) return 1;
    w->out_at = 0;
    w->out_left = OUT_SIZE;
    return 0;
}

/* One byte to the output, handing the buffer on when full. */
static int put_byte(AoeDeflateWriterRor *w, uint8_t b) {
    while (w->out_left < 1)
        if (hand_on(w)) return 1;
    --w->out_left;
    w->out[w->out_at++] = b;
    return 0;
}

/* 0x005227c0: n bits, LSB first; only counted while measuring. */
static int put_bits(AoeDeflateWriterRor *w, uint32_t value, int n) {
    if (w->measuring) {
        w->measured += (uint32_t)n;
        return 0;
    }
    w->bits |= value << w->bit_count;
    w->bit_count += n;
    while (w->bit_count >= 8) {
        if (put_byte(w, (uint8_t)w->bits)) return 1;
        w->bits >>= 8;
        w->bit_count -= 8;
    }
    return 0;
}

/* 0x00522790: to a byte boundary. */
static int align(AoeDeflateWriterRor *w) {
    if (put_bits(w, 0, 7)) return 1;
    w->bit_count = 0;
    return 0;
}

/* 0x00523660: sifts heap[k] down; the smaller frequency, and on equal
   frequencies the higher symbol, rises. */
static void sift(Huff *h, const int32_t *freq, int32_t n, int32_t k) {
    int32_t v = h->heap[k], j = k * 2;
    while (j <= n) {
        int32_t c;
        if (j < n) {
            int32_t r = h->heap[j + 1], l = h->heap[j];
            if (freq[r] < freq[l] || (freq[r] == freq[l] && l < r)) ++j;
        }
        c = h->heap[j];
        if (freq[v] < freq[c] || (freq[c] == freq[v] && c < v)) break;
        h->heap[k] = c;
        k = j;
        j = k * 2;
    }
    h->heap[k] = v;
}

/* 0x00523540: code lengths from frequencies (which it overwrites). */
static void lengths_of(Huff *h, int32_t n, int32_t *freq, int32_t *len) {
    int32_t i, m = 0;
    for (i = 0; i < n; ++i) {
        h->group[i] = -1;
        len[i] = 0;
    }
    for (i = 0; i < n; ++i)
        if (freq[i]) h->heap[++m] = i;
    if (m < 2) {
        if (m == 1) len[h->heap[1]] = 1;
        return;
    }
    for (i = m >> 1; i != 0; --i) sift(h, freq, m, i);
    do {
        int32_t a = h->heap[1], b, x, last;
        h->heap[1] = h->heap[m];
        --m;
        sift(h, freq, m, 1);
        b = h->heap[1];
        freq[b] += freq[a];
        sift(h, freq, m, 1);
        x = b;
        do {
            last = x;
            ++len[x];
            x = h->group[x];
        } while (x != -1);
        h->group[last] = a;
        for (x = a; x != -1; x = h->group[x]) ++len[x];
    } while (m != 1);
}

/* 0x005231b0: counts by length and the symbols in length order. */
static void sort_lengths(Huff *h, int32_t n, const int32_t *len) {
    int32_t i, sum = 0;
    h->used = 0;
    memset(h->count, 0, sizeof h->count);
    for (i = 0; i < n; ++i) ++h->count[len[i]];
    for (i = 1; i <= 32; ++i) {
        h->offset[i] = sum;
        sum += h->count[i];
    }
    for (i = 0; i < n; ++i)
        if (len[i]) {
            ++h->used;
            h->sorted[++h->offset[len[i]]] = i;
        }
}

/* 0x005236f0: limits the counts to `max` bits. */
static void limit_lengths(Huff *h, int32_t max) {
    int32_t l, sum = 0, over;
    if (h->used <= 1) return;
    for (l = max + 1; l <= 32; ++l) h->count[max] += h->count[l];
    for (l = max; l > 0; --l) sum += h->count[l] << (max - l);
    if (sum == 1 << max) return;
    over = sum - (1 << max);
    do {
        --h->count[max];
        for (l = max - 1; l > 0; --l)
            if (h->count[l]) {
                --h->count[l];
                h->count[l + 1] += 2;
                break;
            }
    } while (--over != 0);
}

/* 0x00523240: lengths from the counts in sorted order, then the
   canonical codes, bit-reversed. */
static void assign_codes(Huff *h, int32_t n, int32_t *len, int32_t max, uint32_t *codes) {
    int32_t i, l, k = 0, code = 0;
    if (!h->used) return;
    for (i = 0; i < n; ++i) codes[i] = 0;
    for (l = 1; l <= max; ++l)
        for (i = 0; i < h->count[l]; ++i) h->lengths[k++] = l;
    h->offset[1] = 0;
    for (l = 2; l <= max; ++l) {
        code = (h->count[l - 1] + code) * 2;
        h->offset[l] = code;
    }
    for (i = 0; i < h->used; ++i) len[h->sorted[i + 1]] = h->lengths[i];
    for (i = 0; i < n; ++i)
        if (len[i]) {
            uint32_t c = (uint32_t)h->offset[len[i]]++, r = 0;
            for (l = 0; l < len[i]; ++l) {
                r = r * 2 | (c & 1);
                c >>= 1;
            }
            codes[i] = r;
        }
}

/* 0x005230a0: the fixed code. */
static void fixed_code(AoeDeflateWriterRor *w) {
    int32_t i;
    for (i = 0; i < 0x90; ++i) w->lit_len[i] = 8;
    for (; i < 0x100; ++i) w->lit_len[i] = 9;
    for (; i < 0x118; ++i) w->lit_len[i] = 7;
    for (; i < (int32_t)LIT; ++i) w->lit_len[i] = 8;
    sort_lengths(&w->h, LIT, w->lit_len);
    assign_codes(&w->h, LIT, w->lit_len, 15, w->lit_code);
    for (i = 0; i < (int32_t)DIST; ++i) w->dist_len[i] = 5;
    sort_lengths(&w->h, DIST, w->dist_len);
    assign_codes(&w->h, DIST, w->dist_len, 15, w->dist_code);
}

/* 0x00523a80: n repeats of `value` (16 when over 2). */
static void run_of(AoeDeflateWriterRor *w, int32_t value, int32_t n) {
    if (n > 2) {
        ++w->cl_freq[16];
        w->runs[w->runs_end++] = 16;
        w->runs[w->runs_end++] = n - 3;
        return;
    }
    w->cl_freq[value] += n;
    while (n--) w->runs[w->runs_end++] = value;
}

/* 0x00523ae0: n zeros (17 for 3..10, 18 above). */
static void zeros_of(AoeDeflateWriterRor *w, int32_t n) {
    if (n > 2) {
        if (n < 11) {
            ++w->cl_freq[17];
            w->runs[w->runs_end++] = 17;
            w->runs[w->runs_end++] = n - 3;
        } else {
            ++w->cl_freq[18];
            w->runs[w->runs_end++] = 18;
            w->runs[w->runs_end++] = n - 11;
        }
        return;
    }
    w->cl_freq[0] += n;
    while (n--) w->runs[w->runs_end++] = 0;
}

/* 0x005237a0: the dynamic header's lengths and their code. */
static void header_code(AoeDeflateWriterRor *w) {
    int32_t i, prev = 0xff, zeros = 0, repeats = 0, value = 0, total;
    memset(w->cl_freq, 0, sizeof w->cl_freq);
    for (w->lit_count = 0x11d; w->lit_count >= 0 && !w->lit_len[w->lit_count]; --w->lit_count) {}
    if (++w->lit_count < 0x102) w->lit_count = 0x101;
    for (w->dist_count = 0x1d; w->dist_count >= 0 && !w->dist_len[w->dist_count]; --w->dist_count) {}
    if (++w->dist_count < 2) w->dist_count = 1;
    memcpy(w->all_len, w->lit_len, (size_t)w->lit_count * sizeof *w->all_len);
    memcpy(w->all_len + w->lit_count, w->dist_len, (size_t)w->dist_count * sizeof *w->all_len);
    w->runs_end = 0;
    total = w->lit_count + w->dist_count;
    for (i = 0; i < total; ++i) {
        value = w->all_len[i];
        if (value == 0) {
            if (repeats) {
                run_of(w, prev, repeats);
                repeats = 0;
            }
            if (++zeros == 0x8a) {
                zeros_of(w, 0x8a);
                zeros = 0;
            }
        } else {
            if (zeros) {
                zeros_of(w, zeros);
                zeros = 0;
            }
            if (prev == value) {
                if (++repeats == 6) {
                    run_of(w, prev, 6);
                    repeats = 0;
                }
            } else {
                if (repeats) {
                    run_of(w, prev, repeats);
                    repeats = 0;
                }
                ++w->cl_freq[value];
                w->runs[w->runs_end++] = value;
            }
        }
        prev = value;
    }
    if (repeats == 0) {
        if (zeros) zeros_of(w, zeros);
    } else {
        run_of(w, value, repeats);
    }
    lengths_of(&w->h, CL, w->cl_freq, w->cl_len);
    sort_lengths(&w->h, CL, w->cl_len);
    limit_lengths(&w->h, 7);
    assign_codes(&w->h, CL, w->cl_len, 7, w->cl_code);
}

/* 0x00523350: the dynamic codes from the block's symbols. */
static void dynamic_code(AoeDeflateWriterRor *w) {
    const uint8_t *s = w->symbols;
    uint32_t flags = 0;
    int32_t i, bits = 0;
    memset(w->lit_freq, 0, sizeof w->lit_freq);
    memset(w->dist_freq, 0, sizeof w->dist_freq);
    for (i = 0; i < w->symbol_count; ++i) {
        if (!bits) {
            flags = w->flags[i / 32];
            bits = 32;
        }
        if (!(flags & 0x80000000u)) {
            ++w->lit_freq[*s++];
        } else {
            ++w->lit_freq[257 + length_symbol(s[0])];
            ++w->dist_freq[dist_symbol((uint32_t)(s[1] | s[2] << 8))];
            s += 3;
        }
        flags <<= 1;
        --bits;
    }
    ++w->lit_freq[256];
    lengths_of(&w->h, LIT, w->lit_freq, w->lit_len);
    sort_lengths(&w->h, LIT, w->lit_len);
    limit_lengths(&w->h, 15);
    assign_codes(&w->h, LIT, w->lit_len, 15, w->lit_code);
    lengths_of(&w->h, DIST, w->dist_freq, w->dist_len);
    sort_lengths(&w->h, DIST, w->dist_len);
    limit_lengths(&w->h, 15);
    assign_codes(&w->h, DIST, w->dist_len, 15, w->dist_code);
    header_code(w);
}

/* 0x00523b60 / 0x00523b90: BTYPE 2 and the dynamic header. */
static int dynamic_header(AoeDeflateWriterRor *w) {
    int32_t n, i;
    uint32_t r;
    if (put_bits(w, 2, 2) || put_bits(w, (uint32_t)(w->lit_count - 0x101), 5) ||
        put_bits(w, (uint32_t)(w->dist_count - 1), 5))
        return 1;
    for (n = 0x12; n >= 0 && !w->cl_len[cl_order[n]]; --n) {}
    if (++n < 5) n = 4;
    if (put_bits(w, (uint32_t)(n - 4), 4)) return 1;
    for (i = 0; i < n; ++i)
        if (put_bits(w, (uint32_t)w->cl_len[cl_order[i]], 3)) return 1;
    for (r = 0; r < w->runs_end;) {
        int32_t sym = w->runs[r++];
        if (put_bits(w, w->cl_code[sym], w->cl_len[sym])) return 1;
        if (sym == 16 && put_bits(w, (uint32_t)w->runs[r++], 2)) return 1;
        if (sym == 17 && put_bits(w, (uint32_t)w->runs[r++], 3)) return 1;
        if (sym == 18 && put_bits(w, (uint32_t)w->runs[r++], 7)) return 1;
    }
    return 0;
}

/* 0x00522e50: the block's symbols and its end. */
static int put_symbols(AoeDeflateWriterRor *w) {
    const uint8_t *s = w->symbols;
    uint32_t flags = 0;
    int32_t i, bits = 0;
    for (i = 0; i < w->symbol_count; ++i) {
        if (!bits) {
            flags = w->flags[i / 32];
            bits = 32;
        }
        if (!(flags & 0x80000000u)) {
            if (put_bits(w, w->lit_code[*s], w->lit_len[*s])) return 1;
            ++s;
        } else {
            uint32_t length_3 = s[0], distance = (uint32_t)(s[1] | s[2] << 8);
            int l = length_symbol(length_3), d = dist_symbol(distance);
            if (put_bits(w, w->lit_code[257 + l], w->lit_len[257 + l]) ||
                put_bits(w, length_3 & ((1u << length_extra[l]) - 1), length_extra[l]) ||
                put_bits(w, w->dist_code[d], w->dist_len[d]) ||
                put_bits(w, (distance - 1) & ((1u << dist_extra[d]) - 1), dist_extra[d]))
                return 1;
            s += 3;
        }
        flags <<= 1;
        --bits;
    }
    return put_bits(w, w->lit_code[256], w->lit_len[256]);
}

static int fixed_block(AoeDeflateWriterRor *w) {
    fixed_code(w);
    return put_bits(w, 1, 2) || put_symbols(w);
}

/* 0x00523d30: a stored block of the block's bytes from the window. */
static int stored_block(AoeDeflateWriterRor *w) {
    uint32_t from = w->stored_from, n = (uint32_t)w->block_bytes;
    if (put_bits(w, 0, 2) || align(w) || put_byte(w, (uint8_t)n) ||
        put_byte(w, (uint8_t)(n >> 8)) || put_byte(w, (uint8_t)~n) ||
        put_byte(w, (uint8_t)(~n >> 8)))
        return 1;
    while (n--) {
        if (put_byte(w, w->window[from])) return 1;
        from = (from + 1) & 0x7fff;
    }
    return 0;
}

/* The stored size in bits after BFINAL. */
static uint32_t stored_size(const AoeDeflateWriterRor *w) {
    uint32_t size = (uint32_t)w->block_bytes * 8 + 0x22, pad = (uint32_t)(w->bit_count + 2) & 7;
    return pad ? size - pad + 8 : size;
}

/* 0x00522af0: writes the block (block mode 2), then the empty final
   block when `final`. */
static int put_block(AoeDeflateWriterRor *w, int final) {
    w->block_start = w->cur;
    if (w->symbol_count) {
        if (put_bits(w, 0, 1)) return 1;
        if (w->symbol_count < 0x80) {
            uint32_t fixed, dynamic, stored;
            w->measuring = 1;
            w->measured = 0;
            if (fixed_block(w)) return 1;
            fixed = w->measured;
            w->measured = 0;
            dynamic_code(w);
            if (dynamic_header(w) || put_symbols(w)) return 1;
            dynamic = w->measured;
            w->measuring = 0;
            stored = stored_size(w);
            if (stored < fixed && stored < dynamic) {
                if (stored_block(w)) return 1;
            } else if (fixed < dynamic) {
                if (fixed_block(w)) return 1;
            } else if (dynamic_header(w) || put_symbols(w)) {
                return 1;
            }
        } else if (w->block_bytes < 0x3999) {
            uint32_t dynamic;
            w->measuring = 1;
            w->measured = 0;
            dynamic_code(w);
            if (dynamic_header(w) || put_symbols(w)) return 1;
            dynamic = w->measured;
            w->measuring = 0;
            if (stored_size(w) < dynamic) {
                if (stored_block(w)) return 1;
            } else if (dynamic_header(w) || put_symbols(w)) {
                return 1;
            }
        } else {
            dynamic_code(w);
            if (dynamic_header(w) || put_symbols(w)) return 1;
        }
    }
    w->flag_word = 0;
    w->flag_bits = 32;
    w->symbol_at = 0;
    w->symbol_count = 0;
    w->block_bytes = 0;
    w->stored_from = w->block_start;
    if (final) return put_bits(w, 1, 1) || fixed_block(w);
    return 0;
}

/* 0x00524560: a full flag word; a block at 0x3000 symbols. */
static int next_flags(AoeDeflateWriterRor *w) {
    ++w->flag_word;
    w->flag_bits = 32;
    w->symbol_count += 32;
    return w->symbol_count == MAX_SYMBOLS ? put_block(w, 0) : 0;
}

static int literal(AoeDeflateWriterRor *w) {
    w->symbols[w->symbol_at++] = w->window[w->cur];
    ++w->cur;
    --w->lookahead;
    ++w->block_bytes;
    w->flags[w->flag_word] <<= 1;
    return --w->flag_bits == 0 && next_flags(w);
}

static int matched(AoeDeflateWriterRor *w, uint32_t distance) {
    w->symbols[w->symbol_at++] = (uint8_t)(w->best - 3);
    w->symbols[w->symbol_at++] = (uint8_t)distance;
    w->symbols[w->symbol_at++] = (uint8_t)(distance >> 8);
    w->cur += (uint32_t)w->best;
    w->lookahead -= w->best;
    w->block_bytes += w->best;
    w->flags[w->flag_word] = w->flags[w->flag_word] * 2 | 1;
    return --w->flag_bits == 0 && next_flags(w);
}

static uint16_t word_at(const AoeDeflateWriterRor *w, uint32_t at) {
    return (uint16_t)(w->window[at] | w->window[at + 1] << 8);
}

/* 0x005243b0: the longest match along the chain from `at`. The budget
   is tested only at the top of the loop, which the unrolled body reaches
   after four links, or after any link whose last word matches; so up to
   three links more than the budget are visited. */
static void longest(AoeDeflateWriterRor *w, uint32_t at) {
    uint32_t index = at & 0x7fff;
    uint16_t scan = word_at(w, at), tail = word_at(w, at + (uint32_t)w->best - 1);
    int32_t budget = CHAIN_LIMIT;
    while (budget > 0) {
        uint32_t p, q;
        int32_t groups = 0x20, length, k;
        for (k = 0; k < 4; ++k) {
            --budget;
            index = w->next[index];
            if (index == 0xffff) return;
            if (word_at(w, index + (uint32_t)w->best - 1) == tail) break;
        }
        if (k == 4 || word_at(w, index) != scan) continue;
        p = at;
        q = index;
        for (;;) {
            for (k = 0; k < 4; ++k) {
                p += 2;
                q += 2;
                if (word_at(w, q) != word_at(w, p)) goto mismatch;
            }
            if (--groups <= 0) break;
        }
        w->best = 0x102;
        w->match = index;
        return;
    mismatch:
        length = (int32_t)((p - at) & ~1u) + (w->window[q] == w->window[p]);
        if (length > w->best) {
            w->best = length;
            w->match = index;
            tail = word_at(w, at + (uint32_t)length - 1);
        }
    }
}

/* 0x005245b0: the symbols up to the limit. */
static int match_step(AoeDeflateWriterRor *w) {
    while (w->lookahead != 0 && w->cur < w->limit) {
        int r;
        if (w->next[w->cur & 0x7fff] == 0xffff) {
            r = literal(w);
        } else {
            w->best = 2;
            longest(w, w->cur);
            if (w->best == 2) {
                r = literal(w);
            } else if (w->lookahead < w->best && (w->best = w->lookahead) < 3) {
                r = literal(w);
            } else {
                uint32_t distance = (w->cur - w->match) & 0x7fff;
                r = w->best == 3 && distance > 0x3fff ? literal(w) : matched(w, distance);
            }
        }
        if (r) return 1;
    }
    w->cur &= 0x7fff;
    return 0;
}

/* 0x00522930: unhooks the step about to be overwritten. */
static void unhook(AoeDeflateWriterRor *w, uint32_t from) {
    uint32_t p;
    for (p = from; p < from + STEP; ++p) {
        uint16_t v = w->back[p];
        if (!(v & 0x8000))
            w->next[v] = 0xffff;
        else if (v != 0xffff)
            w->head[v & 0x7fff] = 0xffff;
    }
}

/* 0x00522980: hashes positions [from, from + n - 2). */
static void hook(AoeDeflateWriterRor *w, uint32_t from, int32_t n) {
    int32_t keep = n - 2 < 1 ? 0 : n - 2;
    uint32_t p, h;
    for (p = from + (uint32_t)keep; (int32_t)(p - from) < n; ++p) w->back[p] = w->next[p] = 0xffff;
    if (n <= 2) return;
    h = (uint32_t)w->window[from] << 5 ^ w->window[from + 1];
    for (p = from; p < from + (uint32_t)n - 2; ++p) {
        h = w->window[p + 2] ^ (h & 0xff) << 5;
        w->back[p] = (uint16_t)(h | 0x8000);
        w->next[p] = w->head[h];
        if (w->next[p] != 0xffff) w->back[w->next[p]] = (uint16_t)p;
        w->head[h] = (uint16_t)p;
    }
}

/* 0x00524dc0: input into the step; 1 while the step is not full. */
static int take_input(AoeDeflateWriterRor *w) {
    int32_t n = w->in_left < w->room ? w->in_left : w->room;
    if (n) memcpy(w->window + w->fill, w->in, (size_t)n);
    w->in += n;
    w->in_left -= n;
    w->fill = (w->fill + (uint32_t)n) & 0x7fff;
    w->room -= n;
    w->lookahead += n;
    if (w->room) {
        if (w->finish) memset(w->window + w->fill, 0, (size_t)w->room);
        return 1;
    }
    w->room = STEP;
    return 0;
}

/* 0x00522570. */
static int run(AoeDeflateWriterRor *w) {
    for (;;) {
        uint32_t ahead;
        if (take_input(w) && !w->finish) return 0;
        if (w->wrapped) unhook(w, w->step);
        hook(w, w->step, w->lookahead);
        if (w->step == 0) memcpy(w->window + WINDOW, w->window, MIRROR);
        /* 0x00523ea0 */
        ahead = (w->step - w->cur) & 0x7fff;
        w->lookahead += (int32_t)ahead;
        w->limit = w->cur + ahead + 0xefd;
        if (match_step(w)) return 1;
        w->step += STEP;
        if (w->step == WINDOW) {
            w->step = 0;
            w->wrapped = 1;
        }
        if (w->finish && w->in_left == 0) break;
        w->lookahead = 0;
    }
    /* 0x00524da0, 0x00522a70 */
    w->limit = 0xffff;
    if (match_step(w)) return 1;
    if (w->flag_bits != 32) {
        w->symbol_count += 32 - w->flag_bits;
        w->flags[w->flag_word] <<= w->flag_bits;
        ++w->flag_word;
        w->flag_bits = 32;
    }
    if (put_block(w, 1) || align(w) || hand_on(w)) return 1;
    w->magic = DONE;
    return 1;
}

AoeDeflateWriterRor *aoe_deflate_writer_new_ror(AoeDeflateSinkRor sink, void *context) {
    AoeDeflateWriterRor *w;
    if (!sink || !(w = calloc(1, sizeof *w))) return NULL;
    /* 0x00524ef0 / 0x00524fc0 */
    w->out_left = OUT_SIZE;
    w->sink = sink;
    w->context = context;
    w->room = STEP;
    memset(w->head, 0xff, sizeof w->head);
    memset(w->next, 0xff, sizeof w->next);
    memset(w->back, 0xff, sizeof w->back);
    w->flag_bits = 32;
    w->magic = OPEN;
    return w;
}

int aoe_deflate_writer_write_ror(AoeDeflateWriterRor *w, const void *data, size_t size,
                                 int finish) {
    if (!w || w->magic != OPEN || size > INT32_MAX || (size && !data)) return 2;
    w->in = data;
    w->in_left = (int32_t)size;
    w->finish = finish;
    return run(w);
}

void aoe_deflate_writer_free_ror(AoeDeflateWriterRor *w) { free(w); }
