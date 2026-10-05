#include "music_manager.h"

#include <stdio.h>
#include <string.h>

void aoe_music_init_ror(AoeMusicRor *m, const AoeMusicOpsRor *ops, uint8_t source,
                        uint32_t window, uint32_t notify, void *sound, const char *dir) {
    memset(m, 0, sizeof *m);
    m->ops = ops;
    m->source = source;
    m->sound = sound;
    m->notify = notify;
    m->window = window;
    m->mixer_volume = 1;
    m->fade_ms_len = 2000;
    if (dir) snprintf(m->dir, sizeof m->dir, "%s", dir);
    aoe_music_open_ror(m);
}

static int32_t mixer_to_volume(const AoeMusicRor *m) {
    return (int32_t)((uint32_t)(m->mixer_value * 10000) /
                     (uint32_t)(m->mixer_max - m->mixer_min)) - 10000;
}

int aoe_music_mixer_open_ror(AoeMusicRor *m) {
    if (!m->ops->mixer_open ||
        !m->ops->mixer_open(m->ops->context, m->source, &m->mixer_min, &m->mixer_max,
                            &m->mixer_value))
        return 0;
    m->mixer = 1;
    return 1;
}

void aoe_music_mixer_close_ror(AoeMusicRor *m) {
    if (!m->mixer) return;
    m->ops->mixer_close(m->ops->context);
    m->mixer = 0;
}

static int32_t count_files(AoeMusicRor *m, const char *fmt) {
    char path[0x108];
    int32_t n = 0;
    for (;;) {
        snprintf(path, sizeof path, fmt, m->dir, n + 1);
        if (!m->ops->exists(m->ops->context, path)) break;
        ++n;
    }
    return n;
}

int aoe_music_open_ror(AoeMusicRor *m) {
    const AoeMusicOpsRor *ops = m->ops;
    if (m->source == 1) {
        int32_t tracks = -1;
        if (!ops->open_cd(ops->context, &m->device, &tracks)) return 0;
        m->open = 1;
        if (tracks != -1) m->tracks = tracks;
        if (aoe_music_mixer_open_ror(m)) m->mixer_volume = m->volume = mixer_to_volume(m);
        return 1;
    }
    if (m->source == 2) {
        if (aoe_music_mixer_open_ror(m)) m->mixer_volume = m->volume = mixer_to_volume(m);
        m->tracks = count_files(m, "%sxmusic%d.mid");
    } else if (m->source == 3) {
        m->tracks = count_files(m, "%smusic%d.wav");
        m->volume = 0;
    } else {
        return 0;
    }
    m->open = 1;
    return 1;
}

void aoe_music_close_ror(AoeMusicRor *m) {
    if (!m->open) return;
    aoe_music_reset_ror(m);
    if (m->source == 1 || m->source == 2) {
        if (m->mixer_volume != 1) {
            aoe_music_set_volume_ror(m, m->mixer_volume, 0);
            m->mixer_volume = 1;
        }
        aoe_music_mixer_close_ror(m);
        if (m->source == 1) {
            m->ops->close(m->ops->context, m->device);
            m->device = 0;
        }
    }
    m->open = 0;
    m->tracks = 0;
}

int aoe_music_set_source_ror(AoeMusicRor *m, uint8_t source) {
    if (source == m->source) return m->open;
    aoe_music_close_ror(m);
    m->source = source;
    return aoe_music_open_ror(m);
}

void aoe_music_set_volume_ror(AoeMusicRor *m, int32_t v, int keep) {
    if (v < -10000) v = -10000;
    else if (v > 0) v = 0;
    if (keep) m->volume = v;
    if (!m->open) return;
    if (m->source == 1 || m->source == 2) {
        if (!m->mixer) return;
        m->mixer_value =
            (int32_t)((uint32_t)((m->mixer_max - m->mixer_min) * (v + 10000)) / 10000u);
        m->ops->mixer_set(m->ops->context, m->mixer_value);
    } else if (m->source == 3 && m->sound) {
        m->ops->wav_volume(m->ops->context, v);
    }
}

static int fade_first(AoeMusicRor *m) {
    if (m->playing && m->fade_on && m->fade_ms_len) {
        aoe_music_fade_begin_ror(m);
        return 1;
    }
    return 0;
}

