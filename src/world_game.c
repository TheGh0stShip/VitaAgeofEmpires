#include "world_game.h"

#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

#define W (g->w)
#define OPS (g->ops)
#define CTX (g->ops->context)

static void log_rand(AoeWorldGameRor *g, const char *fmt, int32_t v) {
    char text[0x80];
    snprintf(text, sizeof text, fmt, (int)v);
    OPS->log(CTX, text);
}

/* ---- update ---- */

static void ai_log(AoeWorldGameRor *g) {
    int i;
    if (W->state != 1) {
        uint32_t t;
        if (!((uint32_t)(W->game_time - g->ai_log_time) > 30000 || g->ai_log_time == 0)) return;
        if (!g->ai_log) {
            char path[0x100];
            snprintf(path, sizeof path, "AILog\\%s.log", OPS->ai_log_name(CTX));
            g->ai_log = OPS->open_append(CTX, path);
            if (!g->ai_log) return;
        }
        t = (uint32_t)W->game_time / 1000;
        {
            uint32_t h = 0, m = 0, s = t;
            if (s > 59) {
                m = s / 60;
                s %= 60;
                if (m > 59) {
                    h = m / 60;
                    m %= 60;
                }
            }
            if (g->ai_log_time == 0)
                g->vtbl->ai_log_header(W, g->ai_log, 1);
            else
                fprintf(g->ai_log, "\nStatus Log at GameTime %02ld:%02ld:%02ld:\n", (long)h,
                        (long)m, (long)s);
        }
        for (i = 1; i < W->player_count; ++i)
            if (W->players[i])
                OPS->player_ai_log(CTX, W->players[i], g->ai_log, g->ai_log_time == 0);
        g->ai_log_time = W->game_time + 1;
        return;
    }
    if (!g->ai_log) return;
    fprintf(g->ai_log, "GAME OVER!\n");
    for (i = 1; i < W->player_count; ++i)
        if (W->players[i])
            fprintf(g->ai_log, OPS->player_won(CTX, W->players[i]) ? "  Player #%d Won.\n"
                                                                  : "  Player #%d Lost.\n",
                    i);
    fclose(g->ai_log);
    g->ai_log = NULL;
}

uint8_t aoe_world_update_ror(AoeWorldGameRor *g) {
    uint32_t t0, now, elapsed;
    int32_t turn, saved, i;
    int catch_up = 0;
    W->field_f8 = W->field_f4;
    t0 = OPS->now_ms(CTX);
    turn = OPS->comm_turn(CTX, W->game_time);
    OPS->perf(CTX, 0xe, OPS->now_ms(CTX) - t0);
    if (turn == 0) {
        W->time_step = 0;
        return W->state;
    }
    g->field_7d2498 = 0;
    OPS->srand(CTX, W->seed);
    now = OPS->now_ms(CTX);
    aoe_world_idle_sound_ror(W, now);
    aoe_world_idle_sprite_ror(W, now);
    if (turn == -1) {
        if (W->last_ms == 0) {
            catch_up = 1;
            elapsed = 0;
        } else {
            elapsed = now - W->last_ms;
            if (elapsed > 100) elapsed = 100;
        }
    } else {
        elapsed = (uint32_t)turn;
    }
    W->last_ms = now;
    W->old_time = W->game_time;
    if (W->field_1c) {
        if (turn != -1) W->game_time = (int32_t)((float)elapsed * W->speed) + W->old_time;
        W->field_1c = 0;
    } else {
        W->game_time = (int32_t)((float)elapsed * W->speed) + W->old_time;
    }
    W->time_step = W->game_time - W->old_time;
    if (W->time_step != 0 || catch_up) {
        OPS->comm_42a140(CTX);
        OPS->scenario_476270(CTX);
        W->step_seconds = (float)(uint32_t)W->time_step * 0.001f;
        ++aoe_world_counter_ror;
        for (i = 0; i < W->player_count; ++i) {
            if (!W->players[i]) continue;
            W->update_times[i] += W->time_step;
            OPS->player_update(CTX, W->players[i]);
            OPS->player_eb40(CTX, W->players[i]);
        }
        if (OPS->hook_582edc) OPS->hook_582edc(CTX);
        saved = OPS->rand(CTX);
        for (i = 0; i < W->player_count; ++i)
            if (W->players[i]) OPS->player_451270(CTX, W->players[i]);
        for (i = 0; i < W->player_count; ++i)
            if (W->players[i]) OPS->player_436a20(CTX, W->players[i]);
        OPS->srand(CTX, saved);
        W->update_times[W->update_slot] = 0;
        if (++W->update_slot >= W->player_count) W->update_slot = 0;
        g->vtbl->check_victory(W);
    }
    W->seed = OPS->rand(CTX);
    OPS->comm_seed(CTX, W->seed);
    if (OPS->ai_logging == 1) ai_log(g);
    return W->state;
}

