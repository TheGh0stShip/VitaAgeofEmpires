#include "mouse_pointer.h"

#include <stdio.h>
#include <string.h>

#define AOE_DDERR_NOCLIPPERATTACHED ((int32_t)0x88760238u)

AoeMousePtrRor *aoe_mouse_global_ror;

void aoe_mouse_init_ror(AoeMousePtrRor *m, int32_t mode, const AoeMouseOpsRor *ops) {
    memset(m, 0, sizeof *m);
    m->last_x = m->last_y = -32767;
    m->mode = mode;
    m->resource = -1;
    m->last_time = ops->time(ops->context) - 0x24;
}

static void *save_surface(const AoeMousePtrRor *m, const AoeMouseOpsRor *ops) {
    return ops->area_surface(ops->context, m->save_area);
}

void aoe_mouse_set_frame_ror(AoeMousePtrRor *m, int game, int32_t frame, const AoeMouseOpsRor *ops) {
    AoeMouseFrameRor f;
    if (frame < 0 || frame >= m->frame_count) return;
    ops->shape_frame(ops->context, m->shape, frame, &f);
    if (game) {
        m->game_frame = frame;
        m->game_w = f.width;
        m->game_h = f.height;
        m->game_hot_y = f.hot_y;
        m->game_hot_x = f.hot_x;
        m->game_right = m->game_w - f.hot_x;
        m->game_below = m->game_h - f.hot_y;
    } else {
        m->normal_frame = frame;
        m->normal_w = f.width;
        m->normal_h = f.height;
        m->normal_hot_y = f.hot_y;
        m->normal_hot_x = f.hot_x;
        m->normal_right = m->normal_w - f.hot_x;
        m->normal_below = m->normal_h - f.hot_y;
    }
}

int aoe_mouse_make_areas_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops) {
    int32_t i;
    char name[32];
    if (!m->enabled) return 1;
    if (!m->save_area && m->max_w > 0 && m->max_h > 0) {
        m->save_area = ops->area_new(ops->context, "Mouse Save area", m->max_w, m->max_h);
        if (!m->save_area) return 0;
    }
    if (m->mode == 0)
        for (i = 0; i < m->max_cursors; ++i) {
            if (m->cursor_areas[i]) continue;
            snprintf(name, sizeof name, "Mouse Cursor %d", (int)i);
            m->cursor_areas[i] = ops->area_new(ops->context, name, m->max_w, m->max_h);
            if (!m->cursor_areas[i]) return 0;
        }
    return 1;
}

void aoe_mouse_forget_ror(AoeMousePtrRor *m) {
    m->saved = 0;
    m->drawn = 0;
}

void aoe_mouse_free_areas_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops) {
    int32_t i;
    if (!m->enabled) return;
    if (m->save_area) {
        ops->area_free(ops->context, m->save_area);
        m->save_area = NULL;
    }
    for (i = 0; i < m->max_cursors; ++i)
        if (m->cursor_areas[i]) {
            ops->area_free(ops->context, m->cursor_areas[i]);
            m->cursor_areas[i] = NULL;
        }
    m->frame_count = 0;
    aoe_mouse_forget_ror(m);
}

int aoe_mouse_load_ror(AoeMousePtrRor *m, const char *name, int32_t resource, int reload,
                       const AoeMouseOpsRor *ops) {
    AoeMouseFrameRor f;
    int32_t i;
    int result = 1;
    if (m->shape) ops->shape_free(ops->context, m->shape);
    if (!reload) {
        m->resource = resource;
        snprintf(m->file, sizeof m->file, "%s.slp", name);
    }
    m->shape = ops->shape_load(ops->context, m->file, m->resource);
    if (!m->shape || !ops->shape_ok(ops->context, m->shape)) return 0;
    m->frame_count = ops->shape_frames(ops->context, m->shape);
    m->max_w = m->max_h = 0;
    for (i = 0; i < m->frame_count; ++i) {
        ops->shape_frame(ops->context, m->shape, i, &f);
        if (m->max_w < f.width) m->max_w = f.width;
        if (m->max_h < f.height) m->max_h = f.height;
    }
    if (!m->max_w || !m->max_h) return 0;
    m->max_w = 0x40;
    m->max_h = 0x30;
    if (!m->save_area && !aoe_mouse_make_areas_ror(m, ops)) return 0;
    aoe_mouse_set_frame_ror(m, 0, m->normal_frame, ops);
    aoe_mouse_set_frame_ror(m, 1, m->game_frame, ops);
    if (m->mode != 0) return 1;
    if (m->frame_count > m->max_cursors) return 0;
    for (i = 0; i < m->frame_count; ++i) {
        ops->shape_frame(ops->context, m->shape, i, &f);
        if (f.width > m->max_w || f.height > m->max_h) return 0;
    }
    for (i = 0; i < m->max_cursors; ++i) {
        ops->draw_sync(ops->context);
        if (i >= m->frame_count) continue;
        if (!ops->area_lock(ops->context, m->cursor_areas[i], "mouseptr::LoadCursors")) {
            result = 0;
            continue;
        }
        ops->area_clear(ops->context, m->cursor_areas[i], m->max_w, m->max_h);
        ops->shape_frame(ops->context, m->shape, i, &f);
        ops->shape_draw(ops->context, m->shape, m->cursor_areas[i], f.hot_x, f.hot_y, i);
        ops->area_unlock(ops->context, m->cursor_areas[i], "mouseptr::LoadCursors");
    }
    return result;
}

