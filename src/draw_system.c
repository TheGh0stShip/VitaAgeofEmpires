#include "draw_system.h"

#include <stdlib.h>
#include <string.h>

enum { TYPE_GDI = 1, TYPE_DDRAW = 2, SCREEN_WINDOW = 1, SCREEN_FULL = 2 };

/* ---- result messages ---- */

int32_t aoe_draw_lost_message_ror(int32_t result) {
    switch ((uint32_t)result) {
    case 0x887601C2u: return 0x6a6;
    case 0x80070057u: return 0x6a5;
    case 0x88760082u: return 0x6a4;
    case 0: return 0;
    default: return 0x707;
    }
}

int32_t aoe_draw_restore_message_ror(int32_t result) {
    switch ((uint32_t)result) {
    case 0x88760082u: return 0x70b;
    case 0x8007000Eu: return 0x70e;
    case 0x80004001u: return 0x70f;
    case 0x80004005u: return 0x708;
    case 0x80070057u: return 0x70c;
    case 0x8876005Fu: return 0x70a;
    case 0x8876024Cu: return 0x709;
    case 0x887600E1u: return 0x70d;
    case 0x8876024Bu: return 0x710;
    case 0: return 0;
    default: return 0x76b;
    }
}

int32_t aoe_draw_blt_message_ror(int32_t result) {
    switch ((uint32_t)result) {
    case 0x88760118u: return 0x8a5;
    case 0x88760082u: return 0x899;
    case 0x80070057u: return 0x89a;
    case 0x80004001u: return 0x8a3;
    case 0x80004005u: return 0x898;
    case 0x8876006Eu: return 0x89c;
    case 0x88760096u: return 0x89d;
    case 0x887600B4u: return 0x89e;
    case 0x887600CDu: return 0x8a0;
    case 0x887600FAu: return 0x8a4;
    case 0x887601C2u: return 0x8a2;
    case 0x88760122u: return 0x8a6;
    case 0x88760136u: return 0x8a7;
    case 0x88760154u: return 0x89b;
    case 0x887601AEu: return 0x8a8;
    case 0x8876023Fu: return 0x89f;
    case 0x88760240u: return 0x8a1;
    case 0: return 0;
    default: return 0x8fb;
    }
}

int32_t aoe_draw_create_message_ror(int32_t result) {
    switch ((uint32_t)result) {
    case 0x887600D4u: return 0x901;
    case 0x88760064u: return 0x902;
    case 0x80070057u: return 0x8fe;
    case 0x80004005u: return 0x8fc;
    case 0x8007000Eu: return 0x908;
    case 0x8876005Fu: return 0x90b;
    case 0x88760082u: return 0x8fd;
    case 0x88760091u: return 0x903;
    case 0x887600B4u: return 0x904;
    case 0x88760233u: return 0x900;
    case 0x887600E1u: return 0x907;
    case 0x887600E6u: return 0x905;
    case 0x88760154u: return 0x906;
    case 0x8876017Cu: return 0x8ff;
    case 0x88760234u: return 0x909;
    case 0x88760235u: return 0x90a;
    case 0: return 0;
    default: return 0x95f;
    }
}

/* ---- draw system ---- */

void aoe_draw_clear_ror(AoeDrawSystemRor *d, const AoeDrawOpsRor *ops) {
    memset(d, 0, sizeof *d);
    d->bits = 8;
    d->hardware_clear = 1;
    d->ops = ops;
    /* The original also seeds the assembly renderer's colour masks. */
}

static void blank_primary(AoeDrawSystemRor *d) {
    d->ops->fill(d->ops->context, d->primary_surface, NULL, 0);
}

void aoe_draw_shutdown_ror(AoeDrawSystemRor *d) {
    const AoeDrawOpsRor *ops = d->ops;
    if (d->screen == SCREEN_FULL && d->primary_surface) blank_primary(d);
    aoe_draw_free_surfaces_ror(d);
    if (!d->device) return;
    if (d->palette_object) {
        ops->release(ops->context, d->palette_object);
        d->palette_object = NULL;
    }
    if (d->clipper) {
        ops->release(ops->context, d->clipper);
        d->clipper = NULL;
    }
    if (d->mode_set) {
        ops->restore_mode(ops->context, d->device);
        d->mode_set = 0;
    }
    ops->release(ops->context, d->device);
    d->device = NULL;
}

void aoe_draw_note_mode_ror(AoeDrawSystemRor *d, int32_t width,
                            int32_t height, int32_t bits) {
    if (bits == 8) {
        if (width == 640 && height == 480) d->modes[0] = 1;
        else if (width == 800 && height == 600) d->modes[1] = 1;
        else if (width == 1024 && height == 768) d->modes[2] = 1;
        else if (width == 1280 && height == 1024) d->modes[3] = 1;
        else if (width == 320 && height == 200) d->modes[4] = 1;
        else if (width == 320 && height == 240) d->modes[5] = 1;
    } else if (bits == 16) {
        if (width == 640 && height == 480) d->modes[8] = 1;
        else if (width == 800 && height == 600) d->modes[9] = 1;
        else if (width == 1024 && height == 768) d->modes[10] = 1;
        else if (width == 320 && height == 200) d->modes[6] = 1;
        else if (width == 320 && height == 240) d->modes[7] = 1;
    }
}

