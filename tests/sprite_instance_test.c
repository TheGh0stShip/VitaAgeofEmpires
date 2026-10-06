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

    graphic.sequence_flags = 1;
    assert(aoe_sprite_instance_init(&sprite, &graphic, NULL, 0, 0));
    assert(sprite.kind == 2 && sprite.animation.period == 0.5f);
    assert(sprite.animation.run_state == 1);
    assert(aoe_sprite_instance_tick(&sprite, 0.5f, 1.0f));
    assert(sprite.animation.frame == 1);
    graphic.angle_count = 8;
    AoeGraphicFrame selected;
    assert(aoe_sprite_instance_frame(&sprite, 2, &selected));
    assert(selected.slp_frame == 9 && selected.mirror_horizontal == 0);

    graphic.sequence_flags = 5;
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
    graphic.sequence_flags = 1;
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
                        path, i, graphic->name, graphic->sequence_flags,
                        graphic->frame_count, graphic->frame_duration,
                        graphic->loop_delay);
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

static int32_t drawn_frame, drawn_x, drawn_y, played;
static void part_draw(void *c, const DatGraphic *g, int32_t a, int32_t f,
                      int32_t x, int32_t y) {
    (void)c; (void)g; (void)a;
    drawn_frame = f; drawn_x = x; drawn_y = y;
}
static void play_sound(void *c, int16_t id) { (void)c; (void)id; ++played; }

static void draw_slots(void) {
    DatGraphic g = {.present = 1, .frame_count = 4, .angle_count = 1,
                    .layer = 20, .sound_id = 7};
    AoeSpriteInstance s;
    memset(&s, 0, sizeof(s));
    s.graphic = &g; s.kind = 1; s.offset_x = 3; s.offset_y = -2;
    /* static: frame 0, offset added */
    assert(aoe_sprite_instance_draw_top_ror(&s, 0, 10, 20, part_draw, play_sound, NULL) == 1);
    assert(drawn_frame == 0 && drawn_x == 13 && drawn_y == 18 && played == 0);
    assert(aoe_sprite_instance_draw_lower_ror(&s, 0, 10, 20, 0, part_draw, NULL) == 0);
    /* animated: current frame, no offset; changed frame plays sounds once */
    s.kind = 2; s.animation.frame = 1; s.animation.previous_frame = 0;
    s.animation.frame_changed = 1;
    assert(aoe_sprite_instance_draw_top_ror(&s, 0, 10, 20, part_draw, play_sound, NULL) == 1);
    assert(drawn_x == 10 && drawn_y == 20 && played == 1);
    assert(s.animation.frame_changed == 0);
    aoe_sprite_instance_draw_top_ror(&s, 0, 10, 20, part_draw, play_sound, NULL);
    assert(played == 1);
    g.layer = 5; s.animation.frame = 2;
    assert(aoe_sprite_instance_draw_lower_ror(&s, 0, 1, 1, 0, part_draw, NULL) == 1);
    assert(drawn_frame == 2);
    {
        int16_t x0, y0, x1, y1;
        g.box[0] = -1; g.box[1] = -1; g.box[2] = 1; g.box[3] = 1;
        assert(aoe_sprite_instance_bounds_ror(&s, 0, NULL, NULL, &x0, &y0, &x1, &y1));
        assert(x0 == -1 && y1 == 1);
    }
}

int main(int argc, char **argv) {
    synthetic();
    draw_slots();
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
