#include "main_view.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 0x0050fee0 */
void aoe_main_view_set_map_ror(AoeMainViewRor *v, void *map)
{
    const AoeMainViewOpsRor *ops = v->ops;

    v->map = map;
    if (map != NULL) {
        v->world = ops->map_world(ops->context, map);
        ops->map_attached(ops->context, map);
        ops->map_geometry(ops->context, v->world, v->geometry);
    } else {
        v->world = NULL;
        memset(v->geometry, 0, sizeof(v->geometry));
    }
    ops->recompute_layout(ops->context);
}

/* 0x0050ffa0 */
void aoe_main_view_set_player_ror(AoeMainViewRor *v, void *player)
{
    v->player = player;
    v->point_x = AOE_MAIN_VIEW_NO_POINT_ROR;
    v->point_y = AOE_MAIN_VIEW_NO_POINT_ROR;
    v->ops->redraw(v->ops->context, v, 2);
}

/* 0x0050fec0 */
void aoe_main_view_set_mode_ror(AoeMainViewRor *v, int32_t mode)
{
    v->ops->base_set_mode(v->ops->context, mode);
    if (mode == 2)
        v->drag_flag = 0;
}

/* 0x0050f590 */
void aoe_main_view_handle_ror(AoeMainViewRor *v, int32_t value)
{
    const AoeMainViewOpsRor *ops = v->ops;

    ops->base_handler(ops->context, value);
    if (ops->global_flag(ops->context))
        ops->notify_area(ops->context, v->area);
}

/* 0x0050fba0 */
void aoe_main_view_free_overlay_ror(AoeMainViewRor *v)
{
    if (v->overlay != NULL) {
        v->ops->destroy_overlay(v->ops->context, v->overlay);
        v->overlay = NULL;
    }
}

/* 0x00518e50 */
int aoe_game_view_check_redraw_ror(AoeMainViewRor *v)
{
    const AoeMainViewOpsRor *ops = v->ops;
    int32_t state = ops->game_state(ops->context);
    int32_t mode;

    if (state != 4 && state != 5 && state != 6 && state != 7)
        return 0;
    mode = ops->game_edit_mode(ops->context);
    if (mode == 1 || mode == 7 || mode == 8)
        ops->redraw(ops->context, v, 1);
    return 1;
}

/* 0x0051a180 / 0x0051a1d0 */
int aoe_game_view_after_base_ror(AoeMainViewRor *v, int base_ok)
{
    if (!base_ok)
        return 0;
    if (v->aux_panel != NULL)
        v->ops->redraw(v->ops->context, v->aux_panel, 1);
    return 1;
}

/* 0x0051afb0 */
int aoe_game_view_clear_highlight_ror(AoeMainViewRor *v)
{
    v->ops->set_highlight(v->ops->context, -1, -1, -1, -1);
    v->ops->redraw(v->ops->context, v, 1);
    return 0;
}

/* 0x005104a0 */
void aoe_main_view_step_blits_ror(AoeMainViewRor *v, int force)
{
    const AoeMainViewOpsRor *ops = v->ops;
    int32_t i;

    if (!v->blits_active)
        return;
    if (!force && ops->surface_busy(ops->context))
        return;
    i = ++v->blit_index;
    if (v->blits != NULL && i >= 0 && i < v->blit_count) {
        const AoeMainViewBlitRor *b = &v->blits[i];
        int32_t src[4];

        src[0] = b->left + v->blit_off_x;
        src[1] = b->top + v->blit_off_y;
        src[2] = b->right + 1 + v->blit_off_x;
        src[3] = b->bottom + 1 + v->blit_off_y;
        ops->surface_blit(ops->context, b->dest_x + v->blit_off_x,
                          b->dest_y + v->blit_off_y, src);
        if (v->blit_index + 1 < v->blit_count)
            return;
    }
    v->blits_active = 0;
}

/* 0x00510ef0 */
void aoe_main_view_draw_ror(AoeMainViewRor *v, uint8_t code, void *target)
{
    static const char name[] = "view::draw_view";
    const AoeMainViewOpsRor *ops = v->ops;
    /* The original leaves these uninitialised unless drawing into the
       terrain buffer; only that path uses them meaningfully. */
    int32_t l = 0, t = 0, r = 0, b = 0;

    if (target == NULL)
        target = v->surface;
    v->draw_target = target;
    if (target == v->overlay) {
        l = v->left; t = v->top; r = v->right; b = v->bottom;
        v->left = 0;
        v->top = 0;
        v->right = r - l;
        v->bottom = b - t;
        v->scroll_x = (int16_t)(v->scroll_x - (int16_t)l);
        v->scroll_y = (int16_t)(v->scroll_y - (int16_t)t);
        v->origin_x += l;
        v->origin_y += t;
    }
    if (ops->lock_surface(ops->context, target, name)) {
        if (code == 10)
            ops->draw_region(ops->context, l, t, r, b);
        else
            ops->draw_code(ops->context, code);
        ops->unlock_surface(ops->context, target, name);
    }
    if (v->draw_target == v->overlay) {
        v->scroll_x = (int16_t)(v->scroll_x + (int16_t)l);
        v->scroll_y = (int16_t)(v->scroll_y + (int16_t)t);
        v->right += l;
        v->bottom += t;
        v->left += l;
        v->top += t;
        v->origin_x -= l;
        v->origin_y -= t;
    }
    v->draw_target = NULL;
}

/* 0x00511430 */
uint8_t aoe_main_view_pick_ror(AoeMainViewRor *v, uint8_t kind, int32_t arg,
                               int32_t x, int32_t y, int32_t mask,
                               AoeMainViewPickRor *out)
{
    int32_t value = kind != 0x28 ? mask : 0;
    int32_t a[2], b[2];
    uint8_t r;

    a[0] = b[0] = x;
    a[1] = b[1] = y;
    memset(out, 0, sizeof(*out));
    r = v->ops->pick(v->ops->context, kind, arg, a, b, &value,
                     &out->word0, &out->word1, &out->half0, &out->half1);
    if (r == '4')
        out->object_value = value;
    else if (r == '3')
        out->other_value = value;
    return r;
}

/* 0x00514500 */
int aoe_main_view_cursor_in_view_ror(AoeMainViewRor *v, int32_t dx,
                                     int32_t dy, int32_t *x, int32_t *y)
{
    const AoeMainViewOpsRor *ops = v->ops;

    ops->cursor_pos(ops->context, x, y);
    if (ops->windowed(ops->context)) {
        ops->screen_to_client(ops->context, x, y);
        if (*x > 0x7fff)
            *x -= 0x10000;
        if (*y > 0x7fff)
            *y -= 0x10000;
    }
    *x += dx;
    *y += dy;
    return v->left <= *x && *x <= v->right && v->top <= *y &&
           *y <= v->bottom;
}

/* 0x00513980 */
int aoe_main_view_update_row_starts_ror(AoeMainViewRor *v,
                                        const void *surface,
                                        uint8_t *const *surface_rows)
{
    int32_t height = v->bottom - v->top + 1;
    int32_t i;

    if (v->rows_surface == surface && v->row_count == height)
        return 1;
    if (height <= 0 || surface == NULL)
        return 1;
    if (height != v->row_count || v->row_starts == NULL) {
        free(v->row_starts);
        v->row_starts = malloc((size_t)height * sizeof(*v->row_starts));
        v->row_count = height;
        if (v->row_starts == NULL)
            return 0;
    }
    for (i = 0; i < height; i++)
        v->row_starts[i] = surface_rows[v->top + i] + v->left;
    return 1;
}

void aoe_main_view_free_row_starts_ror(AoeMainViewRor *v)
{
    free(v->row_starts);
    v->row_starts = NULL;
    v->row_count = 0;
}

/* 0x00518e00 */
int32_t aoe_game_view_timed_draw_ror(AoeMainViewRor *v, int32_t a, int32_t b,
                                     int32_t c, int32_t d, int32_t e)
{
    const AoeMainViewOpsRor *ops = v->ops;
    uint32_t start = ops->time_ms(ops->context);
    int32_t r = ops->base_draw(ops->context, a, b, c, d, e);

    ops->record_time(ops->context, 13, ops->time_ms(ops->context) - start);
    return r;
}

