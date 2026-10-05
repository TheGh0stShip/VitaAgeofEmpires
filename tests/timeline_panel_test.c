#include "timeline_panel.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static AoePlayerHistoryRor hist[3]; /* players 1, 2 */
static int locks, unlocks, dc_locks, dc_unlocks, boxes, vlines, texts, shapes, rings_n, outlines;
static int closed, parent_bg = -1, begun, ended, freed;
static int32_t col_sum[64], col_first_len[64];
static char last_text[128];
static int32_t last_text_x, last_text_y;
static uint8_t ring_colors[6];
static int shape_obj;

static int p_setup(void *c, int32_t x, int32_t y, int32_t w, int32_t h) { (void)c; (void)x; (void)y; (void)w; (void)h; return 1; }
static int p_begin(void *c) { (void)c; ++begun; return 1; }
static void p_end(void *c) { (void)c; ++ended; }
static int p_has(void *c) { (void)c; return 1; }
static int p_84(void *c) { (void)c; return 0; }
static void p_bg(void *c, int f) { (void)c; parent_bg = f; }
static int p_lock(void *c, const char *w) { (void)c; (void)w; ++locks; return 1; }
static void p_unlock(void *c, const char *w) { (void)c; (void)w; ++unlocks; }
static int p_dclock(void *c, const char *w) { (void)c; (void)w; ++dc_locks; return 1; }
static void p_dcunlock(void *c, const char *w) { (void)c; (void)w; ++dc_unlocks; }
static void p_box(void *c, int32_t a, int32_t b, int32_t d, int32_t e, uint8_t col) { (void)c; (void)a; (void)b; (void)d; (void)e; (void)col; ++boxes; }
static void p_outline(void *c, int32_t a, int32_t b, int32_t d, int32_t e, uint8_t col) { (void)c; (void)a; (void)b; (void)d; (void)e; (void)col; ++outlines; }
static void p_rings(void *c, int32_t a, int32_t b, int32_t d, int32_t e, int n, const uint8_t *cols) {
    (void)c; (void)a; (void)b; (void)d; (void)e; rings_n = n; memcpy(ring_colors, cols, (size_t)n * 2);
}
static int32_t graph_left;
static void p_vline(void *c, int32_t x, int32_t y, int32_t len, uint8_t col) {
    (void)c; (void)y; (void)col;
    ++vlines;
    if (x - graph_left >= 0 && x - graph_left < 64) {
        if (col_sum[x - graph_left] == 0) col_first_len[x - graph_left] = len;
        col_sum[x - graph_left] += len;
    }
}
static uint8_t p_pal(void *c, uint32_t rgb) { (void)c; return (uint8_t)(rgb & 0xff); }
static void p_text(void *c, int font, int32_t x, int32_t y, const char *s, int32_t n, uint32_t rgb) {
    (void)c; (void)font; (void)rgb; ++texts;
    snprintf(last_text, sizeof last_text, "%.*s", (int)n, s);
    last_text_x = x; last_text_y = y;
}
static int p_string(void *c, int32_t id, char *o, int32_t n) { (void)c; snprintf(o, (size_t)n, "S%x", (unsigned)id); return 1; }
static void p_format(void *c, int32_t k, int32_t a, int32_t b, char *o, int32_t n) { (void)c; (void)k; (void)b; snprintf(o, (size_t)n, "civ%d", (int)a); }
static void *p_sload(void *c, const char *n) { (void)c; (void)n; return &shape_obj; }
static void p_sfree(void *c, void *s) { (void)c; (void)s; ++freed; }
static int32_t p_sframes(void *c, void *s) { (void)c; (void)s; return 5; }
static void p_ssize(void *c, void *s, int32_t f, int16_t *w, int16_t *h) { (void)c; (void)s; (void)f; *w = 10; *h = 8; }
static void p_sbox(void *c, void *s, int16_t *l, int16_t *t, int16_t *r, int16_t *b) { (void)c; (void)s; *l = 0; *t = 0; *r = 15; *b = 11; }
static void p_sdraw(void *c, void *s, int32_t x, int32_t y, int32_t f) { (void)c; (void)s; (void)x; (void)y; (void)f; ++shapes; }
static void p_fclose(void *c, uintptr_t *s) { (void)c; (void)s; ++closed; }
static int16_t p_count(void *c) { (void)c; return 3; }
static const AoePlayerHistoryRor *p_hist(void *c, int32_t p) { (void)c; return &hist[p]; }
static int16_t p_color(void *c, int32_t p) { (void)c; return (int16_t)(p - 1); }
static uint8_t p_civ(void *c, int32_t p) { (void)c; return (uint8_t)(p + 3); }
static const char *p_name(void *c, int32_t p) { (void)c; return p == 1 ? "Ann" : "Bob"; }

