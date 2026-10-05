#include "game_screen.h"

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
