#include "ui_button.h"

#include "draw_prims.h"
#include "draw_system.h"
#include "shape.h"

#include <stdlib.h>
#include <string.h>

#define B(p) ((AoeUiButtonRor *)(p))
#define BVT(b) ((const AoeUiButtonVtblRor *)(b)->panel.vt)

static uint32_t now(AoeUiButtonRor *b) {
    const AoeUiPlatformRor *pf = b->panel.system ? b->panel.system->platform : NULL;
    return pf && pf->now_ms ? pf->now_ms(pf->context) : 0;
}

static void redraw(AoeUiButtonRor *b, int32_t level) {
    b->panel.vt->set_redraw(&b->panel, level);
}

static void notify_parent(AoeUiButtonRor *b, int32_t code) {
    AoeUiPanelRor *parent = b->panel.parent;
    if (parent)
        parent->vt->action(parent, &b->panel, code, b->ids[b->state],
                           b->values[b->state]);
}

static void play_sound(AoeUiButtonRor *b) {
    if (!b->sound || !b->ops) return;
    if (b->ops->sound_playing && b->ops->sound_playing(b->ops->context, b->sound) &&
        b->ops->sound_stop)
        b->ops->sound_stop(b->ops->context, b->sound);
    if (b->ops->sound_play) b->ops->sound_play(b->ops->context, b->sound);
}

void aoe_ui_button_init_ror(AoeUiButtonRor *b, AoeUiSystemRor *system,
                            const AoeUiButtonOpsRor *ops) {
    if (!b) return;
    memset(b, 0, sizeof(*b));
    aoe_ui_panel_init_ror(&b->panel, system);
    b->panel.vt = &aoe_ui_button_vtable_ror.base;
    b->ops = ops;
    b->type = 1;
    b->draw_type = 1;
    b->notify = 1;
    b->center_picture = 1;
    b->rect_hit = 1;
    b->text_x = b->text_y = -1;
    b->state_count = 1;
    b->sound_id = -1;
    b->panel.field_ee = 3;
    for (int i = 0; i < AOE_UI_BUTTON_STATES; ++i) {
        b->frames[i] = -1;
        b->text_color[i] = 0xffffff;
        b->focus_color[i] = 0xffff;
    }
}

static void free_texts(AoeUiButtonRor *b, int16_t state) {
    free(b->text1[state]);
    free(b->text2[state]);
    b->text1[state] = b->text2[state] = NULL;
}

void aoe_ui_button_release_ror(AoeUiButtonRor *b) {
    if (!b) return;
    for (int16_t i = 0; i < AOE_UI_BUTTON_STATES; ++i) free_texts(b, i);
    free(b->group);
    b->group = NULL;
    aoe_ui_panel_release_ror(&b->panel);
}

static void button_destroy(AoeUiPanelRor *p) { aoe_ui_button_release_ror(B(p)); }

int aoe_ui_button_setup_ror(AoeUiButtonRor *b, AoeUiAreaRor *area,
                            AoeUiPanelRor *parent, int32_t x, int32_t y,
                            int32_t w, int32_t h, int32_t draw_type,
                            void *sound, int32_t notify, int32_t id) {
    if (!b) return 0;
    aoe_ui_panel_setup_ror(&b->panel, area, parent, x, y, w, h, 0);
    b->notify = notify;
    b->draw_type = draw_type;
    aoe_ui_button_set_sound_ror(b, sound);
    aoe_ui_button_set_id_ror(b, 0, id, 0);
    return 1;
}

void aoe_ui_button_set_draw_type_ror(AoeUiButtonRor *b, int32_t type) {
    if (b) b->draw_type = type;
}

void aoe_ui_button_set_text_pos_ror(AoeUiButtonRor *b, int32_t x, int32_t y) {
    if (!b) return;
    b->text_x = x;
    b->text_y = y;
    redraw(b, 1);
}

