#include "wav.h"

#include <assert.h>
#include <string.h>

static void put16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put32(uint8_t *p, uint32_t value) {
    put16(p, (uint16_t)value);
    put16(p + 2, (uint16_t)(value >> 16));
}

int main(void) {
    uint8_t bytes[60] = {0};
    AoeWavView wav;

    memcpy(bytes, "RIFF", 4);
    put32(bytes + 4, sizeof(bytes) - 8);
    memcpy(bytes + 8, "WAVE", 4);

    memcpy(bytes + 12, "fmt ", 4);
    put32(bytes + 16, 16);
    put16(bytes + 20, 1);       /* PCM */
    put16(bytes + 22, 1);       /* mono */
    put32(bytes + 24, 22050);
    put32(bytes + 28, 44100);
    put16(bytes + 32, 2);
    put16(bytes + 34, 16);

    memcpy(bytes + 36, "JUNK", 4);
    put32(bytes + 40, 3);
    bytes[44] = 1;
    bytes[45] = 2;
    bytes[46] = 3;
    bytes[47] = 0;              /* word-alignment pad */

    memcpy(bytes + 48, "data", 4);
    put32(bytes + 52, 4);
    bytes[56] = 0x11;
    bytes[57] = 0x22;
    bytes[58] = 0x33;
    bytes[59] = 0x44;

    assert(aoe_wav_parse(bytes, sizeof(bytes), &wav));
    assert(wav.format_tag == 1 && wav.channels == 1);
    assert(wav.sample_rate == 22050 && wav.byte_rate == 44100);
    assert(wav.block_align == 2 && wav.bits_per_sample == 16);
    assert(wav.data_size == 4 && wav.data == bytes + 56);
    assert(aoe_wav_pcm_supported(&wav));

    put16(bytes + 20, 2);       /* structurally valid non-PCM */
    assert(aoe_wav_parse(bytes, sizeof(bytes), &wav));
    assert(!aoe_wav_pcm_supported(&wav));
    put16(bytes + 20, 1);

    assert(!aoe_wav_parse(bytes, sizeof(bytes) - 1, &wav));
    put32(bytes + 40, 0x1000);  /* chunk extends beyond RIFF */
    assert(!aoe_wav_parse(bytes, sizeof(bytes), &wav));
    put32(bytes + 40, 3);

    memcpy(bytes, "RIFX", 4);
    assert(!aoe_wav_parse(bytes, sizeof(bytes), &wav));
    assert(!aoe_wav_parse(NULL, 0, &wav));
    assert(!aoe_wav_parse(bytes, sizeof(bytes), NULL));
    return 0;
}
