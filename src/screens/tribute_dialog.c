#include "game_ui.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define DIPLO(p) ((AoeDiplomacyDialogRor *)(p))

/* Resource columns: x 440 wood, 480 food, 520 gold, 560 stone; their
   button picture frames (tradicon.shp) and help IDs. */
static const int32_t col_x[4] = {0x1b8, 0x1e0, 0x208, 0x230};
static const int col_res[4] = {1, 0, 3, 2};
static const int16_t col_frame[4] = {0, 2, 3, 1};

static void free_panel(void *child) {
    AoeUiPanelRor *p = child;
    if (!p) return;
    p->vt->destroy(p);
    free(p);
}

/* 0x004321f0: every child released (deleting form 0x004321d0). */
static void diplo_destroy(AoeUiPanelRor *p) {
    AoeDiplomacyDialogRor *d = DIPLO(p);
    int i, j;
    for (i = 0; i < 6; i++) free_panel(d->headers[i]);
    free_panel(d->title);
    free_panel(d->notice);
    free_panel(d->fee);
    free_panel(d->ally_label);
    for (i = 0; i < 8; i++) {
        free_panel(d->names[i]);
        free_panel(d->civs[i]);
        for (j = 0; j < 4; j++) free_panel(d->gifts[j][i]);
    }
    for (i = 0; i < 24; i++) free_panel(d->stances[i]);
    for (j = 0; j < 4; j++) free_panel(d->stocks[j]);
    free_panel(d->allied_victory);
    free_panel(d->ok);
    free_panel(d->clear);
    free_panel(d->cancel);
    free_panel(d->help);
    if (d->ui && d->ui->ez_ops && d->ui->ez_ops->free_shape) {
        const AoeUiEzOpsRor *e = d->ui->ez_ops;
        if (d->icons) e->free_shape(e->context, d->icons);
        if (d->colours) e->free_shape(e->context, d->colours);
    }
    d->colours = NULL;
    memset(&d->title, 0, sizeof(*d) - offsetof(AoeDiplomacyDialogRor, title));
    aoe_ui_dialog_release_ror(&d->dialog);
}

static int32_t row_font(const AoeGameUiOpsRor *ops, int32_t player) {
    int32_t i, n, found = 0;
    if (ops->player_type && ops->player_type(ops->context, player) == 2) return 8;
    if (!(ops->networked && ops->networked(ops->context))) return 0xb;
    n = ops->comm_slots ? ops->comm_slots(ops->context) : 0;
    for (i = 1; i <= n; i++) {
        int32_t s;
        if (ops->slot_player(ops->context, i) != player) continue;
        s = ops->comm_slot_status ? ops->comm_slot_status(ops->context, i) : 0;
        if (s == 2 || s == 4) found = 1;
    }
    return found ? 0xb : 8;
}

static float attr(const AoeGameUiOpsRor *ops, int32_t player, int32_t i) {
    return ops->player_attribute ? ops->player_attribute(ops->context, player, i) : 0.0f;
}

static void show(void *panel, int on) {
    AoeUiPanelRor *p = panel;
    if (p) p->vt->set_active(p, on);
}

/* String 9856 is a printf format with one %d: the fee in percent. */
static void format_fee(AoeGameUiRor *ui, char *out, size_t n, int value) {
    char fmt[100];
    const char *d;
    fmt[0] = 0;
    aoe_ui_string_ror(ui->system, 0x2680, fmt, sizeof(fmt));
    d = strstr(fmt, "%d");
    if (!d) {
        snprintf(out, n, "%s", fmt);
        return;
    }
    snprintf(out, n, "%.*s%d%s", (int)(d - fmt), fmt, value, d + 2);
}

/* 0x00431470: "Diplomacy Dialog", 625 x 405 over the parent. A title
   (9851), the column heads (9862 name, 9852 civilization, 9853..9855
   the stances, 9856 tribute), the tribute fee line (9856 formatted with
   attribute 46 x 100) and the no-market notice (9863); per player
   1..count-1 at y 80 + 30 (p - 1) the name (font 8 for a computer or a
   networked player without a connected human slot), the civilization,
   for another player three stance radios (x 240, 310, 380; help
   30340..30342; disabled in mode 5) on the viewed player's stance, and
   for a living other player outside mode 5 four tribute buttons (wood,
   food, gold, stone; help 30343..30346). The allied victory box (help
   30348); outside mode 5 OK (4001) and Clear Tribute (9859); Cancel
   (4002, Escape), help (4009); the stock left per resource on the
   viewed player's row. Without a market (building 84) the notice shows
   in place of the fee and stocks; age attribute 30 == 3 hides the fee.
   The exhausted columns are disabled; OK is focused and OK, Clear and
   Cancel cycle the tab order. */
