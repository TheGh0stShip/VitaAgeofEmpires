#include "graphic_art.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void synthetic(void) {
    DatGraphic graphic = {.present = 1, .slp_id = 123,
                          .frame_count = 1, .angle_count = 1};
    AoeGraphicFrame selected = {0};
    SlpFrame frame = {0};
    DrsEntry shp = {.type = 0, .id = 123};
    DrsArchive archive = {.entries = &shp, .entry_count = 1};
    DrsArchive *stack[] = {&archive};
    assert(aoe_graphic_art_load_frame(&graphic, &selected, stack, 1,
                                      &frame) == AOE_GRAPHIC_ART_MISSING);
    shp.type = drs_type(" phs");
    assert(aoe_graphic_art_load_frame(&graphic, &selected, stack, 1,
                                      &frame) == AOE_GRAPHIC_ART_UNSUPPORTED_SHP);
    assert(frame.pixels == NULL);
    graphic.present = 0;
    assert(aoe_graphic_art_load_frame(&graphic, &selected, stack, 1,
                                      &frame) == AOE_GRAPHIC_ART_INVALID);
}

static int all_frames(const char *path, const DatGraphics *graphics,
                      DrsArchive *const *archives, size_t archive_count) {
    const DrsEntry **seen = calloc(graphics->count, sizeof(*seen));
    if (!seen) return 0;
    size_t seen_count = 0, frames = 0, malformed = 0;
    size_t player = 0, shadow = 0, outline1 = 0, outline2 = 0;
    for (uint16_t i = 0; i < graphics->count; ++i) {
        const DatGraphic *graphic = &graphics->graphics[i];
        DrsMatch match;
        if (aoe_graphic_art_find(graphic, archives, archive_count,
                                 &match) != AOE_GRAPHIC_ART_OK)
            continue;
        size_t j = 0;
        for (; j < seen_count; ++j)
            if (seen[j] == match.entry) break;
        if (j != seen_count) continue;
        seen[seen_count++] = match.entry;
        uint8_t *bytes = malloc(match.entry->size ? match.entry->size : 1);
        if (!bytes) {
            free(seen);
            return 0;
        }
        uint32_t frame_count = 0;
        if (!drs_read(match.archive, match.entry, bytes,
                      match.entry->size) ||
            !slp_frame_count(bytes, match.entry->size, &frame_count)) {
            ++malformed;
            free(bytes);
            continue;
        }
        for (uint32_t frame_index = 0; frame_index < frame_count;
             ++frame_index) {
            SlpFrame frame = {0};
            if (!slp_decode_frame(bytes, match.entry->size,
                                  frame_index, &frame)) {
                ++malformed;
                continue;
            }
            unsigned kinds = 0;
            for (size_t p = 0; p < (size_t)frame.width * frame.height;
                 ++p)
                kinds |= 1u << frame.pixels[p].kind;
            player += (kinds & (1u << SLP_PLAYER)) != 0;
            shadow += (kinds & (1u << SLP_SHADOW)) != 0;
            outline1 += (kinds & (1u << SLP_OUTLINE1)) != 0;
            outline2 += (kinds & (1u << SLP_OUTLINE2)) != 0;
            if ((kinds & ((1u << SLP_OUTLINE1) |
                          (1u << SLP_OUTLINE2))) &&
                outline1 + outline2 <= 12)
                printf("%s: outline SLP %d frame %u (%s)\n", path,
                       graphic->slp_id, frame_index, graphic->name);
            ++frames;
            slp_free_frame(&frame);
        }
        free(bytes);
    }
    printf("%s: all direct-art frames: %zu unique SLPs, %zu decoded, "
           "%zu malformed; %zu player, %zu shadow, %zu outline1, "
           "%zu outline2 frames\n", path, seen_count, frames,
           malformed, player, shadow, outline1, outline2);
    free(seen);
    return malformed == 0;
}

