#include "ai_build.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "le_bytes.h"

static void copy63(char *out, const char *text) {
    size_t n = 0;
    while (text && n < 63 && text[n]) ++n;
    if (n) memcpy(out, text, n);
    out[n] = '\0';
}

/* ---- plan items ---- */

void aoe_ai_plan_item_init_ror(AoeAiPlanItemRor *item) {
    if (!item) return;
    memset(item, 0, sizeof(*item));
    item->type = item->game_id = item->unique_id = -1;
}

void aoe_ai_plan_item_set_ror(AoeAiPlanItemRor *item, int32_t type,
                              int32_t game_id, int32_t unique_id,
                              const char *name, float x, float y, float z,
                              float size_x, float size_y, float size_z) {
    if (!item) return;
    item->type = type;
    item->game_id = game_id;
    item->unique_id = unique_id;
    item->x = x;
    item->y = y;
    item->z = z;
    item->size_x = size_x;
    item->size_y = size_y;
    item->size_z = size_z;
    item->skip = 0;
    copy63(item->name, name);
}

void aoe_ai_plan_item_copy_ror(AoeAiPlanItemRor *item,
                               const AoeAiPlanItemRor *from) {
    if (item && from && item != from) *item = *from;
}

void aoe_ai_plan_item_release_ror(AoeAiPlanItemRor *item) { (void)item; }

const char *aoe_ai_plan_item_name_ror(const AoeAiPlanItemRor *item) {
    return item ? item->name : NULL;
}

float aoe_ai_plan_item_float_ror(const AoeAiPlanItemRor *item, int which) {
    if (!item) return 0.0f;
    switch (which) {
    case AOE_AI_ITEM_X: return item->x;
    case AOE_AI_ITEM_Y: return item->y;
    case AOE_AI_ITEM_Z: return item->z;
    case AOE_AI_ITEM_SIZE_X: return item->size_x;
    case AOE_AI_ITEM_SIZE_Y: return item->size_y;
    case AOE_AI_ITEM_SIZE_Z: return item->size_z;
    default: return 0.0f;
    }
}

void aoe_ai_plan_item_set_position_ror(AoeAiPlanItemRor *item, int which,
                                       float value) {
    if (!item) return;
    if (which == AOE_AI_ITEM_X) item->x = value;
    else if (which == AOE_AI_ITEM_Y) item->y = value;
    else if (which == AOE_AI_ITEM_Z) item->z = value;
}

int32_t aoe_ai_plan_item_skip_ror(const AoeAiPlanItemRor *item) {
    return item ? item->skip : 0;
}

void aoe_ai_plan_item_set_skip_ror(AoeAiPlanItemRor *item, int32_t skip) {
    if (item) item->skip = skip;
}

int32_t aoe_ai_plan_item_type_ror(const AoeAiPlanItemRor *item) {
    return item ? item->type : -1;
}

int32_t aoe_ai_plan_item_game_id_ror(const AoeAiPlanItemRor *item) {
    return item ? item->game_id : -1;
}

int32_t aoe_ai_plan_item_unique_id_ror(const AoeAiPlanItemRor *item) {
    return item ? item->unique_id : -1;
}

void aoe_ai_plan_item_set_game_id_ror(AoeAiPlanItemRor *item, int32_t id) {
    if (item) item->game_id = id;
}

void aoe_ai_plan_item_set_unique_id_ror(AoeAiPlanItemRor *item, int32_t id) {
    if (item) item->unique_id = id;
}

int aoe_ai_plan_item_same_place_ror(const AoeAiPlanItemRor *a,
                                    const AoeAiPlanItemRor *b) {
    return a && b && a->x == b->x && a->y == b->y;
}

/* ---- build items ---- */

void aoe_ai_build_item_init_ror(AoeAiBuildItemRor *item) {
    if (!item) return;
    memset(item, 0, sizeof(*item));
    aoe_ai_plan_item_init_ror(&item->plan);
    item->category = -1;
    item->build_from = -1;
    item->field_90[0] = item->field_90[1] = item->field_90[2] = -1;
    item->cap = -1;
}

void aoe_ai_build_item_set_ror(AoeAiBuildItemRor *item, int32_t category,
                               int32_t type, int32_t unique_id,
                               int32_t field_78, int32_t field_7c,
                               int32_t build_from, const char *name,
                               float size_x, float size_y, float size_z,
                               int32_t field_90, int32_t field_94,
                               int32_t field_98, int32_t field_9c) {
    if (!item) return;
    memset(item, 0, sizeof(*item));
    aoe_ai_plan_item_set_ror(&item->plan, type, -1, unique_id, name, -1.0f,
                             -1.0f, -1.0f, size_x, size_y, size_z);
    item->category = category;
    item->field_78 = field_78;
    item->field_7c = field_7c;
    item->build_from = build_from;
    item->field_90[0] = field_90;
    item->field_90[1] = field_94;
    item->field_90[2] = field_98;
    item->field_9c = field_9c;
    item->cap = -1;
}

void aoe_ai_build_item_release_ror(AoeAiBuildItemRor *item) { (void)item; }

static int32_t *build_field(AoeAiBuildItemRor *item, int field) {
    switch (field) {
    case AOE_AI_BUILD_IN_PROGRESS: return &item->in_progress;
    case AOE_AI_BUILD_BUILT: return &item->built;
    case AOE_AI_BUILD_ATTEMPTS: return &item->build_attempts;
    case AOE_AI_BUILD_FROM: return &item->build_from;
    case AOE_AI_BUILD_FIELD_90: return &item->field_90[0];
    case AOE_AI_BUILD_FIELD_9C: return &item->field_9c;
    case AOE_AI_BUILD_COUNT: return &item->built_count;
    case AOE_AI_BUILD_CAP: return &item->cap;
    case AOE_AI_BUILD_SKIP_CYCLES: return &item->skip_cycles;
    default: return NULL;
    }
}

int32_t aoe_ai_build_item_get_ror(const AoeAiBuildItemRor *item, int field) {
    int32_t *value;
    if (!item) return 0;
    if (field == AOE_AI_BUILD_PERMANENT_SKIP) return item->permanent_skip;
    value = build_field((AoeAiBuildItemRor *)item, field);
    return value ? *value : 0;
}

void aoe_ai_build_item_put_ror(AoeAiBuildItemRor *item, int field,
                               int32_t value) {
    int32_t *slot;
    if (!item) return;
    if (field == AOE_AI_BUILD_PERMANENT_SKIP) {
        item->permanent_skip = (uint8_t)value;
        return;
    }
    if (field == AOE_AI_BUILD_FROM || field == AOE_AI_BUILD_FIELD_90 ||
        field == AOE_AI_BUILD_FIELD_9C) return; /* no original setter */
    if ((slot = build_field(item, field))) *slot = value;
}

int32_t aoe_ai_build_item_field_90_ror(const AoeAiBuildItemRor *item,
                                       int32_t index) {
    if (!item || index <= 0 || index >= 3) return -1;
    return item->field_90[index];
}

void aoe_ai_build_item_add_attempt_ror(AoeAiBuildItemRor *item) {
    if (item) ++item->build_attempts;
}

void aoe_ai_build_item_drop_attempt_ror(AoeAiBuildItemRor *item) {
    if (item && --item->build_attempts < 0) item->build_attempts = 0;
}

void aoe_ai_build_item_add_built_ror(AoeAiBuildItemRor *item) {
    if (item) ++item->built_count;
}

void aoe_ai_build_item_add_skip_cycle_ror(AoeAiBuildItemRor *item) {
    if (item) ++item->skip_cycles;
}

/* ---- Build AI build list ---- */

static void build_link_after(AoeAiBuildItemRor *at, AoeAiBuildItemRor *item) {
    item->prev = at;
    item->next = at->next;
    at->next->prev = item;
    at->next = item;
}

static void build_append(AoeAiBuildListRor *list, AoeAiBuildItemRor *item) {
    build_link_after(list->sentinel.prev, item);
}

