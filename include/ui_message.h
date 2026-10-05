#ifndef VITA_AOE_UI_MESSAGE_H
#define VITA_AOE_UI_MESSAGE_H

#include <stdint.h>

#include "ui_panel.h"

/* Patched Rise of Rome 1.0a message panel (pnl_msg, vtable 0x005454ec,
   0x00465730..0x00466910): one line of shadowed text, optionally
   rendered once into an image buffer ("message panel") and blitted, with
   up to two icons before it, hidden again after a timeout for styles 1, 3
   and 4. GDI work goes through AoeUiMessageOpsRor; the DrawText format
   bits are kept (0x01 centre, 0x02 right, 0x04 v-centre, 0x08 bottom,
   0x10 word break, 0x20 single line). */
typedef struct {
    void *context;
    /* GetTextExtentPoint32A; 0 on failure. */
    int (*extent)(void *context, uintptr_t font, const char *text,
                  int32_t *cx, int32_t *cy);
    /* The image buffer (0x004382a0 + 0x00438500) and its release. */
    void *(*image_create)(void *context, int32_t width, int32_t height);
    void (*image_free)(void *context, void *image);
    /* 0x00438d20: clear the buffer rect. */
    void (*image_clear)(void *context, void *image, const AoeUiRectRor *r);
    /* DrawTextA into the buffer (image) or the panel's area (NULL). */
    void (*draw_text)(void *context, void *image, const char *text,
                      const AoeUiRectRor *r, uint32_t format, uint32_t rgb,
                      int opaque);
    /* 0x00439240: blit part of the buffer to the area at (x, y). */
    void (*blit)(void *context, void *image, int32_t x, int32_t y,
                 const AoeUiRectRor *src);
    /* Palette entry as a COLORREF (GetPaletteEntries). */
    uint32_t (*palette_rgb)(void *context, uint8_t index);
    /* Icon frame metrics (shape frame header +0x10 width, +0x18 / +0x1c
       hotspot) and drawing (0x0049fe00). */
    int (*icon_frame)(void *context, void *shape, int32_t frame,
                      int32_t *width, int32_t *hot_x, int32_t *hot_y);
    void (*draw_icon)(void *context, void *image, void *shape, int32_t frame,
                      int32_t x, int32_t y);
} AoeUiMessageOpsRor;

typedef struct {
    AoeUiPanelRor panel;
    int32_t style;               /* +0xf4: 1 centred, 2 opaque, 5 right */
    char text[0x200];            /* +0xf8 */
    int32_t shown;               /* +0x2f8 */
    uint32_t shown_ms;           /* +0x2fc */
    int32_t timeout_ms;          /* +0x300, 8000 */
    uintptr_t font;              /* +0x304 */
    int32_t font_w, font_h;      /* +0x308, +0x30c */
    uint8_t color, shadow;       /* +0x310, +0x311 palette indices */
    int32_t palette_colors;      /* +0x314 */
    uint32_t rgb, shadow_rgb;    /* +0x318, +0x31c */
    int32_t align;               /* +0x320, 2: 4 centre, 3 right */
    int32_t valign;              /* +0x324: 4 centre, 1 bottom */
    int32_t wrap;                /* +0x328 */
    void *image;                 /* +0x330 */
    AoeUiRectRor buffer;         /* +0x334 */
    AoeUiRectRor text_rect;      /* +0x344, in the buffer */
    AoeUiRectRor screen_rect;    /* +0x354 */
    int32_t buffer_w, buffer_h;  /* +0x364, +0x368, -1 */
    int32_t rendered;            /* +0x36c */
    void *icon[2];               /* +0x370, +0x374 */
    int32_t icon_frame[2];       /* +0x378, +0x37c */
    const AoeUiMessageOpsRor *ops;
} AoeUiMessageRor;

extern const AoeUiPanelVtblRor aoe_ui_message_vtable_ror;

/* 0x00465730 constructor (font and its size); 0x00465840 destructor. */
void aoe_ui_message_init_ror(AoeUiMessageRor *m, AoeUiSystemRor *system,
                             const AoeUiMessageOpsRor *ops, uintptr_t font,
                             int32_t font_w, int32_t font_h);
void aoe_ui_message_release_ror(AoeUiMessageRor *m);
/* 0x00465a60 alignment, vertical alignment and wrap; 0x00466910 font. */
void aoe_ui_message_set_layout_ror(AoeUiMessageRor *m, int32_t align,
                                   int32_t valign, int32_t wrap);
void aoe_ui_message_set_font_ror(AoeUiMessageRor *m, uintptr_t font,
                                 int32_t font_w, int32_t font_h);
/* 0x00465a90: show text in palette colours (unchanged text, style and
   font is a no-op); 0x00465ba0: in RGB colours with icons. Both restart
   the timeout, render and activate. 0x00465d10 hides. */
void aoe_ui_message_show_ror(AoeUiMessageRor *m, int32_t style,
                             const char *text, uint8_t color, uint8_t shadow,
                             uintptr_t font, int32_t font_w, int32_t font_h);
void aoe_ui_message_show_rgb_ror(AoeUiMessageRor *m, int32_t style,
                                 const char *text, uint32_t rgb,
                                 uint32_t shadow_rgb, uintptr_t font,
                                 int32_t font_w, int32_t font_h, void *icon1,
                                 int32_t frame1, void *icon2, int32_t frame2);
void aoe_ui_message_hide_ror(AoeUiMessageRor *m);
/* 0x00466280: lay the text out in the buffer and draw it (four shadow
   passes, then the text), icons to its left. Returns `rendered`. */
int32_t aoe_ui_message_render_ror(AoeUiMessageRor *m);
/* Slots: +0x0c 0x004658d0 (a new buffer when the panel outgrows it),
   +0x30 0x00465d50 draw (blit the rendered buffer, or draw directly),
   +0x4c 0x00466230 idle (timeout), +0xbc 0x004661c0 the text's screen
   rect while rendered and shown. */

#endif
