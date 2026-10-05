#include "ui_ez.h"

#include "palette_fade.h"
#include "draw_prims.h"
#include "draw_system.h"
#include "ui_ez_slots.h"
#include "ui_dialog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define EZ(p) ((AoeUiEzRor *)(p))
#define EZ_VT(ez) ((const AoeUiEzVtblRor *)(ez)->panel.vt)
#define TEXT_VT(t) ((const AoeUiTextVtblRor *)(t)->panel.vt)

static void copy_name(char *dst, size_t size, const char *src) {
    size_t n = strlen(src);
    if (n >= size) n = size - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

void aoe_ui_ez_reset_ror(AoeUiEzRor *ez) {
    if (!ez) return;
    ez->ref_w = 640;
    ez->ref_h = 480;
    ez->info_name[0] = '\0';
    ez->info_id = -1;
    ez->background = NULL;
    ez->background2 = NULL;
    ez->palette = 0;
    ez->pointer_name[0] = '\0';
    ez->pointer_id = -1;
    ez->background_mode = 0;
    ez->bevel_on = 0;
    memset(ez->bevel, 0, sizeof(ez->bevel));
    ez->text_color = 0xffffff;
    ez->text_shadow = 0;
    ez->focus_color = 0xffff;
    ez->focus_shadow = 0;
    ez->hot_color = 0xffffff;
    ez->hot_shadow = 0;
    ez->popup_name[0] = '\0';
    ez->popup_id = -1;
    ez->button_pictures = NULL;
    ez->shadow_table = NULL;
    ez->fill_color = 0;
    ez->fill_color2 = 0;
    ez->ime_toggle = 0;
    ez->help_armed = 0;
    ez->stock_brush = 0;
    ez->brush = 0;
    ez->field_468 = 0;
    ez->shadow_amount = 0;
    ez->shadow_panel = NULL;
    ez->saved_pointer = 0;
}

int aoe_ui_ez_init_named_ror(AoeUiEzRor *ez, AoeUiSystemRor *system,
                             const AoeUiEzOpsRor *ops, const char *name) {
    int ok;
    if (!ez) return 0;
    ok = aoe_ui_panel_init_named_ror(&ez->panel, system, name);
    ez->panel.vt = &aoe_ui_ez_vtable_ror.base;
    ez->ops = ops;
    ez->field_470 = 0;
    aoe_ui_ez_reset_ror(ez);
    return ok;
}

void aoe_ui_ez_init_ror(AoeUiEzRor *ez, AoeUiSystemRor *system,
                        const AoeUiEzOpsRor *ops) {
    if (!ez) return;
    aoe_ui_panel_init_ror(&ez->panel, system);
    ez->panel.vt = &aoe_ui_ez_vtable_ror.base;
    ez->ops = ops;
    ez->field_470 = 0;
    aoe_ui_ez_reset_ror(ez);
}

static void free_shape(AoeUiEzRor *ez, void **shape) {
    if (*shape && ez->ops && ez->ops->free_shape)
        ez->ops->free_shape(ez->ops->context, *shape);
    *shape = NULL;
}

static void free_shadow_table(AoeUiEzRor *ez) {
    if (ez->shadow_table && ez->ops && ez->ops->free_shadow)
        ez->ops->free_shadow(ez->ops->context, ez->shadow_table);
    ez->shadow_table = NULL;
}

void aoe_ui_ez_free_ror(AoeUiEzRor *ez) {
    if (!ez) return;
    if (ez->shadow_panel) {
        aoe_surface_free_ror(ez->shadow_panel);
        free(ez->shadow_panel);
        ez->shadow_panel = NULL;
    }
    ez->brush = 0; /* the original deletes a brush that is not stock */
    free_shape(ez, &ez->background);
    free_shape(ez, &ez->background2);
    if (ez->palette) {
        aoe_ui_palette_release_ror(ez->panel.system, ez->palette);
        ez->palette = 0;
    }
    free_shadow_table(ez);
    free_shape(ez, &ez->button_pictures);
}

void aoe_ui_ez_release_ror(AoeUiEzRor *ez) {
    if (!ez) return;
    aoe_ui_ez_free_ror(ez);
    aoe_ui_panel_release_ror(&ez->panel);
}

void aoe_ui_ez_destroy_slot_ror(AoeUiPanelRor *p) { aoe_ui_ez_release_ror(EZ(p)); }

/* ---- settings ---- */

void aoe_ui_ez_set_info_ror(AoeUiEzRor *ez, const char *name, int32_t id) {
    if (!ez) return;
    if (!name) ez->info_name[0] = '\0';
    else copy_name(ez->info_name, sizeof(ez->info_name), name);
    ez->info_id = id;
}

void aoe_ui_ez_set_reference_slot_ror(AoeUiEzRor *ez, int32_t w, int32_t h) {
    ez->ref_w = w;
    ez->ref_h = h;
}

/* 0x00455470 / 0x004555e0 / 0x00455a50. */
static int load_picture(AoeUiEzRor *ez, void **slot, const char *name,
                        int32_t id) {
    char file[0x104];
    free_shape(ez, slot);
    if (!name || !*name || strcasecmp(name, "none") == 0) {
        if (id == -1) return *slot != NULL;
        file[0] = '\0';
    } else {
        snprintf(file, sizeof(file), strchr(name, '.') ? "%s" : "%s.shp", name);
    }
    if (ez->ops && ez->ops->load_shape)
        *slot = ez->ops->load_shape(ez->ops->context, file, id);
    return *slot != NULL;
}

int aoe_ui_ez_set_background_ror(AoeUiEzRor *ez, const char *name,
                                 int32_t id) {
    return ez ? load_picture(ez, &ez->background, name, id) : 0;
}

int aoe_ui_ez_set_background2_ror(AoeUiEzRor *ez, const char *name,
                                  int32_t id) {
    return ez ? load_picture(ez, &ez->background2, name, id) : 0;
}

void aoe_ui_ez_set_button_pictures_ror(AoeUiEzRor *ez, const char *name,
                                       int32_t id) {
    if (ez) load_picture(ez, &ez->button_pictures, name, id);
}

#define AOE_UI_EZ_BLACK_BRUSH 4 /* GetStockObject(BLACK_BRUSH) */

void aoe_ui_ez_set_fill_color_ror(AoeUiEzRor *ez, uint8_t color) {
    if (!ez) return;
    ez->panel.vt->set_color(&ez->panel, color);
    ez->fill_color = color;
    ez->fill_color2 = color;
    ez->brush = AOE_UI_EZ_BLACK_BRUSH;
    ez->stock_brush = 1;
    ez->field_468 = 0;
}

static void *create_shadow(AoeUiEzRor *ez) {
    if (!ez->ops || !ez->ops->create_shadow) return NULL;
    return ez->ops->create_shadow(ez->ops->context, ez->panel.area,
                                  ez->shadow_amount, ez->palette);
}

int aoe_ui_ez_set_palette_ror(AoeUiEzRor *ez, const char *name, int32_t id) {
    if (!ez) return 0;
    if (ez->palette) {
        aoe_ui_palette_release_ror(ez->panel.system, ez->palette);
        ez->palette = 0;
    }
    free_shadow_table(ez);
    if (id == -1 && (!name || !*name || strcasecmp(name, "none") == 0))
        return 1;
    ez->palette = aoe_ui_palette_ror(ez->panel.system, name, id);
    if (!ez->palette || ez->shadow_amount < 1) return 0;
    ez->shadow_table = create_shadow(ez);
    return 1;
}

int aoe_ui_ez_set_pointer_ror(AoeUiEzRor *ez, const char *name, int32_t id) {
    if (!ez) return 0;
    copy_name(ez->pointer_name, sizeof(ez->pointer_name), name ? name : "");
    ez->pointer_id = id;
    return 1;
}

void aoe_ui_ez_set_bevel_on_ror(AoeUiEzRor *ez, int32_t on) {
    if (ez) ez->bevel_on = on;
}

void aoe_ui_ez_set_background_mode_ror(AoeUiEzRor *ez, int32_t mode) {
    if (ez) ez->background_mode = mode;
}

void aoe_ui_ez_set_bevel_colors_ror(AoeUiEzRor *ez, const uint8_t colors[6]) {
    if (ez && colors) memcpy(ez->bevel, colors, sizeof(ez->bevel));
}

void aoe_ui_ez_set_text_colors_ror(AoeUiEzRor *ez, uint32_t color,
                                   uint32_t shadow) {
    if (!ez) return;
    ez->text_color = color;
    ez->text_shadow = shadow;
}

void aoe_ui_ez_set_focus_colors_ror(AoeUiEzRor *ez, uint32_t color,
                                    uint32_t shadow) {
    if (!ez) return;
    ez->focus_color = color;
    ez->focus_shadow = shadow;
}

void aoe_ui_ez_set_hot_colors_ror(AoeUiEzRor *ez, uint32_t color,
                                  uint32_t shadow) {
    if (!ez) return;
    ez->hot_color = color;
    ez->hot_shadow = shadow;
}

void aoe_ui_ez_set_popup_ror(AoeUiEzRor *ez, const char *name, int32_t id) {
    if (!ez) return;
    if (!name) ez->popup_name[0] = '\0';
    else copy_name(ez->popup_name, sizeof(ez->popup_name), name);
    ez->popup_id = id;
}

void aoe_ui_ez_set_shadow_amount_ror(AoeUiEzRor *ez, int32_t amount) {
    if (!ez) return;
    ez->shadow_amount = amount == -1 ? 0 : amount;
    free_shadow_table(ez);
    if (ez->palette && ez->shadow_amount > 0) ez->shadow_table = create_shadow(ez);
    if (ez->panel.has_focus) ez->panel.vt->set_redraw(&ez->panel, 2);
}

const char *aoe_ui_ez_info_name_ror(const AoeUiEzRor *ez) {
    return ez ? ez->info_name : NULL;
}

int32_t aoe_ui_ez_info_id_ror(const AoeUiEzRor *ez) { return ez ? ez->info_id : -1; }

uintptr_t aoe_ui_ez_palette_ror(const AoeUiEzRor *ez) { return ez ? ez->palette : 0; }

const char *aoe_ui_ez_popup_name_ror(const AoeUiEzRor *ez) {
    return ez ? ez->popup_name : NULL;
}

int32_t aoe_ui_ez_popup_id_ror(const AoeUiEzRor *ez) { return ez ? ez->popup_id : -1; }

/* ---- setup and the info file ---- */

#define EZ_NAME 0x104

typedef struct {
    char label[0x100];
    char bg[3][EZ_NAME], bg2[3][EZ_NAME];
    int32_t bg_id[3], bg2_id[3];
    char palette[EZ_NAME], pointer[EZ_NAME], pictures[EZ_NAME], popup[EZ_NAME];
    int32_t palette_id, pointer_id, shadow, pictures_id, popup_id, mode, fill;
    int32_t bevel[6];
    int32_t rgb[6][3];
} EzInfo;

static uint32_t rgb(const int32_t c[3]) {
    return (uint32_t)(c[0] & 0xff) | (uint32_t)(c[1] & 0xff) << 8 |
           (uint32_t)(c[2] & 0xff) << 16;
}

static void apply_info(AoeUiEzRor *ez, const char *text) {
    EzInfo in;
    int set;
    memset(&in, 0, sizeof(in));
    /* The original format with field widths for the port's buffers. */
    if (sscanf(text,
               "%255s %259s %259s %d %d   %255s %259s %259s %d %d   "
               "%255s %259s %259s %d %d   %255s %259s %d   %255s %259s %d   "
               "%255s %255s %d   %255s %259s %d   %255s %259s %d   "
               "%255s %d   %255s %d    %255s %d %d %d %d %d %d   "
               "%255s %d %d %d   %255s %d %d %d   %255s %d %d %d   "
               "%255s %d %d %d  %255s %d %d %d  %255s %d %d %d",
               in.label, in.bg[0], in.bg2[0], &in.bg_id[0], &in.bg2_id[0],
               in.label, in.bg[1], in.bg2[1], &in.bg_id[1], &in.bg2_id[1],
               in.label, in.bg[2], in.bg2[2], &in.bg_id[2], &in.bg2_id[2],
               in.label, in.palette, &in.palette_id, in.label, in.pointer,
               &in.pointer_id, in.label, in.label, &in.shadow, in.label,
               in.pictures, &in.pictures_id, in.label, in.popup, &in.popup_id,
               in.label, &in.mode, in.label, &in.fill, in.label, &in.bevel[0],
               &in.bevel[1], &in.bevel[2], &in.bevel[3], &in.bevel[4],
               &in.bevel[5], in.label, &in.rgb[0][0], &in.rgb[0][1],
               &in.rgb[0][2], in.label, &in.rgb[1][0], &in.rgb[1][1],
               &in.rgb[1][2], in.label, &in.rgb[2][0], &in.rgb[2][1],
               &in.rgb[2][2], in.label, &in.rgb[3][0], &in.rgb[3][1],
               &in.rgb[3][2], in.label, &in.rgb[4][0], &in.rgb[4][1],
               &in.rgb[4][2], in.label, &in.rgb[5][0], &in.rgb[5][1],
               &in.rgb[5][2]) == EOF)
        return;
    aoe_ui_ez_set_bevel_on_ror(ez, 1);
    {
        int32_t width = ez->panel.area ? ez->panel.area->width : 0;
        if (width < 800 || strcasecmp(in.bg[1], "none") == 0) set = 0;
        else if (width < 0x400) set = 1;
        else set = 2;
    }
    aoe_ui_ez_set_background_ror(ez, in.bg[set], in.bg_id[set]);
    aoe_ui_ez_set_background2_ror(ez, in.bg2[set], in.bg2_id[set]);
    aoe_ui_ez_set_shadow_amount_ror(ez, in.shadow);
    aoe_ui_ez_set_palette_ror(ez, in.palette, in.palette_id);
    aoe_ui_ez_set_pointer_ror(ez, in.pointer, in.pointer_id);
    aoe_ui_ez_set_background_mode_ror(ez, in.mode);
    aoe_ui_ez_set_fill_color_ror(ez, (uint8_t)in.fill);
    {
        uint8_t bevel[6];
        int i;
        for (i = 0; i < 6; ++i) bevel[i] = (uint8_t)in.bevel[i];
        aoe_ui_ez_set_bevel_colors_ror(ez, bevel);
    }
    aoe_ui_ez_set_text_colors_ror(ez, rgb(in.rgb[0]), rgb(in.rgb[1]));
    aoe_ui_ez_set_focus_colors_ror(ez, rgb(in.rgb[2]), rgb(in.rgb[3]));
    aoe_ui_ez_set_hot_colors_ror(ez, rgb(in.rgb[4]), rgb(in.rgb[5]));
    aoe_ui_ez_set_button_pictures_ror(ez, in.pictures, in.pictures_id);
    aoe_ui_ez_set_popup_ror(ez, in.popup, in.popup_id);
}

int aoe_ui_ez_setup_ror(AoeUiEzRor *ez, AoeUiAreaRor *area,
                        AoeUiPanelRor *parent, const char *info_name,
                        int32_t info_id, int32_t full, int32_t x, int32_t y,
                        int32_t w, int32_t h, int32_t field_470) {
    char *text = NULL;
    if (!ez) return 0;
    ez->field_470 = field_470;
    aoe_ui_ez_set_info_ror(ez, info_name, info_id);
    if (!full) {
        if (w > 0 && h > 0 && area) {
            if (x == -1) x = area->width / 2 - w / 2;
            if (y == -1) y = area->height / 2 - h / 2;
        }
    } else {
        x = y = 0;
        w = area ? area->width : 0;
        h = area ? area->height : 0;
    }
    if (!aoe_ui_panel_setup_ror(&ez->panel, area, parent, x, y, w, h, 0))
        return 0;
    EZ_VT(ez)->set_reference(ez, w, h);
    if (full)
        ez->panel.vt->set_position_mode(&ez->panel, 1, 0, 0, 0, 0, 0, 0, 0, 0,
                                        NULL, NULL, NULL, NULL);
    if (!ez->ops || !ez->ops->load_info) return 1;
    if (ez->ops->loose_files && ez->info_name[0]) {
        char file[0x104];
        snprintf(file, sizeof(file),
                 strchr(ez->info_name, '.') ? "%s" : "%s.sin", ez->info_name);
        text = ez->ops->load_info(ez->ops->context, file, -1);
    }
    if (!text) {
        if (ez->info_id == -1) return 1;
        text = ez->ops->load_info(ez->ops->context, NULL, ez->info_id);
        if (!text) return 1;
    }
    apply_info(ez, text);
    free(text);
    return 1;
}

/* ---- creators ---- */

static int32_t sx(const AoeUiEzRor *ez, int32_t v) {
    return ez->ref_w ? ez->panel.width * v / ez->ref_w : v;
}

static int32_t sy(const AoeUiEzRor *ez, int32_t v) {
    return ez->ref_h ? ez->panel.height * v / ez->ref_h : v;
}

static int font_of(AoeUiEzRor *ez, int32_t index, uintptr_t *font,
                   int32_t *w, int32_t *h) {
    *font = 0;
    *w = *h = 0;
    return ez->ops && ez->ops->font &&
           ez->ops->font(ez->ops->context, index, font, w, h);
}

static int fail(AoeUiEzRor *ez) {
    ez->panel.field_d8 = 1;
    return 0;
}

static AoeUiButtonRor *new_button(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                                  AoeUiButtonRor **out, int32_t x, int32_t y,
                                  int32_t w, int32_t h, int32_t draw_type,
                                  int32_t sound_id, int32_t command) {
    AoeUiButtonRor *b = calloc(1, sizeof(*b));
    void *sound = NULL;
    if (b) aoe_ui_button_init_ror(b, ez->panel.system,
                                  ez->ops ? ez->ops->button_ops : NULL);
    *out = b;
    if (!b || b->panel.field_d8) return NULL;
    if (sound_id >= 0 && ez->ops && ez->ops->sound)
        sound = ez->ops->sound(ez->ops->context, sound_id);
    if (!aoe_ui_button_setup_ror(b, ez->panel.area, parent, sx(ez, x),
                                 sy(ez, y), sx(ez, w), sy(ez, h), draw_type,
                                 sound, 1, command))
        return NULL;
    aoe_ui_button_set_sound_id_ror(b, sound_id);
    return b;
}

static void button_bevel(AoeUiEzRor *ez, AoeUiButtonRor *b) {
    if (ez->bevel_on) aoe_ui_button_set_bevel_ror(b, 4, ez->bevel);
}

static void two_state_colors(AoeUiEzRor *ez, AoeUiButtonRor *b) {
    aoe_ui_button_set_text_color_ror(b, 0, ez->text_color, ez->text_shadow);
    aoe_ui_button_set_focus_color_ror(b, 0, ez->focus_color, ez->focus_shadow);
    aoe_ui_button_set_text_color_ror(b, 1, ez->hot_color, ez->hot_shadow);
    aoe_ui_button_set_focus_color_ror(b, 1, ez->focus_color, ez->focus_shadow);
}

int aoe_ui_ez_create_button_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                                AoeUiButtonRor **out, const char *text,
                                const char *text2, int32_t x, int32_t y,
                                int32_t w, int32_t h, int32_t font,
                                int32_t sound_id, int32_t command) {
    AoeUiButtonRor *b;
    uintptr_t f;
    int32_t fw, fh, i;
    if (!ez || !out) return 0;
    b = new_button(ez, parent, out, x, y, w, h, 3, sound_id, command);
    if (!b) return fail(ez);
    font_of(ez, font < 0 ? 10 : font, &f, &fw, &fh);
    aoe_ui_button_set_text_info_ror(b, text, f, fw, fh, -1, -1);
    if (text2) aoe_ui_button_set_text_ror(b, 1, text2);
    button_bevel(ez, b);
    for (i = 0; i < 9; ++i) {
        aoe_ui_button_set_text_color_ror(b, i, ez->text_color, ez->text_shadow);
        aoe_ui_button_set_focus_color_ror(b, i, ez->focus_color, ez->focus_shadow);
    }
    aoe_ui_button_set_text_color_ror(b, 1, ez->hot_color, ez->hot_shadow);
    aoe_ui_button_set_focus_color_ror(b, 1, ez->focus_color, ez->focus_shadow);
    return 1;
}

