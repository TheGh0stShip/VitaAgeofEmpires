/* Patched RoR 1.0a "Multiplayer Wait Screen" 0x0048d3a0..0x0048de70. */
#include "mp_wait_screen.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int32_t asked[64];
static int n_asked, quits, net = 1;

static int32_t width(void *c, uintptr_t f, const char *t, int32_t n) { (void)c; (void)f; (void)t; return n * 8; }
static void string(void *c, int32_t id, char *out, int size) {
    (void)c;
    if (n_asked < 64) asked[n_asked++] = id;
    snprintf(out, (size_t)size, "S%d", (int)id);
}
static int fonts(void *c, int32_t index, uintptr_t *font, int32_t *w, int32_t *h) {
    (void)c; *font = (uintptr_t)index + 1; *w = 8; *h = 12; return 1;
}
static int networked(void *c) { (void)c; return net; }
static void quit(void *c) { (void)c; ++quits; }

static int has(int32_t id) {
    int i;
    for (i = 0; i < n_asked; ++i) if (asked[i] == id) return 1;
    return 0;
}

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 640, 480};
    AoeUiEzOpsRor ez_ops = {0};
    AoeGameUiOpsRor ops = {0};
    static AoeGameSettingsRor g;
    static AoeGameUiRor ui;
    AoeMpWaitScreenRor *s;
    AoeUiPanelRor *p;
    int i, shown;

    pf.text_width = width;
    pf.string = string;
    sys.platform = &pf;
    ez_ops.font = fonts;
    ops.networked = networked;
    ops.quit_program = quit;
    ui.system = &sys;
    ui.ez_ops = &ez_ops;
    ui.ops = &ops;
    ui.settings = &g;

    /* A random map game: size 2, type 4, victory 7 (score 5000), age 2,
       resources 1, difficulty 3, options set, path finding 1. */
    aoe_settings_set_flag_8fc_ror(&g, 0);
    aoe_settings_set_option_ae5_ror(&g, 1);
    aoe_settings_set_map_size_ror(&g, 2);
    aoe_settings_set_int_a80_ror(&g, 4);
    aoe_settings_set_a8c_a90_ror(&g, 7, 5000);
    aoe_settings_set_starting_age_ror(&g, 2);
    aoe_settings_set_resource_level_ror(&g, 1);
    aoe_settings_set_byte_993_ror(&g, 3);
    aoe_settings_set_option_ad8_ror(&g, 1);
    aoe_settings_set_byte_985_ror(&g, 0);
    aoe_settings_set_option_ad9_ror(&g, 1);
    aoe_settings_set_byte_982_ror(&g, 1);
    aoe_settings_set_byte_983_ror(&g, 1);
    aoe_settings_set_population_ror(&g, 75);
    s = calloc(1, sizeof *s);
    n_asked = 0;
    assert(aoe_mp_wait_screen_init_ror(s, &ui, &area));
    p = &s->ez.panel;
    assert(strcmp(s->ez.info_name, "scr3") == 0 && s->ez.info_id == 0xc385);
    assert(s->title && s->title->panel.x == 0x14 && s->title->panel.width == 0x17c);
    assert(s->header->panel.x == 0x1a4 && s->header->panel.y == 0x54);
    assert(s->line[0]->panel.y == 0x53 && s->line[19]->panel.y == 0x53 + 0x18 * 19 &&
           s->line[3]->panel.width == 0xdc);
    assert(s->hidden_close && s->hidden_close->panel.active);
    /* The header: 0x2617 in 0x25ed; lines from 1: size, type, (skip),
       victory with value, age, resources, difficulty, three options, the
       networked option, path finding, population -> next free is 14. */
    assert(strcmp(s->header->lines->text, "S9709") == 0);
    assert(aoe_settings_flag_8fc_clear_ror(&g));
    assert(s->lines_used == 14);
    assert(!s->line[0]->panel.active && s->line[1]->panel.active && s->line[2]->panel.active &&
           !s->line[3]->panel.active && s->line[4]->panel.active && s->line[13]->panel.active &&
           !s->line[14]->panel.active);
    assert(strcmp(s->line[1]->lines->text, "S9690") == 0 &&  /* 0x25da map size */
           strcmp(s->line[2]->lines->text, "S9654") == 0 &&  /* 0x25b6 map type */
           strcmp(s->line[4]->lines->text, "S9662") == 0 &&  /* 0x25be victory + value */
           strcmp(s->line[13]->lines->text, "S9712") == 0);  /* 0x25f0 population */
    assert(has(0x2975) && has(0x296e) && has(0x10e9) && has(0x1069) && has(0x25e6) &&
           has(0x2bd3) && has(0xfa4) && has(0x25bb) && has(0x260f) && has(0x2617));
    for (shown = 0, i = 0; i < 20; ++i) shown += s->line[i]->panel.active;
    assert(shown == 12);
    /* The hidden close button closes the window; other actions fall
       through. */
    p->vt->action(p, &s->hidden_close->panel, 1, 0, 0);
    assert(quits == 1);
    p->vt->action(p, &s->header->panel, 1, 0, 0);
    assert(quits == 1);
    assert(aoe_ui_system_close_ror(&sys, "Multiplayer Wait Screen"));

    /* A scenario, not networked: the header names it, lines start at 3,
       victory 1 and default resources; no networked line. */
    aoe_settings_set_flag_8fc_ror(&g, 1);
    aoe_settings_set_scenario_name_ror(&g, "Hill.scn");
    aoe_settings_set_a8c_a90_ror(&g, 1, 0);
    aoe_settings_set_starting_age_ror(&g, 0);
    aoe_settings_set_resource_level_ror(&g, 0);
    aoe_settings_set_byte_a14_ror(&g, 2);
    net = 0;
    s = calloc(1, sizeof *s);
    n_asked = 0;
    assert(aoe_mp_wait_screen_init_ror(s, &ui, &area));
    assert(s->lines_used == (aoe_settings_flag_8fc_clear_ror(&g) ? 15 : 13));
    assert(has(0x10e1) && !has(0x25bb) && has(0x2610) && has(0x10e7));
    assert(!has(0x2617) && !has(0x25b5));
    assert(aoe_ui_system_close_ror(&sys, "Multiplayer Wait Screen"));
    free(sys.panels);
    return 0;
}
