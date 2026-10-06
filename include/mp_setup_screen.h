#ifndef VITA_AOE_MP_SETUP_SCREEN_H
#define VITA_AOE_MP_SETUP_SCREEN_H

#include <stdint.h>

#include "game_settings.h"

/* Patched Rise of Rome 1.0a "MP Setup Screen" (vtable 0x00546d7c;
   0x00487f40 onwards): the state of the multiplayer setup screen
   and the methods that do not build panels. The screen's panels and the
   game, comm and panel system are reached through ops. Semantic storage;
   offsets noted. */

enum { AOE_MP_SETUP_PLAYERS = 8 };

typedef struct {
    void *context;
    /* The game's settings flags: 0x0041b760 multiplayer (+0x97e),
       0x0041b750 (+0x97d), 0x0041b700 (+0x8fc), read from `settings`. */
    /* The comm: 0x00423c20 this machine hosts; the game's +0x48 slot. */
    int (*host)(void *context);
    void (*game_slot_48)(void *context);
    /* srand(timeGetTime()) and rand (0x00526053 / 0x0052605d). */
    void (*seed_now)(void *context);
    int32_t (*rand)(void *context);
    /* A player's name (0x004241a0), a string (0x004542a0), the yes/no
       dialog (0x00457a80: text, name, 450 x 100) and an OK message on the
       current screen (0x00451ab0 then 0x004578d0, 450 x 100). */
    const char *(*player_name)(void *context, int32_t player);
    const char *(*string)(void *context, int32_t id);
    void (*yes_no)(void *context, const char *text, const char *name, int32_t w, int32_t h);
    void (*ok_message)(void *context, int32_t id, int32_t w, int32_t h);
    /* The screen's settings refresh (not yet translated). */
    void (*refresh)(void *context, int32_t arg);
    /* The chat panel (+0x618): line count (0x0046a0b0), last visible
       line (+0xfe), drop line 0 (0x0046a480), add wrapped text
       (0x00468ba0), scroll to the bottom (0x00469500(7, 0, 1)); the chat
       line i (0x0041c9f0). */
    int16_t (*chat_count)(void *context);
    int16_t (*chat_bottom)(void *context);
    void (*chat_remove_first)(void *context);
    void (*chat_add)(void *context, const char *text);
    void (*chat_scroll_bottom)(void *context);
    const char *(*chat_line)(void *context, int32_t i);
    /* Idle: the window minimised (IsIconic), the game's +0x1d8 and the
       capture release (0x0041bdc0), the host shares its player data
       (0x00420640(0)), the time, the base idle (0x00453360). */
    int (*minimised)(void *context);
    int (*capture_released)(void *context);
    void (*release_capture)(void *context);
    void (*share_player_data)(void *context);
    uint32_t (*now)(void *context);
    void (*base_idle)(void *context);
    /* The player-count dropdown (+0x6e0): clear (0x00461f10), append
       (0x00461da0), index of an ID (0x00461e70), select (0x00461d50). */
    void (*count_clear)(void *context);
    void (*count_append)(void *context, const char *text, int32_t id);
    int32_t (*count_index)(void *context, int32_t id);
    void (*count_select)(void *context, int32_t index);
    /* The colour buttons (+0x570 + 4 i, eight each): state text
       (0x0045ef90), state ID and value (0x0045ef40), state colour
       (0x0045f210), state count (0x0045ef20); a scenario slot is in use
       (0x00476250) and its kind (0x00476210). */
    void (*color_text)(void *context, int32_t button, int32_t state, const char *text);
    void (*color_state)(void *context, int32_t button, int32_t state, int32_t id);
    void (*color_rgb)(void *context, int32_t button, int32_t state, uint32_t rgb);
    void (*color_count)(void *context, int32_t button, int32_t count);
    int (*scenario_slot_used)(void *context, int32_t slot);
    int32_t (*scenario_slot_kind)(void *context, int32_t slot);
    /* A scenario slot's civilisation (0x00476230); a comm slot's state
       (0x00423d40). */
    int32_t (*scenario_slot_civ)(void *context, int32_t slot);
    void (*comm_slot)(void *context, int32_t slot, int32_t state);
    /* Teardown: the loaded scenario deleted (its +0x04 slot with 1), a
       screen closed by name (0x00451df0), the panels released
       (0x00419950 each) and the base destructor (0x00468170). */
    void (*free_scenario)(void *context);
    void (*close_named)(void *context, const char *name);
    void (*release_panels)(void *context);
    /* A comm slot's player type (0x00423d10; 2 and 4 are humans); the
       index of ID i in the +0x5f0 list (0x00461e70, -1 when absent). */
    int32_t (*slot_kind)(void *context, int32_t slot);
    int32_t (*position_index)(void *context, int32_t id);
    /* The tab order (vtable +0xcc) and the focused child (0x00453eb0). */
    void (*tab_order)(void *context, void *const *panels, int32_t n);
    void (*focus)(void *context, void *panel);
    /* Leaving: the lobby launched us (0x004241e0, comm +0x14), the wait
       cursor (0x0041bd60), the comm dropped with a reason (0x004237c0),
       a screen made by name (nonzero when made; the "Main Error Screen"
       with text `arg` 0x96a via 0x00486fd0 / 0x004871b0; others ignore
       it), shown (0x00451be0(name, 0)) and the program closed
       (0x00419930, WM_CLOSE). */
    int (*lobby_launched)(void *context);
    void (*wait_cursor)(void *context);
    void (*drop)(void *context, int32_t reason);
    int (*make_screen)(void *context, const char *name, int32_t arg);
    void (*show_screen)(void *context, const char *name);
    void (*quit)(void *context);
    /* The host information (0x0041d2a0 made, 0x0041d390 released and
       freed): its address count (+0x218), host name (0x0041d3e0),
       address i (0x0041d4a0) and alias i (0x0041d4d0), each NULL when
       absent; an OK dialog with a text (0x00457920: text, name, w, h). */
    void *(*host_open)(void *context);
    void (*host_close)(void *context, void *host);
    uint32_t (*host_count)(void *context, void *host);
    const char *(*host_name)(void *context, void *host);
    const char *(*host_address)(void *context, void *host, uint32_t i);
    const char *(*host_alias)(void *context, void *host, uint32_t i);
    void (*ok_text)(void *context, const char *text, const char *name, int32_t w, int32_t h);
    /* Starting: every playing human ready (0x00425440), our comm slot
       (0x00423c10), the +0x5f0 position list's item ID (0x00461ef0) and
       count (its +0xfc's 0x0046a0b0), the CD check (0x00419530, 0 asks
       for the flag bit), the session launch (comm 0x00424c80, then
       0x00451cc0 and 0x004244f0 on 0x00580d7c) and the scenario game
       started (0x00502910(0), nonzero on success). */
    int (*all_ready)(void *context);
    int32_t (*self_slot)(void *context);
    int32_t (*position_id)(void *context, int32_t index);
    int32_t (*position_count)(void *context);
    int32_t (*cd_check)(void *context, int32_t players);
    void (*launch_session)(void *context);
    int (*start_game)(void *context);
} AoeMpSetupScreenOpsRor;

