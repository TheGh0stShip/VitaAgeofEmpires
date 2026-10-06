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

static void mp_setup_defocus_chat(AoeMpSetupScreenRor *m) {
    if (m->panel_610 && OPS->panel_active(CTX, m->panel_610)) OPS->focus(CTX, m->panel_610);
}

static void mp_setup_ready(AoeMpSetupScreenRor *m) {
    int host_here;
    if (!OPS->button_state(CTX, m->panel_714)) {
        OPS->log(CTX, "$$$ SELECTED NOT READY");
        OPS->send_ready(CTX, 0, 0, 0, OPS->host(CTX) ? 0 : OPS->cd_check(CTX, 0) + 1, 0, 0, 0, 0);
        mp_setup_defocus_chat(m);
        m->field_85c = 0;
    } else {
        OPS->log(CTX, "$$$ SELECTED I'M READY");
        if (OPS->host(CTX)) {
            OPS->send_ready(CTX, 1, 0, 0, 0, 0, 0, 0, 0);
        } else {
            int32_t s;
            if (OPS->player_version(CTX, 0) != 2) {
                OPS->button_slot_e4(CTX, m->panel_714, 0);
                OPS->ok_message(CTX, 0x25e3, 450, 100);
                return;
            }
            s = OPS->self_slot(CTX) - 1;
            OPS->log(CTX, "$$$ SAVING MYCIV & MYTEAM");
            m->field_72c = OPS->list_id(CTX, m->civ_lists[s]);
            m->field_730 = OPS->list_id(CTX, m->pos_lists[s]);
            m->field_734 = OPS->button_value(CTX, m->color_buttons[s]);
            m->field_738 = OPS->button_state(CTX, m->team_buttons[s]) + 1;
            m->field_73c = aoe_settings_flag_8fc_ror(m->settings) ? OPS->scenario_checksum(CTX) : 0;
            OPS->send_ready(CTX, 1, m->field_72c, m->field_730 + 1, OPS->cd_check(CTX, 0) + 1,
                            m->field_734, m->field_738, m->field_73c, 2);
        }
        mp_setup_defocus_chat(m);
        m->field_85c = 1;
    }
    OPS->update_ready(CTX);
    host_here = OPS->host(CTX);
    if (host_here) {
        OPS->button_set_state(CTX, m->panel_620, m->field_85c);
        OPS->button_set_state(CTX, m->panel_700, m->field_85c == 0);
        OPS->game_slot_48(CTX);
        m->field_88c = 0;
        OPS->share_player_data_now(CTX);
    }
}

