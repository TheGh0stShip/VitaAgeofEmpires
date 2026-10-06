/* The panel system's platform over the user's data and the software
   draw system. */
#include "ui_host.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "palette.h"
#include "slp.h"

#define HOST(c) ((AoeUiHostRor *)(c))
#define TYPE_BINA 0x62696e61u

/* ---- files ---- */

static int name_equal(const char *a, const char *b) {
    for (; *a && *b; ++a, ++b)
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
    return *a == *b;
}

/* `dir`/`name` matched case-insensitively into `out`; 0 when missing. */
static int find_file(const char *dir, const char *name, char *out, size_t size) {
    DIR *d = opendir(dir);
    struct dirent *e;
    int found = 0;
    if (!d) return 0;
    while ((e = readdir(d)) != NULL) {
        if (name_equal(e->d_name, name)) {
            size_t a = strlen(dir), b = strlen(e->d_name);
            if (a + 1 + b + 1 > size) break;   /* too long: treat as missing */
            memcpy(out, dir, a);
            out[a] = '/';
            memcpy(out + a + 1, e->d_name, b + 1);
            found = 1;
            break;
        }
    }
    closedir(d);
    return found;
}

static int find_in(const AoeUiHostRor *h, const char *sub, const char *name, char *out,
                   size_t size) {
    char dir[600];
    if (sub && sub[0]) {
        char subdir[600];
        if (!find_file(h->root, sub, subdir, sizeof subdir)) return 0;
        snprintf(dir, sizeof dir, "%s", subdir);
    } else {
        snprintf(dir, sizeof dir, "%s", h->root);
    }
    return find_file(dir, name, out, size);
}

static void open_archive(AoeUiHostRor *h, const char *sub, const char *name) {
    char path[700];
    DrsArchive *a;
    if (h->archive_count >= AOE_UI_HOST_ARCHIVES || !find_in(h, sub, name, path, sizeof path))
        return;
    a = &h->archives[h->archive_count];
    if (drs_open(a, path) == 0) return;
    h->order[h->archive_count] = a;
    h->archive_count++;
}

uint8_t *aoe_ui_host_resource_ror(AoeUiHostRor *h, uint32_t type, int32_t id, uint32_t *size) {
    DrsMatch m;
    uint8_t *data;
    if (id < 0 || !drs_find_first(h->order, h->archive_count, type, (uint32_t)id, &m)) return NULL;
    data = malloc(m.entry->size ? m.entry->size : 1);
    if (!data) return NULL;
    if (drs_read(m.archive, m.entry, data, m.entry->size) == 0) {
        free(data);
        return NULL;
    }
    if (size) *size = m.entry->size;
    return data;
}

/* ---- shapes ---- */

static uint8_t *shape_load_file(void *c, const char *path, uint32_t *size) {
    (void)c; (void)path; (void)size;
    return NULL;   /* loose files are off: everything comes from the archives */
}

static uint8_t *shape_load_resource(void *c, uint32_t type, int32_t id, uint32_t *size,
                                    int32_t *owned) {
    uint8_t *data = aoe_ui_host_resource_ror(HOST(c), type, id, size);
    *owned = 1;
    return data;
}

static void shape_release(void *c, const uint8_t *data, uint32_t size) {
    (void)c; (void)data; (void)size;
}

/* An SLP frame with its top-left at (x, y), clipped to the surface's clip
   (the original sprite rasterizer's job). */
static void draw_slp(void *c, AoeShapeSurfaceRor *dst, const AoeShapeRor *s, int32_t frame,
                     int32_t x, int32_t y, const uint8_t *table, int batched, int mirrored) {
    SlpFrame f;
    int32_t l = dst->clip_left, t = dst->clip_top, r = dst->clip_right, b = dst->clip_bottom;
    (void)c; (void)table; (void)batched;
    if (!s->slp || dst->direction != 1 || !dst->bits) return;
    if (l < 0) l = 0;
    if (t < 0) t = 0;
    if (r > dst->width - 1) r = dst->width - 1;
    if (b > dst->height - 1) b = dst->height - 1;
    if (l > r || t > b) return;
    memset(&f, 0, sizeof f);
    if (!slp_decode_frame(s->slp, s->size, (uint32_t)frame, &f)) return;
    slp_blit_graphic_indexed(&f, dst->bits + (size_t)t * (size_t)dst->pitch + (size_t)l,
                             (size_t)dst->pitch * (size_t)(b - t) + (size_t)(r - l + 1),
                             (uint32_t)(r - l + 1), (uint32_t)(b - t + 1), (size_t)dst->pitch,
                             x - l + f.hotspot_x, y - t + f.hotspot_y, mirrored, 0, NULL);
    slp_free_frame(&f);
}