typedef struct {
    int32_t kick_player;                     /* +0x724 */
    int32_t max_list;                        /* +0x728 */
    int32_t field_72c;                       /* +0x72c */
    int32_t field_730;                       /* +0x730 */
    int32_t field_734, field_738, field_73c; /* +0x734.. */
    uint8_t field_740;                       /* +0x740 */
    int32_t settings_flag;                   /* +0x840 (0x0041b710) */
    int32_t field_844;                       /* +0x844 */
    int32_t scenario;                        /* +0x848 a scenario is loaded */
    int32_t field_84c;                       /* +0x84c */
    int32_t max_players;                     /* +0x850 */
    int32_t field_854, field_858, field_85c; /* +0x854.. */
    uint8_t colors[AOE_MP_SETUP_PLAYERS];    /* +0x860 */
    int32_t field_868[8];                    /* +0x868 */
    uint32_t share_time;                     /* +0x888 */
    int32_t field_88c;                       /* +0x88c */
    int32_t chat_from, chat_to;              /* +0x890, +0x894 */
    void *panel_610, *panel_620;             /* +0x610, +0x620 */
    void *panel_700, *panel_70c, *panel_714; /* +0x700, +0x70c, +0x714 */
    AoeGameSettingsRor *settings;
    const AoeMpSetupScreenOpsRor *ops;
} AoeMpSetupScreenRor;

