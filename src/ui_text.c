#include "ui_text.h"

#include "draw_prims.h"
#include "draw_system.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define T(p) ((AoeUiTextRor *)(p))
#define TVT(t) ((const AoeUiTextVtblRor *)(t)->panel.vt)

static void redraw(AoeUiTextRor *t, int32_t level) {
    t->panel.vt->set_redraw(&t->panel, level);
}

void aoe_ui_text_init_ror(AoeUiTextRor *t, AoeUiSystemRor *system) {
    if (!t) return;
    memset(t, 0, sizeof(*t));
    aoe_ui_panel_init_ror(&t->panel, system);
    t->panel.vt = &aoe_ui_text_vtable_ror.base;
    t->fill_color = 0xff;
    t->shadow_style = 2;
    t->color = 0xffffff;
    t->focus_color = 0xffff;
    t->panel.handles_mouse = 0;
    t->panel.field_ee = 1;
}

void aoe_ui_text_release_ror(AoeUiTextRor *t) {
    if (!t) return;
    aoe_ui_text_clear_ror(t);
    aoe_ui_panel_release_ror(&t->panel);
}

static void text_destroy(AoeUiPanelRor *p) { aoe_ui_text_release_ror(T(p)); }

static void free_line(AoeUiLineRor *line) {
    free(line->text);
    free(line->column);
    free(line);
}

void aoe_ui_text_layout_ror(AoeUiTextRor *t, int32_t update) {
    int32_t pitch;
    if (!t) return;
    t->pitch = t->line_height + 2;
    if (!t->line_height) {
        t->visible = 1;
    } else {
        int32_t space = t->panel.height - t->margin * 2;
        if (t->spacing) pitch = t->line_height + 1 + t->spacing * 2;
        else pitch = t->pitch;
        t->visible = (int16_t)(space / pitch);
    }
    if (t->visible < 1) t->visible = 1;
    if (t->count <= t->top) t->top = (int16_t)(t->count - 1);
    if (t->count - t->top < t->visible) t->top = (int16_t)(t->count - t->visible);
    if (t->top < 0) t->top = 0;
    t->bottom = (int16_t)(t->top - 1 + t->visible);
    if (t->bottom < 0) t->bottom = 0;
    if (t->count <= t->bottom) t->bottom = (int16_t)(t->count - 1);
    if (t->selected < 0) t->selected = 0;
    if (t->count <= t->selected) t->selected = (int16_t)(t->count - 1);
    if (t->scrollbar) {
        if (update && t->scroll && t->scroll->set_range)
            t->scroll->set_range(t->scroll->context,
                                 t->count - t->visible + 1, t->top);
        if (t->has_scrollbar) {
            if (t->visible < t->count) {
                if (!t->scrollbar_shown) {
                    aoe_ui_panel_set_rect_ror(&t->panel, t->panel.x, t->panel.y,
                                              t->full_width - t->scrollbar_width,
                                              t->panel.height);
                    t->scrollbar_shown = 1;
                    if (t->panel.active) {
                        t->scrollbar->vt->set_active(t->scrollbar, 1);
                        t->text_width = t->panel.width;
                        return;
                    }
                }
            } else if (t->scrollbar_shown) {
                aoe_ui_panel_set_rect_ror(&t->panel, t->panel.x, t->panel.y,
                                          t->full_width, t->panel.height);
                t->scrollbar_shown = 0;
                if (t->panel.active)
                    t->scrollbar->vt->set_active(t->scrollbar, 0);
            }
        }
    }
    t->text_width = t->panel.width;
}

/* CompareStringA(user locale, NORM_IGNORECASE): byte order ignoring case. */
static int compare_nocase(const char *a, const char *b) {
    for (;; ++a, ++b) {
        int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
        if (ca != cb || !ca) return ca - cb;
    }
}

static char *dup_n(const char *text, size_t n) {
    char *out = malloc(n + 1);
    if (out) {
        memcpy(out, text, n);
        out[n] = '\0';
    }
    return out;
}

