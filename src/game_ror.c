#include "game_ror.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- command line ---- */

/* The two-byte codes for 'A'..'_'. */
static const uint8_t letter_code[31][2] = {
    {104, 120}, {42, 95},  {51, 67},  {63, 62},  {65, 109}, {74, 109}, {49, 90},  {64, 41},
    {35, 76},   {125, 123}, {99, 87}, {43, 33},  {37, 99},  {100, 87}, {37, 94},  {119, 62},
    {104, 120}, {37, 37},  {49, 36},  {32, 94},  {95, 106}, {38, 84},  {52, 51},  {41, 40},
    {58, 58},   {83, 63},  {38, 75},  {82, 121}, {55, 88},  {59, 105}, {42, 38}};

void aoe_game_ror_encode_ror(const char *in, char *out, int32_t n) {
    int32_t len = 0;
    char c = *in;
    while (c != '\0' && len < n) {
        ++in;
        if (c < 'A' || c > '_') {
            *out++ = c;
            ++len;
        } else {
            *out++ = (char)letter_code[c - 'A'][0];
            *out++ = (char)letter_code[c - 'A'][1];
            len += 2;
        }
        c = *in;
    }
    *out = '\0';
}

void aoe_game_ror_command_line_ror(const char *line, const AoeGameRorCommandOpsRor *ops) {
    char upper[0x100], coded[0x202], key[0x10], digits[0x100];
    const char *p;
    int32_t i, v;
    strncpy(upper, line, 0xff);
    upper[0xff] = '\0';
    for (i = 0; upper[i]; ++i) upper[i] = (char)toupper((unsigned char)upper[i]);
    aoe_game_ror_encode_ror(upper, coded, 0x200);
    if (strstr(upper, "NOTERRAINSOUND")) ops->no_terrain_sound(ops->context);
    aoe_game_ror_encode_ror("LIMIT=", key, (int32_t)sizeof key - 2);
    p = strstr(coded, key);
    if (p) {
        while (*p != '=') ++p;
        for (i = 0; p[1] != ' ' && p[1] != '\0' && i < (int32_t)sizeof digits - 1; ++p)
            digits[i++] = p[1];
        digits[i] = '\0';
        v = atoi(digits);
        if (v < 0x1a)
            v = 0x19;
        else if (v > 200)
            v = 200;
        ops->set_population(ops->context, (uint8_t)v);
    }
    ops->base_command_line(ops->context);
}

/* ---- tribe settings ---- */

void aoe_game_ror_settings_get_ror(const AoeGameSettingsRor *g, AoeTribeSettingsRor *o) {
    int i;
    memset(o, 0, sizeof *o);
    o->map_size = aoe_settings_map_size_ror(g);
    o->map_type = aoe_settings_int_a80_ror(g);
    o->a84 = aoe_settings_int_a84_ror(g);
    o->a88 = aoe_settings_int_a88_ror(g);
    o->victory = aoe_settings_int_a8c_ror(g);
    o->victory_amount = aoe_settings_int_a90_ror(g);
    for (i = 0; i < 9; ++i) {
        o->byte_a94[i] = aoe_settings_byte_a94_ror(g, i);
        o->int_aa0[i] = aoe_settings_int_aa0_ror(g, i);
        o->byte_ac4[i] = aoe_settings_byte_ac4_ror(g, i);
        o->byte_acd[i] = aoe_settings_byte_acd_ror(g, i);
    }
    o->ad6 = aoe_settings_option_ad6_ror(g);
    o->ad7 = aoe_settings_option_ad7_ror(g);
    o->ad8 = aoe_settings_option_ad8_ror(g);
    o->ad9 = aoe_settings_option_ad9_ror(g);
    o->resource_level = aoe_settings_resource_level_ror(g);
    o->starting_age = aoe_settings_starting_age_ror(g);
    o->ae4 = aoe_settings_option_ae4_ror(g);
    o->ae5 = aoe_settings_option_ae5_ror(g);
    o->population = aoe_settings_population_limit_ror(g);
}

void aoe_game_ror_settings_set_ror(AoeGameSettingsRor *g, const AoeTribeSettingsRor *in) {
    int i;
    aoe_settings_set_map_size_ror(g, in->map_size);
    aoe_settings_set_int_a80_ror(g, in->map_type);
    aoe_settings_set_int_a84_ror(g, in->a84);
    aoe_settings_set_int_a88_ror(g, in->a88);
    aoe_settings_set_a8c_a90_ror(g, in->victory, in->victory_amount);
    for (i = 0; i < 9; ++i) {
        aoe_settings_set_byte_a94_ror(g, i, in->byte_a94[i]);
        aoe_settings_set_int_aa0_ror(g, i, in->int_aa0[i]);
        aoe_settings_set_byte_ac4_ror(g, i, in->byte_ac4[i]);
        aoe_settings_set_byte_acd_ror(g, i, in->byte_acd[i]);
    }
    aoe_settings_set_option_ad6_ror(g, in->ad6);
    aoe_settings_set_option_ad7_ror(g, in->ad7);
    aoe_settings_set_option_ad8_ror(g, in->ad8);
    aoe_settings_set_option_ad9_ror(g, in->ad9);
    aoe_settings_set_resource_level_ror(g, in->resource_level);
    aoe_settings_set_starting_age_ror(g, in->starting_age);
    aoe_settings_set_option_ae4_ror(g, in->ae4);
    aoe_settings_set_option_ae5_ror(g, in->ae5);
    aoe_settings_set_population_ror(g, in->population);
}

static void put32(uint8_t *p, int32_t v) {
    uint32_t u = (uint32_t)v;
    p[0] = (uint8_t)u;
    p[1] = (uint8_t)(u >> 8);
    p[2] = (uint8_t)(u >> 16);
    p[3] = (uint8_t)(u >> 24);
}

