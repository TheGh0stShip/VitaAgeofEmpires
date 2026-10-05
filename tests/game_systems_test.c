/* Patched RoR 1.0a base game subsystem steps (0x00416240..0x0041c1b0). */
#include "game_systems.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char tok[16];
static char loaded[8][32];
static int nloaded, preloaded;
static void *shape(void *c, const char *n, int32_t id) {
    (void)c;
    snprintf(loaded[nloaded++], 32, "%s:%d", n, id);
    return strcmp(n, "waypoint.shp") == 0 ? NULL : &tok[nloaded];
}
static void preload(void *c, void *s) { (void)c; (void)s; ++preloaded; }

static int start_ok = 1, destroyed, s58;
static int32_t vol_set = 12345, reg_vol = -1, saved_vol;
static void *create(void *c) { (void)c; return &tok[0]; }
static int start(void *c, void *s) { (void)c; (void)s; return start_ok; }
static void destroy(void *c, void *s) { (void)c; (void)s; ++destroyed; }
static void set_volume(void *c, void *s, int32_t v) { (void)c; (void)s; vol_set = v; }
static int32_t get_volume(void *c, void *s) { (void)c; (void)s; return -700; }
static int32_t rget(void *c, const char *n) { (void)c; return strcmp(n, "Sound Volume") == 0 ? reg_vol : -1; }
static char rset_name[32];
static void rset(void *c, const char *n, int32_t v) { (void)c; snprintf(rset_name, 32, "%s", n); saved_vol = v; }
static void f58(void *c) { (void)c; ++s58; }

static int comm_calls, c2e0, c310, psounds, freed, music_stops, b8;
static void comm_sound(void *c, void *s) { (void)c; (void)s; ++comm_calls; }
static void k2e0(void *c) { (void)c; ++c2e0; }
static void k310(void *c) { (void)c; ++c310; }
static void psound(void *c, int32_t i, void *s) { (void)c; (void)i; (void)s; ++psounds; }
static void free_sound(void *c, void *s) { (void)c; (void)s; ++freed; }
static void stop_music(void *c) { (void)c; ++music_stops; }
static int ok(void *c) { (void)c; return 1; }
static void fb8(void *c) { (void)c; ++b8; }

static int32_t mouse_reg = -1, mouse_set = -9;
static int32_t mget(void *c, const char *n) { (void)c; (void)n; return mouse_reg; }
static void mset(void *c, const char *n, int32_t v) { (void)c; (void)n; mouse_set = v; }
static int created_custom = -1, setup_args[3], afters;
static void *pcreate(void *c, int custom) { (void)c; created_custom = custom; return &tok[1]; }
static int psetup(void *c, void *p, int32_t cm, int32_t id, int32_t fr) { (void)c; (void)p; setup_args[0] = cm; setup_args[1] = id; setup_args[2] = fr; return 1; }
static void pafter(void *c) { (void)c; ++afters; }

static int attached;
static void *screate(void *c, const char *n) { (void)c; assert(strcmp(n, "Diamond Map Save Area") == 0); return &tok[2]; }
static int sattach(void *c, void *s, int32_t w, int32_t h) { (void)c; (void)s; ++attached; return w == 280 && h == 140; }

int main(void) {
    AoeGameResourceOpsRor r = {0, shape, shape, preload};
    void *table[3];
    int16_t count = 0;
    aoe_game_load_common_shapes_ror(&r, table, &count);
    assert(count == 3 && strcmp(loaded[0], "groupnum.shp:50403") == 0 && table[1] == NULL &&
           strcmp(loaded[2], "moveto.shp:50405") == 0);
    nloaded = 0;
    aoe_game_load_common_sounds_ror(&r, table, &count);
    assert(strcmp(loaded[0], "button1.wav:50300") == 0 && strcmp(loaded[2], "chatrcvd.wav:50302") == 0);
    assert(preloaded == 3);

    /* Sound: missing volume gives 0; a failed start drops the system
       and skips +0x58. */
    {
        AoeGameSoundOpsRor s = {0, 1, create, start, destroy, set_volume, get_volume, rget, rset, f58};
        void *sys = NULL;
        assert(aoe_game_start_sound_ror(&s, &sys) && sys && vol_set == 0 && s58 == 1);
        reg_vol = 300;
        aoe_game_start_sound_ror(&s, &sys);
        assert(vol_set == -300);
        start_ok = 0;
        assert(aoe_game_start_sound_ror(&s, &sys) && !sys && destroyed == 1 && s58 == 2);
        start_ok = 1;
        s.sound_on = 0;
        sys = NULL;
        assert(aoe_game_start_sound_ror(&s, &sys) && !sys && s58 == 3);
        s.sound_on = 1;
        /* Off: everything detached and freed, the volume saved. */
        {
            AoeGameSoundCycleOpsRor cyc = {0, 1, comm_sound, k2e0, k310, 2, psound, free_sound, stop_music, ok, ok, fb8};
            void **sounds = calloc(3, sizeof(void *));
            int16_t n = 3;
            sounds[0] = &tok[3];
            sounds[2] = &tok[4];
            sys = &tok[0];
            aoe_game_sound_off_ror(&cyc, &s, &sys, &sounds, &n);
            assert(comm_calls == 1 && c2e0 == 1 && psounds == 2 && freed == 2 && !sounds);
            assert(music_stops == 1 && !sys && saved_vol == 700 && strcmp(rset_name, "Sound Volume") == 0);
            assert(aoe_game_sound_on_ror(&cyc, &sys) && psounds == 4 && c310 == 1 && comm_calls == 2 && b8 == 1);
        }
    }
    /* Pointer. */
    {
        AoeGamePointerOpsRor p = {0, mget, mset, pcreate, psetup, pafter};
        int32_t custom = 1;
        void *ptr;
        assert(aoe_game_setup_pointer_ror(&p, &custom, &ptr) && mouse_set == 0 && created_custom == 0);
        assert(setup_args[0] == 1 && setup_args[1] == 51000 && setup_args[2] == 10 && afters == 1);
        mouse_reg = 2;
        assert(aoe_game_setup_pointer_ror(&p, &custom, &ptr) && custom == 0);
        mouse_reg = 1;
        aoe_game_setup_pointer_ror(&p, &custom, &ptr);
        assert(created_custom == 1);
    }
    /* Saved-game versions. */
    assert(aoe_game_saved_version_ok_ror("1.12") && aoe_game_saved_version_ok_ror("1.03"));
    assert(!aoe_game_saved_version_ok_ror("1.02") && !aoe_game_saved_version_ok_ror("1.13"));
    /* Map save area. */
    {
        AoeGameSurfaceOpsRor so = {0, screate, sattach, destroy};
        void *area;
        assert(aoe_game_map_save_area_ror(&so, &area) && area && attached == 1);
    }
    {
        char out[64];
        aoe_game_no_ai_caption_ror(out, sizeof out, 3);
        assert(strcmp(out, "No AI Information for Player #3.") == 0);
    }
    assert(strstr(aoe_game_global_key_ror, "Age of Empires Expansion\\1.0"));
    puts("game systems ok");
    return 0;
}