int aoe_ui_text_insert_ror(AoeUiTextRor *t, int32_t index, const char *text,
                           const char *column, int32_t id) {
    AoeUiLineRor *line, *at;
    if (!t || !(line = calloc(1, sizeof(*line)))) return 0;
    if (!text) line->text = dup_n("", 0);
    else if (!t->max_chars) line->text = dup_n(text, strlen(text));
    else {
        size_t n = strlen(text);
        line->text = dup_n(text, n < (size_t)t->max_chars ? n : (size_t)t->max_chars);
    }
    if (!line->text || (column && !(line->column = dup_n(column, strlen(column))))) {
        free_line(line);
        return 0;
    }
    line->id = id;
    at = t->lines;
    if (!at) {
        t->lines = line;
    } else if (!t->sorted) {
        if (index == 0) {
            line->next = at;
            t->lines = line;
        } else {
            if (index < t->count) {
                for (int32_t i = 0; i != index - 1; ++i) at = at->next;
                line->next = at->next;
            } else {
                while (at->next) at = at->next;
            }
            at->next = line;
        }
    } else {
        AoeUiLineRor *prev = NULL;
        while (at && compare_nocase(line->text, at->text) >= 0) {
            prev = at;
            at = at->next;
        }
        line->next = at;
        if (prev) prev->next = line;
        else t->lines = line;
    }
    ++t->count;
    aoe_ui_text_layout_ror(t, 1);
    redraw(t, 1);
    return 1;
}

int aoe_ui_text_append_ror(AoeUiTextRor *t, const char *text, int32_t id) {
    return t ? aoe_ui_text_insert_ror(t, t->count, text, NULL, id) : 0;
}

int aoe_ui_text_append_id_ror(AoeUiTextRor *t, int32_t string_id, int32_t id) {
    char text[0x100];
    if (!t) return 0;
    aoe_ui_string_ror(t->panel.system, string_id, text, sizeof(text));
    return aoe_ui_text_insert_ror(t, t->count, text, NULL, id);
}

int aoe_ui_text_replace_ror(AoeUiTextRor *t, int32_t index, const char *text,
                            const char *column, int32_t id) {
    int32_t sorted;
    if (!t) return 0;
    sorted = t->sorted;
    t->sorted = 0;
    if (!aoe_ui_text_insert_ror(t, index, text, column, id)) {
        t->sorted = sorted;
        return 0;
    }
    aoe_ui_text_remove_ror(t, index + 1);
    t->sorted = sorted;
    return 1;
}

int aoe_ui_text_remove_ror(AoeUiTextRor *t, int32_t index) {
    AoeUiLineRor *line;
    if (!t || index < 0 || index > t->count - 1) return 0;
    if (index == 0) {
        line = t->lines;
        t->lines = line->next;
    } else {
        AoeUiLineRor *at = t->lines;
        for (int32_t i = 0; i != index - 1; ++i) at = at->next;
        line = at->next;
        at->next = line->next;
    }
    if (!line) return 0;
    free_line(line);
    --t->count;
    if (t->count == 0) t->selected = 0;
    else if (t->count <= t->selected) t->selected = (int16_t)(t->count - 1);
    aoe_ui_text_layout_ror(t, 1);
    redraw(t, 1);
    return 1;
}

void aoe_ui_text_clear_ror(AoeUiTextRor *t) {
    AoeUiLineRor *line;
    if (!t) return;
    line = t->lines;
    while (line) {
        AoeUiLineRor *next = line->next;
        free_line(line);
        line = next;
    }
    t->lines = NULL;
    t->count = 0;
    aoe_ui_text_layout_ror(t, 1);
    redraw(t, 1);
}

int16_t aoe_ui_text_count_ror(const AoeUiTextRor *t) { return t ? t->count : 0; }
int16_t aoe_ui_text_visible_ror(const AoeUiTextRor *t) { return t ? t->visible : 0; }
int16_t aoe_ui_text_selected_ror(const AoeUiTextRor *t) { return t ? t->selected : 0; }

int32_t aoe_ui_text_selection_ror(const AoeUiTextRor *t) {
    return !t || !t->count ? -1 : t->selected;
}

static AoeUiLineRor *line_at(const AoeUiTextRor *t, int32_t index) {
    AoeUiLineRor *line = t ? t->lines : NULL;
    for (int32_t i = 0; line && i != index; ++i) line = line->next;
    return line;
}

int32_t aoe_ui_text_index_of_id_ror(const AoeUiTextRor *t, int32_t id) {
    int32_t i = 0;
    for (AoeUiLineRor *l = t ? t->lines : NULL; l; l = l->next, ++i)
        if (l->id == id) return i;
    return -1;
}

