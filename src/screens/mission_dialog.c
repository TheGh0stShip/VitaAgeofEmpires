#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define MD(p) ((AoeMissionDialogRor *)(p))

static const char *const NAME = "Mission Dialog";
static const int32_t message_of[3] = {4, 0, 1};

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

static void md_destroy(AoeUiPanelRor *p) {
    AoeMissionDialogRor *d = MD(p);
    int i;
    /* The original also stops the scenario sound before the game (its
       sound object, 0x004a2910); the port's sound belongs to the host. */
    DROP(d->title);
    DROP(d->text);
    DROP(d->scrollbar);
    DROP(d->ok);
    for (i = 0; i < 3; ++i) DROP(d->buttons[i]);
    aoe_ui_ez_release_ror(&d->ez);
}

static const char *message(const AoeMissionDialogRor *d, int32_t i) {
    const AoeGameUiOpsRor *ops = d->ui->ops;
    return ops && ops->scenario_message ? ops->scenario_message(ops->context, i) : NULL;
}

static void place(AoeUiPanelRor *c, int32_t x, int32_t y, int32_t w, int32_t h) {
    c->vt->set_fixed_position(c, x, y, w, h);
}

static void show_message(AoeMissionDialogRor *d, int32_t i) {
    ((const AoeUiTextVtblRor *)d->text->panel.vt)->set_text(d->text, message(d, i));
}

int aoe_mission_dialog_init_ror(AoeMissionDialogRor *d, AoeGameUiRor *ui, AoeUiAreaRor *area,
                                const char *text, uint8_t in_game, void *picture) {
    static const int32_t ids[3] = {0x2a31, 0x2a32, 0x2a37};
    AoeUiEzRor *ez = &d->ez;
    const AoeGameUiOpsRor *ops = ui->ops;
    const char *name = ops && ops->scenario_title ? ops->scenario_title(ops->context) : NULL;
    char title[0x104];
    int shown = 0, i;
    memset(d, 0, sizeof *d);
    d->ui = ui;
    d->in_game = in_game;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_mission_dialog_vtable_ror.base;
    if (!aoe_ui_screen_setup_ror(ez, area, "scr9", 0xc38c, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_shadow_area_ror(ez, 0);
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    if (name && *name)
        snprintf(title, sizeof title, "%s", name);
    else
        aoe_ui_string_ror(ui->system, 0x2655, title, sizeof title);
    aoe_ui_ez_create_text_ror(ez, &ez->panel, &d->title, title, 10, 0xf, 0x26c, 0x14, 4, 1, 1, 0);
    aoe_ui_ez_create_text_ror(ez, &ez->panel, &d->text, text, 0xf, 0x46, 300, 0x154, 0xb, 0, 0,
                              1);
    if (d->text) aoe_ui_ez_create_text_scrollbar_ror(ez, &d->scrollbar, d->text, 0x14);
    if (d->text && ez->bevel_on)
        ((const AoeUiTextVtblRor *)d->text->panel.vt)->set_bevel(d->text, 3, ez->bevel);
    aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->ok, 0xfa1, 0, 200, 0x1b8, 0xf0, 0x1e, 0, 0,
                                   0);
    for (i = 0; i < 3; ++i)
        aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->buttons[i], ids[i], 0, 0x145,
                                       0x13c + 0x20 * i, 0xb4, 0x1e, 0, -1, 0);
    if (!d->ok || !d->text || !d->title || !d->buttons[0] || !d->buttons[1] || !d->buttons[2])
        return 1; /* the original goes on regardless; the port stops */
    for (i = 0; i < 3; ++i) aoe_ui_button_set_group_ror(d->buttons[i], d->buttons, 3);
    if (ez->panel.width <= 0x280) {
        place(&d->title->panel, 3, 0xf, 0x27a, 0x14);
        place(&d->text->panel, 0x14, 0x32, 600, 0x73);
        place(&d->buttons[0]->panel, 5, 0x1bd, 0x96, 0x1e);
        place(&d->buttons[1]->panel, 0xa5, 0x1bd, 0x96, 0x1e);
        place(&d->buttons[2]->panel, 0x145, 0x1bd, 0x96, 0x1e);
        place(&d->ok->panel, 0x1e5, 0x1bd, 0x96, 0x1e);
    } else if (ez->panel.width <= 800) {
        aoe_ui_ez_set_reference_slot_ror(ez, 800, 0x280);
        place(&d->title->panel, 3, 0xf, 0x31a, 0x14);
        place(&d->text->panel, 0x1e, 0x2d, 0x2e4, 0xe6);
        place(&d->buttons[0]->panel, 5, 0x235, 0xbe, 0x1e);
        place(&d->buttons[1]->panel, 0xcd, 0x235, 0xbe, 0x1e);
        place(&d->buttons[2]->panel, 0x195, 0x235, 0xbe, 0x1e);
        place(&d->ok->panel, 0x25d, 0x235, 0xbe, 0x1e);
    } else {
        aoe_ui_ez_set_reference_slot_ror(ez, 0x400, 0x300);
        place(&d->title->panel, 3, 0xf, 0x3fa, 0x14);
        place(&d->text->panel, 0x28, 0x41, 0x3b0, 0x11d);
        place(&d->buttons[0]->panel, 10, 0x2b2, 0xf0, 0x32);
        place(&d->buttons[1]->panel, 0x10a, 0x2b2, 0xf0, 0x32);
        place(&d->buttons[2]->panel, 0x20a, 0x2b2, 0xf0, 0x32);
        place(&d->ok->panel, 0x306, 0x2b2, 0xf0, 0x32);
    }
    /* Instructions first, then hints, then history; the first present
       one is shown and its button selected. */
    if (message(d, 0)) {
        show_message(d, 0);
        aoe_ui_button_select_ror(d->buttons[1]);
        shown = 1;
    } else {
        d->buttons[1]->panel.vt->set_active(&d->buttons[1]->panel, 0);
    }
    if (message(d, 4)) {
        if (!shown) {
            show_message(d, 4);
            aoe_ui_button_select_ror(d->buttons[0]);
        }
        ++shown;
    } else {
        d->buttons[0]->panel.vt->set_active(&d->buttons[0]->panel, 0);
    }
    if (message(d, 1)) {
        if (!shown) {
            show_message(d, 1);
            aoe_ui_button_select_ror(d->buttons[2]);
        }
        ++shown;
    } else {
        d->buttons[2]->panel.vt->set_active(&d->buttons[2]->panel, 0);
        if (!shown) d->text->panel.vt->set_active(&d->text->panel, 0);
    }
    /* The original's flag is a byte that counts; exactly one leaves no
       choice. */
    if (shown == 1)
        for (i = 0; i < 3; ++i) d->buttons[i]->panel.vt->set_active(&d->buttons[i]->panel, 0);
    aoe_ui_panel_set_current_ror(&ez->panel, &d->ok->panel);
    d->picture = picture;
    return 1;
}

