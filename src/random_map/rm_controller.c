#include "random_map.h"

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"
#include "aoe_stream.h"

/* ---- Record tables ---- */

static void get_bytes(AoeOutcomeStream *in, void *out, size_t n) {
    const uint8_t *p = aoe_stream_take(in, n);
    if (p) memcpy(out, p, n);
    else memset(out, 0, n);
}

static void read_land(AoeOutcomeStream *in, AoeRmLandRecRor *l) {
    uint8_t b[0x2c];
    get_bytes(in, b, sizeof b);
    l->id = aoe_le_i32(b);
    l->terrain = b[4];
    l->f08 = aoe_le_i32(b + 8);
    l->radius = aoe_le_i32(b + 0xc);
    l->player = b[0x10];
    l->placement = b[0x11];
    l->fixed_x = aoe_le_i32(b + 0x14);
    l->fixed_y = aoe_le_i32(b + 0x18);
    l->area_percent = b[0x1c];
    l->per_player = b[0x1d];
    l->zone_radius = aoe_le_i32(b + 0x20);
    l->zone_weight = aoe_le_i32(b + 0x24);
    l->f28 = aoe_le_i32(b + 0x28);
}

static void write_land(AoeOutcomeStream *out, const AoeRmLandRecRor *l) {
    uint8_t b[0x2c] = {0};
    aoe_le_put_u32(b, (uint32_t)l->id);
    b[4] = l->terrain;
    aoe_le_put_u32(b + 8, (uint32_t)l->f08);
    aoe_le_put_u32(b + 0xc, (uint32_t)l->radius);
    b[0x10] = l->player;
    b[0x11] = l->placement;
    aoe_le_put_u32(b + 0x14, (uint32_t)l->fixed_x);
    aoe_le_put_u32(b + 0x18, (uint32_t)l->fixed_y);
    b[0x1c] = l->area_percent;
    b[0x1d] = l->per_player;
    aoe_le_put_u32(b + 0x20, (uint32_t)l->zone_radius);
    aoe_le_put_u32(b + 0x24, (uint32_t)l->zone_weight);
    aoe_le_put_u32(b + 0x28, (uint32_t)l->f28);
    aoe_stream_put(out, b, sizeof b);
}

static void read_object(AoeOutcomeStream *in, AoeRmObjectRecRor *o) {
    uint8_t b[0x2c];
    get_bytes(in, b, sizeof b);
    o->unit = aoe_le_i32(b);
    o->f04 = aoe_le_i32(b + 4);
    o->f08 = b[8];
    o->f09 = b[9];
    o->f0c = aoe_le_i32(b + 0xc);
    o->f10 = aoe_le_i32(b + 0x10);
    o->f14 = aoe_le_i32(b + 0x14);
    o->f18 = aoe_le_i32(b + 0x18);
    o->f1c = aoe_le_i32(b + 0x1c);
    o->land = aoe_le_i32(b + 0x20);
    o->f24 = aoe_le_i32(b + 0x24);
    o->f28 = aoe_le_i32(b + 0x28);
}

static void write_object(AoeOutcomeStream *out, const AoeRmObjectRecRor *o) {
    uint8_t b[0x2c] = {0};
    aoe_le_put_u32(b, (uint32_t)o->unit);
    aoe_le_put_u32(b + 4, (uint32_t)o->f04);
    b[8] = o->f08;
    b[9] = o->f09;
    aoe_le_put_u32(b + 0xc, (uint32_t)o->f0c);
    aoe_le_put_u32(b + 0x10, (uint32_t)o->f10);
    aoe_le_put_u32(b + 0x14, (uint32_t)o->f14);
    aoe_le_put_u32(b + 0x18, (uint32_t)o->f18);
    aoe_le_put_u32(b + 0x1c, (uint32_t)o->f1c);
    aoe_le_put_u32(b + 0x20, (uint32_t)o->land);
    aoe_le_put_u32(b + 0x24, (uint32_t)o->f24);
    aoe_le_put_u32(b + 0x28, (uint32_t)o->f28);
    aoe_stream_put(out, b, sizeof b);
}

static void read_ints(AoeOutcomeStream *in, int32_t *v, int n) {
    int i;
    for (i = 0; i < n; ++i) v[i] = aoe_stream_i32(in);
}

static void write_ints(AoeOutcomeStream *out, const int32_t *v, int n) {
    int i;
    for (i = 0; i < n; ++i) aoe_stream_put_i32(out, v[i]);
}

/* Header +0x04..+0x2f. */
static void read_header_tail(AoeOutcomeStream *in, AoeRmMapRecRor *m) {
    read_ints(in, m->margin, 4);
    m->f14 = aoe_stream_i32(in);
    m->edge = aoe_stream_i32(in);
    m->f1c = aoe_stream_i32(in);
    m->land_percent = aoe_stream_i32(in);
    m->f24 = aoe_stream_i32(in);
    m->land_count = aoe_stream_i32(in);
    m->land_ptr = (uint32_t)aoe_stream_i32(in);
}

static void write_header_tail(AoeOutcomeStream *out, const AoeRmMapRecRor *m) {
    write_ints(out, m->margin, 4);
    aoe_stream_put_i32(out, m->f14);
    aoe_stream_put_i32(out, m->edge);
    aoe_stream_put_i32(out, m->f1c);
    aoe_stream_put_i32(out, m->land_percent);
    aoe_stream_put_i32(out, m->f24);
    aoe_stream_put_i32(out, m->land_count);
    aoe_stream_put_i32(out, (int32_t)m->land_ptr);
}

static void controller_init(AoeRmControllerRor *c, int ror, AoeRng *rng,
                            const AoeRmGameOpsRor *game) {
    memset(c, 0, sizeof(*c));
    aoe_rm_module_init_ror(&c->base, NULL, NULL, NULL, 0, rng);
    c->map_index = -1;
    c->ror = ror;
    c->game = game;
}

