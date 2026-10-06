#include "ui_panel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- panel system ---- */

static const AoeUiPlatformRor *platform(const AoeUiPanelRor *p) {
    return p && p->system ? p->system->platform : NULL;
}

uintptr_t aoe_ui_palette_ror(AoeUiSystemRor *s, const char *name, int32_t id) {
    char key[0x104];
    int32_t free_slot = -1;
    if (!s) return 0;
    key[0] = '\0';
    if (name) {
        if (!strchr(name, '.')) snprintf(key, sizeof(key), "%s.pal", name);
        else snprintf(key, sizeof(key), "%s", name);
    }
    for (int32_t i = 0; i < AOE_UI_PALETTES; ++i) {
        if (!s->palettes[i]) {
            if (free_slot == -1) free_slot = i;
            continue;
        }
        if ((id != -1 && s->palette_ids[i] == id) || !strcmp(s->palette_names[i], key)) {
            ++s->palette_refs[i];
            return s->palettes[i];
        }
    }
    if (free_slot == -1 || !s->platform || !s->platform->create_palette) return 0;
    s->palettes[free_slot] = s->platform->create_palette(s->platform->context, key, id);
    if (!s->palettes[free_slot]) return 0;
    ++s->palette_refs[free_slot];
    memcpy(s->palette_names[free_slot], key, sizeof(key));
    s->palette_ids[free_slot] = id;
    return s->palettes[free_slot];
}

void aoe_ui_palette_release_ror(AoeUiSystemRor *s, uintptr_t palette) {
    for (int32_t i = 0; s && i < AOE_UI_PALETTES; ++i) {
        if (s->palettes[i] != palette) continue;
        if (--s->palette_refs[i] == 0) {
            if (s->platform && s->platform->delete_palette)
                s->platform->delete_palette(s->platform->context, palette);
            s->palettes[i] = 0;
            s->palette_names[i][0] = '\0';
        }
        return;
    }
}

int32_t aoe_ui_enabled_ror(const AoeUiSystemRor *s) { return s ? s->enabled : 0; }
void aoe_ui_enable_ror(AoeUiSystemRor *s) { if (s) s->enabled = 1; }
void aoe_ui_disable_ror(AoeUiSystemRor *s) { if (s) s->enabled = 0; }

void aoe_ui_stop_sounds_ror(AoeUiSystemRor *s) {
    for (int32_t i = 0; s && i < s->panel_count; ++i)
        if (s->panels[i]) s->panels[i]->vt->stop_sound(s->panels[i]);
}

int32_t aoe_ui_restart_sounds_ror(AoeUiSystemRor *s) {
    int32_t ok = 1;
    for (int32_t i = 0; s && i < s->panel_count; ++i)
        if (s->panels[i] && !s->panels[i]->vt->restart_sound(s->panels[i]))
            ok = 0;
    return ok;
}

void aoe_ui_mark_restore_ror(AoeUiSystemRor *s) {
    for (int32_t i = 0; s && i < s->panel_count; ++i)
        if (s->panels[i]) s->panels[i]->need_restore = 1;
}

AoeUiPanelRor *aoe_ui_capture_panel_ror(const AoeUiSystemRor *s) {
    return s ? s->capture : NULL;
}

AoeUiPanelRor *aoe_ui_modal_panel_ror(const AoeUiSystemRor *s) {
    return s ? s->modal : NULL;
}

static void register_panel(AoeUiSystemRor *s, AoeUiPanelRor *p) {
    AoeUiPanelRor **grown;
    if (!s) return;
    if (s->panel_count == s->panel_capacity) {
        int32_t cap = s->panel_capacity ? s->panel_capacity * 2 : 8;
        if (!(grown = realloc(s->panels, (size_t)cap * sizeof(*grown)))) return;
        s->panels = grown;
        s->panel_capacity = cap;
    }
    s->panels[s->panel_count++] = p;
}

static void unregister_panel(AoeUiSystemRor *s, AoeUiPanelRor *p) {
    for (int32_t i = 0; s && i < s->panel_count; ++i)
        if (s->panels[i] == p) {
            memmove(&s->panels[i], &s->panels[i + 1],
                    (size_t)(s->panel_count - i - 1) * sizeof(*s->panels));
            --s->panel_count;
            return;
        }
}

/* ---- named panels, screens and modal panels ---- */

AoeUiPanelRor *aoe_ui_system_find_ror(const AoeUiSystemRor *s,
                                      const char *name) {
    if (!s || !name) return NULL;
    for (int32_t i = 0; i < s->panel_count; ++i)
        if (s->panels[i]->name && strcmp(s->panels[i]->name, name) == 0)
            return s->panels[i];
    return NULL;
}

int aoe_ui_system_has_ror(const AoeUiSystemRor *s, const AoeUiPanelRor *p) {
    return p && aoe_ui_system_find_ror(s, p->name) != NULL;
}

void aoe_ui_system_set_keyboard_ror(AoeUiSystemRor *s, AoeUiPanelRor *p) {
    if (s) s->keyboard = p;
}

void aoe_ui_panel_set_previous_ror(AoeUiPanelRor *p, AoeUiPanelRor *previous) {
    if (p) p->previous = previous;
}

static void update_window(AoeUiSystemRor *s) {
    if (s->platform && s->platform->update_window)
        s->platform->update_window(s->platform->context);
}

void aoe_ui_system_restore_modal_ror(AoeUiSystemRor *s) {
    if (!s) return;
    if (s->modal && s->modal != s->screen && s->screen)
        s->screen->chained_modal = s->modal;
    s->modal = s->screen;
}

void aoe_ui_system_show_ror(AoeUiSystemRor *s, AoeUiPanelRor *p, int restore) {
    if (!s || !p) return;
    p->previous = s->screen;
    if (s->screen) s->screen->vt->set_focus(s->screen, 0);
    s->screen = p;
    p->vt->set_focus(p, 1);
    s->screen->vt->set_redraw(s->screen, 2);
    update_window(s);
    if (restore) aoe_ui_system_restore_modal_ror(s);
}

