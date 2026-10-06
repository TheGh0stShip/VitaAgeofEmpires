/* A software DirectDraw stand-in for the translated draw system. */
#include "soft_draw.h"

#include <stdlib.h>
#include <string.h>

#define DISPLAY(c) ((AoeSoftDisplayRor *)(c))

/* DirectDraw-style results: 0 success, a nonzero error otherwise. */
enum { SD_OK = 0, SD_ERROR = 1 };

typedef struct {
    int kind;                  /* 1 device, 2 palette, 3 clipper, 4 surface */
} SdObject;

typedef struct {
    SdObject head;
    uint8_t entries[1024];
} SdPalette;

typedef struct {
    SdObject head;
    AoeSoftSurfaceRor surface;
} SdSurface;

/* A surface handle is its SdSurface; NULL stays NULL. */
#define SURF(h) ((h) ? &((SdSurface *)(h))->surface : (AoeSoftSurfaceRor *)NULL)

static SdObject device_object = {1};
static SdObject clipper_object = {3};

static int32_t sd_create(void *c, void **device) {
    (void)c;
    *device = &device_object;
    return SD_OK;
}

static void sd_release(void *c, void *object) {
    SdObject *o = object;
    if (!o) return;
    if (o->kind == 4) {
        SdSurface *s = (SdSurface *)o;
        if (s->surface.primary && DISPLAY(c)->primary == &s->surface) DISPLAY(c)->primary = NULL;
        free(s->surface.bits);
        free(s);
    } else if (o->kind == 2) {
        free(o);
    }
}

static int32_t sd_set_level(void *c, void *device, void *window, uint32_t flags) {
    (void)c; (void)device; (void)window; (void)flags;
    return SD_OK;
}

static int32_t sd_get_mode(void *c, void *device, int32_t *w, int32_t *h, int32_t *bits) {
    (void)device;
    *w = DISPLAY(c)->width;
    *h = DISPLAY(c)->height;
    *bits = DISPLAY(c)->bits;
    return SD_OK;
}

static int32_t sd_set_mode(void *c, void *device, int32_t w, int32_t h, int32_t bits) {
    (void)device;
    if (w != DISPLAY(c)->width || h != DISPLAY(c)->height || bits != 8) return SD_ERROR;
    return SD_OK;
}

static void sd_restore_mode(void *c, void *device) { (void)c; (void)device; }

static void sd_enum_modes(void *c, void *device, void (*each)(void *, int32_t, int32_t, int32_t),
                          void *user) {
    (void)device;
    each(user, DISPLAY(c)->width, DISPLAY(c)->height, 8);
}

static int32_t sd_get_caps(void *c, void *device, uint32_t *caps, uint32_t *key_caps) {
    (void)c; (void)device;
    *caps = 0x400000u;      /* colour-key blits */
    *key_caps = 0x200u;     /* source key */
    return SD_OK;
}

static int32_t sd_desktop_width(void *c) { return DISPLAY(c)->width; }

static int32_t sd_create_palette(void *c, void *device, const uint8_t entries[1024],
                                 void **palette) {
    SdPalette *p = calloc(1, sizeof *p);
    (void)c; (void)device;
    if (!p) return SD_ERROR;
    p->head.kind = 2;
    memcpy(p->entries, entries, sizeof p->entries);
    *palette = p;
    return SD_OK;
}

static int32_t sd_palette_get(void *c, void *palette, int32_t start, int32_t count, uint8_t *out) {
    SdPalette *p = palette;
    (void)c;
    if (!p || start < 0 || count < 0 || start + count > 256) return SD_ERROR;
    memcpy(out, p->entries + 4 * start, 4 * (size_t)count);
    return SD_OK;
}

static int32_t sd_palette_set(void *c, void *palette, int32_t start, int32_t count,
                              const uint8_t *in) {
    SdPalette *p = palette;
    if (!p || start < 0 || count < 0 || start + count > 256) return SD_ERROR;
    memcpy(p->entries + 4 * start, in, 4 * (size_t)count);
    if (DISPLAY(c)->primary && DISPLAY(c)->primary->palette == p->entries) {
        memcpy(DISPLAY(c)->palette, p->entries, sizeof DISPLAY(c)->palette);
        DISPLAY(c)->palette_changes++;
    }
    return SD_OK;
}

static int32_t sd_create_clipper(void *c, void *device, void **clipper) {
    (void)c; (void)device;
    *clipper = &clipper_object;
    return SD_OK;
}

static int32_t sd_clipper_window(void *c, void *clipper, void *window) {
    (void)c; (void)clipper; (void)window;
    return SD_OK;
}

