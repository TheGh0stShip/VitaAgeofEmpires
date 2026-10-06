#include "ai_tactics.h"

#include <math.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"
#include "aoe_stream.h"

int32_t aoe_ai_group_find_action_ror(AoeAiGroupRor *group, uint32_t field_0,
                                     uint32_t code, int32_t rule) {
    AoeAiRuleRor *r;
    if (!group || field_0 >= 5 || rule < 0 || rule >= 10) return -1;
    r = &group->rules[rule];
    for (int32_t i = 0; i < AOE_AI_RULE_ACTIONS; ++i)
        if (r->actions[i].field_0 == field_0 && r->actions[i].code == code)
            return i;
    return -1;
}

static int32_t lookup(const char *name, const char *const *names, int n) {
    if (!name) return 0;
    for (int i = 0; i < n; ++i)
        if (!strcmp(name, names[i])) return i + 1;
    return 0;
}

uint8_t aoe_ai_attack_kind_ror(const char *name) {
    static const char *const names[] = {"Attack", "Annoy", "Flank", "Recon"};
    return (uint8_t)lookup(name, names, 4);
}

int32_t aoe_ai_speed_kind_ror(const char *name) {
    static const char *const names[] = {"Stationary", "Moving", "Slow", "Fast",
                                        "Any"};
    return lookup(name, names, 5);
}

int32_t aoe_ai_any_kind_ror(const char *name) {
    return name && !strcmp(name, "Any") ? 5 : 0;
}

int32_t aoe_ai_members_total_ror(const AoeAiMembersRor *m, int32_t c) {
    return m && c >= 0 && c < 5 ? m->totals[c] : -1;
}

void aoe_ai_members_set_total_ror(AoeAiMembersRor *m, int32_t c,
                                  int32_t value) {
    if (m && c >= 0 && c < 5) m->totals[c] = value;
}

uint8_t aoe_ai_members_count_ror(const AoeAiMembersRor *m) {
    uint8_t n = 0;
    for (int i = 0; m && i < AOE_AI_MEMBERS; ++i) n += m->slots[i].id != -1;
    return n;
}

uint8_t aoe_ai_members_count_category_ror(const AoeAiMembersRor *m,
                                          uint32_t category) {
    uint8_t n = 0;
    for (int i = 0; m && i < AOE_AI_MEMBERS; ++i)
        n += m->slots[i].id != -1 && m->slots[i].category == category;
    return n;
}

uint8_t aoe_ai_members_count_role_ror(const AoeAiMembersRor *m,
                                      uint32_t category, uint32_t role) {
    uint8_t n = 0;
    for (int i = 0; m && i < AOE_AI_MEMBERS; ++i)
        n += m->slots[i].id != -1 && m->slots[i].category == category &&
             m->slots[i].role == role;
    return n;
}

uint8_t aoe_ai_members_category_ror(const AoeAiMembersRor *m, int32_t id) {
    if (!m || id < 0) return 0xff;
    for (int i = 0; i < AOE_AI_MEMBERS; ++i)
        if (m->slots[i].id == id) return m->slots[i].category;
    return 0xff;
}

int aoe_ai_members_add_ror(AoeAiMembersRor *m, int32_t id, uint8_t category,
                           uint8_t role) {
    for (int i = 0; m && i < AOE_AI_MEMBERS; ++i)
        if (m->slots[i].id == -1) {
            m->slots[i].id = id;
            m->slots[i].category = category;
            m->slots[i].role = role;
            return 1;
        }
    return 0;
}

int32_t aoe_ai_members_ids_ror(const AoeAiMembersRor *m, int32_t *out) {
    int32_t n = 0;
    for (int i = 0; m && out && i < AOE_AI_MEMBERS; ++i)
        if (m->slots[i].id != -1) out[n++] = m->slots[i].id;
    return n;
}