/* 0x004895b0 */
int aoe_mp_setup_screen_action_ror(AoeMpSetupScreenRor *m, void *panel, int32_t action,
                                   int32_t a, int32_t b) {
    const char *name;
    int32_t i;
    if (!panel) return OPS->base_action(CTX, panel, action, a, b);
    if (action == 1) {
        if (panel == m->panel_700) { mp_setup_defocus_chat(m); aoe_mp_setup_screen_start_ror(m); return 1; }
        if (panel == m->panel_714) { mp_setup_ready(m); return 1; }
        if (panel == m->panel_620) {
            mp_setup_defocus_chat(m);
            if (!OPS->screen_exists(CTX, "Game Settings Screen")) {
                OPS->wait_cursor(CTX);
                OPS->make_screen(CTX, "Game Settings Screen", 0);
            }
            OPS->show_screen(CTX, "Game Settings Screen");
            return 1;
        }
        if (panel == m->panel_710) { OPS->help(CTX); return 1; }
        if (panel == m->panel_718) { OPS->quit(CTX); return 1; }
        if (panel == m->panel_70c) { mp_setup_defocus_chat(m); aoe_mp_setup_screen_leave_ror(m, 0); return 1; }
    }
    if (panel == m->panel_858) {
        if (action == 1) { aoe_mp_setup_screen_addresses_ror(m); return 1; }
    } else if (action == 1) {
        for (i = 0; i < 8; ++i) {
            if (panel == m->color_buttons[i]) {
                OPS->set_color(CTX, i, OPS->button_value(CTX, panel));
                if (aoe_settings_flag_97e_ror(m->settings) && !OPS->host(CTX))
                    m->field_734 = OPS->button_value(CTX, panel);
                return 1;
            }
            if (panel == m->team_buttons[i]) {
                OPS->set_byte_bbd0(CTX, i, OPS->button_state(CTX, panel) + 1);
                if (aoe_settings_flag_97e_ror(m->settings) && !OPS->host(CTX))
                    m->field_738 = OPS->button_state(CTX, panel) + 1;
                return 1;
            }
        }
    }
    if (panel == m->panel_610 && action == 0) {
        const char *t = OPS->input_text(CTX, panel);
        if (t && *t) {
            OPS->send_chat(CTX, t);
            OPS->input_set(CTX, panel, "");
            return 1;
        }
    }
    if (panel == m->panel_6e0 && action == 0) {
        OPS->set_player_count(CTX, OPS->list_id(CTX, panel));
        OPS->settings_refresh(CTX);
        return 1;
    }
    name = OPS->panel_name(CTX, panel);
    if (name && strcmp(name, "Game Settings Screen") == 0) {
        if (action != 0) {
            OPS->settings_refresh(CTX);
            if (!aoe_settings_flag_97d_ror(m->settings)) {
                OPS->game_slot_48(CTX);
                m->field_88c = 0;
            }
        }
        OPS->show_screen(CTX, "MP Setup Screen");
        return 1;
    }
    if (name && strcmp(name, "Kick Dialog") == 0) {
        void *list = m->kind_lists[m->kick_player - 1];
        int32_t index = 0;
        if (action == 0) {
            int32_t id = OPS->list_id(CTX, list);
            OPS->kick_send(CTX, m->kick_player);
            OPS->comm_slot(CTX, m->kick_player, id);
            index = OPS->list_index(CTX, list, id);
        }
        OPS->list_select(CTX, list, index);
        OPS->delete_panel(CTX, panel);
        return 1;
    }
    if (action == 0) {
        for (i = 0; i < OPS->player_count(CTX); ++i) {
            if (panel == m->kind_lists[i]) {
                if (aoe_settings_flag_97e_ror(m->settings) && OPS->list_selected(CTX, panel) > 0 &&
                    OPS->slot_human(CTX, i + 1)) {
                    aoe_mp_setup_screen_kick_ror(m, i + 1);
                    return 1;
                }
                OPS->comm_slot(CTX, i + 1, OPS->list_id(CTX, panel));
                return 1;
            }
            if (panel == m->civ_lists[i]) {
                OPS->set_civ(CTX, i, OPS->list_id(CTX, panel));
                if (aoe_settings_flag_97e_ror(m->settings) && !OPS->host(CTX))
                    m->field_72c = OPS->list_id(CTX, panel);
                return 1;
            }
            if (panel == m->pos_lists[i]) {
                int32_t next = 1, s;
                if (aoe_settings_flag_97e_ror(m->settings)) {
                    OPS->set_position(CTX, i, OPS->list_id(CTX, panel));
                    if (!OPS->host(CTX)) m->field_730 = OPS->list_id(CTX, panel);
                    return 1;
                }
                /* Single player: any list sets player 0's position; the
                   others take the remaining used scenario slots in turn. */
                OPS->set_position(CTX, 0, OPS->list_id(CTX, panel));
                for (s = 0; s < 8; ++s) {
                    if (s != OPS->position_of(CTX, 0) && OPS->scenario_slot_used(CTX, s)) {
                        OPS->set_position(CTX, next, s);
                        if (++next >= OPS->player_count(CTX)) break;
                    }
                }
                OPS->update_ready(CTX);
                return 1;
            }
        }
    }
    return OPS->base_action(CTX, panel, action, a, b);
}

