#include "demo_app.h"

#include "color_table.h"
#include "cpx.h"
#include "live_unit_sprite_backend.h"
#include "palette.h"
#include "scenario_scene.h"
#include "world_combat.h"

#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Formatting note: VitaSDK's newlib printf does not consume C99 %zu/%llu
   arguments (later arguments shift, and a %s then reads a number as a
   pointer), so this file formats only int, long and double values.

   Demo shell around the reconstructed runtime. Everything drawn in the
   scene comes from the user's original data through the translated
   loaders, world update and renderer; only the overlay text, its font and
   the camera/input mapping belong to this shell. */

enum { DEMO_PATH = 512, DEMO_MAX_SCENARIOS = 256 };

typedef struct {
    char path[DEMO_PATH * 2]; /* .scx/.scn, or the .cpx holding it */
    int32_t cpx_index;    /* -1 for a loose scenario */
    char name[96];
} DemoScenario;

typedef struct {
    uint8_t colors[AOE_SCENARIO_RUNTIME_PLAYERS];
    uint8_t active_player;
} DemoResolver;

struct AoeDemo {
    char root[DEMO_PATH];
    AoeDemoClock clock;
    char status[256];

    DatBlob blob;
    DatIntro intro;
    DatSounds sounds;
    DatGraphics graphics;
    DatTerrains terrains;
    DatBorders borders;
    DatMapGeometry geometry;
    DatRandomMaps random_maps;
    DatEffects effects;
    DatTechs techs;
    DatCivilizations civs;

    DrsArchive exp_graphics, base_graphics, terrain_drs, border_drs,
        exp_interface, base_interface;
    DrsArchive *graphic_archives[6];
    TerrainArtCache terrain_art;
    BorderArtCache border_art;
    AoeGraphicArtCache graphic_cache;
    AoeColorTableSet color_tables;
    Palette palette;
    uint32_t palette_abgr[256];
    int water_start;          /* palette_water_cycle_* state */
    float water_elapsed_ms;
    uint8_t shadow[256];
    uint8_t *edge_bytes;
    TileEdgeIndex edges;
    uint8_t *pixels, *first_clip, *second_clip;
    /* Terrain layer cache: sprites are drawn after all terrain (the view's
       sprite list), so the terrain layer can be kept between frames,
       shifted when the camera scrolls (only exposed strips redrawn) and
       redrawn when the map's tile data changes. */
    uint8_t *terrain;
    int terrain_valid;
    int32_t terrain_cam_x, terrain_cam_y;
    uint64_t terrain_hash;
    uint32_t palette_half[256]; /* overlay backgrounds (halved colours) */
    AoeDemoParallel parallel;
    void *parallel_context;
    int parallel_parts;

    DemoScenario *scenarios;
    size_t scenario_count;
    size_t current;

    int loaded;
    ScnMap scenario;
    AoeScenarioRuntimeRor runtime;
    AoeWorldMotionRor motion;
    AoeWorldCombatRor combat;
    DemoResolver resolver;
    AoeLiveUnitIndexedBackend backend;
    AoeLiveUnitDrawRorOps draw_ops;

    float camera_x, camera_y;
    int poi;
    int hud;
    int paused;
    float sim_accumulator;
    char message[160];
    float message_time;
    AoeDemoStats stats;
};

/* ------------------------------------------------------------------ */
/* Files */

static void demo_status(AoeDemo *demo, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(demo->status, sizeof(demo->status), format, args);
    va_end(args);
}

static void demo_message(AoeDemo *demo, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(demo->message, sizeof(demo->message), format, args);
    va_end(args);
    demo->message_time = 4.0f;
}

static int name_equal(const char *a, const char *b) {
    for (; *a && *b; ++a, ++b)
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
    return *a == *b;
}

static int name_has_suffix(const char *name, const char *suffix) {
    size_t n = strlen(name), s = strlen(suffix);
    return n >= s && name_equal(name + n - s, suffix);
}

/* Resolves `name` inside `dir`, ignoring ASCII case (the original data was
   authored for case-insensitive Windows paths). */
static int find_file(const char *dir, const char *name, char *out,
                     size_t size) {
    DIR *d = opendir(dir);
    struct dirent *e;
    int found = 0;
    if (!d) return 0;
    while (!found && (e = readdir(d)) != NULL)
        if (name_equal(e->d_name, name))
            found = snprintf(out, size, "%s/%s", dir, e->d_name) <
                    (int)size;
    closedir(d);
    return found;
}

static int find_dir(const char *root, const char *name, char *out,
                    size_t size) {
    return find_file(root, name, out, size);
}

static uint8_t *read_file(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");
    uint8_t *bytes = NULL;
    long length;
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) == 0 && (length = ftell(file)) > 0 &&
        fseek(file, 0, SEEK_SET) == 0 &&
        (bytes = malloc((size_t)length)) != NULL) {
        if (fread(bytes, 1, (size_t)length, file) != (size_t)length) {
            free(bytes);
            bytes = NULL;
        } else {
            *size = (size_t)length;
        }
    }
    fclose(file);
    return bytes;
}

static int scenario_compare(const void *a, const void *b) {
    const DemoScenario *x = a, *y = b;
    int c = strcmp(x->path, y->path);
    return c ? c : (x->cpx_index > y->cpx_index) - (x->cpx_index < y->cpx_index);
}

static int add_scenario(AoeDemo *demo, const char *path, int32_t index,
                        const char *name) {
    DemoScenario *s;
    if (demo->scenario_count >= DEMO_MAX_SCENARIOS) return 1;
    s = &demo->scenarios[demo->scenario_count++];
    snprintf(s->path, sizeof(s->path), "%s", path);
    s->cpx_index = index;
    snprintf(s->name, sizeof(s->name), "%s", name);
    return 1;
}

static void scan_scenarios(AoeDemo *demo) {
    char dir[DEMO_PATH], path[DEMO_PATH * 2];
    DIR *d;
    struct dirent *e;
    if (find_dir(demo->root, "scenario", dir, sizeof(dir)) &&
        (d = opendir(dir)) != NULL) {
        while ((e = readdir(d)) != NULL)
            if (name_has_suffix(e->d_name, ".scx") ||
                name_has_suffix(e->d_name, ".scn")) {
                snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
                add_scenario(demo, path, -1, e->d_name);
            }
        closedir(d);
    }
    if (find_dir(demo->root, "campaign", dir, sizeof(dir)) &&
        (d = opendir(dir)) != NULL) {
        while ((e = readdir(d)) != NULL) {
            CpxArchive cpx = {0};
            if (!name_has_suffix(e->d_name, ".cpx")) continue;
            snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
            if (!cpx_open(&cpx, path)) continue;
            for (uint32_t i = 0; i < cpx.count; ++i) {
                char name[96];
                snprintf(name, sizeof(name), "%.28s: %.60s", cpx.title,
                         cpx.scenarios[i].name);
                add_scenario(demo, path, (int32_t)i, name);
            }
            cpx_close(&cpx);
        }
        closedir(d);
    }
    qsort(demo->scenarios, demo->scenario_count, sizeof(*demo->scenarios),
          scenario_compare);
}

