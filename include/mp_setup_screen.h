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
    /* 0x004895b0's action handler. A panel is active (+0x6c); a button's
       state (0x0045f400), value (0x0045f410), set state (0x0045f3a0) and
       vtable +0xe4; a list's item ID (0x00461ed0), selected index
       (0x00461e50), index of an ID (0x00461e70) and select (0x00461d50);
       an input's text (0x00462560, NULL when none) and set text
       (0x004623e0). */
    int (*panel_active)(void *context, void *panel);
    int32_t (*button_state)(void *context, void *panel);
    int32_t (*button_value)(void *context, void *panel);
    void (*button_set_state)(void *context, void *panel, int32_t state);
    void (*button_slot_e4)(void *context, void *panel, int32_t v);
    int32_t (*list_id)(void *context, void *panel);
    int32_t (*list_selected)(void *context, void *panel);
    int32_t (*list_index)(void *context, void *panel, int32_t id);
    void (*list_select)(void *context, void *panel, int32_t index);
    const char *(*input_text)(void *context, void *panel);
    void (*input_set)(void *context, void *panel, const char *text);
    /* The debug log (0x0042c330); player 0's version (0x0041b820); the
       comm ready message (0x004251c0, eight values); the scenario file's
       checksum (0x0041c040 of "<dir><name>.scn", or ".scx" when no .scn
       exists, 0x0041b740 the name); the ready-state redraw (0x48b640);
       the host shares player data (0x00420640(1)); the chat sent
       (0x004213e0); a screen exists (0x00451ae0); the help (0x00457cd0);
       the screen refresh (0x48c010). */
    void (*log)(void *context, const char *text);
    int32_t (*player_version)(void *context, int32_t player);
    void (*send_ready)(void *context, int32_t ready, int32_t civ, int32_t team, int32_t cd,
                       int32_t color, int32_t flag, int32_t checksum, int32_t version);
    int32_t (*scenario_checksum)(void *context);
    void (*update_ready)(void *context);
    void (*share_player_data_now)(void *context);
    void (*send_chat)(void *context, const char *text);
    int (*screen_exists)(void *context, const char *name);
    void (*help)(void *context);
    void (*settings_refresh)(void *context);
    /* Player settings: colour (0x00505420), the 0x0041bbd0 byte, player
       count (0x0041bb30), the player count (0x0041b9a0), civilisation
       (0x005053c0), position (0x005053f0) and position of a player
       (0x005053e0); the comm kicks (0x004256c0), a comm slot is a human
       here (0x00423da0); the dialog that sent the action, its name
       (0x00454170) and deletion (vtable +0x00 with 1); the base action
       handler (0x00457e50). */
    void (*set_color)(void *context, int32_t player, int32_t color);
    void (*set_byte_bbd0)(void *context, int32_t player, int32_t v);
    void (*set_player_count)(void *context, int32_t n);
    int32_t (*player_count)(void *context);
    void (*set_civ)(void *context, int32_t player, int32_t civ);
    void (*set_position)(void *context, int32_t player, int32_t pos);
    int32_t (*position_of)(void *context, int32_t player);
    void (*kick_send)(void *context, int32_t slot);
    int (*slot_human)(void *context, int32_t slot);
    const char *(*panel_name)(void *context, void *panel);
    void (*delete_panel)(void *context, void *panel);
    int (*base_action)(void *context, void *panel, int32_t action, int32_t a, int32_t b);
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
    void *panel_710, *panel_718, *panel_6e0; /* +0x710, +0x718, +0x6e0 */
    void *panel_858;                         /* +0x858 (addresses button) */
    void *color_buttons[8], *team_buttons[8];          /* +0x570, +0x590 */
    void *kind_lists[8], *civ_lists[8], *pos_lists[8]; /* +0x5b0, +0x5d0, +0x5f0 */
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

/* 0x004895b0 action handler: a panel action. Action 1 on: +0x700 start
   (0x00489ef0, after focusing the chat); +0x714 ready toggle (see the
   source); +0x620 the "Game Settings Screen" (made when absent) shown;
   +0x710 help; +0x718 quit; +0x70c leave (0); +0x858 the addresses;
   colour / +0x590 buttons i set the colour / byte (state + 1), a
   non-host remembers them. Action 0 on: the chat input sends non-empty
   text and clears it; +0x6e0 the player count then the refresh; kind,
   civilisation and position lists of player i. Closing dialogs: "Game
   Settings Screen" (refresh and +0x48 on a non-zero action, then this
   screen shown), "Kick Dialog" (yes kicks +0x724, no reselects 0, then
   the dialog deleted). Anything else goes to the base handler. */