void aoe_ui_button_set_text_info_ror(AoeUiButtonRor *b, const char *text,
                                     uintptr_t font, int32_t font_width,
                                     int32_t font_height, int32_t x,
                                     int32_t y) {
    aoe_ui_button_set_text_pos_ror(b, x, y);
    aoe_ui_button_set_text_ror(b, 0, text);
    aoe_ui_button_set_font_ror(b, font, font_width, font_height);
}

int aoe_ui_button_set_group_ror(AoeUiButtonRor *b, AoeUiButtonRor **group,
                                int16_t count) {
    if (!b) return 0;
    b->type = 2;
    free(b->group);
    b->group = NULL;
    if (count) {
        b->group_count = count;
        if (!(b->group = calloc((size_t)count, sizeof(*b->group)))) return 0;
        for (int16_t i = 0; i < count; ++i) b->group[i] = group[i];
    }
    return 1;
}

void aoe_ui_button_set_cycle_ror(AoeUiButtonRor *b, int16_t count) {
    if (!b) return;
    b->type = 3;
    b->state_count = count;
}

static int state_ok(int32_t s) { return s >= 0 && s < AOE_UI_BUTTON_STATES; }

void aoe_ui_button_set_id_ror(AoeUiButtonRor *b, int16_t state, int32_t id,
                              int32_t value) {
    if (!b || !state_ok(state)) return;
    b->ids[state] = id;
    b->values[state] = value;
}

void aoe_ui_button_set_picture_ror(AoeUiButtonRor *b, int16_t state,
                                   void *picture, int16_t frame) {
    if (!b || !state_ok(state)) return;
    b->pictures[state] = picture;
    b->frames[state] = frame;
    redraw(b, 1);
}

static char *copy_text(const char *text, size_t n) {
    char *out = malloc(n + 1);
    if (out) {
        memcpy(out, text, n);
        out[n] = '\0';
    }
    return out;
}

int aoe_ui_button_set_text_ror(AoeUiButtonRor *b, int16_t state,
                               const char *text) {
    const char *newline;
    int ok = 1;
    if (!b || !state_ok(state)) return 0;
    free_texts(b, state);
    if (text && *text) {
        newline = strchr(text, '\n');
        b->text1[state] = copy_text(text, newline ? (size_t)(newline - text)
                                                  : strlen(text));
        if (!b->text1[state]) ok = 0;
        else if (newline && !(b->text2[state] = copy_text(newline + 1,
                                                          strlen(newline + 1))))
            ok = 0;
    }
    redraw(b, 1);
    return ok;
}

int aoe_ui_button_set_texts_ror(AoeUiButtonRor *b, int16_t state,
                                const char *line1, const char *line2) {
    int ok = 1;
    if (!b || !state_ok(state)) return 0;
    free_texts(b, state);
    if (line1 && *line1) {
        if (!(b->text1[state] = copy_text(line1, strlen(line1)))) ok = 0;
        else if (line2 && *line2 &&
                 !(b->text2[state] = copy_text(line2, strlen(line2))))
            ok = 0;
    }
    redraw(b, 1);
    return ok;
}

int aoe_ui_button_set_text_id_ror(AoeUiButtonRor *b, int16_t state,
                                  int32_t string_id) {
    char text[0x100];
    if (!b) return 0;
    aoe_ui_string_ror(b->panel.system, string_id, text, sizeof(text));
    return aoe_ui_button_set_text_ror(b, state, text);
}

void aoe_ui_button_set_font_ror(AoeUiButtonRor *b, uintptr_t font,
                                int32_t width, int32_t height) {
    if (!b) return;
    b->font = font;
    b->font_width = width;
    b->font_height = height;
    redraw(b, 1);
}

void aoe_ui_button_set_text_color_ror(AoeUiButtonRor *b, int32_t state,
                                      uint32_t color, uint32_t shadow) {
    if (!b || !state_ok(state)) return;
    b->text_color[state] = color;
    b->text_shadow[state] = shadow;
    redraw(b, 1);
}