/* ------------------------------------------------------------------ */
/* Scene callbacks (as the host scene tool binds them) */

static int resolve_unit(void *opaque, const AoeLiveUnit *unit,
                        AoeLiveUnitViewportStateRor *state) {
    const DemoResolver *resolver = opaque;
    int owner;
    if (!resolver || !unit || !state) return 0;
    owner = unit->runtime_owner_index;
    /* A doppelganger (class 0x19) draws with its source's colour object
       (+0x90), under its source's ID, and not for its hidden players. */
    if (unit->core.class_code == 0x19) owner = unit->doppel.source_owner;
    if (owner < 0 || owner >= AOE_SCENARIO_RUNTIME_PLAYERS) return 0;
    memset(state, 0, sizeof(*state));
    state->active_player = resolver->active_player;
    state->owner_color = resolver->colors[owner];
    if (unit->core.class_code == 0x19) {
        state->type25_controller =
            unit->doppel.source ? &unit->doppel.source->core : NULL;
        state->type25_hidden_players = unit->doppel.hidden_players;
    }
    return 1;
}

static int resolve_color(void *opaque, const void *selected,
                         uint8_t fallback, uint8_t *color,
                         const uint8_t **table) {
    AoeLiveUnitIndexedBackend *backend = opaque;
    (void)selected;
    if (!backend || !color || !table) return 0;
    *color = fallback;
    *table = backend->shadow_table;
    return 1;
}

/* ------------------------------------------------------------------ */
/* Open / close */

static void update_palette_lut(AoeDemo *demo, int first, int count) {
    for (int i = first; i < first + count; ++i) {
        PaletteColor c = demo->palette.colors[i];
        demo->palette_abgr[i] = 0xff000000u | (uint32_t)c.b << 16 |
                                (uint32_t)c.g << 8 | c.r;
        demo->palette_half[i] =
            0xff000000u | ((demo->palette_abgr[i] >> 1) & 0x007f7f7fu);
    }
}

static int open_drs(AoeDemo *demo, DrsArchive *archive, const char *dir,
                    const char *name) {
    char path[DEMO_PATH * 2];
    if (!find_file(dir, name, path, sizeof(path)) ||
        !drs_open(archive, path)) {
        demo_status(demo, "Missing or unreadable %s/%s", dir, name);
        return 0;
    }
    return 1;
}

AoeDemo *aoe_demo_open(const char *root, AoeDemoClock clock, char *error,
                       size_t error_size) {
    AoeDemo *demo = calloc(1, sizeof(*demo));
    char base[DEMO_PATH], exp[DEMO_PATH], path[DEMO_PATH * 2];
    DrsArchive *terrain_archives[1], *border_archives[1];
    DrsArchive *interface_archives[2];
    DrsMatch palette_match;
    uint8_t *palette_bytes = NULL;
    size_t edge_size = 0;
    if (!demo) {
        if (error) snprintf(error, error_size, "Out of memory");
        return NULL;
    }
    snprintf(demo->root, sizeof(demo->root), "%s", root);
    demo->clock = clock;
    demo->hud = 1;
    demo->resolver = (DemoResolver){{8, 0, 1, 2, 3, 4, 5, 6, 7}, 1};
    demo->graphic_archives[0] = &demo->exp_graphics;
    demo->graphic_archives[1] = &demo->base_graphics;
    demo->graphic_archives[2] = &demo->exp_interface;
    demo->graphic_archives[3] = &demo->base_interface;
    demo->graphic_archives[4] = &demo->terrain_drs;
    demo->graphic_archives[5] = &demo->border_drs;
    terrain_archives[0] = &demo->terrain_drs;
    border_archives[0] = &demo->border_drs;
    interface_archives[0] = &demo->exp_interface;
    interface_archives[1] = &demo->base_interface;

    if (!find_dir(root, "data", base, sizeof(base)) ||
        !find_dir(root, "data2", exp, sizeof(exp))) {
        demo_status(demo, "Copy the game's data and data2 folders to %s",
                    root);
        goto fail;
    }
    if (!find_file(exp, "empires.dat", path, sizeof(path)) ||
        !dat_load(path, &demo->blob)) {
        demo_status(demo, "Missing or unreadable %s/EMPIRES.DAT", exp);
        goto fail;
    }
    if (!dat_parse_intro(&demo->blob, &demo->intro) ||
        !dat_parse_sounds(&demo->blob, &demo->intro, &demo->sounds) ||
        !dat_parse_graphics(&demo->blob, &demo->sounds, &demo->graphics) ||
        !dat_parse_terrains(&demo->blob, &demo->intro, &demo->graphics,
                            &demo->terrains) ||
        !dat_parse_borders(&demo->blob, &demo->terrains, &demo->borders) ||
        !dat_parse_map_geometry(&demo->blob, &demo->terrains, &demo->borders,
                                &demo->geometry) ||
        !dat_parse_random_maps(&demo->blob, &demo->geometry,
                               &demo->random_maps) ||
        !dat_parse_effects(&demo->blob, &demo->random_maps, &demo->effects) ||
        !dat_parse_civilizations(&demo->blob, &demo->effects, &demo->civs) ||
        !dat_parse_techs(&demo->blob, &demo->civs, &demo->techs)) {
        demo_status(demo, "EMPIRES.DAT is not the Rise of Rome 1.0a DAT");
        goto fail;
    }
    if (!open_drs(demo, &demo->exp_graphics, exp, "graphics.drs") ||
        !open_drs(demo, &demo->exp_interface, exp, "interfac.drs") ||
        !open_drs(demo, &demo->base_graphics, base, "graphics.drs") ||
        !open_drs(demo, &demo->base_interface, base, "interfac.drs") ||
        !open_drs(demo, &demo->terrain_drs, base, "terrain.drs") ||
        !open_drs(demo, &demo->border_drs, base, "border.drs"))
        goto fail;
    if (!terrain_art_load_direct(&demo->terrain_art, &demo->terrains,
                                 terrain_archives, 1) ||
        !border_art_load_direct(&demo->border_art, &demo->borders,
                                border_archives, 1)) {
        demo_status(demo, "Terrain or border art failed to load");
        goto fail;
    }
    if (!drs_find_first(interface_archives, 2, drs_type("anib"), 50500,
                        &palette_match) ||
        !(palette_bytes = malloc(palette_match.entry->size)) ||
        !drs_read(palette_match.archive, palette_match.entry, palette_bytes,
                  palette_match.entry->size) ||
        !palette_parse_jasc(palette_bytes, palette_match.entry->size,
                            &demo->palette) ||
        !aoe_shadow_table_load(interface_archives, 2, demo->shadow)) {
        demo_status(demo, "Palette 50500 or the shadow table is missing");
        goto fail;
    }
    free(palette_bytes);
    palette_bytes = NULL;
    palette_water_cycle_apply(&demo->palette, 0);
    update_palette_lut(demo, 0, 256);
    if (!aoe_color_table_set_load_directory(&demo->intro, base,
                                            &demo->color_tables) ||
        !aoe_color_table_set_load_drs_fallback(&demo->intro,
                                               interface_archives, 2,
                                               &demo->color_tables)) {
        demo_status(demo, "Color tables failed to load");
        goto fail;
    }
    if (!find_file(base, "tileedge.dat", path, sizeof(path)) ||
        !(demo->edge_bytes = read_file(path, &edge_size)) ||
        !tileedge_parse(demo->edge_bytes, edge_size, &demo->edges)) {
        demo_status(demo, "Missing or unreadable %s/TILEEDGE.DAT", base);
        goto fail;
    }
    /* Decoded frames are cached up to 48 MiB; the Vita build reserves its
       newlib heap for this and the runtime. */
    if (!aoe_graphic_art_cache_init(&demo->graphic_cache, 512,
                                    48u * 1024u * 1024u)) {
        demo_status(demo, "Out of memory for the sprite cache");
        goto fail;
    }
    demo->pixels = calloc((size_t)AOE_DEMO_WIDTH * AOE_DEMO_HEIGHT, 1);
    demo->first_clip = calloc((size_t)AOE_DEMO_WIDTH * AOE_DEMO_HEIGHT, 1);
    demo->second_clip = calloc((size_t)AOE_DEMO_WIDTH * AOE_DEMO_HEIGHT, 1);
    demo->terrain = calloc((size_t)AOE_DEMO_WIDTH * AOE_DEMO_HEIGHT, 1);
    demo->scenarios = calloc(DEMO_MAX_SCENARIOS, sizeof(*demo->scenarios));
    if (!demo->pixels || !demo->first_clip || !demo->second_clip ||
        !demo->terrain || !demo->scenarios) {
        demo_status(demo, "Out of memory for the frame buffers");
        goto fail;
    }
    scan_scenarios(demo);
    if (!demo->scenario_count) {
        demo_status(demo, "No scenarios: add *.scx to %s/scenario or *.cpx "
                    "to %s/campaign", root, root);
        goto fail;
    }
    demo->backend = (AoeLiveUnitIndexedBackend){
        .archives = demo->graphic_archives, .archive_count = 6,
        .cache = &demo->graphic_cache, .shadow_table = demo->shadow,
        .skip_missing_art = 1,
        .resolve_fixed_color_table = aoe_color_table_set_resolve,
        .fixed_color_context = &demo->color_tables};
    demo->backend.resolve_color_source = resolve_color;
    demo->backend.color_source_context = &demo->backend;
    demo->draw_ops = (AoeLiveUnitDrawRorOps){
        .context = &demo->backend,
        .sprite = aoe_live_unit_sprite_indexed_ror};
    demo_status(demo, "Loaded %u scenarios", (unsigned)demo->scenario_count);
    return demo;
fail:
    free(palette_bytes);
    if (error) snprintf(error, error_size, "%s", demo->status);
    aoe_demo_close(demo);
    return NULL;
}