int aoe_ui_system_show_named_ror(AoeUiSystemRor *s, const char *name,
                                 int restore) {
    AoeUiPanelRor *p = aoe_ui_system_find_ror(s, name);
    if (!p) return 0;
    if (s->capture) aoe_ui_panel_release_capture_ror(s->capture);
    s->modal = NULL;
    s->keyboard = NULL;
    aoe_ui_system_show_ror(s, p, restore);
    return 1;
}

int aoe_ui_system_add_ror(AoeUiSystemRor *s, AoeUiPanelRor *p, int show,
                          int restore) {
    if (!s || !p) return 0;
    if (!aoe_ui_system_find_ror(s, p->name)) {
        p->previous = NULL;
        register_panel(s, p);
    }
    if (show) aoe_ui_system_show_ror(s, p, restore);
    return 1;
}

int aoe_ui_system_remove_ror(AoeUiSystemRor *s, const char *name) {
    AoeUiPanelRor *p = aoe_ui_system_find_ror(s, name);
    if (!p) return 0;
    unregister_panel(s, p);
    return 1;
}

int aoe_ui_system_close_ror(AoeUiSystemRor *s, const char *name) {
    AoeUiPanelRor *p = aoe_ui_system_find_ror(s, name);
    if (!p) return 0;
    unregister_panel(s, p);
    if (s->screen == p) s->screen = NULL;
    if (s->capture == p) s->capture = NULL;
    if (s->keyboard == p) s->keyboard = NULL;
    if (s->modal == p) s->modal = NULL;
    p->vt->destroy(p);
    free(p);
    return 1;
}

int aoe_ui_system_back_ror(AoeUiSystemRor *s, int close) {
    AoeUiPanelRor *cur, *prev;
    if (!s || !s->screen) return 0;
    cur = s->screen;
    prev = cur->previous;
    if (!prev) return 0;
    if (!close) {
        prev->previous = cur;
    } else {
        if (prev->previous == cur) prev->previous = NULL;
        aoe_ui_system_close_ror(s, cur->name);
    }
    s->screen = prev;
    prev->vt->set_focus(prev, 1);
    s->screen->vt->set_redraw(s->screen, 2);
    update_window(s);
    return 1;
}

void aoe_ui_system_set_modal_ror(AoeUiSystemRor *s, AoeUiPanelRor *p) {
    AoeUiPanelRor *m;
    if (!s) return;
    if (s->modal && p) p->chained_modal = s->modal;
    m = s->modal;
    if (!m) {
        if (s->saved_focus && s->saved_focus->parent)
            aoe_ui_panel_set_current_ror(s->saved_focus->parent, s->saved_focus);
    } else if (!m->tab_stop && m->parent) {
        aoe_ui_panel_set_current_ror(m->parent, s->saved_focus);
    }
    s->saved_focus = NULL;
    s->modal = p;
    if (p && p != s->screen) {
        if (p->parent) {
            s->saved_focus = p->parent->current;
            aoe_ui_panel_set_current_ror(p->parent, p);
        } else {
            s->saved_focus = NULL;
            p->vt->set_focus(p, 1);
        }
    }
}

int aoe_ui_system_pop_modal_ror(AoeUiSystemRor *s) {
    AoeUiPanelRor *prev;
    if (!s || !s->modal) return 0;
    prev = s->modal->chained_modal;
    if (!prev) return 0;
    prev->vt->set_focus(prev, 1);
    s->capture = prev;
    s->keyboard = prev;
    s->modal = prev;
    return 1;
}

/* ---- base panel ---- */

void aoe_ui_panel_init_ror(AoeUiPanelRor *p, AoeUiSystemRor *system) {
    if (!p) return;
    memset(p, 0, sizeof(*p));
    p->vt = &aoe_ui_panel_vtable_ror;
    p->system = system;
    p->redraw = 2;
    p->active = 1;
    p->overlap_redraw = 1;
    p->handles_mouse = 1;
    p->hold_ms = 250;
    p->help_id = p->help_page = -1;
}

int aoe_ui_panel_init_named_ror(AoeUiPanelRor *p, AoeUiSystemRor *system,
                                const char *name) {
    aoe_ui_panel_init_ror(p, system);
    if (!p || !aoe_ui_panel_set_name_ror(p, name)) return 0;
    p->need_restore = 0;
    if (p->name && p->name[0]) register_panel(system, p);
    return 1;
}

int aoe_ui_panel_set_name_ror(AoeUiPanelRor *p, const char *name) {
    size_t n;
    if (!p) return 0;
    if (!name || !name[0]) {
        free(p->name);
        p->name = NULL;
        return 1;
    }
    n = strlen(name);
    if (!p->name || strlen(p->name) < n) {
        char *grown = malloc(n + 1);
        if (!grown) return 0;
        free(p->name);
        p->name = grown;
    }
    memcpy(p->name, name, n + 1);
    return 1;
}

void aoe_ui_insert_child_ror(AoeUiPanelRor *parent, AoeUiNodeRor *node,
                             AoeUiNodeRor *ref, int after) {
    if (!parent || !node) return;
    if (!ref) {
        AoeUiNodeRor *last = parent->first;
        if (!last) {
            node->prev = node->next = NULL;
        } else {
            while (last->next) last = last->next;
            last->next = node;
            node->prev = last;
            node->next = NULL;
        }
    } else if (!after) {
        node->next = ref;
        node->prev = ref->prev;
        ref->prev = node;
        if (node->prev) node->prev->next = node;
    } else {
        node->prev = ref;
        node->next = ref->next;
        ref->next = node;
        if (node->next) node->next->prev = node;
    }
    if (!node->prev) parent->first = node;
    if (!node->next) parent->last = node;
}

void aoe_ui_remove_child_ror(AoeUiPanelRor *parent, AoeUiNodeRor *node) {
    if (!parent || !node) return;
    if (node == parent->first) parent->first = node->next;
    if (node == parent->last) parent->last = node->prev;
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    node->prev = node->next = NULL;
    if (parent->current == node->panel) parent->current = NULL;
}