static void *ez_load_shape(void *c, const char *file, int32_t id) {
    AoeShapeRor *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    aoe_shape_load_ror(s, file, id, &HOST(c)->shape_ops);
    if (!aoe_shape_loaded_ror(s)) {
        aoe_shape_free_ror(s, &HOST(c)->shape_ops);
        free(s);
        return NULL;
    }
    return s;
}

static void ez_free_shape(void *c, void *shape) {
    if (!shape) return;
    aoe_shape_free_ror(shape, &HOST(c)->shape_ops);
    free(shape);
}

/* ---- info files and palettes ---- */

static char *ez_load_info(void *c, const char *file, int32_t id) {
    uint32_t size = 0;
    uint8_t *data;
    char *text;
    if (file) return NULL;   /* loose files are off */
    data = aoe_ui_host_resource_ror(HOST(c), TYPE_BINA, id, &size);
    if (!data) return NULL;
    text = malloc((size_t)size + 1);
    if (text) {
        memcpy(text, data, size);
        text[size] = '\0';
    }
    free(data);
    return text;
}

static uintptr_t create_palette(void *c, const char *name, int32_t id) {
    AoeUiHostRor *h = HOST(c);
    Palette pal;
    uint32_t size = 0;
    uint8_t *data;
    int i, slot = -1;
    for (i = 0; i < AOE_UI_HOST_PALETTES; ++i)
        if (!h->palettes[i].used) { slot = i; break; }
    if (slot < 0) return 0;
    data = aoe_ui_host_resource_ror(h, TYPE_BINA, id, &size);
    if (!data) return 0;
    if (!palette_parse_jasc(data, size, &pal)) {
        free(data);
        return 0;
    }
    free(data);
    {
        AoeUiHostPaletteRor *p = &h->palettes[slot];
        memset(p, 0, sizeof *p);
        snprintf(p->name, sizeof p->name, "%s", name ? name : "");
        p->id = id;
        for (i = 0; i < 256; ++i) {
            p->entries[4 * i] = pal.colors[i].r;
            p->entries[4 * i + 1] = pal.colors[i].g;
            p->entries[4 * i + 2] = pal.colors[i].b;
        }
        p->used = 1;
        return (uintptr_t)p;
    }
}

/* The draw system's window_palette: the entries of one of our palette
   handles (the original's HPALETTE, GetPaletteEntries). */
static int window_palette(void *c, void *hpalette, uint8_t out[1024]) {
    (void)c;
    if (!hpalette) return 0;
    memcpy(out, ((AoeUiHostPaletteRor *)hpalette)->entries, 1024);
    return 1;
}

static void delete_palette(void *c, uintptr_t palette) {
    AoeUiHostPaletteRor *p = (AoeUiHostPaletteRor *)palette;
    (void)c;
    if (p) p->used = 0;
}

static uintptr_t ez_area_palette(void *c, AoeUiAreaRor *area) { (void)area; return HOST(c)->area_palette; }
static uintptr_t ez_default_palette(void *c) { return HOST(c)->default_palette; }

static void ez_set_area_palette(void *c, AoeUiAreaRor *area, uintptr_t palette) {
    AoeUiHostRor *h = HOST(c);
    (void)area;
    if (!palette) return;
    h->area_palette = palette;
    aoe_draw_set_palette_ror(&h->draw, 0, 256, ((AoeUiHostPaletteRor *)palette)->entries);
}

/* ---- fonts and text ---- */

static int ez_font(void *c, int32_t index, uintptr_t *font, int32_t *w, int32_t *hgt) {
    AoeUiHostRor *h = HOST(c);
    if (index < 0 || index >= AOE_UI_FONT_SLOTS || !h->fonts.present[index]) return 0;
    *font = (uintptr_t)&h->fonts.fonts[index];
    *w = h->fonts.width[index];
    *hgt = h->fonts.height[index];
    return 1;
}

static void host_string(void *c, int32_t id, char *out, int size) {
    aoe_lang_string_ror(&HOST(c)->lang_x, &HOST(c)->lang_b, id, out, size);
}