/* 0x00488c40: the fields cleared (panels are the caller's) and the
   settings reset. Single player: map size 3 and, unless +0x8fc, a random
   map type (rand % 10: 0..1 -> 4, 2 -> 3, 3 -> 2, 4 -> 1, 5..8 kept, 9 ->
   0); then +0xa8c / +0xa90 0, +0x988 4, +0x9a0 / +0x9a4 0, +0xa84 /
   +0xa88 1, +0x985 0, +0x986 1, +0x982 1, options 0, 0, 1, 0, resources
   0, age 0, +0xae4 0, +0x993 from +0xa10, +0x1190 0. Multiplayer: no
   scenario, the defaults with map size 3, type 3, +0x988 8, +0x982 0 and
   +0xae5 0; the random seed, then each player: a random civilisation
   1..16, team slot i, flag and version 0, colour i + 1, +0x994 1 and
   +0xacd 0; a host makes player 0 version 2 and calls the game's +0x48. */
void aoe_mp_setup_screen_reset_ror(AoeMpSetupScreenRor *m);
/* 0x00488c40's map type pick from a random number. */
int32_t aoe_mp_setup_random_map_type_ror(int32_t r);
/* 0x0048b630 (+0x68, shared with the MP startup screen's slot): 1. */
int aoe_mp_setup_screen_slot_68_ror(void);
/* 0x0048ce30: kick `player`: remembered, then the "Kick Dialog" yes/no
   with string 0x25c8 formatted with its name. */
void aoe_mp_setup_screen_kick_ror(AoeMpSetupScreenRor *m, int32_t player);
/* 0x0048ce90: the refresh (0), then string 0x25c9 (`ready` set) or
   0x25ca in an OK message. */
void aoe_mp_setup_screen_ready_message_ror(AoeMpSetupScreenRor *m, int ready);
/* 0x0048bf80: chat line i added (when +0x97d is clear): a list over 99
   lines drops its first, and a list scrolled to its last line follows
   the new one. */
void aoe_mp_setup_screen_chat_ror(AoeMpSetupScreenRor *m, int32_t i);
/* 0x004894e0 (+0x4c idle): pending chat lines (+0x890..+0x894) added
   unless minimised; the capture released when +0x1d8 is clear; in a
   multiplayer game the host data is shared whenever the time moved
   (first call only stamps it); then the base idle. */
void aoe_mp_setup_screen_idle_ror(AoeMpSetupScreenRor *m);
/* 0x0048d1d0: the player-count dropdown: "2".."max_list" with IDs, the
   current +0x988 selected. */
void aoe_mp_setup_screen_fill_counts_ror(AoeMpSetupScreenRor *m);
/* 0x0048d250: each of the eight colour buttons gets one state per
   usable colour (all without a scenario; with one, used slots, and only
   kind 1 ones when +0x97d): text and ID i + 1 and its colour. */
void aoe_mp_setup_screen_fill_colors_ror(AoeMpSetupScreenRor *m);
/* 0x0048cfb0: the players from the settings. Without +0x8fc each gets
   a random civilisation 1..16 and colour i + 1. With it, each used
   scenario slot (0..8) in turn takes the next player (team slot, its
   civilisation, colour slot + 1); +0x988 becomes the dropdown maximum;
   the first kind 1 slot moves to player 0 (a later player already on
   that slot gets player 0's values). Then every +0x994 is 1, comm slot
   1 is 2, 2..8 are 4, 9 is 0, and the counts are refilled. */
