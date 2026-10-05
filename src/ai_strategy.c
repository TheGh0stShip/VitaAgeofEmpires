#include "ai_strategy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"
#include "aoe_stream.h"

/* ---- Growable ID list ---- */

/* Grow to exactly `capacity` entries, keeping the old ones (the inline
   pattern after operator new; 0 when the allocation fails). */
static int list_grow(AoeAiIntListRor *list, int32_t capacity) {
    int32_t *data;
    int32_t i;
    if (capacity <= 0) return 0;
    data = (int32_t *)calloc((size_t)capacity, sizeof(*data));
    if (!data) return 0;
    for (i = 0; i < list->capacity && i < capacity; ++i)
        data[i] = list->data[i];
    free(list->data);
    list->data = data;
    list->capacity = capacity;
    return 1;
}

int aoe_ai_int_list_contains_ror(const AoeAiIntListRor *list, int32_t value) {
    int32_t i;
    if (!list) return 0;
    for (i = 0; i < list->count; ++i) {
        if (i >= list->capacity) return 0;
        if (list->data[i] == value) return 1;
    }
    return 0;
}

/* Append without the uniqueness check (the inline form the callers use
   after their own contains test, growing to count + 1). */
static int list_push(AoeAiIntListRor *list, int32_t value) {
    if (list->capacity - 1 < list->count && !list_grow(list, list->count + 1))
        return 0;
    list->data[list->count++] = value;
    return 1;
}

int aoe_ai_int_list_add_unique_ror(AoeAiIntListRor *list, int32_t value) {
    if (!list || aoe_ai_int_list_contains_ror(list, value)) return 0;
    return list_push(list, value);
}

int32_t *aoe_ai_int_list_at_ror(AoeAiIntListRor *list, int32_t i) {
    if (!list || i < 0) return NULL;
    if (list->capacity - 1 < i && !list_grow(list, i + 1)) return NULL;
    return &list->data[i];
}

int aoe_ai_int_list_remove_ror(AoeAiIntListRor *list, int32_t value) {
    int32_t i, n;
    if (!list) return 0;
    n = list->capacity;
    for (i = 0; i < n; ++i)
        if (list->data[i] == value) break;
    if (i >= n) return 0;
    for (; i < n - 1; ++i) list->data[i] = list->data[i + 1];
    if (--list->count < 0) list->count = 0;
    return 1;
}

void aoe_ai_int_list_release_ror(AoeAiIntListRor *list) {
    if (!list) return;
    free(list->data);
    memset(list, 0, sizeof(*list));
}

/* Remove the first occurrence, shifting the rest down; the count does not
   drop below 0 (the original scans up to the capacity). */
static void list_remove(AoeAiIntListRor *list, int32_t value) {
    int32_t i, n = list->count;
    for (i = 0; i < n; ++i)
        if (list->data[i] == value) break;
    if (i >= n) return;
    for (; i < n - 1; ++i) list->data[i] = list->data[i + 1];
    if (--list->count < 0) list->count = 0;
}

/* ---- RoR play list ---- */

void aoe_ai_ror_play_list_init_ror(AoeAiPlayListRor *list) {
    aoe_ai_play_list_init_ror(list);
}

void aoe_ai_ror_play_list_release_ror(AoeAiPlayListRor *list) {
    aoe_ai_play_list_release_ror(list);
}

int32_t aoe_ai_ror_play_any_kind_ror(const char *name) {
    return aoe_ai_any_kind_ror(name);
}

int32_t aoe_ai_ror_play_speed_kind_ror(const char *name) {
    return aoe_ai_speed_kind_ror(name);
}

int32_t aoe_ai_play_category_ror(const char *name) {
    if (!name) return -1;
    if (!strcmp(name, "Hand")) return 0;
    if (!strcmp(name, "Ranged")) return 1;
    if (!strcmp(name, "Priest")) return 4;
    if (!strcmp(name, "Siege")) return 5;
    if (!strcmp(name, "Cavalry")) return 2;
    if (!strcmp(name, "Legion")) return 3;
    return -1;
}

int32_t aoe_ai_play_unit_category_ror(int16_t unit_class,
                                      AoeAiUnitRangeFn range, void *context) {
    if (unit_class == 0x12) return 4;
    if (unit_class == 0x1c) return 3;
    if (unit_class == 0x0d) return 5;
    if (unit_class == 0x0c || unit_class == 0x23) return 2;
    return range && range(context) > (double)1.0f ? 1 : 0;
}