static void load_string(void *c, int32_t id, char *out, int32_t size) {
    host_string(c, id, out, size);
}

static const AoeFontFileRor *resolve_font(void *c, const char *face, int bold) {
    AoeUiHostRor *h = HOST(c);
    if (name_equal(face, "Arial")) {
        if (bold && h->arial_bold.info) return &h->arial_bold;
        return h->arial.info ? &h->arial : NULL;
    }
    if (name_equal(face, "Copperplate Gothic Light"))
        return h->copperplate.info ? &h->copperplate : NULL;
    return NULL;
}

/* ---- pictures (pnl_pic) ---- */

static void *pic_load_shape(void *c, const char *name, int32_t id) {
    (void)name;   /* loose files are off: the resource ID names it */
    return ez_load_shape(c, NULL, id);
}
static int pic_shape_valid(void *c, void *shape) { (void)c; return shape && aoe_shape_loaded_ror(shape); }
static int pic_shape_size(void *c, void *shape, int32_t frame, int16_t *w, int16_t *hgt) {
    (void)c;
    return aoe_shape_size_ror(shape, frame, w, hgt);
}
static void *pic_load_bitmap(void *c, const char *name) { (void)c; (void)name; return NULL; }
static void pic_bitmap_size(void *c, void *b, int32_t *w, int32_t *hgt) { (void)c; (void)b; *w = *hgt = 0; }
static void pic_free_bitmap(void *c, void *b) { (void)c; (void)b; }
static void pic_draw_bitmap(void *c, AoeUiAreaRor *a, void *b, int32_t x, int32_t y) {
    (void)c; (void)a; (void)b; (void)x; (void)y;
}
static void draw_shape(void *c, AoeUiAreaRor *area, void *shape, int32_t frame, int32_t x,
                       int32_t y);
static void pic_draw_shape(void *c, AoeUiAreaRor *area, void *shape, int32_t x, int32_t y,
                           int32_t frame) {
    draw_shape(c, area, shape, frame, x, y);
}

static int32_t text_width(void *c, uintptr_t font, const char *text, int32_t count) {
    (void)c;
    return aoe_ui_font_text_width_ror((const AoeUiFontRor *)font, text, count);
}

static AoeDrawSurfaceRor *area_surface(AoeUiAreaRor *area) {
    return area ? area->surface : NULL;
}

static void set_clip(int32_t out[4], const AoeUiRectRor *r, int32_t w, int32_t hgt) {
    if (r) {
        out[0] = r->left; out[1] = r->top; out[2] = r->right; out[3] = r->bottom;
    } else {
        out[0] = 0; out[1] = 0; out[2] = w - 1; out[3] = hgt - 1;
    }
}

static int text_begin(void *c, AoeUiAreaRor *area, const AoeUiRectRor *clip, uintptr_t font) {
    AoeUiHostRor *h = HOST(c);
    AoeDrawSurfaceRor *s = area_surface(area);
    if (!s || !font) return 0;
    h->text_bits = aoe_surface_lock_ror(s, 1);
    if (!h->text_bits) return 0;
    h->text_font = (const AoeUiFontRor *)font;
    set_clip(h->text_clip, clip, s->view.width, s->view.height);
    return 1;
}

static void text_clip(void *c, const AoeUiRectRor *clip) {
    AoeUiHostRor *h = HOST(c);
    set_clip(h->text_clip, clip, h->area.width, h->area.height);
}

static void text_out(void *c, int32_t x, int32_t y, const char *text, int32_t count,
                     uint32_t color) {
    AoeUiHostRor *h = HOST(c);
    AoeDrawSurfaceRor *s = h->area.surface;
    if (!h->text_bits || !s) return;
    aoe_ui_font_draw_ror(h->text_font, h->text_bits, s->view.pitch, s->view.width,
                         s->view.height, h->text_clip, x, y, text, count,
                         aoe_ui_nearest_index_ror(h->draw.palette, color));
}

static void text_end(void *c) {
    AoeUiHostRor *h = HOST(c);
    if (h->text_bits && h->area.surface) aoe_surface_unlock_ror(h->area.surface);
    h->text_bits = NULL;
    h->text_font = NULL;
}

/* DrawTextExA, the flags this UI uses: DT_CENTER 1, DT_RIGHT 2,
   DT_VCENTER 4, DT_SINGLELINE 0x20; one line. */
