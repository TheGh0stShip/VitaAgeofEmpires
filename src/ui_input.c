#include "ui_input.h"

#include <stdlib.h>
#include <string.h>

#define I(p) ((AoeUiInputRor *)(p))

static uint32_t now(const AoeUiInputRor *in) {
    const AoeUiPlatformRor *pf =
        in->text.panel.system ? in->text.panel.system->platform : NULL;
    return pf && pf->now_ms ? pf->now_ms(pf->context) : 0;
}

static void beep(AoeUiInputRor *in) {
    if (in->ops && in->ops->beep) in->ops->beep(in->ops->context, in->sound);
}

static void caret_on(AoeUiInputRor *in) {
    in->caret_on = 1;
    in->blink_at = now(in);
}

void aoe_ui_input_init_ror(AoeUiInputRor *in, AoeUiSystemRor *system,
                           const AoeUiInputOpsRor *ops, uint32_t blink_ms) {
    if (!in) return;
    aoe_ui_text_init_ror(&in->text, system);
    in->text.panel.vt = &aoe_ui_input_vtable_ror.base;
    in->ops = ops;
    in->caret_color = 0;
    in->blink_ms = blink_ms;
    in->caret_on = 1;
    in->insert = 1;
    in->text.panel.handles_mouse = 1;
    in->text.bevel_type = 1;
    in->blink_at = 0;
    in->sound = NULL;
    in->current = in->buffers[0] = in->buffers[1] = NULL;
    in->length = in->max = in->cursor = 0;
    in->text.panel.field_ee = 7;
    in->text.align = 5;
}

void aoe_ui_input_release_ror(AoeUiInputRor *in) {
    if (!in) return;
    free(in->buffers[0]);
    free(in->buffers[1]);
    in->buffers[0] = in->buffers[1] = in->current = NULL;
    in->text.panel.handles_mouse = 1;
    aoe_ui_text_release_ror(&in->text);
}

static void destroy(AoeUiPanelRor *p) { aoe_ui_input_release_ror(I(p)); }

int aoe_ui_input_setup_ror(AoeUiInputRor *in, AoeUiAreaRor *area,
                           AoeUiPanelRor *parent, int32_t x, int32_t y,
                           int32_t w, int32_t h, uintptr_t font,
                           int32_t font_width, int32_t line_height,
                           void *background, int16_t background_w,
                           int16_t background_h, int32_t fill,
                           uint8_t fill_color, int32_t border,
                           uint8_t border_color, uint8_t caret_color,
                           int16_t max, void *sound, const char *text,
                           int32_t format) {
    size_t size;
    if (!in || max < 0) return 0;
    if (!aoe_ui_text_setup_ror(&in->text, area, parent, x, y, w, h, font,
                               font_width, line_height, background,
                               background_w, background_h, fill, fill_color,
                               border, border_color,
                               format == 7 ? 0 : max, NULL))
        return 0;
    in->caret_color = caret_color;
    in->format = format;
    if (in->text.panel.has_focus) caret_on(in);
    in->sound = max ? sound : NULL;
    if (format == 7) {
        aoe_ui_text_set_alignment_ror(&in->text, 3, 1);
        aoe_ui_text_set_wrap_ror(&in->text, 1);
    } else {
        aoe_ui_text_set_alignment_ror(&in->text, 0, 5);
    }
    size = (size_t)max * 2 + 1;
    free(in->buffers[0]);
    free(in->buffers[1]);
    in->buffers[0] = calloc(size, 1);
    in->buffers[1] = in->buffers[0] ? calloc(size, 1) : NULL;
    if (!in->buffers[0] || !in->buffers[1]) return 0;
    in->current = in->buffers[0];
    ((const AoeUiTextVtblRor *)in->text.panel.vt)->set_text(&in->text, text);
    in->max = max;
    aoe_ui_input_relayout_ror(in);
    return 1;
}

const char *aoe_ui_input_text_ror(const AoeUiInputRor *in) {
    return in && in->current ? in->current : "";
}

int aoe_ui_input_set_text_ror(AoeUiTextRor *t, const char *text) {
    AoeUiInputRor *in = (AoeUiInputRor *)t;
    size_t cap;
    if (!in->current) return 1;
    cap = (size_t)in->max * 2;  /* the original copies unbounded */
    if (!text) {
        in->current[0] = 0;
    } else {
        size_t n = strlen(text);
        if (in->max && n > cap) n = cap;
        memcpy(in->current, text, n);
        in->current[n] = 0;
    }
    in->length = (int16_t)strlen(in->current);
    in->cursor = 0;
    in->text.selected = 0;
    in->text.column_chars = 0;
    if (in->format != 7 && in->length > 0) {
        in->cursor = in->length;
        in->text.column_chars = in->length;
    }
    aoe_ui_input_relayout_ror(in);
    return 1;
}

