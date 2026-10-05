#include "chat_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- taunts ---- */

int aoe_taunt_make_waves_ror(AoeTauntPlayerRor *t, AoeSoundSystemRor *system) {
    int i;
    for (i = 0; i < 4; ++i) {
        t->waves[i] = malloc(sizeof *t->waves[i]);
        if (t->waves[i]) aoe_wave_init_ror(t->waves[i], system, "Silence", -1);
    }
    return 1;
}

void aoe_taunt_init_ror(AoeTauntPlayerRor *t, AoeSoundSystemRor *system) {
    t->prefix[0] = '\0';
    aoe_taunt_make_waves_ror(t, system);
    t->muted = 0;
}

void aoe_taunt_free_waves_ror(AoeTauntPlayerRor *t) {
    int i;
    for (i = 0; i < 4; ++i) {
        if (!t->waves[i]) continue;
        aoe_wave_free_thunk_ror(t->waves[i]);
        free(t->waves[i]);
        t->waves[i] = NULL;
    }
}

void aoe_taunt_free_waves_thunk_ror(AoeTauntPlayerRor *t) {
    aoe_taunt_free_waves_ror(t);
}

int32_t aoe_taunt_number_ror(const char *text) {
    int32_t n;
    if (*text == '!') ++text;
    n = atoi(text);
    if (n == 0) return 0;
    return n > 999 ? 0 : n;
}

int aoe_taunt_play_ror(AoeTauntPlayerRor *t, int32_t n) {
    char name[0x81 + 16];
    int i;
    if (t->muted || n == 0) return 0;
    snprintf(name, sizeof name, "%sTaunt%.3d.wav", t->prefix, (int)n);
    for (i = 0; i < 4; ++i)
        if (t->waves[i] && !aoe_wave_playing_ror(t->waves[i]))
            aoe_wave_unload_ror(t->waves[i]);
    for (i = 0; i < 4; ++i)
        if (t->waves[i] && !aoe_wave_playing_ror(t->waves[i])) break;
    if (i == 4) return 0;
    if (!aoe_wave_load_ror(t->waves[i], name, -1)) {
        aoe_wave_unload_ror(t->waves[i]);
        return 0;
    }
    return aoe_wave_play_ror(t->waves[i]);
}

int32_t aoe_taunt_line_ror(AoeTauntPlayerRor *t, const char *text,
                           int commands) {
    int32_t n;
    if (strncmp(text, "!MUTE", 5) == 0 && commands) t->muted = 1;
    if (strncmp(text, "!NOMUTE", 7) == 0 && commands) t->muted = 0;
    if (t->muted) return 0;
    n = aoe_taunt_number_ror(text);
    if (n) aoe_taunt_play_ror(t, n);
    return n;
}

/* ---- chat log ---- */

void aoe_chat_log_init_ror(AoeChatLogRor *c, void *window,
                           AoeSoundSystemRor *system) {
    int32_t i;
    c->taunts = malloc(sizeof *c->taunts);
    if (c->taunts) aoe_taunt_init_ror(c->taunts, system);
    c->window = window;
    for (i = 0; i < 51; ++i) c->lines[i] = NULL;
    c->last = 0;
    /* The original sets flags 1..9 only; the port also clears flag 0. */
    c->recipients[0] = 0;
    for (i = 0; i < 10; ++i) aoe_chat_log_set_recipient_ror(c, i, 0);
}

void aoe_chat_log_free_ror(AoeChatLogRor *c) {
    aoe_chat_log_clear_ror(c);
    if (c->taunts) {
        aoe_taunt_free_waves_thunk_ror(c->taunts);
        free(c->taunts);
        c->taunts = NULL;
    }
}

int aoe_chat_log_add_ror(AoeChatLogRor *c, const char *sender,
                         const char *text, int commands) {
    size_t n;
    if (!sender || !text) return 0;
    if (++c->last > 50) c->last = 0;
    free(c->lines[c->last]);
    n = strlen(sender) + strlen(text) + 3;
    c->lines[c->last] = calloc(n, 1);
    if (!c->lines[c->last]) return 0;
    snprintf(c->lines[c->last], n, "%s: %s", sender, text);
    if (c->notify) c->notify(c->window, c->last);
    if (c->taunts) aoe_taunt_line_ror(c->taunts, text, commands);
    return 1;
}

const char *aoe_chat_log_line_ror(const AoeChatLogRor *c, int32_t i) {
    return i >= 0 && i < 51 ? c->lines[i] : NULL;
}

void aoe_chat_log_clear_ror(AoeChatLogRor *c) {
    int32_t i;
    for (i = 0; i < 51; ++i) {
        free(c->lines[i]);
        c->lines[i] = NULL;
    }
    c->last = 0;
}

void aoe_chat_log_set_recipient_ror(AoeChatLogRor *c, int32_t i,
                                    int32_t value) {
    if (i > 0 && i < 10) c->recipients[i] = value;
}

int32_t aoe_chat_log_recipient_ror(const AoeChatLogRor *c, int32_t i) {
    return i >= 0 && i < 10 ? c->recipients[i] : 0;
}

void aoe_chat_log_free_waves_ror(AoeChatLogRor *c) {
    if (c->taunts) aoe_taunt_free_waves_ror(c->taunts);
}

int aoe_chat_log_make_waves_ror(AoeChatLogRor *c, AoeSoundSystemRor *system) {
    return c->taunts ? aoe_taunt_make_waves_ror(c->taunts, system) : 1;
}