/* 0x0050f420 */
int aoe_main_view_construct_ror(AoeMainViewLifeRor *v,
                                const AoeMainViewLifeOpsRor *ops,
                                const int32_t base_args[7],
                                int32_t owner_arg, const char *shape_name)
{
    char name[260];
    int i;

    ops->base_construct(ops->context, base_args);
    memset(v, 0, sizeof(*v));
    v->link = NULL;
    v->owner_arg = owner_arg;
    v->point_x = AOE_MAIN_VIEW_NO_POINT_ROR;
    v->point_y = AOE_MAIN_VIEW_NO_POINT_ROR;
    v->flag_140 = 0;
    v->white_pen = ops->stock_object(ops->context, 6);
    v->res[AOE_MAIN_VIEW_RES_PEN] = ops->create_pen(ops->context, 0, 1, 0xffu);
    v->null_brush = ops->stock_object(ops->context, 5);
    snprintf(name, sizeof(name), "%s.shp", shape_name);
    v->res[AOE_MAIN_VIEW_RES_SHAPE] = ops->load_shape(ops->context, name);
    ops->init_edges(ops->context);
    for (i = 0; i < AOE_MAIN_VIEW_LISTS_ROR; i++) {
        v->res[AOE_MAIN_VIEW_RES_LIST0 + i] = ops->alloc_zero(
            ops->context, AOE_MAIN_VIEW_LIST_CAPACITY_ROR,
            AOE_MAIN_VIEW_LIST_ENTRY_SIZE_ROR);
        v->list_count[i] = 0;
    }
    return 1;
}

static void release_res(AoeMainViewLifeRor *v, const AoeMainViewLifeOpsRor *ops,
                        int which)
{
    if (v->res[which] != NULL) {
        ops->release(ops->context, (AoeMainViewResourceRor)which, v->res[which]);
        /* The original clears only the shape and the pen. */
        if (which == AOE_MAIN_VIEW_RES_SHAPE || which == AOE_MAIN_VIEW_RES_PEN)
            v->res[which] = NULL;
    }
}

/* 0x0050f240 */
void aoe_main_view_destruct_ror(AoeMainViewLifeRor *v,
                                const AoeMainViewLifeOpsRor *ops)
{
    int i;

    release_res(v, ops, AOE_MAIN_VIEW_RES_SHAPE);
    ops->free_overlay(ops->context);
    for (i = AOE_MAIN_VIEW_RES_PEN; i < AOE_MAIN_VIEW_RES_TIMERS; i++)
        release_res(v, ops, i);
    if (ops->global_flag(ops->context))
        ops->clear_globals(ops->context);
    release_res(v, ops, AOE_MAIN_VIEW_RES_TIMERS);
    ops->free_marks(ops->context);
    ops->base_destruct(ops->context);
    ops->base_destruct(ops->context);
}

/* 0x0050f220 */
void *aoe_main_view_delete_ror(AoeMainViewLifeRor *v,
                               const AoeMainViewLifeOpsRor *ops,
                               uint8_t flags)
{
    aoe_main_view_destruct_ror(v, ops);
    if (flags & 1)
        ops->delete_self(ops->context, v);
    return v;
}

/* 0x00516800 */
void aoe_view_grid_init_ror(AoeViewGridRor *grid, AoeViewGridGlobalsRor *g,
                            int32_t rows, int32_t cols,
                            void (*build_lookup)(AoeViewGridRor *grid))
{
    int32_t r, c;

    grid->row_count = rows;
    grid->col_count = cols + 1;
    grid->lookup = NULL;
    grid->rows = rows > 0 ? calloc((size_t)rows, sizeof(*grid->rows)) : NULL;
    for (r = 0; grid->rows != NULL && r < grid->row_count; r++) {
        AoeViewGridCellRor *row = calloc((size_t)grid->col_count, sizeof(*row));
        grid->rows[r] = row;
        for (c = 0; row != NULL && c < grid->col_count; c++) {
            row[c].capacity = 0x20;
            row[c].count = 0;
            row[c].items = calloc(0x20, 8);
        }
    }
    for (r = 0; r < AOE_VIEW_GRID_BUFFERS_ROR; r++) {
        grid->buffers[r] = malloc(AOE_VIEW_GRID_BUFFER_SIZE_ROR);
        g->buffers[r] = grid->buffers[r];
        g->used[r] = 0;
    }
    if (build_lookup != NULL)
        build_lookup(grid);
    g->current = grid;
}

/* 0x005168d0 */
void aoe_view_grid_free_ror(AoeViewGridRor *grid, AoeViewGridGlobalsRor *g)
{
    int32_t r, c;

    if (grid->rows != NULL) {
        for (r = 0; r < grid->row_count; r++) {
            AoeViewGridCellRor *row = grid->rows[r];
            if (row == NULL)
                continue;
            for (c = 0; c < grid->col_count; c++)
                free(row[c].items);
            free(row);
        }
        free(grid->rows);
        /* The original leaves the freed pointer in place. */
    }
    for (r = 0; r < AOE_VIEW_GRID_BUFFERS_ROR; r++) {
        free(grid->buffers[r]);
        g->buffers[r] = NULL;
        g->used[r] = 0;
    }
    free(grid->lookup);
    g->current = NULL;
}

/* 0x005160d0 */
int aoe_game_view_has_draw_node_ror(
    const uint8_t *(*find)(void *ctx, int32_t id, int32_t y0, int32_t y1,
                           int32_t key),
    void *ctx, int32_t id, int any)
{
    const uint8_t *flags = find(ctx, id, 6, 0x28, 0);

    if (flags == NULL)
        return 0;
    if (!any && (*flags & 1))
        return 0;
    return 1;
}

/* 0x00517820 */
int32_t aoe_view_footprint_index_ror(int32_t radius)
{
    if (radius > 16)
        return 16;
    if (radius < 0)
        return 0;
    return radius;
}

const void *aoe_view_footprint_table_ror(const void *const normal[17],
                                        const void *const alt_set[17],
                                        int32_t radius, int alt)
{
    int32_t i = aoe_view_footprint_index_ror(radius);
    return alt ? alt_set[i] : normal[i];
}

/* 0x00518700 */
void aoe_game_view_draw_pass_ror(void *ctx, void (*first)(void *),
                                 void (*second)(void *),
                                 void (*base)(void *))
{
    first(ctx);
    second(ctx);
    base(ctx);
}

/* 0x00517e90 */
void aoe_int_grid_read_ror(AoeIntGridRor *g,
                           void (*read)(void *ctx, void *buf, uint32_t n),
                           void *ctx)
{
    int32_t old_w = g->width, old_h = g->height;

    read(ctx, &g->width, 4);
    read(ctx, &g->height, 4);
    if (g->height > 0 && g->width > 0) {
        uint32_t n = (uint32_t)g->height * (uint32_t)g->width;
        if (old_w != g->width || old_h != g->height) {
            free(g->cells);
            g->cells = calloc(n, 4);
        }
        read(ctx, g->cells, n * 4u);
        return;
    }
    g->height = 0;
    g->cells = NULL;
    g->width = 0;
}

/* 0x005116c0 */
uint8_t aoe_main_view_unit_cursor_ror(uint8_t unit_kind, int is_target,
                                      int32_t mode)
{
    int any = mode == 1 || mode == 2;

    switch (unit_kind) {
    case 2:
        if (is_target)
            return 5;
        if (any)
            return 3;
        return mode == 3 ? 2 : 0;
    case 3:
        if (is_target)
            return 5;
        return any ? 3 : 0;
    case 4:
        if (is_target)
            return 5;
        if (any)
            return 4;
        return mode == 3 ? 3 : 2;
    default:
        return 1;
    }
}

