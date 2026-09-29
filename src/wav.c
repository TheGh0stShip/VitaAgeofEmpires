#include "wav.h"

#include <limits.h>
#include <string.h>

static uint16_t le16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int aoe_wav_parse(const uint8_t *bytes, size_t size, AoeWavView *wav) {
    size_t limit, pos;
    int have_fmt = 0, have_data = 0;

    if (!wav) return 0;
    memset(wav, 0, sizeof(*wav));
    if (!bytes || size < 12 || memcmp(bytes, "RIFF", 4) != 0 ||
        memcmp(bytes + 8, "WAVE", 4) != 0)
        return 0;

    {
        uint32_t riff_size = le32(bytes + 4);
        if (riff_size < 4 || (uint64_t)riff_size + 8u > size)
            return 0;
        limit = (size_t)riff_size + 8u;
    }

    pos = 12;
    while (pos < limit) {
        const uint8_t *chunk;
        uint32_t chunk_size;
        size_t payload, next;

        if (limit - pos < 8) return 0;
        chunk = bytes + pos;
        chunk_size = le32(chunk + 4);
        payload = pos + 8;
        if ((uint64_t)chunk_size > (uint64_t)(limit - payload))
            return 0;

        if (!have_fmt && memcmp(chunk, "fmt ", 4) == 0) {
            if (chunk_size < 16) return 0;
            wav->format_tag = le16(bytes + payload);
            wav->channels = le16(bytes + payload + 2);
            wav->sample_rate = le32(bytes + payload + 4);
            wav->byte_rate = le32(bytes + payload + 8);
            wav->block_align = le16(bytes + payload + 12);
            wav->bits_per_sample = le16(bytes + payload + 14);
            if (!wav->channels || !wav->sample_rate || !wav->block_align)
                return 0;
            have_fmt = 1;
        } else if (!have_data && memcmp(chunk, "data", 4) == 0) {
            wav->data = bytes + payload;
            wav->data_size = chunk_size;
            have_data = 1;
        }

        next = payload + (size_t)chunk_size;
        if (chunk_size & 1u) {
            if (next == limit) return 0; /* RIFF chunks are word aligned. */
            ++next;
        }
        if (next > limit) return 0;
        pos = next;
    }

    return pos == limit && have_fmt && have_data;
}

int aoe_wav_pcm_supported(const AoeWavView *wav) {
    uint32_t bytes_per_sample, expected_align;
    uint64_t expected_rate;

    if (!wav || wav->format_tag != 1 ||
        (wav->channels != 1 && wav->channels != 2) ||
        (wav->bits_per_sample != 8 && wav->bits_per_sample != 16))
        return 0;

    bytes_per_sample = (uint32_t)wav->bits_per_sample / 8u;
    expected_align = (uint32_t)wav->channels * bytes_per_sample;
    expected_rate = (uint64_t)wav->sample_rate * expected_align;
    return wav->block_align == expected_align &&
           expected_rate <= UINT32_MAX &&
           wav->byte_rate == (uint32_t)expected_rate;
}