static void md_draw(AoeUiPanelRor *p) {
    AoeMissionDialogRor *d = MD(p);
    const AoeGameUiOpsRor *ops = d->ui->ops;
    int32_t x, y;
    aoe_ui_ez_draw_slot_ror(p);
    if (!d->picture || !ops || !ops->draw_picture) return;
    if (p->width <= 0x280) {
        y = p->y + 0xa6;
        x = p->x + 7;
    } else if (p->width <= 800) {
        y = p->y + 0x118;
        x = p->x + 0x53;
    } else {
        y = p->y + 0x16b;
        x = p->x + 0xc5;
    }
    ops->draw_picture(ops->context, d->picture, p->area, x, y);
}

static int32_t md_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                         int32_t extra) {
    AoeMissionDialogRor *d = MD(p);
    const AoeGameUiOpsRor *ops = d->ui->ops;
    int i;
    if (code == 1 && from) {
        for (i = 0; i < 3; ++i)
            if (d->buttons[i] && from == &d->buttons[i]->panel) show_message(d, message_of[i]);
        if (d->ok && from == &d->ok->panel) {
            if (!d->in_game) {
                if (ops && ops->start_game) ops->start_game(ops->context);
            } else {
                aoe_ui_system_show_named_ror(d->ui->system, "Game Screen", 0);
                if (ops && ops->resume_game) ops->resume_game(ops->context);
            }
            aoe_ui_system_close_ror(d->ui->system, NAME);
            return 1;
        }
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_mission_dialog_vtable_ror = {
    AOE_UI_EZ_SLOTS_MD(md_destroy, md_draw, aoe_ui_panel_handle_idle_ror,
                       aoe_ui_panel_handle_paint_ror, aoe_ui_ez_no_key_action_ror,
                       aoe_ui_panel_zero_ror, md_action, aoe_ui_screen_focus_slot_ror,
                       aoe_ui_ez_mouse_down_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
