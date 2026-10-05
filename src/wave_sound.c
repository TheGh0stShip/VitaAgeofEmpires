#include "wave_sound.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "le_bytes.h"

#define WAV_HEADER 0x2c

/* ---- sound system ---- */

void aoe_sound_clear_ror(AoeSoundSystemRor *s, const AoeAudioOpsRor *ops) {
    s->saved_volume = 1;
    s->ready = 0;
    s->window = NULL;
    s->device = NULL;
    s->primary = NULL;
    s->result = 0;
    s->muted = 0;
    s->queued = 0;
    s->volume = 0;
    s->mixer_ok = 0;
    s->mixer = NULL;
    s->dir[0] = '\0';
    s->ops = ops;
}

int aoe_sound_create_ror(AoeSoundSystemRor *s, const AoeAudioOpsRor *ops,
                         void *window, const char *dir) {
    aoe_sound_clear_ror(s, ops);
    return aoe_sound_open_ror(s, window, dir);
}

void aoe_sound_set_dir_ror(AoeSoundSystemRor *s, const char *dir) {
    size_t n;
    if (!dir) {
        s->dir[0] = '\0';
        return;
    }
    n = strlen(dir);
    if (n >= sizeof s->dir) n = sizeof s->dir - 1;
    memcpy(s->dir, dir, n);
    s->dir[n] = '\0';
}

int aoe_sound_open_ror(AoeSoundSystemRor *s, void *window, const char *dir) {
    const AoeAudioOpsRor *ops = s->ops;
    static const AoeWaveFormatRor primary = {1, 1, 22050, 44100, 2, 16};
    int priority = 0;

    aoe_sound_set_dir_ror(s, dir);
    s->window = window;
    s->ready = 0;
    s->device = NULL;
    s->result = ops->create_device(ops->context, &s->device);
    if (s->result != 0) return 0;
    if (ops->primary_16bit(ops->context, s->device)) {
        s->result = ops->set_level(ops->context, s->device, window, 2);
        if (s->result == 0) priority = 1;
    }
    if (!priority)
        s->result = ops->set_level(ops->context, s->device, window, 1);
    if (s->result != 0) {
        ops->release(ops->context, s->device);
        s->device = NULL;
        return 0;
    }
    s->result = ops->create_primary(ops->context, s->device, &s->primary);
    if (s->result == 0) {
        if (priority)
            s->result = ops->set_format(ops->context, s->primary, &primary);
        s->result = ops->play(ops->context, s->primary, 1);
        if (s->result == 0) {
            s->ready = 1;
            if (ops->music_attach)
                ops->music_attach(ops->context, window, s->device,
                                  s->primary);
            aoe_sound_mixer_open_ror(s);
            return s->ready;
        }
        /* The original keeps the primary buffer here; the port drops it. */
        ops->release(ops->context, s->primary);
        s->primary = NULL;
    }
    ops->release(ops->context, s->device);
    s->device = NULL;
    return 0;
}

void aoe_sound_close_ror(AoeSoundSystemRor *s) {
    const AoeAudioOpsRor *ops = s->ops;
    aoe_sound_mixer_close_ror(s);
    if (!s->ready) return;
    if (ops->music_detach) ops->music_detach(ops->context);
    if (s->primary) {
        ops->stop(ops->context, s->primary);
        ops->release(ops->context, s->primary);
        s->primary = NULL;
    }
    if (s->device) {
        ops->release(ops->context, s->device);
        s->device = NULL;
    }
    s->ready = 0;
}

void aoe_sound_close_thunk_ror(AoeSoundSystemRor *s) {
    aoe_sound_close_ror(s);
}

int aoe_sound_ready_ror(const AoeSoundSystemRor *s) { return s->ready; }

void aoe_sound_set_volume_ror(AoeSoundSystemRor *s, int32_t volume,
                              int remember) {
    if (remember) s->volume = volume;
    if (!s->mixer_ok) return;
    s->mixer_value =
        (uint32_t)((s->mixer_max - s->mixer_min) * (volume + 10000)) / 10000u;
    s->result = s->ops->mixer_set(s->ops->context, s->mixer, s->mixer_value);
}