static void note_mode_cb(void *user, int32_t width, int32_t height,
                         int32_t bits) {
    aoe_draw_note_mode_ror(user, width, height, bits);
}

void aoe_draw_probe_modes_ror(AoeDrawSystemRor *d, int use_device) {
    const AoeDrawOpsRor *ops = d->ops;
    int had;
    if (!use_device) {
        int32_t w;
        d->modes[0] = 1;
        w = ops->desktop_width ? ops->desktop_width(ops->context) : 0;
        if (w > 799) d->modes[1] = 1;
        if (w > 0x3ff) d->modes[2] = 1;
        if (w > 0x4ff) d->modes[3] = 1;
        return;
    }
    had = d->device != NULL;
    if (!had && ops->create(ops->context, &d->device) != 0) {
        d->device = NULL;
        return;
    }
    ops->enum_modes(ops->context, d->device, note_mode_cb, d);
    if (!had) {
        ops->release(ops->context, d->device);
        d->device = NULL;
    }
}

int32_t aoe_draw_mode_available_ror(const AoeDrawSystemRor *d, int32_t width,
                                    int32_t height, int32_t bits) {
    if (bits == 8) {
        if (width == 800) return d->modes[1];
        if (width == 320) {
            if (height == 200) return d->modes[4];
            if (height == 240) return d->modes[5];
            return 0;
        }
        if (width == 640) return d->modes[0];
        if (width == 1024) return d->modes[2];
        if (width == 1280) return d->modes[3];
    } else if (bits == 16) {
        if (width == 800) return d->modes[9];
        if (width == 320) {
            if (height == 200) return d->modes[6];
            if (height == 240) return d->modes[7];
            return 0;
        }
        if (width == 640) return d->modes[8];
        if (width == 1024) return d->modes[10];
    }
    return 0;
}

static int fail(AoeDrawSystemRor *d, int32_t error) {
    d->error = error;
    return 0;
}

int aoe_draw_open_ror(AoeDrawSystemRor *d, void *instance, void *window,
                      void *hpalette, uint8_t type, uint8_t screen,
                      int32_t width, int32_t height, uint32_t flags) {
    const AoeDrawOpsRor *ops = d->ops;
    uint8_t entries[1024];
    uint32_t caps = 0, key_caps = 0;

    d->window = window;
    d->hpalette = hpalette;
    d->instance = instance;
    d->screen = screen;
    d->width = width;
    d->type = type;
    d->height = height;
    d->flags = flags;
    if (type == TYPE_GDI) return 0;
    if (ops->create(ops->context, &d->device) != 0) return fail(d, 1);
    aoe_draw_probe_modes_ror(d, 1);
    if (d->screen == SCREEN_WINDOW) {
        int32_t w, h, bits;
        if (ops->set_level(ops->context, d->device, window, 8) != 0)
            return fail(d, 1);
        if (ops->get_mode(ops->context, d->device, &w, &h, &bits) != 0)
            return fail(d, 1);
        if (bits != 8) return fail(d, 2);
    } else {
        if (ops->set_level(ops->context, d->device, window, 0x11) != 0)
            return fail(d, 1);
        d->bits = 8;
        if (ops->set_mode(ops->context, d->device, d->width, d->height, 8) !=
            0)
            return fail(d, 1);
        d->mode_set = 1;
    }
    if (!hpalette || !ops->window_palette ||
        !ops->window_palette(ops->context, hpalette, entries))
        memset(entries, 0, sizeof entries);
    memset(d->palette, 0, sizeof d->palette);
    if (ops->create_palette(ops->context, d->device, entries,
                            &d->palette_object) != 0)
        return fail(d, 1);
    ops->palette_get(ops->context, d->palette_object, 0, 256, d->palette);
    if (ops->create_clipper(ops->context, d->device, &d->clipper) != 0)
        return fail(d, 1);
    if (ops->clipper_window(ops->context, d->clipper, window) != 0)
        return fail(d, 1);
    if (!aoe_draw_make_surfaces_ror(d)) return fail(d, 1);
    d->bank_switched = 0;
    if (ops->get_caps(ops->context, d->device, &caps, &key_caps) == 0) {
        if (caps & 0x8000000u) d->bank_switched = 1;
        if ((caps & 0x400000u) && (key_caps & 0x200u)) d->key_blits = 1;
    }
    if (d->screen == SCREEN_FULL) blank_primary(d);
    return 1;
}

/* Does the display still show this system's 8-bit mode? */
static int mode_matches(AoeDrawSystemRor *d) {
    int32_t w, h, bits;
    if (d->ops->get_mode(d->ops->context, d->device, &w, &h, &bits) != 0)
        return 1;
    return w == d->width && h == d->height && bits == 8;
}

