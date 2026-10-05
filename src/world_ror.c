#include "world_ror.h"

#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

#define R(w) ((AoeRorWorldRor *)(w))
#define ROPS (w->ror)
#define RCTX (w->ror->context)

AoeWorldVtblRor aoe_ror_world_vtable_ror;

static void clear_victory(AoeRorWorldRor *w) {
    w->victory = 0;
    w->relics = 0;
    w->ruins = 0;
    w->time_limit = 0;
    w->time_left = 0.0f;
    w->field_11c = 0;
}

void aoe_ror_world_release_ror(AoeRorWorldRor *w, FILE **ai_log) {
    if (w->random_map) {
        ROPS->rm_free(RCTX, w->random_map);
        w->random_map = NULL;
    }
    aoe_world_release_ror(&w->base, ai_log);
}

int aoe_ror_world_load_text_ror(AoeWorldBaseRor *base, FILE *f) {
    AoeRorWorldRor *w = R(base);
    char name[0x100];
    aoe_world_load_text_ror(base, f);
    name[0] = '\0';
    if (fscanf(f, "%255s ", name) != 1) name[0] = '\0';
    w->random_map = ROPS->rm_text(RCTX, name);
    clear_victory(w);
    return 1;
}

void aoe_ror_world_load_data_ror(AoeWorldBaseRor *base, AoeOutcomeStream *s, int32_t a,
                                 int32_t b) {
    AoeRorWorldRor *w = R(base);
    aoe_world_load_data_ror(base, s, a, b);
    w->random_map = ROPS->rm_read(RCTX, s, w);
    clear_victory(w);
}

void aoe_ror_world_save_data_ror(const AoeWorldBaseRor *base, AoeOutcomeStream *s) {
    const AoeRorWorldRor *w = (const AoeRorWorldRor *)base;
    aoe_world_save_data_ror(base, s);
    ROPS->rm_write(RCTX, w->random_map, s);
}

int aoe_ror_world_load_data_file_ror(AoeWorldBaseRor *w, const char *path, int32_t a,
                                     int32_t b) {
    return aoe_world_load_data_file_ror(w, path, a, b);
}

void aoe_ror_world_civ_from_text_ror(AoeWorldBaseRor *base, int16_t id, int16_t flag, FILE *f) {
    AoeRorWorldRor *w = R(base);
    if (flag != 1) {
        aoe_world_civ_from_text_ror(base, id, flag, f);
        return;
    }
    if (id >= 0 && id < base->civ_count) base->civs[id] = ROPS->civ_text(RCTX, f);
}

int aoe_ror_world_civ_from_stream_ror(AoeWorldBaseRor *base, AoeOutcomeStream *s, int16_t id,
                                      uint8_t flag) {
    AoeRorWorldRor *w = R(base);
    if (flag != 1) return aoe_world_civ_from_stream_ror(base, s, id, flag);
    if (id >= 0 && id < base->civ_count) base->civs[id] = ROPS->civ_read(RCTX, s, base->sprites);
    return 1;
}

void aoe_ror_world_map_from_text_ror(AoeWorldBaseRor *base, const char *a, const char *b,
                                     const char *c, const char *d, int16_t x, int16_t y,
                                     int16_t z, void **sounds, const char *extra) {
    AoeRorWorldRor *w = R(base);
    base->map = ROPS->map_text(RCTX, a, b, c, d, x, y, z, sounds);
    base->ops->map_text_extra(base->ops->context, base->map, extra);
}

void aoe_ror_world_map_from_stream_ror(AoeWorldBaseRor *base, AoeOutcomeStream *s, int32_t arg) {
    AoeRorWorldRor *w = R(base);
    (void)arg;
    base->map = ROPS->map_read(RCTX, s, base->sounds, 1);
}

void aoe_ror_world_effects_from_stream_ror(AoeWorldBaseRor *base, AoeOutcomeStream *s) {
    AoeRorWorldRor *w = R(base);
    base->effects = ROPS->effects_read(RCTX, s);
}

void aoe_ror_world_slot_54_ror(AoeWorldBaseRor *base, AoeOutcomeStream *s, int32_t arg) {
    AoeRorWorldRor *w = R(base);
    (void)s;
    base->field_58 = ROPS->field58_new(RCTX, w, arg);
}