void aoe_ai_build_list_init_ror(AoeAiBuildListRor *list, int32_t player,
                                AoeAiLogRor *log) {
    if (!list) return;
    memset(list, 0, sizeof(*list));
    aoe_ai_module_init_ror(&list->module, "Build AI", 0x3e9, player, log);
    aoe_ai_build_item_init_ror(&list->sentinel);
    list->sentinel.next = list->sentinel.prev = &list->sentinel;
    strcpy(list->name_2a5, "None");
    strcpy(list->name_3a6, "None");
    strcpy(list->name_4a7, "None");
}

void aoe_ai_build_list_free_excluded_ror(AoeAiBuildListRor *list) {
    if (!list) return;
    free(list->excluded);
    list->excluded = NULL;
    list->excluded_count = list->field_5b4 = list->excluded_capacity = 0;
}

void aoe_ai_build_list_clear_ror(AoeAiBuildListRor *list) {
    AoeAiBuildItemRor *item;
    if (!list || !list->sentinel.next) return;
    item = list->sentinel.next;
    while (item != &list->sentinel && item) {
        AoeAiBuildItemRor *next = item->next;
        free(item);
        item = next;
    }
    list->sentinel.next = list->sentinel.prev = &list->sentinel;
    list->count = 0;
}

void aoe_ai_build_list_release_ror(AoeAiBuildListRor *list) {
    if (!list) return;
    if (list->count > 0) aoe_ai_build_list_clear_ror(list);
    aoe_ai_build_list_free_excluded_ror(list);
    aoe_ai_module_release_ror(&list->module);
}

int32_t aoe_ai_build_list_count_ror(const AoeAiBuildListRor *list) {
    return list ? list->count : 0;
}

const char *aoe_ai_build_list_file_ror(const AoeAiBuildListRor *list) {
    return list ? list->file : NULL;
}

const char *aoe_ai_build_list_name_ror(const AoeAiBuildListRor *list,
                                       int which) {
    if (!list) return NULL;
    if (which == 0) return list->name_2a5;
    if (which == 1) return list->name_3a6;
    if (which == 2) return list->name_4a7;
    return NULL;
}

int32_t aoe_ai_build_list_field_5a8_ror(const AoeAiBuildListRor *list) {
    return list ? list->field_5a8 : 0;
}

int aoe_ai_build_list_has_file_ror(const AoeAiBuildListRor *list) {
    return list && strcmp(list->file, "NONE") && strcmp(list->file, "NULL");
}

AoeAiBuildItemRor *aoe_ai_build_list_find_ror(const AoeAiBuildListRor *list,
                                              int32_t unique_id) {
    if (!list) return NULL;
    for (AoeAiBuildItemRor *i = list->sentinel.next;
         i && i != &list->sentinel; i = i->next)
        if (i->plan.unique_id == unique_id) return i;
    return NULL;
}

int32_t aoe_ai_build_list_count_of_ror(const AoeAiBuildListRor *list,
                                       int32_t type, int32_t category,
                                       int filter) {
    int32_t n = 0;
    if (!list) return 0;
    for (AoeAiBuildItemRor *i = list->sentinel.next;
         i && i != &list->sentinel; i = i->next) {
        int match;
        if (i->plan.type != type || i->category != category) continue;
        switch (filter) {
        case AOE_AI_COUNT_BUILT: match = i->built == 1; break;
        case AOE_AI_COUNT_NOT_BUILT: match = i->built == 0; break;
        case AOE_AI_COUNT_BUILT_OR_IN_PROGRESS:
            match = i->built == 1 || i->in_progress == 1;
            break;
        case AOE_AI_COUNT_IN_PROGRESS: match = i->in_progress == 1; break;
        default: match = 1; break;
        }
        n += match;
    }
    return n;
}

int aoe_ai_build_list_insert_ror(AoeAiBuildListRor *list,
                                 AoeAiObjectFn objects, void *context,
                                 int32_t type, int32_t copies,
                                 int32_t category, int32_t build_from,
                                 int32_t cap, int32_t position) {
    AoeAiBuildItemRor *at;
    const char *name;
    float sx = 3.0f, sy = 3.0f, rx, ry;
    int32_t unique;
    if (!list || list->count < position) return 0;
    at = list->sentinel.next;
    for (int32_t i = 0; at != &list->sentinel && i < position; ++i)
        at = at->next;
    if (category == 6) {
        name = "SetGatherPercentage";
    } else if (objects && objects(context, type, &name, &rx, &ry)) {
        sx = rx + rx;
        sy = ry + ry;
    } else {
        name = "AddedBuildListItem";
    }
    unique = list->count;
    for (int32_t n = 0; n < copies; ++n) {
        AoeAiBuildItemRor *item = malloc(sizeof(*item));
        if (!item) return 0;
        aoe_ai_build_item_set_ror(item, category, type, unique, 1, 0,
                                  build_from, name, sx, sy, 1.0f, -1, -1, -1,
                                  0);
        item->plan.game_id = -1;
        item->cap = cap;
        build_link_after(at, item);
        ++unique;
        ++list->count;
    }
    return 1;
}

/* fgets(buffer, 0x100, file) over a text buffer. */
static const char *next_line(const char *text, char line[256]) {
    size_t n = 0;
    if (!text || !*text) return NULL;
    while (n < 255 && text[n] && text[n] != '\n') ++n;
    if (n < 255 && text[n] == '\n') ++n;
    memcpy(line, text, n);
    line[n] = '\0';
    return text + n;
}

static int is_comment(const char *line) {
    char a = 0, b = 0;
    return sscanf(line, "%c%c", &a, &b) == 2 && a == '/' && b == '/';
}

int aoe_ai_build_list_load_text_ror(AoeAiBuildListRor *list,
                                    const char *file, const char *text,
                                    AoeAiObjectFn objects, void *context) {
    char line[256], name[64] = "";
    char letter = 0;
    int type = 0, copies = 0, from = 0;
    if (!list || !file) return 0;
    copy63(list->file, file);
    if (!strcmp(list->file, "NONE") || !list->file[0]) {
        aoe_ai_log_ror(&list->module, "There is no build list to load.");
        return 1;
    }
    aoe_ai_log_ror(&list->module, "Trying to load build list in file '%s'.",
                   file);
    if (!text) {
        aoe_ai_log_ror(&list->module, "  I cannot do it cap'n!");
        return 0;
    }
    aoe_ai_log_ror(&list->module,
                   "  File open successful, nuking any old items.");
    if (list->count > 0) aoe_ai_build_list_clear_ror(list);
    list->count = 0;
    while ((text = next_line(text, line))) {
        float sx = 1.5f, sy = 1.5f, rx, ry;
        int cap = -1, category, skip = 0, sized = 0;
        const char *object_name;
        if (is_comment(line)) continue;
        sscanf(line, "%c%d %63s %d %d", &letter, &type, name, &copies, &from);
        switch (letter) {
        case 'A':
            sscanf(line, "%*c%*d %*s %*d %*d %d", &cap);
            /* fall through */
        case 'B':
            category = AOE_AI_BUILD_BUILDING;
            for (int32_t i = 0; i < list->excluded_count &&
                                i < list->excluded_capacity; ++i)
                if (list->excluded[i] == type) skip = 1;
            sized = 1;
            break;
        case 'C': category = AOE_AI_BUILD_C; break;
        case 'E': category = AOE_AI_BUILD_E; break;
        case 'G': category = AOE_AI_BUILD_GATHER; break;
        case 'L': category = AOE_AI_BUILD_L; break;
        case 'R': category = AOE_AI_BUILD_RESEARCH; break;
        case 'T':
            sscanf(line, "%*c%*d %*s %*d %*d %d", &cap);
            /* fall through */
        case 'U':
            category = AOE_AI_BUILD_UNIT;
            sized = 1;
            break;
        default: skip = 1; category = 0; break;
        }
        if (skip) continue;
        if (sized && type >= 0 && objects &&
            objects(context, type, &object_name, &rx, &ry)) {
            sx = rx;
            sy = ry;
        }
        sx += sx;
        sy += sy;
        for (int n = 0; n < copies; ++n) {
            AoeAiBuildItemRor *item = malloc(sizeof(*item));
            if (!item) return 0;
            aoe_ai_build_item_set_ror(item, category, type, list->count, 1, 0,
                                      from, name, sx, sy, 0.0f, -1, -1, -1,
                                      0);
            ++list->count;
            build_append(list, item);
            item->cap = cap;
        }
    }
    return 1;
}

