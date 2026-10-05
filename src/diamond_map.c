#include "diamond_map.h"

#include <stdlib.h>
#include <string.h>

#include "draw_prims.h"

#define DM(p) ((AoeDiamondMapRor *)(p))
#define VT(m) ((const AoeDiamondMapVtblRor *)(m)->panel.vt)

static int32_t ftol(float v) { return (int32_t)v; }

void aoe_diamond_map_init_ror(AoeDiamondMapRor *m, AoeUiSystemRor *system,
                              const AoeDiamondMapOpsRor *ops) {
    aoe_ui_panel_init_ror(&m->panel, system);
    m->panel.vt = &aoe_diamond_map_vtable_ror()->base;
    m->ops = ops;
    m->world = NULL;
    m->player = NULL;
    m->has_map = 0;
    m->mouse_x = m->mouse_y = 0;
    m->rows = NULL;
    m->diagonals = NULL;
    m->map_w = m->map_h = 0;
    m->first_row = 0;
    m->widest = 0;
    m->row_count = 0;
    m->diagonal_count = 0;
    m->diagonals_per_row = 0;
    m->pixels_per_diagonal = 0;
    m->rows_per_diagonal = 0;
    m->row_height = 0;
    m->mean_pixels = 0;
    m->buffer = NULL;
    m->owns_buffer = 0;
    memset(m->frame, 0, sizeof m->frame);
    m->main_view = NULL;
    m->dragging = 0;
    m->phase = 0;
    m->player_mask = 0;
    m->frame_x = m->frame_y = -1;
    m->full_redraw = 1;
    m->select_color = 0xff;
}

void aoe_diamond_map_free_buffer_ror(AoeDiamondMapRor *m) {
    if (m->buffer && m->owns_buffer) {
        aoe_surface_free_ror(m->buffer);
        free(m->buffer);
        m->buffer = NULL;
        m->owns_buffer = 0;
    }
}

void aoe_diamond_map_release_ror(AoeDiamondMapRor *m) {
    aoe_diamond_map_free_buffer_ror(m);
    free(m->diagonals);
    m->diagonals = NULL;
    free(m->rows);
    m->rows = NULL;
    aoe_ui_panel_release_ror(&m->panel);
}

static void destroy(AoeUiPanelRor *p) { aoe_diamond_map_release_ror(DM(p)); }

int aoe_diamond_map_setup_ror(AoeDiamondMapRor *m, AoeUiAreaRor *area,
                              AoeUiPanelRor *parent, int32_t x, int32_t y,
                              int32_t w, int32_t h, int32_t color,
                              AoeDrawSurfaceRor *buffer) {
    m->buffer = buffer;
    m->owns_buffer = 0;
    aoe_ui_panel_setup_ror(&m->panel, area, parent, x, y, w, h, (uint8_t)color);
    return 1;
}

int aoe_diamond_map_load_ror(AoeDiamondMapRor *m, const char *name, int32_t id) {
    int32_t w = 0, h = 0, y;
    uint8_t *bits;
    free(m->rows);
    m->rows = NULL;
    free(m->diagonals);
    m->diagonals = NULL;
    bits = m->ops && m->ops->load_bitmap
               ? m->ops->load_bitmap(m->ops->context, name, id, &w, &h)
               : NULL;
    if (!bits) return 0;
    m->rows = calloc(h > 0 ? (size_t)h : 1, sizeof *m->rows);
    if (!m->rows) {
        free(bits);
        return 0;
    }
    m->first_row = -1;
    m->widest = 0;
    m->row_count = 0;
    for (y = 0; y < (int16_t)h; ++y) {
        const uint8_t *row = bits + (size_t)y * (size_t)w;
        int16_t x;
        for (x = 0; x < (int16_t)w; ++x) {
            if (row[x] == 0xff) {
                int16_t run = 0, k = x;
                while (k < (int16_t)w && row[k] == 0xff) {
                    ++run;
                    ++k;
                }
                m->rows[m->row_count].x = x;
                m->rows[m->row_count].width = run;
                ++m->row_count;
                if (m->widest < run) m->widest = run;
                if (m->first_row == -1) m->first_row = (int16_t)y;
                break;
            }
        }
    }
    free(bits);
    if (m->world) aoe_diamond_map_set_world_ror(m, m->world);
    return 1;
}

