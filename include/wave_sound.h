#ifndef VITA_AOE_WAVE_SOUND_H
#define VITA_AOE_WAVE_SOUND_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a sound system (0x004a2560..0x004a2b90) and
   wave objects (0x004a2be0..0x004a3490). The device, its buffers, the
   output mixer, files, the executable's "wav " resources and the
   streamed music player are AoeAudioOpsRor; results follow the original's
   convention (0 success). Semantic storage; offsets are noted. */

/* A PCM format as the wave header carries it (WAVEFORMATEX, cbSize 0). */
typedef struct {
    int16_t tag, channels;
    int32_t rate, bytes_per_second;
    int16_t block_align, bits;
} AoeWaveFormatRor;

/* The play call's result when the buffer's memory was lost. */
#define AOE_AUDIO_BUFFER_LOST ((int32_t)0x88780096u)

typedef struct {
    void *context;
    /* Nonzero: load waves from loose files before resources. */
    int prefer_files;
    /* The device: create it; does its primary buffer take 16-bit
       samples; set its cooperation level (1 normal, 2 priority); the
       primary buffer; release any device object. */
    int32_t (*create_device)(void *context, void **device);
    int (*primary_16bit)(void *context, void *device);
    int32_t (*set_level)(void *context, void *device, void *window,
                         int32_t level);
    int32_t (*create_primary)(void *context, void *device, void **primary);
    int32_t (*set_format)(void *context, void *buffer,
                          const AoeWaveFormatRor *format);
    void (*release)(void *context, void *object);
    /* Sound buffers: a static buffer with volume, pan and frequency
       control; filling it; its status (bit 0 playing, bit 2 looping);
       the per-play settings; play from the start (looping or once). */
    int32_t (*create_buffer)(void *context, void *device,
                             const AoeWaveFormatRor *format, uint32_t size,
                             void **buffer);
    int32_t (*fill)(void *context, void *buffer, const uint8_t *data,
                    uint32_t size);
    int32_t (*status)(void *context, void *buffer, uint32_t *status);
    void (*set_volume)(void *context, void *buffer, int32_t volume);
    void (*set_frequency)(void *context, void *buffer, int32_t frequency);
    void (*set_pan)(void *context, void *buffer, int32_t pan);
    void (*rewind)(void *context, void *buffer);
    int32_t (*play)(void *context, void *buffer, int looping);
    void (*stop)(void *context, void *buffer);
    /* The output mixer's wave volume: open the first mixer that has one
       (its range and value), set it, close it. */
    void *(*mixer_open)(void *context, int32_t *min, int32_t *max,
                        uint32_t *value);
    int32_t (*mixer_set)(void *context, void *mixer, uint32_t value);
    void (*mixer_close)(void *context, void *mixer);
    /* A whole file (heap, free()) or NULL; a "wav " resource (`owned`
       when the caller must free() it) or NULL. */
    uint8_t *(*read_file)(void *context, const char *path, uint32_t *size);
    const uint8_t *(*resource)(void *context, int32_t id, uint32_t *size,
                               int *owned);
    /* The streamed music player: attach it to the device, detach it
       (stopping any stream), play a file at a volume, pause, resume and
       stop. */
    void (*music_attach)(void *context, void *window, void *device,
                         void *primary);
    void (*music_detach)(void *context);
    int32_t (*music_play)(void *context, const char *path, int32_t a,
                          int32_t b);
    int32_t (*music_pause)(void *context);
    int32_t (*music_resume)(void *context);
    int32_t (*music_stop)(void *context);
} AoeAudioOpsRor;

struct AoeWaveRor;

typedef struct {
    uint8_t ready;             /* +0x000 */
    uint8_t muted;             /* +0x001: no queueing or playing */
    void *window;              /* +0x004 */
    void *device;              /* +0x008 */
    void *primary;             /* +0x00c */
    int32_t result;            /* +0x010 */
    int32_t saved_volume;      /* +0x014: 1 when none was read */
    int32_t volume;            /* +0x018 */
    char dir[0x104];           /* +0x01c */
    int32_t mixer_ok;          /* +0x120 */
    void *mixer;               /* +0x124 */
    int32_t mixer_min;         /* +0x24c */
    int32_t mixer_max;         /* +0x250 */
    uint32_t mixer_value;      /* +0x294 */
    struct AoeWaveRor *queue[0x100]; /* +0x298 */
    int32_t queued;            /* +0x698 */
    const AoeAudioOpsRor *ops;
} AoeSoundSystemRor;

/* 0x004a2590 clears a system; 0x004a2560 clears and opens it. 0x004a25d0:
   open the device on `window` (priority level when its primary buffer
   takes 16-bit samples, else or failing that normal), a playing primary
   buffer (22050 Hz 16-bit mono at priority level), attach the music
   player and read the mixer volume. 0x004a2770 (and its thunk
   0x004a2580): put the mixer volume back, detach the music, release the
   buffers and device. 0x004a27c0: the wave directory ("" for none).
   0x004a2800: ready. */