int32_t aoe_ui_text_index_of_ror(const AoeUiTextRor *t, const char *text) {
    int32_t i = 0;
    for (AoeUiLineRor *l = t ? t->lines : NULL; l && text; l = l->next, ++i)
        if (!strcmp(l->text, text)) return i;
    return -1;
}

const char *aoe_ui_text_line_ror(const AoeUiTextRor *t, int32_t index) {
    AoeUiLineRor *line = index < 0 ? NULL : line_at(t, index);
    return line ? line->text : "";
}

const char *aoe_ui_text_selected_text_ror(const AoeUiTextRor *t) {
    return aoe_ui_text_line_ror(t, t ? t->selected : 0);
}

const char *aoe_ui_text_column_ror(const AoeUiTextRor *t, int32_t index) {
    AoeUiLineRor *line = index < 0 ? NULL : line_at(t, index);
    return line ? line->column : "";
}

int32_t aoe_ui_text_id_ror(const AoeUiTextRor *t, int32_t index) {
    AoeUiLineRor *line = index < 0 ? NULL : line_at(t, index);
    return line ? line->id : -1;
}

int32_t aoe_ui_text_selected_id_ror(const AoeUiTextRor *t) {
    return aoe_ui_text_id_ror(t, t ? t->selected : 0);
}

void aoe_ui_text_select_ror(AoeUiTextRor *t, int16_t index) {
    if (!t) return;
    t->selected = index;
    aoe_ui_text_layout_ror(t, 1);
}

void aoe_ui_text_select_id_ror(AoeUiTextRor *t, int32_t id) {
    int32_t i = aoe_ui_text_index_of_id_ror(t, id);
    if (i >= 0) aoe_ui_text_select_ror(t, (int16_t)i);
}

void aoe_ui_text_scroll_ror(AoeUiTextRor *t, uint8_t how, int16_t value,
                            int32_t update) {
    if (!t) return;
    switch (how) {
    case 0: t->top = (int16_t)(t->top + value); break;
    case 1: t->top = value; break;
    case 2: ++t->top; break;
    case 3: --t->top; break;
    case 4: t->top = (int16_t)(t->top + t->visible - 1); break;
    case 5: t->top = (int16_t)(t->top + 1 - t->visible); break;
    case 6: t->top = 0; break;
    case 7: t->top = (int16_t)(t->count - 1); break;
    default: break;
    }
    aoe_ui_text_layout_ror(t, update);
    redraw(t, 1);
}

int32_t aoe_ui_text_key_action_ror(AoeUiPanelRor *p, int32_t key, int32_t count,
                          int32_t alt, int32_t ctrl, int32_t shift) {
    AoeUiTextRor *t = T(p);
    uint8_t how;
    switch (key) {
    case 0x21: how = 5; break;
    case 0x22: how = 4; break;
    case 0x23: aoe_ui_text_scroll_ror(t, 7, 0, 1); return 1;
    case 0x24: aoe_ui_text_scroll_ror(t, 6, 0, 1); return 1;
    case 0x26: how = 3; break;
    case 0x28: how = 2; break;
    default: return 0;
    }
    aoe_ui_text_scroll_ror(t, how, 0, 1);
    if (p->parent && p->tab_stop)
        p->parent->vt->key_action(p->parent, key, count, alt, ctrl, shift);
    return 1;
}

int32_t aoe_ui_text_action_ror(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code,
                      int32_t a, int32_t b) {
    AoeUiTextRor *t = T(p);
    if (t->scrollbar && from == t->scrollbar) {
        switch (code) {
        case 0: aoe_ui_text_scroll_ror(t, 3, 0, 1); return 1;
        case 1: aoe_ui_text_scroll_ror(t, 2, 0, 1); return 1;
        case 2: aoe_ui_text_scroll_ror(t, 5, 0, 1); return 1;
        case 3: aoe_ui_text_scroll_ror(t, 4, 0, 1); return 1;
        case 4: aoe_ui_text_scroll_ror(t, 1, (int16_t)a, 0); return 1;
        default: break;
        }
    }
    return aoe_ui_panel_action_ror(p, from, code, a, b);
}