static void unload_scenario(AoeDemo *demo) {
    if (!demo->loaded) return;
    aoe_world_combat_destroy_ror(&demo->combat);
    aoe_world_motion_destroy_ror(&demo->motion);
    aoe_scenario_runtime_destroy_ror(&demo->runtime);
    scn_free_map(&demo->scenario);
    memset(&demo->combat, 0, sizeof(demo->combat));
    memset(&demo->motion, 0, sizeof(demo->motion));
    memset(&demo->runtime, 0, sizeof(demo->runtime));
    memset(&demo->scenario, 0, sizeof(demo->scenario));
    demo->loaded = 0;
}

void aoe_demo_close(AoeDemo *demo) {
    if (!demo) return;
    unload_scenario(demo);
    aoe_graphic_art_cache_clear(&demo->graphic_cache);
    free(demo->scenarios);
    free(demo->terrain);
    free(demo->second_clip);
    free(demo->first_clip);
    free(demo->pixels);
    free(demo->edge_bytes);
    border_art_free(&demo->border_art);
    terrain_art_free(&demo->terrain_art);
    drs_close(&demo->base_interface);
    drs_close(&demo->exp_interface);
    drs_close(&demo->border_drs);
    drs_close(&demo->terrain_drs);
    drs_close(&demo->base_graphics);
    drs_close(&demo->exp_graphics);
    dat_free_civilizations(&demo->civs);
    dat_free_techs(&demo->techs);
    dat_free_effects(&demo->effects);
    dat_free_random_maps(&demo->random_maps);
    dat_free_borders(&demo->borders);
    dat_free_terrains(&demo->terrains);
    dat_free_graphics(&demo->graphics);
    dat_free_sounds(&demo->sounds);
    dat_free_intro(&demo->intro);
    dat_free(&demo->blob);
    free(demo);
}

size_t aoe_demo_scenario_count(const AoeDemo *demo) {
    return demo ? demo->scenario_count : 0;
}

const char *aoe_demo_status(const AoeDemo *demo) {
    return demo ? demo->status : "";
}

/* ------------------------------------------------------------------ */
/* Units */

typedef int (*UnitVisitor)(AoeDemo *demo, AoeLiveUnit *unit, void *context);

/* Scenario units in player/record order, then units created at runtime. */
static AoeLiveUnit *visit_units(AoeDemo *demo, UnitVisitor visit,
                                void *context) {
    AoeScenarioRuntimeRor *rt = &demo->runtime;
    for (int p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p)
        for (uint32_t i = 0; i < rt->players[p].unit_count; ++i) {
            AoeLiveUnit *u = &rt->players[p].units[i];
            if (u->constructed && visit(demo, u, context)) return u;
        }
    for (size_t i = 0; i < rt->editor_unit_count; ++i) {
        AoeLiveUnit *u = rt->editor_units[i];
        if (u && u->constructed && visit(demo, u, context)) return u;
    }
    return NULL;
}

static int alive(const AoeLiveUnit *u) {
    return u->constructed && u->core.state == 2 && u->core.unit_template;
}

static int count_live(AoeDemo *demo, AoeLiveUnit *u, void *context) {
    (void)demo;
    /* States 3..5 include resource carcasses (forage, kills) that stay on
       the map until gathered. */
    *(uint32_t *)context += u->core.unit_template && u->core.state <= 5;
    return 0;
}

typedef struct {
    int owner;
    int16_t template_id;
} FindTemplate;

