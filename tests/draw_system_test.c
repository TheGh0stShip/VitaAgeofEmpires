#include "draw_system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #cond);     \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

/* A fake device whose surfaces are plain memory. */
typedef struct {
    int32_t w, h, pitch;
    uint8_t *bits;
    int lost;
} FakeSurface;

static int dev, pal, clip;
static int32_t desk_bits = 8, mode_w = 640, mode_h = 480;
static int primaries, releases, fills, blts, lost_restores;
static int32_t last_dst[4], last_src[4], last_keyed;
static uint8_t last_palette[1024];
static int span_calls;
static int32_t spans[64][3];

static int32_t create(void *c, void **d) {
    (void)c;
    *d = &dev;
    return 0;
}
static void release(void *c, void *o) {
    (void)c;
    ++releases;
    if (o != &dev && o != &pal && o != &clip) {
        free(((FakeSurface *)o)->bits);
        free(o);
    }
}
static int32_t set_level(void *c, void *d, void *w, uint32_t f) {
    (void)c;
    (void)d;
    (void)w;
    (void)f;
    return 0;
}
static int32_t get_mode(void *c, void *d, int32_t *w, int32_t *h, int32_t *b) {
    (void)c;
    (void)d;
    *w = mode_w;
    *h = mode_h;
    *b = desk_bits;
    return 0;
}
static int32_t set_mode(void *c, void *d, int32_t w, int32_t h, int32_t b) {
    (void)c;
    (void)d;
    (void)b;
    mode_w = w;
    mode_h = h;
    return 0;
}
static void restore_mode(void *c, void *d) {
    (void)c;
    (void)d;
}
static void enum_modes(void *c, void *d,
                       void (*each)(void *, int32_t, int32_t, int32_t),
                       void *user) {
    (void)c;
    (void)d;
    each(user, 640, 480, 8);
    each(user, 800, 600, 16);
    each(user, 320, 200, 8);
}
static int32_t get_caps(void *c, void *d, uint32_t *caps, uint32_t *key) {
    (void)c;
    (void)d;
    *caps = 0x400000u;
    *key = 0x200u;
    return 0;
}
static int32_t desktop_width(void *c) {
    (void)c;
    return 1024;
}
static int32_t create_palette(void *c, void *d, const uint8_t e[1024],
                              void **p) {
    (void)c;
    (void)d;
    memcpy(last_palette, e, 1024);
    *p = &pal;
    return 0;
}
static int32_t palette_get(void *c, void *p, int32_t s, int32_t n,
                           uint8_t *out) {
    (void)c;
    (void)p;
    memcpy(out, last_palette + s * 4, (size_t)n * 4);
    return 0;
}
static int32_t palette_set(void *c, void *p, int32_t s, int32_t n,
                           const uint8_t *in) {
    (void)c;
    (void)p;
    memcpy(last_palette + s * 4, in, (size_t)n * 4);
    return 0;
}
static int window_palette(void *c, void *h, uint8_t out[1024]) {
    (void)c;
    (void)h;
    memset(out, 7, 1024);
    return 1;
}
static int32_t create_clipper(void *c, void *d, void **cl) {
    (void)c;
    (void)d;
    *cl = &clip;
    return 0;
}
static int32_t clipper_window(void *c, void *cl, void *w) {
    (void)c;
    (void)cl;
    (void)w;
    return 0;
}
static FakeSurface *make(int32_t w, int32_t h) {
    FakeSurface *f = calloc(1, sizeof *f);
    f->w = w;
    f->h = h;
    f->pitch = (w + 3) & ~3;
    f->bits = calloc((size_t)f->pitch, (size_t)h);
    return f;
}
static int32_t create_primary(void *c, void *d, void **s) {
    (void)c;
    (void)d;
    ++primaries;
    *s = make(mode_w, mode_h);
    return 0;
}
static int32_t attach(void *c, void *s, void *o) {
    (void)c;
    (void)s;
    (void)o;
    return 0;
}
static int32_t create_surface(void *c, void *d, int32_t w, int32_t h,
                              int sys, void **s) {
    (void)c;
    (void)d;
    (void)sys;
    *s = make(w, h);
    return 0;
}
static int32_t describe(void *c, void *s, int32_t *pitch, uint32_t *caps) {
    (void)c;
    *pitch = ((FakeSurface *)s)->pitch;
    *caps = 0x800;
    return 0;
}
static int32_t fill(void *c, void *s, const int32_t *r, uint32_t color) {
    FakeSurface *f = s;
    int32_t x, y;
    (void)c;
    ++fills;
    for (y = r ? r[1] : 0; y < (r ? r[3] : f->h); ++y)
        for (x = r ? r[0] : 0; x < (r ? r[2] : f->w); ++x)
            f->bits[y * f->pitch + x] = (uint8_t)color;
    return 0;
}
static int32_t blt(void *c, void *d, const int32_t dr[4], void *s,
                   const int32_t sr[4], int keyed) {
    (void)c;
    (void)d;
    (void)s;
    ++blts;
    memcpy(last_dst, dr, sizeof last_dst);
    memcpy(last_src, sr, sizeof last_src);
    last_keyed = keyed;
    return 0;
}
static int32_t is_lost(void *c, void *s) {
    (void)c;
    return ((FakeSurface *)s)->lost ? (int32_t)0x887601C2u : 0;
}
static int32_t restore(void *c, void *s) {
    (void)c;
    ((FakeSurface *)s)->lost = 0;
    ++lost_restores;
    return 0;
}
static int32_t lock(void *c, void *s, int wait, uint8_t **bits) {
    (void)c;
    (void)wait;
    *bits = ((FakeSurface *)s)->bits;
    return 0;
}
static void unlock(void *c, void *s, uint8_t *bits) {
    (void)c;
    (void)s;
    (void)bits;
}
static int32_t get_dc(void *c, void *s, void **dc) {
    (void)c;
    *dc = s;
    return 0;
}
static void release_dc(void *c, void *s, void *dc) {
    (void)c;
    (void)s;
    (void)dc;
}
static int32_t set_key(void *c, void *s, uint8_t k) {
    (void)c;
    (void)s;
    (void)k;
    return 0;
}
static void client_size(void *c, void *w, int32_t *x, int32_t *y) {
    (void)c;
    (void)w;
    *x = 320;
    *y = 200;
}
static void client_origin(void *c, void *w, int32_t *x, int32_t *y) {
    (void)c;
    (void)w;
    *x = 10;
    *y = 20;
}
static void span_fill(void *c, int32_t x0, int32_t x1, int32_t y,
                      uint8_t color) {
    (void)c;
    (void)color;
    if (span_calls < 64) {
        spans[span_calls][0] = x0;
        spans[span_calls][1] = x1;
        spans[span_calls][2] = y;
    }
    ++span_calls;
}