static void text_rect(void *c, const char *text, const AoeUiRectRor *rect, uint32_t flags,
                      uint32_t color) {
    AoeUiHostRor *h = HOST(c);
    int32_t n = (int32_t)strlen(text), w, x = rect->left, y = rect->top;
    if (!h->text_font) return;
    w = aoe_ui_font_text_width_ror(h->text_font, text, n);
    if (flags & 1) x = rect->left + (rect->right - rect->left - w) / 2;
    else if (flags & 2) x = rect->right - w;
    if (flags & 4) y = rect->top + (rect->bottom - rect->top - h->text_font->height) / 2;
    text_out(c, x, y, text, n, color);
}

/* ---- the rest of the panel platform ---- */

static void invalidate(void *c, AoeUiAreaRor *area, const AoeUiRectRor *r) { (void)c; (void)area; (void)r; }
static void validate(void *c, AoeUiAreaRor *area, const AoeUiRectRor *r) { (void)c; (void)area; (void)r; }

static void fill(void *c, const AoeUiRectRor *r, uint8_t color) {
    AoeUiHostRor *h = HOST(c);
    AoeDrawSurfaceRor *s = h->area.surface;
    int32_t rc[4];
    if (!s || !r) return;
    rc[0] = r->left > h->clip[0] ? r->left : h->clip[0];
    rc[1] = r->top > h->clip[1] ? r->top : h->clip[1];
    rc[2] = r->right < h->clip[2] ? r->right : h->clip[2];
    rc[3] = r->bottom < h->clip[3] ? r->bottom : h->clip[3];
    if (rc[0] > rc[2] || rc[1] > rc[3]) return;
    aoe_surface_fill_ror(s, rc, color);
}

static void clip(void *c, AoeUiAreaRor *area, const AoeUiRectRor *r) {
    AoeUiHostRor *h = HOST(c);
    AoeDrawSurfaceRor *s = area_surface(area);
    set_clip(h->clip, r, h->area.width, h->area.height);
    if (s) aoe_surface_set_clip_ror(s, h->clip[0], h->clip[1], h->clip[2], h->clip[3]);
}

static void capture(void *c, AoeUiAreaRor *area) { (void)c; (void)area; }
static void release(void *c) { (void)c; }
static uint32_t now_ms(void *c) { return HOST(c)->now_ms ? HOST(c)->now_ms(HOST(c)->now_context) : 0; }
static int key_down(void *c, int vk) { return vk >= 0 && vk < 256 && HOST(c)->keys[vk]; }
static int iconic(void *c, AoeUiAreaRor *area) { (void)c; (void)area; return 0; }

static void draw_shape(void *c, AoeUiAreaRor *area, void *shape, int32_t frame, int32_t x,
                       int32_t y) {
    AoeDrawSurfaceRor *s = area_surface(area);
    if (!s || !shape || !aoe_surface_lock_ror(s, 1)) return;
    aoe_shape_draw_ror(shape, &s->view, x, y, frame, 0, NULL, &HOST(c)->shape_draw);
    aoe_surface_unlock_ror(s);
}

static uint32_t get_sys_color(void *c, int32_t index) { (void)c; return index == 8 ? 0 : 0xffffff; }
static void set_sys_colors(void *c, uint32_t w, uint32_t t) { (void)c; (void)w; (void)t; }

/* ---- open / close / paint ---- */

static int load_language(AoeUiHostRor *h) {
    static const char *const xnames[] = {"languagex.dll", "LANGUA_1.DLL"};
    static const char *const subs[] = {"", "data2", "data", "GAME"};
    char path[700];
    size_t i, j;
    for (i = 0; i < 2 && !h->lang_x.string_dir; ++i)
        for (j = 0; j < 4; ++j)
            if (find_in(h, subs[j], xnames[i], path, sizeof path) &&
                aoe_pe_strings_load(&h->lang_x, path))
                break;
    for (j = 0; j < 4; ++j)
        if (find_in(h, subs[j], "language.dll", path, sizeof path) &&
            aoe_pe_strings_load(&h->lang_b, path))
            break;
    return h->lang_x.string_dir || h->lang_b.string_dir;
}

static void load_font(AoeUiHostRor *h, AoeFontFileRor *f, const char *name, int bold) {
    char path[700];
    if (find_in(h, "fonts", name, path, sizeof path)) aoe_font_file_load_ror(f, path, bold);
}

