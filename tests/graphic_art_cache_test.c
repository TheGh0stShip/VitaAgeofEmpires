#include "graphic_art_cache.h"

#include <assert.h>
#include <stdio.h>

static void synthetic(void) {
    AoeGraphicArtCache cache = {0};
    assert(!aoe_graphic_art_cache_init(&cache, 0, 1024));
    assert(!aoe_graphic_art_cache_init(&cache, 2, 0));
    assert(aoe_graphic_art_cache_init(&cache, 1, 1024));
    const SlpFrame *frame = NULL;
    AoeGraphicFrame selected = {0};
    assert(aoe_graphic_art_cache_get(&cache, NULL, &selected,
                                     NULL, 0, &frame) ==
           AOE_GRAPHIC_ART_INVALID);
    assert(frame == NULL);
    aoe_graphic_art_cache_clear(&cache);
    assert(cache.entries == NULL && cache.capacity == 0);
}

static int real_dat(const char *path, DrsArchive *const *archives,
                    size_t archive_count) {
    DatBlob blob = {0};
    DatIntro intro = {0};
    DatSounds sounds = {0};
    DatGraphics graphics = {0};
    int ok = 0;
    if (!dat_load(path, &blob) || !dat_parse_intro(&blob, &intro) ||
        !dat_parse_sounds(&blob, &intro, &sounds) ||
        !dat_parse_graphics(&blob, &sounds, &graphics)) goto done;
    AoeGraphicArtCache cache = {0};
    if (!aoe_graphic_art_cache_init(&cache, 1, 2u * 1024u * 1024u))
        goto done;
    const DatGraphic *picked[2] = {0};
    for (uint16_t i = 0; i < graphics.count && !picked[1]; ++i) {
        const DatGraphic *graphic = &graphics.graphics[i];
        if (!graphic->present || !graphic->frame_count ||
            !graphic->angle_count || graphic->slp_id < 0 ||
            (picked[0] && picked[0]->slp_id == graphic->slp_id)) continue;
        AoeGraphicFrame selected;
        const SlpFrame *frame = NULL;
        if (!aoe_graphic_select_frame(graphic, 0, 0, &selected) ||
            aoe_graphic_art_cache_get(&cache, graphic, &selected,
                                       archives, archive_count, &frame) !=
                AOE_GRAPHIC_ART_OK) continue;
        assert(frame && frame->pixels);
        picked[picked[0] != NULL] = graphic;
        if (!picked[1]) {
            const SlpFrame *same = NULL;
            assert(aoe_graphic_art_cache_get(&cache, graphic, &selected,
                                              archives, archive_count,
                                              &same) == AOE_GRAPHIC_ART_OK);
            assert(same == frame);
        }
    }
    if (picked[0] && picked[1]) {
        AoeGraphicFrame selected;
        const SlpFrame *frame = NULL;
        assert(aoe_graphic_select_frame(picked[0], 0, 0, &selected));
        assert(aoe_graphic_art_cache_get(&cache, picked[0], &selected,
                                         archives, archive_count, &frame) ==
               AOE_GRAPHIC_ART_OK);
        assert(cache.hits >= 1 && cache.misses >= 3 &&
               cache.evictions >= 2);
        printf("%s: cache %llu hits, %llu misses, %llu evictions\n",
               path, (unsigned long long)cache.hits,
               (unsigned long long)cache.misses,
               (unsigned long long)cache.evictions);
        ok = 1;
    }
    aoe_graphic_art_cache_clear(&cache);
done:
    dat_free_graphics(&graphics);
    dat_free_sounds(&sounds);
    dat_free_intro(&intro);
    dat_free(&blob);
    return ok;
}

static int real_overlay(DrsArchive *expansion, DrsArchive *base) {
    const uint32_t type = drs_type(" pls");
    if (!drs_find(expansion, type, 149) || !drs_find(base, type, 149))
        return 0;
    DrsArchive *expansion_first[2] = {expansion, base};
    DrsArchive *base_first[2] = {base, expansion};
    DatGraphic graphic = {.present = 1, .slp_id = 149};
    AoeGraphicFrame selected = {.slp_frame = 0};
    AoeGraphicArtCache cache = {0};
    const SlpFrame *frame = NULL;
    if (!aoe_graphic_art_cache_init(&cache, 2, 1024u * 1024u))
        return 0;
    int ok = aoe_graphic_art_cache_get(&cache, &graphic, &selected,
                 expansion_first, 2, &frame) == AOE_GRAPHIC_ART_OK &&
             aoe_graphic_art_cache_get(&cache, &graphic, &selected,
                 base_first, 2, &frame) == AOE_GRAPHIC_ART_OK &&
             cache.misses == 2 && cache.hits == 0 &&
             aoe_graphic_art_cache_get(&cache, &graphic, &selected,
                 expansion_first, 2, &frame) == AOE_GRAPHIC_ART_OK &&
             cache.misses == 2 && cache.hits == 1;
    aoe_graphic_art_cache_clear(&cache);
    return ok;
}

int main(int argc, char **argv) {
    synthetic();
    if (argc == 1) return 0;
    if (argc < 3 || argc > 10) return 2;
    DrsArchive archives[8] = {{0}};
    DrsArchive *stack[8];
    size_t count = (size_t)argc - 2, opened = 0;
    for (; opened < count; ++opened) {
        if (!drs_open(&archives[opened], argv[opened + 2])) break;
        stack[opened] = &archives[opened];
    }
    int ok = opened == count && real_dat(argv[1], stack, count);
    if (ok && count == 4)
        ok = real_overlay(stack[0], stack[2]);
    for (size_t i = 0; i < opened; ++i)
        drs_close(&archives[i]);
    return ok ? 0 : 1;
}
