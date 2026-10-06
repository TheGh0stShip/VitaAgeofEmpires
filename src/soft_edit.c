/* A software EDIT control for the translated edit panel. */
#include "soft_edit.h"

#include <string.h>

#include "ui_font.h"

#define SE(c) ((AoeSoftEditRor *)(c))
#define WIN(w) ((AoeSoftEditWindowRor *)(w))

static int is_window(const AoeSoftEditRor *e, const void *w) {
    const AoeSoftEditWindowRor *p = w;
    return p && p >= e->windows && p < e->windows + AOE_SOFT_EDIT_WINDOWS && p->used;
}

static void *se_create(void *c, AoeUiAreaRor *area, int32_t x, int32_t y, int32_t w, int32_t h,
                       int multiline) {
    AoeSoftEditRor *e = SE(c);
    int i;
    for (i = 0; i < AOE_SOFT_EDIT_WINDOWS; ++i) {
        AoeSoftEditWindowRor *win = &e->windows[i];
        if (win->used) continue;
        memset(win, 0, sizeof *win);
        win->used = 1;
        win->area = area;
        win->x = x; win->y = y; win->w = w; win->h = h;
        win->multiline = multiline;
        return win;
    }
    return NULL;
}

static void se_destroy(void *c, void *w) {
    AoeSoftEditRor *e = SE(c);
    if (!is_window(e, w)) return;
    if (e->focus == w) e->focus = e->main_window;
    memset(w, 0, sizeof(AoeSoftEditWindowRor));
}

static void se_show(void *c, void *w, int show) {
    if (is_window(SE(c), w)) WIN(w)->shown = show != 0;
}

static void se_move(void *c, void *w, int32_t x, int32_t y, int32_t wd, int32_t h, int repaint) {
    (void)repaint;
    if (!is_window(SE(c), w)) return;
    WIN(w)->x = x; WIN(w)->y = y; WIN(w)->w = wd; WIN(w)->h = h;
}

static void se_invalidate(void *c, void *w, int update) { (void)c; (void)w; (void)update; }
static void *se_focus(void *c) { return SE(c)->focus; }

/* The panel owning a window: the edit panels of the panel tree. */
static AoeUiEditRor *find_owner(AoeUiPanelRor *p, const void *w) {
    AoeUiNodeRor *n;
    if (!p) return NULL;
    if (p->vt == &aoe_ui_edit_vtable_ror && ((AoeUiEditRor *)p)->window == w)
        return (AoeUiEditRor *)p;
    for (n = p->first; n; n = n->next) {
        AoeUiEditRor *r = find_owner(n->panel, w);
        if (r) return r;
    }
    return NULL;
}

static void se_set_focus(void *c, void *w) {
    AoeSoftEditRor *e = SE(c);
    e->focus = w;
    if (is_window(e, w) && !WIN(w)->owner && e->system) {
        AoeUiEditRor *o = find_owner(e->system->modal, w);
        if (!o) o = find_owner(e->system->screen, w);
        WIN(w)->owner = o;
    }
}

static void *se_main_window(void *c, AoeUiAreaRor *area) { (void)area; return SE(c)->main_window; }

static void se_set_text(void *c, void *w, const char *text) {
    if (!is_window(SE(c), w)) return;
    strncpy(WIN(w)->text, text ? text : "", AOE_SOFT_EDIT_TEXT - 1);
    WIN(w)->text[AOE_SOFT_EDIT_TEXT - 1] = '\0';
    WIN(w)->sel_start = WIN(w)->sel_end = (int32_t)strlen(WIN(w)->text);
}

static void se_get_text(void *c, void *w, char *out, int32_t size) {
    if (size <= 0) return;
    if (!is_window(SE(c), w)) {
        out[0] = '\0';
        return;
    }
    strncpy(out, WIN(w)->text, (size_t)size - 1);
    out[size - 1] = '\0';
}

static void se_get_selection(void *c, void *w, int32_t *start, int32_t *end) {
    int32_t a, b;
    if (!is_window(SE(c), w)) {
        *start = *end = 0;
        return;
    }
    a = WIN(w)->sel_start;
    b = WIN(w)->sel_end;
    *start = a < b ? a : b;
    *end = a < b ? b : a;
}

static void se_set_selection(void *c, void *w, int32_t start, int32_t end) {
    int32_t n;
    if (!is_window(SE(c), w)) return;
    n = (int32_t)strlen(WIN(w)->text);
    if (start < 0 || start > n) start = n;
    if (end < 0 || end > n) end = n;
    WIN(w)->sel_start = start;
    WIN(w)->sel_end = end;
}

static void se_undo(void *c, void *w) {
    char t[AOE_SOFT_EDIT_TEXT];
    if (!is_window(SE(c), w)) return;
    memcpy(t, WIN(w)->text, sizeof t);
    memcpy(WIN(w)->text, WIN(w)->undo_text, sizeof t);
    memcpy(WIN(w)->undo_text, t, sizeof t);
    WIN(w)->sel_start = WIN(w)->sel_end = (int32_t)strlen(WIN(w)->text);
}

static void se_set_font(void *c, void *w, uintptr_t font) {
    if (is_window(SE(c), w)) WIN(w)->font = font;
}