int aoe_ui_host_open_ror(AoeUiHostRor *h, const char *root, int32_t width, int32_t height,
                         uint32_t (*now)(void *context), void *now_context, char *error,
                         size_t error_size) {
    memset(h, 0, sizeof *h);
    snprintf(h->root, sizeof h->root, "%s", root);
    h->now_ms = now;
    h->now_context = now_context;
    /* The expansion's archives first, then the base game's (0x004fe600). */
    open_archive(h, "data2", "INTERFAC.DRS");
    open_archive(h, "data2", "GRAPHICS.DRS");
    open_archive(h, "data2", "SOUNDS.DRS");
    open_archive(h, "data", "Interfac.drs");
    open_archive(h, "data", "graphics.drs");
    open_archive(h, "data", "sounds.drs");
    open_archive(h, "data", "Terrain.drs");
    open_archive(h, "data", "Border.drs");
    if (h->archive_count == 0) {
        snprintf(error, error_size, "no DRS archives under %s (data/, data2/)", root);
        return 0;
    }
    if (!load_language(h)) {
        snprintf(error, error_size, "no language library (languagex.dll / language.dll) under %s",
                 root);
        aoe_ui_host_close_ror(h);
        return 0;
    }
    load_font(h, &h->arial, "arial.ttf", 0);
    load_font(h, &h->arial_bold, "arialbd.ttf", 1);
    load_font(h, &h->copperplate, "COPRGTL.TTF", 0);
    aoe_ui_font_table_ror(&h->fonts, load_string, resolve_font, h);

    /* The draw system, full screen 8-bit (0x00436c30 open). */
    aoe_soft_draw_init_ror(&h->display, &h->draw_ops, width, height);
    h->draw_ops.window_palette = window_palette;
    aoe_draw_clear_ror(&h->draw, &h->draw_ops);
    if (!aoe_draw_open_ror(&h->draw, NULL, NULL, NULL, 2, 2, width, height, 0)) {
        snprintf(error, error_size, "the draw system did not open");
        aoe_ui_host_close_ror(h);
        return 0;
    }
    h->area.window = h;
    h->area.width = width;
    h->area.height = height;
    h->area.surface = h->draw.back;
    set_clip(h->clip, NULL, width, height);

    h->shape_ops.context = h;
    h->shape_ops.load_count = &h->shape_loads;
    h->shape_ops.load_file = shape_load_file;
    h->shape_ops.load_resource = shape_load_resource;
    h->shape_ops.release = shape_release;
    h->shape_draw.context = h;
    h->shape_draw.batching = &h->shape_batching;
    h->shape_draw.drawn = &h->shapes_drawn;
    h->shape_draw.draw_slp = draw_slp;

    h->platform.context = h;
    h->platform.invalidate = invalidate;
    h->platform.validate = validate;
    h->platform.fill = fill;
    h->platform.clip = clip;
    h->platform.capture = capture;
    h->platform.release = release;
    h->platform.now_ms = now_ms;
    h->platform.key_down = key_down;
    h->platform.iconic = iconic;
    h->platform.string = host_string;
    h->platform.create_palette = create_palette;
    h->platform.delete_palette = delete_palette;
    h->platform.text_width = text_width;
    h->platform.text_begin = text_begin;
    h->platform.text_clip = text_clip;
    h->platform.text_out = text_out;
    h->platform.text_end = text_end;
    h->platform.text_rect = text_rect;
    h->platform.draw_shape = draw_shape;
    h->platform.get_sys_color = get_sys_color;
    h->platform.set_sys_colors = set_sys_colors;

    h->ez_ops.context = h;
    h->ez_ops.font = ez_font;
    h->ez_ops.load_shape = ez_load_shape;
    h->ez_ops.free_shape = ez_free_shape;
    h->ez_ops.load_info = ez_load_info;
    h->ez_ops.area_palette = ez_area_palette;
    h->ez_ops.default_palette = ez_default_palette;
    h->ez_ops.set_area_palette = ez_set_area_palette;
    h->ez_ops.shape_ops = &h->shape_draw;
    h->ez_ops.caret_blink_ms = 500;
    h->ez_ops.help_mode = &h->help_mode;

    h->picture_ops.context = h;
    h->picture_ops.load_shape = pic_load_shape;
    h->picture_ops.shape_valid = pic_shape_valid;
    h->picture_ops.free_shape = ez_free_shape;
    h->picture_ops.shape_size = pic_shape_size;
    h->picture_ops.load_bitmap = pic_load_bitmap;
    h->picture_ops.bitmap_size = pic_bitmap_size;
    h->picture_ops.free_bitmap = pic_free_bitmap;
    h->picture_ops.draw_bitmap = pic_draw_bitmap;
    h->picture_ops.draw_shape = pic_draw_shape;

    /* The game's default palette: base Interfac.drs 50500. */
    h->default_palette = create_palette(h, "palette", 50500);
    if (h->default_palette) ez_set_area_palette(h, &h->area, h->default_palette);

    aoe_ui_system_init_ror(&h->system, &h->platform);
    h->system.app_active = 1;
    return 1;
}