int aoe_ai_members_load_ror(AoeAiMembersRor *m, AoeOutcomeStream *in) {
    const uint8_t *p;
    if (!m || !in || !in->data) return 0;
    m->field_190 = aoe_stream_i32(in);
    m->field_194 = aoe_stream_i32(in);
    m->field_198 = aoe_stream_i32(in);
    m->field_19c = aoe_stream_i32(in);
    m->field_1a0 = aoe_stream_i32(in);
    for (int i = 0; i < 5; ++i) m->totals[i] = aoe_stream_i32(in);
    m->field_1b8 = aoe_stream_u8(in);
    if ((p = aoe_stream_take(in, 400)))
        for (int i = 0; i < AOE_AI_MEMBERS; ++i) {
            const uint8_t *s = p + 8 * i;
            m->slots[i].id = aoe_le_i32(s);
            m->slots[i].category = s[4];
            m->slots[i].role = s[5];
            m->slots[i].pad[0] = s[6];
            m->slots[i].pad[1] = s[7];
        }
    m->field_1bc = aoe_stream_i32(in);
    m->field_1c0 = aoe_stream_i32(in);
    return !in->failed;
}

int aoe_ai_members_save_ror(const AoeAiMembersRor *m, AoeOutcomeStream *out) {
    if (!m || !out || !out->data) return 0;
    aoe_stream_put_i32(out, m->field_190);
    aoe_stream_put_i32(out, m->field_194);
    aoe_stream_put_i32(out, m->field_198);
    aoe_stream_put_i32(out, m->field_19c);
    aoe_stream_put_i32(out, m->field_1a0);
    for (int i = 0; i < 5; ++i) aoe_stream_put_i32(out, m->totals[i]);
    aoe_stream_put_u8(out, m->field_1b8);
    for (int i = 0; i < AOE_AI_MEMBERS; ++i) {
        uint8_t s[8];
        aoe_le_put_u32(s, (uint32_t)m->slots[i].id);
        s[4] = m->slots[i].category;
        s[5] = m->slots[i].role;
        s[6] = m->slots[i].pad[0];
        s[7] = m->slots[i].pad[1];
        aoe_stream_put(out, s, 8);
    }
    aoe_stream_put_i32(out, m->field_1bc);
    aoe_stream_put_i32(out, m->field_1c0);
    return !out->failed;
}

void aoe_ai_members_purge_ror(AoeAiMembersRor *m, AoeAiUnitFn unit,
                              void *context) {
    for (int i = 0; m && unit && i < AOE_AI_MEMBERS; ++i) {
        uint8_t state = 0;
        float hp;
        if (m->slots[i].id == -1) continue;
        if (!unit(context, m->slots[i].id, &state, &hp) || state > 2) {
            m->slots[i].id = -1;
            m->slots[i].category = 0xff;
            m->slots[i].role = 0xff;
        }
    }
}

void aoe_ai_members_sum_hp_ror(AoeAiMembersRor *m, uint32_t category,
                               AoeAiUnitFn unit, void *context) {
    int32_t sum = 0;
    if (!m || !unit) return;
    for (int i = 0; i < AOE_AI_MEMBERS; ++i) {
        uint8_t state;
        float hp = 0.0f;
        if (m->slots[i].id != -1 && m->slots[i].category == category &&
            unit(context, m->slots[i].id, &state, &hp))
            sum += aoe_ftol32(hp);
    }
    aoe_ai_members_set_total_ror(m, (int32_t)category, sum);
}

void aoe_ai_play_list_init_ror(AoeAiPlayListRor *list) {
    if (!list) return;
    list->count = 0;
    list->plays = NULL;
}

void aoe_ai_play_list_release_ror(AoeAiPlayListRor *list) {
    if (!list || !list->plays) return;
    list->count = 0;
    free(list->plays);
    list->plays = NULL;
}

AoeAiPlayRor *aoe_ai_play_list_get_ror(AoeAiPlayListRor *list, int32_t i) {
    if (!list || !list->plays || i < 0 || i >= list->count) return NULL;
    return &list->plays[i];
}

void aoe_ai_counters_init_ror(AoeAiCountersRor *c, int32_t count) {
    int32_t i = 0;
    if (!c) return;
    c->field_4 = c->field_8 = 0;
    c->count = count;
    for (; i < count && i < 8; ++i) {
        c->values[i] = 0;
        c->second[i] = c->third[i] = -1;
    }
    for (; i < 8; ++i) c->values[i] = c->second[i] = c->third[i] = -1;
}

