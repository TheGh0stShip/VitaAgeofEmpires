#include "game_startup.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* The three spellings the command line accepts. */
static int has_word(const char *line, const char *a, const char *b, const char *c) {
    return strstr(line, a) || strstr(line, b) || (c && strstr(line, c));
}

/* The digits after "<key>=" up to the first space (atoi). */
static int resend(const char *line, const char *key, int32_t *out) {
    const char *p = strstr(line, key);
    char digits[256];
    size_t n = 0;
    if (!p) return 0;
    while (*p != '=') ++p;
    ++p;
    while (*p && !isspace((unsigned char)*p) && n + 1 < sizeof digits) digits[n++] = *p++;
    digits[n] = '\0';
    *out = atoi(digits);
    return 1;
}

int aoe_game_parse_command_line_ror(AoeGameStartupRor *g) {
    AoeStartupConfigRor *c = g->config;
    char line[256];
    size_t i;
    strncpy(line, c->command_line, 255);
    line[255] = '\0';
    for (i = 0; line[i]; ++i) line[i] = (char)toupper((unsigned char)line[i]);
    if (has_word(line, "NOSTARTUP", "NO_STARTUP", "NO STARTUP")) c->no_startup = 1;
    if (has_word(line, "SYSTEMMEMORY", "SYSTEM_MEMORY", "SYSTEM MEMORY")) c->system_memory = 1;
    if (has_word(line, "MIDIMUSIC", "MIDI_MUSIC", "MIDI MUSIC")) {
        c->music = 1;
        c->music_cd = 0;
        c->music_8b0 = 0;
        c->music_midi = 1;
        c->music_8b8 = 0;
    }
    if (strstr(line, "MSYNC")) g->msync = 1;
    g->mfill = strstr(line, "MFILL") == NULL;
    if (has_word(line, "NOSOUND", "NO_SOUND", "NO SOUND")) c->sound = 0;
    if (strstr(line, "640")) {
        c->screen_w = 640;
        c->screen_h = 480;
    }
    if (strstr(line, "800")) {
        c->screen_w = 800;
        c->screen_h = 600;
    }
    if (strstr(line, "1024")) {
        c->screen_w = 1024;
        c->screen_h = 768;
    }
    resend(line, "RESEND1=", &g->resend1);
    resend(line, "RESEND2=", &g->resend2);
    if (c->sound == 0 || has_word(line, "NOMUSIC", "NO_MUSIC", "NO MUSIC")) c->music = 0;
    if (c->check_depth == 1 && c->full_screen == 1) g->custom_mouse = 1;
    if (has_word(line, "NORMALMOUSE", "NORMAL_MOUSE", "NORMAL MOUSE")) g->custom_mouse = 0;
    return 1;
}

void aoe_game_read_registry_ror(AoeGameStartupRor *g, const AoeRegistryOpsRor *r) {
    AoeStartupConfigRor *c = g->config;
    int32_t v = r->get(r->context, "Screen Size");
    if (v == 0x400) c->screen_w = 0x400, c->screen_h = 0x300;
    else if (v == 0x280) c->screen_w = 0x280, c->screen_h = 0x1e0;
    else if (v == 800) c->screen_w = 800, c->screen_h = 600;
    else if (v == 0x500) c->screen_w = 0x500, c->screen_h = 0x400;
    g->rollover_text = r->get(r->context, "Rollover Text") == 2 ? 0 : 1;
    v = r->get(r->context, "Mouse Style");
    if (v == 2) c->mouse_style = 2;
    else if (v == 1) c->mouse_style = 1;
    v = r->get(r->context, "Game Speed");
    if (v != -1) g->game_speed = (float)v * 0.1f;
    v = r->get(r->context, "Difficulty");
    if (v != -1) g->difficulty = v;
    v = r->get(r->context, "Path Finding");
    if (v > 0 && v < 4 && g->settings) aoe_settings_set_byte_a14_ror(g->settings, (uint8_t)(v - 1));
    v = r->get(r->context, "MP Path Finding");
    if (v > 0 && v < 4 && g->settings) aoe_settings_set_byte_983_ror(g->settings, (uint8_t)(v - 1));
    v = r->get(r->context, "Scroll Speed");
    if (v != -1 && v > 9 && v < 0xc9) {
        c->scroll_speed = v;
        c->scroll_speed_8d0 = v;
    }
}