/* 0x0048b640 */
void aoe_mp_setup_screen_rows_ror(AoeMpSetupScreenRor *m, const AoeMpSetupRowsOpsRor *o) {
    const AoeGameSettingsRor *g = m->settings;
    int networked = aoe_settings_flag_97e_ror(g) != 0, flag_97d = aoe_settings_flag_97d_ror(g) != 0;
    int32_t count = aoe_settings_byte_988_ror(g), i, j, c;
    if (networked && o->host) {
        int changed = 0;
        for (i = 0; i < count; ++i) {
            if (!o->slot_stats(o->context, i + 1) || !o->kind_shows_unknown(o->context, i)) continue;
            for (j = 0; j < count; ++j) {
                char text[96];
                if (j == i || !o->slot_stats(o->context, j + 1)) continue;
                if (aoe_settings_byte_ac4_ror(g, j) != aoe_settings_byte_ac4_ror(g, i)) continue;
                if (o->slot_type(o->context, j + 1) != 2) continue;
                snprintf(text, sizeof text, "    SETTING CIV TEAM 50 %d %d", (int)i, (int)j);
                o->log(o->context, text);
                aoe_settings_set_byte_a94_ror(m->settings, j, (uint8_t)(aoe_settings_byte_a94_ror(g, i) + 0x32));
                aoe_settings_set_player_994_ror(m->settings, j, (uint8_t)(aoe_settings_player_994_ror(g, i) + 0x32));
                changed = 1;
            }
        }
        if (changed) o->share(o->context);
    }
    for (i = 0; i < count && i < AOE_MP_SETUP_PLAYERS; ++i) {
        int32_t kind, color, team, v;
        int ours = networked && i + 1 == o->our_slot;
        const char *text;
        uint32_t c1, c2;
        if (networked) kind = o->slot_type(o->context, i + 1);
        else kind = i == 0 ? 2 : o->list_selected_id(o->context, AOE_MPS_KIND_LIST, i);
        if (!networked || kind != 2) {
            o->set_active(o->context, AOE_MPS_KIND_LABEL, i, 0);
            o->set_active(o->context, AOE_MPS_VERSION, i, 0);
        } else {
            o->set_active(o->context, AOE_MPS_KIND_LABEL, i, aoe_settings_player_flag_ror(g, i));
            if (o->host || aoe_settings_player_version_ror(g, 0) != 0 || i == 0) {
                o->set_text(o->context, AOE_MPS_VERSION, i, aoe_settings_player_version_text_ror(g, i));
                o->set_active(o->context, AOE_MPS_VERSION, i, 1);
            } else if (ours) {
                aoe_settings_set_player_version_ror(m->settings, i, 2);
                o->set_text(o->context, AOE_MPS_VERSION, i, aoe_settings_player_version_text_ror(g, i));
                o->set_active(o->context, AOE_MPS_VERSION, i, 1);
            } else {
                o->set_active(o->context, AOE_MPS_VERSION, i, 0);
            }
        }
        color = ours && m->field_734 ? m->field_734 : aoe_settings_byte_ac4_ror(g, i);
        o->color_state(o->context, i, color, &text, &c1, &c2);
        o->set_text(o->context, AOE_MPS_COLOR_TEXT, i, text);
        o->set_colors(o->context, AOE_MPS_COLOR_TEXT, i, c1, c2);
        o->set_active(o->context, AOE_MPS_COLOR_BUTTON, i, 0);
        o->set_active(o->context, AOE_MPS_COLOR_TEXT, i, 0);
        if ((networked && ours && !m->field_85c) || (flag_97d && ours && m->max_players > 1)) {
            o->set_active(o->context, AOE_MPS_COLOR_BUTTON, i, 1);
            o->enable(o->context, AOE_MPS_COLOR_BUTTON, i);
        } else if (kind == 2) {
            o->set_active(o->context, AOE_MPS_COLOR_TEXT, i, 1);
        }
        if (ours && m->field_738) team = m->field_738 - 1;
        else { team = aoe_settings_player_994_ror(g, i); if (team > 0x31) team -= 0x32; team -= 1; }
        o->team_select(o->context, i, team);
        o->set_text(o->context, AOE_MPS_TEAM_TEXT, i, o->team_text(o->context, i));
        o->set_active(o->context, AOE_MPS_TEAM_BUTTON, i, 0);
        o->set_active(o->context, AOE_MPS_TEAM_TEXT, i, 0);
        if ((networked && ((ours && !m->field_85c) || (o->host && kind == 4 && !m->field_85c))) ||
            (flag_97d && (ours || kind == 4))) {
            o->set_active(o->context, AOE_MPS_TEAM_BUTTON, i, 1);
            o->enable(o->context, AOE_MPS_TEAM_BUTTON, i);
        } else if (kind == 2 || kind == 4) {
            o->set_active(o->context, AOE_MPS_TEAM_TEXT, i, 1);
        }
        if (networked)
            o->name_color(o->context, i, kind == 2 && !o->slot_stats(o->context, i + 1) ? 0xffu : 0xff00u);
        if (!networked) {
            if (ours) o->set_text_id(o->context, AOE_MPS_NAME, i, 0x25b3);
            else o->set_text(o->context, AOE_MPS_NAME, i, o->list_selected_text(o->context, AOE_MPS_KIND_LIST, i));
        } else {
            if (o->list_first_id(o->context, i) == 2) o->list_remove_first(o->context, i);
            if (kind == 2) o->list_insert_first(o->context, i, o->comm_name(o->context, i + 1), 2);
            o->list_select_value(o->context, AOE_MPS_KIND_LIST, i, kind);
            o->set_text(o->context, AOE_MPS_NAME, i, o->list_selected_text(o->context, AOE_MPS_KIND_LIST, i));
        }
        if (!o->list_open(o->context, AOE_MPS_CIV_LIST, i)) {
            if (ours && m->field_72c) v = m->field_72c;
            else { v = aoe_settings_byte_a94_ror(g, i); if (v > 0x31) v -= 0x32; }
            o->list_select_value(o->context, AOE_MPS_CIV_LIST, i, v);
        }
        o->set_text(o->context, AOE_MPS_CIV_TEXT, i, o->list_selected_text(o->context, AOE_MPS_CIV_LIST, i));
        if (!o->list_open(o->context, AOE_MPS_POS_LIST, i)) {
            v = ours && m->field_730 != -1 ? m->field_730 : aoe_settings_int_aa0_ror(g, i);
            o->list_select_value(o->context, AOE_MPS_POS_LIST, i, v);
        }
        o->set_text(o->context, AOE_MPS_POS_TEXT, i, o->list_selected_text(o->context, AOE_MPS_POS_LIST, i));
        if (!networked || !o->host || ours) {
            o->set_active(o->context, AOE_MPS_KIND_LIST, i, 0);
            o->set_active(o->context, AOE_MPS_NAME, i, 1);
        } else {
            o->set_active(o->context, AOE_MPS_KIND_LIST, i, 1);
            o->set_active(o->context, AOE_MPS_NAME, i, 0);
        }
        o->set_active(o->context, AOE_MPS_CIV_LIST, i, 0);
        o->set_active(o->context, AOE_MPS_CIV_TEXT, i, 0);
        if ((networked && ((ours && !m->field_85c) || (o->host && kind == 4 && !m->field_85c))) ||
            (flag_97d && (ours || kind == 4)))
            o->set_active(o->context, AOE_MPS_CIV_LIST, i, 1);
        else if (kind == 2 || kind == 4)
            o->set_active(o->context, AOE_MPS_CIV_TEXT, i, 1);
        o->set_active(o->context, AOE_MPS_POS_LIST, i, 0);
        o->set_active(o->context, AOE_MPS_POS_TEXT, i, 0);
    }
    for (; i < AOE_MP_SETUP_PLAYERS; ++i)
        for (c = 0; c < AOE_MPS_CONTROLS; ++c) o->set_active(o->context, c, i, 0);
    aoe_mp_setup_screen_fill_colors_ror(m);
}

