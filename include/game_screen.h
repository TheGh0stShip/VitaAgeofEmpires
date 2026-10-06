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

/* A map score line (0x00465ba0 style 5): text, RGB colour and shadow,
   the font (8 or 0xb, 0x004199e0), and two icons (picture, frame). */
typedef struct AoeGameScoreLineRor {
    char text[0x200];
    uint32_t colour, shadow;
    int32_t font;
    void *icon1;
    int32_t frame1;
    void *icon2;
    int32_t frame2;
} AoeGameScoreLineRor;

/* What the screen reads of an object. */
typedef struct {
    int32_t id;           /* +0x04 */
    int16_t category;     /* template +0x14 */
    uint8_t type;         /* template +0x04 */
    float x, y;           /* +0x38, +0x3c */
    int32_t field_20;     /* +0x20 */
    int16_t template_id;  /* template +0x10 */
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
    /* The command buttons rebuilt for the requested page (not yet
       translated); the
       game's +0x1b4 / +0x1b8 (edit mode 6 with sub mode 1 pages by 10). */
    void (*refresh_buttons)(void *context);
    int32_t (*edit_mode)(void *context, int32_t *sub_mode);
    /* The current object is the viewed player's (template owner +0x4a
       against the world's +0x7c); 0x0045dbb0 select an object; 0x0041c120
       a sound. */
    int (*owned_by_viewed)(void *context, int32_t id);
    void (*select_object)(void *context, int32_t id);
    void (*sound)(void *context, int32_t id);
    /* Map orders on (0x0055873c) and the zoom step (in or out). */
    int (*orders_enabled)(void *context);
    void (*zoom)(void *context, int in);
    /* Game speed (0x00484c60): the world's speed (+0x5b4 -> +0x18), the
       game's fast-speed flag (0x0041b7f0, +0x989), the speed command
       0x004e98f0 (opcode 0x67) and the stored speed 0x0041c180 (+0xa0c). */
    float (*game_speed)(void *context);
    int (*fast_speeds)(void *context);
    void (*send_speed)(void *context, float speed);
    void (*set_speed_setting)(void *context, float speed);
    /* Escape (0x00484900): the screen's armed press (+0x45c) and its
       disarm (0x00457d30); the view's +0xf4 and the outline reset
       0x0050ffd0(-1, -1, -1, -1) (edit mode 0x15); the game's +0x10
       mode change. Dialogs: the queued commands run (0x0042a140), the
       comm pause 0x00419a60(1, 1), and a dialog made by name ("Diplomacy
       Dialog", "Menu Dialog" 0x00433c30). */
    int (*escape_armed)(void *context);
    void (*escape_disarm)(void *context);
    void (*view_cancel)(void *context, void *view);
    void (*outline_reset)(void *context);
    void (*set_mode)(void *context, int32_t mode, int32_t sub);
    void (*run_commands)(void *context);
    void (*comm_pause)(void *context, int32_t on, int32_t flag);
    void (*open_dialog)(void *context, const char *name);
    /* The viewed player's next object of a template after `after` (0:
       from the start), 0x004f0ac0; 0 when none. */
    int32_t (*next_of_type)(void *context, int32_t template_id, int32_t after);
    /* The viewed player's current building (+0x134, a type 'P' object):
       train there (0x004f1410 -> 0x004e9490) or research there
       (0x004f1200 -> 0x004e9530); research affordable (0x004f1850, the
       shortfall resource on failure). The instant flag (0x00558740) and
       the world's +0x58 command 0x004e9c20(screen +0x5c8, id, arg). */
    void (*train_at_current)(void *context, int16_t id);
    void (*research_at_current)(void *context, int16_t id);
    int (*can_research)(void *context, int16_t id, int16_t *failing);
    int (*instant)(void *context);
    void (*instant_train)(void *context, int16_t id, int32_t arg);
    /* The screen's sound (+0x7b8) destroyed and freed (0x004a2c60), the
       base children broadcast (0x00454390, +0xd4). */
    void (*free_sound)(void *context, void *sound);
    void (*base_broadcast)(void *context);
    /* Hover help (0x00484620): the game window is the foreground window,
       the view has the focus (0x00453f30), a capture/modal panel is set
       (0x00451ad0 on the panel system), the game's mouse (0x00419b20),
       the view's +0x33c (keeps the help line), the diamond map's tile
       under a point (0x0042df00 on +0x4bc, nonzero over the map), a
       button's hit test (vtable +0xe8) and its help text (+0x2d0). */
    int (*foreground)(void *context);
    int (*view_focus)(void *context);
    int (*modal)(void *context);
    void (*mouse)(void *context, int32_t *x, int32_t *y);
    int (*view_keeps_help)(void *context);
    int (*over_map)(void *context, int32_t x, int32_t y);
    int (*button_hit)(void *context, void *button, int32_t x, int32_t y);
    const char *(*button_help_text)(void *context, void *button);
    /* Event cue (0x004844a0): sound on (the game's +0xc -> +0x8a8, its
       +0x60 sound system, 0x007c052c clear); the world's cue table
       (+0x28: count +0x8d90, 0x198-byte entries: active byte +0x8c,
       priority byte +0xd1, effect +0xb0); the effect's file chosen
       without playing (0x004a24a0); music stop (0x004a2910), play
       (0x004a2870(name, 0, 0)) and the playing flag (0x007c0320). */
    int (*sound_ready)(void *context);
    int16_t (*cue_count)(void *context);
    void (*cue)(void *context, int32_t i, uint8_t *active, uint8_t *priority, void **effect);
    const char *(*cue_choose)(void *context, void *effect);
    void (*music_stop)(void *context);
    void (*music_play)(void *context, const char *name);
    int (*music_playing)(void *context);
    /* Map scores (0x00484f90 / 0x00483c40): the world's +0x11c flag, the
       help panel's placement (vtable +0x18 (8, 4, 0, y, 4, 0, 0, 0x22,
       0x22, 0, 0, 0, 0)), the game's player count (0x0041b9a0) and each
       one's world slot (0x0041c080), a world player's score (+0x34's
       0x00454170), colour (+0x10c -> +0x26) and lost state (+0x80 == 2);
       the comm's player kind (0x00423d10), latency (+0x14d4 -> +0x1bc[p]),
       ping (0x00428e60) and name (0x004241a0); 0x00465ba0 the line. */
    void (*set_map_scores)(void *context, uint8_t on);
    void (*help_place)(void *context, void *panel, int32_t y);
    int32_t (*game_players)(void *context);
    int32_t (*world_slot)(void *context, int32_t player);
    int32_t (*score)(void *context, int32_t slot);
    int16_t (*colour)(void *context, int32_t slot);
    int (*lost)(void *context, int32_t slot);
    int32_t (*player_kind)(void *context, int32_t player);
    uint32_t (*latency)(void *context, int32_t player);
    uint32_t (*ping)(void *context, int32_t player);
    const char *(*player_name)(void *context, int32_t player);
    void (*score_line)(void *context, void *panel, const struct AoeGameScoreLineRor *line);
    /* The destructor 0x0047ea00: 0x00451df0 close a dialog by name, the
       wave sound playing (0x004a3440) and stopped (0x004a3410), all
       sounds stopped (0x004a2510), a shape freed (0x0049f840), a buffer
       freed (0x005260f8), a child panel deleted through its vtable, the
       screen's 0x00468170. */
    void (*close_dialog)(void *context, const char *name);
    int (*sound_playing)(void *context, void *sound);
    void (*sound_stop)(void *context, void *sound);
    void (*stop_all_sounds)(void *context);
    void (*free_shape)(void *context, void *shape);
    void (*free_buffer)(void *context, void *buffer);
    void (*delete_panel)(void *context, void *panel);
    void (*base_release)(void *context);
    /* Idle 0x0047fbe0: the wait cursor released (0x0041bdc0) unless game
       +0x1d8; the panel system's modal panel (0x00451ad0) and its
       rectangle; the base idle 0x00453360; SetPaletteEntries
       (0x00438200(first, count, entries)). */
    int (*game_flag_1d8)(void *context);
    void (*arrow_cursor)(void *context);
    void *(*modal_panel)(void *context);
    int (*panel_rect)(void *context, void *panel, int32_t ltrb[4]);
    void (*base_idle)(void *context);
    void (*set_palette_entries)(void *context, int32_t first, int32_t count,
                                const uint8_t (*rgb)[4]);
    /* Window messages 0x00480440: GetKeyState < 0 for a virtual key; the
       panel's current child (+0x3c); the close question (0x00457a30
       string 3101 "CloseProgramDialog" 450 x 100); the view history's
       step back (0x00506550); the panel's 0x00452fa0. */
    int (*key_down)(void *context, int32_t vk);
    void *(*current_child)(void *context);
    void (*ask_close)(void *context);
    void (*view_back)(void *context);
    int32_t (*base_message)(void *context, uint32_t msg, uint32_t wparam, uint32_t lparam);
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
    uint32_t speed_time;                         /* +0x7bc */
    void *sound;                                 /* +0x7b8 */
    int32_t idle_id;                             /* +0x7c4 */
    int16_t button_pages;                        /* +0x5e0 */
    int16_t button_page;                         /* +0x5e2 requested */
    int16_t shown_page;                          /* +0x5e4 */
    int32_t alt_bar_held;                        /* 0x007c0648 */
    void *more_buttons[5];                       /* +0x4f8.. (with buttons: 17) */
    void *help_panel;                            /* +0x50c */
    void *scores[8];                             /* +0x58c.. */
    void *ping_picture;                          /* +0x448 */
    int32_t width, height;                       /* +0x14, +0x18 */
    int32_t mouse_x, mouse_y;                    /* +0x708, +0x70c */
    uint32_t cue_interval;                       /* +0x678 */
    uint32_t cue_time;                           /* +0x67c */
    int32_t cue_playing;                         /* +0x684 */
    uint32_t cue_start;                          /* +0x688 */
    /* The rest of the destructor's inventory: shapes +0x478..+0x4a8
       (8 then 5), the diamond map +0x4bc, panels +0x4c4, +0x510, +0x514,
       +0x540, +0x57c, buffers +0x5d0..+0x5dc, the cue sound flag +0x680.
       The palette cycle 0x004843d0: its period (+0x690), last time
       (+0x694), start index (+0x698) and seven RGB triples (+0x69e). */
    void *shapes[13];
    void *diamond_map;
    void *panel_4c4, *panel_510, *panel_514, *panel_540, *panel_57c;
    void *buffers[4];
    int32_t cue_sound;
    uint32_t cycle_period, cycle_time;
    int16_t cycle_index;
    uint8_t cycle_colors[7][3];
    void *self; /* the screen panel itself, for the modal comparison */
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

/* 0x00482290: back to the first button page and rebuild; 0x004847e0
   the same after action (6, 1), nothing while the comm waits. */
void aoe_game_screen_first_page_ror(AoeGameScreenRor *s);
void aoe_game_screen_cancel_page_ror(AoeGameScreenRor *s);
/* 0x00485140: the next button page (wrapping to 0); in edit mode 6 / 1
   it flips between 0 and 10. Nothing while the comm waits. */
void aoe_game_screen_next_page_ror(AoeGameScreenRor *s);
/* 0x00485aa0: the go-to-idle key: the search starts after the current
   unit when it is the viewed player's and of an idle category (4, 0x15,
   0x13, 2), else after +0x7c4; the found unit is selected alone and
   centred and the view redrawn, otherwise sound 3. */
void aoe_game_screen_goto_idle_ror(AoeGameScreenRor *s);
/* 0x00480f90 (the character handler in the table at 0x00546850): '+' or '=' zooms in, '-' out, when map
   orders are on. Always 0. */
int aoe_game_screen_zoom_key_ror(AoeGameScreenRor *s, int32_t key);
/* 0x00484c60 (the step the '+'/'-' key's zoom op reaches): the game speed
   one step faster (in != 0) or slower. Rate-limited to once a second
   (unless 0x0041b750, the op 'paused'), nothing in game state 5. Slower:
   16->8->4->2->1.5->1. Faster: 1->1.5->2, then only with 0x0041b750 and
   the fast-speeds flag 2->4->8->16. Other speeds: nothing. A networked
   game tells everyone (string 0xbf3); the speed command is sent and, when
   0x0041b750, also stored. */
void aoe_game_screen_speed_step_ror(AoeGameScreenRor *s, int in);
/* 0x00484900: Escape. An armed press is disarmed; otherwise, unless the
   comm waits, an edit mode is left (mode 0x15 first cancels the view's
   placement and resets the outline; then mode (0, 0)), and without one
   the selection is cleared (0x004856b0). */
void aoe_game_screen_escape_ror(AoeGameScreenRor *s);
/* 0x00484b60: unless the comm waits, the "Diplomacy Dialog" (a single
   player game, 0x0041b750 == 1, pauses first: 0x00419a60(1, 1)). */
void aoe_game_screen_diplomacy_ror(AoeGameScreenRor *s);
/* 0x004850b0: the "Menu Dialog": in game state 6 the queued commands
   run first; a single player game pauses as above. */
void aoe_game_screen_menu_ror(AoeGameScreenRor *s);
/* 0x00485390: the building keys 0..7 (templates 0x6d, 0x0c, 0x2d, 0x57,
   0x31, 0x65, 0x68, 0): unless the comm waits, the next such object
   after the current one (when it is of that template, else from the
   start) becomes the only selection, the view centred on it and
   redrawn. Other keys and no object: nothing. */
void aoe_game_screen_building_key_ror(AoeGameScreenRor *s, int32_t key);
/* 0x00485520: a train button: with the instant flag the world's command
   (+0x5c8, id, arg); else affordable (0x004f15f0): the unit to place
   (+0x3fc) and trained at the current building; else the shortfall (kind
   0x67) on line 1 in colour 0x56. Nothing while the comm waits. */
void aoe_game_screen_train_button_ror(AoeGameScreenRor *s, int16_t id, int32_t arg);
/* 0x004852f0: a research button: researched at the current building
   when affordable, else the shortfall as above. */
void aoe_game_screen_research_button_ror(AoeGameScreenRor *s, int16_t id);
/* 0x00484a70: the quick chat: in game state 5 with no dialog (+0x7b4)
   string 0xbfe on line 1 (colour 0x32), else the "Send Quick Message
   Dialog". 0x004849b0: the same in a single player game (0x0041b750),
   otherwise the same check, then the "Send Message Dialog". */
void aoe_game_screen_quick_chat_ror(AoeGameScreenRor *s);
void aoe_game_screen_chat_ror(AoeGameScreenRor *s);
/* 0x00485a50 (+0xd4): the screen's sound freed and cleared, then the
   base broadcast. */
void aoe_game_screen_broadcast_ror(AoeGameScreenRor *s);
/* 0x004844a0: the event cue. With sound on: when idle, every
   cue_interval ms the active cue (with an effect) of the highest priority
   (above 0, first wins) has its file played as music (stopping any) and
   the cue is playing; while playing, once a second: music stopped by
   itself ends it, and after 6 s it is stopped. */
void aoe_game_screen_cue_ror(AoeGameScreenRor *s);
/* 0x00484620: hover help. With the game window in front, the view
   focused and no modal panel, a moved mouse is kept; the help line is
   hidden unless the view keeps it; inside the screen (x < width, y <=
   height) and off the diamond map, the first of the 17 buttons that is
   active, has a state and is hit shows its help (style 0, colour 0xff). */
void aoe_game_screen_hover_ror(AoeGameScreenRor *s);
/* 0x00484f90: the map scores on or off: the world's flag, the eight
   score panels shown or hidden, refreshed when on (0x00483c40), the help
   line placed at y 0xb8 (on) or 4, the view redrawn. */
void aoe_game_screen_map_scores_ror(AoeGameScreenRor *s, int on);
/* 0x00483c40: the score lines. Each game player with a world slot gets
   a team (shared with mutually allied living players when alive) and
   its team's average score; sorted (0x00483bc0); in a networked game the
   human with the lowest rating (1000/latency, clamped 1..100, below 11:
   the slowest link) is marked (icon 2 frame 0x11) and humans pinging 300+ get
   icon 1 frame 0x13 (0x14 above 1000). Lines "%s: %d/%d" (name, score,
   average) in the player's colour, font 8 for lost players and networked
   non-humans (kinds other than 2 and 4), else 0xb; empty entries hide
   their panel; stops at the first missing panel. */
void aoe_game_screen_scores_ror(AoeGameScreenRor *s);


/* 0x0047ea00 destructor (deleting 0x0047e9c0): the "Config Dialog"
   closed; the sound (+0x7b8) stopped if playing, then destroyed; the
   cue sound flag cleared (stopping all sounds when one plays); the 13
   shapes freed; the 4 buffers freed; then the panels deleted in the
   original order: chat[8], buttons[12] and more_buttons[5], age text,
   title, scores[8], resources, +0x4c4, +0x510, help, +0x514, +0x540,
   timers[10], lines[6] (last first), +0x57c, +0x584, the view, the
   diamond map; then the screen's 0x00468170. */
void aoe_game_screen_release_ror(AoeGameScreenRor *s);
/* 0x0047eed0 (+0x24; 0x0047eec0 at +0x20 only forwards to the panel's
   0x00452bb0): when the system's modal panel overlaps this screen's
   rectangle `self_ltrb` (either rectangle's edge inside the other on
   both axes), that panel is redrawn with `flag`. */
void aoe_game_screen_redraw_overlaps_ror(AoeGameScreenRor *s, const int32_t self_ltrb[4],
                                         int32_t flag);
/* 0x0047fbe0 idle (+0x4c): the wait cursor released unless game +0x1d8;
   in states 4, 5 and 6 with this screen modal and the view focused the
   palette cycles; in states 4 and 6 the event cue and hover help run;
   then the base idle. */
void aoe_game_screen_idle_ror(AoeGameScreenRor *s);
/* 0x004843d0: once per period, palette entries 0xf8..0xfe take the
   seven colours starting at the index (wrapping at 7), the index steps
   back one (6 after 0). */
void aoe_game_screen_palette_cycle_ror(AoeGameScreenRor *s, uint32_t now);
/* 0x00480440 (+0x48 window messages): WM_SYSKEYDOWN (0x104) of 'Y' with
   +0x78 set while the view is the current child opens the menu dialog
   (1); WM_KEYUP (0x101) of F3 (0x72) or Pause (0x13) with +0x78 set, the
   view focused and no Alt / Ctrl / Shift pauses; WM_CLOSE (0x10) once
   while no quit is pending sets the request and, in game states 4..7
   with the window not iconic, asks (1); WM_MBUTTONUP (0x208) steps the
   view history back; everything else (and the rest) goes to the panel's
   handler. `quit_requested` is the 0x007c0338 flag; `iconic` the
   window's. */
int32_t aoe_game_screen_message_ror(AoeGameScreenRor *s, uint32_t msg, uint32_t wparam,
                                    uint32_t lparam, int32_t *quit_requested, int iconic);


/* 0x0047fa50 (+0x54 paint): nothing without an area, invisible, inactive,
   with the application inactive (game +0x18) or the window iconic. A
   pending redraw (+0x38) is "full" unless it is below 2, the view lacks
   the focus (0x00453f30) and the current child is neither the "Send
   Quick Message Dialog" nor the "Help Dialog"; a full redraw passes the
   level to every child (+0x20), to the parent's overlap redraw (+0x24,
   when its +0x7c is set) and draws the screen (+0x30); the level is
   cleared and +0x84 set. Then every child (only the current one when
   not full) paints (+0x54); the time taken goes to statistic 0x11
   (0x00417370) and +0x84 clears. */
typedef struct {
    void *context;
    int (*can_paint)(void *context);
    uint32_t (*now)(void *context);
    int32_t (*redraw_level)(void *context);
    void (*set_redraw_level)(void *context, int32_t level);
    int (*view_focus)(void *context);
    const char *(*current_name)(void *context);
    int32_t (*child_count)(void *context);
    void *(*child)(void *context, int32_t i);
    int (*is_current)(void *context, void *child);
    void (*child_set_redraw)(void *context, void *child, int32_t level);
    void (*child_paint)(void *context, void *child);
    int (*parent_wants_overlaps)(void *context);
    void (*parent_redraw_overlaps)(void *context, int32_t level);
    void (*draw_self)(void *context);
    void (*set_painting)(void *context, int on);
    void (*statistic)(void *context, int32_t slot, uint32_t elapsed);
} AoeGameScreenPaintOpsRor;
int32_t aoe_game_screen_paint_ror(const AoeGameScreenPaintOpsRor *o);

#endif