static void free_maps(AoeRmControllerRor *c) {
    int32_t i;
    if (c->maps) {
        for (i = 0; i < c->map_count; ++i) {
            free(c->maps[i].lands);
            free(c->maps[i].terrains);
            free(c->maps[i].objects);
            free(c->maps[i].elevations);
        }
        free(c->maps);
    }
    c->maps = NULL;
    c->map_count = 0;
}

int aoe_rm_controller_load_ror(AoeRmControllerRor *c, AoeOutcomeStream *in,
                               int ror, AoeRng *rng,
                               const AoeRmGameOpsRor *game) {
    int32_t i;
    if (!c || !in) return 0;
    controller_init(c, ror, rng, game);
    c->map_count = aoe_stream_i32(in);
    c->maps_ptr = (uint32_t)aoe_stream_i32(in);
    if (c->map_count <= 0) return !in->failed;
    if (c->map_count > 0x10000) return 0;
    c->maps = calloc((size_t)c->map_count, sizeof(*c->maps));
    if (!c->maps) return 0;
    for (i = 0; i < c->map_count; ++i) {
        AoeRmMapRecRor *m = &c->maps[i];
        m->id = aoe_stream_i32(in);
        read_header_tail(in, m);
        m->terrain_count = aoe_stream_i32(in);
        m->terrain_ptr = (uint32_t)aoe_stream_i32(in);
        m->object_count = aoe_stream_i32(in);
        m->object_ptr = (uint32_t)aoe_stream_i32(in);
        m->elevation_count = aoe_stream_i32(in);
        m->elevation_ptr = (uint32_t)aoe_stream_i32(in);
    }
    for (i = 0; i < c->map_count && !in->failed; ++i) {
        AoeRmMapRecRor *m = &c->maps[i];
        int32_t k;
        read_header_tail(in, m);
        if (m->land_count > 0) {
            m->lands = calloc((size_t)m->land_count, sizeof(*m->lands));
            if (!m->lands || m->land_count > 0x10000) return 0;
            for (k = 0; k < m->land_count; ++k) read_land(in, &m->lands[k]);
        }
        m->terrain_count = aoe_stream_i32(in);
        m->terrain_ptr = (uint32_t)aoe_stream_i32(in);
        if (m->terrain_count > 0) {
            m->terrains = calloc((size_t)m->terrain_count, sizeof(*m->terrains));
            if (!m->terrains || m->terrain_count > 0x10000) return 0;
            for (k = 0; k < m->terrain_count; ++k)
                read_ints(in, &m->terrains[k].percent, 6);
        }
        m->object_count = aoe_stream_i32(in);
        m->object_ptr = (uint32_t)aoe_stream_i32(in);
        if (m->object_count > 0) {
            m->objects = calloc((size_t)m->object_count, sizeof(*m->objects));
            if (!m->objects || m->object_count > 0x10000) return 0;
            for (k = 0; k < m->object_count; ++k) read_object(in, &m->objects[k]);
        }
        m->elevation_count = aoe_stream_i32(in);
        m->elevation_ptr = (uint32_t)aoe_stream_i32(in);
        if (m->elevation_count > 0) {
            m->elevations = calloc((size_t)m->elevation_count, sizeof(*m->elevations));
            if (!m->elevations || m->elevation_count > 0x10000) return 0;
            for (k = 0; k < m->elevation_count; ++k)
                read_ints(in, m->elevations[k].v, 6);
        }
    }
    return !in->failed;
}

int aoe_rm_controller_save_ror(const AoeRmControllerRor *c,
                               AoeOutcomeStream *out) {
    int32_t i, k;
    if (!c || !out) return 0;
    aoe_stream_put_i32(out, c->map_count);
    aoe_stream_put_i32(out, (int32_t)c->maps_ptr);
    if (c->map_count <= 0) return !out->failed;
    for (i = 0; i < c->map_count; ++i) {
        const AoeRmMapRecRor *m = &c->maps[i];
        aoe_stream_put_i32(out, m->id);
        write_header_tail(out, m);
        aoe_stream_put_i32(out, m->terrain_count);
        aoe_stream_put_i32(out, (int32_t)m->terrain_ptr);
        aoe_stream_put_i32(out, m->object_count);
        aoe_stream_put_i32(out, (int32_t)m->object_ptr);
        aoe_stream_put_i32(out, m->elevation_count);
        aoe_stream_put_i32(out, (int32_t)m->elevation_ptr);
    }
    for (i = 0; i < c->map_count; ++i) {
        const AoeRmMapRecRor *m = &c->maps[i];
        write_header_tail(out, m);
        for (k = 0; k < m->land_count; ++k) write_land(out, &m->lands[k]);
        aoe_stream_put_i32(out, m->terrain_count);
        aoe_stream_put_i32(out, (int32_t)m->terrain_ptr);
        for (k = 0; k < m->terrain_count; ++k) write_ints(out, &m->terrains[k].percent, 6);
        aoe_stream_put_i32(out, m->object_count);
        aoe_stream_put_i32(out, (int32_t)m->object_ptr);
        for (k = 0; k < m->object_count; ++k) write_object(out, &m->objects[k]);
        aoe_stream_put_i32(out, m->elevation_count);
        aoe_stream_put_i32(out, (int32_t)m->elevation_ptr);
        for (k = 0; k < m->elevation_count; ++k) write_ints(out, m->elevations[k].v, 6);
    }
    return !out->failed;
}

void aoe_rm_controller_release_ror(AoeRmControllerRor *c) {
    if (!c) return;
    free_maps(c);
    aoe_rm_module_release_ror(&c->base);
}

/* fscanf("%d") over a text buffer. */
static int scan_int(const char **p, int32_t *out) {
    const char *s = *p;
    long v = 0;
    int neg = 0, any = 0;
    if (!s) return 0;
    while (*s && isspace((unsigned char)*s)) ++s;
    if (*s == '-' || *s == '+') neg = *s++ == '-';
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s++ - '0');
        if (v > 0x7fffffffL) v = 0x7fffffffL;
        any = 1;
    }
    if (!any) return 0;
    *out = (int32_t)(neg ? -v : v);
    *p = s;
    return 1;
}