int aoe_diplomacy_dialog_init_ror(AoeDiplomacyDialogRor *d, AoeGameUiRor *ui,
                                  AoeUiPanelRor *parent) {
    static const int32_t hx[5] = {5, 0x89, 0xeb, 0x122, 0x168};
    static const int32_t hw[5] = {0x82, 0x50, 0x46, 0x46, 0x46};
    static const int32_t hid[5] = {0x2686, 0x267c, 0x267d, 0x267e, 0x267f};
    const AoeGameUiOpsRor *ops;
    AoeUiEzRor *ez, *pez = (AoeUiEzRor *)parent;
    AoeUiPanelRor *tab[3];
    char buf[100];
    int32_t p, count, me, y, mode5, n;
    int i, j, market;
    if (!d || !ui || !ui->ops || !ui->game || !ui->ops->slot_player) return 0;
    ops = ui->ops;
    memset(d, 0, sizeof(*d));
    aoe_ui_dialog_init_ror(&d->dialog, ui->system, ui->ez_ops, "Diplomacy Dialog");
    d->dialog.ez.panel.vt = &aoe_diplomacy_dialog_vtable_ror.base;
    d->ui = ui;
    ez = &d->dialog.ez;
    /* The tribute button pictures (tradicon.shp 50732) and the player
       colour icons (dlg_plbn.shp 50746); either failing to load fails. */
    if (ui->ez_ops && ui->ez_ops->load_shape) {
        d->icons = ui->ez_ops->load_shape(ui->ez_ops->context, "tradicon.shp", 0xc62c);
        if (!d->icons) return 0;
        d->colours = ui->ez_ops->load_shape(ui->ez_ops->context, "dlg_plbn.shp", 0xc63a);
        if (!d->colours) return 0;
    }
    me = ui->game->view_player;
    mode5 = ui->game->mode == 5;
    if (!parent ||
        !aoe_ui_dialog_setup_ror(&d->dialog, parent->area, parent, 0x271, 0x195,
                                 aoe_ui_ez_popup_name_ror(pez),
                                 aoe_ui_ez_popup_id_ror(pez), 1))
        return 0;
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &d->title, 0x267b, 0x48, 5, 500, 0x1e,
                                      1, 1, 1, 0) ||
        !aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &d->notice, 0x2687, 0x1b8, 0x6e, 0xaf,
                                      0x3c, 0xb, 1, 1, 1))
        return 0;
    for (i = 0; i < 5; i++)
        if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &d->headers[i], hid[i], hx[i], 0x32,
                                          hw[i], 0x1e, 0xb, 0, 1, 0))
            return 0;
    format_fee(ui, buf, sizeof(buf), (int)(attr(ops, me, 46) * 100.0f));
    if (!aoe_ui_ez_create_text_ror(ez, &ez->panel, &d->fee, buf, 0x1b8, 0x32, 0xb4, 0x1e, 0xb,
                                   0, 1, 0) ||
        !aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &d->ally_label, 0x2681, 0x13e, 0x14a, 300,
                                      0x1e, 0xb, 0, 1, 0))
        return 0;
    d->own_row = -1;
    count = ops->player_count ? ops->player_count(ops->context) : 0;
    for (p = 1, y = 0x50; p < count && p <= 8; p++, y += 0x1e) {
        int r = p - 1;
        int32_t type = ops->player_type ? ops->player_type(ops->context, p) : 0;
        const char *name = ops->player_name ? ops->player_name(ops->context, p) : NULL;
        if (p == me) d->own_row = r;
        if (!aoe_ui_ez_create_text_ror(ez, &ez->panel, &d->names[r], name ? name : "", 5, y,
                                       0x82, 0x1e, row_font(ops, p), 0, 1, 0))
            return 0;
        buf[0] = 0;
        if (ops->player_civ_name) ops->player_civ_name(ops->context, p, buf, sizeof(buf));
        if (!aoe_ui_ez_create_text_ror(ez, &ez->panel, &d->civs[r], buf, 0x89, y, 100, 0x1e,
                                       0xb, 0, 1, 0))
            return 0;
        if (p != me) {
            AoeUiButtonRor **st = &d->stances[r * 3];
            int32_t stance = ops->player_stance ? ops->player_stance(ops->context, me, p) : 3;
            for (j = 0; j < 3; j++) {
                if (!aoe_ui_ez_create_radio_ror(ez, &ez->panel, &st[j], 0xf0 + 0x46 * j, y, 0x1e,
                                                0x1e, 0, 0))
                    return 0;
                aoe_ui_panel_set_help_ror(&st[j]->panel, 0x7684 + j, -1);
            }
            for (j = 0; j < 3; j++) {
                aoe_ui_button_set_group_ror(st[j], st, 3);
                if (mode5) aoe_ui_button_set_disabled_ror(st[j], 1);
            }
            aoe_ui_button_set_state_ror(st[stance == 0 ? 0 : stance == 1 ? 1 : 2], 1);
        }
        if (!(mode5 || p == me || type != 0)) {
            for (j = 0; j < 4; j++) {
                if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->gifts[j][r], -1, 0,
                                                    col_x[j], y, 0x28, 0x1e, 0xb, 0, 0))
                    return 0;
            }
            for (j = 0; j < 4; j++)
                aoe_ui_button_set_picture_ror(d->gifts[j][r], 0, d->icons, col_frame[j]);
            for (j = 0; j < 4; j++) aoe_ui_button_set_draw_type_ror(d->gifts[j][r], 4);
            for (j = 0; j < 4; j++) aoe_ui_panel_set_help_ror(&d->gifts[j][r]->panel, 0x7687 + j, -1);
        }
    }
    if (!aoe_ui_ez_create_check_ror(ez, &ez->panel, &d->allied_victory, 0x11a, 0x147, 0x1e,
                                    0x1e, 0, 0))
        return 0;
    aoe_ui_button_set_state_ror(d->allied_victory,
                                ops->allied_victory && ops->allied_victory(ops->context, me));
    aoe_ui_panel_set_help_ror(&d->allied_victory->panel, 0x768c, -1);
    if (mode5) aoe_ui_button_set_disabled_ror(d->allied_victory, 1);
    if (!mode5) {
        if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->ok, 0xfa1, 0, 10, 0x16f, 0xbe,
                                            0x1e, 0, 0, 0))
            return 0;
        aoe_ui_panel_set_help_ror(&d->ok->panel, 0x7531, -1);
        if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->clear, 0x2683, 0, 0xd7, 0x16f,
                                            0xbe, 0x1e, 0, 0, 0))
            return 0;
        aoe_ui_panel_set_help_ror(&d->clear->panel, 0x768b, -1);
    }
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->cancel, 0xfa2, 0, 0x1a4, 0x16f, 0xbe,
                                        0x1e, 0, 0, 0))
        return 0;
    aoe_ui_panel_set_help_ror(&d->cancel->panel, 0x7532, -1);
    d->cancel->hotkey = 0x1b;
    d->cancel->hotkey_shift = 0;
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->help, 0xfa9, 0, 500, 0x147, 0x1e,
                                        0x1e, 0, 0, 0))
        return 0;
    for (j = 0; j < 4; j++) {
        int res = col_res[j];
        snprintf(buf, sizeof(buf), "%d", (int)(attr(ops, me, res) - d->tribute.spent[res]));
        if (!aoe_ui_ez_create_text_ror(ez, &ez->panel, &d->stocks[j], buf, col_x[j],
                                       d->own_row * 0x1e + 0x50, 0x28, 0x1e, 0xb, 0, 1, 0))
            return 0;
    }
    market = ops->has_building && ops->has_building(ops->context, me, 0x54);
    d->has_market = market;
    show(d->notice, !market);
    show(d->fee, market);
    for (j = 0; j < 4; j++) show(d->stocks[j], market);
    if ((int)attr(ops, me, 0x1e) == 3) show(d->fee, 0);
    for (j = 0; j < 4; j++) {
        int res = col_res[j];
        int out = aoe_tribute_exhausted_ror(&d->tribute, res, attr(ops, me, res), mode5);
        for (i = 0; i < 8; i++)
            if (d->gifts[j][i]) aoe_ui_button_set_disabled_ror(d->gifts[j][i], out);
    }
    if (d->ok) aoe_ui_panel_set_current_ror(&ez->panel, &d->ok->panel);
    n = 0;
    if (d->ok) tab[n++] = &d->ok->panel;
    if (d->clear) tab[n++] = &d->clear->panel;
    if (d->cancel) tab[n++] = &d->cancel->panel;
    aoe_ui_panels_tab_cycle_ror(tab, (int16_t)n);
    return 1;
}