static int match_template(AoeDemo *demo, AoeLiveUnit *u, void *context) {
    const FindTemplate *f = context;
    (void)demo;
    return alive(u) && u->runtime_owner_index == f->owner &&
           u->core.unit_template->id == f->template_id;
}

static int is_soldier(const AoeLiveUnit *u) {
    const DatUnit *t = u->core.unit_template;
    int attack_task = 0;
    if (!alive(u) || t->type != 70 || !(t->speed > 0.0f) ||
        t->attack_count == 0)
        return 0;
    for (uint16_t k = 0; k < t->task_count; ++k) {
        if (t->tasks[k].action_type == 5) return 0; /* villagers gather */
        attack_task |= t->tasks[k].action_type == 7;
    }
    return attack_task;
}

typedef struct {
    float x, y, best;
    int owner, enemies;
    AoeLiveUnit *found;
} FindNearest;

static int nearest_enemy(AoeDemo *demo, AoeLiveUnit *u, void *context) {
    FindNearest *f = context;
    float dx, dy, d;
    (void)demo;
    if (!alive(u) || u->runtime_owner_index < 1 ||
        u->runtime_owner_index == f->owner || u->core.unit_template->type < 70)
        return 0;
    dx = u->core.position.x38 - f->x;
    dy = u->core.position.y3c - f->y;
    d = dx * dx + dy * dy;
    if (!f->found || d < f->best) {
        f->found = u;
        f->best = d;
    }
    return 0;
}

typedef struct {
    float cx, cy, radius;
    int owner;
    uint32_t ordered;
} AttackOrder;