void aoe_ui_panel_release_ror(AoeUiPanelRor *p) {
    if (!p) return;
    aoe_ui_panel_release_capture_ror(p);
    if (p->name) unregister_panel(p->system, p);
    if (p->node) {
        if (p->parent) aoe_ui_remove_child_ror(p->parent, p->node);
        free(p->node);
        p->node = NULL;
    }
    p->first = NULL;
    p->region = 0;
    if (p->parent) p->parent->vt->set_redraw(p->parent, 2);
    free(p->name);
    p->name = NULL;
}

void aoe_ui_panel_destroy_ror(AoeUiPanelRor *p) { aoe_ui_panel_release_ror(p); }

int aoe_ui_panel_setup_ror(AoeUiPanelRor *p, AoeUiAreaRor *area,
                           AoeUiPanelRor *parent, int32_t x, int32_t y,
                           int32_t w, int32_t h, uint8_t color) {
    if (!p) return 0;
    p->parent = parent;
    p->area = area;
    if (p->position_mode == 0) {
        p->offset[1] = y;
        p->min_w = p->max_w = w;
        p->offset[0] = x;
        p->offset[2] = p->offset[3] = 0;
        p->min_h = p->max_h = h;
    }
    p->vt->handle_size(p, area ? area->width : 0, area ? area->height : 0);
    p->vt->set_color(p, color);
    p->captured = 0;
    if (!p->node) {
        if (!(p->node = calloc(1, sizeof(*p->node)))) return 0;
        p->node->panel = p;
        if (p->parent) aoe_ui_insert_child_ror(p->parent, p->node, NULL, 0);
    }
    return 1;
}

void aoe_ui_panel_set_rect_ltrb_ror(AoeUiPanelRor *p, int32_t l, int32_t t,
                                    int32_t r, int32_t b) {
    if (p) p->vt->set_rect(p, l, t, r - l + 1, b - t + 1);
}

void aoe_ui_panel_set_rect_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                               int32_t w, int32_t h) {
    int32_t r = x - 1 + w, b = y - 1 + h;
    AoeUiPanelRor *parent;
    if (!p) return;
    p->width = w;
    p->height = h;
    p->x = x;
    p->y = y;
    p->rect = (AoeUiRectRor){x, y, r, b};
    p->clip = p->rect;
    parent = p->parent;
    if (parent && p->clip_to_parent) {
        if (x < parent->clip.left) p->clip.left = parent->clip.left;
        if (y < parent->clip.top) p->clip.top = parent->clip.top;
        if (parent->clip.right < r) p->clip.right = parent->clip.right;
        if (parent->clip.bottom < b) p->clip.bottom = parent->clip.bottom;
    }
    if (p->clip.left < 0) p->clip.left = 0;
    if (p->clip.top < 0) p->clip.top = 0;
    if (p->area) {
        if (p->area->width <= p->clip.right) p->clip.right = p->area->width - 1;
        if (p->area->height <= p->clip.bottom)
            p->clip.bottom = p->area->height - 1;
    }
    p->region = 0;
    if (p->clip.left <= p->clip.right && p->clip.top <= p->clip.bottom) {
        p->visible = 1;
        p->region = 1;
    } else {
        p->visible = 0;
    }
    if (p->active) p->vt->set_redraw(p, 2);
}

void aoe_ui_panel_set_color_ror(AoeUiPanelRor *p, uint8_t color) {
    if (!p) return;
    p->color = color;
    if (p->active) p->vt->set_redraw(p, 2);
}

void aoe_ui_panel_set_active_ror(AoeUiPanelRor *p, int32_t active) {
    if (!p || p->active == active) return;
    p->active = active;
    if (active) {
        p->vt->set_redraw(p, 1);
        return;
    }
    aoe_ui_panel_release_capture_ror(p);
    if (p->parent) p->parent->vt->set_redraw(p->parent, 1);
}

void aoe_ui_panel_set_position_mode_ror(
    AoeUiPanelRor *p, int32_t mode, int32_t left, int32_t top, int32_t right,
    int32_t bottom, int32_t min_w, int32_t max_w, int32_t min_h, int32_t max_h,
    AoeUiPanelRor *anchor_l, AoeUiPanelRor *anchor_t, AoeUiPanelRor *anchor_r,
    AoeUiPanelRor *anchor_b) {
    if (!p) return;
    p->position_mode = mode;
    p->offset[0] = left;
    p->offset[1] = top;
    p->offset[2] = right;
    p->offset[3] = bottom;
    p->min_w = min_w;
    p->max_w = max_w;
    p->min_h = min_h;
    p->max_h = max_h;
    p->anchor[0] = anchor_l;
    p->anchor[1] = anchor_t;
    p->anchor[2] = anchor_r;
    p->anchor[3] = anchor_b;
    p->vt->handle_size(p, p->area ? p->area->width : 0,
                       p->area ? p->area->height : 0);
    if (p->active) p->vt->set_redraw(p, 2);
}

void aoe_ui_panel_set_fixed_position_ror(AoeUiPanelRor *p, int32_t x,
                                         int32_t y, int32_t w, int32_t h) {
    if (p)
        p->vt->set_position_mode(p, 0, x, y, 0, 0, w, w, h, h, NULL, NULL,
                                 NULL, NULL);
}

void aoe_ui_panel_set_redraw_ror(AoeUiPanelRor *p, int32_t level) {
    const AoeUiPlatformRor *pf;
    if (!p || (level == 1 && p->redraw == 2)) return;
    p->redraw = level;
    if (!level || !p->area || !p->visible || !p->active) return;
    if ((pf = platform(p)) && pf->invalidate)
        pf->invalidate(pf->context, p->area, &p->clip);
}

void aoe_ui_panel_redraw_overlaps_ror(AoeUiPanelRor *p, AoeUiPanelRor *of,
                                      AoeUiPanelRor *skip, int32_t level) {
    int32_t l, t, r, b;
    if (!p || !of) return;
    l = of->clip.left;
    t = of->clip.top;
    r = of->clip.right;
    b = of->clip.bottom;
    for (AoeUiNodeRor *n = p->first; n; n = n->next) {
        AoeUiPanelRor *c = n->panel;
        int32_t cl, ct, cr, cb, apart;
        if (c == of || c == skip || of->z >= c->z) continue;
        cl = c->clip.left;
        ct = c->clip.top;
        cr = c->clip.right;
        cb = c->clip.bottom;
        apart = ((cl < l || r < cl) && (cr < l || r < cr) && (l <= cl || cr <= r)) ||
                ((ct < t || b < ct) && (cb < t || b < cb) && (t <= ct || cb <= b));
        if (!apart ||
            (((cl <= l && l <= cr) || (cl <= r && r <= cr) || (l < cl && cr < r)) &&
             ((ct <= t && t <= cb) || (ct <= b && b <= cb) || (t < ct && cb < b))))
            c->vt->set_redraw(c, level);
    }
}