int aoe_draw_change_mode_ror(AoeDrawSystemRor *d, int32_t width,
                             int32_t height, int32_t bits) {
    const AoeDrawOpsRor *ops = d->ops;
    int32_t old_w, old_h;
    int32_t r;
    if (d->type != TYPE_DDRAW || d->screen != SCREEN_FULL) return 0;
    d->changing_mode = 1;
    blank_primary(d);
    old_w = d->width;
    old_h = d->height;
    d->width = width;
    d->height = height;
    r = ops->set_mode(ops->context, d->device, width, height, bits);
    d->changing_mode = 0;
    if (r != 0) {
        d->width = old_w;
        d->height = old_h;
        return 0;
    }
    d->bits = bits;
    d->mode_set = 1;
    if (d->primary_surface) {
        ops->release(ops->context, d->primary_surface);
        d->primary_surface = NULL;
    }
    aoe_draw_make_surfaces_ror(d);
    if (d->back) aoe_surface_allocate_ror(d->back, d->width, d->height, 0);
    if (ops->size_message)
        ops->size_message(ops->context, d->window, width & 0xffff,
                          height & 0xffff);
    return 1;
}

void aoe_draw_blank_ror(AoeDrawSystemRor *d) {
    if (d->screen == SCREEN_FULL && d->primary_surface) blank_primary(d);
}

int aoe_draw_restore_ror(AoeDrawSystemRor *d) {
    const AoeDrawOpsRor *ops = d->ops;
    AoeDrawNodeRor *n;
    int result = 0;
    int32_t r;
    if (d->type != TYPE_DDRAW) return 0;
    if (!d->primary_surface || !d->back || !d->back->surface) return 3;
    r = ops->is_lost(ops->context, d->primary_surface);
    if (r != 0) {
        aoe_draw_lost_message_ror(r);
        if (!mode_matches(d)) return 1;
        r = ops->restore(ops->context, d->primary_surface);
        if (r != 0) {
            aoe_draw_restore_message_ror(r);
            return 3;
        }
        d->restored = 1;
        blank_primary(d);
        result = 2;
    }
    for (n = d->surfaces; n; n = n->next)
        if (n->surface != d->primary && aoe_surface_restore_ror(n->surface) == 3)
            return 3;
    return result;
}

void aoe_draw_clear_restored_ror(AoeDrawSystemRor *d) {
    AoeDrawNodeRor *n;
    d->restored = 0;
    for (n = d->surfaces; n; n = n->next) n->surface->restored = 0;
}

void aoe_draw_free_surfaces_ror(AoeDrawSystemRor *d) {
    if (d->back) {
        aoe_surface_free_ror(d->back);
        free(d->back);
        d->back = NULL;
    }
    if (!d->primary_surface) return;
    if (d->primary) {
        aoe_surface_free_ror(d->primary);
        free(d->primary);
    }
    d->primary = NULL;
    d->ops->release(d->ops->context, d->primary_surface);
    d->primary_surface = NULL;
}

static void surface_size(AoeDrawSystemRor *d, int32_t *w, int32_t *h) {
    if (d->screen == SCREEN_WINDOW) {
        d->ops->client_size(d->ops->context, d->window, w, h);
    } else {
        *w = d->width;
        *h = d->height;
    }
}

static AoeDrawSurfaceRor *new_surface(AoeDrawSystemRor *d, const char *name,
                                      int32_t shares) {
    AoeDrawSurfaceRor *s = malloc(sizeof *s);
    int32_t w, h;
    if (!s) return NULL;
    if (!aoe_surface_init_ror(s, name, 0)) {
        free(s);
        return NULL;
    }
    surface_size(d, &w, &h);
    if (!aoe_surface_attach_ror(s, d, w, h, 0, shares)) {
        /* The original keeps the half-made object; the port frees it. */
        aoe_surface_free_ror(s);
        free(s);
        return NULL;
    }
    return s;
}

int aoe_draw_make_surfaces_ror(AoeDrawSystemRor *d) {
    const AoeDrawOpsRor *ops = d->ops;
    if (d->type == TYPE_DDRAW) {
        if (!d->primary_surface) {
            if (ops->create_primary(ops->context, d->device,
                                    &d->primary_surface) != 0) {
                d->primary_surface = NULL;
                return 0;
            }
            if (d->screen == SCREEN_FULL) blank_primary(d);
            if (d->bits == 8 &&
                ops->attach_palette(ops->context, d->primary_surface,
                                    d->palette_object) != 0)
                return 0;
            if (ops->attach_clipper(ops->context, d->primary_surface,
                                    d->clipper) != 0)
                return 0;
        }
        if (!d->primary) {
            d->primary = new_surface(d, "Primary Surface", 1);
            if (!d->primary) return 0;
        }
    }
    if (!d->back) {
        d->back = new_surface(d, "Back Buffer", 0);
        if (!d->back) return 0;
    }
    return 1;
}

void aoe_draw_fit_window_ror(AoeDrawSystemRor *d) {
    if (!d->back) return;
    if (d->type == TYPE_GDI || d->screen == SCREEN_WINDOW)
        d->ops->client_size(d->ops->context, d->window, &d->width,
                            &d->height);
    if (d->primary) aoe_surface_allocate_ror(d->primary, d->width, d->height, 0);
    aoe_surface_allocate_ror(d->back, d->width, d->height, 0);
}

void aoe_draw_palette_changed_ror(AoeDrawSystemRor *d, void *window) {
    if (window != d->window) aoe_draw_realize_ror(d);
}

int aoe_draw_realize_ror(AoeDrawSystemRor *d) {
    if (d->hpalette &&
        (d->type == TYPE_GDI || d->screen == SCREEN_WINDOW) &&
        d->ops->realize &&
        d->ops->realize(d->ops->context, d->window, d->hpalette))
        return 1;
    aoe_draw_set_palette_ror(d, 0, 256, d->palette);
    return 0;
}