int aoe_ui_ez_create_button_id_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                                   AoeUiButtonRor **out, int32_t text_id,
                                   int32_t text2_id, int32_t x, int32_t y,
                                   int32_t w, int32_t h, int32_t font,
                                   int32_t sound_id, int32_t command) {
    char text[0x78], text2[0x78];
    if (!ez) return 0;
    text[0] = text2[0] = '\0';
    aoe_ui_string_ror(ez->panel.system, text_id, text, sizeof(text));
    aoe_ui_string_ror(ez->panel.system, text2_id, text2, sizeof(text2));
    return aoe_ui_ez_create_button_ror(ez, parent, out, text, text2, x, y, w,
                                       h, font, sound_id, command);
}

/* Check and radio buttons: the button pictures at two frames, or an
   "X" on a text button. */
static void mark_button(AoeUiEzRor *ez, AoeUiButtonRor *b, int32_t frame0,
                        int32_t frame1, int text_first) {
    uintptr_t f;
    int32_t fw, fh;
    if (!ez->button_pictures) {
        if (!text_first) aoe_ui_button_set_draw_type_ror(b, 3);
        font_of(ez, 0, &f, &fw, &fh);
        aoe_ui_button_set_text_info_ror(b, " ", f, fw, fh, -1, -1);
        aoe_ui_button_set_text_ror(b, 1, "X");
        if (text_first) aoe_ui_button_set_draw_type_ror(b, 3);
    } else {
        aoe_ui_button_set_draw_type_ror(b, 6);
        aoe_ui_button_set_picture_ror(b, 0, ez->button_pictures, (int16_t)frame0);
        aoe_ui_button_set_picture_ror(b, 1, ez->button_pictures, (int16_t)frame1);
    }
}

