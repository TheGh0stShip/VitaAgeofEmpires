#ifndef VITA_AOE_STREAM_H
#define VITA_AOE_STREAM_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "le_bytes.h"
#include "player_outcome.h" /* AoeOutcomeStream */

/* Little-endian, unaligned reads and writes of the decompressed save
   bytes behind the original stream calls 0x0046b800 / 0x0046ba40. A short
   read or a full buffer sets `failed`; later calls then do nothing. */
static inline void aoe_stream_put(AoeOutcomeStream *s, const void *bytes,
                                  size_t n) {
    if (s->failed || s->pos > s->size || s->size - s->pos < n) {
        s->failed = 1;
        return;
    }
    if (n) memcpy(s->data + s->pos, bytes, n);
    s->pos += n;
}

static inline void aoe_stream_put_u8(AoeOutcomeStream *s, uint8_t v) {
    aoe_stream_put(s, &v, 1);
}

static inline void aoe_stream_put_i32(AoeOutcomeStream *s, int32_t v) {
    uint8_t b[4];
    aoe_le_put_u32(b, (uint32_t)v);
    aoe_stream_put(s, b, 4);
}

static inline void aoe_stream_put_f32(AoeOutcomeStream *s, float v) {
    uint8_t b[4];
    aoe_le_put_f32(b, v);
    aoe_stream_put(s, b, 4);
}

static inline const uint8_t *aoe_stream_take(AoeOutcomeStream *s, size_t n) {
    const uint8_t *p;
    if (s->failed || s->pos > s->size || s->size - s->pos < n) {
        s->failed = 1;
        return NULL;
    }
    p = s->data + s->pos;
    s->pos += n;
    return p;
}

static inline uint8_t aoe_stream_u8(AoeOutcomeStream *s) {
    const uint8_t *p = aoe_stream_take(s, 1);
    return p ? p[0] : 0;
}

static inline int32_t aoe_stream_i32(AoeOutcomeStream *s) {
    const uint8_t *p = aoe_stream_take(s, 4);
    return p ? aoe_le_i32(p) : 0;
}

static inline float aoe_stream_f32(AoeOutcomeStream *s) {
    const uint8_t *p = aoe_stream_take(s, 4);
    return p ? aoe_le_f32(p) : 0.0f;
}

#endif