int aoe_mp_setup_screen_action_ror(AoeMpSetupScreenRor *m, void *panel, int32_t action,
                                   int32_t a, int32_t b);


/* ---- 0x0048b640 player rows and 0x0048c010 settings text ---- */

/* The twelve controls of a player row. */
enum {
    AOE_MPS_NAME = 0,      /* +0x490 */
    AOE_MPS_CIV_TEXT,      /* +0x4b0 */
    AOE_MPS_POS_TEXT,      /* +0x4d0 */
    AOE_MPS_KIND_LABEL,    /* +0x4f0 */
    AOE_MPS_VERSION,       /* +0x510 */
    AOE_MPS_COLOR_TEXT,    /* +0x530 */
    AOE_MPS_TEAM_TEXT,     /* +0x550 */
    AOE_MPS_COLOR_BUTTON,  /* +0x570 */
    AOE_MPS_TEAM_BUTTON,   /* +0x590 */
    AOE_MPS_KIND_LIST,     /* +0x5b0 */
    AOE_MPS_CIV_LIST,      /* +0x5d0 */
    AOE_MPS_POS_LIST,      /* +0x5f0 */
    AOE_MPS_CONTROLS
};
typedef struct {
    void *context;
    int host;                                           /* 0x00423c20 */
    int32_t our_slot;                                   /* 0x00423c10 */
    int (*slot_stats)(void *context, int32_t slot);     /* 0x004253b0 */
    int32_t (*slot_type)(void *context, int32_t slot);  /* 0x00423d10 */
    const char *(*comm_name)(void *context, int32_t slot); /* 0x004241a0 */
    int (*kind_shows_unknown)(void *context, int32_t i);   /* kind list +0x120 == 0xff */
    int32_t (*list_selected_id)(void *context, int control, int32_t i);  /* 0x00461ed0 */
    int (*list_open)(void *context, int control, int32_t i);             /* its +0x104 == 2 */
    void (*list_select_value)(void *context, int control, int32_t i, int32_t value); /* 0x00461e70 + 0x00461d50 */
    const char *(*list_selected_text)(void *context, int control, int32_t i);        /* 0x00461eb0 */
    int32_t (*list_first_id)(void *context, int32_t i);                  /* kind list 0x00461ef0(0) */
    void (*list_remove_first)(void *context, int32_t i);                 /* 0x00461e30(0) */
    void (*list_insert_first)(void *context, int32_t i, const char *text, int32_t id); /* 0x00461e00 */
    /* The colour button's state for the colour id (0x0045f310 ..
       0x0045f470): its text and two colours. */
    void (*color_state)(void *context, int32_t i, int32_t color, const char **text, uint32_t *c1, uint32_t *c2);
    void (*team_select)(void *context, int32_t i, int32_t index);        /* team button +0xe4 */
    const char *(*team_text)(void *context, int32_t i);                  /* its state's text */
    void (*set_active)(void *context, int control, int32_t i, int32_t v);
    void (*set_text)(void *context, int control, int32_t i, const char *text);
    void (*set_text_id)(void *context, int control, int32_t i, int32_t id); /* +0xe8 */
    void (*set_colors)(void *context, int control, int32_t i, uint32_t c1, uint32_t c2); /* 0x00469070 */
    void (*name_color)(void *context, int32_t i, uint32_t rgb);          /* 0x00461130 + 0x00469070 */
    void (*enable)(void *context, int control, int32_t i);               /* 0x0045f3a0(0) */
    void (*log)(void *context, const char *text);                        /* 0x0042c330 */
    void (*share)(void *context);                                        /* game +0x48 */
} AoeMpSetupRowsOpsRor;