void aoe_draw_present_ror(AoeDrawSystemRor *d, const int32_t *rect) {
    const AoeDrawOpsRor *ops = d->ops;
    int32_t src[4], dst[4], ox = 0, oy = 0, r, b;
    if (d->type == TYPE_GDI || !d->primary_surface || !d->back ||
        !d->back->surface)
        return;
    if (d->screen == SCREEN_WINDOW) {
        int32_t w, h;
        ops->client_size(ops->context, d->window, &w, &h);
        ops->client_origin(ops->context, d->window, &ox, &oy);
        src[0] = 0;
        src[1] = 0;
        src[2] = w;
        src[3] = h;
    } else {
        src[0] = 0;
        src[1] = 0;
        src[2] = d->width;
        src[3] = d->height;
    }
    if (!rect) {
        dst[0] = ox;
        dst[1] = oy;
        dst[2] = ox + src[2];
        dst[3] = oy + src[3];
        ops->blt(ops->context, d->primary_surface, dst, d->back->surface, src,
                 0);
        return;
    }
    src[0] = rect[0] < 0 ? 0 : rect[0];
    src[1] = rect[1] < 0 ? 0 : rect[1];
    r = rect[2] >= d->width ? d->width - 1 : rect[2];
    b = rect[3] >= d->height ? d->height - 1 : rect[3];
    dst[0] = ox + src[0];
    dst[1] = oy + src[1];
    src[2] = r + 1;
    src[3] = b + 1;
    dst[2] = ox + r + 1;
    dst[3] = oy + b + 1;
    ops->blt(ops->context, d->primary_surface, dst, d->back->surface, src, 0);
}

void aoe_draw_set_palette_handle_ror(AoeDrawSystemRor *d, void *hpalette) {
    uint8_t entries[1024];
    d->hpalette = hpalette;
    if (!hpalette || !d->ops->window_palette ||
        !d->ops->window_palette(d->ops->context, hpalette, entries))
        memset(entries, 0, sizeof entries);
    memset(entries, 0, 4);
    entries[1020] = entries[1021] = entries[1022] = 0xff;
    entries[1023] = 0;
    aoe_draw_set_palette_ror(d, 0, 256, entries);
}

void aoe_draw_set_palette_ror(AoeDrawSystemRor *d, int32_t start,
                              int32_t count, const uint8_t *entries) {
    d->palette_marker = -99;
    if (count == 256) ++d->full_palette_sets;
    if (count > 0 && start >= 0 && start + count <= 256)
        memcpy(d->palette + start * 4, entries, (size_t)count * 4);
    if (d->type != TYPE_GDI && d->palette_object) {
        if (count == 7) ++d->short_palette_sets;
        d->ops->palette_set(d->ops->context, d->palette_object, start, count,
                            entries);
    }
}

/* ---- surfaces ---- */

static const AoeDrawOpsRor *sops(const AoeDrawSurfaceRor *s) {
    return s->system ? s->system->ops : NULL;
}

/* The DirectDraw side (no system, or not type 1). */
static int ddraw_side(const AoeDrawSurfaceRor *s) {
    return !s->system || s->system->type != TYPE_GDI;
}

int aoe_surface_init_ror(AoeDrawSurfaceRor *s, const char *name,
                         int32_t system_memory) {
    size_t n = name ? strlen(name) : 0;
    memset(s, 0, sizeof *s);
    s->key = 0xff;
    s->system_memory = system_memory;
    s->view.direction = 1;
    if (n > 0) {
        s->name = malloc(n + 1);
        if (!s->name) return 0;
        memcpy(s->name, name, n + 1);
    }
    return 1;
}

void aoe_surface_free_ror(AoeDrawSurfaceRor *s) {
    const AoeDrawOpsRor *ops = sops(s);
    AoeDrawNodeRor *n = s->node;
    if (n) {
        if (s->system && s->system->surfaces == n) s->system->surfaces = n->next;
        if (n->prev) n->prev->next = n->next;
        if (n->next) n->next->prev = n->prev;
        free(n);
        s->node = NULL;
    }
    if (ddraw_side(s)) {
        if (s->surface && !s->shares_primary && ops) {
            ops->release(ops->context, s->surface);
            s->surface = NULL;
        }
    } else if (s->dc) {
        if (ops->free_bitmap)
            ops->free_bitmap(ops->context, s->dc, s->bitmap, s->old_bitmap);
    }
    free(s->row_table);
    s->row_table = NULL;
    free(s->wrap_rows);
    s->wrap_rows = NULL;
    if (s->region) {
        aoe_span_region_free_ror(s->region);
        free(s->region);
        s->region = NULL;
    }
    free(s->name);
    s->name = NULL;
}

int aoe_surface_attach_ror(AoeDrawSurfaceRor *s, AoeDrawSystemRor *d,
                           int32_t width, int32_t height, int32_t extra,
                           int32_t shares_primary) {
    s->system = d;
    s->shares_primary = shares_primary;
    if (d) {
        s->window = d->window;
        if (!s->node) {
            AoeDrawNodeRor *n = calloc(1, sizeof *n), *last;
            if (!n) return 0;
            n->surface = s;
            s->node = n;
            if (!d->surfaces) {
                d->surfaces = n;
            } else {
                for (last = d->surfaces; last->next; last = last->next) {}
                last->next = n;
                n->prev = last;
            }
        }
        if (d->type == TYPE_GDI) return 0;
    }
    aoe_surface_allocate_ror(s, width, height, extra);
    return 1;
}