int aoe_ui_ez_create_check_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                               AoeUiButtonRor **out, int32_t x, int32_t y,
                               int32_t w, int32_t h, int32_t sound_id,
                               int32_t command) {
    AoeUiButtonRor *b;
    if (!ez || !out) return 0;
    b = new_button(ez, parent, out, x, y, w, h, 6, sound_id, command);
    if (!b) return fail(ez);
    aoe_ui_button_set_cycle_ror(b, 2);
    mark_button(ez, b, 0, 2, 1);
    button_bevel(ez, b);
    two_state_colors(ez, b);
    return 1;
}

int aoe_ui_ez_create_radio_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                               AoeUiButtonRor **out, int32_t x, int32_t y,
                               int32_t w, int32_t h, int32_t sound_id,
                               int32_t command) {
    AoeUiButtonRor *b;
    if (!ez || !out) return 0;
    b = new_button(ez, parent, out, x, y, w, h, 2, sound_id, command);
    if (!b) return fail(ez);
    mark_button(ez, b, 4, 6, 0);
    button_bevel(ez, b);
    two_state_colors(ez, b);
    return 1;
}

static void text_colors(AoeUiEzRor *ez, AoeUiTextRor *t) {
    aoe_ui_text_set_colors_ror(t, ez->text_color, ez->text_shadow);
    aoe_ui_text_set_focus_colors_ror(t, ez->focus_color, ez->focus_shadow);
}

