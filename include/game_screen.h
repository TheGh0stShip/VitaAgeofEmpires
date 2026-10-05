#ifndef VITA_AOE_GAME_SCREEN_H
#define VITA_AOE_GAME_SCREEN_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a game screen (vtable 0x005468cc; methods in
   0x00480620..0x00485b90): the message lines, the age title, the command
   buttons, the timer panels, and the hotkey handlers that act on the
   viewed player. The child panels are opaque handles and their virtual
   calls, the game (0x00580e38), the viewed player (0x00419b70) and its
   objects are ops. Semantic storage; offsets noted. */

enum { AOE_GAME_SCREEN_BUTTONS = 12, AOE_GAME_SCREEN_TIMERS = 10 };

/* What the screen reads of an object. */
typedef struct {
    int32_t id;           /* +0x04 */
    int16_t category;     /* template +0x14 */
    uint8_t type;         /* template +0x04 */
    float x, y;           /* +0x38, +0x3c */
    int32_t field_20;     /* +0x20 */
} AoeGameScreenObjectRor;

/* A command button's setting (0x00483760). */
typedef struct {
    void *picture;        /* arg 1 */
    int16_t index;        /* arg 2: the button */
    int32_t frame;        /* arg 3 */
    int32_t command;      /* arg 4: the state's ID */
    int32_t value;        /* arg 5: the state's value */
    int32_t help1, help2; /* args 6, 7 */
    int32_t hotkey_id;    /* arg 8: a string whose first character is the key */
    int32_t field_2c8;    /* arg 9 */
    const char *line2;    /* arg 10 */
    const char *help;     /* arg 11 (NULL: the formatted name) */
    int32_t disabled;     /* arg 12 */
} AoeGameScreenButtonRor;