/* The stock left of a column's resource, as "%ld". */
static void set_stock(AoeDiplomacyDialogRor *d, int j) {
    const AoeGameUiOpsRor *ops = d->ui->ops;
    int res = col_res[j];
    char buf[32];
    if (!d->stocks[j]) return;
    snprintf(buf, sizeof(buf), "%ld",
             (long)(attr(ops, d->ui->game->view_player, res) - d->tribute.spent[res]));
    aoe_ui_text_set_text_ror(d->stocks[j], buf);
}

/* 0x00432ff0 / 0x00433080 / 0x00433110 / 0x004331a0 (tribute.h): a
   column's buttons disabled when exhausted. */
static void refresh_column(AoeDiplomacyDialogRor *d, int j) {
    AoeGameUiRor *ui = d->ui;
    int res = col_res[j], i;
    int out = aoe_tribute_exhausted_ror(&d->tribute, res,
                                        attr(ui->ops, ui->game->view_player, res),
                                        ui->game->mode == 5);
    for (i = 0; i < 8; i++)
        if (d->gifts[j][i]) aoe_ui_button_set_disabled_ror(d->gifts[j][i], out);
}

/* OK: the allied victory and each other player's stance sent when
   changed, then that player's promised tribute (gold, wood, food,
   stone; above 0) with the fee (attribute 46). */