void aoe_ai_build_list_describe_ror(const AoeAiBuildListRor *list) {
    if (!list) return;
    aoe_ai_log_ror(&list->module, "Build list '%s':", list->file);
    for (const AoeAiBuildItemRor *i = list->sentinel.next;
         i && i != &list->sentinel; i = i->next) {
        aoe_ai_log_ror(&list->module,
                       "%sType=%-4d %-20s gID=%-4d Cat=%d uID=%-3d inProg=%d "
                       "bldAtt=%-2d skip=%d #blds=%d cap=%d bldFrom=%d "
                       "permSkip=%d skipCy=%d.",
                       i->built == 1 ? "BLT " : "    ", (int)i->plan.type,
                       i->plan.name, (int)i->plan.game_id, (int)i->category,
                       (int)i->plan.unique_id, (int)i->in_progress,
                       (int)i->build_attempts, (int)i->plan.skip,
                       (int)i->built_count, (int)i->cap,
                       (int)i->build_from, (int)i->permanent_skip,
                       (int)i->skip_cycles);
    }
}

/* ---- streams ---- */

static void put(AoeOutcomeStream *s, const void *bytes, size_t n) {
    if (s->failed || s->pos > s->size || s->size - s->pos < n) {
        s->failed = 1;
        return;
    }
    memcpy(s->data + s->pos, bytes, n);
    s->pos += n;
}

static void put_i32(AoeOutcomeStream *s, int32_t v) {
    uint8_t b[4];
    aoe_le_put_u32(b, (uint32_t)v);
    put(s, b, 4);
}

static void put_f32(AoeOutcomeStream *s, float v) {
    uint8_t b[4];
    aoe_le_put_f32(b, v);
    put(s, b, 4);
}

static void put_text(AoeOutcomeStream *s, const char *text, size_t max) {
    size_t n = 0;
    while (n < max && text[n]) ++n;
    put_i32(s, (int32_t)n);
    put(s, text, n);
}

static const uint8_t *take(AoeOutcomeStream *s, size_t n) {
    const uint8_t *p;
    if (s->failed || s->pos > s->size || s->size - s->pos < n) {
        s->failed = 1;
        return NULL;
    }
    p = s->data + s->pos;
    s->pos += n;
    return p;
}

static int32_t get_i32(AoeOutcomeStream *s) {
    const uint8_t *p = take(s, 4);
    return p ? aoe_le_i32(p) : 0;
}

static float get_f32(AoeOutcomeStream *s) {
    const uint8_t *p = take(s, 4);
    return p ? aoe_le_f32(p) : 0.0f;
}

/* Length-prefixed text into `out` (capacity `size`); the original reads
   the full length into its buffer. Longer text fails the stream. */
static void get_text(AoeOutcomeStream *s, char *out, size_t size) {
    int32_t n = get_i32(s);
    const uint8_t *p;
    if (n < 0 || (size_t)n >= size) {
        s->failed = 1;
        out[0] = '\0';
        return;
    }
    p = take(s, (size_t)n);
    if (!p) {
        out[0] = '\0';
        return;
    }
    memcpy(out, p, (size_t)n);
    out[n] = '\0';
}

static void put_plan_head(AoeOutcomeStream *s, const AoeAiPlanItemRor *p) {
    put_text(s, p->name, sizeof(p->name));
    put_i32(s, p->type);
    put_i32(s, p->game_id);
    put_i32(s, p->unique_id);
    put_f32(s, p->x);
    put_f32(s, p->y);
    put_f32(s, p->z);
    put_f32(s, p->size_x);
    put_f32(s, p->size_y);
    put_f32(s, p->size_z);
    put_i32(s, p->skip);
}

int aoe_ai_build_list_save_ror(const AoeAiBuildListRor *list,
                               AoeOutcomeStream *out) {
    if (!list || !out || !out->data) return 0;
    put_i32(out, list->count);
    put_text(out, list->file, sizeof(list->file));
    put_text(out, list->name_2a5, sizeof(list->name_2a5));
    put_text(out, list->name_3a6, sizeof(list->name_3a6));
    put_text(out, list->name_4a7, sizeof(list->name_4a7));
    put_i32(out, list->field_5a8);
    for (const AoeAiBuildItemRor *i = list->sentinel.next;
         i && i != &list->sentinel; i = i->next) {
        uint8_t skip = i->permanent_skip;
        put_plan_head(out, &i->plan);
        put_i32(out, i->category);
        put_i32(out, i->in_progress);
        put_i32(out, i->built);
        put_i32(out, i->build_attempts);
        put_i32(out, i->build_from);
        put_i32(out, i->field_90[0]);
        put_i32(out, aoe_ai_build_item_field_90_ror(i, 1));
        put_i32(out, aoe_ai_build_item_field_90_ror(i, 2));
        put_i32(out, i->field_9c);
        put_i32(out, i->built_count);
        put_i32(out, i->cap);
        put_i32(out, i->skip_cycles);
        put(out, &skip, 1);
    }
    return !out->failed;
}

int aoe_ai_build_list_load_ror(AoeAiBuildListRor *list, int32_t player,
                               AoeAiLogRor *log, AoeOutcomeStream *in,
                               float version) {
    if (!list || !in || !in->data) return 0;
    aoe_ai_build_list_init_ror(list, player, log);
    list->count = get_i32(in);
    get_text(in, list->file, sizeof(list->file));
    get_text(in, list->name_2a5, sizeof(list->name_2a5));
    get_text(in, list->name_3a6, sizeof(list->name_3a6));
    if (version < 3.1f) strcpy(list->name_4a7, "None");
    else get_text(in, list->name_4a7, sizeof(list->name_4a7));
    list->field_5a8 = get_i32(in);
    for (int32_t n = 0; n < list->count && !in->failed; ++n) {
        AoeAiBuildItemRor *item;
        char name[260];
        int32_t type, game_id, unique, skip, category, in_progress, built,
            attempts, from, f90, f94, f98, f9c, count, cap = -1, cycles;
        float x, y, z, sx, sy, sz;
        const uint8_t *perm;
        get_text(in, name, sizeof(name));
        type = get_i32(in);
        game_id = get_i32(in);
        unique = get_i32(in);
        x = get_f32(in);
        y = get_f32(in);
        z = get_f32(in);
        sx = get_f32(in);
        sy = get_f32(in);
        sz = get_f32(in);
        skip = get_i32(in);
        category = get_i32(in);
        in_progress = get_i32(in);
        built = get_i32(in);
        attempts = get_i32(in);
        from = get_i32(in);
        f90 = get_i32(in);
        f94 = get_i32(in);
        f98 = get_i32(in);
        f9c = get_i32(in);
        count = get_i32(in);
        if (version >= 2.2f) cap = get_i32(in);
        cycles = get_i32(in);
        perm = take(in, 1);
        if (in->failed) break;
        if (!(item = malloc(sizeof(*item)))) {
            in->failed = 1;
            break;
        }
        aoe_ai_build_item_set_ror(item, category, type, unique, 1, 0, from,
                                  name, sx, sy, sz, f90, f94, f98, f9c);
        item->built = built;
        item->in_progress = in_progress;
        item->build_attempts = attempts;
        item->plan.skip = skip;
        item->plan.game_id = game_id;
        item->plan.x = x;
        item->plan.y = y;
        item->plan.z = z;
        item->built_count = count;
        item->cap = cap;
        item->skip_cycles = cycles;
        item->permanent_skip = *perm;
        build_append(list, item);
    }
    if (in->failed) {
        aoe_ai_build_list_clear_ror(list);
        return 0;
    }
    return 1;
}

/* ---- ConAI construction items ---- */

