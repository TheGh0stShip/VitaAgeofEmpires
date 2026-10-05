#include "game_window.h"

#include <stdio.h>

static int32_t routed(const AoeGameWindowOpsRor *ops, int slot, uint32_t msg,
                      uint32_t wparam, uint32_t lparam, int32_t screen_result) {
    if (!ops->handler(ops->context, slot, msg, wparam, lparam)) return screen_result;
    return ops->default_proc(ops->context, msg, wparam, lparam);
}

int32_t aoe_game_window_proc_ror(const AoeGameWindowOpsRor *ops, uint32_t msg,
                                 uint32_t wparam, uint32_t lparam) {
    int32_t result = 0;
    if (ops->pre_dispatch) ops->pre_dispatch(ops->context, msg, wparam, lparam);
    if (ops->has_screen) {
        result = ops->screen_message(ops->context, msg, wparam, lparam);
        if (result != 0 && msg != 0x200 && msg != 0x113) return result;
    }
    switch (msg) {
    case 0x112: /* WM_SYSCOMMAND: no screen saver over a full screen */
        if (wparam == 0xf100 && ops->full_screen && !ops->iconic(ops->context)) return 0;
        break;
    case 0x1c: /* WM_ACTIVATEAPP */
        if (ops->activate_app) ops->activate_app(ops->context, wparam);
        return routed(ops, AOE_GAME_SLOT_ACTIVATE, msg, wparam, lparam, result);
    case 0x100: case 0x104:
        return routed(ops, AOE_GAME_SLOT_KEY, msg, wparam, lparam, result);
    case 0x105: /* WM_SYSKEYUP */
        if (wparam == 0x12 && ops->has_screen && ops->screen_alt_up) ops->screen_alt_up(ops->context);
        break;
    case 0x111:
        return routed(ops, AOE_GAME_SLOT_COMMAND, msg, wparam, lparam, result);
    case 2:
        return routed(ops, AOE_GAME_SLOT_DESTROY, msg, wparam, lparam, result);
    case 5:
        if (!ops->size_locked) return routed(ops, AOE_GAME_SLOT_SIZE, msg, wparam, lparam, result);
        break;
    case 6:
        if (ops->track_focus) ops->track_focus(ops->context, wparam, lparam);
        break;
    case 0xf:
        return routed(ops, AOE_GAME_SLOT_PAINT, msg, wparam, lparam, result);
    case 0x10:
        return routed(ops, AOE_GAME_SLOT_CLOSE, msg, wparam, lparam, result);
    case 0x400:
        return routed(ops, AOE_GAME_SLOT_USER, msg, wparam, lparam, result);
    case 0x311:
        return routed(ops, AOE_GAME_SLOT_PALETTE, msg, wparam, lparam, result);
    case 0x3b9: case 0x500:
        return routed(ops, AOE_GAME_SLOT_MCI, msg, wparam, lparam, result);
    case 0x30f:
        if (!ops->handler(ops->context, AOE_GAME_SLOT_QUERYPAL, msg, wparam, lparam)) return result;
        return routed(ops, AOE_GAME_SLOT_CLOSE, msg, wparam, lparam, result);
    case 0x200:
        if (!ops->handler(ops->context, AOE_GAME_SLOT_MOUSE, msg, wparam, lparam)) return result;
        if (ops->after_mouse) ops->after_mouse(ops->context);
        break;
    case 0x113:
        ops->handler(ops->context, AOE_GAME_SLOT_IDLE, msg, wparam, lparam);
        break;
    case 0x116:
        return routed(ops, AOE_GAME_SLOT_INITMENU, msg, wparam, lparam, result);
    case 0x212:
        return routed(ops, AOE_GAME_SLOT_EXITMENU, msg, wparam, lparam, result);
    case 0x218: /* WM_POWERBROADCAST query: BROADCAST_QUERY_DENY */
        if (wparam == 0) return 0x424d5144;
        break;
    default:
        break;
    }
    return ops->default_proc(ops->context, msg, wparam, lparam);
}

int aoe_game_key_down_ror(AoeGameKeyOpsRor *ops, int own_window, uint32_t msg,
                          uint32_t wparam, uint32_t lparam) {
    if (!ops->active) return 1;
    ops->last_repeat = (int32_t)(lparam & 0xffff);
    ops->last_key = (int32_t)wparam;
    ops->ctrl = ops->key_state(ops->context, 0x11);
    ops->alt = ops->key_state(ops->context, 0x12);
    ops->shift = ops->key_state(ops->context, 0x10);
    if (own_window && msg == 0x104 &&
        (ops->last_key == 0x12 || ops->last_key == 0x20 || ops->last_key == 0x79) &&
        ops->full_screen && !ops->iconic(ops->context))
        return 0;
    if (ops->ctrl && ops->last_key == 0x7b) {
        ops->screenshot(ops->context);
        return 0;
    }
    return ops->key(ops->context, ops->last_key, ops->last_repeat, ops->ctrl, ops->alt,
                    ops->shift) != 0;
}

