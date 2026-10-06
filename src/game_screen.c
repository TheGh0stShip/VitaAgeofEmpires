#include "game_screen.h"

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#define OPS (s->ops)
#define CTX (s->ops->context)

void aoe_game_screen_mouse_ror(AoeGameScreenRor *s, int32_t a, int32_t b, int32_t c, int32_t d,
                               int32_t e) {
    int i;
    for (i = 0; i < 6; ++i) OPS->message_hide(CTX, s->lines[i]);
    OPS->base_mouse(CTX, a, b, c, d, e);
}

void aoe_game_screen_show_584_ror(AoeGameScreenRor *s) {
    OPS->set_text_id(CTX, s->panel_584, 0x2329);
    if (!OPS->active(CTX, s->panel_584)) {
        OPS->set_active(CTX, s->panel_584, 1);
        OPS->redraw(CTX, s->panel_584, 1);
    }
    OPS->redraw(CTX, s->view, 1);
}

void aoe_game_screen_hide_584_ror(AoeGameScreenRor *s) {
    if (!OPS->active(CTX, s->panel_584)) return;
    OPS->set_active(CTX, s->panel_584, 0);
    OPS->redraw(CTX, s->panel_584, 1);
    OPS->redraw(CTX, s->view, 1);
}

void aoe_game_screen_chat_text_ror(AoeGameScreenRor *s, const char *text) {
    OPS->message(CTX, s->chat[s->chat_next], 3, text, 0xff);
    if (++s->chat_next > 7) s->chat_next = 0;
}

void aoe_game_screen_chat_line_ror(AoeGameScreenRor *s, int32_t i) {
    char line[0x100];
    const char *text = OPS->chat_line(CTX, i);
    /* The original copies without a bound into 256 bytes. */
    strncpy(line, text ? text : "", sizeof line - 1);
    line[sizeof line - 1] = '\0';
    aoe_game_screen_chat_text_ror(s, line);
}

void aoe_game_screen_age_ror(AoeGameScreenRor *s) {
    char name[0x200];
    if (!s->age_text) return;
    s->age = (int32_t)OPS->age(CTX);
    name[0] = '\0';
    OPS->format(CTX, 0x65, s->age, 0, name, sizeof name);
    OPS->set_text(CTX, s->age_text, name);
    OPS->redraw(CTX, s->age_text, 1);
    if (s->age >= 1 && s->age <= 4) OPS->help(CTX, s->age_text, 0x1068 + s->age, -1);
}

void aoe_game_screen_title_ror(AoeGameScreenRor *s, const char *text) {
    const char *now = OPS->text(CTX, s->title);
    if (now && strcmp(now, text) == 0) return;
    OPS->set_text(CTX, s->title, text);
}

void aoe_game_screen_lines_ror(AoeGameScreenRor *s, const char *const text[6]) {
    int i;
    for (i = 0; i < 6; ++i) OPS->message(CTX, s->lines[i], 2, text[i], 0x56);
}

void aoe_game_screen_line_ror(AoeGameScreenRor *s, int32_t style, const char *text,
                              uint8_t color) {
    OPS->message(CTX, s->lines[0], style, text, color);
}

void aoe_game_screen_hide_unused_ror(AoeGameScreenRor *s) {
    int i;
    for (i = 0; i < AOE_GAME_SCREEN_BUTTONS; ++i) {
        void *b = s->buttons[i];
        if (!OPS->button_used(CTX, b)) {
            OPS->set_active(CTX, b, 0);
            OPS->button_state(CTX, b, 0, -1);
            OPS->help(CTX, b, -1, -1);
        } else {
            OPS->set_active(CTX, b, 1);
        }
    }
}

uint32_t aoe_game_screen_button_slot_ror(uint8_t slot) {
    if (slot != 0 && slot < 6) return slot - 1u;
    if (slot > 5 && slot < 11) return slot;
    if (slot > 10 && slot < 16) return slot - 11u;
    if (slot > 15 && slot < 21) return slot - 10u;
    return 0;
}

static int32_t hotkey_string(int32_t command, int32_t id) {
    switch (command) {
    case 1: return 0x104b;
    case 2: return 0x104a;
    case 3: return 0x1037;
    case 5: return 0x1038;
    case 7: return 0x104e;
    case 8: return 0x1039;
    case 9: return 0x103a;
    case 0xc: return 0x103b;
    case 0x16: return 0x103d;
    case 0x17: return 0x103e;
    case 0x1c: return 0x1048;
    case 0x1d: return 0x1049;
    case 0x1e: return 0x104c;
    case 0x1f: return 0x104d;
    default: return id;
    }
}

void aoe_game_screen_set_button_ror(AoeGameScreenRor *s, const AoeGameScreenButtonRor *b) {
    void *btn = s->buttons[b->index];
    char name[0x200];
    int32_t id, hotkey = 0;
    OPS->button_set_used(CTX, btn, 1);
    if (OPS->button_disabled(CTX, btn) != b->disabled)
        OPS->button_set_disabled(CTX, btn, b->disabled);
    if (OPS->button_id(CTX, btn) == b->command && OPS->button_value(CTX, btn) == b->value) return;
    OPS->button_drop_press(CTX, btn);
    OPS->button_reset(CTX, btn);
    name[0] = '\0';
    OPS->format(CTX, 0x66, b->command, b->value, name, sizeof name);
    OPS->button_picture(CTX, btn, b->picture, b->frame);
    OPS->button_state(CTX, btn, b->command, b->value);
    OPS->button_lines(CTX, btn, name, b->line2);
    OPS->help(CTX, btn, b->help1, b->help2);
    OPS->button_help_line(CTX, btn, b->help ? b->help : name);
    id = hotkey_string(b->command, b->hotkey_id);
    if (id != 0) {
        const char *key = OPS->string(CTX, id);
        hotkey = (int32_t)(signed char)key[0];
    }
    if (b->command == 6 || b->command == 0xb) hotkey = 0x1b;
    OPS->button_fields(CTX, btn, b->field_2c8, b->command == 0x11 ? 2 : 0, hotkey);
    OPS->set_active(CTX, btn, 1);
}

/* An earlier living player mutually allied with `player`. */
static int allied_before(const AoeGameScreenRor *s, int32_t player) {
    int32_t j;
    if (!OPS->player_alive(CTX, player)) return 0;
    for (j = 1; j < player; ++j) {
        if (OPS->stance(CTX, player, j) == 0 && OPS->stance(CTX, j, player) == 0 &&
            OPS->player_alive(CTX, j))
            return 1;
    }
    return 0;
}

void aoe_game_screen_timers_ror(AoeGameScreenRor *s) {
    int32_t mode = OPS->world_mode(CTX), used = 0, p, which;
    if (mode == 2) {
        OPS->timer_set(CTX, s->timers[0], 3, 0);
        return;
    }
    if (mode != 0) return;
    for (p = 1; p < OPS->player_count(CTX); ++p) {
        int32_t years;
        OPS->wonder_years(CTX, p, &years);
        if (years > 0) {
            OPS->timer_set(CTX, s->timers[used++], 4, p);
            if (used > 9) return;
        }
        for (which = 0; which < 2; ++which) {
            if (OPS->player_timer(CTX, p, which) == -1.0f) continue;
            if (allied_before(s, p)) continue;
            OPS->timer_set(CTX, s->timers[used++], 5 + which, p);
            if (used > 9) return;
        }
    }
    while (used < AOE_GAME_SCREEN_TIMERS) OPS->timer_set(CTX, s->timers[used++], 0, 0);
}

int aoe_game_screen_score_cmp_ror(const int32_t *a, const int32_t *b) {
    static const int order[4] = {4, 2, 3, 1};
    int i;
    for (i = 0; i < 4; ++i) {
        int k = order[i];
        if (a[k] < b[k]) return 1;
        if (b[k] < a[k]) return -1;
    }
    return 0;
}

void aoe_game_screen_announce_ror(AoeGameScreenRor *s, int32_t string_id, int32_t action) {
    if (OPS->comm_waiting(CTX)) return;
    OPS->message(CTX, s->lines[0], 1, OPS->string(CTX, string_id), 0x32);
    OPS->action(CTX, action, 0);
}

void aoe_game_screen_train_ror(AoeGameScreenRor *s, int16_t id) {
    int16_t failing;
    char text[0x200];
    if (OPS->comm_waiting(CTX)) return;
    if (OPS->can_afford(CTX, id, &failing)) {
        OPS->set_place_unit(CTX, id);
        OPS->action(CTX, OPS->template_flag(CTX, id) ? 0x15 : 7, 0);
        return;
    }
    text[0] = '\0';
    OPS->format(CTX, 0x67, 1, failing, text, sizeof text);
    OPS->message(CTX, s->lines[0], 1, text, 0x56);
}

void aoe_game_screen_alt_bar_ror(AoeGameScreenRor *s) {
    if (!OPS->alt_bar(CTX)) {
        s->alt_bar_held = 0;
        OPS->set_alt_bar(CTX, 1);
        OPS->set_active(CTX, s->title, 1);
        OPS->set_active(CTX, s->resources, 0);
        return;
    }
    if (!s->alt_bar_held) {
        s->alt_bar_held = 1;
        return;
    }
    OPS->set_alt_bar(CTX, 0);
    OPS->set_active(CTX, s->title, 0);
    OPS->set_active(CTX, s->resources, 1);
}

void aoe_game_screen_group_clear_ror(AoeGameScreenRor *s) {
    if (!OPS->comm_waiting(CTX)) OPS->group_assign(CTX, -1);
}

void aoe_game_screen_group_assign_ror(AoeGameScreenRor *s, int32_t group) {
    if (OPS->comm_waiting(CTX)) return;
    OPS->group_assign(CTX, group ? group : 10);
}

