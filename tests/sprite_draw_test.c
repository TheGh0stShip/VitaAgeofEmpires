#include "sprite_draw.h"
#include "color_table.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t fixed_table_marker[256] = {1};

static const uint8_t *fixed_table(void *context, int16_t color_id) {
    assert(context == (void *)(uintptr_t)0x55 && color_id == 7);
    return fixed_table_marker;
}

static void composite_plan_test(void) {
    DatGraphic child[2] = {
        {.present = 1, .layer = 5, .frame_count = 2, .angle_count = 4},
        {.present = 1, .layer = 11, .frame_count = 1, .angle_count = 8}
    };
    DatGraphicDelta deltas[2] = {
        {.graphic_id = 1, .graphic = &child[0], .offset_x = 3,
         .offset_y = -4, .display_angle = -1},
        {.graphic_id = 2, .graphic = &child[1], .offset_x = -1,
         .offset_y = 2, .display_angle = 3}
    };
    DatGraphic root = {.present = 1, .layer = 1, .frame_count = 1,
                       .angle_count = 8, .delta_count = 2,
                       .deltas = deltas};
    AoeSpriteInstance instance = {.graphic = &root, .offset_x = 10,
                                  .offset_y = 20, .kind = 1};
    AoeSpriteDrawPart parts[2];
    size_t count = 99;
    assert(aoe_sprite_draw_instance_plan_ror(
        &instance, 3, 100, 200, 300, 400, NULL, 0, &count));
    assert(count == 2);
    assert(aoe_sprite_draw_instance_plan_ror(
        &instance, 3, 100, 200, 300, 400, parts, 2, &count));
    assert(parts[0].graphic == &child[0] &&
           parts[0].anchor_x == 313 && parts[0].anchor_y == 416 &&
           parts[0].frame.slp_frame == 2);
    assert(parts[1].graphic == &child[1] &&
           parts[1].anchor_x == 109 && parts[1].anchor_y == 222 &&
           parts[1].frame.slp_frame == 3);
    assert(!aoe_sprite_draw_instance_plan_ror(
        &instance, 3, 100, 200, 300, 400, parts, 1, &count));
    DatGraphicDelta self = {.graphic_id = -1, .graphic = &root,
                            .offset_x = 2, .offset_y = 4,
                            .display_angle = -1};
    root.delta_count = 1;
    root.deltas = &self;
    assert(aoe_sprite_draw_instance_plan_ror(
        &instance, 3, 100, 200, 300, 400, parts, 1, &count));
    assert(count == 1 && parts[0].graphic == &root &&
           parts[0].anchor_x == 312 && parts[0].anchor_y == 424 &&
           parts[0].frame.slp_frame == 3);
    root.delta_count = 0;
    root.deltas = NULL;
    assert(aoe_sprite_draw_instance_plan_ror(
        &instance, 99, 100, 200, 300, 400, parts, 1, &count));
    assert(count == 1 && parts[0].anchor_x == 310 &&
           parts[0].anchor_y == 420 && parts[0].frame.slp_frame == 0);
    uint8_t selected_table[256] = {0};
    root.player_color = -1;
    assert(aoe_sprite_draw_transform_table_ror(
        &root, selected_table, fixed_table, (void *)(uintptr_t)0x55) ==
        selected_table);
    root.player_color = 7;
    assert(aoe_sprite_draw_transform_table_ror(
        &root, selected_table, fixed_table, (void *)(uintptr_t)0x55) ==
        fixed_table_marker);
}

