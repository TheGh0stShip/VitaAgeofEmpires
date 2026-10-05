#include "player_history.h"

#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

int aoe_history_init_ror(AoePlayerHistoryRor *h, AoeOutcomeStream *stream, float version) {
    memset(h, 0, sizeof *h);
    if (stream) return aoe_history_load_ror(h, stream, version);
    return 1;
}

void aoe_history_release_ror(AoePlayerHistoryRor *h) {
    free(h->bytes);
    h->bytes = NULL;
    while (h->events) {
        AoeHistoryEventRor *e = h->events;
        h->events = e->next;
        free(e);
    }
}

int aoe_history_load_ror(AoePlayerHistoryRor *h, AoeOutcomeStream *s, float version) {
    const uint8_t *p;
    int32_t n;
    AoeHistoryEventRor **tail = &h->events;
    h->count = aoe_stream_i32(s);
    h->field_8 = aoe_stream_i32(s);
    h->capacity = aoe_stream_i32(s);
    h->bytes = calloc((size_t)(h->capacity > 0 ? h->capacity : 1), 1);
    if (!h->bytes) return 0;
    p = aoe_stream_take(s, (size_t)(h->capacity > 0 ? h->capacity : 0));
    if (p && h->capacity > 0) memcpy(h->bytes, p, (size_t)h->capacity);
    n = aoe_stream_i32(s);
    for (; n > 0 && !s->failed; --n) {
        AoeHistoryEventRor *e = calloc(1, sizeof *e);
        if (!e) return 0;
        if (version > 7.2f) {
            e->kind = aoe_stream_u8(s);
            aoe_stream_take(s, 3);
            e->sample = aoe_stream_i32(s);
            e->time = aoe_stream_i32(s);
            aoe_stream_i32(s); /* the saved next pointer */
        } else {
            e->kind = aoe_stream_u8(s);
            aoe_stream_take(s, 3);
            e->sample = aoe_stream_i32(s);
            e->time = e->sample * 15000;
            aoe_stream_i32(s);
        }
        *tail = e;
        tail = &e->next;
    }
    return !s->failed;
}

void aoe_history_save_ror(const AoePlayerHistoryRor *h, AoeOutcomeStream *s) {
    const AoeHistoryEventRor *e;
    int32_t n = 0;
    static const uint8_t pad[3] = {0, 0, 0};
    aoe_stream_put_i32(s, h->count);
    aoe_stream_put_i32(s, h->field_8);
    aoe_stream_put_i32(s, h->capacity);
    aoe_stream_put(s, h->bytes, (size_t)(h->capacity > 0 ? h->capacity : 0));
    for (e = h->events; e; e = e->next) ++n;
    aoe_stream_put_i32(s, n);
    for (e = h->events; e; e = e->next) {
        aoe_stream_put_u8(s, e->kind);
        aoe_stream_put(s, pad, 3);
        aoe_stream_put_i32(s, e->sample);
        aoe_stream_put_i32(s, e->time);
        /* The original writes its next pointer; only presence was ever
           meaningful, so 1 / 0 is written. */
        aoe_stream_put_i32(s, e->next ? 1 : 0);
    }
}

void aoe_history_add_ror(AoePlayerHistoryRor *h, int skip, uint8_t v) {
    if (h->capacity <= h->count) {
        int32_t old = h->capacity;
        uint8_t *grown = calloc((size_t)old + 0x3c, 1);
        if (!grown) return;
        h->capacity = old + 0x3c;
        if (old > 0) {
            memcpy(grown, h->bytes, (size_t)old);
            free(h->bytes);
        }
        h->bytes = grown;
    }
    if (!skip) h->bytes[h->count] = v;
    ++h->count;
}

uint8_t aoe_history_get_ror(const AoePlayerHistoryRor *h, int skip, int32_t i) {
    if (i < h->count && i >= 0 && !skip) return h->bytes[i];
    return 0;
}

void aoe_history_event_ror(AoePlayerHistoryRor *h, uint8_t kind, int32_t time) {
    AoeHistoryEventRor *e;
    if (h->count <= 0) return;
    e = calloc(1, sizeof *e);
    if (!e) return;
    e->next = h->events;
    e->sample = h->count;
    e->time = time;
    e->kind = kind;
    h->events = e;
}

int aoe_history_find_ror(const AoePlayerHistoryRor *h, int32_t sample, uint8_t *kind) {
    const AoeHistoryEventRor *e;
    for (e = h->events; e; e = e->next) {
        if (e->sample == sample) {
            *kind = e->kind;
            return 1;
        }
    }
    return 0;
}