/* ---- Resource AI ---- */

void aoe_ai_resource_init_ror(AoeAiResourceRor *ai, int32_t player,
                              int32_t resource_count, AoeAiLogRor *log) {
    if (!ai) return;
    memset(ai, 0, sizeof(*ai));
    aoe_ai_module_init_ror(&ai->module, "Resource AI", 0x3ef, player, log);
    ai->field_f0 = 0;
    ai->resource_count = resource_count;
}

int aoe_ai_resource_load_ror(AoeAiResourceRor *ai, int32_t player,
                             AoeOutcomeStream *in, AoeAiLogRor *log) {
    if (!ai || !in) return 0;
    memset(ai, 0, sizeof(*ai));
    aoe_ai_module_init_ror(&ai->module, "ResAI", 0x3ef, player, log);
    ai->resource_count = aoe_stream_i32(in);
    return !in->failed;
}

void aoe_ai_resource_release_ror(AoeAiResourceRor *ai) {
    if (ai) aoe_ai_module_release_ror(&ai->module);
}

int aoe_ai_resource_save_ror(const AoeAiResourceRor *ai,
                             AoeOutcomeStream *out) {
    if (!ai || !out) return 0;
    aoe_stream_put_i32(out, ai->resource_count);
    return 1;
}

int32_t aoe_ai_resource_amount_ror(const AoeAiResourceRor *ai,
                                   const float *resources, int32_t i) {
    if (!ai || !resources || i < 0 || i >= ai->resource_count) return -1;
    return aoe_ftol32(resources[i]);
}

int aoe_ai_resource_can_afford_ror(const AoeAiResourceRor *ai,
                                   const float *resources,
                                   const AoeAiCountersRor *costs) {
    int32_t i;
    if (!ai || !costs) return 0;
    if (ai->resource_count != aoe_ai_counters_count_ror(costs)) return 0;
    for (i = 0; i < aoe_ai_counters_count_ror(costs); ++i) {
        /* fild then fcomp against the float: exact in double. */
        if ((double)aoe_ai_counters_get_ror(costs, i) > (double)resources[i])
            return 0;
    }
    return 1;
}

const char *aoe_ai_resource_name_ror(int32_t i) {
    switch (i) {
    case 0: return "Food";
    case 1: return "Wood";
    case 2: return "Stone";
    case 3: return "Gold";
    default: return "Unknown";
    }
}

/* ---- Strategic numbers ---- */

const int32_t aoe_ai_sn_defaults_ror[AOE_AI_SN_COUNT] = {
    /*   0 */ 34, 0, 66, 2, 2, -1, -1, -1, -1, -1,
    /*  10 */ -1, -1, -1, -1, -1, -1, 4, -1, 4, 50,
    /*  20 */ 50, -1, 12, 10, 75, 1, 10, -1, 4, 85,
    /*  30 */ 70, 75, 100, -1, 50, 0, 3, 0, 0, 0,
    /*  40 */ 0, 4, 0, 1, 3, 0, 30, 1, 30, 3,
    /*  50 */ 0, 0, 0, 0, 0, 0, 7, 3, 2, 1,
    /*  60 */ 5, 1, 1, 2, 0, 0, 0, 0, 0, 0,
    /*  70 */ 0, 1, 2, 12, 20, 3, 15, 50, 0, 0,
    /*  80 */ 0, 0, 0, 0, 4, 2, 25, 100, 3, -25,
    /*  90 */ 0, 100, 5, 1, 0, 0, 0, 0, 1, 10,
    /* 100 */ 3, 1, 15, 0, 0, 0, -1, -1, -1, 0,
    /* 110 */ 0, 0, 300, 0, 25, 10, 10, 0, 0, 0,
    /* 120 */ 0, 0, 100, 0, 0, 0, 0, 0, -1, 0,
    /* 130 */ 0, 3, 0, 0, 0, 1, 15, 0, 0, 0,
    /* 140 */ 0, 0, 0, 1, 0, 10, 50, 10, 20, 40,
    /* 150 */ -1, 3, 0, -1, -1, 10, 0, 0, 0, 0,
    /* 160 */ 50, 2, 0, 7, 10, 25, 7, 2, 50, 10,
    /* 170 */ 200, 250, 150, 200, 2, -1, 0, -1, 10, 30,
    /* 180 */ 1, 1, 0, 0, 20, 50, 0, 0, 1, 50,
    /* 190 */ 3000, 2000, 1500, 3000, 1, 25, 1, 120, 50, 25,
    /* 200 */ 50, 0, 25, 1, 5, 1, 1, 1, 0, 0,
    /* 210 */ 0, 1, 0, 0, 0, 1, 50, 0, 100, 100,
    /* 220 */ 8, 3, 10, 0, 0, 1,
};