static const AoeDrawOpsRor ops = {
    .create = create,
    .release = release,
    .set_level = set_level,
    .get_mode = get_mode,
    .set_mode = set_mode,
    .restore_mode = restore_mode,
    .enum_modes = enum_modes,
    .get_caps = get_caps,
    .desktop_width = desktop_width,
    .create_palette = create_palette,
    .palette_get = palette_get,
    .palette_set = palette_set,
    .window_palette = window_palette,
    .create_clipper = create_clipper,
    .clipper_window = clipper_window,
    .create_primary = create_primary,
    .attach_clipper = attach,
    .attach_palette = attach,
    .create_surface = create_surface,
    .describe = describe,
    .fill = fill,
    .blt = blt,
    .is_lost = is_lost,
    .restore = restore,
    .lock = lock,
    .unlock = unlock,
    .get_dc = get_dc,
    .release_dc = release_dc,
    .set_key = set_key,
    .client_size = client_size,
    .client_origin = client_origin,
    .span_fill = span_fill,
};

static void test_messages(void) {
    CHECK(aoe_draw_lost_message_ror(0) == 0);
    CHECK(aoe_draw_lost_message_ror((int32_t)0x887601C2u) == 0x6a6);
    CHECK(aoe_draw_lost_message_ror(5) == 0x707);
    CHECK(aoe_draw_restore_message_ror((int32_t)0x8876024Bu) == 0x710);
    CHECK(aoe_draw_restore_message_ror(1) == 0x76b);
    CHECK(aoe_draw_blt_message_ror((int32_t)0x88760240u) == 0x8a1);
    CHECK(aoe_draw_blt_message_ror(1) == 0x8fb);
    CHECK(aoe_draw_create_message_ror((int32_t)0x8876017Cu) == 0x8ff);
    CHECK(aoe_draw_create_message_ror(1) == 0x95f);
}

