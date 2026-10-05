#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define ABOUT(p) ((AoeAboutDialogRor *)(p))
static const char *const NAME = "About Dialog";

static void free_panel(void *child) {
    AoeUiPanelRor *p = child;
    if (!p) return;
    p->vt->destroy(p);
    free(p);
}

static void about_destroy(AoeUiPanelRor *p) {
    AoeAboutDialogRor *d = ABOUT(p);
    free_panel(d->title);
    free_panel(d->title2);
    free_panel(d->version);
    free_panel(d->copyright);
    free_panel(d->copyright2);
    free_panel(d->developer);
    free_panel(d->engine);
    free_panel(d->pid_label);
    free_panel(d->pid);
    free_panel(d->credits);
    free_panel(d->close);
    free_panel(d->logo);
    free_panel(d->frame);
    free_panel(d->notice);
    free_panel(d->warning);
    memset(&d->title, 0, (size_t)((char *)&d->ui - (char *)&d->title));
    aoe_ui_dialog_release_ror(&d->dialog);
}

static int text_id(AoeAboutDialogRor *d, AoeUiTextRor **out, int32_t id,
                   int32_t x, int32_t y, int32_t w, int32_t h, int32_t font,
                   int32_t wrap) {
    AoeUiEzRor *ez = &d->dialog.ez;
    return aoe_ui_ez_create_text_id_ror(ez, &ez->panel, out, id, x, y, w, h,
                                        font, 0, 0, wrap);
}

static int text(AoeAboutDialogRor *d, AoeUiTextRor **out, const char *s,
                int32_t x, int32_t y, int32_t w, int32_t h, int32_t font,
                int32_t wrap) {
    AoeUiEzRor *ez = &d->dialog.ez;
    return aoe_ui_ez_create_text_ror(ez, &ez->panel, out, s, x, y, w, h, font,
                                     0, 0, wrap);
}

/* The right edge of a text panel's first line (0x0046a760 always
   succeeds here: there is no drawing context to fail). */
static int32_t first_line_right(AoeUiTextRor *t) {
    AoeUiRectRor r;
    int32_t col = 0;
    memset(&r, 0, sizeof(r));
    aoe_ui_text_line_rect_ror(t, 0, 0, &r, &col);
    return r.right;
}

static void append(char *dst, size_t size, const char *src) {
    size_t n = strlen(dst);
    if (n < size) snprintf(dst + n, size - n, "%s", src ? src : "");
}

