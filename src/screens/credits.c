#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"
#include "ui_dialog.h"

#define CT(p) ((AoeCreditsTextRor *)(p))
#define CR(p) ((AoeCreditsScreenRor *)(p))

/* ---- the scrolling text ---- */

static uint32_t panel_now(const AoeUiPanelRor *p) {
    const AoeUiPlatformRor *pf = p->system ? p->system->platform : NULL;
    return pf && pf->now_ms ? pf->now_ms(pf->context) : 0;
}

void aoe_credits_text_set_ror(AoeCreditsTextRor *c, const char *text) {
    aoe_ui_text_set_text_ror(&c->text, text);
    c->last_ms = 0;
    c->offset = 0;
    c->done = 0;
}

static int credits_text_set_slot(AoeUiTextRor *t, const char *text) {
    aoe_credits_text_set_ror(CT(t), text);
    return 1;
}

int aoe_credits_text_init_ror(AoeCreditsTextRor *c, AoeUiSystemRor *system,
                              AoeUiAreaRor *area, AoeUiPanelRor *parent,
                              int32_t x, int32_t y, int32_t w, int32_t h,
                              int32_t speed, uintptr_t font, int32_t font_w,
                              int32_t font_h, const char *text) {
    aoe_ui_text_init_ror(&c->text, system);
    c->speed = speed;
    c->text.panel.vt = &aoe_credits_text_vtable_ror.base;
    c->offset = 0;
    c->last_ms = 0;
    c->done = 0;
    if (!aoe_ui_text_setup_ror(&c->text, area, parent, x, y, w, h, font,
                               font_w, font_h, NULL, 0, 0, 0, 0, 0, 0, 0, ""))
        return 0;
    aoe_ui_text_set_alignment_ror(&c->text, 3, 0);
    aoe_ui_text_set_wrap_ror(&c->text, 1);
    aoe_credits_text_set_ror(c, text ? text : "");
    aoe_ui_text_set_colors_ror(&c->text, c->text.color, c->text.shadow);
    return 1;
}

static int32_t credits_text_idle(AoeUiPanelRor *p) {
    AoeCreditsTextRor *c = CT(p);
    uint32_t now = panel_now(p);
    if (c->last_ms == 0) {
        c->last_ms = now;
    } else {
        int32_t px = (int32_t)((double)(now - c->last_ms) * 0.001 * c->speed);
        if (px > 0) {
            c->offset += px;
            p->vt->set_redraw(p, 1);
            c->last_ms = now;
        }
    }
    if (c->done && p->parent) p->parent->vt->action(p->parent, p, 1, 0, 0);
    return aoe_ui_panel_handle_idle_ror(p);
}

/* 0x0047cf00: one line raised by the scroll; the last line above the
   panel's top ends the credits. */
static void out(const AoeUiPlatformRor *pf, int32_t style, int32_t x,
                int32_t y, const char *s, uint32_t color, uint32_t shadow) {
    int32_t n = (int32_t)strlen(s);
    if (style == 1) {
        pf->text_out(pf->context, x - 1, y + 1, s, n, color);
        pf->text_out(pf->context, x, y, s, n, shadow);
    } else if (style == 2) {
        pf->text_out(pf->context, x - 1, y + 1, s, n, shadow);
        pf->text_out(pf->context, x, y, s, n, color);
    } else if (style == 0) {
        pf->text_out(pf->context, x, y, s, n, color);
    }
}

static void credits_line(AoeCreditsTextRor *c, const AoeUiPlatformRor *pf,
                         int16_t line, uint32_t color, uint32_t shadow) {
    AoeUiTextRor *t = &c->text;
    const char *a = aoe_ui_text_line_ror(t, line);
    const char *b = aoe_ui_text_column_ror(t, line);
    AoeUiRectRor r;
    int32_t x, y;
    if (!a && !b) return;
    aoe_ui_text_line_rect_ror(t, line, line, &r, NULL);
    x = r.left;
    if (t->align != 0 && t->align != 6) x += 5;
    y = t->panel.height - c->offset + r.top;
    if (line == t->count - 1 && t->line_height + y < t->panel.y) c->done = 1;
    if (t->has_column_clip) pf->text_clip(pf->context, &t->column_clip);
    if (a) out(pf, t->shadow_style, x, y, a, color, shadow);
    if (b) {
        if (t->has_column_clip) pf->text_clip(pf->context, &t->panel.clip);
        out(pf, t->shadow_style, t->column_x + x, y, b, color, shadow);
    }
}