/* 0x005176b0 */
void aoe_visibility_fill_ror(AoeVisibilityFillRor *m, int8_t value)
{
    int32_t x, y;

    for (y = 0; y < m->height; y++)
        for (x = 0; x < m->width; x++)
            m->rows[y][x] = value;
    if (value == -1 || value == 0) {
        uint32_t mask = m->clear_mask;
        if (value == -1)
            mask &= m->unexplore_mask;
        for (y = 0; y < m->height; y++)
            for (x = 0; x < m->width; x++)
                m->tile_masks[y][x] &= mask;
    } else {
        uint32_t bits = m->explored_bits | m->visible_bits;
        for (y = 0; y < m->height; y++)
            for (x = 0; x < m->width; x++)
                m->tile_masks[y][x] |= bits;
    }
    m->explored_count = value != -1 ? m->cell_total : 0;
}

/* 0x0050cdf0 */
void aoe_world_leader_ror(const AoeLeaderOpsRor *ops, int16_t count,
                          int32_t category, AoeLeaderRor *out)
{
    int32_t best = 0, leader = 1, i;
    uint8_t allies = 0;
    const char *name;

    for (i = 1; i < count; i++) {
        if (ops->excluded(ops->context, i) == 0) {
            int32_t sc = ops->score(ops->context, i, category);
            if (sc > best) {
                best = sc;
                leader = i;
            }
        }
    }
    out->score = best;
    out->colour = ops->colour(ops->context, leader);
    name = ops->name(ops->context, leader);
    /* The original copies unbounded; bounded here. */
    snprintf(out->name, sizeof(out->name), "%s", name ? name : "");
    for (i = 1; i < count; i++) {
        if (i != leader && ops->stance(ops->context, i, leader) == 0 &&
            ops->stance(ops->context, leader, i) == 0)
            allies++;
    }
    out->allies = count < 2 ? 0 : allies;
}

/* 0x0051a210 */
void aoe_game_view_render_ror(const AoeGameViewRenderOpsRor *ops)
{
    uint32_t start = ops->time_ms(ops->context);
    int32_t s0 = ops->stat(ops->context, 11), s1;
    uint32_t end;

    if (!ops->has_map(ops->context)) {
        ops->begin_paint(ops->context, 0);
        ops->clear_area(ops->context);
        ops->end_paint(ops->context);
    } else {
        if (ops->debug_log(ops->context))
            ops->log(ops->context, "<vs>");
        ops->render(ops->context);
        if (ops->debug_log(ops->context))
            ops->log(ops->context, "<vf>\n");
    }
    s1 = ops->stat(ops->context, 11);
    end = ops->time_ms(ops->context);
    ops->record(ops->context, 6, (end - (uint32_t)(s1 - s0)) - start);
    ops->advance(ops->context);
    ops->record(ops->context, 12, (uint32_t)(s1 - s0));
}

/* 0x00511790 */
int aoe_main_view_tile_point_ror(const AoeMainViewRor *v,
                                 const AoeMainViewTileRor *const *tiles,
                                 int32_t width, int32_t height,
                                 const int16_t *slope_offsets, int16_t x,
                                 int16_t y, int16_t *out_x, int16_t *out_y,
                                 int centre)
{
    const AoeMainViewTileRor *t;
    int16_t sy, step = v->geometry[4];

    if (out_x != NULL)
        *out_x = 0;
    if (out_y != NULL)
        *out_y = 0;
    if (x < 0 || x >= width || y < 0 || y >= height)
        return 0;
    t = &tiles[y][x];
    sy = (int16_t)(slope_offsets[t->slope] - (int16_t)v->origin_y + t->y);
    if (centre) {
        if (t->slope != 0)
            sy = (int16_t)(sy + step / 2);
        if (t->slope == 6 || t->slope == 5 || t->slope == 3 || t->slope == 11)
            sy = (int16_t)(sy - step);
    }
    *out_x = (int16_t)(t->x - (int16_t)v->origin_x);
    *out_y = sy;
    return 1;
}

/* 0x005150c0 */
int aoe_main_view_edge_query_ror(
    int player_active, uint8_t flags,
    int16_t (*query)(void *ctx, int32_t kind), void *ctx, int32_t out[4])
{
    out[0] = out[2] = out[1] = out[3] = -1;
    if (!player_active || flags == 0)
        return 0;
    if (flags & 1)
        out[0] = query(ctx, 1);
    if (flags & 2)
        out[3] = query(ctx, 4);
    if (flags & 4)
        out[2] = query(ctx, 3);
    if (flags & 8)
        out[1] = query(ctx, 2);
    return 1;
}

/* 0x00519360 */
int aoe_game_view_mouse_up_ror(const AoeGameViewMouseOpsRor *ops,
                               int32_t press_x, int32_t press_y, int32_t a,
                               int32_t b, int32_t c, int32_t d)
{
    int32_t state = ops->game_state(ops->context);

    if (state < 4 || state > 7)
        return 0;
    if (ops->comm_waiting(ops->context)) {
        if (ops->dragging(ops->context))
            ops->cancel_drag(ops->context);
        else
            ops->release_capture(ops->context);
        return 0;
    }
    if (ops->dragging(ops->context)) {
        ops->finish_drag(ops->context, a, b);
        return 1;
    }
    switch (ops->edit_mode(ops->context)) {
    case 0:
        ops->release_capture(ops->context);
        ops->drag_box(ops->context, 2, press_x, press_y, c, d);
        ops->finish_drag(ops->context, a, b);
        break;
    case 1:
    case 7:
    case 8:
        ops->redraw(ops->context, 1);
        break;
    default:
        break;
    }
    return 1;
}

/* 0x00515d30 */
void aoe_main_view_end_drag_ror(const AoeMainViewDragOpsRor *ops,
                                uint8_t *kind, int32_t press_x,
                                int32_t press_y, int32_t cur_x,
                                int32_t cur_y)
{
    switch (*kind) {
    case 1:
    case 6:
    case 7:
    case 8:
        ops->warp_cursor(ops->context, press_x, press_y);
        break;
    case 2: {
        int32_t l = press_x, r = cur_x, t = press_y, b = cur_y;
        if (cur_x < press_x) {
            l = cur_x;
            r = press_x;
        }
        if (cur_y < press_y) {
            t = cur_y;
            b = press_y;
        }
        ops->select_area(ops->context, 4, l, t, r, b);
        ops->redraw(ops->context, 1);
        break;
    }
    case 9:
        ops->point_action(ops->context, press_x, press_y, press_x, press_y, 0);
        ops->redraw(ops->context, 1);
        break;
    default:
        break;
    }
    if (*kind == 1 || *kind == 2 || *kind == 6 || *kind == 7 || *kind == 8)
        ops->restore_mouse(ops->context);
    ops->release_capture(ops->context);
    *kind = 0;
}

/* 0x0051afd0 */
int32_t aoe_game_view_help_ror(const AoeGameViewHelpOpsRor *ops, char *text,
                               size_t text_size, int32_t *page, int32_t x,
                               int32_t y)
{
    void *obj = ops->pick_object(ops->context, x, y);
    const char *s;

    if (obj == NULL)
        return ops->base_help(ops->context, text, page, x, y);
    s = ops->string(ops->context, ops->help_string_id(ops->context, obj));
    snprintf(text, text_size, "%s", s ? s : "");
    *page = ops->help_page(ops->context, obj);
    return 1;
}

/* 0x00511310 */
void aoe_main_view_brush_highlight_ror(AoeMainViewRor *v)
{
    const AoeMainViewOpsRor *ops = v->ops;
    AoeMainViewPickRor pick;
    int32_t mx, my, x, y, half;
    float fx, fy;
    int16_t size;

    ops->set_highlight(ops->context, -1, -1, -1, -1);
    ops->game_mouse(ops->context, &mx, &my);
    if (mx < v->left || mx > v->right || my < v->top || my > v->bottom)
        return;
    if (aoe_main_view_pick_ror(v, 0x28, 0, mx, my, 0, &pick) != '3')
        return;
    memcpy(&fx, &pick.word0, sizeof(fx));
    memcpy(&fy, &pick.word1, sizeof(fy));
    if (ops->game_edit_mode(ops->context) == 0x13) {
        fx = (float)((int32_t)(fx * 0.33333334f) * 3) + 1.0f;
        fy = (float)((int32_t)(fy * 0.33333334f) * 3) + 1.0f;
        size = 3;
    } else {
        size = ops->brush_size(ops->context);
    }
    x = (int32_t)fx;
    y = (int32_t)fy;
    half = (int32_t)size >> 1;
    ops->set_highlight(ops->context, x - half, y - half, x + half, y + half);
}