typedef struct {
    void *context;
    /* Panel virtual calls: +0x14 active, +0x20 redraw, +0xe8 text from a
       string ID, +0xec text; the +0x6c active flag; 0x0046a600 the text;
       0x004542c0 help IDs. */
    void (*set_active)(void *context, void *panel, int on);
    void (*redraw)(void *context, void *panel, int all);
    void (*set_text_id)(void *context, void *panel, int32_t id);
    void (*set_text)(void *context, void *panel, const char *text);
    int (*active)(void *context, void *panel);
    const char *(*text)(void *context, void *panel);
    void (*help)(void *context, void *panel, int32_t id1, int32_t id2);
    /* Message panels: 0x00465a90 show (style, palette colour), 0x00465d10
       hide. */
    void (*message)(void *context, void *panel, int32_t style, const char *text,
                    uint8_t color);
    void (*message_hide)(void *context, void *panel);
    /* Command buttons: +0x2b0 disabled (0x0045f3a0 sets it), the state's
       ID 0x0045f410 and value 0x0045f420, 0x00460690 drop a press,
       0x004f7bc0(0), 0x0045ef60 picture, 0x0045ef40 ID and value,
       0x0045f0a0 lines, 0x004f7510 help line; the +0x3d0 used flag, +0x2c8,
       +0x2cc and the hotkey +0x298 / +0x29c are set directly. */
    int32_t (*button_disabled)(void *context, void *button);
    void (*button_set_disabled)(void *context, void *button, int32_t disabled);
    int32_t (*button_id)(void *context, void *button);
    int32_t (*button_value)(void *context, void *button);
    void (*button_drop_press)(void *context, void *button);
    void (*button_reset)(void *context, void *button);
    void (*button_picture)(void *context, void *button, void *picture, int32_t frame);
    void (*button_state)(void *context, void *button, int32_t id, int32_t value);
    void (*button_lines)(void *context, void *button, const char *line1, const char *line2);
    void (*button_help_line)(void *context, void *button, const char *text);
    int32_t (*button_used)(void *context, void *button);
    void (*button_set_used)(void *context, void *button, int32_t used);
    void (*button_fields)(void *context, void *button, int32_t field_2c8, int32_t field_2cc,
                          int32_t hotkey);
    /* Timer panel 0x004faf80. */
    void (*timer_set)(void *context, void *timer, int32_t mode, int32_t index);
    /* The game: 0x004199c0 waiting on the comm, +0x10 an action, +0x28 a
       formatted string (b unused for kind 0x65), 0x004542a0 a string,
       0x00419c80 the software cursor, +0x3fc the unit to place, +0x8e8
       the alternate bar flag, +0x1b0 its state. */
    int32_t (*comm_waiting)(void *context);
    void (*action)(void *context, int32_t action, int32_t arg);
    void (*format)(void *context, int32_t kind, int32_t a, int32_t b, char *out, int32_t n);
    const char *(*string)(void *context, int32_t id);
    void (*software_cursor)(void *context, int on);
    void (*set_place_unit)(void *context, int16_t id);
    int32_t (*alt_bar)(void *context);
    void (*set_alt_bar)(void *context, int32_t on);
    /* 0x0041c9f0 a chat line; 0x0041c100 a player's wonder years. */
    const char *(*chat_line)(void *context, int32_t i);
    void (*wonder_years)(void *context, int32_t player, int32_t *years);
    /* The world (+0x5b4): its mode (+0x108), player count (+0x3c), each
       player's alive byte (+0x108), its two victory timers (+0x848,
       +0x844; -1.0f when off) and stance (0x0045da60; 0 allied). */
    int32_t (*world_mode)(void *context);
    int16_t (*player_count)(void *context);
    int (*player_alive)(void *context, int32_t player);
    float (*player_timer)(void *context, int32_t player, int which);
    int (*stance)(void *context, int32_t player, int32_t other);
    /* The viewed player: its age (attribute 6), cost check 0x004f15f0
       (template, failing resource, 1.0, 0), template flag +0x16c, the
       selection (+0x138[25], count +0x1a0, current +0x134; objects by
       slot, NULL for empty, `set_current`), groups (0x0045de00 assign,
       0x0045df40 select, 0x0045dcb0 clear the selection, 0x0045e270 clear
       the groups, 0x0045e250 the +0x124 block), the view (0x0045d330
       position, 0x0045d310 tile), 0x0045e0c0 the selected IDs, its
       vtable +0xac / +0xbc / +0xc0, and 0x004065f0 (an object's action
       list is not empty). */
    float (*age)(void *context);
    int (*can_afford)(void *context, int16_t id, int16_t *failing);
    int (*template_flag)(void *context, int16_t id);
    int16_t (*selected_count)(void *context);
    int32_t (*selected)(void *context, int32_t slot);
    int32_t (*current)(void *context);
    void (*set_current)(void *context, int32_t id);
    int (*object)(void *context, int32_t id, AoeGameScreenObjectRor *out);
    void (*group_assign)(void *context, int32_t group);
    void (*group_select)(void *context, int32_t group);
    void (*clear_selection)(void *context);
    void (*clear_groups)(void *context);
    void (*clear_block)(void *context);
    void (*view_position)(void *context, float x, float y);
    void (*view_tile)(void *context, int32_t x, int32_t y);
    int (*selected_ids)(void *context, int32_t **ids, int16_t *count);
    void (*free_ids)(void *context, int32_t *ids);
    void (*player_call)(void *context, int slot, int32_t arg);
    int (*object_busy)(void *context, int32_t id);
    /* The player's object list (+0x28: head +4, count +8) in order, the
       world's lookup (0x005206d0) and an object's +0x210. */
    int32_t (*list_count)(void *context);
    int32_t (*list_id)(void *context, int32_t i);
    int (*world_has)(void *context, int32_t id);
    int (*object_ready)(void *context, int32_t id);
    /* The base screen: 0x004553a0 (help mode, then the panel's). */
    void (*base_mouse)(void *context, int32_t a, int32_t b, int32_t c, int32_t d, int32_t e);
    /* The viewed player's commands: 0x004f1240 delete the selected
       object (0), 0x004f12c0 opcode 0x6d with a value (1). */
    void (*player_command)(void *context, int which, uint32_t value);
    /* The game: its state (+0x1b0), paused (0x0041b750), a networked
       game (0x0041b760), the pause toggle (0x00419a40), a chat line to
       everyone (0x004213e0) and the time. */
    int32_t (*game_state)(void *context);
    int (*paused)(void *context);
    int (*networked)(void *context);
    void (*toggle_pause)(void *context);
    void (*send_chat)(void *context, const char *text);
    uint32_t (*now)(void *context);
} AoeGameScreenOpsRor;

