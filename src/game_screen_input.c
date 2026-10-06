#include "game_screen_input.h"

#include <string.h>

/* 0x004807b0: game screen key-down. */
int aoe_game_screen_key_down_ror(AoeGameScreenInputRor *s, const AoeGameScreenInputOpsRor *o,
                                 int32_t key, int32_t count, int alt, int ctrl, int shift)
{
    void *c = o->ctx;
    int playing;
    int32_t i;

    if (o->busy(c))
        return 0;
    if (o->child_active(c) && o->child_key(c, key, count, alt, ctrl, shift))
        return 1;
    playing = o->game_state(c) != 5;
    if (ctrl) {
        if (shift && alt) {
            /* Chat typing: letters upper-cased, digits kept, others ignored. */
            if (playing) {
                if (key > 0x60 && key < 0x7b)
                    key -= 0x20;
                else if (key > 0x5a || key < 0x41) {
                    if (key < 0x30 || key > 0x39)
                        return 1;
                }
                if (key != 0) {
                    if (s->chat_len == AOE_GS_INPUT_CHAT_LEN) {
                        s->chat[AOE_GS_INPUT_CHAT_LEN] = (char)key;
                        memmove(s->chat, s->chat + 1, AOE_GS_INPUT_CHAT_LEN + 1);
                    } else {
                        s->chat[s->chat_len] = (char)key;
                        s->chat_len++;
                        s->chat[s->chat_len] = 0;
                    }
                    if (o->send_chat(c, s->chat)) {
                        s->chat_len = 0;
                        s->chat[0] = 0;
                    }
                }
            }
            return 1;
        }
        if (key > 0x2f && key < 0x3a) {
            o->select_group(c, key - 0x30);
            return 1;
        }
    }
    if (shift) {
        if (key == 9) {
            o->cycle_idle(c, 0);
            return 1;
        }
        if (key >= 0x30 && key <= 0x39) {
            o->group(c, key - 0x30, 0);
            return 1;
        }
        if (key == 0x70) {
            o->help(c);
            return 1;
        }
        if (key >= 0x71 && key <= 0x76) {
            if (playing && o->can_issue(c))
                o->fkey(c, key - 0x71);
            return 1;
        }
    }
    if (alt) {
        if (key > 0x2f && key < 0x3a) {
            o->group(c, key - 0x30, 1);
            o->space(c);
            return 1;
        }
        return 0;
    }
    if (!ctrl) {
        if (!o->popup_open(c) && o->game_mode(c) == 6 && o->game_sub_mode(c) == 1) {
            int32_t n = o->button_count(c);
            for (i = 0; i < n; i++) {
                int32_t id = o->button_id(c, i);
                if (key == o->button_hotkey(c, id)) {
                    o->clear_popup(c);
                    o->press_button(c, id);
                    return 1;
                }
            }
        }
    } else {
        for (i = 0; i < AOE_GS_INPUT_GROUP_KEYS; i++) {
            if (key == s->group_keys[i]) {
                o->group_key(c, i);
                return 1;
            }
        }
    }
    if (s->key_menu != 0 && key == s->key_menu) {
        o->group_key(c, 0);
        return 0;
    }
    if (ctrl)
        return 0;
    if (o->game_mode(c) != 6 || o->game_sub_mode(c) != 1) {
        if (s->key_a != 0 && key == s->key_a && playing) {
            if (o->can_issue(c))
                o->key_a_action(c);
            return 0;
        }
        if (s->key_b != 0 && key == s->key_b && playing) {
            if (o->can_issue(c))
                o->key_b_action(c);
            return 0;
        }
        if (s->key_c != 0 && key == s->key_c && playing) {
            if (o->can_issue(c))
                o->key_c_action(c);
            return 0;
        }
        if (s->key_d != 0 && key == s->key_d) {
            o->key_d_action(c);
            return 0;
        }
    }
    switch (key) {
    case 9: o->cycle_idle(c, 1); return 1;
    case 0x0d: o->enter(c); return 0;
    case 0x20: o->space(c); return 0;
    case 0x24: o->home(c); return 0;
    case 0x2e:
        if (playing && o->can_issue(c))
            o->delete_key(c);
        return 0;
    case 0x73: o->key_s(c, o->toggle_flag(c) == 0); return 0;
    case 0x74: o->key_t(c); return 0;
    case 0x7a:
        o->set_zoom(c, o->zoomed(c) == 0);
        o->zoom_refresh(c);
        return 0;
    case 0xbe: o->key_be(c); return 1;
    default:
        if (key >= 0x30 && key <= 0x39) {
            o->group(c, key - 0x30, 1);
            return 0;
        }
        return 0;
    }
}
