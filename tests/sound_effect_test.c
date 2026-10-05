/* Patched RoR 1.0a sound effects 0x004a1f10..0x004a2510. */
#include "sound_effect.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int waves, freed, played, unloaded, stopped, alt;
static int wave_tokens[16];
static void *create(void *c, void *sys, const AoeSoundVariantRor *v) {
    (void)c; (void)v;
    if (!sys) return NULL;
    return &wave_tokens[waves++ % 16];
}
static void free_wave(void *c, void *w) { (void)c; (void)w; ++freed; }
static int playing(void *c, void *w) { (void)c; return w == &wave_tokens[0]; }
static void stop(void *c, void *w) { (void)c; (void)w; ++stopped; }
static void unload(void *c, void *w) { (void)c; (void)w; ++unloaded; }
static void play(void *c, void *w, int a) { (void)c; (void)w; ++played; alt = a; }
static uint32_t clock_ms = 1000;
static uint32_t now(void *c) { (void)c; return clock_ms; }
static char log_text[64];
static void log_fn(void *c, const char *t) { (void)c; strncat(log_text, t, sizeof(log_text) - strlen(log_text) - 1); }

int main(void) {
    AoeSoundEffectRor e, f;
    AoeRng rng;
    int sys = 1;
    uint8_t buf[256];
    AoeOutcomeStream out, in;
    AoeSoundEffectOpsRor ops = {NULL, create, free_wave, playing, stop, unload,
                                play, now, log_fn, &rng};

    /* Text: two variants, 30% and 70%. */
    assert(aoe_sound_effect_load_text_ror(&e, "2\n 101 click 30\n 102 clack 70\n", 7));
    assert(e.id == 7 && e.count == 2 && e.cache_ms == 300000);
    assert(strcmp(e.items[0].name, "click.wav") == 0 && e.items[1].resource == 102 &&
           e.items[1].probability == 70 && !e.items[0].wave);
    /* No waves yet: playing does nothing (but still rolls). */
    aoe_rng_seed(&rng, 1);
    aoe_sound_effect_play_ror(&e, 0, &ops);
    assert(played == 0);
    aoe_sound_effect_attach_ror(&e, &sys, &ops);
    assert(waves == 2 && e.items[0].state == 2);

    /* The first roll of seed 1 is 41: 41 * 100 / 32767 = 0 -> the first
       variant (0 - 30 < 1). */
    aoe_rng_seed(&rng, 1);
    aoe_sound_effect_play_ror(&e, 1, &ops);
    assert(played == 1 && alt == 1 && e.items[0].state == 1 &&
           e.items[0].last_ms == 1000 && strcmp(log_text, "{click.wav}") == 0);
    assert(aoe_sound_effect_pick_ror(&e, &ops) != NULL);

    /* Idle past the cache time unloads it. */
    aoe_sound_effect_expire_ror(&e, 1000 + 300000, &ops);
    assert(unloaded == 0);
    aoe_sound_effect_expire_ror(&e, 1000 + 300001, &ops);
    assert(unloaded == 1 && e.items[0].state == 2);
    aoe_sound_effect_stop_ror(&e, &ops);
    assert(stopped == 1);

    /* Binary round trip: 13-byte names; waves made on load. */
    memset(&out, 0, sizeof(out));
    out.data = buf;
    out.size = sizeof(buf);
    aoe_sound_effect_save_ror(&e, &out);
    assert(!out.failed && out.pos == 2 + 2 + 2 + 4 + 2 * (13 + 4 + 2));
    memset(&in, 0, sizeof(in));
    in.data = buf;
    in.size = out.pos;
    assert(aoe_sound_effect_load_ror(&f, &in, &sys, &ops));
    assert(f.id == 7 && f.count == 2 && f.cache_ms == 300000 &&
           strcmp(f.items[1].name, "clack.wav") == 0 && f.items[1].wave &&
           f.items[1].state == 2);
    aoe_sound_effect_free_ror(&f, &ops);
    aoe_sound_effect_free_ror(&e, &ops);
    assert(freed == 4 && !e.items && e.count == 0);
    /* Without variants: nothing to pick. */
    assert(!aoe_sound_effect_pick_ror(&e, &ops));
    return 0;
}