void aoe_ai_sn_init_ror(AoeAiStrategicNumbersRor *sn) {
    if (sn) memcpy(sn->values, aoe_ai_sn_defaults_ror, sizeof(sn->values));
}

void aoe_ai_sn_set_ror(AoeAiStrategicNumbersRor *sn, int32_t index,
                       int32_t value) {
    if (sn && index >= 0 && index < AOE_AI_SN_COUNT) sn->values[index] = value;
}

int32_t aoe_ai_sn_get_ror(const AoeAiStrategicNumbersRor *sn, int32_t index) {
    if (!sn || index < 0 || index >= AOE_AI_SN_COUNT) return -1;
    return sn->values[index];
}

int aoe_ai_sn_load_file_ror(AoeAiStrategicNumbersRor *sn, const char *name,
                            const AoeAiTextFileOpsRor *ops) {
    char line[256];
    void *file;
    int index = -1, value = -1;
    if (!sn || !name || !ops || !ops->open || !ops->gets) return 0;
    if (!strcmp(name, "NONE") || !name[0]) return 0;
    /* The original formats the name with "%s" into a 256-byte buffer. */
    snprintf(line, sizeof line, "%s", name);
    file = ops->open(ops->context, line);
    if (!file) return 0;
    while (ops->gets(ops->context, file, line, (int)sizeof line)) {
        char first = 0, second = 0;
        int skip = 0;
        sscanf(line, " %c%c", &first, &second);
        if (first == '/') {
            skip = second == '/';
        } else if (first == 'D') {
            skip = second == 'E';
        } else if (first == 'V') {
            skip = second == 'C';
        } else if (first == 'E' && second == 'N') {
            break;
        }
        if (skip) continue;
        sscanf(line, "%d %d", &index, &value);
        aoe_ai_sn_set_ror(sn, index, value);
    }
    if (ops->close) ops->close(ops->context, file);
    if (strstr(name, ".tmp") && ops->remove) ops->remove(ops->context, name);
    return 1;
}

/* The numbers each category copies, in the original order. */
static const int16_t cat_1[] = {
    0, 1, 2, 3, 4, 5, 35, 100, 225, 117, 118, 119, 120, 156, 159, 158,
    157, 160, 142, 148, 149, 203, 163, 164, 166, 165, 167, 179, 180, 205,
    207, 206, 220, 222, 208, 209, 210, 211, 223, 212, 213, 214, 224, 169,
    170, 171, 172, 173, 190, 191, 192, 193, 174, 204};
static const int16_t cat_2[] = {
    16, 26, 36, 37, 93, 94, 98, 121, 131, 143, 162};
static const int16_t cat_12[] = {18};
static const int16_t cat_13[] = {19};
static const int16_t cat_14[] = {20};
static const int16_t cat_16[] = {22, 72, 57, 92};
static const int16_t cat_17[] = {23};
static const int16_t cat_18[] = {24};
static const int16_t cat_21[] = {25, 28, 38, 39};
static const int16_t cat_22[] = {29};
static const int16_t cat_23[] = {32, 145};
static const int16_t cat_24[] = {34};
static const int16_t cat_25[] = {40, 75};
static const int16_t cat_26[] = {41};
static const int16_t cat_27[] = {43, 44, 42, 45};
static const int16_t cat_28[] = {30, 31, 91, 95, 96, 97};
static const int16_t cat_29[] = {46, 102, 47, 103, 104, 134};
static const int16_t cat_30[] = {48};
static const int16_t cat_31[] = {49, 135, 136, 200};
static const int16_t cat_32[] = {50, 51, 52, 53, 54, 55, 56, 70};
static const int16_t cat_33[] = {58, 59, 60};
static const int16_t cat_34[] = {61, 62, 63};
static const int16_t cat_35[] = {64, 65, 66, 216};
static const int16_t cat_36[] = {67, 68, 69};
static const int16_t cat_37[] = {71};
static const int16_t cat_38[] = {73, 74};
static const int16_t cat_39[] = {76};
static const int16_t cat_40[] = {
    77, 78, 79, 80, 81, 82, 83, 89, 90, 106, 107, 108, 109, 110, 111, 122,
    123, 144, 184, 185};