static int32_t next_int(const char **p) {
    int32_t v = 0;
    scan_int(p, &v);
    return v;
}

static AoeRmMapRecRor *map_at(AoeRmControllerRor *c, int32_t index) {
    return index >= 0 && index < c->map_count ? &c->maps[index] : NULL;
}

int aoe_rm_controller_load_text_ror(AoeRmControllerRor *c,
                                    const char *lands, const char *terrains,
                                    const char *objects, const char *header,
                                    int ror, AoeRng *rng,
                                    const AoeRmGameOpsRor *game) {
    const char *p;
    int32_t n, i, k;
    if (!c) return 0;
    controller_init(c, ror, rng, game);
    if (header) {
        p = header;
        c->map_count = next_int(&p);
        if (c->map_count > 0 && c->map_count <= 0x10000) {
            c->maps = calloc((size_t)c->map_count, sizeof(*c->maps));
            if (!c->maps) return 0;
            for (i = 0; i < c->map_count; ++i) c->maps[i].id = next_int(&p);
        }
    }
    if (c->map_count < 1 || !c->maps) {
        c->map_count = 0;
        free(c->maps);
        c->maps = NULL;
        return 1;
    }
    if ((p = lands) != NULL) {
        n = next_int(&p);
        for (i = 0; i < n; ++i) {
            AoeRmMapRecRor *m = map_at(c, next_int(&p));
            AoeRmMapRecRor scratch;
            if (!m) m = &scratch; /* the original writes outside the table */
            memset(&scratch, 0, sizeof scratch);
            m->land_count = next_int(&p);
            for (k = 0; k < 4; ++k) m->margin[k] = next_int(&p);
            m->f14 = next_int(&p);
            m->edge = next_int(&p);
            m->f1c = next_int(&p);
            m->land_percent = next_int(&p);
            free(m->lands);
            m->lands = NULL;
            if (m->land_count > 0 && m->land_count <= 0x10000) {
                m->lands = calloc((size_t)m->land_count, sizeof(*m->lands));
                if (!m->lands) return 0;
                for (k = 0; k < m->land_count; ++k) {
                    AoeRmLandRecRor *l = &m->lands[k];
                    l->id = next_int(&p);
                    l->terrain = (uint8_t)next_int(&p);
                    l->f08 = next_int(&p);
                    l->radius = next_int(&p);
                    l->player = (uint8_t)next_int(&p);
                    l->placement = (uint8_t)next_int(&p);
                    l->fixed_x = next_int(&p);
                    l->fixed_y = next_int(&p);
                    l->area_percent = (uint8_t)next_int(&p);
                    l->per_player = (uint8_t)next_int(&p);
                    l->zone_radius = next_int(&p);
                    l->zone_weight = next_int(&p);
                    l->f28 = 8;
                }
            }
            if (m == &scratch) free(scratch.lands);
        }
    }
    if ((p = terrains) != NULL) {
        n = next_int(&p);
        for (i = 0; i < n; ++i) {
            AoeRmMapRecRor *m = map_at(c, next_int(&p));
            int32_t count = next_int(&p);
            AoeRmTerrainRecRor dummy;
            if (m) {
                free(m->terrains);
                m->terrains = NULL;
                m->terrain_count = count;
                if (count > 0 && count <= 0x10000) {
                    m->terrains = calloc((size_t)count, sizeof(*m->terrains));
                    if (!m->terrains) return 0;
                }
            }
            for (k = 0; k < count && k < 0x10000; ++k) {
                AoeRmTerrainRecRor *t = m && m->terrains ? &m->terrains[k] : &dummy;
                t->percent = next_int(&p);
                t->terrain = next_int(&p);
                t->clumps = next_int(&p);
                t->f0c = next_int(&p);
                t->f10 = next_int(&p);
                t->f14 = 20;
            }
        }
    }
    if ((p = objects) != NULL) {
        n = next_int(&p);
        for (i = 0; i < n; ++i) {
            AoeRmMapRecRor *m = map_at(c, next_int(&p));
            int32_t count = next_int(&p);
            AoeRmObjectRecRor dummy;
            if (m) {
                free(m->objects);
                m->objects = NULL;
                m->object_count = count;
                if (count > 0 && count <= 0x10000) {
                    m->objects = calloc((size_t)count, sizeof(*m->objects));
                    if (!m->objects) return 0;
                }
            }
            for (k = 0; k < count && k < 0x10000; ++k) {
                AoeRmObjectRecRor *o = m && m->objects ? &m->objects[k] : &dummy;
                o->unit = next_int(&p);
                o->f04 = next_int(&p);
                o->f08 = (uint8_t)next_int(&p);
                o->f09 = (uint8_t)next_int(&p);
                o->f0c = next_int(&p);
                o->f10 = next_int(&p);
                o->f14 = next_int(&p);
                o->f18 = next_int(&p);
                o->f1c = next_int(&p);
                o->land = next_int(&p);
                o->f24 = next_int(&p);
                o->f28 = next_int(&p);
            }
        }
    }
    return 1;
}

/* ---- Land placement 0x0046dff0 ---- */

static int32_t scaled(AoeRmControllerRor *c, int32_t span) {
    return aoe_rm_rand_scaled_ror(&c->base, span);
}