void aoe_ai_con_item_init_ror(AoeAiConItemRor *item) {
    if (!item) return;
    memset(item, 0, sizeof(*item));
    aoe_ai_plan_item_init_ror(&item->plan);
}

void aoe_ai_con_item_set_ror(AoeAiConItemRor *item, float x, float y,
                             float z, float size_x, float size_y,
                             float size_z, int32_t type, const char *name) {
    if (!item) return;
    memset(item, 0, sizeof(*item));
    aoe_ai_plan_item_set_ror(&item->plan, type, -1, -1, name, x, y, z,
                             size_x, size_y, size_z);
}

void aoe_ai_con_item_copy_ror(AoeAiConItemRor *item,
                              const AoeAiConItemRor *from) {
    if (!item || !from || item == from) return;
    item->plan = from->plan;
    item->next = item->prev = NULL;
    item->field_74 = from->field_74;
    item->field_78 = from->field_78;
    item->counter = from->counter;
}

void aoe_ai_con_item_release_ror(AoeAiConItemRor *item) { (void)item; }

void aoe_ai_con_item_set_74_ror(AoeAiConItemRor *item, int32_t value) {
    if (item) item->field_74 = value;
}

void aoe_ai_con_item_set_78_ror(AoeAiConItemRor *item, int32_t value) {
    if (item) item->field_78 = value;
}

int32_t aoe_ai_con_item_78_ror(const AoeAiConItemRor *item) {
    return item ? item->field_78 : 0;
}

int32_t aoe_ai_con_item_counter_ror(const AoeAiConItemRor *item) {
    return item ? item->counter : 0;
}

void aoe_ai_con_item_set_counter_ror(AoeAiConItemRor *item, int32_t value) {
    if (item) item->counter = value;
}

void aoe_ai_con_item_add_ror(AoeAiConItemRor *item) {
    if (item) ++item->counter;
}

void aoe_ai_con_item_drop_ror(AoeAiConItemRor *item) {
    if (item && --item->counter < 0) item->counter = 0;
}

/* ---- ConAI construction plan ---- */

static void con_append(AoeAiConItemRor *sentinel, AoeAiConItemRor *item) {
    item->next = sentinel;
    item->prev = sentinel->prev;
    sentinel->prev->next = item;
    sentinel->prev = item;
}

static void con_free_all(AoeAiConItemRor *sentinel) {
    AoeAiConItemRor *item = sentinel->next;
    while (item && item != sentinel) {
        AoeAiConItemRor *next = item->next;
        free(item);
        item = next;
    }
    sentinel->next = sentinel->prev = sentinel;
}

void aoe_ai_con_plan_init_ror(AoeAiConPlanRor *plan, int32_t player,
                              AoeAiLogRor *log) {
    if (!plan) return;
    memset(plan, 0, sizeof(*plan));
    aoe_ai_module_init_ror(&plan->module, "ConAI", 0x3ea, player, log);
    aoe_ai_con_item_init_ror(&plan->plan);
    aoe_ai_con_item_init_ror(&plan->placed);
    plan->plan.next = plan->plan.prev = &plan->plan;
    plan->placed.next = plan->placed.prev = &plan->placed;
    plan->origin_x = plan->origin_y = plan->origin_z = -1.0f;
    plan->map_width = plan->map_height = -1;
}

void aoe_ai_con_plan_clear_ror(AoeAiConPlanRor *plan) {
    if (!plan) return;
    con_free_all(&plan->plan);
    plan->count = 0;
}

void aoe_ai_con_plan_release_ror(AoeAiConPlanRor *plan) {
    if (!plan) return;
    if (plan->count > 0) aoe_ai_con_plan_clear_ror(plan);
    /* The original never frees the placed list here. */
    con_free_all(&plan->placed);
    plan->placed_count = 0;
    aoe_ai_module_release_ror(&plan->module);
}

const char *aoe_ai_con_plan_file_ror(const AoeAiConPlanRor *plan) {
    return plan ? plan->file : NULL;
}

void aoe_ai_con_plan_place_ror(AoeAiConPlanRor *plan, int32_t width,
                               int32_t height, float x, float y, float z) {
    AoeAiConItemRor *item;
    if (!plan) return;
    plan->map_width = width;
    plan->origin_y = y;
    plan->map_height = height;
    plan->origin_x = x;
    plan->origin_z = z;
    if (x <= 0.0f || y <= 0.0f || (float)width <= x || (float)height <= y) {
        aoe_ai_con_plan_clear_ror(plan);
        return;
    }
    item = plan->plan.next;
    while (item && item != &plan->plan) {
        AoeAiConItemRor *next = item->next;
        double half_x, half_y;
        item->plan.x = item->plan.x + x;
        item->plan.y = item->plan.y + y;
        item->plan.z = item->plan.z + z;
        half_x = (double)(float)(item->plan.size_x * 0.5);
        half_y = (double)(float)(item->plan.size_y * 0.5);
        if ((double)item->plan.x - half_x < 0.0 ||
            (double)item->plan.y - half_y < 0.0 ||
            (double)(width - 1) < (double)item->plan.x + half_x ||
            (double)(height - 1) < (double)item->plan.y + half_y) {
            --plan->count;
            item->prev->next = item->next;
            item->next->prev = item->prev;
            free(item);
        }
        item = next;
    }
}

static AoeAiConItemRor *find_at(AoeAiConItemRor *sentinel, float x,
                                float y) {
    for (AoeAiConItemRor *i = sentinel->next; i && i != sentinel; i = i->next)
        if (i->plan.x == x && i->plan.y == y) return i;
    return NULL;
}

AoeAiConItemRor *aoe_ai_con_plan_at_ror(AoeAiConPlanRor *plan, float x,
                                        float y) {
    AoeAiConItemRor *item;
    if (!plan) return NULL;
    if ((item = find_at(&plan->plan, x, y))) return item;
    return find_at(&plan->placed, x, y);
}

AoeAiConItemRor *aoe_ai_con_plan_placed_at_ror(AoeAiConPlanRor *plan,
                                               float x, float y) {
    return plan ? find_at(&plan->placed, x, y) : NULL;
}

void aoe_ai_con_plan_mark_ror(AoeAiConPlanRor *plan,
                              const AoeAiPlanItemRor *like, int32_t value) {
    if (!plan) return;
    for (AoeAiConItemRor *i = plan->plan.next; i && i != &plan->plan;
         i = i->next) {
        if (aoe_ai_plan_item_same_place_ror(&i->plan, like)) {
            i->field_78 = value;
            return;
        }
    }
}

static int32_t release(AoeAiConItemRor *item, int drop) {
    int32_t unique = item->plan.unique_id;
    item->field_74 = 0;
    item->field_78 = 0;
    item->plan.unique_id = -1;
    if (drop == 1) aoe_ai_con_item_drop_ror(item);
    return unique;
}

int32_t aoe_ai_con_plan_release_item_ror(AoeAiConPlanRor *plan,
                                         const AoeAiPlanItemRor *like,
                                         int drop) {
    if (!plan) return -1;
    for (AoeAiConItemRor *i = plan->plan.next; i && i != &plan->plan;
         i = i->next)
        if (aoe_ai_plan_item_same_place_ror(&i->plan, like))
            return release(i, drop);
    return -1;
}

int32_t aoe_ai_con_plan_release_at_ror(AoeAiConPlanRor *plan, float x,
                                       float y, int drop) {
    AoeAiConItemRor *item = plan ? find_at(&plan->plan, x, y) : NULL;
    return item ? release(item, drop) : -1;
}

void aoe_ai_con_plan_drop_ror(AoeAiConPlanRor *plan, float x, float y,
                              int32_t times) {
    AoeAiConItemRor *item = aoe_ai_con_plan_at_ror(plan, x, y);
    for (int32_t n = 0; item && n < times; ++n) aoe_ai_con_item_drop_ror(item);
}

void aoe_ai_con_plan_add_ror(AoeAiConPlanRor *plan, float x, float y,
                             int32_t times) {
    AoeAiConItemRor *item = aoe_ai_con_plan_at_ror(plan, x, y);
    for (int32_t n = 0; item && n < times; ++n) aoe_ai_con_item_add_ror(item);
}

