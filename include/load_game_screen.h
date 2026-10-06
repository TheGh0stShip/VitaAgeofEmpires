#ifndef VITA_AOE_LOAD_GAME_SCREEN_H
#define VITA_AOE_LOAD_GAME_SCREEN_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a "Load Saved Game Screen" (vtable 0x005468cc,
   constructor 0x00485c90): the saved-game list and the teardown. The
   panels, the file search and the list are reached through ops. */

enum { AOE_LOAD_GAME_PANELS = 7 };

typedef struct {
    void *context;
    /* The list (+0x480): clear (thunk of 0x0046a6d0), its sorted flag
       (+0x15c), append a line (0x0046a150(text, 0)). */
    void (*list_clear)(void *context);
    void (*list_set_sorted)(void *context, int32_t on);
    void (*list_append)(void *context, const char *text);
    /* The saved-game directory (game settings +0xb12) and a file search
       (the C runtime's _findfirst / _findnext): calls `each`
       with every name matching `pattern`. */
    const char *(*save_dir)(void *context);
    void (*find_files)(void *context, const char *pattern,
                       void (*each)(void *arg, const char *name), void *arg);
    /* Teardown: a panel released (0x00419950), the base destructor
       (0x00468170), the object freed (0x00525fc2). */
    void (*release_panel)(void *context, void *panel);
    void (*base_destroy)(void *context);
    void (*free_self)(void *context);
} AoeLoadGameScreenOpsRor;

typedef struct {
    void *panels[AOE_LOAD_GAME_PANELS]; /* +0x478..+0x490 */
    const AoeLoadGameScreenOpsRor *ops;
} AoeLoadGameScreenRor;

/* 0x004860a0: the list cleared and sorted, then every "<dir>*.gam" and
   then "<dir>*.gmx" file added without its last four characters (the
   extension). */
void aoe_load_game_screen_fill_ror(AoeLoadGameScreenRor *l);
/* 0x00485ff0 destructor: the seven panels released in order, then the
   base destructor; 0x00485fd0 the deleting form (`flags` bit 0 frees). */
void aoe_load_game_screen_destroy_ror(AoeLoadGameScreenRor *l, uint8_t flags);

#endif
