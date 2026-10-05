#include "command_button.h"

#include <stdio.h>
#include <string.h>

#define CB(p) ((AoeCommandButtonRor *)(p))

static AoeUiButtonVtblRor vtable;
static int vtable_ready;

const AoeUiButtonVtblRor *aoe_command_button_vtable_ror(void) {
    if (!vtable_ready) {
        vtable = aoe_ui_button_vtable_ror;
        vtable.base.draw = aoe_command_button_draw_ror;
        vtable.base.is_inside = aoe_command_button_inside_ror;
        vtable_ready = 1;
    }
    return &vtable;
}

int aoe_command_button_init_ror(AoeCommandButtonRor *c, AoeUiSystemRor *system,
                                const AoeUiButtonOpsRor *button_ops,
                                const AoeCommandButtonOpsRor *ops,
                                AoeUiAreaRor *area, AoeUiPanelRor *parent,
                                void *sound, uintptr_t font, int32_t font_w,
                                int32_t font_h, void *background) {
    aoe_ui_button_init_ror(&c->button, system, button_ops);
    c->style = 0;
    c->player_colors = 0;
    c->background_frame = 0;
    c->help[0] = '\0';
    c->field_3d0 = 0;
    c->show_count = 0;
    c->count = 0;
    c->background = background;
    c->ops = ops;
    c->cover = NULL;
    c->button.panel.vt = &aoe_command_button_vtable_ror()->base;
    if (!aoe_ui_button_setup_ror(&c->button, area, parent, 0, 0, 0, 0, 1, sound,
                                 1, -1)) {
        c->button.panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_button_set_font_ror(&c->button, font, font_w, font_h);
    return 1;
}

void aoe_command_button_set_help_ror(AoeCommandButtonRor *c, const char *text) {
    if (!text) {
        c->help[0] = '\0';
        return;
    }
    strncpy(c->help, text, sizeof c->help);
    c->help[sizeof c->help - 1] = '\0';
}

void aoe_command_button_show_count_ror(AoeCommandButtonRor *c, int32_t show) {
    c->show_count = show;
    c->button.panel.vt->set_redraw(&c->button.panel, 1);
}

void aoe_command_button_set_count_ror(AoeCommandButtonRor *c, int32_t count) {
    c->count = count;
    c->button.panel.vt->set_redraw(&c->button.panel, 1);
}

int32_t aoe_command_button_inside_ror(AoeUiPanelRor *p, int32_t x, int32_t y) {
    AoeCommandButtonRor *c = CB(p);
    AoeUiPanelRor *cover;
    if (!aoe_ui_panel_is_inside_ror(p, x, y)) return 0;
    if ((c->style == 2 || c->style == 3) && c->cover &&
        (cover = *c->cover) != NULL && cover->vt->is_inside(cover, x, y))
        return 0;
    return 1;
}

static void shape(AoeCommandButtonRor *c, void *s, int32_t frame, int32_t x,
                  int32_t y, const AoeUiRectRor *clip, int32_t remap,
                  int disabled) {
    if (c->ops && c->ops->draw_shape)
        c->ops->draw_shape(c->ops->context, c->button.panel.area, s, frame, x,
                           y, clip, remap, disabled);
}

/* A centred line with its shadow one pixel down-left. */
static void line(AoeUiButtonRor *b, const AoeUiPlatformRor *pf,
                 const char *text, int32_t cx, int32_t y, int shift) {
    int16_t st = b->state;
    int32_t n = (int16_t)strlen(text);
    int32_t w = pf->text_width ? pf->text_width(pf->context, b->font, text, n) : 0;
    int32_t x = cx - w / 2 + b->panel.x;
    if (shift) ++x, ++y;
    pf->text_out(pf->context, x - 1, y + 1, text, n, b->text_shadow[st]);
    pf->text_out(pf->context, x, y, text, n, b->text_color[st]);
}

void aoe_command_button_draw_ror(AoeUiPanelRor *p) {
    AoeCommandButtonRor *c = CB(p);
    AoeUiButtonRor *b = &c->button;
    const AoeUiPlatformRor *pf = p->system ? p->system->platform : NULL;
    int16_t st = b->state;
    if (c->style == 3) {
        aoe_ui_button_draw_slot_ror(p);
        return;
    }
    if (!p->area || !p->visible || !p->active) {
        p->redraw = 0;
        return;
    }
    p->vt->begin_draw(p, 0);
    if (p->parent) p->parent->vt->draw_rect(p->parent, &p->clip);
    if (c->style <= 1 && c->background)
        shape(c, c->background, (b->pressed != 0) + c->background_frame, p->x,
              p->y, &p->clip, -1, 0);
    else if (p->parent)
        p->parent->vt->draw_rect(p->parent, &p->clip);
    if (c->style != 1 || !b->text1[0]) {
        if (b->pictures[0] && b->frames[0] >= 0) {
            int16_t frame = b->frames[0];
            int32_t inset = 0, x, y, remap = -1;
            AoeUiRectRor r = p->clip;
            if (c->style == 0) {
                inset = b->pressed ? 3 : 2;
                r.left += inset;
                r.top += inset;
                r.right -= 2;
                r.bottom -= inset;
            } else if (c->style == 1) {
                r.left += 1;
                r.top += 1;
                r.right -= 1;
                r.bottom -= 1;
                inset = b->pressed ? 2 : 1;
            } else if (b->pressed) {
                ++frame;
            }
            x = p->x + inset;
            y = p->y + inset;
            if (c->player_colors && c->ops && c->ops->player_color)
                remap = (c->ops->player_color(c->ops->context) + 1) * 0x10;
            shape(c, b->pictures[0], frame, x, y, &r, remap, b->disabled != 0);
        }
    } else if (pf && pf->text_begin &&
               pf->text_begin(pf->context, p->area, &p->clip, b->font)) {
        const char *s1 = b->text1[st] ? b->text1[st] : b->text1[0];
        int shift = b->invert_press ? !b->pressed : b->pressed != 0;
        if (s1) {
            const char *s2 = b->text2[st] ? b->text2[st] : b->text2[0];
            int32_t y = (b->text2[st] ? p->y - b->font_height
                                      : p->y - b->font_height / 2) +
                        p->height / 2;
            line(b, pf, s1, p->width / 2, y, shift);
            if (s2) line(b, pf, s2, p->width / 2, p->y + p->height / 2, shift);
        }
        pf->text_end(pf->context);
    }
    if (c->show_count && c->count > 0 && pf && pf->text_begin &&
        pf->text_begin(pf->context, p->area, &p->clip, 0)) {
        char n[20];
        int32_t d = b->pressed != 0;
        snprintf(n, sizeof n, "%d", c->count);
        pf->text_out(pf->context, p->x + 3 + d, p->y + 2 + d, n,
                     (int32_t)strlen(n), 0xcccccc);
        pf->text_end(pf->context);
    }
    p->vt->end_draw(p);
}