void aoe_game_screen_group_select_ror(AoeGameScreenRor *s, int32_t group, int clear) {
    if (OPS->comm_waiting(CTX)) return;
    if (clear) OPS->clear_selection(CTX);
    OPS->group_select(CTX, group ? group : 10);
    OPS->redraw(CTX, s->view, 1);
}

void aoe_game_screen_groups_clear_ror(AoeGameScreenRor *s) {
    if (OPS->comm_waiting(CTX)) return;
    OPS->clear_groups(CTX);
    OPS->redraw(CTX, s->view, 1);
}

void aoe_game_screen_selection_clear_ror(AoeGameScreenRor *s) {
    if (OPS->comm_waiting(CTX)) return;
    OPS->clear_selection(CTX);
    OPS->clear_block(CTX);
    OPS->redraw(CTX, s->view, 1);
}

void aoe_game_screen_selection_action_ror(AoeGameScreenRor *s, int32_t string_id,
                                          int32_t action) {
    int32_t *ids = NULL;
    int16_t n = 0;
    if (OPS->comm_waiting(CTX)) return;
    if (!OPS->selected_ids(CTX, &ids, &n)) return;
    OPS->free_ids(CTX, ids);
    OPS->message(CTX, s->lines[0], 1, OPS->string(CTX, string_id), 0x32);
    OPS->action(CTX, action, 0);
}

void aoe_game_screen_player_call_ror(AoeGameScreenRor *s, int slot, int32_t arg) {
    if (!OPS->comm_waiting(CTX)) OPS->player_call(CTX, slot, arg);
}

void aoe_game_screen_center_ror(AoeGameScreenRor *s) {
    AoeGameScreenObjectRor o;
    int32_t id;
    if (OPS->comm_waiting(CTX)) return;
    id = OPS->current(CTX);
    if (id == 0 || !OPS->object(CTX, id, &o)) return;
    OPS->view_position(CTX, o.x, o.y);
    OPS->view_tile(CTX, (int32_t)o.x, (int32_t)o.y);
}

void aoe_game_screen_cycle_ror(AoeGameScreenRor *s, int next) {
    int16_t count = OPS->selected_count(CTX), seen = 0;
    int32_t current = OPS->current(CTX), pick = 0;
    int slot, found = 0;
    if (OPS->comm_waiting(CTX) || count < 2) return;
    for (slot = 0; slot < 25 && seen < count; ++slot) {
        int32_t id = OPS->selected(CTX, slot);
        if (id == 0) continue;
        if (!next) {
            if (id == current && pick != 0) break;
            if (id != current) pick = id;
        } else if (id == current) {
            found = 1;
        } else if (found) {
            pick = id;
            break;
        } else if (pick == 0) {
            pick = id;
        }
        ++seen;
    }
    if (pick != 0) OPS->set_current(CTX, pick);
}

int aoe_game_screen_selection_busy_ror(const AoeGameScreenRor *s) {
    int16_t count = OPS->selected_count(CTX), seen = 0;
    int slot;
    for (slot = 0; slot < 25 && seen < count; ++slot) {
        AoeGameScreenObjectRor o;
        int32_t id = OPS->selected(CTX, slot);
        if (id == 0) continue;
        if (OPS->object(CTX, id, &o) && o.type > 0x27 && o.type < 0x51 &&
            OPS->object_busy(CTX, id))
            return 1;
        ++seen;
    }
    return 0;
}

void aoe_game_screen_focus_ror(AoeGameScreenRor *s, void (*focus)(void *context, int32_t v),
                               int32_t v) {
    focus(CTX, v);
    OPS->software_cursor(CTX, s->field_78 ? 0 : 1);
}

int32_t aoe_game_screen_next_idle_ror(AoeGameScreenRor *s, int32_t after) {
    int32_t count = OPS->list_count(CTX), start = 0, k;
    if (count <= 0) return -1;
    if (OPS->world_has(CTX, after)) {
        for (k = 0; k < count; ++k) {
            if (OPS->list_id(CTX, k) == after) {
                start = k;
                break;
            }
        }
    }
    for (k = 1; k <= count; ++k) {
        AoeGameScreenObjectRor o;
        int32_t id = OPS->list_id(CTX, (start + k) % count);
        if (!OPS->object(CTX, id, &o)) continue;
        if ((o.category == 4 || o.category == 0x15 || o.category == 0x13 || o.category == 2) &&
            OPS->object_ready(CTX, id) && o.field_20 == 0) {
            s->idle_id = id;
            return id;
        }
    }
    return -1;
}

void aoe_game_screen_delete_ror(AoeGameScreenRor *s) {
    if (s->ops->comm_waiting(s->ops->context)) return;
    s->ops->player_command(s->ops->context, 0, 0);
}

void aoe_game_screen_command_6d_ror(AoeGameScreenRor *s, uint32_t value) {
    if (s->ops->comm_waiting(s->ops->context)) return;
    s->ops->player_command(s->ops->context, 1, value);
}

void aoe_game_screen_pause_ror(AoeGameScreenRor *s) {
    const AoeGameScreenOpsRor *o = s->ops;
    uint32_t now = o->now(o->context);
    if (now - s->pause_time < 1000 && !o->paused(o->context)) return;
    s->pause_time = now;
    if (o->game_state(o->context) == 5) return;
    if (o->networked(o->context)) {
        char line[256];
        const char *text = o->string(o->context, 0xbf4);
        snprintf(line, sizeof line, "%s", text ? text : "");
        o->send_chat(o->context, line);
    }
    o->toggle_pause(o->context);
}

void aoe_game_screen_first_page_ror(AoeGameScreenRor *s) {
    s->button_page = 0;
    OPS->refresh_buttons(CTX);
}

void aoe_game_screen_cancel_page_ror(AoeGameScreenRor *s) {
    if (OPS->comm_waiting(CTX)) return;
    OPS->action(CTX, 6, 1);
    aoe_game_screen_first_page_ror(s);
}

void aoe_game_screen_next_page_ror(AoeGameScreenRor *s) {
    int32_t sub = 0;
    if (OPS->comm_waiting(CTX)) return;
    if (OPS->edit_mode(CTX, &sub) == 6 && sub == 1)
        s->button_page = (int16_t)(s->button_page != 0 ? 0 : 10);
    else if (s->shown_page < s->button_pages - 1)
        s->button_page = (int16_t)(s->shown_page + 1);
    else
        s->button_page = 0;
    OPS->refresh_buttons(CTX);
}

void aoe_game_screen_goto_idle_ror(AoeGameScreenRor *s) {
    AoeGameScreenObjectRor o;
    int32_t after = s->idle_id, cur = OPS->current(CTX), id;
    if (cur != 0 && OPS->owned_by_viewed(CTX, cur) && OPS->object(CTX, cur, &o) &&
        (o.category == 4 || o.category == 0x15 || o.category == 0x13 || o.category == 2))
        after = cur;
    id = aoe_game_screen_next_idle_ror(s, after);
    if (id == -1 || !OPS->object(CTX, id, &o)) {
        OPS->sound(CTX, 3);
        return;
    }
    OPS->clear_selection(CTX);
    OPS->select_object(CTX, id);
    OPS->view_position(CTX, o.x, o.y);
    OPS->view_tile(CTX, (int32_t)o.x, (int32_t)o.y);
    OPS->redraw(CTX, s->view, 1);
}

int aoe_game_screen_zoom_key_ror(AoeGameScreenRor *s, int32_t key) {
    if ((key == '+' || key == '=' || key == '-') && OPS->orders_enabled(CTX))
        OPS->zoom(CTX, key != '-');
    return 0;
}

void aoe_game_screen_speed_step_ror(AoeGameScreenRor *s, int in) {
    const AoeGameScreenOpsRor *o = s->ops;
    uint32_t now = o->now(o->context);
    float cur, next;
    if (now - s->speed_time < 1000 && !o->paused(o->context)) return;
    s->speed_time = now;
    if (o->game_state(o->context) == 5) return;
    cur = o->game_speed(o->context);
    if (!in) {
        if (cur == 16.0f) next = 8.0f;
        else if (cur == 8.0f) next = 4.0f;
        else if (cur == 4.0f) next = 2.0f;
        else if (cur == 2.0f) next = 1.5f;
        else if (cur == 1.5f) next = 1.0f;
        else return;
    } else if (cur == 1.0f) {
        next = 1.5f;
    } else if (cur == 1.5f) {
        next = 2.0f;
    } else {
        if (!o->paused(o->context) || !o->fast_speeds(o->context)) return;
        if (cur == 2.0f) next = 4.0f;
        else if (cur == 4.0f) next = 8.0f;
        else if (cur == 8.0f) next = 16.0f;
        else return;
    }
    if (o->networked(o->context)) {
        char line[256];
        const char *text = o->string(o->context, 0xbf3);
        snprintf(line, sizeof line, "%s", text ? text : "");
        o->send_chat(o->context, line);
    }
    o->send_speed(o->context, next);
    if (o->paused(o->context)) o->set_speed_setting(o->context, next);
}

void aoe_game_screen_escape_ror(AoeGameScreenRor *s) {
    int32_t sub = 0, mode;
    if (OPS->escape_armed(CTX)) {
        OPS->escape_disarm(CTX);
        return;
    }
    if (OPS->comm_waiting(CTX)) return;
    mode = OPS->edit_mode(CTX, &sub);
    if (mode == 0) {
        aoe_game_screen_selection_clear_ror(s);
        return;
    }
    if (mode == 0x15) {
        OPS->view_cancel(CTX, s->view);
        OPS->outline_reset(CTX);
    }
    OPS->set_mode(CTX, 0, 0);
}