void aoe_diamond_map_set_world_ror(AoeDiamondMapRor *m, const void *world) {
    const AoeDiamondMapOpsRor *ops = m->ops;
    float position = 0.0f, accumulated = 0.0f, half = 0.0f;
    int32_t sum = 0;
    int16_t i, last;
    m->world = world;
    if (!world || !m->rows) {
        m->has_map = 0;
        return;
    }
    m->has_map = 1;
    m->map_w = (int16_t)ops->map_width(ops->context);
    m->map_h = (int16_t)ops->map_height(ops->context);
    m->diagonal_count = (int16_t)(m->map_w + m->map_h - 1);
    free(m->diagonals);
    m->diagonals = calloc(m->diagonal_count > 0 ? (size_t)m->diagonal_count : 1,
                          sizeof *m->diagonals);
    if (!m->diagonals) return;
    m->diagonals_per_row = (float)m->diagonal_count / (float)m->row_count;
    if (m->diagonal_count > m->row_count) half = m->diagonals_per_row * 0.5f;
    position = half;
    for (i = 0; i < m->row_count; ++i) {
        AoeDiamondRowRor *row = &m->rows[i];
        AoeDiamondDiagonalRor *d;
        int16_t width = row->width, n, ceil_ppt;
        int16_t di = (int16_t)ftol(position);
        float tpp, ppt;
        if (di >= m->diagonal_count) di = (int16_t)(m->diagonal_count - 1);
        row->diagonal = di;
        if (di < m->map_w) {
            n = m->map_h;
            if (di < n) n = (int16_t)(di + 1);
        } else {
            n = (int16_t)(m->map_w - di + m->map_h - 1);
        }
        tpp = (float)n / (float)width;
        ppt = 1.0f / tpp;
        ceil_ppt = (int16_t)ftol(ppt);
        if ((float)ceil_ppt != ppt) ++ceil_ppt;
        d = &m->diagonals[di];
        d->row = i;
        d->valid = 1;
        d->tiles = n;
        d->tiles_per_pixel = tpp;
        d->pixels_ceil = ceil_ppt;
        d->pixels_per_tile = ppt;
        d->pixels_trunc = (int16_t)ftol((float)width / ppt);
        d->width_per_tile = (float)width / (float)n;
        d->half = d->width_per_tile < 1.0f ? tpp * 0.5f : 0.0f;
        if (di < m->map_w) {
            d->first_y = 0;
            d->first_x = (int16_t)(m->map_w - di - 1);
        } else {
            d->first_y = (int16_t)(di - m->map_w + 1);
            d->first_x = 0;
        }
        sum += ceil_ppt;
        accumulated += d->width_per_tile;
        position += m->diagonals_per_row;
    }
    {
        float inv = 1.0f / m->diagonals_per_row;
        m->row_height = (int16_t)ftol(inv);
        if ((float)m->row_height != inv) ++m->row_height;
    }
    m->mean_pixels = m->row_count ? (int16_t)((int16_t)sum / m->row_count) : 0;
    if (m->mean_pixels == 0) m->mean_pixels = 1;
    m->pixels_per_diagonal = accumulated / (float)m->row_count;
    m->rows_per_diagonal = (float)m->row_count / (float)m->diagonal_count;
    /* Diagonals no row landed on copy the last that did (not their first
       tile). */
    for (i = 0, last = 0; i < m->diagonal_count; ++i) {
        AoeDiamondDiagonalRor *d = &m->diagonals[i], *s = &m->diagonals[last];
        if (d->valid) {
            last = i;
            continue;
        }
        d->row = s->row;
        d->tiles = s->tiles;
        d->tiles_per_pixel = s->tiles_per_pixel;
        d->pixels_ceil = s->pixels_ceil;
        d->pixels_per_tile = s->pixels_per_tile;
        d->pixels_trunc = s->pixels_trunc;
        d->width_per_tile = s->width_per_tile;
        d->half = s->half;
    }
    m->frame_x = -1;
    m->frame_y = -1;
    m->panel.vt->set_redraw(&m->panel, 2);
}