/* 0x00519d50 */
int aoe_game_view_mouse_down_ror(const AoeGameViewMouseOpsRor *ops,
                                 int32_t a, int32_t b, int32_t c, int32_t d)
{
    int32_t state;

    if (ops->comm_waiting(ops->context))
        return 0;
    state = ops->game_state(ops->context);
    if (state < 4 || state > 7)
        return 0;
    if (ops->edit_mode(ops->context) != 0x13) {
        ops->capture(ops->context);
        return 1;
    }
    if (ops->player_pending(ops->context)) {
        ops->player_clear(ops->context);
        ops->redraw(ops->context, 1);
    }
    ops->set_game_mode(ops->context, 0x13, 1);
    ops->drag_box(ops->context, 9, a, b, c, d);
    return 1;
}

/* 0x00519e00 */
int aoe_game_view_mouse_move_ror(const AoeGameViewMouseOpsRor *ops,
                                 int player_busy, int32_t press_x,
                                 int32_t press_y, int32_t a, int32_t b,
                                 int32_t c, int32_t d)
{
    int32_t state = ops->game_state(ops->context);

    if (state < 4 || state > 7)
        return 0;
    if (ops->comm_waiting(ops->context)) {
        if (ops->dragging(ops->context))
            ops->cancel_drag(ops->context);
        else
            ops->release_capture(ops->context);
        return 0;
    }
    if (ops->dragging(ops->context)) {
        ops->finish_drag(ops->context, a, b);
        return 1;
    }
    if (player_busy)
        return 0;
    ops->release_capture(ops->context);
    ops->drag_box(ops->context, 1, press_x, press_y, c, d);
    ops->finish_drag(ops->context, a, b);
    return 1;
}

/* 0x0051ad40 */
uint8_t aoe_game_view_map_order_ror(const AoeGameViewOrderOpsRor *ops,
                                    int32_t x, int32_t y)
{
    int32_t out[5] = {0, 0, 0, 0, 0};
    uint8_t result = 0;

    if (!ops->orders_enabled(ops->context) ||
        !ops->has_selection(ops->context))
        return 0;
    if (ops->pick(ops->context, x, y, out) != '2') {
        ops->clamp_point(ops->context, out);
        result = ops->order(ops->context, out[3], out[0], out[1]);
        ops->redraw(ops->context, 1);
    }
    ops->set_game_mode(ops->context, 0, 0);
    return result;
}

/* 0x00515e60 */
int aoe_main_view_brush_stroke_ror(const AoeMainViewBrushOpsRor *ops,
                                   int32_t *prev_x, int32_t *prev_y,
                                   int32_t a, int32_t b, int32_t c,
                                   int32_t d, int paint)
{
    float fx, fy, ux, uy;
    int32_t x, y;

    if (ops->pick(ops->context, c, d, &fx, &fy) != '3')
        return 0;
    x = (int32_t)fx;
    y = (int32_t)fy;
    ops->pick(ops->context, a, b, &ux, &uy);
    switch (ops->edit_mode(ops->context)) {
    case 9:
        if (paint && ops->elevation != -1)
            ops->stroke(ops->context, 0x10, *prev_x, *prev_y, x, y,
                        ops->size, ops->elevation);
        break;
    case 10:
        if (paint && ops->terrain != -1)
            ops->stroke(ops->context, 0x18, *prev_x, *prev_y, x, y,
                        ops->size, ops->terrain);
        break;
    case 0x13:
        ops->stroke(ops->context, 0x20, *prev_x, *prev_y, x, y, 0,
                    ops->cliff_mode == 1);
        ops->redraw(ops->context, 2);
        break;
    default:
        break;
    }
    *prev_x = x;
    *prev_y = y;
    return 1;
}

/* ---- Mouse drag start (C3L) ---- */

static uint8_t drag_map_pick(AoeMainViewRor *v, int32_t x, int32_t y,
                             float *fx, float *fy)
{
    AoeMainViewPickRor pick;
    uint8_t r = aoe_main_view_pick_ror(v, 0x28, 0, x, y, 0, &pick);

    memcpy(fx, &pick.word0, sizeof(*fx));
    memcpy(fy, &pick.word1, sizeof(*fy));
    return r;
}

/* 0x005154e0 */
int aoe_main_view_begin_drag_ror(AoeMainViewRor *v,
                                 AoeMainViewDragStateRor *drag,
                                 const AoeMainViewDragStartOpsRor *ops,
                                 uint8_t kind, int32_t x, int32_t y,
                                 int32_t keep_a, int32_t keep_b)
{
    float fx, fy;
    int32_t tx, ty;

    if (kind == 3) {
        if (drag_map_pick(v, x, y, &fx, &fy) != '3')
            return 0;
        tx = (int32_t)fx;
        ty = (int32_t)fy;
        v->ops->set_highlight(v->ops->context, tx, ty, tx, ty);
    } else if (kind == 9) {
        /* The pick result is not checked here. */
        drag_map_pick(v, x, y, &fx, &fy);
        drag->anchor_x = (int32_t)fx;
        drag->anchor_y = (int32_t)fy;
        if (!ops->point_action(ops->context, x, y, x, y, 1))
            return 0;
        v->ops->redraw(v->ops->context, v, 1);
    }
    drag->kind = kind;
    drag->cur_x = drag->press_x = x;
    drag->cur_y = drag->press_y = y;
    ops->capture(ops->context);
    if (kind == 1 || kind == 2 || kind == 6 || kind == 7 || kind == 8)
        ops->hide_mouse(ops->context);
    if (kind == 2) {
        if (keep_b == 0 && keep_a == 0)
            ops->player_clear(ops->context);
        v->ops->redraw(v->ops->context, v, 1);
    }
    if ((kind == 6 || kind == 8) && drag_map_pick(v, x, y, &fx, &fy) == '3') {
        ops->set_view_point(ops->context, fx + 0.5f, fy + 0.5f);
        v->ops->redraw(v->ops->context, v, 1);
        return 1;
    }
    return 0;
}

/* ---- Pixel picking in the draw rows (C3L) ---- */

static int shape_u32(const uint8_t *data, size_t size, uint32_t off,
                     uint32_t *out)
{
    if (off > size || size - off < 4)
        return 0;
    *out = (uint32_t)data[off] | ((uint32_t)data[off + 1] << 8) |
           ((uint32_t)data[off + 2] << 16) | ((uint32_t)data[off + 3] << 24);
    return 1;
}

/* The 4-bit count in the high nibble, or the next byte when it is 0. */
static int shape_nibble_count(const uint8_t *data, size_t size, uint32_t *p,
                              uint8_t cmd, int32_t *count)
{
    *count = cmd >> 4;
    if (*count == 0) {
        if (*p >= size)
            return 0;
        *count = data[(*p)++];
    }
    return 1;
}