/* 0x0048b640: the player rows. A networked host first gives every human
   slot (type 2) sharing a colour with a slot whose kind list shows "?"
   (+0x120 0xff) that slot's civilisation and team plus 50 ("SETTING CIV
   TEAM 50" logged), then shares the settings. For each player below the
   count: the kind (comm slot type when networked, else 2 for the first
   and the kind list's selection); not a human slot: kind label and
   version hidden; human: the label follows the player flag, the version
   text shown when hosting or player 0 has a version, for player 0, or
   for our own slot after setting version 2; else hidden. The colour
   (our pending +0x734 when set, else byte 0xac4) picks the button
   state whose text and colours go to the colour text; both hidden, then
   the button shown and enabled for ourselves (networked, +0x85c clear)
   or with +0x97d and more than one player, else the text for a human.
   The team button selects +0x738 - 1 (ours, when set) or byte 0x994 -
   1 (less 50 above 49); the team text takes its text; both hidden, then
   the button for ourselves / a host's computer slot (+0x85c clear) or
   with +0x97d (ours or computer), else the text for kinds 2 / 4.
   Networked: the name colour is red (0xff) for a human slot without
   stats, else green (0xff00). The name: not networked, ours shows string
   0x25b3, others the kind list's text; networked, a leading item of ID 2
   is dropped, a human slot's comm name is inserted first with ID 2, the
   kind is selected and its text shown. A closed civilisation list
   selects +0x72c (ours, when set) or byte 0xa94 (less 50 above 49), a
   closed position list +0x730 (ours, not -1) or int 0xaa0; their texts
   follow. The kind list shows only for a networked host's other slots
   (the name hides then); the civilisation list / text and position list
   / text follow the team rule. Rows from the count to 7 are hidden
   entirely. Then the colour buttons refill (0x0048d250). */
void aoe_mp_setup_screen_rows_ror(AoeMpSetupScreenRor *m, const AoeMpSetupRowsOpsRor *o);

typedef struct {
    void *context;
    const char *scenario_dir;                            /* settings +0xc17 */
    const char *stored_name;                             /* +0x740 */
    void (*store_name)(void *context, const char *name);
    int (*file_exists)(void *context, const char *path); /* 0x0052694c != -1 */
    void (*scenario_delete)(void *context);              /* +0x848 vtable +4(1) */
    int (*scenario_load)(void *context, const char *path); /* 0x00417670(path, 0) */
    int (*slot_active)(void *context, int32_t slot);     /* 0x00476250 */
    int32_t (*slot_kind)(void *context, int32_t slot);   /* 0x00476210 */
    int32_t (*fixed_positions)(void *context);           /* 0x00507e00 */
    int32_t (*scenario_info)(void *context, const char *path); /* 0x0041c040 */
    void (*pos_clear)(void *context, int32_t j);         /* 0x00461f10 on list j */
    void (*pos_append)(void *context, int32_t j, const char *text, int32_t id); /* 0x00461da0 */
    void (*string)(void *context, int32_t id, char *out, int size); /* 0x00454270 / 0x004542a0 */
    void (*header)(void *context, const char *text);     /* +0x624 +0xec */
    void (*line)(void *context, int32_t i, const char *text); /* +0x62c + 4 i: +0xec and shown */
    void (*hide_lines)(void *context);                   /* the 20 lines hidden */
    void (*count_controls)(void *context, int shown);    /* +0x6dc, +0x6e0 */
    void (*rows)(void *context);                         /* 0x0048b640 */
} AoeMpSetupInfoOpsRor;

/* 0x0048c010: the settings text. The scenario is (re)loaded when not in
   scenario mode with +0x840 clear, or in scenario mode with +0x840 set
   or the name changed from +0x740: in scenario mode the old one is
   deleted, "<dir><name>.scn" (or ".scx" when that is missing) loaded
   (0x00417670; a name starting with '.' is skipped) and a failure clears
   +0x8fc; with one, the eight position lists are cleared, +0x84c takes
   0x00507e00, every active slot counts into +0x728 and, networked or
   (with +0x97d) of kind 1, into +0x850 with "%d" entries in all eight
   lists; the first eight active slots become the players' positions,
   colours (slot + 1, also +0x860) and team byte 1. With +0x840's source
   (0x0041b710) the lists instead hold "1".."8" with the players
   1..8 and +0x728 / +0x850 8, +0x84c 0. With +0x97d the players follow
   the settings (0x0048cfb0). +0x840 is refreshed, the name stored (or
   cleared), +0x868 cleared and +0x73c set from the scenario's info
   (0x0041c040; 0 outside scenario mode). Then the 20 lines are hidden
   and refilled as the wait screen does (header 0x25ed with 0x2617 /
   0x25b5 or the scenario name; lines from 1 or 3: map size and type
   with 0x0041b710; players "%s 2 - 8" (0x25b7) in random mode, "%s %d -
   %d" (2, +0x728) without fixed positions and more than two, else "%s
   %d"; victory; age; resources; difficulty; options 0xad8, 0x985,
   0xad9; networked 0x982; path finding; population). With +0x97d the
   count controls show in random mode or with free positions and more
   than two players. Then the rows (0x0048b640). */
void aoe_mp_setup_screen_info_ror(AoeMpSetupScreenRor *m, const AoeMpSetupInfoOpsRor *o);

#endif