void aoe_ai_counters_release_ror(AoeAiCountersRor *c) { (void)c; }

int32_t aoe_ai_counters_get_ror(const AoeAiCountersRor *c, int32_t i) {
    return c && i >= 0 && i < c->count && i < 8 ? c->values[i] : -1;
}

void aoe_ai_counters_set_ror(AoeAiCountersRor *c, int32_t i, int32_t v) {
    if (c && i >= 0 && i < c->count && i < 8) c->values[i] = v;
}

void aoe_ai_counters_add_ror(AoeAiCountersRor *c, int32_t i, int32_t v) {
    if (c && i >= 0 && i < c->count && i < 8) c->values[i] += v;
}

/* 0x004f2040 (after 0x004eabc1) */
void aoe_ai_counters_add_costs_ror(AoeAiCountersRor *c, const int16_t types[3],
                                   const int16_t amounts[3]) {
    for (int i = 0; i < 3; ++i) aoe_ai_counters_add_ror(c, types[i], amounts[i]);
}

void aoe_ai_counters_fill_ror(AoeAiCountersRor *c, int32_t v) {
    for (int i = 0; c && i < 8; ++i) c->values[i] = v;
}

int32_t aoe_ai_counters_count_ror(const AoeAiCountersRor *c) {
    return c ? c->count : 0;
}

void aoe_ai_strategy_init_ror(AoeAiModuleRor *ai, int32_t player,
                              AoeAiLogRor *log) {
    aoe_ai_module_init_ror(ai, "Strategy AI", 0x3f0, player, log);
}

void aoe_ai_strategy_release_ror(AoeAiModuleRor *ai) {
    aoe_ai_module_release_ror(ai);
}

void aoe_ai_tac_init_ror(AoeAiModuleRor *ai, int32_t player,
                         AoeAiLogRor *log) {
    aoe_ai_module_init_ror(ai, "TacAI", 0x3f1, player, log);
}

void aoe_ai_tac_release_ror(AoeAiModuleRor *ai) {
    aoe_ai_module_release_ror(ai);
}

void aoe_ai_trade_init_ror(AoeAiTradeRor *ai, int32_t player,
                           AoeAiLogRor *log) {
    if (!ai) return;
    aoe_ai_module_init_ror(&ai->module, "Trade AI", 0x3f2, player, log);
    ai->field_f0 = 0;
}

void aoe_ai_trade_load_ror(AoeAiTradeRor *ai, int32_t player,
                           AoeAiLogRor *log) {
    aoe_ai_trade_init_ror(ai, player, log);
}

void aoe_ai_trade_release_ror(AoeAiTradeRor *ai) {
    if (ai) aoe_ai_module_release_ror(&ai->module);
}

int32_t aoe_ai_unit_default_c0_ror(void) { return -1; }

void aoe_ai_members_init_ror(AoeAiMembersRor *m) {
    if (!m) return;
    memset(m, 0, sizeof *m);
    m->field_190 = m->field_194 = -1;
    m->field_1b8 = 0xfd;
    m->field_1bc = -1;
    for (int i = 0; i < AOE_AI_MEMBERS; ++i) {
        m->slots[i].id = -1;
        m->slots[i].category = m->slots[i].role = 0xff;
    }
    for (int i = 0; i < 5; ++i) m->totals[i] = -1;
}

static int members_full(const AoeAiMembersRor *m, const AoeAiGroupRor *play) {
    return aoe_ai_members_count_ror(m) == play->field_42;
}

