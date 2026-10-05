#include "campaign_progress.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "le_bytes.h"

#define CPN_HEADER 0x108
#define CPN_ENTRY 0x208

static const char aoe_cpn_version[4] = {'1', '.', '0', '0'};

static void copy_text(char *dst, size_t size, const char *src) {
    size_t n = src ? strlen(src) : 0;
    if (n >= size) n = size - 1;
    if (n) memmove(dst, src, n);
    dst[n] = '\0';
}

static char *dup_text(const char *s) {
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    if (d) memcpy(d, s, n);
    return d;
}

static void campaign_log(const AoeCampaignOpsRor *ops, const char *text) {
    if (ops && ops->log) ops->log(ops->context, text);
}

/* ---- campaign ---- */

static void put_entry(uint8_t *out, const AoeCampaignEntryRor *e) {
    aoe_le_put_u32(out, (uint32_t)e->size);
    aoe_le_put_u32(out + 4, (uint32_t)e->offset);
    memcpy(out + 8, e->name, sizeof e->name);
    memcpy(out + 0x107, e->file, sizeof e->file);
}

int aoe_campaign_load_ror(AoeCampaignRor *c, const char *name,
                          const AoeCampaignOpsRor *ops) {
    uint8_t *data = NULL;
    uint32_t size = 0;
    char path[0x200];
    int32_t i, whole;

    snprintf(path, sizeof path, "%s%s", ops->campaign_dir, name);
    copy_text(c->path, sizeof c->path, path);
    if (ops->read_file) data = ops->read_file(ops->context, c->path, &size);
    if (!data) {
        copy_text(c->title, sizeof c->title, "FILE NOT FOUND");
        c->count = 0;
        memset(c->version, 0, sizeof c->version);
        c->entries = NULL;
        return 0;
    }
    /* The header and entry table are read straight into the object; a short
       file leaves the rest zero. */
    memset(c->version, 0, sizeof c->version);
    memset(c->title, 0, sizeof c->title);
    c->count = 0;
    if (size >= 4) memcpy(c->version, data, 4);
    if (size >= 4 + 0x100) memcpy(c->title, data + 4, 0x100);
    else if (size > 4) memcpy(c->title, data + 4, size - 4);
    c->title[sizeof c->title - 1] = '\0';
    if (size >= CPN_HEADER) c->count = aoe_le_i32(data + 0x104);
    c->entries = NULL;
    if (c->count > 0) {
        /* Port guard: no more entries than the file holds (the original
           allocates what the header says). */
        whole = (int32_t)((size - CPN_HEADER) / CPN_ENTRY);
        if (c->count > whole) c->count = whole;
        c->entries = calloc((size_t)c->count, sizeof *c->entries);
        if (!c->entries) c->count = 0;
        for (i = 0; c->entries && i < c->count; ++i) {
            const uint8_t *e = data + CPN_HEADER + (size_t)i * CPN_ENTRY;
            AoeCampaignEntryRor *out = &c->entries[i];
            out->size = aoe_le_i32(e);
            out->offset = aoe_le_i32(e + 4);
            memcpy(out->name, e + 8, sizeof out->name);
            memcpy(out->file, e + 0x107, sizeof out->file);
            out->name[sizeof out->name - 1] = '\0';
            out->file[sizeof out->file - 1] = '\0';
        }
    }
    free(data);
    return 1;
}

int aoe_campaign_create_ror(AoeCampaignRor *c, const char *name,
                            const char *title, int32_t count,
                            const char *const *files,
                            const char *const *names,
                            const AoeCampaignOpsRor *ops) {
    char path[0x200];
    int32_t i;

    snprintf(path, sizeof path, "%s%s", ops->campaign_dir, name);
    copy_text(c->path, sizeof c->path, path);
    memcpy(c->version, aoe_cpn_version, 4);
    copy_text(c->title, sizeof c->title, title);
    c->count = count;
    c->entries = NULL;
    if (count > 0) {
        c->entries = calloc((size_t)count, sizeof *c->entries);
        if (!c->entries) {
            c->count = 0;
            return 0;
        }
        for (i = 0; i < count; ++i) {
            copy_text(c->entries[i].name, sizeof c->entries[i].name,
                      names[i]);
            copy_text(c->entries[i].file, sizeof c->entries[i].file,
                      files[i]);
        }
    }
    return aoe_campaign_write_ror(c, ops);
}

