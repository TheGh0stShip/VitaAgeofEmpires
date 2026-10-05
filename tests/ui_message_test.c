/* Patched RoR 1.0a message panel 0x00465730..0x00466910 and picture panel
   0x00466950..0x00466d60. */
#include "ui_message.h"
#include "ui_picture.h"

#include <assert.h>
#include <string.h>

static uint32_t clock_ms;
static uint32_t now(void *c) { (void)c; return clock_ms; }

static int extent(void *c, uintptr_t f, const char *t, int32_t *cx, int32_t *cy) {
    (void)c; (void)f;
    *cx = (int32_t)strlen(t) * 6;
    *cy = 12;
    return 1;
}
static int images;
static void *img_create(void *c, int32_t w, int32_t h) { (void)c; (void)w; (void)h; ++images; return &images; }
static void img_free(void *c, void *i) { (void)c; (void)i; --images; }
static AoeUiRectRor rects[16];
static uint32_t fmts[16], cols[16];
static int n_draw;
static void draw_text(void *c, void *img, const char *t, const AoeUiRectRor *r,
                      uint32_t f, uint32_t rgb, int opaque) {
    (void)c; (void)img; (void)t; (void)opaque;
    rects[n_draw] = *r; fmts[n_draw] = f; cols[n_draw++] = rgb;
}
static int blits;
static void blit(void *c, void *img, int32_t x, int32_t y, const AoeUiRectRor *s) {
    (void)c; (void)img; (void)x; (void)y; (void)s; ++blits;
}
static uint32_t pal(void *c, uint8_t i) { (void)c; return 0x10000u * i; }
static int icon_frame(void *c, void *s, int32_t f, int32_t *w, int32_t *hx, int32_t *hy) {
    (void)c; (void)s; (void)f; *w = 10; *hx = 12; *hy = -3; return 1;
}
static int32_t icon_x, icon_y;
static void draw_icon(void *c, void *img, void *s, int32_t f, int32_t x, int32_t y) {
    (void)c; (void)img; (void)s; (void)f; icon_x = x; icon_y = y;
}