void aoe_ui_text_set_font_ror(AoeUiTextRor *t, uintptr_t font, int32_t width,
                              int32_t line_height) {
    if (!t) return;
    t->font_width = width;
    t->font = font;
    t->line_height = line_height;
    aoe_ui_text_layout_ror(t, 1);
    redraw(t, 1);
}

void aoe_ui_text_set_wrap_ror(AoeUiTextRor *t, int32_t wrap) {
    if (!t) return;
    t->wrap = wrap;
    redraw(t, 1);
}

void aoe_ui_text_set_colors_ror(AoeUiTextRor *t, uint32_t color,
                                uint32_t shadow) {
    if (!t) return;
    t->color = color;
    t->shadow = shadow;
    redraw(t, 1);
}

void aoe_ui_text_set_focus_colors_ror(AoeUiTextRor *t, uint32_t color,
                                      uint32_t shadow) {
    if (!t) return;
    t->focus_color = color;
    t->focus_shadow = shadow;
    redraw(t, 1);
}

void aoe_ui_text_set_background_ror(AoeUiTextRor *t, void *picture,
                                    int16_t width, int16_t height) {
    if (!t) return;
    t->background = picture;
    t->background_w = picture ? width : 0;
    t->background_h = picture ? height : 0;
}

void aoe_ui_text_set_border_ror(AoeUiTextRor *t, int32_t border,
                                uint8_t color) {
    if (!t) return;
    t->border = border;
    t->bevel_type = border ? 1 : 0;
    t->border_color = color;
    redraw(t, 1);
}

void aoe_ui_text_set_bevel_ror(AoeUiTextRor *t, int32_t type,
                               const uint8_t colors[6]) {
    if (!t) return;
    if (colors) memcpy(t->bevel, colors, sizeof(t->bevel));
    t->bevel_type = type;
    switch (type) {
    case 1: case 2: case 5: t->margin = 1; break;
    case 3: case 6: t->margin = 2; break;
    case 4: case 7: t->margin = 3; break;
    default: t->margin = 0; break;
    }
    aoe_ui_text_layout_ror(t, 1);
}

void aoe_ui_text_bevel_slot_ror(AoeUiTextRor *t, int32_t type, const uint8_t c[6]) {
    aoe_ui_text_set_bevel_ror(t, type, c);
}

void aoe_ui_text_set_spacing_ror(AoeUiTextRor *t, int32_t spacing) {
    if (!t) return;
    t->spacing = spacing;
    aoe_ui_text_layout_ror(t, 1);
}

void aoe_ui_text_set_sorted_ror(AoeUiTextRor *t, int32_t sorted) {
    if (!t) return;
    t->sorted = sorted;
    redraw(t, 1);
}

void aoe_ui_text_set_scrollbar_ror(AoeUiTextRor *t, AoeUiPanelRor *scrollbar,
                                   int32_t shown) {
    if (!t) return;
    t->scrollbar = scrollbar;
    t->has_scrollbar = shown;
    if (scrollbar) {
        t->scrollbar_width = scrollbar->width;
        t->panel.vt->set_rect(&t->panel, t->panel.x, t->panel.y,
                              t->panel.width, t->panel.height);
    }
}

void aoe_ui_text_set_rect_slot_ror(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t w, int32_t h) {
    AoeUiTextRor *t = T(p);
    aoe_ui_panel_set_rect_ror(p, x, y, w, h);
    t->full_width = p->width;
    if (t->has_scrollbar && t->scrollbar) {
        int32_t rx = p->x, ry = p->y;
        t->scrollbar->vt->set_active(t->scrollbar, 0);
        t->scrollbar_shown = 0;
        if (p->parent) {
            rx -= p->parent->x;
            ry -= p->parent->y;
        }
        t->scrollbar->vt->set_fixed_position(t->scrollbar,
                                             t->full_width - t->scrollbar_width + rx,
                                             ry, t->scrollbar_width, p->height);
    }
    aoe_ui_text_layout_ror(t, 1);
    if (t->column_x != 0) {
        t->has_column_clip = 0;
        if (p->visible) {
            int32_t right = p->rect.left - 10 + t->column_x;
            if (right > p->clip.right) right = p->clip.right;
            t->column_clip.left = p->clip.left;
            t->column_clip.top = p->clip.top;
            t->column_clip.right = right;
            t->column_clip.bottom = p->clip.bottom;
            t->has_column_clip = 1;
        }
    }
}