void aoe_ror_world_player_from_stream_ror(AoeRorWorldRor *w, AoeOutcomeStream *s, uint8_t kind,
                                          int16_t index,
                                          void (*load_74)(void *context, void *player,
                                                          AoeOutcomeStream *s)) {
    AoeWorldBaseRor *b = &w->base;
    void *p;
    if (kind != 1 && kind != 2 && kind != 3) {
        aoe_world_player_from_stream_ror(b, s, kind, index, load_74);
        return;
    }
    p = ROPS->player_read(RCTX, s, w, index, kind == 2);
    b->players[index] = p;
    if (p && load_74) load_74(b->ops->context, p, s);
}

void aoe_ror_world_new_gaia_ror(AoeRorWorldRor *w) {
    AoeWorldBaseRor *b = &w->base;
    b->players[0] = ROPS->gaia_new(RCTX, w, b->civs[0]);
}

void aoe_ror_world_new_players_ror(AoeRorWorldRor *w, const int8_t type[9], const int8_t civ[9],
                                   const char names[8][0x41]) {
    AoeWorldBaseRor *b = &w->base;
    int16_t i;
    for (i = 1; i < b->player_count && i < 9; ++i) {
        int computer;
        if (type[i] == 0)
            computer = 0;
        else if (type[i] == 4)
            computer = ROPS->computer_flag != 0;
        else
            continue;
        b->players[i] = ROPS->player_new(RCTX, w, b->civs[(int)civ[i]], i, names[i - 1],
                                         (uint8_t)civ[i], computer);
    }
}

void aoe_ror_world_new_player_ror(AoeRorWorldRor *w, int16_t index, uint8_t civ, uint8_t kind,
                                  const char *name) {
    AoeWorldBaseRor *b = &w->base;
    b->players[index] = ROPS->player_new(RCTX, w, b->civs[civ], index, name, civ, kind == 4);
}

void aoe_ror_world_scenario_new_ror(AoeRorWorldRor *w, const void *info) {
    w->base.scenario = ROPS->scenario_new(RCTX, info);
}

void aoe_ror_world_scenario_from_stream_ror(AoeRorWorldRor *w, AoeOutcomeStream *s,
                                            int32_t arg) {
    w->base.scenario = ROPS->scenario_read(RCTX, s, arg);
}

void aoe_ror_world_save_state_ror(AoeRorWorldRor *w, AoeOutcomeStream *s) {
    aoe_stream_put_i32(s, w->victory);
    aoe_stream_put_i32(s, w->relics);
    aoe_stream_put_i32(s, w->ruins);
    aoe_stream_put_u8(s, w->time_limit);
    aoe_stream_put_f32(s, w->time_left);
    aoe_stream_put_u8(s, w->field_11c);
    aoe_stream_put_u8(s, w->field_11d);
}

int aoe_ror_world_load_state_ror(AoeRorWorldRor *w, AoeOutcomeStream *s, int loaded_base) {
    if (!loaded_base) return 0;
    w->victory = w->version < 5.6f ? 0 : aoe_stream_i32(s);
    if (w->version < 5.3f) {
        w->relics = 0;
        w->ruins = 0;
    } else {
        w->relics = aoe_stream_i32(s);
        w->ruins = aoe_stream_i32(s);
    }
    if (w->version < 5.4f) {
        w->time_limit = 0;
        w->time_left = 0.0f;
    } else {
        w->time_limit = aoe_stream_u8(s);
        w->time_left = aoe_stream_f32(s);
    }
    w->field_11c = w->version < 6.6f ? 0 : aoe_stream_u8(s);
    w->field_11d = w->version >= 6.97f ? aoe_stream_u8(s) : 2;
    return 1;
}