static AoeUiTextRor *new_text(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                              AoeUiTextRor **out, int32_t x, int32_t y,
                              int32_t w, int32_t h, int32_t font) {
    AoeUiTextRor *t = calloc(1, sizeof(*t));
    uintptr_t f;
    int32_t fw, fh;
    if (t) aoe_ui_text_init_ror(t, ez->panel.system);
    *out = t;
    if (!t || t->panel.field_d8) return NULL;
    font_of(ez, font < 0 ? 10 : font, &f, &fw, &fh);
    if (!aoe_ui_text_setup_ror(t, ez->panel.area, parent, sx(ez, x), sy(ez, y),
                               sx(ez, w), sy(ez, h), f, fw, fh, NULL, 0, 0, 0,
                               0, 0, 0, 0, NULL))
        return NULL;
    return t;
}

static void text_alignment(AoeUiTextRor *t, int32_t centred, int32_t middle) {
    aoe_ui_text_set_alignment_ror(t, middle ? 0 : 3, centred ? 0 : 1);
}

int aoe_ui_ez_create_text_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                              AoeUiTextRor **out, const char *text, int32_t x,
                              int32_t y, int32_t w, int32_t h, int32_t font,
                              int32_t centred, int32_t middle, int32_t wrap) {
    AoeUiTextRor *t;
    if (!ez || !out) return 0;
    t = new_text(ez, parent, out, x, y, w, h, font);
    if (!t) return fail(ez);
    text_alignment(t, centred, middle);
    aoe_ui_text_set_wrap_ror(t, wrap);
    TEXT_VT(t)->set_text(t, text);
    text_colors(ez, t);
    return 1;
}

int aoe_ui_ez_create_text_id_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                                 AoeUiTextRor **out, int32_t text_id,
                                 int32_t x, int32_t y, int32_t w, int32_t h,
                                 int32_t font, int32_t centred,
                                 int32_t middle, int32_t wrap) {
    char text[0x100];
    if (!ez) return 0;
    text[0] = '\0';
    aoe_ui_string_ror(ez->panel.system, text_id, text, sizeof(text));
    return aoe_ui_ez_create_text_ror(ez, parent, out, text, x, y, w, h, font,
                                     centred, middle, wrap);
}

int aoe_ui_ez_create_lines_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                               AoeUiTextRor **out, const char *const *lines,
                               int16_t count, int32_t x, int32_t y, int32_t w,
                               int32_t h, int32_t font, int32_t centred,
                               int32_t middle) {
    AoeUiTextRor *t;
    if (!ez || !out) return 0;
    t = new_text(ez, parent, out, x, y, w, h, font);
    if (!t) return fail(ez);
    TEXT_VT(t)->set_lines(t, lines, count);
    aoe_ui_text_scroll_ror(t, 6, 0, 1);
    text_alignment(t, centred, middle);
    text_colors(ez, t);
    return 1;
}

int aoe_ui_ez_create_input_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                               AoeUiInputRor **out, const char *text,
                               int16_t max, int32_t format, int32_t x,
                               int32_t y, int32_t w, int32_t h, int32_t font) {
    AoeUiInputRor *in;
    uintptr_t f;
    int32_t fw, fh;
    if (!ez || !out) return 0;
    in = calloc(1, sizeof(*in));
    if (in)
        aoe_ui_input_init_ror(in, ez->panel.system,
                              ez->ops ? ez->ops->input_ops : NULL,
                              ez->ops ? ez->ops->caret_blink_ms : 0);
    *out = in;
    if (!in || in->text.panel.field_d8) return fail(ez);
    font_of(ez, font < 0 ? 10 : font, &f, &fw, &fh);
    if (!aoe_ui_input_setup_ror(in, ez->panel.area, parent, sx(ez, x),
                                sy(ez, y), sx(ez, w), sy(ez, h), f, fw, fh,
                                NULL, 0, 0, 0, 0, 1, 0xff, 0xff, max, NULL,
                                text, format))
        return fail(ez);
    if (ez->bevel_on) TEXT_VT(&in->text)->set_bevel(&in->text, 3, ez->bevel);
    text_colors(ez, &in->text);
    return 1;
}

static const char *const one_space[1] = {" "};