/* ---- drawing ---- */

static const AoeUiPlatformRor *platform(const AoeUiTextRor *t) {
    return t->panel.system ? t->panel.system->platform : NULL;
}

/* The area's surface locked for drawing, or NULL. */
static AoeShapeSurfaceRor *lock_area(AoeUiTextRor *t) {
    struct AoeDrawSurfaceRor *s = t->panel.area ? t->panel.area->surface : NULL;
    if (!s || !aoe_surface_lock_ror(s, 1)) return NULL;
    return &s->view;
}

static void unlock_area(AoeUiTextRor *t) {
    struct AoeDrawSurfaceRor *s = t->panel.area ? t->panel.area->surface : NULL;
    if (s) aoe_surface_unlock_ror(s);
}

void aoe_ui_text_draw_background_ror(AoeUiTextRor *t) {
    AoeUiPanelRor *p = &t->panel;
    if (t->background) {
        const AoeUiPlatformRor *pf = platform(t);
        int16_t cols, rows = 0, i, j;
        if (!lock_area(t)) return;
        if (t->background_w < p->width) {
            cols = (int16_t)(p->width / t->background_w);
            if (cols * t->background_w < p->width) ++cols;
        } else {
            cols = 1;
        }
        /* The original only counts rows when the panel is no taller than
           a tile (so a taller panel gets none). */
        if (p->height <= t->background_h) {
            rows = (int16_t)(p->height / t->background_h);
            if (rows * t->background_h < p->height) ++rows;
        }
        for (j = 0; j < rows; ++j)
            for (i = 0; i < cols; ++i)
                if (pf && pf->draw_shape)
                    pf->draw_shape(pf->context, p->area, t->background, 0,
                                   t->background_w * i + p->x,
                                   t->background_h * j + p->y);
        unlock_area(t);
        return;
    }
    if (t->fill) {
        AoeShapeSurfaceRor *v = lock_area(t);
        if (!v) return;
        aoe_draw_fill_ror(v, p->x, p->y, p->width - 1 + p->x,
                          p->height - 1 + p->y, t->fill_color);
        unlock_area(t);
        return;
    }
    if (p->parent) {
        if (t->bevel_type > 1 && t->bevel_type < 8) {
            p->parent->vt->draw_parent_rect(p->parent, &p->clip);
            return;
        }
        if (!p->parent->painting) p->parent->vt->draw_rect(p->parent, &p->clip);
    }
}

void aoe_ui_text_draw_border_ror(AoeUiTextRor *t) {
    AoeUiPanelRor *p = &t->panel;
    AoeShapeSurfaceRor *v;
    const uint8_t *b = t->bevel;
    int32_t l = p->x, top = p->y, r = p->width - 1 + p->x,
            bot = p->height - 1 + p->y;
    if (t->bevel_type == 0) return;
    v = lock_area(t);
    if (v) {
        uint8_t c[6];
        switch (t->bevel_type) {
        case 1:
            aoe_draw_rect_ror(v, l, top, r, bot, t->border_color);
            break;
        case 2:
            aoe_draw_bevel1_ror(v, l, top, r, bot, b[5], b[0]);
            break;
        case 5:
            aoe_draw_bevel1_ror(v, l, top, r, bot, b[0], b[5]);
            break;
        case 3:
            c[0] = b[5], c[1] = b[4], c[2] = b[1], c[3] = b[0];
            aoe_draw_bevel2_ror(v, l, top, r, bot, c);
            break;
        case 6:
            c[0] = b[0], c[1] = b[1], c[2] = b[4], c[3] = b[5];
            aoe_draw_bevel2_ror(v, l, top, r, bot, c);
            break;
        case 4:
            c[0] = b[5], c[1] = b[4], c[2] = b[3], c[3] = b[2], c[4] = b[1],
            c[5] = b[0];
            aoe_draw_bevel3_ror(v, l, top, r, bot, c);
            break;
        case 7:
            aoe_draw_bevel3_ror(v, l, top, r, bot, b);
            break;
        }
    }
    unlock_area(t);
}