static void whole_region(AoeDrawSurfaceRor *s, int32_t w, int32_t h) {
    int32_t y;
    if (s->region) {
        aoe_span_region_free_ror(s->region);
        free(s->region);
    }
    s->region = malloc(sizeof *s->region);
    if (s->region && !aoe_span_region_init_ror(s->region, w, h)) {
        free(s->region);
        s->region = NULL;
    }
    for (y = 0; s->region && y < h; ++y)
        aoe_span_region_add_ror(s->region, 0, w - 1, y);
    s->active = s->region;
}

void aoe_surface_allocate_ror(AoeDrawSurfaceRor *s, int32_t width,
                              int32_t height, int32_t extra) {
    AoeDrawSystemRor *d = s->system;
    if (d && d->type != TYPE_GDI && d->device && width && height &&
        (width != s->view.width || height != s->view.height || !s->surface)) {
        const AoeDrawOpsRor *ops = d->ops;
        int32_t w = width < 1 ? 1 : width, h = height < 1 ? 1 : height;
        int32_t rows = extra ? h + 10 : h;
        s->extra_rows = extra;
        s->info_width = w;
        s->info_height = s->view.direction == 1 ? -rows : rows;
        s->view.width = w;
        s->view.height = h;
        if (!s->shares_primary) {
            int32_t r;
            if (s->surface) {
                ops->release(ops->context, s->surface);
                s->surface = NULL;
                s->desc_pitch = 0;
                s->desc_bits = NULL;
                s->desc_caps = 0;
                s->view.pitch = 0;
                s->video = 0;
            }
            r = ops->create_surface(ops->context, d->device, w, rows,
                                    (d->flags & 1) || extra ||
                                        s->system_memory,
                                    &s->surface);
            if (r == 0) {
                if (ops->describe(ops->context, s->surface, &s->desc_pitch,
                                  &s->desc_caps) == 0) {
                    aoe_surface_read_desc_ror(s);
                    aoe_surface_build_rows_ror(s);
                    ops->fill(ops->context, s->surface, NULL, 0);
                }
            } else {
                s->surface = NULL;
                aoe_draw_create_message_ror(r);
            }
        } else {
            s->surface = d->primary_surface;
        }
        aoe_surface_clip_ror(s, NULL);
        aoe_surface_fill_ror(s, NULL, 0);
    }
    whole_region(s, width, height);
}

int aoe_surface_restore_ror(AoeDrawSurfaceRor *s) {
    AoeDrawSystemRor *d = s->system;
    const AoeDrawOpsRor *ops;
    int32_t r;
    if (!d || d->type != TYPE_DDRAW) return 0;
    ops = d->ops;
    if (!s->surface) return 3;
    r = ops->is_lost(ops->context, s->surface);
    if (r == 0) return 0;
    aoe_draw_lost_message_ror(r);
    if (!mode_matches(d)) return 1;
    r = ops->restore(ops->context, s->surface);
    if (r == 0) {
        ops->fill(ops->context, s->surface, NULL, 0);
        s->restored = 1;
        return 2;
    }
    if ((uint32_t)r == 0x8876024Bu || (uint32_t)r == 0x8007000Eu) {
        ops->release(ops->context, s->surface);
        s->surface = NULL;
        aoe_surface_allocate_ror(s, s->view.width, s->view.height, 0);
        if (s->surface) return 2;
    }
    aoe_draw_restore_message_ror(r);
    return 3;
}

uint8_t *aoe_surface_lock_ror(AoeDrawSurfaceRor *s, int wait) {
    const AoeDrawOpsRor *ops = sops(s);
    if (ddraw_side(s)) {
        if (!s->surface || !ops) return NULL;
        if (s->view.bits) return s->view.bits;
        if (s->dc) return NULL;
        if (ops->lock(ops->context, s->surface, wait != 0, &s->desc_bits) != 0)
            return NULL;
        s->view.bits = s->desc_bits;
        aoe_surface_read_desc_ror(s);
        if (s->view.bits != s->base) aoe_surface_build_rows_ror(s);
    }
    return s->view.bits;
}

void aoe_surface_unlock_ror(AoeDrawSurfaceRor *s) {
    const AoeDrawOpsRor *ops = sops(s);
    if (ddraw_side(s) && s->surface && s->view.bits && ops) {
        ops->unlock(ops->context, s->surface, s->view.bits);
        s->view.bits = NULL;
    }
}

void *aoe_surface_dc_ror(AoeDrawSurfaceRor *s) {
    const AoeDrawOpsRor *ops = sops(s);
    if (!ddraw_side(s)) return s->dc;
    if (!s->surface || !ops) return NULL;
    if (!s->dc) {
        if (s->view.bits) return NULL;
        ops->get_dc(ops->context, s->surface, &s->dc);
    }
    return s->dc;
}

