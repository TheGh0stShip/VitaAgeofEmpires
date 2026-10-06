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


/* 0x00486200 (+0xb8 actions). A "Cancel Dialog" answer closes it; yes
   (code 0) deletes the selected game's .gam and .gmx in the saved-game
   directory (game info +0xb12), refreshes the list at its line and,
   with no selection left, disables Delete and OK (0x0045f3a0(1)).
   Delete (panel 6) with a selection asks string 0x24ca with the name
   (450 x 100). A double click on the list (panel 2, code 3) acts as OK.
   OK (panel 4, code 1) with lines: "<dir><name>.gmx", or ".gam" when
   that file is missing (0x0052694c), loaded by 0x00502d60; a failure
   shows the OK dialog 0x2501. Cancel (panel 5): back to the "Game
   Screen" (resuming a game the settings paused, 0x00419a60(0, 0)) when
   it exists, else a new "Single Player Menu" (0x0049e040) under the wait
   cursor; the screen closes. Others go to the easy panel's 0x00457e50. */
typedef struct {
    void *context;
    const char *(*dialog_name)(void *context, void *panel);   /* 0x00454170 */
    void (*close_dialog)(void *context, const char *name);
    const char *(*selected_name)(void *context);               /* 0x0046a600 ("" none) */
    int32_t (*line_count)(void *context);                      /* 0x0046a0b0 */
    const char *(*save_dir)(void *context);                    /* game info +0xb12 */
    void (*delete_file)(void *context, const char *path);
    void (*refill_at_line)(void *context);                     /* 0x0046a550 / 0x0046a480 */
    void (*disable_buttons)(void *context);                    /* 0x0045f3a0(1) x2 */
    void (*ask)(void *context, const char *text);              /* yes/no "Cancel Dialog" */
    void (*string)(void *context, int32_t id, char *out, int size);
    int (*file_exists)(void *context, const char *path);
    int (*load_game)(void *context, const char *path);         /* 0x00502d60 */
    void (*ok_dialog)(void *context, int32_t text_id);
    int (*game_screen_exists)(void *context);
    int (*paused_by_settings)(void *context);
    int (*game_flag_a00)(void *context);
    void (*resume)(void *context);
    void (*wait_cursor)(void *context);
    void (*open_single_player_menu)(void *context);
    void (*show_screen)(void *context, const char *name);
    void (*close_self)(void *context);
    int32_t (*base_action)(void *context, void *from, int32_t code);
} AoeLoadGameActionOpsRor;
int32_t aoe_load_game_screen_action_ror(AoeLoadGameScreenRor *l, const AoeLoadGameActionOpsRor *o,
                                        void *from, int32_t code);

#endif