void aoe_ui_button_set_focus_color_ror(AoeUiButtonRor *b, int32_t state,
                                       uint32_t color, uint32_t shadow) {
    if (!b || !state_ok(state)) return;
    b->focus_color[state] = color;
    b->focus_shadow[state] = shadow;
    redraw(b, 1);
}

void aoe_ui_button_set_sound_ror(AoeUiButtonRor *b, void *sound) {
    if (b) b->sound = sound;
}

void aoe_ui_button_set_bevel_ror(AoeUiButtonRor *b, int32_t type,
                                 const uint8_t colors[6]) {
    if (!b) return;
    b->bevel_type = type;
    if (colors) memcpy(b->bevel, colors, sizeof(b->bevel));
}

void aoe_ui_button_set_disabled_ror(AoeUiButtonRor *b, int32_t disabled) {
    if (!b) return;
    b->disabled = disabled;
    redraw(b, 1);
    b->panel.handles_mouse = disabled ? 0 : 1;
}

void aoe_ui_button_set_sound_id_ror(AoeUiButtonRor *b, int32_t id) {
    if (b) b->sound_id = id;
}

void aoe_ui_button_select_ror(AoeUiButtonRor *b) {
    if (!b) return;
    for (int16_t i = 0; i < b->group_count; ++i) {
        b->group[i]->pressed = 0;
        BVT(b->group[i])->set_state(b->group[i], 0);
    }
    if (b->draw_type == 3 || b->draw_type == 5) b->pressed = 1;
    BVT(b)->set_state(b, 1);
}

void aoe_ui_button_set_state_ror(AoeUiButtonRor *b, int16_t state) {
    if (!b) return;
    b->state = state;
    redraw(b, 1);
}

void aoe_ui_button_set_state_by_id_ror(AoeUiButtonRor *b, int32_t id) {
    for (int16_t i = 0; b && i < b->state_count && i < AOE_UI_BUTTON_STATES; ++i)
        if (b->ids[i] == id) {
            b->state = i;
            redraw(b, 1);
            return;
        }
}

int16_t aoe_ui_button_state_ror(const AoeUiButtonRor *b) { return b ? b->state : 0; }
int32_t aoe_ui_button_id_ror(const AoeUiButtonRor *b) { return b ? b->ids[b->state] : 0; }
int32_t aoe_ui_button_value_ror(const AoeUiButtonRor *b) {
    return b ? b->values[b->state] : 0;
}

int aoe_ui_button_texts_ror(const AoeUiButtonRor *b, int16_t state,
                            const char **line1, const char **line2) {
    if (!b || !line1 || !line2) return 0;
    if (state == -1) state = b->state;
    if (!state_ok(state)) return 0;
    *line1 = b->text1[state];
    *line2 = b->text2[state];
    return *line1 != NULL;
}

void aoe_ui_button_colors_ror(const AoeUiButtonRor *b, int16_t state,
                              uint32_t *color, uint32_t *shadow) {
    if (!b || !color || !shadow) return;
    if (state == -1) state = b->state;
    if (!state_ok(state)) return;
    *color = b->text_color[state];
    *shadow = b->text_shadow[state];
}

int32_t aoe_ui_button_hit_test_ror(AoeUiButtonRor *b, int32_t x, int32_t y) {
    void *picture;
    if (!b || !b->panel.vt->is_inside(&b->panel, x, y)) return 0;
    picture = state_ok(b->state) ? b->pictures[b->state] : NULL;
    if (b->rect_hit || !picture) return 1;
    return b->ops && b->ops->shape_hit &&
           b->ops->shape_hit(b->ops->context, picture,
                             x - b->picture_x - b->panel.x,
                             y - b->picture_y - b->panel.y, b->frames[b->state]);
}

static void finish_click(AoeUiButtonRor *b, int32_t released, int32_t clicked) {
    if (b->type == 3) {
        BVT(b)->set_state(b, b->state == b->state_count - 1 ? 0 : b->state + 1);
    } else if (b->type == 2) {
        aoe_ui_button_select_ror(b);
    }
    notify_parent(b, released);
    if (b->notify == 1) {
        notify_parent(b, clicked);
    } else if ((b->ids[b->state] || b->values[b->state]) && b->ops &&
               b->ops->command) {
        b->ops->command(b->ops->context, b->panel.area, b->ids[b->state],
                        b->values[b->state]);
    }
}