int aoe_ai_members_fill_ror(AoeAiMembersRor *m, uint8_t mode,
                            AoeAiGroupRor *play, const int32_t *ids,
                            int32_t count, AoeAiCategoryFn category,
                            void *context) {
    int32_t bucket[6][AOE_AI_MEMBERS], per[6] = {0}, r, c, j;
    int ok = 1;
    uint8_t shortfall = 0;
    if (!m || !play) return 0;
    for (j = 0; j < count; ++j) {
        int32_t cat = category ? category(context, ids[j]) : -1;
        if (cat < 0 || cat >= 6 || per[cat] >= AOE_AI_MEMBERS) {
            ok = 0;
            continue;
        }
        bucket[cat][per[cat]++] = ids[j];
    }
    if (play->range_count != 0) {
        for (r = 0;; ++r) {
            AoeAiCountRangeRor *range = aoe_ai_group_range_ror(play, r);
            if (!range) return 0;
            for (c = 0; c < 6; ++c) {
                uint8_t min = aoe_ai_range_min_ror(range, c);
                int32_t taken = 0;
                if (!min) continue;
                for (j = 0; j < per[c] && taken < min; ++j) {
                    if (bucket[c][j] == -1) continue;
                    if (aoe_ai_members_add_ror(m, bucket[c][j], (uint8_t)r, (uint8_t)c) == 1) {
                        ++taken;
                        bucket[c][j] = -1;
                        if (members_full(m, play)) goto done;
                    } else {
                        ok = 0;
                    }
                }
            }
            if (r + 1 >= play->range_count) break;
        }
    }
    for (c = 0; c < 6; ++c) {
        for (j = 0; j < per[c]; ++j) {
            int32_t id = bucket[c][j];
            if (id == -1) continue;
            if (mode == 1) {
                int32_t best = -1;
                uint8_t best_n = 0;
                for (r = 0; r < play->range_count; ++r) {
                    AoeAiCountRangeRor *range = aoe_ai_group_range_ror(play, r);
                    if (!range) continue;
                    if (aoe_ai_members_count_role_ror(m, (uint32_t)r, (uint32_t)c) <
                            aoe_ai_range_max_ror(range, c) &&
                        (best == -1 ||
                         aoe_ai_members_count_category_ror(m, (uint32_t)r) < best_n)) {
                        best_n = aoe_ai_members_count_category_ror(m, (uint32_t)r);
                        best = r;
                    }
                }
                if (best == -1) continue;
                if (aoe_ai_members_add_ror(m, id, (uint8_t)best, (uint8_t)c) == 1) {
                    bucket[c][j] = -1;
                    if (members_full(m, play)) goto done;
                } else {
                    ok = 0;
                }
            } else if (mode == 2) {
                for (r = 0; r < play->range_count; ++r) {
                    AoeAiCountRangeRor *range = aoe_ai_group_range_ror(play, r);
                    if (!range ||
                        !(aoe_ai_members_count_role_ror(m, (uint32_t)r, (uint32_t)c) <
                          aoe_ai_range_max_ror(range, c)))
                        continue;
                    if (aoe_ai_members_add_ror(m, id, (uint8_t)r, (uint8_t)c) == 1) {
                        bucket[c][j] = -1;
                        if (members_full(m, play)) goto done;
                        break;
                    }
                    ok = 0;
                }
            }
        }
    }
done:
    for (r = 0; r < play->range_count; ++r) {
        AoeAiCountRangeRor *range = aoe_ai_group_range_ror(play, r);
        if (!range) continue;
        for (c = 0; c < 6; ++c) {
            uint8_t min = aoe_ai_range_min_ror(range, c);
            uint8_t have = aoe_ai_members_count_role_ror(m, (uint32_t)r, (uint32_t)c);
            if (have < min) shortfall = (uint8_t)(shortfall + (min - have));
        }
    }
    m->shortfall = shortfall;
    return ok;
}

void aoe_ai_point_zero_ror(AoeAiPointRor *p) {
    if (p) p->x = p->y = p->z = 0;
}

void aoe_ai_point_set_ror(AoeAiPointRor *p, int32_t x, int32_t y, int32_t z) {
    if (!p) return;
    p->x = x;
    p->y = y;
    p->z = z;
}

void aoe_ai_point_rotate_ror(const AoeAiPointRor *p, const AoeAiPointRor *origin,
                             AoeAiPointRor *out, uint8_t turns) {
    double a, sn, cs, dx, dy;
    if (!p || !out) return;
    *out = *p;
    if (!turns || !origin) return;
    a = (double)turns * (double)0.7853981852531433f;
    sn = (double)(float)sin(a);
    cs = cos(a);
    dx = (double)(p->x - origin->x);
    dy = (double)(p->y - origin->y);
    out->x = aoe_ftol32(dx * cs - dy * sn);
    out->y = aoe_ftol32(dx * sn + dy * cs);
}