int aoe_sound_mixer_open_ror(AoeSoundSystemRor *s) {
    const AoeAudioOpsRor *ops = s->ops;
    uint32_t range;
    if (!ops->mixer_open) return 0;
    s->mixer = ops->mixer_open(ops->context, &s->mixer_min, &s->mixer_max,
                               &s->mixer_value);
    if (!s->mixer) return 0;
    s->mixer_ok = 1;
    range = (uint32_t)(s->mixer_max - s->mixer_min);
    /* Port guard: an empty range reads as silence (the original
       divides by it). */
    s->volume = range ? (int32_t)(s->mixer_value * 10000u / range) - 10000
                      : -10000;
    s->saved_volume = s->volume;
    return 1;
}

void aoe_sound_mixer_close_ror(AoeSoundSystemRor *s) {
    if (!s->mixer_ok) return;
    if (s->saved_volume != 1) {
        aoe_sound_set_volume_ror(s, s->saved_volume, 0);
        s->saved_volume = 1;
    }
    if (s->mixer) {
        s->ops->mixer_close(s->ops->context, s->mixer);
        s->mixer = NULL;
    }
    s->mixer_ok = 0;
}

int32_t aoe_sound_music_play_ror(AoeSoundSystemRor *s, const char *name,
                                 int32_t a, int32_t b) {
    char path[0x104];
    if (!s->ready) return 0;
    snprintf(path, sizeof path, "%s%s", s->dir, name);
    return s->ops->music_play(s->ops->context, path, a, b);
}

int32_t aoe_sound_music_pause_ror(AoeSoundSystemRor *s) {
    return s->ready ? s->ops->music_pause(s->ops->context) : 0;
}

int32_t aoe_sound_music_resume_ror(AoeSoundSystemRor *s) {
    return s->ready ? s->ops->music_resume(s->ops->context) : 0;
}

int32_t aoe_sound_music_stop_ror(AoeSoundSystemRor *s) {
    return s->ready ? s->ops->music_stop(s->ops->context) : 0;
}

int aoe_sound_queue_ror(AoeSoundSystemRor *s, AoeWaveRor *w) {
    if (s->queued == 0xff || s->muted) return 0;
    s->queue[s->queued++] = w;
    return 1;
}

void aoe_sound_play_queue_ror(AoeSoundSystemRor *s) {
    int32_t i;
    for (i = 0; i < s->queued; ++i) aoe_wave_play_ror(s->queue[i]);
}

void aoe_sound_clear_queue_ror(AoeSoundSystemRor *s) { s->queued = 0; }

/* ---- waves ---- */

void aoe_wave_clear_ror(AoeWaveRor *w) {
    w->system = NULL;
    w->parent = NULL;
    w->loaded = 0;
    w->failed = 0;
    w->name[0] = '\0';
    w->data = NULL;
    w->format = NULL;
    w->buffer = NULL;
    w->size = 0;
    w->volume = 0;
    w->frequency = 0;
    w->pan = 0;
    w->looping = 0;
}

static void set_name(AoeWaveRor *w, const char *name) {
    strncpy(w->name, name, 12);
    w->name[12] = '\0';
}

void aoe_wave_init_ror(AoeWaveRor *w, AoeSoundSystemRor *system,
                       const char *name, int32_t resource) {
    aoe_wave_clear_ror(w);
    w->system = system;
    w->resource = resource;
    if (name) set_name(w, name);
}

void aoe_wave_free_ror(AoeWaveRor *w) {
    if (w->parent) {
        aoe_wave_free_ror(w->parent);
        aoe_wave_clear_ror(w);
        return;
    }
    if (w->loaded) {
        aoe_wave_stop_ror(w);
        aoe_wave_unload_ror(w);
        aoe_wave_clear_ror(w);
    }
}