void aoe_sound_clear_ror(AoeSoundSystemRor *s, const AoeAudioOpsRor *ops);
int aoe_sound_create_ror(AoeSoundSystemRor *s, const AoeAudioOpsRor *ops,
                         void *window, const char *dir);
int aoe_sound_open_ror(AoeSoundSystemRor *s, void *window, const char *dir);
void aoe_sound_close_ror(AoeSoundSystemRor *s);
void aoe_sound_close_thunk_ror(AoeSoundSystemRor *s);
void aoe_sound_set_dir_ror(AoeSoundSystemRor *s, const char *dir);
int aoe_sound_ready_ror(const AoeSoundSystemRor *s);
/* 0x004a2810: volume -10000..0 onto the mixer's range (kept when
   `remember`). 0x004a29d0: open the mixer and take its volume as both
   the saved and current one. 0x004a2b90: restore the saved volume and
   close the mixer. */
void aoe_sound_set_volume_ror(AoeSoundSystemRor *s, int32_t volume,
                              int remember);
int aoe_sound_mixer_open_ror(AoeSoundSystemRor *s);
void aoe_sound_mixer_close_ror(AoeSoundSystemRor *s);
/* Music, when ready: 0x004a2870 plays "<dir><name>", 0x004a28f0 pauses,
   0x004a2900 resumes, 0x004a2910 stops (0 when not ready). */
int32_t aoe_sound_music_play_ror(AoeSoundSystemRor *s, const char *name,
                                 int32_t a, int32_t b);
int32_t aoe_sound_music_pause_ror(AoeSoundSystemRor *s);
int32_t aoe_sound_music_resume_ror(AoeSoundSystemRor *s);
int32_t aoe_sound_music_stop_ror(AoeSoundSystemRor *s);
/* 0x004a2950: queue a wave (at most 255, not while muted); 0x004a2990
   plays the queue; 0x004a29c0 empties it. */
int aoe_sound_queue_ror(AoeSoundSystemRor *s, struct AoeWaveRor *w);
void aoe_sound_play_queue_ror(AoeSoundSystemRor *s);
void aoe_sound_clear_queue_ror(AoeSoundSystemRor *s);

/* A wave (0x3c bytes). One that shares another's buffer (`parent`) plays
   that wave's buffer with its own settings. */
typedef struct AoeWaveRor {
    AoeSoundSystemRor *system; /* +0x00 */
    struct AoeWaveRor *parent; /* +0x04 */
    uint8_t loaded;            /* +0x08 */
    uint8_t failed;            /* +0x09 */
    char name[13];             /* +0x0a */
    int32_t resource;          /* +0x18 */
    int32_t volume;            /* +0x1c */
    int32_t frequency;         /* +0x20 */
    int32_t pan;               /* +0x24 */
    int32_t looping;           /* +0x28 */
    uint8_t *data;             /* +0x2c */
    AoeWaveFormatRor *format;  /* +0x30 */
    void *buffer;              /* +0x34 */
    uint32_t size;             /* +0x38 */
} AoeWaveRor;

/* 0x004a2c30 clears a wave; 0x004a2be0 one on `system` with a file name
   (12 characters) and resource id. 0x004a2c60 (and its thunk
   0x004a2c20): a sharing wave frees its parent and clears itself; a
   loaded one stops, unloads and clears. */
void aoe_wave_clear_ror(AoeWaveRor *w);
void aoe_wave_init_ror(AoeWaveRor *w, AoeSoundSystemRor *system,
                       const char *name, int32_t resource);
void aoe_wave_free_ror(AoeWaveRor *w);
void aoe_wave_free_thunk_ror(AoeWaveRor *w);
/* 0x004a2ca0: load into a buffer (a sharing wave loads its parent;
   nothing after a failure): a new name replaces the old; a negative
   `resource` means the wave's own. From "<dir><name>" when loose files
   are preferred or there is no resource (a 44-byte "RIFF" header of PCM
   format, then the data), else from the resource (header then data).
   Failures mark the wave failed. */
int aoe_wave_load_ror(AoeWaveRor *w, const char *name, int32_t resource);
/* 0x004a3280 unload and 0x004a3410 stop the last wave of the parent
   chain; 0x004a32f0 play (loading the buffer's owner, one parent up, on
   first use; a lost buffer is reloaded and played again; 1 also when
   the system cannot play); 0x004a3440: is it playing (status exactly
   playing or looping); 0x004a3490: queue it on its system. */
void aoe_wave_unload_ror(AoeWaveRor *w);
void aoe_wave_stop_ror(AoeWaveRor *w);
int aoe_wave_play_ror(AoeWaveRor *w);
int aoe_wave_playing_ror(AoeWaveRor *w);
int aoe_wave_queue_ror(AoeWaveRor *w);

#endif
