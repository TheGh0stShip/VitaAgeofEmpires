#include "mp_setup_screen.h"

#include <stdio.h>
#include <string.h>

#define OPS (m->ops)
#define CTX (m->ops->context)

int32_t aoe_mp_setup_random_map_type_ror(int32_t r) {
    int32_t t = r % 10;
    if (t < 2) return 4;
    if (t == 2) return 3;
    if (t == 3) return 2;
    if (t == 4) return 1;
    if (t <= 8) return t;
    return 0;
}

/* 0x00488c40 */
void aoe_mp_setup_screen_reset_ror(AoeMpSetupScreenRor *m) {
    AoeGameSettingsRor *g = m->settings;
    int i;
    m->kick_player = 0;
    m->field_72c = 0;
    m->field_730 = -1;
    m->field_734 = m->field_738 = m->field_73c = 0;
    m->field_740 = 0;
    m->settings_flag = aoe_settings_flag_8fc_clear_ror(g);
    m->field_844 = 0;
    m->scenario = 0;
    m->max_players = 8;
    m->max_list = 8;
    m->field_84c = 0;
    m->field_854 = m->field_858 = m->field_85c = 0;
    memset(m->field_868, 0, sizeof m->field_868);
    m->share_time = 0;
    m->field_88c = 0;
    m->chat_from = m->chat_to = 0;
    if (!aoe_settings_flag_97e_ror(g)) {
        if (!aoe_settings_flag_8fc_ror(g)) {
            aoe_settings_set_map_size_ror(g, 3);
            aoe_settings_set_int_a80_ror(g, aoe_mp_setup_random_map_type_ror(OPS->rand(CTX)));
        }
        aoe_settings_set_a8c_a90_ror(g, 0, 0);
        aoe_settings_set_byte_988_ror(g, 4);
        aoe_settings_set_int_9a0_ror(g, 0);
        aoe_settings_set_int_9a4_ror(g, 0);
        aoe_settings_set_int_a84_ror(g, 1);
        aoe_settings_set_int_a88_ror(g, 1);
        aoe_settings_set_byte_985_ror(g, 0);
        aoe_settings_set_byte_986_ror(g, 1);
        aoe_settings_set_byte_982_ror(g, 1);
        aoe_settings_set_option_ad8_ror(g, 0);
        aoe_settings_set_option_ad7_ror(g, 0);
        aoe_settings_set_option_ad6_ror(g, 1);
        aoe_settings_set_option_ad9_ror(g, 0);
        aoe_settings_set_resource_level_ror(g, 0);
        aoe_settings_set_starting_age_ror(g, 0);
        aoe_settings_set_option_ae4_ror(g, 0);
        aoe_settings_set_byte_993_ror(g, (uint8_t)aoe_settings_int_a10_ror(g));
        aoe_settings_set_byte_1190_ror(g, 0);
        return;
    }
    aoe_settings_set_int_9a0_ror(g, 0);
    aoe_settings_set_int_9a4_ror(g, 0);
    aoe_settings_set_flag_8fc_ror(g, 0);
    aoe_settings_set_scenario_name_ror(g, "");
    aoe_settings_set_map_size_ror(g, 3);
    aoe_settings_set_int_a80_ror(g, 3);
    aoe_settings_set_a8c_a90_ror(g, 0, 0);
    aoe_settings_set_int_a84_ror(g, 1);
    aoe_settings_set_int_a88_ror(g, 1);
    aoe_settings_set_byte_988_ror(g, 8);
    aoe_settings_set_byte_985_ror(g, 0);
    aoe_settings_set_byte_986_ror(g, 1);
    aoe_settings_set_byte_982_ror(g, 0);
    aoe_settings_set_option_ad8_ror(g, 0);
    aoe_settings_set_option_ad7_ror(g, 0);
    aoe_settings_set_option_ad6_ror(g, 1);
    aoe_settings_set_option_ad9_ror(g, 0);
    aoe_settings_set_resource_level_ror(g, 0);
    aoe_settings_set_starting_age_ror(g, 0);
    aoe_settings_set_option_ae4_ror(g, 0);
    aoe_settings_set_byte_993_ror(g, (uint8_t)aoe_settings_int_a10_ror(g));
    aoe_settings_set_option_ae5_ror(g, 0);
    aoe_settings_set_byte_1190_ror(g, 0);
    OPS->seed_now(CTX);
    for (i = 0; i < AOE_MP_SETUP_PLAYERS; ++i) {
        m->colors[i] = (uint8_t)(i + 1);
        aoe_settings_set_byte_a94_ror(g, i, (uint8_t)(OPS->rand(CTX) % 16 + 1));
        aoe_settings_set_int_aa0_ror(g, i, i);
        aoe_settings_set_player_flag_ror(g, i, 0);
        aoe_settings_set_player_version_ror(g, i, 0);
        aoe_settings_set_byte_ac4_ror(g, i, m->colors[i]);
        aoe_settings_set_player_994_ror(g, i, 1);
        aoe_settings_set_byte_acd_ror(g, i, 0);
    }
    if (OPS->host(CTX)) {
        aoe_settings_set_player_version_ror(g, 0, 2);
        OPS->game_slot_48(CTX);
        m->field_88c = 0;
    }
}

