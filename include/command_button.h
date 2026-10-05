#ifndef VITA_AOE_COMMAND_BUTTON_H
#define VITA_AOE_COMMAND_BUTTON_H

#include <stdint.h>

#include "ui_button.h"

/* Patched Rise of Rome 1.0a game-screen command button (tpnl_btn, vtable
   0x00549cac, 0x004f7420..0x004f7be0): a button with a style, an
   optional background shape, a help line and an optional count drawn in
   its corner. Styles: 0 a framed picture, 1 a picture or centred text, 2
   a bare picture (the pressed look is the next frame), 3 the plain
   button draw. Only +0x30 (draw) and +0xc0 (inside) differ from the
   button. */

typedef struct {
    void *context;
    /* A shape frame at (x, y), clipped to `clip`; `remap` >= 0 draws the
       player colours from that palette index (the original's colour
       table, 0x00540760), `disabled` through the grey table
       (0x00540680). */
    void (*draw_shape)(void *context, AoeUiAreaRor *area, void *shape,
                       int32_t frame, int32_t x, int32_t y,
                       const AoeUiRectRor *clip, int32_t remap, int disabled);
    /* The viewed player's colour (its colour record +0x26). */
    int32_t (*player_color)(void *context);
} AoeCommandButtonOpsRor;

typedef struct {
    AoeUiButtonRor button;
    int32_t show_count;        /* +0x2b8 */
    int32_t count;             /* +0x2bc */
    uint8_t style;             /* +0x2c0 */
    void *background;          /* +0x2c4 */
    int32_t player_colors;     /* +0x2c8 */
    int32_t background_frame;  /* +0x2cc */
    char help[0x100];          /* +0x2d0 */
    int32_t field_3d0;         /* +0x3d0 */
    const AoeCommandButtonOpsRor *ops;
    /* The parent's map panel (game screen +0x4bc), which covers style 2
       and 3 buttons. */
    AoeUiPanelRor *const *cover;
} AoeCommandButtonRor;

extern const AoeUiButtonVtblRor *aoe_command_button_vtable_ror(void);

/* 0x004f7420: a button (draw type 1, the sound, action notify, ID -1)
   with the font and background shape; 0 when setup fails. */
int aoe_command_button_init_ror(AoeCommandButtonRor *c, AoeUiSystemRor *system,
                                const AoeUiButtonOpsRor *button_ops,
                                const AoeCommandButtonOpsRor *ops,
                                AoeUiAreaRor *area, AoeUiPanelRor *parent,
                                void *sound, uintptr_t font, int32_t font_w,
                                int32_t font_h, void *background);
/* 0x004f7510 help line (0x100 characters, NULL clears); 0x004f7bc0 count
   shown; 0x004f7be0 count (each redraws). */
void aoe_command_button_set_help_ror(AoeCommandButtonRor *c, const char *text);
void aoe_command_button_show_count_ror(AoeCommandButtonRor *c, int32_t show);
void aoe_command_button_set_count_ror(AoeCommandButtonRor *c, int32_t count);
/* +0x30 0x004f75b0 draw; +0xc0 0x004f7550 inside (not over the map for
   styles 2 and 3). */
void aoe_command_button_draw_ror(AoeUiPanelRor *p);
int32_t aoe_command_button_inside_ror(AoeUiPanelRor *p, int32_t x, int32_t y);

#endif