void aoe_ui_button_click_ror(AoeUiButtonRor *b) { if (b) finish_click(b, 3, 1); }
void aoe_ui_button_right_click_ror(AoeUiButtonRor *b) { if (b) finish_click(b, 6, 4); }

static void paint_now(AoeUiButtonRor *b) {
    if (b->ops && b->ops->paint_now) b->ops->paint_now(b->ops->context, b->panel.area);
}

void aoe_ui_button_cancel_ror(AoeUiButtonRor *b) {
    if (!b) return;
    aoe_ui_panel_release_capture_ror(&b->panel);
    b->held_key = 0;
    if (b->pressed) {
        if (b->type != 2) b->pressed = 0;
        redraw(b, 1);
    }
}

static int32_t press(AoeUiButtonRor *b, int32_t code) {
    if (b->disabled) return 0;
    play_sound(b);
    aoe_ui_panel_capture_ror(&b->panel);
    b->pressed = 1;
    b->press_ms = now(b);
    redraw(b, 1);
    notify_parent(b, code);
    return 1;
}

static int32_t drag(AoeUiButtonRor *b, int32_t x, int32_t y, int32_t in,
                    int32_t out) {
    int32_t inside = b->panel.vt->is_inside(&b->panel, x, y) != 0;
    if (b->pressed != inside) {
        b->pressed = inside;
        if (inside) b->press_ms = now(b);
        redraw(b, 1);
        notify_parent(b, b->pressed ? in : out);
    }
    return 1;
}

static int32_t release(AoeUiButtonRor *b, void (*click)(AoeUiButtonRor *)) {
    aoe_ui_panel_release_capture_ror(&b->panel);
    if (b->pressed) {
        if (b->type != 2) b->pressed = 0;
        redraw(b, 1);
        paint_now(b);
        click(b);
        return 1;
    }
    if (b->type == 2 && b->state == 1) {
        b->pressed = 1;
        redraw(b, 1);
    }
    return 1;
}

static int32_t left_down(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)x; (void)y; (void)c; (void)s;
    return press(B(p), 2);
}
static int32_t right_down(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)x; (void)y; (void)c; (void)s;
    return press(B(p), 5);
}
static int32_t left_drag(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)c; (void)s;
    return drag(B(p), x, y, 2, 3);
}
static int32_t right_drag(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)c; (void)s;
    return drag(B(p), x, y, 5, 6);
}
static int32_t left_up(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)x; (void)y; (void)c; (void)s;
    return release(B(p), aoe_ui_button_click_ror);
}
static int32_t right_up(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)x; (void)y; (void)c; (void)s;
    return release(B(p), aoe_ui_button_right_click_ror);
}

static int32_t mouse_down(AoeUiPanelRor *p, uint8_t button, int32_t x,
                          int32_t y, int32_t c, int32_t s) {
    if (!p->active || p->captured || !BVT(B(p))->hit_test(B(p), x, y)) return 0;
    return aoe_ui_panel_mouse_down_ror(p, button, x, y, c, s);
}

static int32_t key_down(AoeUiPanelRor *p, int32_t key, int32_t count,
                        int32_t alt, int32_t ctrl, int32_t shift) {
    AoeUiButtonRor *b = B(p);
    if (p->active && !b->disabled && !alt && !ctrl && b->hotkey &&
        key == b->hotkey && !b->held_key &&
        (!b->hotkey_shift || shift == b->hotkey_shift)) {
        if (p->tab_stop && p->parent && !p->has_focus)
            aoe_ui_panel_set_current_ror(p->parent, p);
        play_sound(b);
        b->held_key = key;
        b->pressed = 1;
        b->press_ms = now(b);
        redraw(b, 1);
        notify_parent(b, 2);
        return 1;
    }
    return aoe_ui_panel_key_down_ror(p, key, count, alt, ctrl, shift);
}

