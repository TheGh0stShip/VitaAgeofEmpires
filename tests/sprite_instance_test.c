#include <stdlib.h>
#include "sprite_instance.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void synthetic(void) {
    DatGraphic graphic = {.present = 1, .frame_count = 4,
                          .frame_duration = 0.5f};
    AoeSpriteInstance sprite = {0};
    assert(aoe_sprite_instance_init(&sprite, &graphic, NULL, 3, -4));
    assert(sprite.kind == 1 && sprite.graphic == &graphic);
    assert(sprite.offset_x == 3 && sprite.offset_y == -4);
    assert(aoe_sprite_instance_tick(&sprite, 2.0f, 1.0f));
    assert(sprite.animation.frame == 0);

    graphic.sequence_type = 1;
    assert(aoe_sprite_instance_init(&sprite, &graphic, NULL, 0, 0));
    assert(sprite.kind == 2 && sprite.animation.period == 0.5f);
    assert(sprite.animation.run_state == 1);
    assert(aoe_sprite_instance_tick(&sprite, 0.5f, 1.0f));
    assert(sprite.animation.frame == 1);
    graphic.angle_count = 8;
    AoeGraphicFrame selected;
    assert(aoe_sprite_instance_frame(&sprite, 2, &selected));
    assert(selected.slp_frame == 9 && selected.mirror_horizontal == 0);

    graphic.sequence_type = 5;
    AoeRng rng;
    aoe_rng_seed(&rng, 1);
    /* memcpy, not assignment: assignment leaves padding unspecified. */
    AoeSpriteInstance before;
    memcpy(&before, &sprite, sizeof(before));
    assert(!aoe_sprite_instance_init(&sprite, &graphic, NULL, 0, 0));
    assert(memcmp(&sprite, &before, sizeof(sprite)) == 0);
    assert(aoe_sprite_instance_init(&sprite, &graphic, &rng, 0, 0));
    assert(sprite.kind == 2 && sprite.animation.frame >= 0);
    assert(sprite.animation.frame < 4);
    assert(sprite.animation.previous_frame == sprite.animation.frame);
    assert(rng.state != 1);

    graphic.frame_duration = 0.0f;
    graphic.frame_count = 1;
    assert(aoe_sprite_instance_init(&sprite, &graphic, &rng, 0, 0));
    assert(sprite.animation.frame == 0 && sprite.animation.period == 0.001f);
    graphic.frame_count = 4;
    assert(!aoe_sprite_instance_init(&sprite, &graphic, &rng, 0, 0));
    graphic.sequence_type = 1;
    assert(aoe_sprite_instance_init(&sprite, &graphic, NULL, 0, 0));
    assert(sprite.animation.period == 0.001f);
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
    if (ok) {
        size_t static_count = 0, animated_count = 0, invalid_count = 0;
        AoeRng rng;
        aoe_rng_seed(&rng, 1);
        for (uint16_t i = 0; i < graphics.count; ++i) {
            DatGraphic *graphic = &graphics.graphics[i];
            if (!graphic->present) continue;
            AoeSpriteInstance sprite;
            if (!aoe_sprite_instance_init(&sprite, graphic, &rng, 0, 0)) {
                fprintf(stderr, "%s: unsupported graphic %u (%s), flags %u, "
                        "frames %u, duration %g, replay %g\n",
                        path, i, graphic->name, graphic->sequence_type,
                        graphic->frame_count, graphic->frame_duration,
                        graphic->replay_delay);
                ++invalid_count;
                continue;
            }
            if (!aoe_sprite_instance_tick(&sprite, 0.016f, 1.0f)) {
                fprintf(stderr, "%s: cannot tick graphic %u (%s)\n",
                        path, i, graphic->name);
                ++invalid_count;
                continue;
            }
            if (sprite.kind == 1) ++static_count;
            else ++animated_count;
        }
        printf("%s: %zu static, %zu animated, %zu unsupported\n",
               path, static_count, animated_count, invalid_count);
        ok = invalid_count == 0 &&
             static_count + animated_count == graphics.present_count;
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
    {
        /* Frame-reached / looped queries (0x00416180 / 0x004161b0). */
        AoeSpriteInstance inst;
        memset(&inst, 0, sizeof(inst));
        inst.kind = 2;
        inst.animation.frame = 5;
        inst.animation.previous_frame = 3;
        assert(!aoe_sprite_instance_reached_ror(&inst, 4));
        inst.animation.frame_changed = 1;
        assert(aoe_sprite_instance_reached_ror(&inst, 4) &&
               aoe_sprite_instance_reached_ror(&inst, 5) &&
               !aoe_sprite_instance_reached_ror(&inst, 3) &&
               !aoe_sprite_instance_reached_ror(&inst, 6));
        inst.animation.looped = 1;
        assert(aoe_sprite_instance_reached_ror(&inst, 9) &&
               aoe_sprite_instance_looped_ror(&inst));
        inst.kind = 1;
        assert(!aoe_sprite_instance_reached_ror(&inst, 9) &&
               !aoe_sprite_instance_looped_ror(&inst));
    }
    {
        /* 0x00415d60: bit 0 frees the instance. */
        AoeSpriteInstance stack_inst, *heap = malloc(sizeof *heap);
        assert(heap && aoe_sprite_instance_delete_ror(&stack_inst, 0) == &stack_inst);
        assert(aoe_sprite_instance_delete_ror(heap, 1) == heap);
    }
    return 0;
}
