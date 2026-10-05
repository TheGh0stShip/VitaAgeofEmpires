#include "game_ui.h"

#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define CFG(p) ((AoeConfigDialogRor *)(p))

static const char *const NAME = "Config Dialog";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

static void cfg_destroy(AoeUiPanelRor *p) {
    AoeConfigDialogRor *d = CFG(p);
    int i;
    DROP(d->title);
    DROP(d->speed_label);
    for (i = 0; i < 3; ++i) DROP(d->speed_text[i]);
    for (i = 0; i < 3; ++i) DROP(d->speed[i]);
    DROP(d->music_label);
    DROP(d->music_high);
    DROP(d->music_low);
    DROP(d->music);
    DROP(d->sound_label);
    DROP(d->sound_high);
    DROP(d->sound_low);
    DROP(d->sound);
    DROP(d->scroll_label);
    DROP(d->scroll_fast);
    DROP(d->scroll_slow);
    DROP(d->scroll);
    DROP(d->screen_label);
    for (i = 0; i < 3; ++i) DROP(d->screen_text[i]);
    for (i = 0; i < 3; ++i) DROP(d->screen[i]);
    DROP(d->mouse_label);
    for (i = 0; i < 2; ++i) DROP(d->mouse_text[i]);
    for (i = 0; i < 2; ++i) DROP(d->mouse[i]);
    DROP(d->flag_label);
    for (i = 0; i < 2; ++i) DROP(d->flag_text[i]);
    for (i = 0; i < 2; ++i) DROP(d->flag[i]);
    DROP(d->path_label);
    for (i = 0; i < 3; ++i) DROP(d->path_text[i]);
    for (i = 0; i < 3; ++i) DROP(d->path[i]);
    DROP(d->ok);
    DROP(d->cancel);
    aoe_ui_dialog_release_ror(&d->dialog);
}

static int label(AoeUiEzRor *ez, AoeUiTextRor **out, int32_t id, int32_t x, int32_t y, int32_t w,
                 int32_t h, int32_t middle) {
    return aoe_ui_ez_create_text_id_ror(ez, &ez->panel, out, id, x, y, w, h, 0xb, 0, 1, middle);
}

/* A row of radio buttons with their texts, grouped. */
static int radios(AoeUiEzRor *ez, AoeUiButtonRor **buttons, AoeUiTextRor **texts,
                  const int32_t *ids, int n, int32_t bx, int32_t tx, int32_t tw) {
    int i;
    for (i = 0; i < n; ++i) {
        int32_t y = i * 0x23 + 0xe1;
        if (!label(ez, &texts[i], ids[i], tx, y, tw, 0x1e, 0) ||
            !aoe_ui_ez_create_radio_ror(ez, &ez->panel, &buttons[i], bx, y, 0x1e, 0x1e, 0, 0))
            return 0;
    }
    for (i = 0; i < n; ++i) aoe_ui_button_set_group_ror(buttons[i], buttons, (int16_t)n);
    return 1;
}

static int bar(AoeUiEzRor *ez, AoeUiValueScrollbarRor **out, int32_t x) {
    return aoe_ui_ez_create_value_scrollbar_ror(ez, &ez->panel, out, 0, 100, 10, 0x14, x, 0x55,
                                                0x14, 0x64);
}

