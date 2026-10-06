/* The game-side screen switching for the translated screens. */
#include "ui_director.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deflate_writer.h"
#include "inflate.h"
#include "game_startup.h"

#define DIR(c) ((AoeUiDirectorRor *)(c))

/* Each opener constructs its screen with calloc (the screens are heap
   objects the panel system frees on close) and keeps it registered. */
#define OPENER(fn, Type, init, ...)                                          \
    static void fn(void *c) {                                                \
        AoeUiDirectorRor *d = DIR(c);                                        \
        Type *s = calloc(1, sizeof *s);                                      \
        if (!s) return;                                                      \
        if (!init(s, &d->ui, &d->host->area, ##__VA_ARGS__)) {               \
            free(s);                                                         \
            return;                                                          \
        }                                                                    \
        d->opened++;                                                         \
    }

static int build_main_menu(AoeMainMenuRor *m, AoeGameUiRor *ui, AoeUiAreaRor *area,
                           const AoeUiPictureOpsRor *pictures) {
    return aoe_main_menu_init_ror(m, ui, area, pictures);
}

OPENER(open_main_menu_impl, AoeMainMenuRor, build_main_menu, &DIR(c)->host->picture_ops)
OPENER(open_single_player_menu, AoeSinglePlayerMenuRor, aoe_single_player_menu_init_ror)
OPENER(open_editor_menu, AoeScenarioEditorMenuRor, aoe_scenario_editor_menu_init_ror)
OPENER(open_mp_startup, AoeMpStartupScreenRor, aoe_mp_startup_screen_init_ror)
OPENER(open_credits, AoeCreditsScreenRor, aoe_credits_screen_init_ror)

OPENER(open_select_scenario, AoeSelectScenarioRor, aoe_select_scenario_init_ror)
OPENER(open_name_selection, AoeNameSelectionRor, aoe_name_selection_init_ror)
OPENER(open_campaign_selection, AoeCampaignSelectionRor, aoe_campaign_selection_init_ror)

static void open_main_menu(void *c) { open_main_menu_impl(c); }

/* ---- the scenario index ---- */

static void free_scenarios(AoeUiDirectorRor *d) {
    int32_t i;
    for (i = 0; i < d->scenario_count; ++i)
        if (d->scenarios[i].has_record) aoe_scenario_record_free_ror(&d->scenarios[i].record.base);
    free(d->scenarios);
    d->scenarios = NULL;
    d->scenario_count = 0;
}

static int has_suffix(const char *name, const char *suffix) {
    size_t n = strlen(name), k = strlen(suffix), i;
    if (n < k) return 0;
    for (i = 0; i < k; ++i)
        if (tolower((unsigned char)name[n - k + i]) != suffix[i]) return 0;
    return 1;
}

static int compare_names(const void *a, const void *b) {
    const char *x = ((const AoeDirectorScenarioRor *)a)->name;
    const char *y = ((const AoeDirectorScenarioRor *)b)->name;
    for (; *x && *y; ++x, ++y) {
        int cx = tolower((unsigned char)*x), cy = tolower((unsigned char)*y);
        if (cx != cy) return cx - cy;
    }
    return tolower((unsigned char)*x) - tolower((unsigned char)*y);
}

/* The record after the file's 4-byte version (the game's +0x38 read). */
static int read_record(const char *path, AoeScenarioRecordExRor *r) {
    FILE *f = fopen(path, "rb");
    uint8_t *data;
    long n;
    AoeOutcomeStream s;
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) <= 4 || n > (16L << 20) ||
        fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }
    data = malloc((size_t)n);
    if (!data || fread(data, 1, (size_t)n, f) != (size_t)n) {
        free(data);
        fclose(f);
        return 0;
    }
    fclose(f);
    memset(&s, 0, sizeof s);
    s.data = data + 4;
    s.size = (size_t)n - 4;
    memset(r, 0, sizeof *r);
    aoe_scenario_record_ex_read_ror(r, &s);
    free(data);
    if (r->base.error || s.failed) {
        aoe_scenario_record_free_ror(&r->base);
        return 0;
    }
    return 1;
}

