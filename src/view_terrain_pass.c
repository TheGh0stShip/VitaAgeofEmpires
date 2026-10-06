/* Patched RoR 1.0a game view terrain pass 0x00512400 and tile drawer
   0x00513ea0. */
#include "view_terrain_pass.h"

#include <stddef.h>

#define STEP(id, a, b, c) o->step(o->context, (id), (int32_t)(a), (int32_t)(b), (int32_t)(c))

static int inside(const int32_t r[4], int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    /* The original's four comparisons: left <= x1, x0 <= right, top <= y1, y0 <= bottom. */
    return !(r[2] < x0 || x1 < r[0] || r[3] < y0 || y1 < r[1]);
}

static void add_outlines(const AoeViewTerrainPassOpsRor *o, const void *a, const void *b,
                         const void *edge, int use_a, int use_b, int use_edge, int32_t sx,
                         int32_t sy) {
    if (use_a) o->region_add(o->context, a, sx, sy);
    if (use_b) o->region_add(o->context, b, sx, sy);
    if (use_edge) {
        if (use_a) o->region_remove(o->context, edge, sx, sy);
        if (use_b) o->region_remove(o->context, edge, sx, sy);
    }
}

static void marks(AoeViewTerrainPassRor *v, const AoeViewTerrainPassOpsRor *o) {
    AoeViewMarkRor *m, *next;
    uint32_t now;
    int any_done = 0;
    if (!v->marks) return;
    now = o->time_ms(o->context);
    for (m = v->marks; m; m = next) {
        int done = 0;
        next = m->next;
        if (m->shape == 0) continue;
        if (m->frame == 0) {
            m->frame = (int32_t)now; /* +0x24: the start time */
        } else if ((m->kind == 1 || m->kind == 2) && now - (uint32_t)m->frame >= (uint32_t)m->interval) {
            uint32_t elapsed = now - (uint32_t)m->frame;
            do {
                ++m->field_c; /* +0x0c: the frame */
                if (o->frame_count(o->context, m->shape) <= m->field_c) {
                    m->field_c = 0;
                    if (m->kind == 2) done = any_done = 1;
                }
                elapsed -= (uint32_t)m->interval;
                m->frame += m->interval;
            } while ((uint32_t)m->interval <= elapsed);
        }
        if (!done) {
            o->draw_mark(o->context, m);
            continue;
        }
        if (m->prev) m->prev->next = m->next;
        if (m->next) m->next->prev = m->prev;
        if (!m->prev) v->marks = m->next;
        o->free_mark(o->context, m);
    }
    if (any_done) STEP(AOE_VTP_MARKS_DROP, 0, 0, 0);
}

static void hover(AoeViewTerrainPassRor *v, const AoeViewTerrainPassOpsRor *o, int32_t dx,
                  int32_t dy) {
    int32_t x, y, id, frame = 0, string = 0;
    uint32_t now;
    if (o->game_state(o->context) != 4 || !o->has_area(o->context) || !o->mouse_in_game(o->context))
        return;
    now = o->time_ms(o->context);
    if (!o->cursor_in_view(o->context, -dx, -dy, &x, &y)) return;
    if (o->pixel_object(o->context, x, y, &id) == 1) {
        if (v->hover_on && v->hover_id == id) return;
        v->hover_on = 1;
        if (v->hover_id == id) return;
        v->hover_time = now;
        v->hover_id = id;
        if (o->object_cursor(o->context, id, &frame, &string) == 1) {
            STEP(AOE_VTP_SET_MOUSE_FRAME, frame, 0, 0);
            if (v->hover_messages && o->messages_enabled(o->context))
                STEP(AOE_VTP_SHOW_MESSAGE, string, 0, 0);
            return;
        }
        STEP(AOE_VTP_SET_MOUSE_FRAME, 0, 0, 0);
    } else {
        if (!v->hover_on) return;
        v->hover_on = 0;
        v->hover_id = INT32_MIN;
        STEP(AOE_VTP_SET_MOUSE_FRAME, 0, 0, 0);
    }
    if (v->hover_messages && o->messages_enabled(o->context)) STEP(AOE_VTP_HIDE_MESSAGE, 0, 0, 0);
}