/* ---- colours and players ---- */

void aoe_world_assign_colors_ror(AoeWorldGameRor *g, const AoeWorldGameInfoRor *info) {
    int32_t limit, i, next = -1;
    uint8_t *used, *assigned;
    OPS->player_color(CTX, W->players[0], 7);
    limit = info->player_count;
    if (W->player_count + 1 < info->player_count) limit = W->player_count + 1;
    used = calloc((size_t)(W->color_count > 0 ? W->color_count : 1), 1);
    assigned = calloc((size_t)(W->player_count > 0 ? W->player_count : 1), 1);
    if (!used || !assigned) {
        free(used);
        free(assigned);
        return;
    }
    for (i = 1; i < W->player_count; ++i) {
        int16_t c = i < 9 ? info->color[i] : -1;
        if (i - 1 < limit && c >= 0 && c < W->color_count && !used[c]) {
            OPS->player_color(CTX, W->players[i], c);
            used[c] = 1;
            assigned[i] = 1;
        }
    }
    for (i = 1; i < W->player_count; ++i) {
        if (assigned[i]) continue;
        do {
            do ++next;
            while (next < W->color_count && used[next] == 1);
        } while (next < W->color_count && W->colors[next]->field_24 != 1);
        if (next >= W->color_count) break; /* the original runs off the end */
        OPS->player_color(CTX, W->players[i], next);
        used[next] = 1;
    }
    free(used);
    free(assigned);
}

void aoe_world_assign_default_colors_ror(AoeWorldGameRor *g) {
    int32_t i, next = -1;
    uint8_t *used;
    OPS->player_color(CTX, W->players[0], 7);
    used = calloc((size_t)(W->color_count > 0 ? W->color_count : 1), 1);
    if (!used) return;
    for (i = 1; i < W->player_count; ++i) {
        do {
            do ++next;
            while (next < W->color_count && used[next] == 1);
        } while (next < W->color_count && W->colors[next]->field_24 != 1);
        if (next >= W->color_count) break;
        OPS->player_color(CTX, W->players[i], next);
        used[next] = 1;
    }
    free(used);
}

void aoe_world_new_players_ror(AoeWorldGameRor *g, const AoeWorldGameInfoRor *info,
                               void *(*player_new)(void *context, AoeWorldBaseRor *w, void *civ,
                                                   int16_t index, const char *name,
                                                   uint8_t civ_id, int flag)) {
    int16_t i;
    for (i = 1; i < W->player_count && i < 9; ++i) {
        int8_t civ;
        if (info->type[i] == -1) continue;
        civ = info->civ[i];
        W->players[i] = player_new(CTX, W, W->civs[civ], i, info->names[i - 1], (uint8_t)civ, 1);
    }
}

/* ---- saved game state ---- */

static void put16(AoeOutcomeStream *s, int16_t v) {
    uint8_t b[2] = {(uint8_t)v, (uint8_t)((uint16_t)v >> 8)};
    aoe_stream_put(s, b, 2);
}

static int16_t get16(AoeOutcomeStream *s) {
    const uint8_t *p = aoe_stream_take(s, 2);
    return p ? (int16_t)(p[0] | (p[1] << 8)) : 0;
}