typedef struct {
    void *view;                                  /* +0x4b8 */
    void *resources;                             /* +0x4c0 */
    void *buttons[AOE_GAME_SCREEN_BUTTONS];      /* +0x4c8.. */
    void *timers[AOE_GAME_SCREEN_TIMERS];        /* +0x518.. */
    void *lines[6];                              /* +0x544.. */
    void *chat[8];                               /* +0x55c.. */
    void *age_text;                              /* +0x580 */
    void *panel_584;                             /* +0x584 */
    void *title;                                 /* +0x588 */
    int32_t age;                                 /* +0x710 */
    int32_t chat_next;                           /* +0x724 */
    int32_t field_78;                            /* +0x78 */
    int32_t dialog;                              /* +0x7b4 */
    uint32_t pause_time;                         /* +0x7c0 */
    int32_t idle_id;                             /* +0x7c4 */
    int32_t alt_bar_held;                        /* 0x007c0648 */
    const AoeGameScreenOpsRor *ops;
} AoeGameScreenRor;

/* 0x004805b0: the six lines hidden, then the base's 0x004553a0. */
void aoe_game_screen_mouse_ror(AoeGameScreenRor *s, int32_t a, int32_t b, int32_t c, int32_t d,
                               int32_t e);
/* 0x00480640: +0x584 given text 0x2329, shown when inactive, the view
   redrawn; 0x00480690 hides it (when active) and redraws. */
void aoe_game_screen_show_584_ror(AoeGameScreenRor *s);
void aoe_game_screen_hide_584_ror(AoeGameScreenRor *s);
/* 0x004806d0: chat line `i` (copied, 255 + 1 bytes) shown in the next
   chat panel (style 3, colour 0xff); 0x00480760 a given text. The index
   cycles 0..7. */
void aoe_game_screen_chat_line_ror(AoeGameScreenRor *s, int32_t i);
void aoe_game_screen_chat_text_ror(AoeGameScreenRor *s, const char *text);
/* 0x00482040: the age title (when there is one): the viewed player's age
   to +0x710, its name (kind 0x65) shown, help 0x1069..0x106c for ages
   1..4. */
void aoe_game_screen_age_ror(AoeGameScreenRor *s);
/* 0x00482140: the title's text, unless it already reads that. */
void aoe_game_screen_title_ror(AoeGameScreenRor *s, const char *text);
/* 0x004821b0: six lines (style 2, colour 0x56); 0x00482260 the first
   line. */
void aoe_game_screen_lines_ror(AoeGameScreenRor *s, const char *const text[6]);
void aoe_game_screen_line_ror(AoeGameScreenRor *s, int32_t style, const char *text,
                              uint8_t color);
/* 0x004834a0: unused buttons (+0x3d0 clear) hidden and reset (state
   0, -1, help -1); the rest shown. */
void aoe_game_screen_hide_unused_ror(AoeGameScreenRor *s);
/* 0x00483710: a 1-based slot to its button position. */
uint32_t aoe_game_screen_button_slot_ror(uint8_t slot);
/* 0x00483760: set a button (used, disabled, and unless the ID and value
   are as they are: name (kind 0x66), picture, state, lines, help IDs,
   help line, +0x2cc (2 for command 0x11) and the hotkey), then show it. */
void aoe_game_screen_set_button_ror(AoeGameScreenRor *s, const AoeGameScreenButtonRor *b);
/* 0x004839a0: the timer panels from the world: mode 2 one panel (3, 0);
   mode 0 up to ten: each player's wonder (4), and the two victory timers
   (5, 6) unless an earlier living player is mutually allied; the rest
   (0, 0). */