void aoe_ror_world_ai_log_header_ror(AoeRorWorldRor *w, FILE *f, int first) {
    char text[0x100];
    const char *scenario;
    if (!f || first != 1) return;
    fprintf(f, "\n\n\n*****************************************************\n");
    fprintf(f, "AI Log file for Game Started %s", ROPS->time_text(RCTX));
    fprintf(f, "Number Players:          %d\n", w->base.player_count - 1);
    scenario = ROPS->scenario_name(RCTX);
    if (scenario && scenario[0]) {
        fprintf(f, "Scenario:                %s\n", scenario);
        return;
    }
    ROPS->string(RCTX, ROPS->map_size(RCTX) + 0x2973, text, sizeof text);
    fprintf(f, "Map Size:                %s\n", text);
    ROPS->string(RCTX, ROPS->map_type(RCTX) + 0x296a, text, sizeof text);
    fprintf(f, "Map Type:                %s\n", text);
    ROPS->string(RCTX, ROPS->victory_kind(RCTX) + 0x28a6, text, sizeof text);
    fprintf(f, "Victory Condition:       %s\n", text);
}

void aoe_ror_world_editor_resources_ror(AoeRorWorldRor *w) {
    int i;
    for (i = 1; i < w->base.player_count; ++i) {
        void *p = w->base.players[i];
        ROPS->set_attribute(RCTX, p, 0, 200.0f);
        ROPS->set_attribute(RCTX, p, 1, 200.0f);
        ROPS->set_attribute(RCTX, p, 3, 0.0f);
        ROPS->set_attribute(RCTX, p, 2, 100.0f);
    }
}

/* Mutually allied (stance 0 both ways). */
static int allied(AoeRorWorldRor *w, int32_t a, int32_t b) {
    void **p = w->base.players;
    return ROPS->stance(RCTX, p[a], b) == 0 && ROPS->stance(RCTX, p[b], a) == 0;
}

static void time_limit(AoeRorWorldRor *w) {
    AoeWorldBaseRor *b = &w->base;
    int32_t score[9], i, j, best = -1;
    void *winner = NULL;
    for (i = 0; i < 9; ++i) score[i] = -1;
    for (i = 1; i < b->player_count && i < 9; ++i) {
        int32_t members = 1;
        score[i] = ROPS->score(RCTX, b->players[i]);
        if (ROPS->alive(RCTX, b->players[i])) {
            for (j = 1; j < b->player_count; ++j) {
                if (j == i || !allied(w, i, j) || !ROPS->alive(RCTX, b->players[j])) continue;
                score[i] += ROPS->score(RCTX, b->players[j]);
                ++members;
            }
        }
        score[i] /= members;
    }
    for (i = 1; i < b->player_count && i < 9; ++i) {
        if (best < score[i]) {
            winner = b->players[i];
            best = score[i];
        }
    }
    if (b->player_count > 1 && winner) {
        ROPS->win(RCTX, winner);
        b->field_1e = 0x67;
    }
}

/* The relic (attribute 7, timer 0) or ruin (attribute 14, timer 1)
   hold for each team. */
static void hold_check(AoeRorWorldRor *w, int32_t need, int which, void *teams[9][9],
                       const int32_t sizes[9], int32_t team_count) {
    static const int attr_count[2] = {7, 14};
    static const int attr_flag[2] = {55, 54};
    static const int32_t ev_start[2] = {0x72, 0x74};
    static const int32_t ev_lost[2] = {0x73, 0x75};
    static const uint8_t ends[2] = {0x65, 0x66};
    int32_t t, k;
    for (t = 0; t < team_count; ++t) {
        int32_t held = 0;
        void *leader = teams[t][0];
        for (k = 0; k < sizes[t]; ++k)
            held += (int32_t)ROPS->attribute(RCTX, teams[t][k], attr_count[which]);
        if (held == need) {
            float left = ROPS->timer(RCTX, leader, which);
            if (left == -1.0f) {
                for (k = 0; k < sizes[t]; ++k)
                    ROPS->set_attribute(RCTX, teams[t][k], attr_flag[which], 1.0f);
                if (w->victory == 0) {
                    for (k = 0; k < sizes[t]; ++k) ROPS->set_timer(RCTX, teams[t][k], which, 2000.0f);
                    ROPS->event(RCTX, ev_start[which], ROPS->number(RCTX, leader));
                }
            } else if (left >= 1.0f && w->victory == 0) {
                left -= w->base.step_seconds + w->base.step_seconds;
                for (k = 0; k < sizes[t]; ++k) ROPS->set_timer(RCTX, teams[t][k], which, left);
                if (left < 1.0f) {
                    ROPS->win(RCTX, leader);
                    w->base.field_1e = ends[which];
                }
            }
        } else if (ROPS->timer(RCTX, leader, which) != -1.0f) {
            for (k = 0; k < sizes[t]; ++k)
                ROPS->set_attribute(RCTX, teams[t][k], attr_flag[which], 0.0f);
            if (w->victory == 0) {
                for (k = 0; k < sizes[t]; ++k) ROPS->set_timer(RCTX, teams[t][k], which, -1.0f);
                ROPS->event(RCTX, ev_lost[which], ROPS->number(RCTX, leader));
            }
        }
    }
}