static int real_composite_plan_audit(const DatGraphics *graphics) {
    size_t roots = 0, plans = 0, unresolved = 0;
    for (uint16_t i = 0; i < graphics->count; ++i) {
        const DatGraphic *root = &graphics->graphics[i];
        int root_unresolved = 0;
        if (!root->present || !root->delta_count || !root->angle_count ||
            !root->frame_count) continue;
        ++roots;
        AoeSpriteInstance instance = {.graphic = root, .kind = 1};
        AoeSpriteDrawPart *parts = calloc(root->delta_count, sizeof(*parts));
        if (!parts) return 0;
        for (uint16_t d = 0; d < root->delta_count; ++d)
            if (!root->deltas[d].graphic) {
                fprintf(stderr,
                        "unresolved delta: graphic %u (%s), delta %u id %d\n",
                        i, root->name, d, root->deltas[d].graphic_id);
                fprintf(stderr, "placeholder %08x offset %d,%d angle %d\n",
                        root->deltas[d].pointer_placeholder,
                        root->deltas[d].offset_x, root->deltas[d].offset_y,
                        root->deltas[d].display_angle);
                ++unresolved;
                root_unresolved = 1;
            }
        if (root_unresolved) {
            free(parts);
            continue;
        }
        for (uint16_t angle = 0; angle < root->angle_count; ++angle) {
            size_t count = 0;
            if (!aoe_sprite_draw_instance_plan_ror(
                    &instance, angle, -100, -50, 100, 50,
                    parts, root->delta_count, &count)) {
                fprintf(stderr,
                        "composite plan failed: graphic %u (%s), angle %u\n",
                        i, root->name, angle);
                free(parts);
                return 0;
            }
            ++plans;
        }
        free(parts);
    }
    printf("composite graphics: %zu roots, %zu angle plans, "
           "%zu unresolved deltas\n", roots, plans, unresolved);
    return roots != 0 && plans != 0 && unresolved == 0;
}

static int real_player_draw(const DatGraphics *graphics,
                            const Palette *palette,
                            DrsArchive *const *archives,
                            size_t archive_count) {
    AoeRng rng;
    aoe_rng_seed(&rng, 1);
    for (uint16_t i = 0; i < graphics->count; ++i) {
        const DatGraphic *graphic = &graphics->graphics[i];
        if (!graphic->present || !graphic->frame_count ||
            !graphic->angle_count || graphic->slp_id < 0) continue;
        AoeSpriteInstance instance;
        AoeGraphicFrame selection;
        SlpFrame frame = {0};
        if (!aoe_sprite_instance_init(&instance, graphic, &rng, 0, 0) ||
            !aoe_sprite_instance_frame(&instance, 0, &selection) ||
            aoe_graphic_art_load_frame(graphic, &selection, archives,
                                       archive_count, &frame) !=
                AOE_GRAPHIC_ART_OK) continue;
        size_t players = 0;
        for (size_t j = 0; j < (size_t)frame.width * frame.height; ++j)
            players += frame.pixels[j].kind == SLP_PLAYER;
        if (!players || !aoe_graphic_player_only(&frame) ||
            frame.width > 256 || frame.height > 256) {
            slp_free_frame(&frame);
            continue;
        }
        uint32_t width = frame.width + 8, height = frame.height + 8;
        size_t bytes = (size_t)width * height * 4;
        uint8_t *expected = calloc(bytes, 1), *actual = calloc(bytes, 1);
        if (!expected || !actual) {
            free(expected);
            free(actual);
            slp_free_frame(&frame);
            return 0;
        }
        int32_t anchor_x = frame.hotspot_x + 4;
        int32_t anchor_y = frame.hotspot_y + 4;
        if (instance.kind == 1) {
            anchor_x -= instance.offset_x;
            anchor_y -= instance.offset_y;
        }
        int32_t x, y;
        AoeGraphicPlayerEffects effects = {
            palette, graphic->player_color >= 0 ?
                (uint8_t)graphic->player_color : 7
        };
        AoeSpriteDrawSurface surface = {actual, bytes, width, height,
                                        (size_t)width * 4};
        int rejects_unresolved =
            aoe_sprite_draw_instance_cached_rgba(&instance, 0,
                anchor_x, anchor_y, archives, archive_count, NULL,
                palette, &surface, NULL, NULL) == AOE_GRAPHIC_ART_INVALID;
        for (size_t j = 0; j < bytes; ++j)
            rejects_unresolved &= actual[j] == 0;
        int ok = aoe_sprite_draw_position(&instance, anchor_x, anchor_y,
                                           &x, &y) &&
                 rejects_unresolved &&
                 slp_blit_graphic_rgba(&frame, palette, expected, bytes,
                       width, height, surface.stride, x, y,
                       selection.mirror_horizontal,
                       aoe_graphic_apply_player, &effects) &&
                 aoe_sprite_draw_instance_player_cached_rgba(&instance, 0,
                       anchor_x, anchor_y, archives, archive_count, NULL,
                       palette, &surface, 7) == AOE_GRAPHIC_ART_OK &&
                 memcmp(expected, actual, bytes) == 0;
        if (ok) {
            size_t pixel_count = (size_t)width * height;
            uint8_t *indices = calloc(pixel_count, 1);
            if (!indices) ok = 0;
            else {
                AoeSpriteIndexSurface indexed = {indices, pixel_count,
                                                 width, height, width};
                ok = aoe_sprite_draw_instance_cached_indexed(&instance, 0,
                          anchor_x, anchor_y, archives, archive_count,
                          NULL, &indexed, 7, NULL) == AOE_GRAPHIC_ART_OK;
                int64_t left = selection.mirror_horizontal ?
                    (int64_t)x + frame.hotspot_x - frame.width :
                    (int64_t)x - frame.hotspot_x;
                int64_t top = (int64_t)y - frame.hotspot_y;
                for (uint32_t sy = 0; sy < frame.height && ok; ++sy)
                    for (uint32_t sx = 0; sx < frame.width && ok; ++sx) {
                        const SlpPixel *pixel =
                            &frame.pixels[(size_t)sy * frame.width + sx];
                        if (pixel->kind != SLP_PLAYER) continue;
                        int64_t dx = left + (selection.mirror_horizontal ?
                            frame.width - 1 - sx : sx);
                        int64_t dy = top + sy;
                        if (dx < 0 || dx >= width || dy < 0 || dy >= height) {
                            ok = 0;
                            break;
                        }
                        ok = indices[(size_t)dy * width + (size_t)dx] ==
                             aoe_graphic_player_palette_index(pixel->index,
                                                                effects.player_color);
                    }
                free(indices);
            }
        }
        if (ok) {
            DatGraphic fixed = *graphic;
            fixed.player_color = 2;
            instance.graphic = &fixed;
            effects.player_color = 2;
            memset(expected, 0, bytes);
            memset(actual, 0, bytes);
            ok = slp_blit_graphic_rgba(&frame, palette, expected, bytes,
                       width, height, surface.stride, x, y,
                       selection.mirror_horizontal,
                       aoe_graphic_apply_player, &effects) &&
                 aoe_sprite_draw_instance_player_cached_rgba(&instance, 0,
                       anchor_x, anchor_y, archives, archive_count, NULL,
                       palette, &surface, 7) == AOE_GRAPHIC_ART_OK &&
                 memcmp(expected, actual, bytes) == 0;
        }
        if (ok)
            printf("player-color graphic %u (%s), %zu player pixels, DAT color %d\n",
                   i, graphic->name, players, graphic->player_color);
        free(expected);
        free(actual);
        slp_free_frame(&frame);
        return ok;
    }
    return 0;
}