/* ---- 0x0040d840: the play file loader. ---- */

#define PLAY_FILE_MAX 16000

_Static_assert(sizeof(AoeAiGroupRor) <= sizeof(AoeAiPlayRor),
               "a play holds a group template");

static AoeAiGroupRor *play_at(AoeAiPlayListRor *list, int32_t i) {
    if (i < 0 || i >= list->count) return NULL; /* the original indexes */
    return (AoeAiGroupRor *)&list->plays[i];
}

static int32_t clamp_percent(int32_t v) {
    return v < 0 ? 0 : v > 100 ? 100 : v;
}

/* 0x0040d840 first pass: eight plays per "Play" line, at most 16000. */
static int32_t count_plays(const AoeAiPlayLoadOpsRor *ops, void *file) {
    char line[256], word[256];
    int32_t count = 0;
    word[0] = '\0';
    while (ops->gets(ops->context, file, line, sizeof line)) {
        if (!line[0]) continue;
        sscanf(line, "%255s ", word); /* kept when nothing is read */
        if (!strcmp(word, "Play")) count += 8;
    }
    return count > PLAY_FILE_MAX ? PLAY_FILE_MAX : count;
}

/* "Play": the text between the first two '"' names the next play. */
static void quoted_name(const char *line, char *out) {
    const char *a = strchr(line, '"'), *b;
    size_t n = 0;
    if (a) {
        b = strchr(a + 1, '"');
        if (b) n = (size_t)(b - a - 1);
        else n = strlen(a + 1); /* the original runs past the line */
        memcpy(out, a + 1, n);
    }
    out[n] = '\0';
}

/* "EndPlay": an "HP" play is kept as it is (the seven spare plays are
   dropped from the count); any other is copied into the next seven
   plays named "<name>F1".."F7", with each Move action's point rotated
   about (0, 0, 0) by k eighths of a turn. Returns the new play index. */
static int32_t expand_play(AoeAiPlayListRor *list, int32_t play) {
    AoeAiGroupRor *base = play_at(list, play);
    char name[256];
    int32_t k;
    if (!base) return play;
    strcpy(name, base->name);
    if (name[0] == 'H' && name[1] == 'P') {
        list->count -= 7;
        return play;
    }
    for (k = 1; k < 8; k++) {
        AoeAiGroupRor *dst = play_at(list, play + k);
        char fname[300];
        int32_t i, r, c;
        if (!dst) break;
        snprintf(fname, sizeof fname, "%sF%d", base->name, (int)k);
        aoe_ai_group_set_name_ror(dst, fname);
        dst->field_43 = base->field_43;
        dst->field_41 = base->field_41;
        dst->field_42 = base->field_42;
        dst->field_4e = base->field_4e;
        dst->mode = base->mode;
        dst->field_51 = base->field_51;
        dst->field_50 = base->field_50;
        for (i = 0; i < 5; i++) {
            aoe_ai_group_add_slot_44_ror(dst, aoe_ai_group_slot_44_ror(base, i));
            aoe_ai_group_add_slot_49_ror(dst, aoe_ai_group_slot_49_ror(base, i));
        }
        dst->range_count = base->range_count;
        for (r = 0; r < AOE_AI_GROUP_RANGES; r++) {
            AoeAiCountRangeRor *from = aoe_ai_group_range_ror(base, r);
            AoeAiCountRangeRor *to = aoe_ai_group_range_ror(dst, r);
            for (c = 0; c < 6; c++) {
                aoe_ai_range_set_min_ror(to, c, aoe_ai_range_min_ror(from, c));
                aoe_ai_range_set_max_ror(to, c, aoe_ai_range_max_ror(from, c));
            }
        }
        for (r = 0; r < AOE_AI_GROUP_RULES; r++) {
            AoeAiRuleRor *from = aoe_ai_group_rule_ror(base, r);
            AoeAiRuleRor *to = aoe_ai_group_rule_ror(dst, r);
            for (i = 0; i < AOE_AI_RULE_ACTIONS; i++) {
                const AoeAiActionRor *act = aoe_ai_rule_action_ror(from, i);
                AoeAiActionRor moved;
                if (act->code == 1) {
                    AoeAiPointRor origin, out, p;
                    aoe_ai_point_set_ror(&origin, 0, 0, 0);
                    aoe_ai_point_zero_ror(&out);
                    aoe_ai_point_set_ror(&p, act->a, act->b, act->c);
                    aoe_ai_point_rotate_ror(&p, &origin, &out, (uint8_t)k);
                    aoe_ai_action_set_ror(&moved, act->field_0, act->code,
                                          out.x, out.y, out.z);
                    act = &moved;
                }
                aoe_ai_rule_add_action_ror(to, act);
            }
            for (i = 0; i < AOE_AI_RULE_TRIGGERS; i++)
                aoe_ai_rule_add_trigger_ror(to, aoe_ai_rule_trigger_ror(from, i));
        }
    }
    return play + 7;
}