int aoe_about_dialog_init_ror(AoeAboutDialogRor *d, AoeGameUiRor *ui,
                              AoeUiPanelRor *parent) {
    AoeUiEzRor *ez, *pez = (AoeUiEzRor *)parent;
    AoeUiSystemRor *s;
    char line[0x200], big[0x1000];
    AoeUiPanelRor *tab[2];
    if (!d || !ui) return 0;
    s = ui->system;
    aoe_ui_dialog_init_ror(&d->dialog, s, ui->ez_ops, NAME);
    d->dialog.ez.panel.vt = &aoe_about_dialog_vtable_ror.base;
    memset(&d->title, 0, (size_t)((char *)&d->ui - (char *)&d->title));
    d->ui = ui;
    ez = &d->dialog.ez;
    if (!parent ||
        !aoe_ui_dialog_setup_ror(&d->dialog, parent->area, parent, 0x208, 0x1d8,
                                 aoe_ui_ez_popup_name_ror(pez),
                                 aoe_ui_ez_popup_id_ror(pez), 1))
        return 0;
    text(d, &d->frame, "", 10, 10, 0x1f4, 0x190, 4, 0);
    if (d->frame)
        ((const AoeUiTextVtblRor *)d->frame->panel.vt)
            ->set_bevel(d->frame, 3, ez->bevel);
    text_id(d, &d->title, 0x2419, 0x14, 0x14, 0x1e0, 0x14, 4, 0);
    text_id(d, &d->title2, 0x2427, 0x14, 0x28, 0x1e0, 0x14, 4, 0);
    text_id(d, &d->version, 0x241a, 0x14, 0x3c, 0x1e0, 0x14, 0xb, 0);
    if (d->version) {
        snprintf(line, sizeof(line), "%s", aoe_ui_text_line_ror(d->version, 0));
        append(line, sizeof(line), "a");
        ((const AoeUiTextVtblRor *)d->version->panel.vt)->set_text(d->version, line);
    }
    aoe_ui_string_ror(s, 0x2424, line, sizeof(line));
    if (!line[0]) {
        text_id(d, &d->copyright, 0x241b, 0x14, 0x55, 0x1e0, 0x14, 10, 0);
        if (d->copyright) {
            int32_t right = first_line_right(d->copyright);
            d->logo = calloc(1, sizeof(*d->logo));
            if (d->logo) {
                aoe_ui_picture_init_ror(d->logo, s, ui->picture_ops);
                aoe_ui_picture_setup_ror(d->logo, ez->panel.area, &ez->panel,
                                         right - ez->panel.x + 3, 0x56, 0xf, 0xe,
                                         "circlep", 0xc4e0, 0, 1);
                text_id(d, &d->copyright2, 0x241c,
                        d->logo->panel.x - ez->panel.x +
                            aoe_ui_panel_width_ror(&d->logo->panel),
                        0x55, 0x1e0, 0x14, 10, 0);
            }
        }
    } else {
        snprintf(line, sizeof(line), "%s", aoe_ui_string_static_ror(s, 0x241b));
        append(line, sizeof(line), aoe_ui_string_static_ror(s, 0x2424));
        append(line, sizeof(line), " ");
        append(line, sizeof(line), aoe_ui_string_static_ror(s, 0x241c));
        text(d, &d->copyright, line, 0x14, 0x55, 0x1e0, 0x14, 10, 0);
    }
    text_id(d, &d->developer, 0x241d, 0x14, 0x69, 0x1e0, 0x14, 10, 0);
    text_id(d, &d->engine, 0x2423, 0x14, 0x7d, 0x1e0, 0x28, 10, 1);
    text_id(d, &d->pid_label, 0x241e, 0x14, 0xaa, 0x5f, 0x14, 10, 0);
    if (d->pid_label) {
        int32_t right = first_line_right(d->pid_label);
        const char *pid = ui->ops && ui->ops->product_id
                              ? ui->ops->product_id(ui->ops->context)
                              : NULL;
        snprintf(big, sizeof(big), "%s", pid ? pid : "00000-000-0000000-00000");
        text(d, &d->pid, big, right - ez->panel.x + 4, 0xaa, 200, 0x14, 0xb, 0);
    }
    aoe_ui_string_ror(s, 0x241f, big, sizeof(big));
    text(d, &d->warning, big, 0x14, 0xc3, 0x1e0, 0xdc, 10, 1);
    text_id(d, &d->notice, 0x2426, 0x14, 0x15e, 0x1e0, 0x3c, 10, 1);
    aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->credits, 0x2420, 0, 0x46,
                                   0x1ae, 0xaa, 0x1e, 0, 0, 0);
    aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->close, 0x2421, 0, 0x118,
                                   0x1ae, 0xaa, 0x1e, 0, 0, 0);
    if (!d->credits || !d->close) return 0;
    d->close->hotkey = 0x1b;
    d->close->hotkey_shift = 0;
    tab[0] = &d->credits->panel;
    tab[1] = &d->close->panel;
    aoe_ui_panels_tab_cycle_ror(tab, 2);
    aoe_ui_panel_set_current_ror(&ez->panel, &d->credits->panel);
    return 1;
}

static int32_t about_action(AoeUiPanelRor *p, AoeUiPanelRor *from,
                            int32_t code, int32_t value, int32_t extra) {
    AoeAboutDialogRor *d = ABOUT(p);
    AoeGameUiRor *ui = d->ui;
    AoeUiSystemRor *s = ui->system;
    if (code == 1) {
        if (d->close && from == &d->close->panel) {
            if (aoe_ui_system_find_ror(s, "Game Screen")) {
                if (ui->settings && aoe_settings_flag_97d_ror(ui->settings) == 1 &&
                    ui->game && ui->game->comm_state == 0)
                    aoe_game_comm_update_ror(ui->game, 0, 0, ui->game_ops);
                aoe_ui_system_show_named_ror(s, "Game Screen", 0);
                p->vt->set_active(p, 0);
            }
            aoe_ui_system_close_ror(s, NAME);
            return 1;
        }
        if (d->credits && from == &d->credits->panel) {
            if (ui->ops && ui->ops->open_credits) ui->ops->open_credits(ui->ops->context);
            aoe_ui_system_show_named_ror(s, "Credits Screen", 0);
            aoe_ui_system_close_ror(s, NAME);
            return 1;
        }
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_about_dialog_vtable_ror = {
    AOE_UI_EZ_SLOTS(about_destroy, aoe_ui_panel_handle_idle_ror,
                    aoe_ui_panel_handle_paint_ror, aoe_ui_ez_no_key_action_ror,
                    aoe_ui_panel_zero_ror, about_action,
                    aoe_ui_ez_set_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