static int32_t get32(const uint8_t *p) {
    return (int32_t)((uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 |
                     (uint32_t)p[3] << 24);
}

void aoe_game_ror_settings_pack_ror(const AoeTribeSettingsRor *in, uint8_t out[0x6c]) {
    int i;
    memset(out, 0, 0x6c);
    put32(out + 0x00, in->map_size);
    put32(out + 0x04, in->map_type);
    put32(out + 0x08, in->a84);
    put32(out + 0x0c, in->a88);
    put32(out + 0x10, in->victory);
    put32(out + 0x14, in->victory_amount);
    for (i = 0; i < 9; ++i) {
        out[0x18 + i] = in->byte_a94[i];
        put32(out + 0x24 + 4 * i, in->int_aa0[i]);
        out[0x48 + i] = in->byte_ac4[i];
        out[0x51 + i] = in->byte_acd[i];
    }
    out[0x5a] = in->ad6;
    out[0x5b] = in->ad7;
    out[0x5c] = in->ad8;
    out[0x5d] = in->ad9;
    put32(out + 0x60, in->resource_level);
    put32(out + 0x64, in->starting_age);
    out[0x68] = in->ae4;
    out[0x69] = in->ae5;
    out[0x6a] = in->population;
}

void aoe_game_ror_settings_unpack_ror(const uint8_t in[0x6c], AoeTribeSettingsRor *o) {
    int i;
    memset(o, 0, sizeof *o);
    o->map_size = get32(in + 0x00);
    o->map_type = get32(in + 0x04);
    o->a84 = get32(in + 0x08);
    o->a88 = get32(in + 0x0c);
    o->victory = get32(in + 0x10);
    o->victory_amount = get32(in + 0x14);
    for (i = 0; i < 9; ++i) {
        o->byte_a94[i] = in[0x18 + i];
        o->int_aa0[i] = get32(in + 0x24 + 4 * i);
        o->byte_ac4[i] = in[0x48 + i];
        o->byte_acd[i] = in[0x51 + i];
    }
    o->ad6 = in[0x5a];
    o->ad7 = in[0x5b];
    o->ad8 = in[0x5c];
    o->ad9 = in[0x5d];
    o->resource_level = get32(in + 0x60);
    o->starting_age = get32(in + 0x64);
    o->ae4 = in[0x68];
    o->ae5 = in[0x69];
    o->population = in[0x6a];
}

void aoe_game_send_settings_ror(const AoeGameSettingsRor *g, const AoeGameRorCommOpsRor *ops) {
    uint8_t record[AOE_SETTINGS_RECORD_SIZE];
    if (!ops->has_comm) return;
    aoe_settings_pack_ror(g, record);
    ops->send(ops->context, record, (int32_t)sizeof record);
}

void aoe_game_receive_settings_ror(AoeGameSettingsRor *g, const AoeGameRorCommOpsRor *ops) {
    const uint8_t *data;
    int32_t n = 0;
    if (!ops->has_comm) return;
    data = ops->receive(ops->context, &n);
    if (data && n == AOE_SETTINGS_RECORD_SIZE) aoe_settings_apply_ror(g, data);
}

void aoe_game_ror_send_settings_ror(const AoeGameSettingsRor *g,
                                    const AoeGameRorCommOpsRor *ops) {
    uint8_t record[0x114];
    AoeTribeSettingsRor t;
    if (!ops->has_comm) return;
    aoe_settings_pack_ror(g, record);
    aoe_game_ror_settings_get_ror(g, &t);
    aoe_game_ror_settings_pack_ror(&t, record + AOE_SETTINGS_RECORD_SIZE);
    ops->send(ops->context, record, (int32_t)sizeof record);
}

void aoe_game_ror_receive_settings_ror(AoeGameSettingsRor *g, const AoeGameRorCommOpsRor *ops) {
    const uint8_t *data;
    int32_t n = 0;
    AoeTribeSettingsRor t;
    if (!ops->has_comm) return;
    data = ops->receive(ops->context, &n);
    if (!data || n != 0x114) return;
    aoe_settings_apply_ror(g, data);
    aoe_game_ror_settings_unpack_ror(data + AOE_SETTINGS_RECORD_SIZE, &t);
    aoe_game_ror_settings_set_ror(g, &t);
}

/* ---- trigonometry ---- */

static void trig(float *s, float *c, float *t, float *a, int n, double step, int q1, int q3,
                 int h) {
    int i;
    for (i = 0; i < n; ++i) {
        double r = (double)i * step;
        s[i] = (float)sin(r);
        c[i] = (float)cos(r);
        t[i] = (i == q1 || i == q3) ? 0.0f : (float)tan(r);
        a[i] = (i == 0 || i == h) ? 0.0f : (float)atan(r);
    }
}

void aoe_game_ror_trig_ror(AoeTrigTablesRor *t) {
    trig(t->sin4096, t->cos4096, t->tan4096, t->atan4096, 4096, 0.001533980788, 0x400, 0xc00,
         0x800);
    trig(t->sin360, t->cos360, t->tan360, t->atan360, 360, 0.01745329252, 0x5a, 0x10e, 0xb4);
}

/* ---- view history ---- */

void aoe_game_ror_view_push_ror(AoeViewHistoryRor *h, int32_t x, int32_t y) {
    if (h->head >= 0 && h->x[h->head] == x && h->y[h->head] == y) {
        h->cursor = h->head;
        return;
    }
    if (++h->head > 4) h->head = 0;
    h->x[h->head] = x;
    h->y[h->head] = y;
    h->cursor = h->head;
}

void aoe_game_ror_view_back_ror(AoeViewHistoryRor *h,
                                void (*move)(void *context, float x, float y), void *context) {
    int32_t x, y;
    if (h->cursor == -1) return;
    x = h->x[h->cursor];
    y = h->y[h->cursor];
    if (--h->cursor < 0) h->cursor = 4;
    if (h->x[h->cursor] == -1 || h->y[h->cursor] == -1) h->cursor = h->head;
    move(context, (float)x, (float)y);
}

/* ---- computer names ---- */

int32_t aoe_game_ror_pick_name_ror(AoeComputerNamesRor *n, int32_t civ, int32_t count,
                                   int32_t (*rand)(void *context), void *context) {
    int32_t tries = 0, pick, i;
    uint8_t *used = n->used[civ];
    do {
        pick = count > 0 ? rand(context) % count : 0;
        if (pick > count - 1) pick = count - 1;
    } while (used[pick] && ++tries < 0x14);
    if (used[pick] == 1) {
        for (i = 0; i < 10; ++i)
            if (!used[i]) {
                pick = i;
                break;
            }
    }
    used[pick] = 1;
    return civ * 10 + pick - 9;
}

void aoe_game_ror_clear_names_ror(AoeComputerNamesRor *n) { memset(n, 0, sizeof *n); }

/* ---- names and colour tables ---- */

void aoe_game_ror_set_name_ror(char name[0x104], const char *text) {
    if (!text) {
        name[0] = '\0';
        return;
    }
    strncpy(name, text, 0x103);
    name[0x103] = '\0';
}

void aoe_game_ror_set_color_tables_ror(AoeColorTablesRor *t, const void *a, int32_t b,
                                       const void *c, int32_t d) {
    t->a = a;
    t->b = b;
    t->c = c;
    t->d = d;
}

void aoe_game_ror_pick_color_tables_ror(AoeColorTablesRor *t, int16_t mode,
                                        const AoeColorTablesRor sets[2]) {
    const AoeColorTablesRor *s = &sets[mode == 1 ? 1 : 0];
    aoe_game_ror_set_color_tables_ror(t, s->a, s->b, s->c, s->d);
}

/* ---- the game screen ---- */

static int in_game(int32_t mode) { return mode == 4 || mode == 6 || mode == 5; }

void aoe_game_ror_set_edit_mode_ror(const AoeGameRorScreenOpsRor *ops, int32_t a, int32_t b,
                                    int32_t *edit_mode) {
    int32_t old = *edit_mode;
    ops->set_edit_mode(ops->context, a, b);
    if (ops->game_screen) ops->mode_changed(ops->context, old, *edit_mode);
}

void aoe_game_ror_set_player_ror(const AoeGameRorScreenOpsRor *ops, int32_t v) {
    int16_t old = 0, now = 0;
    int have = ops->world(ops->context, &old);
    ops->set_player(ops->context, v);
    if (have && ops->world(ops->context, &now) && ops->game_screen)
        ops->player_changed(ops->context, old, now);
}

static void *view_part(const AoeGameRorScreenOpsRor *ops, int which) {
    if (in_game(ops->mode) && ops->game_screen) return ops->panel(ops->context, ops->game_screen, which);
    if (ops->mode == 7) {
        void *editor = ops->editor(ops->context);
        if (editor) return ops->panel(ops->context, editor, which + 2);
    }
    return NULL;
}

void *aoe_game_ror_main_view_ror(const AoeGameRorScreenOpsRor *ops) { return view_part(ops, 0); }
void *aoe_game_ror_minimap_ror(const AoeGameRorScreenOpsRor *ops) { return view_part(ops, 1); }

void aoe_game_ror_title_ror(const AoeGameRorScreenOpsRor *ops, const char *title) {
    if (ops->game_screen) ops->title(ops->context, title);
}

void aoe_game_ror_status_lines_ror(const AoeGameRorScreenOpsRor *ops) {
    const char *lines[6];
    int i;
    if (!ops->game_screen) return;
    for (i = 5; i >= 0; --i) lines[i] = ops->status_line(ops->context, i + 1);
    ops->lines(ops->context, lines);
}

/* ---- input ---- */

int32_t aoe_game_ror_window_proc_ror(AoeGameRorInputOpsRor *ops, uint32_t msg) {
    if (msg == 7) {
        ops->cursor(ops->context, ops->wait_cursor == 0);
    } else if (msg == 0x200 && ops->mode == 1) {
        ops->hide_cursor(ops->context);
    } else if ((msg == 0x201 || msg == 0x204) && ops->mode == 1) {
        ops->end_video(ops->context, 1);
        return 0;
    }
    return ops->base_proc(ops->context, msg);
}

int32_t aoe_game_ror_video_proc_ror(AoeGameRorInputOpsRor *ops, uint32_t msg) {
    if (msg == 0x200) {
        ops->hide_cursor(ops->context);
        return 0;
    }
    if (msg != 0x201 && msg != 0x204) return ops->base_proc(ops->context, msg);
    ops->end_video(ops->context, 1);
    return 0;
}

int32_t aoe_game_ror_palette_ror(AoeGameRorInputOpsRor *ops) {
    if (ops->mode == 1) {
        if (ops->video && !ops->video_busy) ops->send(ops->context, ops->video, 0x476);
        return 1;
    }
    return ops->base_palette(ops->context);
}

int32_t aoe_game_ror_activate_ror(AoeGameRorInputOpsRor *ops, int32_t active) {
    ops->base_activate(ops->context, active);
    if (ops->mode == 1 && ops->video) {
        if (active == 1) {
            if (ops->paused) {
                ops->send(ops->context, ops->video, 0x855);
                ops->paused = 0;
            }
        } else if (!ops->paused) {
            ops->send(ops->context, ops->video, 0x809);
            ops->paused = 1;
        }
    }
    return 1;
}

int32_t aoe_game_ror_key_ror(AoeGameRorInputOpsRor *ops, int32_t key) {
    if (ops->mode != 1) {
        if (key != 0x70) return 1;
        if (!ops->session || !(in_game(ops->mode) || ops->mode == 7)) ops->help(ops->context);
        return 0;
    }
    if (key == 0x1b) {
        ops->end_video(ops->context, 0);
        return 0;
    }
    if (key != 0xd && key != 0x20) return 1;
    ops->end_video(ops->context, 1);
    return 0;
}

/* ---- screens ---- */

void aoe_game_ror_delete_screens_ror(void (*remove)(void *context, const char *name),
                                     void *context, int all, void **game_screen) {
    static const char *const names[] = {
        "Object List Dialog", "OKDialog", "YesNoDialog", "RestartDialog", "QuitGameDialog",
        "CloseProgramDialog", "QuitAndLoadDialog", "ResignDialog", "Help Dialog",
        "About Dialog", "Send Quick Message Dialog", "Mission Dialog", "Send Message Dialog",
        "Diplomacy Dialog", "Menu Dialog", "Save Game Screen", "Load Saved Game Screen",
        "End Screen", "Achievements Screen", "Multiplayer Wait Screen",
        "Multiplayer Disconnect Screen"};
    size_t i;
    for (i = 0; i < sizeof names / sizeof names[0]; ++i) remove(context, names[i]);
    if (all) {
        *game_screen = NULL;
        remove(context, "Game Screen");
    }
}

int32_t aoe_game_ror_end_text_id_ror(uint8_t won) { return won == 1 ? 0x47e : 0x47f; }

int aoe_game_ror_to_menu_ror(const char *screen, const AoeGameRorMenuOpsRor *ops) {
    if (!ops) return 0;
    if (ops->video_playing && ops->end_video) ops->end_video(ops->context);
    if (!ops->make_screen || !ops->make_screen(ops->context)) return 0;
    if (ops->show_named) ops->show_named(ops->context, screen);
    if (ops->set_mode) ops->set_mode(ops->context, 2);
    if (!ops->music_source || ops->music_held) return 1;
    switch (ops->music_source) {
    case 1:
        if (ops->play_track) ops->play_track(ops->context, 2);
        break;
    case 2:
        if (ops->play_file) ops->play_file(ops->context, "xopen.mid");
        break;
    case 3:
        if (ops->play_file) ops->play_file(ops->context, "open.wav");
        break;
    default:
        break;
    }
    return 1;
}

void aoe_game_ror_leave_ror(const AoeGameRorLeaveOpsRor *o) {
    int32_t s;
    if (!o) return;
    if (!o->networked) {
        if (o->reset_comm) o->reset_comm(o->context);
        return;
    }
    s = o->state;
    if (s != 4 && s != 6 && s != 7 && s != 5) return;
    if (o->send_markers) o->send_markers(o->context);
    if (o->poll) o->poll(o->context);
    if (o->unacknowledged && o->unacknowledged(o->context) > 0 && o->now_ms) {
        uint32_t start, last;
        if (o->wait_cursor) o->wait_cursor(o->context);
        if (o->status) o->status(o->context, 0x4c0);
        start = last = o->now_ms(o->context);
        for (;;) {
            uint32_t now;
            if (o->world_done && o->world_done(o->context)) break;
            if (o->poll) o->poll(o->context);
            now = o->now_ms(o->context);
            if (now - last < 500) continue;
            if (!o->unacknowledged(o->context)) break;
            last = o->now_ms(o->context);
            if (last - start >= 15000) break;
        }
    }
    if (o->reset_comm) o->reset_comm(o->context);
    if (o->unlink) o->unlink(o->context, 5);
}

int aoe_game_ror_quit_session_ror(const AoeGameRorEndOpsRor *o) {
    if (o && o->has_comm) {
        aoe_game_ror_leave_ror(&o->leave);
        if (o->set_state) o->set_state(o->leave.context, 0);
    }
    return 1;
}

void aoe_game_ror_end_session_ror(const AoeGameRorEndOpsRor *o) {
    int32_t s;
    if (!o) return;
    aoe_game_ror_leave_ror(&o->leave);
    if (o->test_pending) {
        if (o->back_to_editor) o->back_to_editor(o->leave.context);
        return;
    }
    s = o->leave.state;
    if (s != 4 && s != 6 && s != 5 && s != 3) return;
    if (o->set_state) o->set_state(o->leave.context, 0);
    if (!o->has_world) return;
    if (o->world_ended) o->world_ended(o->leave.context);
    if (o->play_video) o->play_video(o->leave.context, 4, o->won);
}

int aoe_game_ror_ensure_world_ror(AoeGameRorWorldOpsRor *ops) {
    if (ops->world) return 1;
    ops->world = ops->create_world(ops->context);
    if (!ops->world) return 0;
    if (!ops->load(ops->context, ops->world)) {
        ops->delete_world(ops->context, ops->world);
        ops->world = NULL;
        return 0;
    }
    return 1;
}

int aoe_game_ror_check_world_ror(AoeGameRorWorldOpsRor *ops) {
    int ok = 0;
    ops->cursor(ops->context, 1);
    ops->status(ops->context, 0x44d);
    if (ops->world) {
        ops->delete_world(ops->context, ops->world);
        ops->world = NULL;
    }
    ops->world = ops->create_world(ops->context);
    if (ops->world) {
        ok = ops->check(ops->context, ops->world) != 0;
        ops->delete_world(ops->context, ops->world);
        ops->world = NULL;
    }
    ops->status_close(ops->context);
    ops->cursor(ops->context, 0);
    return ok;
}

/* 0x004fdea0 */
void aoe_game_ror_run_log_ror(AoeGameRorRunLogRor *log, const char *text, int32_t stamped) {
    void *f;
    char line[1100];
    if (!log->enabled) return;
    f = log->open(log->context, "c:\\aoerun.txt", log->opened ? "a" : "w");
    log->opened = 1;
    if (!f) return;
    if (stamped) {
        char when[64], cut[25];
        when[0] = 0;
        log->now_text(log->context, when, (int32_t)sizeof when);
        /* The original copies the first 24 characters of the time text
           (the ctime() line without its newline). */
        memcpy(cut, when, 24);
        cut[24] = 0;
        snprintf(line, sizeof line, "%s - %.1024s\n", cut, text);
    } else {
        snprintf(line, sizeof line, "%.1024s\n", text);
    }
    log->write(log->context, f, line);
    log->close(log->context, f);
}

/* ---- destructor, editor open, video close ---- */

void aoe_game_ror_destroy_ror(AoeGameRorDestroyOpsRor *ops) {
    static const char *const screens[] = {
        "Video Screen", "Create Dialog", "Campaign Editor Screen", "Game Setup Screen",
        "Game Settings Screen", "Select Scenario Screen", "Campaign Selection Screen",
        "Load Saved Game Screen", "MP Startup Screen", "MP Setup Screen",
        "Connecting Screen", "Join Screen", "Main Menu", "Save Game Screen", "End Screen",
        "Scenario Editor Menu", "Scenario Editor Open", "Scenario Editor Screen",
        "Single Player Menu", "New Name Dialog", "Name Selection Screen", "Status Screen",
        "Temp Screen", "ScenarioEditorInfo", "FeaturesInfo", "QuotesInfo", "Logo1Screen",
        "Logo2Screen"};
    static const char *const archives[] = {"sounds.drs", "graphics.drs", "Terrain.drs",
                                           "Border.drs", "Interfac.drs"};
    size_t i;
    void *unused_screen = NULL;
    ops->field_1250 = 0;
    ops->mode = 0;
    if (ops->video_window) {
        ops->destroy_window(ops->context, ops->video_window);
        ops->video_window = 0;
    }
    aoe_game_ror_delete_screens_ror(ops->remove_screen, ops->context, 1,
                                    ops->game_screen ? ops->game_screen : &unused_screen);
    for (i = 0; i < sizeof screens / sizeof screens[0]; ++i)
        ops->remove_screen(ops->context, screens[i]);
    for (i = 0; i < sizeof archives / sizeof archives[0]; ++i)
        ops->close_archive(ops->context, archives[i]);
    if (ops->language_library && *ops->language_library) {
        ops->free_library(ops->context, *ops->language_library);
        *ops->language_library = 0;
    }
    if (ops->base_destroy) ops->base_destroy(ops->context);
}

int aoe_game_ror_open_editor_ror(AoeGameRorEditorOpsRor *ops, const char *name) {
    snprintf(ops->scenario_name, sizeof ops->scenario_name, "%s", name);
    ops->remove_screen(ops->context, "Scenario Editor Screen");
    ops->set_state(ops->context, 0);
    if (ops->open_editor(ops->context, ops->scenario_name)) return 1;
    if (!ops->fallback(ops->context, ops->scenario_name, 0) && !ops->main_menu(ops->context))
        ops->give_up(ops->context);
    return 0;
}

void aoe_game_ror_close_video_ror(AoeGameRorVideoCloseOpsRor *ops) {
    if (!ops->video_up) return;
    if (ops->mode_switched) {
        ops->step(ops->context, AOE_GAME_ROR_VSTEP_SET_MODE, ops->width, ops->height);
        ops->step(ops->context, AOE_GAME_ROR_VSTEP_BLANK, 0, 0);
        ops->step(ops->context, AOE_GAME_ROR_VSTEP_PALETTE, 0, 0);
        ops->mode_switched = 0;
        ops->step(ops->context, AOE_GAME_ROR_VSTEP_CHECK_LOST, 0, 0);
        ops->step(ops->context, AOE_GAME_ROR_VSTEP_CLEAR_RESTORED, 0, 0);
        ops->step(ops->context, AOE_GAME_ROR_VSTEP_4197B0, 0, 0);
        if (ops->redraw_flag) *ops->redraw_flag = 1;
    }
    ops->step(ops->context, AOE_GAME_ROR_VSTEP_BLANK, 0, 0);
    ops->step(ops->context, AOE_GAME_ROR_VSTEP_PALETTE, 0, 0);
    ops->step(ops->context, AOE_GAME_ROR_VSTEP_41C290, 0, 0);
    ops->step(ops->context, AOE_GAME_ROR_VSTEP_INVALIDATE, 0, 0);
    ops->step(ops->context, AOE_GAME_ROR_VSTEP_STATE, 2, 0);
    ops->step(ops->context, AOE_GAME_ROR_VSTEP_B0, 0, 0);
    ops->step(ops->context, AOE_GAME_ROR_VSTEP_ARROW_CURSOR, 0, 0);
    ops->step(ops->context, AOE_GAME_ROR_VSTEP_SHOW_CURSOR, 0, 0);
    ops->video_up = 0;
}

int aoe_game_ror_open_video_ror(AoeGameRorVideoOpenOpsRor *ops) {
    char line[64];
    if (ops->video_up) return 1;
    ops->prepare(ops->context);
    ops->double_size = 1;
    ops->mode_switched = 0;
    ops->mode_16bit = 0;
    if (ops->full_screen) {
        ops->hide_cursor(ops->context);
        ops->old_width = ops->screen_width;
        ops->old_height = ops->screen_height;
        if (!ops->eight_bit_switch) {
            if (ops->mode_available(ops->context, 640, 480, 16)) {
                ops->log(ops->context, "640x480 16bit");
                ops->mode_switched = ops->set_mode(ops->context, 640, 480, 16);
                if (ops->mode_switched) {
                    ops->double_size = 1;
                    ops->mode_16bit = 1;
                    ops->log(ops->context, "640x480 16bit success");
                }
            }
            if (!ops->mode_switched && ops->screen_width != 640 &&
                ops->mode_available(ops->context, 640, 480, 8)) {
                ops->log(ops->context, "640x480 8bit");
                ops->mode_switched = ops->set_mode(ops->context, 640, 480, 8);
                if (ops->mode_switched) {
                    ops->double_size = 1;
                    ops->log(ops->context, "640x480 8bit success");
                }
            }
        }
    }
    snprintf(line, sizeof line, "video_changed_res=%d, double_size=%d", (int)ops->mode_switched,
             (int)ops->double_size);
    ops->log(ops->context, line);
    ops->state(ops->context, 1);
    ops->after(ops->context);
    ops->video_up = 1;
    return 1;
}

void aoe_game_ror_stats_text_ror(const AoeGameBaseRor *g, int show_max, char *title, int title_size,
                                 char *view, int view_size) {
    uint32_t frames, views;
    if (!g || !title || !view) return;
    frames = (uint32_t)g->timing_504[1];
    views = (uint32_t)g->timing_504[2];
#define F4(s) ((unsigned long)(g->timing[(s)].field_4 / frames))
#define MX(s) ((unsigned long)g->timing[(s)].field_1c)
    if (frames == 0) {
        snprintf(title, (size_t)title_size, "f?");
    } else if (!show_max) {
        snprintf(title, (size_t)title_size,
                 "t%lu,f%lu,avg(r(v%lu,m%lu,o%lu),s%lu,u%lu,c%lu,l%lu,p%lu,o%lu)",
                 (unsigned long)g->timing_504[7], (unsigned long)g->timing_504[8], F4(6), F4(7),
                 F4(17), F4(3), F4(1), F4(14),
                 (unsigned long)((g->timing[12].field_4 + g->timing[10].field_4) / frames),
                 F4(13), F4(4));
    } else {
        snprintf(title, (size_t)title_size,
                 "t%lu,f%lu,max(r(v%lu,m%lu,o%lu),s%lu,u%lu,c%lu,ls%lu,lg%lu,p%lu,o%lu)",
                 (unsigned long)g->timing_504[7], (unsigned long)g->timing_504[8], MX(6), MX(7),
                 MX(17), MX(3), MX(1), MX(14), MX(10), MX(12), MX(13), MX(4));
    }
#undef F4
#undef MX
    if (views)
        snprintf(view, (size_t)view_size, "avg view time=%lu, count=%lu",
                 (unsigned long)(g->timing[6].field_4 / views), (unsigned long)views);
    else
        snprintf(view, (size_t)view_size, "avg view time=???");
}

/* 0x005016d0 */
void aoe_game_ror_ai_lines_ror(AoeGameRorAiLinesOpsRor *ops, char lines[6][256]) {
    const char *t;
    const char *show[6];
    int i;
    if (!ops || !ops->has_world) return;
    for (i = 0; i < 6; i++) lines[i][0] = 0;
    if (ops->debug_mode) {
        t = ops->ai_text(ops->context, 1);
        if (t) snprintf(lines[0], 256, "Player #%d: %s", (int)ops->player, t);
        else snprintf(lines[0], 256, "Player #%d: AI is not functional.", (int)ops->player);
        for (i = 2; i < 7; i++) {
            t = ops->ai_text(ops->context, i);
            snprintf(lines[i - 1], 256, "%s", t ? t : "");
        }
    } else {
        t = ops->ai_text(ops->context, 0);
        snprintf(lines[0], 256, "%s", t ? t : "");
        snprintf(lines[1], 256, "Player #%d is not a computer player.", (int)ops->player);
    }
    if (ops->panel_up && ops->show) {
        for (i = 0; i < 6; i++) show[i] = lines[ops->debug_mode ? 5 - i : i];
        ops->show(ops->context, show);
    }
}

/* 0x005003f0 */
int aoe_game_ror_play_video_ror(AoeGameRorVideoStateRor *s, const AoeGameRorVideoPlayOpsRor *o,
                                int32_t after, const char *name) {
    char path[260], mode[100];
    int32_t w, h;
    if (s->window) {
        mode[0] = '\0';
        o->window_mode(o->context, s->window, mode, sizeof mode);
        if (mode[0] && strcmp(mode, "stopped") != 0) o->window_stop(o->context, s->window);
        o->window_close(o->context, s->window);
        s->window = NULL;
    }
    s->after = after;
    if (o->no_videos || !name || !*name) goto fail;
    snprintf(path, sizeof path, "%s%s.avi", o->avi_dir, name);
    if (!o->file_exists(o->context, path)) {
        const char *cd = o->cd_path(o->context);
        if (!cd) goto fail;
        snprintf(path, sizeof path, "%sgame\\avi\\%s.avi", cd, name);
        if (!o->file_exists(o->context, path)) goto fail;
    }
    if (!o->videos_enabled || (!*o->video_up && !o->open_video(o->context))) goto fail;
    s->window = o->create_window(o->context, path);
    if (!s->window) goto fail;
    if (!o->mode_16bit) o->set_palette(o->context, s->window);
    o->client_size(o->context, s->window, &w, &h);
    if (o->double_size) { w *= 2; h *= 2; }
    o->move_window(o->context, s->window, o->screen_width / 2 - w / 2, o->screen_height / 2 - h / 2,
                   w, h);
    o->before_play(o->context);
    s->field_bf8 = 0;
    s->start_time = o->time_now(o->context);
    o->play(o->context, s->window);
    o->after_play(o->context);
    return 1;
fail:
    o->close_video(o->context);
    return 0;
}

/* 0x00505670 */
int aoe_game_ror_idle_ror(const AoeGameRorIdleOpsRor *o) {
    char text[0x900], mode[100];
    if (!o->base_tick(o->context)) return 0;
    if (*o->busy) return 0;
    *o->busy = 1;
    if (o->mode == 1 && o->video_window) {
        if (o->now(o->context) - *o->video_time >= 1000) {
            mode[0] = '\0';
            o->window_mode(o->context, o->video_window, mode, sizeof mode);
            if (!mode[0] || strcmp(mode, "stopped") == 0) {
                o->end_video(o->context);
            } else if (o->video_field_bf8 == 0 && strcmp(mode, "paused") == 0) {
                o->window_message_855(o->context, o->video_window);
            }
            *o->video_time = o->now(o->context);
        }
    } else if (o->mode == 3) {
        if (o->comm_started(o->context)) {
            o->start_game(o->context);
        } else if (o->now(o->context) - *o->wait_time > 499) {
            int32_t own = o->own_slot(o->context), i;
            size_t n;
            text[0] = '\0';
            o->string(o->context, 0x454, text, sizeof text);
            n = strlen(text);
            n += (size_t)snprintf(text + n, sizeof text - n, "\n");
            for (i = 1; i < 9; ++i) {
                if (i == own || !o->slot_playing(o->context, i)) continue;
                n += (size_t)snprintf(text + n, sizeof text - n, "\n%s", o->slot_name(o->context, i));
                if (n >= sizeof text) n = sizeof text - 1;
            }
            if (o->wait_screen_up(o->context)) o->wait_screen_text(o->context, text);
            *o->wait_time = o->now(o->context);
        }
    } else if ((o->mode == 4 || o->mode == 5 || o->mode == 6) && !o->global_580e28 &&
               o->has_game_screen) {
        o->screen_idle(o->context);
    }
    *o->busy = 0;
    return 1;
}

/* 0x00502d60 */
int32_t aoe_game_ror_start_file_ror(const AoeGameRorStartFileOpsRor *o, const char *name) {
    char base[260], *dot;
    const char *back;
    int32_t i, n;
    *o->busy = 0;
    o->cursor(o->context, 1);
    if (*o->mode == 4 || *o->mode == 5 || *o->mode == 6) {
        *o->mode = 0;
        o->world_stop(o->context);
        *o->game_screen = NULL;
        o->remove_screen(o->context, "Game Screen");
    }
    o->status(o->context, 0x44d);
    if (o->ensure_world(o->context)) {
        int ok;
        o->status(o->context, 0x452);
        if (o->has_sound) o->sound_hold(o->context, 1);
        ok = o->load(o->context, name);
        n = o->player_count;
        for (i = 0; i < n; ++i) o->clear_wonder(o->context, i);
        *o->global_558094 = 1;
        if (o->has_sound) o->sound_hold(o->context, 0);
        if (ok) {
            o->settings_bytes(o->context, o->map_8db8, o->map_8db9, 1, 1, (uint8_t)(n - 1));
            *o->global_554738 = o->path_finding == 1 ? 5000 : o->path_finding == 2 ? 7500 : 2500;
            for (i = 0; i < 9; ++i) {
                if (i < n) {
                    o->slot_player(o->context, i, i);
                    if (i > 0) {
                        o->slot_name(o->context, i);
                        o->slot_state(o->context, i, i == 1 ? 2 : 4);
                    }
                } else {
                    o->slot_player(o->context, i, 0);
                    o->slot_state(o->context, i, 0);
                }
            }
            snprintf(base, sizeof base, "%s", name);
            dot = strchr(base, '.');
            if (dot) *dot = '\0';
            o->set_names(o->context, base);
            o->delete_screens(o->context);
            return o->start(o->context);
        }
    }
    back = o->return_name(o->context);
    if (back && *back) {
        o->show_screen(o->context, back);
        o->remove_screen(o->context, "Status Screen");
    } else {
        o->status_close(o->context);
    }
    o->cursor(o->context, 0);
    return 0;
}

/* 0x005046e0 */
void aoe_game_ror_start_game_ror(const AoeGameRorStartOpsRor *o) {
    char line[0x3f4], size[0x14], type[0x14], victory[0x100], res[0x100], age[0x100], diff[0x100];
    const char *s;
    int i, n;
    size_t len;
    o->show_screen(o->context, "Game Screen");
    o->remove_screen(o->context, "Status Screen");
    o->remove_screen(o->context, "Multiplayer");
    o->log(o->context, "game started", 1);
    type[0] = '\0';
    o->string(o->context, o->map_type >= 0 && o->map_type <= 6 ? 0x296a + o->map_type : 0x2804,
              type, sizeof type);
    size[0] = '\0';
    o->string(o->context, o->map_size >= 0 && o->map_size <= 5 ? 0x2973 + o->map_size : 0x2804,
              size, sizeof size);
    snprintf(line, sizeof line, "    randomgame=%d, randommap=%d, mapsize=%s, maptype=%s",
             (int)o->random_game, (int)o->random_map, size, type);
    o->log(o->context, line, 0);
    switch (o->victory) {
    case 0: snprintf(victory, sizeof victory, "Default"); break;
    case 7: snprintf(victory, sizeof victory, "Time(%d)", (int)o->victory_value); break;
    case 8: snprintf(victory, sizeof victory, "Score(%d)", (int)o->victory_value); break;
    case 9: snprintf(victory, sizeof victory, "Standard"); break;
    default: snprintf(victory, sizeof victory, "Unknown"); break;
    }
    if (o->death_match) s = "DeathMatch";
    else s = o->resource_level == 0 ? "Default" : o->resource_level == 1 ? "Low"
           : o->resource_level == 2 ? "Medium" : o->resource_level == 3 ? "High" : "Unknown";
    snprintf(res, sizeof res, "%s", s);
    s = o->starting_age == 0 ? "Default" : o->starting_age == 2 ? "Stone" : o->starting_age == 3 ? "Tool"
      : o->starting_age == 4 ? "Bronze" : o->starting_age == 5 ? "Iron" : "Unknown";
    snprintf(age, sizeof age, "%s", s);
    snprintf(diff, sizeof diff, "Unknown");
    o->string(o->context, 0x2bd0 + o->difficulty, diff, sizeof diff);
    snprintf(line, sizeof line, "    resources=%.200s, age=%.200s, victory=%.200s, difficulty=%.200s", res, age,
             victory, diff);
    o->log(o->context, line, 0);
    snprintf(line, sizeof line, "    fulltech=%d, revealmap=%d", o->full_tech, o->reveal_map);
    o->log(o->context, line, 0);
    n = o->player_count;
    len = (size_t)snprintf(line, sizeof line, "    numplayers=%d, players=", n - 1);
    for (i = 1; i < n && len < sizeof line; ++i)
        len += (size_t)snprintf(line + len, sizeof line - len, "%d", o->slot_ac4(o->context, i - 1));
    if (len < sizeof line) len += (size_t)snprintf(line + len, sizeof line - len, ", teams=");
    for (i = 1; i < n && len < sizeof line; ++i)
        len += (size_t)snprintf(line + len, sizeof line - len, "%d", o->slot_994(o->context, i - 1) - 1);
    if (len < sizeof line) len += (size_t)snprintf(line + len, sizeof line - len, ", civs=");
    for (i = 1; i < n && len < sizeof line; ++i)
        len += (size_t)snprintf(line + len, sizeof line - len, "%d ", o->slot_a94(o->context, i - 1));
    o->log(o->context, line, 0);
    if (o->music_kind) {
        if (o->music_kind == 1) o->music_tracks(o->context, 5, 0xe);
        else if (o->music_kind == 2) o->music_tracks(o->context, 1, o->music_track);
        else if (o->music_kind == 3) o->music_stop(o->context);
        *o->menu_music_held = 0;
    }
    o->timing_reset(o->context);
    o->set_state(o->context, 4);
    if (o->settings_paused) o->resume(o->context);
    *o->field_9fc = 0;
    o->chat_drop(o->context);
    o->chat_recipient(o->context, o->viewed_player);
    if (o->can_centre_cursor) o->centre_cursor(o->context);
    o->cursor_release(o->context);
}

/* 0x004fef10 */
const char *const aoe_game_ror_sound_names_ror[AOE_GAME_ROR_SOUNDS] = {
    "button1.wav", "button2.wav", "chatrcvd.wav", "cantdo.wav", "tribute.wav", "resdone.wav",
    "pkilled.wav", "pdropped.wav", "mstart.wav", "mdone.wav", "mkilled.wav", "artheld.wav",
    "artlost.wav", "convwarn.wav", "convdone.wav", "res.wav", "farmdie.wav"};
int aoe_game_ror_load_sounds_ror(const AoeGameRorSoundOpsRor *o, void ***sounds, int16_t *count) {
    int i;
    *count = AOE_GAME_ROR_SOUNDS;
    *sounds = o->alloc_zero(o->context, AOE_GAME_ROR_SOUNDS, sizeof(void *));
    if (!*sounds) return 0;
    for (i = 0; i < AOE_GAME_ROR_SOUNDS; ++i)
        (*sounds)[i] = o->load(o->context, aoe_game_ror_sound_names_ror[i], 0xc47c + i);
    for (i = 0; i < AOE_GAME_ROR_SOUNDS; ++i)
        if ((*sounds)[i]) o->preload(o->context, (*sounds)[i]);
    return 1;
}

/* 0x00503cb0 */
static int coded_is(const char *coded, const char *plain) {
    char key[0x80];
    aoe_game_ror_encode_ror(plain, key, (int32_t)sizeof key - 2);
    return strcmp(coded, key) == 0;
}
int aoe_game_ror_cheat_ror(const AoeGameRorCheatOpsRor *o, int32_t player, const char *text) {
    static const struct { const char *name; int32_t code; } searched[6] = {
        {"STORMBILLY", 0xe}, {"CONVERT THIS!", 0xf}, {"BIG MOMMA", 0x10}, {"POW", 0x11},
        {"GRANTLINKSPENCE", 0x12}, {"KING ARTHUR", 0x13}};
    static const struct { const char *name; int32_t code; } plain_codes[11] = {
        {"PHOTON MAN", 0}, {"E=MC2 TROOPER", 1}, {"JACK BE NIMBLE", 2}, {"BIG BERTHA", 3},
        {"FLYING DUTCHMAN", 4}, {"UPSIDFLINTMOBILE", 7}, {"HOYOHOYO", 8}, {"DARK RAIN", 10},
        {"BLACK RIDER", 0xb}, {"MEDUSA", 0xc}, {"ICBM", 0xd}};
    static const struct { const char *name; int32_t code; } kill_codes[9] = {
        {"DIEDIEDIE", 100}, {"KILL1", 0xc9}, {"KILL2", 0xca}, {"KILL3", 0xcb}, {"KILL4", 0xcc},
        {"KILL5", 0xcd}, {"KILL6", 0xce}, {"KILL7", 0xcf}, {"KILL9", 0xd0}};
    static const struct { const char *name; int32_t code; } late_codes[7] = {
        {"PEPPERONI PIZZA", 0x14}, {"COINAGE", 0x15}, {"WOODSTOCK", 0x17}, {"QUARRY", 0x16},
        {"STEROIDS", 0x18}, {"BIGDADDY", 0xdc}, {"REVEAL MAP", 0xe6}};
    char upper[0x100], coded[0x202];
    int i;
    if (!text || player < 0 || player >= o->player_count) return 0;
    snprintf(upper, sizeof upper, "%s", text);
    for (i = 0; upper[i]; ++i) upper[i] = (char)toupper((unsigned char)upper[i]);
    aoe_game_ror_encode_ror(upper, coded, 0x200);
    if (!o->cheats_on) return 0;
    if (coded_is(coded, "HOME RUN") && o->flag_41b750) { o->cheat(o->context, 0x66); return 1; }
    for (i = 0; i < 6; ++i)
        if (strstr(upper, searched[i].name)) { o->cheat(o->context, searched[i].code); return 1; }
    for (i = 0; i < 11; ++i)
        if (coded_is(coded, plain_codes[i].name)) { o->cheat(o->context, plain_codes[i].code); return 1; }
    if (o->flag_41b750)
        for (i = 0; i < 9; ++i)
            if (coded_is(coded, kill_codes[i].name)) { o->cheat(o->context, kill_codes[i].code); return 1; }
    if (coded_is(coded, "HARI KARI")) { o->cheat(o->context, 0x65); return 1; }
    if (coded_is(coded, "RESIGN")) { o->cheat(o->context, 0x67); return 1; }
    if (coded_is(coded, "GAIA")) {
        if (o->flag_41b750) {
            o->gaia(o->context, o->edit_mode == 6 && o->field_1b8 == 1);
            return 1;
        }
        o->cheat(o->context, -1);
        return 1;
    }
    for (i = 0; i < 7; ++i)
        if (coded_is(coded, late_codes[i].name)) { o->cheat(o->context, late_codes[i].code); return 1; }
    if (coded_is(coded, "NO FOG")) { o->cheat(o->context, 0xe7); return 1; }
    /* The original also scans the first word (sscanf "%s ") and discards it. */
    if (coded_is(coded, "ZEUS")) { o->player_54(o->context, player); return 1; }
    return 0;
}

/* 0x00501980 */
void aoe_game_ror_notify_ror(const AoeGameNotifyOpsRor *o, int32_t kind, int32_t a, int32_t b,
                             int32_t c, int32_t d) {
    char fmt[0x100], text[0x200];
    int ours = a == o->viewed_player, i;
    if (!o->has_game_screen) return;
#define GN_SOUND(n) o->step(o->context, AOE_GN_SOUND, (n), 0, NULL)
#define GN_NAMED(id, who) do { o->string(o->context, (id), fmt, sizeof fmt); \
        snprintf(text, sizeof text, fmt, o->player_name(o->context, (who))); } while (0)
    switch (kind) {
    case 1: case 2: case 3:
        if (!ours) return;
        GN_SOUND(3);
        o->format(o->context, kind == 1 ? 0x66 : kind == 2 ? 0x67 : 0x69, 0, text, sizeof text);
        o->step(o->context, AOE_GN_FIRST_LINE, 0, 0, text);
        return;
    case 4:
        if (ours || o->dropped[a]) return;
        o->dropped[a] = 1;
        GN_SOUND(7);
        if (a > 0) { GN_NAMED(0xbc4, a); o->step(o->context, AOE_GN_CHAT, 0, 0, text); }
        return;
    case 5:
        if (ours || a <= 0) return;
        GN_SOUND(6);
        GN_NAMED(b == 0 ? 0xbc5 : 0xbc4, a);
        o->step(o->context, AOE_GN_CHAT, 0, 0, text);
        return;
    case 6:
        if (ours) return;
        GN_SOUND(6);
        if (a > 0) { GN_NAMED(0xbc6, a); o->step(o->context, AOE_GN_CHAT, 0, 0, text); }
        return;
    case 7:
        if (b != o->viewed_player) return;
        GN_SOUND(4);
        o->string(o->context, c == 0 ? 0xbc8 : c == 1 ? 0xbc7 : c == 3 ? 0xbc1 : 0xbc9, fmt, sizeof fmt);
        snprintf(text, sizeof text, fmt, o->player_name(o->context, a), (int)a);
        o->step(o->context, AOE_GN_CHAT, 0, 0, text);
        return;
    case 8:
        o->step(o->context, AOE_GN_TIMERS, 0, 0, NULL);
        if (b != o->viewed_player) return;
        fmt[0] = '\0';
        if (c == 0) o->string(o->context, 0xbd2, fmt, sizeof fmt);
        else if (c == 1) o->string(o->context, 0xbd1, fmt, sizeof fmt);
        else if (c == 3) o->string(o->context, 0xbd3, fmt, sizeof fmt);
        snprintf(text, sizeof text, fmt, o->player_name(o->context, a));
        o->step(o->context, AOE_GN_CHAT, 0, 0, text);
        return;
    case 100:
        if (!ours) return;
        GN_SOUND(b == 0x65 || b == 0x66 || b == 0x67 ? 5 : 0xf);
        o->step(o->context, AOE_GN_BUTTONS, 0, 0, NULL);
        o->step(o->context, AOE_GN_VIEW_REMEMBER, c, d, NULL);
        return;
    case 0x66: case 0x69:
        if (!ours) return;
        o->step(o->context, AOE_GN_BUTTONS, 0, 0, NULL);
        o->step(o->context, AOE_GN_VIEW_REMEMBER, c, d, NULL);
        return;
    case 0x68: case 0x7d:
        if (!ours) return;
        if (kind == 0x68) GN_SOUND(3);
        o->format(o->context, kind == 0x68 ? 1000 : 1, b, text, sizeof text);
        o->step(o->context, AOE_GN_FIRST_LINE, 0, 0, text);
        return;
    case 0x6b:
        if (!ours) return;
        GN_SOUND(3);
        o->format(o->context, 0x6a, 0, text, sizeof text);
        o->step(o->context, AOE_GN_FIRST_LINE, 0, 0, text);
        return;
    case 0x6c:
        GN_SOUND(8);
        if (ours) o->string(o->context, 0xbf2, text, sizeof text);
        else GN_NAMED(0xbcb, a);
        o->step(o->context, AOE_GN_CHAT, 0, 0, text);
        o->step(o->context, AOE_GN_VIEW_REMEMBER, c, d, NULL);
        return;
    case 0x6d: case 0x6e: {
        int32_t id;
        if (kind == 0x6e && o->world_108) return;
        GN_SOUND(kind == 0x6d ? 9 : 10);
        if (ours) {
            id = kind == 0x6d ? (o->world_108 ? 0xbee : 0xbcc) : 0xbcf;
            o->string(o->context, id, text, sizeof text);
        } else {
            int allied = o->player_108(o->context, a) && !o->stance(o->context, o->viewed_player) &&
                         o->player_108(o->context, o->viewed_player) && !o->stance(o->context, a);
            if (kind == 0x6d) id = o->world_108 ? 0xbef : allied ? 0xbcd : 0xbce;
            else id = 0xbd0;
            GN_NAMED(id, a);
        }
        o->step(o->context, AOE_GN_VIEW_REMEMBER, c, d, NULL);
        o->step(o->context, AOE_GN_CHAT, 0, 0, text);
        o->step(o->context, AOE_GN_TIMERS, 0, 0, NULL);
        return;
    }
    case 0x72: case 0x74: {
        int32_t id;
        GN_SOUND(0xb);
        if (o->world_108) id = kind == 0x72 ? 0xbf0 : 0xbf1;
        else if (ours) id = kind == 0x72 ? 0xbd4 : 0xbd9;
        else {
            int allied = o->player_108(o->context, a) && !o->stance(o->context, o->viewed_player) &&
                         o->player_108(o->context, o->viewed_player) && !o->stance(o->context, a);
            id = allied ? (kind == 0x72 ? 0xbd4 : 0xbd9) : (kind == 0x72 ? 0xbd5 : 0xbda);
        }
        o->string(o->context, id, text, sizeof text);
        o->step(o->context, AOE_GN_CHAT, 0, 0, text);
        o->step(o->context, AOE_GN_TIMERS, 0, 0, NULL);
        for (i = 1; i < o->player_count; ++i) o->step(o->context, AOE_GN_TELL_PLAYER, i, a, NULL);
        return;
    }
    case 0x73: case 0x75:
        if (o->world_108) return;
        GN_SOUND(0xc);
        o->string(o->context, kind == 0x73 ? 0xbd6 : 0xbdb, text, sizeof text);
        o->step(o->context, AOE_GN_CHAT, 0, 0, text);
        o->step(o->context, AOE_GN_TIMERS, 0, 0, NULL);
        return;
    case 0x76: case 0x77: case 0x78: case 0x79: case 0x7a:
        if (!ours) return;
        GN_SOUND(3);
        o->string(o->context, 0xbe7 + (kind - 0x76), text, sizeof text);
        o->step(o->context, AOE_GN_FIRST_LINE, 0, 0, text);
        return;
    case 0x7b:
        if (!ours && b == o->viewed_player) GN_SOUND(0xd);
        o->step(o->context, AOE_GN_VIEW_REMEMBER, c, d, NULL);
        return;
    case 0x7c:
        if (ours) o->step(o->context, AOE_GN_BUTTONS, 0, 0, NULL);
        return;
    case 0x7e:
        if (!ours) return;
        GN_SOUND(0x10);
        o->step(o->context, AOE_GN_VIEW_REMEMBER, c, d, NULL);
        return;
    default:
        return;
    }
#undef GN_SOUND
#undef GN_NAMED
}

