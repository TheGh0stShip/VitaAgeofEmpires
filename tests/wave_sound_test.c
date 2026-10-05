#include "wave_sound.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #cond);     \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

static char trace[1024];
static int dev, prim, buf, mixer_handle;
static int caps16, level2_fails, lose_next, mixer_present;
static uint32_t buffer_status;
static int32_t last_volume, last_freq, last_pan, mixer_last;
static AoeWaveFormatRor last_format;
static uint32_t filled;
static uint8_t wav_file[0x2c + 4];
static int file_present, resource_present;

static void note(const char *s) {
    strncat(trace, s, sizeof trace - strlen(trace) - 1);
    strncat(trace, " ", sizeof trace - strlen(trace) - 1);
}

static int32_t create_device(void *c, void **d) {
    (void)c;
    note("dev");
    *d = &dev;
    return 0;
}
static int primary_16bit(void *c, void *d) {
    (void)c;
    (void)d;
    return caps16;
}
static int32_t set_level(void *c, void *d, void *w, int32_t level) {
    (void)c;
    (void)d;
    (void)w;
    note(level == 2 ? "prio" : "normal");
    return level == 2 && level2_fails ? 1 : 0;
}
static int32_t create_primary(void *c, void *d, void **p) {
    (void)c;
    (void)d;
    *p = &prim;
    return 0;
}
static int32_t set_format(void *c, void *b, const AoeWaveFormatRor *f) {
    (void)c;
    (void)b;
    last_format = *f;
    note("format");
    return 0;
}
static void release(void *c, void *o) {
    (void)c;
    note(o == &dev ? "rel-dev" : o == &prim ? "rel-prim" : "rel-buf");
}
static int32_t create_buffer(void *c, void *d, const AoeWaveFormatRor *f,
                             uint32_t size, void **b) {
    (void)c;
    (void)d;
    last_format = *f;
    filled = size;
    *b = &buf;
    note("buffer");
    return 0;
}
static int32_t fill(void *c, void *b, const uint8_t *data, uint32_t size) {
    (void)c;
    (void)b;
    CHECK(size == 4 && memcmp(data, "PCM!", 4) == 0);
    return 0;
}
static int32_t status(void *c, void *b, uint32_t *s) {
    (void)c;
    (void)b;
    *s = buffer_status;
    return 0;
}
static void set_volume(void *c, void *b, int32_t v) {
    (void)c;
    (void)b;
    last_volume = v;
}
static void set_frequency(void *c, void *b, int32_t v) {
    (void)c;
    (void)b;
    last_freq = v;
}
static void set_pan(void *c, void *b, int32_t v) {
    (void)c;
    (void)b;
    last_pan = v;
}
static void rewind_buf(void *c, void *b) {
    (void)c;
    (void)b;
}
static int32_t play(void *c, void *b, int looping) {
    (void)c;
    note(b == &prim ? "play-prim" : looping ? "play-loop" : "play");
    if (b == &buf && lose_next) {
        lose_next = 0;
        return AOE_AUDIO_BUFFER_LOST;
    }
    return 0;
}
static void stop(void *c, void *b) {
    (void)c;
    note(b == &prim ? "stop-prim" : "stop");
}
static void *mixer_open(void *c, int32_t *mn, int32_t *mx, uint32_t *v) {
    (void)c;
    if (!mixer_present) return NULL;
    *mn = 0;
    *mx = 65535;
    *v = 32768;
    return &mixer_handle;
}
static int32_t mixer_set(void *c, void *m, uint32_t v) {
    (void)c;
    (void)m;
    mixer_last = (int32_t)v;
    return 0;
}
static void mixer_close(void *c, void *m) {
    (void)c;
    (void)m;
    note("mixer-close");
}
static uint8_t *read_file(void *c, const char *path, uint32_t *size) {
    uint8_t *d;
    (void)c;
    if (!file_present || strcmp(path, "snd/boom.wav") != 0) return NULL;
    d = malloc(sizeof wav_file);
    memcpy(d, wav_file, sizeof wav_file);
    *size = sizeof wav_file;
    return d;
}
static const uint8_t *resource(void *c, int32_t id, uint32_t *size,
                               int *owned) {
    (void)c;
    if (!resource_present || id != 5) return NULL;
    note("resource");
    *size = sizeof wav_file;
    *owned = 0;
    return wav_file;
}
static void music_attach(void *c, void *w, void *d, void *p) {
    (void)c;
    (void)w;
    (void)d;
    (void)p;
    note("music-attach");
}
static void music_detach(void *c) {
    (void)c;
    note("music-detach");
}
static char music_path[64];
static int32_t music_play(void *c, const char *path, int32_t a, int32_t b) {
    (void)c;
    snprintf(music_path, sizeof music_path, "%s", path);
    return a + b;
}
static int32_t music_pause(void *c) {
    (void)c;
    return 7;
}
static int32_t music_resume(void *c) {
    (void)c;
    return 1;
}
static int32_t music_stop(void *c) {
    (void)c;
    return 1;
}

static AoeAudioOpsRor ops = {
    NULL,         0,          create_device, primary_16bit, set_level,
    create_primary, set_format, release,    create_buffer, fill,
    status,       set_volume, set_frequency, set_pan,       rewind_buf,
    play,         stop,       mixer_open,    mixer_set,     mixer_close,
    read_file,    resource,   music_attach,  music_detach,  music_play,
    music_pause,  music_resume, music_stop,
};

static void make_wav(void) {
    memset(wav_file, 0, sizeof wav_file);
    memcpy(wav_file, "RIFF", 4);
    wav_file[0x14] = 1;    /* PCM */
    wav_file[0x16] = 1;    /* mono */
    wav_file[0x18] = 0x22; /* 22050 */
    wav_file[0x19] = 0x56;
    wav_file[0x22] = 8;
    wav_file[0x28] = 4; /* data size */
    memcpy(wav_file + 0x2c, "PCM!", 4);
}