int aoe_mp_setup_screen_slot_68_ror(void) { return 1; }

void aoe_mp_setup_screen_kick_ror(AoeMpSetupScreenRor *m, int32_t player) {
    char text[256];
    const char *name;
    m->kick_player = player;
    name = OPS->player_name(CTX, player);
    snprintf(text, sizeof text, OPS->string(CTX, 0x25c8), name);
    OPS->yes_no(CTX, text, "Kick Dialog", 450, 100);
}

void aoe_mp_setup_screen_ready_message_ror(AoeMpSetupScreenRor *m, int ready) {
    OPS->refresh(CTX, 0);
    OPS->ok_message(CTX, ready ? 0x25c9 : 0x25ca, 450, 100);
}

void aoe_mp_setup_screen_chat_ror(AoeMpSetupScreenRor *m, int32_t i) {
    int16_t n;
    int follow;
    if (aoe_settings_flag_97d_ror(m->settings)) return;
    n = OPS->chat_count(CTX);
    follow = n >= 1 && OPS->chat_bottom(CTX) == n - 1;
    if (n > 99) OPS->chat_remove_first(CTX);
    OPS->chat_add(CTX, OPS->chat_line(CTX, i));
    if (follow) OPS->chat_scroll_bottom(CTX);
}

void aoe_mp_setup_screen_idle_ror(AoeMpSetupScreenRor *m) {
    if (m->chat_from != 0 && !OPS->minimised(CTX)) {
        int32_t i;
        for (i = m->chat_from; i <= m->chat_to; ++i) aoe_mp_setup_screen_chat_ror(m, i);
        m->chat_to = 0;
        m->chat_from = 0;
    }
    if (!OPS->capture_released(CTX)) OPS->release_capture(CTX);
    if (aoe_settings_flag_97e_ror(m->settings)) {
        if (m->share_time == 0) {
            m->share_time = OPS->now(CTX);
        } else if (OPS->now(CTX) != m->share_time) {
            OPS->share_player_data(CTX);
            m->share_time = OPS->now(CTX);
        }
    }
    OPS->base_idle(CTX);
}

void aoe_mp_setup_screen_fill_counts_ror(AoeMpSetupScreenRor *m) {
    char text[16];
    int32_t i;
    OPS->count_clear(CTX);
    for (i = 2; i <= m->max_list; ++i) {
        snprintf(text, sizeof text, "%d", (int)i);
        OPS->count_append(CTX, text, i);
    }
    OPS->count_select(CTX, OPS->count_index(CTX, aoe_settings_byte_988_ror(m->settings)));
}