/* Picture ops. */
static int shapes_ok = 1, loads, frees;
static int shape_tag, bitmap_tag;
static char last_name[32];
static void *load_shape(void *c, const char *n, int32_t id) { (void)c; (void)id; strcpy(last_name, n); ++loads; return &shape_tag; }
static int shape_valid(void *c, void *s) { (void)c; (void)s; return shapes_ok; }
static void free_any(void *c, void *s) { (void)c; (void)s; ++frees; }
static int shape_size(void *c, void *s, int32_t f, int16_t *w, int16_t *h) { (void)c; (void)s; (void)f; *w = 32; *h = 20; return 1; }
static void *load_bitmap(void *c, const char *n) { (void)c; strcpy(last_name, n); ++loads; return &bitmap_tag; }
static void bitmap_size(void *c, void *b, int32_t *w, int32_t *h) { (void)c; (void)b; *w = 64; *h = 48; }
static int drawn_shape, drawn_bitmap;
static void draw_bitmap(void *c, AoeUiAreaRor *a, void *b, int32_t x, int32_t y) { (void)c; (void)a; (void)b; (void)x; (void)y; ++drawn_bitmap; }
static void draw_shape(void *c, AoeUiAreaRor *a, void *s, int32_t x, int32_t y, int32_t f) { (void)c; (void)a; (void)s; (void)x; (void)y; (void)f; ++drawn_shape; }

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 640, 480};
    AoeUiMessageOpsRor mops = {NULL, extent, img_create, img_free, NULL,
                               draw_text, blit, pal, icon_frame, draw_icon};
    AoeUiPictureOpsRor pops = {NULL, load_shape, shape_valid, free_any,
                               shape_size, load_bitmap, bitmap_size, free_any,
                               draw_bitmap, draw_shape};
    static AoeUiMessageRor m;
    static AoeUiPictureRor pic;
    AoeUiRectRor r;

    pf.now_ms = now;
    sys.platform = &pf;

    aoe_ui_message_init_ror(&m, &sys, &mops, 7, 6, 12);
    assert(m.timeout_ms == 8000 && m.align == 2 && m.buffer_w == -1 &&
           !m.panel.active && m.font == 7);
    aoe_ui_panel_setup_ror(&m.panel, &area, NULL, 10, 20, 200, 30, 0);
    assert(images == 1 && m.image && m.buffer_w == 200 && m.buffer.right == 199);

    /* Centred style: the text box is centred in the buffer. */
    clock_ms = 100;
    aoe_ui_message_show_ror(&m, 1, "hello", 3, 4, 0, 0, 0);
    assert(m.shown && m.panel.active && m.shown_ms == 100 && m.rendered);
    /* 5 chars -> 30 x 12, box 32 x 14 at ((200 - 32) >> 1) - 1 = 83. */
    assert(m.text_rect.left == 83 && m.text_rect.top == 0 &&
           m.text_rect.right == 115 && m.text_rect.bottom == 14);
    assert(n_draw == 5 && fmts[0] == 0x21 && cols[0] == 0x40000 && cols[4] == 0x30000);
    assert(rects[0].left == 0 && rects[0].right == 197 && rects[4].left == 1 &&
           rects[4].bottom == 28);
    assert(m.screen_rect.left == m.panel.rect.left + 83);
    /* Showing the same text again changes nothing. */
    n_draw = 0;
    aoe_ui_message_show_ror(&m, 1, "hello", 3, 4, 0, 0, 0);
    assert(n_draw == 0);
    m.panel.vt->get_true_rect(&m.panel, &r);
    /* Not visible yet (no clip), so the panel rect. */
    assert(r.left == m.panel.rect.left || r.left == m.screen_rect.left);

    /* Timeout for style 1. */
    clock_ms = 8099;
    m.panel.vt->handle_idle(&m.panel);
    assert(m.shown);
    clock_ms = 8100;
    m.panel.vt->handle_idle(&m.panel);
    assert(!m.shown && !m.panel.active && !m.rendered);

    /* Left-aligned bottom text with an icon; RGB colours. */
    aoe_ui_message_set_layout_ror(&m, 2, 1, 0);
    n_draw = 0;
    aoe_ui_message_show_rgb_ror(&m, 0, "abc", 0x111, 0x222, 0, 0, 0, &shape_tag, 2, NULL, 0);
    /* 18 x 12 -> box 19 x 13 at y = 30 - 13 - 1 = 16. */
    assert(m.text_rect.top == 16 && m.text_rect.bottom == 29 && fmts[0] == 0x28);
    assert(cols[0] == 0x222 && cols[4] == 0x111);
    assert(icon_x == 12 - 10 - 4 && icon_y == -3 + 6 + 16 && m.text_rect.left == -14);
    /* The panel's style 0 never times out. */
    clock_ms = 100000;
    m.panel.vt->handle_idle(&m.panel);
    assert(m.shown);
    /* Long text wraps when allowed. */
    aoe_ui_message_set_layout_ror(&m, 4, 4, 1);
    aoe_ui_message_show_rgb_ror(&m, 0, "a long line of text here, longer than the box",
                                1, 2, 0, 0, 0, NULL, 0, NULL, 0);
    assert(m.text_rect.left == 0 && m.text_rect.right == 199 && m.text_rect.bottom == 29);
    aoe_ui_message_hide_ror(&m);
    aoe_ui_message_release_ror(&m);
    assert(images == 0);

    /* Picture: shape first, sized from frame 0. */
    aoe_ui_picture_init_ror(&pic, &sys, &pops);
    assert(pic.id == -1 && pic.keep == 1 && pic.panel.field_ee == 2);
    assert(aoe_ui_picture_setup_ror(&pic, &area, NULL, 5, 6, 1, 1, "logo", 50, 1, 1));
    assert(!strcmp(last_name, "logo.shp") && pic.is_shape && pic.loaded &&
           pic.panel.width == 32 && pic.panel.height == 20);
    /* An invalid shape falls back to the bitmap. */
    shapes_ok = 0;
    assert(aoe_ui_picture_load_ror(&pic) && !pic.is_shape && pic.bitmap &&
           !strcmp(last_name, "logo.bmp"));
    /* Without keep the picture is loaded around each draw. */
    pic.keep = 0;
    aoe_ui_picture_free_ror(&pic);
    frees = 0;
    pic.panel.visible = 1;
    pic.panel.active = 1;
    pic.panel.vt->draw(&pic.panel);
    assert(drawn_bitmap == 1 && !pic.loaded && frees == 2);
    /* No name and no ID: nothing to load. */
    pic.name[0] = 0;
    pic.id = -1;
    assert(!aoe_ui_picture_load_ror(&pic));
    aoe_ui_picture_release_ror(&pic);
    return 0;
}