void aoe_ui_panel_begin_draw_ror(AoeUiPanelRor *p, int32_t clear) {
    const AoeUiPlatformRor *pf = platform(p);
    if (!p) return;
    if (pf && pf->clip) pf->clip(pf->context, p->area, &p->clip);
    if (clear && pf && pf->fill) pf->fill(pf->context, &p->clip, p->color);
}

void aoe_ui_panel_end_draw_ror(AoeUiPanelRor *p) {
    const AoeUiPlatformRor *pf = platform(p);
    if (!p) return;
    p->redraw = 0;
    if (pf && pf->clip) pf->clip(pf->context, p->area, NULL);
    p->need_restore = 0;
}

void aoe_ui_panel_draw_ror(AoeUiPanelRor *p) {
    const AoeUiPlatformRor *pf;
    if (!p) return;
    p->redraw = 0;
    if (!p->area || !p->visible || !p->active) return;
    if (!p->own_fill && p->parent) {
        if (p->parent_rect) p->parent->vt->draw_parent_rect(p->parent, &p->clip);
        else p->parent->vt->draw_rect(p->parent, &p->clip);
        return;
    }
    if ((pf = platform(p)) && pf->fill) pf->fill(pf->context, &p->clip, p->color);
}

void aoe_ui_panel_draw_rect_ror(AoeUiPanelRor *p, const AoeUiRectRor *r) {
    AoeUiRectRor saved;
    if (!p || !r) return;
    saved = p->clip;
    p->clip = *r;
    p->vt->draw(p);
    p->clip = saved;
}

void aoe_ui_panel_draw_offset_ror(AoeUiPanelRor *p, int32_t dx, int32_t dy,
                                  const AoeUiRectRor *r) {
    AoeUiRectRor saved;
    if (!p || !r) return;
    p->x += dx;
    p->y += dy;
    saved = p->clip;
    p->clip = *r;
    p->vt->draw(p);
    p->clip = saved;
    p->x -= dx;
    p->y -= dy;
}

void aoe_ui_panel_draw_parent_rect_ror(AoeUiPanelRor *p,
                                       const AoeUiRectRor *r) {
    if (!p) return;
    p->parent_rect = 1;
    p->vt->draw_rect(p, r);
    p->parent_rect = 0;
}

void aoe_ui_panel_draw_parent_offset_ror(AoeUiPanelRor *p, int32_t dx,
                                         int32_t dy, const AoeUiRectRor *r) {
    if (!p) return;
    p->parent_rect = 1;
    p->vt->draw_offset(p, dx, dy, r);
    p->parent_rect = 0;
}

void aoe_ui_panel_validate_ror(AoeUiPanelRor *p) {
    const AoeUiPlatformRor *pf = platform(p);
    if (p && p->area && p->visible && p->active && pf && pf->validate)
        pf->validate(pf->context, p->area, &p->clip);
}

void aoe_ui_decode_mouse_ror(uint32_t wparam, uint32_t lparam, int32_t xy[2],
                             int32_t *ctrl, int32_t *shift) {
    uint32_t x = lparam & 0xffff, y = lparam >> 16;
    if (!xy || !ctrl || !shift) return;
    xy[0] = x > 60000 ? (int32_t)x - 0x10000 : (int32_t)x;
    xy[1] = y > 60000 ? (int32_t)y - 0x10000 : (int32_t)y;
    *ctrl = (int32_t)(wparam & 8);
    *shift = (int32_t)(wparam & 4);
}

static int key_held(const AoeUiPanelRor *p, int vk) {
    const AoeUiPlatformRor *pf = platform(p);
    return pf && pf->key_down && pf->key_down(pf->context, vk);
}

int32_t aoe_ui_panel_handle_message_ror(AoeUiPanelRor *p, void *window,
                                        uint32_t msg, uint32_t wparam,
                                        uint32_t lparam) {
    AoeUiSystemRor *s;
    AoeUiPanelRor *target;
    if (!p || !p->active) return 0;
    s = p->system;
    if (msg == 0x100 || msg == 0x102 || msg == 0x104) {
        if (!p->area || window != p->area->window || !aoe_ui_enabled_ror(s))
            return 0;
        target = s->keyboard ? s->keyboard : s->modal ? s->modal : p;
        if (msg == 0x102) return target->vt->char_input(target, (int32_t)wparam, (int32_t)lparam);
        return target->vt->key_down(target, (int32_t)wparam, (int32_t)lparam,
                                    key_held(p, 0x12), key_held(p, 0x11),
                                    key_held(p, 0x10));
    }
    if (msg >= 0x200 && msg <= 0x206) {
        int32_t xy[2], ctrl, shift;
        if (!p->area || window != p->area->window || !aoe_ui_enabled_ror(s))
            return 0;
        aoe_ui_decode_mouse_ror(wparam, lparam, xy, &ctrl, &shift);
        target = s->capture ? s->capture : s->keyboard ? s->keyboard
                                       : s->modal ? s->modal : p;
        switch (msg) {
        case 0x200: return target->vt->mouse_move(target, xy[0], xy[1], ctrl, shift);
        case 0x201: return target->vt->mouse_down(target, 1, xy[0], xy[1], ctrl, shift);
        case 0x202: return target->vt->mouse_up(target, 1, xy[0], xy[1], ctrl, shift);
        case 0x203: return target->vt->mouse_dbl(target, 1, xy[0], xy[1], ctrl, shift);
        case 0x204: return target->vt->mouse_down(target, 2, xy[0], xy[1], ctrl, shift);
        case 0x205: return target->vt->mouse_up(target, 2, xy[0], xy[1], ctrl, shift);
        default: return target->vt->mouse_dbl(target, 2, xy[0], xy[1], ctrl, shift);
        }
    }
    for (AoeUiNodeRor *n = p->last; n; n = n->prev) {
        int32_t r = n->panel->vt->handle_message(n->panel, window, msg,
                                                 wparam, lparam);
        if (r) return r;
    }
    if (msg == 0x114 || msg == 0x115) return p->vt->scroll(p, wparam & 0xffff, wparam >> 16);
    if (msg == 0x111) return p->vt->command(p, wparam, lparam);
    if (msg == 0x113) return p->vt->timer(p, wparam, lparam);
    if (msg == 0x400) return p->vt->user(p, wparam, lparam);
    return 0;
}