void aoe_world_save_state_ror(AoeWorldGameRor *g, AoeOutcomeStream *s) {
    int i;
    g->version = 7.24f;
    aoe_stream_put_i32(s, W->game_time);
    aoe_stream_put_i32(s, W->old_time);
    aoe_stream_put_i32(s, W->time_step);
    aoe_stream_put_f32(s, W->step_seconds);
    aoe_stream_put_i32(s, W->field_10);
    aoe_stream_put_f32(s, W->speed);
    aoe_stream_put_u8(s, W->field_1c);
    aoe_stream_put_i32(s, W->field_68);
    aoe_stream_put_i32(s, W->field_6c);
    aoe_stream_put_i32(s, W->seed);
    put16(s, W->field_7c);
    put16(s, W->player_count);
    aoe_stream_put_u8(s, W->state);
    /* The campaign number is saved one higher in a campaign game. */
    if (OPS->flag_720(CTX)) ++W->campaign;
    aoe_stream_put_i32(s, W->campaign);
    aoe_stream_put_i32(s, W->campaign_player);
    aoe_stream_put_i32(s, W->campaign_scenario);
    if (OPS->flag_720(CTX)) --W->campaign;
    aoe_stream_put_i32(s, W->update_slot);
    for (i = 0; i < 9; ++i) aoe_stream_put_i32(s, W->update_times[i]);
    OPS->map_save(CTX, s);
    for (i = 0; i < W->player_count; ++i) {
        OPS->player_save(CTX, W->players[i], 0x84, s);
        OPS->player_save(CTX, W->players[i], 0x88, s);
    }
    for (i = 0; i < W->player_count; ++i) OPS->player_save(CTX, W->players[i], 0x8c, s);
    OPS->scenario_save(CTX, W->scenario, s);
    if (W->field_100 == -1) W->field_100 = OPS->default_100(CTX);
    aoe_stream_put_i32(s, W->field_100);
}

int aoe_world_load_state_ror(AoeWorldGameRor *g, AoeOutcomeStream *s) {
    uint8_t state;
    int i;
    W->state = 2;
    W->field_1e = 0;
    W->game_time = aoe_stream_i32(s);
    W->old_time = aoe_stream_i32(s);
    W->time_step = aoe_stream_i32(s);
    W->step_seconds = aoe_stream_f32(s);
    W->field_10 = aoe_stream_i32(s);
    W->speed = aoe_stream_f32(s);
    W->field_1c = aoe_stream_u8(s);
    W->field_68 = aoe_stream_i32(s);
    W->field_6c = aoe_stream_i32(s);
    W->seed = aoe_stream_i32(s);
    W->field_7c = get16(s);
    W->player_count = get16(s);
    state = aoe_stream_u8(s);
    W->campaign = aoe_stream_i32(s);
    W->campaign_player = aoe_stream_i32(s);
    W->campaign_scenario = aoe_stream_i32(s);
    if (g->version >= 7.24f) {
        int32_t had = W->campaign;
        if (had != 0) {
            W->campaign = had - 1;
            OPS->campaign_restore(CTX, 1, W->campaign, W->campaign_player,
                                  W->campaign_scenario);
        } else {
            OPS->campaign_restore(CTX, 0, 0, 0, 0);
        }
    }
    W->update_slot = aoe_stream_i32(s);
    for (i = 0; i < 9; ++i) W->update_times[i] = aoe_stream_i32(s);
    W->last_ms = 0;
    OPS->map_load(CTX, s);
    OPS->call_5209c0(CTX);
    W->field_fc = -1;
    if (W->player_count > 0) {
        W->players = calloc((size_t)W->player_count, sizeof *W->players);
        for (i = 0; i < W->player_count; ++i) {
            uint8_t flag = aoe_stream_u8(s);
            g->vtbl->player_from_stream(W, s, flag, (int16_t)i);
        }
    }
    OPS->call_520c60(CTX);
    OPS->call_518000(CTX, 1);
    for (i = 0; i < W->player_count; ++i) OPS->player_load_body(CTX, W->players[i], s);
    OPS->call_518000(CTX, 0);
    g->vtbl->scenario_from_stream(W, s);
    if (g->version < 7.16f)
        W->field_100 = -1;
    else
        W->field_100 = aoe_stream_i32(s);
    if (OPS->single_player(CTX)) W->speed = OPS->speed(CTX);
    W->state = state;
    return 1;
}

int aoe_world_save_game_ror(AoeWorldGameRor *g, const char *name) {
    char path[300];
    AoeOutcomeStream *s;
    g->version = 7.24f;
    snprintf(path, sizeof path, "%s%s", OPS->save_dir, name);
    s = OPS->open_write(CTX, path);
    if (!s) return 0;
    aoe_stream_put(s, "VER 8.6", 8);
    aoe_stream_put_f32(s, g->version);
    g->vtbl->save_state(W, s);
    if (OPS->close_write(CTX, s) == 0) return 1;
    OPS->remove_file(CTX, path);
    return 0;
}