int aoe_campaign_write_ror(AoeCampaignRor *c, const AoeCampaignOpsRor *ops) {
    uint8_t **parts = NULL;
    uint32_t *sizes = NULL;
    uint8_t *out;
    size_t total = CPN_HEADER, pos;
    int32_t i, offset;
    int ok;

    if (c->count > 0) {
        parts = calloc((size_t)c->count, sizeof *parts);
        sizes = calloc((size_t)c->count, sizeof *sizes);
        if (!parts || !sizes) {
            free(parts);
            free(sizes);
            return 0;
        }
        total += (size_t)c->count * CPN_ENTRY;
        /* Each scenario file is appended after the table; a missing one
           keeps its entry's size and offset. */
        offset = (int32_t)total;
        for (i = 0; i < c->count; ++i) {
            char path[0x200];
            snprintf(path, sizeof path, "%s%s", ops->scenario_dir,
                     c->entries[i].file);
            if (ops->read_file)
                parts[i] = ops->read_file(ops->context, path, &sizes[i]);
            if (!parts[i]) continue;
            c->entries[i].offset = offset;
            offset += (int32_t)sizes[i];
            c->entries[i].size = (int32_t)sizes[i];
            total += sizes[i];
        }
    }
    out = calloc(1, total);
    ok = out != NULL;
    if (ok) {
        memcpy(out, c->version, 4);
        memcpy(out + 4, c->title, 0x100);
        aoe_le_put_u32(out + 0x104, (uint32_t)c->count);
        pos = CPN_HEADER;
        for (i = 0; i < c->count; ++i, pos += CPN_ENTRY)
            put_entry(out + pos, &c->entries[i]);
        for (i = 0; i < c->count; ++i) {
            if (!parts[i]) continue;
            if (sizes[i]) memcpy(out + pos, parts[i], sizes[i]);
            pos += sizes[i];
        }
        ok = ops->write_file &&
             ops->write_file(ops->context, c->path, out, (uint32_t)total);
    }
    for (i = 0; parts && i < c->count; ++i) free(parts[i]);
    free(parts);
    free(sizes);
    free(out);
    return ok;
}

void aoe_campaign_free_ror(AoeCampaignRor *c) {
    if (c->entries) {
        free(c->entries);
        c->entries = NULL;
    }
}

int32_t aoe_campaign_open_ror(const AoeCampaignRor *c, int32_t i,
                              const AoeCampaignOpsRor *ops) {
    if (i < 0 || i >= c->count || !ops->open_slice) return -1;
    return ops->open_slice(ops->context, c->path, c->entries[i].offset,
                           c->entries[i].size);
}

const char *aoe_campaign_title_ror(const AoeCampaignRor *c) {
    return c->title;
}

int32_t aoe_campaign_count_ror(const AoeCampaignRor *c) { return c->count; }

int32_t aoe_campaign_names_ror(const AoeCampaignRor *c, char ***names) {
    int32_t i;
    *names = c->count > 0 ? calloc((size_t)c->count, sizeof **names) : NULL;
    for (i = 0; *names && i < c->count; ++i)
        (*names)[i] = dup_text(c->entries[i].name);
    return c->count;
}

const char *aoe_campaign_name_ror(const AoeCampaignRor *c, int32_t i) {
    return c->entries[i].name;
}

/* ---- campaign progress ---- */

static void progress_resize(AoeCampaignProgressRor *p, int32_t count) {
    uint8_t *states = count > 0 ? calloc((size_t)count, 1) : NULL;
    int32_t keep = p->count < count ? p->count : count;
    if (states && keep > 0 && p->states) memcpy(states, p->states, (size_t)keep);
    free(p->states);
    p->states = states;
}