int aoe_ui_ez_create_dropdown_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                                  AoeUiDropdownRor **out, int32_t list_w,
                                  int32_t list_h, int32_t x, int32_t y,
                                  int32_t w, int32_t h, int32_t font) {
    AoeUiDropdownRor *d;
    uintptr_t f;
    int32_t fw, fh, h_px, side;
    if (!ez || !out) return 0;
    h_px = sy(ez, h);
    side = sx(ez, 18);
    /* Port choice: the original reads the table entry before the first
       for a negative index here. */
    font_of(ez, font < 0 ? 10 : font, &f, &fw, &fh);
    if (ez->panel.field_d8) return 0;
    d = calloc(1, sizeof(*d));
    if (d) aoe_ui_dropdown_init_ror(d, ez->panel.system,
                                    ez->ops ? ez->ops->dropdown_ops : NULL);
    *out = d;
    if (!d || d->panel.field_d8) return fail(ez);
    if (!aoe_ui_dropdown_setup_ror(d, ez->panel.area, parent, f, fw, fh, 0, 0,
                                   1, 0xff, sx(ez, x), sy(ez, y), sx(ez, w),
                                   h_px, 0, 0, side, h_px, sx(ez, list_w),
                                   sy(ez, list_h), one_space, 1, side, side,
                                   side))
        return fail(ez);
    if (ez->button_pictures)
        aoe_ui_dropdown_set_pictures_ror(d, ez->button_pictures, 0xe, -1, 8,
                                         10, 0xc);
    if (ez->bevel_on) aoe_ui_dropdown_set_bevel_ror(d, 3, ez->bevel);
    aoe_ui_dropdown_set_colors_ror(d, ez->text_color, ez->text_shadow);
    text_colors(ez, &d->list->text);
    TEXT_VT(d->text)->set_bevel(d->text, 3, ez->bevel);
    return 1;
}

int aoe_ui_ez_create_list_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                              AoeUiListRor **out, int32_t x, int32_t y,
                              int32_t w, int32_t h, int32_t font) {
    AoeUiListRor *l = NULL;
    uintptr_t f;
    int32_t fw, fh;
    if (!ez || !out) return 0;
    font_of(ez, font < 0 ? 10 : font, &f, &fw, &fh);
    if (!ez->panel.field_d8) {
        l = calloc(1, sizeof(*l));
        if (l) aoe_ui_list_init_ror(l, ez->panel.system);
        *out = l;
        if (l && !l->text.panel.field_d8 &&
            aoe_ui_text_setup_lines_ror(&l->text, ez->panel.area, parent,
                                        sx(ez, x), sy(ez, y), sx(ez, w),
                                        sy(ez, h), f, fw, fh, NULL, 0, 0, 0, 0,
                                        1, 0xff, 0, one_space, 1))
            aoe_ui_list_move_ror(l, 1, 0, 1);
        else
            ez->panel.field_d8 = 1;
    }
    /* Port guard: the original styles *out even when nothing was made. */
    l = *out;
    if (l) {
        if (ez->bevel_on) TEXT_VT(&l->text)->set_bevel(&l->text, 3, ez->bevel);
        text_colors(ez, &l->text);
    }
    return 1;
}

static void scrollbar_style(AoeUiEzRor *ez, AoeUiScrollbarRor *s) {
    if (ez->button_pictures)
        aoe_ui_scrollbar_set_pictures_ror(s, ez->button_pictures, -1, 8, 10, 0xc);
    if (ez->bevel_on) aoe_ui_scrollbar_set_bevel_ror(s, 3, ez->bevel);
}

int aoe_ui_ez_create_scrollbar_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                                   AoeUiScrollbarRor **out,
                                   AoeUiTextRor *target, int32_t range,
                                   int32_t x, int32_t y, int32_t width,
                                   int32_t h) {
    AoeUiScrollbarRor *s;
    if (!ez || !out) return 0;
    s = calloc(1, sizeof(*s));
    if (s) aoe_ui_scrollbar_init_ror(s, ez->panel.system);
    *out = s;
    if (!s || s->panel.field_d8 ||
        !aoe_ui_scrollbar_setup_ror(s, ez->panel.area, parent, sx(ez, x),
                                    sy(ez, y), width, sy(ez, h), width, width,
                                    target ? &target->panel : NULL, range, 0))
        return 0;
    if (target) aoe_ui_text_set_scrollbar_ror(target, &s->panel, 0);
    scrollbar_style(ez, s);
    return 1;
}

int aoe_ui_ez_create_text_scrollbar_ror(AoeUiEzRor *ez,
                                        AoeUiScrollbarRor **out,
                                        AoeUiTextRor *target, int32_t width) {
    if (!ez || !target) return 0;
    if (!aoe_ui_ez_create_scrollbar_ror(ez, target->panel.parent, out, target,
                                        0, 0, 0, width, 0))
        return 0;
    aoe_ui_text_set_scrollbar_ror(target, &(*out)->panel, 1);
    return 1;
}

int aoe_ui_ez_create_value_scrollbar_ror(AoeUiEzRor *ez,
                                         AoeUiPanelRor *parent,
                                         AoeUiValueScrollbarRor **out,
                                         int32_t horizontal, int32_t range,
                                         int32_t line_step, int32_t page_step,
                                         int32_t x, int32_t y, int32_t w,
                                         int32_t h) {
    AoeUiValueScrollbarRor *v;
    if (!ez || !out) return 0;
    v = calloc(1, sizeof(*v));
    if (v) aoe_ui_value_scrollbar_init_ror(v, ez->panel.system, horizontal);
    *out = v;
    if (!v || v->bar.panel.field_d8 ||
        !aoe_ui_value_scrollbar_setup_ror(v, ez->panel.area, parent, sx(ez, x),
                                          sy(ez, y), sx(ez, w), sy(ez, h), 0,
                                          range, line_step, page_step, 1, 0))
        return 0;
    scrollbar_style(ez, &v->bar);
    return 1;
}

int aoe_ui_ez_create_edit_ror(AoeUiEzRor *ez, AoeUiPanelRor *parent,
                              AoeUiEditRor **out, const char *text,
                              int16_t max, int32_t format, int32_t x,
                              int32_t y, int32_t w, int32_t h, int32_t font,
                              int32_t ime_enabled, int32_t ime_open) {
    AoeUiEditRor *e;
    uintptr_t f;
    int32_t fw, fh, px = sx(ez, x), py = sy(ez, y), pw = sx(ez, w);
    if (!ez || !out) return 0;
    e = calloc(1, sizeof(*e));
    if (e) aoe_ui_edit_init_ror(e, ez->panel.system, ez->ops ? ez->ops->edit_ops : NULL);
    *out = e;
    if (!e || e->panel.field_d8) return fail(ez);
    font_of(ez, font < 0 ? 10 : font, &f, &fw, &fh);
    if (!aoe_ui_edit_setup_ror(e, ez->panel.area, parent, px, py, pw, h, f, max,
                               text, format, format != 7))
        return fail(ez);
    if (ez->bevel_on) aoe_ui_edit_set_bevel_ror(e, 3, ez->bevel);
    aoe_ui_edit_set_colors_ror(e, ez->text_color, ez->text_shadow);
    aoe_ui_edit_set_ime_ror(e, ime_enabled, ime_open);
    return 1;
}

