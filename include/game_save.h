#ifndef VITA_AOE_GAME_SAVE_H
#define VITA_AOE_GAME_SAVE_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a tribe game: the "Save" of a running game
   (0x00501250, thiscall on the game object, `ret 4`), the counterpart of
   the load 0x00502d60 (aoe_game_ror_start_file_ror in game_ror.h). The
   file itself is written by the world's save virtual (world +0x3f4,
   vtable +0xd8 = 0x0051e0c0, aoe_world_save_game_ror in world_game.h:
   "<save dir><name>", "VER 8.6", the version, the state through +0x68
   with the players); this level only gates on the game state, wraps the
   save in the status screen and the wait cursor and, on success, keeps
   the name without its extension and returns to the game screen. The
   game, the world and the screen system are ops; offsets noted. */

typedef struct {
    void *context;
    int32_t mode;                /* game +0x1b0 */
    int32_t *global_558094;      /* 0x00558094: set to 1 after every save attempt */
    /* The wait cursor (0x0041bd60, wait 1) and its release (0x0041bdc0,
       wait 0). */
    void (*cursor)(void *context, int wait);
    /* The status screen with string `id` (0x00500390(id, 0, -1)) and its
       close (0x005003e0). */
    void (*status)(void *context, int32_t id);
    void (*status_close)(void *context);
    /* The world's save (+0x3f4 vtable +0xd8 = 0x0051e0c0) of the file
       `name`; the original keeps only the returned byte (and eax, 0xff),
       nonzero on success. */
    uint8_t (*world_save)(void *context, const char *name);
    /* The saved game name (+0xe64, 0x004ff4b0). */
    void (*set_saved_name)(void *context, const char *name);
    /* A single player game (0x0041b750: the byte +0x97d, zero-extended)
       and the comm pause cleared (0x00419a60(0, 0)). */
    uint8_t (*single_player)(void *context);
    void (*unpause)(void *context);
    /* The screen system (0x005830e8): a screen shown by name
       (0x00451be0(name, 0)) and removed by name (0x00451df0). */
    void (*show_screen)(void *context, const char *name);
    void (*remove_screen)(void *context, const char *name);
} AoeGameSaveOpsRor;

/* 0x00501250: save the running game under `name`. Outside game states
   4, 6, 7 and 5 nothing happens (0). Otherwise the wait cursor
   (0x0041bd60), status 0x453 (0x00500390(0x453, 0, -1)) and the world's
   save (+0xd8) of `name`; 0x00558094 becomes 1 either way. A failed save
   closes the status (0x005003e0), releases the cursor (0x0041bdc0) and
   gives 0. A saved one: `name` copied to a 264-byte buffer (0x108) and
   cut at its first '.' (strchr, 0x00528350) becomes the saved game name
   (0x004ff4b0); a single player game (0x0041b750 == 1) leaves the comm
   pause (0x00419a60(0, 0)); "Game Screen" is shown (0x00451be0(name, 0)),
   "Save Game Screen" and "Status Screen" removed (0x00451df0), the
   cursor released (0x0041bdc0); 1. The original copies the name
   without a bound; here a longer name is truncated to 263 characters. */
int32_t aoe_game_save_ror(const AoeGameSaveOpsRor *o, const char *name);

#endif
