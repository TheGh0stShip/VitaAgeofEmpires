#include "game_ui.h"
#include "mp_setup_panels.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

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

/* 0x004894e0 up to its base idle (0x00453360), which the caller runs:
   the state's op, or the panel slot when bound to the screen's vtable. */
static void mp_setup_idle(AoeMpSetupScreenRor *m) {
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
}

/* 0x004894e0 */
void aoe_mp_setup_screen_idle_ror(AoeMpSetupScreenRor *m) {
    mp_setup_idle(m);
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

/* 0x004895b0 up to its base handler (0x00457e50): 1 when handled, 0 for
   the caller's base (the state's op, or the easy panel's slot when bound
   to the screen's vtable). */
static int mp_setup_action(AoeMpSetupScreenRor *m, void *panel, int32_t action) {
    const char *name;
    int32_t i;
    if (!panel) return 0;
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
    return 0;
}

/* 0x004895b0 */
int aoe_mp_setup_screen_action_ror(AoeMpSetupScreenRor *m, void *panel, int32_t action,
                                   int32_t a, int32_t b) {
    if (mp_setup_action(m, panel, action)) return 1;
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

/* 0x0048abc0 */
static void mpc_log(const AoeMpSetupCommandOpsRor *o, const char *name, int32_t code, int32_t arg) {
    char text[128];
    snprintf(text, sizeof text, "$$$ USER_COMMAND: %s (%d,%d)", name, (int)code, (int)arg);
    o->log(o->context, text);
}
static void mpc_reduce(AoeMpSetupScreenRor *m, const AoeMpSetupCommandOpsRor *o, int32_t i, int ours,
                       const char *civ_text, const char *team_text, const char *my_civ,
                       const char *my_team, int *changed) {
    const AoeGameSettingsRor *g = m->settings;
    if (aoe_settings_byte_a94_ror(g, i) > 0x31) {
        o->log(o->context, civ_text);
        aoe_settings_set_byte_a94_ror(m->settings, i, (uint8_t)(aoe_settings_byte_a94_ror(g, i) - 0x32));
        if (changed) *changed = 1;
        if (ours && my_civ) { m->field_72c = 0; o->log(o->context, my_civ); }
    }
    if (aoe_settings_player_994_ror(g, i) > 0x31) {
        o->log(o->context, team_text);
        aoe_settings_set_player_994_ror(m->settings, i, (uint8_t)(aoe_settings_player_994_ror(g, i) - 0x32));
        if (changed) *changed = 1;
        if (ours && my_team) { m->field_738 = 0; o->log(o->context, my_team); }
    }
}
int aoe_mp_setup_screen_command_ror(AoeMpSetupScreenRor *m, const AoeMpSetupCommandOpsRor *o,
                                    int32_t code, int32_t arg) {
    const AoeGameSettingsRor *g = m->settings;
    char fmt[0x100], text[0x100];
    int32_t i, count;
    if (aoe_settings_flag_97d_ror(g)) return 1;
    switch (code) {
    case 0x17a4:
        mpc_log(o, "COMM_UPDATE_PLAYERS", code, arg);
        if (o->host && !m->field_854) {
            m->field_854 = 1;
            if (o->cd_ok) aoe_settings_set_player_flag_ror(m->settings, o->our_slot - 1, 1);
        }
        if (o->host) {
            int changed = 0;
            for (i = 0; i < 8; ++i) {
                int32_t slot = i + 1, type = o->slot_type(o->context, slot);
                if ((slot == o->our_slot && type == 2)) continue;
                if (slot != o->our_slot && type == 2) {
                    int32_t st[7];
                    o->slot_stats(o->context, slot, st);
                    if (st[0] && st[0] != aoe_settings_byte_a94_ror(g, i)) { aoe_settings_set_byte_a94_ror(m->settings, i, (uint8_t)st[0]); changed = 1; }
                    if (st[1] && st[1] - 1 != aoe_settings_int_aa0_ror(g, i)) { aoe_settings_set_int_aa0_ror(m->settings, i, st[1] - 1); changed = 1; }
                    if (st[2] == 1 && aoe_settings_player_flag_ror(g, i) == 1) { aoe_settings_set_player_flag_ror(m->settings, i, 0); changed = 1; }
                    else if (st[2] == 2 && aoe_settings_player_flag_ror(g, i) == 0) { aoe_settings_set_player_flag_ror(m->settings, i, 1); changed = 1; }
                    if (st[3] && st[3] != aoe_settings_byte_ac4_ror(g, i)) { aoe_settings_set_byte_ac4_ror(m->settings, i, (uint8_t)st[3]); changed = 1; }
                    if (st[4] && st[4] != aoe_settings_player_994_ror(g, i)) { aoe_settings_set_player_994_ror(m->settings, i, (uint8_t)st[4]); changed = 1; }
                    if (st[5]) m->field_868[i] = st[5];
                    if (st[6]) aoe_settings_set_player_version_ror(m->settings, i, (uint8_t)st[6]);
                } else if (type != 4) {
                    int32_t civ = o->rand_(o->context) % 16 + 1;
                    if (aoe_settings_byte_a94_ror(g, i) != civ) { aoe_settings_set_byte_a94_ror(m->settings, i, (uint8_t)civ); changed = 1; }
                    if (aoe_settings_int_aa0_ror(g, i) != i) { aoe_settings_set_int_aa0_ror(m->settings, i, i); changed = 1; }
                    if (aoe_settings_player_flag_ror(g, i)) { aoe_settings_set_player_flag_ror(m->settings, i, 0); changed = 1; }
                    if (aoe_settings_byte_ac4_ror(g, i) != m->colors[i]) { aoe_settings_set_byte_ac4_ror(m->settings, i, m->colors[i]); changed = 1; }
                    if (aoe_settings_player_994_ror(g, i) != 1) { aoe_settings_set_player_994_ror(m->settings, i, 1); changed = 1; }
                    if (aoe_settings_player_version_ror(g, i)) { aoe_settings_set_player_version_ror(m->settings, i, 0); changed = 1; }
                    m->field_868[i] = 0;
                }
            }
            if (changed) { o->step(o->context, AOE_MPC_SHARE, 0); m->field_88c = 0; }
        }
        o->step(o->context, AOE_MPC_ROWS, 0);
        o->step(o->context, AOE_MPC_REDRAW, 1);
        return 1;
    case 0x17a6:
        mpc_log(o, "COMM_UPDATE_PARAMS", code, arg);
        o->step(o->context, AOE_MPC_APPLY_PARAMS, 0);
        count = aoe_settings_byte_988_ror(g);
        for (i = 0; i < count; ++i)
            mpc_reduce(m, o, i, i + 1 == o->our_slot, "$$$ CIV2>50", "$$$ TEAM2>50",
                       "$$$ RESETTING MYCIV2", "$$$ RESETTING MYTEAM2", NULL);
        if (m->field_730 != 0 && strcmp(m->stored_name ? m->stored_name : "", aoe_settings_scenario_name_ror(g)) != 0)
            m->field_730 = -1;
        o->step(o->context, AOE_MPC_INFO, 0);
        if (!o->host) {
            if (m->field_854 && m->field_73c == m->field_844 &&
                o->slot_stat(o->context, o->our_slot) == m->field_85c)
                return 1;
            m->field_854 = 1;
            m->field_844 = m->field_73c;
            o->send_ready(o->context, m->field_85c, o->cd_ok + 1, m->field_73c);
        }
        return 1;
    case 0x17a7:
        mpc_log(o, "COMM_LAUNCH_GAME", code, arg);
        o->step(o->context, AOE_MPC_LAUNCH, 0);
        return 1;
    case 0x17a8: mpc_log(o, "COMM_CANCEL_GAME", code, arg); o->step(o->context, AOE_MPC_READY, 0); return 1;
    case 0x17a9: mpc_log(o, "COMM_HOST_EXITED", code, arg); o->step(o->context, AOE_MPC_READY, 0); return 1;
    case 0x17aa: mpc_log(o, "COMM_PLAYER_KICKED", code, arg); o->step(o->context, AOE_MPC_READY, 1); return 1;
    case 0x17b2: mpc_log(o, "COMM_PLAYER_DROPPED", code, arg); o->step(o->context, AOE_MPC_READY, 1); return 1;
    case 0x17ae: mpc_log(o, "COMM_NO_LINK", code, arg); o->step(o->context, AOE_MPC_LEAVE, 0); return 1;
    case 0x17b3: mpc_log(o, "COMM_OTHER_PLAYER_DROPPED", code, arg); o->step(o->context, AOE_MPC_ROWS, 0); return 1;
    case 0x17b6: case 0x17b7:
        mpc_log(o, code == 0x17b6 ? "COMM_BAD_VERSION" : "COMM_PLAYER_SERVICE_ERROR", code, arg);
        fmt[0] = '\0';
        o->string(o->context, code == 0x17b6 ? 0x25dd : 0x25de, fmt, sizeof fmt);
        snprintf(text, sizeof text, fmt, o->comm_name(o->context, arg));
        o->ok_dialog(o->context, text);
        return 1;
    case 0x17b9:
        mpc_log(o, "COMM_HOST_BUSY", code, arg);
        if (o->launched) o->step(o->context, AOE_MPC_LEAVE, 1);
        else o->step(o->context, AOE_MPC_READY, 0);
        return 1;
    case 0x17bb:
        mpc_log(o, "COMM_SHARED_DATA_SENT", code, arg);
        if (o->host) {
            int changed = 0;
            count = aoe_settings_byte_988_ror(g);
            for (i = 0; i < count; ++i)
                mpc_reduce(m, o, i, 0, "$$$ HOST RESET CIV+50", "$$$ HOST RESET TEAM+50", NULL, NULL, &changed);
            if (changed) m->field_88c = 1;
        }
        return 1;
    case 0x17d5:
        mpc_log(o, "CHAT_RECEIVED", code, arg);
        if (!o->iconic) { o->step(o->context, AOE_MPC_CHAT, arg); return 1; }
        m->chat_to = arg;
        if (m->chat_from == 0) m->chat_from = arg;
        return 1;
    default:
        snprintf(text, sizeof text, "$$$ USER_COMMAND: UNKNOWN (%d, %d)", (int)code, (int)arg);
        o->log(o->context, text);
        return 1;
    }
}


/* ---- 0x00424c80: the comm's session launch ---- */

static void launch_log(const AoeCommRor *c, const char *text) {
    if (c->ops && c->ops->log) c->ops->log(c->ops->context, text);
}

/* 0x00424c80 */
void aoe_mp_setup_comm_launch_ror(AoeCommRor *c, const AoeMpSetupLaunchOpsRor *o) {
    char line[64];
    uint32_t slot;
    int32_t serial;
    o->enable_new_players(o->context, 0); /* 0x004258d0(0x00580eac, 0) */
    c->state = 5;
    o->active_range(o->context);          /* 0x0041e130 */
    c->field_1718 = 1;
    o->gtd_reset(o->context);             /* 0x00421a20 */
    o->compact_players(o->context);       /* 0x00424760 */
    for (slot = 1, serial = 2000; slot <= c->count && slot < AOE_COMM_SLOTS;
         ++slot, serial += 2000) {
        if (c->dpid[slot] == 0) continue;
        c->rx_serial[slot] = serial;
        snprintf(line, sizeof line, "Set expected ser# for player %d to %d", (int)slot,
                 (int)(serial + 1));
        launch_log(c, line);
    }
    c->field_14e4 = (int32_t)(c->self * 2000u + 1u);
    snprintf(line, sizeof line, "HOST Set own serial to =%d", (int)c->field_14e4);
    launch_log(c, line);
    o->share_data(o->context, 1);         /* 0x00420640(1) */
    c->turn = (uint32_t)c->lead + 1u;
    if (c->multiplayer) {
        launch_log(c, "Clear player acknowledgements");
        for (slot = c->low; slot <= c->high && slot < AOE_COMM_SLOTS; ++slot)
            if (slot != c->self) c->last_turn[slot] = 0;
    }
    o->reset_ping_times(o->context);      /* 0x00423970 */
    c->ops->post(c->ops->context, 0x17a6, 0); /* 0x004200d0(0x17a6) */
}

/* ---- 0x00487f40: the screen as panels (game_ui.h) ---- */

#define MU(p) ((AoeMpSetupUiRor *)(p))

static const char *const NAME = "MP Setup Screen";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

static void help(void *panel, int32_t id, int32_t page) {
    aoe_ui_panel_set_help_ror((AoeUiPanelRor *)panel, id, page);
}

/* 0x004891a0 (deleting 0x00488c20): the scenario freed and the dialogs
   closed through the ops, then every panel. */
static void mu_destroy(AoeUiPanelRor *p) {
    AoeMpSetupUiRor *u = MU(p);
    int i;
    aoe_mp_setup_screen_destroy_ror(&u->state);
    DROP(u->title);
    DROP(u->name_label);
    DROP(u->civ_label);
    DROP(u->settings_label);
    DROP(u->color_label);
    DROP(u->team_label);
    for (i = 0; i < AOE_MP_SETUP_PLAYERS; ++i) {
        DROP(u->texts_490[i]);
        DROP(u->texts_4b0[i]);
        DROP(u->texts_4d0[i]);
        DROP(u->ready_texts[i]);
        DROP(u->texts_510[i]);
        DROP(u->texts_530[i]);
        DROP(u->texts_550[i]);
        DROP(u->color_buttons[i]);
        DROP(u->team_buttons[i]);
        DROP(u->kind_lists[i]);
        DROP(u->civ_lists[i]);
        DROP(u->pos_lists[i]);
    }
    DROP(u->chat_input);
    DROP(u->chat_label);
    DROP(u->chat);
    DROP(u->chat_bar);
    DROP(u->settings);
    DROP(u->info_title);
    for (i = 0; i < AOE_MP_SETUP_INFO_LINES; ++i) DROP(u->info[i]);
    DROP(u->count_label);
    DROP(u->count);
    DROP(u->start);
    DROP(u->cancel);
    DROP(u->help);
    DROP(u->ready);
    DROP(u->hidden);
    DROP(u->ready_label);
    DROP(u->addresses);
    aoe_ui_ez_release_ror(&u->ez);
}

/* The creators on the screen's own panel. */
#define TEXT_ID(out, ...) aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &(out), __VA_ARGS__)
#define TEXT(out, ...) aoe_ui_ez_create_text_ror(ez, &ez->panel, &(out), __VA_ARGS__)
#define DROPDOWN(out, ...) aoe_ui_ez_create_dropdown_ror(ez, &ez->panel, &(out), __VA_ARGS__)
#define BUTTON(out, ...) aoe_ui_ez_create_button_ror(ez, &ez->panel, &(out), __VA_ARGS__)
#define BUTTON_ID(out, ...) aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &(out), __VA_ARGS__)
#define CHECK(out, ...) aoe_ui_ez_create_check_ror(ez, &ez->panel, &(out), __VA_ARGS__)

/* 0x00487f40 */
int aoe_mp_setup_ui_init_ror(AoeMpSetupUiRor *u, AoeGameUiRor *ui, AoeUiAreaRor *area,
                             const AoeMpSetupScreenOpsRor *ops) {
    AoeMpSetupScreenRor *m = &u->state;
    AoeUiEzRor *ez = &u->ez;
    AoeGameSettingsRor *st = ui->settings;
    AoeUiPanelRor *tab[4], *focus;
    int net, single, host, i, k, n;
    memset(u, 0, sizeof *u);
    u->ui = ui;
    m->settings = st;
    m->ops = ops;
    /* "$$$ CHAT SCREEN: CONSTRUCTOR BEGIN" (0x0042c330) is empty in this
       build, as are the two logs further down. */
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_mp_setup_ui_vtable_ror.base;
    aoe_mp_setup_screen_reset_ror(m); /* 0x00488c40 */
    OPS->chat_drop(CTX);              /* 0x0041ca10 */
    for (i = 1; i <= AOE_MP_SETUP_PLAYERS; ++i) OPS->chat_recipient(CTX, i); /* 0x0041ca50(i, 1) */
    net = aoe_settings_flag_97e_ror(st) != 0;
    single = aoe_settings_flag_97d_ror(st) != 0;
    host = OPS->host(CTX) != 0;
    if (!aoe_ui_screen_setup_ror(ez, area, net ? "scr3" : "scr2", net ? 0xc385 : 0xc384, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_shadow_area_ror(ez, 0);
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    if (!TEXT_ID(u->title, net ? 0x25ce : 0x25cf, 0x14, 6, 600, 0x1e, 1, 1, 0, 0) ||
        !TEXT_ID(u->name_label, 0x25d0, 0x1a, 0x32, 0xa0, 0x1e, 0, 0, 1, 0) ||
        !TEXT_ID(u->civ_label, 0x25d1, 0xbd, 0x32, 0xa0, 0x1e, 0, 0, 1, 0))
        return 1;
    if (!single && !host &&
        !TEXT_ID(u->settings_label, 0x25d2, 0x1a4, 0x32, 0xd2, 0x1e, 0, 0, 1, 0))
        return 1;
    if (!TEXT_ID(u->color_label, 0x25ae, 0xf0, 0x32, 100, 0x1e, 0, 0, 1, 0)) return 1;
    aoe_ui_text_set_alignment_ror(u->color_label, 0, 2);
    if (!TEXT_ID(u->team_label, 0x25af, 0x136, 0x32, 100, 0x1e, 0, 0, 1, 0)) return 1;
    aoe_ui_text_set_alignment_ror(u->team_label, 0, 2);
    for (i = 0; i < AOE_MP_SETUP_PLAYERS; ++i) {
        int32_t y = 0x55 + 0x18 * i;
        AoeUiDropdownRor *d;
        AoeUiButtonRor *b;
        if (!DROPDOWN(u->kind_lists[i], 0x9e, 100, 7, y - 1, 0x9e, 0x18, 0xb)) return 1;
        d = u->kind_lists[i];
        aoe_ui_dropdown_set_button_left_ror(d, 1);
        aoe_ui_dropdown_set_border_ror(d, 0);
        help(d, 0x75f9, -1);
        aoe_ui_dropdown_clear_ror(d);
        aoe_ui_dropdown_append_id_ror(d, 0x25d3, 4);
        if (net) aoe_ui_dropdown_append_id_ror(d, 0x25d4, 1);
        aoe_ui_dropdown_append_id_ror(d, net ? 0x25d5 : 0x25d6, 0);
        if (!DROPDOWN(u->civ_lists[i], 0x80, 100, 0xaa, y - 1, 0x80, 0x18, 0xb)) return 1;
        d = u->civ_lists[i];
        aoe_ui_dropdown_set_button_left_ror(d, 1);
        aoe_ui_dropdown_set_border_ror(d, 0);
        help(d, 0x75fa, 0x25f09);
        aoe_ui_dropdown_clear_ror(d);
        for (k = 0; k < 12; ++k) aoe_ui_dropdown_append_id_ror(d, 0x27f7 + k, k + 1);
        aoe_ui_dropdown_append_id_ror(d, 0x2806, 13);
        aoe_ui_dropdown_append_id_ror(d, 0x2807, 14);
        aoe_ui_dropdown_append_id_ror(d, 0x2809, 15);
        aoe_ui_dropdown_append_id_ror(d, 0x2808, 16);
        aoe_ui_dropdown_append_id_ror(d, 0x280a, 17);
        if (!DROPDOWN(u->pos_lists[i], 0x28, 100, 0x144, y - 1, 0x28, 0x18, 0xb)) return 1;
        d = u->pos_lists[i];
        aoe_ui_dropdown_set_button_left_ror(d, 1);
        aoe_ui_dropdown_set_border_ror(d, 0);
        help(d, 0x75fb, -1);
        if (!BUTTON(u->color_buttons[i], "", NULL, 0x136, y, 0x1e, 0x14, 0xb, 0, 1)) return 1;
        help(u->color_buttons[i], 0x7604, -1);
        if (!TEXT(u->texts_530[i], "", 0x131, y, 0x28, 0x16, 0xb, 1, 1, 0)) return 1;
        if (!BUTTON_ID(u->team_buttons[i], 0x25b0, 0, 0x17c, y, 0x1e, 0x14, 0xb, 0, 1)) return 1;
        b = u->team_buttons[i];
        help(b, 0x7603, -1);
        aoe_ui_button_set_cycle_ror(b, 5);
        for (k = 1; k <= 4; ++k) {
            char text[8];
            snprintf(text, sizeof text, "%d", k);
            aoe_ui_button_set_text_ror(b, (int16_t)k, text);
            aoe_ui_button_set_id_ror(b, (int16_t)k, k + 1, 0);
        }
        if (!TEXT(u->texts_550[i], "", 0x177, y, 0x28, 0x16, 0xb, 1, 1, 0) ||
            !TEXT(u->texts_490[i], "", 0x1c, y + 2, 0x8b, 0x16, 0xb, 0, 0, 0) ||
            !TEXT(u->texts_4b0[i], "", 0xbf, y + 2, 0x6d, 0x16, 0xb, 0, 0, 0) ||
            !TEXT(u->texts_4d0[i], "", 0x159, y + 2, 0x28, 0x16, 0xb, 0, 0, 0) ||
            !TEXT_ID(u->ready_texts[i], 0x25df, 0x151, y + 3, 0x14, 0x16, 6, 0, 0, 0) ||
            !TEXT(u->texts_510[i], "", 0x161, y + 3, 0x1d, 0x16, 6, 0, 0, 0))
            return 1;
        u->ready_texts[i]->panel.vt->set_active(&u->ready_texts[i]->panel, 0);
    }
    if (single || host) {
        if (!BUTTON_ID(u->settings, 0x25d2, 0, 0x1a4, 0x32, 0xd2, 0x1e, 0, 0, 0)) return 1;
        help(u->settings, 0x75fc, -1);
    }
    if (!TEXT(u->info_title, "", 0x1a4, 0x54, 0xdc, 0x44, 0xb, 0, 0, 1)) return 1;
    for (k = 0; k < AOE_MP_SETUP_INFO_LINES; ++k)
        if (!TEXT(u->info[k], "", 0x1a4, 0x53 + 0x18 * k, 0xdc, 0x14, 0xb, 0, 0, 0)) return 1;
    if (net) {
        if (!TEXT_ID(u->chat_label, 0x25d7, 10, 0x118, 100, 0x14, 4, 0, 1, 0) ||
            !TEXT(u->chat, "", 10, 300, 400, 0x60, 10, 0, 0, 0) ||
            !aoe_ui_ez_create_text_scrollbar_ror(ez, &u->chat_bar, u->chat, 0x14) ||
            !aoe_ui_ez_create_edit_ror(ez, &ez->panel, &u->chat_input, "", 0xf7, 0, 10, 0x192,
                                       400, 0x17, 10, 1, 0))
            return 1;
        help(u->chat_input, 0x75fe, -1);
        ((const AoeUiTextVtblRor *)u->chat->panel.vt)->set_bevel(u->chat, 3, ez->bevel);
        help(u->chat, 0x75fd, -1);
        aoe_ui_text_clear_ror(u->chat);
    }
    if (single) {
        if (!TEXT_ID(u->count_label, 0x25d8, 0x1a, 0x127, 0x14f, 0x14, 0, 0, 1, 0) ||
            !DROPDOWN(u->count, 0x46, 100, 0x1f, 0x142, 0x46, 0x18, 0xb))
            return 1;
        help(u->count, 0x7600, -1);
    }
    m->field_85c = 0;
    if (!single) {
        if (host) {
            if (!CHECK(u->ready, 10, 0x1b8, 0x1e, 0x1e, 0, 0) ||
                !TEXT_ID(u->ready_label, 0x25bf, 0x2d, 0x1b8, 0x96, 0x1e, 0, 0, 1, 0))
                return 1;
            help(u->ready_label, 0x7602, -1);
            if (!BUTTON(u->start, "", NULL, 0xc3, 0x1b8, 200, 0x1e, 0, 0, 0)) return 1;
            aoe_ui_button_set_text_id_ror(u->start, 0, 0x25ee);
            help(u->start, 0x75ff, -1);
            aoe_ui_button_set_disabled_ror(u->start, 1);
            if (!BUTTON_ID(u->cancel, 0xfa2, 0, 0x195, 0x1b8, 0xb4, 0x1e, 0, 0, 0)) return 1;
            help(u->cancel, 0x7532, -1);
            if (!BUTTON_ID(u->addresses, 0x25ef, 0, 600, 400, 0x1e, 0x1e, 0, 0, 0)) return 1;
            help(u->addresses, 0x7601, -1);
            OPS->send_ready(CTX, 0, 0, 0, 0, 0, 0, 0, 0); /* 0x004251c0 */
        } else {
            if (!CHECK(u->ready, 100, 0x1b8, 0x1e, 0x1e, 0, 0) ||
                !TEXT_ID(u->ready_label, 0x25bf, 0x87, 0x1b8, 0x96, 0x1e, 0, 0, 1, 0))
                return 1;
            help(u->ready_label, 0x7602, -1);
            if (!BUTTON_ID(u->cancel, 0xfa2, 0, 0x14a, 0x1b8, 0xf0, 0x1e, 0, 0, 0)) return 1;
            help(u->cancel, 0x7532, -1);
        }
    } else {
        if (!BUTTON(u->start, "", NULL, 0x46, 0x1b8, 0xf0, 0x1e, 0, 0, 0)) return 1;
        aoe_ui_button_set_text_id_ror(u->start, 0, 0x25ee);
        help(u->start, 0x75ff, -1);
        aoe_ui_panel_set_current_ror(&ez->panel, &u->start->panel);
        if (!BUTTON_ID(u->cancel, 0xfa2, 0, 0x14a, 0x1b8, 0xf0, 0x1e, 0, 0, 0)) return 1;
        help(u->cancel, 0x7532, -1);
    }
    if (!BUTTON_ID(u->help, 0xfa9, 0, 600, 0x1b8, 0x1e, 0x1e, 0, 0, 0) ||
        !BUTTON_ID(u->hidden, 0x3ea, 0, 0, 0, 0, 0, -1, -1, 0))
        return 1;
    u->hidden->panel.vt->set_active(&u->hidden->panel, 1);
    u->hidden->panel.vt->set_position_mode(&u->hidden->panel, 9, 4, 4, 4, 4, 0x11, 0x11, 0x11,
                                           0x11, NULL, NULL, NULL, NULL);
    u->cancel->hotkey = 0x1b;
    u->cancel->hotkey_shift = 0;
    /* The state's slots for the handlers that take panels. */
    m->panel_610 = u->chat_input;
    m->panel_620 = u->settings;
    m->panel_700 = u->start;
    m->panel_70c = u->cancel;
    m->panel_710 = u->help;
    m->panel_714 = u->ready;
    m->panel_718 = u->hidden;
    m->panel_6e0 = u->count;
    m->panel_858 = u->addresses;
    for (i = 0; i < AOE_MP_SETUP_PLAYERS; ++i) {
        m->color_buttons[i] = u->color_buttons[i];
        m->team_buttons[i] = u->team_buttons[i];
        m->kind_lists[i] = u->kind_lists[i];
        m->civ_lists[i] = u->civ_lists[i];
        m->pos_lists[i] = u->pos_lists[i];
    }
    /* 0x0048ced0, the tab order, on the panels themselves. */
    n = 0;
    if (single) {
        tab[n++] = &u->start->panel;
        tab[n++] = &u->cancel->panel;
        tab[n++] = &u->settings->panel;
        focus = &u->start->panel;
    } else if (u->settings) {
        tab[n++] = &u->chat_input->panel;
        tab[n++] = &u->start->panel;
        tab[n++] = &u->cancel->panel;
        tab[n++] = &u->settings->panel;
        focus = &u->chat_input->panel;
    } else {
        tab[n++] = &u->chat_input->panel;
        tab[n++] = &u->ready->panel;
        tab[n++] = &u->cancel->panel;
        focus = &u->chat_input->panel;
    }
    aoe_ui_panels_tab_cycle_ror(tab, (int16_t)n);
    aoe_ui_panel_set_current_ror(&ez->panel, focus);
    /* 0x0048c010 (the summary; "$$$ CHAT SCREEN: UPDATE SUMMARY" logged
       first) and 0x0048b640 (the rows) need their panel ops
       (aoe_mp_setup_screen_info_ror / _rows_ror) and are not run here. */
    if (single) aoe_mp_setup_screen_players_ror(m); /* 0x0048cfb0 */
    OPS->send_chat(CTX, "\x03\x02\x01");            /* 0x004213e0: the bytes 3, 2, 1 */
    /* "$$$ CHAT SCREEN: CONSTRUCTOR DONE" is the third empty log. */
    return 1;
}

/* +0x4c 0x004894e0 on the panels: the state's idle (pending chat lines
   into the chat text, the capture, the multiplayer data share), then the
   panel's base idle 0x00453360 instead of the state's base_idle op. */
int32_t aoe_mp_setup_ui_idle_ror(AoeUiPanelRor *p) {
    mp_setup_idle(&MU(p)->state);
    return aoe_ui_panel_handle_idle_ror(p);
}

/* +0xb8 0x004895b0 on the panels: the panel that fired (start, ready,
   settings, help, quit, cancel, addresses, colour / team buttons, chat
   input, player count, kind / civilisation / position lists, the closing
   "Game Settings Screen" and "Kick Dialog") handled by the state through
   its ops; anything else goes to the easy panel's 0x00457e50 instead of
   the state's base_action op. */
int32_t aoe_mp_setup_ui_action_ror(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code,
                                   int32_t value, int32_t extra) {
    if (mp_setup_action(&MU(p)->state, from, code)) return 1;
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

/* 0x00546d7c: +0x00 0x00488c20, +0x4c 0x004894e0, +0xb8 0x004895b0;
   +0x64 (0x0048abc0, the comm user commands) needs its command ops and
   stays the base slot here, as does +0x68 (0x0048b630). */
const AoeUiEzVtblRor aoe_mp_setup_ui_vtable_ror = {
    AOE_UI_EZ_SLOTS_MDU(mu_destroy, aoe_ui_ez_draw_slot_ror, aoe_mp_setup_ui_idle_ror,
                        aoe_ui_panel_handle_paint_ror, aoe_ui_ez_no_key_action_ror,
                        aoe_ui_panel_zero_ror, aoe_mp_setup_ui_action_ror,
                        aoe_ui_screen_focus_slot_ror, aoe_ui_ez_mouse_down_slot_ror,
                        aoe_ui_panel_ignore_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