int aoe_world_load_game_ror(AoeWorldGameRor *g, const char *name) {
    char path[300];
    AoeOutcomeStream *s;
    const uint8_t *tag;
    aoe_world_counter_ror = 0;
    aoe_world_reset_ror(W);
    snprintf(path, sizeof path, "%s%s", OPS->save_dir, name);
    s = OPS->open_read(CTX, path);
    if (!s) return 0;
    tag = aoe_stream_take(s, 8);
    g->version = aoe_stream_f32(s);
    if (tag && memcmp(tag, "VER 8.6", 8) == 0) {
        g->vtbl->load_state(W, s);
        OPS->close_read(CTX, s);
        return 1;
    }
    OPS->close_read(CTX, s);
    return 0;
}

/* ---- starting a game ---- */

static void alliances(AoeWorldGameRor *g) {
    int32_t i, j;
    for (i = 1; i < 9; ++i) {
        int32_t t = OPS->slot_type(CTX, i), team;
        if (t != 2 && t != 4) continue;
        team = OPS->team(CTX, i - 1);
        if (team <= 1) continue;
        for (j = 1; j < 9; ++j) {
            int32_t tj = OPS->slot_type(CTX, j), pi, pj;
            if ((tj != 2 && tj != 4) || OPS->team(CTX, j - 1) != team) continue;
            pi = OPS->slot_player(CTX, i);
            pj = OPS->slot_player(CTX, j);
            if (pi == pj) continue;
            OPS->player_stance(CTX, W->players[pi], pj, 0);
            OPS->player_da40(CTX, W->players[pi], 1);
        }
    }
    if (!OPS->flag_710(CTX) || OPS->flag_720(CTX)) return;
    for (i = 1; i < 9; ++i) {
        int32_t t = OPS->slot_type(CTX, i);
        if (!(t == 4 || OPS->computer_team_rule == 1 || (OPS->computer_team_rule == 2 && i > 1)))
            continue;
        if (OPS->team(CTX, i - 1) != 1) continue;
        for (j = 1; j < 9; ++j) {
            int32_t tj = OPS->slot_type(CTX, j), pi, pj;
            if (tj != 4 && tj != 2) continue;
            pi = OPS->slot_player(CTX, i);
            pj = OPS->slot_player(CTX, j);
            if (pi == pj) continue;
            OPS->player_stance(CTX, W->players[pi], pj, 1);
            OPS->player_da40(CTX, W->players[pi], 1);
        }
    }
}

int aoe_world_new_game_ror(AoeWorldGameRor *g, const AoeWorldGameInfoRor *info, int16_t v7c) {
    int random = 0, ok, i;
    int32_t seed;
    char text[0x100];
    aoe_world_counter_ror = 0;
    OPS->reset_paths(CTX);
    OPS->reset_paths(CTX);
    W->field_fc = -1;
    memset(g->path_flags, 0, sizeof g->path_flags);
    aoe_world_reset_ror(W);
    W->state = 3;
    W->field_7c = v7c;
    if (!OPS->session(CTX)) {
        seed = OPS->fixed_seed(CTX);
        if (seed == -1) seed = OPS->rand(CTX);
    } else {
        seed = OPS->comm_new_seed(CTX);
    }
    W->seed = seed;
    OPS->srand(CTX, seed);
    OPS->set_used_seed(CTX, W->seed);
    W->seed = OPS->rand(CTX);
    log_rand(g, "World: new_game (rand=%d)", W->seed);
    W->player_count = info->player_count + 1;
    OPS->map_new(CTX, info->map_w, info->map_h);
    W->seed = OPS->rand(CTX);
    log_rand(g, "  World: after_new_map (rand=%d)", W->seed);
    W->player_count = info->player_count + 1;
    W->players = calloc((size_t)W->player_count, sizeof *W->players);
    g->vtbl->new_gaia(W);
    W->seed = OPS->rand(CTX);
    log_rand(g, "  World: after_gaia (rand=%d)", W->seed);
    g->vtbl->new_players(W, info);
    W->seed = OPS->rand(CTX);
    log_rand(g, "  World: after_players (rand=%d)", W->seed);
    alliances(g);
    W->game_time = W->old_time = W->time_step = 0;
    W->last_ms = 0;
    W->speed = OPS->single_player(CTX) ? OPS->speed(CTX) : 1.0f;
    W->field_1c = 0;
    W->step_seconds = 0.0f;
    if (info->scenario == NULL) {
        if (!info->campaign) {
            ok = g->vtbl->random_map_game(W, info);
            random = 1;
        } else {
            ok = g->vtbl->campaign_game(W, info);
        }
    } else {
        ok = g->vtbl->scenario_game(W, info->scenario, info);
    }
    OPS->call_5209c0(CTX);
    for (i = 0; i < W->player_count; ++i)
        if (W->players[i]) OPS->player_start(CTX, W->players[i]);
    if (ok) W->state = 0;
    W->seed = OPS->rand(CTX);
    log_rand(g, "  World: end_of_new_game (rand=%d)", W->seed);
    text[0] = '\0';
    OPS->string(CTX, 0x277b, text, sizeof text);
    if (random)
        for (i = 1; i < W->player_count; ++i)
            if (W->players[i]) OPS->player_2c(CTX, W->players[i], text, info->field_2a0);
    return ok;
}