uint32_t aoe_mp_setup_color_rgb_ror(int32_t i) {
    static const uint32_t rgb[8] = {0xff0000, 0xff, 0xffff, 0x284673,
                                    0x764f0, 0xff00, 0xb4b4b4, 0x93bf2b};
    return i >= 0 && i < 8 ? rgb[i] : 0;
}

void aoe_mp_setup_screen_fill_colors_ror(AoeMpSetupScreenRor *m) {
    int32_t b, i;
    for (b = 0; b < AOE_MP_SETUP_PLAYERS; ++b) {
        int32_t state = 0;
        for (i = 0; i < AOE_MP_SETUP_PLAYERS; ++i) {
            char text[16];
            if (aoe_settings_flag_8fc_ror(m->settings) && m->scenario &&
                (!OPS->scenario_slot_used(CTX, i) ||
                 (aoe_settings_flag_97d_ror(m->settings) && OPS->scenario_slot_kind(CTX, i) != 1)))
                continue;
            snprintf(text, sizeof text, "%d", (int)(i + 1));
            OPS->color_text(CTX, b, state, text);
            OPS->color_state(CTX, b, state, i + 1);
            OPS->color_rgb(CTX, b, state, aoe_mp_setup_color_rgb_ror(i));
            ++state;
        }
        OPS->color_count(CTX, b, state);
    }
}

void aoe_mp_setup_screen_players_ror(AoeMpSetupScreenRor *m) {
    AoeGameSettingsRor *g = m->settings;
    int32_t i;
    if (!aoe_settings_flag_8fc_ror(g)) {
        for (i = 0; i < AOE_MP_SETUP_PLAYERS; ++i) {
            aoe_settings_set_byte_a94_ror(g, i, (uint8_t)(OPS->rand(CTX) % 16 + 1));
            aoe_settings_set_byte_ac4_ror(g, i, (uint8_t)(i + 1));
        }
    } else {
        int32_t first = -1, civ = -1, color = -1, next = 0, slot;
        if (m->scenario) {
            for (slot = 0; slot < 9; ++slot) {
                if (!OPS->scenario_slot_used(CTX, slot)) continue;
                aoe_settings_set_int_aa0_ror(g, next, slot);
                aoe_settings_set_byte_a94_ror(g, next, (uint8_t)OPS->scenario_slot_civ(CTX, slot));
                aoe_settings_set_byte_ac4_ror(g, next, (uint8_t)(slot + 1));
                if (OPS->scenario_slot_kind(CTX, slot) == 1 && first == -1) {
                    civ = OPS->scenario_slot_civ(CTX, slot);
                    first = slot;
                    color = slot + 1;
                }
                ++next;
            }
        }
        aoe_settings_set_byte_988_ror(g, (uint8_t)m->max_list);
        if (first != -1) {
            for (i = 1; i < m->max_list; ++i) {
                if (aoe_settings_int_aa0_ror(g, i) == first) {
                    aoe_settings_set_int_aa0_ror(g, i, aoe_settings_int_aa0_ror(g, 0));
                    aoe_settings_set_byte_a94_ror(g, i, aoe_settings_byte_a94_ror(g, 0));
                    aoe_settings_set_byte_ac4_ror(g, i, aoe_settings_byte_ac4_ror(g, 0));
                    break;
                }
            }
            aoe_settings_set_int_aa0_ror(g, 0, first);
            aoe_settings_set_byte_a94_ror(g, 0, (uint8_t)civ);
            aoe_settings_set_byte_ac4_ror(g, 0, (uint8_t)color);
        }
    }
    for (i = 0; i < AOE_MP_SETUP_PLAYERS; ++i) aoe_settings_set_player_994_ror(g, i, 1);
    OPS->comm_slot(CTX, 1, 2);
    for (i = 2; i < 9; ++i) OPS->comm_slot(CTX, i, 4);
    OPS->comm_slot(CTX, 9, 0);
    aoe_mp_setup_screen_fill_counts_ror(m);
}