static int order_attack(AoeDemo *demo, AoeLiveUnit *u, void *context) {
    AttackOrder *a = context;
    FindNearest f;
    int16_t code = 0;
    if (u->runtime_owner_index != a->owner || !is_soldier(u) ||
        fabsf(u->core.position.x38 - a->cx) > a->radius ||
        fabsf(u->core.position.y3c - a->cy) > a->radius)
        return 0;
    memset(&f, 0, sizeof(f));
    f.x = u->core.position.x38;
    f.y = u->core.position.y3c;
    f.owner = a->owner;
    visit_units(demo, nearest_enemy, &f);
    if (f.found &&
        aoe_world_combat_order_ror(&demo->combat, u, f.found,
                                   f.found->core.position.x38,
                                   f.found->core.position.y3c,
                                   f.found->core.position.z40, &code))
        ++a->ordered;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Camera */

static void clamp_camera(AoeDemo *demo) {
    float x, y;
    if (terrain_clamp_camera_tiles(demo->runtime.map.width,
                                   demo->runtime.map.height, demo->camera_x,
                                   demo->camera_y, &x, &y)) {
        demo->camera_x = x;
        demo->camera_y = y;
    }
}

typedef struct {
    int owner;
    float x, y;
    uint32_t count;
} OwnerCentroid;

static int sum_owner(AoeDemo *demo, AoeLiveUnit *u, void *context) {
    OwnerCentroid *c = context;
    (void)demo;
    if (alive(u) && u->runtime_owner_index == c->owner) {
        c->x += u->core.position.x38;
        c->y += u->core.position.y3c;
        ++c->count;
    }
    return 0;
}

/* Points of interest: each player's serialized camera, then the centre of
   that player's units. */
static void next_point_of_interest(AoeDemo *demo) {
    int players = demo->runtime.player_count - 1;
    if (players < 1) return;
    for (int tries = 0; tries < 2 * players; ++tries) {
        int slot, p;
        demo->poi = (demo->poi + 1) % (2 * players);
        slot = demo->poi / 2;
        p = slot + 1;
        if (demo->poi % 2 == 0) {
            const ScnPlayerDetail *d = &demo->scenario.player_details[slot];
            if (d->camera_x <= 0.0f && d->camera_y <= 0.0f) continue;
            demo->camera_x = d->camera_x;
            demo->camera_y = d->camera_y;
            demo_message(demo, "Player %d camera", p);
        } else {
            OwnerCentroid c = {p, 0, 0, 0};
            visit_units(demo, sum_owner, &c);
            if (!c.count) continue;
            demo->camera_x = c.x / (float)c.count;
            demo->camera_y = c.y / (float)c.count;
            demo_message(demo, "Player %d forces (%u units)", p, c.count);
        }
        clamp_camera(demo);
        return;
    }
}

/* ------------------------------------------------------------------ */
/* Scenario */

int aoe_demo_load_scenario(AoeDemo *demo, size_t index) {
    const DemoScenario *s;
    char extracted[DEMO_PATH + 32];
    const char *path;
    if (!demo || !demo->scenario_count) return 0;
    unload_scenario(demo);
    demo->current = index % demo->scenario_count;
    s = &demo->scenarios[demo->current];
    path = s->path;
    if (s->cpx_index >= 0) {
        CpxArchive cpx = {0};
        int ok;
        snprintf(extracted, sizeof(extracted), "%s/demo_cache.scx",
                 demo->root);
        ok = cpx_open(&cpx, s->path) &&
             cpx_extract(&cpx, (uint32_t)s->cpx_index, extracted);
        cpx_close(&cpx);
        if (!ok) {
            demo_status(demo, "Could not extract %s", s->name);
            return 0;
        }
        path = extracted;
    }
    if (!scn_load_map(path, (uint8_t)demo->intro.terrain_count,
                      &demo->scenario)) {
        demo_status(demo, "Unsupported scenario %s", s->name);
        return 0;
    }
    if (!aoe_scenario_runtime_build_ror(
            &demo->runtime, &demo->scenario, &demo->intro, &demo->civs,
            &demo->graphics, &demo->terrains, &demo->borders,
            &demo->geometry, &demo->techs, &demo->effects, NULL, 1) ||
        !aoe_scenario_runtime_start_game_ror(&demo->runtime) ||
        !aoe_scenario_runtime_set_active_player_ror(&demo->runtime, 1) ||
        !aoe_world_motion_init_ror(&demo->motion, &demo->runtime) ||
        !aoe_world_combat_init_ror(&demo->combat, &demo->motion)) {
        demo->loaded = 1;
        unload_scenario(demo);
        demo_status(demo, "Runtime build failed for %s", s->name);
        return 0;
    }
    demo->loaded = 1;
    demo->terrain_valid = 0;
    demo->camera_x = demo->scenario.player_details[0].camera_x;
    demo->camera_y = demo->scenario.player_details[0].camera_y;
    demo->poi = 0;
    clamp_camera(demo);
    demo->sim_accumulator = 0.0f;
    demo->stats.trained = demo->stats.attack_orders = 0;
    demo->stats.world_updates = 0;
    demo_status(demo, "%s", s->name);
    demo_message(demo, "Loaded %s (%ux%u)", s->name, demo->runtime.map.width,
                 demo->runtime.map.height);
    return 1;
}

/* Loads `index`, or the next scenario that builds, at most one full lap. */
static void load_from(AoeDemo *demo, size_t index, int step) {
    for (size_t tries = 0; tries < demo->scenario_count; ++tries) {
        if (aoe_demo_load_scenario(demo, index)) return;
        index = (index + demo->scenario_count + (size_t)step) %
                demo->scenario_count;
    }
}

/* ------------------------------------------------------------------ */
/* Update */

static uint64_t now_us(const AoeDemo *demo) {
    return demo->clock ? demo->clock() : 0;
}

/* One world tick: 0.05 s, the step used by the world regressions. */
enum { DEMO_MAX_STEPS = 4 };
static const float demo_tick = 0.05f;

int aoe_demo_update(AoeDemo *demo, const AoeDemoInput *input, float dt) {
    uint64_t start;
    if (!demo) return 0;
    if (input) {
        if (input->pressed & AOE_DEMO_BUTTON_QUIT) return 0;
        if (input->pressed & AOE_DEMO_BUTTON_HUD) demo->hud = !demo->hud;
        if (input->pressed & AOE_DEMO_BUTTON_PAUSE) {
            demo->paused = !demo->paused;
            demo_message(demo, "%s", demo->paused ? "Paused" : "Running");
        }
        if (input->pressed & AOE_DEMO_BUTTON_NEXT)
            load_from(demo, demo->current + 1, 1);
        if (input->pressed & AOE_DEMO_BUTTON_PREV)
            load_from(demo, demo->current + demo->scenario_count - 1, -1);
    }
    if (!demo->loaded) {
        load_from(demo, demo->current, 1);
        if (!demo->loaded) return 1;
    }
    if (input) {
        /* Screen-space scrolling through the projection x+y = sx/hw,
           y-x = sy/hh, at 900 pixels per second at full deflection. */
        float hw = (float)demo->geometry.tile_half_width;
        float hh = (float)demo->geometry.tile_half_height;
        float px = input->scroll_x * 900.0f * dt;
        float py = input->scroll_y * 900.0f * dt;
        if (hw > 0.0f && hh > 0.0f && (px != 0.0f || py != 0.0f)) {
            demo->camera_x += 0.5f * (px / hw - py / hh);
            demo->camera_y += 0.5f * (px / hw + py / hh);
            clamp_camera(demo);
        }
        if (input->pressed & AOE_DEMO_BUTTON_CAMERA)
            next_point_of_interest(demo);
        if (input->pressed & AOE_DEMO_BUTTON_TRAIN) {
            FindTemplate f = {1, 109};
            AoeLiveUnit *tc = visit_units(demo, match_template, &f);
            if (!tc)
                demo_message(demo, "Player 1 has no town center");
            else if (!aoe_world_combat_train_ror(&demo->combat, tc, 83, 1))
                demo_message(demo, "Cannot train: resources or population");
            else {
                ++demo->stats.trained;
                demo->camera_x = tc->core.position.x38;
                demo->camera_y = tc->core.position.y3c;
                clamp_camera(demo);
                demo_message(demo, "Villager queued at the town center");
            }
        }
        if (input->pressed & AOE_DEMO_BUTTON_ATTACK) {
            AttackOrder a = {demo->camera_x, demo->camera_y, 14.0f, 1, 0};
            visit_units(demo, order_attack, &a);
            demo->stats.attack_orders += a.ordered;
            demo_message(demo, a.ordered
                                   ? "%u soldiers ordered to attack"
                                   : "No player 1 soldiers near the camera",
                         a.ordered);
        }
    }
    if (dt > 0.0f) {
        float fps = 1.0f / dt;
        demo->stats.fps = demo->stats.fps > 0.0f
                              ? demo->stats.fps * 0.9f + fps * 0.1f
                              : fps;
    }
    if (demo->message_time > 0.0f) demo->message_time -= dt;
    /* The water colour cycle runs on wall time (0x004843d0). */
    demo->water_elapsed_ms += dt * 1000.0f;
    if (demo->water_elapsed_ms >= AOE_WATER_CYCLE_INTERVAL_MS) {
        demo->water_elapsed_ms = 0.0f;
        demo->water_start = palette_water_cycle_next(demo->water_start);
        palette_water_cycle_apply(&demo->palette, demo->water_start);
        update_palette_lut(demo, AOE_WATER_CYCLE_FIRST,
                           AOE_WATER_CYCLE_COUNT);
    }
    start = now_us(demo);
    if (!demo->paused) {
        int steps = 0;
        demo->sim_accumulator += dt;
        while (demo->sim_accumulator >= demo_tick && steps < DEMO_MAX_STEPS) {
            if (!aoe_world_combat_update_world_ror(&demo->combat, demo_tick)) {
                demo_status(demo, "World update failed");
                demo->paused = 1;
                break;
            }
            demo->sim_accumulator -= demo_tick;
            ++demo->stats.world_updates;
            ++steps;
        }
        /* Drop time the device could not keep up with. */
        if (demo->sim_accumulator > demo_tick) demo->sim_accumulator = 0.0f;
    }
    demo->stats.update_ms = (float)(now_us(demo) - start) / 1000.0f;
    return 1;
}

/* ------------------------------------------------------------------ */
/* Overlay: the demo's own 5x7 font (not original art) */

static const uint8_t font_digits[10][7] = {
    {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e},
    {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e},
    {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f},
    {0x1f, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0e},
    {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02},
    {0x1f, 0x10, 0x1e, 0x01, 0x01, 0x11, 0x0e},
    {0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e},
    {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e},
    {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c}};

static const uint8_t font_letters[26][7] = {
    {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}, /* A */
    {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e},
    {0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e},
    {0x1e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e},
    {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f},
    {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10},
    {0x0e, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0f},
    {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11},
    {0x0e, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e},
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0c},
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f},
    {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11},
    {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11},
    {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e},
    {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10},
    {0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d},
    {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11},
    {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e},
    {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04},
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0a},
    {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11},
    {0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04},
    {0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f}}; /* Z */

static const struct {
    char c;
    uint8_t rows[7];
} font_symbols[] = {
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c}},
    {',', {0x00, 0x00, 0x00, 0x00, 0x0c, 0x04, 0x08}},
    {':', {0x00, 0x0c, 0x0c, 0x00, 0x0c, 0x0c, 0x00}},
    {'/', {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00}},
    {'-', {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00}},
    {'(', {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}},
    {')', {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}},
    {'%', {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03}},
    {'+', {0x00, 0x04, 0x04, 0x1f, 0x04, 0x04, 0x00}},
    {'=', {0x00, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x00}},
    {'\'', {0x04, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00}},
    {'!', {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04}},
    {'?', {0x0e, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}},
    {'<', {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02}},
    {'>', {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08}},
    {'_', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f}},
    {'[', {0x0e, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0e}},
    {']', {0x0e, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0e}},
    {'#', {0x0a, 0x0a, 0x1f, 0x0a, 0x1f, 0x0a, 0x0a}},
    {'*', {0x00, 0x15, 0x0e, 0x1f, 0x0e, 0x15, 0x00}}};