int32_t aoe_ui_panel_handle_idle_ror(AoeUiPanelRor *p) {
    const AoeUiPlatformRor *pf = platform(p);
    if (!p) return 0;
    if (p->active && p->mouse_state == 1 && pf && pf->now_ms &&
        p->hold_ms <= (int32_t)(pf->now_ms(pf->context) - p->down_ms)) {
        p->mouse_state = 2;
        p->vt->actions[p->mouse_button == 1 ? AOE_UI_LEFT_HOLD : AOE_UI_RIGHT_HOLD](
            p, p->down_x, p->down_y, p->down_ctrl, p->down_shift);
    }
    for (AoeUiNodeRor *n = p->last; n; n = n->prev) {
        int32_t r = n->panel->vt->handle_idle(n->panel);
        if (r) return r;
    }
    return 0;
}

static int anchored(const AoeUiPanelRor *a) { return a && a->active; }

int32_t aoe_ui_panel_handle_size_ror(AoeUiPanelRor *p, int32_t w, int32_t h) {
    int32_t l, t, r, b;
    if (!p) return 0;
    if (!p->parent) {
        l = t = 0;
        r = w - 1;
        b = h - 1;
    } else {
        l = p->parent->rect.left;
        t = p->parent->rect.top;
        r = p->parent->rect.right;
        b = p->parent->rect.bottom;
        w = r - l + 1;
        h = b - t + 1;
    }
    if (p->position_mode == 0) {
        l += p->offset[0];
        t += p->offset[1];
        r = p->min_w - 1 + l;
        b = p->min_h - 1 + t;
    } else {
        int32_t aw, ah;
        l = anchored(p->anchor[0]) ? p->anchor[0]->rect.right + 1 + p->offset[0]
                                   : p->offset[0] + l;
        t = anchored(p->anchor[1]) ? p->anchor[1]->rect.bottom + 1 + p->offset[1]
                                   : p->offset[1] + t;
        r = anchored(p->anchor[2]) ? p->anchor[2]->rect.left - p->offset[2] - 1
                                   : r - p->offset[2];
        b = anchored(p->anchor[3]) ? p->anchor[3]->rect.top - p->offset[3] - 1
                                   : b - p->offset[3];
        aw = r - l + 1;
        ah = b - t + 1;
        if (p->min_w && aw < p->min_w) {
            r = l - 1 + p->min_w;
        } else if (p->max_w && p->max_w < aw) {
            int32_t m = p->position_mode;
            if (m == 3 || m == 7 || m == 8) r = l - 1 + p->max_w;
            else l = r - p->max_w + 1;
        }
        if (p->min_h && ah < p->min_h) {
            b = t - 1 + p->min_h;
        } else if (p->max_h && p->max_h < ah) {
            int32_t m = p->position_mode;
            if (m == 4 || m == 7 || m == 9) b = t - 1 + p->max_h;
            else t = b - p->max_h + 1;
        }
        if (p->position_mode == 2) {
            int32_t cw = r - l + 1, ch = b - t + 1;
            int32_t nl = cw == w ? 0 : w / 2 - cw / 2;
            r = (r - l) + nl;
            l = nl;
            t = ch == h ? 0 : h / 2 - ch / 2;
            b = ch - 1 + t;
        }
    }
    p->vt->set_rect_ltrb(p, l, t, r, b);
    for (int pass = 0; pass < 2; ++pass)
        for (AoeUiNodeRor *n = p->first; n; n = n->next) {
            int32_t res = n->panel->vt->handle_size(n->panel, w, h);
            if (res) return res;
        }
    return 0;
}

int32_t aoe_ui_panel_handle_paint_ror(AoeUiPanelRor *p) {
    const AoeUiPlatformRor *pf = platform(p);
    if (!p || !p->area || !p->visible || !p->active || !p->system ||
        !p->system->app_active ||
        (pf && pf->iconic && pf->iconic(pf->context, p->area)))
        return 0;
    if (p->redraw) {
        for (AoeUiNodeRor *n = p->first; n; n = n->next)
            n->panel->vt->set_redraw(n->panel, p->redraw);
        if (p->parent && p->parent->overlap_redraw)
            p->parent->vt->redraw_overlaps(p->parent, p, p, p->redraw);
        p->vt->draw(p);
        p->redraw = 0;
        p->painting = 1;
    }
    for (AoeUiNodeRor *n = p->first; n; n = n->next)
        n->panel->vt->handle_paint(n->panel);
    p->painting = 0;
    return 0;
}

int32_t aoe_ui_panel_key_down_ror(AoeUiPanelRor *p, int32_t key,
                                  int32_t count, int32_t alt, int32_t ctrl,
                                  int32_t shift) {
    int32_t r;
    if (!p || !p->has_focus) return 0;
    for (AoeUiNodeRor *n = p->last; n; n = n->prev)
        if ((r = n->panel->vt->key_down(n->panel, key, count, alt, ctrl, shift)))
            return r;
    if ((r = p->vt->key_action(p, key, count, alt, ctrl, shift))) return r;
    if (!alt && !ctrl && key == 9 && p->tab_stop && p->parent) {
        AoeUiPanelRor *next = shift ? p->tab_prev : p->tab_next;
        if (next) {
            aoe_ui_panel_set_current_ror(p->parent, next);
            return 1;
        }
    }
    return 0;
}