void aoe_ui_input_relayout_ror(AoeUiInputRor *in) {
    int16_t top, sel;
    if (!in) return;
    top = in->text.top;
    sel = in->text.selected;
    aoe_ui_text_set_text_ror(&in->text, in->current);
    if (sel >= in->text.count) sel = (int16_t)(in->text.count - 1);
    in->text.selected = sel;
    if (top >= in->text.count) in->text.top = (int16_t)(in->text.count - 1);
    aoe_ui_text_scroll_ror(&in->text, 1, top, 1);
    in->text.panel.vt->set_redraw(&in->text.panel, 1);
}

/* Keep the caret's line in view (bounded in case the window cannot move). */
static void follow(AoeUiInputRor *in) {
    int guard = in->text.count + 1;
    while (in->text.selected < in->text.top && guard-- > 0)
        aoe_ui_text_scroll_ror(&in->text, 3, 1, 1);
    guard = in->text.count + 1;
    while (in->text.bottom < in->text.selected && guard-- > 0)
        aoe_ui_text_scroll_ror(&in->text, 2, 1, 1);
}

static void skip_newline(AoeUiInputRor *in, char ch) {
    if (in->text.column_chars > 0 && in->cursor > 0 &&
        in->current[in->cursor - 1] == ch) {
        --in->cursor;
        --in->text.column_chars;
    }
}

void aoe_ui_input_caret_from_line_ror(AoeUiInputRor *in) {
    const AoeUiLineRor *line;
    int16_t i;
    if (!in || !in->current) return;
    in->cursor = 0;
    for (i = 0, line = in->text.lines; line && i < in->text.selected; ++i, line = line->next)
        in->cursor = (int16_t)(in->cursor + (int16_t)strlen(line->text));
    in->cursor = (int16_t)(in->cursor + in->text.column_chars);
    skip_newline(in, '\n');
    skip_newline(in, '\r');
    follow(in);
}

void aoe_ui_input_line_from_caret_ror(AoeUiInputRor *in) {
    const AoeUiLineRor *line;
    int16_t acc = 0;
    if (!in || !in->current) return;
    in->text.selected = 0;
    in->text.column_chars = 0;
    for (line = in->text.lines;
         line && in->text.selected != in->text.count - 1 &&
         (int16_t)strlen(line->text) + acc <= in->cursor;
         line = line->next) {
        ++in->text.selected;
        acc = (int16_t)(acc + (int16_t)strlen(line->text));
    }
    in->text.column_chars = (int16_t)(in->cursor - acc);
    skip_newline(in, '\n');
    skip_newline(in, '\r');
    follow(in);
}

static char *swap(AoeUiInputRor *in, char **src) {
    char *dst;
    if (in->current == in->buffers[0]) {
        *src = in->buffers[0];
        dst = in->buffers[1];
    } else {
        *src = in->buffers[1];
        dst = in->buffers[0];
    }
    in->current = dst;
    dst[0] = 0;
    return dst;
}

static void edited(AoeUiInputRor *in) {
    aoe_ui_input_relayout_ror(in);
    aoe_ui_input_line_from_caret_ror(in);
    caret_on(in);
}

int aoe_ui_input_insert_ror(AoeUiInputRor *in, char ch) {
    char *src, *dst;
    if (!in || !in->current) return 0;
    if (in->length >= in->max) {
        beep(in);
        return 0;
    }
    dst = swap(in, &src);
    if (in->cursor > 0) {
        strncpy(dst, src, (size_t)in->cursor);
        src += in->cursor;
        dst += in->cursor;
    }
    dst[0] = ch;
    strncpy(dst + 1, src, (size_t)(in->length - in->cursor + 1));
    ++in->length;
    ++in->cursor;
    edited(in);
    return 1;
}

int aoe_ui_input_delete_ror(AoeUiInputRor *in) {
    char *src, *dst, *s, *d, *next;
    int16_t n = 1;
    int back = 0;
    if (!in || !in->current || in->length <= in->cursor) return 0;
    dst = swap(in, &src);
    s = src;
    d = dst;
    if (in->cursor > 0) {
        strncpy(dst, src, (size_t)in->cursor);
        s = src + in->cursor;
        d = dst + in->cursor;
    }
    if (*s == '\r') {
        next = s + 1;
        if (*next == '\n') {
            next = s + 2;
            n = 2;
        }
    } else if (*s == '\n' && src < s && s[-1] == '\r') {
        --d;
        next = s + 1;
        n = 2;
        back = 1;
    } else {
        next = s + 1;
    }
    strncpy(d, next, (size_t)(in->length - in->cursor + 1));
    if (back) --in->cursor;
    in->length = (int16_t)(in->length - n);
    edited(in);
    return 1;
}

