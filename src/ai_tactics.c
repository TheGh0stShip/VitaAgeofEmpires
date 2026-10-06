#include "ai_tactics.h"

#include <math.h>

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