int aoe_mouse_setup_ror(AoeMousePtrRor *m, int enabled, AoeMouseHostRor *host, const char *name,
                        int32_t resource, int32_t max_cursors, const AoeMouseOpsRor *ops) {
    char file[260];
    aoe_mouse_global_ror = m;
    m->enabled = enabled;
    m->host = host;
    if (!enabled) return 1;
    if (host->gdi_only) return 0;
    m->draw_surface = host->surface;
    m->primary_surface = host->primary_surface;
    m->primary_area = host->primary_area;
    ops->set_cursor(ops->context, 0);
    snprintf(file, sizeof file, "%s.slp", name);
    m->shape = ops->shape_load(ops->context, file, resource);
    if (!m->shape || !ops->shape_ok(ops->context, m->shape)) return 0;
    m->max_cursors = max_cursors;
    if (max_cursors <= 0 || max_cursors > 16) return 0;
    m->frame_count = ops->shape_frames(ops->context, m->shape);
    if (m->frame_count > m->max_cursors || !aoe_mouse_load_ror(m, name, resource, 0, ops)) return 0;
    aoe_mouse_set_frame_ror(m, 0, 0, ops);
    aoe_mouse_set_frame_ror(m, 1, 0, ops);
    m->ready = 1;
    return 1;
}

void aoe_mouse_destroy_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops) {
    if (m->shape) {
        ops->shape_free(ops->context, m->shape);
        m->shape = NULL;
    }
    aoe_mouse_free_areas_ror(m, ops);
    aoe_mouse_global_ror = NULL;
}

int aoe_mouse_detach_ror(AoeMousePtrRor *m) {
    if (m->detached) return 0;
    if (!m->enabled) return 1;
    if (!m->ready) return 0;
    aoe_mouse_forget_ror(m);
    m->host = NULL;
    m->saved_surface = m->draw_surface = m->primary_surface = NULL;
    m->primary_area = NULL;
    m->visible = 0;
    m->detached = 1;
    return 1;
}

int aoe_mouse_attach_ror(AoeMousePtrRor *m, AoeMouseHostRor *host, const AoeMouseOpsRor *ops) {
    if (!m->detached) return 0;
    if (m->enabled) {
        if (!m->ready) return 0;
        m->host = host;
        m->draw_surface = host->surface;
        m->primary_surface = host->primary_surface;
        m->primary_area = host->primary_area;
        if (!aoe_mouse_load_ror(m, NULL, -1, 1, ops)) return 0;
        m->detached = 0;
    }
    return 1;
}

void aoe_mouse_set_region_ror(AoeMousePtrRor *m, int32_t l, int32_t t, int32_t r, int32_t b) {
    m->region.left = l;
    m->region.top = t;
    m->region.right = r;
    m->region.bottom = b;
}

void aoe_mouse_set_region_on_ror(AoeMousePtrRor *m, int on) { m->region_on = on; }
int aoe_mouse_region_on_ror(const AoeMousePtrRor *m) { return m->region_on; }
int aoe_mouse_in_game_ror(const AoeMousePtrRor *m) { return m->region_set && m->region_on; }

