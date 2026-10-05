#include "campaign_progress.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #cond);     \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

typedef struct {
    char path[64];
    uint8_t *data;
    uint32_t size;
} FakeFile;

static FakeFile files[16];
static int file_count;
static int32_t slice_offset, slice_size;
static char slice_path[64];
static int logs;

static FakeFile *find_file(const char *path) {
    int i;
    for (i = 0; i < file_count; ++i)
        if (strcmp(files[i].path, path) == 0) return &files[i];
    return NULL;
}

static int put_file(void *ctx, const char *path, const uint8_t *data,
                    uint32_t size) {
    FakeFile *f = find_file(path);
    (void)ctx;
    if (!f) {
        f = &files[file_count++];
        snprintf(f->path, sizeof f->path, "%s", path);
    }
    free(f->data);
    f->data = malloc(size ? size : 1);
    memcpy(f->data, data, size);
    f->size = size;
    return 1;
}

static uint8_t *get_file(void *ctx, const char *path, uint32_t *size) {
    FakeFile *f = find_file(path);
    uint8_t *d;
    (void)ctx;
    if (!f) return NULL;
    d = malloc(f->size ? f->size : 1);
    memcpy(d, f->data, f->size);
    *size = f->size;
    return d;
}

static int32_t open_slice(void *ctx, const char *path, int32_t offset,
                          int32_t size) {
    (void)ctx;
    snprintf(slice_path, sizeof slice_path, "%s", path);
    slice_offset = offset;
    slice_size = size;
    return 7;
}

static void find(void *ctx, const char *pattern,
                 void (*each)(void *user, const char *name), void *user) {
    const char *ext = strrchr(pattern, '.');
    int i;
    (void)ctx;
    for (i = 0; i < file_count; ++i) {
        const char *base = files[i].path + strlen("camp/");
        if (strncmp(files[i].path, "camp/", 5) != 0) continue;
        if (strcmp(strrchr(base, '.'), ext) == 0) each(user, base);
    }
}

static void log_text(void *ctx, const char *text) {
    (void)ctx;
    (void)text;
    ++logs;
}

static const AoeCampaignOpsRor ops = {
    NULL,     "camp/",  "scn/",  get_file, put_file, open_slice,
    find,     get_file, put_file, log_text,
};

static void free_names(char **names, int32_t n) {
    int32_t i;
    for (i = 0; i < n; ++i) free(names[i]);
    free(names);
}

static void reset_files(void) {
    int i;
    for (i = 0; i < file_count; ++i) {
        free(files[i].data);
        files[i].data = NULL;
    }
    file_count = 0;
}

static void test_campaign_file(void) {
    static const char *const scn_files[3] = {"a.scn", "b.scn", "c.scn"};
    static const char *const scn_names[3] = {"First", "Second", "Third"};
    AoeCampaignRor c, back;
    FakeFile *f;
    char **names = NULL;

    put_file(NULL, "scn/a.scn", (const uint8_t *)"AAAA", 4);
    put_file(NULL, "scn/b.scn", (const uint8_t *)"BBBBBB", 6);
    CHECK(aoe_campaign_create_ror(&c, "x.cpn", "Glory", 3, scn_files,
                                  scn_names, &ops));
    f = find_file("camp/x.cpn");
    CHECK(f != NULL);
    /* Header, three entries, then the two scenarios that exist. */
    CHECK(f->size == 0x108 + 3 * 0x208 + 10);
    CHECK(memcmp(f->data, "1.00Glory", 9) == 0);
    CHECK(f->data[0x104] == 3);
    CHECK(f->data[0x108] == 4 && f->data[0x10c] == 0x20 && f->data[0x10d] == 7);
    CHECK(strcmp((const char *)f->data + 0x108 + 8, "First") == 0);
    CHECK(strcmp((const char *)f->data + 0x108 + 0x107, "a.scn") == 0);
    CHECK(c.entries[1].offset == 0x108 + 3 * 0x208 + 4 &&
          c.entries[1].size == 6);
    CHECK(c.entries[2].offset == 0 && c.entries[2].size == 0);
    CHECK(memcmp(f->data + 0x108 + 3 * 0x208, "AAAABBBBBB", 10) == 0);

    CHECK(aoe_campaign_load_ror(&back, "x.cpn", &ops));
    CHECK(strcmp(aoe_campaign_title_ror(&back), "Glory") == 0);
    CHECK(aoe_campaign_count_ror(&back) == 3);
    CHECK(strcmp(aoe_campaign_name_ror(&back, 2), "Third") == 0);
    CHECK(aoe_campaign_open_ror(&back, 1, &ops) == 7);
    CHECK(strcmp(slice_path, "camp/x.cpn") == 0);
    CHECK(slice_offset == 0x108 + 3 * 0x208 + 4 && slice_size == 6);
    CHECK(aoe_campaign_open_ror(&back, 3, &ops) == -1);
    CHECK(aoe_campaign_open_ror(&back, -1, &ops) == -1);
    CHECK(aoe_campaign_names_ror(&back, &names) == 3);
    CHECK(strcmp(names[1], "Second") == 0);
    free_names(names, 3);
    aoe_campaign_free_ror(&back);
    CHECK(back.entries == NULL);

    CHECK(!aoe_campaign_load_ror(&back, "missing.cpn", &ops));
    CHECK(strcmp(back.title, "FILE NOT FOUND") == 0 && back.count == 0);
    aoe_campaign_free_ror(&c);
}