static void credits_text_draw(AoeUiPanelRor *p) {
    AoeCreditsTextRor *c = CT(p);
    const AoeUiPlatformRor *pf = p->system ? p->system->platform : NULL;
    int16_t line;
    p->redraw = 0;
    if (!p->area || !p->visible || !p->active) return;
    aoe_ui_text_draw_background_ror(&c->text);
    p->vt->begin_draw(p, 0);
    if (pf && pf->text_begin &&
        pf->text_begin(pf->context, p->area, &p->clip, c->text.font)) {
        for (line = 0; line < c->text.count; ++line)
            credits_line(c, pf, line, c->text.color, c->text.shadow);
        pf->text_end(pf->context);
    }
    aoe_ui_text_draw_border_ror(&c->text);
    p->vt->end_draw(p);
}

static void credits_text_destroy(AoeUiPanelRor *p) {
    aoe_ui_text_release_ror(&CT(p)->text);
}

const AoeUiTextVtblRor aoe_credits_text_vtable_ror = {
    {credits_text_destroy,
     aoe_ui_panel_setup_ror,
     aoe_ui_panel_set_rect_ltrb_ror,
     aoe_ui_text_set_rect_slot_ror,
     aoe_ui_panel_set_color_ror,
     aoe_ui_text_set_active_slot_ror,
     aoe_ui_panel_set_position_mode_ror,
     aoe_ui_panel_set_fixed_position_ror,
     aoe_ui_panel_set_redraw_ror,
     aoe_ui_panel_redraw_overlaps_ror,
     aoe_ui_panel_begin_draw_ror,
     aoe_ui_panel_end_draw_ror,
     credits_text_draw,
     aoe_ui_panel_draw_rect_ror,
     aoe_ui_panel_draw_offset_ror,
     aoe_ui_panel_draw_parent_rect_ror,
     aoe_ui_panel_draw_parent_offset_ror,
     aoe_ui_panel_validate_ror,
     aoe_ui_panel_handle_message_ror,
     credits_text_idle,
     aoe_ui_panel_handle_size_ror,
     aoe_ui_panel_handle_paint_ror,
     aoe_ui_panel_key_down_ror,
     aoe_ui_panel_char_ror,
     aoe_ui_panel_ignore_ror,
     aoe_ui_panel_ignore_ror,
     aoe_ui_panel_ignore_ror,
     aoe_ui_panel_ignore_ror,
     aoe_ui_panel_mouse_down_ror,
     aoe_ui_panel_mouse_move_ror,
     aoe_ui_panel_mouse_up_ror,
     aoe_ui_panel_mouse_dbl_ror,
     {aoe_ui_panel_no_action_ror, aoe_ui_text_left_down_ror, aoe_ui_panel_no_action_ror,
      aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
      aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
      aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
      aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror},
     aoe_ui_text_key_action_ror,
     aoe_ui_panel_zero_ror,
     aoe_ui_panel_no_char_action_ror,
     aoe_ui_text_action_ror,
     aoe_ui_panel_get_true_rect_ror,
     aoe_ui_panel_is_inside_ror,
     aoe_ui_panel_set_focus_ror,
     aoe_ui_panel_set_tab_order_ror,
     aoe_ui_panel_get_help_ror,
     aoe_ui_panel_stop_sound_ror,
     aoe_ui_panel_restart_sound_ror,
     aoe_ui_panel_slot_e0_ror},
    aoe_ui_text_lines_slot_ror,
    aoe_ui_text_set_text_id_ror,
    credits_text_set_slot,
    aoe_ui_text_bevel_slot_ror,
};

/* ---- the screen ---- */

static uint32_t clock_ms(void *context) {
    const AoeUiPlatformRor *pf = context;
    return pf && pf->now_ms ? pf->now_ms(pf->context) : 0;
}

static AoeDrawSystemRor *draw_system(const AoeCreditsScreenRor *c) {
    const AoeUiPanelRor *p = &c->ez.panel;
    return p->area && p->area->surface ? p->area->surface->system : NULL;
}