void aoe_rm_place_lands_ror(AoeRmControllerRor *c, int32_t first,
                            int32_t edge, int32_t count,
                            const AoeRmLandRecRor *land) {
    int32_t w = c->base.width, h = c->base.height, i, x0, y0, x1, y1, cx, cy;
    double spacing, half;
    float spacing2;
    (void)aoe_rng_next(c->base.rng); /* "calc land start" log line */
    spacing = (double)(w - edge * 2) / sqrt((double)count);
    if (count > 3) spacing *= count >= 7 ? (double)0.8f : (double)0.9f;
    spacing2 = (float)(spacing * spacing);
    cx = w / 2;
    half = spacing * 0.9;
    x0 = aoe_ftol32((double)cx - half);
    cy = h / 2;
    y0 = aoe_ftol32((double)cy - half);
    x1 = aoe_ftol32(half + (double)cx);
    y1 = aoe_ftol32((double)cy + half);
    if (x0 <= edge) x0 = edge + 1;
    if (y0 <= edge) y0 = edge + 1;
    if (x1 >= w - edge - 1) x1 = w - edge - 2;
    if (y1 >= h - edge - 1) y1 = h - edge - 2;
    switch (land->placement) {
    case 0:
        for (i = first; i < first + count; ++i) {
            c->lands[i].x = scaled(c, w - edge * 2) + edge;
            c->lands[i].y = scaled(c, h - edge * 2) + edge;
        }
        break;
    case 1:
        for (i = first; i < first + count; ++i) {
            c->lands[i].x = land->fixed_x;
            c->lands[i].y = land->fixed_y;
        }
        break;
    case 2: {
        /* The original retries without limit; a pathological map would
           hang it. The port gives up after a very large number of draws
           and keeps the last candidate. */
        long guard = 0;
        for (i = first; i < first + count;) {
            int32_t x = scaled(c, w - edge * 2) + edge;
            int32_t y = scaled(c, h - edge * 2) + edge;
            int32_t j, clash = -1;
            if (x0 < x && x < x1 && y0 < y && y < y1 && ++guard < 10000000L) continue;
            for (j = first; j < i; ++j) {
                int32_t dx = x - c->lands[j].x, dy = y - c->lands[j].y;
                if (dx < 0) dx = -dx;
                if (dy < 0) dy = -dy;
                if ((double)(dy * dy + dx * dx) <= (double)spacing2) {
                    clash = j;
                    break;
                }
            }
            if (clash >= 0 && ++guard < 10000000L) {
                if (clash == i - 1) {
                    c->lands[clash].x = x;
                    c->lands[clash].y = y;
                }
                continue;
            }
            c->lands[i].x = x;
            c->lands[i].y = y;
            ++i;
        }
        break;
    }
    default:
        break;
    }
}

/* ---- Land setup 0x0046e3a0 ---- */

static int32_t area_share(int32_t percent, int32_t total, int32_t divisor) {
    return (int32_t)((int64_t)percent * total / divisor);
}

static void copy_bounds(AoeRmControllerRor *c, AoeRmLandWorkRor *w) {
    int k;
    for (k = 0; k < 4; ++k) w->bounds[k] = c->bounds[k];
    w->avoid = c->avoid;
}

/* The team regrouping works on the original's 0x1a0-byte stack frame
   (indices are byte offsets / 4 from esp after the prologue): land teams
   at +0x3c, team sizes and distances at +0x60, player teams at +0x80 and
   the 8-wide neighbour table at +0xa0. Rows overlap when there are more
   than eight lands, and a "row -1" read lands on the player teams, as in
   the original; the frame here is large enough to keep such indices
   defined. The uninitialised +0x38 starts at 0. */
enum { FRAME = 1024 };
#define F(off) frame[(off) / 4]
static int32_t fget(const int32_t *frame, int32_t byte_off) {
    return byte_off >= 0 && byte_off / 4 < FRAME ? frame[byte_off / 4] : 0;
}
static void fset(int32_t *frame, int32_t byte_off, int32_t v) {
    if (byte_off >= 0 && byte_off / 4 < FRAME) frame[byte_off / 4] = v;
}
#define LT(i) (0x3c + 4 * (i))
#define D(i) (0x60 + 4 * (i))
#define TM(i) (0x80 + 4 * (i))
#define ORD(r, j) (0xa0 + 0x20 * (r) + 4 * (j))