int aoe_ui_input_backspace_ror(AoeUiInputRor *in) {
    if (!in || in->cursor <= 0) return 0;
    --in->cursor;
    return aoe_ui_input_delete_ror(in);
}

int aoe_ui_input_paste_ror(AoeUiInputRor *in) {
    char clip[0x1000], *src, *dst, *s, *d;
    int32_t n;
    if (!in || !in->current || !in->ops || !in->ops->clipboard) return 0;
    memset(clip, 0, sizeof clip);
    n = in->ops->clipboard(in->ops->context, clip, (int32_t)sizeof clip);
    if (n < 0) return 0;
    if (n > 0xfff) n = 0xfff;
    if (in->max <= in->length + n) {
        beep(in);
        return 0;
    }
    dst = swap(in, &src);
    strncpy(dst, src, (size_t)in->cursor);
    s = src + in->cursor;
    d = dst + in->cursor;
    memcpy(d, clip, (size_t)n);
    d += n;
    strncpy(d, s, (size_t)(in->length - in->cursor + 1));
    in->length = (int16_t)(in->length + n);
    in->cursor = (int16_t)(in->cursor + n);
    edited(in);
    return 1;
}

static int digit(int32_t ch) { return ch >= '0' && ch <= '9'; }

static int name_char(int32_t ch) {
    return ch != '/' && ch != '"' && ch != '*' && ch != '?' && ch != '>' &&
           ch != '<' && ch != '|';
}

/* The format filter of 0x00463980. Formats 1-3 take '.' only in format
   1 and only once the text already has one (the original's _mbschr test
   is inverted), so a decimal point can never be typed first. */
static int accepts(const AoeUiInputRor *in, int32_t ch) {
    const char *sel;
    switch (in->format) {
    case 1: case 2: case 3:
        if (digit(ch)) return 1;
        if (ch == '-') return in->current[0] == 0 && in->format != 3;
        if (ch == '.') return strchr(in->current, '.') && in->format == 1;
        return 0;
    case 4: case 5:
        if (ch == '.') return in->format == 4 && !strchr(in->current, '.');
        return ch != '\\' && ch != ':' && name_char(ch);
    case 6:
        if (ch == ':') return in->length == 1;
        return name_char(ch);
    case 8:
        sel = aoe_ui_text_selected_text_ror(&in->text);
        if (strlen(sel) < 2 && digit(ch)) return 1;
        return strlen(sel) == 2 && ch == '0' && !strcmp("10", sel);
    default:
        return 1;
    }
}

static int32_t char_action(AoeUiPanelRor *p, int32_t ch, int32_t count) {
    AoeUiInputRor *in = I(p);
    int16_t n = (int16_t)count, i;
    if (ch == 8 || ch == 9 || ch == 10 || ch == 13 || ch == 27 || ch < 0x1b) return 1;
    if (n == 0) n = 1;
    for (i = 0; i < n; ++i) {
        if (!accepts(in, ch)) {
            beep(in);
            return 1;
        }
        if (!aoe_ui_input_insert_ror(in, (char)ch)) return 1;
    }
    return 1;
}

static int16_t line_len(const AoeUiInputRor *in, int16_t line) {
    return (int16_t)strlen(aoe_ui_text_line_ror(&in->text, line));
}

static void clamp_column(AoeUiInputRor *in) {
    int16_t len = line_len(in, in->text.selected);
    if (len < in->text.column_chars) in->text.column_chars = len;
}

static void moved(AoeUiInputRor *in) {
    aoe_ui_input_caret_from_line_ror(in);
    in->text.panel.vt->set_redraw(&in->text.panel, 1);
}