typedef struct {
    int32_t play, rule; /* -1 outside a play / rule phase */
    char c1, c2;        /* kept between lines, like the original */
} PlayParse;

/* One line of the second pass. Returns 0 when the play limit stops it. */
static int parse_play_line(AoeAiPlayListRor *list,
                           const AoeAiPlayLoadOpsRor *ops, PlayParse *st,
                           const char *line) {
    char word[256], text[256];
    int v = 0, a = 0, b = 0, c = 0, d = 0;
    int in_play = st->play != -1;
    AoeAiGroupRor *play;
    word[0] = text[0] = '\0';
    if (sscanf(line, " %c%c", &st->c1, &st->c2) == EOF) return 1;
    if (st->c1 == '/' && st->c2 == '/') return 1;
    sscanf(line, "%255s ", word);
    play = play_at(list, st->play);
    if (!strcmp(word, "Play")) {
        quoted_name(line, word);
        if (++st->play > PLAY_FILE_MAX - 1) return 0;
        play = play_at(list, st->play);
        if (play) aoe_ai_group_set_name_ror(play, word);
    } else if (in_play && !strcmp(word, "PlayType")) {
        sscanf(line, "%*s %255s ", word);
        if (play) play->field_43 = aoe_ai_attack_kind_ror(word);
    } else if (in_play && !strcmp(word, "MinUnits")) {
        sscanf(line, "%*s %d ", &v);
        if (play) play->field_41 = (uint8_t)v;
    } else if (in_play && !strcmp(word, "MaxUnits")) {
        sscanf(line, "%*s %d ", &v);
        if (play) play->field_42 = (uint8_t)v;
    } else if (in_play && !strcmp(word, "Power")) {
        sscanf(line, "%*s %d ", &v);
        if (play) play->field_4e = (uint8_t)v;
    } else if (in_play && !strcmp(word, "Overflow")) {
        sscanf(line, "%*s %255s ", word);
        if (play && !strcmp(word, "Balance")) play->mode = 1;
        else if (play && !strcmp(word, "FirstGroup")) play->mode = 2;
    } else if (in_play && !strcmp(word, "DeathPercentage")) {
        sscanf(line, "%*s %d ", &v);
        if (play) play->field_51 = (uint8_t)clamp_percent(v);
    } else if (in_play && !strcmp(word, "Intelligence")) {
        sscanf(line, "%*s %d ", &v);
        if (play) play->field_50 = (uint8_t)clamp_percent(v);
    } else if (in_play && !strcmp(word, "TargetChar")) {
        int32_t id;
        sscanf(line, "%*s %255s ", text);
        id = ops->target_char ? ops->target_char(ops->context, text) : 0;
        if (id && play) aoe_ai_group_add_slot_49_ror(play, (uint8_t)id);
    } else if (in_play && !strcmp(word, "TargetType")) {
        int32_t id;
        sscanf(line, "%*s %255s ", text);
        id = ops->target_type ? ops->target_type(ops->context, text) : 0;
        if (id && play) aoe_ai_group_add_slot_44_ror(play, (uint8_t)id);
    } else if (in_play && !strcmp(word, "NumberGroups")) {
        sscanf(line, "%*s %d ", &v);
        if (play) play->range_count = (uint8_t)v;
    } else if (in_play && st->rule == -1 && !strcmp(word, "Group")) {
        /* Group <range> <category> <min> - <max>: a negative maximum
           also zeroes the minimum; the maximum is capped at 50. */
        int32_t cat;
        sscanf(line, "%*s %d %255s %d - %d ", &v, text, &a, &b);
        if (v < 0 || v >= AOE_AI_GROUP_RANGES || !ops->category) return 1;
        cat = ops->category(ops->context, text);
        if (cat == -1 || !play) return 1;
        aoe_ai_range_set_min_ror(aoe_ai_group_range_ror(play, v), cat,
                                 (uint8_t)(b < 0 ? 0 : a));
        aoe_ai_range_set_max_ror(aoe_ai_group_range_ror(play, v), cat,
                                 (uint8_t)(b > 50 ? 50 : b));
    } else if (in_play && !strcmp(word, "Phase")) {
        sscanf(line, "%*s %d ", &v);
        st->rule = v;
    } else if (in_play && st->rule != -1 && !strcmp(word, "Group")) {
        /* Group <field_0 0..4> <action> <a> <b> <c> into the phase. */
        AoeAiRuleRor *rule = play ? aoe_ai_group_rule_ror(play, st->rule) : NULL;
        AoeAiActionRor act;
        sscanf(line, "%*s %d %255s %d %d %d ", &v, text, &a, &b, &c);
        if (v < 0 || v >= 5 || !rule) return 1;
        aoe_ai_action_set_named_ror(&act, (uint8_t)v, text, a, b, c);
        aoe_ai_rule_add_action_ror(rule, &act);
    } else if (in_play && st->rule != -1 &&
               (!strcmp(word, "Trigger") || !strcmp(word, "RTrigger"))) {
        /* [R]Trigger <n> <trigger> <a> <b> [<e>]: stored with the phase
           number as its +0xc byte and n as +0xd. */
        AoeAiRuleRor *rule = play ? aoe_ai_group_rule_ror(play, st->rule) : NULL;
        AoeAiTriggerRor trig;
        if (word[0] == 'R')
            sscanf(line, "%*s %d %255s %d %d %d ", &v, text, &a, &d, &c);
        else
            sscanf(line, "%*s %d %255s %d %d ", &v, text, &a, &d);
        if (!rule) return 1;
        aoe_ai_trigger_set_named_ror(&trig, text, a, d, (uint8_t)st->rule,
                                     (uint8_t)v, c);
        aoe_ai_rule_add_trigger_ror(rule, &trig);
    } else if (in_play && !strcmp(word, "EndPlay")) {
        st->rule = -1;
        st->play = expand_play(list, st->play);
    }
    return 1;
}