void aoe_diamond_map_set_player_ror(AoeDiamondMapRor *m, const void *player) {
    m->player = player;
    m->panel.vt->set_redraw(&m->panel, 2);
}

void aoe_diamond_map_set_redraw_ror(AoeUiPanelRor *p, int32_t redraw) {
    aoe_ui_panel_set_redraw_ror(p, redraw);
    if (redraw == 2) DM(p)->full_redraw = 1;
}

int aoe_diamond_map_make_buffer_ror(AoeDiamondMapRor *m) {
    AoeUiPanelRor *p = &m->panel;
    if (!m->buffer) {
        AoeDrawSystemRor *d = p->area && p->area->surface ? p->area->surface->system : NULL;
        m->buffer = calloc(1, sizeof *m->buffer);
        if (!m->buffer) return 0;
        if (!aoe_surface_init_ror(m->buffer, "Diamond map (restored)", 0) ||
            !aoe_surface_attach_ror(m->buffer, d, p->width, p->height, 0, 0)) {
            aoe_surface_free_ror(m->buffer);
            free(m->buffer);
            m->buffer = NULL;
            return 0;
        }
        m->owns_buffer = 1;
    } else if (m->owns_buffer) {
        aoe_surface_allocate_ror(m->buffer, p->width, p->height, 0);
    }
    return 1;
}

/* Row y of a locked surface. */
static uint8_t *row_bits(const AoeShapeSurfaceRor *v, int32_t y) {
    return v->direction == 1 ? v->bits + (ptrdiff_t)y * v->pitch
                             : v->bits + (ptrdiff_t)(v->height - y - 1) * v->pitch;
}

void aoe_diamond_map_clear_image_ror(AoeDiamondMapRor *m) {
    AoeUiPanelRor *p = &m->panel;
    AoeShapeSurfaceRor *v;
    int16_t i;
    if (!m->buffer || !aoe_surface_lock_ror(m->buffer, 1)) return;
    v = &m->buffer->view;
    for (i = 0; i < m->row_count; ++i) {
        int32_t y = (int16_t)(p->y + m->first_row) + i;
        if (p->clip.top <= y && y <= p->clip.bottom) {
            int16_t x = m->rows[i].x;
            int16_t left = (int16_t)(p->x + x);
            int16_t right = (int16_t)(m->rows[i].width + left - 1);
            int32_t l = left;
            if (left < p->clip.left) {
                x = (int16_t)(x + (int16_t)p->clip.left - left);
                l = p->clip.left;
            }
            if (p->clip.right < right) right = (int16_t)p->clip.right;
            if (p->clip.left <= (int16_t)l && right <= p->clip.right)
                memset(row_bits(v, m->first_row + i) + x, p->color,
                       (size_t)(right - (int16_t)l + 1));
        }
    }
    aoe_surface_unlock_ror(m->buffer);
}

void aoe_diamond_map_blit_ror(AoeDiamondMapRor *m) {
    AoeUiPanelRor *p = &m->panel;
    int32_t r[4] = {0, 0, p->width - 1, p->height - 1};
    if (m->buffer && p->area && p->area->surface)
        aoe_surface_blit_ror(m->buffer, p->area->surface, p->x, p->y, r, 0);
}