void aoe_ui_director_refresh_scenarios_ror(AoeUiDirectorRor *d) {
    static const char *const kinds[2] = {".scn", ".scx"};
    char dir[600];
    int k;
    free_scenarios(d);
    if (!aoe_ui_host_find_ror(d->host, "scenario", NULL, dir, sizeof dir)) return;
    for (k = 0; k < 2; ++k) {
        DIR *dp = opendir(dir);
        struct dirent *e;
        int32_t first = d->scenario_count;
        if (!dp) return;
        while ((e = readdir(dp)) != NULL) {
            AoeDirectorScenarioRor *grown;
            if (!has_suffix(e->d_name, kinds[k]) || strlen(e->d_name) >= 260) continue;
            grown = realloc(d->scenarios, sizeof *grown * (size_t)(d->scenario_count + 1));
            if (!grown) break;
            d->scenarios = grown;
            memset(&grown[d->scenario_count], 0, sizeof *grown);
            snprintf(grown[d->scenario_count].name, sizeof grown->name, "%s", e->d_name);
            d->scenario_count++;
        }
        closedir(dp);
        qsort(d->scenarios + first, (size_t)(d->scenario_count - first), sizeof *d->scenarios,
              compare_names);
    }
    for (k = 0; k < d->scenario_count; ++k) {
        char path[900];
        snprintf(path, sizeof path, "%s/%s", dir, d->scenarios[k].name);
        d->scenarios[k].has_record = read_record(path, &d->scenarios[k].record);
    }
}

static void index_refresh(void *c) { aoe_ui_director_refresh_scenarios_ror(DIR(c)); }

static const char *index_name(void *c, int32_t i) {
    AoeUiDirectorRor *d = DIR(c);
    return i >= 0 && i < d->scenario_count ? d->scenarios[i].name : NULL;
}

static int index_info(void *c, int32_t id, const char **description, int32_t *flag,
                      int32_t *players) {
    AoeUiDirectorRor *d = DIR(c);
    if (id < 0 || id >= d->scenario_count || !d->scenarios[id].has_record) return 0;
    *description = d->scenarios[id].record.base.text;
    *flag = d->scenarios[id].record.has_conditions;
    *players = d->scenarios[id].record.players;
    return 1;
}

/* ---- campaign files and the player file ---- */