void aoe_game_screen_timers_ror(AoeGameScreenRor *s);
/* 0x00483bc0: the score comparator (+0x10, +0x8, +0xc, +0x4 descending). */
int aoe_game_screen_score_cmp_ror(const int32_t *a, const int32_t *b);

/* Hotkey handlers (nothing while the comm waits). Announcements
   0x00484790 (0xbe2, 0x67), 0x00484b10 (0xbe4, 0x65), 0x00484ef0 (0xbe3,
   0x66), 0x00484f40 (0xbec, 0x68), 0x00485660 (0xbed, 0x69), 0x00485860
   (0xbe5, 100): the string on line 1 (style 1, colour 0x32), then the
   action (arg 0). */
void aoe_game_screen_announce_ror(AoeGameScreenRor *s, int32_t string_id, int32_t action);
/* 0x00484820: train `id`: affordable, the unit to place and action 0x15
   (template flag) or 7; else the shortfall (kind 0x67) on line 1. */
void aoe_game_screen_train_ror(AoeGameScreenRor *s, int16_t id);
/* 0x00484be0: the alternate bar: on (title shown, resources hidden),
   then a second press is swallowed, a third turns it off. */
void aoe_game_screen_alt_bar_ror(AoeGameScreenRor *s);
/* 0x00484e80: group -1 assigned; 0x00484eb0 group n (0 is 10);
   0x00485050 select group n (0 is 10; `clear` first), view redrawn;
   0x00485620 clear the groups; 0x004856b0 clear the selection and the
   block. */
void aoe_game_screen_group_clear_ror(AoeGameScreenRor *s);
void aoe_game_screen_group_assign_ror(AoeGameScreenRor *s, int32_t group);
void aoe_game_screen_group_select_ror(AoeGameScreenRor *s, int32_t group, int clear);
void aoe_game_screen_groups_clear_ror(AoeGameScreenRor *s);
void aoe_game_screen_selection_clear_ror(AoeGameScreenRor *s);
/* 0x004851d0 / 0x00485770: with a selection, announce 0xbe0 / 0xbe1 and
   action 4 / 5. */
void aoe_game_screen_selection_action_ror(AoeGameScreenRor *s, int32_t string_id,
                                          int32_t action);
/* 0x004854f0 / 0x00485800 / 0x00485830: the player's +0xac / +0xbc(arg) /
   +0xc0. */
void aoe_game_screen_player_call_ror(AoeGameScreenRor *s, int slot, int32_t arg);
/* 0x00485700: the view centred on the current unit. */
void aoe_game_screen_center_ror(AoeGameScreenRor *s);
/* 0x004858e0: the current unit becomes the previous (0) or next selected
   one (wrapping; with more than one selected). */
void aoe_game_screen_cycle_ror(AoeGameScreenRor *s, int next);
/* 0x004859b0: a selected unit of type 0x28..0x50 (inclusive) with
   actions. */
int aoe_game_screen_selection_busy_ror(const AoeGameScreenRor *s);
/* 0x00485a10: the fading focus (0x004681c0 through `focus`), then the
   software cursor unless +0x78. */
void aoe_game_screen_focus_ror(AoeGameScreenRor *s, void (*focus)(void *context, int32_t v),
                               int32_t v);
/* 0x00485b90: the next idle unit after `after` (categories 4, 0x15, 0x13,
   2; +0x210 and +0x20 clear), its ID kept at +0x7c4. -1 for none. */
int32_t aoe_game_screen_next_idle_ror(AoeGameScreenRor *s, int32_t after);

/* 0x00484980: delete the selected object; 0x004858b0: opcode 0x6d with
   `value` (both nothing while the comm waits). 0x00485260: the pause
   key, at most once a second unless paused: outside state 5 a networked
   game first tells everyone (string 0xbf4), then the pause toggles. */
void aoe_game_screen_delete_ror(AoeGameScreenRor *s);
void aoe_game_screen_command_6d_ror(AoeGameScreenRor *s, uint32_t value);
void aoe_game_screen_pause_ror(AoeGameScreenRor *s);

#endif