void aoe_diamond_map_draw_tile_ror(AoeDiamondMapRor *m, int16_t x, int16_t y,
                                   int32_t mode, uint8_t color) {
    const AoeDiamondMapOpsRor *ops = m->ops;
    AoeDiamondDiagonalRor *d = &m->diagonals[m->map_w - x - 1 + y];
    AoeDrawSurfaceRor *s;
    int16_t m_xy, k, n, r;
    int32_t px, py, i;
    if (!d->valid) return;
    if (!ops->all_visible(ops->context) &&
        !(ops->explored_mask(ops->context, m->player) & ops->visibility(ops->context, x, y)))
        return;
    m_xy = x < y ? x : y;
    if (d->width_per_tile < 1.0f) {
        float v = (float)m_xy * d->width_per_tile;
        k = (int16_t)ftol(v);
        if (!(v - (float)k < d->width_per_tile)) return;
    } else {
        k = (int16_t)ftol((float)m_xy * d->pixels_per_tile);
    }
    py = m->first_row + d->row;
    px = m->rows[d->row].x + k;
    n = d->pixels_ceil;
    if (mode == 0) {
        int16_t ty, tx;
        if (color == 0) {
            uint8_t terrain = (uint8_t)(ops->tile_terrain(ops->context, x, y) & 0x1f);
            switch (ops->tile_shape(ops->context, x, y)) {
            case 0: color = ops->terrain_color(ops->context, terrain, 1); break;
            case 1: case 4: case 5: case 7: case 8: case 9: case 0xc: case 0xd: case 0xf:
                color = ops->terrain_color(ops->context, terrain, 2);
                break;
            case 2: case 3: case 6: case 0xa: case 0xb: case 0xe: case 0x10:
                color = ops->terrain_color(ops->context, terrain, 0);
                break;
            default: color = 0; break;
            }
        }
        /* A neighbour with a mode-4 object colours the tile (the last
           found wins). */
        for (ty = (int16_t)(y - 1); ty < y + 2; ++ty) {
            if (ty < 0 || ty >= m->map_h) continue;
            for (tx = (int16_t)(x - 1); tx < x + 2; ++tx) {
                uint8_t c;
                if (tx < 0 || tx >= m->map_w) continue;
                if (ops->tile_feature && ops->tile_feature(ops->context, tx, ty, &c)) color = c;
            }
        }
        s = m->buffer;
    } else {
        px += m->panel.x;
        py += m->panel.y;
        color = m->select_color;
        s = m->panel.area ? m->panel.area->surface : NULL;
    }
    if (!s) return;
    if (m->row_height == 1) {
        for (i = 0; i < n; ++i) aoe_surface_plot_ror(s, px + i, py, color);
    } else {
        for (r = 0; r < m->row_height; ++r)
            for (i = 0; i < n; ++i) aoe_surface_plot_ror(s, px + i, py + r, color);
    }
}

void aoe_diamond_map_draw_tiles_ror(AoeDiamondMapRor *m) {
    int16_t x, y;
    if (!m->buffer || !aoe_surface_lock_ror(m->buffer, 1)) return;
    for (y = 0; y < m->map_h; ++y)
        for (x = 0; x < m->map_w; ++x) aoe_diamond_map_draw_tile_ror(m, x, y, 0, 0);
    aoe_surface_unlock_ror(m->buffer);
}

void aoe_diamond_map_draw_explored_ror(AoeDiamondMapRor *m) {
    const int16_t *xy = NULL;
    int32_t n = m->ops->explored_tiles
                    ? m->ops->explored_tiles(m->ops->context, m->player, &xy)
                    : 0, i;
    if (n <= 0 || !m->buffer || !aoe_surface_lock_ror(m->buffer, 1)) return;
    for (i = 0; i < n; ++i) aoe_diamond_map_draw_tile_ror(m, xy[i * 4], xy[i * 4 + 2], 0, 0);
    aoe_surface_unlock_ror(m->buffer);
}