/* One string with the panel's shadow style. */
static void out(const AoeUiPlatformRor *pf, int32_t style, int32_t x,
                int32_t y, const char *text, int32_t n, uint32_t color,
                uint32_t shadow) {
    if (style == 1) {
        pf->text_out(pf->context, x - 1, y + 1, text, n, color);
        pf->text_out(pf->context, x, y, text, n, shadow);
    } else if (style == 2) {
        pf->text_out(pf->context, x - 1, y + 1, text, n, shadow);
        pf->text_out(pf->context, x, y, text, n, color);
    } else if (style == 0) {
        pf->text_out(pf->context, x, y, text, n, color);
    }
}

void aoe_ui_text_draw_line_ror(AoeUiTextRor *t, int16_t row, int16_t line,
                               uint32_t color, uint32_t shadow) {
    const AoeUiPlatformRor *pf = platform(t);
    const char *a = aoe_ui_text_line_ror(t, line);
    const char *b = aoe_ui_text_column_ror(t, line);
    AoeUiRectRor rect;
    int32_t offset, x, x2;
    if (!a && !b) return;
    aoe_ui_text_line_rect_ror(t, row, line, &rect, &offset);
    x = rect.left;
    if (t->align != 0 && t->align != 6) x = rect.left + 5;
    x2 = t->column_x + x;
    if (t->has_column_clip) pf->text_clip(pf->context, &t->column_clip);
    if (a) out(pf, t->shadow_style, x, rect.top, a, (int32_t)strlen(a), color,
               shadow);
    if (b) {
        if (t->has_column_clip) pf->text_clip(pf->context, &t->panel.clip);
        out(pf, t->shadow_style, x2, rect.top, b, (int32_t)strlen(b), color,
            shadow);
    }
}

void aoe_ui_text_draw_slot_ror(AoeUiPanelRor *p) {
    AoeUiTextRor *t = T(p);
    const AoeUiPlatformRor *pf = platform(t);
    int16_t line, row = 0;
    p->redraw = 0;
    if (!p->area || !p->visible || !p->active) return;
    aoe_ui_text_draw_background_ror(t);
    p->vt->begin_draw(p, 0);
    if (pf && pf->text_begin && pf->text_begin(pf->context, p->area, &p->clip,
                                               t->font)) {
        for (line = t->top; line <= t->bottom; ++line, ++row) {
            int focus = !((p->tab_stop == 0 || p->has_focus == 0) &&
                          p->captured == 0);
            aoe_ui_text_draw_line_ror(t, row, line,
                                      focus ? t->focus_color : t->color,
                      focus ? t->focus_shadow : t->shadow);
        }
        pf->text_end(pf->context);
    }
    aoe_ui_text_draw_border_ror(t);
    p->vt->end_draw(p);
}

void aoe_ui_text_set_active_slot_ror(AoeUiPanelRor *p, int32_t active) {
    AoeUiTextRor *t = T(p);
    aoe_ui_panel_set_active_ror(p, active);
    if (t->has_scrollbar && t->scrollbar)
        t->scrollbar->vt->set_active(t->scrollbar,
                                     p->active ? t->scrollbar_shown : 0);
}

int32_t aoe_ui_text_left_down_ror(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)x; (void)y; (void)c; (void)s;
    if (p->parent) p->parent->vt->action(p->parent, p, 1, 0, 0);
    return 0;
}

void aoe_ui_text_set_alignment_ror(AoeUiTextRor *t, int32_t vertical,
                                   int32_t align) {
    if (!t) return;
    t->vertical = vertical;
    t->align = align;
    redraw(t, 1);
}

int aoe_ui_text_setup_ror(AoeUiTextRor *t, AoeUiAreaRor *area,
                          AoeUiPanelRor *parent, int32_t x, int32_t y,
                          int32_t w, int32_t h, uintptr_t font,
                          int32_t font_width, int32_t line_height,
                          void *background, int16_t background_w,
                          int16_t background_h, int32_t fill,
                          uint8_t fill_color, int32_t border,
                          uint8_t border_color, int16_t max_chars,
                          const char *text) {
    if (!t) return 0;
    aoe_ui_panel_setup_ror(&t->panel, area, parent, x, y, w, h, 0);
    t->fill = fill;
    t->fill_color = fill_color;
    t->border = border;
    t->border_color = border_color;
    t->max_chars = max_chars;
    if (border) t->bevel_type = 1;
    ((const AoeUiTextVtblRor *)t->panel.vt)->set_text(t, text);
    aoe_ui_text_set_font_ror(t, font, font_width, line_height);
    aoe_ui_text_set_alignment_ror(t, 0, 0);
    aoe_ui_text_set_background_ror(t, background, background_w, background_h);
    return 1;
}