static const uint8_t *glyph(char c) {
    static const uint8_t unknown[7] = {0x1f, 0x11, 0x11, 0x11,
                                       0x11, 0x11, 0x1f};
    if (c >= '0' && c <= '9') return font_digits[c - '0'];
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (c >= 'A' && c <= 'Z') return font_letters[c - 'A'];
    for (size_t i = 0; i < sizeof(font_symbols) / sizeof(*font_symbols); ++i)
        if (font_symbols[i].c == c) return font_symbols[i].rows;
    return unknown;
}

void aoe_demo_fill_rect(uint32_t *fb, size_t stride, int x, int y, int w,
                        int h, uint32_t color, int blend_half) {
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > AOE_DEMO_WIDTH ? AOE_DEMO_WIDTH : x + w;
    int y1 = y + h > AOE_DEMO_HEIGHT ? AOE_DEMO_HEIGHT : y + h;
    for (int j = y0; j < y1; ++j)
        for (int i = x0; i < x1; ++i) {
            uint32_t *p = &fb[(size_t)j * stride + (size_t)i];
            *p = blend_half ? 0xff000000u | ((*p >> 1) & 0x007f7f7fu) |
                                  ((color >> 1) & 0x007f7f7fu)
                            : color;
        }
}

void aoe_demo_draw_text(uint32_t *fb, size_t stride, int x, int y, int scale,
                        uint32_t color, const char *text) {
    int cx = x;
    for (; *text; ++text) {
        const uint8_t *rows;
        if (*text == '\n') {
            cx = x;
            y += 9 * scale;
            continue;
        }
        if (*text != ' ') {
            rows = glyph(*text);
            for (int r = 0; r < 7; ++r)
                for (int c = 0; c < 5; ++c)
                    if (rows[r] & (0x10 >> c)) {
                        /* One-pixel drop shadow keeps text legible. */
                        aoe_demo_fill_rect(fb, stride, cx + c * scale + 1,
                                           y + r * scale + 1, scale, scale,
                                           0xff000000u, 0);
                        aoe_demo_fill_rect(fb, stride, cx + c * scale,
                                           y + r * scale, scale, scale,
                                           color, 0);
                    }
        }
        cx += 6 * scale;
    }
}

/* ------------------------------------------------------------------ */
/* Render */

/* FNV-1a over the map tile records and tile +10 draw flags: the inputs of
   the terrain layer besides the camera (fog is off, so visibility masks do
   not affect it). */
static uint64_t map_terrain_hash(const AoeRuntimeMap *map) {
    uint64_t h = 1469598103934665603ull;
    size_t n = (size_t)map->width * map->height;
    const uint8_t *t = (const uint8_t *)map->tiles;
    for (size_t i = 0; i < n * sizeof(*map->tiles); ++i)
        h = (h ^ t[i]) * 1099511628211ull;
    for (size_t i = 0; map->draw_flags && i < n; ++i)
        h = (h ^ map->draw_flags[i]) * 1099511628211ull;
    return h;
}

static void scene_draw_init(AoeDemo *demo, AoeScenarioSceneDrawRor *draw,
                            const TerrainViewCamera *camera,
                            const TerrainViewIndexedSurface *surface,
                            int pass) {
    *draw = (AoeScenarioSceneDrawRor){
        .geometry = &demo->geometry, .terrains = &demo->terrains,
        .borders = &demo->borders, .terrain_art = &demo->terrain_art,
        .border_art = &demo->border_art, .camera = camera,
        .surface = surface, .tile_edges = &demo->edges,
        .first_clip = {demo->first_clip,
                       (size_t)AOE_DEMO_WIDTH * AOE_DEMO_HEIGHT,
                       AOE_DEMO_WIDTH},
        .second_clip = {demo->second_clip,
                        (size_t)AOE_DEMO_WIDTH * AOE_DEMO_HEIGHT,
                        AOE_DEMO_WIDTH},
        /* Scenario fog history is not reconstructed yet; the original
           exploration-gate bypass shows the whole map. */
        .fog_enabled = 0, .bypass_exploration_gate = 1,
        .resolve_unit = resolve_unit,
        .resolve_unit_context = &demo->resolver,
        .draw_ops = &demo->draw_ops, .pass = pass};
}

/* Draws the terrain layer into `target` inside one clip rectangle. */
static int draw_terrain_rect(AoeDemo *demo, uint8_t *target,
                             const TerrainViewCamera *camera, uint32_t left,
                             uint32_t top, uint32_t width, uint32_t height) {
    AoeScenarioSceneDrawRor draw;
    AoeScenarioSceneResultRor result;
    TerrainViewIndexedSurface surface = {
        target, (size_t)AOE_DEMO_WIDTH * AOE_DEMO_HEIGHT, AOE_DEMO_WIDTH,
        AOE_DEMO_HEIGHT, AOE_DEMO_WIDTH, left, top, width, height};
    /* The full view's camera (same pixel origin and tile walk, so anchors
       stay in absolute screen coordinates) with its view rectangle
       narrowed to the clip, as the terrain walk requires. */
    TerrainViewCamera strip = *camera;
    if (!width || !height) return 1;
    strip.view_left = (int32_t)left;
    strip.view_top = (int32_t)top;
    strip.view_width = (int32_t)width;
    strip.view_height = (int32_t)height;
    for (uint32_t y = top; y < top + height; ++y)
        memset(target + (size_t)y * AOE_DEMO_WIDTH + left, 0, width);
    scene_draw_init(demo, &draw, &strip, &surface, AOE_SCENE_PASS_TERRAIN);
    return aoe_scenario_runtime_draw_scene_ror(&demo->runtime, &draw,
                                               &result);
}