void aoe_diamond_map_draw_ror(AoeUiPanelRor *p) {
    AoeDiamondMapRor *m = DM(p);
    const AoeDiamondMapOpsRor *ops = m->ops;
    if (!p->area || !m->buffer || !p->visible || !p->active) {
        p->redraw = 0;
        return;
    }
    if (m->has_map && m->player && m->rows && m->diagonals) {
        AoeDrawSurfaceRor *area = p->area->surface;
        p->vt->begin_draw(p, 0);
        m->player_mask = 1u << (ops->viewed_index(ops->context) & 0x1f);
        if (!m->full_redraw) {
            aoe_diamond_map_draw_explored_ror(m);
        } else {
            if (!p->parent) {
                aoe_surface_fill_ror(m->buffer, NULL, p->color);
            } else {
                int32_t r[4] = {p->rect.left, p->rect.top, p->rect.right, p->rect.bottom};
                p->parent->vt->draw_rect(p->parent, &p->clip);
                if (area) aoe_surface_blit_ror(area, m->buffer, 0, 0, r, 0);
                aoe_diamond_map_clear_image_ror(m);
            }
            aoe_diamond_map_draw_tiles_ror(m);
            m->full_redraw = 0;
        }
        aoe_diamond_map_blit_ror(m);
        if (ops->edit_mode(ops->context) != 0x15) VT(m)->draw_selected_area(m);
        VT(m)->draw_objects(m);
        if (area && aoe_surface_lock_ror(area, 1)) {
            VT(m)->draw_view_frame(m);
            aoe_surface_unlock_ror(area);
        }
        p->vt->end_draw(p);
        if (ops->clear_explored) ops->clear_explored(ops->context, m->player);
        return;
    }
    p->vt->begin_draw(p, 0);
    if (m->rows) {
        aoe_diamond_map_clear_image_ror(m);
        aoe_diamond_map_blit_ror(m);
    } else if (p->parent) {
        p->parent->vt->draw_rect(p->parent, &p->clip);
    } else if (p->area->surface) {
        int32_t r[4] = {p->clip.left, p->clip.top, p->clip.right, p->clip.bottom};
        aoe_surface_fill_ror(p->area->surface, r, p->color);
    }
    p->vt->end_draw(p);
}

void aoe_diamond_map_draw_selected_area_ror(AoeDiamondMapRor *m) {
    int32_t x0, y0, x1, y1, x, y;
    AoeDrawSurfaceRor *area = m->panel.area ? m->panel.area->surface : NULL;
    if (!m->main_view || !m->ops->view_tiles ||
        !m->ops->view_tiles(m->ops->context, &x0, &y0, &x1, &y1) || !area ||
        !aoe_surface_lock_ror(area, 1))
        return;
    for (y = y0; y <= y1; ++y)
        for (x = x0; x <= x1; ++x)
            aoe_diamond_map_draw_tile_ror(m, (int16_t)x, (int16_t)y, 1, 0);
    aoe_surface_unlock_ror(area);
}