static const int16_t cat_41[] = {84, 85, 137};
static const int16_t cat_42[] = {86, 87, 202, 150, 151, 152, 161, 168};
static const int16_t cat_43[] = {
    88, 101, 155, 175, 176, 177, 181, 182, 183, 186, 187, 194, 196, 197,
    188, 195, 189, 198, 199, 215, 217, 218, 219, 221};
static const int16_t cat_44[] = {99};
static const int16_t cat_45[] = {105};
static const int16_t cat_46[] = {112};
static const int16_t cat_47[] = {113, 114, 115, 116, 146, 147};
static const int16_t cat_48[] = {
    124, 125, 126, 127, 128, 129, 130, 132, 133, 178, 201};
static const int16_t cat_49[] = {138, 139, 140, 141};

typedef struct {
    const int16_t *numbers;
    int32_t count;
} Category;

#define CAT(n) {cat_##n, (int32_t)(sizeof cat_##n / sizeof cat_##n[0])}
static const Category categories[50] = {
    {NULL, 0}, CAT(1), CAT(2), {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0},
    {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, CAT(12), CAT(13),
    CAT(14), {NULL, 0}, CAT(16), CAT(17), CAT(18), {NULL, 0}, {NULL, 0},
    CAT(21), CAT(22), CAT(23), CAT(24), CAT(25), CAT(26), CAT(27), CAT(28),
    CAT(29), CAT(30), CAT(31), CAT(32), CAT(33), CAT(34), CAT(35), CAT(36),
    CAT(37), CAT(38), CAT(39), CAT(40), CAT(41), CAT(42), CAT(43), CAT(44),
    CAT(45), CAT(46), CAT(47), CAT(48), CAT(49)};
#undef CAT

int32_t aoe_ai_sn_category_numbers_ror(int32_t category,
                                       const int16_t **numbers) {
    const Category *c;
    uint32_t index = (uint32_t)category;
    if (index >= 50u || !categories[index].count) {
        if (numbers) *numbers = NULL;
        return 0;
    }
    c = &categories[index];
    if (numbers) *numbers = c->numbers;
    return c->count;
}

int32_t aoe_ai_sn_category_kind_ror(int32_t category) {
    return aoe_ai_sn_category_numbers_ror(category, NULL) ? -2 : 0;
}

int aoe_ai_sn_apply_category_ror(const AoeAiStrategicNumbersRor *sn,
                                 int32_t victory_kind, int32_t category,
                                 AoeAiSnSetFn set, void *context) {
    const int16_t *numbers;
    int32_t i, n;
    if (!sn || victory_kind < 0 || victory_kind >= 13) return 0;
    n = aoe_ai_sn_category_numbers_ror(category, &numbers);
    if (!n) return 0;
    for (i = 0; i < n; ++i)
        if (set) set(context, numbers[i], sn->values[numbers[i]]);
    return 1;
}

/* Multiply by the factor (x87 fild/fmul, 53-bit), truncate; 0 becomes 1. */
static void scale_one(AoeAiStrategicNumbersRor *sn, int index, float factor) {
    int32_t v = sn->values[index];
    if (v <= 0) return;
    v = aoe_ftol32((double)v * (double)factor);
    sn->values[index] = v ? v : 1;
}

void aoe_ai_sn_scale_difficulty_ror(AoeAiStrategicNumbersRor *sn,
                                    int32_t difficulty,
                                    int32_t settings_flag) {
    float factor = 1.0f, factor_b = 1.0f;
    int32_t add_46 = 0, add_104 = 0, add_88 = 0;
    if (!sn) return;
    switch (difficulty) {
    case 1:
        if (settings_flag == 1) {
            factor = factor_b = 0.8f;
            add_46 = 30;
            add_104 = 120;
            add_88 = 1;
        }
        break;
    case 2:
        factor = factor_b = 0.6f;
        add_46 = 60;
        add_104 = 240;
        add_88 = 2;
        break;
    case 3:
        factor = factor_b = 0.4f;
        add_46 = 90;
        add_104 = settings_flag == 1 ? 360 : 900;
        add_88 = 3;
        break;
    case 4:
        factor = factor_b = 0.2f;
        add_46 = 120;
        add_104 = settings_flag == 1 ? 480 : 1260;
        add_88 = 4;
        break;
    default:
        return;
    }
    scale_one(sn, 36, factor);
    scale_one(sn, 16, factor);
    scale_one(sn, 26, factor);
    sn->values[46] += add_46;
    sn->values[48] += add_46;
    scale_one(sn, 19, factor_b);
    scale_one(sn, 20, factor_b);
    sn->values[104] += add_104;
    scale_one(sn, 25, factor);
    scale_one(sn, 28, factor);
    scale_one(sn, 38, factor);
    scale_one(sn, 58, factor);
    scale_one(sn, 59, factor);
    scale_one(sn, 60, factor);
    scale_one(sn, 67, factor);
    scale_one(sn, 68, factor);
    scale_one(sn, 69, factor);
    sn->values[88] += add_88;
    sn->values[103] = difficulty < 4 ? 1 : 0;
}

/* ---- Strategy AI ---- */

static void set_point(float out[4], float x, float y, float z) {
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

static void strategy_fields(AoeAiStrategyRor *ai) {
    ai->field_f0 = 0;
    ai->victory_kind = -1;
    ai->target_id = ai->target_type = -1;
    ai->second_id = ai->second_type = -1;
    ai->field_22c = ai->field_230 = -1;
    ai->update_count = 0;
    set_point(ai->target_point, -1.0f, -1.0f, -1.0f);
    set_point(ai->second_point, -1.0f, -1.0f, -1.0f);
    aoe_ai_sn_init_ror(&ai->numbers);
    ai->difficulty = 0;
}

void aoe_ai_strategy_ai_init_ror(AoeAiStrategyRor *ai, int32_t player,
                                 AoeAiLogRor *log) {
    if (!ai) return;
    memset(ai, 0, sizeof(*ai));
    aoe_ai_strategy_init_ror(&ai->module, player, log);
    strategy_fields(ai);
    ai->sn_file[0] = '\0';
}

void aoe_ai_strategy_ai_release_ror(AoeAiStrategyRor *ai) {
    if (!ai) return;
    aoe_ai_int_list_release_ror(&ai->rejected);
    aoe_ai_int_list_release_ror(&ai->applied);
    aoe_ai_int_list_release_ror(&ai->categories);
    aoe_ai_strategy_release_ror(&ai->module);
}

void aoe_ai_strategy_ai_set_owner_ror(AoeAiStrategyRor *ai, int32_t owner) {
    if (ai) ai->field_f0 = owner;
}

const char *aoe_ai_strategy_ai_sn_file_ror(const AoeAiStrategyRor *ai) {
    return ai ? ai->sn_file : NULL;
}

void aoe_ai_strategy_ai_load_sn_file_ror(AoeAiStrategyRor *ai,
                                         const char *name,
                                         const AoeAiTextFileOpsRor *files) {
    size_t i;
    if (!ai || !name) return;
    /* strncpy(+0xf8, name, 0xff): copy, then pad with zeros; byte 255
       keeps its value. */
    for (i = 0; i < 0xff && name[i]; ++i) ai->sn_file[i] = name[i];
    for (; i < 0xff; ++i) ai->sn_file[i] = '\0';
    aoe_ai_sn_load_file_ror(&ai->numbers, ai->sn_file, files);
}

void aoe_ai_strategy_ai_set_sn_ror(AoeAiStrategyRor *ai, int32_t index,
                                   int32_t value) {
    if (!ai) return;
    aoe_ai_log_ror(&ai->module, "Setting SN %d to %d", index, value);
    aoe_ai_sn_set_ror(&ai->numbers, index, value);
}

int32_t aoe_ai_strategy_ai_get_sn_ror(const AoeAiStrategyRor *ai,
                                      int32_t index) {
    return ai ? aoe_ai_sn_get_ror(&ai->numbers, index) : -1;
}

void aoe_ai_strategy_ai_scale_ror(AoeAiStrategyRor *ai, int32_t difficulty,
                                  int32_t settings_flag) {
    if (ai) aoe_ai_sn_scale_difficulty_ror(&ai->numbers, difficulty,
                                           settings_flag);
}

int32_t aoe_ai_strategy_ai_victory_kind_ror(const AoeAiStrategyRor *ai) {
    return ai->victory_kind;
}

int32_t aoe_ai_strategy_ai_target_id_ror(const AoeAiStrategyRor *ai) {
    return ai->target_id;
}

int32_t aoe_ai_strategy_ai_second_id_ror(const AoeAiStrategyRor *ai) {
    return ai->second_id;
}

const float *aoe_ai_strategy_ai_target_point_ror(const AoeAiStrategyRor *ai) {
    return ai->target_point;
}

const float *aoe_ai_strategy_ai_second_point_ror(const AoeAiStrategyRor *ai) {
    return ai->second_point;
}

static int apply(AoeAiStrategyRor *ai, int32_t category,
                 const AoeAiStrategyOpsRor *ops) {
    return aoe_ai_sn_apply_category_ror(&ai->numbers, ai->victory_kind,
                                        category,
                                        ops ? ops->set_tactical_sn : NULL,
                                        ops ? ops->context : NULL);
}

void aoe_ai_strategy_ai_set_victory_kind_ror(AoeAiStrategyRor *ai,
                                             int32_t kind,
                                             const AoeAiStrategyOpsRor *ops) {
    static const int32_t first[] = {1, 2, 12, 13, 14, 16, 17, 18,
                                    21, 22, 23, 24, 25};
    size_t k;
    int32_t i;
    if (!ai) return;
    ai->victory_kind = kind;
    ai->categories.count = 0;
    for (k = 0; k < sizeof first / sizeof first[0]; ++k)
        if (!aoe_ai_int_list_contains_ror(&ai->categories, first[k]))
            list_push(&ai->categories, first[k]);
    for (i = 26; i <= 49; ++i)
        aoe_ai_int_list_add_unique_ror(&ai->categories, i);
    for (i = 0; i < ai->categories.count; ++i) {
        int32_t *category = aoe_ai_int_list_at_ror(&ai->categories, i);
        if (!category) return;
        if (aoe_ai_sn_category_kind_ror(*category) != -2) continue;
        if (apply(ai, *category, ops) == 1)
            aoe_ai_int_list_add_unique_ror(&ai->applied, *category);
        else
            aoe_ai_int_list_add_unique_ror(&ai->rejected, *category);
    }
}

int aoe_ai_strategy_ai_update_ror(AoeAiStrategyRor *ai,
                                  const AoeAiStrategyOpsRor *ops) {
    int32_t i, best, best_kind;
    if (!ai) return 0;
    if (++ai->update_count >= 0) {
        AoeAiVictoryTargetRor target;
        int32_t kind = 11;
        ai->update_count = 0;
        if (ops && ops->first_condition &&
            ops->first_condition(ops->context, &target)) {
            kind = target.kind;
            if (target.has_unit_28) {
                ai->target_id = target.unit_28_id;
                ai->target_type = target.unit_28_type;
            }
            if (target.has_unit_24) {
                ai->second_id = target.unit_24_id;
                ai->second_type = target.unit_24_type;
            }
            ai->target_point[0] = target.area[0];
            ai->target_point[1] = target.area[1];
            ai->target_point[2] = 1.0f;
            ai->second_point[0] = target.area[2];
            ai->second_point[1] = target.area[3];
            ai->second_point[2] = 1.0f;
            ai->field_22c = target.field_1c;
            ai->field_230 = target.field_20;
        }
        if (kind != ai->victory_kind)
            aoe_ai_strategy_ai_set_victory_kind_ror(ai, kind, ops);
    }

    /* Rejected categories with a kind above 0 (none: the kinds are -2 and
       0): the best is applied and moves to the applied list. */
    best = -1;
    best_kind = 0;
    for (i = 0; i < ai->rejected.count; ++i) {
        int32_t kind = aoe_ai_sn_category_kind_ror(ai->rejected.data[i]);
        if (best_kind < kind) {
            best = ai->rejected.data[i];
            best_kind = kind;
        }
    }
    if (best != -1 && apply(ai, best, ops) == 1) {
        aoe_ai_int_list_add_unique_ror(&ai->applied, best);
        list_remove(&ai->rejected, best);
    }

    /* The applied category with the lowest priority below 101; the
       priority stub gives 0, so the first one, and it never reaches 1. */
    best = -1;
    best_kind = 101;
    for (i = 0; i < ai->applied.count; ++i) {
        const int32_t priority = 0;
        if (priority < best_kind) {
            best = ai->applied.data[i];
            best_kind = priority;
        }
    }
    if (best != -1 && best_kind == 1) {
        list_remove(&ai->applied, best);
        aoe_ai_int_list_add_unique_ror(&ai->rejected, best);
    }

    /* Categories of kind -1 are applied again (none). */
    for (i = 0; i < ai->categories.count; ++i)
        if (aoe_ai_sn_category_kind_ror(ai->categories.data[i]) == -1)
            apply(ai, ai->categories.data[i], ops);
    return 1;
}

int aoe_ai_strategy_ai_save_ror(const AoeAiStrategyRor *ai,
                                AoeOutcomeStream *out) {
    const AoeAiIntListRor *lists[3];
    size_t len;
    int k, i;
    if (!ai || !out) return 0;
    aoe_stream_put_i32(out, ai->victory_kind);
    aoe_stream_put_i32(out, ai->target_id);
    aoe_stream_put_i32(out, ai->second_id);
    aoe_stream_put_i32(out, ai->second_type);
    for (i = 0; i < 4; ++i) aoe_stream_put_f32(out, ai->target_point[i]);
    for (i = 0; i < 4; ++i) aoe_stream_put_f32(out, ai->second_point[i]);
    aoe_stream_put_i32(out, ai->field_22c);
    aoe_stream_put_i32(out, ai->field_230);
    aoe_stream_put_i32(out, ai->update_count);
    aoe_stream_put_i32(out, ai->difficulty);
    len = strnlen(ai->sn_file, sizeof ai->sn_file);
    aoe_stream_put_i32(out, (int32_t)len);
    aoe_stream_put(out, ai->sn_file, len);
    lists[0] = &ai->categories;
    lists[1] = &ai->applied;
    lists[2] = &ai->rejected;
    for (k = 0; k < 3; ++k) {
        aoe_stream_put_i32(out, lists[k]->count);
        for (i = 0; i < lists[k]->count; ++i)
            aoe_stream_put_i32(out, lists[k]->data[i]);
    }
    return !out->failed;
}

int aoe_ai_strategy_ai_load_ror(AoeAiStrategyRor *ai, int32_t player,
                                float version, AoeOutcomeStream *in,
                                int32_t settings_flag,
                                const AoeAiTextFileOpsRor *files,
                                AoeAiLogRor *log) {
    AoeAiIntListRor *lists[3];
    const uint8_t *bytes;
    int32_t len, n, i;
    int k;
    char name[256];
    if (!ai || !in) return 0;
    memset(ai, 0, sizeof(*ai));
    aoe_ai_strategy_init_ror(&ai->module, player, log);
    strategy_fields(ai);
    ai->victory_kind = aoe_stream_i32(in);
    ai->target_id = aoe_stream_i32(in);
    ai->second_id = aoe_stream_i32(in);
    ai->second_type = aoe_stream_i32(in);
    for (i = 0; i < 4; ++i) ai->target_point[i] = aoe_stream_f32(in);
    for (i = 0; i < 4; ++i) ai->second_point[i] = aoe_stream_f32(in);
    ai->field_22c = aoe_stream_i32(in);
    ai->field_230 = aoe_stream_i32(in);
    ai->update_count = aoe_stream_i32(in);
    if (version >= 1.5f) ai->difficulty = aoe_stream_i32(in);
    len = aoe_stream_i32(in);
    /* The original reads `len` bytes into the 256-byte name field. */
    if (len < 0 || len > 255) {
        in->failed = 1;
        return 0;
    }
    bytes = aoe_stream_take(in, (size_t)len);
    if (bytes) memcpy(ai->sn_file, bytes, (size_t)len);
    ai->sn_file[len] = '\0';
    memcpy(name, ai->sn_file, sizeof name);
    aoe_ai_strategy_ai_load_sn_file_ror(ai, name, files);
    aoe_ai_strategy_ai_scale_ror(ai, ai->difficulty, settings_flag);
    lists[0] = &ai->categories;
    lists[1] = &ai->applied;
    lists[2] = &ai->rejected;
    for (k = 0; k < 3; ++k) {
        n = aoe_stream_i32(in);
        for (i = 0; i < n && !in->failed; ++i)
            aoe_ai_int_list_add_unique_ror(lists[k], aoe_stream_i32(in));
    }
    return !in->failed;
}