/* Brings the cached terrain layer up to date for `camera`. */
static int update_terrain(AoeDemo *demo, const TerrainViewCamera *camera) {
    const int32_t w = AOE_DEMO_WIDTH, h = AOE_DEMO_HEIGHT;
    uint64_t hash = map_terrain_hash(&demo->runtime.map);
    int32_t dx = camera->camera_pixel_x - demo->terrain_cam_x;
    int32_t dy = camera->camera_pixel_y - demo->terrain_cam_y;
    int ok = 1;
    if (!demo->terrain_valid || hash != demo->terrain_hash ||
        dx <= -w || dx >= w || dy <= -h || dy >= h) {
        ok = draw_terrain_rect(demo, demo->terrain, camera, 0, 0, w, h);
        ++demo->stats.terrain_full;
    } else if (dx || dy) {
        /* New pixel (x, y) is the old pixel (x + dx, y + dy). */
        int32_t y0 = dy > 0 ? 0 : h - 1, y1 = dy > 0 ? h : -1;
        int32_t step = dy > 0 ? 1 : -1;
        int32_t cx0 = dx > 0 ? 0 : -dx, cw = w - (dx > 0 ? dx : -dx);
        for (int32_t y = y0; y != y1; y += step) {
            int32_t sy = y + dy;
            if (sy < 0 || sy >= h) continue;
            memmove(demo->terrain + (size_t)y * w + cx0,
                    demo->terrain + (size_t)sy * w + cx0 + dx, (size_t)cw);
        }
        /* Exposed column band (all rows), then exposed row band without
           those columns: each a clipped terrain draw. */
        if (dx > 0)
            ok = draw_terrain_rect(demo, demo->terrain, camera,
                                   (uint32_t)(w - dx), 0, (uint32_t)dx,
                                   (uint32_t)h);
        else if (dx < 0)
            ok = draw_terrain_rect(demo, demo->terrain, camera, 0, 0,
                                   (uint32_t)-dx, (uint32_t)h);
        if (ok && dy > 0)
            ok = draw_terrain_rect(demo, demo->terrain, camera,
                                   (uint32_t)cx0, (uint32_t)(h - dy),
                                   (uint32_t)cw, (uint32_t)dy);
        else if (ok && dy < 0)
            ok = draw_terrain_rect(demo, demo->terrain, camera,
                                   (uint32_t)cx0, 0, (uint32_t)cw,
                                   (uint32_t)-dy);
        ++demo->stats.terrain_shifted;
    }
    demo->terrain_valid = ok;
    demo->terrain_hash = hash;
    demo->terrain_cam_x = camera->camera_pixel_x;
    demo->terrain_cam_y = camera->camera_pixel_y;
    return ok;
}

size_t aoe_demo_verify_terrain_cache(AoeDemo *demo) {
    TerrainViewCamera camera;
    size_t mismatches = 0;
    if (!demo || !demo->loaded || !demo->terrain_valid ||
        !terrain_compute_view_camera(demo->runtime.map.tiles,
                                     demo->runtime.map.width,
                                     demo->runtime.map.height,
                                     &demo->geometry, demo->camera_x,
                                     demo->camera_y, 0, 0, AOE_DEMO_WIDTH,
                                     AOE_DEMO_HEIGHT, &camera))
        return 0;
    if (!draw_terrain_rect(demo, demo->pixels, &camera, 0, 0, AOE_DEMO_WIDTH,
                           AOE_DEMO_HEIGHT))
        return (size_t)-1;
    for (size_t i = 0; i < (size_t)AOE_DEMO_WIDTH * AOE_DEMO_HEIGHT; ++i)
        mismatches += demo->pixels[i] != demo->terrain[i];
    return mismatches;
}

/* One frame's colour conversion: overlay bands (HUD rows, message box)
   use the halved palette. Parts are disjoint row bands. */
typedef struct {
    const AoeDemo *demo;
    uint32_t *fb;
    size_t stride;
    int hud, message;
    uint32_t mx0, mx1;
} ExpandJob;

static void expand_part(void *arg, int part, int parts) {
    const ExpandJob *job = arg;
    const AoeDemo *demo = job->demo;
    uint32_t y0 = (uint32_t)(AOE_DEMO_HEIGHT * part / parts);
    uint32_t y1 = (uint32_t)(AOE_DEMO_HEIGHT * (part + 1) / parts);
    for (uint32_t y = y0; y < y1; ++y) {
        const uint8_t *src = demo->pixels + (size_t)y * AOE_DEMO_WIDTH;
        uint32_t *dst = job->fb + (size_t)y * job->stride;
        int band = job->hud && (y < 52 || y >= AOE_DEMO_HEIGHT - 20);
        const uint32_t *lut = band ? demo->palette_half : demo->palette_abgr;
        if (!band && job->message && y >= 70 && y < 100) {
            for (uint32_t x = 0; x < job->mx0; ++x) dst[x] = lut[src[x]];
            for (uint32_t x = job->mx0; x < job->mx1; ++x)
                dst[x] = demo->palette_half[src[x]];
            for (uint32_t x = job->mx1; x < AOE_DEMO_WIDTH; ++x)
                dst[x] = lut[src[x]];
        } else {
            for (uint32_t x = 0; x < AOE_DEMO_WIDTH; ++x)
                dst[x] = lut[src[x]];
        }
    }
}

void aoe_demo_set_parallel(AoeDemo *demo, AoeDemoParallel run, void *context,
                           int parts) {
    if (!demo) return;
    demo->parallel = run;
    demo->parallel_context = context;
    demo->parallel_parts = parts > 0 ? parts : 1;
}

/* Draws the indexed scene into demo->pixels and updates draw stats. */
static void render_scene(AoeDemo *demo) {
    TerrainViewCamera camera;
    AoeScenarioSceneResultRor result;
    memset(&result, 0, sizeof(result));
    if (demo->loaded &&
        terrain_compute_view_camera(demo->runtime.map.tiles,
                                    demo->runtime.map.width,
                                    demo->runtime.map.height, &demo->geometry,
                                    demo->camera_x, demo->camera_y, 0, 0,
                                    AOE_DEMO_WIDTH, AOE_DEMO_HEIGHT,
                                    &camera)) {
        TerrainViewIndexedSurface surface = {
            demo->pixels, (size_t)AOE_DEMO_WIDTH * AOE_DEMO_HEIGHT,
            AOE_DEMO_WIDTH, AOE_DEMO_HEIGHT, AOE_DEMO_WIDTH,
            0, 0, AOE_DEMO_WIDTH, AOE_DEMO_HEIGHT};
        AoeScenarioSceneDrawRor draw;
        uint64_t t0 = now_us(demo), t1;
        int ok = update_terrain(demo, &camera);
        t1 = now_us(demo);
        demo->stats.terrain_ms = (float)(t1 - t0) / 1000.0f;
        memcpy(demo->pixels, demo->terrain,
               (size_t)AOE_DEMO_WIDTH * AOE_DEMO_HEIGHT);
        scene_draw_init(demo, &draw, &camera, &surface,
                        AOE_SCENE_PASS_OBJECTS);
        ok = ok && aoe_scenario_runtime_draw_scene_ror(&demo->runtime, &draw,
                                                       &result);
        demo->stats.objects_ms = (float)(now_us(demo) - t1) / 1000.0f;
        if (!ok)
            demo_status(demo, "Scene draw failed: unit %d graphic %.20s "
                        "status %d", demo->backend.last_unit_id,
                        demo->backend.last_graphic
                            ? demo->backend.last_graphic->name : "-",
                        demo->backend.last_status);
    } else {
        memset(demo->pixels, 0, (size_t)AOE_DEMO_WIDTH * AOE_DEMO_HEIGHT);
    }
    demo->stats.units_drawn = (uint32_t)result.units.drawn;
    demo->stats.terrain_tiles = (uint32_t)result.terrain_tiles;
    /* The overlay's unit count walks every unit: refresh it twice a second
       rather than every frame. */
    if (demo->stats.frames % 30 == 0) {
        demo->stats.live_units = 0;
        if (demo->loaded)
            visit_units(demo, count_live, &demo->stats.live_units);
    }
    demo->stats.camera_x = demo->camera_x;
    demo->stats.camera_y = demo->camera_y;
    ++demo->stats.frames;
}