void aoe_game_write_registry_ror(const AoeGameStartupRor *g, int32_t screen_width,
                                 const AoeRegistryOpsRor *r) {
    const AoeStartupConfigRor *c = g->config;
    if (screen_width) r->set(r->context, "Screen Size", screen_width);
    r->set(r->context, "Rollover Text", 2 - (g->rollover_text != 0));
    r->set(r->context, "Mouse Style", c->mouse_style == 2 ? 2 : 1);
    r->set(r->context, "Game Speed", (int32_t)(g->game_speed * 10.0f));
    r->set(r->context, "Difficulty", g->difficulty);
    r->set(r->context, "Path Finding",
           (g->settings ? aoe_settings_byte_a14_ror(g->settings) : 0) + 1);
    r->set(r->context, "MP Path Finding",
           (g->settings ? aoe_settings_byte_983_ror(g->settings) : 0) + 1);
    r->set(r->context, "Scroll Speed", c->scroll_speed);
}

int32_t aoe_game_error_string_id_ror(int32_t kind, int32_t code) {
    if (kind == 2) return code;
    if (kind != 1) return 0;
    switch (code) {
    case 1: case 2: case 4: case 5: case 6: case 0xe: case 0xf: return 0x7d2;
    case 3: return 0x7e1;
    case 7: case 8: case 0xb: case 0xd: case 0x11: return 0x7d3;
    case 9: case 0x10: return 0x7d5;
    case 10: case 0xc: return 0x7d4;
    case 0x12: return 0x7dc;
    case 0x13: return 0x7db;
    case 0x14: return 0x7dd;
    case 0x15: return 0x7de;
    case 0x16: return 0x7df;
    case 0x17: return 0x7e0;
    default: return 0;
    }
}

int aoe_game_choose_screen_ror(AoeGameStartupRor *g, int32_t desktop_bits,
                               int (*try_mode)(void *context, int32_t width),
                               void *context) {
    AoeStartupConfigRor *c = g->config;
    if (c->check_depth && desktop_bits < 8) {
        g->error = 0x13;
        return 0;
    }
    if (try_mode(context, c->screen_w)) return 1;
    if (c->screen_w != 0x280 && try_mode(context, 0x280)) {
        c->screen_w = 0x280, c->screen_h = 0x1e0;
    } else if (c->screen_w != 800 && try_mode(context, 800)) {
        c->screen_w = 800, c->screen_h = 600;
    } else if (c->screen_w != 0x400 && try_mode(context, 0x400)) {
        c->screen_w = 0x400, c->screen_h = 0x300;
    } else {
        if (c->screen_w == 0x500 || !try_mode(context, 0x400)) return 0;
        c->screen_w = 0x500, c->screen_h = 0x400;
    }
    return 1;
}

int32_t aoe_game_music_source_ror(const AoeStartupConfigRor *c) {
    if (c->music_cd) return 1;
    if (c->music_midi) return 2;
    if (c->music_8b0) return 4;
    return c->music_8b8 ? 3 : 1;
}

int32_t aoe_game_music_volume_ror(AoeStartupConfigRor *c, int32_t registry_value) {
    int32_t volume = -5000;
    if (registry_value != -1) {
        volume = -registry_value;
        if (registry_value == 0x26ac || volume <= -0x26ad) {
            c->music = 0;
            return -1;
        }
    }
    return volume;
}

int aoe_game_cd_check_ror(const AoeStartupConfigRor *c, int32_t players,
                          const AoeCdCheckOpsRor *ops) {
    char label[256];
    const char *root;
    int32_t slot, counted = 0, i;
    if (!c->cd_required) return 1;
    if (players > 0) {
        for (slot = 0; slot < 9;) {
            i = slot;
            ++slot;
            if (ops->slot_kind(ops->context, slot) == 2 && ops->slot_flag(ops->context, i))
                ++counted;
        }
        return players <= c->cd_players * counted;
    }
    if (ops->override_cd) return 1;
    root = ops->cd_path ? ops->cd_path(ops->context) : NULL;
    if (!root || ops->drive_type(ops->context, root) != 5) return 0;
    if (!ops->volume_label(ops->context, root, label, sizeof label)) return 0;
    {
        const char *a = label, *b = c->volume_label;
        while (*a && tolower((unsigned char)*a) == tolower((unsigned char)*b)) ++a, ++b;
        return tolower((unsigned char)*a) == tolower((unsigned char)*b);
    }
}

int aoe_game_not_expired_ror(const AoeStartupConfigRor *c, int64_t now,
                             int64_t (*make_time)(int year, int month0, int day)) {
    return now <= make_time(c->expiry_year, c->expiry_month - 1, c->expiry_day);
}

