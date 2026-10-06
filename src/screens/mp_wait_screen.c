/* Patched RoR 1.0a "Multiplayer Wait Screen" (vtable 0x00546ea8,
   0x0048d3a0..0x0048de70). */
#include "mp_wait_screen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define WS(p) ((AoeMpWaitScreenRor *)(p))

static const char *const NAME = "Multiplayer Wait Screen";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

static void ws_destroy(AoeUiPanelRor *p) {
    AoeMpWaitScreenRor *s = WS(p);
    int i;
    DROP(s->title);
    DROP(s->hidden_close);
    DROP(s->header);
    for (i = 0; i < 20; ++i) DROP(s->line[i]);
    aoe_ui_ez_release_ror(&s->ez);
}

static int32_t ws_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                         int32_t extra) {
    AoeMpWaitScreenRor *s = WS(p);
    const AoeGameUiOpsRor *ops = s->ui->ops;
    if (from && s->hidden_close && from == &s->hidden_close->panel && code == 1) {
        if (ops && ops->quit_program) ops->quit_program(ops->context);
        return 1;
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

static void str(AoeMpWaitScreenRor *s, int32_t id, char *out, int size) {
    out[0] = '\0';
    aoe_ui_string_ror(s->ez.panel.system, id, out, size);
}

/* A line: `fmt_id`'s string formatted with `arg`, set and shown. */
static void line_text(AoeMpWaitScreenRor *s, int32_t fmt_id, const char *arg) {
    char fmt[0x100], text[0x200];
    if (s->lines_used >= 20) return;
    str(s, fmt_id, fmt, sizeof fmt);
    snprintf(text, sizeof text, fmt, arg);
    aoe_ui_text_set_text_ror(s->line[s->lines_used], text);
    s->line[s->lines_used]->panel.vt->set_active(&s->line[s->lines_used]->panel, 1);
    ++s->lines_used;
}

static void line_number(AoeMpWaitScreenRor *s, int32_t fmt_id, int32_t value) {
    char fmt[0x100], text[0x200];
    if (s->lines_used >= 20) return;
    str(s, fmt_id, fmt, sizeof fmt);
    snprintf(text, sizeof text, fmt, (long)value);
    aoe_ui_text_set_text_ror(s->line[s->lines_used], text);
    s->line[s->lines_used]->panel.vt->set_active(&s->line[s->lines_used]->panel, 1);
    ++s->lines_used;
}

/* A line whose argument is itself a string of the table. */
static void line_id(AoeMpWaitScreenRor *s, int32_t fmt_id, int32_t arg_id) {
    char arg[0x100];
    str(s, arg_id, arg, sizeof arg);
    line_text(s, fmt_id, arg);
}

static void fill(AoeMpWaitScreenRor *s) {
    const AoeGameSettingsRor *g = s->ui->settings;
    const AoeGameUiOpsRor *ops = s->ui->ops;
    int networked = ops && ops->networked && ops->networked(ops->context);
    char arg[0x100], fmt[0x100], text[0x200];
    int32_t v;
    if (aoe_settings_flag_8fc_ror(g)) {
        str(s, 0x25ed, fmt, sizeof fmt);
        snprintf(text, sizeof text, fmt, aoe_settings_scenario_name_ror(g));
        aoe_ui_text_set_text_ror(s->header, text);
        s->lines_used = 3;
    } else {
        str(s, aoe_settings_option_ae5_ror(g) ? 0x2617 : 0x25b5, arg, sizeof arg);
        str(s, 0x25ed, fmt, sizeof fmt);
        snprintf(text, sizeof text, fmt, arg);
        aoe_ui_text_set_text_ror(s->header, text);
        s->lines_used = 1;
    }
    if (aoe_settings_flag_8fc_clear_ror(g)) {
        v = aoe_settings_map_size_ror(g);
        if (v >= 0 && v <= 5) str(s, 0x2973 + v, arg, sizeof arg); else arg[0] = '\0';
        line_text(s, 0x25da, arg);
        v = aoe_settings_int_a80_ror(g);
        if (v >= 0 && v <= 8) str(s, 0x296a + v, arg, sizeof arg); else arg[0] = '\0';
        line_text(s, 0x25b6, arg);
    }
    ++s->lines_used; /* one line left empty */
    v = aoe_settings_int_a8c_ror(g);
    if (v == 1) {
        line_id(s, 0x25bc, 0x10e1);
    } else if (v == 7 || v == 8) {
        char a[0x100];
        str(s, v == 7 ? 0x10e9 : 0x10ea, arg, sizeof arg);
        str(s, 0x25be, fmt, sizeof fmt);
        snprintf(a, sizeof a, fmt, arg, (long)aoe_settings_int_a90_ror(g));
        if (s->lines_used < 20) {
            aoe_ui_text_set_text_ror(s->line[s->lines_used], a);
            s->line[s->lines_used]->panel.vt->set_active(&s->line[s->lines_used]->panel, 1);
            ++s->lines_used;
        }
    } else if (v == 9) {
        line_id(s, 0x25bc, 0x10ec);
    } else {
        line_id(s, 0x25bc, aoe_settings_flag_8fc_clear_ror(g) ? 0x10ec : 0x10e7);
    }
    v = aoe_settings_starting_age_ror(g);
    line_id(s, 0x25e4, v == 0 ? 0x10e7 : v == 1 ? 0x106e : 0x1067 + v);
    v = aoe_settings_resource_level_ror(g);
    line_id(s, 0x25e5, v == 0 ? 0x10e7 : 0x25e5 + v);
    line_id(s, 0x25e0, 0x2bd0 + aoe_settings_byte_993_ror(g));
    line_id(s, 0x25e9, aoe_settings_option_ad8_ror(g) ? 0xfa4 : 0xfa3);
    line_id(s, 0x25b8, aoe_settings_byte_985_ror(g) ? 0xfa3 : 0xfa4);
    line_id(s, 0x25ec, aoe_settings_option_ad9_ror(g) ? 0xfa3 : 0xfa4);
    if (networked) line_id(s, 0x25bb, aoe_settings_byte_982_ror(g) ? 0xfa3 : 0xfa4);
    line_id(s, 0x25f1,
            0x260e + (networked ? aoe_settings_byte_983_ror(g) : aoe_settings_byte_a14_ror(g)));
    line_number(s, 0x25f0, aoe_settings_population_limit_ror(g));
}

int aoe_mp_wait_screen_init_ror(AoeMpWaitScreenRor *s, AoeGameUiRor *ui, AoeUiAreaRor *area) {
    AoeUiEzRor *ez = &s->ez;
    int i;
    memset(s, 0, sizeof *s);
    s->ui = ui;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_mp_wait_screen_vtable_ror()->base;
    if (!aoe_ui_screen_setup_ror(ez, area, "scr3", 0xc385, 0)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    aoe_ui_ez_create_text_ror(ez, &ez->panel, &s->title, "", 0x14, 0x28, 0x17c, 0x190, 4, 1, 1, 1);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &s->hidden_close, 0x3ea, 0, 0, 0, 0, 0,
                                        -1, -1, 0))
        return 1;
    s->hidden_close->panel.vt->set_active(&s->hidden_close->panel, 1);
    s->hidden_close->panel.vt->set_position_mode(&s->hidden_close->panel, 9, 4, 4, 4, 4, 0x11,
                                                 0x11, 0x11, 0x11, NULL, NULL, NULL, NULL);
    if (!aoe_ui_ez_create_text_ror(ez, &ez->panel, &s->header, "", 0x1a4, 0x54, 0xdc, 0x44, 0xb,
                                   0, 0, 1))
        return 1;
    for (i = 0; i < 20; ++i)
        if (!aoe_ui_ez_create_text_ror(ez, &ez->panel, &s->line[i], "", 0x1a4, 0x53 + 0x18 * i,
                                       0xdc, 0x14, 0xb, 0, 0, 0))
            return 1;
    for (i = 0; i < 20; ++i) s->line[i]->panel.vt->set_active(&s->line[i]->panel, 0);
    if (ui->settings) fill(s);
    return 1;
}

static AoeUiEzVtblRor vtable;

const AoeUiEzVtblRor *aoe_mp_wait_screen_vtable_ror(void) {
    if (!vtable.base.destroy) {
        vtable = aoe_ui_screen_vtable_ror;
        vtable.base.destroy = ws_destroy;
        vtable.base.action = ws_action;
    }
    return &vtable;
}
