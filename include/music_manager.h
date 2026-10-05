#ifndef VITA_AOE_MUSIC_MANAGER_H
#define VITA_AOE_MUSIC_MANAGER_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a music manager (0x0044fad0..0x00450cc0; the
   game's +0x64): music from the CD's audio tracks (source 1, MCI
   "cdaudio"), numbered MIDI files "<dir>xmusic%d.mid" (2, MCI sequencer)
   or numbered WAV files "<dir>music%d.wav" (3, the sound system). It
   plays a track, a range of tracks (looping when asked) or a named file,
   pauses and resumes, sets the volume (-10000..0, through the mixer for
   CD and MIDI) and fades out (at the fade length's pace) before the next
   request. MCI, the mixer, files and the clock are ops. Semantic
   storage; offsets noted. */

typedef struct {
    void *context;
    /* MCI: open the CD device (its track count in `tracks`, -1 when the
       status fails) or a sequencer for a file; play (from/to tracks for
       the CD, 0 for "to end"), stop, pause, resume, close, and the
       current position (the track for the CD). */
    int (*open_cd)(void *context, uint32_t *device, int32_t *tracks);
    int (*open_midi)(void *context, const char *path, uint32_t *device);
    int (*play)(void *context, uint32_t device, int32_t from, int32_t to, int has_to,
                int32_t seek, uint32_t notify);
    void (*stop)(void *context, uint32_t device);
    void (*pause)(void *context, uint32_t device);
    int (*resume)(void *context, uint32_t device, uint32_t notify);
    void (*close)(void *context, uint32_t device);
    int (*position)(void *context, uint32_t device, int32_t *position);
    /* The mixer line for the source (0 when there is none): its range and
       value, and setting the value. */
    int (*mixer_open)(void *context, int source, int32_t *min, int32_t *max, int32_t *value);
    void (*mixer_close)(void *context);
    void (*mixer_set)(void *context, int32_t value);
    /* Files: whether one opens; WAV playback through the sound system
       (0x004a2870 play, 0x004a28d0 volume, 0x004a28f0 pause, 0x004a2900
       resume, 0x004a2910 stop). */
    int (*exists)(void *context, const char *path);
    int (*wav_play)(void *context, const char *path, int32_t volume);
    void (*wav_volume)(void *context, int32_t volume);
    void (*wav_pause)(void *context);
    void (*wav_resume)(void *context);
    void (*wav_stop)(void *context);
    uint32_t (*now_ms)(void *context);
} AoeMusicOpsRor;

typedef struct {
    char dir[0x104];          /* +0x000 */
    uint8_t source;           /* +0x104: 1 CD, 2 MIDI, 3 WAV */
    void *sound;              /* +0x108: the sound system (WAV) */
    uint32_t window;          /* +0x10c */
    uint32_t notify;          /* +0x110 */
    int32_t open;             /* +0x114 */
    uint32_t device;          /* +0x118 */
    uint8_t playing;          /* +0x128 */
    char file[0x107];         /* +0x129 */
    int32_t from, to;         /* +0x230, +0x234 */
    int32_t start_track;      /* +0x238 */
    int32_t seek;             /* +0x23c */
    int32_t loop;             /* +0x240 */
    int32_t track;            /* +0x244 */
    int32_t tracks;           /* +0x248 */
    int32_t plays;            /* +0x24c */
    int32_t mixer_volume;     /* +0x250 */
    int32_t volume;           /* +0x254 */
    int32_t paused;           /* +0x258 */
    int32_t pause_again;      /* +0x25c */
    int32_t field_260;        /* +0x260 */
    int32_t fade_on;          /* +0x264 */
    int32_t fade_ms_len;      /* +0x268, 2000 */
    int32_t fading;           /* +0x26c */
    int32_t fade_volume;      /* +0x270 */
    uint32_t fade_ms;         /* +0x274 */
    int32_t mixer;            /* +0x278 */
    int32_t mixer_min, mixer_max, mixer_value; /* +0x3a4, +0x3a8, +0x3ec */
    const AoeMusicOpsRor *ops;
} AoeMusicRor;

/* 0x0044fad0: a manager for a source (window, notify window, sound
   system, directory) opened at once. 0x0044fe00 (also 0x0044fbe0) closes
   it: playing stops, the mixer's volume restored to the saved one (CD,
   MIDI), the CD device closed. */
void aoe_music_init_ror(AoeMusicRor *m, const AoeMusicOpsRor *ops, uint8_t source,
                        uint32_t window, uint32_t notify, void *sound, const char *dir);
void aoe_music_close_ror(AoeMusicRor *m);
/* 0x0044fbf0: open the source: the CD device (its tracks) or the count of
   numbered files present; the mixer's volume (CD, MIDI) becomes the
   saved and current volume (value x 10000 / range - 10000). */
int aoe_music_open_ror(AoeMusicRor *m);
/* 0x0044fec0 / 0x00450050: the mixer line opened / closed. */
int aoe_music_mixer_open_ror(AoeMusicRor *m);
void aoe_music_mixer_close_ror(AoeMusicRor *m);
/* 0x00450090: switch source (closing and reopening; the same source
   returns whether it is open). */
int aoe_music_set_source_ror(AoeMusicRor *m, uint8_t source);
/* 0x004500d0: the volume (clamped to -10000..0; kept when `keep`). */
void aoe_music_set_volume_ror(AoeMusicRor *m, int32_t volume, int keep);
/* 0x00450180 a track (loop), 0x00450200 a named file (not for the CD),
   0x004502a0 a range of tracks (clamped to the count): each fades the
   current music out first when a fade is on and something plays. */
int aoe_music_play_track_ror(AoeMusicRor *m, int32_t track, int32_t loop, int32_t seek);
int aoe_music_play_file_ror(AoeMusicRor *m, const char *name, int32_t loop, int32_t seek);
int aoe_music_play_range_ror(AoeMusicRor *m, int32_t from, int32_t to, int32_t loop,
                             int32_t start_track, int32_t seek);
/* 0x00450360: start playing (tracks from..to, or a named file); 0 when
   nothing starts. */
int aoe_music_start_ror(AoeMusicRor *m, int32_t from, int32_t to, const char *file,
                        int32_t start_track, int32_t seek);
/* 0x004506f0 pause (a second pause cancels the pending one), 0x004507a0
   resume (or finish a fade), 0x004508a0 stop, 0x00450870 stop and forget
   the request. */
int aoe_music_pause_ror(AoeMusicRor *m);
int aoe_music_resume_ror(AoeMusicRor *m);
int aoe_music_stop_ror(AoeMusicRor *m);
void aoe_music_reset_ror(AoeMusicRor *m);
/* 0x00450950 begin a fade (from the current volume); 0x00450990 the fade
   tick: -10000 x elapsed / length each tick, stopping when silent;
   0x00450a80 the fade's end: the volume restored and the queued request
   started. */
void aoe_music_fade_begin_ror(AoeMusicRor *m);
void aoe_music_fade_tick_ror(AoeMusicRor *m);
void aoe_music_fade_end_ror(AoeMusicRor *m);
/* 0x00450c10: a track ended (the notify message): the next track of the
   range (MIDI, WAV), else the loop starts over. */
void aoe_music_track_done_ror(AoeMusicRor *m);
/* 0x00450cc0: what plays (source, from, to, track, file, loop and the
   device position). 0 when closed. */
int aoe_music_state_ror(const AoeMusicRor *m, uint8_t *source, int32_t *from, int32_t *to,
                        int32_t *track, char *file, int32_t *loop, int32_t *position);

#endif