void aoe_game_screen_diplomacy_ror(AoeGameScreenRor *s) {
    if (OPS->comm_waiting(CTX)) return;
    if (OPS->paused(CTX) == 1) OPS->comm_pause(CTX, 1, 1);
    OPS->open_dialog(CTX, "Diplomacy Dialog");
}

void aoe_game_screen_menu_ror(AoeGameScreenRor *s) {
    if (OPS->game_state(CTX) == 6) OPS->run_commands(CTX);
    if (OPS->paused(CTX) == 1) OPS->comm_pause(CTX, 1, 1);
    OPS->open_dialog(CTX, "Menu Dialog");
}

void aoe_game_screen_building_key_ror(AoeGameScreenRor *s, int32_t key) {
    static const int16_t types[8] = {0x6d, 0x0c, 0x2d, 0x57, 0x31, 0x65, 0x68, 0};
    AoeGameScreenObjectRor o;
    int32_t cur, after = 0, id;
    if (OPS->comm_waiting(CTX)) return;
    if (key < 0 || key > 7) return;
    cur = OPS->current(CTX);
    if (cur != 0 && OPS->object(CTX, cur, &o) && o.template_id == types[key]) after = cur;
    id = OPS->next_of_type(CTX, types[key], after);
    if (id == 0 || !OPS->object(CTX, id, &o)) return;
    OPS->clear_selection(CTX);
    OPS->select_object(CTX, id);
    OPS->view_position(CTX, o.x, o.y);
    OPS->view_tile(CTX, (int32_t)o.x, (int32_t)o.y);
    OPS->redraw(CTX, s->view, 1);
}

static void game_screen_shortfall(AoeGameScreenRor *s, int16_t failing) {
    char text[0x200];
    text[0] = '\0';
    OPS->format(CTX, 0x67, 1, failing, text, sizeof text);
    OPS->message(CTX, s->lines[0], 1, text, 0x56);
}

void aoe_game_screen_train_button_ror(AoeGameScreenRor *s, int16_t id, int32_t arg) {
    int16_t failing = 0;
    if (OPS->comm_waiting(CTX)) return;
    if (OPS->instant(CTX)) {
        OPS->instant_train(CTX, id, arg);
        return;
    }
    if (OPS->can_afford(CTX, id, &failing)) {
        OPS->set_place_unit(CTX, id);
        OPS->train_at_current(CTX, id);
        return;
    }
    game_screen_shortfall(s, failing);
}

void aoe_game_screen_research_button_ror(AoeGameScreenRor *s, int16_t id) {
    int16_t failing = 0;
    if (OPS->comm_waiting(CTX)) return;
    if (OPS->can_research(CTX, id, &failing)) {
        OPS->research_at_current(CTX, id);
        return;
    }
    game_screen_shortfall(s, failing);
}

static int game_screen_chat_blocked(AoeGameScreenRor *s) {
    if (s->dialog == 0 && OPS->game_state(CTX) == 5) {
        OPS->message(CTX, s->lines[0], 1, OPS->string(CTX, 0xbfe), 0x32);
        return 1;
    }
    return 0;
}

void aoe_game_screen_quick_chat_ror(AoeGameScreenRor *s) {
    if (game_screen_chat_blocked(s)) return;
    OPS->open_dialog(CTX, "Send Quick Message Dialog");
}

void aoe_game_screen_chat_ror(AoeGameScreenRor *s) {
    if (OPS->paused(CTX)) {
        aoe_game_screen_quick_chat_ror(s);
        return;
    }
    if (game_screen_chat_blocked(s)) return;
    OPS->open_dialog(CTX, "Send Message Dialog");
}

void aoe_game_screen_broadcast_ror(AoeGameScreenRor *s) {
    if (s->sound != NULL) {
        OPS->free_sound(CTX, s->sound);
        s->sound = NULL;
    }
    OPS->base_broadcast(CTX);
}

/* 0x004844a0 */
void aoe_game_screen_cue_ror(AoeGameScreenRor *s) {
    uint32_t now;
    if (!OPS->sound_ready(CTX)) return;
    now = OPS->now(CTX);
    if (!s->cue_playing) {
        int16_t i, n, best = -1;
        uint8_t top = 0;
        if (now - s->cue_time < s->cue_interval) return;
        s->cue_time = now;
        n = OPS->cue_count(CTX);
        for (i = 0; i < n; ++i) {
            uint8_t on = 0, priority = 0;
            void *effect = NULL;
            OPS->cue(CTX, i, &on, &priority, &effect);
            if (on && priority > top && effect) {
                best = i;
                top = priority;
            }
        }
        if (best != -1) {
            uint8_t on, priority;
            void *effect = NULL;
            const char *name;
            OPS->cue(CTX, best, &on, &priority, &effect);
            name = OPS->cue_choose(CTX, effect);
            if (name) {
                OPS->music_stop(CTX);
                OPS->music_play(CTX, name);
                s->cue_playing = 1;
                s->cue_start = OPS->now(CTX);
            }
        }
    } else if (now - s->cue_time > 999) {
        s->cue_time = now;
        if (!OPS->music_playing(CTX)) {
            s->cue_playing = 0;
        } else if (now - s->cue_start > 5999) {
            OPS->music_stop(CTX);
            s->cue_playing = 0;
        }
    }
}

static void *hover_button(AoeGameScreenRor *s, int i) {
    return i < AOE_GAME_SCREEN_BUTTONS ? s->buttons[i] : s->more_buttons[i - AOE_GAME_SCREEN_BUTTONS];
}

/* 0x00484620 */
void aoe_game_screen_hover_ror(AoeGameScreenRor *s) {
    int32_t x, y;
    int i;
    if (!OPS->foreground(CTX) || !OPS->view_focus(CTX) || OPS->modal(CTX)) return;
    OPS->mouse(CTX, &x, &y);
    if (x == s->mouse_x && y == s->mouse_y) return;
    s->mouse_x = x;
    s->mouse_y = y;
    if (!OPS->view_keeps_help(CTX)) OPS->message_hide(CTX, s->help_panel);
    if (x < 0 || x >= s->width || y < 0 || y > s->height) return;
    if (OPS->over_map(CTX, x, y)) return;
    for (i = 0; i < AOE_GAME_SCREEN_BUTTONS + 5; ++i) {
        void *b = hover_button(s, i);
        if (OPS->active(CTX, b) && OPS->button_id(CTX, b) != -1 && OPS->button_hit(CTX, b, x, y)) {
            OPS->message(CTX, s->help_panel, 0, OPS->button_help_text(CTX, b), 0xff);
            return;
        }
    }
}

/* 0x00484f90 */
void aoe_game_screen_map_scores_ror(AoeGameScreenRor *s, int on) {
    int i;
    OPS->set_map_scores(CTX, (uint8_t)on);
    for (i = 0; i < 8; ++i)
        if (s->scores[i]) OPS->set_active(CTX, s->scores[i], on);
    if (on) aoe_game_screen_scores_ror(s);
    OPS->help_place(CTX, s->help_panel, (uint8_t)on ? 0xb8 : 4);
    OPS->redraw(CTX, s->view, 1);
}

static uint32_t score_colour(int16_t c, uint32_t *shadow) {
    static const uint32_t colours[8] = {0xff0000, 0x2139c6, 0xffff, 0x4f73a3,
                                        0xf77f3, 0xc700, 0x7b90a2, 0xadad00};
    *shadow = c == 0 ? 0xb3b3b3 : 0;
    return c >= 0 && c < 8 ? colours[c] : 0xffffff;
}

static int score_entry_cmp(const void *a, const void *b) {
    return aoe_game_screen_score_cmp_ror((const int32_t *)a, (const int32_t *)b);
}

/* 0x00483c40 */
void aoe_game_screen_scores_ror(AoeGameScreenRor *s) {
    /* Entries: player, world slot, team, score, team average. */
    int32_t e[9][5];
    int32_t teams = 0, i, j, best = 0, n;
    uint32_t rating = 11;
    int net;
    memset(e, 0, sizeof e);
    for (i = 1; i <= OPS->game_players(CTX); ++i) {
        int32_t *p = e[i - 1];
        int32_t slot = OPS->world_slot(CTX, i);
        int32_t members = 1;
        if (slot == 0 || slot >= OPS->player_count(CTX)) continue;
        if (p[2] == 0) p[2] = ++teams;
        p[0] = i;
        p[1] = slot;
        p[3] = p[4] = OPS->score(CTX, slot);
        if (OPS->player_alive(CTX, slot)) {
            for (j = 1; j <= OPS->game_players(CTX); ++j) {
                int32_t other;
                if (j == i) continue;
                other = OPS->world_slot(CTX, j);
                if (other == 0 || other >= OPS->player_count(CTX)) continue;
                if (OPS->stance(CTX, slot, other) != 0 || OPS->stance(CTX, other, slot) != 0 ||
                    !OPS->player_alive(CTX, other))
                    continue;
                if (e[j - 1][2] == 0) e[j - 1][2] = p[2];
                p[4] += OPS->score(CTX, other);
                ++members;
            }
        }
        p[4] /= members;
    }
    n = OPS->game_players(CTX);
    if (n > 9) n = 9;
    qsort(e, (size_t)n, sizeof e[0], score_entry_cmp);
    net = OPS->networked(CTX);
    if (net) {
        for (i = 0; i < 8; ++i) {
            uint32_t lat, r;
            if (!e[i][1] || OPS->player_kind(CTX, e[i][0]) != 2) continue;
            lat = OPS->latency(CTX, e[i][0]);
            r = lat >= 1000 ? 1 : lat < 10 ? 100 : 1000 / lat;
            if (r < rating) {
                rating = r;
                best = e[i][0];
            }
        }
    }
    for (i = 0; i < 8; ++i) {
        AoeGameScoreLineRor line;
        const char *name;
        int32_t kind;
        if (!s->scores[i]) return;
        if (!e[i][1]) {
            OPS->set_active(CTX, s->scores[i], 0);
            continue;
        }
        memset(&line, 0, sizeof line);
        name = OPS->player_name(CTX, e[i][0]);
        snprintf(line.text, sizeof line.text, "%s: %d/%d", name ? name : "", (int)e[i][3],
                 (int)e[i][4]);
        line.colour = score_colour(OPS->colour(CTX, e[i][1]), &line.shadow);
        kind = net ? OPS->player_kind(CTX, e[i][0]) : 2;
        line.font = OPS->lost(CTX, e[i][1]) || (net && kind != 2 && kind != 4) ? 8 : 0xb;
        if (net && kind == 2) {
            uint32_t ping = OPS->ping(CTX, e[i][0]);
            if (e[i][0] == best) {
                line.icon2 = s->ping_picture;
                line.frame2 = 0x11;
            }
            if (ping > 299) {
                line.icon1 = s->ping_picture;
                line.frame1 = ping < 0x3e9 ? 0x13 : 0x14;
            }
        }
        OPS->score_line(CTX, s->scores[i], &line);
    }
}