int aoe_config_dialog_init_ror(AoeConfigDialogRor *d, AoeGameUiRor *ui, int32_t mode,
                               AoeUiPanelRor *parent) {
    static const int32_t screen_ids[3] = {0x24e7, 0x24e8, 0x24e9};
    static const int32_t mouse_ids[2] = {0x24eb, 0x24ec};
    static const int32_t flag_ids[2] = {0x24ee, 0x24ef};
    static const int32_t path_ids[3] = {0x260e, 0x260f, 0x2610};
    const AoeGameUiOpsRor *ops = ui->ops;
    AoeUiEzRor *ez = &d->dialog.ez, *pez = (AoeUiEzRor *)parent;
    AoeUiPanelRor *tab[2];
    int networked, i;
    float speed;
    memset(d, 0, sizeof *d);
    d->ui = ui;
    d->mode = mode;
    aoe_ui_dialog_init_ror(&d->dialog, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_config_dialog_vtable_ror.base;
    if (!parent ||
        !aoe_ui_dialog_setup_ror(&d->dialog, parent->area, parent, 600, 0x186,
                                 aoe_ui_ez_popup_name_ror(pez), aoe_ui_ez_popup_id_ror(pez), 1) ||
        !aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &d->title, 0x24d7, 0x67, 10, 0x18b, 0x1e, 1,
                                      1, 1, 0) ||
        !label(ez, &d->speed_label, 0x24df, 0x14, 0x32, 0x6e, 0x1e, 0))
        return 1;
    for (i = 0; i < 3; ++i) {
        int32_t y = i * 0x23 + 0x55;
        if (!label(ez, &d->speed_text[i], 0x24d8 + i, 0x37, y, 0x6e, 0x1e, 0) ||
            !aoe_ui_ez_create_radio_ror(ez, &ez->panel, &d->speed[i], 0x14, y, 0x1e, 0x1e, 0, 0))
            return 1;
    }
    for (i = 0; i < 3; ++i) aoe_ui_button_set_group_ror(d->speed[i], d->speed, 3);
    if (ops && ops->game_state && ops->game_state(ops->context) == 5)
        for (i = 0; i < 3; ++i) aoe_ui_button_set_disabled_ror(d->speed[i], 1);
    speed = ops && ops->game_speed ? ops->game_speed(ops->context) : 1.0f;
    d->speed_old = speed;
    if (speed == 2.0f) aoe_ui_button_select_ror(d->speed[2]);
    else if (speed == 1.5f) aoe_ui_button_select_ror(d->speed[1]);
    else if (speed == 1.0f) aoe_ui_button_select_ror(d->speed[0]);
    if (!label(ez, &d->music_label, 0x24db, 0xa5, 0x2d, 0x6e, 0x28, 1) ||
        !label(ez, &d->music_high, 0x24dc, 0xbe, 0x55, 0x6e, 0x14, 0) ||
        !label(ez, &d->music_low, 0x24dd, 0xbe, 0xa5, 0x6e, 0x14, 0) || !bar(ez, &d->music, 0xaa))
        return 1;
    d->music_old = ops && ops->music_level ? ops->music_level(ops->context) : 99;
    aoe_ui_scrollbar_set_position_ror(&d->music->bar, d->music_old);
    d->music_now = d->music_old;
    if (!label(ez, &d->sound_label, 0x24de, 0x131, 0x2d, 0x6e, 0x28, 1) ||
        !label(ez, &d->sound_high, 0x24dc, 0x14a, 0x55, 0x6e, 0x14, 0) ||
        !label(ez, &d->sound_low, 0x24dd, 0x14a, 0xa5, 0x6e, 0x14, 0) || !bar(ez, &d->sound, 0x136))
        return 1;
    d->sound_old = ops && ops->sound_level ? ops->sound_level(ops->context) : 99;
    aoe_ui_scrollbar_set_position_ror(&d->sound->bar, d->sound_old);
    d->sound_now = d->sound_old;
    if (!label(ez, &d->scroll_label, 0x24f0, 0x1bd, 0x2d, 0x6e, 0x28, 1) ||
        !label(ez, &d->scroll_fast, 0x24f1, 0x1d6, 0x55, 0x6e, 0x14, 0) ||
        !label(ez, &d->scroll_slow, 0x24f2, 0x1d6, 0xa5, 0x6e, 0x14, 0) ||
        !bar(ez, &d->scroll, 0x1c2))
        return 1;
    d->scroll_old = ops && ops->scroll_speed ? ops->scroll_speed(ops->context) : 10;
    aoe_ui_scrollbar_set_position_ror(&d->scroll->bar, d->scroll_old - 10);
    if (!label(ez, &d->screen_label, 0x24e0, 0x14, 0xbe, 200, 0x1e, 0)) return 1;
    for (i = 0; i < 3; ++i) {
        int32_t y = i * 0x23 + 0xe1;
        if (!label(ez, &d->screen_text[i], screen_ids[i], 0x37, y, 100, 0x1e, 0) ||
            !aoe_ui_ez_create_radio_ror(ez, &ez->panel, &d->screen[i], 0x14, y, 0x1e, 0x1e, 0, 0))
            return 1;
    }
    for (i = 0; i < 3; ++i) aoe_ui_button_set_group_ror(d->screen[i], d->screen, 3);
    {
        static const int32_t widths[3] = {0x280, 800, 0x400};
        int32_t w = ops && ops->screen_width ? ops->screen_width(ops->context) : 0x280;
        for (i = 0; i < 3; ++i)
            if (w == widths[i]) aoe_ui_button_select_ror(d->screen[i]);
        for (i = 0; i < 3; ++i)
            if (!ops || !ops->screen_mode_available ||
                !ops->screen_mode_available(ops->context, widths[i]))
                aoe_ui_button_set_disabled_ror(d->screen[i], 1);
    }
    if (!label(ez, &d->mouse_label, 0x24ea, 0xaa, 0xbe, 0xd2, 0x1e, 0) ||
        !radios(ez, d->mouse, d->mouse_text, mouse_ids, 2, 0xaa, 0xcd, 0x78))
        return 1;
    aoe_ui_button_select_ror(ops && ops->mouse_buttons && ops->mouse_buttons(ops->context) == 2
                                 ? d->mouse[0]
                                 : d->mouse[1]);
    if (!label(ez, &d->flag_label, 0x24ed, 0x14a, 0xbe, 0xd2, 0x1e, 0) ||
        !radios(ez, d->flag, d->flag_text, flag_ids, 2, 0x14a, 0x16d, 0x78))
        return 1;
    aoe_ui_button_select_ror(ops && ops->game_flag_a08 && ops->game_flag_a08(ops->context)
                                 ? d->flag[0]
                                 : d->flag[1]);
    if (!label(ez, &d->path_label, 0x260d, 0x1cc, 0xbe, 0xd2, 0x1e, 0) ||
        !radios(ez, d->path, d->path_text, path_ids, 3, 0x1cc, 0x1ef, 0x78))
        return 1;
    networked = ops && ops->networked && ops->networked(ops->context);
    if (networked)
        for (i = 0; i < 3; ++i) aoe_ui_button_set_disabled_ror(d->path[i], 1);
    if (ui->settings) {
        uint8_t level = networked ? aoe_settings_byte_983_ror(ui->settings)
                                  : aoe_settings_byte_a14_ror(ui->settings);
        if (level < 3) aoe_ui_button_select_ror(d->path[level]);
    }
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->ok, 0xfa1, 0, 0x78, 0x159, 0xaa, 0x1e,
                                        0, 0, 0) ||
        !aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->cancel, 0xfa2, 0, 0x136, 0x159, 0xaa,
                                        0x1e, 0, 0, 0))
        return 1;
    d->cancel->hotkey = 0x1b;
    d->cancel->hotkey_shift = 0;
    aoe_ui_panel_set_current_ror(&ez->panel, &d->ok->panel);
    tab[0] = &d->ok->panel;
    tab[1] = &d->cancel->panel;
    aoe_ui_panels_tab_cycle_ror(tab, 2);
    return 1;
}