/* 0x0040d840 */
int aoe_ai_play_list_load_ror(AoeAiPlayListRor *list, const char *name,
                              const AoeAiPlayLoadOpsRor *ops) {
    void *file;
    int32_t i, count;
    char path[256], line[256];
    PlayParse st;
    if (!list) return 0;
    if (!name || !strcmp(name, "NONE")) {
        strcpy(list->name, name ? "NONE" : "NULL");
        return 0;
    }
    strncpy(list->name, name, 0x40);
    list->name[0x40] = '\0';
    snprintf(path, sizeof path, "%s", list->name);
    if (!ops || !ops->open || !ops->gets) return 0;
    file = ops->open(ops->context, path);
    if (!file) return 0;
    free(list->plays);
    list->plays = NULL;
    list->count = 0;
    count = count_plays(ops, file);
    list->count = count;
    list->plays = calloc(count ? (size_t)count : 1u, sizeof(AoeAiPlayRor));
    if (!list->plays) return 0; /* the file stays open, as in the original */
    for (i = 0; i < count; i++)
        aoe_ai_group_init_ror((AoeAiGroupRor *)&list->plays[i]);
    if (ops->rewind) ops->rewind(ops->context, file);
    st.play = st.rule = -1;
    st.c1 = st.c2 = 0;
    do {
        if (!ops->gets(ops->context, file, line, sizeof line)) break;
        if (line[0] && !parse_play_line(list, ops, &st, line)) break;
    } while (st.play < PLAY_FILE_MAX);
    if (ops->close) ops->close(ops->context, file);
    return 1;
}
