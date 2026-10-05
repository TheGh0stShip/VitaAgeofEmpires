/* Patched RoR 1.0a diamond map base (0x0042c360..0x0042e1f0). */
#include "diamond_map.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int32_t w, h; uint8_t *bits; } MemSurface;
static int32_t mem_create(void *c, void *d, int32_t w, int32_t h, int sys, void **out) {
    MemSurface *m = calloc(1, sizeof *m);
    (void)c; (void)d; (void)sys;
    m->w = w; m->h = h;
    m->bits = calloc((size_t)w, (size_t)h);
    *out = m;
    return 0;
}
static int32_t mem_describe(void *c, void *s, int32_t *pitch, uint32_t *caps) {
    (void)c;
    *pitch = ((MemSurface *)s)->w;
    *caps = 0x800;
    return 0;
}
static int32_t mem_fill(void *c, void *s, const int32_t *r, uint32_t color) {
    MemSurface *m = s;
    (void)c;
    if (!r) memset(m->bits, (int)color, (size_t)m->w * (size_t)m->h);
    return 0;
}
static int32_t mem_lock(void *c, void *s, int wait, uint8_t **bits) {
    (void)c; (void)wait;
    *bits = ((MemSurface *)s)->bits;
    return 0;
}
static void mem_unlock(void *c, void *s, uint8_t *bits) { (void)c; (void)s; (void)bits; }
static void mem_release(void *c, void *o) {
    (void)c;
    free(((MemSurface *)o)->bits);
    free(o);
}
static int blits;
static int32_t mem_blt(void *c, void *d, const int32_t dr[4], void *s, const int32_t sr[4],
                       int keyed) {
    (void)c; (void)d; (void)dr; (void)s; (void)sr; (void)keyed;
    ++blits;
    return 0;
}

/* The diamond: rows of 0xff 2, 4, 6, 8, 6, 4, 2 wide, centred in 8. */
static uint8_t *load_bitmap(void *c, const char *name, int32_t id, int32_t *w, int32_t *h) {
    static const int widths[7] = {2, 4, 6, 8, 6, 4, 2};
    uint8_t *b;
    (void)c;
    assert(strcmp(name, "map.bmp") == 0 && id == 0xc4e1);
    *w = 8;
    *h = 9; /* a blank row above and below */
    b = calloc(8 * 9, 1);
    for (int y = 0; y < 7; ++y)
        memset(b + (y + 1) * 8 + (8 - widths[y]) / 2, 0xff, (size_t)widths[y]);
    return b;
}
static int32_t map_w(void *c) { (void)c; return 4; }
static int32_t map_h(void *c) { (void)c; return 4; }
static uint8_t shape(void *c, int32_t x, int32_t y) { (void)c; return x == 3 && y == 3 ? 2 : 0; }
static uint8_t terrain(void *c, int32_t x, int32_t y) { (void)c; (void)x; (void)y; return 0x21; }
static uint8_t terrain_color(void *c, uint8_t t, int which) {
    (void)c;
    assert(t == 1);
    return (uint8_t)(0x40 + which);
}
static int all_visible_flag = 1;
static int all_visible(void *c) { (void)c; return all_visible_flag; }
static int keep_explored(void *c) { (void)c; return 0; }
static int feature(void *c, int32_t x, int32_t y, uint8_t *color) {
    (void)c;
    if (x == 0 && y == 3) { *color = 0x77; return 1; }
    return 0;
}
static uint32_t visibility(void *c, int32_t x, int32_t y) { (void)c; return x == 0 ? 1 : 0; }
static uint32_t explored(void *c, const void *p) { (void)c; (void)p; return 1; }
static uint32_t visible(void *c, const void *p) { (void)c; (void)p; return 1; }
static int fog_on;
static int fog(void *c, const void *p) { (void)c; (void)p; return fog_on; }
static int32_t shared(void *c, const void *p, int16_t n) { (void)c; (void)p; (void)n; return 0; }
static uint8_t player_color(void *c, const void *p) { (void)c; (void)p; return 0x55; }
static float view_x = 1.5f, view_y = 1.5f;
static void view_center(void *c, const void *p, float *x, float *y) { (void)c; (void)p; *x = view_x; *y = view_y; }
static void set_view_center(void *c, const void *p, float x, float y) { (void)c; (void)p; view_x = x; view_y = y; }
static int32_t viewed_index(void *c) { (void)c; return 1; }
static int32_t edit_mode(void *c) { (void)c; return 0; }
static void view_size(void *c, int16_t *w, int16_t *h) { (void)c; *w = 2; *h = 1; }
static char world_token, player_token, gaia_token;
static char unit_token;
static void *unit_list[1] = {&unit_token};
static int32_t player_count(void *c) { (void)c; return 2; }
static const void *world_player(void *c, int32_t i) { (void)c; return i == 0 ? (const void *)&gaia_token : &player_token; }
static int32_t objects(void *c, const void *p, int doppel, void ***out) {
    (void)c;
    *out = unit_list;
    return p == &player_token && !doppel ? 1 : 0;
}
static int drawn;
static int read_object(void *c, void *o, AoeDiamondObjectRor *out) {
    (void)c;
    assert(o == &unit_token);
    memset(out, 0, sizeof *out);
    out->has_sprite = 1;
    out->x = 3.2f;
    out->y = 3.9f;
    out->owner = &player_token;
    out->map_mode = 1;
    ++drawn;
    return 1;
}

