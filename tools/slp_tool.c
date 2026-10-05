#include "drs.h"
#include "palette.h"
#include "slp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int render(const char *archive_path, uint32_t id,
                  const char *palette_path, const char *output_path,
                  uint32_t frame_index, uint32_t player_base) {
    DrsArchive archive = {0}, palettes = {0};
    const DrsEntry *sprite, *palette_entry;
    uint8_t *sprite_data = NULL, *palette_data = NULL;
    SlpFrame frame = {0};
    Palette palette;
    FILE *out = NULL;
    int ok = 0;
    if (!drs_open(&archive, archive_path) || !drs_open(&palettes, palette_path)) goto done;
    sprite = drs_find(&archive, drs_type(" pls"), id);
    palette_entry = drs_find(&palettes, drs_type("anib"), 50500);
    if (!sprite || !palette_entry) goto done;
    sprite_data = malloc(sprite->size ? sprite->size : 1);
    palette_data = malloc(palette_entry->size ? palette_entry->size : 1);
    if (!sprite_data || !palette_data ||
        !drs_read(&archive, sprite, sprite_data, sprite->size) ||
        !drs_read(&palettes, palette_entry, palette_data, palette_entry->size) ||
        !palette_parse_jasc(palette_data, palette_entry->size, &palette) ||
        !slp_decode_frame(sprite_data, sprite->size, frame_index, &frame)) goto done;
    out = fopen(output_path, "wb");
    if (!out || fprintf(out, "P7\nWIDTH %u\nHEIGHT %u\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n",
                        frame.width, frame.height) < 0) goto done;
    for (size_t i = 0; i < (size_t)frame.width * frame.height; ++i) {
        SlpPixel pixel = frame.pixels[i];
        uint32_t index = pixel.kind == SLP_PLAYER ?
            (player_base + pixel.index) & 255u : pixel.index;
        PaletteColor color = palette.colors[index];
        uint8_t rgba[4] = {color.r, color.g, color.b, 255};
        if (pixel.kind == SLP_TRANSPARENT) rgba[3] = 0;
        if (pixel.kind == SLP_SHADOW) {
            rgba[0] = rgba[1] = rgba[2] = 0;
            rgba[3] = 95;
        }
        if (fwrite(rgba, 1, 4, out) != 4) goto done;
    }
    ok = 1;
done:
    if (out) fclose(out);
    slp_free_frame(&frame);
    free(sprite_data);
    free(palette_data);
    drs_close(&archive);
    drs_close(&palettes);
    return ok;
}

int main(int argc, char **argv) {
    DrsArchive archive;
    size_t sprites = 0, frames = 0, failures = 0;
    int scan;
    if (argc >= 6 && argc <= 8 && !strcmp(argv[1], "render")) {
        /* Optional player colour: the palette index of its first shade
           (16, the blue player, by default). */
        if (render(argv[2], (uint32_t)strtoul(argv[3], NULL, 0), argv[4], argv[5],
                   argc >= 7 ? (uint32_t)strtoul(argv[6], NULL, 0) : 0,
                   argc == 8 ? (uint32_t)strtoul(argv[7], NULL, 0) : 16u))
            return 0;
        fprintf(stderr, "render failed\n");
        return 1;
    }
    if (argc < 3 || argc > 4 ||
        (strcmp(argv[1], "scan") && strcmp(argv[1], "info"))) {
        fprintf(stderr, "usage: slp_tool scan ARCHIVE | info ARCHIVE ID | render ARCHIVE ID PALETTE_ARCHIVE OUTPUT.pam [FRAME [PLAYER_BASE]]\n");
        return 2;
    }
    scan = strcmp(argv[1], "scan") == 0;
    if ((!scan && argc != 4) || !drs_open(&archive, argv[2])) {
        fprintf(stderr, "invalid archive or arguments\n");
        return 2;
    }
    for (size_t i = 0; i < archive.entry_count; ++i) {
        const DrsEntry *entry = &archive.entries[i];
        uint8_t *data;
        uint32_t count;
        size_t kinds[6] = {0};
        if (entry->type != drs_type(" pls") ||
            (!scan && entry->id != (uint32_t)strtoul(argv[3], NULL, 0))) continue;
        ++sprites;
        data = malloc(entry->size ? entry->size : 1);
        if (!data || !drs_read(&archive, entry, data, entry->size) ||
            !slp_frame_count(data, entry->size, &count)) {
            fprintf(stderr, "SLP %u: invalid header\n", entry->id);
            ++failures;
            free(data);
            continue;
        }
        for (uint32_t j = 0; j < count; ++j) {
            SlpFrame frame;
            ++frames;
            if (!slp_decode_frame(data, entry->size, j, &frame)) {
                if (failures < 20) fprintf(stderr, "SLP %u frame %u: decode failed\n", entry->id, j);
                ++failures;
            } else {
                if (!scan) printf("SLP %u frame %u: %ux%u hotspot (%d,%d)\n",
                                  entry->id, j, frame.width, frame.height,
                                  frame.hotspot_x, frame.hotspot_y);
                if (!scan) for (size_t p = 0;
                    p < (size_t)frame.width * frame.height; ++p)
                    if (frame.pixels[p].kind < 6)
                        ++kinds[frame.pixels[p].kind];
                slp_free_frame(&frame);
            }
        }
        if (!scan)
            printf("pixel kinds: transparent=%zu color=%zu player=%zu "
                   "shadow=%zu outline1=%zu outline2=%zu\n",
                   kinds[0], kinds[1], kinds[2], kinds[3], kinds[4], kinds[5]);
        free(data);
    }
    printf("%zu sprites, %zu frames, %zu failures\n", sprites, frames, failures);
    drs_close(&archive);
    return failures || !sprites ? 1 : 0;
}