static void diplo_apply(AoeDiplomacyDialogRor *d) {
    static const int order[4] = {3, 1, 0, 2};
    const AoeGameUiOpsRor *ops = d->ui->ops;
    int32_t me = d->ui->game->view_player, count, p;
    float fee = attr(ops, me, 46);
    if (d->allied_victory && ops->set_allied_victory) {
        int on = aoe_ui_button_state_ror(d->allied_victory) == 1;
        int was = ops->allied_victory ? ops->allied_victory(ops->context, me) != 0 : 0;
        if (on != was) ops->set_allied_victory(ops->context, me, on);
    }
    count = ops->player_count ? ops->player_count(ops->context) : 0;
    for (p = 1; p < count && p <= 8; p++) {
        AoeUiButtonRor **st = &d->stances[(p - 1) * 3];
        uint8_t stance;
        int i;
        if (p == me || !st[0]) continue;
        stance = aoe_diplomacy_stance_ror(aoe_ui_button_state_ror(st[0]) == 1,
                                          aoe_ui_button_state_ror(st[1]) == 1);
        if (ops->set_stance &&
            (!ops->player_stance || ops->player_stance(ops->context, me, p) != stance))
            ops->set_stance(ops->context, me, p, stance);
        for (i = 0; i < 4; i++) {
            float amount = d->tribute.given[order[i]][p];
            if (amount > 0.0f && ops->give_tribute)
                ops->give_tribute(ops->context, me, p, order[i], amount, fee);
        }
    }
}

/* +0xb8 0x004323b0: only clicks (code 1) on a child; others go to the
   Ez action. OK applies; OK and Cancel resume a game paused by the
   settings, show the game screen and close the dialog; help arms the
   help mode; Clear Tribute forgets every promise, blanks the buttons and
   restores the stocks; a tribute button promises 100 more (tribute.h),
   shows the player's total on the button and the stock left, and the
   column is rechecked. */