int aoe_ai_con_plan_load_text_ror(AoeAiConPlanRor *plan, const char *file,
                                  const char *text, int32_t width,
                                  int32_t height, float x, float y,
                                  float z) {
    char line[256], name[64] = "";
    float px = 0, py = 0, pz = 0, sx = 0, sy = 0, sz = 0;
    int type = 0;
    if (!plan || !file) return 0;
    aoe_ai_con_plan_place_ror(plan, width, height, x, y, z);
    if (plan->count > 0) aoe_ai_con_plan_clear_ror(plan);
    copy63(plan->file, file);
    if (!strcmp(plan->file, "NONE")) {
        aoe_ai_log_ror(&plan->module,
                       "There is no construction plan to load.");
        return 1;
    }
    aoe_ai_log_ror(&plan->module, "Loading construction plan in file '%s'.",
                   file);
    if (!text) return 0;
    while ((text = next_line(text, line))) {
        AoeAiConItemRor *item;
        if (is_comment(line)) continue;
        sscanf(line, "%f %f %f %f %f %f %d %63s", &px, &py, &pz, &sx, &sy,
               &sz, &type, name);
        px += plan->origin_x;
        py += plan->origin_y;
        pz += plan->origin_z;
        if (aoe_ai_con_plan_at_ror(plan, px, py)) continue;
        if (!(item = malloc(sizeof(*item)))) return 0;
        aoe_ai_con_item_set_ror(item, px, py, pz, sx, sy, sz, type, name);
        con_append(&plan->plan, item);
        ++plan->count;
    }
    return 1;
}

static void put_con_items(AoeOutcomeStream *out,
                          const AoeAiConItemRor *sentinel) {
    for (const AoeAiConItemRor *i = sentinel->next; i && i != sentinel;
         i = i->next) {
        put_plan_head(out, &i->plan);
        put_i32(out, i->field_74);
        put_i32(out, i->field_78);
        put_i32(out, i->counter);
    }
}

int aoe_ai_con_plan_save_ror(const AoeAiConPlanRor *plan,
                             AoeOutcomeStream *out) {
    if (!plan || !out || !out->data) return 0;
    put_i32(out, plan->count);
    put_text(out, plan->file, sizeof(plan->file));
    put_f32(out, plan->origin_x);
    put_f32(out, plan->origin_y);
    put_f32(out, plan->origin_z);
    put_i32(out, plan->map_width);
    put_i32(out, plan->map_height);
    put_con_items(out, &plan->plan);
    put_i32(out, plan->placed_count);
    put_con_items(out, &plan->placed);
    return !out->failed;
}

static int get_con_items(AoeOutcomeStream *in, AoeAiConItemRor *sentinel,
                         int32_t count) {
    for (int32_t n = 0; n < count && !in->failed; ++n) {
        AoeAiConItemRor *item;
        char name[260];
        int32_t type, game_id, unique, skip, f74, f78, counter;
        float x, y, z, sx, sy, sz;
        get_text(in, name, sizeof(name));
        type = get_i32(in);
        game_id = get_i32(in);
        unique = get_i32(in);
        x = get_f32(in);
        y = get_f32(in);
        z = get_f32(in);
        sx = get_f32(in);
        sy = get_f32(in);
        sz = get_f32(in);
        skip = get_i32(in);
        f74 = get_i32(in);
        f78 = get_i32(in);
        counter = get_i32(in);
        if (in->failed) break;
        if (!(item = malloc(sizeof(*item)))) return 0;
        aoe_ai_con_item_set_ror(item, x, y, z, sx, sy, sz, type, name);
        item->field_74 = f74;
        item->field_78 = f78;
        item->counter = counter;
        item->plan.game_id = game_id;
        item->plan.unique_id = unique;
        item->plan.skip = skip;
        con_append(sentinel, item);
    }
    return !in->failed;
}

int aoe_ai_con_plan_load_ror(AoeAiConPlanRor *plan, int32_t player,
                             AoeAiLogRor *log, AoeOutcomeStream *in) {
    if (!plan || !in || !in->data) return 0;
    aoe_ai_con_plan_init_ror(plan, player, log);
    plan->count = get_i32(in);
    get_text(in, plan->file, sizeof(plan->file));
    plan->origin_x = get_f32(in);
    plan->origin_y = get_f32(in);
    plan->origin_z = get_f32(in);
    plan->map_width = get_i32(in);
    plan->map_height = get_i32(in);
    if (!get_con_items(in, &plan->plan, plan->count)) goto fail;
    plan->placed_count = get_i32(in);
    if (!get_con_items(in, &plan->placed, plan->placed_count)) goto fail;
    return 1;
fail:
    con_free_all(&plan->plan);
    con_free_all(&plan->placed);
    plan->count = plan->placed_count = 0;
    return 0;
}

int aoe_ai_build_list_add_research_ror(AoeAiBuildListRor *list, AoeAiObjectFn objects,
                                       void *context, int32_t type, int32_t position) {
    if (type != 5 && (type < 7 || type > 8)) return 0;
    if (position == -1) {
        const AoeAiBuildItemRor *i = list->sentinel.next;
        position = 0;
        while (i != &list->sentinel && i && i->built == 1) {
            i = i->next;
            ++position;
        }
    }
    return aoe_ai_build_list_insert_ror(list, objects, context, type, 1, 1, 0x2d, 1, position);
}

int aoe_ai_build_list_add_gather_ror(AoeAiBuildListRor *list, AoeAiObjectFn objects,
                                     void *context, int32_t type, int32_t build_from,
                                     int32_t position) {
    return aoe_ai_build_list_insert_ror(list, objects, context, type, 1, 6, build_from, 1, position);
}

int32_t aoe_ai_build_list_count_ror2(const AoeAiBuildListRor *list, int32_t type,
                                     int32_t category,
                                     int32_t (*extra)(void *context, int32_t type),
                                     void *context) {
    if (category == 0 && (type == 0x67 || type == 0x44))
        return aoe_ai_build_list_count_of_ror(list, type, 0, AOE_AI_COUNT_IN_PROGRESS) +
               extra(context, type);
    return aoe_ai_build_list_count_of_ror(list, type, category, AOE_AI_COUNT_BUILT_OR_IN_PROGRESS);
}

int aoe_ai_build_list_all_started_ror(const AoeAiBuildListRor *list, int32_t category_a,
                                      int32_t category_b) {
    const AoeAiBuildItemRor *i;
    for (i = list->sentinel.next; i && i != &list->sentinel; i = i->next)
        if ((i->category == category_a || i->category == category_b) && i->built == 0 &&
            i->in_progress == 0)
            return 0;
    return 1;
}

AoeAiBuildItemRor *aoe_ai_build_list_pending_ror(const AoeAiBuildListRor *list, int32_t type,
                                                 int32_t category_a, int32_t category_b) {
    AoeAiBuildItemRor *i;
    for (i = list->sentinel.next; i && i != &list->sentinel; i = i->next)
        if (i->built == 0 && i->in_progress == 0 && i->plan.type == type &&
            (i->category == category_a || i->category == category_b))
            return i;
    return NULL;
}

void aoe_ai_build_list_retry_ror(AoeAiBuildListRor *list,
                                 int32_t (*count)(void *context, int32_t type, int32_t category),
                                 void *context) {
    int32_t c67 = count(context, 0x67, 0), c44 = count(context, 0x44, 0);
    AoeAiBuildItemRor *i;
    for (i = list->sentinel.next; i && i != &list->sentinel; i = i->next) {
        if (i->built == 1 || i->in_progress == 1) continue;
        if (c67 >= 1 && i->plan.type == 0x67) continue;
        if (c44 >= 1 && i->plan.type == 0x44) continue;
        aoe_ai_plan_item_set_skip_ror(&i->plan, 0);
        i->build_attempts = 0;
        i->skip_cycles = 0;
        i->permanent_skip = 0;
    }
}