int32_t aoe_ui_panel_char_ror(AoeUiPanelRor *p, int32_t key, int32_t count) {
    int32_t r;
    if (!p || !p->has_focus) return 0;
    for (AoeUiNodeRor *n = p->last; n; n = n->prev)
        if ((r = n->panel->vt->char_input(n->panel, key, count))) return r;
    return p->vt->char_action(p, key, count);
}

int32_t aoe_ui_panel_mouse_down_ror(AoeUiPanelRor *p, uint8_t button,
                                    int32_t x, int32_t y, int32_t ctrl,
                                    int32_t shift) {
    const AoeUiPlatformRor *pf = platform(p);
    int32_t r;
    if (!p || !p->active) return 0;
    if (!p->captured) {
        if (!p->vt->is_inside(p, x, y)) return 0;
        for (AoeUiNodeRor *n = p->last; n; n = n->prev)
            if ((r = n->panel->vt->mouse_down(n->panel, button, x, y, ctrl, shift)))
                return r;
        if (!p->handles_mouse) return 0;
        p->mouse_button = button;
        p->down_x = x;
        p->down_y = y;
        p->down_ctrl = ctrl;
        p->down_shift = shift;
        p->down_ms = pf && pf->now_ms ? pf->now_ms(pf->context) : 0;
        p->mouse_state = 1;
        if (p->tab_stop && p->parent && p->mouse_button == 1)
            aoe_ui_panel_set_current_ror(p->parent, p);
    }
    if (!p->handles_mouse) return 0;
    p->vt->actions[p->mouse_button == 1 ? AOE_UI_LEFT_DOWN : AOE_UI_RIGHT_DOWN](
        p, x, y, ctrl, shift);
    return 1;
}

static int32_t magnitude(int32_t v) { return v < 0 ? -v : v; }

int32_t aoe_ui_panel_mouse_move_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                    int32_t ctrl, int32_t shift) {
    int32_t r;
    if (!p || !p->active) return 0;
    if (!p->captured) {
        if (!p->vt->is_inside(p, x, y)) return 0;
        for (AoeUiNodeRor *n = p->last; n; n = n->prev)
            if ((r = n->panel->vt->mouse_move(n->panel, x, y, ctrl, shift)))
                return r;
        return p->handles_mouse ? p->vt->actions[AOE_UI_HOVER](p, x, y, ctrl, shift)
                                : 0;
    }
    if (!p->handles_mouse) return 0;
    if (p->mouse_state == 1 || p->mouse_state == 2) {
        int32_t dx = magnitude(x - p->down_x), dy = magnitude(y - p->down_y);
        if (p->mouse_button == 1) {
            if (dx <= p->drag_slack && dy <= p->drag_slack) return 1;
        } else if (dx < 11 && dy < 11) {
            return 1;
        }
    }
    p->mouse_state = 3;
    if (p->mouse_button == 1)
        p->vt->actions[AOE_UI_LEFT_DRAG](p, x, y, ctrl, shift);
    else if (p->mouse_button != 2)
        p->vt->actions[AOE_UI_HOVER](p, x, y, ctrl, shift);
    else
        p->vt->actions[AOE_UI_RIGHT_DRAG](p, x, y, ctrl, shift);
    return 1;
}

int32_t aoe_ui_panel_mouse_up_ror(AoeUiPanelRor *p, uint8_t button, int32_t x,
                                  int32_t y, int32_t ctrl, int32_t shift) {
    int32_t r;
    if (!p || !p->active) return 0;
    if (!p->captured) {
        if (!p->vt->is_inside(p, x, y)) return 0;
        for (AoeUiNodeRor *n = p->last; n; n = n->prev)
            if ((r = n->panel->vt->mouse_up(n->panel, button, x, y, ctrl, shift)))
                return r;
        return 0;
    }
    if (!p->handles_mouse) return 0;
    if (p->vt->actions[p->mouse_button == 1 ? AOE_UI_LEFT_UP : AOE_UI_RIGHT_UP](
            p, x, y, ctrl, shift))
        return 0;
    if (button == p->mouse_button) {
        p->mouse_button = 0;
        p->mouse_state = 0;
    }
    return 1;
}

int32_t aoe_ui_panel_mouse_dbl_ror(AoeUiPanelRor *p, uint8_t button, int32_t x,
                                   int32_t y, int32_t ctrl, int32_t shift) {
    int32_t r;
    if (!p || !p->active) return 0;
    if (!p->captured) {
        if (!p->vt->is_inside(p, x, y)) return 0;
        for (AoeUiNodeRor *n = p->last; n; n = n->prev)
            if ((r = n->panel->vt->mouse_dbl(n->panel, button, x, y, ctrl, shift)))
                return r;
    }
    if (!p->handles_mouse) return 0;
    if ((r = p->vt->actions[button == 1 ? AOE_UI_LEFT_DBL : AOE_UI_RIGHT_DBL](
             p, x, y, ctrl, shift)))
        return r;
    p->vt->mouse_down(p, button, x, y, ctrl, shift);
    return 1;
}

int32_t aoe_ui_panel_action_ror(AoeUiPanelRor *p, AoeUiPanelRor *from,
                                int32_t a, int32_t b, int32_t c) {
    if (!p || !p->parent) return 0;
    return p->parent->vt->action(p->parent, from, a, b, c);
}

void aoe_ui_panel_get_true_rect_ror(AoeUiPanelRor *p, AoeUiRectRor *out) {
    if (p && out) *out = p->rect;
}

int32_t aoe_ui_panel_is_inside_ror(AoeUiPanelRor *p, int32_t x, int32_t y) {
    return p && p->active && p->clip.left <= x && x <= p->clip.right &&
           p->clip.top <= y && y <= p->clip.bottom;
}

void aoe_ui_panel_set_focus_ror(AoeUiPanelRor *p, int32_t focus) {
    if (!p || focus == p->has_focus) return;
    p->has_focus = focus;
    p->vt->set_redraw(p, 1);
    if (p->current) p->current->vt->set_focus(p->current, p->has_focus);
}

void aoe_ui_panel_set_tab_order_ror(AoeUiPanelRor *p, AoeUiPanelRor *prev,
                                    AoeUiPanelRor *next) {
    if (!p) return;
    p->tab_prev = prev;
    p->tab_next = next;
    p->tab_stop = 1;
}