/* A full-screen or memory-surface display, where the palette fades. */
static int fades(const AoeCreditsScreenRor *c) {
    const AoeDrawSystemRor *d = draw_system(c);
    return d && (d->screen == 2 || d->type == 1);
}

static void music(AoeCreditsScreenRor *c, int32_t from, int32_t to) {
    const AoeGameUiOpsRor *ops = c->ui->ops;
    if (ops && ops->music_track && ops->play_music &&
        ops->music_track(ops->context) == 0xf)
        ops->play_music(ops->context, from, to);
}

static void credits_destroy(AoeUiPanelRor *p) {
    AoeCreditsScreenRor *c = CR(p);
    const AoeUiEzOpsRor *ops = c->ez.ops;
    int i;
    for (i = 0; i < 7; ++i) {
        if (c->pictures[i] && ops && ops->free_shape)
            ops->free_shape(ops->context, c->pictures[i]);
        c->pictures[i] = NULL;
    }
    c->ez.background = NULL;
    if (c->text) {
        c->text->text.panel.vt->destroy(&c->text->text.panel);
        free(c->text);
        c->text = NULL;
    }
    music(c, 5, 0xe);
    aoe_ui_ez_release_ror(&c->ez);
}

int aoe_credits_screen_init_ror(AoeCreditsScreenRor *c, AoeGameUiRor *ui,
                                AoeUiAreaRor *area) {
    const AoeUiEzOpsRor *eops = ui->ez_ops;
    const AoeGameUiOpsRor *ops = ui->ops;
    AoeUiEzRor *ez = &c->ez;
    uintptr_t font = 0;
    int32_t fw = 0, fh = 0, i;
    char *text = NULL;
    memset(c, 0, sizeof *c);
    c->ui = ui;
    aoe_ui_screen_init_ror(ez, ui->system, eops, "Credits Screen");
    ez->panel.vt = &aoe_credits_screen_vtable_ror.base;
    c->field_47c = 1;
    c->picture = 1;
    c->state = 1;
    c->preload = 1;
    if (!aoe_ui_screen_setup_ror(ez, area, "scr_cred", 0xc38b, 1)) {
        ez->panel.field_d8 = 1; /* setup failed */
        return 0;
    }
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    if (eops && eops->font) eops->font(eops->context, 4, &font, &fw, &fh);
    if (ops && ops->read_file) {
        uint32_t size = 0;
        uint8_t *raw = ops->read_file(ops->context, "data2\\list.cr", &size);
        if (raw) {
            text = calloc(1, (size_t)size + 1);
            if (text) memcpy(text, raw, size);
            free(raw);
        }
    }
    c->text = calloc(1, sizeof *c->text);
    if (c->text &&
        !aoe_credits_text_init_ror(c->text, ui->system, ez->panel.area,
                                   &ez->panel, ez->panel.width / 2 - 0xfa,
                                   ez->panel.height / 2 - 0x96, 500, 300, 0x1e,
                                   font, fw, fh, text)) {
        /* the panel stays, empty, as in the original */
    }
    free(text);
    c->pictures[0] = ez->background;
    for (i = 2; i <= 7; ++i) {
        char name[16];
        snprintf(name, sizeof name, "credit%d", i);
        c->pictures[i - 1] = eops && eops->load_shape
                                 ? eops->load_shape(eops->context, name,
                                                    0xc3e9 + i)
                                 : NULL;
    }
    music(c, 0xf, 0xf);
    return 1;
}

void aoe_credits_screen_leave_ror(AoeCreditsScreenRor *c) {
    AoeGameUiRor *ui = c->ui;
    AoeUiSystemRor *s = ui->system;
    const char *show = "Game Screen";
    if (aoe_ui_system_find_ror(s, "Game Screen")) {
        if (ui->settings && aoe_settings_flag_97d_ror(ui->settings) == 1 &&
            ui->game && ui->game->comm_state == 0)
            aoe_game_comm_update_ror(ui->game, 0, 0, ui->game_ops);
    } else {
        AoeMainMenuRor *m = calloc(1, sizeof *m);
        if (m) aoe_main_menu_init_ror(m, ui, c->ez.panel.area, ui->picture_ops);
        show = "Main Menu";
    }
    aoe_ui_system_show_named_ror(s, show, 0);
    aoe_ui_system_close_ror(s, "Credits Screen");
}