static int32_t key_action(AoeUiPanelRor *p, int32_t key, int32_t count,
                          int32_t alt, int32_t ctrl, int32_t shift) {
    AoeUiButtonRor *b = B(p);
    AoeUiPanelRor *target;
    (void)count; (void)shift;
    if (b->held_key) return 0;
    switch (key) {
    case 0x0d:
    case 0x20:
        if (b->disabled || alt || ctrl) return 0;
        play_sound(b);
        b->held_key = key;
        b->pressed = 1;
        b->press_ms = now(b);
        redraw(b, 1);
        notify_parent(b, 2);
        return 1;
    case 0x26:
    case 0x28:
        if (alt || ctrl || !p->tab_stop || !p->parent) return 1;
        target = key == 0x26 ? p->tab_prev : p->tab_next;
        if (target) aoe_ui_panel_set_current_ror(p->parent, target);
        return 1;
    default:
        return 0;
    }
}

static int32_t handle_message(AoeUiPanelRor *p, void *window, uint32_t msg,
                              uint32_t wparam, uint32_t lparam) {
    AoeUiButtonRor *b = B(p);
    int32_t held = b->held_key;
    if (msg == 0x101 && held && (int32_t)wparam == held &&
        (held == b->hotkey || !b->hotkey || held == 0x0d || held == 0x20)) {
        b->held_key = 0;
        if (b->type != 2) b->pressed = 0;
        redraw(b, 1);
        paint_now(b);
        aoe_ui_button_click_ror(b);
        return 1;
    }
    return aoe_ui_panel_handle_message_ror(p, window, msg, wparam, lparam);
}

static int32_t zero_action(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)p; (void)x; (void)y; (void)c; (void)s;
    return 0;
}

static void stop_sound(AoeUiPanelRor *p) { B(p)->sound = NULL; }

static int32_t restart_sound(AoeUiPanelRor *p) {
    AoeUiButtonRor *b = B(p);
    if (b->sound_id != -1 && b->ops && b->ops->sound_by_id)
        b->sound = b->ops->sound_by_id(b->ops->context, b->sound_id);
    return 1;
}

static void set_rect(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t w, int32_t h) {
    aoe_ui_panel_set_rect_ror(p, x, y, w, h);
}

/* ---- drawing (0x0045f4b0) ---- */

static AoeShapeSurfaceRor *lock_area(AoeUiButtonRor *b) {
    struct AoeDrawSurfaceRor *s = b->panel.area ? b->panel.area->surface : NULL;
    if (!s || !aoe_surface_lock_ror(s, 1)) return NULL;
    return &s->view;
}

static void unlock_area(AoeUiButtonRor *b) {
    struct AoeDrawSurfaceRor *s = b->panel.area ? b->panel.area->surface : NULL;
    if (s) aoe_surface_unlock_ror(s);
}

/* The pressed look: pressed, inverted by `invert_press`. */
static int pressed_look(const AoeUiButtonRor *b) {
    return b->invert_press ? !b->pressed : b->pressed != 0;
}

static void draw_text(AoeUiButtonRor *b, const AoeUiPlatformRor *pf,
                      const char *text, int32_t x, int32_t y) {
    int16_t st = b->state;
    int32_t n = (int16_t)strlen(text);
    uint32_t color = b->panel.has_focus ? b->focus_color[st] : b->text_color[st];
    uint32_t shadow = b->panel.has_focus ? b->focus_shadow[st] : b->text_shadow[st];
    pf->text_out(pf->context, x - 1, y + 1, text, n, shadow);
    pf->text_out(pf->context, x, y, text, n, color);
}