static void replace_selection(AoeSoftEditWindowRor *w, const char *with, size_t n) {
    int32_t a = w->sel_start < w->sel_end ? w->sel_start : w->sel_end;
    int32_t b = w->sel_start < w->sel_end ? w->sel_end : w->sel_start;
    size_t len = strlen(w->text);
    if (len - (size_t)(b - a) + n >= AOE_SOFT_EDIT_TEXT) return;
    memcpy(w->undo_text, w->text, sizeof w->text);
    memmove(w->text + a + n, w->text + b, len - (size_t)b + 1);
    memcpy(w->text + a, with, n);
    w->sel_start = w->sel_end = a + (int32_t)n;
}

/* The EDIT control's own handling (DefWindowProc of the EDIT class). */
static int32_t se_default_proc(void *c, void *w, uint32_t msg, uint32_t wp, uint32_t lp) {
    AoeSoftEditRor *e = SE(c);
    AoeSoftEditWindowRor *win = w;
    int32_t n;
    int shift;
    (void)lp;
    if (!is_window(e, w)) return 0;
    n = (int32_t)strlen(win->text);
    shift = e->key_down && e->key_down(e->key_context, 0x10);
    if (msg == 0x102) {
        if (wp == 8) {
            if (win->sel_start == win->sel_end && win->sel_end > 0) win->sel_start = win->sel_end - 1;
            replace_selection(win, "", 0);
        } else if (wp >= 0x20 && wp < 0x100) {
            char ch = (char)wp;
            replace_selection(win, &ch, 1);
        }
        return 0;
    }
    if (msg == 0x100) {
        int32_t caret = win->sel_end;
        switch (wp) {
        case 0x25: if (caret > 0) --caret; break;      /* Left */
        case 0x27: if (caret < n) ++caret; break;      /* Right */
        case 0x24: caret = 0; break;                   /* Home */
        case 0x23: caret = n; break;                   /* End */
        case 0x2e:                                     /* Delete */
            if (win->sel_start == win->sel_end && win->sel_end < n) win->sel_end++;
            replace_selection(win, "", 0);
            return 0;
        default: return 0;
        }
        win->sel_end = caret;
        if (!shift) win->sel_start = caret;
        return 0;
    }
    if (msg == 0x201) {   /* a click: the caret to the end */
        win->sel_start = win->sel_end = n;
        return 0;
    }
    return 0;
}

static void se_post_main(void *c, void *main, uint32_t msg, uint32_t wp, uint32_t lp) {
    AoeSoftEditRor *e = SE(c);
    (void)main;
    if (e->system && e->system->screen)
        e->system->screen->vt->handle_message(e->system->screen, e->main_window, msg, wp, lp);
}

static void se_fill_background(void *c, void *w, uintptr_t brush) { (void)c; (void)w; (void)brush; }
static uintptr_t se_ime_context(void *c, void *w, int enabled) { (void)c; (void)w; (void)enabled; return 0; }
static int se_ime_set_open(void *c, uintptr_t ime, int open) { (void)c; (void)ime; (void)open; return 0; }
static void se_ime_release(void *c, void *w, uintptr_t ime, int enabled) { (void)c; (void)w; (void)ime; (void)enabled; }
static int se_key_down(void *c, int vk) { return SE(c)->key_down ? SE(c)->key_down(SE(c)->key_context, vk) : 0; }
static int se_lead_byte(void *c, uint8_t ch) { (void)c; (void)ch; return 0; }
static void se_beep(void *c) { (void)c; }
static int se_software_cursor(void *c) { (void)c; return 1; }
static void se_hide_pointer(void *c) { (void)c; }
static uint8_t se_nearest(void *c, AoeUiAreaRor *area, uint32_t color) {
    (void)area;
    return SE(c)->palette ? aoe_ui_nearest_index_ror(SE(c)->palette, color) : 0;
}

void aoe_soft_edit_init_ror(AoeSoftEditRor *e, AoeUiEditOpsRor *ops, AoeUiSystemRor *system,
                            void *main_window, int (*key_down)(void *, int), void *key_context,
                            const uint8_t *palette) {
    memset(e, 0, sizeof *e);
    e->system = system;
    e->main_window = main_window;
    e->focus = main_window;
    e->key_down = key_down;
    e->key_context = key_context;
    e->palette = palette;
    memset(ops, 0, sizeof *ops);
    ops->context = e;
    ops->create = se_create;
    ops->destroy = se_destroy;
    ops->show = se_show;
    ops->move = se_move;
    ops->invalidate = se_invalidate;
    ops->focus = se_focus;
    ops->set_focus = se_set_focus;
    ops->main_window = se_main_window;
    ops->set_text = se_set_text;
    ops->get_text = se_get_text;
    ops->get_selection = se_get_selection;
    ops->set_selection = se_set_selection;
    ops->undo = se_undo;
    ops->set_font = se_set_font;
    ops->default_proc = se_default_proc;
    ops->post_main = se_post_main;
    ops->fill_background = se_fill_background;
    ops->ime_context = se_ime_context;
    ops->ime_set_open = se_ime_set_open;
    ops->ime_release = se_ime_release;
    ops->key_down = se_key_down;
    ops->lead_byte = se_lead_byte;
    ops->beep = se_beep;
    ops->scrollbar_width = 16;
    ops->software_cursor = se_software_cursor;
    ops->hide_pointer = se_hide_pointer;
    ops->nearest_index = se_nearest;
}

AoeSoftEditWindowRor *aoe_soft_edit_focus_window_ror(AoeSoftEditRor *e) {
    return is_window(e, e->focus) ? WIN(e->focus) : NULL;
}

AoeUiEditRor *aoe_soft_edit_focused_ror(AoeSoftEditRor *e) {
    AoeSoftEditWindowRor *w = aoe_soft_edit_focus_window_ror(e);
    return w ? w->owner : NULL;
}