void aoe_mp_setup_screen_destroy_ror(AoeMpSetupScreenRor *m) {
    static const char *const names[5] = {"Game Settings Screen", "Select Tribe Screen",
                                         "Kick Dialog", "OKDialog", "YesNoDialog"};
    int i;
    if (m->scenario) {
        OPS->free_scenario(CTX);
        m->scenario = 0;
    }
    for (i = 0; i < 5; ++i) OPS->close_named(CTX, names[i]);
    OPS->release_panels(CTX);
}

void aoe_mp_setup_screen_random_positions_ror(AoeMpSetupScreenRor *m) {
    const AoeMpSetupScreenOpsRor *o = m->ops;
    int32_t pos[8], humans = 0, free_n = 0, i, first = 0, n;
    for (i = 1; i <= (int32_t)aoe_settings_byte_988_ror(m->settings); ++i) {
        int32_t k = o->slot_kind(o->context, i);
        if (k == 2 || k == 4) ++humans;
    }
    for (i = 0; i < 8; ++i) {
        if (o->position_index(o->context, i) == -1) pos[i] = -2;
        else { pos[i] = -1; ++free_n; }
    }
    if (humans > free_n) return;
    for (i = 0; i < (int32_t)aoe_settings_byte_988_ror(m->settings); ++i) {
        int32_t k = o->slot_kind(o->context, i + 1), j, r, tries = 0;
        if (k != 2 && k != 4) continue;
        n = m->max_list;
        for (j = 0; j < n && j < 8; ++j)
            if (pos[j] == -1) { first = j; break; }
        for (;;) {
            r = n > 0 ? (int32_t)(((int64_t)o->rand(o->context) * n) / 0x7fff) : 0;
            if (r >= n) r = n - 1;
            if (r < 0) r = 0;
            if (r < 8 && pos[r] == -1) break;
            if (++tries >= 1000) { r = first; break; }
        }
        aoe_settings_set_int_aa0_ror(m->settings, i, r);
        if (r >= 0 && r < 8) pos[r] = i;
    }
}

void aoe_mp_setup_screen_tab_order_ror(AoeMpSetupScreenRor *m) {
    const AoeMpSetupScreenOpsRor *o = m->ops;
    if (aoe_settings_flag_97d_ror(m->settings)) {
        void *list[3];
        list[0] = m->panel_700; list[1] = m->panel_70c; list[2] = m->panel_620;
        o->tab_order(o->context, list, 3);
        o->focus(o->context, m->panel_700);
    } else if (m->panel_620 != NULL) {
        void *list[4];
        list[0] = m->panel_610; list[1] = m->panel_700; list[2] = m->panel_70c;
        list[3] = m->panel_620;
        o->tab_order(o->context, list, 4);
        o->focus(o->context, m->panel_610);
    } else {
        void *list[3];
        list[0] = m->panel_610; list[1] = m->panel_714; list[2] = m->panel_70c;
        o->tab_order(o->context, list, 3);
        o->focus(o->context, m->panel_610);
    }
}

void aoe_mp_setup_screen_leave_ror(AoeMpSetupScreenRor *m, int error) {
    const AoeMpSetupScreenOpsRor *o = m->ops;
    const char *name;
    if (aoe_settings_flag_97d_ror(m->settings)) {
        o->wait_cursor(o->context);
        name = aoe_settings_flag_8fc_ror(m->settings) ? "Select Scenario Screen"
                                                      : "Single Player Menu";
        o->make_screen(o->context, name, 0);
    } else if (o->lobby_launched(o->context)) {
        name = "Main Error Screen";
        if (!error || !o->make_screen(o->context, name, 0x96a)) {
            o->quit(o->context);
            return;
        }
    } else {
        o->wait_cursor(o->context);
        o->drop(o->context, 5);
        name = "Join Screen";
        o->make_screen(o->context, name, 0);
    }
    o->show_screen(o->context, name);
    o->close_named(o->context, "MP Setup Screen");
}

static void addr_cat(char *out, size_t n, const char *s) {
    size_t len = strlen(out);
    if (s && len + 1 < n) strncat(out, s, n - len - 1);
}

