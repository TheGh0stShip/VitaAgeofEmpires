#include "palette_fade.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "le_bytes.h"

void aoe_palette_blend_ror(const uint8_t src[1024], uint8_t dst[1024],
                           uint32_t color, int32_t percent, int32_t keep_a,
                           int32_t keep_b) {
    int32_t i, j, k;
    for (i = 0; i < 256; ++i) {
        const uint8_t *s = src + i * 4;
        uint8_t *d = dst + i * 4;
        int32_t c[3], t, light;
        if (i == keep_a || i == keep_b) {
            memcpy(d, s, 4);
            continue;
        }
        c[0] = s[0];
        c[1] = s[1];
        c[2] = s[2];
        /* Two bubble passes: brightest first. */
        for (j = 0; j < 2; ++j)
            for (k = 1; k < 3; ++k)
                if (c[k - 1] < c[k]) t = c[k - 1], c[k - 1] = c[k], c[k] = t;
        light = (c[1] + c[2]) * (0xff - c[0]) / 512 + c[0];
        for (k = 0; k < 3; ++k) {
            int32_t want = (int32_t)((color >> (8 * k)) & 0xff) * light / 0xff;
            d[k] = (uint8_t)(s[k] + (want - s[k]) * percent / 100);
        }
        d[3] = s[3];
    }
}

void aoe_surface_palette_ror(const AoeDrawSurfaceRor *s, uint8_t out[1024]) {
    memcpy(out, s->system->palette, 1024);
}

void aoe_palette_apply_ror(AoeDrawSystemRor *d, uint8_t entries[1024]) {
    memset(entries, 0, 4);
    entries[0x3fc] = entries[0x3fd] = entries[0x3fe] = 0xff;
    entries[0x3ff] = 0;
    aoe_draw_set_palette_ror(d, 0, 256, entries);
}

static void base_palette(AoeDrawSystemRor *d, const uint8_t *base,
                         uint8_t out[1024]) {
    if (base) memcpy(out, base, 1024);
    else if (d->back) aoe_surface_palette_ror(d->back, out);
    else memcpy(out, d->palette, 1024);
}

/* Eased percent for `elapsed_ms`; done (1) at the end. */
static int eased(float seconds, float seconds_sq, int direction,
                 uint32_t elapsed_ms, int32_t *percent) {
    float f = (float)(int32_t)elapsed_ms * 0.001f;
    if (direction != 1) f = seconds - f;
    if (seconds <= 0.0f || seconds < f || f < 0.0f) {
        *percent = direction == 1 ? 100 : 0;
        return 1;
    }
    *percent = (int32_t)((double)f * (double)f / (double)seconds_sq * 100.0);
    if (direction == 1 && *percent > 99) {
        *percent = 100;
        return 1;
    }
    if (direction != 1 && *percent < 1) {
        *percent = 0;
        return 1;
    }
    return 0;
}

void aoe_palette_fade_ror(AoeDrawSystemRor *d, uint32_t color, float seconds,
                          int direction, const uint8_t *base, int32_t keep_a,
                          int32_t keep_b, uint32_t (*now_ms)(void *),
                          void *clock) {
    uint8_t from[1024], to[1024];
    uint32_t start = now_ms(clock);
    float sq = seconds * seconds;
    int32_t percent;
    int done;
    memset(from, 0, sizeof from);
    base_palette(d, base, from);
    do {
        done = eased(seconds, sq, direction, now_ms(clock) - start, &percent);
        aoe_palette_blend_ror(from, to, color, percent, keep_a, keep_b);
        aoe_palette_apply_ror(d, to);
    } while (!done);
}

void aoe_palette_fade_start_ror(AoePaletteFadeRor *f, AoeDrawSystemRor *d,
                                uint32_t color, float seconds, int direction,
                                const uint8_t *base, int32_t keep_a,
                                int32_t keep_b, uint32_t now_ms) {
    f->color = color;
    f->seconds = seconds;
    f->direction = (uint8_t)direction;
    f->keep_a = keep_a;
    f->keep_b = keep_b;
    f->system = d;
    memset(f->base, 0, sizeof f->base);
    memset(f->work, 0, sizeof f->work);
    base_palette(d, base, f->base);
    f->start_ms = now_ms;
    f->seconds_sq = seconds * seconds;
    f->percent = direction != 1 ? 100 : 0;
}

int aoe_palette_fade_step_ror(AoePaletteFadeRor *f, uint32_t now_ms) {
    int done = eased(f->seconds, f->seconds_sq, f->direction,
                     now_ms - f->start_ms, &f->percent);
    aoe_palette_blend_ror(f->base, f->work, f->color, f->percent, f->keep_a,
                          f->keep_b);
    aoe_palette_apply_ror(f->system, f->work);
    return done;
}

int aoe_surface_snapshot_ror(AoeDrawSurfaceRor *s, const char *format,
                             int32_t *counter, const AoeSnapshotOpsRor *ops) {
    int32_t w = s->view.width, h = s->view.height, tries = 0, row;
    uint32_t stride = ((uint32_t)w + 3u) & 0xfffcu;
    uint32_t size = (uint32_t)h * stride + 0x436;
    uint8_t palette[1024], *out, *p;
    char name[64];
    int ok;

    aoe_surface_palette_ror(s, palette);
    for (;;) {
        snprintf(name, sizeof name, format ? format : "C:\\AOE_%03d.BMP",
                 (int)*counter);
        ++tries;
        if (!ops->exists(ops->context, name)) break;
        ++*counter;
        if (tries > 1000) return 0;
    }
    out = calloc(1, size);
    if (!out) return 0;
    out[0] = 'B';
    out[1] = 'M';
    aoe_le_put_u32(out + 2, size);
    aoe_le_put_u32(out + 10, 0x436);
    aoe_le_put_u32(out + 14, 0x28);
    aoe_le_put_u32(out + 18, (uint32_t)w);
    aoe_le_put_u32(out + 22, (uint32_t)h);
    out[26] = 1;
    out[28] = 8;
    for (row = 0; row < 256; ++row) {
        out[0x36 + row * 4] = palette[row * 4 + 2];
        out[0x36 + row * 4 + 1] = palette[row * 4 + 1];
        out[0x36 + row * 4 + 2] = palette[row * 4];
    }
    p = out + 0x436;
    if (aoe_surface_lock_ror(s, 1) || !s->system ||
        s->system->type != 2) {
        for (row = h - 1; row >= 0 && s->rows; --row, p += stride)
            memcpy(p, s->rows[row], (size_t)w);
        aoe_surface_unlock_ror(s);
    }
    ok = ops->write(ops->context, name, out, size);
    free(out);
    return ok;
}