static void test_modes(void) {
    AoeDrawSystemRor d;
    aoe_draw_clear_ror(&d, &ops);
    CHECK(d.bits == 8 && d.hardware_clear == 1);
    aoe_draw_probe_modes_ror(&d, 0);
    CHECK(d.modes[0] && d.modes[1] && d.modes[2] && !d.modes[3]);
    aoe_draw_clear_ror(&d, &ops);
    aoe_draw_probe_modes_ror(&d, 1);
    CHECK(d.device == NULL); /* made and released again */
    CHECK(aoe_draw_mode_available_ror(&d, 640, 480, 8));
    CHECK(aoe_draw_mode_available_ror(&d, 800, 600, 16));
    CHECK(aoe_draw_mode_available_ror(&d, 320, 200, 8));
    CHECK(!aoe_draw_mode_available_ror(&d, 320, 240, 8));
    CHECK(!aoe_draw_mode_available_ror(&d, 800, 600, 8));
    /* Width 800 alone decides, as in the original. */
    CHECK(aoe_draw_mode_available_ror(&d, 800, 1, 16));
}

static void test_open_windowed(void) {
    AoeDrawSystemRor d;
    aoe_draw_clear_ror(&d, &ops);
    CHECK(!aoe_draw_open_ror(&d, NULL, NULL, NULL, 1, 1, 640, 480, 0));
    aoe_draw_clear_ror(&d, &ops);
    desk_bits = 16;
    CHECK(!aoe_draw_open_ror(&d, NULL, NULL, NULL, 2, 1, 640, 480, 0));
    CHECK(d.error == 2);
    aoe_draw_shutdown_ror(&d);
    desk_bits = 8;
    aoe_draw_clear_ror(&d, &ops);
    CHECK(aoe_draw_open_ror(&d, NULL, (void *)1, (void *)2, 2, 1, 640, 480,
                            0));
    /* The window's palette went into the device palette. */
    CHECK(d.palette[0] == 7 && d.key_blits && !d.bank_switched);
    /* Windowed surfaces follow the client area. */
    CHECK(d.back && d.back->view.width == 320 && d.back->view.height == 200);
    CHECK(d.primary && d.primary->shares_primary &&
          d.primary->surface == d.primary_surface);
    CHECK(d.surfaces && d.surfaces->surface == d.primary &&
          d.surfaces->next->surface == d.back);
    /* The back buffer is clipped to itself with a whole region. */
    CHECK(d.back->view.clip_right == 319 && d.back->view.clip_bottom == 199);
    CHECK(d.back->active && d.back->active->count[199] == 1);
    CHECK(d.back->video == 0 && d.back->view.pitch == 320);

    aoe_draw_present_ror(&d, NULL);
    CHECK(last_dst[0] == 10 && last_dst[1] == 20 && last_dst[2] == 330 &&
          last_dst[3] == 220 && last_src[2] == 320);
    {
        int32_t r[4] = {-5, 3, 900, 50};
        aoe_draw_present_ror(&d, r);
        CHECK(last_src[0] == 0 && last_src[1] == 3 && last_src[2] == 640 &&
              last_src[3] == 51);
        CHECK(last_dst[0] == 10 && last_dst[1] == 23 && last_dst[2] == 650);
    }

    /* Palettes: the window palette with black and white ends. */
    aoe_draw_set_palette_handle_ror(&d, (void *)2);
    CHECK(d.palette[0] == 0 && d.palette[4] == 7 && d.palette[1020] == 0xff &&
          d.palette[1023] == 0);
    CHECK(last_palette[1021] == 0xff && d.full_palette_sets == 1);

    /* A lost primary: a changed desktop asks for a new mode. */
    ((FakeSurface *)d.primary_surface)->lost = 1;
    mode_w = 800;
    CHECK(aoe_draw_restore_ror(&d) == 1);
    mode_w = 640;
    CHECK(aoe_draw_restore_ror(&d) == 2 && d.restored);
    CHECK(aoe_draw_restore_ror(&d) == 0);
    aoe_draw_clear_restored_ror(&d);
    CHECK(!d.restored);
    aoe_draw_shutdown_ror(&d);
    CHECK(d.device == NULL && d.back == NULL && d.primary == NULL);
}

