#include "drs.h"
#include "wav.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_file(const char *path, uint8_t **bytes, size_t *size) {
    FILE *file;
    long end;
    uint8_t *buffer;

    *bytes = NULL;
    *size = 0;
    file = fopen(path, "rb");
    if (!file || fseek(file, 0, SEEK_END) != 0 ||
        (end = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) != 0) {
        if (file) fclose(file);
        return 0;
    }
    buffer = malloc(end ? (size_t)end : 1);
    if (!buffer || (end && fread(buffer, 1, (size_t)end, file) != (size_t)end)) {
        free(buffer);
        fclose(file);
        return 0;
    }
    fclose(file);
    *bytes = buffer;
    *size = (size_t)end;
    return 1;
}

static int parse_id(const char *text, uint32_t *id) {
    char *end;
    unsigned long value;
    errno = 0;
    value = strtoul(text, &end, 0);
    if (errno || !*text || *end || value > UINT32_MAX) return 0;
    *id = (uint32_t)value;
    return 1;
}

static int show(const uint8_t *bytes, size_t size) {
    AoeWavView wav;
    if (!aoe_wav_parse(bytes, size, &wav)) {
        fprintf(stderr, "invalid or unsupported RIFF/WAVE structure\n");
        return 0;
    }
    printf("format tag: %u\nchannels: %u\nsample rate: %u\n"
           "byte rate: %u\nblock align: %u\nbits/sample: %u\n"
           "data bytes: %u\ndirect PCM layout: %s\n",
           wav.format_tag, wav.channels, wav.sample_rate, wav.byte_rate,
           wav.block_align, wav.bits_per_sample, wav.data_size,
           aoe_wav_pcm_supported(&wav) ? "yes" : "no");
    return 1;
}

int main(int argc, char **argv) {
    uint8_t *bytes = NULL;
    size_t size = 0;
    int ok = 0;

    if (argc == 3 && strcmp(argv[1], "file") == 0) {
        if (read_file(argv[2], &bytes, &size))
            ok = show(bytes, size);
    } else if (argc == 4 && strcmp(argv[1], "drs") == 0) {
        DrsArchive archive = {0};
        const DrsEntry *entry;
        uint32_t id;
        if (parse_id(argv[3], &id) && drs_open(&archive, argv[2]) &&
            (entry = drs_find(&archive, drs_type(" vaw"), id)) != NULL) {
            bytes = malloc(entry->size ? entry->size : 1);
            if (bytes && drs_read(&archive, entry, bytes, entry->size)) {
                size = entry->size;
                ok = show(bytes, size);
            }
        }
        drs_close(&archive);
    } else {
        fprintf(stderr, "usage: wav_tool file SOUND.wav | "
                        "wav_tool drs SOUNDS.DRS RESOURCE_ID\n");
        return 2;
    }

    free(bytes);
    return ok ? 0 : 1;
}