int aoe_game_view_terrain_pass_ror(AoeViewTerrainPassRor *v, const AoeViewTerrainPassOpsRor *o,
                                   uint8_t mode, int32_t dx, int32_t dy) {
    int32_t sx0 = v->start_x, sy0 = v->start_y, cols = v->cols + 5, rows = v->rows * 2 + 12;
    int all_visible = !v->has_visibility || v->reveal_all;
    int32_t r, c, edit;
    if (v->timers_active > 0) STEP(AOE_VTP_TIMERS, 0, 0, 0);
    v->mode = mode;
    o->terrains_uncounted(o->context);
    STEP(AOE_VTP_ROWS_RESET, 0, 0, 0);
    v->last_region = -1;
    for (r = 0; r < rows; ++r) {
        int32_t tx = sx0, ty = sy0;
        if ((r & 3) == 3 && o->mouse_global(o->context)) STEP(AOE_VTP_MOUSE, 0, 0, 0);
        if ((r & 1) == 1 && o->blits_active(o->context)) STEP(AOE_VTP_STEP_BLITS, 0, 0, 0);
        for (c = 0; c < cols; ++c, ++tx, ++ty) {
            AoeViewTileRor t;
            int32_t sx, sy, right, bottom, region = 0, in_second = 0;
            uint32_t bits, edge_v = 0xffffffff, edge_e = 0xffffffff;
            uint8_t fog;
            int present = 0;
            if (tx < 0 || ty < 0) continue;
            if (ty >= v->map_h || tx >= v->map_w) break;
            o->tile(o->context, tx, ty, &t);
            present = 1;
            sx = t.x - v->origin_x;
            sy = t.y - v->origin_y;
            right = sx + v->tile_w;
            bottom = o->shape_height(o->context, t.shape) + sy;
            bits = o->visibility(o->context, tx, ty);
            if (v->visible_mask & bits) fog = AOE_VIEW_FOG_VISIBLE;
            else if ((v->explored_mask & bits) || all_visible)
                fog = v->fog_edges ? AOE_VIEW_FOG_EXPLORED : AOE_VIEW_FOG_VISIBLE;
            else {
                fog = AOE_VIEW_FOG_UNSEEN;
                t.last_fog = 0;
                o->tile_store(o->context, tx, ty, &t);
            }
            if (fog != AOE_VIEW_FOG_UNSEEN && sx <= v->clip[2] && sy <= v->clip[3] &&
                v->clip[0] < right && v->clip[1] < bottom) {
                uint8_t terrain = t.terrain & 0x1f, draw_id;
                const void *oa, *ob, *oe;
                int use_a, use_b, use_edge, skip = 0;
                int16_t repl;
                if (v->count_terrains && terrain < v->terrain_count)
                    o->terrain_counted(o->context, terrain);
                if (v->incremental && t.last_fog == fog) {
                    int unchanged = 0;
                    if (fog == AOE_VIEW_FOG_VISIBLE) {
                        edge_v = v->fog_edges ? o->neighbor_mask(o->context, tx, ty, v->visible_mask) : 0;
                        if (t.edge_visible == edge_v) {
                            edge_e = all_visible ? 0 : o->neighbor_mask(o->context, tx, ty, v->explored_mask);
                            if (t.edge_explored == edge_e) unchanged = 1;
                        }
                    } else {
                        edge_e = all_visible ? 0 : o->neighbor_mask(o->context, tx, ty, v->explored_mask);
                        if (t.edge_explored == edge_e) unchanged = 1;
                    }
                    if (unchanged) {
                        if (!v->highlight_on) skip = 1;
                        else {
                            in_second = v->highlight2_on && inside(v->rect2, sx, sy, right, bottom);
                            if (inside(v->rect1, sx, sy, right, bottom)) region = 1;
                            else if (!in_second) skip = 1;
                            else { in_second = 0; region = 2; }
                        }
                    }
                }
                if (!skip) {
                    if (!v->fog_edges) edge_v = 0;
                    if (all_visible) edge_e = 0;
                    if (edge_v == 0xffffffff)
                        edge_v = fog == AOE_VIEW_FOG_VISIBLE ? o->neighbor_mask(o->context, tx, ty, v->visible_mask) : 0;
                    if (edge_e == 0xffffffff)
                        edge_e = fog == AOE_VIEW_FOG_UNSEEN ? 0 : o->neighbor_mask(o->context, tx, ty, v->explored_mask);
                    oa = o->outline(o->context, t.shape, edge_v, 0);
                    ob = o->outline(o->context, t.shape, edge_v, 1);
                    oe = o->edge_outline(o->context, t.shape, edge_e);
                    if (fog == AOE_VIEW_FOG_EXPLORED) {
                        ob = o->outline(o->context, t.shape, 0, 0);
                        use_b = 1;
                        use_a = 0;
                    } else {
                        use_a = oa != NULL;
                        use_b = ob != NULL;
                    }
                    use_edge = (int32_t)edge_e >= 1 && oe != NULL;
                    if (region != v->last_region) {
                        STEP(AOE_VTP_REGION_BOUNDS, region, 0, 0);
                        v->last_region = region;
                    }
                    add_outlines(o, oa, ob, oe, use_a, use_b, use_edge, sx, sy);
                    if (in_second) {
                        STEP(AOE_VTP_REGION_BOUNDS, 2, 0, 0);
                        v->last_region = 2;
                        add_outlines(o, oa, ob, oe, use_a, use_b, use_edge, sx, sy);
                    }
                    draw_id = terrain;
                    repl = o->terrain_replacement(o->context, terrain);
                    if (repl != -1) draw_id = (uint8_t)repl;
                    t.last_fog = fog;
                    t.fog_9 = fog;
                    t.edge_visible = (uint8_t)edge_v;
                    t.flags &= 0xbf;
                    t.edge_explored = (uint8_t)edge_e;
                    o->tile_store(o->context, tx, ty, &t);
                    o->draw_tile(o->context, tx, ty, draw_id, sx, sy, fog, use_a, use_b);
                    STEP(AOE_VTP_TILES_DRAWN, 0, 0, 0);
                }
            }
            if (present) {
                if (t.flags & 0x40) {
                    uint8_t terrain = t.terrain & 0x1f, draw_id = terrain;
                    int16_t repl = o->terrain_replacement(o->context, terrain);
                    if (repl != -1) draw_id = (uint8_t)repl;
                    o->draw_tile(o->context, tx, ty, draw_id, sx, sy, fog, 1, 1);
                    o->tile(o->context, tx, ty, &t);
                    t.flags &= 0xbf;
                    o->tile_store(o->context, tx, ty, &t);
                }
                STEP(AOE_VTP_BATCH, 1, 0, 0);
                if (fog == AOE_VIEW_FOG_UNSEEN) STEP(AOE_VTP_ROW_WINDOW, 0, 5, 0);
                if (fog != AOE_VIEW_FOG_VISIBLE) STEP(AOE_VTP_FOGGED, 1, 0, 0);
                o->draw_objects(o->context, sx, sy + (t.terrain >> 5) * v->half_h,
                                fog == AOE_VIEW_FOG_EXPLORED);
                STEP(AOE_VTP_ROW_WINDOW, 0, 0x28, 0);
                STEP(AOE_VTP_FOGGED, 0, 0, 0);
                STEP(AOE_VTP_BATCH, 0, 0, 0);
            }
        }
        if (r & 1) ++sy0; else --sx0;
    }
    while (o->blits_active(o->context)) STEP(AOE_VTP_STEP_BLITS, 1, 0, 0);
    if (o->mouse_global(o->context)) STEP(AOE_VTP_MOUSE, 0, 0, 0);
    STEP(AOE_VTP_BATCH, 1, 0, 0);
    if (v->drag_kind == 2) STEP(AOE_VTP_DRAG_BOX, 0, 0, 0);
    edit = o->edit_mode(o->context);
    if (edit == 1 || (edit > 6 && edit < 9)) STEP(AOE_VTP_PREVIEW, 0, 0, 0);
    if (o->edit_mode(o->context) == 0x15) STEP(AOE_VTP_MODE_15, 0, 0, 0);
    marks(v, o);
    STEP(AOE_VTP_BATCH, 0, 0, 0);
    STEP(AOE_VTP_LINK_ROWS, 0, 0, 0);
    STEP(AOE_VTP_COUNTS_RESET, 0, 0, 0);
    if (v->last_region != 0) STEP(AOE_VTP_REGION_BOUNDS, 0, 0, 0);
    STEP(AOE_VTP_SHADOW_ROWS, 0, 0, 0);
    if (o->mouse_global(o->context)) STEP(AOE_VTP_MOUSE, 0, 0, 0);
    hover(v, o, dx, dy);
    STEP(AOE_VTP_PANEL_BOXES, dx, dy, 0);
    if (!v->incremental) {
        STEP(AOE_VTP_DIRTY_ALL, 0, 0, 0);
        STEP(AOE_VTP_ROWS_RESET, 0, 0, 0);
    } else {
        STEP(AOE_VTP_REGION_OP, 0, 0, 0);
        if (v->highlight_on) {
            int32_t y;
            if (v->rect1[0] >= 0)
                for (y = v->rect1[1]; y <= v->rect1[3]; ++y)
                    STEP(AOE_VTP_DIRTY_SPAN, v->rect1[0], v->rect1[2], y);
            if (v->highlight2_on && v->rect2[0] >= 0)
                for (y = v->rect2[1]; y <= v->rect2[3]; ++y)
                    STEP(AOE_VTP_DIRTY_SPAN, v->rect2[0], v->rect2[2], y);
        }
        STEP(AOE_VTP_DIFF_ROWS, 0, 0, 0);
    }
    STEP(AOE_VTP_COPY_SPANS, dx, dy, 0);
    STEP(AOE_VTP_ROW_STARTS, 0, 0, 0);
    STEP(AOE_VTP_RASTER_SETUP, 0, 0, 0);
    STEP(AOE_VTP_SPRITE_ROWS, 0, 0, 0);
    STEP(AOE_VTP_UNLOCK, 0, 0, 0);
    STEP(AOE_VTP_SWAP_ROWS, 0, 0, 0);
    return 0;
}