void aoe_ui_button_draw_slot_ror(AoeUiPanelRor *p) {
    AoeUiButtonRor *b = B(p);
    const AoeUiPlatformRor *pf = p->system ? p->system->platform : NULL;
    int16_t st = b->state;
    AoeShapeSurfaceRor *v;
    int32_t t = b->draw_type;
    if (!p->area || !p->visible || !p->active || t == 1) {
        p->redraw = 0;
        return;
    }
    if (t == 5) {
        struct AoeDrawSurfaceRor *s = p->area->surface;
        if (s) {
            int32_t r[4] = {p->clip.left, p->clip.top, p->clip.right,
                            p->clip.bottom};
            aoe_surface_fill_ror(s, r, p->color);
        }
    } else if (p->parent) {
        if (b->bevel_type < 2 || b->bevel_type > 4) {
            if (!p->parent->painting) p->parent->vt->draw_rect(p->parent, &p->clip);
        } else {
            p->parent->vt->draw_parent_rect(p->parent, &p->clip);
        }
    }
    p->vt->begin_draw(p, 0);
    if ((t == 2 || t == 4 || t == 6) && (v = lock_area(b)) != NULL) {
        (void)v;
        if (b->pictures[st]) {
            int32_t x, y;
            if (b->center_picture) {
                int32_t x0 = 0, y0 = 0, x1 = -1, y1 = -1;
                aoe_shape_bounds_ror(b->pictures[st], b->frames[st], &x0, &y0,
                                     &x1, &y1);
                b->picture_x = (int16_t)((int16_t)(p->width / 2) -
                                         (int16_t)((x1 - x0 + 1) / 2) -
                                         (int16_t)x0);
                b->picture_y = (int16_t)((int16_t)(p->height / 2) -
                                         (int16_t)((y1 - y0 + 1) / 2) -
                                         (int16_t)y0);
            }
            x = b->picture_x + p->x;
            y = b->picture_y + p->y;
            if (pressed_look(b)) {
                ++x;
                --y;
            }
            if (pf && pf->draw_shape)
                pf->draw_shape(pf->context, p->area, b->pictures[st],
                               b->frames[st], x, y);
        }
        unlock_area(b);
    }
    if ((t == 3 || t == 4 || t == 5) && pf && pf->text_begin &&
        pf->text_begin(pf->context, p->area, &p->clip, b->font)) {
        int32_t tx = b->text_x == -1 ? p->width / 2 : b->text_x;
        int32_t ty = b->text_y == -1 ? p->height / 2 : b->text_y;
        const char *s1 = b->text1[st] ? b->text1[st] : b->text1[0];
        if (s1) {
            const char *s2;
            int32_t w = pf->text_width
                            ? pf->text_width(pf->context, b->font, s1,
                                             (int16_t)strlen(s1))
                            : 0;
            int32_t x = p->x - w / 2 + tx;
            int32_t y = (b->text2[st] ? p->y - b->font_height
                                      : p->y - b->font_height / 2) + ty;
            if (pressed_look(b)) {
                ++x;
                --y;
            }
            draw_text(b, pf, s1, x, y);
            s2 = b->text2[st] ? b->text2[st] : b->text2[0];
            if (s2) {
                w = pf->text_width ? pf->text_width(pf->context, b->font, s2,
                                                    (int16_t)strlen(s2))
                                   : 0;
                x = p->x - w / 2 + tx;
                y = p->y + ty;
                if (pressed_look(b)) {
                    ++x;
                    --y;
                }
                draw_text(b, pf, s2, x, y);
            }
        }
        pf->text_end(pf->context);
    }
    if ((t == 3 || t == 4 || t == 5 || t == 6) && (v = lock_area(b)) != NULL) {
        const uint8_t *e = b->bevel;
        int32_t l = p->x, top = p->y, r = p->width - 1 + p->x,
                bot = p->height - 1 + p->y;
        uint8_t c[6];
        if (b->disabled) {
            aoe_draw_rect_ror(v, l, top, r, bot, e[5]);
            aoe_draw_rect_ror(v, l + 1, top + 1, r - 1, bot - 1, e[0]);
        } else if (pressed_look(b)) {
            switch (b->bevel_type) {
            case 1:
                aoe_draw_rect_ror(v, l + 1, top + 1, r - 1, bot - 1, 0xff);
                break;
            case 2:
                aoe_draw_bevel1_ror(v, l, top, r, bot, e[5], e[0]);
                break;
            case 3:
                c[0] = e[5], c[1] = e[4], c[2] = e[1], c[3] = e[0];
                aoe_draw_bevel2_flat_ror(v, l, top, r, bot, c);
                break;
            case 4:
                c[0] = e[5], c[1] = e[4], c[2] = e[3], c[3] = e[2],
                c[4] = e[1], c[5] = e[0];
                aoe_draw_bevel3_flat_ror(v, l, top, r, bot, c);
                break;
            }
        } else {
            switch (b->bevel_type) {
            case 1:
                aoe_draw_vline_ror(v, p->x - 1 + p->width, p->y + 1,
                                   p->height - 1, 0);
                aoe_draw_hline_ror(v, p->x + 1, p->y - 1 + p->height,
                                   p->width - 1, 0);
                aoe_draw_rect_ror(v, l, top, r - 1, bot - 1, 0xff);
                break;
            case 2:
                aoe_draw_bevel1_ror(v, l, top, r, bot, e[0], e[5]);
                break;
            case 3:
                c[0] = e[0], c[1] = e[1], c[2] = e[4], c[3] = e[5];
                aoe_draw_bevel2_ror(v, l, top, r, bot, c);
                break;
            case 4:
                aoe_draw_bevel3_ror(v, l, top, r, bot, e);
                break;
            }
        }
        unlock_area(b);
    }
    p->vt->end_draw(p);
}

