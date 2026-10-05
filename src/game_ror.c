#include "game_ror.h"

#include <ctype.h>
#include <math.h>
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