/* 0x004fe600 */
int aoe_game_ror_init_resources_ror(const AoeGameRorInitOpsRor *o) {
    static const uint8_t cycle[21] = {0x17, 0x27, 0x7c, 0x27, 0x3f, 0x90, 0x3f, 0x5f, 0x9f, 0x57, 0x7b,
                                      0xb4, 0x3f, 0x5f, 0xa0, 0x27, 0x3f, 0x91, 0x17, 0x27, 0x7b};
    char upper[0x100], dll[0x40], text[0x300], name[0x104];
    const char *p;
    int32_t join, bytes, month, year, hour, minute, second;
    int i;
    if (*o->error) return 0;
    snprintf(upper, sizeof upper, "%.255s", o->options);
    for (i = 0; upper[i]; ++i) upper[i] = (char)toupper((unsigned char)upper[i]);
    snprintf(dll, sizeof dll, "languagex.dll");
    p = strstr(upper, "STRING=");
    if (p) {
        int n = 0;
        while (*p != '=') ++p;
        ++p;
        while (*p && !isspace((unsigned char)*p) && n < (int)sizeof dll - 1) dll[n++] = *p++;
        dll[n] = '\0';
    }
    if (!o->load_library(o->context, dll)) { *o->error = 1; return 0; }
    if (!o->file_exists(o->context, "empires.exe")) { *o->error = 0x17; return 0; }
    o->register_drs(o->context, "sounds.drs", "tribe", o->data_dir, 1);
    o->register_drs(o->context, "graphics.drs", "tribe", o->data_dir, 0);
    o->register_drs(o->context, "Interfac.drs", "tribe", o->data_dir, 0);
    o->register_drs(o->context, "sounds.drs", "tribe", "data\\", 1);
    o->register_drs(o->context, "graphics.drs", "tribe", "data\\", 0);
    o->register_drs(o->context, "Terrain.drs", "tribe", "data\\", 0);
    o->register_drs(o->context, "Border.drs", "tribe", "data\\", 0);
    o->register_drs(o->context, "Interfac.drs", "tribe", "data\\", 0);
    if (!o->step_4166b0(o->context)) return 0;
    o->palette_entries(o->context, 0xf8, 7, cycle);
    o->input_window(o->context);
    o->codec_check(o->context);
    if (!o->lobby_switch) {
        join = o->lobby_join(o->context);
        if (join != 1) {
            int ok;
            if (o->editor_name[0]) ok = o->open_editor(o->context, o->editor_name);
            else if (o->saved_name[0]) ok = o->start_file(o->context, o->saved_name);
            else if (!o->settings_890) { o->play_video(o->context, "logo1"); ok = 1; }
            else return o->main_menu(o->context) ? (o->after(o->context), 1) : 0;
            if (!ok) {
                if (!o->main_menu(o->context)) return 0;
                if (o->screen_exists(o->context, "Main Menu")) o->ok_dialog(o->context, 0x961);
            }
        } else {
            o->set_flag_97e(o->context);
            o->new_mp_setup(o->context);
            o->show(o->context, "MP Setup Screen");
            o->remove(o->context, "Status Screen");
        }
    } else {
        o->new_status(o->context);
        o->show(o->context, "Status Screen");
        join = o->lobby_join(o->context);
        if (join == 1) {
            o->set_flag_97e(o->context);
            if (!o->new_mp_setup(o->context)) return 0;
            o->show(o->context, "MP Setup Screen");
        } else if (join != -1) {
            o->close_window(o->context);
            return 1;
        } else {
            if (!o->new_error_screen(o->context, 0x96a)) return 0;
            o->show(o->context, "Main Error Screen");
        }
        o->remove(o->context, "Status Screen");
    }
    o->after(o->context);
    o->log(o->context, "------------------------------", 0);
    o->log(o->context, "", 1);
    if (o->file_info(o->context, 0, name, sizeof name, &bytes, &month, &year, &hour, &minute, &second)) {
        snprintf(text, sizeof text, "program=%s, size=%d, date=%d/%d/%d, time=%d:%d:%d", name, (int)bytes,
                 (int)month, (int)month, (int)year, (int)hour, (int)minute, (int)second);
        o->log(o->context, text, 0);
    }
    if (o->file_info(o->context, 1, name, sizeof name, &bytes, &month, &year, &hour, &minute, &second)) {
        snprintf(text, sizeof text, "data=%s, size=%d, date=%d/%d/%d, time=%d:%d:%d", name, (int)bytes,
                 (int)month, (int)month, (int)year, (int)hour, (int)minute, (int)second);
        o->log(o->context, text, 0);
    }
    snprintf(text, sizeof text, "options=%.255s", o->options);
    o->log(o->context, text, 0);
    return 1;
}