const AoeUiButtonVtblRor aoe_ui_button_vtable_ror = {
    {button_destroy,
     aoe_ui_panel_setup_ror,
     aoe_ui_panel_set_rect_ltrb_ror,
     set_rect,
     aoe_ui_panel_set_color_ror,
     aoe_ui_panel_set_active_ror,
     aoe_ui_panel_set_position_mode_ror,
     aoe_ui_panel_set_fixed_position_ror,
     aoe_ui_panel_set_redraw_ror,
     aoe_ui_panel_redraw_overlaps_ror,
     aoe_ui_panel_begin_draw_ror,
     aoe_ui_panel_end_draw_ror,
     aoe_ui_button_draw_slot_ror,
     aoe_ui_panel_draw_rect_ror,
     aoe_ui_panel_draw_offset_ror,
     aoe_ui_panel_draw_parent_rect_ror,
     aoe_ui_panel_draw_parent_offset_ror,
     aoe_ui_panel_validate_ror,
     handle_message,
     aoe_ui_panel_handle_idle_ror,
     aoe_ui_panel_handle_size_ror,
     aoe_ui_panel_handle_paint_ror,
     key_down,
     aoe_ui_panel_char_ror,
     aoe_ui_panel_ignore_ror,
     aoe_ui_panel_ignore_ror,
     aoe_ui_panel_ignore_ror,
     aoe_ui_panel_ignore_ror,
     mouse_down,
     aoe_ui_panel_mouse_move_ror,
     aoe_ui_panel_mouse_up_ror,
     aoe_ui_panel_mouse_dbl_ror,
     {aoe_ui_panel_no_action_ror, left_down, aoe_ui_panel_no_action_ror,
      left_drag, left_up, zero_action, right_down,
      aoe_ui_panel_no_action_ror, right_drag, right_up, zero_action},
     key_action,
     aoe_ui_panel_zero_ror,
     aoe_ui_panel_no_char_action_ror,
     aoe_ui_panel_action_ror,
     aoe_ui_panel_get_true_rect_ror,
     aoe_ui_panel_is_inside_ror,
     aoe_ui_panel_set_focus_ror,
     aoe_ui_panel_set_tab_order_ror,
     aoe_ui_panel_get_help_ror,
     stop_sound,
     restart_sound,
     aoe_ui_panel_slot_e0_ror},
    aoe_ui_button_set_state_ror,
    aoe_ui_button_hit_test_ror,
};
