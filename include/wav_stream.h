#ifndef VITA_AOE_WAV_STREAM_H
#define VITA_AOE_WAV_STREAM_H

#include <stddef.h>
#include <stdint.h>

/* Patched Rise of Rome 1.0a streamed WAV player (0x0043a6f0..0x0043af60):
   a RIFF/WAVE file read through mmio, played through a DirectSound buffer
   of six segments (a segment is a fifth of a second: average bytes per
   second x 120 / 600) that a 50 ms multimedia timer refills behind the
   play position. The original keeps it in globals (0x007d3b64..
   0x007d3bec, 0x00582ed0 / 0x00582ed4); here it is one object over a
   file in memory, with the sound buffer and the window's messages as
   ops. */

typedef struct {
    uint16_t format_tag, channels;     /* WAVEFORMATEX */
    uint32_t samples_per_sec, avg_bytes_per_sec;
    uint16_t block_align, bits_per_sample;
    uint16_t extra_size;               /* +0x10 (0 for PCM) */
} AoeWavFormatRor;

typedef struct {
    const uint8_t *data;   /* the whole file */
    size_t size;
    size_t riff_data;      /* offset of the RIFF chunk's data ("WAVE") */
    size_t chunk, left;    /* the read position in the data chunk and the bytes left */
    size_t chunk_size;     /* the data chunk's size (0x007d3ba0) */
    AoeWavFormatRor format;
} AoeWavFileRor;

/* 0x0043a6f0: open: a RIFF "WAVE" with a "fmt " chunk of at least 16
   bytes (the extra size and bytes read for a non-PCM tag). Returns 0, or
   0xe100 (no file), 0xe101 (not a WAVE / no format), 0xe102 (short).
   0x0043a8a0: the "data" chunk found from the start of the RIFF data
   (0x109 when there is none), its size the bytes left. 0x0043a8f0: up to
   `n` bytes of it (fewer at its end; the original's 0xe103 for a file
   shorter than its chunk cannot happen here: the chunk is clipped to the
   file). 0x0043a9f0 closes (nothing here). */
uint32_t aoe_wav_open_ror(AoeWavFileRor *f, const uint8_t *data, size_t size);
uint32_t aoe_wav_find_data_ror(AoeWavFileRor *f);
uint32_t aoe_wav_read_ror(AoeWavFileRor *f, uint8_t *out, size_t n, size_t *got);

typedef struct {
    void *context;
    /* DirectSound buffer: made (size bytes, the format; 0 on success),
       its play position, written at an offset, played looping, stopped,
       its volume, released. */
    int (*create)(void *context, size_t size, const AoeWavFormatRor *format);
    size_t (*position)(void *context);
    void (*write)(void *context, size_t offset, const uint8_t *data, size_t n);
    void (*play)(void *context, int32_t volume);
    void (*stop)(void *context);
    void (*release)(void *context);
    /* The window's messages: 0x500 the end, 0x502 progress. */
    void (*post)(void *context, uint32_t message);
    /* timeSetEvent(50 ms) (nonzero when the timer runs) / timeKillEvent. */
    int (*start_timer)(void *context);
    void (*stop_timer)(void *context);
} AoeWavStreamOpsRor;

typedef struct {
    AoeWavFileRor file;
    int open;                      /* 0x007d3be8 */
    int playing;                   /* 0x007c0320 */
    int timer;                     /* 0x007d3be4 */
    int enabled;                   /* 0x00582ed0 */
    int busy;                      /* 0x00582ed4 */
    int paused;                    /* 0x007d3bdc */
    int stopping;                  /* 0x007d3bc8 */
    int loop;                      /* 0x007d3bcc */
    int ending;                    /* 0x007d3bd0: the file ran out */
    int32_t drain;                 /* 0x007d3bd4: segments still to play */
    size_t segment, ring;          /* 0x007d3bb8, 0x007d3bb4 */
    size_t cursor;                 /* 0x007d3bbc: next segment to fill */
    size_t last_position;          /* 0x007d3bc0 */
    size_t played;                 /* 0x007d3bc4 */
    int32_t percent;               /* 0x007d3b68 */
    uint8_t scratch[0x10000];
    const AoeWavStreamOpsRor *ops;
} AoeWavStreamRor;

/* 0x0043aa30: set up (100%, nothing open, enabled). 0x0043aa80: shut down
   (stopping what plays). */
void aoe_wav_stream_init_ror(AoeWavStreamRor *s, const AoeWavStreamOpsRor *ops);
void aoe_wav_stream_shutdown_ror(AoeWavStreamRor *s);
/* 0x0043aac0(file, loop, volume): stop what plays, open a PCM file (else
   0), find its data, make the buffer of six segments and fill it whole
   (a short file loops or is padded with silence, 0x80 for 8 bits and 0
   otherwise, and its end is marked), post progress, play looping at the
   volume and start the timer. 1 when it plays. Host-only: the original
   undoes the start when timeBeginPeriod fails; the timer op only reports
   whether it runs, and a ring above 64 KB is refused. */
int aoe_wav_stream_play_ror(AoeWavStreamRor *s, const uint8_t *data, size_t size, int loop,
                            int32_t volume);
/* 0x0043ae40: stop the timer, the buffer and release it. */
void aoe_wav_stream_stop_ror(AoeWavStreamRor *s);
/* 0x0043aef0: 0x500 stops; 0x502 recomputes the percentage played. */
void aoe_wav_stream_message_ror(AoeWavStreamRor *s, uint32_t message);
/* 0x0043af60, the timer: unless busy, paused or stopping, and once the
   play position has left the segment at the cursor: with the file ended
   and nothing left to drain, post the end; else count what was played,
   post progress, and fill the cursor's segment (from the file, looping
   or padding; or with silence while draining, down to 2 segments) and
   advance it around the ring. */
void aoe_wav_stream_tick_ror(AoeWavStreamRor *s);

#endif