static void test_surface_ops(void) {
    AoeDrawSystemRor d;
    AoeDrawSurfaceRor *b, other;
    uint8_t *bits;
    int32_t r[4];

    aoe_draw_clear_ror(&d, &ops);
    CHECK(aoe_draw_open_ror(&d, NULL, NULL, NULL, 2, 2, 64, 48, 0));
    CHECK(d.mode_set && mode_w == 64);
    b = d.back;
    bits = aoe_surface_lock_ror(b, 1);
    CHECK(bits && b->rows[1] == bits + b->view.pitch);
    CHECK(aoe_surface_lock_ror(b, 1) == bits);
    CHECK(aoe_surface_dc_ror(b) == NULL); /* not while locked */
    aoe_surface_unlock_ror(b);
    CHECK(aoe_surface_dc_ror(b) != NULL);
    CHECK(aoe_surface_lock_ror(b, 1) == NULL); /* not while the DC is out */
    aoe_surface_release_dc_ror(b);

    /* Software clears through the row table. */
    d.hardware_clear = 0;
    r[0] = 2;
    r[1] = 3;
    r[2] = 4;
    r[3] = 3;
    aoe_surface_fill_ror(b, r, 9);
    bits = ((FakeSurface *)b->surface)->bits;
    CHECK(bits[3 * 64 + 2] == 9 && bits[3 * 64 + 4] == 9 &&
          bits[3 * 64 + 5] == 0 && bits[4 * 64 + 2] == 0);
    d.hardware_clear = 1;
    aoe_surface_fill_ror(b, r, 5); /* the device fill, exclusive edges */
    CHECK(bits[3 * 64 + 4] == 5 && bits[3 * 64 + 5] == 0);

    aoe_surface_plot_ror(b, 63, 47, 1); /* not locked: no bits */
    aoe_surface_lock_ror(b, 1);
    aoe_surface_plot_ror(b, 63, 47, 1);
    aoe_surface_plot_ror(b, 64, 0, 1);
    CHECK(bits[47 * 64 + 63] == 1);
    aoe_surface_unlock_ror(b);

    /* Clipping: reversed corners are put in order. */
    r[0] = 50;
    r[1] = 40;
    r[2] = 10;
    r[3] = 100;
    aoe_surface_clip_ror(b, r);
    CHECK(b->view.clip_left == 10 && b->view.clip_right == 50 &&
          b->view.clip_top == 40 && b->view.clip_bottom == 47);
    CHECK(b->active->first[40]->x0 == 10 && b->active->first[40]->x1 == 50);

    /* Copies are cut to both surfaces. */
    aoe_surface_init_ror(&other, "other", 1);
    aoe_surface_attach_ror(&other, &d, 30, 30, 0, 0);
    aoe_surface_set_key_ror(&other, 1, 3);
    aoe_surface_blit_ror(&other, b, 5, 30, NULL, 1);
    CHECK(last_keyed == 1);
    CHECK(last_dst[0] == 10 && last_src[0] == 5);  /* clip left 10 */
    CHECK(last_dst[1] == 40 && last_src[1] == 10); /* clip top 40 */
    CHECK(last_dst[2] == 35 && last_dst[3] == 48 && last_src[3] == 18);
    aoe_surface_set_key_ror(&other, 0, 3);
    CHECK(other.key == 0xff);

    /* Spans of this surface's rectangle copied into another. */
    {
        AoeSpanRor sp = {NULL, NULL, 1, 2};
        AoeSpanRor *list[30] = {0};
        int32_t rr[4] = {4, 5, 10, 6};
        uint8_t *ob;
        list[5] = &sp;
        list[6] = &sp;
        aoe_surface_lock_ror(&other, 1);
        aoe_surface_lock_ror(b, 1);
        other.rows[5][4 + 1] = 0x11;
        other.rows[6][4 + 2] = 0x22;
        aoe_surface_span_copy_ror(&other, b, 20, 0, rr, list);
        ob = b->rows[0];
        CHECK(ob[21] == 0x11 && b->rows[1][22] == 0x22 && ob[23] == 0);
        aoe_surface_unlock_ror(b);
        aoe_surface_unlock_ror(&other);
    }

    /* A rectangle moved left by whole words. */
    {
        int32_t from[4] = {8, 0, 15, 0}, to[4] = {0, 0, 7, 0};
        aoe_surface_lock_ror(b, 1);
        memcpy(b->rows[0] + 8, "ABCDEFGH", 8);
        aoe_surface_scroll_ror(b, from, to, -8, 0, 0, 0);
        CHECK(memcmp(b->rows[0], "ABCDEFGH", 8) == 0);
        aoe_surface_unlock_ror(b);
    }
    aoe_surface_free_ror(&other);

    /* Mode change remakes the surfaces at the new size. */
    CHECK(aoe_draw_change_mode_ror(&d, 80, 60, 8));
    CHECK(d.width == 80 && d.back->view.width == 80 && mode_w == 80);
    aoe_draw_shutdown_ror(&d);
}