/* 0x005149d0 */
int aoe_shape_hit_ror(const uint8_t *data, size_t size,
                      const AoeShapeHitFrameRor *frame, int32_t x, int32_t y)
{
    uint32_t outline, start, p;
    int32_t width, pos, left, right, count;
    uint8_t cmd;

    if (frame == NULL || data == NULL)
        return 0;
    width = frame->width;
    if (x < 0 || x >= width || y < 0 || y >= frame->height)
        return 0;
    if (!shape_u32(data, size, frame->outline_offset + (uint32_t)y * 4,
                   &outline))
        return 0;
    left = (int16_t)(outline & 0xffff);
    right = (int16_t)(outline >> 16);
    if (left < 0 || x < left || x > width - right)
        return 0;
    if (!shape_u32(data, size, frame->cmd_table_offset + (uint32_t)y * 4,
                   &start))
        return 0;
    p = start;
    pos = left;
    while (pos < width && x >= pos) {
        if (p >= size)
            return 0;
        cmd = data[p++];
        switch (cmd & 0x0f) {
        case 0x0: case 0x4: case 0x8: case 0xc: /* colour run */
            pos += cmd >> 2;
            if (x < pos)
                return 2;
            p += (uint32_t)(cmd >> 2);
            break;
        case 0x1: case 0x5: case 0x9: case 0xd: /* skip */
            pos += cmd >> 2;
            break;
        case 0x2: /* long colour run */
            if (p >= size)
                return 0;
            count = (int32_t)(((uint32_t)cmd << 4) | data[p]);
            pos += count;
            if (x < pos)
                return 2;
            p += (uint32_t)count + 1;
            break;
        case 0x3: /* long skip */
            if (p >= size)
                return 0;
            pos += (int32_t)(((uint32_t)cmd << 4) | data[p]);
            p++;
            break;
        case 0x6: /* player colour run */
            if (!shape_nibble_count(data, size, &p, cmd, &count))
                return 0;
            pos += count;
            if (x < pos)
                return 2;
            p += (uint32_t)count;
            break;
        case 0x7: case 0xa: /* fills */
            if (!shape_nibble_count(data, size, &p, cmd, &count))
                return 0;
            pos += count;
            if (x < pos)
                return 2;
            p++;
            break;
        case 0xb: /* shadow */
            if (!shape_nibble_count(data, size, &p, cmd, &count))
                return 0;
            pos += count;
            if (x < pos)
                return 2;
            break;
        default: /* 0xe extended, 0xf end of row */
            return 0;
        }
    }
    return 1;
}

static int clamp_rows(int32_t *lo, int32_t *hi)
{
    if (*lo > AOE_VIEW_DRAW_ROWS_MAX_ROR || *hi < 0)
        return 0;
    if (*lo < 0)
        *lo = 0;
    if (*hi > AOE_VIEW_DRAW_ROWS_MAX_ROR)
        *hi = AOE_VIEW_DRAW_ROWS_MAX_ROR;
    return 1;
}

/* 0x00514d70 */
int aoe_main_view_pixel_object_ror(const AoeMainViewRor *v,
                                   const AoeViewDrawRowsOpsRor *rows,
                                   int32_t x, int32_t y, int32_t lo,
                                   int32_t hi, int32_t *id)
{
    const void *node;
    AoeViewDrawItemRor it;
    int32_t row, px;
    int hit;

    *id = 0;
    if (lo > hi || x < v->left || x > v->right || y < v->top ||
        y > v->bottom || !clamp_rows(&lo, &hi))
        return 0;
    for (row = hi; row >= lo; row--) {
        hit = 0;
        for (node = rows->row_head(rows->context, row); node != NULL;
             node = it.next) {
            rows->item(rows->context, node, &it);
            if (it.key != 0)
                continue;
            px = (it.flags & 2) ? it.right - x : x - it.left;
            if (aoe_shape_hit_ror(it.shape, it.shape_size, it.frame, px,
                                  y - it.top) == 2) {
                hit = 1;
                *id = it.object_id;
            }
        }
        if (hit)
            return 1;
    }
    return 0;
}

static int box_object_counts(const AoeViewDrawRowsOpsRor *rows,
                             const AoeViewDrawItemRor *it, int32_t player,
                             uint8_t min_kind)
{
    int16_t owner;
    uint8_t state, kind;

    if (!rows->object(rows->context, it->object_id, &owner, &state, &kind))
        return 0;
    return (player == -1 || owner == player) && state == 2 &&
           kind >= min_kind;
}

/* 0x00514b60 */
int32_t aoe_main_view_box_objects_ror(const AoeMainViewRor *v,
                                      const AoeViewDrawRowsOpsRor *rows,
                                      int32_t x0, int32_t y0, int32_t x1,
                                      int32_t y1, int32_t lo, int32_t hi,
                                      int32_t player, uint8_t min_kind,
                                      AoeViewPickedRor out[AOE_VIEW_PICKED_MAX_ROR])
{
    const void *node;
    AoeViewDrawItemRor it;
    int32_t ox = v->left, oy = v->top, n = 0, row, i;

    if (x1 < v->left || x0 > v->right || y1 < v->top || y0 > v->bottom ||
        !clamp_rows(&lo, &hi) || lo > hi)
        return 0;
    for (row = hi; row >= lo; row--) {
        for (node = rows->row_head(rows->context, row); node != NULL;
             node = it.next) {
            rows->item(rows->context, node, &it);
            if (it.key != 0 || it.object_id < 0 || x0 - ox > it.right ||
                it.left > x1 - ox || y0 - oy > it.bottom ||
                it.top > y1 - oy ||
                !box_object_counts(rows, &it, player, min_kind))
                continue;
            for (i = 0; i < n && out[i].id != it.object_id; i++)
                ;
            if (i < n) {
                if (out[i].count > 1)
                    out[i].count = 1;
                continue;
            }
            out[n].id = it.object_id;
            out[n].count = 1;
            out[n].field_6 = 1;
            out[n].field_14 = it.field_14;
            out[n].field_18 = it.field_18;
            if (++n >= AOE_VIEW_PICKED_MAX_ROR)
                return AOE_VIEW_PICKED_MAX_ROR;
        }
    }
    return n;
}

/* ---- Line/box draw nodes (C3L) ---- */

/* 0x00513a40 */
void aoe_view_line_node_dirty_ror(const AoeViewLineNodeRor *n,
                                  const AoeViewLineOpsRor *ops)
{
    const int16_t *p = n->pt;
    int32_t lo, hi, y0, y1, y, t;
    int i;

    if (n->kind == 1) {
        for (i = 0; i < 4; i++)
            ops->add_line(ops->context, p[i * 2], p[i * 2 + 1],
                          p[((i + 1) & 3) * 2], p[((i + 1) & 3) * 2 + 1]);
        return;
    }
    if (n->kind == 2 || n->kind == 3) {
        y0 = p[1];
        y1 = p[3];
        if (y0 > y1) {
            t = y0; y0 = y1; y1 = t;
        }
        for (y = y0; y <= y1; y++)
            ops->add_span(ops->context, (int32_t)p[0] & ~3,
                          (int32_t)p[2] | 3, y);
    }
    if (n->kind == 4) {
        lo = p[0];
        hi = p[2];
        if (lo > hi) {
            t = lo; lo = hi; hi = t;
        }
        y0 = p[1];
        y1 = p[3];
        if (y0 > y1) {
            t = y0; y0 = y1; y1 = t;
        }
        ops->add_span(ops->context, lo & ~3, hi | 3, y0);
        ops->add_span(ops->context, lo & ~3, hi | 3, y1);
        for (y = y0 + 1; y < y1; y++) {
            ops->add_span(ops->context, lo & ~3, lo | 3, y);
            ops->add_span(ops->context, hi & ~3, hi | 3, y);
        }
    }
}

/* 0x00513bc0 */
void aoe_view_line_node_draw_ror(const AoeViewLineNodeRor *n,
                                 const AoeViewLineOpsRor *ops)
{
    const int16_t *p = n->pt;
    int32_t y;
    int i;

    if (n->kind == 1) {
        for (i = 0; i < 4; i++)
            ops->draw_line(ops->context, p[i * 2], p[i * 2 + 1],
                           p[((i + 1) & 3) * 2], p[((i + 1) & 3) * 2 + 1],
                           n->colour);
        return;
    }
    if (n->kind == 2) {
        for (y = p[1]; y <= p[3]; y++) {
            if (p[5] != 0)
                ops->draw_line(ops->context, p[0], y, p[4], y, 0x4a);
            if (p[7] != 0)
                ops->draw_line(ops->context, p[6], y, p[2], y, 0x97);
        }
    }
    if (n->kind == 4) {
        ops->draw_line(ops->context, p[0], p[1], p[2], p[1], 0xff);
        ops->draw_line(ops->context, p[0], p[3], p[2], p[3], 0xff);
        ops->draw_line(ops->context, p[0], p[1], p[0], p[3], 0xff);
        ops->draw_line(ops->context, p[2], p[1], p[2], p[3], 0xff);
    }
}

