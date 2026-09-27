#include "graphic_frame.h"

#include <assert.h>
#include <stdio.h>

static void synthetic(void) {
    DatGraphic graphic = {.present = 1, .frame_count = 3,
                          .angle_count = 8};
    AoeGraphicFrame result = {99, 99};
    assert(aoe_graphic_select_frame(&graphic, 4, 2, &result));
    assert(result.slp_frame == 14 && result.mirror_horizontal == 0);
    graphic.mirror_mode = 5;
    assert(aoe_graphic_select_frame(&graphic, 0, 2, &result));
    assert(result.slp_frame == 8 && result.mirror_horizontal == 1);
    assert(aoe_graphic_select_frame(&graphic, 2, 1, &result));
    assert(result.slp_frame == 1 && result.mirror_horizontal == 0);
    assert(aoe_graphic_select_frame(&graphic, 6, 0, &result));
    assert(result.slp_frame == 12 && result.mirror_horizontal == 1);
    graphic.angle_count = 2;
    graphic.mirror_mode = 1;
    assert(aoe_graphic_select_frame(&graphic, 1, 2, &result));
    assert(result.slp_frame == 2 && result.mirror_horizontal == 1);
    assert(aoe_graphic_select_frame(&graphic, 0, 99, &result));
    assert(result.slp_frame == 0 && result.mirror_horizontal == 0);
    result = (AoeGraphicFrame){99, 99};
    assert(!aoe_graphic_select_frame(&graphic, 2, 0, &result));
    assert(result.slp_frame == 99 && result.mirror_horizontal == 99);
}

static int real_dat(const char *path) {
    DatBlob blob = {0};
    DatIntro intro = {0};
    DatSounds sounds = {0};
    DatGraphics graphics = {0};
    if (!dat_load(path, &blob)) return 0;
    int ok = dat_parse_intro(&blob, &intro) &&
             dat_parse_sounds(&blob, &intro, &sounds) &&
             dat_parse_graphics(&blob, &sounds, &graphics);
    size_t checked = 0, blank = 0, failed = 0;
    if (ok) {
        for (uint16_t i = 0; i < graphics.count; ++i) {
            const DatGraphic *graphic = &graphics.graphics[i];
            if (!graphic->present) continue;
            if (!graphic->frame_count || !graphic->angle_count) {
                ++blank;
                continue;
            }
            for (uint16_t angle = 0; angle < graphic->angle_count; ++angle) {
                AoeGraphicFrame result;
                if (!aoe_graphic_select_frame(graphic, angle, 0, &result)) {
                    fprintf(stderr, "%s: selector failed for graphic %u angle %u\n",
                            path, i, angle);
                    ++failed;
                } else {
                    ++checked;
                }
            }
        }
        printf("%s: %zu angle selections, %zu blank graphics, %zu failures\n",
               path, checked, blank, failed);
        ok = failed == 0;
    }
    dat_free_graphics(&graphics);
    dat_free_sounds(&sounds);
    dat_free_intro(&intro);
    dat_free(&blob);
    return ok;
}

int main(int argc, char **argv) {
    synthetic();
    for (int i = 1; i < argc; ++i)
        if (!real_dat(argv[i])) return 1;
    return 0;
}