static int real_dat(const char *path, DrsArchive *const *archives,
                    size_t archive_count, int audit_all_frames) {
    DatBlob blob = {0};
    DatIntro intro = {0};
    DatSounds sounds = {0};
    DatGraphics graphics = {0};
    if (!dat_load(path, &blob)) return 0;
    int ok = dat_parse_intro(&blob, &intro) &&
             dat_parse_sounds(&blob, &intro, &sounds) &&
             dat_parse_graphics(&blob, &sounds, &graphics);
    size_t loaded = 0, missing = 0, unsupported = 0, invalid = 0, blank = 0;
    size_t negative_slp = 0, unresolved_positive_slp = 0;
    size_t checked_angles = 0, out_of_range = 0;
    size_t player_frames = 0, shadow_frames = 0;
    size_t outline1_frames = 0, outline2_frames = 0;
    if (ok) {
        for (uint16_t i = 0; i < graphics.count; ++i) {
            const DatGraphic *graphic = &graphics.graphics[i];
            if (!graphic->present) continue;
            if (!graphic->frame_count || !graphic->angle_count) {
                ++blank;
                continue;
            }
            AoeGraphicFrame selected;
            SlpFrame frame = {0};
            if (!aoe_graphic_select_frame(graphic, 0, 0, &selected)) {
                ++invalid;
                continue;
            }
            int status = aoe_graphic_art_load_frame(graphic, &selected,
                                                     archives, archive_count,
                                                     &frame);
            if (status == AOE_GRAPHIC_ART_OK) {
                assert(frame.width && frame.height && frame.pixels);
                unsigned kinds = 0;
                for (size_t j = 0; j < (size_t)frame.width * frame.height;
                     ++j)
                    kinds |= 1u << frame.pixels[j].kind;
                player_frames += (kinds & (1u << SLP_PLAYER)) != 0;
                shadow_frames += (kinds & (1u << SLP_SHADOW)) != 0;
                outline1_frames += (kinds & (1u << SLP_OUTLINE1)) != 0;
                outline2_frames += (kinds & (1u << SLP_OUTLINE2)) != 0;
                if ((kinds & ((1u << SLP_OUTLINE1) |
                              (1u << SLP_OUTLINE2))) &&
                    outline1_frames + outline2_frames <= 6)
                    printf("%s: outline graphic %u (%s)\n", path, i,
                           graphic->name);
                ++loaded;
                slp_free_frame(&frame);
                uint32_t available = 0;
                if (aoe_graphic_art_frame_count(graphic, archives,
                                                 archive_count,
                                                 &available) !=
                    AOE_GRAPHIC_ART_OK) {
                    ++invalid;
                    continue;
                }
                for (uint16_t angle = 0; angle < graphic->angle_count;
                     ++angle) {
                    AoeGraphicFrame last = {0};
                    if (!aoe_graphic_select_frame(graphic, angle,
                                  (int16_t)(graphic->frame_count - 1),
                                  &last)) {
                        ++invalid;
                    } else if (last.slp_frame >= available) {
                        if (out_of_range < 8)
                            fprintf(stderr, "%s: graphic %u angle %u selects "
                                    "frame %u of %u\n", path, i, angle,
                                    last.slp_frame, available);
                        ++out_of_range;
                    } else {
                        ++checked_angles;
                    }
                }
            } else if (status == AOE_GRAPHIC_ART_MISSING) {
                ++missing;
                if (graphic->slp_id < 0) {
                    ++negative_slp;
                } else {
                    if (unresolved_positive_slp < 12)
                        printf("%s: unresolved graphic %u (%s), SLP %d\n",
                               path, i, graphic->name, graphic->slp_id);
                    ++unresolved_positive_slp;
                }
            } else if (status == AOE_GRAPHIC_ART_UNSUPPORTED_SHP) {
                ++unsupported;
            } else {
                fprintf(stderr, "%s: invalid graphic %u (%s), SLP %d, "
                        "selected frame %u\n", path, i, graphic->name,
                        graphic->slp_id, selected.slp_frame);
                ++invalid;
            }
        }
        printf("%s: %zu loaded, %zu missing, %zu SHP-only, "
               "%zu invalid, %zu blank, %zu angles in bounds, "
               "%zu DAT/SLP range mismatches\n",
               path, loaded, missing, unsupported, invalid, blank,
               checked_angles, out_of_range);
        printf("%s: first-frame effects: %zu player, %zu shadow, "
               "%zu outline1, %zu outline2\n", path, player_frames,
               shadow_frames, outline1_frames, outline2_frames);
        printf("%s: missing direct art: %zu negative DAT IDs, %zu "
               "unresolved positive SLP IDs\n", path, negative_slp,
               unresolved_positive_slp);
        ok = invalid == 0;
        if (ok && audit_all_frames)
            ok = all_frames(path, &graphics, archives, archive_count);
    }
    dat_free_graphics(&graphics);
    dat_free_sounds(&sounds);
    dat_free_intro(&intro);
    dat_free(&blob);
    return ok;
}

int main(int argc, char **argv) {
    synthetic();
    if (argc == 1) return 0;
    int audit_all_frames = argc > 1 &&
                           strcmp(argv[argc - 1], "--all-frames") == 0;
    if (argc < 3 + audit_all_frames || argc > 10 + audit_all_frames)
        return 2;
    size_t count = (size_t)argc - 2 - (size_t)audit_all_frames;
    DrsArchive archives[8] = {{0}};
    DrsArchive *stack[8];
    size_t opened = 0;
    for (; opened < count; ++opened) {
        if (!drs_open(&archives[opened], argv[opened + 2])) break;
        stack[opened] = &archives[opened];
    }
    int ok = opened == count && real_dat(argv[1], stack, count,
                                         audit_all_frames);
    for (size_t i = 0; i < opened; ++i)
        drs_close(&archives[i]);
    return ok ? 0 : 1;
}