static int32_t key_action(AoeUiPanelRor *p, int32_t key, int32_t count,
                          int32_t alt, int32_t ctrl, int32_t shift) {
    AoeUiInputRor *in = I(p);
    AoeUiTextRor *t = &in->text;
    (void)count; (void)alt;
    switch (key) {
    case 8:
        aoe_ui_input_backspace_ror(in);
        return 1;
    case 0x0d:
        if (in->format != 7) {
            if (p->parent) p->parent->vt->action(p->parent, p, 0, 0, 0);
            return 1;
        }
        aoe_ui_input_insert_ror(in, '\n');
        return 1;
    case 0x1b:
        if (p->parent) p->parent->vt->action(p->parent, p, 1, 0, 0);
        return 1;
    case 0x21:
        if (t->count < 1 || t->selected < 1) return 1;
        if (t->selected < t->top || t->selected > t->bottom) t->selected = t->top;
        if (t->selected == t->top) aoe_ui_text_scroll_ror(t, 5, 0, 1);
        t->selected = t->top;
        clamp_column(in);
        moved(in);
        return 1;
    case 0x22:
        if (t->count < 1 || t->selected >= t->count - 1) return 1;
        if (t->selected < t->top || t->selected > t->bottom) t->selected = t->bottom;
        if (t->selected == t->bottom) aoe_ui_text_scroll_ror(t, 4, 0, 1);
        t->selected = t->bottom;
        clamp_column(in);
        moved(in);
        return 1;
    case 0x23:
        if (t->count < 1) return 1;
        if (!ctrl) {
            if (t->column_chars == line_len(in, t->selected)) return 1;
        } else {
            if (t->selected == t->count - 1 &&
                t->column_chars == line_len(in, t->selected))
                return 1;
            t->selected = (int16_t)(t->count - 1);
        }
        t->column_chars = line_len(in, t->selected);
        t->panel.vt->set_redraw(&t->panel, 1);
        aoe_ui_input_caret_from_line_ror(in);
        return 1;
    case 0x24:
        if (t->count < 1) return 1;
        if (!ctrl) {
            if (t->column_chars != 0) {
                t->column_chars = 0;
                t->panel.vt->set_redraw(&t->panel, 1);
                aoe_ui_input_caret_from_line_ror(in);
            }
        } else if (t->column_chars != 0 || t->selected != 0) {
            t->column_chars = 0;
            t->selected = 0;
            moved(in);
        }
        return 1;
    case 0x25:
        if (t->count < 1) return 1;
        if (t->column_chars < 1) {
            if (t->selected < 1) return 1;
            --t->selected;
            t->column_chars = line_len(in, t->selected);
        } else {
            --t->column_chars;
        }
        moved(in);
        return 1;
    case 0x26:
        if (t->count > 0 && t->selected > 0) {
            --t->selected;
            clamp_column(in);
            moved(in);
        }
        return 1;
    case 0x27:
        if (t->count < 1) return 1;
        if (line_len(in, t->selected) <= t->column_chars) {
            if (t->selected < t->count - 1) {
                ++t->selected;
                t->column_chars = 0;
                moved(in);
            }
            return 1;
        }
        ++t->column_chars;
        moved(in);
        return 1;
    case 0x28:
        if (t->count < 1 || t->selected >= t->count - 1) return 1;
        ++t->selected;
        clamp_column(in);
        moved(in);
        return 1;
    case 0x2d:
        if (!shift) {
            if (!ctrl) in->insert = !in->insert;
            return 1;
        }
        aoe_ui_input_paste_ror(in);
        return 1;
    case 0x2e:
        aoe_ui_input_delete_ror(in);
        return 1;
    case 0x56:
        if (ctrl) {
            aoe_ui_input_paste_ror(in);
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}

static int32_t idle(AoeUiPanelRor *p) {
    AoeUiInputRor *in = I(p);
    if (p->has_focus && now(in) - in->blink_at >= in->blink_ms) {
        in->caret_on = !in->caret_on;
        p->vt->set_redraw(p, 1);
        in->blink_at = now(in);
    }
    aoe_ui_panel_handle_idle_ror(p);
    return 0;
}

static void set_focus(AoeUiPanelRor *p, int32_t focus) {
    aoe_ui_panel_set_focus_ror(p, focus);
    if (p->has_focus) caret_on(I(p));
}

static int no_lines(AoeUiTextRor *t, const char *const *lines, int16_t count) {
    (void)t; (void)lines; (void)count;
    return 0;
}

const AoeUiTextVtblRor aoe_ui_input_vtable_ror = {
    {destroy,
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
     aoe_ui_panel_draw_ror,
     aoe_ui_panel_draw_rect_ror,
     aoe_ui_panel_draw_offset_ror,
     aoe_ui_panel_draw_parent_rect_ror,
     aoe_ui_panel_draw_parent_offset_ror,
     aoe_ui_panel_validate_ror,
     aoe_ui_panel_handle_message_ror,
     idle,
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
     {aoe_ui_panel_no_action_ror, aoe_ui_text_left_down_ror,
      aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
      aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
      aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
      aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
      aoe_ui_panel_no_action_ror},
     key_action,
     aoe_ui_panel_zero_ror,
     char_action,
     aoe_ui_text_action_ror,
     aoe_ui_panel_get_true_rect_ror,
     aoe_ui_panel_is_inside_ror,
     set_focus,
     aoe_ui_panel_set_tab_order_ror,
     aoe_ui_panel_get_help_ror,
     aoe_ui_panel_stop_sound_ror,
     aoe_ui_panel_restart_sound_ror,
     aoe_ui_panel_slot_e0_ror},
    no_lines,
    aoe_ui_text_set_text_id_ror,
    aoe_ui_input_set_text_ror,
    aoe_ui_text_bevel_slot_ror,
};