void aoe_ai_ror_build_list_init_ror(AoeAiRorBuildListRor *list, int32_t player, AoeAiLogRor *log) {
    aoe_ai_build_list_init_ror(&list->base, player, log);
    list->owner = NULL;
}

int aoe_ai_ror_build_list_load_ror(AoeAiRorBuildListRor *list, int32_t player, AoeAiLogRor *log,
                                   AoeOutcomeStream *in, float version) {
    list->owner = NULL;
    return aoe_ai_build_list_load_ror(&list->base, player, log, in, version);
}

void aoe_ai_ror_build_list_release_ror(AoeAiRorBuildListRor *list) {
    aoe_ai_build_list_release_ror(&list->base);
}

void aoe_ai_ror_build_list_set_owner_ror(AoeAiRorBuildListRor *list, void *owner) {
    list->owner = owner;
}

int aoe_ai_build_list_stop_ror(AoeAiBuildListRor *list, int32_t unique_id) {
    AoeAiBuildItemRor *item = aoe_ai_build_list_find_ror(list, unique_id);
    if (item) item->in_progress = 0;
    return 1;
}

static void exclude(AoeAiBuildListRor *l, int32_t type) {
    int32_t i;
    for (i = 0; i < l->excluded_count && i < l->excluded_capacity; ++i)
        if (l->excluded[i] == type) return;
    if (l->excluded_capacity - 1 < l->excluded_count) {
        int32_t n = l->excluded_count + 1;
        int32_t *grown = malloc((size_t)n * sizeof *grown);
        if (grown) {
            for (i = 0; i < l->excluded_capacity && i < n; ++i) grown[i] = l->excluded[i];
            free(l->excluded);
            l->excluded = grown;
            l->excluded_capacity = n;
        }
    }
    /* Host guard: the original writes past a failed growth. */
    if (l->excluded_count < l->excluded_capacity) l->excluded[l->excluded_count++] = type;
}

int aoe_ai_ror_build_list_load_ror2(AoeAiRorBuildListRor *list, const char *file,
                                    const char *text, AoeAiObjectFn objects, void *context) {
    exclude(&list->base, 0x46);
    exclude(&list->base, 0x67);
    exclude(&list->base, 0x44);
    return aoe_ai_build_list_load_text_ror(&list->base, file, text, objects, context);
}

#define EACH_ITEM(l, it)                                                                   \
    for (AoeAiBuildItemRor *it = (l)->base.sentinel.next; it && it != &(l)->base.sentinel; \
         it = it->next)

static void finish(const AoeAiRorBuildOpsRor *ops) {
    if (ops && ops->update) ops->update(ops->context);
}

static void mark_built(AoeAiBuildItemRor *it, int32_t game_id, const AoeAiRorBuildOpsRor *ops) {
    it->plan.game_id = game_id;
    it->in_progress = 0;
    it->built = 1;
    it->built_count++;
    if (it->plan.type == 0x114 && ops && ops->owner_flag_122b4) *ops->owner_flag_122b4 = 1;
}

int aoe_ai_ror_build_done_ror(AoeAiRorBuildListRor *list, const AoeAiRorBuildOpsRor *ops,
                              int32_t arg, int32_t unique_id, int set_game_id, int32_t game_id) {
    AoeAiBuildItemRor *it = aoe_ai_build_list_find_ror(&list->base, unique_id);
    if (it) {
        it->in_progress = 0;
        it->built = 1;
        if (set_game_id) it->plan.game_id = game_id;
        it->built_count++;
    }
    if (ops && ops->player_hook) ops->player_hook(ops->context, arg);
    finish(ops);
    return 1;
}

int aoe_ai_ror_build_lost_ror(AoeAiRorBuildListRor *list, const AoeAiRorBuildOpsRor *ops,
                              int32_t game_id) {
    EACH_ITEM(list, it) {
        if (it->plan.game_id == game_id && it->built == 1) {
            it->in_progress = 0;
            it->built = 0;
            it->plan.game_id = -1;
            it->build_attempts = 0;
            if (it->plan.type == 0x114 && ops && ops->owner_flag_122b4) *ops->owner_flag_122b4 = 0;
            break;
        }
    }
    finish(ops);
    return 1;
}

int aoe_ai_ror_build_placed_ror(AoeAiRorBuildListRor *list, const AoeAiRorBuildOpsRor *ops,
                                int32_t object_type, int32_t object_id, int32_t unique_id) {
    EACH_ITEM(list, it) {
        if (it->plan.type == object_type && !it->built && unique_id != -1 &&
            it->plan.unique_id == unique_id) {
            mark_built(it, object_id, ops);
            snprintf(list->base.name_2a5, 64, "%s", it->plan.name);
            list->base.field_5a8 = it->plan.unique_id;
            break;
        }
    }
    finish(ops);
    return 1;
}

int aoe_ai_ror_build_object_ror(AoeAiRorBuildListRor *list, const AoeAiRorBuildOpsRor *ops,
                                int32_t object_type, int32_t object_id, int quiet) {
    if (ops && ops->key_building && ops->key_building(ops->context, (int16_t)object_type) &&
        ops->report_key)
        ops->report_key(ops->context, object_id);
    if (quiet) return 0;
    EACH_ITEM(list, it) {
        if (it->plan.type == object_type && !it->built) {
            mark_built(it, object_id, ops);
            return 1;
        }
    }
    return 0;
}

void aoe_ai_ror_build_skip_ror(AoeAiRorBuildListRor *list, const AoeAiRorBuildOpsRor *ops) {
    EACH_ITEM(list, it) {
        if (!it->built && !it->in_progress && it->plan.skip != 1) {
            int32_t want = ops && ops->value ? ops->value(ops->context, 0xb1) : 0;
            if (it->plan.type != want || it->category != 0) {
                it->build_attempts = ops && ops->value ? ops->value(ops->context, 0x72) : 0;
                it->plan.skip = 1;
                it->skip_cycles++;
                if (ops && ops->value && it->skip_cycles == ops->value(ops->context, 0x92))
                    it->permanent_skip = 1;
                return;
            }
        }
    }
}

int aoe_ai_build_list_add_item_ror(AoeAiBuildListRor *list, AoeAiObjectFn objects,
                                   void *context, int32_t type, int32_t cap, int32_t position) {
    int32_t category = 2, from;
    switch (type) {
    case 4: case 5: case 0x19: case 0x27: case 0x29: from = 0x57; break;
    case 0xb: case 0x23: case 0x24: case 0x118: from = 0x31; break;
    case 0xd: case 0xf: case 0x11: case 0x12: case 0x13: case 0xfa: case 0x115: from = 0x2d; break;
    case 0x25: case 0x28: case 0x2e: case 299: from = 0x65; break;
    case 0x49: case 0x4b: from = 0xc; break;
    case 0x53: from = 0x6d; break;
    case 0x5d: from = 0; break;
    case 0x7d: from = 0x68; break;
    case 0x2d: case 0x31: case 0x32: case 0x44: case 0x45: case 0x46: case 0x4f: case 0x67:
    case 199: case 0x114: case 0x116:
        category = 0;
        from = -1;
        break;
    default:
        return 0;
    }
    if (position == -1) {
        const AoeAiBuildItemRor *i = list->sentinel.next;
        position = 0;
        while (i != &list->sentinel && i && i->built == 1) {
            i = i->next;
            ++position;
        }
    }
    return aoe_ai_build_list_insert_ror(list, objects, context, type, 1, category, from, cap,
                                        position);
}

void aoe_ai_con_plan_note_ror(AoeAiConPlanRor *plan, int32_t type, float x, float y,
                              int32_t value,
                              void (*mark)(void *context, int32_t type, int32_t x, int32_t y,
                                           uint8_t state),
                              void *context) {
    for (AoeAiConItemRor *it = plan->plan.next; it && it != &plan->plan; it = it->next) {
        if (it->plan.type == type && it->plan.x == x && it->plan.y == y) {
            it->field_78 = value;
            return;
        }
    }
    if (mark) mark(context, type, (int32_t)x, (int32_t)y, 1);
}