void aoe_ui_host_close_ror(AoeUiHostRor *h) {
    size_t i;
    if (h->system.platform) aoe_ui_system_release_ror(&h->system);
    if (h->draw.ops) aoe_draw_shutdown_ror(&h->draw);
    for (i = 0; i < h->archive_count; ++i) drs_close(&h->archives[i]);
    aoe_pe_strings_free(&h->lang_x);
    aoe_pe_strings_free(&h->lang_b);
    aoe_font_file_free_ror(&h->arial);
    aoe_font_file_free_ror(&h->arial_bold);
    aoe_font_file_free_ror(&h->copperplate);
    memset(h, 0, sizeof *h);
}

void aoe_ui_host_paint_ror(AoeUiHostRor *h) {
    AoeUiPanelRor *screen = h->system.screen;
    if (screen) screen->vt->handle_paint(screen);
    aoe_draw_present_ror(&h->draw, NULL);
}

/* ---- input ---- */

int32_t aoe_ui_host_message_ror(AoeUiHostRor *h, uint32_t msg, uint32_t wparam, uint32_t lparam) {
    AoeUiPanelRor *screen = h->system.screen;
    if (msg == 0x100 || msg == 0x104) h->keys[wparam & 0xff] = 1;
    if (msg == 0x101 || msg == 0x105) h->keys[wparam & 0xff] = 0;
    if (!screen) return 0;
    return screen->vt->handle_message(screen, h->area.window, msg, wparam, lparam);
}

static uint32_t mouse_wparam(const AoeUiHostRor *h) {
    return h->buttons | (h->keys[0x10] ? 4u : 0u) | (h->keys[0x11] ? 8u : 0u);
}

static uint32_t mouse_lparam(int32_t x, int32_t y) {
    return ((uint32_t)(uint16_t)y << 16) | (uint16_t)x;
}

void aoe_ui_host_mouse_move_ror(AoeUiHostRor *h, int32_t x, int32_t y) {
    h->mouse_x = x;
    h->mouse_y = y;
    aoe_ui_host_message_ror(h, 0x200, mouse_wparam(h), mouse_lparam(x, y));
}

void aoe_ui_host_mouse_button_ror(AoeUiHostRor *h, int button, int down, int32_t x, int32_t y) {
    uint32_t bit = button == 2 ? 2u : 1u, msg;
    h->mouse_x = x;
    h->mouse_y = y;
    if (down) h->buttons |= bit; else h->buttons &= ~bit;
    h->keys[button == 2 ? 2 : 1] = (uint8_t)(down != 0);   /* VK_LBUTTON 1, VK_RBUTTON 2 */
    msg = button == 2 ? (down ? 0x204u : 0x205u) : (down ? 0x201u : 0x202u);
    aoe_ui_host_message_ror(h, msg, mouse_wparam(h), mouse_lparam(x, y));
}

void aoe_ui_host_click_ror(AoeUiHostRor *h, int32_t x, int32_t y) {
    aoe_ui_host_mouse_move_ror(h, x, y);
    aoe_ui_host_mouse_button_ror(h, 1, 1, x, y);
    aoe_ui_host_mouse_button_ror(h, 1, 0, x, y);
}

void aoe_ui_host_key_ror(AoeUiHostRor *h, int32_t vk, int down) {
    aoe_ui_host_message_ror(h, down ? 0x100u : 0x101u, (uint32_t)vk, down ? 1u : 0xc0000001u);
}

void aoe_ui_host_char_ror(AoeUiHostRor *h, int32_t c) {
    aoe_ui_host_message_ror(h, 0x102u, (uint32_t)c, 1u);
}

void aoe_ui_host_idle_ror(AoeUiHostRor *h) {
    AoeUiPanelRor *screen = h->system.screen;
    if (screen) screen->vt->handle_idle(screen);
}
