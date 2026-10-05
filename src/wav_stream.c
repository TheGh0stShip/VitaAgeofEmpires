/* Patched RoR 1.0a streamed WAV player (0x0043a6f0..0x0043af60). */
#include "wav_stream.h"

#include <string.h>

#include "le_bytes.h"


static int tag_at(const uint8_t *p, const char *tag) { return memcmp(p, tag, 4) == 0; }

/* A sub-chunk of [from, end) with this tag: its data offset and size. */
static int find_chunk(const AoeWavFileRor *f, size_t from, size_t end, const char *tag,
                      size_t *at, size_t *size) {
    while (from + 8 <= end) {
        uint32_t n = aoe_le_u32(f->data + from + 4);
        if (tag_at(f->data + from, tag)) {
            *at = from + 8;
            *size = n;
            return 1;
        }
        from += 8 + (size_t)n + (n & 1);
    }
    return 0;
}

uint32_t aoe_wav_open_ror(AoeWavFileRor *f, const uint8_t *data, size_t size) {
    size_t at, n, end;
    const uint8_t *p;
    memset(f, 0, sizeof *f);
    if (!data) return 0xe100;
    f->data = data;
    f->size = size;
    if (size < 12 || !tag_at(data, "RIFF") || !tag_at(data + 8, "WAVE")) return 0xe101;
    f->riff_data = 8;
    end = 8 + (size_t)aoe_le_u32(data + 4);
    if (end > size) end = size;
    if (!find_chunk(f, 12, end, "fmt ", &at, &n) || n < 16) return 0xe101;
    if (at + 16 > size) return 0xe102;
    p = data + at;
    f->format.format_tag = aoe_le_u16(p);
    f->format.channels = aoe_le_u16(p + 2);
    f->format.samples_per_sec = aoe_le_u32(p + 4);
    f->format.avg_bytes_per_sec = aoe_le_u32(p + 8);
    f->format.block_align = aoe_le_u16(p + 12);
    f->format.bits_per_sample = aoe_le_u16(p + 14);
    if (f->format.format_tag != 1) {
        if (at + 18 > size) return 0xe102;
        f->format.extra_size = aoe_le_u16(p + 16);
        if (at + 18 + f->format.extra_size > size) return 0xe102;
    }
    return 0;
}

uint32_t aoe_wav_find_data_ror(AoeWavFileRor *f) {
    size_t at, n, end;
    if (!f->data) return 0x109;
    end = 8 + (size_t)aoe_le_u32(f->data + 4);
    if (end > f->size) end = f->size;
    if (!find_chunk(f, f->riff_data + 4, end, "data", &at, &n)) return 0x109;
    f->chunk_size = n;
    if (at + n > f->size) n = f->size - at;
    f->chunk = at;
    f->left = n;
    return 0;
}

uint32_t aoe_wav_read_ror(AoeWavFileRor *f, uint8_t *out, size_t n, size_t *got) {
    if (n > f->left) n = f->left;
    if (n) memcpy(out, f->data + f->chunk, n);
    f->chunk += n;
    f->left -= n;
    *got = n;
    return 0;
}

static uint8_t silence_of(const AoeWavStreamRor *s) {
    return s->file.format.bits_per_sample == 8 ? 0x80 : 0;
}

/* Bytes of the ring between the play position and the cursor, in
   segments: what still has to play after the file ran out. */
static int32_t segments_ahead(const AoeWavStreamRor *s, size_t position) {
    size_t d = s->cursor < position ? s->ring - position + s->cursor : s->cursor - position;
    return s->segment ? (int32_t)(d / s->segment) : 0;
}

/* Fill `out` from the file: a short read loops back to the data, or is
   padded with silence (when `pad` allows) and marks the end. */
static void fill(AoeWavStreamRor *s, uint8_t *out, size_t n, int loop, int pad, size_t position,
                 int from_play) {
    size_t got = 0;
    aoe_wav_read_ror(&s->file, out, n, &got);
    if (got >= n) return;
    if (!loop) {
        if (pad) memset(out + got, silence_of(s), n - got);
        s->ending = 1;
        s->drain = from_play ? (s->segment ? (int32_t)(got / s->segment) : 0)
                             : segments_ahead(s, position);
        return;
    }
    while (got < n) {
        size_t more = 0;
        out += got;
        n -= got;
        aoe_wav_find_data_ror(&s->file);
        aoe_wav_read_ror(&s->file, out, n, &more);
        if (!more) break; /* host guard: an empty data chunk */
        got = more;
    }
}