void aoe_diamond_map_draw_objects_ror(AoeDiamondMapRor *m) {
    const AoeDiamondMapOpsRor *ops = m->ops;
    AoeDrawSurfaceRor *area = m->panel.area ? m->panel.area->surface : NULL;
    void **list;
    int32_t n, i, pl, count;
    if (!area || !aoe_surface_lock_ror(area, 1)) return;
    m->phase = 1;
    n = ops->objects(ops->context, ops->world_player(ops->context, 0), 0, &list);
    for (i = 0; i < n; ++i)
        if (list[i]) VT(m)->draw_object(m, list[i], 0xff, 0xff);
    m->phase = 2;
    count = ops->player_count(ops->context);
    for (pl = 1; pl < count; ++pl) {
        const void *player = ops->world_player(ops->context, pl);
        uint8_t c = ops->player_map_color(ops->context, player);
        n = ops->objects(ops->context, player, 1, &list);
        for (i = 0; i < n; ++i)
            if (list[i]) VT(m)->draw_object(m, list[i], c, c);
        n = ops->objects(ops->context, player, 0, &list);
        for (i = 0; i < n; ++i) {
            AoeDiamondObjectRor o;
            if (!list[i] || !ops->read_object(ops->context, list[i], &o)) continue;
            if (!(o.flags_36 & 1) && (o.owner != m->player || !o.drawn_last))
                VT(m)->draw_object(m, list[i], c, c);
        }
    }
    {
        void *selected[25];
        int32_t k = ops->selection ? ops->selection(ops->context, m->player, selected, 25) : 0;
        if (k > 0) {
            m->phase = 3;
            for (i = 0; i < k; ++i)
                if (selected[i]) VT(m)->draw_object(m, selected[i], 0xff, 0xff);
        }
    }
    m->phase = 4;
    {
        uint8_t c = ops->player_map_color(ops->context, m->player);
        n = ops->objects(ops->context, m->player, 0, &list);
        for (i = 0; i < n; ++i) {
            AoeDiamondObjectRor o;
            if (list[i] && ops->read_object(ops->context, list[i], &o) && o.drawn_last)
                VT(m)->draw_object(m, list[i], c, c);
        }
    }
    aoe_surface_unlock_ror(area);
}

void aoe_diamond_map_draw_object_ror(AoeDiamondMapRor *m, void *object,
                                     uint8_t color, uint8_t small_color) {
    const AoeDiamondMapOpsRor *ops = m->ops;
    AoeDiamondObjectRor o;
    int16_t tx, ty;
    int v;
    if (!ops->read_object(ops->context, object, &o) || !o.has_sprite) return;
    tx = (int16_t)ftol(o.x);
    ty = (int16_t)ftol(o.y);
    if (!ops->fog(ops->context, m->player)) {
        v = 0xf;
    } else if (!ops->all_visible(ops->context) || ops->keep_explored(ops->context)) {
        uint32_t bits = ops->visibility(ops->context, tx, ty);
        v = 0;
        if (ops->explored_mask(ops->context, m->player) & bits) v = 0x80;
        if (ops->visible_mask(ops->context, m->player) & bits) v = 0xf;
        if (ops->all_visible(ops->context) && v == 0) v = 0x80;
        if (!ops->keep_explored(ops->context) && v == 0x80) {
            v = 0xf;
        } else if (v != 0xf) {
            if (v != 0x80 || !o.fog_visible) return;
        }
    } else {
        v = 0xf;
    }
    if (o.type == 0x19) {
        if (v == 0xf) return;
        if (ops->shared_vision(ops->context, m->player, o.owner_number) == 1 &&
            !(o.doppel_mask & m->player_mask)) {
            v = o.doppel_mode;
            color = small_color = o.doppel_color;
        } else {
            color = small_color = 0xff;
            v = 0;
        }
    } else {
        v = o.map_mode;
        if (v == 3) color = o.map_color;
    }
    if (v == 1)
        VT(m)->draw_dot(m, tx, ty, small_color, 1, object);
    else if (v == 2 || v == 3)
        VT(m)->draw_dot(m, tx, ty, color, 2, object);
}

