/* GDI-style text from user-supplied TrueType fonts. */
#include "ui_font.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game_startup.h"   /* aoe_game_font_string_ror (0x00418e70) */

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#include "../third_party/stb/stb_truetype.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/* ---- big-endian table reads, bounded ---- */
static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}
static int16_t be16s(const uint8_t *p) { return (int16_t)(((uint16_t)p[0] << 8) | p[1]); }
static uint16_t be16(const uint8_t *p) { return (uint16_t)(((uint16_t)p[0] << 8) | p[1]); }

/* The offset of table `tag` with at least `need` bytes, or 0. */
static size_t find_table(const uint8_t *d, size_t size, const char *tag, size_t need) {
    uint16_t n, i;
    if (size < 12) return 0;
    n = be16(d + 4);
    for (i = 0; i < n; ++i) {
        size_t rec = 12 + (size_t)i * 16;
        uint32_t off, len;
        if (rec + 16 > size) return 0;
        if (memcmp(d + rec, tag, 4) != 0) continue;
        off = be32(d + rec + 8);
        len = be32(d + rec + 12);
        if (off > size || len > size - off || len < need) return 0;
        return off;
    }
    return 0;
}

int aoe_font_file_open_ror(AoeFontFileRor *f, uint8_t *data, size_t size, int bold) {
    stbtt_fontinfo *info;
    size_t head, hhea, os2;
    memset(f, 0, sizeof *f);
    f->data = data;
    f->size = size;
    f->bold = bold;
    if (!data || size < 12) return 0;
    head = find_table(data, size, "head", 54);
    hhea = find_table(data, size, "hhea", 36);
    os2 = find_table(data, size, "OS/2", 78);
    if (!head || !hhea || !os2) return 0;
    info = calloc(1, sizeof *info);
    if (!info) return 0;
    if (!stbtt_InitFont(info, data, 0)) {
        free(info);
        return 0;
    }
    f->info = info;
    f->units_per_em = be16(data + head + 18);
    f->avg_width = be16s(data + os2 + 2);
    f->win_ascent = be16(data + os2 + 74);
    f->win_descent = be16(data + os2 + 76);
    f->line_gap = be16s(data + hhea + 8);
    f->hhea_span = be16s(data + hhea + 4) - be16s(data + hhea + 6);
    if (f->units_per_em <= 0) {
        aoe_font_file_free_ror(f);
        return 0;
    }
    return 1;
}