void aoe_main_view_wall_run_ror(const AoeMainViewWallOpsRor *ops, int32_t x0, int32_t y0,
                                int32_t x1, int32_t y1) {
    int32_t dx = x0 == x1 ? 0 : (x1 <= x0 ? -1 : 1);
    int32_t dy = y0 == y1 ? 0 : (y1 <= y0 ? -1 : 1);
    int32_t x = x0, y = y0;
    for (;;) {
        float fx = (float)x + 0.5f, fy = (float)y + 0.5f;
        if (!ops->blocked(ops->context, fx, fy)) ops->place(ops->context, fx, fy);
        if (x == x1 && y == y1) break;
        x += dx;
        y += dy;
    }
}

int aoe_main_view_wall_drag_ror(const AoeMainViewWallOpsRor *ops, int32_t x0, int32_t y0,
                                int32_t x1, int32_t y1, int ack) {
    int result = 0;
    int32_t ax, ay;
    if (!ops->editing) return 0;
    if (ops->game_mode != 7) {
        if (ops->has_template) result = ops->command(ops->context, x0, y0, x1, y1) != 0;
        if (ack) ops->game_10(ops->context);
    } else if (ops->has_template) {
        ax = x1 - x0 < 0 ? x0 - x1 : x1 - x0;
        ay = y1 - y0 < 0 ? y0 - y1 : y1 - y0;
        if (ay == ax) {
            aoe_main_view_wall_run_ror(ops, x0, y0, x1, y1);
        } else if (ay < ax) {
            aoe_main_view_wall_run_ror(ops, x0, y0, x1, y0);
            if (y0 < y1) aoe_main_view_wall_run_ror(ops, x1, y0 + 1, x1, y1);
            else if (y1 < y0) aoe_main_view_wall_run_ror(ops, x1, y0 - 1, x1, y1);
        } else {
            aoe_main_view_wall_run_ror(ops, x0, y0, x0, y1);
            if (x0 < x1) aoe_main_view_wall_run_ror(ops, x0 + 1, y1, x1, y1);
            else if (x1 < x0) aoe_main_view_wall_run_ror(ops, x0 - 1, y1, x1, y1);
        }
        result = 1;
    }
    ops->clear_marks(ops->context);
    ops->redraw(ops->context, 1);
    return result;
}

/* 0x00509880 */
uint8_t aoe_game_view_unit_cursor_ror(uint8_t unit_kind, int16_t template_id,
                                      uint8_t unit_48, int is_target,
                                      int32_t mode)
{
    if (template_id == 0x4f || template_id == 199 || template_id == 0x45 ||
        template_id == 0x116)
        unit_kind = 3;
    if (unit_kind == 3 && !is_target && mode == 3)
        return unit_48 <= 1 ? 2 : 0;
    return aoe_main_view_unit_cursor_ror(unit_kind, is_target, mode);
}

/* 0x005099b0 */
int aoe_game_view_select_same_ror(const AoeGameViewSelectOpsRor *ops,
                                  int32_t x, int32_t y, int shift,
                                  int ctrl)
{
    AoeGameViewUnitRor hit, info;
    void *object;
    int8_t class_match;
    int32_t i, n;

    if (ops->game_state(ops->context) == 7)
        return 0;
    object = ops->pick_object(ops->context, x, y, &hit);
    if (!object || hit.kind != 4 || !hit.owned)
        return 0;
    if (!shift && !ctrl)
        ops->clear_selection(ops->context);
    ops->select(ops->context, object);
    class_match = hit.unit_class == 3 ? 3 : -1;
    n = ops->unit_count(ops->context);
    for (i = 0; i < n; i++) {
        void *unit = ops->unit_at(ops->context, i, &info);
        if ((info.template_id == hit.template_id ||
             (int8_t)info.unit_class == class_match) &&
            ops->on_screen(ops->context, info.id))
            ops->select(ops->context, unit);
    }
    ops->select(ops->context, object);
    return 1;
}

/* 0x0051a4d0 (0x00509990 is a thunk to it) */
uint8_t aoe_game_view_command_ror(const AoeGameViewCommandOpsRor *ops,
                                  int32_t origin_x, int32_t origin_y,
                                  int32_t x, int32_t y, int allow_move,
                                  int32_t arg)
{
    void *target;
    uint8_t result;

    if (!ops->orders_enabled(ops->context) ||
        !ops->has_selection(ops->context))
        return 0;
    target = ops->find_target(ops->context, x, y, arg);
    if (target) {
        result = ops->order_target(ops->context, target);
        if (result)
            ops->flash(ops->context, target);
        ops->clear_marks(ops->context);
        ops->redraw(ops->context, 1);
        return result;
    }
    if (allow_move) {
        float point[2] = {0.0f, 0.0f};
        if (ops->pick_map(ops->context, x, y, point) == '3') {
            ops->clamp_point(ops->context, point);
            result = ops->order_move(ops->context, point[0], point[1]);
            if (result) {
                ops->rewind_timers(ops->context, 2);
                ops->add_mark(ops->context, origin_x + x, origin_y + y);
                ops->redraw(ops->context, 1);
            }
            return result;
        }
    }
    ops->clear_marks(ops->context);
    ops->redraw(ops->context, 1);
    return 0;
}

/* ---- Tile object pick (C3L) ---- */

/* 0x005152f0 */
void *aoe_main_view_tile_object_ror(const AoeTilePickOpsRor *ops,
                                    int tile_present, void *const *objects,
                                    int32_t count, void *view_player,
                                    uint8_t pick_mode, uint8_t min_kind,
                                    int32_t x, int32_t y, const int16_t a[3],
                                    const int16_t b[3], int16_t *out_x,
                                    int16_t *out_y, void *preferred,
                                    int selectable_only)
{
    void *best = NULL, *obj, *ctx = ops->context;
    int32_t i, dx, dy;
    int16_t ox, oy;

    if (!tile_present)
        return NULL;
    for (i = 0; i < count && (obj = objects[i]) != NULL; i++) {
        if (ops->state(ctx, obj) >= 7 ||
            (selectable_only && !ops->selectable(ctx, obj)) ||
            ops->kind(ctx, obj) < min_kind)
            continue;
        if (pick_mode == '+' || pick_mode == ',') {
            if (ops->owner(ctx, obj) != view_player)
                continue;
            if (pick_mode == ',') {
                if (ops->hit_box(ctx, obj, x, y, a, b)) {
                    if (!ops->select_add(ctx, obj))
                        break;
                    best = obj;
                }
                continue;
            }
        }
        if (!ops->hit(ctx, obj, x, y, b, 5))
            continue;
        if (preferred != NULL && obj == preferred) {
            if (best != NULL)
                return best;
        } else if (best != NULL) {
            if (ops->owner(ctx, obj) != view_player &&
                ops->owner(ctx, best) == view_player)
                continue;
            if (ops->kind(ctx, obj) < ops->kind(ctx, best))
                continue;
        }
        best = obj;
        if (!ops->sprite_offset(ctx, obj, &dx, &dy))
            dx = dy = 0;
        ops->offset(ctx, obj, &ox, &oy);
        *out_x = (int16_t)(ox + dx + x);
        *out_y = (int16_t)(oy + dy + y);
    }
    return best;
}

/* 0x005151d0 */
int aoe_main_view_terrain_hit_ror(const AoeMainViewTerrainHitOpsRor *ops,
                                  const uint8_t *tile, int16_t left,
                                  int16_t top, int32_t tx, int32_t ty,
                                  const int32_t point[2])
{
    int16_t shape = 0, terrain = 0, frame;
    void *art;

    if (tile) {
        shape = tile[4];
        terrain = (int16_t)(tile[5] & 0x1f);
    }
    if (point[0] < left ||
        point[0] >= ops->shape_width(ops->context, shape) + left)
        return 0;
    if (point[1] < top ||
        point[1] >= ops->shape_height(ops->context, shape) + top)
        return 0;
    if (terrain >= ops->terrain_count || !ops->loaded(ops->context, terrain))
        return 1;
    art = ops->art(ops->context, terrain);
    frame = ops->frame(ops->context, terrain, shape, tx, ty);
    if (!art || frame == -1)
        return 1;
    return ops->pixel(ops->context, art, point[0] - left, point[1] - top,
                      frame) != 0;
}