/* ---- 0x00513ea0 ---- */
int aoe_game_view_draw_tile_ror(const AoeViewTileDrawOpsRor *o, const AoeViewTileRor *t,
                                uint8_t terrain, int16_t sx, int16_t sy, int16_t x, int16_t y,
                                uint8_t fog, int a, int b) {
    uint8_t flags = t->flags, type = t->border & 0xf, bshape = t->border >> 4;
    uint8_t draw_terrain = 1;
    int has_border = 0, result = 0;
    int32_t frames[4] = {-1, -1, -1, -1};
    if ((int16_t)terrain >= o->terrain_count || !o->terrain_enabled(o->context, terrain)) return 0;
    if (type != 0 && o->border_enabled(o->context, type) && o->border_shape(o->context, type)) {
        if (!o->border_per_edge(o->context, type)) {
            if (o->border_frame(o->context, type, t->shape, bshape, x, y) != -1) {
                has_border = 1;
                draw_terrain = o->border_draws_terrain(o->context, type);
            }
        } else if (o->border_frames(o->context, type, t->shape, bshape, x, y, frames)) {
            has_border = 1;
            draw_terrain = 1;
        }
    }
    if (o->grid > 0 && (flags & 0xdf) == 0 && o->game_state == 7 &&
        (((int8_t)(x / o->grid) + (int8_t)(y / o->grid)) & 1) == 1)
        flags |= 0x20;
    if (draw_terrain == 0) {
        if (!has_border) return 0;
    } else {
        const void *shape = o->terrain_shape(o->context, terrain);
        int16_t frame = o->terrain_frame(o->context, terrain, t->shape, x, y);
        if (shape && frame != -1) {
            if (fog) {
                uint32_t bits = o->visibility(o->context, x, y);
                if (a == 0) {
                    if (b != 0 && (o->explored_mask & bits) == 0) ++*o->sync_explored;
                } else if (b == 0 && (o->visible_mask & bits) == 0) {
                    ++*o->sync_visible;
                }
            }
            o->blit(o->context, sx, sy, shape, frame, fog, flags, a, b);
            result = 1;
        }
        if (!has_border) {
            if (x > 0) o->mark_dirty(o->context, (int16_t)(x - 1), y);
            if (o->map_h - 1 <= y) return result;
            o->mark_dirty(o->context, x, (int16_t)(y + 1));
            return result;
        }
    }
    {
        const void *shape = o->border_shape(o->context, type);
        if (shape) {
            if (!o->border_per_edge(o->context, type)) {
                int16_t frame = o->border_frame(o->context, type, t->shape, bshape, x, y);
                if (frame != -1) o->blit(o->context, sx, sy, shape, frame, fog, flags, a, b);
            } else if (o->border_frames(o->context, type, t->shape, bshape, x, y, frames)) {
                static const int order[4] = {0, 2, 1, 3};
                int i;
                for (i = 0; i < 4; ++i)
                    if (frames[order[i]] != -1)
                        o->blit(o->context, sx, sy, shape, frames[order[i]], fog, flags, a, b);
            }
        }
    }
    if (x > 0) o->mark_dirty(o->context, (int16_t)(x - 1), y);
    if (o->map_h - 1 <= y) return result;
    o->mark_dirty(o->context, x, (int16_t)(y + 1));
    return result;
}