void aoe_surface_release_dc_ror(AoeDrawSurfaceRor *s) {
    const AoeDrawOpsRor *ops = sops(s);
    if (ddraw_side(s) && s->surface && s->dc && ops) {
        ops->release_dc(ops->context, s->surface, s->dc);
        s->dc = NULL;
    }
}

static int32_t clamp_to(int32_t v, int32_t n) {
    if (v < 0) return 0;
    return v >= n ? n - 1 : v;
}

void aoe_surface_fill_ror(AoeDrawSurfaceRor *s, const int32_t *rect,
                          uint8_t color) {
    if (s->system && !s->system->hardware_clear) {
        aoe_surface_soft_fill_ror(s, rect, color);
        return;
    }
    if (ddraw_side(s)) {
        const AoeDrawOpsRor *ops = sops(s);
        int32_t r[4];
        if (!s->surface || !ops) return;
        if (!rect) {
            ops->fill(ops->context, s->surface, NULL, color);
            return;
        }
        r[0] = rect[0];
        r[1] = rect[1];
        r[2] = rect[2] + 1;
        r[3] = rect[3] + 1;
        ops->fill(ops->context, s->surface, r, color);
        return;
    }
    if (!s->view.bits) return;
    if (!rect) {
        memset(s->view.bits, color, (size_t)s->view.pitch * s->view.height);
        return;
    }
    {
        int32_t l = clamp_to(rect[0], s->view.width);
        int32_t t = clamp_to(rect[1], s->view.height);
        int32_t r = clamp_to(rect[2], s->view.width);
        int32_t b = clamp_to(rect[3], s->view.height);
        int32_t step, y;
        uint8_t *p;
        /* Strictly wider and taller than one pixel, as in the original. */
        if (l >= r || t >= b) return;
        if (s->view.direction == 1) {
            step = s->view.pitch;
            p = s->view.bits + step * t;
        } else {
            p = s->view.bits + (s->view.height - t - 1) * s->view.pitch;
            step = -s->view.pitch;
        }
        p += l;
        for (y = t; y <= b; ++y, p += step) memset(p, color, (size_t)(r - l + 1));
    }
}

void aoe_surface_soft_fill_ror(AoeDrawSurfaceRor *s, const int32_t *rect,
                               uint8_t color) {
    int32_t l = 0, t = 0, r, b, rows, y;
    int locked = s->system && s->system->type == TYPE_DDRAW;
    if (!rect) {
        r = s->view.width;
        b = s->view.height;
    } else {
        l = rect[0];
        t = rect[1];
        r = rect[2];
        b = rect[3];
    }
    if (r < 0 || l > r || l >= s->view.width || b < 0 || t > b ||
        t >= s->view.height)
        return;
    if (l < 0) l = 0;
    if (r >= s->view.width) r = s->view.width - 1;
    if (t < 0) t = 0;
    if (b >= s->view.height) b = s->view.height - 1;
    rows = b - t + 1;
    if (locked && !aoe_surface_lock_ror(s, 1)) return;
    for (y = t; rows > 0; ++y, --rows)
        memset(s->rows[y] + l, color, (size_t)(r - l + 1));
    if (locked) aoe_surface_unlock_ror(s);
}

void aoe_surface_build_rows_ror(AoeDrawSurfaceRor *s) {
    int32_t size = s->view.height * 4, i, step;
    uint8_t *p;
    if (size != s->row_table_size) {
        free(s->row_table);
        s->row_table = malloc((size_t)(size > 0 ? s->view.height : 1) *
                              sizeof *s->row_table);
        s->row_table_size = size;
        s->rows = s->row_table;
    }
    if (s->view.direction == 1 || s->extra_rows) {
        p = s->extra_rows ? s->view.bits + s->view.pitch * 5 : s->view.bits;
        step = s->extra_rows ? s->view.width : s->view.pitch;
    } else {
        p = s->view.bits + (s->view.height - 1) * s->view.pitch;
        step = -s->view.pitch;
    }
    for (i = 0; s->row_table && i < s->view.height; ++i, p += step)
        s->row_table[i] = p;
    if (s->wrap_rows) aoe_surface_wrap_rows_ror(s, s->wrap_x, s->wrap_y);
    s->base = s->view.bits;
}

void aoe_surface_wrap_rows_ror(AoeDrawSurfaceRor *s, int32_t x, int32_t y) {
    int32_t size = s->view.height * 4, i, row = y;
    if (!size || !s->extra_rows || x < 0 || y < 0) return;
    if (size != s->wrap_size) {
        free(s->wrap_rows);
        s->wrap_rows = malloc((size_t)s->view.height * sizeof *s->wrap_rows);
        s->wrap_size = size;
    }
    if (!s->wrap_rows) return;
    for (i = 0; i < s->view.height; ++i) {
        /* Port guard: a start row past the end wraps like the rest. */
        if (row >= s->view.height) row = 0;
        s->wrap_rows[i] = s->row_table[row++] + x;
    }
    s->wrap_x = x;
    s->wrap_y = y;
    s->rows = s->wrap_rows;
}

void aoe_surface_read_desc_ror(AoeDrawSurfaceRor *s) {
    s->view.pitch = s->desc_pitch;
    s->info_width = s->desc_pitch;
    s->video = (s->desc_caps & 0x800u) ? 0 : 1;
}