void aoe_ror_world_check_victory_ror(AoeRorWorldRor *w) {
    AoeWorldBaseRor *b = &w->base;
    if (b->state == 0) {
        if (w->victory == 2 && w->time_limit && w->time_left >= 1.0f) {
            w->time_left -= b->step_seconds + b->step_seconds;
            if (w->time_left < 1.0f) time_limit(w);
        }
        if (w->relics > 0 || w->ruins > 0) {
            void *teams[9][9];
            int32_t sizes[9], team_of[9], team_count = 0, i, j;
            memset(teams, 0, sizeof teams);
            for (i = 0; i < 9; ++i) {
                sizes[i] = 0;
                team_of[i] = -1;
            }
            for (i = 1; i < b->player_count && i < 9; ++i) {
                if (team_of[i] != -1) continue;
                team_of[i] = team_count;
                teams[team_count][sizes[team_count]++] = b->players[i];
                if (ROPS->alive(RCTX, b->players[i])) {
                    for (j = 1; j < b->player_count && j < 9; ++j) {
                        if (team_of[j] != -1 || !ROPS->alive(RCTX, b->players[j])) continue;
                        if (ROPS->stance(RCTX, b->players[j], i) != 0 ||
                            ROPS->stance(RCTX, b->players[i], j) != 0)
                            continue;
                        team_of[j] = team_count;
                        teams[team_count][sizes[team_count]++] = b->players[j];
                    }
                }
                ++team_count;
            }
            if (w->relics > 0 && team_count > 0) hold_check(w, w->relics, 0, teams, sizes, team_count);
            if (w->ruins > 0 && team_count > 0) hold_check(w, w->ruins, 1, teams, sizes, team_count);
        }
    }
    ROPS->base_check(RCTX, w);
}

void aoe_ror_world_call_4e99d0_ror(AoeRorWorldRor *w, int32_t v) {
    ROPS->call_4e99d0(RCTX, w->base.field_58, w->base.field_7c, v);
}

static void map_text_names(AoeWorldBaseRor *w, const char *a, const char *b, const char *c,
                           const char *d) {
    aoe_world_map_text_names_ror(w, a, b, c, d);
}

void aoe_ror_world_vtable_init_ror(void) {
    aoe_ror_world_vtable_ror = aoe_world_base_vtable_ror;
    aoe_ror_world_vtable_ror.load_text = aoe_ror_world_load_text_ror;
    aoe_ror_world_vtable_ror.civ_from_text = aoe_ror_world_civ_from_text_ror;
    aoe_ror_world_vtable_ror.map_from_text = aoe_ror_world_map_from_text_ror;
    aoe_ror_world_vtable_ror.map_text_names = map_text_names;
    aoe_ror_world_vtable_ror.civ_from_stream = aoe_ror_world_civ_from_stream_ror;
    aoe_ror_world_vtable_ror.map_from_stream = aoe_ror_world_map_from_stream_ror;
    aoe_ror_world_vtable_ror.effects_from_stream = aoe_ror_world_effects_from_stream_ror;
    aoe_ror_world_vtable_ror.slot_54 = aoe_ror_world_slot_54_ror;
    aoe_ror_world_vtable_ror.load_data = aoe_ror_world_load_data_ror;
    aoe_ror_world_vtable_ror.save_data = aoe_ror_world_save_data_ror;
}
