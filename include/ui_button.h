#ifndef VITA_AOE_UI_BUTTON_H
#define VITA_AOE_UI_BUTTON_H

#include <stdint.h>

#include "ui_panel.h"

/* Patched Rise of Rome 1.0a button panel (vtable 0x00544e30,
   0x0045ebf0..0x00460700). Up to nine states, each with a command ID and
   value, a picture and frame, two text lines and colour pairs. Types: 1
   plain, 2 radio (stays down), 3 cycling through states. Notifications
   reach the parent through +0xb8 action codes: 2 pressed, 3 released, 1
   clicked (left); 5, 6, 4 for the right button. Semantic storage. */

#define AOE_UI_BUTTON_STATES 9

typedef struct AoeUiButtonRor AoeUiButtonRor;

typedef struct {
    void *context;
    /* Sound object +0x148: playing, stop, play. */
    int (*sound_playing)(void *context, void *sound);
    void (*sound_stop)(void *context, void *sound);
    void (*sound_play)(void *context, void *sound);
    void *(*sound_by_id)(void *context, int32_t id); /* 0x00419a20 */
    /* SendMessage(window, WM_PAINT) and (WM_COMMAND, id, value). */
    void (*paint_now)(void *context, AoeUiAreaRor *area);
    void (*command)(void *context, AoeUiAreaRor *area, int32_t id,
                    int32_t value);
    /* Shape hit test (x, y, frame) on the state's picture. */
    int (*shape_hit)(void *context, void *picture, int32_t x, int32_t y,
                     int16_t frame);
} AoeUiButtonOpsRor;

typedef struct {
    AoeUiPanelVtblRor base;
    void (*set_state)(AoeUiButtonRor *, int16_t);                   /* +0xe4 */
    int32_t (*hit_test)(AoeUiButtonRor *, int32_t, int32_t);         /* +0xe8 */
} AoeUiButtonVtblRor;

struct AoeUiButtonRor {
    AoeUiPanelRor panel;
    const AoeUiButtonOpsRor *ops;
    int32_t ids[AOE_UI_BUTTON_STATES];      /* +0xf4 */
    int32_t values[AOE_UI_BUTTON_STATES];   /* +0x118 */
    int32_t type;                           /* +0x13c */
    int32_t draw_type;                      /* +0x140 */
    int32_t notify;                         /* +0x144: 1 action, else command */
    void *sound;                            /* +0x148 */
    void *pictures[AOE_UI_BUTTON_STATES];   /* +0x14c */
    int16_t frames[AOE_UI_BUTTON_STATES];   /* +0x170 */
    int32_t center_picture;                 /* +0x184 */
    int32_t picture_x, picture_y;           /* +0x188, +0x18c */
    int32_t rect_hit;                       /* +0x190: no shape hit test */
    int32_t invert_press;                   /* +0x194 */
    char *text1[AOE_UI_BUTTON_STATES];      /* +0x198 */
    char *text2[AOE_UI_BUTTON_STATES];      /* +0x1bc */
    int32_t text_x, text_y;                 /* +0x1e0, +0x1e4 (-1 centre) */
    uintptr_t font;                         /* +0x1e8 */
    int32_t font_width, font_height;        /* +0x1ec, +0x1f0 */
    int16_t state_count, state;             /* +0x1f4, +0x1f6 */
    int32_t pressed;                        /* +0x1f8 */
    AoeUiButtonRor **group;                 /* +0x1fc */
    int16_t group_count;                    /* +0x200 */
    uint32_t press_ms;                      /* +0x204 */
    uint32_t text_color[AOE_UI_BUTTON_STATES], text_shadow[AOE_UI_BUTTON_STATES];
    uint32_t focus_color[AOE_UI_BUTTON_STATES], focus_shadow[AOE_UI_BUTTON_STATES];
    int32_t hotkey, hotkey_shift;           /* +0x298, +0x29c */
    int32_t bevel_type;                     /* +0x2a0 */
    uint8_t bevel[6];                       /* +0x2a4..+0x2a9 */
    int32_t held_key;                       /* +0x2ac */
    int32_t disabled;                       /* +0x2b0 */
    int32_t sound_id;                       /* +0x2b4 */
};

extern const AoeUiButtonVtblRor aoe_ui_button_vtable_ror;

/* +0x30 0x0045f4b0: draw types 2/4/6 draw the state's picture frame
   (centred when asked), 3/4/5 its text (the state's or the first; a
   second line under the first when the state has one; a shadow one
   pixel down-left; the focus colours with focus), 3..6 the bevel (two
   rectangles when disabled, else by type, inverted when pressed); 5
   fills with the panel colour first, the others redraw the parent
   behind. A pressed look (pressed, or not pressed when inverted) shifts
   the picture and text one pixel right and up. */
void aoe_ui_button_draw_slot_ror(AoeUiPanelRor *p);

/* 0x0045ebf0 constructor (one state, colours white/black and yellow/
   black, frames -1, text centred); 0x0045ed50 destructor (deleting
   0x0045ed30); 0x0045edc0 setup (area, parent, x, y, w, h, draw type,
   sound, notify, first command ID). */
void aoe_ui_button_init_ror(AoeUiButtonRor *b, AoeUiSystemRor *system,
                            const AoeUiButtonOpsRor *ops);