static AoeCampaignRor make_campaign(const char *title, int32_t count) {
    AoeCampaignRor c;
    int32_t i;
    memset(&c, 0, sizeof c);
    snprintf(c.title, sizeof c.title, "%s", title);
    c.count = count;
    c.entries = calloc((size_t)count, sizeof *c.entries);
    for (i = 0; i < count; ++i)
        snprintf(c.entries[i].name, sizeof c.entries[i].name, "%s %d", title,
                 (int)i);
    snprintf(c.path, sizeof c.path, "camp/%s.cpn", title);
    return c;
}

static void test_progress(void) {
    AoeCampaignRor c = make_campaign("Egypt", 3), *list[1] = {&c};
    AoeCampaignProgressRor p, back;
    AoeOutcomeStream s;
    uint8_t buf[0x200];
    char **names = NULL;
    int32_t cur = 99, n;

    aoe_progress_init_ror(&p, &c);
    CHECK(p.count == 3 && p.states[0] == 2 && p.states[1] == 0);
    CHECK(p.current == 0 && p.furthest == 0);
    CHECK(!aoe_progress_select_ror(&p, 1)); /* locked */
    CHECK(!aoe_progress_select_ror(&p, -1));

    aoe_progress_complete_ror(&p);
    CHECK(p.states[0] == 1 && p.states[1] == 2);
    CHECK(p.furthest == 1 && p.current == 0);
    CHECK(aoe_progress_select_ror(&p, 1) && aoe_progress_current_ror(&p) == 1);
    n = aoe_progress_scenarios_ror(&p, &names, &cur);
    CHECK(n == 2 && cur == 1 && strcmp(names[1], "Egypt 1") == 0);
    free_names(names, n);

    aoe_progress_complete_ror(&p);
    aoe_progress_select_ror(&p, 2);
    aoe_progress_complete_ror(&p);
    /* The last scenario: furthest stays clamped, and it is reopened. */
    CHECK(p.furthest == 2 && p.states[2] == 2 && p.states[1] == 1);

    s.data = buf;
    s.size = sizeof buf;
    s.pos = 0;
    s.failed = 0;
    aoe_progress_save_ror(&p, &s, &ops);
    CHECK(!s.failed && s.pos == 0xff + 12 + 3);
    CHECK(buf[0xff] == 2 && buf[0xff + 4] == 3 && buf[0xff + 8] == 2);

    /* Loading against a campaign that lost a scenario clamps. */
    c.count = 2;
    s.size = s.pos;
    s.pos = 0;
    CHECK(aoe_progress_load_ror(&back, &s, list, 1));
    CHECK(back.campaign == &c && back.count == 2);
    CHECK(back.furthest == 1 && back.current == 1);
    CHECK(back.states[0] == 1 && back.states[1] == 1);
    CHECK(aoe_progress_is_ror(&back, "Egypt"));
    CHECK(aoe_progress_open_ror(&back, &ops) == 7 && slice_size == 0);

    /* Rebinding to a longer campaign grows the states locked. */
    c.count = 3;
    aoe_progress_bind_ror(&back, list, 1);
    CHECK(back.count == 3 && back.states[2] == 0 && back.furthest == 1);
    aoe_progress_bind_ror(&back, list, 0);
    CHECK(back.campaign == NULL);
    n = aoe_progress_scenarios_ror(&back, &names, &cur);
    CHECK(n == -1 && cur == -1);

    aoe_progress_free_ror(&back);
    aoe_progress_free_ror(&p);
    aoe_progress_init_ror(&p, NULL);
    CHECK(p.title[0] == 0 && p.count == 0 && p.current == 0);
    aoe_campaign_free_ror(&c);
}