static int on(const AoeUiButtonRor *b) { return b && aoe_ui_button_state_ror(b) != 0; }

static void leave(AoeConfigDialogRor *d, AoeUiPanelRor *p) {
    AoeUiSystemRor *sys = d->ui->system;
    const AoeGameUiOpsRor *ops = d->ui->ops;
    if (d->mode == 0) {
        if (ops && ops->resume_game) ops->resume_game(ops->context);
        aoe_ui_system_show_named_ror(sys, "Game Screen", 0);
        p->vt->set_active(p, 0);
    } else {
        aoe_ui_system_show_named_ror(sys, "Main Menu", 0);
    }
    aoe_ui_system_close_ror(sys, NAME);
}

static void apply(AoeConfigDialogRor *d, AoeUiPanelRor *p) {
    const AoeGameUiOpsRor *ops = d->ui->ops;
    AoeGameSettingsRor *g = d->ui->settings;
    AoeUiSystemRor *sys = d->ui->system;
    int32_t w = 0x280, h = 0x1e0;
    if (ops && ops->resume_game) ops->resume_game(ops->context);
    if (ops && ops->set_mouse_buttons) ops->set_mouse_buttons(ops->context, on(d->mouse[0]) ? 2 : 1);
    if (ops && ops->set_game_flag_a08) ops->set_game_flag_a08(ops->context, on(d->flag[0]));
    if (g && aoe_settings_flag_97d_ror(g)) {
        static const int32_t budgets[3] = {0x9c4, 5000, 0x1d4c};
        int i;
        for (i = 0; i < 3; ++i)
            if (on(d->path[i])) {
                aoe_settings_set_byte_a14_ror(g, (uint8_t)i);
                if (ops && ops->set_path_budget) ops->set_path_budget(ops->context, budgets[i]);
                break;
            }
    }
    if (!ops || !ops->game_state || ops->game_state(ops->context) != 5) {
        float speed = on(d->speed[0]) ? 1.0f : on(d->speed[1]) ? 1.5f : on(d->speed[2]) ? 2.0f : 0.0f;
        if (ops && ops->orders_enabled && ops->orders_enabled(ops->context) && speed != 0.0f &&
            speed != d->speed_old) {
            if (ops->change_game_speed) ops->change_game_speed(ops->context, speed);
            if (g && aoe_settings_flag_97d_ror(g)) aoe_settings_set_float_a0c_ror(g, speed);
        }
    }
    if (d->music_now != d->music_old && ops) {
        if (d->music_now < 99) {
            if (ops->music_on && !ops->music_on(ops->context) && ops->music_enable)
                ops->music_enable(ops->context, d->music_now);
        } else if (ops->music_on && ops->music_on(ops->context) && ops->music_disable) {
            ops->music_disable(ops->context);
        }
        if (ops->set_sound_level) ops->set_sound_level(ops->context, d->sound_now);
    }
    if (on(d->screen[1])) w = 800, h = 600;
    else if (on(d->screen[2])) w = 0x400, h = 0x300;
    aoe_ui_system_show_named_ror(sys, "Game Screen", 0);
    p->vt->set_active(p, 0);
    if (ops && ops->set_resolution) ops->set_resolution(ops->context, w, h);
    aoe_ui_system_close_ror(sys, NAME);
}