void aoe_surface_clip_ror(AoeDrawSurfaceRor *s, const int32_t *rect) {
    int32_t l, t, r, b, tmp;
    if (!rect) {
        aoe_surface_set_clip_ror(s, 0, 0, s->view.width - 1,
                                 s->view.height - 1);
        return;
    }
    l = rect[0] < 0 ? 0 : rect[0] > s->view.width - 1 ? s->view.width - 1 : rect[0];
    r = rect[2] < 0 ? 0 : rect[2] > s->view.width - 1 ? s->view.width - 1 : rect[2];
    t = rect[1] < 0 ? 0 : rect[1] > s->view.height - 1 ? s->view.height - 1 : rect[1];
    b = rect[3] < 0 ? 0 : rect[3] > s->view.height - 1 ? s->view.height - 1 : rect[3];
    if (r < l) tmp = r, r = l, l = tmp;
    if (b < t) tmp = b, b = t, t = tmp;
    aoe_surface_set_clip_ror(s, l, t, r, b);
}

void aoe_surface_set_clip_ror(AoeDrawSurfaceRor *s, int32_t left,
                              int32_t top, int32_t right, int32_t bottom) {
    int32_t y;
    s->view.clip_left = left;
    s->view.clip_top = top;
    s->view.clip_right = right;
    s->view.clip_bottom = bottom;
    if (s->region && s->view.height != s->region->rows) {
        aoe_span_region_free_ror(s->region);
        free(s->region);
        s->region = NULL;
    }
    if (!s->region) {
        s->region = malloc(sizeof *s->region);
        if (s->region &&
            !aoe_span_region_init_ror(s->region, s->view.width,
                                      s->view.height)) {
            free(s->region);
            s->region = NULL;
        }
    }
    if (s->region) {
        aoe_span_region_clear_ror(s->region);
        aoe_span_region_set_bounds_ror(s->region, left, top, right, bottom);
        for (y = top; y <= bottom; ++y)
            aoe_span_region_add_ror(s->region, left, right, y);
    }
    s->active = s->region;
}

void aoe_surface_blit_ror(AoeDrawSurfaceRor *s, AoeDrawSurfaceRor *dst,
                          int32_t x, int32_t y, const int32_t *rect,
                          int use_key) {
    const AoeDrawOpsRor *ops = sops(s);
    if (ddraw_side(s)) {
        int32_t src[4], to[4], cut;
        if (!s->surface || !dst || !dst->surface || !ops) return;
        if (!rect) {
            src[0] = 0;
            src[1] = 0;
            src[2] = s->view.width - 1;
            src[3] = s->view.height - 1;
        } else {
            memcpy(src, rect, sizeof src);
        }
        src[2] += 1;
        src[3] += 1;
        to[0] = x;
        to[1] = y;
        to[2] = src[2] - src[0] + x;
        to[3] = src[3] - src[1] + y;
        if (src[0] < 0) {
            to[0] -= src[0];
            src[0] = 0;
        }
        if (src[1] < 0) {
            to[1] -= src[1];
            src[1] = 0;
        }
        if (src[2] > s->view.width) {
            cut = src[2] - s->view.width;
            src[2] -= cut;
            to[2] -= cut;
        }
        if (src[3] > s->view.height) {
            cut = src[3] - s->view.height;
            src[3] -= cut;
            to[3] -= cut;
        }
        if (to[0] < dst->view.clip_left) {
            cut = dst->view.clip_left - to[0];
            to[0] += cut;
            src[0] += cut;
        }
        if (to[1] < dst->view.clip_top) {
            cut = dst->view.clip_top - to[1];
            to[1] += cut;
            src[1] += cut;
        }
        if (dst->view.clip_right + 1 < to[2]) {
            cut = to[2] - dst->view.clip_right - 1;
            to[2] -= cut;
            src[2] -= cut;
        }
        if (dst->view.clip_bottom + 1 < to[3]) {
            cut = to[3] - dst->view.clip_bottom - 1;
            to[3] -= cut;
            src[3] -= cut;
        }
        if (to[0] < to[2] && to[1] < to[3] && src[0] < src[2] &&
            src[1] < src[3]) {
            int32_t r = ops->blt(ops->context, dst->surface, to, s->surface,
                                 src, s->keyed && use_key);
            s->system->last_blt = r;
            s->system->last_blt_message =
                r ? aoe_draw_blt_message_ror(r) : 0;
        }
        return;
    }
    if (!s->view.bits || !dst || !dst->view.bits) return;
    {
        int32_t sx = 0, sy = 0, w, h;
        if (!rect) {
            w = s->view.width;
            h = s->view.height;
        } else {
            sx = rect[0];
            sy = rect[1];
            w = rect[2] - sx + 1;
            h = rect[3] - sy + 1;
        }
        if (w + sx > s->view.width) w = s->view.width - sx;
        if (h + sy > s->view.height) h = s->view.height - sy;
        if (ops->soft_blit)
            ops->soft_blit(ops->context, dst, x, y, s, sx, sy, w, h,
                           s->keyed && use_key, s->key);
        s->system->last_blt_message = 0;
    }
}