/* The binding shared by load (0x0043bb40) and rebind (0x0043be80): the
   last campaign of the same title wins; the states follow its scenario
   count, and the furthest and chosen scenarios are clamped below it. When
   the campaign gained scenarios, load keeps the shorter buffer (and later
   reads past it); the port grows it with locked scenarios in both. */
static void progress_bind(AoeCampaignProgressRor *p,
                          AoeCampaignRor **campaigns, int32_t n) {
    int32_t i, count;
    p->campaign = NULL;
    for (i = 0; i < n; ++i) {
        if (!campaigns[i] ||
            strcmp(aoe_campaign_title_ror(campaigns[i]), p->title) != 0)
            continue;
        p->campaign = campaigns[i];
        count = aoe_campaign_count_ror(p->campaign);
        if (p->count == count) continue;
        progress_resize(p, count);
        p->count = count;
        if (p->furthest >= count) p->furthest = count - 1;
        if (p->current > p->furthest) p->current = p->furthest;
    }
}

void aoe_progress_init_ror(AoeCampaignProgressRor *p, AoeCampaignRor *c) {
    memset(p, 0, sizeof *p);
    if (!c) return;
    p->campaign = c;
    copy_text(p->title, sizeof p->title, aoe_campaign_title_ror(c));
    p->count = aoe_campaign_count_ror(c);
    if (p->count > 0) {
        p->states = calloc((size_t)p->count, 1);
        if (p->states) p->states[0] = 2;
        return;
    }
    p->current = -1;
    p->furthest = -1;
}

int aoe_progress_load_ror(AoeCampaignProgressRor *p, AoeOutcomeStream *in,
                          AoeCampaignRor **campaigns, int32_t n) {
    const uint8_t *title = aoe_stream_take(in, 0xff);
    memset(p, 0, sizeof *p);
    if (title) memcpy(p->title, title, 0xff);
    p->current = aoe_stream_i32(in);
    p->count = aoe_stream_i32(in);
    p->furthest = aoe_stream_i32(in);
    if (p->count > 0) {
        const uint8_t *states = aoe_stream_take(in, (size_t)p->count);
        if (!states) {
            p->count = 0;
        } else {
            p->states = malloc((size_t)p->count);
            if (p->states) memcpy(p->states, states, (size_t)p->count);
            else p->count = 0;
        }
    }
    progress_bind(p, campaigns, n);
    return !in->failed;
}

void aoe_progress_save_ror(const AoeCampaignProgressRor *p,
                           AoeOutcomeStream *out,
                           const AoeCampaignOpsRor *ops) {
    char text[0x140];
    snprintf(text, sizeof text, "  (PIF)Saving campaign - (%d) %s",
             (int)p->furthest, p->title);
    campaign_log(ops, text);
    aoe_stream_put(out, p->title, 0xff);
    aoe_stream_put_i32(out, p->current);
    aoe_stream_put_i32(out, p->count);
    aoe_stream_put_i32(out, p->furthest);
    if (p->count > 0) aoe_stream_put(out, p->states, (size_t)p->count);
}

void aoe_progress_bind_ror(AoeCampaignProgressRor *p,
                           AoeCampaignRor **campaigns, int32_t n) {
    progress_bind(p, campaigns, n);
}

int aoe_progress_is_ror(const AoeCampaignProgressRor *p, const char *title) {
    return strcmp(p->title, title) == 0;
}

int aoe_progress_select_ror(AoeCampaignProgressRor *p, int32_t i) {
    /* Port guard: the original does not test for a negative index. */
    if (i < 0 || i >= p->count || !p->states[i]) return 0;
    p->current = i > p->furthest ? p->furthest : i;
    return 1;
}

int32_t aoe_progress_current_ror(const AoeCampaignProgressRor *p) {
    return p->current;
}

void aoe_progress_complete_ror(AoeCampaignProgressRor *p) {
    if (p->current < 0) return;
    p->states[p->current] = 1;
    if (p->furthest <= p->current) {
        p->furthest = p->current + 1;
        /* Still locked here, so this choice normally fails. */
        aoe_progress_select_ror(p, p->furthest);
    }
    if (p->furthest >= p->count) p->furthest = p->count - 1;
    p->states[p->furthest] = 2;
}