static void test_wrap_and_line(void) {
    AoeDrawSurfaceRor s;
    uint8_t mem[16 * 26];
    int i;
    aoe_surface_init_ror(&s, NULL, 0);
    s.view.bits = mem;
    s.view.width = 16;
    s.view.height = 16;
    s.view.pitch = 16;
    s.extra_rows = 1;
    aoe_surface_build_rows_ror(&s);
    /* Extra rows start five rows in and step by the width. */
    CHECK(s.rows[0] == mem + 80 && s.rows[1] == mem + 96);
    aoe_surface_wrap_rows_ror(&s, 3, 14);
    CHECK(s.rows[0] == s.row_table[14] + 3 && s.rows[2] == s.row_table[0] + 3);
    aoe_surface_free_ror(&s);

    span_calls = 0;
    aoe_draw_line_ror(&ops, 0, 0, 5, 2, 1); /* shallow */
    CHECK(span_calls == 3);
    CHECK(spans[0][0] == 0 && spans[0][1] == 1 && spans[0][2] == 0);
    CHECK(spans[1][0] == 2 && spans[1][1] == 3 && spans[1][2] == 1);
    CHECK(spans[2][0] == 4 && spans[2][1] == 5 && spans[2][2] == 2);
    span_calls = 0;
    /* Steep: the ends swap so the line runs from its left end. */
    aoe_draw_line_ror(&ops, 1, 4, 0, 0, 1);
    CHECK(span_calls == 5 && spans[0][2] == 0 && spans[4][2] == 4);
    CHECK(spans[0][0] == 0 && spans[4][0] == 1);
    for (i = 0; i < 5; ++i) CHECK(spans[i][0] == spans[i][1]);
    span_calls = 0;
    aoe_draw_line_ror(&ops, 2, 3, 2, 1, 1); /* vertical */
    CHECK(span_calls == 3 && spans[0][2] == 1);
}

int main(void) {
    test_messages();
    test_modes();
    test_open_windowed();
    test_surface_ops();
    test_wrap_and_line();
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    puts("draw_system_test: ok");
    return 0;
}