void aoe_ui_ez_place_ror(AoeUiEzRor *ez, AoeUiPanelRor *child, int32_t x,
                         int32_t y, int32_t w, int32_t h) {
    if (!ez || !child) return;
    child->vt->set_fixed_position(child, sx(ez, x), sy(ez, y), sx(ez, w),
                                  sy(ez, h));
}

/* ---- dialogs ---- */

static void dialog(AoeUiEzRor *ez, const char *name, int32_t w, int32_t h,
                   int32_t type, const char *text, int32_t bw, int32_t bh) {
    AoeUiMessageDialogRor *m = calloc(1, sizeof(*m));
    if (!m) return;
    aoe_ui_message_dialog_init_ror(m, ez->panel.system, ez->ops, name);
    aoe_ui_message_dialog_setup_ror(m, &ez->panel, ez->popup_name,
                                    ez->popup_id, w, h, (uint8_t)type, text,
                                    bw, bh);
}

static void close_existing(AoeUiEzRor *ez, const char *name) {
    if (aoe_ui_system_find_ror(ez->panel.system, name))
        aoe_ui_system_close_ror(ez->panel.system, name);
}

void aoe_ui_ez_help_dialog_ror(AoeUiEzRor *ez, const char *text,
                               const char *name, int32_t w, int32_t h) {
    uint8_t *mode;
    char copy[0x100];
    if (!ez || !ez->ops || !ez->ops->help_mode) return;
    mode = ez->ops->help_mode;
    if (*mode == 1) {
        if (ez->ops->win_help) ez->ops->win_help(ez->ops->context);
        return;
    }
    if (*mode == 2) return;
    *mode = 2;
    copy_name(copy, sizeof(copy), name && *name ? name : "OKDialog");
    close_existing(ez, copy);
    dialog(ez, copy, w, h, 5, text, 0x5a, 0x1e);
}

void aoe_ui_ez_ok_dialog_ror(AoeUiEzRor *ez, const char *text,
                             const char *name, int32_t w, int32_t h) {
    char copy[0x100];
    if (!ez) return;
    copy_name(copy, sizeof(copy), name && *name ? name : "OKDialog");
    close_existing(ez, copy);
    dialog(ez, copy, w, h, 0, text, 0x5a, 0x1e);
}

void aoe_ui_ez_yes_no_dialog_ror(AoeUiEzRor *ez, const char *text,
                                 const char *name, int32_t w, int32_t h) {
    if (ez) dialog(ez, name && *name ? name : "YesNoDialog", w, h, 2, text,
                   0x5a, 0x1e);
}

void aoe_ui_ez_yes_no_cancel_dialog_ror(AoeUiEzRor *ez, const char *text,
                                        const char *name, int32_t w,
                                        int32_t h) {
    if (ez) dialog(ez, name && *name ? name : "YesNoCancelDialog", w, h, 4,
                   text, 0x78, 0x19);
}

static const char *string_of(AoeUiEzRor *ez, int32_t id, char *buf, int size) {
    buf[0] = '\0';
    aoe_ui_string_ror(ez->panel.system, id, buf, size);
    return buf;
}

void aoe_ui_ez_help_dialog_id_ror(AoeUiEzRor *ez, int32_t text_id,
                                  const char *name, int32_t w, int32_t h) {
    char text[0x100];
    if (ez) aoe_ui_ez_help_dialog_ror(ez, string_of(ez, text_id, text, sizeof(text)),
                                      name, w, h);
}

void aoe_ui_ez_ok_dialog_id_ror(AoeUiEzRor *ez, int32_t text_id,
                                const char *name, int32_t w, int32_t h) {
    char text[0x100];
    if (ez) aoe_ui_ez_ok_dialog_ror(ez, string_of(ez, text_id, text, sizeof(text)),
                                    name, w, h);
}

void aoe_ui_ez_yes_no_dialog_id_ror(AoeUiEzRor *ez, int32_t text_id,
                                    const char *name, int32_t w, int32_t h) {
    char text[0x100];
    if (ez) aoe_ui_ez_yes_no_dialog_ror(ez, string_of(ez, text_id, text, sizeof(text)),
                                        name, w, h);
}

void aoe_ui_ez_yes_no_cancel_dialog_id_ror(AoeUiEzRor *ez, int32_t text_id,
                                           const char *name, int32_t w,
                                           int32_t h) {
    char text[0x100];
    if (ez)
        aoe_ui_ez_yes_no_cancel_dialog_ror(
            ez, string_of(ez, text_id, text, sizeof(text)), name, w, h);
}

/* ---- help mode ---- */

static void cursor(AoeUiEzRor *ez, int help, int32_t mode) {
    if (ez->ops && ez->ops->cursor) ez->ops->cursor(ez->ops->context, help, mode);
}

static void set_pointer_state(AoeUiEzRor *ez, int32_t state) {
    if (ez->ops && ez->ops->set_pointer_state)
        ez->ops->set_pointer_state(ez->ops->context, state);
}

void aoe_ui_ez_help_mode_on_ror(AoeUiEzRor *ez) {
    if (!ez) return;
    ez->help_armed = 1;
    cursor(ez, 1, 6);
    ez->saved_pointer = ez->ops && ez->ops->pointer_state
                            ? ez->ops->pointer_state(ez->ops->context)
                            : 0;
    set_pointer_state(ez, 0);
}

void aoe_ui_ez_help_mode_off_ror(AoeUiEzRor *ez) {
    if (!ez) return;
    ez->help_armed = 0;
    cursor(ez, 0, 0);
    set_pointer_state(ez, ez->saved_pointer);
}

int32_t aoe_ui_ez_help_press_ror(AoeUiEzRor *ez, uint8_t button, int32_t x,
                                 int32_t y) {
    char text[1000];
    int32_t page = 0;
    if (!ez || ez->help_armed != 1) return 0;
    ez->help_armed = 0;
    cursor(ez, 0, 0);
    set_pointer_state(ez, ez->saved_pointer);
    if (button == 1) {
        text[0] = '\0';
        if (ez->panel.vt->get_help(&ez->panel, text, &page, x, y))
            aoe_ui_ez_help_popup_ror(ez, ez->panel.name, text, page, x, y);
    }
    return 1;
}

/* ---- slots ---- */

void aoe_ui_ez_begin_draw_slot_ror(AoeUiPanelRor *p, int32_t clear) {
    aoe_ui_panel_begin_draw_ror(p, clear);
}

void aoe_ui_ez_draw_slot_ror(AoeUiPanelRor *p) {
    AoeUiEzRor *ez = EZ(p);
    if (p->need_restore) {
        if (ez->shadow_panel) aoe_ui_ez_shadow_area_ror(ez, 1);
        p->need_restore = 0;
    }
    EZ_VT(ez)->draw_background(ez, p->parent_rect ? 1 : 0);
}