int aoe_ui_text_setup_lines_ror(AoeUiTextRor *t, AoeUiAreaRor *area,
                                AoeUiPanelRor *parent, int32_t x, int32_t y,
                                int32_t w, int32_t h, uintptr_t font,
                                int32_t font_width, int32_t line_height,
                                void *background, int16_t background_w,
                                int16_t background_h, int32_t fill,
                                uint8_t fill_color, int32_t border,
                                uint8_t border_color, int16_t max_chars,
                                const char *const *lines, int16_t count) {
    if (!t) return 0;
    aoe_ui_panel_setup_ror(&t->panel, area, parent, x, y, w, h, 0);
    t->fill = fill;
    t->fill_color = fill_color;
    t->border = border;
    t->border_color = border_color;
    t->max_chars = max_chars;
    if (border) t->bevel_type = 1;
    ((const AoeUiTextVtblRor *)t->panel.vt)->set_lines(t, lines, count);
    aoe_ui_text_set_font_ror(t, font, font_width, line_height);
    aoe_ui_text_set_alignment_ror(t, 3, 1);
    aoe_ui_text_set_background_ror(t, background, background_w, background_h);
    return 1;
}

int aoe_ui_text_set_text_ror(AoeUiTextRor *t, const char *text) {
    const char *nl;
    if (!t) return 0;
    aoe_ui_text_clear_ror(t);
    if (!text) {
        if (t->max_chars) aoe_ui_text_append_ror(t, "", 0);
        return 1;
    }
    if (!*text) return aoe_ui_text_append_ror(t, "", 0);
    if (t->wrap || t->align == 6) return aoe_ui_text_wrap_ror(t, text);
    while ((nl = strchr(text, '\n'))) {
        char line[0x100];
        size_t n = (size_t)(int16_t)(nl - text);
        if (n > sizeof(line) - 1) n = sizeof(line) - 1;
        memcpy(line, text, n);
        line[n] = '\0';
        aoe_ui_text_append_ror(t, line, 0);
        text = nl + 1;
    }
    return aoe_ui_text_append_ror(t, text, 0);
}

static int32_t width_of(AoeUiTextRor *t, const char *text, int32_t count) {
    const AoeUiPlatformRor *pf = t->panel.system ? t->panel.system->platform : NULL;
    return pf && pf->text_width ? pf->text_width(pf->context, t->font, text, count)
                                : 0;
}

int aoe_ui_text_wrap_ror(AoeUiTextRor *t, const char *text) {
    int32_t width;
    if (!t) return 0;
    if (!text || !*text) return 1;
    width = (t->has_scrollbar ? t->full_width - t->scrollbar_width
                              : t->panel.width) - 10;
    for (;;) {
        int32_t len = (int32_t)strlen(text), count, take = 0;
        int newline = 0, crlf = 0, has_cr = 0;
        char line[0x100];
        if (*text == '\n') {
            newline = 1;
        } else {
            const char *p = text, *prev = NULL;
            int32_t w = width_of(t, text, 1), last_fit;
            count = 1;
            while (count <= len && w < width) {
                if (*p == '\n') {
                    newline = 1;
                    take = count - 1;
                    if (prev && *prev == '\r') {
                        crlf = 1;
                        take = count - 2;
                    }
                    break;
                }
                if (*p == '\r') has_cr = 1;
                ++count;
                w = width_of(t, text, count);
                prev = p++;
            }
            if (!newline) {
                int32_t i = 1;
                if (width_of(t, text, len) <= width)
                    return aoe_ui_text_append_ror(t, text, 0);
                last_fit = 1;
                take = 1;
                if (len > 1) {
                    while (i < len) {
                        if (width < width_of(t, text, i)) break;
                        last_fit = i - 1;
                        ++i;
                    }
                    for (;;) {
                        take = last_fit;
                        if (i < 2) break;
                        --i;
                        take = i;
                        if (isspace((unsigned char)text[i - 1])) break;
                    }
                }
            }
        }
        if (take > (int32_t)sizeof(line) - 3) take = (int32_t)sizeof(line) - 3;
        memset(line, 0, sizeof(line));
        memcpy(line, text, (size_t)take);
        if (crlf) strcat(line, " ");
        if (newline) strcat(line, " ");
        if (has_cr)
            for (char *c = line; *c; ++c)
                if (*c == '\r') *c = ' ';
        aoe_ui_text_append_ror(t, line, 0);
        text += take;
        if (crlf) ++text;
        if (newline) ++text;
    }
}