/* 0x0051ae10 */
uint8_t aoe_game_view_alt_command_ror(const AoeGameViewCommandOpsRor *ops,
                                      int32_t origin_x, int32_t origin_y,
                                      int32_t x, int32_t y)
{
    void *target;
    uint8_t result = 0;
    float point[2] = {0.0f, 0.0f};

    if (!ops->orders_enabled(ops->context) ||
        !ops->has_selection(ops->context))
        return 0;
    target = ops->find_target(ops->context, x, y, -1);
    if (target) {
        result = ops->order_target(ops->context, target);
        if (result)
            ops->flash(ops->context, target);
        ops->clear_marks(ops->context);
    } else if (ops->pick_map(ops->context, x, y, point) == '3') {
        ops->clamp_point(ops->context, point);
        result = ops->order_move(ops->context, point[0], point[1]);
        if (result) {
            ops->rewind_timers(ops->context, 2);
            ops->add_mark(ops->context, origin_x + x, origin_y + y);
        }
    } else {
        ops->clear_marks(ops->context);
    }
    ops->redraw(ops->context, 1);
    ops->set_game_mode(ops->context, 0, 0);
    return result;
}

int aoe_main_view_place_click_ror(const AoeMainViewPlaceOpsRor *ops, int32_t x, int32_t y,
                                  int32_t ack) {
    float point[2] = {0.0f, 0.0f};
    if (!ops->editing) return 0;
    if (ops->mode_1b4 != 7) return ops->base_click(ops->context, x, y, 1);
    if (ops->pick(ops->context, x, y, point) != '3') return 0;
    ops->snap(ops->context, point);
    if (ops->template_id == -1 || !ops->has_template) return 0;
    ops->adjust(ops->context, point);
    if (ops->blocked(ops->context, point[0], point[1])) return 0;
    if (ack || ops->template_id == 0x6d) ops->game_10(ops->context);
    return ops->build(ops->context, ops->template_id, point[0], point[1]) != 0;
}

int32_t aoe_view_collect_nearby_ror(const AoeViewClassListsRor *l, int32_t cx, int32_t cy,
                                    int32_t radius, int32_t player, const int32_t *classes,
                                    int32_t class_count, const uint8_t kind_bucket[256],
                                    AoeViewNearbyRor *out) {
    int32_t added = 0, i;
    if (!l || !out || class_count < 1 || player >= l->player_count) return 0;
    if (radius > 15) radius = 15;
    memset(out->count, 0, sizeof out->count);
    for (i = 0; i < class_count; ++i) {
        const AoeViewClassListRor *cl = l->list(l->context, player, classes[i]);
        uint32_t n;
        if (!cl) continue;
        for (n = 0; n < cl->count; ++n) {
            const AoeViewClassNodeRor *node = &cl->nodes[n];
            int32_t dx = node->x, dy = node->y, d, b;
            if (dx > cx + radius) continue;
            if (dx < cx) { if (dx < cx - radius) continue; dx = cx - dx; } else dx -= cx;
            if (dy > cy + radius) continue;
            if (dy < cy) { if (dy < cy - radius) continue; dy = cy - dy; } else dy -= cy;
            d = l->distance[dy & 0xf][dx & 0xf];
            if (d > radius) continue;
            b = kind_bucket[node->kind];
            if (b < 0 || b >= AOE_VIEW_NEARBY_BUCKETS || out->count[b] >= AOE_VIEW_NEARBY_MAX)
                continue;
            out->entry[b][out->count[b]] = *node;
            out->entry[b][out->count[b]].byte_4 = (uint8_t)d;
            ++out->count[b];
            ++added;
        }
    }
    return added;
}

/* ---- 0x0050fbd0 / 0x0050fa10 / 0x00508bb0 ---- */

int aoe_main_view_rebuild_overlay_ror(AoeMainViewRor *v, AoeMainViewBuffersRor *b,
                                      const AoeMainViewSetupOpsRor *o) {
    int32_t h, rows, k;
    if (!v || !b || !o) return 0;
    if (b->width == 0 && b->height == 0) return 1;
    if (!v->overlay) {
        v->overlay = o->surface_create(o->context, "Terrain Buffer");
        if (v->overlay) o->surface_init(o->context, v->overlay, v->surface, b->width, b->height);
    }
    h = b->height;
    if (v->overlay && h > 0) {
        o->surface_clear(o->context, v->overlay, b->width, h);
        b->blit_origin_x = b->blit_origin_y = 0;
        o->surface_set_origin(o->context, v->overlay, 0, 0);
        rows = h + 0x1c2;
        for (k = 0; k < 2; ++k) {
            if (b->draw_rows[k]) o->draw_rows_free(o->context, b->draw_rows[k]);
            b->draw_rows[k] = o->draw_rows_new(o->context, rows * 2, rows, 0x40, -200, h + 0xf9, 0x28);
        }
        for (k = 0; k < 2; ++k) o->dirty(o->context, b->draw_rows[k], 0, 0, b->width - 1, h - 1);
    }
    return v->overlay != NULL;
}

void aoe_main_view_setup_ror(AoeMainViewRor *v, AoeMainViewBuffersRor *b,
                             const AoeMainViewSetupOpsRor *o, int32_t x, int32_t y, int32_t w,
                             int32_t h) {
    int k;
    if (!v || !b || !o) return;
    o->base_setup(o->context, x, y, w, h);
    b->width = w;
    b->height = h;
    o->layout(o->context);
    aoe_main_view_rebuild_overlay_ror(v, b, o);
    v->point_x = v->point_y = AOE_MAIN_VIEW_NO_POINT_ROR;
    for (k = 0; k < 3; ++k) {
        if (b->regions[k]) o->region_free(o->context, b->regions[k]);
        b->regions[k] = o->region_new(o->context, w, h);
    }
    if (o->game_region_enabled(o->context) && w > 0 && h > 0) {
        o->game_region(o->context, x, y, x + w, y + h);
        o->pointer_switch(o->context, 1);
    }
    o->redraw(o->context, 2);
}

int aoe_main_view_place_preview_ror(const AoeMainViewPreviewOpsRor *o, int32_t x, int32_t y,
                                    int32_t *cursor_code, int32_t *preview_y) {
    int32_t tx, ty;
    int16_t c[8];
    float fx, fy;
    if (!o) return 0;
    if (o->tile_at(o->context, x, y, &tx, &ty)) {
        fx = (float)tx + 0.5f;
        fy = (float)ty + 0.5f;
        o->corners(o->context, fx, fy, c);
        if (!o->can_place(o->context, fx, fy)) {
            *cursor_code = 5;
        } else {
            uint32_t v = (o->template_flags(o->context) >> 7) & 7;
            *cursor_code = (int32_t)(((v < 4 ? v + 0x24 : 0x2c - v) << 4) | 9);
        }
        if (o->template_has_graphic(o->context)) {
            int32_t cx = 1 + (c[4] - c[0]) / 2 + c[0];
            int32_t cy = 1 + (c[5] - c[1]) / 2 + c[1];
            *preview_y = (int16_t)cy;
            o->draw_preview(o->context, cx, cy);
        }
    }
    *cursor_code = 0;
    return 1;
}