/* 0x0048a780 */
void aoe_mp_setup_screen_addresses_ror(AoeMpSetupScreenRor *m) {
    const AoeMpSetupScreenOpsRor *o = m->ops;
    char text[1024], line[256];
    uint32_t n, i;
    void *host = o->host_open(o->context);
    if (!host) return;
    n = o->host_count(o->context, host);
    text[0] = '\0';
    if (n == 0) {
        o->ok_message(o->context, 0x238d, 0x1c2, 100);
    } else {
        const char *fmt = o->string(o->context, 0x238c);
        addr_cat(text, sizeof text, "\n\n");
        snprintf(line, sizeof line, fmt ? fmt : "%s", o->host_name(o->context, host));
        addr_cat(text, sizeof text, line);
        addr_cat(text, sizeof text, "\n");
        for (i = 0; i < n; ++i) {
            const char *a = o->host_address(o->context, host, i);
            const char *b = o->host_alias(o->context, host, i);
            if (a) {
                addr_cat(text, sizeof text, a);
                addr_cat(text, sizeof text, " ");
            }
            if (b) addr_cat(text, sizeof text, b);
            addr_cat(text, sizeof text, "\n");
        }
        o->ok_text(o->context, text, "OKDialog", 0x1c2, (int32_t)(n * 0x12 + 100));
    }
    o->host_close(o->context, host);
}

static int start_fail_named(AoeMpSetupScreenRor *m, int32_t id, int32_t player) {
    char text[1024];
    const char *fmt = OPS->string(CTX, id);
    const char *name = OPS->player_name(CTX, player);
    snprintf(text, sizeof text, fmt ? fmt : "%s", name ? name : "");
    OPS->ok_text(CTX, text, NULL, 0x1c2, 100);
    return 0;
}

static int start_fail_count(AoeMpSetupScreenRor *m, int32_t id) {
    char text[256];
    const char *fmt = OPS->string(CTX, id);
    snprintf(text, sizeof text, fmt ? fmt : "%d", (int)m->max_list);
    OPS->ok_text(CTX, text, NULL, 0x1c2, 100);
    return 0;
}