void aoe_diamond_map_draw_dot_ror(AoeDiamondMapRor *m, int16_t x, int16_t y,
                                  uint8_t color, int16_t shape, void *object) {
    AoeDiamondDiagonalRor *d = &m->diagonals[m->map_w - x + y - 1];
    AoeShapeSurfaceRor *v;
    int16_t mn = x < y ? x : y, k, px, py;
    (void)object;
    if (!m->panel.area || !m->panel.area->surface) return;
    v = &m->panel.area->surface->view;
    k = (int16_t)ftol((float)mn * (d->width_per_tile < 1.0f ? d->width_per_tile
                                                           : d->pixels_per_tile));
    px = (int16_t)(m->rows[d->row].x + (int16_t)m->panel.x + k);
    py = (int16_t)(m->first_row + (int16_t)m->panel.y + d->row);
    switch (shape) {
    case 1:
        aoe_draw_fill_ror(v, px - 1, py - 1, px + 1, py + 1, color);
        break;
    case 2:
        aoe_draw_fill_ror(v, px, py - 1, px, py + 1, color);
        aoe_draw_fill_ror(v, px - 1, py, px + 1, py, color);
        break;
    case 3:
        aoe_draw_fill_ror(v, px - 3, py, px + 3, py + 1, color);
        aoe_draw_fill_ror(v, px - 2, py - 1, px + 2, py + 2, color);
        break;
    case 4:
        aoe_draw_fill_ror(v, px - 2, py - 2, px + 2, py + 2, color);
        break;
    default:
        break;
    }
}

void aoe_diamond_map_draw_view_frame_ror(AoeDiamondMapRor *m) {
    const AoeDiamondMapOpsRor *ops = m->ops;
    AoeUiPanelRor *p = &m->panel;
    AoeDrawSurfaceRor *area = p->area ? p->area->surface : NULL;
    float cx = 0, cy = 0;
    int16_t tx, ty;
    int32_t y;
    if (!area) return;
    ops->view_center(ops->context, m->player, &cx, &cy);
    tx = (int16_t)ftol(cx);
    ty = (int16_t)ftol(cy);
    if (tx != m->frame_x || ty != m->frame_y) {
        AoeDiamondDiagonalRor *d;
        int16_t mn, k, px, py, vw = 0, vh = 0;
        int32_t half;
        float fy;
        if (tx < 0 || tx >= ops->map_width(ops->context) || ty < 0 ||
            ty >= ops->map_height(ops->context))
            return;
        d = &m->diagonals[m->map_w - tx + ty - 1];
        mn = tx < ty ? tx : ty;
        k = (int16_t)ftol((float)mn * (d->width_per_tile < 1.0f ? d->width_per_tile
                                                               : d->pixels_per_tile));
        py = (int16_t)(m->first_row + (int16_t)p->y + d->row);
        px = (int16_t)(m->rows[d->row].x + (int16_t)p->x + k);
        if (ops->view_size) ops->view_size(ops->context, &vw, &vh);
        half = vw / 2;
        m->frame[0] = ftol((float)px - (float)half * m->pixels_per_diagonal);
        fy = (float)py;
        m->frame[1] = ftol(fy - (float)vh * m->rows_per_diagonal);
        m->frame[2] = ftol((float)(half + 2) * m->pixels_per_diagonal + (float)px);
        m->frame[3] = ftol((float)(vh + 4) * m->rows_per_diagonal + fy);
        m->frame_x = tx;
        m->frame_y = ty;
    }
    for (y = m->frame[1]; y <= m->frame[3]; ++y) {
        int32_t top = m->first_row + p->y;
        AoeDiamondRowRor *r;
        int32_t x0, x1, l = m->frame[0], rt = m->frame[2];
        if (y < top || y > m->row_count + p->y - 1 + m->first_row) continue;
        r = &m->rows[y - p->y - m->first_row];
        x0 = r->x + p->x;
        x1 = r->width + p->x + r->x;
        if (y == m->frame[1] || y == m->frame[3]) {
            if (l < x1 && x0 <= rt) {
                if (l < x0) l = x0;
                if (x1 <= rt) rt = x1 - 1;
                aoe_draw_hline_ror(&area->view, l, y, rt - l + 1, 0xff);
            }
        } else {
            if (l < x1 && x0 <= l) aoe_surface_plot_ror(area, l, y, 0xff);
            if (rt < x1 && x0 <= rt) aoe_surface_plot_ror(area, rt, y, 0xff);
        }
    }
}