int32_t aoe_progress_scenarios_ror(const AoeCampaignProgressRor *p,
                                   char ***names, int32_t *current) {
    int32_t i;
    if (!p->campaign) {
        *current = -1;
        return -1;
    }
    *names = calloc((size_t)p->furthest + 1, sizeof **names);
    for (i = 0; *names && i <= p->furthest; ++i)
        (*names)[i] = dup_text(aoe_campaign_name_ror(p->campaign, i));
    *current = p->current;
    return p->furthest + 1;
}

int32_t aoe_progress_open_ror(const AoeCampaignProgressRor *p,
                              const AoeCampaignOpsRor *ops) {
    /* Port guard: an unbound record has no campaign to open. */
    if (p->current < 0 || !p->campaign) return -1;
    return aoe_campaign_open_ror(p->campaign, p->current, ops);
}

void aoe_progress_free_ror(AoeCampaignProgressRor *p) {
    if (p->states) {
        free(p->states);
        p->states = NULL;
    }
}

/* ---- player ---- */

void aoe_pif_player_init_ror(AoePifPlayerRor *p, const char *name,
                             AoeCampaignRor **campaigns, int32_t n) {
    copy_text(p->name, sizeof p->name, name);
    p->count = 0;
    p->records = NULL;
    p->campaigns = campaigns;
    p->current = -1;
    p->campaign_count = n;
}

int aoe_pif_player_load_ror(AoePifPlayerRor *p, AoeOutcomeStream *in,
                            AoeCampaignRor **campaigns, int32_t n) {
    const uint8_t *name;
    int32_t i;
    memset(p, 0, sizeof *p);
    p->campaign_count = n;
    p->campaigns = campaigns;
    name = aoe_stream_take(in, 0xff);
    if (name) memcpy(p->name, name, 0xff);
    p->current = aoe_stream_i32(in);
    p->count = aoe_stream_i32(in);
    if (in->failed) p->count = 0;
    if (p->count < 1) return !in->failed;
    p->records = calloc((size_t)p->count, sizeof *p->records);
    if (!p->records) {
        p->count = 0;
        return 0;
    }
    for (i = 0; i < p->count; ++i) {
        p->records[i] = malloc(sizeof **p->records);
        if (p->records[i])
            aoe_progress_load_ror(p->records[i], in, campaigns, n);
    }
    return !in->failed;
}

void aoe_pif_player_save_ror(const AoePifPlayerRor *p, AoeOutcomeStream *out,
                             const AoeCampaignOpsRor *ops) {
    char text[0x140];
    int32_t i;
    snprintf(text, sizeof text, "(PIF)Saving player - %s", p->name);
    campaign_log(ops, text);
    aoe_stream_put(out, p->name, 0xff);
    aoe_stream_put_i32(out, p->current);
    aoe_stream_put_i32(out, p->count);
    for (i = 0; i < p->count; ++i)
        aoe_progress_save_ror(p->records[i], out, ops);
}

void aoe_pif_player_free_ror(AoePifPlayerRor *p) {
    int32_t i;
    if (!p->records) return;
    for (i = 0; i < p->count; ++i) {
        if (!p->records[i]) continue;
        aoe_progress_free_ror(p->records[i]);
        free(p->records[i]);
    }
    free(p->records);
    p->records = NULL;
    p->count = 0;
}

void aoe_pif_player_rebind_ror(AoePifPlayerRor *p, AoeCampaignRor **campaigns,
                               int32_t n) {
    int32_t i;
    p->campaign_count = n;
    p->campaigns = campaigns;
    for (i = 0; i < p->count; ++i)
        aoe_progress_bind_ror(p->records[i], campaigns, n);
}

const char *aoe_pif_player_name_ror(const AoePifPlayerRor *p) {
    return p->name;
}