static int real_shadow_draw(const DatGraphics *graphics,
                            const Palette *palette,
                            DrsArchive *const *archives,
                            size_t archive_count) {
    AoeRng rng;
    aoe_rng_seed(&rng, 1);
    for (uint16_t i = 0; i < graphics->count; ++i) {
        const DatGraphic *graphic = &graphics->graphics[i];
        if (!graphic->present || !graphic->frame_count ||
            !graphic->angle_count || graphic->slp_id < 0 ||
            graphic->player_color != 10) continue;
        AoeSpriteInstance instance;
        AoeGraphicFrame selection;
        SlpFrame frame = {0};
        if (!aoe_sprite_instance_init(&instance, graphic, &rng, 0, 0) ||
            !aoe_sprite_instance_frame(&instance, 0, &selection) ||
            aoe_graphic_art_load_frame(graphic, &selection, archives,
                                       archive_count, &frame) !=
                AOE_GRAPHIC_ART_OK) continue;
        size_t shadows = 0;
        int outline = 0;
        for (size_t j = 0; j < (size_t)frame.width * frame.height; ++j) {
            shadows += frame.pixels[j].kind == SLP_SHADOW;
            outline |= frame.pixels[j].kind >= SLP_OUTLINE1;
        }
        if (!shadows || outline || frame.width > 256 ||
            frame.height > 256) {
            slp_free_frame(&frame);
            continue;
        }
        uint32_t width = frame.width + 8, height = frame.height + 8;
        size_t bytes = (size_t)width * height;
        uint8_t *indices = malloc(bytes);
        if (!indices) {
            slp_free_frame(&frame);
            return 0;
        }
        memset(indices, 5, bytes);
        AoeSpriteIndexSurface surface = {indices, bytes, width,
                                         height, width};
        int32_t anchor_x = frame.hotspot_x + 4;
        int32_t anchor_y = frame.hotspot_y + 4;
        if (instance.kind == 1) {
            anchor_x -= instance.offset_x;
            anchor_y -= instance.offset_y;
        }
        int32_t x, y;
        int ok = aoe_sprite_draw_position(&instance, anchor_x, anchor_y,
                                           &x, &y) &&
                 aoe_sprite_draw_instance_cached_indexed(&instance, 0,
                       anchor_x, anchor_y, archives, archive_count, NULL,
                       &surface, 7, NULL) == AOE_GRAPHIC_ART_INVALID;
        for (size_t j = 0; j < bytes; ++j)
            ok &= indices[j] == 5;
        uint8_t shadow_table[256];
        if (ok)
            ok = aoe_shadow_table_load(archives, archive_count,
                                       shadow_table) &&
                 aoe_sprite_draw_instance_cached_indexed(&instance, 0,
                    anchor_x, anchor_y, archives, archive_count, NULL,
                    &surface, 7, shadow_table) == AOE_GRAPHIC_ART_OK;
        int64_t left = selection.mirror_horizontal ?
            (int64_t)x + frame.hotspot_x - frame.width :
            (int64_t)x - frame.hotspot_x;
        int64_t top = (int64_t)y - frame.hotspot_y;
        int64_t sample_dx = -1, sample_dy = -1;
        for (uint32_t sy = 0; sy < frame.height && ok; ++sy)
            for (uint32_t sx = 0; sx < frame.width && ok; ++sx) {
                if (frame.pixels[(size_t)sy * frame.width + sx].kind !=
                    SLP_SHADOW) continue;
                int64_t dx = left + (selection.mirror_horizontal ?
                    frame.width - 1 - sx : sx);
                int64_t dy = top + sy;
                if (dx < 0 || dx >= width || dy < 0 || dy >= height) {
                    ok = 0;
                    break;
                }
                sample_dx = dx;
                sample_dy = dy;
                ok = indices[(size_t)dy * width + (size_t)dx] ==
                     shadow_table[5];
            }
        if (ok && sample_dx >= 0) {
            size_t rgba_bytes = bytes * 4;
            uint8_t *rgba = malloc(rgba_bytes);
            if (!rgba) ok = 0;
            else {
                ok = palette_expand_indexed_rgba(palette, indices, bytes,
                          width, height, width, rgba, rgba_bytes,
                          (size_t)width * 4);
                if (ok) {
                    PaletteColor color = palette->colors[shadow_table[5]];
                    const uint8_t *pixel = rgba +
                        ((size_t)sample_dy * width + (size_t)sample_dx) * 4;
                    ok = pixel[0] == color.r && pixel[1] == color.g &&
                         pixel[2] == color.b && pixel[3] == 255;
                }
                free(rgba);
            }
        }
        if (ok)
            printf("shadow graphic %u (%s), %zu shadow pixels, DAT color %d (original table)\n",
                   i, graphic->name, shadows, graphic->player_color);
        free(indices);
        slp_free_frame(&frame);
        return ok;
    }
    return 0;
}