int aoe_diamond_map_tile_at_ror(const AoeDiamondMapRor *m, int32_t x, int32_t y,
                                int16_t *tx, int16_t *ty) {
    const AoeDiamondRowRor *r;
    const AoeDiamondDiagonalRor *d;
    int16_t k;
    if (!m->has_map || !m->rows) return 0;
    y -= m->panel.y;
    x -= m->panel.x;
    if (y < m->first_row || y > m->row_count - 1 + m->first_row) return 0;
    r = &m->rows[y - m->first_row];
    if (x < r->x || x > r->width - 1 + r->x) return 0;
    d = &m->diagonals[r->diagonal];
    k = (int16_t)ftol((float)(x - r->x) * d->tiles_per_pixel + d->half);
    if (tx) *tx = (int16_t)(d->first_x + k);
    if (ty) *ty = (int16_t)(d->first_y + k);
    return 1;
}

static void redraw_both(AoeDiamondMapRor *m) {
    m->panel.vt->set_redraw(&m->panel, 1);
    if (m->main_view) m->main_view->vt->set_redraw(m->main_view, 1);
}

void aoe_diamond_map_press_ror(AoeDiamondMapRor *m, int32_t x, int32_t y) {
    int16_t tx, ty;
    if (!aoe_diamond_map_tile_at_ror(m, x, y, &tx, &ty)) return;
    aoe_ui_panel_capture_ror(&m->panel);
    m->dragging = 1;
    m->ops->set_view_center(m->ops->context, m->player, (float)tx + 0.5f, (float)ty + 0.5f);
    redraw_both(m);
    m->mouse_x = x;
    m->mouse_y = y;
}

void aoe_diamond_map_drag_ror(AoeDiamondMapRor *m, int32_t x, int32_t y) {
    int16_t tx, ty;
    if (!aoe_diamond_map_tile_at_ror(m, x, y, &tx, &ty)) return;
    m->ops->set_view_center(m->ops->context, m->player, (float)tx + 0.5f, (float)ty + 0.5f);
    redraw_both(m);
    m->mouse_x = x;
    m->mouse_y = y;
}

void aoe_diamond_map_release_drag_ror(AoeDiamondMapRor *m) {
    m->dragging = 0;
    aoe_ui_panel_release_capture_ror(&m->panel);
}

int32_t aoe_diamond_map_size_ror(AoeUiPanelRor *p, int32_t w, int32_t h) {
    AoeDiamondMapRor *m = DM(p);
    aoe_ui_panel_handle_size_ror(p, w, h);
    m->frame_x = -1;
    m->frame_y = -1;
    aoe_diamond_map_make_buffer_ror(m);
    return 0;
}

int32_t aoe_diamond_map_inside_ror(AoeUiPanelRor *p, int32_t x, int32_t y) {
    if (!aoe_ui_panel_is_inside_ror(p, x, y)) return 0;
    return aoe_diamond_map_tile_at_ror(DM(p), x, y, NULL, NULL);
}

static AoeDiamondMapVtblRor vtable;
static int vtable_ready;

const AoeDiamondMapVtblRor *aoe_diamond_map_vtable_ror(void) {
    if (!vtable_ready) {
        vtable.base = aoe_ui_panel_vtable_ror;
        vtable.base.destroy = destroy;
        vtable.base.set_redraw = aoe_diamond_map_set_redraw_ror;
        vtable.base.draw = aoe_diamond_map_draw_ror;
        vtable.base.handle_size = aoe_diamond_map_size_ror;
        vtable.base.is_inside = aoe_diamond_map_inside_ror;
        vtable.draw_selected_area = aoe_diamond_map_draw_selected_area_ror;
        vtable.draw_objects = aoe_diamond_map_draw_objects_ror;
        vtable.draw_object = aoe_diamond_map_draw_object_ror;
        vtable.draw_dot = aoe_diamond_map_draw_dot_ror;
        vtable.draw_view_frame = aoe_diamond_map_draw_view_frame_ror;
        vtable_ready = 1;
    }
    return &vtable;
}