int aoe_ai_con_plan_fits_ror(const AoeAiConPlanRor *plan, const AoeAiPlanItemRor *item,
                             int free_placement,
                             int (*fallback)(void *context, const AoeAiPlanItemRor *item),
                             void *context) {
    if (plan->origin_x > 0.0f && plan->origin_y > 0.0f && plan->origin_x < (float)plan->map_width &&
        plan->origin_y < (float)plan->map_height) {
        if (item->type == 0x2d) return 1;
        if (!free_placement && plan->count) {
            for (const AoeAiConItemRor *e = plan->plan.next; e && e != &plan->plan; e = e->next) {
                int match = e->plan.type == item->type ||
                            (item->size_x == e->plan.size_x && item->size_y == e->plan.size_y &&
                             e->plan.type == -1);
                if (match && !e->field_78 && !e->field_74) return 1;
            }
            return 0;
        }
    }
    return fallback ? fallback(context, item) : 0;
}

int32_t aoe_ai_ror_build_costs_ror(const AoeAiRorBuildListRor *list, AoeAiCountersRor *costs,
                                   int32_t limit, AoeAiCostFn research_cost,
                                   AoeAiCostFn object_cost, void *context) {
    int32_t n = 0;
    aoe_ai_counters_fill_ror(costs, 0);
    for (const AoeAiBuildItemRor *it = list->base.sentinel.next;
         it && it != &list->base.sentinel; it = it->next) {
        int16_t pairs[6] = {-1, 0, -1, 0, -1, 0};
        if (it->built || it->in_progress || it->category == 6) continue;
        if (it->cap != -1 && it->built_count > it->cap) continue;
        if (it->category == 1 || it->category == 4) {
            if (research_cost) research_cost(context, it->plan.type, pairs);
        } else if (object_cost) {
            object_cost(context, it->plan.type, pairs);
        }
        for (int i = 0; i < 3; ++i) aoe_ai_counters_add_ror(costs, pairs[2 * i], pairs[2 * i + 1]);
        if (++n == limit) return n;
    }
    return n;
}

int aoe_ai_ror_build_failed_ror(AoeAiRorBuildListRor *list, const AoeAiRorBuildFailOpsRor *ops,
                                int32_t reason, int32_t object, int32_t unique_id, float x,
                                float y, int32_t arg) {
    int32_t flag = 1;
    uint8_t state = 2;
    AoeAiBuildItemRor *it;
    if (arg != -1 && ops->player_hook) ops->player_hook(ops->context, arg);
    if (reason == -1) state = 0;
    else if (reason >= 1 && reason <= 7) flag = 0;
    if (reason == 8 && ops->object_alive && ops->object_alive(ops->context, object)) {
        if (ops->listed_unit && ops->listed_unit(ops->context, object) == 1) {
            const AoeAiConItemRor *c = ops->plan_at ? ops->plan_at(ops->context, x, y) : NULL;
            if (ops->avoid) {
                if (c)
                    ops->avoid(ops->context, object, c->plan.x - c->plan.size_x * 0.5f,
                               c->plan.y - c->plan.size_y * 0.5f, c->plan.size_x * 0.5f + c->plan.x,
                               c->plan.size_y * 0.5f + c->plan.y);
                else
                    ops->avoid(ops->context, object, x - 2.0f, y - 2.0f, x + 2.0f, y + 2.0f);
            }
        } else if (ops->plan_block) {
            ops->plan_block(ops->context, x, y, 10);
        }
    }
    if (ops->plan_note) ops->plan_note(ops->context, x, y, flag);
    it = aoe_ai_build_list_find_ror(&list->base, unique_id);
    if (it) {
        it->in_progress = 0;
        if (ops->set_mark)
            ops->set_mark(ops->context, it->plan.type, (int32_t)x, (int32_t)y, state);
        if (it->plan.type == 0x114 && ops->owner_flag_122b0) *ops->owner_flag_122b0 = 0;
    }
    return 1;
}

int aoe_ai_ror_build_list_augment_ror(AoeAiRorBuildListRor *list, AoeAiObjectFn objects,
                                      void *context, int32_t (*sn)(void *context, int32_t i),
                                      AoeAiCostFn research_cost, AoeAiCostFn object_cost) {
    AoeAiBuildListRor *l = &list->base;
    int32_t houses = 0, docks = 0, boats_d = 0, boats_11 = 0, boats_13 = 0;
    int32_t u65 = -1, u66 = -1, u67 = -1, n, step, pos, i;
    int has5 = 0, has7 = 0, has8 = 0;
#define SN(i) (sn ? sn(context, (i)) : 0)
    for (AoeAiBuildItemRor *it = l->sentinel.next; it && it != &l->sentinel; it = it->next) {
        int32_t ty = it->plan.type;
        if (it->category == 0) {
            if (ty == 0x46) ++houses;
            else if (ty == 0x2d) ++docks;
        } else if (it->category == 2) {
            if (ty == 0xd) ++boats_d;
            else if (ty == 0x11 || ty == 0x12) ++boats_11;
            else if (ty == 0x13 || ty == 0xfa || ty == 0x115) ++boats_13;
        } else if (it->category == 1 || it->category == 4) {
            if (ty == 0x65) u65 = it->plan.unique_id;
            else if (ty == 0x66) u66 = it->plan.unique_id;
            else if (ty == 0x67) u67 = it->plan.unique_id;
            else if (ty == 5) has5 = 1;
            else if (ty == 7) has7 = 1;
            else if (ty == 8) has8 = 1;
        }
    }
    if (docks < SN(0xd4) && SN(0xd0) == 1) {
        n = SN(0xd4) - docks;
        step = u66 != -1 ? u66 / n : u65 != -1 ? u65 / n : 10;
        for (pos = step, i = 0; i < n; ++i, pos += step)
            aoe_ai_build_list_add_item_ror(l, objects, context, 0x2d, -1, pos);
    }
    if (boats_d < SN(0xd5) && SN(0xd1) == 1) {
        n = SN(0xd5) - boats_d;
        if (n > 0) {
            step = u66 != -1 ? u66 / n : u65 != -1 ? u65 / n : 10;
            for (pos = step, i = 0; i < n; ++i, pos += step)
                aoe_ai_build_list_add_item_ror(l, objects, context, 0xd, -1, pos);
        }
    }
    if (boats_11 < SN(0xd6) && SN(0xd2) == 1) {
        n = SN(0xd6) - boats_11;
        if (u65 == -1) {
            if (u66 == -1) step = 10, pos = 0;
            else step = (l->count - u66) / (n * 2), pos = -1;
        } else {
            step = (l->count - u65) / (n * 2);
            pos = u66;
        }
        for (i = 0; i < n; ++i) {
            pos += step;
            aoe_ai_build_list_add_item_ror(l, objects, context, 0x11, -1, pos);
        }
        if (!has8 && u67 != -1) aoe_ai_build_list_add_research_ror(l, objects, context, 8, u67 + 2);
    }
    if (boats_13 < SN(0xe0) && SN(0xdf) == 1 && (u67 != -1 || u66 != -1)) {
        int32_t base = u67 != -1 ? u67 : u66;
        n = SN(0xe0) - boats_13;
        step = base / n;
        for (pos = step, i = 0; i < n; ++i, pos += step)
            aoe_ai_build_list_add_research_ror(l, objects, context, 0x13, pos);
        if (!has5 && u66 != -1) aoe_ai_build_list_add_research_ror(l, objects, context, 5, u66 + 5);
        if (!has7 && u67 != -1) aoe_ai_build_list_add_research_ror(l, objects, context, 7, u67 + 5);
    }
    if (houses == 0 && SN(0xb4) == 1) {
        int32_t index = 0, room = 0, units = 0;
        for (AoeAiBuildItemRor *it = l->sentinel.next; it && it != &l->sentinel; it = it->next) {
            if (it->category == 0 && it->plan.type == 0x6d) room += 4;
            if (it->category == 2) ++units;
            if (room < units &&
                aoe_ai_build_list_add_item_ror(l, objects, context, 0x46, -1,
                                               index < 2 ? 1 : index - 1) == 1)
                room += 4;
            if (SN(0x98) + 0x32 < room) break;
            ++index;
        }
    }
    {
        int32_t share = SN(0xa0), group = SN(0x9b);
        if (share) {
            AoeAiCountersRor costs;
            int32_t k = 0, at = 0;
            aoe_ai_counters_init_ror(&costs, 4);
            aoe_ai_counters_fill_ror(&costs, 0);
            for (AoeAiBuildItemRor *it = l->sentinel.next; it && it != &l->sentinel; it = it->next) {
                int16_t pairs[6] = {-1, 0, -1, 0, -1, 0};
                if (it->category == 6) {
                    ++k;
                    continue;
                }
                if (it->category == 1 || it->category == 4) {
                    if (research_cost) research_cost(context, it->plan.type, pairs);
                } else if (object_cost) {
                    object_cost(context, it->plan.type, pairs);
                }
                for (i = 0; i < 3; ++i) aoe_ai_counters_add_ror(&costs, pairs[2 * i], pairs[2 * i + 1]);
                if (group + 4 <= ++k) {
                    int32_t sum = 0;
                    for (i = 0; i < 4; ++i) sum += aoe_ai_counters_get_ror(&costs, i);
                    if (sum > 0) {
                        for (i = 0; i < 4; ++i)
                            aoe_ai_build_list_add_gather_ror(
                                l, objects, context, i,
                                aoe_ai_counters_get_ror(&costs, i) * share / sum, at);
                        aoe_ai_counters_fill_ror(&costs, 0);
                        at += group + 4;
                        k = 0;
                    }
                }
            }
            aoe_ai_counters_release_ror(&costs);
        }
    }
#undef SN
    return 1;
}