int aoe_pif_player_select_campaign_ror(AoePifPlayerRor *p, int32_t i) {
    AoeCampaignProgressRor **records, *record;
    const char *title;
    int32_t r;
    if (i < 0 || i >= p->campaign_count || !p->campaigns[i]) return 0;
    title = aoe_campaign_title_ror(p->campaigns[i]);
    for (r = 0; r < p->count; ++r) {
        if (aoe_progress_is_ror(p->records[r], title)) {
            p->current = r;
            return 1;
        }
    }
    records = calloc((size_t)p->count + 1, sizeof *records);
    record = malloc(sizeof *record);
    if (!records || !record) {
        free(records);
        free(record);
        return 0;
    }
    if (p->count) memcpy(records, p->records, (size_t)p->count * sizeof *records);
    free(p->records);
    p->records = records;
    aoe_progress_init_ror(record, p->campaigns[i]);
    p->records[p->count] = record;
    p->current = p->count;
    ++p->count;
    return 1;
}

int32_t aoe_pif_player_campaign_ror(const AoePifPlayerRor *p) {
    int32_t i;
    if (p->current < 0 || p->current >= p->count) return -1;
    for (i = 0; i < p->campaign_count; ++i)
        if (aoe_progress_is_ror(p->records[p->current],
                                aoe_campaign_title_ror(p->campaigns[i])))
            return i;
    return -1;
}

/* The original tests the campaign list at the record's index; past the
   end of that list the port treats the slot as present. */
static int player_slot_present(const AoePifPlayerRor *p) {
    if (p->current < 0) return 0;
    if (p->current < p->campaign_count && !p->campaigns[p->current]) return 0;
    return p->current < p->count;
}

int32_t aoe_pif_player_scenario_ror(const AoePifPlayerRor *p) {
    if (!player_slot_present(p)) return -1;
    return aoe_progress_current_ror(p->records[p->current]);
}

int aoe_pif_player_select_scenario_ror(AoePifPlayerRor *p, int32_t i) {
    if (!player_slot_present(p)) return 0;
    return aoe_progress_select_ror(p->records[p->current], i);
}

int32_t aoe_pif_player_scenarios_ror(const AoePifPlayerRor *p, char ***names,
                                     int32_t *current) {
    if (p->current < 0) return 0;
    return aoe_progress_scenarios_ror(p->records[p->current], names, current);
}

void aoe_pif_player_complete_ror(AoePifPlayerRor *p) {
    if (p->current >= 0) aoe_progress_complete_ror(p->records[p->current]);
}

int32_t aoe_pif_player_open_ror(const AoePifPlayerRor *p,
                                const AoeCampaignOpsRor *ops) {
    if (p->current < 0) return -1;
    return aoe_progress_open_ror(p->records[p->current], ops);
}

/* ---- player information file ---- */

typedef struct {
    char **names;
    int32_t count;
} NameList;

static void collect_name(void *user, const char *name) {
    NameList *l = user;
    char **names = realloc(l->names, ((size_t)l->count + 1) * sizeof *names);
    if (!names) return;
    l->names = names;
    l->names[l->count] = dup_text(name);
    if (l->names[l->count]) ++l->count;
}

static void free_campaigns(AoePifRor *f) {
    int32_t i;
    if (!f->campaigns) return;
    for (i = 0; i < f->campaign_count; ++i) {
        if (!f->campaigns[i]) continue;
        aoe_campaign_free_ror(f->campaigns[i]);
        free(f->campaigns[i]);
    }
    free(f->campaigns);
    f->campaigns = NULL;
}

void aoe_pif_find_campaigns_ror(AoePifRor *f, const AoeCampaignOpsRor *ops) {
    static const char *const patterns[2] = {"%s*.cpn", "%s*.cpx"};
    NameList list = {NULL, 0};
    char pattern[0x200];
    int32_t i, k;

    free_campaigns(f);
    /* The original counts the matches of both patterns, then lists them
       again to load each one; the port lists each pattern once. */
    for (k = 0; k < 2; ++k) {
        snprintf(pattern, sizeof pattern, patterns[k], ops->campaign_dir);
        if (ops->find) ops->find(ops->context, pattern, collect_name, &list);
    }
    f->campaign_count = list.count;
    if (list.count > 0) {
        f->campaigns = calloc((size_t)list.count, sizeof *f->campaigns);
        for (i = 0; f->campaigns && i < list.count; ++i) {
            f->campaigns[i] = malloc(sizeof **f->campaigns);
            if (f->campaigns[i])
                aoe_campaign_load_ror(f->campaigns[i], list.names[i], ops);
        }
        if (!f->campaigns) f->campaign_count = 0;
    }
    for (i = 0; i < list.count; ++i) free(list.names[i]);
    free(list.names);
    for (i = 0; i < f->count; ++i)
        aoe_pif_player_rebind_ror(f->players[i], f->campaigns,
                                  f->campaign_count);
}