int aoe_music_play_track_ror(AoeMusicRor *m, int32_t track, int32_t loop, int32_t seek) {
    m->file[0] = '\0';
    m->from = m->to = track;
    m->start_track = 0;
    m->seek = seek;
    m->loop = loop;
    if (fade_first(m)) return 1;
    return aoe_music_start_ror(m, track, track, NULL, 0, seek);
}

int aoe_music_play_file_ror(AoeMusicRor *m, const char *name, int32_t loop, int32_t seek) {
    if (m->source == 1) return 0;
    snprintf(m->file, sizeof m->file, "%s", name);
    m->from = m->to = m->start_track = 0;
    m->seek = seek;
    m->loop = loop;
    if (fade_first(m)) return 1;
    return aoe_music_start_ror(m, 0, 0, m->file, 0, seek);
}

int aoe_music_play_range_ror(AoeMusicRor *m, int32_t from, int32_t to, int32_t loop,
                             int32_t start_track, int32_t seek) {
    m->loop = loop;
    m->file[0] = '\0';
    m->from = from;
    m->to = to;
    m->start_track = start_track;
    m->seek = seek;
    if (m->tracks == 0) return 0;
    if (m->tracks < from) m->from = m->tracks;
    if (to < m->from) m->to = m->from;
    else if (m->tracks < to) m->to = m->tracks;
    if (fade_first(m)) return 1;
    return aoe_music_start_ror(m, m->from, m->to, NULL, start_track, seek);
}

int aoe_music_start_ror(AoeMusicRor *m, int32_t from, int32_t to, const char *file,
                        int32_t start_track, int32_t seek) {
    const AoeMusicOpsRor *ops = m->ops;
    char path[0x110];
    if (!m->open) return 0;
    if (m->playing) aoe_music_stop_ror(m);
    if (!file || !*file) {
        if (m->tracks == 0 || m->tracks < from || from == 0) return 0;
        m->track = start_track == 0 ? from : start_track;
    }
    if (m->source == 1) {
        int has_to = from < m->tracks && to < m->tracks;
        if (ops->play(ops->context, m->device, (uint8_t)m->track, (uint8_t)(to + 1), has_to,
                      seek, m->notify)) {
            m->playing = 1;
            ++m->plays;
        }
    } else if (m->source == 2) {
        if (!file || !*file) {
            snprintf(path, sizeof path, "%sxmusic%d.mid", m->dir, m->track);
        } else {
            snprintf(path, sizeof path, "%s%s", m->dir, file);
            if (!strchr(path, '.')) strncat(path, ".mid", sizeof path - strlen(path) - 1);
        }
        if (ops->exists(ops->context, path) && ops->open_midi(ops->context, path, &m->device)) {
            if (ops->play(ops->context, m->device, 0, 0, 0, seek, m->notify)) {
                m->playing = 1;
                ++m->plays;
            }
        }
    } else if (m->source == 3 && m->sound) {
        if (!file || !*file)
            snprintf(path, sizeof path, "%smusic%d.wav", m->dir, m->track);
        else
            snprintf(path, sizeof path, "%s%s", m->dir, file);
        if (!ops->wav_play(ops->context, path, m->volume)) return 0;
        m->playing = 1;
    }
    if (!m->playing) return 0;
    m->paused = 0;
    m->pause_again = 0;
    m->field_260 = 0;
    return 1;
}

int aoe_music_pause_ror(AoeMusicRor *m) {
    if (!m->open || !m->playing) return 0;
    if (m->paused) {
        m->pause_again = 0;
        return 1;
    }
    m->paused = 1;
    if (m->pause_again) {
        m->pause_again = 0;
        return 1;
    }
    if (m->source == 1 || m->source == 2) m->ops->pause(m->ops->context, m->device);
    else if (m->source == 3 && m->sound) m->ops->wav_pause(m->ops->context);
    return 1;
}

int aoe_music_resume_ror(AoeMusicRor *m) {
    if (!m->open || !m->playing || !m->paused) return 0;
    m->paused = 0;
    m->pause_again = 0;
    if (m->fading) {
        aoe_music_fade_end_ror(m);
        return 1;
    }
    if (m->source == 1 || m->source == 2) {
        if (m->ops->resume(m->ops->context, m->device, m->notify)) ++m->plays;
    } else if (m->source == 3 && m->sound) {
        m->ops->wav_resume(m->ops->context);
    }
    return 1;
}