static void test_pif(void) {
    static const char *const one[1] = {"a.scn"};
    AoeCampaignRor tmp;
    AoePifRor f, g;
    char **names = NULL;
    int32_t cur = 99, n;

    reset_files();
    put_file(NULL, "scn/a.scn", (const uint8_t *)"AAAA", 4);
    CHECK(aoe_campaign_create_ror(&tmp, "r.cpn", "Rome", 1, one, one, &ops));
    aoe_campaign_free_ror(&tmp);
    CHECK(aoe_campaign_create_ror(&tmp, "z.cpx", "Zama", 1, one, one, &ops));
    aoe_campaign_free_ror(&tmp);

    memset(&f, 0, sizeof f);
    CHECK(!aoe_pif_load_ror(&f, "player.nfo", &ops));
    CHECK(f.current == -1 && f.count == 0 && f.campaign_count == 2);
    /* .cpn campaigns come before .cpx ones. */
    CHECK(strcmp(f.campaigns[0]->title, "Rome") == 0);
    CHECK(strcmp(f.campaigns[1]->title, "Zama") == 0);
    CHECK(aoe_pif_player_ror(&f) == NULL);
    CHECK(aoe_pif_scenario_names_ror(&f, &names, &cur) == -1 && cur == -1);

    CHECK(aoe_pif_add_player_ror(&f, "Ann"));
    CHECK(aoe_pif_add_player_ror(&f, "Bob"));
    CHECK(f.current == 1 && strcmp(aoe_pif_player_ror(&f), "Bob") == 0);
    CHECK(aoe_pif_campaign_ror(&f) == -1);
    CHECK(aoe_pif_select_campaign_ror(&f, 1));
    CHECK(aoe_pif_campaign_ror(&f) == 1 && aoe_pif_scenario_ror(&f) == 0);
    CHECK(!aoe_pif_select_campaign_ror(&f, 2));
    CHECK(aoe_pif_select_campaign_ror(&f, 1)); /* existing record */
    CHECK(f.players[1]->count == 1);
    CHECK(aoe_pif_open_ror(&f, &ops) == 7 && slice_size == 4);

    n = aoe_pif_campaign_names_ror(&f, &names, &cur);
    CHECK(n == 2 && cur == 1 && strcmp(names[0], "Rome") == 0);
    free_names(names, n);
    n = aoe_pif_player_names_ror(&f, &names, &cur);
    CHECK(n == 2 && cur == 1 && strcmp(names[0], "Ann") == 0);
    free_names(names, n);

    logs = 0;
    aoe_pif_complete_ror(&f, &ops); /* also saves */
    CHECK(logs == 4); /* file, two players, one campaign record */
    CHECK(find_file("player.nfo") != NULL);
    aoe_pif_free_ror(&f, &ops);

    memset(&g, 0, sizeof g);
    CHECK(aoe_pif_load_ror(&g, "player.nfo", &ops));
    CHECK(g.count == 2 && g.current == 1);
    CHECK(strcmp(g.players[0]->name, "Ann") == 0);
    CHECK(g.players[1]->records[0]->campaign == g.campaigns[1]);
    CHECK(g.players[1]->records[0]->states[0] == 2);
    CHECK(aoe_pif_select_player_ror(&g, 0) && g.current == 0);
    CHECK(!aoe_pif_select_player_ror(&g, 5) && g.current == -1);
    aoe_pif_select_player_ror(&g, 1);
    aoe_pif_remove_player_ror(&g, 1);
    CHECK(g.count == 1 && g.current == 0);
    aoe_pif_remove_player_ror(&g, 3);
    CHECK(g.count == 1);
    g.path[0] = 0; /* no save on free */
    aoe_pif_free_ror(&g, &ops);
    reset_files();
}

int main(void) {
    test_campaign_file();
    test_progress();
    test_pif();
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    puts("campaign_progress_test: ok");
    return 0;
}