int aoe_game_single_instance_ror(const AoeInstanceOpsRor *ops) {
    void *w;
    if (!ops->mutex_exists(ops->context)) {
        ops->release_mutex(ops->context, 0);
        return 1;
    }
    w = ops->find_window(ops->context);
    if (w) {
        if (ops->lobby(ops->context)) {
            ops->close_window(ops->context, w);
            ops->release_mutex(ops->context, 0);
            return 1;
        }
        ops->bring_to_front(ops->context, w);
    }
    ops->release_mutex(ops->context, 1);
    return 0;
}

int aoe_game_enough_memory_ror(uint32_t physical, uint32_t page_file) {
    return !(((physical < 0x1400000 || page_file < 0xa00000) &&
              (physical < 0xa00000 || page_file < 0x1400000)) &&
             page_file < 0x1e00000);
}

int aoe_game_startup_chain_ror(AoeGameStartupRor *g, const AoeStartupChainOpsRor *ops) {
    static const struct { int step; int32_t code; } plan[] = {
        {AOE_START_STEP_70, 5}, {AOE_START_STEP_74, 6}, {AOE_START_STEP_78, -7},
        {AOE_START_STEP_7C, 0x11}, {AOE_START_STEP_A0, 7}, {AOE_START_STEP_144, 7},
        {AOE_START_STEP_80, 8}, {AOE_START_STEP_94, 10}, {AOE_START_STEP_8C, 0x10},
        {AOE_START_STEP_90, 9}, {AOE_START_STEP_98, 0xb}, {AOE_START_STEP_9C, 0xc}};
    uint32_t physical = 0, page = 0;
    size_t i;
    if (!ops->step(ops->context, AOE_START_STEP_6C)) {
        g->error = 2;
        return 0;
    }
    if (!ops->step(ops->context, AOE_START_STEP_LIBRARY)) {
        g->error = 1;
        return 0;
    }
    ops->memory(ops->context, &physical, &page);
    if (!aoe_game_enough_memory_ror(physical, page)) {
        g->error = 0x16;
        return 0;
    }
    ops->step(ops->context, AOE_START_STEP_148);
    if (g->config->check_expiry && !ops->step(ops->context, AOE_START_STEP_EXPIRY)) {
        g->error = 3;
        return 0;
    }
    if (g->config->single_instance && !ops->step(ops->context, AOE_START_STEP_INSTANCE)) {
        g->error = 4;
        return 0;
    }
    if (!ops->nodxcheck(ops->context) && ops->directx_version(ops->context) < 0x501) {
        g->error = 0x14;
        return 0;
    }
    for (i = 0; i < sizeof plan / sizeof plan[0]; ++i) {
        if (!ops->step(ops->context, plan[i].step)) {
            if (plan[i].code < 0) {
                if (g->error == 0) g->error = -plan[i].code;
            } else {
                g->error = plan[i].code;
            }
            return 0;
        }
    }
    if (!ops->step(ops->context, AOE_START_STEP_COMM) ||
        !ops->step(ops->context, AOE_START_STEP_A4)) {
        g->error = 0xd;
        return 0;
    }
    return 1;
}

uint32_t aoe_game_message_loop_ror(const AoeMessageLoopOpsRor *ops) {
    uint32_t message = 0, wparam = 0;
    for (;;) {
        int32_t s = ops->state(ops->context);
        if (!ops->active(ops->context) || (s != 4 && s != 2) || ops->comm_waiting(ops->context)) {
            if (!ops->get(ops->context, &message, &wparam)) return wparam;
            ops->dispatch(ops->context);
            continue;
        }
        if (!ops->peek(ops->context, &message, &wparam)) {
            ops->idle(ops->context);
            continue;
        }
        if (message == 0x12) return wparam;
        if (ops->filter(ops->context)) ops->dispatch(ops->context);
    }
}

int32_t aoe_game_font_string_ror(int16_t index, int *second) {
    *second = 0;
    switch (index) {
    case 0: return 0x6e;
    case 1: return 0x71;
    case 4: return 0x74;
    case 6: return 0x83;
    case 7: return 0x77;
    case 8: *second = 1; return 0x80;
    case 9: return 0x7a;
    case 10: return 0x7d;
    case 11: return 0x80;
    default: return 0;
    }
}

void aoe_game_wrap_point_ror(int32_t *x, int32_t *y) {
    if (*x > 60000) *x -= 0x10000;
    if (*y > 60000) *y -= 0x10000;
}