void aoe_wave_free_thunk_ror(AoeWaveRor *w) { aoe_wave_free_ror(w); }

static AoeWaveRor *chain_end(AoeWaveRor *w) {
    while (w->parent) w = w->parent;
    return w;
}

static void read_format(AoeWaveFormatRor *f, const uint8_t *header) {
    f->tag = (int16_t)(header[0x14] | header[0x15] << 8);
    f->channels = (int16_t)(header[0x16] | header[0x17] << 8);
    f->rate = aoe_le_i32(header + 0x18);
    f->bytes_per_second = aoe_le_i32(header + 0x1c);
    f->block_align = (int16_t)(header[0x20] | header[0x21] << 8);
    f->bits = (int16_t)(header[0x22] | header[0x23] << 8);
}

static void drop_buffer(AoeWaveRor *w) {
    const AoeAudioOpsRor *ops = w->system->ops;
    ops->release(ops->context, w->buffer);
    w->buffer = NULL;
}

/* Format, buffer and fill from w->data / w->size; `keep_data` leaves the
   samples allocated (the resource path keeps them until unload). */
static int upload(AoeWaveRor *w, const uint8_t *header, int keep_data) {
    const AoeAudioOpsRor *ops = w->system->ops;
    w->format = malloc(sizeof *w->format);
    if (!w->format) {
        w->failed = 1;
        free(w->data);
        w->data = NULL;
        return 0;
    }
    read_format(w->format, header);
    if (ops->create_buffer(ops->context, w->system->device, w->format,
                           w->size, &w->buffer) != 0) {
        w->failed = 1;
        w->buffer = NULL;
        free(w->format);
        w->format = NULL;
        /* The original keeps the samples here; the port frees them. */
        free(w->data);
        w->data = NULL;
        return 0;
    }
    if (ops->fill(ops->context, w->buffer, w->data, w->size) != 0) {
        w->failed = 1;
        drop_buffer(w);
        free(w->format);
        w->format = NULL;
        free(w->data);
        w->data = NULL;
        return 0;
    }
    free(w->format);
    w->format = NULL;
    if (!keep_data) {
        free(w->data);
        w->data = NULL;
    }
    w->loaded = 1;
    return 1;
}

/* The loose file; -1 when it cannot be opened (the resource is tried). */
static int load_file(AoeWaveRor *w) {
    const AoeAudioOpsRor *ops = w->system->ops;
    char path[0x104 + 16];
    uint8_t header[WAV_HEADER], *file;
    uint32_t size = 0, have;

    snprintf(path, sizeof path, "%s%s", w->system->dir, w->name);
    file = ops->read_file ? ops->read_file(ops->context, path, &size) : NULL;
    if (!file) return -1;
    /* Port: a header shorter than 44 bytes fails like an empty read. */
    if (size < WAV_HEADER || memcmp(file, "RIFF", 4) != 0 ||
        file[0x14] != 1 || file[0x15] != 0) {
        w->failed = 1;
        free(file);
        return 0;
    }
    w->size = aoe_le_u32(file + 0x28);
    have = size - WAV_HEADER;
    if (have == 0) {
        w->failed = 1;
        free(file);
        return 0;
    }
    w->data = calloc(1, w->size ? w->size : 1);
    if (!w->data) {
        w->failed = 1;
        free(file);
        return 0;
    }
    memcpy(w->data, file + WAV_HEADER, have < w->size ? have : w->size);
    memcpy(header, file, WAV_HEADER);
    free(file);
    return upload(w, header, 0);
}