static int32_t cfg_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                          int32_t extra) {
    AoeConfigDialogRor *d = CFG(p);
    const AoeGameUiOpsRor *ops = d->ui->ops;
    int i;
    if (!from) return aoe_ui_panel_action_ror(p, from, code, value, extra);
    if (d->ok && from == &d->ok->panel && code == 1) {
        if (d->mode == 0) apply(d, p);
        else leave(d, p);
        return 1;
    }
    for (i = 0; i < 3; ++i) {
        if (d->speed[i] && from == &d->speed[i]->panel && code == 1) return 1;
        if (d->screen[i] && from == &d->screen[i]->panel) return 1;
    }
    if (d->music && from == &d->music->bar.panel && code == 4) {
        d->music_now = value;
        if (!ops || !ops->set_music_level || !ops->set_music_level(ops->context, value)) return 1;
        if (ops->set_sound_level) ops->set_sound_level(ops->context, d->sound_now);
        return 1;
    }
    if (d->sound && from == &d->sound->bar.panel && code == 4) {
        d->sound_now = value;
        if (ops && ops->set_sound_level) ops->set_sound_level(ops->context, value);
        return 1;
    }
    if (d->scroll && from == &d->scroll->bar.panel && code == 4) {
        if (ops && ops->set_scroll_speed) ops->set_scroll_speed(ops->context, value + 10);
        return aoe_ui_panel_action_ror(p, from, code, value, extra);
    }
    if (d->cancel && from == &d->cancel->panel && code == 1) {
        if (ops && ops->set_music_level) ops->set_music_level(ops->context, d->music_old);
        if (ops && ops->set_sound_level) ops->set_sound_level(ops->context, d->sound_old);
        if (ops && ops->set_scroll_speed) ops->set_scroll_speed(ops->context, d->scroll_old);
        leave(d, p);
        return 1;
    }
    return aoe_ui_panel_action_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_config_dialog_vtable_ror = {
    AOE_UI_EZ_SLOTS(cfg_destroy, aoe_ui_panel_handle_idle_ror, aoe_ui_panel_handle_paint_ror,
                    aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror, cfg_action,
                    aoe_ui_ez_set_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