/* ---- 0x0047ea00 destructor, 0x0047eed0, 0x0047fbe0, 0x004843d0 ---- */

#define O(s) ((s)->ops)
#define C(s) ((s)->ops->context)

static void drop_panel(AoeGameScreenRor *s, void **slot) {
    if (*slot && O(s)->delete_panel) O(s)->delete_panel(C(s), *slot);
    *slot = NULL;
}

void aoe_game_screen_release_ror(AoeGameScreenRor *s) {
    int i;
    if (!s || !s->ops) return;
    if (O(s)->close_dialog) O(s)->close_dialog(C(s), "Config Dialog");
    if (s->sound) {
        if (O(s)->sound_playing && O(s)->sound_playing(C(s), s->sound) && O(s)->sound_stop)
            O(s)->sound_stop(C(s), s->sound);
        if (O(s)->free_sound) O(s)->free_sound(C(s), s->sound);
        s->sound = NULL;
    }
    if (s->cue_sound) {
        if (s->cue_playing) {
            s->cue_playing = 0;
            if (O(s)->stop_all_sounds) O(s)->stop_all_sounds(C(s));
        }
        s->cue_sound = 0;
    }
    for (i = 0; i < 13; ++i) {
        if (s->shapes[i] && O(s)->free_shape) O(s)->free_shape(C(s), s->shapes[i]);
        s->shapes[i] = NULL;
    }
    for (i = 0; i < 4; ++i) {
        if (s->buffers[i] && O(s)->free_buffer) O(s)->free_buffer(C(s), s->buffers[i]);
        s->buffers[i] = NULL;
    }
    for (i = 0; i < 8; ++i) drop_panel(s, &s->chat[i]);
    for (i = 0; i < AOE_GAME_SCREEN_BUTTONS; ++i) drop_panel(s, &s->buttons[i]);
    for (i = 0; i < 5; ++i) drop_panel(s, &s->more_buttons[i]);
    drop_panel(s, &s->age_text);
    drop_panel(s, &s->title);
    for (i = 0; i < 8; ++i) drop_panel(s, &s->scores[i]);
    drop_panel(s, &s->resources);
    drop_panel(s, &s->panel_4c4);
    drop_panel(s, &s->panel_510);
    drop_panel(s, &s->help_panel);
    drop_panel(s, &s->panel_514);
    drop_panel(s, &s->panel_540);
    for (i = 0; i < AOE_GAME_SCREEN_TIMERS; ++i) drop_panel(s, &s->timers[i]);
    for (i = 5; i >= 0; --i) drop_panel(s, &s->lines[i]);
    drop_panel(s, &s->panel_57c);
    drop_panel(s, &s->panel_584);
    drop_panel(s, &s->view);
    drop_panel(s, &s->diamond_map);
    if (O(s)->base_release) O(s)->base_release(C(s));
}

static int span_overlaps(int32_t a0, int32_t a1, int32_t b0, int32_t b1) {
    return (a0 <= b0 && b0 <= a1) || (a0 <= b1 && b1 <= a1) || (b0 < a0 && a1 < b1);
}

void aoe_game_screen_redraw_overlaps_ror(AoeGameScreenRor *s, const int32_t a[4], int32_t flag) {
    void *m;
    int32_t b[4];
    if (!s || !s->ops || !O(s)->modal_panel || !O(s)->panel_rect) return;
    m = O(s)->modal_panel(C(s));
    if (!m || !O(s)->panel_rect(C(s), m, b)) return;
    if ((span_overlaps(a[0], a[2], b[0], b[2]) && span_overlaps(a[1], a[3], b[1], b[3])) ||
        (span_overlaps(b[0], b[2], a[0], a[2]) && span_overlaps(b[1], b[3], a[1], a[3])))
        if (O(s)->redraw) O(s)->redraw(C(s), m, flag);
}

void aoe_game_screen_palette_cycle_ror(AoeGameScreenRor *s, uint32_t now) {
    uint8_t entries[7][4];
    int k;
    int16_t at;
    if (!s || now - s->cycle_time < s->cycle_period) return;
    at = s->cycle_index;
    for (k = 0; k < 7; ++k) {
        entries[k][0] = s->cycle_colors[at][0];
        entries[k][1] = s->cycle_colors[at][1];
        entries[k][2] = s->cycle_colors[at][2];
        entries[k][3] = 0;
        if (++at == 7) at = 0;
    }
    s->cycle_index = (int16_t)(s->cycle_index == 0 ? 6 : s->cycle_index - 1);
    if (s->ops && O(s)->set_palette_entries) O(s)->set_palette_entries(C(s), 0xf8, 7, entries);
    s->cycle_time = now;
}

void aoe_game_screen_idle_ror(AoeGameScreenRor *s) {
    int32_t state;
    if (!s || !s->ops) return;
    if (!(O(s)->game_flag_1d8 && O(s)->game_flag_1d8(C(s))) && O(s)->arrow_cursor)
        O(s)->arrow_cursor(C(s));
    state = O(s)->game_state ? O(s)->game_state(C(s)) : 0;
    if (state == 4 || state == 5 || state == 6) {
        if (O(s)->modal_panel && O(s)->modal_panel(C(s)) == s->self && O(s)->view_focus &&
            O(s)->view_focus(C(s)))
            aoe_game_screen_palette_cycle_ror(s, O(s)->now ? O(s)->now(C(s)) : 0);
    }
    if (state == 4 || state == 6) {
        aoe_game_screen_cue_ror(s);
        aoe_game_screen_hover_ror(s);
    }
    if (O(s)->base_idle) O(s)->base_idle(C(s));
}

int32_t aoe_game_screen_message_ror(AoeGameScreenRor *s, uint32_t msg, uint32_t wparam,
                                    uint32_t lparam, int32_t *quit_requested, int iconic) {
    if (!s || !s->ops) return 0;
    if (msg == 0x104) {
        if (wparam == 0x79 && s->field_78 && O(s)->current_child &&
            O(s)->current_child(C(s)) == s->view) {
            aoe_game_screen_menu_ror(s);
            return 1;
        }
    } else if (msg == 0x101) {
        if (s->field_78 && O(s)->view_focus && O(s)->view_focus(C(s)) &&
            (wparam == 0x72 || wparam == 0x13) && O(s)->key_down &&
            !O(s)->key_down(C(s), 0x12) && !O(s)->key_down(C(s), 0x11) &&
            !O(s)->key_down(C(s), 0x10))
            aoe_game_screen_pause_ror(s);
    } else if (msg == 0x10) {
        if (quit_requested && *quit_requested == 0) {
            int32_t state = O(s)->game_state ? O(s)->game_state(C(s)) : 0;
            *quit_requested = 1;
            if (state >= 4 && state <= 7 && !iconic) {
                if (O(s)->ask_close) O(s)->ask_close(C(s));
                return 1;
            }
        }
    } else if (msg == 0x208) {
        if (O(s)->view_back) O(s)->view_back(C(s));
    }
    return O(s)->base_message ? O(s)->base_message(C(s), msg, wparam, lparam) : 0;
}

int32_t aoe_game_screen_paint_ror(const AoeGameScreenPaintOpsRor *o) {
    uint32_t t0;
    int32_t level, n, i;
    int full;
    if (!o || !o->can_paint(o->context)) return 0;
    t0 = o->now(o->context);
    level = o->redraw_level(o->context);
    full = 1;
    if (level != 2 && !o->view_focus(o->context)) {
        const char *name = o->current_name(o->context);
        if (!name || (strcmp(name, "Send Quick Message Dialog") != 0 &&
                      strcmp(name, "Help Dialog") != 0))
            full = 0;
    }
    n = o->child_count(o->context);
    if (level != 0) {
        if (full) {
            for (i = 0; i < n; ++i) o->child_set_redraw(o->context, o->child(o->context, i), level);
            if (o->parent_wants_overlaps(o->context)) o->parent_redraw_overlaps(o->context, level);
            o->draw_self(o->context);
        }
        o->set_redraw_level(o->context, 0);
        o->set_painting(o->context, 1);
    }
    for (i = 0; i < n; ++i) {
        void *c = o->child(o->context, i);
        if (full || o->is_current(o->context, c)) o->child_paint(o->context, c);
    }
    o->statistic(o->context, 0x11, o->now(o->context) - t0);
    o->set_painting(o->context, 0);
    return 0;
}