static int load_resource(AoeWaveRor *w, int32_t resource) {
    const AoeAudioOpsRor *ops = w->system->ops;
    const uint8_t *res = NULL;
    uint8_t header[WAV_HEADER];
    uint32_t size = 0;
    int owned = 0;

    if (resource >= 0 && ops->resource)
        res = ops->resource(ops->context, resource, &size, &owned);
    if (!res || size < WAV_HEADER) {
        if (res && owned) free((void *)res);
        w->failed = 1;
        return 0;
    }
    memcpy(header, res, WAV_HEADER);
    w->size = aoe_le_u32(header + 0x28);
    w->data = calloc(1, w->size ? w->size : 1);
    if (!w->data) {
        w->failed = 1;
        if (owned) free((void *)res);
        return 0;
    }
    /* Port guard: no more than the resource holds. */
    memcpy(w->data, res + WAV_HEADER,
           size - WAV_HEADER < w->size ? size - WAV_HEADER : w->size);
    if (owned) free((void *)res);
    return upload(w, header, 1);
}

int aoe_wave_load_ror(AoeWaveRor *w, const char *name, int32_t resource) {
    int r;
    if (w->failed) return 0;
    if (w->parent) return aoe_wave_load_ror(w->parent, name, -1);
    if (!w->system) return 0;
    if (w->loaded) aoe_wave_unload_ror(w);
    if (name) set_name(w, name);
    if (!w->name[0]) return 0;
    if (resource < 0) resource = w->resource;
    if (w->system->ops->prefer_files || resource < 0) {
        r = load_file(w);
        if (r >= 0) return r;
    }
    return load_resource(w, resource);
}

void aoe_wave_unload_ror(AoeWaveRor *w) {
    w = chain_end(w);
    if (w->loaded) {
        const AoeAudioOpsRor *ops = w->system->ops;
        aoe_wave_stop_ror(w);
        if (w->buffer) {
            ops->release(ops->context, w->buffer);
            w->buffer = NULL;
        }
        free(w->data);
        w->data = NULL;
        free(w->format);
        w->format = NULL;
        w->loaded = 0;
    }
    w->failed = 0;
}

void aoe_wave_stop_ror(AoeWaveRor *w) {
    w = chain_end(w);
    if (w->system && w->system->ready && w->loaded && w->buffer)
        w->system->ops->stop(w->system->ops->context, w->buffer);
}

static void start(const AoeAudioOpsRor *ops, const AoeWaveRor *w,
                  const AoeWaveRor *owner) {
    ops->set_volume(ops->context, owner->buffer, w->volume);
    ops->set_frequency(ops->context, owner->buffer, w->frequency);
    ops->set_pan(ops->context, owner->buffer, w->pan);
    ops->rewind(ops->context, owner->buffer);
}

int aoe_wave_play_ror(AoeWaveRor *w) {
    AoeWaveRor *owner = w->parent ? w->parent : w;
    const AoeAudioOpsRor *ops;
    uint32_t status;
    int looping;

    if (!owner->system || !owner->system->ready || owner->system->muted)
        return 1;
    ops = owner->system->ops;
    if (!owner->loaded && !aoe_wave_load_ror(owner, w->name, -1)) return 0;
    if (!owner->buffer) {
        owner->failed = 1;
        return 0;
    }
    ops->status(ops->context, owner->buffer, &status);
    looping = owner->looping != 0;
    start(ops, w, owner);
    if (ops->play(ops->context, owner->buffer, looping) ==
        AOE_AUDIO_BUFFER_LOST) {
        if (!aoe_wave_load_ror(owner, NULL, -1)) return 0;
        start(ops, w, owner);
        ops->play(ops->context, owner->buffer, looping);
    }
    return 1;
}

int aoe_wave_playing_ror(AoeWaveRor *w) {
    AoeWaveRor *owner = w->parent ? w->parent : w;
    const AoeAudioOpsRor *ops;
    uint32_t status = 0;
    if (!w->system || !owner->loaded || !owner->buffer) return 0;
    ops = w->system->ops;
    if (ops->status(ops->context, owner->buffer, &status) != 0) return 0;
    return status == 1 || status == 4;
}

int aoe_wave_queue_ror(AoeWaveRor *w) {
    if (!w->system || w->system->muted) return 0;
    return aoe_sound_queue_ror(w->system, w);
}