/* 0x00503100 */
int aoe_game_ror_setup_random_game_ror(const AoeGameRorRandomOpsRor *o) {
    AoeGameSettingsRor *g = o->settings;
    AoeRandomPlayerRor players[8];
    AoeRandomStartRor start;
    int32_t order[9], i, s, count, next = 0, n = 0, ok, seed;
    int networked = aoe_settings_flag_97e_ror(g) != 0, scenario = 0;
    int campaign = aoe_settings_int_9a0_ror(g) != 0, scenario_mode = aoe_settings_flag_8fc_ror(g) != 0;
    *o->busy = 0;
    o->reset_fields(o->context);
    memset(&start, 0, sizeof start);
    memset(players, 0, sizeof players);
    if (aoe_settings_byte_1190_ror(g)) {
        int32_t type;
        if (o->global_7c0640 == 1) { aoe_settings_set_map_size_ror(g, 2); type = 4; }
        else {
            aoe_settings_set_map_size_ror(g, 3);
            type = o->rand_(o->context) % 10;
            if (type < 2) type = 4; else if (type == 2) type = 3; else if (type == 3) type = 2;
            else if (type == 4) type = 1; else if (type != 5 && type != 6 && type != 7) type = type == 8 ? 8 : 0;
        }
        aoe_settings_set_int_a80_ror(g, type);
        o->clear_names(o->context);
        count = aoe_settings_byte_988_ror(g);
        for (i = 0; i < count; ++i) {
            int32_t civ = (o->rand_(o->context) & 0xf) + 1;
            aoe_settings_set_byte_a94_ror(g, i, (uint8_t)civ);
            aoe_settings_set_byte_acd_ror(g, i, (uint8_t)o->pick_name(o->context, civ));
        }
    }
    if (!campaign) {
        if (!scenario_mode) {
            start.kind = 0;
            start.map_type = aoe_settings_int_a80_ror(g);
            start.map_w = aoe_settings_map_width_ror(g);
            start.map_h = aoe_settings_map_height_ror(g);
        } else {
            const char *nm = aoe_settings_scenario_name_ror(g);
            start.kind = 1;
            snprintf(start.path, sizeof start.path, "%s%s.scn", o->scenario_dir, nm);
            if (!o->file_exists(o->context, start.path)) snprintf(start.path, sizeof start.path, "%s.scx", nm);
            else snprintf(start.path, sizeof start.path, "%s.scn", nm);
        }
    } else {
        start.kind = 2;
    }
    if (!networked) { seed = o->own_seed; if (seed == -1) seed = o->rand_(o->context); }
    else seed = o->comm_seed;
    o->srand_(o->context, seed);
    for (i = 0; i < 9; ++i) {
        int32_t civ = aoe_settings_byte_a94_ror(g, i);
        if (civ > 16 || civ < 0) aoe_settings_set_byte_a94_ror(g, i, (uint8_t)(o->rand_(o->context) * 16 / 0x7fff + 1));
    }
    o->clear_names(o->context);
    count = aoe_settings_byte_988_ror(g);
    for (i = 0; i < count; ++i)
        if (o->slot_type(o->context, i + 1) == 4)
            aoe_settings_set_byte_acd_ror(g, i, (uint8_t)o->pick_name(o->context, aoe_settings_byte_a94_ror(g, i)));
    if (campaign || scenario_mode) {
        scenario = o->scenario_load(o->context, campaign ? NULL : start.path, campaign);
        if (!scenario) return 0;
    }
    for (i = 0; i < 9; ++i) { order[i] = -1; o->set_slot_player(o->context, i, 0); }
    for (s = 1; s <= count; ++s) {
        int32_t type = o->slot_type(o->context, s);
        if (type == 2) {
            int32_t c = aoe_settings_byte_ac4_ror(g, s - 1) - 1;
            if (c < 0 || c > 7) continue;
            if (order[c] == -1) order[c] = next++;
            o->set_slot_player(o->context, s, order[c] + 1);
        } else if (type == 4) {
            int32_t c;
            for (c = 0; c < 8; ++c) {
                int32_t h, taken = 0;
                if (order[c] != -1) continue;
                for (h = 1; h <= 8; ++h)
                    if (o->slot_type(o->context, h) == 2 && aoe_settings_byte_ac4_ror(g, h - 1) - 1 == c) { taken = 1; break; }
                if (taken) continue;
                aoe_settings_set_byte_ac4_ror(g, s - 1, (uint8_t)(c + 1));
                order[c] = next++;
                o->set_slot_player(o->context, s, order[c] + 1);
                break;
            }
        }
    }
    o->clear_names(o->context);
    for (i = 0; i < 8; ++i) {
        int32_t slot = 0, type, color;
        AoeRandomPlayerRor *p = &players[n];
        for (s = 1; s <= 8; ++s) if (o->slot_player(o->context, s) == i + 1) { slot = s; break; }
        if (!slot) break;
        type = o->slot_type(o->context, slot);
        color = aoe_settings_byte_ac4_ror(g, slot - 1);
        p->kind = (type == 4 || o->global_580de0 == 1 || (o->global_580de0 == 2 && i > 0)) ? 4 : 0;
        p->civ = (uint8_t)(campaign ? o->scenario_civ(o->context, color - 1) : aoe_settings_byte_a94_ror(g, slot - 1));
        p->name[0] = '\0';
        if (networked && type == 2) {
            snprintf(p->name, sizeof p->name, "%.64s", o->comm_name(o->context, slot));
        } else {
            if (!campaign && aoe_settings_flag_8fc_clear_ror(g) && p->kind == 0)
                o->string(o->context, 0x286d, p->name, sizeof p->name);
            else if (campaign || scenario_mode)
                snprintf(p->name, sizeof p->name, "%.64s", o->scenario_name(o->context, color - 1));
            else if (p->kind == 0)
                snprintf(p->name, sizeof p->name, "%.64s", o->settings_name(o->context));
            if (!p->name[0]) {
                if (p->kind == 0) o->string(o->context, 0x286d, p->name, sizeof p->name);
                else if (!campaign && aoe_settings_byte_acd_ror(g, slot - 1))
                    o->string(o->context, 0x1130 + aoe_settings_byte_acd_ror(g, slot - 1), p->name, sizeof p->name);
                else o->string(o->context, 0x1130 + o->pick_name(o->context, p->civ), p->name, sizeof p->name);
            }
            o->set_comm_name(o->context, slot, p->name);
        }
        p->color = color;
        p->position = (int16_t)(color - 1);
        ++n;
    }
    if (scenario) o->scenario_delete(o->context);
    if (n == 0) return 0;
    if (o->has_sound) o->sound_hold(o->context, 1);
    ok = o->world_start(o->context, players, n, &start, o->slot_player(o->context, networked ? o->our_slot : 1));
    if (ok && o->world_player_count(o->context) < 2) { ok = 0; o->world_stop(o->context); }
    if (o->has_sound) o->sound_hold(o->context, 0);
    for (i = 0; i < o->world_player_count(o->context); ++i) o->clear_wonder(o->context, i);
    o->refresh_world(o->context);
    if (aoe_settings_byte_985_ror(g))
        for (i = 1; i < o->world_player_count(o->context); ++i)
            if (o->owner_kind(o->context, i) == 0) o->explore_all(o->context, i);
    {
        uint8_t pf = networked ? aoe_settings_byte_983_ror(g) : aoe_settings_byte_a14_ror(g);
        *o->global_554738 = pf == 1 ? 5000 : pf == 2 ? 7500 : 2500;
    }
    for (s = 1; s < 9; ++s) o->slot_types[s - 1] = o->slot_type(o->context, s);
    return ok;
}