int main(void) {
    static AoeUiSystemRor sys;
    AoeUiPlatformRor pf = {0};
    AoeDiamondMapOpsRor ops = {0};
    static AoeDrawSystemRor d;
    static AoeDrawSurfaceRor surf;
    static const AoeDrawOpsRor mops = {.create_surface = mem_create,
                                       .describe = mem_describe,
                                       .fill = mem_fill,
                                       .lock = mem_lock,
                                       .unlock = mem_unlock,
                                       .release = mem_release,
                                       .blt = mem_blt};
    AoeUiAreaRor area = {(void *)1, 200, 100, &surf};
    static AoeDiamondMapRor m;
    AoeUiPanelRor *p = &m.panel;
    MemSurface *area_bits, *buf;
    int16_t tx, ty;
    sys.platform = &pf;
    ops.map_width = map_w;
    ops.map_height = map_h;
    ops.tile_shape = shape;
    ops.tile_terrain = terrain;
    ops.terrain_color = terrain_color;
    ops.all_visible = all_visible;
    ops.keep_explored = keep_explored;
    ops.tile_feature = feature;
    ops.visibility = visibility;
    ops.explored_mask = explored;
    ops.visible_mask = visible;
    ops.fog = fog;
    ops.shared_vision = shared;
    ops.player_map_color = player_color;
    ops.view_center = view_center;
    ops.set_view_center = set_view_center;
    ops.viewed_index = viewed_index;
    ops.edit_mode = edit_mode;
    ops.view_size = view_size;
    ops.load_bitmap = load_bitmap;
    ops.player_count = player_count;
    ops.world_player = world_player;
    ops.objects = objects;
    ops.read_object = read_object;
    d.type = 2;
    d.device = (void *)1;
    d.ops = &mops;
    aoe_surface_init_ror(&surf, "area", 0);
    aoe_surface_attach_ror(&surf, &d, 200, 100, 0, 0);
    area_bits = surf.surface;

    aoe_diamond_map_init_ror(&m, &sys, &ops);
    assert(m.full_redraw == 1 && m.select_color == 0xff && m.frame_x == -1);
    assert(aoe_diamond_map_setup_ror(&m, &area, NULL, 100, 50, 8, 9, 0, NULL));
    assert(aoe_diamond_map_make_buffer_ror(&m) && m.owns_buffer && m.buffer->view.width == 8);
    buf = m.buffer->surface;
    assert(aoe_diamond_map_load_ror(&m, "map.bmp", 0xc4e1));
    assert(m.row_count == 7 && m.first_row == 1 && m.widest == 8);
    assert(m.rows[0].x == 3 && m.rows[0].width == 2 && m.rows[3].x == 0 && m.rows[3].width == 8);
    aoe_diamond_map_set_player_ror(&m, &player_token);
    aoe_diamond_map_set_world_ror(&m, &world_token);
    /* Seven diagonals on seven rows: one each, two pixels a tile. */
    assert(m.diagonal_count == 7 && m.diagonals_per_row == 1.0f && m.row_height == 1 &&
           m.mean_pixels == 2 && m.pixels_per_diagonal == 2.0f && m.rows_per_diagonal == 1.0f);
    for (int i = 0; i < 7; ++i) {
        AoeDiamondDiagonalRor *g = &m.diagonals[i];
        assert(g->valid && g->row == i && m.rows[i].diagonal == i);
        assert(g->pixels_ceil == 2 && g->pixels_per_tile == 2.0f && g->half == 0.0f);
    }
    assert(m.diagonals[3].tiles == 4 && m.diagonals[6].tiles == 1);
    assert(m.diagonals[0].first_x == 3 && m.diagonals[0].first_y == 0 &&
           m.diagonals[5].first_x == 0 && m.diagonals[5].first_y == 2);

    /* Screen points to tiles and back. */
    assert(aoe_diamond_map_tile_at_ror(&m, 103, 54, &tx, &ty) && tx == 1 && ty == 1);
    assert(aoe_diamond_map_tile_at_ror(&m, 100, 54, &tx, &ty) && tx == 0 && ty == 0);
    assert(aoe_diamond_map_tile_at_ror(&m, 107, 54, &tx, &ty) && tx == 3 && ty == 3);
    assert(!aoe_diamond_map_tile_at_ror(&m, 100, 51, NULL, NULL)); /* left of row 0 */
    assert(!aoe_diamond_map_tile_at_ror(&m, 103, 50, NULL, NULL)); /* above */
    assert(p->vt->is_inside(p, 103, 54) && !p->vt->is_inside(p, 100, 51));

    /* Tiles: (1, 1) is diagonal 3, pixels (2, 4) and (3, 4) of the buffer
       in the middle colour; (3, 3) has shape 2 (dark); (0, 2) borders the
       mode-4 object at (0, 3). */
    assert(aoe_surface_lock_ror(m.buffer, 1));
    aoe_diamond_map_draw_tile_ror(&m, 1, 1, 0, 0);
    aoe_diamond_map_draw_tile_ror(&m, 3, 3, 0, 0);
    aoe_diamond_map_draw_tile_ror(&m, 0, 2, 0, 0);
    aoe_surface_unlock_ror(m.buffer);
    assert(buf->bits[4 * 8 + 2] == 0x41 && buf->bits[4 * 8 + 3] == 0x41);
    assert(buf->bits[4 * 8 + 6] == 0x40 && buf->bits[4 * 8 + 7] == 0x40);
    assert(buf->bits[6 * 8 + 2] == 0x77);
    /* Unexplored tiles are skipped when the map is not all visible. */
    all_visible_flag = 0;
    memset(buf->bits, 0, 72);
    assert(aoe_surface_lock_ror(m.buffer, 1));
    aoe_diamond_map_draw_tile_ror(&m, 1, 1, 0, 0);
    aoe_diamond_map_draw_tile_ror(&m, 0, 1, 0, 0);
    aoe_surface_unlock_ror(m.buffer);
    assert(buf->bits[4 * 8 + 2] == 0 && buf->bits[5 * 8 + 1] == 0x41);
    all_visible_flag = 1;

    /* Dots on the area. */
    assert(aoe_surface_lock_ror(&surf, 1));
    aoe_diamond_map_draw_dot_ror(&m, 1, 1, 0x99, 1, NULL);
    assert(area_bits->bits[54 * 200 + 101] == 0x99 && area_bits->bits[55 * 200 + 103] == 0x99 &&
           area_bits->bits[53 * 200 + 102] == 0x99 && area_bits->bits[56 * 200 + 102] == 0);
    aoe_diamond_map_draw_dot_ror(&m, 3, 3, 0x88, 2, NULL);
    assert(area_bits->bits[54 * 200 + 106] == 0x88 && area_bits->bits[53 * 200 + 106] == 0x88 &&
           area_bits->bits[53 * 200 + 105] == 0);
    /* The view frame round (1.5, 1.5) with a 2 x 1 tile view: top 53,
       bottom 59, left 100, right 108; the top row is cut to the diamond,
       the sides plotted where they are on it. */
    memset(area_bits->bits, 0, 200 * 100);
    aoe_diamond_map_draw_view_frame_ror(&m);
    aoe_surface_unlock_ror(&surf);
    assert(m.frame[0] == 100 && m.frame[1] == 53 && m.frame[2] == 108 && m.frame[3] == 59);
    assert(area_bits->bits[53 * 200 + 101] == 0xff && area_bits->bits[53 * 200 + 106] == 0xff &&
           area_bits->bits[53 * 200 + 100] == 0 && area_bits->bits[53 * 200 + 107] == 0);
    assert(area_bits->bits[54 * 200 + 100] == 0xff && area_bits->bits[54 * 200 + 108] == 0);
    /* Row 55 runs 101..106: neither side is on it. */
    assert(area_bits->bits[55 * 200 + 100] == 0 && area_bits->bits[55 * 200 + 104] == 0);
    assert(m.frame_x == 1 && m.frame_y == 1);

    /* Pressing centres the view on the tile. */
    aoe_diamond_map_press_ror(&m, 107, 54);
    assert(view_x == 3.5f && view_y == 3.5f && m.dragging && m.mouse_x == 107);
    aoe_diamond_map_drag_ror(&m, 100, 54);
    assert(view_x == 0.5f && view_y == 0.5f);
    aoe_diamond_map_release_drag_ror(&m);
    assert(!m.dragging);

    /* A full draw: tiles into the buffer, copied out, full flag cleared. */
    blits = 0;
    p->vt->set_redraw(p, 2);
    assert(m.full_redraw == 1);
    p->vt->draw(p);
    assert(m.full_redraw == 0 && blits >= 1 && m.player_mask == 2);
    /* The player's unit, read for the player pass and again for the
       drawn-last pass (not drawn last, so only once on the map). */
    /* Its 3x3 box round (106, 54) in the player's colour, under the view
       frame (now round tile (0, 0): its right side is column 106). */
    assert(drawn >= 2 && area_bits->bits[55 * 200 + 105] == 0x55 &&
           area_bits->bits[54 * 200 + 107] == 0x55 && area_bits->bits[54 * 200 + 106] == 0xff);

    p->vt->destroy(p);
    aoe_surface_free_ror(&surf);
    puts("diamond map ok");
    return 0;
}