int aoe_ui_panel_get_help_ror(AoeUiPanelRor *p, char *text, int32_t *page,
                              int32_t x, int32_t y) {
    if (!p || !p->active || !p->vt->is_inside(p, x, y)) return 0;
    for (AoeUiNodeRor *n = p->last; n; n = n->prev)
        if (n->panel->vt->get_help(n->panel, text, page, x, y)) return 1;
    if (p->help_id < 0) return 0;
    if (text) strcpy(text, aoe_ui_string_static_ror(p->system, p->help_id));
    if (page) *page = p->help_page;
    return 1;
}

void aoe_ui_children_stop_sound_ror(AoeUiPanelRor *p) {
    for (AoeUiNodeRor *n = p ? p->first : NULL; n; n = n->next)
        n->panel->vt->stop_sound(n->panel);
}

int32_t aoe_ui_children_restart_sound_ror(AoeUiPanelRor *p) {
    int32_t ok = 1;
    for (AoeUiNodeRor *n = p ? p->first : NULL; n; n = n->next)
        if (!n->panel->vt->restart_sound(n->panel)) ok = 0;
    return ok;
}

void aoe_ui_children_e0_ror(AoeUiPanelRor *p) {
    for (AoeUiNodeRor *n = p ? p->first : NULL; n; n = n->next)
        n->panel->vt->slot_e0(n->panel);
}

void aoe_ui_panel_stop_sound_ror(AoeUiPanelRor *p) {
    aoe_ui_children_stop_sound_ror(p);
}

int32_t aoe_ui_panel_restart_sound_ror(AoeUiPanelRor *p) {
    return aoe_ui_children_restart_sound_ror(p);
}

void aoe_ui_panel_slot_e0_ror(AoeUiPanelRor *p) { aoe_ui_children_e0_ror(p); }

int32_t aoe_ui_panel_ignore_ror(AoeUiPanelRor *p, uint32_t a, uint32_t b) {
    (void)p; (void)a; (void)b;
    return 0;
}

int32_t aoe_ui_panel_no_action_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                   int32_t ctrl, int32_t shift) {
    (void)p; (void)x; (void)y; (void)ctrl; (void)shift;
    return 0;
}

int32_t aoe_ui_panel_no_char_action_ror(AoeUiPanelRor *p, int32_t a,
                                        int32_t b) {
    (void)p; (void)a; (void)b;
    return 0;
}

static int32_t no_key_action(AoeUiPanelRor *p, int32_t a, int32_t b,
                             int32_t c, int32_t d, int32_t e) {
    (void)p; (void)a; (void)b; (void)c; (void)d; (void)e;
    return 0;
}

int32_t aoe_ui_panel_zero_ror(AoeUiPanelRor *p) { (void)p; return 0; }

int aoe_ui_panel_capture_ror(AoeUiPanelRor *p) {
    AoeUiSystemRor *s = p ? p->system : NULL;
    const AoeUiPlatformRor *pf = platform(p);
    if (!s) return 0;
    if (s->capture && s->capture != p) return 0;
    if (s->capture == p) return 1;
    if (pf && pf->capture) pf->capture(pf->context, p->area);
    s->capture = p;
    p->captured = 1;
    return 1;
}

void aoe_ui_panel_release_capture_ror(AoeUiPanelRor *p) {
    const AoeUiPlatformRor *pf = platform(p);
    if (!p || !p->captured) return;
    if (p->system && p->system->capture == p) {
        p->system->capture = NULL;
        if (pf && pf->release) pf->release(pf->context);
    }
    p->captured = 0;
}

int aoe_ui_point_in_rect_ror(int32_t x, int32_t y, const AoeUiRectRor *r) {
    return r && r->left <= x && x <= r->right && r->top <= y && y <= r->bottom;
}

int aoe_ui_panel_clamp_point_ror(AoeUiPanelRor *p, int32_t *x, int32_t *y) {
    int moved = 0;
    if (!p || !x || !y) return 0;
    if (*x < p->clip.left + 1) {
        *x = p->clip.left + 1;
        moved = 1;
    } else if (p->clip.right - 1 < *x) {
        *x = p->clip.right - 1;
        moved = 1;
    }
    if (*y < p->clip.top + 1) {
        *y = p->clip.top + 1;
        return 1;
    }
    if (p->clip.bottom - 1 < *y) {
        *y = p->clip.bottom - 1;
        moved = 1;
    }
    return moved;
}

void aoe_ui_panel_set_current_ror(AoeUiPanelRor *p, AoeUiPanelRor *child) {
    if (!p || child == p->current) return;
    if (p->current && p->has_focus) p->current->vt->set_focus(p->current, 0);
    p->current = child;
    if (child && p->has_focus) child->vt->set_focus(child, 1);
}

int32_t aoe_ui_panel_has_focus_ror(const AoeUiPanelRor *p) {
    return p ? p->has_focus : 0;
}

void aoe_ui_panel_set_z_ror(AoeUiPanelRor *p, uint8_t mode, int16_t z) {
    if (!p) return;
    if (p->parent) {
        aoe_ui_panel_set_child_z_ror(p->parent, p, mode, z);
        return;
    }
    if (mode == 0) p->z = z;
    else if (mode == 1 || mode == 2) p->z = 0;
}