int aoe_game_screen_command_ror(int map_orders, int32_t code, int32_t arg, int right,
                                int scores_on,
                                void (*run)(void *ctx, uint32_t handler, int32_t arg, int32_t arg2),
                                void *ctx) {
    uint32_t h = 0;
    int32_t a = 0, a2 = 0;
    if (right) {
        if (!map_orders || code != 0x12) return 0;
        run(ctx, 0x485520u, arg, -1);
        return 1;
    }
    switch (code) {
    case 6: h = 0x4856b0u; break;
    case 8: h = 0x484e80u; break;
    case 9: h = 0x485620u; break;
    case 0xb: h = 0x484900u; break;
    case 0xc: h = 0x485140u; break;
    case 0xd: h = 0x4849b0u; break;
    case 0xe: h = 0x484b60u; break;
    case 0xf: h = 0x4850b0u; break;
    case 0x15: h = 0x457cd0u; break;
    case 0x18: h = 0x484f90u; a = !scores_on; break;
    default: break;
    }
    if (!h && map_orders) {
        switch (code) {
        case 1: h = 0x485770u; break;
        case 2: h = 0x4851d0u; break;
        case 3: h = 0x4847e0u; break;
        case 4: h = 0x42c330u; break;
        case 5: h = 0x4854f0u; break;
        case 7: h = 0x485660u; break;
        case 0x10: h = 0x4ed890u; a = arg; break;
        case 0x11: h = 0x4852f0u; a = arg; break;
        case 0x12: case 0x20: h = 0x485520u; a = arg; a2 = 1; break;
        case 0x13: h = 0x484820u; a = arg; break;
        case 0x14: h = 0x484980u; break;
        case 0x16: h = 0x485830u; break;
        case 0x17: h = 0x485860u; break;
        case 0x19: case 0x1a: case 0x1b: h = 0x4858b0u; a = code - 0x19; break;
        case 0x1c: h = 0x484ef0u; break;
        case 0x1d: h = 0x484b10u; break;
        case 0x1e: h = 0x484790u; break;
        case 0x1f: h = 0x484f40u; break;
        case 0x21: h = 0x485520u; a = arg; a2 = -1; break;
        default: break;
        }
    }
    if (!h) return 0;
    run(ctx, h, a, a2);
    return 1;
}

void aoe_game_screen_player_changed_ror(int32_t *stored_id, int16_t *stored_civ,
                                        const AoeGameScreenPlayerOpsRor *o) {
    int32_t old = *stored_id, now = o->player_id(o->context);
    *stored_id = now;
    *stored_civ = o->player_civ(o->context);
    if (old != now) {
        int32_t st = o->game_state(o->context);
        if (st == 0x15) o->state15_reset(o->context);
        switch (st) {
        case 1: case 2: case 3: case 8: case 9: case 10: case 0xe: case 0x12: break;
        default: o->game_refresh(o->context); break;
        }
    }
    o->first_page(o->context);
}

void aoe_game_screen_help_text_ror(const AoeGameScreenHelpTextOpsRor *o, int32_t name_id,
                                   const AoeGameScreenCostRor costs[3], int has_costs,
                                   char *out, int size) {
    char name[0x200], part[3][0x200], all[0x200], res[0x200];
    int i;
    if (!o || !out || size <= 0) return;
    name[0] = all[0] = 0;
    o->string(o->context, name_id, name, (int)sizeof name);
    for (i = 0; i < 3; ++i) part[i][0] = 0;
    if (has_costs) {
        for (i = 0; i < 3; ++i) {
            if (costs[i].type == -1 || costs[i].type > 3) continue;
            res[0] = 0;
            o->resource_name(o->context, costs[i].type, res, (int)sizeof res);
            snprintf(part[i], sizeof part[i], i == 0 ? "%s: %d" : "  %s: %d", res,
                     (int)costs[i].amount);
        }
        if (part[0][0] || part[1][0] || part[2][0])
            snprintf(all, sizeof all, "(%.160s%.160s%.160s)", part[0], part[1], part[2]);
    }
    if (!name[0]) {
        out[0] = 0;
    } else if (all[0]) {
        snprintf(out, (size_t)size, "%.255s %.255s", name, all);
    } else {
        snprintf(out, (size_t)size, "%s", name);
    }
}

static int32_t logo_y(const AoeGameScreenRor *s) {
    if (s->width < 0x400 || s->height < 0x300) return (s->width < 800 || s->height < 600) ? 0x162 : 0x1da;
    return 0x282;
}

static void draw_logo(const AoeGameScreenRor *s, const AoeGameScreenDrawOpsRor *o, const char *who) {
    if (!o->has_shape(o->context) || !o->lock(o->context, who)) return;
    o->shape_draw(o->context, 0, 0, 0);
    o->shape_draw(o->context, 1, 0, logo_y(s));
    o->unlock(o->context, who);
}

void aoe_game_screen_draw_ror(const AoeGameScreenRor *s, const AoeGameScreenDrawOpsRor *o) {
    if (!s || !o) return;
    if (o->recording(o->context)) {
        int32_t clip[4], view[4];
        o->get_clip(o->context, clip);
        o->begin_draw(o->context, 0);
        if (o->view_rect(o->context, view) && o->lock(o->context, "scr_game::draw")) {
            int32_t work[4];
            o->box(o->context, view[0], view[1], view[2], view[3], 0x74);
            o->unlock(o->context, "scr_game::draw");
            memcpy(work, clip, sizeof work);
            if (work[1] < view[3]) work[1] = view[3];
            o->set_clip(o->context, work);
        }
        draw_logo(s, o, "scr_game::draw2");
        o->end_draw(o->context);
        o->set_clip(o->context, clip);
        return;
    }
    o->begin_draw(o->context, (o->redraw_level(o->context) == 2 || !o->has_shape(o->context)) ? 1 : 0);
    draw_logo(s, o, "scr_game::draw3");
    o->end_draw(o->context);
}

/* 0x0047fc60 */
void aoe_game_screen_idle_tick_ror(AoeGameScreenIdleRor *s, const AoeGameScreenIdleOpsRor *o) {
    uint32_t now, t0;
    int32_t ran = 0, done, iterations = 1, i;
    uint8_t state = o->game_state;
    now = o->now(o->context);
    if (now - s->last < s->interval) return;
    if (state == 6) goto tail;
    if (o->networked) {
        uint32_t span = o->timing_last(o->context, 3) + o->timing_last(o->context, 2);
        uint32_t count = o->timing_count(o->context), avg = 1;
        if (count) { avg = o->timing_total(o->context, 1) / count; if (!avg) avg = 1; }
        if (o->global_558738 && o->own_latency >= 0x43 && o->own_latency <= 0x7d &&
            (float)avg / (float)(int32_t)span <= 2.0f)
            iterations = 2;
        o->timing_set_last(o->context, 2, 0);
        o->timing_set_last(o->context, 3, 0);
    }
    for (i = 0; i < iterations; ++i) {
        t0 = o->now(o->context);
        if (o->pause_request) {
            done = 0;
            o->step(o->context, AOE_GS_IDLE_STATE_6, 0);
            if (o->aborted(o->context)) return;
        } else if (s->dialog) {
            done = 1;
            if (o->aborted(o->context)) return;
        } else {
            done = o->world_update(o->context);
            if (o->aborted(o->context)) return;
            if (done == 1) {
                s->dialog = 1;
                o->step(o->context, AOE_GS_IDLE_VIEW_F4, 0);
                o->step(o->context, AOE_GS_IDLE_HIGHLIGHT, 0);
                o->step(o->context, AOE_GS_IDLE_GAME_10, 0);
                o->step(o->context, AOE_GS_IDLE_CLEAR_SELECTION, 0);
                if (o->networked) o->step(o->context, AOE_GS_IDLE_MP_END, 0);
                if (o->aborted(o->context)) return;
            }
        }
        now = o->now(o->context);
        s->last = now;
        if (o->world_running(o->context)) o->sample_frame(o->context, now - t0);
        if (s->timers_dirty) { o->step(o->context, AOE_GS_IDLE_TIMERS, 0); s->timers_dirty = 0; }
        if (s->centre_pending) o->step(o->context, AOE_GS_IDLE_CENTRE, 0);
        if (s->viewed_unit != o->player_unit(o->context) ||
            s->viewed_count != o->player_count(o->context)) {
            o->step(o->context, AOE_GS_IDLE_OBJECT_CHANGED, o->player_unit(o->context));
            o->step(o->context, AOE_GS_IDLE_PLAYER_CHANGED, 0);
            /* 0x00481f70 records the new unit and count in +0x5c8 / +0x5cc. */
            s->viewed_unit = o->player_unit(o->context);
            s->viewed_count = o->player_count(o->context);
        }
        if (!o->pause_request &&
            (done == 1 || o->viewed_defeated(o->context) || s->field_730 == 1)) {
            if (state == 5) {
                /* With no end pending the original counts the players still in
                   the game and discards the count. */
                if (s->end_pending && o->now(o->context) - s->end_time > 5000) {
                    s->end_pending = 0;
                    if (!o->networked) {
                        o->step(o->context, AOE_GS_IDLE_VIEW_F4, 0);
                        o->step(o->context, AOE_GS_IDLE_HIGHLIGHT, 0);
                        o->step(o->context, AOE_GS_IDLE_GAME_10, 0);
                        o->step(o->context, AOE_GS_IDLE_CLEAR_SELECTION, 0);
                        o->step(o->context, AOE_GS_IDLE_LEAVE, 0);
                        return;
                    }
                    if (o->comm_value(o->context) == 0) o->step(o->context, AOE_GS_IDLE_PANEL_584_SHOW, 0);
                    if (done == 1) {
                        o->step(o->context, AOE_GS_IDLE_RESUME_FLAGS, 0);
                        o->step(o->context, AOE_GS_IDLE_VIEW_REDRAW, 2);
                        o->step(o->context, AOE_GS_IDLE_MAP_REDRAW, 2);
                    }
                    o->step(o->context, AOE_GS_IDLE_PANEL_510_SHOW, 1);
                    o->step(o->context, AOE_GS_IDLE_PANEL_540_HIDE, 0);
                }
            } else {
                int won = o->player_won(o->context);
                o->step(o->context, AOE_GS_IDLE_ESCAPE, 0);
                o->step(o->context, AOE_GS_IDLE_CLEAR_SELECTION, 0);
                o->step(o->context, AOE_GS_IDLE_CLEAR_LINES, 0);
                o->step(o->context, AOE_GS_IDLE_STATE_5, 0);
                if (o->has_focus_panel) o->step(o->context, AOE_GS_IDLE_RELEASE_CAPTURE, 0);
                o->step(o->context, AOE_GS_IDLE_SHOW_GAME_SCREEN, 0);
                o->step(o->context, AOE_GS_IDLE_FOCUS_VIEW, 0);
                o->step(o->context, AOE_GS_IDLE_DELETE_SCREENS, 0);
                o->step(o->context, AOE_GS_IDLE_PANEL_584_SHOW, 1);
                o->step(o->context, AOE_GS_IDLE_PANEL_584_TEXT, won ? 0x232c : 0x232d);
                o->step(o->context, AOE_GS_IDLE_VIEW_REDRAW, 1);
                if (o->sound_enabled) o->step(o->context, AOE_GS_IDLE_END_SOUND, won);
                s->end_time = o->now(o->context);
                s->end_pending = 1;
            }
        }
        {
            int running = o->world_running(o->context);
            if (running && o->time_limit && o->time_limit <= o->world_time(o->context)) {
                o->step(o->context, AOE_GS_IDLE_CLOSE_WINDOW, 0);
                return;
            }
            ran += running;
        }
    }
tail:
    if (!o->is_current_screen) return;
    now = o->now(o->context);
    if (ran) {
        if (o->player_age(o->context) != s->age) o->step(o->context, AOE_GS_IDLE_AGE_TITLE, 0);
        if (o->scores_visible && s->scores_interval <= now - s->scores_time) {
            o->step(o->context, AOE_GS_IDLE_MAP_SCORES, 0);
            s->scores_time = o->now(o->context);
        }
    }
    if (o->has_focus_dialog) {
        if (ran || s->view_interval <= now - s->view_time) {
            o->step(o->context, AOE_GS_IDLE_VIEW_REDRAW, 1);
            s->view_time = now;
        }
        if (ran && s->map_interval <= now - s->map_time) {
            o->step(o->context, AOE_GS_IDLE_MAP_REDRAW, 1);
            s->map_time = now;
        }
        if (o->view_dirty) o->step(o->context, AOE_GS_IDLE_UPDATE_WINDOW, 0);
    }
}

