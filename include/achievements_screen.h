#ifndef VITA_AOE_ACHIEVEMENTS_SCREEN_H
#define VITA_AOE_ACHIEVEMENTS_SCREEN_H

#include <stdint.h>

#include "achievements_page.h"
#include "game_ui.h"
#include "timeline_panel.h"

/* The "Achievements Screen" (vtable 0x00545e84, 0x00477340..0x004792c0):
   the post-game / in-game score pages over a 640 x 480 reference layout.
   The page data (titles, headings, cells) is achievements_page.h; this is
   the panel side. */

typedef struct {
    void *context;
    /* The world (game +0x3f4): its player count (+0x3c) and, for player
       i, the name (+0x44), colour (+0x10c +0x26), victory byte (+0x80
       == 1), outcome manager (+0x34) and attributes (+0x50). The viewed
       player's victory byte (0x00419b70). The game time in ms (world
       +0x04). */
    int16_t (*player_count)(void *context);
    const char *(*player_name)(void *context, int32_t player);
    int16_t (*player_color)(void *context, int32_t player);
    int (*player_victorious)(void *context, int32_t player);
    int (*viewed_victorious)(void *context);
    const AoeOutcomeManager *(*outcome)(void *context, int32_t player);
    const float *(*attributes)(void *context, int32_t player);
    uint32_t (*game_time_ms)(void *context);
    /* The scenario's message (0x00476c70; NULL or "" for none); a
       networked game (0x0041b760); the settings pause byte (game +0x97d
       via 0x0041b750; Close resumes only when it is exactly 1) and game
       +0xa00 (a NULL op reads 0); the resume (0x00419a60(0, 0)),
       the return to the menus (0x00500d80), the restart (0x00500fb0) and
       the window close (0x00419930); the game state (+0x1b0); the window
       iconic (IsIconic). */
    const char *(*scenario_message)(void *context);
    int (*networked)(void *context);
    int (*paused_by_settings)(void *context);
    int (*game_flag_a00)(void *context);
    void (*resume_game)(void *context);
    void (*back_to_menu)(void *context);
    void (*restart)(void *context);
    void (*quit)(void *context);
    int32_t (*game_state)(void *context);
    int (*iconic)(void *context);
    /* The check-mark shape (0x0049f5f0(name, id) / 0x0049f840) and a frame
       drawn into the screen's area (0x0049fe00) after the renderer lock
       0x00438730("scr_ach::draw", 1) / 0x004387c0. */
    void *(*shape_load)(void *context, const char *name, int32_t id);
    void (*shape_free)(void *context, void *shape);
    int (*dc_lock)(void *context, const char *who);
    void (*dc_unlock)(void *context, const char *who);
    void (*shape_draw)(void *context, void *shape, int32_t x, int32_t y, int32_t frame);
    /* The timeline panel's callbacks and a font record by index. */
    const AoeTimelineOpsRor *timeline_ops;
    int (*font)(void *context, int32_t index, AoeTimelineFontRor *out);
} AoeAchievementsScreenOpsRor;

typedef struct {
    AoeUiEzRor ez;
    int32_t mode;                      /* +0x478: 0 from the menus */
    int32_t page;                      /* +0x47c */
    int32_t columns;                   /* +0x480 */
    AoeUiTextRor *title;               /* +0x484 */
    AoeUiTextRor *page_title;          /* +0x488 */
    AoeUiTextRor *footer;              /* +0x48c */
    AoeUiTextRor *message;             /* +0x490 */
    AoeUiTextRor *clock;               /* +0x494 */
    AoeUiTextRor *heading[7];          /* +0x498 */
    AoeUiTextRor *hit[5];              /* +0x4b4: the heading hit boxes */
    AoeUiTextRor *cell[8][7];          /* +0x4c8 */
    AoeUiTextRor *name[8];             /* +0x5a8 */
    AoeUiPanelRor *extra[2];           /* +0x5c8, +0x5cc (never created) */
    AoeUiScrollbarRor *message_bar;    /* +0x5d0 */
    AoeUiButtonRor *close_btn;         /* +0x5d4 (string 0x269c) */
    AoeUiButtonRor *back;              /* +0x5d8 (0x26da), hidden */
    AoeUiButtonRor *back2;             /* +0x5dc (0x269c), hidden */
    AoeUiButtonRor *timeline_btn;      /* +0x5e0 (0x26d8) */
    AoeUiButtonRor *scenario_btn;      /* +0x5e4 (0x26b2) */
    AoeUiButtonRor *restart_btn;       /* +0x5e8 (0x269f) */
    AoeUiButtonRor *hidden_close;      /* +0x5ec (0x3ea, position mode 9) */
    void *check_shape;                 /* +0x5f0 */
    AoeTimelineRor *timeline;          /* +0x5f4 */
    int32_t timeline_shown;            /* its +0x14 state (not a panel here) */
    AoeGameUiRor *ui;
    const AoeAchievementsScreenOpsRor *ops;
} AoeAchievementsScreenRor;

/* The screen's vtable (the screen's with destroy, draw, message, mouse
   and action overridden). */
const AoeUiEzVtblRor *aoe_achievements_screen_vtable_ror(void);

/* 0x00477340 constructor: info "scr10" / 50061 from the menus (`mode`
   0), else "scr7" / 50057 for a victorious viewed player or "scr8" /
   50058; the title (0x269e), page title, footer (0x26db), the clock
   ("%02ld:%02ld:%02ld" of the game time, alignment 3 / 2), the scenario
   message with its 20-wide scrollbar (bevel 4 in the screen's colours)
   when `mode` is set and there is one, the timeline (10, 85, 620 x 330,
   fonts 11 / 9, border 3 in the screen's colours, "timeline.slp"), the
   hidden close button, seven headings, five hit boxes, eight rows (name
   and seven cells coloured by the player's colour: 0xc40000, 0xc8,
   0xeaea, 0x215a8c, 0x80ff, 0x8000, 0x808080, 0x808040, else 0xffffff),
   the buttons on row 441 laid out for the menus / a networked game with
   or without a scenario message, Back and Back2 hidden with Escape, the
   check shape ("viccheck.slp" 51 999 or "defcheck.slp" 52 000), the Tab
   order and page 5 (a message) or 0. Returns 0 (panel +0xd8 set) when
   the setup fails. */
int aoe_achievements_screen_init_ror(AoeAchievementsScreenRor *s, AoeGameUiRor *ui,
                                     AoeUiAreaRor *area, int32_t mode,
                                     const AoeAchievementsScreenOpsRor *ops);
/* 0x00478200 page switch (the wait cursor first): pages 0..4 fill the
   headings and the player rows from achievements_page.h and show
   `columns` of them; 0 also shows the footer and the page buttons; 5
   shows the message and Back2; 6 the timeline. */
void aoe_achievements_screen_set_page_ror(AoeAchievementsScreenRor *s, int32_t page);
/* The player colour of a row (0x00477340's switch). */
uint32_t aoe_achievements_row_color_ror(int16_t color_index);

#endif