static uint8_t *read_whole(const char *path, uint32_t *size) {
    FILE *f = fopen(path, "rb");
    uint8_t *data;
    long n;
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 || n > (64L << 20) ||
        fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    data = malloc(n ? (size_t)n : 1);
    if (!data || fread(data, 1, (size_t)n, f) != (size_t)n) {
        free(data);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *size = (uint32_t)n;
    return data;
}

static int write_whole(const char *path, const uint8_t *data, uint32_t size) {
    FILE *f = fopen(path, "wb");
    int ok;
    if (!f) return 0;
    ok = fwrite(data, 1, size, f) == size;
    return (fclose(f) == 0) && ok;
}

static uint8_t *camp_read_file(void *c, const char *path, uint32_t *size) {
    (void)c;
    return read_whole(path, size);
}

static int camp_write_file(void *c, const char *path, const uint8_t *data, uint32_t size) {
    (void)c;
    return write_whole(path, data, size);
}

/* "<dir>*.ext" patterns: the names of the folder's files with that
   extension, as _findfirst / _findnext list them (name order on NTFS). */
static void camp_find(void *c, const char *pattern,
                      void (*each)(void *user, const char *name), void *user) {
    char dir[600];
    const char *star = strrchr(pattern, '*');
    size_t n;
    DIR *dp;
    struct dirent *e;
    char **names = NULL;
    size_t count = 0, i;
    (void)c;
    if (!star) return;
    n = (size_t)(star - pattern);
    if (n >= sizeof dir) return;
    memcpy(dir, pattern, n);
    dir[n] = '\0';
    dp = opendir(n ? dir : ".");
    if (!dp) return;
    while ((e = readdir(dp)) != NULL) {
        char **grown;
        if (!has_suffix(e->d_name, star + 1)) continue;
        grown = realloc(names, sizeof *names * (count + 1));
        if (!grown) break;
        names = grown;
        /* _findfirst / _findnext give the bare file name. */
        names[count] = malloc(strlen(e->d_name) + 1);
        if (!names[count]) break;
        strcpy(names[count], e->d_name);
        count++;
    }
    closedir(dp);
    for (i = 0; i + 1 < count; ++i) {   /* name order */
        size_t j;
        for (j = i + 1; j < count; ++j) {
            AoeDirectorScenarioRor a, b;
            snprintf(a.name, sizeof a.name, "%s", names[i]);
            snprintf(b.name, sizeof b.name, "%s", names[j]);
            if (compare_names(&a, &b) > 0) {
                char *t = names[i];
                names[i] = names[j];
                names[j] = t;
            }
        }
    }
    for (i = 0; i < count; ++i) {
        each(user, names[i]);
        free(names[i]);
    }
    free(names);
}

/* The player file is a raw deflate stream (the original's compressed
   file I/O, 0x0046b5b0 with the compress flag), decoded with the
   executable's stream decoder (0x00525b0b, inflate.h). */
static uint8_t *camp_read_pif(void *c, const char *path, uint32_t *size) {
    uint32_t packed_size = 0;
    uint8_t *packed = read_whole(path, &packed_size), *out = NULL;
    size_t used = 0;
    (void)c;
    if (!packed) return NULL;
    if (!aoe_inflate_buffer_ror(packed, packed_size, 16u << 20, &out, &used)) out = NULL;
    free(packed);
    *size = (uint32_t)used;
    return out;
}

typedef struct {
    uint8_t *data;
    size_t size, cap;
} Sink;

static int sink_write(void *c, const uint8_t *data, size_t size) {
    Sink *s = c;
    if (s->size + size > s->cap) {
        size_t cap = s->cap ? s->cap : 1024;
        uint8_t *grown;
        while (cap < s->size + size) cap *= 2;
        grown = realloc(s->data, cap);
        if (!grown) return 1;
        s->data = grown;
        s->cap = cap;
    }
    memcpy(s->data + s->size, data, size);
    s->size += size;
    return 0;
}

/* Written with the game's own compressor (deflate_writer.h). */
static int camp_write_pif(void *c, const char *path, const uint8_t *data, uint32_t size) {
    Sink sink = {NULL, 0, 0};
    AoeDeflateWriterRor *w = aoe_deflate_writer_new_ror(9, sink_write, &sink);
    int ok;
    (void)c;
    if (!w) return 0;
    ok = aoe_deflate_writer_write_ror(w, data, size, 1) == 0;
    aoe_deflate_writer_free_ror(w);
    ok = ok && write_whole(path, sink.data, (uint32_t)sink.size);
    free(sink.data);
    return ok;
}

static void camp_log(void *c, const char *text) { (void)c; (void)text; }

static void save_pif(AoeUiDirectorRor *d) {
    if (d->pif_loaded) aoe_pif_save_ror(&d->pif, d->pif.path, &d->campaign_ops);
}

/* The start-up's player file (0x00416xxx): the registry's "Game File
   Number", else the first free game<N>.nfo, which is then stored. */
static void open_pif(AoeUiDirectorRor *d) {
    char path[700];
    const char *root = d->user_dir;
    int32_t n = aoe_registry_get_ror(&d->registry, "Game File Number", -1);
    if (!root[0]) return;
    if (n < 0) {
        for (n = 0; n < 1000; ++n) {
            FILE *f;
            snprintf(path, sizeof path, "%s/game%d.nfo", root, (int)n);
            f = fopen(path, "rb");
            if (!f) break;
            fclose(f);
        }
        aoe_registry_set_ror(&d->registry, "Game File Number", n);
    }
    snprintf(path, sizeof path, "%s/game%d.nfo", root, (int)n);
    aoe_pif_load_ror(&d->pif, path, &d->campaign_ops);
    d->pif_loaded = 1;
}

static int32_t campaign_players(void *c) { return DIR(c)->pif.count; }
static int32_t player_names(void *c, char ***names, int32_t *chosen) {
    return aoe_pif_player_names_ror(&DIR(c)->pif, names, chosen);
}
static void choose_player(void *c, int32_t id) {
    aoe_pif_select_player_ror(&DIR(c)->pif, id);
    save_pif(DIR(c));
}
static void add_player(void *c, const char *name) {
    aoe_pif_add_player_ror(&DIR(c)->pif, name);
    save_pif(DIR(c));
}
static void remove_player(void *c, int32_t id) {
    aoe_pif_remove_player_ror(&DIR(c)->pif, id);
    save_pif(DIR(c));
}
static int32_t campaign_titles(void *c, char ***names, int32_t *chosen) {
    return aoe_pif_campaign_names_ror(&DIR(c)->pif, names, chosen);
}
static int32_t campaign_scenarios(void *c, char ***names, int32_t *chosen) {
    return aoe_pif_scenario_names_ror(&DIR(c)->pif, names, chosen);
}
static void choose_campaign(void *c, int32_t id) {
    aoe_pif_select_campaign_ror(&DIR(c)->pif, id);
    save_pif(DIR(c));
}
static void choose_scenario(void *c, int32_t line) {
    aoe_pif_select_scenario_ror(&DIR(c)->pif, line);
    save_pif(DIR(c));
}
static const AoeCampaignOpsRor *campaign_file_ops(void *c) { return &DIR(c)->campaign_ops; }
static void refresh_campaigns(void *c) { aoe_pif_find_campaigns_ror(&DIR(c)->pif, &DIR(c)->campaign_ops); }
static int file_writable(void *c, const char *path) {
    FILE *f;
    (void)c;
    f = fopen(path, "ab");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static void open_new_name(void *c, AoeUiPanelRor *parent) {
    AoeUiDirectorRor *d = DIR(c);
    AoeNewNameDialogRor *n = calloc(1, sizeof *n);
    if (!n) return;
    if (!aoe_new_name_dialog_init_ror(n, &d->ui, parent)) free(n);
    else d->opened++;
}

static int32_t registry_get(void *c, const char *name) {
    return aoe_registry_get_ror(&DIR(c)->registry, name, -1);
}
static void registry_set(void *c, const char *name, int32_t value) {
    aoe_registry_set_ror(&DIR(c)->registry, name, value);
}

static int networked(void *c) { (void)c; return 0; }
static void arrow_cursor(void *c) { (void)c; }

static int cd_check(void *c) { (void)c; return 1; }
static void reset_input(void *c) { (void)c; }
static void wait_cursor(void *c) { (void)c; }
static void quit_program(void *c) { DIR(c)->quit = 1; }
static uint32_t now_ms(void *c) {
    AoeUiHostRor *h = DIR(c)->host;
    return h->now_ms ? h->now_ms(h->now_context) : 0;
}

void aoe_ui_director_init_ror(AoeUiDirectorRor *d, AoeUiHostRor *host, const char *user_dir) {
    memset(d, 0, sizeof *d);
    d->host = host;
    d->ops.context = d;
    d->ops.cd_check = cd_check;
    d->ops.reset_input = reset_input;
    d->ops.wait_cursor = wait_cursor;
    d->ops.quit_program = quit_program;
    d->ops.now_ms = now_ms;
    d->ops.open_main_menu = open_main_menu;
    d->ops.open_single_player_menu = open_single_player_menu;
    d->ops.open_editor_menu = open_editor_menu;
    d->ops.open_mp_startup = open_mp_startup;
    d->ops.open_credits = open_credits;
    d->ops.open_select_scenario = open_select_scenario;
    d->ops.scenario_index_refresh = index_refresh;
    d->ops.scenario_index_name = index_name;
    d->ops.scenario_index_info = index_info;
    d->ops.networked = networked;
    d->ops.arrow_cursor = arrow_cursor;
    d->ops.open_name_selection = open_name_selection;
    d->ops.open_new_name = open_new_name;
    d->ops.open_campaign_selection = open_campaign_selection;
    d->ops.campaign_players = campaign_players;
    d->ops.campaign_player_names = player_names;
    d->ops.choose_campaign_player = choose_player;
    d->ops.add_campaign_player = add_player;
    d->ops.remove_campaign_player = remove_player;
    d->ops.campaign_titles = campaign_titles;
    d->ops.campaign_scenarios = campaign_scenarios;
    d->ops.choose_campaign = choose_campaign;
    d->ops.choose_scenario = choose_scenario;
    d->ops.campaign_file_ops = campaign_file_ops;
    d->ops.refresh_campaigns = refresh_campaigns;
    d->ops.file_writable = file_writable;
    {
        char root[600], path[700];
        if (aoe_ui_host_find_ror(host, NULL, NULL, root, sizeof root)) {
            snprintf(d->user_dir, sizeof d->user_dir, "%s", user_dir ? user_dir : root);
            snprintf(path, sizeof path, "%s/registry.txt", d->user_dir);
            aoe_registry_load_ror(&d->registry, path);
            if (aoe_ui_host_find_ror(host, "campaign", NULL, path, sizeof path))
                snprintf(d->campaign_dir, sizeof d->campaign_dir, "%s/", path);
            if (aoe_ui_host_find_ror(host, "scenario", NULL, path, sizeof path))
                snprintf(d->scenario_dir, sizeof d->scenario_dir, "%s/", path);
        }
    }
    /* A new game's settings, then the registry as the start-up applies it
       (0x004166b0): difficulty and game speed into the settings. */
    aoe_settings_defaults_ror(&d->settings);
    {
        static AoeStartupConfigRor config;
        AoeGameStartupRor startup;
        AoeRegistryOpsRor reg = {d, registry_get, registry_set};
        memset(&startup, 0, sizeof startup);
        startup.config = &config;
        startup.settings = &d->settings;
        startup.game_speed = aoe_settings_float_a0c_ror(&d->settings);
        startup.difficulty = aoe_settings_int_a10_ror(&d->settings);
        aoe_game_read_registry_ror(&startup, &reg);
        aoe_settings_set_float_a0c_ror(&d->settings, startup.game_speed);
        aoe_settings_set_int_a10_ror(&d->settings, startup.difficulty);
    }
    d->campaign_ops.context = d;
    d->campaign_ops.campaign_dir = d->campaign_dir;
    d->campaign_ops.scenario_dir = d->scenario_dir;
    d->campaign_ops.read_file = camp_read_file;
    d->campaign_ops.write_file = camp_write_file;
    d->campaign_ops.find = camp_find;
    d->campaign_ops.read_pif = camp_read_pif;
    d->campaign_ops.write_pif = camp_write_pif;
    d->campaign_ops.log = camp_log;
    open_pif(d);
    d->ui.system = &host->system;
    d->ui.ez_ops = &host->ez_ops;
    d->ui.settings = &d->settings;
    d->ui.ops = &d->ops;
    d->ui.picture_ops = &host->picture_ops;
    aoe_ui_director_game_init_ror(d);
}

void aoe_ui_director_release_ror(AoeUiDirectorRor *d) {
    aoe_ui_director_game_release_ror(d);
    free_scenarios(d);
    if (d->pif_loaded) {
        aoe_pif_free_ror(&d->pif, &d->campaign_ops);   /* saves, as 0x0043c810 */
        d->pif_loaded = 0;
    }
}

int aoe_ui_director_start_ror(AoeUiDirectorRor *d) {
    open_main_menu(d);
    return aoe_ui_system_show_named_ror(&d->host->system, "Main Menu", 0);
}

const char *aoe_ui_director_screen_name_ror(const AoeUiDirectorRor *d) {
    const AoeUiPanelRor *s = d->host->system.screen;
    return s && s->name ? s->name : "";
}