int aoe_ui_text_set_lines_ror(AoeUiTextRor *t, const char *const *lines,
                              int16_t count) {
    if (!t) return 0;
    aoe_ui_text_clear_ror(t);
    if (count == 0) {
        if (t->max_chars) aoe_ui_text_append_ror(t, "", 0);
        return 1;
    }
    for (int16_t i = 0; i < count; ++i) aoe_ui_text_append_ror(t, lines[i], 0);
    return 1;
}

int aoe_ui_text_set_text_id_ror(AoeUiTextRor *t, int32_t string_id) {
    char text[0x1000];
    if (!t) return 0;
    aoe_ui_string_ror(t->panel.system, string_id, text, sizeof(text));
    return TVT(t)->set_text(t, text);
}

void aoe_ui_text_line_rect_ror(AoeUiTextRor *t, int16_t row, int16_t line,
                               AoeUiRectRor *out, int32_t *column_offset) {
    const char *text;
    int32_t prefix = 0, full = 0, x, y, pitch;
    if (!t || !out) return;
    text = line_at(t, line) ? line_at(t, line)->text : NULL;
    if (text) {
        if (t->column_chars) {
            int32_t n = (int32_t)strlen(text);
            prefix = width_of(t, text, n < t->column_chars ? n : t->column_chars);
        }
        full = width_of(t, text, (int32_t)strlen(text));
    }
    if (t->align != 0 && t->align != 6) prefix += 5;
    if (t->align == 1) {
        x = t->margin + t->panel.x;
    } else if (t->align == 2) {
        x = t->panel.width - t->margin + t->panel.x - full - 7;
    } else if (t->align == 5) {
        x = t->margin + t->panel.x;
        if (t->panel.width <= prefix + 1) x += t->panel.width - prefix - 2;
    } else {
        x = t->panel.width / 2 - full / 2 + t->panel.x;
    }
    pitch = t->spacing ? t->pitch - 1 + t->spacing * 2 : t->pitch;
    if (t->vertical == 3)
        y = row * pitch + t->margin + t->panel.y;
    else if (t->vertical == 4)
        y = t->panel.height - (t->count - row) * pitch - t->margin + t->panel.y -
            t->spacing - 1;
    else
        y = row * pitch - (t->count * pitch) / 2 + t->panel.height / 2 + t->panel.y;
    out->left = x;
    out->top = y;
    out->right = x - 1 + full;
    out->bottom = y + t->pitch;
    if (column_offset) *column_offset = prefix;
}

int aoe_ui_text_char_kind_ror(const uint8_t *text, int (*dbcs_lead)(uint8_t)) {
    if (!text || !*text) return 0;
    if (dbcs_lead && dbcs_lead(*text)) return 2;
    if (*text > 0xa0 && *text < 0xe0) return 1;
    return 0;
}

int aoe_ui_text_lines_slot_ror(AoeUiTextRor *t, const char *const *l, int16_t n) {
    return aoe_ui_text_set_lines_ror(t, l, n);
}

const AoeUiTextVtblRor aoe_ui_text_vtable_ror = {
    {text_destroy,
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
     aoe_ui_text_draw_slot_ror,
     aoe_ui_panel_draw_rect_ror,
     aoe_ui_panel_draw_offset_ror,
     aoe_ui_panel_draw_parent_rect_ror,
     aoe_ui_panel_draw_parent_offset_ror,
     aoe_ui_panel_validate_ror,
     aoe_ui_panel_handle_message_ror,
     aoe_ui_panel_handle_idle_ror,
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
    aoe_ui_text_set_text_ror,
    aoe_ui_text_bevel_slot_ror,
};