static SdSurface *new_surface(int32_t w, int32_t h) {
    SdSurface *s;
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096) return NULL;
    s = calloc(1, sizeof *s);
    if (!s) return NULL;
    s->head.kind = 4;
    s->surface.width = w;
    s->surface.height = h;
    s->surface.pitch = (w + 3) & ~3;
    s->surface.bits = calloc((size_t)s->surface.pitch, (size_t)h);
    if (!s->surface.bits) {
        free(s);
        return NULL;
    }
    return s;
}

static int32_t sd_create_primary(void *c, void *device, void **surface) {
    SdSurface *s = new_surface(DISPLAY(c)->width, DISPLAY(c)->height);
    (void)device;
    if (!s) return SD_ERROR;
    s->surface.primary = 1;
    DISPLAY(c)->primary = &s->surface;
    *surface = s;
    return SD_OK;
}

static int32_t sd_attach_clipper(void *c, void *surface, void *clipper) {
    (void)c; (void)surface; (void)clipper;
    return SD_OK;
}

static int32_t sd_attach_palette(void *c, void *surface, void *palette) {
    SdPalette *p = palette;
    if (!surface || !p) return SD_ERROR;
    SURF(surface)->palette = p->entries;
    if (SURF(surface)->primary) {
        memcpy(DISPLAY(c)->palette, p->entries, sizeof DISPLAY(c)->palette);
        DISPLAY(c)->palette_changes++;
    }
    return SD_OK;
}

static int32_t sd_create_surface(void *c, void *device, int32_t w, int32_t h, int system,
                                 void **surface) {
    SdSurface *s = new_surface(w, h);
    (void)c; (void)device; (void)system;
    if (!s) return SD_ERROR;
    *surface = s;
    return SD_OK;
}

static int32_t sd_describe(void *c, void *surface, int32_t *pitch, uint32_t *caps) {
    (void)c;
    if (!surface) return SD_ERROR;
    *pitch = SURF(surface)->pitch;
    *caps = 0x800u;    /* system memory */
    return SD_OK;
}

/* Clip [l, r) x [t, b) to the surface; 0 when empty. */
static int clip_rect(const AoeSoftSurfaceRor *s, const int32_t *rect, int32_t r[4]) {
    if (rect) {
        r[0] = rect[0]; r[1] = rect[1]; r[2] = rect[2]; r[3] = rect[3];
    } else {
        r[0] = 0; r[1] = 0; r[2] = s->width; r[3] = s->height;
    }
    if (r[0] < 0) r[0] = 0;
    if (r[1] < 0) r[1] = 0;
    if (r[2] > s->width) r[2] = s->width;
    if (r[3] > s->height) r[3] = s->height;
    return r[0] < r[2] && r[1] < r[3];
}

static int32_t sd_fill(void *c, void *surface, const int32_t *rect, uint32_t color) {
    AoeSoftSurfaceRor *s = SURF(surface);
    int32_t r[4], y;
    (void)c;
    if (!s) return SD_ERROR;
    if (!clip_rect(s, rect, r)) return SD_OK;
    for (y = r[1]; y < r[3]; ++y)
        memset(s->bits + (size_t)y * (size_t)s->pitch + r[0], (int)(color & 0xff), (size_t)(r[2] - r[0]));
    return SD_OK;
}

/* An unscaled copy of src_rect to dst_rect's origin (DirectDraw would
   stretch; the draw system only copies like-sized rectangles). */
static int32_t sd_blt(void *c, void *dst, const int32_t dst_rect[4], void *src,
                      const int32_t src_rect[4], int keyed) {
    AoeSoftSurfaceRor *d = SURF(dst), *s = SURF(src);
    int32_t sr[4], dx, dy, w, h, y, x;
    (void)c;
    if (!d || !s) return SD_ERROR;
    if (!clip_rect(s, src_rect, sr)) return SD_OK;
    dx = dst_rect ? dst_rect[0] : 0;
    dy = dst_rect ? dst_rect[1] : 0;
    w = sr[2] - sr[0];
    h = sr[3] - sr[1];
    if (dx < 0) { sr[0] -= dx; w += dx; dx = 0; }
    if (dy < 0) { sr[1] -= dy; h += dy; dy = 0; }
    if (dx + w > d->width) w = d->width - dx;
    if (dy + h > d->height) h = d->height - dy;
    if (w <= 0 || h <= 0) return SD_OK;
    for (y = 0; y < h; ++y) {
        const uint8_t *from = s->bits + (size_t)(sr[1] + y) * (size_t)s->pitch + sr[0];
        uint8_t *to = d->bits + (size_t)(dy + y) * (size_t)d->pitch + dx;
        if (!keyed || !s->keyed) {
            memmove(to, from, (size_t)w);
        } else {
            for (x = 0; x < w; ++x)
                if (from[x] != s->key) to[x] = from[x];
        }
    }
    return SD_OK;
}