int aoe_music_stop_ror(AoeMusicRor *m) {
    if (!m->playing) return 1;
    if (m->source == 1) {
        m->ops->stop(m->ops->context, m->device);
    } else if (m->source == 2) {
        m->ops->stop(m->ops->context, m->device);
        m->ops->close(m->ops->context, m->device);
        m->device = 0;
    } else if (m->source == 3 && m->sound) {
        m->ops->wav_stop(m->ops->context);
    }
    m->playing = 0;
    m->paused = 0;
    m->pause_again = 0;
    return 1;
}

void aoe_music_reset_ror(AoeMusicRor *m) {
    m->loop = 0;
    m->from = m->to = 0;
    m->file[0] = '\0';
    m->fading = 0;
    aoe_music_stop_ror(m);
}

void aoe_music_fade_begin_ror(AoeMusicRor *m) {
    if (m->fading) return;
    m->fade_volume = m->volume;
    m->fade_ms = m->ops->now_ms(m->ops->context);
    m->fading = 1;
}

void aoe_music_fade_tick_ror(AoeMusicRor *m) {
    uint32_t now;
    float elapsed, len;
    int32_t threshold;
    if (m->paused) return;
    now = m->ops->now_ms(m->ops->context);
    elapsed = (float)(now - m->fade_ms);
    if (m->fade_volume <= -10000 && m->playing) aoe_music_stop_ror(m);
    len = (float)m->fade_ms_len;
    m->fade_volume += (int32_t)(elapsed / len * -10000.0f);
    threshold = (int32_t)(250.0f / len * 10000.0f);
    if (m->fade_volume > -10000 - threshold) {
        aoe_music_set_volume_ror(m, m->fade_volume, 0);
        m->fade_ms = m->ops->now_ms(m->ops->context);
        return;
    }
    if (m->playing) aoe_music_stop_ror(m);
    if (m->fading) aoe_music_fade_end_ror(m);
}

void aoe_music_fade_end_ror(AoeMusicRor *m) {
    m->fading = 0;
    aoe_music_set_volume_ror(m, m->volume, 0);
    if ((m->from > 0 && m->to > 0) || m->file[0])
        aoe_music_start_ror(m, m->from, m->to, m->file, m->start_track, m->seek);
}

void aoe_music_track_done_ror(AoeMusicRor *m) {
    if (m->source == 1) {
        if (m->fading || !m->loop) return;
    } else if (m->source == 2 || m->source == 3) {
        if (m->fading) return;
        if (!m->file[0] && m->track < m->to) {
            aoe_music_start_ror(m, m->track + 1, m->track + 1, NULL, 0, 0);
            return;
        }
        if (!m->loop) return;
        if (m->file[0]) {
            aoe_music_start_ror(m, 0, 0, m->file, 0, 0);
            return;
        }
    } else {
        return;
    }
    aoe_music_start_ror(m, m->from, m->to, NULL, 0, 0);
}

int aoe_music_state_ror(const AoeMusicRor *m, uint8_t *source, int32_t *from, int32_t *to,
                        int32_t *track, char *file, int32_t *loop, int32_t *position) {
    *source = m->source;
    *from = *to = *track = 0;
    file[0] = '\0';
    *loop = 0;
    *position = 0;
    if (!m->open) return 0;
    if (m->playing) {
        int32_t p;
        *from = m->from;
        *to = m->to;
        *track = m->track;
        strcpy(file, m->file);
        *loop = m->loop;
        if ((m->source == 1 || m->source == 2) && m->ops->position(m->ops->context, m->device, &p))
            *position = p;
    }
    return 1;
}

void aoe_music_suspend_ror(AoeMusicRor *m, AoeMusicSavedRor *saved) {
    if (!m) return;
    aoe_music_state_ror(m, &saved->source, &saved->from, &saved->to, &saved->track, saved->file,
                        &saved->loop, &saved->position);
    aoe_music_reset_ror(m);
}

int aoe_music_resume_saved_ror(AoeMusicRor *m, const AoeMusicSavedRor *saved) {
    if (!m) return 1;
    if (saved->from != 0) {
        if (saved->from == saved->to) {
            aoe_music_play_track_ror(m, saved->from, saved->loop, saved->position);
            return 1;
        }
        if (saved->to != 0) {
            aoe_music_play_range_ror(m, saved->from, saved->to, saved->loop, saved->track,
                                     saved->position);
            return 1;
        }
    }
    if (saved->file[0] != '\0') aoe_music_play_file_ror(m, saved->file, saved->loop, saved->position);
    return 1;
}