/* ---- 0x005118c0 ---- */
#define WSTEP(id, x, y, a, b, c) o->step(o->context, (id), (int32_t)(x), (int32_t)(y), (int32_t)(a), (int32_t)(b), (int32_t)(c))

static int walk_fogged(const AoeViewTerrainPassRor *v, uint8_t fog) {
    if (fog == AOE_VIEW_FOG_VISIBLE) return 0;
    if (fog == AOE_VIEW_FOG_EXPLORED && !v->fog_edges) return 0;
    return 1;
}

uint8_t aoe_main_view_walk_ror(AoeViewTerrainPassRor *v, const AoeViewWalkOpsRor *o, uint8_t mode,
                               uint8_t min_kind, const int16_t *point, void *arg, void **result,
                               float *out_x, float *out_y, int16_t *out_sx, int16_t *out_sy) {
    int pick = mode == '(' || mode == ')' || mode == '*' || mode == '+' || mode == ',' || mode == '<';
    int objects = mode == ')' || mode == '*' || mode == '+' || mode == ',';
    int draw_mode = mode == 10 || mode == 0x1e || mode == 0x1f || mode == 0x20 || mode == 0x15 ||
                    mode == 11 || mode == 14;
    uint8_t res = '2', phase = 4;
    int32_t target_x = 0, target_y = 0, area[4] = {0, 0, 0, 0};
    void *preferred = NULL, *best = NULL;
    int32_t sx0 = v->scroll_x, sy = v->scroll_y, tx0 = v->start_x, ty0 = v->start_y;
    int32_t cols0 = v->cols + 4, cols = cols0, rows = (v->rows + 6) * 2, r, c;
    v->mode = mode;
    v->min_kind = min_kind;
    if (mode == 10 && v->count_terrains) o->terrains_uncounted(o->context);
    WSTEP(AOE_VW_EXPLORED_QUERY, 0, 0, 0, 0, 0);
    if (pick) {
        if (mode == '<') { target_x = (int32_t)*out_x; target_y = (int32_t)*out_y; }
        *out_x = 0.0f; *out_y = 0.0f; *out_sx = 0; *out_sy = 0;
        if (objects) preferred = *result;
        *result = NULL;
    }
    if (mode == 14 && !o->area(o->context, area)) return 0;
    for (r = 0; r < rows; ++r) {
        int32_t sx = sx0, tx = tx0, ty = ty0;
        for (c = 0; c < cols; ++c, sx += v->tile_w, ++tx, ++ty) {
            int on_map = tx >= 0 && tx < v->map_w && ty >= 0 && ty < v->map_h, off, proceed;
            uint8_t fog = 0, shape = 0, elev = 0;
            AoeViewTileRor t;
            void *tile = NULL;
            int32_t ty_screen;
            int16_t w, h;
            if (on_map) {
                o->tile(o->context, tx, ty, &t);
                fog = o->fog_value(o->context, tx, ty);
                if (!v->has_visibility || v->reveal_all) {
                    if (fog == 0) fog = AOE_VIEW_FOG_EXPLORED;
                } else if (mode == 0x1e || mode == 0x1f) {
                    fog = AOE_VIEW_FOG_VISIBLE;
                }
                if ((mode == 11 && t.fog_9 == t.last_fog) ||
                    (fog != AOE_VIEW_FOG_VISIBLE && fog != AOE_VIEW_FOG_EXPLORED))
                    fog = 0;
                proceed = fog != 0 || mode == 14 || mode == '(' || mode == '<' || mode == '*';
            } else {
                proceed = mode == '(' || mode == '*';
            }
            if (!proceed) continue;
            if (on_map) {
                tile = o->tile_ref(o->context, tx, ty);
                shape = t.shape;
                elev = t.terrain >> 5;
            }
            ty_screen = sy - elev * v->elevation_step;
            switch (shape) {
            case 2: case 6: case 8: case 10: case 0xe: case 0xf: case 0x10:
                ty_screen -= v->elevation_step;
                break;
            default:
                break;
            }
            o->shape_size(o->context, shape, &w, &h);
            if (sx > v->clip[2] || ty_screen > v->clip[3]) off = 1;
            else if (w + sx <= v->clip[0] || h + ty_screen <= v->clip[1]) off = 1;
            else if (mode == 14 && (tx < area[0] || tx > area[2] || ty < area[1] || ty > area[3])) off = 1;
            else off = 0;
            if (draw_mode) {
                if (!off && (mode == 10 || mode == 11 || mode == 14)) {
                    uint8_t terrain = t.terrain & 0x1f;
                    int16_t repl = o->terrain_replacement(o->context, terrain);
                    if (repl != -1) terrain = (uint8_t)repl;
                    if (mode == 10 && v->count_terrains && (int16_t)terrain < v->terrain_count)
                        o->terrain_counted(o->context, terrain);
                    if (mode == 11) {
                        t.last_fog = t.fog_9;
                        o->tile_store(o->context, tx, ty, &t);
                    }
                }
                if ((fog == AOE_VIEW_FOG_VISIBLE || fog == AOE_VIEW_FOG_EXPLORED) &&
                    (mode == 0x1e || mode == 0x1f || mode == 0x20 || mode == 0x15)) {
                    int fogged = walk_fogged(v, fog);
                    int id = mode == 0x1e ? AOE_VW_OBJECTS_1E : mode == 0x1f ? AOE_VW_OBJECTS_1F
                             : mode == 0x20 ? AOE_VW_OBJECTS_20 : AOE_VW_OBJECTS_15;
                    WSTEP(id, tx, ty, sx, sy, fogged | ((fog == AOE_VIEW_FOG_EXPLORED && v->fog_edges) ? 2 : 0));
                }
                continue;
            }
            if ((fog == AOE_VIEW_FOG_VISIBLE || fog == AOE_VIEW_FOG_EXPLORED) && objects) {
                void *obj = o->pick_object(o->context, tile, sx, sy, tx, ty, point, arg, out_sx, out_sy,
                                           preferred, walk_fogged(v, fog));
                if (obj) {
                    if (mode == ',') res = '4';
                    else {
                        int take;
                        if (preferred && obj == preferred && res == '4') { *result = best; return '4'; }
                        if (!best) take = 1;
                        else if (o->object_kind(o->context, obj) >= o->object_kind(o->context, best)) take = 1;
                        else take = best == preferred && obj != preferred;
                        if (take) {
                            o->object_point(o->context, obj, out_x, out_y);
                            best = obj;
                            res = '4';
                        }
                    }
                }
            }
            if (mode == '(' || (mode == '*' && res == '2')) {
                if (o->terrain_hit(o->context, tile, sx, ty_screen, tx, ty, point)) {
                    if (tile) {
                        o->elevation_point(o->context, point[0] - sx, point[2] - ty_screen, tile, out_x, out_y);
                        *out_x += (float)tx;
                        *out_y += (float)ty;
                    } else {
                        *out_x = (float)tx;
                        *out_y = (float)ty;
                    }
                    *out_sx = (int16_t)sx;
                    *out_sy = (int16_t)ty_screen;
                    res = '3';
                    *result = tile;
                }
            } else if (mode == '<' && tx == target_x && ty == target_y) {
                *out_x = (float)tx;
                *out_y = (float)ty;
                *out_sx = (int16_t)sx;
                *out_sy = (int16_t)ty_screen;
                *result = tile;
                return '3';
            }
        }
        if (phase == 1 && cols == cols0) phase = 2;
        switch (phase) {
        case 1: ++cols; sx0 -= v->half_w; --tx0; break;
        case 2: --cols; sx0 += v->half_w; ++ty0; break;
        case 3: phase = 4; sx0 -= v->half_w; --tx0; break;
        default: sx0 += v->half_w; ++ty0; phase = 3; break;
        }
        sy += v->half_h;
    }
    if (mode == 10) WSTEP(AOE_VW_D0_RESET, 0, 0, 0, 0, 0);
    if ((mode == ')' || mode == '*' || mode == '+') && res == '4') *result = best;
    return res;
}