int aoe_mouse_update_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops) {
    int32_t x, y, w, h, right, bottom;
    int in;
    if (!m->enabled) return 0;
    ops->cursor_pos(ops->context, &x, &y);
    if (ops->windowed(ops->context)) {
        if (x > 0x8000) x -= 0x10000;
        if (y > 0x8000) y -= 0x10000;
    }
    m->x = x;
    m->y = y;
    in = m->region_set && m->region_on && x >= m->region.left && x < m->region.right &&
         y >= m->region.top && y < m->region.bottom;
    m->frame = in ? m->game_frame : m->normal_frame;
    if (m->saved && x == m->last_x && y == m->last_y && m->frame == m->drawn_frame) return 0;
    m->last_x = x;
    m->last_y = y;
    m->src.left = m->src.top = 0;
    if (in) {
        m->src.right = m->game_w;
        m->src.bottom = m->game_h;
        m->dest.left = x - m->game_hot_x;
        m->dest.top = y - m->game_hot_y;
        m->dest.right = m->game_right + x;
        m->dest.bottom = m->game_below + y;
    } else {
        m->src.right = m->normal_w;
        m->src.bottom = m->normal_h;
        m->dest.left = x - m->normal_hot_x;
        m->dest.top = y - m->normal_hot_y;
        m->dest.right = m->normal_right + x;
        m->dest.bottom = m->normal_below + y;
    }
    m->clip = m->src;
    right = m->dest.right;
    bottom = m->dest.bottom;
    w = m->host->width;
    h = m->host->height;
    if (right < 0 || bottom < 0 || m->dest.left > w || m->dest.top > h) {
        m->on_screen = 0;
        return 1;
    }
    if (m->dest.left < 0) {
        m->clip.left = -m->dest.left;
        m->src.right += m->dest.left;
        m->dest.left = 0;
    }
    if (right > w) {
        m->clip.right -= right - w;
        m->src.right -= right - w;
        m->dest.right = w;
    }
    if (m->dest.top < 0) {
        m->clip.top = -m->dest.top;
        m->src.bottom += m->dest.top;
        m->dest.top = 0;
    }
    if (bottom > h) {
        m->clip.bottom -= bottom - h;
        m->src.bottom -= bottom - h;
        m->dest.bottom = h;
    }
    m->on_screen = 1;
    return 1;
}

/* Take the clipper off `surface` for the colour-keyed BltFast; 0 on an
   error (kept in +0x148). */
static int unclip(AoeMousePtrRor *m, void *surface, void **clipper, const AoeMouseOpsRor *ops) {
    int32_t hr = ops->get_clipper(ops->context, surface, clipper);
    if (hr == AOE_DDERR_NOCLIPPERATTACHED) {
        *clipper = NULL;
        return 1;
    }
    if (hr) {
        m->error = hr;
        return 0;
    }
    if (*clipper && (hr = ops->set_clipper(ops->context, surface, NULL)) != 0) {
        m->error = hr;
        return 0;
    }
    return 1;
}

static void draw_shape(AoeMousePtrRor *m, void *area, const char *who, const AoeMouseOpsRor *ops) {
    AoeMouseFrameRor f;
    if (!ops->area_lock(ops->context, area, who)) return;
    ops->shape_frame(ops->context, m->shape, m->frame, &f);
    ops->shape_draw(ops->context, m->shape, area, f.hot_x + m->dest.left, m->dest.top + f.hot_y,
                    m->frame);
    ops->area_unlock(ops->context, area, who);
}

static void take_save(AoeMousePtrRor *m, void *surface) {
    m->saved_surface = surface;
    m->save_dest = m->dest;
    m->save_src = m->src;
    m->saved = 1;
}

void aoe_mouse_draw_ror(AoeMousePtrRor *m, int force, const AoeMouseOpsRor *ops) {
    void *clipper = NULL;
    uint32_t t;
    int32_t hr;
    if (!m->visible || !m->enabled) return;
    t = ops->time(ops->context);
    if (m->region_set && m->region_on && !force && t - m->last_time <= 0x21) return;
    if (!aoe_mouse_update_ror(m, ops) && !force) return;
    m->last_time = t;
    if (m->saved && m->saved_surface != m->primary_surface &&
        (hr = ops->blt(ops->context, m->saved_surface, &m->save_dest, save_surface(m, ops),
                       &m->save_src)) != 0) {
        m->error = hr;
        return;
    }
    if (!m->on_screen) {
        m->saved = 0;
    } else {
        if ((hr = ops->blt(ops->context, save_surface(m, ops), &m->src, m->draw_surface, &m->dest)) !=
            0) {
            m->error = hr;
            return;
        }
        take_save(m, m->draw_surface);
        if (m->mode == 0 && !unclip(m, m->draw_surface, &clipper, ops)) return;
        if (m->mode == 1) draw_shape(m, m->host->area, "mouseptr::draw", ops);
        if (m->mode == 0) {
            ops->draw_sync(ops->context);
            hr = ops->blt_fast(ops->context, m->draw_surface, m->dest.left, m->dest.top,
                               ops->area_surface(ops->context, m->cursor_areas[m->frame]), &m->clip);
            if (hr) {
                m->error = hr;
                return;
            }
        }
        m->drawn_frame = m->frame;
        if (m->mode == 0 && clipper &&
            (hr = ops->set_clipper(ops->context, m->draw_surface, clipper)) != 0) {
            m->error = hr;
            return;
        }
    }
    if (!m->region_set || !m->region_on) ops->invalidate(ops->context, &m->dest);
    m->drawn = 1;
}

