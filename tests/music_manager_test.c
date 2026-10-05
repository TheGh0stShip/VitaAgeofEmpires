/* Patched RoR 1.0a music manager (0x0044fad0..0x00450cc0). */
#include "music_manager.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int cd_tracks = 12, plays, stops, pauses, resumes, closes, mixer_sets;
static int32_t last_from, last_to, last_has_to, mixer_value_set;
static uint32_t now;
static int open_cd(void *c, uint32_t *d, int32_t *t) { (void)c; *d = 7; *t = cd_tracks; return 1; }
static int open_midi(void *c, const char *p, uint32_t *d) { (void)c; (void)p; *d = 9; return 1; }
static int play(void *c, uint32_t d, int32_t f, int32_t t, int h, int32_t s, uint32_t n) {
    (void)c; (void)d; (void)s; (void)n;
    last_from = f, last_to = t, last_has_to = h;
    ++plays;
    return 1;
}
static void stop(void *c, uint32_t d) { (void)c; (void)d; ++stops; }
static void pause_(void *c, uint32_t d) { (void)c; (void)d; ++pauses; }
static int resume(void *c, uint32_t d, uint32_t n) { (void)c; (void)d; (void)n; ++resumes; return 1; }
static void close_(void *c, uint32_t d) { (void)c; (void)d; ++closes; }
static int position(void *c, uint32_t d, int32_t *p) { (void)c; (void)d; *p = 3; return 1; }
static int mopen(void *c, int s, int32_t *mn, int32_t *mx, int32_t *v) { (void)c; (void)s; *mn = 0; *mx = 0xffff; *v = 0xffff / 2; return 1; }
static void mclose(void *c) { (void)c; }
static void mset(void *c, int32_t v) { (void)c; ++mixer_sets; mixer_value_set = v; }
static int midi_files = 3;
static char last_path[300];
static int exists(void *c, const char *p) {
    int n;
    (void)c;
    snprintf(last_path, sizeof last_path, "%s", p);
    if (sscanf(p, "dir/xmusic%d.mid", &n) == 1) return n <= midi_files;
    if (sscanf(p, "dir/music%d.wav", &n) == 1) return n <= 2;
    return strcmp(p, "dir/theme.mid") == 0;
}
static int wav_plays;
static int wav_play(void *c, const char *p, int32_t v) { (void)c; (void)v; snprintf(last_path, sizeof last_path, "%s", p); ++wav_plays; return 1; }
static void wv(void *c, int32_t v) { (void)c; (void)v; }
static void wp(void *c) { (void)c; }
static uint32_t clock_(void *c) { (void)c; return now; }

int main(void) {
    AoeMusicOpsRor ops = {0, open_cd, open_midi, play, stop, pause_, resume, close_, position,
                          mopen, mclose, mset, exists, wav_play, wv, wp, wp, wp, clock_};
    static AoeMusicRor m;
    /* CD: twelve tracks, the mixer's half volume as the saved volume. */
    aoe_music_init_ror(&m, &ops, 1, 0, 0, NULL, "dir/");
    assert(m.open && m.tracks == 12 && m.device == 7 && m.fade_ms_len == 2000);
    assert(m.volume == m.mixer_volume && m.volume < -4900 && m.volume > -5100);
    /* A range plays from..to (the "to" byte is to + 1). */
    assert(aoe_music_play_range_ror(&m, 5, 14, 1, 0, 0) && m.to == 12);
    assert(last_from == 5 && last_has_to == 0); /* 12 is not below 12 */
    assert(aoe_music_play_range_ror(&m, 3, 4, 0, 0, 0) && last_has_to && last_to == 5 && m.plays == 2);
    assert(aoe_music_play_range_ror(&m, 20, 1, 0, 0, 0) && m.from == 12 && m.to == 12);
    assert(!aoe_music_play_file_ror(&m, "x", 0, 0)); /* no files on the CD */
    /* Pause twice, resume. */
    assert(aoe_music_pause_ror(&m) && pauses == 1 && aoe_music_pause_ror(&m) && pauses == 1);
    assert(aoe_music_resume_ror(&m) && resumes == 1 && !m.paused);
    /* Track done loops only with loop. */
    m.loop = 0;
    plays = 0;
    aoe_music_track_done_ror(&m);
    assert(plays == 0);
    m.loop = 1;
    aoe_music_track_done_ror(&m);
    assert(plays == 1);
    /* Volume: clamped, through the mixer. */
    aoe_music_set_volume_ror(&m, 500, 1);
    assert(m.volume == 0 && mixer_value_set == 0xffff);
    aoe_music_set_volume_ror(&m, -20000, 0);
    assert(m.volume == 0 && mixer_value_set == 0);
    /* Fade: begins on a new request while playing, ticks down, then
       starts the queued track. */
    m.fade_on = 1;
    now = 1000;
    stops = 0;
    assert(aoe_music_play_track_ror(&m, 2, 0, 0) && m.fading && m.fade_volume == 0);
    now = 2000;
    aoe_music_fade_tick_ror(&m);
    assert(m.fade_volume == -5000 && m.fading);
    now = 3000;
    aoe_music_fade_tick_ror(&m); /* -10000: still above -10000 - 1250 */
    assert(m.fade_volume == -10000 && m.fading && stops == 0);
    plays = 0;
    now = 4000;
    aoe_music_fade_tick_ror(&m); /* silent: stopped, then the queued track */
    assert(!m.fading && stops == 1 && plays == 1 && last_from == 2);
    /* State. */
    {
        uint8_t src;
        int32_t f, t, tr, lp, pos;
        char file[300];
        assert(aoe_music_state_ror(&m, &src, &f, &t, &tr, file, &lp, &pos));
        assert(src == 1 && f == 2 && tr == 2 && pos == 3);
    }
    aoe_music_close_ror(&m);
    assert(!m.open && closes == 1 && m.mixer_volume == 1);

    /* MIDI: three numbered files; the next track follows when one ends. */
    aoe_music_init_ror(&m, &ops, 2, 0, 0, NULL, "dir/");
    assert(m.tracks == 3);
    assert(aoe_music_play_range_ror(&m, 1, 3, 0, 0, 0) && m.track == 1 && strcmp(last_path, "dir/xmusic1.mid") == 0);
    aoe_music_track_done_ror(&m);
    assert(m.track == 2 && strcmp(last_path, "dir/xmusic2.mid") == 0);
    assert(aoe_music_play_file_ror(&m, "theme", 1, 0) && strcmp(last_path, "dir/theme.mid") == 0);
    /* WAV: two numbered files through the sound system. */
    {
        int snd;
        assert(aoe_music_set_source_ror(&m, 3) && m.tracks == 2 && m.volume == 0);
        m.sound = &snd;
        assert(aoe_music_play_track_ror(&m, 2, 0, 0) && strcmp(last_path, "dir/music2.wav") == 0 && wav_plays == 1);
    }
    puts("music manager ok");
    return 0;
}