/* 0x00480fe0 */
int32_t aoe_game_screen_action_ror(const AoeGameScreenActionOpsRor *o, void *from, int32_t code,
                                   int32_t a, int32_t b) {
    if (from) {
        const char *name;
        int i, known = 0;
        if (from != o->overlay && o->overlay_active &&
            o->overlay_action(o->context, from, code, a, b))
            return 1;
        if (code == 1 || code == 4) {
            for (i = 0; i < 17; ++i)
                if (from == o->buttons[i]) {
                    o->step(o->context, AOE_GS_ACT_COMMAND, a, b, code == 4);
                    return 1;
                }
        }
        if (from == o->panel_4c4 && code == 1) {
            o->step(o->context, AOE_GS_ACT_PLAYER_CHANGED, 0, 0, 0);
            return 1;
        }
        name = o->return_name(o->context);
        if (name) {
            static const char *const dialogs[5] = {"RestartDialog", "QuitGameDialog",
                                                    "CloseProgramDialog", "ResignDialog",
                                                    "QuitAndLoadDialog"};
            for (i = 0; i < 5; ++i) {
                if (strcmp(name, dialogs[i]) != 0) continue;
                known = 1;
                o->step(o->context, AOE_GS_ACT_SHOW_GAME_SCREEN, 0, 0, 0);
                o->remove_screen(o->context, dialogs[i]);
                if (code == 0) {
                    switch (i) {
                    case 0: o->step(o->context, AOE_GS_ACT_RESTART, 0, 0, 0); break;
                    case 1: o->step(o->context, AOE_GS_ACT_LEAVE, 0, 0, 0); break;
                    case 2: o->step(o->context, AOE_GS_ACT_CLOSE, 0, 0, 0); break;
                    case 3: *o->field_730 = 1; o->step(o->context, AOE_GS_ACT_RESIGN, 0, 0, 0); break;
                    default: o->step(o->context, AOE_GS_ACT_LOAD_SCREEN, 0, 0, 0); break;
                    }
                    return 1;
                }
                if (i == 2) *o->global_7c0338 = 0;
                break;
            }
            if (known) {
                if (o->settings_paused && !o->game_a00) o->step(o->context, AOE_GS_ACT_RESUME, 0, 0, 0);
                return 1;
            }
        }
    }
    return o->base_action(o->context, from, code, a, b);
}

/* 0x0047f000 */
static void gs_mode(const AoeGameScreenLayoutOpsRor *o, uint16_t off, int32_t a, int32_t b, int32_t c,
                    int32_t d, int32_t e, int32_t f, int32_t g, int32_t h, int32_t i, uint16_t r0,
                    uint16_t r1, uint16_t r2, uint16_t r3) {
    AoeGameScreenModeRor m;
    m.v[0] = a; m.v[1] = b; m.v[2] = c; m.v[3] = d; m.v[4] = e; m.v[5] = f; m.v[6] = g; m.v[7] = h;
    m.v[8] = i;
    m.refs[0] = r0; m.refs[1] = r1; m.refs[2] = r2; m.refs[3] = r3;
    o->position_mode(o->context, off, &m);
}
int32_t aoe_game_screen_layout_ror(const AoeGameScreenLayoutOpsRor *o) {
    static const uint16_t lines[6] = {0x544, 0x548, 0x54c, 0x550, 0x554, 0x558};
    int32_t saved[8];
    int i, k, small = 0;
    int32_t W, half, vh, vh2, mapx, mapy, w580, y4c4, by, x4f8, x4fc, x500, y504;
    o->base_resize(o->context);
    if (!o->has_view) return 0;
    o->set_active(o->context, 0x50c, 1);
    for (i = 0; i < 6; ++i) o->set_active(o->context, lines[i], 1);
    for (i = 0; i < 8; ++i) {
        saved[i] = o->score_active(o->context, i);
        if (saved[i] != -1) o->set_active(o->context, (uint16_t)(0x58c + 4 * i), 1);
    }
    if (o->width < 0x400 || o->height < 0x300) {
        if (o->width < 800 || o->height < 600) {
            small = 1; W = 640; half = 0xf0; vh = 0x14e; vh2 = 0x130; mapx = 0x19b; mapy = 0x16a;
            w580 = 0x6a; y4c4 = 0x169; by = 0x16a; x4f8 = 0x184; x4fc = 0x1cc; x500 = 0x238; y504 = 0x1bc;
        } else {
            W = 800; half = 0x140; vh = 0x1c6; vh2 = 0x1a8; mapx = 0x23b; mapy = 0x1e0; w580 = 0x10c;
            y4c4 = 0x1e1; by = 0x1e2; x4f8 = 0x224; x4fc = 0x26c; x500 = 0x2d8; y504 = 0x234;
        }
    } else {
        W = 0x400; half = 0x1b0; vh = 0x26e; vh2 = 0x250; mapx = 0x31b; mapy = 0x288; w580 = 0x1ec;
        y4c4 = 0x289; by = 0x28a; x4f8 = 0x304; x4fc = 0x34c; x500 = 0x3b8; y504 = 0x2dc;
    }
    gs_mode(o, 0x540, 7, half, 4, 4, 4, 0xa0, 0xa0, 0x14, 0x14, 0, 0, 0, 0);
    if (!o->overlay_active) {
        o->set_rect(o->context, 0x4b8, 0, 0x14, W, vh);
    } else {
        o->set_rect(o->context, 0x57c, 0, 0x14, W, 0x1e);
        o->set_rect(o->context, 0x4b8, 0, 0x32, W, vh2);
    }
    o->set_rect(o->context, 0x4bc, mapx, mapy, 0xdc, 0x72);
    o->set_rect(o->context, 0x580, 0x10e, 2, w580, 0x10);
    o->set_rect(o->context, 0x588, 2, 2, 0x118, 0x10);
    o->set_rect(o->context, 0x4c0, 0, 0, 0x10e, 0x14);
    o->set_rect(o->context, 0x4c4, 5, y4c4, 0x7f, 0x72);
    for (k = 0; k < 12; ++k) {
        uint16_t off = (uint16_t)(0x4c8 + 4 * k);
        o->set_rect(o->context, off, 0x88 + 0x36 * (k % 6), by + 0x3a * (k / 6), 0x36, 0x36);
        o->button_style(o->context, off, 0);
    }
    if (small) {
        o->set_rect(o->context, 0x4dc, 0x196, 0x16a, 0x66, 0x35);
        o->button_style(o->context, 0x4dc, 2);
        o->set_rect(o->context, 0x4f4, 0x196, 0x1bb, 0x66, 0x35);
        o->button_style(o->context, 0x4f4, 2);
    }
    o->set_rect(o->context, 0x4f8, x4f8, 0, 0x48, 0x13);
    o->set_rect(o->context, 0x4fc, x4fc, 0, 0x6c, 0x13);
    o->set_rect(o->context, 0x500, x500, 0, 0x48, 0x13);
    o->set_rect(o->context, 0x504, small ? 0x25d : W == 800 ? 0x2fd : 0x3dd, y504, 0x1e, 0x1e);
    o->button_style(o->context, 0x504, 3);
    o->set_rect(o->context, 0x508, small ? 0x25d : W == 800 ? 0x2fd : 0x3dd, by, 0x1e, 0x1e);
    o->button_style(o->context, 0x508, 3);
    o->button_pages(o->context, small, (int16_t)(small ? 0 : 0xb), (int16_t)(small ? 2 : 10));
    gs_mode(o, 0x584, 2, 4, 4, 4, 4, 600, 600, 0x3c, 0x3c, 0, 0, 0, 0);
    gs_mode(o, 0x514, 7, 4, 4, 4, 4, 0x78, 0x78, 0x14, 0x14, 0, 0, 0, 0);
    for (i = 0; i < 10; ++i)
        gs_mode(o, (uint16_t)(0x518 + 4 * i), 9, 4, 4 + 0x14 * i, 4, 4, 0xaa, 0xaa, 0x14, 0x14, 0, 0, 0, 0);
    gs_mode(o, 0x50c, 8, 4, 0, o->world_11c ? 0xb8 : 4, 4, 0, 0, 0x22, 0x22, 0, 0, 0, 0);
    gs_mode(o, 0x510, 7, (o->screen_width - 500) / 2, 8, (o->screen_width - 500) / 2, 8, 0, 0, 0x14,
            0x14, 0, 0, 0, 0);
    for (i = 0; i < 6; ++i)
        gs_mode(o, lines[i], 8, 4, 0, 4, 0, 0, 0, 0x14, 0x14, 0, 0, 0, i == 0 ? 0x50c : lines[i - 1]);
    for (i = 0; i < 8; ++i) o->set_rect(o->context, (uint16_t)(0x55c + 4 * i), 4, 0x22 + 0x14 * i, 0x26c, 0x14);
    for (i = 7; i >= 0; --i)
        if (saved[i] != -1)
            gs_mode(o, (uint16_t)(0x58c + 4 * i), 10, 4, 0, 4, i == 7 ? 4 : 0, 200, 200, 0x14, 0x14, 0, 0, 0,
                    (uint16_t)(i == 7 ? 0 : 0x590 + 4 * i));
    o->set_active(o->context, 0x50c, 0);
    for (i = 0; i < 6; ++i) o->set_active(o->context, lines[i], 0);
    for (i = 0; i < 8; ++i)
        if (saved[i] != -1) o->set_active(o->context, (uint16_t)(0x58c + 4 * i), saved[i]);
    o->notify(o->context, o->viewed_player);
    return 0;
}