void aoe_ui_button_release_ror(AoeUiButtonRor *b);
int aoe_ui_button_setup_ror(AoeUiButtonRor *b, AoeUiAreaRor *area,
                            AoeUiPanelRor *parent, int32_t x, int32_t y,
                            int32_t w, int32_t h, int32_t draw_type,
                            void *sound, int32_t notify, int32_t id);
/* 0x0045ee30 draw type; 0x0045ee40 text, font and text position;
   0x0045ee80 text position; 0x0045eea0 radio group (type 2);
   0x0045ef20 cycling states (type 3); 0x0045ef40 a state's ID and value;
   0x0045ef60 a state's picture and frame; 0x0045ef90 a state's text
   (split at the first newline); 0x0045f0a0 both lines; 0x0045f1a0 text
   from a string ID; 0x0045f1e0 font; 0x0045f210 / 0x0045f240 text and
   focus colours (states 0..8); 0x0045f270 sound; 0x0045f350 bevel;
   0x0045f3a0 disabled (no mouse); 0x004606d0 sound ID. */
void aoe_ui_button_set_draw_type_ror(AoeUiButtonRor *b, int32_t type);
void aoe_ui_button_set_text_info_ror(AoeUiButtonRor *b, const char *text,
                                     uintptr_t font, int32_t font_width,
                                     int32_t font_height, int32_t x,
                                     int32_t y);
void aoe_ui_button_set_text_pos_ror(AoeUiButtonRor *b, int32_t x, int32_t y);
int aoe_ui_button_set_group_ror(AoeUiButtonRor *b, AoeUiButtonRor **group,
                                int16_t count);
void aoe_ui_button_set_cycle_ror(AoeUiButtonRor *b, int16_t count);
void aoe_ui_button_set_id_ror(AoeUiButtonRor *b, int16_t state, int32_t id,
                              int32_t value);
void aoe_ui_button_set_picture_ror(AoeUiButtonRor *b, int16_t state,
                                   void *picture, int16_t frame);
int aoe_ui_button_set_text_ror(AoeUiButtonRor *b, int16_t state,
                               const char *text);
int aoe_ui_button_set_texts_ror(AoeUiButtonRor *b, int16_t state,
                                const char *line1, const char *line2);
int aoe_ui_button_set_text_id_ror(AoeUiButtonRor *b, int16_t state,
                                  int32_t string_id);
void aoe_ui_button_set_font_ror(AoeUiButtonRor *b, uintptr_t font,
                                int32_t width, int32_t height);
void aoe_ui_button_set_text_color_ror(AoeUiButtonRor *b, int32_t state,
                                      uint32_t color, uint32_t shadow);
void aoe_ui_button_set_focus_color_ror(AoeUiButtonRor *b, int32_t state,
                                       uint32_t color, uint32_t shadow);
void aoe_ui_button_set_sound_ror(AoeUiButtonRor *b, void *sound);
void aoe_ui_button_set_bevel_ror(AoeUiButtonRor *b, int32_t type,
                                 const uint8_t colors[6]);
void aoe_ui_button_set_disabled_ror(AoeUiButtonRor *b, int32_t disabled);
void aoe_ui_button_set_sound_id_ror(AoeUiButtonRor *b, int32_t id);
/* 0x0045f280 select in the radio group; +0xe4 0x0045f2f0 set state;
   0x0045f310 state by command ID; getters 0x0045f400 state, 0x0045f410
   ID, 0x0045f420 value, 0x0045f430 texts (nonzero with a first line),
   0x0045f470 colours. */
void aoe_ui_button_select_ror(AoeUiButtonRor *b);
void aoe_ui_button_set_state_ror(AoeUiButtonRor *b, int16_t state);
void aoe_ui_button_set_state_by_id_ror(AoeUiButtonRor *b, int32_t id);
int16_t aoe_ui_button_state_ror(const AoeUiButtonRor *b);
int32_t aoe_ui_button_id_ror(const AoeUiButtonRor *b);
int32_t aoe_ui_button_value_ror(const AoeUiButtonRor *b);
int aoe_ui_button_texts_ror(const AoeUiButtonRor *b, int16_t state,
                            const char **line1, const char **line2);
void aoe_ui_button_colors_ror(const AoeUiButtonRor *b, int16_t state,
                              uint32_t *color, uint32_t *shadow);
/* Slots: +0x0c 0x0045f3e0 (the base set_rect), +0x48 0x004603a0 (key-up
   of the held key clicks), +0x58 0x00460100 (hotkey press), +0x70
   0x0045fcd0 (hit test first), +0x84 0x0045fd30 / +0x98 0x0045ff10
   (press), +0x8c 0x0045fdd0 / +0xa0 0x0045ffb0 (drag in and out), +0x90
   0x0045fe90 / +0xa4 0x00460070 (release), +0x94 / +0xa8 0x004072b0 (0),
   +0xac 0x00460230 (Enter/Space press, Up/Down move the tab focus),
   +0xd4 0x004606e0 (drop the sound), +0xd8 0x00460700 (reload it), +0xe8
   0x00460440 (the picture's shape unless rect hit). 0x004604c0 /
   0x004605b0: the left/right click (cycle or select, then notify);
   0x00460690: drop a held press. */
int32_t aoe_ui_button_hit_test_ror(AoeUiButtonRor *b, int32_t x, int32_t y);
void aoe_ui_button_click_ror(AoeUiButtonRor *b);
void aoe_ui_button_right_click_ror(AoeUiButtonRor *b);
void aoe_ui_button_cancel_ror(AoeUiButtonRor *b);

#endif