/* 0x0048c010 */
static void mps_line(AoeMpSetupScreenRor *m, const AoeMpSetupInfoOpsRor *o, int32_t i, int32_t fmt_id,
                     const char *arg) {
    char fmt[0x100], text[0x200];
    (void)m;
    fmt[0] = '\0';
    o->string(o->context, fmt_id, fmt, sizeof fmt);
    snprintf(text, sizeof text, fmt, arg);
    o->line(o->context, i, text);
}
static void mps_line_id(AoeMpSetupScreenRor *m, const AoeMpSetupInfoOpsRor *o, int32_t i, int32_t fmt_id,
                        int32_t arg_id) {
    char arg[0x100];
    arg[0] = '\0';
    o->string(o->context, arg_id, arg, sizeof arg);
    mps_line(m, o, i, fmt_id, arg);
}
void aoe_mp_setup_screen_info_ror(AoeMpSetupScreenRor *m, const AoeMpSetupInfoOpsRor *o) {
    const AoeGameSettingsRor *g = m->settings;
    int scenario_mode = aoe_settings_flag_8fc_ror(g) != 0, reload;
    int networked = aoe_settings_flag_97e_ror(g) != 0, flag_97d = aoe_settings_flag_97d_ror(g) != 0;
    char path[0x104], text[0x200], arg[0x100], fmt[0x100];
    int32_t i, j, line, v;
    if (!scenario_mode) reload = m->settings_flag == 0;
    else reload = m->settings_flag != 0 || strcmp(aoe_settings_scenario_name_ror(g), o->stored_name) != 0;
    if (reload) {
        if (scenario_mode) {
            const char *name = aoe_settings_scenario_name_ror(g);
            if (m->scenario) { o->scenario_delete(o->context); m->scenario = 0; }
            snprintf(path, sizeof path, "%s%s.scn", o->scenario_dir, name);
            if (!o->file_exists(o->context, path)) snprintf(path, sizeof path, "%s.scx", name);
            else snprintf(path, sizeof path, "%s.scn", name);
            if (path[0] != '.') m->scenario = o->scenario_load(o->context, path);
            if (!m->scenario) {
                aoe_settings_set_flag_8fc_ror(m->settings, 0);
            } else {
                for (j = 0; j < 8; ++j) o->pos_clear(o->context, j);
                m->field_84c = o->fixed_positions(o->context);
                m->max_players = 0;
                m->max_list = 0;
                for (i = 0; i < 9; ++i) {
                    if (!o->slot_active(o->context, i)) continue;
                    ++m->max_list;
                    snprintf(text, sizeof text, "%d", (int)i + 1);
                    if (networked || (flag_97d && o->slot_kind(o->context, i) == 1)) {
                        ++m->max_players;
                        for (j = 0; j < 8; ++j) o->pos_append(o->context, j, text, i);
                    }
                }
                i = -1;
                for (j = 0; j < 8; ++j) {
                    do { ++i; } while (i < 9 && !o->slot_active(o->context, i));
                    if (i >= 9) break;
                    m->colors[j] = (uint8_t)(i + 1);
                    aoe_settings_set_int_aa0_ror(m->settings, j, i);
                    aoe_settings_set_byte_ac4_ror(m->settings, j, m->colors[j]);
                    aoe_settings_set_player_994_ror(m->settings, j, 1);
                }
            }
        }
        if (aoe_settings_flag_8fc_clear_ror(g)) {
            for (j = 0; j < 8; ++j) o->pos_clear(o->context, j);
            m->max_list = 8;
            m->max_players = 8;
            m->field_84c = 0;
            for (i = 0; i < 8; ++i) {
                m->colors[i] = (uint8_t)(i + 1);
                aoe_settings_set_int_aa0_ror(m->settings, i, i);
                aoe_settings_set_byte_ac4_ror(m->settings, i, m->colors[i]);
                aoe_settings_set_player_994_ror(m->settings, i, 1);
                snprintf(text, sizeof text, "%d", (int)i + 1);
                for (j = 0; j < 8; ++j) o->pos_append(o->context, j, text, i);
            }
        }
        if (flag_97d) aoe_mp_setup_screen_players_ror(m);
        m->settings_flag = aoe_settings_flag_8fc_clear_ror(g);
        o->store_name(o->context, m->settings_flag == 0 ? aoe_settings_scenario_name_ror(g) : "");
        memset(m->field_868, 0, sizeof m->field_868);
        scenario_mode = aoe_settings_flag_8fc_ror(g) != 0;
        if (!scenario_mode) {
            m->field_73c = 0;
        } else {
            const char *name = aoe_settings_scenario_name_ror(g);
            snprintf(path, sizeof path, "%s%s.scn", o->scenario_dir, name);
            if (!o->file_exists(o->context, path)) snprintf(path, sizeof path, "%s.scx", name);
            else snprintf(path, sizeof path, "%s.scn", name);
            m->field_73c = o->scenario_info(o->context, path);
        }
    }
    o->hide_lines(o->context);
    if (!scenario_mode) {
        arg[0] = '\0';
        o->string(o->context, aoe_settings_option_ae5_ror(g) ? 0x2617 : 0x25b5, arg, sizeof arg);
        line = 1;
    } else {
        snprintf(arg, sizeof arg, "%s", aoe_settings_scenario_name_ror(g));
        line = 3;
    }
    fmt[0] = '\0';
    o->string(o->context, 0x25ed, fmt, sizeof fmt);
    snprintf(text, sizeof text, fmt, arg);
    o->header(o->context, text);
    if (aoe_settings_flag_8fc_clear_ror(g)) {
        v = aoe_settings_map_size_ror(g);
        arg[0] = '\0';
        if (v >= 0 && v <= 5) o->string(o->context, 0x2973 + v, arg, sizeof arg);
        mps_line(m, o, line, 0x25da, arg);
        v = aoe_settings_int_a80_ror(g);
        arg[0] = '\0';
        if (v >= 0 && v <= 8) o->string(o->context, 0x296a + v, arg, sizeof arg);
        mps_line(m, o, line + 1, 0x25b6, arg);
        line += 2;
    }
    fmt[0] = '\0';
    o->string(o->context, 0x25b7, fmt, sizeof fmt);
    if (!scenario_mode) snprintf(text, sizeof text, "%s 2 - 8", fmt);
    else if (m->field_84c == 0 && m->max_list != 2) snprintf(text, sizeof text, "%s %d - %d", fmt, 2, (int)m->max_list);
    else snprintf(text, sizeof text, "%s %d", fmt, (int)m->max_list);
    o->line(o->context, line, text);
    v = aoe_settings_int_a8c_ror(g);
    if (v == 7 || v == 8) {
        char a[0x100];
        arg[0] = '\0';
        o->string(o->context, v == 7 ? 0x10e9 : 0x10ea, arg, sizeof arg);
        fmt[0] = '\0';
        o->string(o->context, 0x25be, fmt, sizeof fmt);
        snprintf(a, sizeof a, fmt, arg, (int)aoe_settings_int_a90_ror(g));
        o->line(o->context, line + 1, a);
    } else {
        mps_line_id(m, o, line + 1, 0x25bc,
                    v == 1 ? 0x10e1 : v == 9 ? 0x10ec : aoe_settings_flag_8fc_clear_ror(g) ? 0x10ec : 0x10e7);
    }
    v = aoe_settings_starting_age_ror(g);
    mps_line_id(m, o, line + 2, 0x25e4, v == 0 ? 0x10e7 : v == 1 ? 0x106e : 0x1067 + v);
    v = aoe_settings_resource_level_ror(g);
    mps_line_id(m, o, line + 3, 0x25e5, v == 0 ? 0x10e7 : 0x25e5 + v);
    mps_line_id(m, o, line + 4, 0x25e0, 0x2bd0 + aoe_settings_byte_993_ror(g));
    mps_line_id(m, o, line + 5, 0x25e9, aoe_settings_option_ad8_ror(g) ? 0xfa4 : 0xfa3);
    mps_line_id(m, o, line + 6, 0x25b8, aoe_settings_byte_985_ror(g) ? 0xfa3 : 0xfa4);
    mps_line_id(m, o, line + 7, 0x25ec, aoe_settings_option_ad9_ror(g) ? 0xfa3 : 0xfa4);
    line += 8;
    if (networked) mps_line_id(m, o, line++, 0x25bb, aoe_settings_byte_982_ror(g) ? 0xfa3 : 0xfa4);
    mps_line_id(m, o, line, 0x25f1,
                0x260e + (networked ? aoe_settings_byte_983_ror(g) : aoe_settings_byte_a14_ror(g)));
    fmt[0] = '\0';
    o->string(o->context, 0x25f0, fmt, sizeof fmt);
    snprintf(text, sizeof text, fmt, (int)aoe_settings_population_limit_ror(g));
    o->line(o->context, line + 1, text);
    if (flag_97d)
        o->count_controls(o->context, !scenario_mode || (m->field_84c == 0 && m->max_list > 2));
    o->rows(o->context);
}