static int real_draw(const char *dat_path, DrsArchive *const *archives,
                     size_t archive_count) {
    DatBlob blob = {0};
    DatIntro intro = {0};
    DatSounds sounds = {0};
    DatGraphics graphics = {0};
    Palette palette = {0};
    DrsMatch palette_match;
    uint8_t *palette_bytes = NULL;
    uint8_t *surface_bytes = NULL, *runtime_bytes = NULL;
    AoeGraphicArtCache cache = {0};
    int ok = 0;
    if (!dat_load(dat_path, &blob) ||
        !dat_parse_intro(&blob, &intro) ||
        intro.color_count <= 10 ||
        strcmp(intro.colors[10].name, "Shadow.col") != 0 ||
        !dat_parse_sounds(&blob, &intro, &sounds) ||
        !dat_parse_graphics(&blob, &sounds, &graphics) ||
        !real_composite_plan_audit(&graphics) ||
        !drs_find_first(archives, archive_count, drs_type("anib"),
                        50500, &palette_match)) goto done;
    palette_bytes = malloc(palette_match.entry->size);
    if (!palette_bytes ||
        !drs_read(palette_match.archive, palette_match.entry,
                  palette_bytes, palette_match.entry->size) ||
        !palette_parse_jasc(palette_bytes, palette_match.entry->size,
                            &palette)) goto done;

    for (uint16_t i = 0; i < graphics.count; ++i) {
        const DatGraphic *graphic = &graphics.graphics[i];
        if (!graphic->present || !graphic->frame_count ||
            !graphic->angle_count || graphic->mirror_mode ||
            graphic->slp_id < 0) continue;
        AoeGraphicFrame selected;
        SlpFrame frame = {0};
        if (!aoe_graphic_select_frame(graphic, 0, 0, &selected) ||
            aoe_graphic_art_load_frame(graphic, &selected, archives,
                                       archive_count, &frame) !=
                AOE_GRAPHIC_ART_OK) continue;
        size_t opaque = 0;
        int has_special = 0;
        for (size_t j = 0; j < (size_t)frame.width * frame.height; ++j) {
            if (frame.pixels[j].kind == SLP_COLOR) ++opaque;
            if (frame.pixels[j].kind >= SLP_PLAYER) has_special = 1;
        }
        if (has_special || !opaque || frame.width > 256 ||
            frame.height > 256) {
            slp_free_frame(&frame);
            continue;
        }
        uint32_t width = frame.width + 8, height = frame.height + 8;
        size_t bytes = (size_t)width * height * 4;
        surface_bytes = calloc(bytes, 1);
        runtime_bytes = calloc(bytes, 1);
        if (!surface_bytes || !runtime_bytes) {
            slp_free_frame(&frame);
            goto done;
        }
        AoeSpriteDrawSurface surface = {surface_bytes, bytes, width,
                                        height, (size_t)width * 4};
        AoeRng rng;
        aoe_rng_seed(&rng, 1);
        AoeSpriteInstance instance;
        if (!aoe_sprite_instance_init(&instance, graphic, &rng, 0, 0)) {
            slp_free_frame(&frame);
            goto done;
        }
        int32_t anchor_x = frame.hotspot_x + 4;
        int32_t anchor_y = frame.hotspot_y + 4;
        int status = aoe_sprite_draw_instance_rgba(&instance, 0,
                        anchor_x, anchor_y, archives, archive_count,
                        &palette, &surface, NULL, NULL);
        if (status != AOE_GRAPHIC_ART_OK) {
            slp_free_frame(&frame);
            goto done;
        }
        AoeSpriteRuntime runtime;
        if (!aoe_sprite_runtime_init(&runtime, &rng, NULL, NULL, NULL) ||
            !aoe_sprite_runtime_add(&runtime, graphic, 60, 0, 0)) {
            slp_free_frame(&frame);
            goto done;
        }
        surface.rgba = runtime_bytes;
        status = aoe_sprite_draw_runtime_rgba(&runtime, 0,
                    anchor_x, anchor_y, archives, archive_count,
                    &palette, &surface, NULL, NULL);
        aoe_sprite_runtime_clear(&runtime);
        slp_free_frame(&frame);
        if (status != AOE_GRAPHIC_ART_OK ||
            memcmp(surface_bytes, runtime_bytes, bytes) != 0)
            goto done;
        if (!aoe_graphic_art_cache_init(&cache, 2, 1024u * 1024u))
            goto done;
        memset(runtime_bytes, 0, bytes);
        status = aoe_sprite_draw_instance_cached_rgba(&instance, 0,
                    anchor_x, anchor_y, archives, archive_count, &cache,
                    &palette, &surface, NULL, NULL);
        if (status != AOE_GRAPHIC_ART_OK ||
            memcmp(surface_bytes, runtime_bytes, bytes) != 0 ||
            cache.misses != 1 || cache.hits != 0)
            goto done;
        memset(runtime_bytes, 0, bytes);
        status = aoe_sprite_draw_instance_cached_rgba(&instance, 0,
                    anchor_x, anchor_y, archives, archive_count, &cache,
                    &palette, &surface, NULL, NULL);
        if (status != AOE_GRAPHIC_ART_OK ||
            memcmp(surface_bytes, runtime_bytes, bytes) != 0 ||
            cache.misses != 1 || cache.hits != 1)
            goto done;
        aoe_graphic_art_cache_clear(&cache);
        if (!aoe_graphic_art_cache_init(&cache, 1, 1))
            goto done;
        memset(runtime_bytes, 0, bytes);
        status = aoe_sprite_draw_instance_cached_rgba(&instance, 0,
                    anchor_x, anchor_y, archives, archive_count, &cache,
                    &palette, &surface, NULL, NULL);
        if (status != AOE_GRAPHIC_ART_OK ||
            memcmp(surface_bytes, runtime_bytes, bytes) != 0 ||
            cache.misses != 1 || cache.used_pixel_bytes != 0)
            goto done;
        DatGraphic missing = *graphic;
        missing.slp_id = 251; /* Absent from both installed DRS stacks. */
        AoeSpriteRuntime mixed = {0};
        if (!aoe_sprite_runtime_init(&mixed, &rng, NULL, NULL, NULL))
            goto done;
        if (!aoe_sprite_runtime_add(&mixed, &missing, 0, 0, 0) ||
            !aoe_sprite_runtime_add(&mixed, graphic, 1, 0, 0)) {
            aoe_sprite_runtime_clear(&mixed);
            goto done;
        }
        if (aoe_sprite_runtime_at(&mixed, 0)->graphic != &missing) {
            aoe_sprite_runtime_clear(&mixed);
            goto done;
        }
        memset(runtime_bytes, 0, bytes);
        status = aoe_sprite_draw_runtime_rgba(&mixed, 0, anchor_x,
                    anchor_y, archives, archive_count, &palette, &surface,
                    NULL, NULL);
        if (status != AOE_GRAPHIC_ART_MISSING ||
            memcmp(surface_bytes, runtime_bytes, bytes) != 0) {
            aoe_sprite_runtime_clear(&mixed);
            goto done;
        }
        size_t indexed_bytes = (size_t)width * height;
        uint8_t *indices = calloc(indexed_bytes, 1);
        if (!indices) {
            aoe_sprite_runtime_clear(&mixed);
            goto done;
        }
        AoeSpriteIndexSurface indexed = {indices, indexed_bytes,
                                         width, height, width};
        status = aoe_sprite_draw_runtime_cached_indexed(&mixed, 0,
                    anchor_x, anchor_y, archives, archive_count, NULL,
                    &indexed, 0, NULL);
        aoe_sprite_runtime_clear(&mixed);
        if (status != AOE_GRAPHIC_ART_MISSING) {
            free(indices);
            goto done;
        }
        for (size_t j = 0; j < indexed_bytes; ++j) {
            if (surface_bytes[j * 4 + 3] != 255) continue;
            PaletteColor color = palette.colors[indices[j]];
            if (surface_bytes[j * 4] != color.r ||
                surface_bytes[j * 4 + 1] != color.g ||
                surface_bytes[j * 4 + 2] != color.b) {
                free(indices);
                goto done;
            }
        }
        free(indices);
        size_t changed = 0;
        for (size_t j = 3; j < bytes; j += 4)
            if (surface_bytes[j] == 255) ++changed;
        if (changed != opaque) goto done;
        printf("%s: rendered graphic %u (%s), %zu palette pixels\n",
               dat_path, i, graphic->name, changed);
        ok = 1;
        break;
    }
    if (ok)
        ok = real_player_draw(&graphics, &palette, archives, archive_count);
    if (ok)
        ok = real_shadow_draw(&graphics, &palette, archives, archive_count);
done:
    aoe_graphic_art_cache_clear(&cache);
    free(runtime_bytes);
    free(surface_bytes);
    free(palette_bytes);
    dat_free_graphics(&graphics);
    dat_free_sounds(&sounds);
    dat_free_intro(&intro);
    dat_free(&blob);
    return ok;
}

