#include "game_systems.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char aoe_game_global_key_ror[] =
    "Software\\Microsoft\\Microsoft Games\\Age of Empires Expansion\\1.0";

int aoe_game_load_common_shapes_ror(const AoeGameResourceOpsRor *ops, void *shapes[3],
                                    int16_t *count) {
    *count = 3;
    shapes[0] = shapes[1] = shapes[2] = NULL;
    shapes[0] = ops->load_shape(ops->context, "groupnum.shp", 0xc4e3);
    shapes[1] = ops->load_shape(ops->context, "waypoint.shp", 0xc4e4);
    shapes[2] = ops->load_shape(ops->context, "moveto.shp", 0xc4e5);
    return 1;
}

int aoe_game_load_common_sounds_ror(const AoeGameResourceOpsRor *ops, void *sounds[3],
                                    int16_t *count) {
    int16_t i;
    *count = 3;
    sounds[0] = sounds[1] = sounds[2] = NULL;
    sounds[0] = ops->load_sound(ops->context, "button1.wav", 0xc47c);
    sounds[1] = ops->load_sound(ops->context, "button2.wav", 0xc47d);
    sounds[2] = ops->load_sound(ops->context, "chatrcvd.wav", 0xc47e);
    for (i = 0; i < *count; ++i)
        if (sounds[i]) ops->preload_sound(ops->context, sounds[i]);
    return 1;
}

int aoe_game_start_sound_ror(const AoeGameSoundOpsRor *ops, void **system) {
    if (ops->sound_on) {
        int32_t v, volume = 0;
        *system = ops->create(ops->context);
        if (*system && !ops->start(ops->context, *system)) {
            ops->destroy(ops->context, *system);
            *system = NULL;
            return 1;
        }
        v = ops->registry_get(ops->context, "Sound Volume");
        if (v != -1) volume = -v;
        ops->set_volume(ops->context, *system, volume);
    }
    ops->slot_58(ops->context);
    return 1;
}

void aoe_game_stop_sound_ror(const AoeGameSoundOpsRor *ops, void **system) {
    if (!*system) return;
    ops->registry_set(ops->context, "Sound Volume", -ops->get_volume(ops->context, *system));
    ops->destroy(ops->context, *system);
    *system = NULL;
}

void aoe_game_sound_off_ror(const AoeGameSoundCycleOpsRor *c, const AoeGameSoundOpsRor *ops,
                            void **system, void ***sounds, int16_t *count) {
    int32_t i;
    if (c->has_comm) c->comm_sound(c->context, NULL);
    c->call_4522e0(c->context);
    for (i = 0; i < c->player_count; ++i) c->player_sound(c->context, i, NULL);
    if (*sounds) {
        for (i = 0; i < *count; ++i)
            if ((*sounds)[i]) c->free_sound(c->context, (*sounds)[i]);
        free(*sounds);
        *sounds = NULL;
    }
    c->stop_music(c->context);
    aoe_game_stop_sound_ror(ops, system);
}

int aoe_game_sound_on_ror(const AoeGameSoundCycleOpsRor *c, void **system) {
    int32_t i;
    if (*system) return 1;
    if (!c->slot_94(c->context) || !c->slot_9c(c->context)) return 0;
    for (i = 0; i < c->player_count; ++i) c->player_sound(c->context, i, *system);
    c->call_452310(c->context);
    if (c->has_comm) c->comm_sound(c->context, *system);
    c->slot_b8(c->context);
    return 1;
}

void aoe_game_stop_music_ror(const AoeGameMusicOpsRor *ops, void **music) {
    if (!*music) return;
    ops->stop(ops->context);
    ops->registry_set(ops->context, "Music Volume", -ops->volume(ops->context, *music));
    ops->destroy(ops->context, *music);
    *music = NULL;
}

int aoe_game_setup_pointer_ror(const AoeGamePointerOpsRor *ops, int32_t *custom_mouse,
                               void **pointer) {
    int custom = 0;
    int32_t v = ops->registry_get(ops->context, "Custom Mouse");
    if (v == -1) {
        ops->registry_set(ops->context, "Custom Mouse", 0);
    } else if (v == 1) {
        custom = 1;
    } else if (v == 2) {
        *custom_mouse = 0;
    }
    *pointer = ops->create(ops->context, custom);
    if (!*pointer) return 0;
    if (!ops->setup(ops->context, *pointer, *custom_mouse, 51000, 10)) return 0;
    ops->after(ops->context);
    return 1;
}

int aoe_game_open_registry_ror(const AoeGameRegistryOpsRor *ops, const char *game_key,
                               void **key, void **global_key) {
    *key = ops->open_key(ops->context, game_key);
    *global_key = ops->open_key(ops->context, aoe_game_global_key_ror);
    return *key != NULL;
}

int aoe_game_open_log_ror(const AoeGameLogOpsRor *ops, int32_t flag, int32_t level,
                          void **log) {
    *log = ops->create(ops->context);
    if (!*log) return 0;
    ops->set_flag(ops->context, *log, flag);
    ops->set_level(ops->context, *log, level);
    ops->set_on(ops->context, *log, 1);
    return 1;
}

void aoe_game_draw_log_ror(AoeGameDrawLogOpsRor *ops, const char *text) {
    if (!ops->file) return;
    ops->write(ops->context, ops->file, text);
    ops->flush(ops->context, ops->file);
}

void aoe_game_draw_log_reopen_ror(AoeGameDrawLogOpsRor *ops, const char *text) {
    if (!ops->file) return;
    ops->write(ops->context, ops->file, text);
    ops->close(ops->context, ops->file);
    ops->file = ops->reopen(ops->context);
}

int aoe_game_blank_screen_ror(const AoeGameBlankOpsRor *ops) {
    void *screen = ops->create_screen(ops->context, "Blank Screen");
    if (!screen || !ops->setup(ops->context, screen)) return 0;
    ops->show(ops->context, "Blank Screen");
    ops->after(ops->context);
    ops->paint(ops->context);
    return 1;
}

int aoe_game_saved_version_ok_ror(const char tag[4]) {
    static const char tags[10][4] = {{'1', '.', '1', '2'}, {'1', '.', '1', '1'},
                                     {'1', '.', '1', '0'}, {'1', '.', '0', '9'},
                                     {'1', '.', '0', '8'}, {'1', '.', '0', '7'},
                                     {'1', '.', '0', '6'}, {'1', '.', '0', '5'},
                                     {'1', '.', '0', '4'}, {'1', '.', '0', '3'}};
    int i;
    for (i = 0; i < 10; ++i)
        if (memcmp(tag, tags[i], 4) == 0) return 1;
    return 0;
}

int aoe_game_map_save_area_ror(const AoeGameSurfaceOpsRor *ops, void **area) {
    *area = ops->create(ops->context, "Diamond Map Save Area");
    if (!*area) return 0;
    if (!ops->attach(ops->context, *area, 0x118, 0x8c)) {
        ops->destroy(ops->context, *area);
        *area = NULL;
        return 0;
    }
    return 1;
}

void aoe_game_no_ai_caption_ror(char *out, int size, int16_t player) {
    snprintf(out, (size_t)size, "No AI Information for Player #%d.", player);
}