int32_t aoe_ui_ez_mouse_down_slot_ror(AoeUiPanelRor *p, uint8_t button, int32_t x,
                             int32_t y, int32_t c, int32_t s) {
    if (aoe_ui_ez_help_press_ror(EZ(p), button, x, y)) return 1;
    return aoe_ui_panel_mouse_down_ror(p, button, x, y, c, s);
}

int32_t aoe_ui_ez_action_slot_ror(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code,
                         int32_t value, int32_t extra) {
    AoeUiEzRor *ez = EZ(p);
    static const struct { const char *name; int32_t codes; } dialogs[] = {
        {"OKDialog", 2}, {"YesNoDialog", 2}, {"YesNoCancelDialog", 3}};
    size_t i;
    if (from) {
        const char *name = aoe_ui_panel_name_ror(from);
        for (i = 0; name && i < sizeof(dialogs) / sizeof(dialogs[0]); ++i) {
            if (strcmp(name, dialogs[i].name) != 0) continue;
            if (code >= 0 && code < dialogs[i].codes)
                aoe_ui_system_close_ror(ez->panel.system, dialogs[i].name);
            return 1;
        }
    }
    return aoe_ui_panel_action_ror(p, from, code, value, extra);
}

void aoe_ui_ez_set_focus_slot_ror(AoeUiPanelRor *p, int32_t focus) {
    AoeUiEzRor *ez = EZ(p);
    const AoeUiEzOpsRor *ops = ez->ops;
    aoe_ui_panel_set_focus_ror(p, focus);
    if (p->has_focus && ops) {
        uintptr_t pal;
        if ((ez->pointer_name[0] || ez->pointer_id >= 0) && ops->has_pointer &&
            ops->has_pointer(ops->context) && ops->set_pointer)
            ops->set_pointer(ops->context, ez->pointer_name, ez->pointer_id);
        pal = ez->palette;
        if (!pal && ops->default_palette) pal = ops->default_palette(ops->context);
        if (p->area && p->area->window && pal && ops->area_palette &&
            ops->area_palette(ops->context, p->area) != pal &&
            ops->set_area_palette) {
            ops->set_area_palette(ops->context, p->area, pal);
            p->vt->set_redraw(p, 2);
        }
    }
    if (ez->ime_toggle && ops && ops->ime) ops->ime(ops->context, p->has_focus != 0);
}

int32_t aoe_ui_ez_no_key_action_ror(AoeUiPanelRor *p, int32_t a, int32_t b,
                             int32_t c, int32_t d, int32_t e) {
    (void)p; (void)a; (void)b; (void)c; (void)d; (void)e;
    return 0;
}

/* Background drawing is not translated. */
void aoe_ui_ez_help_popup_ror(AoeUiEzRor *ez, const char *owner,
                              const char *text, int32_t page, int32_t x,
                              int32_t y) {
    AoeUiHelpDialogRor *d;
    if (!text || !*text) return;
    d = calloc(1, sizeof *d);
    if (!d) return;
    /* Registered by name; closing it frees it. */
    if (!aoe_ui_help_dialog_init_ror(d, ez->panel.system, ez->ops, &ez->panel,
                                     owner, text, page, x, y)) {
        d->dialog.ez.panel.vt->destroy(&d->dialog.ez.panel);
        free(d);
    }
}

static struct AoeDrawSurfaceRor *area_surface(AoeUiEzRor *ez) {
    return ez->panel.area ? ez->panel.area->surface : NULL;
}

static void draw_picture(AoeUiEzRor *ez, void *shape, AoeShapeSurfaceRor *dst,
                         int32_t x, int32_t y) {
    if (shape && ez->ops && ez->ops->shape_ops)
        aoe_shape_draw_ror(shape, dst, x, y, 0, 0, NULL, ez->ops->shape_ops);
}

static int picture_size(void *shape, int32_t *w, int32_t *h) {
    int32_t x0 = 0, y0 = 0, x1 = -1, y1 = -1;
    aoe_shape_bounds_ror(shape, 0, &x0, &y0, &x1, &y1);
    *w = x1 - x0 + 1;
    *h = y1 - y0 + 1;
    return 1;
}

void aoe_ui_ez_shadow_area_ror(AoeUiEzRor *ez, int32_t force) {
    struct AoeDrawSurfaceRor *area = area_surface(ez);
    AoeDrawSystemRor *d = area ? area->system : NULL;
    uint32_t flags;
    int32_t w, h;
    if ((ez->shadow_panel && !force) || !ez->background || !ez->shadow_table ||
        !d)
        return;
    flags = d->flags;
    if (!ez->shadow_panel) {
        char name[0x120];
        d->flags &= 1;
        snprintf(name, sizeof name, "%s Shadow Panel",
                 ez->panel.name ? ez->panel.name : "");
        ez->shadow_panel = malloc(sizeof *ez->shadow_panel);
        if (ez->shadow_panel && !aoe_surface_init_ror(ez->shadow_panel, name, 0)) {
            free(ez->shadow_panel);
            ez->shadow_panel = NULL;
        }
    }
    if (ez->shadow_panel) {
        picture_size(ez->background, &w, &h);
        if (ez->panel.width < w) w = ez->panel.width;
        if (ez->panel.height < h) h = ez->panel.height;
        if (aoe_surface_attach_ror(ez->shadow_panel, d, w, h, 0, 0)) {
            aoe_surface_fill_ror(ez->shadow_panel, NULL, ez->fill_color);
            if (aoe_surface_lock_ror(ez->shadow_panel, 1)) {
                AoeShapeSurfaceRor *v = &ez->shadow_panel->view;
                draw_picture(ez, ez->background, v, 0, 0);
                aoe_draw_set_shade_ror(v, ez->shadow_table);
                aoe_draw_shade_ror(v, 0, 0, w - 1, h - 1);
                aoe_surface_unlock_ror(ez->shadow_panel);
            }
        }
    }
    d->flags = flags;
}

static void fill_panel(AoeUiEzRor *ez, uint8_t color) {
    struct AoeDrawSurfaceRor *area = area_surface(ez);
    int32_t r[4];
    if (!area) return;
    r[0] = ez->panel.clip.left;
    r[1] = ez->panel.clip.top;
    r[2] = ez->panel.clip.right;
    r[3] = ez->panel.clip.bottom;
    aoe_surface_fill_ror(area, r, color);
}