/* 0x004822a0 */
#define GSB_CMD(idx, frame, cmd, help) \
    o->set(o->context, 0, (idx), (frame), (cmd), 0, (help), -1, 0, 0, 0, NULL, NULL)
static void gsb_groups(const AoeGameScreenButtonsOpsRor *o) {
    if (o->selection_count < 2) {
        if (o->current_group) GSB_CMD(6, 8, 9, 0x1d4e3);
    } else {
        GSB_CMD(6, 7, 8, 0x1d4e1);
        GSB_CMD(7, 8, 9, 0x1d4e3);
    }
}
static void gsb_entry(const AoeGameScreenButtonsOpsRor *o, int set, int32_t command,
                      const AoeButtonEntryRor *e) {
    o->set(o->context, set, (int32_t)aoe_game_screen_button_slot_ror((uint8_t)e->slot), e->frame,
           command, e->id, e->help, e->help2, e->a, e->b, 0, e->name, e->help_text);
}
void aoe_game_screen_buttons_ror(const AoeGameScreenButtonsOpsRor *o) {
    AoeButtonEntryRor e;
    int16_t page_frame = o->frame_4b4;
    int32_t page_cmd = 6, i, n;
    *o->count = 0;
    *o->shown = -1;
    if (!o->has_current_unit) { o->hide_unused(o->context); return; }
    if (o->edit_mode == 6 && o->field_1b8 == 1) {
        int32_t wonder = -1, has57 = -1, more = 0;
        n = o->train_list(o->context, 0, 0x76);
        *o->count = (int16_t)n;
        for (i = 0; i < n; ++i) {
            o->train_entry(o->context, 0, i, &e);
            if (e.id == 0x6d) wonder = i;
            else if (e.id == 0x57) has57 = i;
        }
        for (i = 0; i < n; ++i) {
            o->train_entry(o->context, 0, i, &e);
            if (i == wonder && has57 == -1) e.slot = 6;
            if ((o->page == 0) ? e.slot < 0xb : e.slot > 10) gsb_entry(o, 3, 0x13, &e);
            else more = 1;
        }
        if (more) o->set(o->context, 4, 5, o->frame_4b0, 0xc, 0, 0x1d4d7, -1, 0, 0, 0, NULL, NULL);
        page_frame = o->frame_4b2; page_cmd = 0xb;
    } else if (o->edit_mode == 0 && o->game_state != 5) {
        if (o->current_is_ours) {
            uint8_t k = o->common_8f;
            int generic = 0;
            if (k == 3) {
                GSB_CMD(0, 2, 3, 0x1d4df);
                GSB_CMD(1, 0, 0x1f, 0x1d4f6);
                if (o->busy) GSB_CMD(2, 3, 5, 0x1d4dd);
                gsb_groups(o);
            } else if (k == 9) {
                if (o->busy) GSB_CMD(0, 3, 5, 0x1d4dd);
                gsb_groups(o);
            } else if (k == 4) {
                int32_t next = 1;
                GSB_CMD(0, 4, 0x16, 0x1d4ea);
                if (o->all_have_124_118) { GSB_CMD(1, 0xc, 0x17, 0x1d4eb); next = 2; }
                if (o->busy) GSB_CMD(next, 3, 5, 0x1d4dd);
                gsb_groups(o);
            } else if (k == 6) {
                GSB_CMD(0, 0xd, 0x1c, 0x1d4f1);
                GSB_CMD(1, 0xe, 0x1d, 0x1d4f2);
                if (o->busy) GSB_CMD(2, 3, 5, 0x1d4dd);
                gsb_groups(o);
            } else if (k == 7) {
                int32_t next = 0;
                if (o->any_cargo) { GSB_CMD(0, 5, 7, 0x1d4e9); next = 1; }
                if (o->busy) GSB_CMD(next, 3, 5, 0x1d4dd);
                gsb_groups(o);
            } else if (k == 5) {
                o->set(o->context, 1, 1, 0x57, 0x19, 0, 0x1a25c, 0x265af, 0, 0, 0, NULL, NULL);
                o->set(o->context, 1, 2, 0x56, 0x1a, 0, 0x1a25d, 0x265af, 0, 0, 0, NULL, NULL);
                o->set(o->context, 1, 3, 0x58, 0x1b, 0, 0x1a25e, 0x265af, 0, 0, 0, NULL, NULL);
                if (o->busy) GSB_CMD(0, 3, 5, 0x1d4dd);
                gsb_groups(o);
            } else if (k == 2 && o->is_building) {
                int16_t sub = o->action_type == 100 ? 0 : o->action_sub;
                if (o->current_state < 2) {
                    GSB_CMD(0, 3, 0x14, 0x1d4e5);
                } else if (o->current_state == 2 &&
                           sub != 0x67 && (sub != 0x66 || o->queue_enabled)) {
                    int32_t rn = o->research_list(o->context, o->current_template);
                    int32_t tn = o->train_list(o->context, 1, o->current_template);
                    int queued = 0;
                    *o->count = (int16_t)(tn + rn);
                    if (o->queue_enabled && o->queue_count > 0) {
                        queued = 1;
                        GSB_CMD(6, 3, 5, 0x1d4dd);
                    }
                    for (i = 0; i < tn; ++i) {
                        o->train_entry(o->context, 1, i, &e);
                        if (!queued) {
                            gsb_entry(o, 2, 0x12, &e);
                            o->queue_flag(o->context, 0, 0);
                        } else {
                            int32_t q = o->queued_of(o->context, (int16_t)e.id);
                            if (q > 0) {
                                gsb_entry(o, 2, 0x12, &e);
                                o->queue_flag(o->context, 1, q);
                            }
                        }
                    }
                    if (queued) { o->hide_unused(o->context); return; }
                    if (o->current_template == 0x6d && rn == 0 && o->age < 4 &&
                        o->age_research(o->context, o->age + 100, &e)) {
                        gsb_entry(o, 1, 0x11, &e);
                        rn = -1;
                    } else {
                        for (i = 0; i < rn; ++i) {
                            o->research_entry(o->context, i, &e);
                            gsb_entry(o, 1, 0x11, &e);
                        }
                    }
                    if (tn == 0 && rn == 0) gsb_groups(o);
                } else {
                    if (o->busy) GSB_CMD(0, 3, 5, 0x1d4dd);
                }
            } else if (k == 0 && o->selection_count > 1 && o->all_8f_3_to_9) {
                if (o->has_class_4) GSB_CMD(0, 2, 3, 0x1d4df);
                if (o->first_has_actions) GSB_CMD(o->has_class_4 ? 1 : 0, 3, 5, 0x1d4dd);
                gsb_groups(o);
            } else {
                generic = 1;
            }
            if (generic) {
                if (o->has_class_4) GSB_CMD(0, 2, 3, 0x1d4df);
                if (o->busy) GSB_CMD(o->has_class_4 ? 1 : 0, 3, 5, 0x1d4dd);
                gsb_groups(o);
                page_cmd = 0xb;
            }
        }
    } else {
        page_frame = o->frame_4b2; page_cmd = 0xb;
    }
    o->set(o->context, 4, 0xb, page_frame, page_cmd, 0, 0x1d4d9, -1, 0, 0, 0, NULL, NULL);
    o->hide_unused(o->context);
}