static void regroup_teams(AoeRmControllerRor *c) {
    static int32_t frame[FRAME];
    int32_t n = c->land_count, i, j, k, teams = 0, t;
    long guard = 0;
    memset(frame, 0, sizeof frame);
    /* Part 1: each land's neighbours by distance (bubble sort). */
    for (i = 0; i < n; ++i) {
        int sorted;
        for (j = 0; j < n; ++j) {
            int32_t dx = c->lands[i].x - c->lands[j].x;
            int32_t dy = c->lands[i].y - c->lands[j].y;
            fset(frame, D(j), dy * dy + dx * dx);
        }
        for (j = 0; j < n; ++j) fset(frame, ORD(i, j), j);
        do {
            sorted = 1;
            if (n <= 1) break;
            for (j = 1; j < n; ++j)
                if (fget(frame, D(j - 1)) > fget(frame, D(j))) {
                    int32_t tmp = fget(frame, D(j - 1));
                    fset(frame, D(j - 1), fget(frame, D(j)));
                    fset(frame, D(j), tmp);
                    tmp = fget(frame, ORD(i, j));
                    fset(frame, ORD(i, j), fget(frame, ORD(i, j - 1)));
                    fset(frame, ORD(i, j - 1), tmp);
                    sorted = 0;
                }
        } while (!sorted);
    }
    /* Part 2: teams from the players' stances (a player counts itself
       too when its stance toward itself is 0). */
    for (k = 0; k < n; ++k) fset(frame, TM(k), -1);
    for (k = 0; k < n; ++k) {
        int32_t m;
        if (fget(frame, TM(k)) != -1) continue;
        fset(frame, TM(k), teams);
        fset(frame, D(teams), 1);
        for (m = 1; m <= c->land_count; ++m)
            if (c->game && c->game->stance &&
                c->game->stance(c->game->context, k + 1, m) == 0) {
                fset(frame, TM(m - 1), teams);
                fset(frame, D(teams), fget(frame, D(teams)) + 1);
            }
        ++teams;
    }
    /* Part 3: grow each team from the first free land through its
       nearest neighbours, trading lands with other teams when stuck. */
    for (j = 0; j < n; ++j) fset(frame, LT(j), -1);
    for (t = 0; t < teams; ++t) {
        int32_t start = 0, size, width, taken = 0;
        while (start < n && fget(frame, LT(start)) != -1) ++start;
        size = fget(frame, D(t));
        F(0x5c) = size;
        if (size <= 0) continue;
        width = size;
        for (;;) {
            int32_t found = -1, at, other, far, swap_land;
            if (++guard > 1000000L) return; /* the port's hang guard */
            for (j = 0; j < width; ++j) {
                int32_t l = fget(frame, ORD(start, j));
                if (fget(frame, LT(l)) == -1) {
                    found = l;
                    break;
                }
            }
            if (found >= 0) {
                fset(frame, LT(found), t);
                if (++taken < F(0x5c)) continue;
                break;
            }
            if (width < n) ++width;
            for (at = n - 1; at >= 0; --at) {
                int32_t l = fget(frame, ORD(start, at));
                if (fget(frame, LT(l)) == -1) {
                    F(0x38) = l;
                    break;
                }
            }
            far = F(0x38);
            other = -1;
            swap_land = F(0x20);
            for (j = 0; j < n; ++j) {
                int32_t l = fget(frame, ORD(far, j));
                int32_t team = fget(frame, LT(l));
                if (team == t || team <= -1) continue;
                other = team;
                swap_land = l;
                for (k = 0; k < width; ++k) {
                    int32_t l3 = fget(frame, ORD(start, k));
                    if (fget(frame, LT(l3)) == other) {
                        swap_land = l3;
                        break;
                    }
                }
                break;
            }
            F(0x20) = swap_land;
            fset(frame, LT(far), other);
            fset(frame, LT(swap_land), -1);
            for (k = n - 1; k >= 0; --k) {
                int32_t l4 = fget(frame, ORD(other, k));
                if (fget(frame, LT(l4)) != other) continue;
                for (j = 0; j < n; ++j) {
                    int32_t l5 = fget(frame, ORD(far, j));
                    if (fget(frame, LT(l5)) == -1 && l5 != swap_land) {
                        fset(frame, LT(l4), -1);
                        fset(frame, LT(l5), other);
                        break;
                    }
                }
            }
        }
    }
    /* Part 4: the d-th land goes to the first free player of its team. */
    for (j = 0; j < n; ++j) fset(frame, D(j), -1);
    for (j = 0; j < n; ++j) {
        int32_t team = fget(frame, LT(j));
        for (k = 0; k < n; ++k)
            if (fget(frame, D(k)) == -1 && fget(frame, TM(k)) == team) {
                fset(frame, D(k), j);
                break;
            }
    }
    {
        int32_t xs[AOE_RM_SLOTS], ys[AOE_RM_SLOTS];
        for (k = 0; k < n && k < AOE_RM_SLOTS; ++k) {
            int32_t l = fget(frame, D(k));
            /* No land for this player: the original reads the slot before
               the table (the controller's tile pointer and width); the
               port keeps the land where it is. */
            if (l < 0 || l >= n) l = k;
            xs[k] = c->lands[l].x;
            ys[k] = c->lands[l].y;
        }
        for (k = 0; k < n && k < AOE_RM_SLOTS; ++k) {
            c->lands[k].x = xs[k];
            c->lands[k].y = ys[k];
        }
    }
}
#undef F

void aoe_rm_setup_lands_ror(AoeRmControllerRor *c, int teams) {
    const AoeRmMapRecRor *m;
    int32_t total, edge, i, slot = 0;
    if (!c || !(m = map_at(c, c->map_index))) return;
    total = (int32_t)((int64_t)m->land_percent * c->base.width * c->base.height / 100);
    edge = m->edge;
    c->land_count = 0;
    for (i = 0; i < m->land_count; ++i)
        c->land_count += m->lands[i].per_player ? c->players : 1;
    c->f145c = (uint8_t)m->f1c;
    c->bounds[0] = m->margin[0];
    c->bounds[1] = m->margin[1];
    c->bounds[2] = c->base.width - m->margin[2];
    c->bounds[3] = c->base.height - m->margin[3];
    c->avoid = (uint8_t)m->f14;
    for (i = 0; i < m->land_count; ++i) {
        const AoeRmLandRecRor *l = &m->lands[i];
        if (slot >= AOE_RM_SLOTS) break;
        if (!l->per_player) {
            AoeRmLandWorkRor *w = &c->lands[slot];
            w->terrain = l->terrain;
            w->f14 = l->f08;
            w->radius = l->radius;
            w->id = l->player;
            aoe_rm_place_lands_ror(c, slot, edge, 1, l);
            w->area = area_share(l->area_percent, total, 100);
            w->weight = l->f28;
            copy_bounds(c, w);
            ++slot;
        } else {
            int32_t p;
            aoe_rm_place_lands_ror(c, slot, edge, c->players, l);
            for (p = 0; p < c->players && slot < AOE_RM_SLOTS; ++p, ++slot) {
                AoeRmLandWorkRor *w = &c->lands[slot];
                w->terrain = l->terrain;
                w->f14 = l->f08;
                w->radius = l->radius;
                w->id = l->per_player == 1 ? l->player : (uint8_t)p;
                w->area = area_share(l->area_percent, total, c->players * 100);
                w->weight = l->f28;
                copy_bounds(c, w);
            }
        }
    }
    if (teams) regroup_teams(c);
}

/* ---- Terrain, object and elevation setup ---- */