void aoe_mp_setup_screen_players_ror(AoeMpSetupScreenRor *m);
/* 0x004891a0 destructor (deleting 0x00488c20): the scenario deleted,
   "Game Settings Screen", "Select Tribe Screen", "Kick Dialog",
   "OKDialog" and "YesNoDialog" closed, then the panels released. */
void aoe_mp_setup_screen_destroy_ror(AoeMpSetupScreenRor *m);
/* 0x0048a610: random start positions. When the free entries of the
   eight +0x5f0 IDs (present ones) are at least the human slots (comm
   slots 1..+0x988 of type 2 or 4), each human player i takes a random
   free position below +0x728 (rand * n / 0x7fff, clamped; up to 1000
   tries, then the first free one) into +0xaa0[i]. */
void aoe_mp_setup_screen_random_positions_ror(AoeMpSetupScreenRor *m);
/* 0x0048ced0: the tab order. +0x97d: +0x700, +0x70c, +0x620 (focus
   +0x700). Otherwise focus +0x610 with +0x610, +0x700, +0x70c, +0x620
   when +0x620 exists, else +0x610, +0x714, +0x70c. */
void aoe_mp_setup_screen_tab_order_ror(AoeMpSetupScreenRor *m);
/* 0x0048aa10: leaving the screen. +0x97d: the wait cursor, then
   "Select Scenario Screen" (+0x8fc) or "Single Player Menu". A lobby
   launch: with `error` the "Main Error Screen" (text 0x96a) when it is
   made, otherwise the program closes (the screen stays). Else the wait
   cursor, the comm dropped (5) and the "Join Screen". The new screen is
   shown and "MP Setup Screen" closed. */
void aoe_mp_setup_screen_leave_ror(AoeMpSetupScreenRor *m, int error);
/* The colour of player colour index i (0..7) in 0x0048d250. */
uint32_t aoe_mp_setup_color_rgb_ror(int32_t i);
/* 0x0048a780: this machine's addresses. Without any, OK message 0x238d;
   else "\n\n" + string 0x238c formatted with the host name + "\n", and
   per address its text + " " and its alias, each line ending "\n", in an
   "OKDialog" 450 x (18 n + 100) (the original concatenates unbounded
   into 1024 bytes; the port truncates). Nothing when the host
   information cannot be made. */
void aoe_mp_setup_screen_addresses_ror(AoeMpSetupScreenRor *m);
/* 0x00489ef0: the start button. Networked: only the host, all ready
   (else 0x25c1); with 0x0041b700 every other human's +0x868 entry must
   match +0x73c (else 0x25e1 with the name), every other human's version
   must be 2 (0x25e2); 0x0041b700 needs a scenario (0x25c2). Networked
   with 0x0041b700: humans (kinds 2, 4) get the position list's IDs in
   turn (wrapping). Networked: at least two teams (distinct +0xac4 of
   kind 2, each kind 4 one), else 0x25c6; 0x0041b700 limits them to
   +0x728 (0x25c3) and with +0x84c exactly (0x25c5); our flag bit from
   the CD check, then the CD check with the kind-2 count (0x7d9). Then:
   random positions (0x0048a610) with 0x0041b700 and option +0xad8; the
   comm slots above the count closed (0x0041b750); civilisations and
   teams over 0x31 lose 50; the humans' +0xac4 / +0xa94 / +0x994
   compacted to the front (defaults i + 1, 1, 1); the "Status Screen"
   closed; networked: the game's +0x48 when anything changed or +0x88c,
   the session launched; the game started (1) or message 0x961 (0).
   A team ID of 0 lands in the original's changed flag (local [0]); kept. */
int aoe_mp_setup_screen_start_ror(AoeMpSetupScreenRor *m);

#endif