/* ---- 0x00505a30 ---- */
const char *const aoe_game_ror_comm_screen_names_ror[3] = {"Temp Screen", "Multiplayer Wait Screen",
                                                           "Status Screen"};
#define GCC(id, a, t) o->step(o->context, (id), (int32_t)(a), (t))
int aoe_game_ror_comm_command_ror(const AoeGameCommCommandOpsRor *o, int32_t code, int32_t slot) {
    if (o->mode != 3) {
        if (o->mode == 4 || o->mode == 5 || o->mode == 6) {
            switch (code) {
            case 0x17a2: if (o->has_game_screen) GCC(AOE_GCC_SHOW_PAUSED, 0, NULL); break;
            case 0x17a3: if (o->has_game_screen) GCC(AOE_GCC_HIDE_PAUSED, 0, NULL); break;
            case 0x17b0:
                if (*o->global_580e24 == 0 &&
                    o->odd_one_out(o->context, o->own_slot(o->context), slot)) {
                    *o->global_580e24 = 1;
                    *o->global_580e28 = 1;
                    GCC(AOE_GCC_TEMP_SCREEN, 0, NULL);
                    GCC(AOE_GCC_DELETE_SCREENS, 1, NULL);
                    GCC(AOE_GCC_PLAYER_45D440, 0, NULL);
                    GCC(AOE_GCC_LEAVE, 0, NULL);
                    if (o->screen_present(o->context)) GCC(AOE_GCC_OK_DIALOG, 0x966, NULL);
                    GCC(AOE_GCC_REMOVE_SCREEN, 0, NULL);
                }
                break;
            case 0x17b2:
                if (o->has_game_screen) {
                    GCC(AOE_GCC_PLAYER_45D440, 0, NULL);
                    GCC(AOE_GCC_LEAVE, 0, NULL);
                    if (o->screen_present(o->context)) GCC(AOE_GCC_OK_DIALOG, 0x25c9, NULL);
                }
                break;
            case 0x17bd: {
                int32_t p = o->slot_player(o->context, slot);
                if (p != o->viewed_player && o->has_game_screen && p > 0 && o->dropped[slot] == 0) {
                    char fmt[0x100], text[0x100];
                    const char *name = o->player_name(o->context, p);
                    GCC(AOE_GCC_SOUND, 2, NULL);
                    fmt[0] = '\0';
                    o->string(o->context, 0x4c1, fmt, sizeof fmt);
                    snprintf(text, sizeof text, fmt, name);
                    GCC(AOE_GCC_CHAT, 0, text);
                    GCC(AOE_GCC_LOG, 0, name);
                }
                break;
            }
            case 0x17d5:
                if (o->has_game_screen) {
                    GCC(AOE_GCC_SOUND, 2, NULL);
                    GCC(AOE_GCC_CHAT_LINE, slot, NULL);
                }
                break;
            default: break;
            }
        }
        return 1;
    }
    switch (code) {
    case 0x17ac: case 0x17b7:
        GCC(AOE_GCC_VTABLE_44, 0, NULL);
        GCC(AOE_GCC_FLAG_97D, 1, NULL);
        /* fall through */
    case 0x17a8: case 0x17a9: case 0x17aa: case 0x17ab: case 0x17b2: case 0x17b6:
        GCC(AOE_GCC_TO_MENUS, 0, NULL);
        GCC(AOE_GCC_REMOVE_SCREEN, 1, NULL);
        GCC(AOE_GCC_REMOVE_SCREEN, 2, NULL);
        if (o->screen_present(o->context)) GCC(AOE_GCC_OK_DIALOG, 0x25c9, NULL);
        break;
    default: break;
    }
    return 1;
}