static int32_t diplo_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                            int32_t extra) {
    AoeDiplomacyDialogRor *d = DIPLO(p);
    AoeGameUiRor *ui = d->ui;
    const AoeGameUiOpsRor *ops = ui->ops;
    int32_t me = ui->game->view_player;
    char buf[32];
    int i, j;
    if (!from || code != 1) return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
    if ((d->ok && from == &d->ok->panel) || (d->cancel && from == &d->cancel->panel)) {
        if (d->ok && from == &d->ok->panel) diplo_apply(d);
        if (ops->resume_game) ops->resume_game(ops->context);
        aoe_ui_system_show_named_ror(ui->system, "Game Screen", 0);
        aoe_ui_system_close_ror(ui->system, "Diplomacy Dialog");
        return 1;
    }
    if (d->help && from == &d->help->panel) {
        aoe_ui_ez_help_mode_on_ror(&d->dialog.ez);
        return 1;
    }
    if (d->clear && from == &d->clear->panel) {
        aoe_tribute_clear_ror(&d->tribute);
        for (j = 0; j < 4; j++)
            for (i = 0; i < 8; i++)
                if (d->gifts[j][i]) aoe_ui_button_set_text_ror(d->gifts[j][i], 0, "");
        for (j = 0; j < 4; j++) set_stock(d, j);
        for (j = 0; j < 4; j++) refresh_column(d, j);
        return 1;
    }
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 4; j++) {
            int res = col_res[j];
            float fee;
            if (!d->gifts[j][i] || from != &d->gifts[j][i]->panel) continue;
            fee = attr(ops, me, 46);
            if (aoe_tribute_click_ror(&d->tribute, res, i + 1, attr(ops, me, res),
                                      fee > 0.0f ? fee : 0.0f)) {
                snprintf(buf, sizeof(buf), "%ld", (long)d->tribute.given[res][i + 1]);
                aoe_ui_button_set_text_ror(d->gifts[j][i], 0, buf);
            }
            set_stock(d, j);
            refresh_column(d, j);
            return 1;
        }
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

typedef struct {
    AoeDiplomacyDialogRor *d;
    AoeUiPanelRor *p;
} DiploDraw;

static void dd_base(void *c) { aoe_ui_ez_draw_slot_ror(((DiploDraw *)c)->p); }
static void dd_begin(void *c, int32_t a) {
    AoeUiPanelRor *p = ((DiploDraw *)c)->p;
    p->vt->begin_draw(p, a);
}
static void dd_end(void *c) {
    AoeUiPanelRor *p = ((DiploDraw *)c)->p;
    p->vt->end_draw(p);
}
static const uint8_t *dd_table(void *c, int32_t pl, int16_t *colour) {
    const AoeGameUiOpsRor *ops = ((DiploDraw *)c)->d->ui->ops;
    return ops->player_colour_table ? ops->player_colour_table(ops->context, pl, colour) : NULL;
}
static void dd_fill(void *c, uint8_t colour) {
    const AoeGameUiOpsRor *ops = ((DiploDraw *)c)->d->ui->ops;
    if (ops->set_fill_colour) ops->set_fill_colour(ops->context, colour);
}
static void dd_icon(void *c, int32_t x, int32_t y, const uint8_t *table) {
    DiploDraw *dd = c;
    const AoeGameUiOpsRor *ops = dd->d->ui->ops;
    if (ops->draw_colour_icon)
        ops->draw_colour_icon(ops->context, dd->p, dd->d->colours, x, y, table);
}

/* +0x30 0x00433230: the Ez draw, then the player colour icons
   (tribute.h aoe_tribute_draw_ror). */
static void diplo_draw(AoeUiPanelRor *p) {
    AoeDiplomacyDialogRor *d = DIPLO(p);
    const AoeGameUiOpsRor *ops = d->ui->ops;
    DiploDraw dd;
    AoeTributeDrawOpsRor t;
    dd.d = d;
    dd.p = p;
    memset(&t, 0, sizeof(t));
    t.context = &dd;
    t.base_draw = dd_base;
    t.has_icons = d->colours != NULL;
    t.begin_draw = dd_begin;
    t.end_draw = dd_end;
    t.player_count = (int16_t)(ops->player_count ? ops->player_count(ops->context) : 0);
    t.colour_table = dd_table;
    t.set_fill = dd_fill;
    t.draw_icon = dd_icon;
    t.x = p->x;
    t.y = p->y;
    aoe_tribute_draw_ror(&t);
}

const AoeUiEzVtblRor aoe_diplomacy_dialog_vtable_ror = {
    AOE_UI_EZ_SLOTS_MD(diplo_destroy, diplo_draw, aoe_ui_panel_handle_idle_ror,
                       aoe_ui_panel_handle_paint_ror, aoe_ui_ez_no_key_action_ror,
                       aoe_ui_panel_zero_ror, diplo_action, aoe_ui_ez_set_focus_slot_ror,
                       aoe_ui_ez_mouse_down_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