void aoe_ui_ez_draw_background_ror(AoeUiEzRor *ez, int32_t parent_rect) {
    AoeUiPanelRor *p = &ez->panel;
    struct AoeDrawSurfaceRor *area = area_surface(ez);
    void *picture;
    int32_t w, h, x, y;
    p->vt->begin_draw(p, 0);
    if (parent_rect && ez->shadow_panel) {
        if (p->redraw == 2) fill_panel(ez, ez->fill_color2);
        w = ez->shadow_panel->view.width;
        h = ez->shadow_panel->view.height;
        if (area) {
            if (ez->background_mode == 0) {
                aoe_surface_blit_ror(ez->shadow_panel, area, p->x, p->y, NULL, 0);
            } else if (ez->background_mode == 1) {
                aoe_surface_blit_ror(
                    ez->shadow_panel, area,
                    (int16_t)((int16_t)(p->width / 2) - (int16_t)(w / 2) +
                              (int16_t)p->x),
                    (int16_t)((int16_t)(p->height / 2) - (int16_t)(h / 2) +
                              (int16_t)p->y),
                    NULL, 0);
            } else if (ez->background_mode == 2 && p->height >= 0) {
                for (y = 0; y <= p->height; y += h)
                    for (x = 0; p->width >= 0 && x <= p->width; x += w)
                        aoe_surface_blit_ror(ez->shadow_panel, area, p->x + x,
                                             p->y + y, NULL, 0);
            }
        }
        goto bevel;
    }
    picture = parent_rect && ez->background2 ? ez->background2 : ez->background;
    if (p->redraw == 2 || !picture) {
        fill_panel(ez, parent_rect ? ez->fill_color2 : ez->fill_color);
        if (!picture) goto bevel;
    }
    if (area && aoe_surface_lock_ror(area, 1)) {
        AoeShapeSurfaceRor *v = &area->view;
        int shade = parent_rect && ez->shadow_table;
        int32_t sx, sy, sr, sb;
        picture_size(picture, &w, &h);
        if (ez->background_mode == 1) {
            sx = (int16_t)((int16_t)(p->width / 2) - (int16_t)(w / 2) +
                           (int16_t)p->x);
            sy = (int16_t)((int16_t)(p->height / 2) - (int16_t)(h / 2) +
                           (int16_t)p->y);
            draw_picture(ez, picture, v, sx, sy);
            sr = sx - 1 + w;
            sb = sy - 1 + h;
        } else if (ez->background_mode == 2) {
            for (y = 0; p->height >= 0 && y <= p->height; y += h)
                for (x = 0; p->width >= 0 && x <= p->width; x += w)
                    draw_picture(ez, picture, v, p->x + x, p->y + y);
            sx = p->x;
            sy = p->y;
            sr = p->width - 1 + sx;
            sb = p->height - 1 + sy;
        } else {
            draw_picture(ez, picture, v, p->x, p->y);
            sx = p->x;
            sy = p->y;
            sr = sx - 1 + w;
            sb = sy - 1 + h;
        }
        if (shade) {
            aoe_draw_set_shade_ror(v, ez->shadow_table);
            aoe_draw_shade_ror(v, sx, sy, sr, sb);
        }
        aoe_surface_unlock_ror(area);
    }
bevel:
    if (ez->bevel_on && area && aoe_surface_lock_ror(area, 1)) {
        aoe_draw_bevel3_ror(&area->view, p->x, p->y, p->width - 1 + p->x,
                            p->height - 1 + p->y, ez->bevel);
        aoe_surface_unlock_ror(area);
    }
    p->vt->end_draw(p);
}

void aoe_ui_ez_draw_background_slot_ror(AoeUiEzRor *ez, int32_t parent_rect) {
    aoe_ui_ez_draw_background_ror(ez, parent_rect);
}



const AoeUiEzVtblRor aoe_ui_ez_vtable_ror = {
    AOE_UI_EZ_DEFAULT_SLOTS(aoe_ui_ez_destroy_slot_ror, aoe_ui_panel_handle_paint_ror,
                   aoe_ui_panel_zero_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};

/* ---- the screen subclass ---- */

int aoe_ui_screen_init_ror(AoeUiEzRor *ez, AoeUiSystemRor *system,
                           const AoeUiEzOpsRor *ops, const char *name) {
    int ok = aoe_ui_ez_init_named_ror(ez, system, ops, name);
    if (ez) ez->panel.vt = &aoe_ui_screen_vtable_ror.base;
    return ok;
}

int aoe_ui_screen_setup_ror(AoeUiEzRor *ez, AoeUiAreaRor *area,
                            const char *info_name, int32_t info_id,
                            int32_t field_470) {
    return aoe_ui_ez_setup_ror(ez, area, NULL, info_name, info_id, 1, 0, 0, 0,
                               0, field_470) != 0;
}

static int32_t screen_paint(AoeUiPanelRor *p) {
    aoe_ui_panel_handle_paint_ror(p);
    return 0;
}

static int32_t screen_help(AoeUiPanelRor *p) {
    aoe_ui_ez_help_dialog_id_ror(EZ(p), 120055, NULL, 450, 100);
    return 0;
}

static uint32_t screen_clock(void *context) {
    const AoeUiPlatformRor *pf = context;
    return pf->now_ms ? pf->now_ms(pf->context) : 0;
}

/* +0xc4 0x004681c0: a screen gaining focus (other than the last one
   faded in) on a full-screen or memory-surface display fades to black
   over 0.13 s and clears, takes the focus, then paints at full black
   and fades back in from its palette. */
void aoe_ui_screen_focus_slot_ror(AoeUiPanelRor *p, int32_t focus) {
    AoeUiEzRor *ez = EZ(p);
    AoeUiSystemRor *s = p->system;
    struct AoeDrawSurfaceRor *surf = p->area ? p->area->surface : NULL;
    AoeDrawSystemRor *d = surf ? surf->system : NULL;
    int32_t was = p->has_focus;
    uint8_t entries[1024];
    const AoeUiPlatformRor *pf = s ? s->platform : NULL;
    int fade = d && pf && focus && p != s->faded_screen &&
               (d->screen == 2 || d->type == 1);
    if (fade) {
        memset(entries, 0, sizeof entries);
        if (!ez->palette || !d->ops->window_palette ||
            !d->ops->window_palette(d->ops->context, (void *)ez->palette,
                                    entries))
            aoe_surface_palette_ror(surf, entries);
        aoe_palette_fade_ror(d, 0, 0.13f, 1, NULL, -1, -1, screen_clock,
                             (void *)pf);
        aoe_surface_fill_ror(surf, NULL, 0);
        aoe_draw_present_ror(d, NULL);
    }
    aoe_ui_ez_set_focus_slot_ror(p, focus);
    if (fade) {
        aoe_palette_fade_ror(d, 0, 0.0f, 2, entries, -1, -1, screen_clock,
                             (void *)pf);
        p->vt->handle_paint(p);
        aoe_palette_fade_ror(d, 0, 0.0f, 1, NULL, -1, -1, screen_clock,
                             (void *)pf);
        aoe_draw_present_ror(d, NULL);
        aoe_palette_fade_ror(d, 0, 0.13f, 2, entries, -1, -1, screen_clock,
                             (void *)pf);
    }
    if (focus != was && p->has_focus && s) s->faded_screen = p;
}

const AoeUiEzVtblRor aoe_ui_screen_vtable_ror = {
    AOE_UI_EZ_SLOTS(aoe_ui_ez_destroy_slot_ror, aoe_ui_panel_handle_idle_ror,
                    screen_paint, aoe_ui_ez_no_key_action_ror, screen_help,
                    aoe_ui_ez_action_slot_ror, aoe_ui_screen_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