void aoe_surface_span_copy_ror(AoeDrawSurfaceRor *s, AoeDrawSurfaceRor *dst,
                               int32_t dx, int32_t dy, const int32_t rect[4],
                               AoeSpanRor *const *spans) {
    int32_t rows, row, to;
    if (rect[0] < 0 || rect[0] > rect[2] || rect[2] >= s->view.width ||
        rect[1] < 0 || rect[1] > rect[3] || rect[3] >= s->view.height ||
        dx < 0 || dx >= dst->view.width || dy < 0 || dy >= dst->view.height)
        return;
    rows = rect[3] - rect[1] + 1;
    if (rows + dy > dst->view.height) rows = dst->view.height - dy;
    /* Locked and never unlocked, as in the original (a later lock gets
       the same bits back). */
    if (s->system && s->system->type == TYPE_DDRAW &&
        !aoe_surface_lock_ror(s, 1))
        return;
    for (row = rect[1], to = dy; rows > 0; ++row, ++to, --rows) {
        const AoeSpanRor *sp;
        for (sp = spans[row]; sp; sp = sp->next)
            memcpy(dst->rows[to] + dx + sp->x0, s->rows[row] + rect[0] + sp->x0,
                   (size_t)(sp->x1 - sp->x0 + 1));
    }
}

void aoe_surface_scroll_ror(AoeDrawSurfaceRor *s, const int32_t from[4],
                            const int32_t to[4], int32_t dx, int32_t dy,
                            int32_t ox, int32_t oy) {
    int32_t rows, src_x, dst_x, words, step = 1, src_row = 0, dst_row = 0,
                                            w;
    int backwards = 0, locked;
    if (dx == 0 && dy == 0) return;
    locked = s->system && s->system->type == TYPE_DDRAW;
    if (locked && !aoe_surface_lock_ror(s, 1)) return;
    rows = from[3] - from[1] + 1;
    src_x = from[0] + ox;
    words = (from[2] - from[0] + 4) >> 2;
    dst_x = to[0] + ox;
    if (dy < 0) {
        step = -1;
        dst_row = to[3] + oy;
        src_row = from[3] + oy;
    }
    if (dy > 0) {
        src_row = from[1] + oy;
        step = 1;
        dst_row = to[1] + oy;
    }
    if (dy == 0) {
        src_row = from[1] + oy;
        dst_row = to[1] + oy;
        step = 1;
        if (dx < 0) {
            backwards = 1;
            src_x = from[2] - 3 + ox;
            dst_x = to[2] - 3 + ox;
        }
    }
    for (; rows > 0; --rows, src_row += step, dst_row += step) {
        uint8_t *a = s->rows[src_row] + src_x, *b = s->rows[dst_row] + dst_x;
        for (w = 0; w < words; ++w) {
            memmove(b, a, 4);
            a += backwards ? -4 : 4;
            b += backwards ? -4 : 4;
        }
    }
    if (locked) aoe_surface_unlock_ror(s);
}

void aoe_surface_set_key_ror(AoeDrawSurfaceRor *s, int32_t keyed,
                             uint8_t color) {
    s->keyed = keyed;
    if (!keyed) {
        s->key = 0xff;
        return;
    }
    s->key = color;
    if (s->surface && s->system && s->system->type == TYPE_DDRAW)
        s->system->ops->set_key(s->system->ops->context, s->surface, color);
}

void aoe_surface_plot_ror(AoeDrawSurfaceRor *s, int32_t x, int32_t y,
                          uint8_t color) {
    if (!s->view.bits || x < s->view.clip_left || x > s->view.clip_right ||
        y < s->view.clip_top || y > s->view.clip_bottom)
        return;
    if (s->view.direction == 1)
        s->view.bits[s->view.pitch * y + x] = color;
    else
        s->view.bits[(s->view.height - y - 1) * s->view.pitch + x] = color;
}

void aoe_draw_line_ror(const AoeDrawOpsRor *ops, int32_t x0, int32_t y0,
                       int32_t x1, int32_t y1, uint8_t color) {
    int32_t dx, dy, step, acc = 0, y, start, x, t;
    if (x0 == x1) {
        if (y1 < y0) t = y0, y0 = y1, y1 = t;
        for (y = y0; y <= y1; ++y) ops->span_fill(ops->context, x0, x1, y, color);
        return;
    }
    if (y0 == y1) {
        ops->span_fill(ops->context, x0, x1, y0, color);
        return;
    }
    if (x1 < x0) {
        t = x0, x0 = x1, x1 = t;
        t = y0, y0 = y1, y1 = t;
    }
    dx = x1 - x0 + 1;
    if (y0 < y1) {
        step = 1;
        dy = y1 - y0 + 1;
    } else {
        step = -1;
        dy = y0 - y1 + 1;
    }
    y = y0;
    if (dy <= dx) {
        start = x0;
        for (x = x0; x < x1; ++x) {
            acc += dy;
            if (acc >= dx) {
                ops->span_fill(ops->context, start, x, y, color);
                y += step;
                acc -= dx;
                start = x + 1;
            }
        }
        ops->span_fill(ops->context, start, x, y, color);
        return;
    }
    x = x0;
    for (;;) {
        ops->span_fill(ops->context, x, x, y, color);
        if (y == y1) break;
        y += step;
        acc += dx;
        if (acc >= dy) {
            ++x;
            acc -= dy;
        }
    }
}
