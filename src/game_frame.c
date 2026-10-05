#include "game_frame.h"

#include <stddef.h>

int aoe_game_frame_register_ror(const AoeGameFrameOpsRor *ops) {
    if (ops->existing_window) return 1;
    return ops->register_class(ops->context, 0xb, ops->class_name, ops->instance, ops->icon_name,
                               ops->menu_name && ops->menu_name[0] ? ops->menu_name : "") != 0;
}

int aoe_game_frame_create_ror(AoeGameFrameOpsRor *ops) {
    int32_t w, h, wr[4], cr[4];
    ops->screen_size(ops->context, &w, &h);
    if (ops->windowed || (w == *ops->width && h == *ops->height))
        ops->window = ops->create_window(ops->context, ops->class_name, ops->title, 0x82080000u,
                                         w, h);
    if (!ops->window) return 0;
    ops->window_rect(ops->context, ops->window, wr);
    ops->client_rect(ops->context, ops->window, cr);
    if (cr[2] != *ops->width || cr[3] != *ops->height)
        ops->move_window(ops->context, ops->window, wr[0], wr[1],
                         *ops->width - wr[0] - cr[2] + wr[2],
                         *ops->height - wr[1] - cr[3] + wr[3]);
    if (ops->windowed) {
        ops->show(ops->context, ops->window, ops->show_command);
        ops->update(ops->context, ops->window);
        ops->focus(ops->context, ops->window);
    }
    *ops->main_window = ops->window;
    ops->after_create(ops->context);
    return 1;
}

void aoe_game_frame_close_ror(const AoeGameFrameOpsRor *ops) {
    ops->send(ops->context, ops->window, 0x10);
}

void aoe_game_frame_update_ror(const AoeGameFrameOpsRor *ops) {
    ops->update(ops->context, ops->window);
}

void aoe_game_frame_clear_ror(const AoeGameFrameOpsRor *ops) {
    if (ops->window) ops->fill_black(ops->context, ops->window);
}

void aoe_game_frame_caption_ror(const AoeGameFrameOpsRor *ops, const char *caption) {
    ops->set_text(ops->context, ops->window, caption);
}

int32_t aoe_game_frame_relay_ror(void *game, int32_t (*proc)(void *game, uint32_t window,
                                                            uint32_t msg, uint32_t wparam,
                                                            uint32_t lparam),
                                 uint32_t window, uint32_t msg, uint32_t wparam,
                                 uint32_t lparam) {
    if (!game) return 0;
    return proc(game, window, msg, wparam, lparam);
}

int aoe_game_comm_create_ror(void **comm, const AoeGameCommOpsRor *ops) {
    int i;
    *comm = ops->comm_new(ops->context);
    if (!*comm) return 0;
    ops->comm_setup(ops->context, *comm);
    *ops->published = *comm;
    if (ops->has_world) ops->world_comm(ops->context, *comm);
    for (i = 0; i < 2; ++i)
        if (ops->listeners[i]) ops->listen(ops->context, ops->listeners[i], *comm);
    return 1;
}

void aoe_game_comm_restart_ror(void **comm, const AoeGameCommOpsRor *ops) {
    if (*comm) {
        ops->comm_free(ops->context, *comm);
        *comm = NULL;
        *ops->published = NULL;
    }
    ops->recreate(ops->context);
}

int aoe_game_chat_create_ror(const AoeGameCommOpsRor *ops, uint32_t window) {
    *ops->chat = ops->chat_new(ops->context, window);
    return *ops->chat != NULL;
}

int aoe_game_palette_load_ror(void **palette, const AoeGameCommOpsRor *ops, const char *name) {
    *palette = ops->palette_load(ops->context, name, 50500);
    return *palette != NULL;
}

int32_t aoe_game_palette_changed_ror(const AoeGameDisplayOpsRor *ops, uint32_t window,
                                     uint32_t wparam, uint32_t a, uint32_t b) {
    if (ops->has_draw) ops->draw_forward(ops->context, 0, window, wparam, a, b);
    if (wparam == window) return 1;
    if (ops->full_screen_palette || !ops->has_draw)
        ops->game_palette(ops->context);
    else if (ops->mode != 1)
        ops->set_palette(ops->context);
    return 1;
}

int32_t aoe_game_display_changed_ror(const AoeGameDisplayOpsRor *ops, int32_t *width,
                                     int32_t *height, uint32_t a, uint32_t b, uint32_t wparam,
                                     uint32_t c) {
    int32_t w, h;
    if (wparam == 1) return 1;
    if (ops->has_draw) {
        ops->draw_forward(ops->context, 1, a, b, wparam, c);
        if (ops->draw_size(ops->context, &w, &h) && w != 0) {
            *width = w;
            *height = h;
        }
        ops->panel_resize(ops->context, *width, *height);
    }
    ops->after_display(ops->context);
    return 1;
}

int32_t aoe_game_mouse_message_ror(const AoeGameDisplayOpsRor *ops, uint32_t wparam,
                                   uint32_t lparam) {
    int32_t x, y;
    if (!ops->has_mouse) return 1;
    if (ops->cursor_capable && !ops->software_cursor) {
        if (!ops->in_client(ops->context))
            ops->cursor_off(ops->context);
        else
            ops->cursor_restore(ops->context);
    }
    /* 0x00419ab0: x and y from the low and high words (above 60000 wraps
       negative), the button bits 1, 2, 8 and 4. */
    x = (int32_t)(lparam & 0xffff);
    y = (int32_t)(lparam >> 16);
    if (x > 60000) x -= 0x10000;
    if (y > 60000) y -= 0x10000;
    ops->mouse(ops->context, x, y, wparam & 1, wparam & 2, wparam & 8, wparam & 4);
    ops->tick(ops->context);
    return 1;
}

void aoe_game_campaign_get_ror(const AoeGameCampaignOpsRor *ops, int32_t *campaign,
                               int32_t *player, int32_t *scenario) {
    *campaign = ops->campaign(ops->context);
    *player = ops->player(ops->context);
    *scenario = ops->scenario(ops->context);
}

int aoe_game_campaign_set_ror(const AoeGameCampaignOpsRor *ops, int32_t campaign,
                              int32_t player, int32_t scenario) {
    return ops->choose_campaign(ops->context, campaign) && ops->choose_player(ops->context, player) &&
           ops->choose_scenario(ops->context, scenario);
}

int aoe_game_play_sound_ror(void *const *sounds, int32_t i,
                            int (*playing)(void *context, void *sound),
                            int (*play)(void *context, void *sound), void *context) {
    if (!sounds || !sounds[i]) return 0;
    if (playing(context, sounds[i])) return 1;
    return play(context, sounds[i]);
}