/* 0x00511080: editor/game-mouse placement preview (see main_view.h). */
int aoe_main_view_editor_preview(const AoeMainViewEditorPreviewOps *o, int32_t left,
                                 int32_t top, int32_t right, int32_t bottom,
                                 int32_t *cursor_code, int32_t *preview_y) {
    int32_t mx, my, tx, ty;
    int16_t c[8];
    float fx, fy;
    int ok;
    if (!o || !o->has_template(o->context)) return 0;
    o->mouse(o->context, &mx, &my);
    if (mx < left || mx > right || my < top || my > bottom) return 0;
    if (!o->pick_terrain(o->context, mx - left, my - top)) return 0;
    if (!o->tile_at(o->context, mx - left, my - top, &tx, &ty)) return 0;
    fx = (float)tx + 0.5f;
    fy = (float)ty + 0.5f;
    o->corners(o->context, fx, fy, c);
    ok = o->can_place(o->context, fx, fy);
    if (!ok) {
        *cursor_code = 5;
    } else {
        uint32_t v = (o->template_flags(o->context) >> 7) & 7;
        *cursor_code = (int32_t)(((v < 4 ? v + 0x24 : 0x2c - v) << 4) | 9);
    }
    if (o->template_has_graphic(o->context)) {
        int32_t cx = 1 + (c[4] - c[0]) / 2 + c[0];
        int32_t cy = 1 + (c[5] - c[1]) / 2 + c[1];
        uint32_t facing = o->template_facing(o->context) & 1u;
        uint32_t shape = ok ? 0x97u : o->template_shape(o->context);
        *preview_y = (int16_t)cy;
        o->draw_preview(o->context, cx, cy, o->relocating(o->context), facing, shape);
    }
    *cursor_code = 0;
    return 1;
}

/* 0x00519ed0: game view right-click (see main_view.h). */
int aoe_main_view_right_click(const AoeMainViewRightClickOps *o, int32_t x, int32_t y,
                              int32_t unit) {
    if (o->mode < 4 || o->mode > 7) return 0;
    if (o->drag_pending) {
        o->cancel_drag(o->context);
        return 1;
    }
    o->prepare(o->context);
    if (o->sub_mode == 0x14) {
        if (o->pick_object(o->context, x, y)) o->object_notify(o->context);
        return 1;
    }
    if (o->busy(o->context)) return 0;
    if (o->selection_kind != 1 && o->selection_kind != 2) return 0;
    if (o->full_player) {
        if (o->mode == 5 || o->mode == 7) return 1;
        if (o->sub_mode == 0) {
            if (unit == 0) {
                o->default_order(o->context, x, y);
                return 1;
            }
            if (o->find_unit(o->context, unit) && o->rally_enabled &&
                o->pick_terrain(o->context, x, y)) {
                o->set_rally(o->context);
                o->redraw(o->context);
            }
            return 1;
        }
        if (o->sub_mode == 0x15) {
            o->clear_box(o->context);
            o->redraw(o->context);
        }
        o->reset_mode(o->context);
        return 1;
    }
    if (o->mode != 7) {
        if (o->sub_mode != 0) {
            o->reset_mode(o->context);
            return 1;
        }
        o->cancel_selection(o->context);
        o->redraw(o->context);
    }
    return 1;
}

int aoe_main_view_queue_scroll_ror(AoeMainViewRor *v, AoeMainViewBlitQueueRor *q,
                                   const int32_t src[4], const int32_t clip[4], int32_t dx,
                                   int32_t dy) {
    int32_t n, disjoint = 0, ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
    int32_t steps_x = 9999, steps_y = 9999, sx = 0, sy = 0;
    int32_t l = src[0], t = src[1], r = src[2], b = src[3];
    if (!v || !q || (dx == 0 && dy == 0)) return 0;
    if (clip[3] < src[1] || clip[2] < src[0] || src[3] < clip[1] || src[2] < clip[0]) {
        disjoint = 1;
        n = 1;
    } else {
        if (dy) steps_y = (ay + (src[3] - src[1])) / ay;
        if (dx) steps_x = ((src[2] - src[0]) + ax) / ax;
        if (steps_y < steps_x) {
            n = steps_y;
            sy = dy;
            if (dy < 0) t = src[3] + 1 + dy;
        } else {
            n = steps_x;
            sx = dx;
            if (dx < 0) l = src[2] + 1 + dx; else r = dx - 1 + src[0];
        }
    }
    if (q->capacity < n) {
        free(q->list);
        q->list = NULL;
    }
    if (!q->list) {
        q->list = calloc((size_t)n, sizeof *q->list);
        if (!q->list) return 0;
        q->capacity = n;
    }
    if (disjoint) {
        q->list[0].left = src[0]; q->list[0].top = src[1]; q->list[0].right = src[2]; q->list[0].bottom = src[3];
        q->list[0].dest_x = clip[0]; q->list[0].dest_y = clip[1];
        q->list[0].dest_right = clip[2]; q->list[0].dest_bottom = clip[3];
    } else {
        int32_t i;
        for (i = 0; i < n; ++i) {
            AoeMainViewBlitRor *e = &q->list[i];
            e->left = l; e->top = t; e->right = r; e->bottom = b;
            e->dest_x = l - dx; e->dest_y = t - dy; e->dest_right = r - dx; e->dest_bottom = b - dy;
            t += sy; l += sx; r += sx;
            if (t < src[1]) t = src[1];
            if (l < src[0]) l = src[0];
            if (r > src[2]) r = src[2];
        }
    }
    v->blits_active = 1;
    v->blits = q->list;
    v->blit_count = n;
    v->blit_index = -1;
    v->blit_off_x = v->left;
    v->blit_off_y = v->top;
    return n;
}

/* 0x0050eff0 */
void aoe_game_view_construct_default_ror(AoeGameViewDefaultsRor *v,
                                         const AoeGameViewDefaultOpsRor *ops) {
    ops->base_construct(ops->context);
    memset(v, 0, sizeof *v);
    v->point_x = v->point_y = AOE_MAIN_VIEW_NO_POINT_ROR;
    v->field_154 = -1;
    v->field_16c = v->field_170 = -1;
    ops->set_highlight(ops->context, -1, -1, -1, -1);
    v->field_340 = INT32_MIN;
    v->timer_capacity = 8;
    v->timers = ops->alloc_zero(ops->context, 8, 0x14);
    if (ops->globals[0]) *ops->globals[0] = 0;
    if (ops->globals[1]) *ops->globals[1] = 0;
}

/* 0x005094d0 */
int32_t aoe_view_edit_click_ror(const AoeViewEditClickOpsRor *o, int32_t x, int32_t y, int32_t a,
                                int32_t b) {
    int32_t px, py, id;
    int ok = 0;
    if (o->game_state < 4 || o->game_state > 7) return 0;
    if (o->comm_value) return o->base_click(o->context, x, y, a, b);
    switch (o->edit_mode) {
    case 100: case 0x65: case 0x66: case 0x67: case 0x68: case 0x69: break;
    default: return o->base_click(o->context, x, y, a, b);
    }
    o->step(o->context, AOE_VIEW_CLICK_RELEASE, 0, 0);
    if (o->mode_ec != 1 && o->mode_ec != 2) return 1;
    switch (o->edit_mode) {
    case 100: case 0x69:
        if (o->point_at(o->context, x, y, &px, &py)) {
            o->clamp(o->context, &px, &py);
            ok = o->edit_mode == 100 ? o->command_6d(o->context, px, py) : o->command_6f(o->context, px, py);
            if (ok) {
                o->step(o->context, AOE_VIEW_CLICK_REWIND, 0, 0);
                o->step(o->context, AOE_VIEW_CLICK_MARK, o->origin_x + x, o->origin_y + y);
                o->step(o->context, AOE_VIEW_CLICK_REDRAW, 0, 0);
            }
        }
        break;
    case 0x65: ok = o->object_command(o->context, x, y, 0x68); break;
    case 0x66: ok = o->object_command(o->context, x, y, 0x69); break;
    case 0x67: ok = o->object_command(o->context, x, y, 7); break;
    default:
        id = o->object_at(o->context, x, y);
        if (id != -1 && o->repair(o->context, id)) {
            ok = 1;
            o->step(o->context, AOE_VIEW_CLICK_FLASH, id, 0);
            o->step(o->context, AOE_VIEW_CLICK_FREE_MARKS, 0, 0);
            o->step(o->context, AOE_VIEW_CLICK_REDRAW, 0, 0);
        }
        break;
    }
    if (!ok) o->step(o->context, AOE_VIEW_CLICK_REFUSE, 0, 0);
    o->step(o->context, AOE_VIEW_CLICK_GAME_10, 0, 0);
    return 1;
}