void aoe_ui_panel_set_child_z_ror(AoeUiPanelRor *parent, AoeUiPanelRor *child,
                                  uint8_t mode, int16_t z) {
    AoeUiPanelRor *focus;
    AoeUiNodeRor *node;
    if (!parent || !child) return;
    if (mode == 0) {
        int taken = 0;
        for (AoeUiNodeRor *n = parent->first; n; n = n->next)
            if (n->panel->z == z) taken = 1;
        if (taken)
            for (AoeUiNodeRor *n = parent->first; n; n = n->next)
                if (z <= n->panel->z) ++n->panel->z;
        child->z = z;
    } else if (mode == 1) {
        int16_t top = 0;
        for (AoeUiNodeRor *n = parent->first; n; n = n->next)
            if (n->panel != child && top < n->panel->z) top = (int16_t)n->panel->z;
        child->z = top + 1;
    } else if (mode == 2) {
        int16_t bottom = 0;
        for (AoeUiNodeRor *n = parent->first; n; n = n->next)
            if (n->panel != child && n->panel->z < bottom)
                bottom = (int16_t)n->panel->z;
        child->z = bottom - 1;
    }
    focus = parent->current;
    parent->current = NULL;
    for (node = parent->first; node && node->panel != child; node = node->next) {}
    if (node) {
        aoe_ui_remove_child_ror(parent, node);
        for (AoeUiNodeRor *n = parent->first; n; n = n->next)
            if (child->z < n->panel->z) {
                aoe_ui_insert_child_ror(parent, node, n, 0);
                parent->current = focus;
                return;
            }
        aoe_ui_insert_child_ror(parent, node, parent->last, 1);
    }
    parent->current = focus;
}

void aoe_ui_panels_tab_cycle_ror(AoeUiPanelRor **panels, int16_t count) {
    for (int32_t i = 0; panels && i < count; ++i) {
        AoeUiPanelRor *prev = i == 0 ? panels[count - 1] : panels[i - 1];
        AoeUiPanelRor *next = i == count - 1 ? panels[0] : panels[i + 1];
        panels[i]->vt->set_tab_order(panels[i], prev, next);
    }
}

AoeUiPanelRor *aoe_ui_panel_field4_ror(const AoeUiPanelRor *p) { return p ? p->previous : NULL; }
void aoe_ui_panel_set_field8_ror(AoeUiPanelRor *p, AoeUiPanelRor *v) { if (p) p->chained_modal = v; }
const char *aoe_ui_panel_name_ror(const AoeUiPanelRor *p) { return p ? p->name : NULL; }
int32_t aoe_ui_panel_y_ror(const AoeUiPanelRor *p) { return p ? p->y : 0; }
int32_t aoe_ui_panel_width_ror(const AoeUiPanelRor *p) { return p ? p->width : 0; }
int32_t aoe_ui_panel_height_ror(const AoeUiPanelRor *p) { return p ? p->height : 0; }
AoeUiAreaRor *aoe_ui_panel_area_ror(const AoeUiPanelRor *p) { return p ? p->area : NULL; }

void aoe_ui_panel_set_help_ror(AoeUiPanelRor *p, int32_t id, int32_t page) {
    if (!p) return;
    p->help_id = id;
    p->help_page = page;
}

int aoe_ui_string_ror(const AoeUiSystemRor *s, int32_t id, char *out,
                      int size) {
    if (!out || size <= 0) return 0;
    out[0] = '\0';
    if (s && s->platform && s->platform->string)
        s->platform->string(s->platform->context, id, out, size);
    return out[0] != '\0';
}

const char *aoe_ui_string_static_ror(const AoeUiSystemRor *s, int32_t id) {
    static char buffer[0x200];
    aoe_ui_string_ror(s, id, buffer, sizeof(buffer));
    return buffer;
}

const AoeUiPanelVtblRor aoe_ui_panel_vtable_ror = {
    aoe_ui_panel_destroy_ror,
    aoe_ui_panel_setup_ror,
    aoe_ui_panel_set_rect_ltrb_ror,
    aoe_ui_panel_set_rect_ror,
    aoe_ui_panel_set_color_ror,
    aoe_ui_panel_set_active_ror,
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
    {aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror},
    no_key_action,
    aoe_ui_panel_zero_ror,
    aoe_ui_panel_no_char_action_ror,
    aoe_ui_panel_action_ror,
    aoe_ui_panel_get_true_rect_ror,
    aoe_ui_panel_is_inside_ror,
    aoe_ui_panel_set_focus_ror,
    aoe_ui_panel_set_tab_order_ror,
    aoe_ui_panel_get_help_ror,
    aoe_ui_panel_stop_sound_ror,
    aoe_ui_panel_restart_sound_ror,
    aoe_ui_panel_slot_e0_ror,
};

int aoe_ui_system_init_ror(AoeUiSystemRor *s, const AoeUiPlatformRor *platform) {
    if (!s) return 0;
    memset(s, 0, sizeof *s);
    s->platform = platform;
    s->ime_off = 1;
    s->enabled = 1;
    if (platform && platform->get_sys_color) {
        s->window_color = platform->get_sys_color(platform->context, 5);
        s->text_color = platform->get_sys_color(platform->context, 8);
    }
    s->colors_saved = 1;
    return 1;
}

void aoe_ui_system_restore_colors_ror(const AoeUiSystemRor *s) {
    const AoeUiPlatformRor *p;
    if (!s || !s->colors_saved || !(p = s->platform) || !p->get_sys_color) return;
    if (p->get_sys_color(p->context, 5) == s->window_color &&
        p->get_sys_color(p->context, 8) == s->text_color)
        return;
    if (p->set_sys_colors) p->set_sys_colors(p->context, s->window_color, s->text_color);
}

void aoe_ui_system_restore_ime_ror(const AoeUiSystemRor *s) {
    if (s && s->ime && !s->ime_off && s->platform && s->platform->ime_associate)
        s->platform->ime_associate(s->platform->context, s->ime);
}

void aoe_ui_system_release_ror(AoeUiSystemRor *s) {
    int32_t i;
    if (!s) return;
    aoe_ui_system_restore_colors_ror(s);
    for (i = 0; i < s->panel_count; ++i) {
        if (!s->panels[i]) continue;
        s->panels[i]->vt->destroy(s->panels[i]);
        free(s->panels[i]);
    }
    free(s->panels);
    s->panels = NULL;
    s->panel_count = s->panel_capacity = 0;
    for (i = 0; i < AOE_UI_PALETTES; ++i) {
        if (!s->palettes[i]) continue;
        if (s->platform && s->platform->delete_palette)
            s->platform->delete_palette(s->platform->context, s->palettes[i]);
        s->palettes[i] = 0;
        s->palette_names[i][0] = 0;
        s->palette_refs[i] = 0;
        s->palette_ids[0] = 0; /* the original clears +0xa94 (slot 0) each time */
    }
    aoe_ui_system_restore_ime_ror(s);
}