int aoe_world_editor_game_ror(AoeWorldGameRor *g, const AoeWorldGameInfoRor *info,
                              int32_t seed) {
    int ok, i;
    aoe_world_counter_ror = 0;
    aoe_world_reset_ror(W);
    W->field_7c = 0;
    if (seed == 0) seed = OPS->comm_new_seed(CTX);
    W->seed = seed;
    OPS->srand(CTX, seed);
    OPS->map_new(CTX, info->map_w, info->map_h);
    OPS->map_from_info(CTX, info->map_info ? info->map_info[0] : 0, 1);
    W->player_count = info->player_count + 1;
    W->players = calloc((size_t)W->player_count, sizeof *W->players);
    g->vtbl->new_gaia(W);
    g->vtbl->new_players(W, info);
    W->game_time = W->old_time = W->time_step = 0;
    W->last_ms = 0;
    W->speed = 1.0f;
    W->field_1c = 0;
    W->step_seconds = 0.0f;
    if (info->scenario == NULL) {
        g->vtbl->assign_colors(W, info);
        g->vtbl->scenario_new(W, info);
        ok = 1;
    } else {
        ok = g->vtbl->scenario_game(W, info->scenario, info);
        if (!ok) return 0;
    }
    for (i = 0; i < W->player_count; ++i) OPS->player_start(CTX, W->players[i]);
    return ok;
}

int aoe_world_random_map_game_ror(AoeWorldGameRor *g, const AoeWorldGameInfoRor *info) {
    int16_t i, keep;
    int pass;
    aoe_world_counter_ror = 0;
    g->vtbl->assign_colors(W, info);
    g->vtbl->scenario_new(W, info);
    W->seed = OPS->rand(CTX);
    log_rand(g, "  World: before_map_gen (rand=%d)", W->seed);
    g->vtbl->map_generate(W, info, W->player_count - 1);
    OPS->call_5209c0(CTX);
    W->seed = OPS->rand(CTX);
    log_rand(g, "  World: after_map_gen (rand=%d)", W->seed);
    W->seed = OPS->rand(CTX);
    log_rand(g, "  World: after_player_random_start (rand=%d)", W->seed);
    keep = W->field_7c;
    W->field_7c = -1;
    for (pass = 0; pass < 3; ++pass) {
        ++aoe_world_counter_ror;
        for (i = 0; i < W->player_count; ++i)
            if (W->players[i]) OPS->player_update(CTX, W->players[i]);
    }
    W->field_7c = keep;
    W->seed = OPS->rand(CTX);
    log_rand(g, "  World: after_player_update (rand=%d)", W->seed);
    return 1;
}

/* The version tag's scenario body; 1.03 skips a sized block first. */
static int scenario_body(AoeWorldGameRor *g, AoeOutcomeStream *s,
                         const AoeWorldGameInfoRor *info) {
    static const char *const tags[] = {"1.01", "1.02", "1.03", "1.04", "1.05", "1.06",
                                       "1.07", "1.08", "1.09", "1.10", "1.11", "1.12"};
    static const int which[] = {0, 1, 1, 2, 3, 4, 5, 6, 7, 7, 7, 7};
    const uint8_t *tag = aoe_stream_take(s, 4);
    int i;
    if (!tag) return 0;
    for (i = 0; i < 12; ++i) {
        if (memcmp(tag, tags[i], 4) != 0) continue;
        if (i == 2) {
            int32_t n = aoe_stream_i32(s);
            if (n > 0 && !aoe_stream_take(s, (size_t)n)) return 0;
        }
        g->vtbl->scenario_body(W, which[i], s, info);
        return 1;
    }
    return 0;
}