int aoe_game_command_ror(const AoeGameCommandOpsRor *ops, int32_t id, int32_t value) {
    if (ops->comm_count(ops->context) > 0xc && !ops->locked) {
        if (id == 0x17b3) ops->player_d4(ops->context, value, 1);
        else if (id == 0x17b4) ops->player_d4(ops->context, value, 0);
    }
    if (id == 0x17a2) {
        if (ops->has_world) ops->world_ec(ops->context, 1);
    } else if (id == 0x17a3 && ops->has_world) {
        ops->world_ec(ops->context, 0);
    }
    return ops->command(ops->context, id, value) != 0;
}

int aoe_game_activate_ror(AoeGameActivateOpsRor *ops, int32_t *active, int on) {
    int running = ops->state == 4 || ops->state == 6 || ops->state == 7;
    *active = on != 0;
    if (*active) {
        if (running && ops->paused_by_focus == 1) {
            ops->paused_by_focus = 0;
            ops->comm_pause(ops->context, 0);
        }
        if (ops->has_screen) ops->screen_activate(ops->context);
        ops->on_active(ops->context);
        return 1;
    }
    if (running && ops->single_player && !ops->comm_waiting(ops->context)) {
        ops->paused_by_focus = 1;
        ops->comm_pause(ops->context, 1);
    }
    ops->on_inactive(ops->context);
    return 1;
}

int aoe_game_close_ror(const AoeGameCloseOpsRor *ops, int32_t *window_active,
                       void **window) {
    *window_active = 0;
    if (ops->timer_running) ops->kill_timer(ops->context);
    *window = NULL;
    ops->post_quit(ops->context);
    return 1;
}

int aoe_game_timer_ror(const AoeGameTimerOpsRor *ops) {
    if (!ops->window_ok) return 0;
    if (*ops->restore_count != 0 && ops->app_active) {
        uint32_t wait = *ops->restore_count > 5 ? 500 : 0xfa;
        if (ops->now_ms(ops->context) - *ops->restore_ms >= wait) {
            ops->restore_palette(ops->context);
            --*ops->restore_count;
            *ops->restore_ms = ops->now_ms(ops->context);
        }
    }
    if (ops->sound_tick) ops->sound_tick(ops->context);
    if (ops->screen_idle) ops->screen_idle(ops->context);
    if (ops->comm_present && ops->comm_present(ops->context)) ops->comm_tick(ops->context);
    if (ops->flag_8ec) ops->slot_13c(ops->context);
    if (ops->flag_8f0) ops->slot_140(ops->context);
    return 1;
}

void aoe_game_fps_caption_ror(char *out, int size, const char *format,
                              uint32_t samples, const uint32_t t[10]) {
    uint32_t n = samples ? samples : 1;
    snprintf(out, (size_t)size, format, (unsigned long)t[0], (unsigned long)(t[1] / n),
             (unsigned long)(t[2] / n), (unsigned long)(t[3] / n), (unsigned long)(t[4] / n),
             (unsigned long)(t[5] / n), (unsigned long)(t[6] / n), (unsigned long)(t[7] / n),
             (unsigned long)(t[8] / n), (unsigned long)(t[9] / n));
}

void aoe_game_wait_cursor_ror(const AoeGameCursorOpsRor *ops, int32_t *flag_1d8) {
    if (ops->windows_cursor) ops->set_cursor(ops->context, 1);
    ops->call_4522d0(ops->context);
    *flag_1d8 = 0;
    if (ops->capture) ops->recapture(ops->context, ops->capture, 1);
}

void aoe_game_arrow_cursor_ror(const AoeGameCursorOpsRor *ops, int32_t *flag_1d8) {
    if (ops->capture) ops->recapture(ops->context, ops->capture, 0);
    ops->call_4522c0(ops->context);
    *flag_1d8 = 1;
    if (ops->windows_cursor) ops->set_cursor(ops->context, 0);
}

int aoe_game_relay_ror(int32_t (*target)(void *context), void *context) {
    return target(context) != 0;
}