void aoe_wav_stream_init_ror(AoeWavStreamRor *s, const AoeWavStreamOpsRor *ops) {
    memset(s, 0, sizeof *s);
    s->percent = 100;
    s->ops = ops;
    s->enabled = 1;
}

void aoe_wav_stream_shutdown_ror(AoeWavStreamRor *s) {
    if (!s->enabled) return;
    if (s->open || s->playing || s->timer) aoe_wav_stream_stop_ror(s);
    s->enabled = 0;
}

void aoe_wav_stream_stop_ror(AoeWavStreamRor *s) {
    const AoeWavStreamOpsRor *ops = s->ops;
    s->stopping = 1;
    if (s->timer) {
        s->timer = 0;
        if (ops && ops->stop_timer) ops->stop_timer(ops->context);
    }
    if (s->playing) {
        s->playing = 0;
        if (ops && ops->stop) ops->stop(ops->context);
    }
    if (s->open) {
        memset(&s->file, 0, sizeof s->file);
        if (ops && ops->release) ops->release(ops->context);
        s->open = 0;
    }
}

int aoe_wav_stream_play_ror(AoeWavStreamRor *s, const uint8_t *data, size_t size, int loop,
                            int32_t volume) {
    const AoeWavStreamOpsRor *ops = s->ops;
    if (s->open || s->playing || s->timer) aoe_wav_stream_stop_ror(s);
    s->paused = 0;
    if (aoe_wav_open_ror(&s->file, data, size)) return 0;
    if (s->file.format.format_tag != 1 || aoe_wav_find_data_ror(&s->file)) {
        memset(&s->file, 0, sizeof s->file);
        return 0;
    }
    s->segment = (size_t)s->file.format.avg_bytes_per_sec * 0x78 / 600;
    s->ring = s->segment * 6;
    /* Host bound: the ring is staged in the scratch buffer. */
    if (!s->ring || s->ring > sizeof s->scratch) return 0;
    if (!ops || !ops->create || ops->create(ops->context, s->ring, &s->file.format)) return 0;
    s->ending = 0;
    s->drain = 0;
    /* As in the original the first fill still sees the previous call's
       loop flag; this call's is stored afterwards. */
    fill(s, s->scratch, s->ring, s->loop, 1, 0, 1);
    if (ops->write) ops->write(ops->context, 0, s->scratch, s->ring);
    s->played = 0;
    s->last_position = 0;
    s->cursor = 0;
    s->stopping = 0;
    s->loop = loop & 0xff;
    s->open = 1;
    if (ops->post) ops->post(ops->context, 0x502);
    if (ops->play) ops->play(ops->context, volume);
    s->timer = ops->start_timer ? ops->start_timer(ops->context) != 0 : 0;
    s->playing = 1;
    return 1;
}

void aoe_wav_stream_message_ror(AoeWavStreamRor *s, uint32_t message) {
    if (message == 0x500) {
        aoe_wav_stream_stop_ror(s);
    } else if (message == 0x502 && s->file.chunk_size) {
        int32_t p;
        if (s->file.chunk_size < s->played) s->played -= s->file.chunk_size;
        p = (int32_t)((float)(s->played * 100) / (float)s->file.chunk_size);
        if (p != s->percent) s->percent = p;
    }
}

void aoe_wav_stream_tick_ror(AoeWavStreamRor *s) {
    const AoeWavStreamOpsRor *ops = s->ops;
    size_t position, n;
    uint8_t bits;
    if (s->busy || s->paused) return;
    s->busy = 1;
    if (s->stopping || !s->open || !ops) goto done;
    position = ops->position ? ops->position(ops->context) : 0;
    if (s->cursor < position && position < s->cursor + s->segment) goto done;
    if (s->ending && s->drain == 0) {
        if (!s->stopping) {
            s->stopping = 1;
            if (ops->post) ops->post(ops->context, 0x500);
        }
        goto done;
    }
    s->played += position < s->last_position ? position + s->ring - s->last_position
                                             : position - s->last_position;
    s->last_position = position;
    if (ops->post) ops->post(ops->context, 0x502);
    n = s->segment;
    bits = (uint8_t)s->file.format.bits_per_sample;
    if (!s->ending) {
        if (s->stopping) memset(s->scratch, silence_of(s), n);
        else fill(s, s->scratch, n, s->loop, bits == 8 || bits == 16, position, 0);
    } else {
        if (--s->drain < 2) goto done;
        /* Other sample sizes leave the locked bytes as they were. */
        memset(s->scratch, silence_of(s), n);
    }
    if (ops->write) ops->write(ops->context, s->cursor, s->scratch, n);
    s->cursor += n;
    if (s->cursor >= s->ring) s->cursor -= s->ring;
done:
    s->busy = 0;
}