int aoe_world_scenario_game_ror(AoeWorldGameRor *g, const char *name,
                                const AoeWorldGameInfoRor *info) {
    char path[300];
    AoeOutcomeStream *s;
    aoe_world_counter_ror = 0;
    if (!name) return 0;
    snprintf(path, sizeof path, "%s%s", OPS->scenario_dir, name);
    s = OPS->open_read(CTX, path);
    if (!s) return 0;
    if (!scenario_body(g, s, info)) {
        OPS->close_read(CTX, s);
        return 0;
    }
    /* The bodies close the stream themselves. */
    g->vtbl->assign_colors(W, info);
    return 1;
}

int aoe_world_campaign_game_ror(AoeWorldGameRor *g, const AoeWorldGameInfoRor *info) {
    AoeOutcomeStream *s;
    aoe_world_counter_ror = 0;
    s = OPS->campaign_stream(CTX);
    if (!s) return 0;
    if (!scenario_body(g, s, info)) {
        OPS->close_read(CTX, s);
        return 0;
    }
    g->vtbl->assign_colors(W, info);
    OPS->campaign_read(CTX, &W->campaign, &W->campaign_player, &W->campaign_scenario);
    return 1;
}

static void save_objects(AoeWorldGameRor *g, AoeOutcomeStream *s, void *player) {
    int32_t count = 0, list, i, n;
    AoeWorldSceneObjectRor o;
    for (list = 0; list < 2; ++list) {
        n = OPS->player_objects(CTX, player, list);
        for (i = 0; i < n; ++i) {
            OPS->player_object(CTX, player, list, i, &o);
            if (o.type != 0x19) ++count;
        }
    }
    aoe_stream_put_i32(s, count);
    for (list = 0; list < 2; ++list) {
        n = OPS->player_objects(CTX, player, list);
        for (i = 0; i < n; ++i) {
            OPS->player_object(CTX, player, list, i, &o);
            if (o.type == 0x19) continue;
            aoe_stream_put_f32(s, o.x);
            aoe_stream_put_f32(s, o.y);
            aoe_stream_put_f32(s, o.z);
            aoe_stream_put_i32(s, o.id);
            put16(s, o.template_id);
            aoe_stream_put_u8(s, o.state);
            aoe_stream_put_f32(s, (o.type > 0x1e && o.type < 0x5a) ? o.angle : (float)o.facing);
        }
    }
}

int aoe_world_save_scenario_ror(AoeWorldGameRor *g, const char *name) {
    char base[300], path[620];
    AoeOutcomeStream *s;
    int32_t count = W->player_count;
    size_t k;
    int i;
    for (k = 0; name[k] && name[k] != '.' && k + 1 < sizeof base; ++k) base[k] = name[k];
    base[k] = '\0';
    snprintf(path, sizeof path, "%s%s.scn", OPS->scenario_dir, base);
    OPS->remove_file(CTX, path);
    snprintf(path, sizeof path, "%s%s", OPS->scenario_dir, name);
    s = OPS->open_write(CTX, path);
    if (!s) return 0;
    aoe_stream_put(s, "1.11", 4);
    OPS->scenario_info_header(CTX, s);
    aoe_stream_put_i32(s, W->field_68);
    OPS->scenario_save(CTX, W->scenario, s);
    OPS->map_scenario_save(CTX, s);
    aoe_stream_put_i32(s, count);
    for (i = 1; i < W->player_count; ++i) OPS->player_save(CTX, W->players[i], 0x64, s);
    for (i = 0; i < W->player_count; ++i) save_objects(g, s, W->players[i]);
    aoe_stream_put_i32(s, count);
    for (i = 1; i < W->player_count; ++i) OPS->player_save(CTX, W->players[i], 0x6c, s);
    if (OPS->close_write(CTX, s) == 0) return 1;
    OPS->remove_file(CTX, path);
    return 0;
}