int aoe_pif_load_ror(AoePifRor *f, const char *path,
                     const AoeCampaignOpsRor *ops) {
    AoeOutcomeStream in;
    uint8_t *data = NULL;
    uint32_t size = 0;
    int32_t i;

    f->count = 0;
    f->players = NULL;
    f->campaign_count = 0;
    f->campaigns = NULL;
    aoe_pif_find_campaigns_ror(f, ops);
    copy_text(f->path, sizeof f->path, path);
    if (ops->read_pif) data = ops->read_pif(ops->context, path, &size);
    if (!data || size < 4 || memcmp(data, aoe_cpn_version, 4) != 0) {
        free(data);
        f->current = -1;
        f->count = 0;
        f->players = NULL;
        return 0;
    }
    in.data = data;
    in.size = size;
    in.pos = 4;
    in.failed = 0;
    f->current = aoe_stream_i32(&in);
    f->count = aoe_stream_i32(&in);
    if (f->count < 1) {
        f->players = NULL;
    } else {
        f->players = calloc((size_t)f->count, sizeof *f->players);
        for (i = 0; f->players && i < f->count; ++i) {
            f->players[i] = malloc(sizeof **f->players);
            if (f->players[i])
                aoe_pif_player_load_ror(f->players[i], &in, f->campaigns,
                                        f->campaign_count);
        }
        if (!f->players) f->count = 0;
    }
    free(data);
    return !in.failed;
}

static size_t player_size(const AoePifPlayerRor *p) {
    size_t n = 0xff + 8;
    int32_t i;
    for (i = 0; i < p->count; ++i)
        n += 0xff + 12 +
             (p->records[i]->count > 0 ? (size_t)p->records[i]->count : 0);
    return n;
}

int aoe_pif_save_ror(AoePifRor *f, const char *path,
                     const AoeCampaignOpsRor *ops) {
    AoeOutcomeStream out;
    size_t size = 12;
    int32_t i;
    int ok;

    campaign_log(ops, "(PIF)Creating Player Info File");
    copy_text(f->path, sizeof f->path, path);
    for (i = 0; i < f->count; ++i) size += player_size(f->players[i]);
    out.data = calloc(1, size);
    if (!out.data) return 0;
    out.size = size;
    out.pos = 0;
    out.failed = 0;
    aoe_stream_put(&out, aoe_cpn_version, 4);
    aoe_stream_put_i32(&out, f->current);
    aoe_stream_put_i32(&out, f->count);
    for (i = 0; i < f->count; ++i)
        aoe_pif_player_save_ror(f->players[i], &out, ops);
    ok = !out.failed && ops->write_pif &&
         ops->write_pif(ops->context, f->path, out.data, (uint32_t)out.pos);
    free(out.data);
    return ok;
}

void aoe_pif_free_ror(AoePifRor *f, const AoeCampaignOpsRor *ops) {
    int32_t i;
    if (f->path[0]) aoe_pif_save_ror(f, f->path, ops);
    free_campaigns(f);
    if (!f->players) return;
    for (i = 0; i < f->count; ++i) {
        if (!f->players[i]) continue;
        aoe_pif_player_free_ror(f->players[i]);
        free(f->players[i]);
    }
    free(f->players);
    f->players = NULL;
}

int aoe_pif_add_player_ror(AoePifRor *f, const char *name) {
    AoePifPlayerRor **players = calloc((size_t)f->count + 1, sizeof *players);
    AoePifPlayerRor *player = malloc(sizeof *player);
    if (!players || !player) {
        free(players);
        free(player);
        return 0;
    }
    if (f->count) memcpy(players, f->players, (size_t)f->count * sizeof *players);
    free(f->players);
    f->players = players;
    aoe_pif_player_init_ror(player, name, f->campaigns, f->campaign_count);
    f->players[f->count] = player;
    f->current = f->count;
    ++f->count;
    return 1;
}