static void test_system(void) {
    AoeSoundSystemRor s;
    trace[0] = 0;
    caps16 = 1;
    level2_fails = 1;
    mixer_present = 1;
    CHECK(aoe_sound_create_ror(&s, &ops, NULL, "snd/"));
    /* Priority refused: normal level, no primary format. */
    CHECK(strcmp(trace, "dev prio normal play-prim music-attach ") == 0);
    CHECK(aoe_sound_ready_ror(&s) && s.mixer_ok);
    CHECK(s.saved_volume == 32768 * 10000 / 65535 - 10000);
    aoe_sound_set_volume_ror(&s, -5000, 1);
    CHECK(s.volume == -5000 && mixer_last == 65535 * 5000 / 10000);
    CHECK(aoe_sound_music_play_ror(&s, "a.mid", 2, 3) == 5);
    CHECK(strcmp(music_path, "snd/a.mid") == 0);
    CHECK(aoe_sound_music_pause_ror(&s) == 7);

    trace[0] = 0;
    aoe_sound_close_ror(&s);
    CHECK(mixer_last == 32767); /* the saved -5000 restored */
    CHECK(strcmp(trace,
                 "mixer-close music-detach stop-prim rel-prim rel-dev ") == 0);
    CHECK(!s.ready && s.saved_volume == 1 && !s.mixer_ok);
    CHECK(aoe_sound_music_pause_ror(&s) == 0);

    trace[0] = 0;
    level2_fails = 0;
    mixer_present = 0;
    CHECK(aoe_sound_create_ror(&s, &ops, NULL, NULL));
    CHECK(strcmp(trace, "dev prio format play-prim music-attach ") == 0);
    CHECK(last_format.rate == 22050 && last_format.bits == 16 &&
          last_format.channels == 1 && last_format.block_align == 2);
    CHECK(!s.mixer_ok && s.dir[0] == 0);
    aoe_sound_close_thunk_ror(&s);
}

static void test_waves(void) {
    AoeSoundSystemRor s;
    AoeWaveRor w, shared;
    int i;

    make_wav();
    caps16 = 0;
    aoe_sound_create_ror(&s, &ops, NULL, "snd/");

    /* A missing file falls back to the resource. */
    aoe_wave_init_ror(&w, &s, "boom.wav", 5);
    file_present = 0;
    resource_present = 1;
    ops.prefer_files = 1;
    trace[0] = 0;
    CHECK(aoe_wave_load_ror(&w, NULL, -1));
    CHECK(strcmp(trace, "resource buffer ") == 0);
    CHECK(w.loaded && filled == 4 && last_format.rate == 22050);
    CHECK(w.data != NULL); /* kept until unload on this path */
    aoe_wave_unload_ror(&w);
    CHECK(!w.loaded && w.data == NULL);

    /* The loose file, with its header checked. */
    file_present = 1;
    CHECK(aoe_wave_load_ror(&w, NULL, -1) && w.data == NULL);
    aoe_wave_unload_ror(&w);
    wav_file[0x14] = 2;
    CHECK(!aoe_wave_load_ror(&w, NULL, -1) && w.failed);
    CHECK(!aoe_wave_load_ror(&w, NULL, -1)); /* stays failed */
    aoe_wave_unload_ror(&w);                  /* clears the failure */
    CHECK(!w.failed);
    wav_file[0x14] = 1;

    /* Playing loads on first use and applies the player's settings. */
    aoe_wave_init_ror(&shared, &s, "boom.wav", 5);
    shared.parent = &w;
    shared.volume = -100;
    shared.frequency = 11025;
    shared.pan = 30;
    trace[0] = 0;
    CHECK(aoe_wave_play_ror(&shared));
    CHECK(w.loaded && strcmp(trace, "buffer play ") == 0);
    CHECK(last_volume == -100 && last_freq == 11025 && last_pan == 30);

    /* A lost buffer is reloaded and played again. */
    lose_next = 1;
    w.looping = 1;
    trace[0] = 0;
    CHECK(aoe_wave_play_ror(&w));
    CHECK(strcmp(trace, "play-loop stop rel-buf buffer play-loop ") == 0);

    buffer_status = 1;
    CHECK(aoe_wave_playing_ror(&shared));
    buffer_status = 5; /* playing and looping reads as not playing */
    CHECK(!aoe_wave_playing_ror(&w));

    /* The queue: up to 255, not while muted. */
    CHECK(aoe_wave_queue_ror(&w) && s.queued == 1);
    trace[0] = 0;
    aoe_sound_play_queue_ror(&s);
    CHECK(strcmp(trace, "play-loop ") == 0);
    aoe_sound_clear_queue_ror(&s);
    for (i = 0; i < 255; ++i) aoe_sound_queue_ror(&s, &w);
    CHECK(!aoe_sound_queue_ror(&s, &w) && s.queued == 255);
    aoe_sound_clear_queue_ror(&s);
    s.muted = 1;
    CHECK(!aoe_wave_queue_ror(&w) && aoe_wave_play_ror(&w));

    /* Freeing a sharing wave frees its parent. */
    s.muted = 0;
    trace[0] = 0;
    aoe_wave_free_thunk_ror(&shared);
    CHECK(strcmp(trace, "stop stop rel-buf ") == 0);
    CHECK(!w.loaded && w.system == NULL && shared.parent == NULL);
    aoe_sound_close_ror(&s);
}

int main(void) {
    test_system();
    test_waves();
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    puts("wave_sound_test: ok");
    return 0;
}