/* 0x0047d440 */
int aoe_game_screen_construct_ror(AoeGameScreenDefaultsRor *d, const AoeGameScreenConstructOpsRor *o,
                                  uint32_t now, uint32_t idle_interval,
                                  const char *(*string)(void *context, int32_t id)) {
    static const uint8_t cycle[21] = {0x17, 0x27, 0x7c, 0x27, 0x3f, 0x90, 0x3f, 0x5f, 0x9f, 0x57, 0x7b,
                                      0xb4, 0x3f, 0x5f, 0xa0, 0x27, 0x3f, 0x91, 0x17, 0x27, 0x7b};
    int32_t i, scores;
    memset(d, 0, sizeof *d);
    d->now = now;
    d->field_7c4 = 0x7fffffff;
    d->idle_interval = idle_interval;
    d->idle_last = now;
    d->map_interval = 0x14d;
    d->map_time = now;
    d->shown = -1;
    d->field_68c = 1;
    d->mouse_x = d->mouse_y = -1;
    d->field_714 = -1;
    d->view_interval = 100;
    d->view_time = now;
    d->key_group = string(o->context, 0x103f)[0];
    d->key_select = string(o->context, 0x104a)[0];
    d->key_idle = string(o->context, 0x103a)[0];
    d->field_780 = 0x41;
    for (i = 0; i < 8; ++i) d->key_number[i] = string(o->context, 0x1040 + i)[0];
    d->field_7a8 = 3000;
    d->field_7a4 = 1;
    d->time_7ac = now;
    o->step(o->context, AOE_GSC_BASE, 0);
    if (!o->step(o->context, AOE_GSC_SETUP, 0)) return 0;
    for (i = 0; i < 8; ++i)
        if (!o->step(o->context, AOE_GSC_SHAPE, i)) return 0;
    if (!o->step(o->context, AOE_GSC_VIEW, 0)) return 0;
    if (!o->step(o->context, AOE_GSC_MINIMAP, 0)) return 0;
    if (!o->step(o->context, AOE_GSC_RESOURCES, 0)) return 0;
    if (!o->step(o->context, AOE_GSC_OBJECT_PANEL, 0)) return 0;
    for (i = 0; i < 17; ++i)
        if (!o->step(o->context, AOE_GSC_BUTTON, i)) return 0;
    for (i = 12; i < 17; ++i) o->step(o->context, AOE_GSC_BUTTON_FIXED, i);
    if (o->settings_paused) o->set_active(o->context, 0x4f8, 0);
    o->step(o->context, AOE_GSC_BUTTON_TEXT, 15);
    o->step(o->context, AOE_GSC_BUTTON_TEXT, 16);
    if (!o->step(o->context, AOE_GSC_TEXT_PANEL, 0)) return 0;
    o->step(o->context, AOE_GSC_TEXT_FROM_ID, 0);
    o->step(o->context, AOE_GSC_TEXT_FROM_ID, 1);
    for (i = 0; i < 12; ++i)
        if (!o->step(o->context, AOE_GSC_TIMER_PANEL, i)) return 0;
    for (i = 0; i < 14; ++i)
        if (!o->step(o->context, AOE_GSC_LINE, i)) return 0;
    if (!o->step(o->context, AOE_GSC_LIST, 0)) return 0;
    if (o->global_580e48) o->set_active(o->context, 0x580, 0);
    if (!o->step(o->context, AOE_GSC_LIST, 1)) return 0;
    if (!o->field_8e8) {
        o->set_active(o->context, 0x588, 0);
        o->set_active(o->context, 0x4c0, 1);
        if (!o->global_580e48) o->set_active(o->context, 0x580, 1);
    } else {
        o->set_active(o->context, 0x588, 1);
        o->set_active(o->context, 0x4c0, 0);
        if (!o->global_580e48) o->set_active(o->context, 0x580, 0);
    }
    if (!o->networked) {
        scores = o->world_players - 1;
    } else {
        scores = 0;
        for (i = 1; i < 9; ++i) if (o->slot_counts(o->context, i)) ++scores;
        if (scores == 0) scores = 1;
    }
    for (i = 0; i < scores; ++i)
        if (!o->step(o->context, AOE_GSC_SCORE, i)) return 0;
    if (!o->step(o->context, AOE_GSC_OVERLAY, 0)) return 0;
    o->step(o->context, AOE_GSC_FINISH, 0);
    d->cue_interval = 20000;
    d->cue_time = now;
    d->cycle_period = 200;
    d->cycle_time = now;
    memcpy(d->cycle_rgb, cycle, sizeof cycle);
    return 1;
}

/* 0x004816d0 */
static void *gsp_replace(const AoeGameScreenStyleOpsRor *o, void **slot, const char *pattern,
                         int32_t style, int32_t id) {
    char name[0x104];
    if (*slot) o->free_shape(o->context, *slot);
    snprintf(name, sizeof name, pattern, (int)style);
    *slot = o->load_shape(o->context, name, id);
    return *slot;
}

void aoe_game_screen_style_ror(const AoeGameScreenStyleOpsRor *o, void *shapes[AOE_GSP_SHAPES],
                                void **page_picture) {
    static const uint8_t bevels[5][6] = {
        {0x7e, 0x6d, 0x6f, 0x6f, 0xee, 0x38}, {0x72, 0x73, 0x74, 0x74, 0xb7, 0xb8},
        {0xb9, 0x77, 0x78, 0x78, 0x78, 0x79}, {0x8a, 0xed, 0xee, 0xee, 0x38, 0x95},
        {0x73, 0x74, 0x75, 0x75, 0x76, 0x77}};
    static const uint16_t bevelled[5] = {0x4f8, 0x4fc, 0x500, 0x504, 0x508};
    char name[0x104];
    int32_t s, id, i;
    uint32_t a, b;
    o->player_to_panels(o->context);
    o->mode_changed(o->context);
    o->store_player(o->context);
    o->age(o->context);
    if (o->player_id != 0 &&
        ((int32_t)o->player_style != *o->style || o->width != *o->style_width)) {
        *o->style = o->player_style;
        *o->style_width = o->width;
        s = o->player_style < 5 ? o->player_style : 0;
        snprintf(name, sizeof name, "dlg6_%d", (int)s);
        id = s == 4 ? 0xcf08 : 0xc356 + s;
        o->set_info(o->context, name, id);
        o->set_popup(o->context, name, id);
        snprintf(name, sizeof name, "btn6_%d", (int)s);
        o->set_button_pictures(o->context, name, s == 4 ? 0xcf0c : 0xc5ad + s);
        o->set_bevel_colors(o->context, bevels[s]);
        for (i = 0; i < 5; ++i)
            o->button_bevel(o->context, bevelled[i], i < 3 ? 3 : 4, bevels[s]);
        gsp_replace(o, &shapes[AOE_GSP_BORDER_A], "btnbrda%d.shp", s, s == 4 ? 0xcf0e : 0xc619 + s);
        gsp_replace(o, &shapes[AOE_GSP_OTHER], "btnoth%d.shp", s, s == 4 ? 0xcf11 : 0xc625 + s);
        gsp_replace(o, &shapes[AOE_GSP_BORDER_B], "btnbrdb%d.shp", s, s == 4 ? 0xcf0f : 0xc61d + s);
        gsp_replace(o, &shapes[AOE_GSP_BORDER_C], "btnbrdc%d.shp", s, s == 4 ? 0xcf10 : 0xc63b + s);
        if (o->width >= 0x400 && o->height >= 0x300)
            gsp_replace(o, &shapes[AOE_GSP_GAME], "gamec%d.shp", s, s == 4 ? 0xcf14 : 0xc635 + s);
        else if (o->width >= 800 && o->height >= 600)
            gsp_replace(o, &shapes[AOE_GSP_GAME], "gameb%d.shp", s, s == 4 ? 0xcf13 : 0xc631 + s);
        else
            gsp_replace(o, &shapes[AOE_GSP_GAME], "gamea%d.shp", s, s == 4 ? 0xcf12 : 0xc62d + s);
        for (i = 0; i < 17; ++i)
            o->button_border(o->context, i,
                             i < 12 ? shapes[AOE_GSP_BORDER_A] : shapes[AOE_GSP_BORDER_B]);
        *page_picture = (o->width >= 800 && o->height >= 600) ? shapes[AOE_GSP_COMMANDS]
                                                              : shapes[AOE_GSP_OTHER];
        o->button_picture(o->context, 0x4dc, 0, *page_picture, o->frame_4b0);
        o->button_picture(o->context, 0x4f4, 0, *page_picture,
                          o->button_id(o->context, 0x4f4) == 6 ? o->frame_4b4 : o->frame_4b2);
        if (s == 2 || s == 3 || s == 4) {
            o->text_colors(o->context, 0x4c0, 0xffffff, 0);
            o->text_colors(o->context, 0x580, 0xffffff, 0);
            a = 0xffffff;
            b = 0;
        } else {
            o->text_colors(o->context, 0x4c0, 0, 0xffffff);
            o->text_colors(o->context, 0x580, 0, 0xffffff);
            a = 0;
            b = 0xffffff;
        }
        for (i = 0; i < 5; ++i) o->button_text_colors(o->context, bevelled[i], 0, a, b);
    }
    o->rebuild_buttons(o->context);
    o->redraw(o->context);
}