int aoe_demo_render_indexed(AoeDemo *demo, uint8_t *dst, size_t stride) {
    uint64_t start;
    if (!demo || !dst || stride < AOE_DEMO_WIDTH) return 0;
    start = now_us(demo);
    render_scene(demo);
    for (uint32_t y = 0; y < AOE_DEMO_HEIGHT; ++y)
        memcpy(dst + (size_t)y * stride,
               demo->pixels + (size_t)y * AOE_DEMO_WIDTH, AOE_DEMO_WIDTH);
    demo->stats.expand_ms = 0.0f;
    demo->stats.draw_ms = (float)(now_us(demo) - start) / 1000.0f;
    return 1;
}

const uint32_t *aoe_demo_palette_abgr(const AoeDemo *demo) {
    return demo ? demo->palette_abgr : NULL;
}

static void add_text(AoeDemoOverlay *o, int x, int y, int scale,
                     uint32_t color, const char *format, ...) {
    va_list args;
    AoeDemoText *t;
    if (o->text_count >= AOE_DEMO_OVERLAY_TEXTS) return;
    t = &o->texts[o->text_count++];
    t->x = x;
    t->y = y;
    t->scale = scale;
    t->color = color;
    va_start(args, format);
    vsnprintf(t->text, sizeof(t->text), format, args);
    va_end(args);
}

void aoe_demo_overlay(const AoeDemo *demo, AoeDemoOverlay *o) {
    const uint32_t white = 0xffffffffu, gold = 0xff40d0ffu;
    if (!o) return;
    memset(o, 0, sizeof(*o));
    if (!demo) return;
    if (demo->hud) {
        const float *r = demo->loaded ? demo->runtime.players[1].resource_values
                                      : NULL;
        o->rects[o->rect_count++] = (AoeDemoRect){0, 0, AOE_DEMO_WIDTH, 52};
        o->rects[o->rect_count++] =
            (AoeDemoRect){0, AOE_DEMO_HEIGHT - 20, AOE_DEMO_WIDTH, 20};
        add_text(o, 8, 6, 2, gold, "%s",
                 "AGE OF EMPIRES: RISE OF ROME - VITA PORT TECH DEMO");
        add_text(o, 8, 30, 1, white,
                 "%.60s  [%u/%u]  units %u drawn %u  tick %lu%s",
                 demo->status, (unsigned)(demo->current + 1),
                 (unsigned)demo->scenario_count, demo->stats.live_units,
                 demo->stats.units_drawn,
                 (unsigned long)demo->stats.world_updates,
                 demo->paused ? "  PAUSED" : "");
        if (r && demo->runtime.players[1].resources.count > 33)
            add_text(o, 8, 40, 1, white,
                     "P1 FOOD %.0f  WOOD %.0f  GOLD %.0f  STONE %.0f  "
                     "POP %.0f/%.0f",
                     r[0], r[1], r[3], r[2], r[11], r[32]);
        add_text(o, 600, 30, 1, white, "FPS %.0f  SIM %.1f MS  DRAW %.1f MS",
                 demo->stats.fps, demo->stats.update_ms,
                 demo->stats.draw_ms);
        add_text(o, 600, 40, 1, white, "TER %.1f  OBJ %.1f  EXP %.1f MS",
                 demo->stats.terrain_ms, demo->stats.objects_ms,
                 demo->stats.expand_ms);
        add_text(o, 8, AOE_DEMO_HEIGHT - 14, 1, white, "%s",
                 "STICK/DPAD SCROLL  X TRAIN VILLAGER  [] ARMY ATTACK  "
                 "O NEXT VIEW  L/R SCENARIO  SELECT PAUSE  TRIANGLE HUD  "
                 "START QUIT");
    }
    if (demo->message_time > 0.0f && demo->message[0]) {
        int w = (int)strlen(demo->message) * 12 + 24;
        if (w > AOE_DEMO_WIDTH) w = AOE_DEMO_WIDTH;
        o->rects[o->rect_count++] =
            (AoeDemoRect){(AOE_DEMO_WIDTH - w) / 2, 70, w, 30};
        add_text(o, (AOE_DEMO_WIDTH - w) / 2 + 12, 78, 2, white, "%s",
                 demo->message);
    }
}

int aoe_demo_render(AoeDemo *demo, uint32_t *fb, size_t stride) {
    AoeDemoOverlay overlay;
    uint64_t start;
    int message;
    uint32_t mx0 = 0, mx1 = 0;
    if (!demo || !fb || stride < AOE_DEMO_WIDTH) return 0;
    start = now_us(demo);
    render_scene(demo);
    aoe_demo_overlay(demo, &overlay);
    message = demo->message_time > 0.0f && demo->message[0];
    if (message) {
        const AoeDemoRect *box = &overlay.rects[overlay.rect_count - 1];
        mx0 = (uint32_t)box->x;
        mx1 = (uint32_t)(box->x + box->w);
    }
    /* Overlay backgrounds are halved through the palette, so the overlay
       never reads the (possibly uncached) framebuffer back. */
    {
        ExpandJob job = {demo, fb, stride, demo->hud, message, mx0, mx1};
        uint64_t t2 = now_us(demo);
        if (demo->parallel && demo->parallel_parts > 1)
            demo->parallel(demo->parallel_context, expand_part, &job,
                           demo->parallel_parts);
        else
            expand_part(&job, 0, 1);
        demo->stats.expand_ms = (float)(now_us(demo) - t2) / 1000.0f;
    }
    demo->stats.draw_ms = (float)(now_us(demo) - start) / 1000.0f;
    for (size_t i = 0; i < overlay.text_count; ++i)
        aoe_demo_draw_text(fb, stride, overlay.texts[i].x,
                           overlay.texts[i].y, overlay.texts[i].scale,
                           overlay.texts[i].color, overlay.texts[i].text);
    return 1;
}

void aoe_demo_set_camera(AoeDemo *demo, float x, float y) {
    if (!demo || !demo->loaded) return;
    demo->camera_x = x;
    demo->camera_y = y;
    clamp_camera(demo);
}

void aoe_demo_stats(const AoeDemo *demo, AoeDemoStats *stats) {
    if (demo && stats) *stats = demo->stats;
}