static void picked(AoeAiRorBuildListRor *list, AoeAiBuildItemRor *it) {
    AoeAiBuildListRor *l = &list->base;
    snprintf(l->name_3a6, 64, "%s", it->plan.name);
    l->field_5a8 = it->plan.unique_id;
    snprintf(l->name_4a7, 64, "%s", it->next ? it->next->plan.name : "");
    l->field_5a8 = it->plan.unique_id;
}

static void item_costs(const AoeAiRorPickOpsRor *o, const AoeAiBuildItemRor *it, int research,
                       AoeAiCountersRor *c) {
    int16_t pairs[6] = {-1, 0, -1, 0, -1, 0};
    aoe_ai_counters_init_ror(c, 4);
    if (research) {
        if (o->research_cost) o->research_cost(o->context, it->plan.type, pairs);
    } else if (o->object_cost) {
        o->object_cost(o->context, it->plan.type, pairs);
    }
    for (int i = 0; i < 3; ++i) aoe_ai_counters_add_ror(c, pairs[2 * i], pairs[2 * i + 1]);
}

AoeAiBuildItemRor *aoe_ai_ror_build_next_ror(AoeAiRorBuildListRor *list, int32_t mode,
                                             const AoeAiRorPickOpsRor *o) {
    AoeAiBuildListRor *l = &list->base;
    int32_t c67, c44, c46, counted = 0, pending = 0;
#define SN(i) (o->sn ? o->sn(o->context, (i)) : 0)
    c67 = aoe_ai_build_list_count_ror2(l, 0x67, 0, o->count_extra, o->context);
    c44 = aoe_ai_build_list_count_ror2(l, 0x44, 0, o->count_extra, o->context);
    c46 = aoe_ai_build_list_count_ror2(l, 0x46, 0, o->count_extra, o->context);
    if (SN(0x73) < 0) return NULL;
    for (AoeAiBuildItemRor *it = l->sentinel.next; it && it != &l->sentinel;) {
        AoeAiBuildItemRor *next = it->next;
        int32_t ty = it->plan.type, cat = it->category;
        if ((cat == 1 || cat == 4) && (ty == 0x65 || ty == 0x66 || ty == 0x67) && o->tech_state) {
            switch (o->tech_state(o->context, ty)) {
            case 0: case 1: it->built = 0; it->in_progress = 0; break;
            case 2: it->built = 0; it->in_progress = 1; break;
            case 3: it->built = 1; it->in_progress = 0; break;
            default: break;
            }
        }
        if (it->built || it->in_progress) goto advance;
        {
            int flag = 1;
            int32_t rule = SN(0xa1);
            ++pending;
            if (rule == 1) flag = c44 || c67;
            else if (rule == 2) flag = (o->resource && o->resource(o->context, 1) >= 0xfa) || c67;
            else if (rule == 3) flag = c44 != 0;
            else if (rule == 4) flag = c44 && c67;
            if (mode == 0) {
                if (!flag && cat == 0 && ty != 0x46 && ty != 0x6d && ty != 0x67 && ty != 0x44)
                    goto advance;
                if (cat == 0 && !(c44 && c67) && c46 >= SN(0xae) && ty != 0x6d && ty != 0x67 &&
                    ty != 0x44)
                    goto advance;
            }
        }
        if (cat == 5 && counted == 0) {
            if (o->tactical_mark) o->tactical_mark(o->context);
            it->in_progress = 0;
            it->built = 1;
            goto advance;
        }
        if (cat == 6) {
            if (counted >= 1) goto advance;
            if (o->set_sn) o->set_sn(o->context, ty + 0x9c, it->build_from);
            it->in_progress = 0;
            it->built = 1;
            goto advance;
        }
        if (mode == 3 && counted > 0) return NULL;
        if (it->cap != -1 && it->built_count > it->cap) goto advance;
        if (mode == 1 && (cat == 1 || cat == 4) && o->can_research &&
            !o->can_research(o->context, ty)) {
            it->in_progress = 0;
            it->built = 1;
            goto advance;
        }
        if (cat == 4) {
            AoeAiCountersRor c;
            int ok;
            item_costs(o, it, 1, &c);
            ok = o->tech_open && o->tech_open(o->context, ty) && o->affordable &&
                 o->affordable(o->context, &c);
            aoe_ai_counters_release_ror(&c);
            if (!ok || mode != 1) return NULL;
        }
        if ((it->plan.skip == 1 && it->build_attempts > 0) || it->permanent_skip) {
            if (it->build_attempts > 0) --it->build_attempts;
            goto advance;
        }
        if (it->plan.skip == 1 && it->build_attempts <= 0) {
            it->plan.skip = 0;
            if (cat == 0 && (ty == 0x67 || ty == 0x44)) it->build_attempts = SN(0x72) - 1;
        }
        if (!(o->population_full && o->population_full(o->context) && cat == 2)) ++counted;
        if (mode == 1) {
            if (cat != 1 && cat != 4) goto advance;
        } else if (cat != mode) {
            goto advance;
        }
        ++it->build_attempts;
        if (it->build_attempts > SN(0x72) && it->plan.skip == 0 && !it->permanent_skip) {
            if (mode == 1 && cat == 4) {
                it->build_attempts = 0;
                return NULL;
            }
            if (!(ty == SN(0xb1) && cat == 0)) {
                it->plan.skip = 1;
                it->build_attempts =
                    cat == 0 && (ty == 0x67 || ty == 0x44) ? SN(0x72) * 6 : SN(0x72);
                goto advance;
            }
        }
        if (mode == 3) {
            if (cat == 3) {
                picked(list, it);
                return it;
            }
        }
        if (mode == 1 ? !(o->tech_open && o->tech_open(o->context, ty))
                      : !(o->usable && o->usable(o->context, ty)))
            goto advance;
        {
            AoeAiCountersRor c;
            int found = 0;
            item_costs(o, it, mode == 1, &c);
            if (o->affordable && o->affordable(o->context, &c)) {
                if (mode == 2 || mode == 1)
                    found = o->producer_present && o->producer_present(o->context, it->build_from);
                else
                    found = o->plan_fits && o->plan_fits(o->context, &it->plan);
            }
            aoe_ai_counters_release_ror(&c);
            if (found) {
                picked(list, it);
                return it;
            }
        }
    advance:
        it = next;
        if (counted > SN(0x73)) return NULL;
    }
#undef SN
    return NULL;
}