int aoe_font_file_load_ror(AoeFontFileRor *f, const char *path, int bold) {
    FILE *fp = fopen(path, "rb");
    uint8_t *data;
    long n;
    memset(f, 0, sizeof *f);
    if (!fp) return 0;
    if (fseek(fp, 0, SEEK_END) != 0 || (n = ftell(fp)) <= 0 || n > (64L << 20) ||
        fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return 0;
    }
    data = malloc((size_t)n);
    if (!data || fread(data, 1, (size_t)n, fp) != (size_t)n) {
        free(data);
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return aoe_font_file_open_ror(f, data, (size_t)n, bold);
}

void aoe_font_file_free_ror(AoeFontFileRor *f) {
    free(f->info);
    free(f->data);
    memset(f, 0, sizeof *f);
}

static int32_t round_px(float v) { return (int32_t)floorf(v + 0.5f); }

int aoe_ui_font_create_ror(AoeUiFontRor *font, const AoeFontFileRor *file,
                           const AoeLogFontRor *lf) {
    float em;
    int32_t cell;
    memset(font, 0, sizeof *font);
    if (!file || !file->info || !lf) return 0;
    cell = file->win_ascent + file->win_descent;
    if (cell <= 0) return 0;
    if (lf->height < 0) em = (float)-lf->height;
    else if (lf->height > 0) em = (float)lf->height * (float)file->units_per_em / (float)cell;
    else em = 12.0f;   /* lfHeight 0: the default size */
    font->file = file;
    font->scale = em / (float)file->units_per_em;
    font->synth_bold = lf->weight >= 600 && !file->bold;
    font->strikeout = lf->strikeout != 0;
    font->ascent = round_px((float)file->win_ascent * font->scale);
    font->descent = round_px((float)file->win_descent * font->scale);
    font->height = font->ascent + font->descent;
    font->ave_width = round_px((float)file->avg_width * font->scale) + (font->synth_bold ? 1 : 0);
    {
        int32_t extra = file->line_gap - (cell - file->hhea_span);
        font->external_leading = extra > 0 ? round_px((float)extra * font->scale) : 0;
    }
    return 1;
}

static const uint16_t cp1252_high[32] = {
    0x20ac, 0x81, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x8d, 0x017d, 0x8f,
    0x90, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x9d, 0x017e, 0x0178};

uint16_t aoe_cp1252_to_unicode_ror(uint8_t c) {
    return (c >= 0x80 && c < 0xa0) ? cp1252_high[c - 0x80] : c;
}

static int32_t advance_px(const AoeUiFontRor *font, uint8_t c) {
    int adv = 0, lsb = 0;
    stbtt_GetCodepointHMetrics((const stbtt_fontinfo *)font->file->info,
                               aoe_cp1252_to_unicode_ror(c), &adv, &lsb);
    return round_px((float)adv * font->scale) + (font->synth_bold ? 1 : 0);
}

int32_t aoe_ui_font_text_width_ror(const AoeUiFontRor *font, const char *text, int32_t count) {
    int32_t i, w = 0;
    if (!font || !font->file || !text) return 0;
    for (i = 0; i < count && text[i]; ++i) w += advance_px(font, (uint8_t)text[i]);
    return w;
}

static void plot(uint8_t *bits, int32_t pitch, const int32_t c[4], int32_t x, int32_t y,
                 uint8_t index) {
    if (x < c[0] || x > c[2] || y < c[1] || y > c[3]) return;
    bits[(size_t)y * (size_t)pitch + (size_t)x] = index;
}

void aoe_ui_font_draw_ror(const AoeUiFontRor *font, uint8_t *bits, int32_t pitch, int32_t width,
                          int32_t height, const int32_t clip[4], int32_t x, int32_t y,
                          const char *text, int32_t count, uint8_t index) {
    const stbtt_fontinfo *info;
    int32_t c[4], pen = x, baseline = y + (font ? font->ascent : 0), i;
    if (!font || !font->file || !bits || !text) return;
    info = (const stbtt_fontinfo *)font->file->info;
    c[0] = clip ? clip[0] : 0;
    c[1] = clip ? clip[1] : 0;
    c[2] = clip ? clip[2] : width - 1;
    c[3] = clip ? clip[3] : height - 1;
    if (c[0] < 0) c[0] = 0;
    if (c[1] < 0) c[1] = 0;
    if (c[2] > width - 1) c[2] = width - 1;
    if (c[3] > height - 1) c[3] = height - 1;
    if (c[0] > c[2] || c[1] > c[3]) return;
    for (i = 0; i < count && text[i]; ++i) {
        int cp = aoe_cp1252_to_unicode_ror((uint8_t)text[i]);
        int w = 0, h = 0, ox = 0, oy = 0, gx, gy, pass;
        unsigned char *glyph = stbtt_GetCodepointBitmap(info, font->scale, font->scale, cp, &w,
                                                        &h, &ox, &oy);
        if (glyph) {
            for (pass = 0; pass <= (font->synth_bold ? 1 : 0); ++pass)
                for (gy = 0; gy < h; ++gy)
                    for (gx = 0; gx < w; ++gx)
                        if (glyph[gy * w + gx] >= 128)
                            plot(bits, pitch, c, pen + ox + gx + pass, baseline + oy + gy, index);
            stbtt_FreeBitmap(glyph, NULL);
        }
        pen += advance_px(font, (uint8_t)text[i]);
    }
    if (font->strikeout) {
        /* GDI places the strike-out near the x-height middle. */
        int32_t sy = baseline - font->ascent / 3, sx;
        for (sx = x; sx < pen; ++sx) plot(bits, pitch, c, sx, sy, index);
    }
}

uint8_t aoe_ui_nearest_index_ror(const uint8_t palette[1024], uint32_t colorref) {
    int32_t r = (int32_t)(colorref & 0xff), g = (int32_t)((colorref >> 8) & 0xff),
            b = (int32_t)((colorref >> 16) & 0xff);
    uint32_t best = 0xffffffffu;
    int i, besti = 0;
    for (i = 0; i < 256; ++i) {
        int32_t dr = palette[4 * i] - r, dg = palette[4 * i + 1] - g, db = palette[4 * i + 2] - b;
        uint32_t d = (uint32_t)(dr * dr + dg * dg + db * db);
        if (d < best) {
            best = d;
            besti = i;
        }
    }
    return (uint8_t)besti;
}

int aoe_ui_font_table_ror(AoeUiFontTableRor *t,
                          void (*load_string)(void *context, int32_t id, char *buf, int32_t size),
                          AoeFontResolveRor resolve, void *context) {
    char charset[16];
    int16_t i;
    int made = 0;
    memset(t, 0, sizeof *t);
    charset[0] = '\0';
    load_string(context, 0x65, charset, (int32_t)sizeof charset);
    for (i = 0; i < AOE_UI_FONT_SLOTS; ++i) {
        AoeLogFontRor lf;
        const AoeFontFileRor *file;
        int second;
        int32_t id = aoe_game_font_string_ror(i, &second);
        int bold;
        if (id == 0 ||
            !aoe_game_font_from_strings_ror(context, load_string, id, (uint8_t)second, charset, &lf))
            continue;
        bold = lf.weight >= 600;
        file = resolve(context, lf.face, bold);
        if (!file) file = resolve(context, "Arial", bold);
        if (!file || !aoe_ui_font_create_ror(&t->fonts[i], file, &lf)) continue;
        t->present[i] = 1;
        t->width[i] = t->fonts[i].ave_width;
        t->height[i] = t->fonts[i].external_leading + t->fonts[i].height;
        ++made;
    }
    return made;
}
