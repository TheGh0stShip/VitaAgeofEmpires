/* Patched RoR 1.0a streamed WAV player 0x0043a6f0..0x0043af60. */
#include "wav_stream.h"

#include <assert.h>
#include <string.h>

typedef struct {
    uint8_t ring[0x10000];
    size_t size, position;
    int creates, plays, stops, releases, ends, progress, timers;
} Fake;

static int f_create(void *c, size_t size, const AoeWavFormatRor *fmt) {
    Fake *f = c;
    (void)fmt;
    f->size = size;
    ++f->creates;
    return 0;
}
static size_t f_position(void *c) { return ((Fake *)c)->position; }
static void f_write(void *c, size_t off, const uint8_t *d, size_t n) { memcpy(((Fake *)c)->ring + off, d, n); }
static void f_play(void *c, int32_t v) { (void)v; ++((Fake *)c)->plays; }
static void f_stop(void *c) { ++((Fake *)c)->stops; }
static void f_release(void *c) { ++((Fake *)c)->releases; }
static void f_post(void *c, uint32_t m) {
    Fake *f = c;
    if (m == 0x500) ++f->ends;
    else ++f->progress;
}
static int f_timer(void *c) { ++((Fake *)c)->timers; return 1; }
static void f_kill(void *c) { (void)c; }

static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put32(uint8_t *p, uint32_t v) { put16(p, (uint16_t)v); put16(p + 2, (uint16_t)(v >> 16)); }

/* A PCM file: 8-bit mono at 1000 Hz (segment 200 bytes, ring 1200). */
static size_t make_wav(uint8_t *out, size_t samples, uint16_t tag) {
    memcpy(out, "RIFF", 4);
    put32(out + 4, (uint32_t)(4 + 8 + 16 + 8 + samples));
    memcpy(out + 8, "WAVEfmt ", 8);
    put32(out + 16, 16);
    put16(out + 20, tag);
    put16(out + 22, 1);
    put32(out + 24, 1000);
    put32(out + 28, 1000);
    put16(out + 32, 1);
    put16(out + 34, 8);
    memcpy(out + 36, "data", 4);
    put32(out + 40, (uint32_t)samples);
    for (size_t i = 0; i < samples; ++i) out[44 + i] = (uint8_t)(i + 1);
    return 44 + samples;
}

int main(void) {
    static uint8_t wav[4096];
    static Fake fake;
    static AoeWavStreamRor s;
    AoeWavStreamOpsRor ops = {&fake, f_create, f_position, f_write, f_play, f_stop, f_release,
                              f_post, f_timer, f_kill};
    AoeWavFileRor file;
    size_t n = make_wav(wav, 1300, 1), got;
    uint8_t buf[8];

    assert(aoe_wav_open_ror(&file, NULL, 0) == 0xe100);
    assert(aoe_wav_open_ror(&file, (const uint8_t *)"RIFX0000WAVE", 12) == 0xe101);
    assert(aoe_wav_open_ror(&file, wav, n) == 0 && file.format.avg_bytes_per_sec == 1000);
    assert(aoe_wav_find_data_ror(&file) == 0 && file.left == 1300 && file.chunk_size == 1300);
    assert(aoe_wav_read_ror(&file, buf, 3, &got) == 0 && got == 3 && buf[2] == 3);

    aoe_wav_stream_init_ror(&s, &ops);
    assert(s.percent == 100 && s.enabled);
    /* A non-PCM file does not play. */
    make_wav(wav, 1300, 2);
    assert(!aoe_wav_stream_play_ror(&s, wav, n, 0, -100));
    make_wav(wav, 1300, 1);
    assert(aoe_wav_stream_play_ror(&s, wav, n, 0, -100));
    assert(s.segment == 200 && s.ring == 1200 && fake.size == 1200);
    assert(fake.ring[0] == 1 && fake.ring[1199] == (uint8_t)1200 && !s.ending);
    assert(fake.plays == 1 && fake.timers == 1 && fake.progress == 1);

    /* Still inside the cursor's segment's span: nothing to do. */
    fake.position = 100;
    aoe_wav_stream_tick_ror(&s);
    assert(fake.progress == 1);
    /* The first segment played: refilled from the file (100 bytes left,
       then silence) and the end marked. */
    fake.position = 250;
    aoe_wav_stream_tick_ror(&s);
    assert(s.played == 250 && fake.progress == 2 && s.cursor == 200);
    assert(fake.ring[0] == (uint8_t)1201 && fake.ring[99] == (uint8_t)1300 && fake.ring[100] == 0x80);
    /* Four segments of the ring are still ahead of the play position. */
    assert(s.ending && s.drain == 4);
    /* Draining writes silence down to two segments, then waits; once the
       count reaches zero the next tick ends it. */
    fake.position = 500;
    aoe_wav_stream_tick_ror(&s);
    assert(s.drain == 3 && s.cursor == 400 && fake.ring[200] == 0x80);
    fake.position = 700;
    aoe_wav_stream_tick_ror(&s);
    assert(s.drain == 2 && s.cursor == 600);
    fake.position = 900;
    aoe_wav_stream_tick_ror(&s);
    assert(s.drain == 1 && s.cursor == 600 && !fake.ends);
    fake.position = 1100;
    aoe_wav_stream_tick_ror(&s);
    assert(s.drain == 0 && !fake.ends);
    aoe_wav_stream_tick_ror(&s);
    assert(fake.ends == 1 && s.stopping);
    aoe_wav_stream_message_ror(&s, 0x500);
    assert(!s.open && fake.releases == 1 && fake.stops == 1);

    /* Looping: the second play fills with the previous call's flag (0),
       then loops from then on. */
    assert(aoe_wav_stream_play_ror(&s, wav, n, 1, 0));
    assert(s.loop == 1);
    fake.position = 300;
    aoe_wav_stream_tick_ror(&s);
    assert(fake.ring[99] == (uint8_t)1300 && fake.ring[100] == 1 && !s.ending);
    aoe_wav_stream_message_ror(&s, 0x502);
    assert(s.percent == 300 * 100 / 1300);
    aoe_wav_stream_shutdown_ror(&s);
    assert(!s.enabled && !s.open);
    return 0;
}