static void add_zones(AoeRmControllerRor *c, const AoeRmMapRecRor *m,
                      AoeRmZoneRor *zones, int32_t *count) {
    int32_t i, at = 0;
    for (i = 0; i < m->land_count; ++i) {
        const AoeRmLandRecRor *l = &m->lands[i];
        if (l->zone_radius < 0) continue;
        if (!l->per_player) {
            if (at < AOE_RM_SLOTS) {
                zones[at].x = c->lands[at].x;
                zones[at].y = c->lands[at].y;
                zones[at].radius = l->zone_radius;
                zones[at].weight = l->zone_weight;
            }
            ++at;
        } else {
            int32_t p;
            for (p = 0; p < c->players; ++p, ++at)
                if (at < AOE_RM_SLOTS) {
                    zones[at].x = c->lands[at].x;
                    zones[at].y = c->lands[at].y;
                    zones[at].radius = l->zone_radius;
                    zones[at].weight = l->zone_weight;
                }
        }
    }
    *count = at;
}

void aoe_rm_setup_terrains_ror(AoeRmControllerRor *c) {
    const AoeRmMapRecRor *m;
    int32_t i, area;
    if (!c || !(m = map_at(c, c->map_index))) return;
    area = (int32_t)((int64_t)m->land_percent * c->base.height * c->base.width / 100);
    c->terrain_count = m->terrain_count;
    for (i = 0; i < m->terrain_count && i < AOE_RM_SLOTS; ++i) {
        const AoeRmTerrainRecRor *t = &m->terrains[i];
        AoeRmTerrainWorkRor *w = &c->terrains[i];
        w->tiles = aoe_ftol32((double)(t->percent * area) * (double)0.01f);
        w->terrain = t->terrain;
        w->clumps = t->clumps;
        w->f04 = 0;
        w->f10 = t->f0c;
        w->f14 = t->f10;
        w->f18 = t->f14;
        w->flag = 1;
    }
    add_zones(c, m, c->terrain_zones, &c->terrain_zone_count);
}

void aoe_rm_setup_objects_ror(AoeRmControllerRor *c) {
    const AoeRmMapRecRor *m;
    int32_t i, rec = 0, player;
    if (!c || !(m = map_at(c, c->map_index))) return;
    c->object_count = m->object_count;
    for (i = 0; i < m->object_count && i < AOE_RM_SLOTS; ++i) {
        c->objects[i].rec = m->objects[i];
        c->objects[i].flag = 1;
    }
    c->object_land_count = c->land_count;
    player = m->land_count > 0 && m->lands[0].per_player ? 1 : 0;
    for (i = 0; i < c->land_count && i < AOE_RM_SLOTS; ++i) {
        AoeRmObjectLandRor *o = &c->object_lands[i];
        int advance;
        o->x = c->lands[i].x;
        o->y = c->lands[i].y;
        o->id = rec < m->land_count ? m->lands[rec].id : 0;
        o->player = player;
        if (player > 0) {
            if (++player > c->players) {
                player = 0;
                advance = 1;
            } else {
                advance = 0;
            }
        } else {
            advance = 1;
        }
        if (advance) {
            ++rec;
            if (rec < m->land_count && m->lands[rec].per_player) player = 1;
        }
    }
}

void aoe_rm_setup_elevation_ror(AoeRmControllerRor *c) {
    static const int32_t layer[4][6] = {
        {10, 1, 10, 2, 0, 0}, {8, 2, 8, 1, 1, 0},
        {6, 3, 6, 1, 2, 0},   {4, 4, 4, 1, 3, 0},
    };
    const AoeRmMapRecRor *m;
    int32_t area, i;
    if (!c || !(m = map_at(c, c->map_index))) return;
    area = (int32_t)((int64_t)m->land_percent * c->base.height * c->base.width / 100);
    c->layer_count = 4;
    for (i = 0; i < 4; ++i) {
        AoeRmLayerRor *l = &c->layers[i];
        l->tiles = area * layer[i][0] / 100;
        l->height = layer[i][1];
        l->f08 = layer[i][2];
        l->f0c = layer[i][3];
        l->f10 = layer[i][4];
        l->f14 = layer[i][5];
    }
    add_zones(c, m, c->elevation_zones, &c->elevation_zone_count);
}

/* ---- Rise of Rome wrappers ---- */