/* Draw the new picture and show it. */
static void show_picture(AoeCreditsScreenRor *c) {
    AoeUiPanelRor *p = &c->ez.panel;
    AoeDrawSystemRor *d = draw_system(c);
    p->vt->set_redraw(p, 1);
    p->vt->handle_paint(p);
    if (d) aoe_draw_present_ror(d, NULL);
}

static int32_t credits_idle(AoeUiPanelRor *p) {
    AoeCreditsScreenRor *c = CR(p);
    const AoeUiPlatformRor *pf = p->system ? p->system->platform : NULL;
    uint32_t now = clock_ms((void *)pf);
    switch (c->state) {
    case 1:
        if (c->shown_ms == 0) {
            c->shown_ms = now;
        } else if (now - c->shown_ms > 9999) {
            if (fades(c)) {
                AoeDrawSystemRor *d = draw_system(c);
                c->state = 2;
                c->fade_color = 0;
                memset(c->palette, 0, sizeof c->palette);
                if (!c->ez.palette || !d->ops->window_palette ||
                    !d->ops->window_palette(d->ops->context,
                                            (void *)c->ez.palette, c->palette))
                    aoe_surface_palette_ror(p->area->surface, c->palette);
                aoe_palette_fade_start_ror(&c->fade, d, c->fade_color, 0.5f, 1,
                                           NULL, 0xff, -1, now);
            } else {
                c->state = 4;
            }
        }
        break;
    case 2:
        if (aoe_palette_fade_step_ror(&c->fade, now)) {
            c->state = 4;
            c->switch_ms = 0;
            show_picture(c);
        }
        break;
    case 4:
        if (c->switch_ms == 0) {
            c->switch_ms = now;
            break;
        }
        c->state = 5;
        if (++c->picture > 7) c->picture = 1;
        c->ez.background = c->pictures[c->picture - 1];
        show_picture(c);
        if (fades(c)) {
            aoe_palette_fade_start_ror(&c->fade, draw_system(c), c->fade_color,
                                       0.5f, 2, c->palette, 0xff, -1, now);
        } else {
            c->state = 1;
            c->shown_ms = now;
        }
        break;
    case 5:
        if (aoe_palette_fade_step_ror(&c->fade, now)) {
            c->state = 1;
            c->shown_ms = now;
        }
        break;
    default:
        break;
    }
    if (c->leave) {
        aoe_credits_screen_leave_ror(c);
        return 1;
    }
    return aoe_ui_panel_handle_idle_ror(p);
}

static void credits_draw(AoeUiPanelRor *p) {
    AoeCreditsScreenRor *c = CR(p);
    int i;
    if (c->state == 4 && fades(c)) {
        aoe_surface_fill_ror(p->area->surface, NULL, 0);
        return;
    }
    if (c->preload) {
        for (i = 0; i < 7; ++i) {
            c->ez.background = c->pictures[i];
            aoe_ui_ez_draw_slot_ror(p);
        }
        c->ez.background = c->pictures[0];
        c->preload = 0;
    }
    aoe_ui_ez_draw_slot_ror(p);
}

static int32_t credits_key(AoeUiPanelRor *p, int32_t key, int32_t a,
                           int32_t b, int32_t c, int32_t d) {
    (void)a, (void)b, (void)c, (void)d;
    if (key != 0xd && key != 0x1b) return 0;
    aoe_credits_screen_leave_ror(CR(p));
    return 1;
}

static int32_t credits_mouse(AoeUiPanelRor *p, uint8_t button, int32_t x,
                             int32_t y, int32_t c, int32_t s) {
    (void)button, (void)x, (void)y, (void)c, (void)s;
    aoe_credits_screen_leave_ror(CR(p));
    return 1;
}

static int32_t credits_action(AoeUiPanelRor *p, AoeUiPanelRor *from,
                              int32_t code, int32_t value, int32_t extra) {
    AoeCreditsScreenRor *c = CR(p);
    if (from && c->text && from == &c->text->text.panel && code == 1) {
        c->leave = 1;
        return 1;
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_credits_screen_vtable_ror = {
    AOE_UI_EZ_SLOTS_MD(credits_destroy, credits_draw, credits_idle, aoe_ui_panel_handle_paint_ror,
                      credits_key, aoe_ui_panel_zero_ror, credits_action,
                      aoe_ui_screen_focus_slot_ror, credits_mouse),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
