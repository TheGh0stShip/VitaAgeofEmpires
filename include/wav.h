#ifndef VITA_AOE_WAV_H
#define VITA_AOE_WAV_H

#include <stddef.h>
#include <stdint.h>

/* Bounds-checked view of one RIFF/WAVE resource. The view borrows the input
   buffer; callers must keep the source bytes alive while using data. */
typedef struct {
    uint16_t format_tag;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    const uint8_t *data;
    uint32_t data_size;
} AoeWavView;

/* Parses RIFF/WAVE chunk structure without decoding/compressing samples.
   Unknown format tags are accepted structurally so tools can inspect them. */
int aoe_wav_parse(const uint8_t *bytes, size_t size, AoeWavView *wav);

/* Returns nonzero only for PCM layouts the future Vita mixer can consume
   directly after the platform-specific signedness/endian conversion. */
int aoe_wav_pcm_supported(const AoeWavView *wav);

#endif
