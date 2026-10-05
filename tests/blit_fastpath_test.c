#include "slp.h"
#include "terrain_art.h"
#include "terrain_draw.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* The summarized-frame fast paths (kind summary, solid row spans, load-time
   plain-frame flags) must write exactly what the per-pixel paths write. */

static uint32_t rng = 12345u;
static uint32_t next(void) {
    rng = rng * 1103515245u + 12345u;
    return rng >> 8;
}

/* Rows: transparent margins around one color run (diamond-like), fully
   transparent, or mixed kinds. */
static void fill_frame(SlpFrame *f, uint32_t w, uint32_t h, int kinds) {
    f->width = w;
    f->height = h;
    f->hotspot_x = (int32_t)(next() % (w + 1)) - 2;
    f->hotspot_y = (int32_t)(next() % (h + 1)) - 2;
    f->pixels = calloc((size_t)w * h, sizeof(SlpPixel));
    assert(f->pixels);
    for (uint32_t y = 0; y < h; ++y) {
        uint32_t mode = next() % 4;
        uint32_t lo = next() % (w + 1), hi = lo + next() % (w - lo + 1);
        for (uint32_t x = 0; x < w; ++x) {
            SlpPixel *p = &f->pixels[(size_t)y * w + x];
            p->index = (uint8_t)next();
            if (mode == 0) p->kind = SLP_TRANSPARENT;
            else if (mode < 3)
                p->kind = x >= lo && x < hi ? SLP_COLOR : SLP_TRANSPARENT;
            else p->kind = (uint8_t)(next() % (uint32_t)kinds);
        }
    }
}

static SlpFrame copy_plain(const SlpFrame *f) {
    SlpFrame c = *f;
    c.pixels = malloc((size_t)f->width * f->height * sizeof(SlpPixel));
    assert(c.pixels);
    memcpy(c.pixels, f->pixels, (size_t)f->width * f->height *
                                    sizeof(SlpPixel));
    c.kinds_known = 0;
    c.kind_mask = 0;
    c.row_spans = NULL;
    return c;
}

static void graphic_cases(void) {
    enum { W = 40, H = 30 };
    uint8_t shadow[256], a[W * H], b[W * H];
    for (int i = 0; i < 256; ++i) shadow[i] = (uint8_t)(255 - i);
    for (int trial = 0; trial < 400; ++trial) {
        SlpFrame fast = {0}, slow;
        int32_t ax = (int32_t)(next() % 70) - 15;
        int32_t ay = (int32_t)(next() % 60) - 15;
        int mirror = (int)(next() & 1);
        int ra, rb;
        fill_frame(&fast, 1 + next() % 24, 1 + next() % 20, 4);
        slow = copy_plain(&fast);
        slp_frame_summarize(&fast);
        for (size_t i = 0; i < sizeof(a); ++i) a[i] = b[i] = (uint8_t)next();
        ra = slp_blit_graphic_indexed(&fast, a, sizeof(a), W, H, W, ax, ay,
                                      mirror, 3, shadow);
        rb = slp_blit_graphic_indexed(&slow, b, sizeof(b), W, H, W, ax, ay,
                                      mirror, 3, shadow);
        assert(ra == rb && memcmp(a, b, sizeof(a)) == 0);
        /* Rejections agree too: shadow pixels without a table. */
        ra = slp_blit_graphic_indexed(&fast, a, sizeof(a), W, H, W, ax, ay,
                                      mirror, 3, NULL);
        rb = slp_blit_graphic_indexed(&slow, b, sizeof(b), W, H, W, ax, ay,
                                      mirror, 3, NULL);
        assert(ra == rb && memcmp(a, b, sizeof(a)) == 0);
        slp_free_frame(&fast);
        slp_free_frame(&slow);
    }
}

static void terrain_cases(void) {
    enum { W = 48, H = 36 };
    uint8_t a[W * H], b[W * H], first[W * H], second[W * H];
    for (int trial = 0; trial < 400; ++trial) {
        SlpFrame fast_frame = {0}, slow_frame;
        uint8_t plain = 1;
        TerrainArtEntry fast_entry = {0}, slow_entry = {0}, border = {0};
        TerrainArtCache fast_cache = {1, &fast_entry};
        TerrainArtCache slow_cache = {1, &slow_entry};
        TerrainArtCache border_cache = {1, &border};
        TerrainDrawPlan plan;
        TerrainSpritePassPlan passes;
        TerrainClipMask m1 = {first, sizeof(first), W};
        TerrainClipMask m2 = {second, sizeof(second), W};
        int32_t ax = (int32_t)(next() % 80) - 16;
        int32_t ay = (int32_t)(next() % 60) - 12;
        int ra, rb;
        fill_frame(&fast_frame, 1 + next() % 30, 1 + next() % 24, 2);
        slow_frame = copy_plain(&fast_frame);
        slp_frame_summarize(&fast_frame);
        fast_entry.frames = &fast_frame;
        fast_entry.frame_count = 1;
        fast_entry.plain_frames = &plain;
        slow_entry.frames = &slow_frame;
        slow_entry.frame_count = 1;
        memset(&plan, 0, sizeof(plan));
        plan.layer_count = 1;
        plan.layers[0].kind = TERRAIN_DRAW_TERRAIN;
        memset(&passes, 0, sizeof(passes));
        passes.count = (uint8_t)(1 + next() % 2);
        for (int p = 0; p < passes.count; ++p) {
            passes.passes[p].clip = (uint8_t)(next() % 3);
            passes.passes[p].dither_mode = (uint8_t)(next() % 2);
        }
        passes.even_mask = next() | (next() << 16);
        passes.even_fill = next();
        passes.odd_mask = next() | (next() << 16);
        passes.odd_fill = next();
        for (size_t i = 0; i < sizeof(a); ++i) {
            a[i] = b[i] = (uint8_t)next();
            first[i] = (uint8_t)(next() % 3 ? 255 : 0);
            second[i] = (uint8_t)(next() % 3 ? 0 : 255);
        }
        {
            int32_t ox = (int32_t)(next() % 7) - 3, oy = (int32_t)(next() % 5);
            ra = terrain_blit_draw_plan_passes_indexed(
                &plan, &fast_cache, &border_cache, &passes, m1, m2, a,
                sizeof(a), W, H, W, ax, ay, ox, oy);
            rb = terrain_blit_draw_plan_passes_indexed(
                &plan, &slow_cache, &border_cache, &passes, m1, m2, b,
                sizeof(b), W, H, W, ax, ay, ox, oy);
            assert(ra == rb && ra == 1 && memcmp(a, b, sizeof(a)) == 0);
        }
        slp_free_frame(&fast_frame);
        slp_free_frame(&slow_frame);
    }
}

int main(void) {
    graphic_cases();
    terrain_cases();
    return 0;
}