/* 0x00489ef0 */
int aoe_mp_setup_screen_start_ror(AoeMpSetupScreenRor *m) {
    AoeGameSettingsRor *g = m->settings;
    int net = aoe_settings_flag_97e_ror(g) != 0;
    int scen = aoe_settings_flag_8fc_ror(g) != 0;
    /* flags[0] is the changed flag; flags[1..8] the team marks. */
    int32_t flags[9] = {0};
    int32_t i, k;
    if (net) {
        if (!OPS->host(CTX)) return 0;
        if (!OPS->all_ready(CTX)) {
            OPS->ok_message(CTX, 0x25c1, 0x1c2, 100);
            return 0;
        }
        if (scen) {
            for (i = 0; i < 8; ++i)
                if (i + 1 != OPS->self_slot(CTX) && OPS->slot_kind(CTX, i + 1) == 2 &&
                    m->field_868[i] != m->field_73c)
                    return start_fail_named(m, 0x25e1, i + 1);
        }
        for (i = 0; i < 8; ++i)
            if (i + 1 != OPS->self_slot(CTX) && OPS->slot_kind(CTX, i + 1) == 2 &&
                aoe_settings_player_version_ror(g, i) != 2)
                return start_fail_named(m, 0x25e2, i + 1);
    }
    if (scen && !m->scenario) {
        OPS->ok_message(CTX, 0x25c2, 0x1c2, 100);
        return 0;
    }
    if (net && scen) {
        int32_t idx = 0;
        for (i = 0; i < 8; ++i) {
            int32_t kind = OPS->slot_kind(CTX, i + 1);
            if (kind != 2 && kind != 4) continue;
            aoe_settings_set_int_aa0_ror(g, i, OPS->position_id(CTX, idx));
            if (++idx >= OPS->position_count(CTX)) idx = 0;
        }
    }
    if (net) {
        int32_t teams = 0, humans = 0;
        uint8_t mask;
        for (i = 0; i < (int32_t)aoe_settings_byte_988_ror(g); ++i) {
            int32_t kind = OPS->slot_kind(CTX, i + 1);
            if (kind == 2) {
                uint8_t t = aoe_settings_byte_ac4_ror(g, i);
                if (t <= 8 && !flags[t]) {
                    ++teams;
                    flags[t] = 1;
                }
            } else if (kind == 4) {
                ++teams;
            }
        }
        if (teams < 2) {
            OPS->ok_message(CTX, 0x25c6, 0x1c2, 100);
            return 0;
        }
        if (scen && m->max_list < teams) return start_fail_count(m, 0x25c3);
        if (scen && m->field_84c && teams != m->max_list) return start_fail_count(m, 0x25c5);
        for (i = 0; i < (int32_t)aoe_settings_byte_988_ror(g); ++i)
            if (OPS->slot_kind(CTX, i + 1) == 2) ++humans;
        mask = (uint8_t)OPS->cd_check(CTX, 0);
        aoe_settings_set_player_flag_ror(g, OPS->self_slot(CTX) - 1, mask);
        if (!OPS->cd_check(CTX, humans)) {
            OPS->ok_message(CTX, 0x7d9, 0x1c2, 100);
            return 0;
        }
    }
    if (scen && aoe_settings_option_ad8_ror(g)) {
        aoe_mp_setup_screen_random_positions_ror(m);
        if (net) flags[0] = 1;
    }
    if (aoe_settings_flag_97d_ror(g) && aoe_settings_byte_988_ror(g) < 8)
        for (i = aoe_settings_byte_988_ror(g) + 1; i < 9; ++i) OPS->comm_slot(CTX, i, 0);
    if (net) {
        /* The original also logs "$$$ HOST RESET2 CIV+50" / "TEAM+50"
           (0x0042c330, empty in this build). */
        for (i = 0; i < (int32_t)aoe_settings_byte_988_ror(g); ++i) {
            if (aoe_settings_byte_a94_ror(g, i) > 0x31) {
                aoe_settings_set_byte_a94_ror(g, i, (uint8_t)(aoe_settings_byte_a94_ror(g, i) - 0x32));
                flags[0] = 1;
            }
            if (aoe_settings_player_994_ror(g, i) > 0x31) {
                aoe_settings_set_player_994_ror(g, i,
                                                (uint8_t)(aoe_settings_player_994_ror(g, i) - 0x32));
                flags[0] = 1;
            }
        }
    }
    if (net) {
        int32_t ids[8], civs[8], teams[8];
        for (i = 0; i < 8; ++i) {
            ids[i] = i + 1;
            civs[i] = 1;
            teams[i] = 1;
        }
        k = 0;
        for (i = 0; i < 8; ++i) {
            int32_t kind = OPS->slot_kind(CTX, i + 1);
            if (kind != 2 && kind != 4) continue;
            ids[k] = aoe_settings_byte_ac4_ror(g, i);
            civs[k] = aoe_settings_byte_a94_ror(g, i);
            teams[k] = aoe_settings_player_994_ror(g, i);
            ++k;
        }
        if (k > 0) {
            for (i = 0; i < 8; ++i) {
                aoe_settings_set_byte_ac4_ror(g, i, (uint8_t)ids[i]);
                aoe_settings_set_byte_a94_ror(g, i, (uint8_t)civs[i]);
                aoe_settings_set_player_994_ror(g, i, (uint8_t)teams[i]);
            }
            flags[0] = 1;
        }
    }
    OPS->close_named(CTX, "Status Screen");
    if (net) {
        if (flags[0] || m->field_88c) {
            OPS->game_slot_48(CTX);
            m->field_88c = 0;
        }
        OPS->launch_session(CTX);
    }
    if (OPS->start_game(CTX)) return 1;
    OPS->ok_message(CTX, 0x961, 0x1c2, 100);
    return 0;
}