int main(int argc, char **argv) {
    composite_plan_test();
    AoeSpriteInstance position = {.kind = 1,
                                  .offset_x = 7, .offset_y = -3};
    int32_t x = 0, y = 0;
    assert(aoe_sprite_draw_position(&position, 10, 20, &x, &y));
    assert(x == 17 && y == 17);
    position.kind = 2;
    assert(aoe_sprite_draw_position(&position, 10, 20, &x, &y));
    assert(x == 10 && y == 20);
    position.kind = 1;
    assert(!aoe_sprite_draw_position(&position, INT32_MAX, 20, &x, &y));
    assert(x == 10 && y == 20);
    assert(aoe_sprite_draw_instance_rgba(NULL, 0, 0, 0,
                                          NULL, 0, NULL, NULL,
                                          NULL, NULL) ==
           AOE_GRAPHIC_ART_INVALID);
    if (argc == 1) return 0;
    if (argc < 3 || argc > 10) return 2;
    DrsArchive archives[8] = {{0}};
    DrsArchive *stack[8];
    size_t count = (size_t)argc - 2, opened = 0;
    for (; opened < count; ++opened) {
        if (!drs_open(&archives[opened], argv[opened + 2])) break;
        stack[opened] = &archives[opened];
    }
    int ok = opened == count && real_draw(argv[1], stack, count);
    for (size_t i = 0; i < opened; ++i)
        drs_close(&archives[i]);
    return ok ? 0 : 1;
}