static const AoeTimelineOpsRor ops = {
    NULL, p_setup, p_begin, p_end, p_has, p_84, p_bg, p_lock, p_unlock, p_dclock, p_dcunlock,
    p_box, p_outline, p_rings, p_vline, p_pal, p_text, p_string, p_format, p_sload, p_sfree,
    p_sframes, p_ssize, p_sbox, p_sdraw, p_fclose, p_count, p_hist, p_color, p_civ, p_name};

int main(void) {
    AoeTimelineRor t, *n;
    AoeTimelineFontRor fa = {1, 6, 10}, fb = {2, 5, 9};
    AoeTimelineMarkRor *m, mk;
    int i, ticks = 0, events = 0;

    aoe_history_init_ror(&hist[1], NULL, 0.0f);
    aoe_history_init_ror(&hist[2], NULL, 0.0f);
    for (i = 0; i < 4; ++i) {
        aoe_history_add_ror(&hist[1], 0, (uint8_t)(10 + i * 10));
        aoe_history_add_ror(&hist[2], 0, 30);
        if (i == 1) aoe_history_event_ror(&hist[1], 1, 500);
    }

    aoe_timeline_init_ror(&t, &ops);
    assert(t.enabled == 1 && t.tick_every == 12 && t.fill_color == 0xff && t.color_49c == 0xffff);
    assert(aoe_timeline_setup_ror(&t, 100, 50, 89, 200, &fa, &fb, "bg", "", 0, 0, 1, 7));
    assert(t.border == 1 && t.background == &shape_obj && t.background_w == 16 && t.background_h == 12);
    assert(t.icons == NULL && t.fill == 1 && t.fill_color == 0 && t.redraw == 1);

    aoe_timeline_layout_ror(&t);
    assert(t.samples == 4 && t.columns == 4 && t.scale == 10.0f && !t.compress);
    assert(t.axis_w == 44 && t.axis_h == 200 - 40 - 26 && t.left == 127 && t.top == 50 + 15 + 20);
    assert(t.graph_w == 42 && t.graph_h == 131 && t.tick_every == 1);
    graph_left = t.left;

    aoe_timeline_draw_graph_ror(&t);
    assert(locks == 1 && unlocks == 1 && vlines == 40 * 2);
    for (i = 0; i < 40; ++i) assert(col_sum[i] == 131);
    /* Sample 0: player 1 has 10 of 40; sample 1: 20 of 50; interpolated between. */
    assert(col_first_len[0] == 32 && col_first_len[10] == 52 && col_first_len[5] == 42);
    assert(t.cur_x == t.left + 40);
    for (m = t.marks; m; m = m->next) {
        if (m->kind == 3) ++ticks; else ++events;
    }
    assert(ticks == 4 && events == 1);
    for (m = t.marks; m && m->kind == 3; m = m->next) {}
    assert(m && m->kind == 1 && m->player == 1 && m->sample == 2 && m->x == t.left + 20);
    assert(m->y == t.top && m->length == 65);

    /* Marks: the event line with its caption, the ticks with their texts. */
    locks = unlocks = vlines = texts = 0;
    aoe_timeline_draw_marks_ror(&t);
    assert(locks == 1 && unlocks == 1 && dc_locks == 1 && dc_unlocks == 1);
    assert(vlines == 1 && texts == 1 + 4 * 2 && !strcmp(m->text, "S106b") && m->text_len == 5);
    assert(m->text_x == t.left + 9 && m->text_y == m->y + 2);
    assert(t.marks->kind == 3 && t.marks->sample == 3 && !strcmp(t.marks->text, "70"));
    assert(!strcmp(t.marks->text2, "0:00"));
    memset(&mk, 0, sizeof mk);
    mk.kind = 3;
    mk.sample = 250; /* 3750 s = 62 min */
    mk.value = 12;
    aoe_timeline_draw_mark_ror(&t, &mk, -1);
    assert(!strcmp(mk.text, "12") && !strcmp(mk.text2, "1:02") && mk.text2_len == 4);

    /* Icons: placement inside the graph and the caption cap. */
    assert(aoe_timeline_set_icons_ror(&t, "icons") && t.icon_w == 16);
    memset(&mk, 0, sizeof mk);
    mk.kind = 6;
    mk.x = t.left + 20;
    mk.y = 3;
    mk.length = 20;
    strcpy(mk.text, "abcdef");
    assert(aoe_timeline_place_ror(&t, &mk, 2) && mk.show == 1);
    assert(t.icon_w == 10 && t.icon_h == 8 && mk.icon_x == mk.x - 5 && mk.icon_y == 10 + 6 - 4);
    assert(mk.text_len == 2 && mk.text_x == mk.icon_x && mk.text_y == mk.icon_y + 9);
    mk.x = t.left + 2; /* too far left */
    assert(!aoe_timeline_place_ror(&t, &mk, 2) && mk.show == 0 && mk.icon_x == 0);
    assert(!aoe_timeline_place_ror(&t, &mk, 9));

    /* Names beside the first sample's bands. */
    texts = 0;
    aoe_timeline_draw_names_ror(&t);
    assert(texts == 2 && !strcmp(last_text, "Bob - civ5") && last_text_x == t.left + 3);
    assert(last_text_y == (98 / 2 - 4) + t.top + 32);

    /* Border styles and background choices. */
    aoe_timeline_set_border_ror(&t, 3, 1, 2, 3, 4, 5, 6);
    aoe_timeline_draw_border_ror(&t);
    assert(rings_n == 2 && ring_colors[0] == 6 && ring_colors[1] == 5 && ring_colors[2] == 2 && ring_colors[3] == 1);
    aoe_timeline_set_border_ror(&t, 7, 1, 2, 3, 4, 5, 6);
    aoe_timeline_draw_border_ror(&t);
    assert(rings_n == 3 && ring_colors[0] == 1 && ring_colors[5] == 6);
    t.border = 1;
    aoe_timeline_draw_border_ror(&t);
    assert(outlines == 1);
    shapes = 0;
    aoe_timeline_draw_background_ror(&t);
    assert(shapes == 6 * 17); /* 89 / 16 and 200 / 12, rounded up */
    aoe_timeline_set_background_ror(&t, NULL);
    boxes = 0;
    aoe_timeline_draw_background_ror(&t);
    assert(boxes == 1);
    t.fill = 0;
    t.border = 4;
    aoe_timeline_draw_background_ror(&t);
    assert(parent_bg == 1);
    t.border = 0;
    aoe_timeline_draw_background_ror(&t);
    assert(parent_bg == 0);

    boxes = texts = 0;
    aoe_timeline_draw_axis_ror(&t);
    assert(boxes == 4 && texts == 2 && !strcmp(last_text, "S2c56"));
    assert(last_text_x == 22 - (5 * 6 >> 1) + t.left && last_text_y == 200 + 50 - 5 - 10);

    aoe_timeline_draw_ror(&t);
    assert(begun == 1 && ended == 1 && t.redraw == 0);

    /* Compression: more samples than columns. */
    t.width = 0x31 + 2;
    aoe_timeline_layout_ror(&t);
    assert(t.compress && t.columns == 2 && t.skip_total == 2 && t.skip_ratio == 1.0f && t.scale == 1.0f);

    /* No history: disabled with one sample. */
    hist[1].count = 0;
    hist[2].count = 0;
    aoe_timeline_layout_ror(&t);
    assert(!t.enabled && t.samples == 1);
    hist[1].count = 4;
    hist[2].count = 4;

    n = aoe_timeline_new_ror(&ops, 800, 600, 64, 48, 320, 240, &fa, &fb, 640, 480);
    assert(n && n->x == 80 && n->y == 60 && n->width == 400 && n->height == 300 && n->samples == 4);
    aoe_timeline_release_ror(n);
    free(n);
    assert(!aoe_timeline_new_ror(&ops, 800, 600, 0, 0, 1, 1, &fa, &fb, 0, 480));

    closed = freed = 0;
    aoe_timeline_release_ror(&t);
    assert(!t.marks && closed == 3 && freed == 1 && !t.icons);
    aoe_history_release_ror(&hist[1]);
    aoe_history_release_ror(&hist[2]);
    return 0;
}