static int32_t sd_is_lost(void *c, void *surface) { (void)c; (void)surface; return SD_OK; }
static int32_t sd_restore(void *c, void *surface) { (void)c; (void)surface; return SD_OK; }

static int32_t sd_lock(void *c, void *surface, int wait, uint8_t **bits) {
    (void)c; (void)wait;
    if (!surface) return SD_ERROR;
    *bits = SURF(surface)->bits;
    return SD_OK;
}

static void sd_unlock(void *c, void *surface, uint8_t *bits) { (void)c; (void)surface; (void)bits; }

static int32_t sd_get_dc(void *c, void *surface, void **dc) {
    (void)c; (void)surface;
    *dc = NULL;
    return SD_ERROR;
}

static void sd_release_dc(void *c, void *surface, void *dc) { (void)c; (void)surface; (void)dc; }

static int32_t sd_set_key(void *c, void *surface, uint8_t color) {
    (void)c;
    if (!surface) return SD_ERROR;
    SURF(surface)->keyed = 1;
    SURF(surface)->key = color;
    return SD_OK;
}

static void sd_client_size(void *c, void *window, int32_t *w, int32_t *h) {
    (void)window;
    *w = DISPLAY(c)->width;
    *h = DISPLAY(c)->height;
}

static void sd_client_origin(void *c, void *window, int32_t *x, int32_t *y) {
    (void)c; (void)window;
    *x = 0;
    *y = 0;
}

void aoe_soft_draw_init_ror(AoeSoftDisplayRor *display, AoeDrawOpsRor *ops, int32_t width,
                            int32_t height) {
    memset(display, 0, sizeof *display);
    display->width = width;
    display->height = height;
    display->bits = 8;
    memset(ops, 0, sizeof *ops);
    ops->context = display;
    ops->create = sd_create;
    ops->release = sd_release;
    ops->set_level = sd_set_level;
    ops->get_mode = sd_get_mode;
    ops->set_mode = sd_set_mode;
    ops->restore_mode = sd_restore_mode;
    ops->enum_modes = sd_enum_modes;
    ops->get_caps = sd_get_caps;
    ops->desktop_width = sd_desktop_width;
    ops->create_palette = sd_create_palette;
    ops->palette_get = sd_palette_get;
    ops->palette_set = sd_palette_set;
    ops->create_clipper = sd_create_clipper;
    ops->clipper_window = sd_clipper_window;
    ops->create_primary = sd_create_primary;
    ops->attach_clipper = sd_attach_clipper;
    ops->attach_palette = sd_attach_palette;
    ops->create_surface = sd_create_surface;
    ops->describe = sd_describe;
    ops->fill = sd_fill;
    ops->blt = sd_blt;
    ops->is_lost = sd_is_lost;
    ops->restore = sd_restore;
    ops->lock = sd_lock;
    ops->unlock = sd_unlock;
    ops->get_dc = sd_get_dc;
    ops->release_dc = sd_release_dc;
    ops->set_key = sd_set_key;
    ops->client_size = sd_client_size;
    ops->client_origin = sd_client_origin;
}

const uint8_t *aoe_soft_draw_primary_ror(const AoeSoftDisplayRor *display, int32_t *pitch) {
    if (!display->primary) return NULL;
    *pitch = display->primary->pitch;
    return display->primary->bits;
}

int aoe_soft_draw_present_abgr_ror(const AoeSoftDisplayRor *display, uint32_t *out,
                                   int32_t stride) {
    const AoeSoftSurfaceRor *p = display->primary;
    uint32_t lut[256];
    int32_t x, y, i;
    if (!p) return 0;
    for (i = 0; i < 256; ++i) {
        const uint8_t *e = display->palette + 4 * i;   /* PALETTEENTRY: R, G, B, flags */
        lut[i] = 0xff000000u | ((uint32_t)e[2] << 16) | ((uint32_t)e[1] << 8) | e[0];
    }
    for (y = 0; y < p->height; ++y) {
        const uint8_t *row = p->bits + (size_t)y * (size_t)p->pitch;
        uint32_t *o = out + (size_t)y * (size_t)stride;
        for (x = 0; x < p->width; ++x) o[x] = lut[row[x]];
    }
    return 1;
}