void aoe_rm_ror_setup_lands_ror(AoeRmControllerRor *c, int teams) {
    AoeRmMapRecRor *m;
    int32_t idx, saved, i, w, h;
    if (!c || !(m = map_at(c, c->map_index))) return;
    idx = c->map_index;
    w = c->base.width;
    h = c->base.height;
    saved = m->edge;
    if ((idx == 5 || idx == 8) && w < 100) m->edge = w > 79 ? 25 : 20;
    aoe_rm_setup_lands_ror(c, teams);
    m->edge = saved;
    if (idx < 5) {
        if (aoe_rng_next(c->base.rng) < 0x3fff) {
            c->bounds[0] = -1;
            for (i = 0; i < c->land_count; ++i) c->lands[i].bounds[0] = -1;
        }
        if (aoe_rng_next(c->base.rng) < 0x3fff) {
            c->bounds[1] = -1;
            for (i = 0; i < c->land_count; ++i) c->lands[i].bounds[1] = -1;
        }
        if (aoe_rng_next(c->base.rng) < 0x3fff) {
            c->bounds[2] = w;
            for (i = 0; i < c->land_count; ++i) c->lands[i].bounds[2] = w;
        }
        if (aoe_rng_next(c->base.rng) < 0x3fff) {
            c->bounds[3] = h;
            for (i = 0; i < c->land_count; ++i) c->lands[i].bounds[3] = h;
        }
    }
    if (idx == 8) {
        if (aoe_rng_next(c->base.rng) < 0x3fff) {
            c->bounds[2] = w;
            c->bounds[0] = -1;
            for (i = 0; i < c->land_count; ++i) {
                c->lands[i].bounds[0] = -1;
                c->lands[i].bounds[2] = w;
            }
        } else {
            c->bounds[3] = h;
            c->bounds[1] = -1;
            for (i = 0; i < c->land_count; ++i) {
                c->lands[i].bounds[1] = -1;
                c->lands[i].bounds[3] = h;
            }
        }
    }
    if ((idx > 1 && idx < 7) || idx == 8) {
        if (aoe_rng_next(c->base.rng) < 0x3fff) {
            int done = 0;
            if (aoe_rng_next(c->base.rng) < 0x3fff) {
                for (i = 0; i < c->land_count; i += 2) c->lands[i].id = 0;
            } else {
                for (i = 0; i < c->land_count; i += 2) c->lands[i].id = (uint8_t)i;
            }
            if (aoe_rng_next(c->base.rng) < 0x3fff) {
                if (c->land_count > 1) {
                    for (i = 1; i < c->land_count; i += 2) c->lands[i].id = 1;
                    aoe_rm_ror_shallows_ror(c);
                    done = 1;
                }
            } else if (c->land_count > 1) {
                for (i = 1; i < c->land_count; i += 2) c->lands[i].id = (uint8_t)i;
            }
            if (!done) aoe_rm_ror_shallows_ror(c);
        } else if (idx == 4) {
            for (i = 0; i < c->land_count; ++i) c->lands[i].id = 0;
            c->f145c = 0;
        } else if (idx > 4) {
            for (i = 0; i < c->land_count; ++i) c->lands[i].id = 0;
        }
    }
    if (idx == 5 || idx == 8) {
        int32_t e = 20, area;
        if (w < 100) {
            e = w > 79 ? 15 : 10;
            if (c->bounds[0] > 0) c->bounds[0] = e;
            if (c->bounds[1] > 0) c->bounds[1] = e;
            if (c->bounds[2] < w) c->bounds[2] = w - e;
            if (c->bounds[3] < h) c->bounds[3] = h - e;
            for (i = 0; i < c->land_count; ++i) {
                AoeRmLandWorkRor *l = &c->lands[i];
                if (l->bounds[0] > 0) l->bounds[0] = e;
                if (l->bounds[1] > 0) l->bounds[1] = e;
                if (l->bounds[2] < w) l->bounds[2] = w - e;
                if (l->bounds[3] < h) l->bounds[3] = h - e;
            }
        }
        area = aoe_ftol32((double)((h - e * 2) * (w - e * 2)) * (double)0.8f);
        for (i = 0; i < c->land_count; ++i) c->lands[i].area = area / c->land_count;
    }
    (void)aoe_rng_next(c->base.rng); /* "land initialization" log line */
}

void aoe_rm_ror_setup_terrains_ror(AoeRmControllerRor *c) {
    AoeRmMapRecRor *m;
    int32_t i, area;
    float factor;
    if (!c || !(m = map_at(c, c->map_index))) return;
    area = (int32_t)((int64_t)m->land_percent * c->base.height * c->base.width / 100);
    aoe_rm_setup_terrains_ror(c);
    factor = (float)((double)area * (double)5e-05f);
    for (i = 0; i < c->terrain_count && i < AOE_RM_SLOTS; ++i)
        if (c->terrains[i].terrain != 6 && (double)factor > 1.0)
            c->terrains[i].clumps = aoe_ftol32((double)c->terrains[i].clumps * (double)factor);
    if (aoe_rng_next(c->base.rng) < 0x1fff) {
        for (i = 0; i < c->terrain_count && i < AOE_RM_SLOTS; ++i) {
            AoeRmTerrainWorkRor *t = &c->terrains[i];
            switch (t->terrain) {
            case 13: t->tiles *= 3; t->clumps *= 3; t->flag = 2; break;
            case 20: case 19: t->tiles /= 2; t->clumps /= 2; break;
            case 10: t->tiles /= 2; t->clumps /= 2; t->flag = 2; break;
            case 6: t->tiles <<= 1; t->flag = 0; break;
            default: break;
            }
        }
    } else {
        for (i = 0; i < c->terrain_count && i < AOE_RM_SLOTS; ++i) {
            AoeRmTerrainWorkRor *t = &c->terrains[i];
            if (t->terrain == 6) t->flag = 0;
            else if (t->terrain == 10 || t->terrain == 13) t->flag = 2;
        }
    }
    (void)aoe_rng_next(c->base.rng); /* "terrain initialization" log line */
}

