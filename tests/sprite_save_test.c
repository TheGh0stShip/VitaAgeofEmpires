/* Patched RoR 1.0a sprite-list save 0x00415540 / load 0x004153c0 and the
   sprite accessors 0x00415ac0, 0x00416150, 0x00416170, 0x00416220. */
#include "sprite_runtime.h"

#include <assert.h>

static DatGraphic table[3];

static int16_t id_of(void *c, const DatGraphic *g) { (void)c; return (int16_t)(g - table); }
static const DatGraphic *lookup(void *c, int16_t id) { (void)c; return id < 3 ? &table[id] : NULL; }

int main(void) {
    AoeRng rng;
    AoeSpriteRuntime a, b;
    uint8_t buf[256];
    AoeOutcomeStream out = {buf, sizeof buf, 0, 0}, in;
    AoeSpriteInstance *walk;
    const AoeSpriteInstance *w2;

    table[1] = (DatGraphic){.present = 1, .frame_count = 1};
    table[2] = (DatGraphic){.present = 1, .frame_count = 4, .sequence_type = 1,
                            .frame_duration = 0.5f};
    aoe_rng_seed(&rng, 1);
    assert(aoe_sprite_runtime_init(&a, &rng, NULL, NULL, NULL));
    assert(aoe_sprite_runtime_add(&a, &table[2], 60, 7, -3));
    assert(aoe_sprite_runtime_add(&a, &table[1], 10, 2, 5));
    walk = (AoeSpriteInstance *)aoe_sprite_runtime_at(&a, 1);
    assert(aoe_sprite_runtime_tick(&a, 0.6f, 1.0f));
    assert(walk->animation.frame == 1 && walk->animation.frame_changed);
    assert(aoe_sprite_instance_playing_ror(walk) &&
           !aoe_sprite_instance_playing_ror(aoe_sprite_runtime_at(&a, 0)));
    assert(aoe_sprite_instance_current_frame_ror(walk) == 1 &&
           aoe_sprite_instance_current_frame_ror(aoe_sprite_runtime_at(&a, 0)) == 0);

    /* Static: 1 + 2 + 4 + 4; animated adds 2 + 4 + 4 + 2 + 3; then 0. */
    assert(aoe_sprite_runtime_save_ror(&a, id_of, NULL, &out));
    assert(out.pos == 11 + 11 + 15 + 1);
    assert(buf[0] == 1 && buf[1] == 1 && buf[2] == 0 && buf[3] == 2 &&
           buf[11] == 2 && buf[12] == 2 && buf[37] == 0);

    /* Loading keeps the order and the animation state; priorities are lost. */
    assert(aoe_sprite_runtime_init(&b, &rng, NULL, NULL, NULL));
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    assert(aoe_sprite_runtime_load_ror(&b, lookup, NULL, &in));
    assert(aoe_sprite_list_count(&b.list) == 2 && in.pos == out.pos);
    w2 = aoe_sprite_runtime_at(&b, 1);
    assert(aoe_sprite_runtime_at(&b, 0)->graphic == &table[1] &&
           aoe_sprite_runtime_at(&b, 0)->offset_y == 5);
    assert(w2->graphic == &table[2] && w2->offset_x == 7 &&
           w2->animation.frame == 1 && w2->animation.previous_frame == 0 &&
           w2->animation.frame_changed == 1 && w2->animation.run_state == 1 &&
           w2->animation.elapsed == walk->animation.elapsed &&
           aoe_sprite_list_priority_at(&b.list, 1) == 0);
    aoe_sprite_runtime_clear(&b);

    /* An unknown kind byte is skipped; a short stream fails. */
    {
        uint8_t odd[] = {7, 9, 0};
        in = (AoeOutcomeStream){odd, sizeof odd, 0, 0};
        assert(aoe_sprite_runtime_load_ror(&b, lookup, NULL, &in) && in.pos == 3);
        in = (AoeOutcomeStream){buf, 5, 0, 0};
        assert(!aoe_sprite_runtime_load_ror(&b, lookup, NULL, &in));
        assert(aoe_sprite_list_count(&b.list) == 0);
    }

    /* Accessors. */
    aoe_sprite_instance_set_offset_ror(walk, 4, 9);
    assert(walk->offset_x == 4 && walk->offset_y == 9);
    aoe_sprite_instance_set_frame_ror(walk, 3);
    aoe_sprite_instance_restart_ror(walk);
    assert(walk->animation.frame == 0 && walk->animation.previous_frame == 3 &&
           walk->animation.frame_changed == 1);
    assert(aoe_sprite_instance_reached_ror(walk, 0) == 0);

    aoe_sprite_runtime_clear(&a);
    return 0;
}