void aoe_mouse_poll_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops) {
    void *clipper = NULL;
    uint32_t t;
    int32_t hr;
    if (!m->visible || !m->enabled || !m->region_set || !m->region_on) return;
    t = ops->time(ops->context);
    if (t - m->last_time <= 0x21 || !aoe_mouse_update_ror(m, ops)) return;
    m->last_time = t;
    if (m->mode == 0 && !unclip(m, m->primary_surface, &clipper, ops)) return;
    if (m->saved) {
        if ((hr = ops->blt(ops->context, m->saved_surface, &m->save_dest, save_surface(m, ops),
                           &m->save_src)) != 0) {
            m->error = hr;
            return;
        }
        if (m->saved_surface != m->primary_surface &&
            (hr = ops->blt(ops->context, m->primary_surface, &m->save_dest, save_surface(m, ops),
                           &m->save_src)) != 0) {
            m->error = hr;
            return;
        }
    }
    if (!m->on_screen) {
        m->saved = 0;
    } else {
        if ((hr = ops->blt(ops->context, save_surface(m, ops), &m->src, m->primary_surface,
                           &m->dest)) != 0) {
            m->error = aoe_mouse_error_ror(hr);
            return;
        }
        take_save(m, m->primary_surface);
        if (m->mode == 0) {
            ops->draw_sync(ops->context);
            hr = ops->blt_fast(ops->context, m->primary_surface, m->dest.left, m->dest.top,
                               ops->area_surface(ops->context, m->cursor_areas[m->frame]), &m->clip);
            if (hr) {
                m->error = hr;
                return;
            }
        }
        if (m->mode == 1) draw_shape(m, m->primary_area, "mouseptr::poll", ops);
        m->drawn_frame = m->frame;
    }
    if (m->mode == 0 && clipper &&
        (hr = ops->set_clipper(ops->context, m->primary_surface, clipper)) != 0) {
        m->error = hr;
        return;
    }
    m->drawn = 1;
}

void aoe_mouse_restore_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops) {
    int32_t hr;
    if (!m->visible || !m->enabled || !m->drawn || !m->saved ||
        m->saved_surface == m->primary_surface)
        return;
    hr = ops->blt(ops->context, m->saved_surface, &m->save_dest, save_surface(m, ops), &m->save_src);
    if (hr) {
        m->error = hr;
        return;
    }
    if (m->region_set && m->region_on) {
        m->saved = 1;
        m->saved_surface = m->primary_surface;
        return;
    }
    ops->invalidate(ops->context, &m->save_dest);
    m->saved = 0;
}

void aoe_mouse_hide_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops) {
    if (m->detached) return;
    if (m->enabled && m->drawn && m->saved) {
        aoe_mouse_restore_ror(m, ops);
        ops->mark_dirty(ops->context, &m->save_dest);
    }
    ops->set_cursor(ops->context, 0);
    m->visible = 0;
    aoe_mouse_forget_ror(m);
}

void aoe_mouse_show_ror(AoeMousePtrRor *m, const AoeMouseOpsRor *ops) {
    if (m->detached) return;
    if (!m->enabled) ops->set_cursor(ops->context, 1);
    m->visible = 1;
}

void aoe_mouse_center_ror(const AoeMouseOpsRor *ops) {
    AoeMouseRectRor r;
    int32_t w, h;
    if (ops->windowed(ops->context)) {
        ops->window_rect(ops->context, &r);
        ops->set_cursor_pos(ops->context, (r.right - r.left) / 2 + r.left,
                            (r.bottom - r.top) / 2 + r.top);
        return;
    }
    ops->screen_size(ops->context, &w, &h);
    ops->set_cursor_pos(ops->context, w / 2, h / 2);
}

int32_t aoe_mouse_error_ror(int32_t hr) {
    static const struct {
        uint32_t hr;
        int32_t code;
    } table[] = {
        {0x00000000u, 0},  {0x80004005u, 1},  {0x8876006eu, 2},  {0x88760082u, 3},
        {0x80070057u, 4},  {0x88760096u, 5},  {0x887600b4u, 6},  {0x8876023fu, 7},
        {0x887600cdu, 8},  {0x88760240u, 9},  {0x887600fau, 10}, {0x88760118u, 11},
        {0x88760122u, 12}, {0x88760136u, 13}, {0x88760154u, 14}, {0x887601aeu, 15},
        {0x887601c2u, 16}, {0x80004001u, 17}, {0x88760037u, 18},
    };
    size_t i;
    for (i = 0; i < sizeof table / sizeof table[0]; ++i)
        if ((uint32_t)hr == table[i].hr) return table[i].code;
    return -1;
}