void aoe_rm_ror_setup_objects_ror(AoeRmControllerRor *c) {
    int32_t i, w, h;
    if (!c || !map_at(c, c->map_index)) return;
    w = c->base.width;
    h = c->base.height;
    aoe_rm_setup_objects_ror(c);
    (void)aoe_rng_next(c->base.rng);
    if (c->game && c->game->game_mode && c->game->game_mode(c->game->context) == 1) {
        for (i = 0; i < c->object_count && i < AOE_RM_SLOTS; ++i)
            if (c->objects[i].rec.unit == 0x53) c->objects[i].rec.f14 = 1;
        for (i = 0; i < c->object_count && i < AOE_RM_SLOTS; ++i)
            if (c->objects[i].rec.unit == 0x6d) c->objects[i].rec.f14 = 0;
    }
    if (c->map_index == 6 && c->land_count < AOE_RM_SLOTS) {
        AoeRmLandWorkRor *l = &c->lands[0];
        int32_t e = w / 6;
        memmove(&c->lands[1], &c->lands[0], (size_t)c->land_count * sizeof(c->lands[0]));
        l->id = (uint8_t)c->land_count;
        l->terrain = 1;
        l->f14 = 7;
        l->radius = 2;
        l->area = aoe_ftol32((double)((h - e * 2) * (w - e * 2)) * (double)0.7f);
        l->weight = 10;
        l->x = c->base.map->width / 2;
        l->y = c->base.map->height / 2;
        l->bounds[0] = e;
        l->bounds[1] = e;
        l->bounds[2] = w - e;
        l->bounds[3] = h - e;
        l->avoid = 10;
        ++c->land_count;
    }
    if (c->map_index < 2) {
        int32_t extra = aoe_rm_rand_scaled_ror(&c->base, 4) + 1, k;
        if (c->players + extra > 10) extra = 10 - c->players;
        for (i = 0; i < c->land_count; ++i) /* (a zero count would trap) */
            c->lands[i].area -= (w + h) * extra / c->land_count;
        for (k = 0; k < extra; ++k) {
            int32_t slot = c->land_count + k, x = 0, y = 0, tries = 0, accept = 0;
            AoeRmLandWorkRor *l;
            if (slot >= AOE_RM_SLOTS) break;
            l = &c->lands[slot];
            l->terrain = 0;
            l->f14 = 7;
            l->radius = 3;
            l->id = (uint8_t)(k + c->land_count);
            l->area = aoe_ftol32((double)(w + h) * 1.25);
            l->weight = 10;
            copy_bounds(c, l);
            do {
                int ok = 1, j;
                ++tries;
                x = aoe_rm_rand_scaled_ror(&c->base, w - 16) + 8;
                y = aoe_rm_rand_scaled_ror(&c->base, h - 16) + 8;
                for (j = 0; j < c->land_count && ok; ++j) {
                    int32_t dx = x - c->lands[j].x, dy = y - c->lands[j].y;
                    if (dy * dy + dx * dx < 400) ok = 0;
                }
                if (ok) {
                    if (tries < 1000) accept = 1;
                    break;
                }
            } while (tries < 1000);
            if (!accept) extra = k; /* the last one is still written */
            l->x = x;
            l->y = y;
            if (c->object_count < AOE_RM_SLOTS) {
                AoeRmObjectWorkRor *o = &c->objects[c->object_count];
                memset(o, 0, sizeof(*o));
                o->rec.f04 = -1;
                o->rec.f08 = 2;
                o->rec.f09 = 0;
                o->rec.f0c = 8;
                o->rec.f14 = 1;
                o->rec.f18 = 4;
                o->rec.land = slot;
                o->rec.f28 = 0x1a;
                o->flag = 0;
                o->rec.unit = aoe_rng_next(c->base.rng) < 0x3fff ? 0x66 : 0x42;
                ++c->object_count;
            }
            if (c->object_land_count < AOE_RM_SLOTS) {
                AoeRmObjectLandRor *ol = &c->object_lands[c->object_land_count];
                ol->y = y;
                ol->x = x;
                ol->id = slot;
                ol->player = 0;
                ++c->object_land_count;
            }
        }
        c->land_count += extra;
    }
    for (i = 0; i < c->object_count && i < AOE_RM_SLOTS; ++i) {
        AoeRmObjectWorkRor *o = &c->objects[i];
        int32_t mw = c->base.map->width;
        if (o->rec.land >= 0) {
            switch (o->rec.unit) {
            case 0x3b: case 0x41: case 0x42: case 0x66: o->flag = 12; break;
            default: break;
            }
        } else if (o->rec.land == -2) {
            switch (o->rec.unit) {
            case 0x3b: case 0x42: case 0x66: o->flag = mw / 6; break;
            case 0x41: o->flag = mw / 8; break;
            default: break;
            }
        }
    }
    (void)aoe_rng_next(c->base.rng); /* "object initialization" log line */
}

void aoe_rm_ror_setup_elevation_ror(AoeRmControllerRor *c) {
    double factor;
    int32_t i;
    if (!c || !map_at(c, c->map_index)) return;
    aoe_rm_setup_elevation_ror(c);
    factor = ((double)aoe_rng_next(c->base.rng) * (double)0.00183111f + (double)70.0f) *
             (double)0.01f;
    if (c->map_index == 7) factor *= (double)5.0f;
    for (i = 0; i < c->layer_count && i < AOE_RM_SLOTS; ++i)
        c->layers[i].tiles = aoe_ftol32((double)c->layers[i].tiles * factor);
    (void)aoe_rng_next(c->base.rng); /* "elevation initialization" log line */
}

int aoe_rm_controller_setup_ror(AoeRmControllerRor *c, AoeRmMapRor *map,
                                int32_t object_arg, int32_t map_index,
                                int32_t players, int teams) {
    if (!c) return 0;
    c->object_arg = object_arg;
    c->map_index = map_index & 0xff;
    c->players = players;
    aoe_rm_module_set_map_ror(&c->base, map);
    if (!aoe_rm_module_alloc_grids_ror(&c->base)) return 0;
    if (c->ror) {
        aoe_rm_ror_setup_lands_ror(c, teams);
        aoe_rm_ror_setup_terrains_ror(c);
        aoe_rm_ror_setup_objects_ror(c);
        aoe_rm_ror_setup_elevation_ror(c);
    } else {
        aoe_rm_setup_lands_ror(c, teams);
        aoe_rm_setup_terrains_ror(c);
        aoe_rm_setup_objects_ror(c);
        aoe_rm_setup_elevation_ror(c);
    }
    return 1;
}

void aoe_rm_controller_cleanup_ror(AoeRmControllerRor *c) {
    if (!c) return;
    c->object_arg = 0;
    c->map_index = -1;
    c->players = 0;
    while (c->base.children) {
        AoeRmModuleRor *child = c->base.children->module;
        if (!child) {
            aoe_rm_module_remove_child_ror(&c->base, NULL);
            continue;
        }
        if (child->vt && child->vt->destroy) child->vt->destroy(child);
        else aoe_rm_module_release_ror(child);
        free(child);
    }
    aoe_rm_module_free_grids_ror(&c->base);
    aoe_rm_module_set_map_ror(&c->base, NULL);
}

void aoe_rm_ror_shallows_ror(AoeRmControllerRor *c) {
    /* The shallows module itself is created by aoe_rm_shallows_create_ror
       (random_map modules); without it only the log line's rand() runs. */
    if (!c) return;
    aoe_rm_shallows_create_ror(c);
    (void)aoe_rng_next(c->base.rng); /* "shallows initialization" log line */
}