int aoe_pif_select_player_ror(AoePifRor *f, int32_t i) {
    if (i < f->count) {
        f->current = i;
        return 1;
    }
    f->current = -1;
    return 0;
}

void aoe_pif_remove_player_ror(AoePifRor *f, int32_t i) {
    int32_t old = f->count;
    if (i < 0 || i >= f->count) return;
    if (f->players[i]) {
        aoe_pif_player_free_ror(f->players[i]);
        free(f->players[i]);
    }
    for (; i < f->count - 1; ++i) f->players[i] = f->players[i + 1];
    f->count = old - 1;
    if (f->current == f->count) f->current = old - 2;
}

static AoePifPlayerRor *chosen(const AoePifRor *f) {
    return f->current >= 0 && f->current < f->count ? f->players[f->current]
                                                     : NULL;
}

int aoe_pif_select_campaign_ror(AoePifRor *f, int32_t i) {
    AoePifPlayerRor *p = chosen(f);
    return p ? aoe_pif_player_select_campaign_ror(p, i) : 0;
}

int aoe_pif_select_scenario_ror(AoePifRor *f, int32_t i) {
    AoePifPlayerRor *p = chosen(f);
    return p ? aoe_pif_player_select_scenario_ror(p, i) : 0;
}

int32_t aoe_pif_player_names_ror(const AoePifRor *f, char ***names,
                                 int32_t *current) {
    int32_t i;
    if (f->count < 1) return f->count;
    *names = calloc((size_t)f->count, sizeof **names);
    for (i = 0; *names && i < f->count; ++i)
        (*names)[i] = dup_text(aoe_pif_player_name_ror(f->players[i]));
    *current = f->current;
    return f->count;
}

int32_t aoe_pif_campaign_names_ror(const AoePifRor *f, char ***names,
                                   int32_t *current) {
    AoePifPlayerRor *p;
    int32_t i;
    if (f->campaign_count > 0) {
        *names = calloc((size_t)f->campaign_count, sizeof **names);
        for (i = 0; *names && i < f->campaign_count; ++i)
            (*names)[i] = dup_text(aoe_campaign_title_ror(f->campaigns[i]));
        p = chosen(f);
        if (p) *current = aoe_pif_player_campaign_ror(p);
    }
    return f->campaign_count;
}

int32_t aoe_pif_scenario_names_ror(const AoePifRor *f, char ***names,
                                   int32_t *current) {
    AoePifPlayerRor *p = chosen(f);
    if (!p) {
        *current = -1;
        return -1;
    }
    *current = aoe_pif_player_scenario_ror(p);
    return aoe_pif_player_scenarios_ror(p, names, current);
}

int32_t aoe_pif_scenario_ror(const AoePifRor *f) {
    AoePifPlayerRor *p = chosen(f);
    return p ? aoe_pif_player_scenario_ror(p) : -1;
}

int32_t aoe_pif_campaign_ror(const AoePifRor *f) {
    AoePifPlayerRor *p = chosen(f);
    return p ? aoe_pif_player_campaign_ror(p) : -1;
}

const char *aoe_pif_player_ror(const AoePifRor *f) {
    /* The original tests only for a negative choice. */
    AoePifPlayerRor *p = chosen(f);
    return p ? aoe_pif_player_name_ror(p) : NULL;
}

void aoe_pif_complete_ror(AoePifRor *f, const AoeCampaignOpsRor *ops) {
    AoePifPlayerRor *p = chosen(f);
    if (p) aoe_pif_player_complete_ror(p);
    aoe_pif_save_ror(f, f->path, ops);
}

int32_t aoe_pif_open_ror(const AoePifRor *f, const AoeCampaignOpsRor *ops) {
    AoePifPlayerRor *p = chosen(f);
    return p ? aoe_pif_player_open_ror(p, ops) : -1;
}
